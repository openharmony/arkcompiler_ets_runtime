/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/ts_types/ts_manager.h"

#include "ecmascript/aot_file_manager.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ts_types/ts_type_table_generator.h"

namespace panda::ecmascript {
TSManager::TSManager(EcmaVM *vm) : vm_(vm), thread_(vm_->GetJSThread()), factory_(vm_->GetFactory()),
                                   assertTypes_(vm_->GetJSOptions().AssertTypes()),
                                   printAnyTypes_(vm_->GetJSOptions().PrintAnyTypes())
{
    JSHandle<TSModuleTable> mTable = factory_->NewTSModuleTable(TSModuleTable::INITIAL_CAPACITY);
    SetTSModuleTable(mTable);
}

void TSManager::Initialize()
{
    TSTypeTableGenerator tableGenerator(this);
    tableGenerator.GenerateDefaultTSTypeTables();
}

void TSManager::RecursivelyMergeClassField(JSHandle<TSClassType> classType)
{
    ASSERT(!classType->GetHasLinked());
    JSHandle<TSClassType> extendClassType = GetExtendClassType(classType);
    if (!extendClassType->GetHasLinked()) {
        RecursivelyMergeClassField(extendClassType);
    }

    ASSERT(extendClassType->GetHasLinked());

    JSHandle<TSObjectType> field(thread_, classType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> layout(thread_, field->GetObjLayoutInfo());
    uint32_t numSelfTypes = layout->GetNumOfProperties();

    JSHandle<TSObjectType> extendField(thread_, extendClassType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> extendLayout(thread_, extendField->GetObjLayoutInfo());
    uint32_t numExtendTypes = extendLayout->GetNumOfProperties();

    uint32_t numTypes = numSelfTypes + numExtendTypes;

    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TSObjLayoutInfo> newLayout = factory->CreateTSObjLayoutInfo(numTypes);

    for (uint32_t index = 0; index < numExtendTypes; index++) {
        JSTaggedValue key = extendLayout->GetKey(index);
        JSTaggedValue type = extendLayout->GetTypeId(index);
        newLayout->AddKeyAndType(thread_, key, type);
    }

    for (uint32_t index = 0; index < numSelfTypes; index++) {
        JSTaggedValue key = layout->GetKey(index);
        if (IsDuplicatedKey(extendLayout, key)) {
            continue;
        }
        JSTaggedValue type = layout->GetTypeId(index);
        newLayout->AddKeyAndType(thread_, key, type);
    }

    field->SetObjLayoutInfo(thread_, newLayout);
    classType->SetHasLinked(true);
}

bool TSManager::IsDuplicatedKey(JSHandle<TSObjLayoutInfo> extendLayout, JSTaggedValue key)
{
    ASSERT_PRINT(key.IsString(), "TS class field key is not a string");
    EcmaString *keyString = EcmaString::Cast(key.GetTaggedObject());

    uint32_t length = extendLayout->GetNumOfProperties();
    for (uint32_t i = 0; i < length; ++i) {
        JSTaggedValue extendKey = extendLayout->GetKey(i);
        ASSERT_PRINT(extendKey.IsString(), "TS class field key is not a string");
        EcmaString *extendKeyString = EcmaString::Cast(extendKey.GetTaggedObject());
        if (EcmaStringAccessor::StringsAreEqual(keyString, extendKeyString)) {
            return true;
        }
    }

    return false;
}

int TSManager::GetHClassIndexByInstanceGateType(const kungfu::GateType &gateType)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    if (!IsClassInstanceTypeKind(gateType)) {
        return -1;
    }
    GlobalTSTypeRef instanceGT = gateType.GetGTRef();
    GlobalTSTypeRef classGT = GetClassType(instanceGT);
    return GetHClassIndex(classGT);
}

int TSManager::GetHClassIndexByClassGateType(const kungfu::GateType &gateType)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    if (!IsClassTypeKind(gateType)) {
        return -1;
    }
    GlobalTSTypeRef classGT = gateType.GetGTRef();
    return GetHClassIndex(classGT);
}

int TSManager::GetHClassIndex(GlobalTSTypeRef classGT)
{
    if (HasOffsetFromGT(classGT)) {
        uint32_t literalOffset = 0;
        CString recordName = "";
        std::tie(recordName, literalOffset) = GetOffsetFromGt(classGT);
        GetCompilationDriver()->AddResolvedMethod(recordName, literalOffset);
    }
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    auto iter = gtIhcMap_.find(classGT);
    if (iter == gtIhcMap_.end()) {
        return -1;
    } else {
        std::unordered_map<int32_t, uint32_t> &cpIndexMap = iter->second.GetCPIndexMap();
        auto indexIter = cpIndexMap.find(curCPID_);
        if (indexIter == cpIndexMap.end()) {
            // This ihc is used in the current constantpool, but has not yet been recorded
            return RecordIhcToVecAndIndexMap(iter->second);
        }
        return indexIter->second;
    }
}

uint32_t TSManager::RecordIhcToVecAndIndexMap(IHClassData &ihcData)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    JSHandle<ConstantPool> constantPool(GetConstantPool());
    CVector<JSTaggedType> &hcVec = snapshotData_.GetSnapshotHCVector(curCPID_);
    hcVec.emplace_back(ihcData.GetIHC());

    uint32_t index = constantPool->GetCacheLength() + hcVec.size() - 1;
    std::unordered_map<int32_t, uint32_t> &cpIndexMap = ihcData.GetCPIndexMap();
    cpIndexMap[curCPID_] = index;

    return index;
}

JSTaggedValue TSManager::GetHClassFromCache(uint32_t index)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    JSHandle<ConstantPool> constantPool(GetConstantPool());
    const CVector<JSTaggedType> &hcVec = snapshotData_.GetSnapshotHCVector(curCPID_);
    return JSTaggedValue(hcVec[index - constantPool->GetCacheLength()]);
}

int TSManager::GetPropertyOffset(JSTaggedValue hclass, JSTaggedValue key)
{
    JSHClass *hc = JSHClass::Cast(hclass.GetTaggedObject());
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hc->GetLayout().GetTaggedObject());
    uint32_t propsNumber = hc->NumberOfProps();
    int entry = layoutInfo->FindElementWithCache(thread_, hc, key, propsNumber);
    if (entry == -1) {
        return entry;
    }

    int offset = hc->GetInlinedPropertiesOffset(entry);
    return offset;
}


JSHandle<TSClassType> TSManager::GetExtendClassType(JSHandle<TSClassType> classType) const
{
    ASSERT(classType.GetTaggedValue().IsTSClassType());
    // Get extended type of classType based on ExtensionGT
    GlobalTSTypeRef extensionGT = classType->GetExtensionGT();
    JSHandle<JSTaggedValue> extendClassType = GetTSType(extensionGT);

    ASSERT(extendClassType->IsTSClassType());
    return JSHandle<TSClassType>(extendClassType);
}

GlobalTSTypeRef TSManager::GetPropType(GlobalTSTypeRef gt, JSHandle<EcmaString> propertyName) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> type = GetTSType(gt);
    ASSERT(type->IsTSType());

    if (type->IsTSClassType()) {
        JSHandle<TSClassType> classType(type);
        return TSClassType::GetPropTypeGT(thread, classType, propertyName);
    } else if (type->IsTSClassInstanceType()) {
        JSHandle<TSClassInstanceType> classInstanceType(type);
        return TSClassInstanceType::GetPropTypeGT(thread, classInstanceType, propertyName);
    } else if (type->IsTSObjectType()) {
        JSHandle<TSObjectType> objectType(type);
        return TSObjectType::GetPropTypeGT(objectType, propertyName);
    } else if (type->IsTSIteratorInstanceType()) {
        JSHandle<TSIteratorInstanceType> iteratorInstance(type);
        return TSIteratorInstanceType::GetPropTypeGT(thread, iteratorInstance, propertyName);
    } else if (type->IsTSInterfaceType()) {
        JSHandle<TSInterfaceType> objectType(type);
        return TSInterfaceType::GetPropTypeGT(thread, objectType, propertyName);
    } else {
        LOG_COMPILER(ERROR) << "unsupport TSType GetPropType: "
                            << static_cast<uint8_t>(type->GetTaggedObject()->GetClass()->GetObjectType());
        return GlobalTSTypeRef::Default();
    }
}

bool TSManager::IsStaticFunc(GlobalTSTypeRef gt) const
{
    ASSERT(IsFunctionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetStatic();
}

GlobalTSTypeRef TSManager::GetSuperPropType(GlobalTSTypeRef gt, JSHandle<EcmaString> propertyName,
                                            PropertyType propType) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> type = GetTSType(gt);
    if (type->IsTSClassType()) {
        JSHandle<TSClassType> classType(type);
        return TSClassType::GetSuperPropTypeGT(thread, classType, propertyName, propType);
    } else {
        UNREACHABLE();
    }
}

GlobalTSTypeRef TSManager::GetSuperPropType(GlobalTSTypeRef gt, const uint64_t key, PropertyType propType) const
{
    JSTaggedValue keyValue = JSTaggedValue(key);
    JSMutableHandle<EcmaString> propertyName(thread_, JSTaggedValue::Undefined());
    if (keyValue.IsInt()) {
        propertyName.Update(factory_->NewFromStdString(std::to_string(keyValue.GetInt())));
    } else if (keyValue.IsDouble()) {
        propertyName.Update(factory_->NewFromStdString(std::to_string(keyValue.GetDouble())));
    } else {
        propertyName.Update(factory_->NewFromStdString(std::to_string(key).c_str()));
    }
    return GetSuperPropType(gt, propertyName, propType);
}

GlobalTSTypeRef TSManager::GetPropType(GlobalTSTypeRef gt, const uint64_t key) const
{
    JSTaggedValue keyValue = JSTaggedValue(key);
    JSMutableHandle<EcmaString> propertyName(thread_, JSTaggedValue::Undefined());
    if (keyValue.IsInt()) {
        propertyName.Update(factory_->NewFromStdString(std::to_string(keyValue.GetInt())));
    } else if (keyValue.IsDouble()) {
        propertyName.Update(factory_->NewFromStdString(std::to_string(keyValue.GetDouble())));
    } else {
        propertyName.Update(factory_->NewFromStdString(std::to_string(key).c_str()));
    }
    return GetPropType(gt, propertyName);
}

uint32_t TSManager::GetUnionTypeLength(GlobalTSTypeRef gt) const
{
    ASSERT(IsUnionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSUnionType());
    JSHandle<TSUnionType> unionType = JSHandle<TSUnionType>(tsType);
    JSHandle<TaggedArray> unionTypeArray(thread_, unionType->GetComponents());
    return unionTypeArray->GetLength();
}

GlobalTSTypeRef TSManager::GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const
{
    ASSERT(IsUnionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSUnionType());
    JSHandle<TSUnionType> unionType = JSHandle<TSUnionType>(tsType);
    JSHandle<TaggedArray> unionTypeArray(thread_, unionType->GetComponents());
    uint32_t typeRawData = unionTypeArray->Get(index).GetInt();
    return GlobalTSTypeRef(typeRawData);
}

TSTypeKind TSManager::GetTypeKind(const GlobalTSTypeRef &gt) const
{
    uint32_t moduleId = gt.GetModuleId();
    if (moduleId != static_cast<uint32_t>(MTableIdx::PRIMITIVE)) {
        JSHandle<JSTaggedValue> type = GetTSType(gt);
        if (type->IsTSType()) {
            JSHandle<TSType> tsType(type);
            JSType hClassType = tsType->GetClass()->GetObjectType();
            switch (hClassType) {
                case JSType::TS_CLASS_TYPE:
                    return TSTypeKind::CLASS;
                case JSType::TS_CLASS_INSTANCE_TYPE:
                    return TSTypeKind::CLASS_INSTANCE;
                case JSType::TS_FUNCTION_TYPE:
                    return TSTypeKind::FUNCTION;
                case JSType::TS_UNION_TYPE:
                    return TSTypeKind::UNION;
                case JSType::TS_ARRAY_TYPE:
                    return TSTypeKind::ARRAY;
                case JSType::TS_OBJECT_TYPE:
                    return TSTypeKind::OBJECT;
                case JSType::TS_INTERFACE_TYPE:
                    return TSTypeKind::INTERFACE_KIND;
                case JSType::TS_ITERATOR_INSTANCE_TYPE:
                    return TSTypeKind::ITERATOR_INSTANCE;
                default:
                    UNREACHABLE();
            }
        } else {
            return TSTypeKind::UNKNOWN;
        }
    }
    return TSTypeKind::PRIMITIVE;
}

void TSManager::Dump()
{
    std::cout << "TSTypeTables:";
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    uint32_t GTLength = table->GetLength();
    for (uint32_t i = 0; i < GTLength; i++) {
        JSHandle<JSTaggedValue>(thread_, table->Get(i))->Dump(std::cout);
    }
}

GlobalTSTypeRef TSManager::GetOrCreateTSIteratorInstanceType(TSRuntimeType runtimeType, GlobalTSTypeRef elementGt)
{
    ASSERT((runtimeType >= TSRuntimeType::ITERATOR_RESULT) && (runtimeType <= TSRuntimeType::ITERATOR));
    GlobalTSTypeRef kindGT = GlobalTSTypeRef(TSModuleTable::RUNTIME_TABLE_ID, static_cast<int>(runtimeType));
    GlobalTSTypeRef foundTypeRef = FindIteratorInstanceInInferTable(kindGT, elementGt);
    if (!foundTypeRef.IsDefault()) {
        return foundTypeRef;
    }

    JSHandle<TSIteratorInstanceType> iteratorInstanceType = factory_->NewTSIteratorInstanceType();
    iteratorInstanceType->SetKindGT(kindGT);
    iteratorInstanceType->SetElementGT(elementGt);

    return AddTSTypeToTypeTable(JSHandle<TSType>(iteratorInstanceType), TSModuleTable::INFER_TABLE_ID);
}

GlobalTSTypeRef TSManager::GetIteratorInstanceElementGt(GlobalTSTypeRef gt) const
{
    ASSERT(IsIteratorInstanceTypeKind(gt));
    JSHandle<JSTaggedValue> type = GetTSType(gt);
    ASSERT(type->IsTSIteratorInstanceType());
    JSHandle<TSIteratorInstanceType> iteratorFuncInstance(type);
    GlobalTSTypeRef elementGT = iteratorFuncInstance->GetElementGT();
    return elementGT;
}

GlobalTSTypeRef TSManager::FindIteratorInstanceInInferTable(GlobalTSTypeRef kindGt, GlobalTSTypeRef elementGt) const
{
    DISALLOW_GARBAGE_COLLECTION;

    JSHandle<TSTypeTable> table = GetTSTypeTable(TSModuleTable::INFER_TABLE_ID);

    for (int index = 1; index <= table->GetNumberOfTypes(); ++index) {  // index 0 reseved for num of types
        JSTaggedValue type = table->Get(index);
        if (!type.IsTSIteratorInstanceType()) {
            continue;
        }

        TSIteratorInstanceType *insType = TSIteratorInstanceType::Cast(type.GetTaggedObject());
        if (insType->GetKindGT() == kindGt && insType->GetElementGT() == elementGt) {
            return insType->GetGT();
        }
    }

    return GlobalTSTypeRef::Default();  // not found
}

GlobalTSTypeRef TSManager::AddTSTypeToTypeTable(const JSHandle<TSType> &type, int tableId) const
{
    JSHandle<TSTypeTable> iTable = GetTSTypeTable(tableId);
    if (UNLIKELY(!GlobalTSTypeRef::IsVaildLocalId(iTable->GetNumberOfTypes() + 1))) {
        LOG_COMPILER(DEBUG) << "The maximum number of TSTypes in TSTypeTable " << tableId << " is reached. ";
        return GlobalTSTypeRef::Default();
    }

    JSHandle<TSTypeTable> newITable = TSTypeTable::PushBackTypeToTable(thread_, iTable, type);
    SetTSTypeTable(newITable, tableId);

    GlobalTSTypeRef gt = GlobalTSTypeRef(tableId, newITable->GetNumberOfTypes());
    type->SetGT(gt);
    return gt;
}

GlobalTSTypeRef TSManager::FindUnionInTypeTable(JSHandle<TSTypeTable> table, JSHandle<TSUnionType> unionType) const
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(unionType.GetTaggedValue().IsTSUnionType());

    for (int index = 1; index <= table->GetNumberOfTypes(); ++index) {  // index 0 reseved for num of types
        JSTaggedValue type = table->Get(index);
        if (!type.IsTSUnionType()) {
            continue;
        }

        TSUnionType *uType = TSUnionType::Cast(type.GetTaggedObject());
        if (uType->IsEqual(unionType)) {
            return uType->GetGT();
        }
    }

    return GlobalTSTypeRef::Default();  // not found
}

GlobalTSTypeRef TSManager::GetOrCreateUnionType(CVector<GlobalTSTypeRef> unionTypeVec)
{
    uint32_t length = unionTypeVec.size();
    JSHandle<TSUnionType> unionType = factory_->NewTSUnionType(length);
    JSHandle<TaggedArray> components(thread_, unionType->GetComponents());
    for (uint32_t unionArgIndex = 0; unionArgIndex < length; unionArgIndex++) {
        components->Set(thread_, unionArgIndex, JSTaggedValue(unionTypeVec[unionArgIndex].GetType()));
    }
    unionType->SetComponents(thread_, components);

    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    uint32_t numOfTables = mTable->GetNumberOfTSTypeTables();
    for (uint32_t tableIndex = 0; tableIndex < numOfTables; ++tableIndex) {
        JSHandle<TSTypeTable> typeTable = GetTSTypeTable(tableIndex);
        GlobalTSTypeRef foundUnionRef = FindUnionInTypeTable(typeTable, unionType);
        if (!foundUnionRef.IsDefault()) {
            return foundUnionRef;
        }
    }

    return AddTSTypeToTypeTable(JSHandle<TSType>(unionType), TSModuleTable::INFER_TABLE_ID);
}

void TSManager::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalModuleTable_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&curCP_)));
    snapshotData_.Iterate(v);
    for (auto iter : gtIhcMap_) {
        iter.second.Iterate(v);
    }
}

JSHandle<TSTypeTable> TSManager::GetTSTypeTable(int entry) const
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    uint32_t tableOffset = TSModuleTable::GetTSTypeTableOffset(entry);
    JSHandle<TSTypeTable> typeTable(thread_, mTable->Get(tableOffset));
    return typeTable;
}

void TSManager::SetTSTypeTable(const JSHandle<TSTypeTable> &table, int tableId) const
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    uint32_t tableOffset = TSModuleTable::GetTSTypeTableOffset(tableId);
    mTable->Set(thread_, tableOffset, table);
}

std::string TSManager::GetFuncName(kungfu::GateType type) const
{
    GlobalTSTypeRef gt = type.GetGTRef();
    ASSERT(IsFunctionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    auto name = functionType->GetName();
    EcmaStringAccessor acc(name);
    std::string nameStr = acc.ToStdString();
    return nameStr;
}

uint32_t TSManager::GetFunctionTypeLength(GlobalTSTypeRef gt) const
{
    ASSERT(IsFunctionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    return functionType->GetLength();
}

GlobalTSTypeRef TSManager::GetFuncParameterTypeGT(GlobalTSTypeRef gt, int index) const
{
    ASSERT(IsFunctionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    return functionType->GetParameterTypeGT(index);
}

GlobalTSTypeRef TSManager::GetFuncThisGT(GlobalTSTypeRef gt) const
{
    ASSERT(IsFunctionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetThisGT();
}

bool TSManager::IsGetterSetterFunc(GlobalTSTypeRef gt) const
{
    if (!IsFunctionTypeKind(gt)) {
        return false;
    }
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetIsGetterSetter();
}

bool TSManager::IsAbstractMethod(GlobalTSTypeRef gt) const
{
    if (!IsFunctionTypeKind(gt)) {
        return false;
    }
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetIsAbstract();
}

bool TSManager::IsMethodSignature(GlobalTSTypeRef gt) const
{
    if (!IsFunctionTypeKind(gt)) {
        return false;
    }
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetIsSignature();
}

GlobalTSTypeRef TSManager::GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const
{
    ASSERT(IsFunctionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    return functionType->GetReturnGT();
}

void TSManager::SetFuncMethodOffset(GlobalTSTypeRef gt, uint32_t methodIndex)
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    functionType->SetMethodOffset(methodIndex);
}

uint32_t TSManager::GetFuncMethodOffset(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    return functionType->GetMethodOffset();
}

GlobalTSTypeRef TSManager::CreateClassInstanceType(GlobalTSTypeRef gt)
{
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    // handle buintin types if builtins.dts is not enabled
    if (tsType->IsUndefined()) {
        return GlobalTSTypeRef::Default();
    }

    ASSERT(tsType->IsTSClassType());
    JSHandle<TSClassInstanceType> classInstanceType = factory_->NewTSClassInstanceType();
    classInstanceType->SetClassGT(gt);
    return AddTSTypeToTypeTable(JSHandle<TSType>(classInstanceType), TSModuleTable::INFER_TABLE_ID);
}

GlobalTSTypeRef TSManager::GetClassType(GlobalTSTypeRef classInstanceGT) const
{
    ASSERT(IsClassInstanceTypeKind(classInstanceGT));
    JSHandle<JSTaggedValue> tsType = GetTSType(classInstanceGT);
    ASSERT(tsType->IsTSClassInstanceType());
    JSHandle<TSClassInstanceType> instanceType(tsType);
    return instanceType->GetClassGT();
}

GlobalTSTypeRef TSManager::GetArrayParameterTypeGT(GlobalTSTypeRef gt) const
{
    ASSERT(IsArrayTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSArrayType());
    JSHandle<TSArrayType> arrayType = JSHandle<TSArrayType>(tsType);
    return arrayType->GetElementGT();
}

void TSManager::GenerateTSHClass(JSHandle<TSClassType> classType)
{
    JSHandle<TSObjectType> instanceType(thread_, classType->GetInstanceType());
    JSHClass *ihc = TSObjectType::GetOrCreateHClass(thread_, instanceType, TSObjectTypeKind::INSTANCE);
    JSHandle<TSObjectType> prototypeType(thread_, classType->GetPrototypeType());
    JSHClass *phc = TSObjectType::GetOrCreateHClass(thread_, prototypeType, TSObjectTypeKind::PROTOTYPE);
    JSHandle<JSHClass> phcHandle(thread_, JSTaggedValue(phc));
    JSHandle<JSObject> prototype = factory_->NewJSObject(phcHandle);
    ihc->SetProto(thread_, prototype);

    GlobalTSTypeRef gt = classType->GetGT();
    gtIhcMap_.insert({gt, IHClassData(JSTaggedValue(ihc).GetRawData())});
}

void TSManager::GenerateTSHClasses()
{
    for (const auto &gt : collectedTypeOffsets_) {
        JSHandle<JSTaggedValue> tsType = GetTSType(gt);
        if (tsType->IsUndefined()) {
            continue;
        }
        ASSERT(tsType->IsTSClassType());
        JSHandle<TSClassType> classType(tsType);
        if (!classType->GetHasLinked()) {
            RecursivelyMergeClassField(classType);
        }
        if (IsUserDefinedClassTypeKind(gt)) {
            GenerateTSHClass(classType);
        }
    }
    collectedTypeOffsets_.clear();
}

JSHandle<JSTaggedValue> TSManager::GetTSType(const GlobalTSTypeRef &gt) const
{
    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();

    if ((moduleId == TSModuleTable::BUILTINS_TABLE_ID && !IsBuiltinsDTSEnabled()) ||
        (moduleId == TSModuleTable::PRIMITIVE_TABLE_ID)) {
        return thread_->GlobalConstants()->GetHandledUndefined();
    }

    JSHandle<TSTypeTable> typeTable = GetTSTypeTable(moduleId);
    JSHandle<JSTaggedValue> tsType(thread_, typeTable->Get(localId));
    return tsType;
}

bool TSManager::IsBuiltinArrayType(kungfu::GateType gateType) const
{
    if (!IsClassInstanceTypeKind(gateType)) {
        return false;
    }
    const GlobalTSTypeRef gateGT = GlobalTSTypeRef(gateType.Value());
    GlobalTSTypeRef classGT = GetClassType(gateGT);
    if (IsBuiltinsDTSEnabled()) {
        uint32_t idx = static_cast<uint32_t>(BuiltinTypeId::ARRAY);
        const JSPandaFile *builtinPandaFile = GetBuiltinPandaFile();
        uint32_t arrayOffset = GetBuiltinOffset(idx);
        bool hasCreatedGT = HasCreatedGT(builtinPandaFile, arrayOffset);
        if (hasCreatedGT) {
            auto gt = GetGTFromOffset(builtinPandaFile, arrayOffset);
            return (gt == classGT);
        }
    }
    uint32_t m = classGT.GetModuleId();
    uint32_t l = classGT.GetLocalId();
    return (m == TSModuleTable::BUILTINS_TABLE_ID) &&
           (l == static_cast<uint32_t>(BuiltinTypeId::ARRAY));
}

bool TSManager::IsTypedArrayType(kungfu::GateType gateType) const
{
    if (!IsClassInstanceTypeKind(gateType)) {
        return false;
    }
    const GlobalTSTypeRef gateGT = GlobalTSTypeRef(gateType.Value());
    GlobalTSTypeRef classGT = GetClassType(gateGT);
    if (IsBuiltinsDTSEnabled()) {
        for (uint32_t i = static_cast<uint32_t>(BuiltinTypeId::TYPED_ARRAY_FIRST);
             i <= static_cast<uint32_t>(BuiltinTypeId::TYPED_ARRAY_LAST); i++) {
            bool hasCreatedGT = HasCreatedGT(GetBuiltinPandaFile(), GetBuiltinOffset(i));
            if (hasCreatedGT && (GetGTFromOffset(GetBuiltinPandaFile(), GetBuiltinOffset(i)) == classGT)) {
                return true;
            }
        }
        return false;
    }
    uint32_t m = classGT.GetModuleId();
    uint32_t l = classGT.GetLocalId();
    return (m == TSModuleTable::BUILTINS_TABLE_ID) &&
           (l >= static_cast<uint32_t>(BuiltinTypeId::TYPED_ARRAY_FIRST)) &&
           (l <= static_cast<uint32_t>(BuiltinTypeId::TYPED_ARRAY_LAST));
}

bool TSManager::IsFloat32ArrayType(kungfu::GateType gateType) const
{
    if (!IsClassInstanceTypeKind(gateType)) {
        return false;
    }
    const GlobalTSTypeRef gateGT = GlobalTSTypeRef(gateType.Value());
    GlobalTSTypeRef classGT = GetClassType(gateGT);
    if (IsBuiltinsDTSEnabled()) {
        uint32_t idx = static_cast<uint32_t>(BuiltinTypeId::FLOAT32_ARRAY);
        return (HasCreatedGT(GetBuiltinPandaFile(), GetBuiltinOffset(idx))) &&
               (GetGTFromOffset(GetBuiltinPandaFile(), GetBuiltinOffset(idx)) == classGT);
    }
    uint32_t m = classGT.GetModuleId();
    uint32_t l = classGT.GetLocalId();
    return (m == TSModuleTable::BUILTINS_TABLE_ID) &&
           (l == static_cast<uint32_t>(BuiltinTypeId::FLOAT32_ARRAY));
}

std::string TSManager::GetBuiltinsName(uint32_t index) const
{
    ASSERT(index >= static_cast<uint32_t>(BuiltinTypeId::FUNCTION) &&
           index <=static_cast<uint32_t>(BuiltinTypeId::INTL));
    BuiltinTypeId typeId = static_cast<BuiltinTypeId>(index);
    switch (typeId) {
        case BuiltinTypeId::FUNCTION:
            return "Function";
        case BuiltinTypeId::RANGE_ERROR:
            return "RangeError";
        case BuiltinTypeId::ERROR:
            return "Error";
        case BuiltinTypeId::OBJECT:
            return "Object";
        case BuiltinTypeId::SYNTAX_ERROR:
            return "SyntaxError";
        case BuiltinTypeId::TYPE_ERROR:
            return "TypeError";
        case BuiltinTypeId::REFERENCE_ERROR:
            return "ReferenceError";
        case BuiltinTypeId::URI_ERROR:
            return "URIError";
        case BuiltinTypeId::SYMBOL:
            return "Symbol";
        case BuiltinTypeId::EVAL_ERROR:
            return "EvalError";
        case BuiltinTypeId::NUMBER:
            return "Number";
        case BuiltinTypeId::PARSE_FLOAT:
            return "parseFloat";
        case BuiltinTypeId::DATE:
            return "Date";
        case BuiltinTypeId::BOOLEAN:
            return "Boolean";
        case BuiltinTypeId::BIG_INT:
            return "BigInt";
        case BuiltinTypeId::PARSE_INT:
            return "parseInt";
        case BuiltinTypeId::WEAK_MAP:
            return "WeakMap";
        case BuiltinTypeId::REG_EXP:
            return "RegExp";
        case BuiltinTypeId::SET:
            return "Set";
        case BuiltinTypeId::MAP:
            return "Map";
        case BuiltinTypeId::WEAK_REF:
            return "WeakRef";
        case BuiltinTypeId::WEAK_SET:
            return "WeakSet";
        case BuiltinTypeId::FINALIZATION_REGISTRY:
            return "FinalizationRegistry";
        case BuiltinTypeId::ARRAY:
            return "Array";
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            return "Uint8ClampedArray";
        case BuiltinTypeId::UINT8_ARRAY:
            return "Uint8Array";
        case BuiltinTypeId::TYPED_ARRAY:
            return "TypedArray";
        case BuiltinTypeId::INT8_ARRAY:
            return "Int8Array";
        case BuiltinTypeId::UINT16_ARRAY:
            return "Uint16Array";
        case BuiltinTypeId::UINT32_ARRAY:
            return "Uint32Array";
        case BuiltinTypeId::INT16_ARRAY:
            return "Int16Array";
        case BuiltinTypeId::INT32_ARRAY:
            return "Int32Array";
        case BuiltinTypeId::FLOAT32_ARRAY:
            return "Float32Array";
        case BuiltinTypeId::FLOAT64_ARRAY:
            return "Float64Array";
        case BuiltinTypeId::BIG_INT64_ARRAY:
            return "BigInt64Array";
        case BuiltinTypeId::BIG_UINT64_ARRAY:
            return "BigUint64Array";
        case BuiltinTypeId::SHARED_ARRAY_BUFFER:
            return "SharedArrayBuffer";
        case BuiltinTypeId::DATA_VIEW:
            return "DataView";
        case BuiltinTypeId::STRING:
            return "String";
        case BuiltinTypeId::ARRAY_BUFFER:
            return "ArrayBuffer";
        case BuiltinTypeId::EVAL:
            return "eval";
        case BuiltinTypeId::IS_FINITE:
            return "isFinite";
        case BuiltinTypeId::ARK_PRIVATE:
            return "ArkPrivate";
        case BuiltinTypeId::PRINT:
            return "print";
        case BuiltinTypeId::DECODE_URI:
            return "decodeURI";
        case BuiltinTypeId::DECODE_URI_COMPONENT:
            return "decodeURIComponent";
        case BuiltinTypeId::IS_NAN:
            return "isNaN";
        case BuiltinTypeId::ENCODE_URI:
            return "encodeURI";
        case BuiltinTypeId::JS_NAN:
            return "NaN";
        case BuiltinTypeId::GLOBAL_THIS:
            return "globalThis";
        case BuiltinTypeId::ENCODE_URI_COMPONENT:
            return "encodeURIComponent";
        case BuiltinTypeId::JS_INFINITY:
            return "Infinity";
        case BuiltinTypeId::MATH:
            return "Math";
        case BuiltinTypeId::JSON:
            return "JSON";
        case BuiltinTypeId::ATOMICS:
            return "Atomics";
        case BuiltinTypeId::UNDEFINED:
            return "undefined";
        case BuiltinTypeId::REFLECT:
            return "Reflect";
        case BuiltinTypeId::PROMISE:
            return "Promise";
        case BuiltinTypeId::PROXY:
            return "Proxy";
        case BuiltinTypeId::GENERATOR_FUNCTION:
            return "GeneratorFunction";
        case BuiltinTypeId::INTL:
            return "Intl";
        default:
            UNREACHABLE();
    }
}

uint32_t TSManager::GetBuiltinIndex(GlobalTSTypeRef builtinGT) const
{
    ASSERT(builtinGT.GetModuleId() == TSModuleTable::BUILTINS_TABLE_ID);
    if (IsBuiltinsDTSEnabled()) {
        for (uint32_t idx = static_cast<uint32_t>(BuiltinTypeId::FUNCTION);
            idx <= static_cast<uint32_t>(BuiltinTypeId::INTL); idx++) {
            if ((HasCreatedGT(GetBuiltinPandaFile(), GetBuiltinOffset(idx))) &&
               (GetGTFromOffset(GetBuiltinPandaFile(), GetBuiltinOffset(idx)) == builtinGT)) {
                   return idx;
               }
        }
    }
    return builtinGT.GetLocalId();
}

std::string TSManager::GetClassTypeStr(GlobalTSTypeRef gt) const
{
    if (gt.GetModuleId() == TSModuleTable::BUILTINS_TABLE_ID) {
        uint32_t index = GetBuiltinIndex(gt);
        return GetBuiltinsName(index);
    }

    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSClassType());
    JSHandle<TSClassType> classType = JSHandle<TSClassType>(tsType);
    JSTaggedValue taggedValue = classType->GetName();
    EcmaStringAccessor accessor(taggedValue);
    return accessor.ToStdString();
}

std::string TSManager::GetClassInstanceTypeStr(GlobalTSTypeRef gt) const
{
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSClassInstanceType());
    JSHandle<TSClassInstanceType> classInstance(tsType);
    return GetClassTypeStr(classInstance->GetClassGT());
}

std::string TSManager::GetFunctionTypeStr(GlobalTSTypeRef gt) const
{
    std::string functionStr = "(";
    uint32_t parameterLength = GetFunctionTypeLength(gt);
    for (uint32_t i = 0; i < parameterLength; i++) {
        GlobalTSTypeRef parameterGT = GetFuncParameterTypeGT(gt, i);
        std::string parameterStr = GetTypeStr(kungfu::GateType(parameterGT));
        if (i != parameterLength - 1) {
            functionStr = functionStr + parameterStr + ", ";
        } else {
            functionStr = functionStr + parameterStr;
        }
    }
    GlobalTSTypeRef returnGT = GetFuncReturnValueTypeGT(gt);
    std::string returnStr = GetTypeStr(kungfu::GateType(returnGT));
    functionStr = functionStr + ") => " + returnStr;
    return functionStr;
}

std::string TSManager::GetArrayTypeStr(GlobalTSTypeRef gt) const
{
    GlobalTSTypeRef elementGt = GetArrayParameterTypeGT(gt);
    std::string arrayStr = GetTypeStr(kungfu::GateType(elementGt)) + "[]";
    return arrayStr;
}

std::string TSManager::GetTypeStr(kungfu::GateType gateType) const
{
    GlobalTSTypeRef gt = gateType.GetGTRef();
    auto typeKind = GetTypeKind(gt);
    switch (typeKind) {
        case TSTypeKind::PRIMITIVE:
            return GetPrimitiveStr(gt);
        case TSTypeKind::CLASS:
            return "typeof " + GetClassTypeStr(gt);
        case TSTypeKind::CLASS_INSTANCE:
            return GetClassInstanceTypeStr(gt);
        case TSTypeKind::FUNCTION:
            return GetFunctionTypeStr(gt);
        case TSTypeKind::UNION:
            return "union";
        case TSTypeKind::ARRAY:
            return GetArrayTypeStr(gt);
        case TSTypeKind::OBJECT:
            return "object";
        case TSTypeKind::IMPORT:
            return "import";
        case TSTypeKind::INTERFACE_KIND:
            return "interface";
        case TSTypeKind::ITERATOR_INSTANCE:
            return "iterator_instance";
        case TSTypeKind::UNKNOWN:
            return "unknown";
        default:
            UNREACHABLE();
    }
}

std::string TSManager::GetPrimitiveStr(const GlobalTSTypeRef &gt) const
{
    auto primitive = static_cast<TSPrimitiveType>(gt.GetLocalId());
    switch (primitive) {
        case TSPrimitiveType::ANY:
            return "any";
        case TSPrimitiveType::NUMBER:
            return "number";
        case TSPrimitiveType::BOOLEAN:
            return "boolean";
        case TSPrimitiveType::VOID_TYPE:
            return "void";
        case TSPrimitiveType::STRING:
            return "string";
        case TSPrimitiveType::SYMBOL:
            return "symbol";
        case TSPrimitiveType::NULL_TYPE:
            return "null";
        case TSPrimitiveType::UNDEFINED:
            return "undefined";
        case TSPrimitiveType::INT:
            return "int";
        case TSPrimitiveType::DOUBLE:
            return "double";
        case TSPrimitiveType::BIG_INT:
            return "bigint";
        default:
            UNREACHABLE();
    }
}

void TSManager::SetCurConstantPool(const JSPandaFile *jsPandaFile, uint32_t methodOffset)
{
    curCPID_ = GetOldConstantPoolIDByMethodOffset(jsPandaFile, methodOffset);
    curCP_ = vm_->FindConstpool(jsPandaFile, curCPID_);
}

int32_t TSManager::GetOldConstantPoolIDByMethodOffset(const JSPandaFile *jsPandaFile, uint32_t methodOffset)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(),
                                            panda_file::File::EntityId(methodOffset));
    return static_cast<int32_t>(indexAccessor.GetHeaderIndex());
}

JSHandle<ConstantPool> TSManager::GetSnapshotConstantPool(uint32_t cpListIndex)
{
    JSHandle<TaggedArray> snapshotCPList = JSHandle<TaggedArray>(thread_, snapshotData_.GetSnapshotCPList());
    return JSHandle<ConstantPool>(thread_, snapshotCPList->Get(cpListIndex));
}

void TSManager::ProcessSnapshotConstantPool(kungfu::BytecodeInfoCollector *bcInfoCollector)
{
    const CMap<int32_t, JSTaggedValue> &oldCPValues = vm_->FindConstpools(
        bcInfoCollector->GetJSPandaFile()).value();
    std::map<int32_t, uint32_t> cpListIndexMap;

    GenerateSnapshotConstantPoolList(cpListIndexMap, oldCPValues);
    FillSnapshotConstantPoolList(cpListIndexMap, bcInfoCollector);
    AddHClassToSnapshotConstantPoolList(cpListIndexMap, bcInfoCollector);
}

void TSManager::GenerateSnapshotConstantPoolList(std::map<int32_t, uint32_t> &cpListIndexMap,
                                                 const CMap<int32_t, JSTaggedValue> &oldCPValues)
{
    // 2: each item need store (constantPoolID, constantpool)
    JSHandle<TaggedArray> snapshotCPList = factory_->NewTaggedArray(oldCPValues.size() *
                                                                    SnapshotData::SNAPSHOT_CP_LIST_ITEM_SIZE);
    snapshotData_.SetSnapshotCPList(snapshotCPList.GetTaggedValue());

    JSMutableHandle<ConstantPool> oldCP(thread_, thread_->GlobalConstants()->GetUndefined());
    uint32_t pos = 0;
    for (auto &iter : oldCPValues) {
        int32_t oldCPID = iter.first;
        oldCP.Update(iter.second);
        uint32_t cpSize = oldCP->GetCacheLength();
        const CVector<JSTaggedType> &hcVec = snapshotData_.GetSnapshotHCVector(oldCPID);
        uint32_t hcVecSize = hcVec.size();
        if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
            LOG_COMPILER(INFO) << "[aot-snapshot] constantPoolID: " << oldCPID;
            LOG_COMPILER(INFO) << "[aot-snapshot] constantPoolSize: " << cpSize;
            LOG_COMPILER(INFO) << "[aot-snapshot] hclassSize: " << hcVecSize;
        }
        JSHandle<ConstantPool> newCp = factory_->NewConstantPool(cpSize + hcVecSize);

        snapshotCPList->Set(thread_, pos++, JSTaggedValue(oldCPID));
        cpListIndexMap[oldCPID] = pos;
        snapshotCPList->Set(thread_, pos++, newCp.GetTaggedValue());
    }
}

void TSManager::FillSnapshotConstantPoolList(const std::map<int32_t, uint32_t> &cpListIndexMap,
                                             kungfu::BytecodeInfoCollector *bcInfoCollector)
{
    const JSPandaFile *jsPandaFile = bcInfoCollector->GetJSPandaFile();

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::STRING,
        [this, jsPandaFile, &cpListIndexMap] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetOldConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSTaggedValue oldCP = vm_->FindConstpool(jsPandaFile, oldCPID);

        JSTaggedValue str = ConstantPool::GetStringFromCache(thread_, oldCP, data.index);

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        newCP->SetObjectToCache(thread_, data.index, str);
    });

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::METHOD,
        [this, jsPandaFile, &cpListIndexMap, bcInfoCollector] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetOldConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSHandle<ConstantPool> oldCP(thread_, vm_->FindConstpool(jsPandaFile, oldCPID));

        panda_file::File::IndexHeader *indexHeader = oldCP->GetIndexHeader();
        Span<const panda_file::File::EntityId> indexs = jsPandaFile->GetPandaFile()->GetMethodIndex(indexHeader);
        uint32_t methodOffset = indexs[data.index].GetOffset();

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        if (!bcInfoCollector->IsSkippedMethod(methodOffset)) {
            snapshotData_.AddIndexInfoToRecordInfo(SnapshotData::RecordType::METHOD,
                                                   std::make_pair(cpListIndex, data.index));
            newCP->SetObjectToCache(thread_, data.index, JSTaggedValue(methodOffset));
        }
    });

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::CLASS_LITERAL,
        [this, jsPandaFile, &cpListIndexMap, bcInfoCollector] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetOldConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSHandle<ConstantPool> oldCP(thread_, vm_->FindConstpool(jsPandaFile, oldCPID));

        auto literalObj = ConstantPool::GetClassLiteralFromCache(thread_, oldCP, data.index, *data.recordName);
        JSHandle<ClassLiteral> classLiteral(thread_, literalObj);
        JSHandle<TaggedArray> arrayHandle(thread_, classLiteral->GetArray());

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        CollectLiteralInfo(arrayHandle, data.index, newCP, bcInfoCollector);
        snapshotData_.AddIndexInfoToRecordInfo(SnapshotData::RecordType::LITERAL,
                                               std::make_pair(cpListIndex, data.index));
    });

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::OBJECT_LITERAL,
        [this, jsPandaFile, &cpListIndexMap, bcInfoCollector] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetOldConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSHandle<ConstantPool> oldCP(thread_, vm_->FindConstpool(jsPandaFile, oldCPID));

        panda_file::File::EntityId id = oldCP->GetEntityId(data.index);
        JSMutableHandle<TaggedArray> elements(thread_, JSTaggedValue::Undefined());
        JSMutableHandle<TaggedArray> properties(thread_, JSTaggedValue::Undefined());
        LiteralDataExtractor::ExtractObjectDatas(thread_, jsPandaFile, id, elements,
                                                 properties, oldCP, *data.recordName);

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        CollectLiteralInfo(properties, data.index, newCP, bcInfoCollector);
        snapshotData_.AddIndexInfoToRecordInfo(SnapshotData::RecordType::LITERAL,
                                               std::make_pair(cpListIndex, data.index));
    });

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::ARRAY_LITERAL,
        [this, jsPandaFile, &cpListIndexMap, bcInfoCollector] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetOldConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSHandle<ConstantPool> oldCP(thread_, vm_->FindConstpool(jsPandaFile, oldCPID));

        panda_file::File::EntityId id = oldCP->GetEntityId(data.index);
        JSHandle<TaggedArray> literal = LiteralDataExtractor::GetDatasIgnoreType(
            thread_, jsPandaFile, id, oldCP, *data.recordName);

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        CollectLiteralInfo(literal, data.index, newCP, bcInfoCollector);
        snapshotData_.AddIndexInfoToRecordInfo(SnapshotData::RecordType::LITERAL,
                                               std::make_pair(cpListIndex, data.index));
    });
}

void TSManager::AddHClassToSnapshotConstantPoolList(const std::map<int32_t, uint32_t> &cpListIndexMap,
                                                    kungfu::BytecodeInfoCollector *bcInfoCollector)
{
    const JSPandaFile *jsPandaFile = bcInfoCollector->GetJSPandaFile();
    JSMutableHandle<ConstantPool> oldCP(thread_, thread_->GlobalConstants()->GetUndefined());
    JSMutableHandle<ConstantPool> newCP(thread_, thread_->GlobalConstants()->GetUndefined());
    for (auto &iter : cpListIndexMap) {
        int32_t oldCPID = iter.first;
        oldCP.Update(vm_->FindConstpool(jsPandaFile, oldCPID));
        uint32_t constantPoolSize = oldCP->GetCacheLength();

        uint32_t cpListIndex = iter.second;
        newCP.Update(GetSnapshotConstantPool(cpListIndex));

        const CVector<JSTaggedType> &hcVec = snapshotData_.GetSnapshotHCVector(oldCPID);
        uint32_t hcVecSize = hcVec.size();
        for (uint32_t i = 0; i < hcVecSize; ++i) {
            newCP->SetObjectToCache(thread_, constantPoolSize + i, JSTaggedValue(hcVec[i]));
        }
    }
}

void TSManager::CollectLiteralInfo(JSHandle<TaggedArray> array, uint32_t constantPoolIndex,
                                   JSHandle<ConstantPool> constantPool,
                                   kungfu::BytecodeInfoCollector *bcInfoCollector)
{
    JSMutableHandle<JSTaggedValue> valueHandle(thread_, JSTaggedValue::Undefined());
    uint32_t len = array->GetLength();
    std::vector<int> methodOffsetVec;
    for (uint32_t i = 0; i < len; i++) {
        valueHandle.Update(array->Get(i));
        if (valueHandle->IsJSFunction()) {
            auto methodOffset = JSHandle<JSFunction>(valueHandle)->GetCallTarget()->GetMethodId().GetOffset();
            if (bcInfoCollector->IsSkippedMethod(methodOffset)) {
                methodOffsetVec.emplace_back(-1);
            } else {
                methodOffsetVec.emplace_back(methodOffset);
            }
        }
    }

    uint32_t methodSize = methodOffsetVec.size();
    JSHandle<AOTLiteralInfo> aotLiteralInfo = factory_->NewAOTLiteralInfo(methodSize);
    for (uint32_t i = 0; i < methodSize; ++i) {
        auto methodOffset = methodOffsetVec[i];
        aotLiteralInfo->Set(thread_, i, JSTaggedValue(methodOffset));
    }

    constantPool->SetObjectToCache(thread_, constantPoolIndex, aotLiteralInfo.GetTaggedValue());
}

void TSManager::ResolveSnapshotConstantPool(const std::map<uint32_t, uint32_t> &methodToEntryIndexMap)
{
    auto &recordMethodInfo = snapshotData_.GetRecordInfo(SnapshotData::RecordType::METHOD);
    for (auto &item: recordMethodInfo) {
        uint32_t cpListIndex = item.first;
        uint32_t methodIndex = item.second;
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);

        JSTaggedValue val = newCP->GetObjectFromCache(methodIndex);
        uint32_t methodOffset = static_cast<uint32_t>(val.GetInt());
        if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
            LOG_COMPILER(INFO) << "[aot-snapshot] store AOT entry index of method (offset: " << methodOffset << ") ";
        }
        uint32_t entryIndex = methodToEntryIndexMap.at(methodOffset);
        newCP->SetObjectToCache(thread_, methodIndex, JSTaggedValue(entryIndex));
    }

    auto &recordLiteralInfo = snapshotData_.GetRecordInfo(SnapshotData::RecordType::LITERAL);
    for (auto &item: recordLiteralInfo) {
        uint32_t cpListIndex = item.first;
        uint32_t literalIndex = item.second;
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);

        JSTaggedValue val = newCP->GetObjectFromCache(literalIndex);
        AOTLiteralInfo *aotLiteralInfo = AOTLiteralInfo::Cast(val.GetTaggedObject());
        uint32_t aotLiteralInfoLen = aotLiteralInfo->GetLength();
        for (uint32_t i = 0; i < aotLiteralInfoLen; ++i) {
            JSTaggedValue methodOffsetVal = aotLiteralInfo->Get(i);
            if (methodOffsetVal.GetInt() == -1) {
                continue;
            }
            uint32_t methodOffset = static_cast<uint32_t>(methodOffsetVal.GetInt());
            if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
                LOG_COMPILER(INFO) << "[aot-snapshot] store AOT entry index of method (offset: "
                                   << methodOffset << ") ";
            }
            uint32_t entryIndex = methodToEntryIndexMap.at(methodOffset);
            aotLiteralInfo->Set(thread_, i, JSTaggedValue(entryIndex));
        }
    }
}

bool TSManager::IsBuiltinMath(kungfu::GateType funcType) const
{
    GlobalTSTypeRef funcGT = funcType.GetGTRef();
    uint32_t moduleId = funcGT.GetModuleId();
    if (moduleId != static_cast<uint32_t>(MTableIdx::BUILTIN)) {
        return false;
    }

    if (IsBuiltinsDTSEnabled()) {
        uint32_t idx = static_cast<uint32_t>(BuiltinTypeId::MATH);
        const JSPandaFile *builtinPandaFile = GetBuiltinPandaFile();
        uint32_t mathOffset = GetBuiltinOffset(idx);
        bool hasCreatedGT = HasCreatedGT(builtinPandaFile, mathOffset);
        if (hasCreatedGT) {
            auto gt = GetGTFromOffset(builtinPandaFile, mathOffset);
            return (funcGT == gt);
        }
        return false;
    }

    uint32_t localId = funcGT.GetLocalId();
    return (localId == static_cast<uint32_t>(BuiltinTypeId::MATH));
}

bool TSManager::IsBuiltin(kungfu::GateType funcType) const
{
    GlobalTSTypeRef funcGt = funcType.GetGTRef();
    uint32_t moduleId = funcGt.GetModuleId();
    return (moduleId == static_cast<uint32_t>(MTableIdx::BUILTIN));
}

void TSManager::GenerateBuiltinSummary()
{
    ASSERT(IsBuiltinsDTSEnabled());
    CString builtinsDTSFileName = GetBuiltinsDTS();
    JSPandaFile *jsPandaFile = JSPandaFileManager::GetInstance()->OpenJSPandaFile(builtinsDTSFileName);
    if (jsPandaFile == nullptr) {
        LOG_COMPILER(FATAL) << "load lib_ark_builtins.d.ts failed";
    }
    JSPandaFileManager::GetInstance()->InsertJSPandaFile(jsPandaFile);
    SetBuiltinPandaFile(jsPandaFile);
    CString builtinsRecordName(TSTypeTable::BUILTINS_TABLE_NAME);
    SetBuiltinRecordName(builtinsRecordName);
    panda_file::File::EntityId summaryOffset(jsPandaFile->GetTypeSummaryOffset(builtinsRecordName));
    JSHandle<TaggedArray> builtinOffsets = LiteralDataExtractor::GetTypeLiteral(thread_, jsPandaFile, summaryOffset);
    for (uint32_t i = 0; i <= static_cast<uint32_t>(BuiltinTypeId::NUM_OF_BUILTIN_TYPES); i++) {
        builtinOffsets_.emplace_back(static_cast<uint32_t>(builtinOffsets->Get(i).GetInt()));
    }
}

void TSManager::PrintNumOfTypes() const
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    uint32_t length = mTable->GetNumberOfTSTypeTables();
    uint32_t totalNumOfTypes = 0;
    for (uint32_t i = 0; i < length; i++) {
        JSHandle<EcmaString> valueString = mTable->GetModuleRequestByModuleId(thread_, i);
        std::string name = EcmaStringAccessor(valueString).ToStdString();
        JSHandle<TSTypeTable> tTable = GetTSTypeTable(i);
        uint32_t numOfExpectedTypes = tTable->GetNumberOfTypes();
        uint32_t numOfTypes = 0;
        for (uint32_t j = 1; j <= numOfExpectedTypes; j++) {
            JSHandle<JSTaggedValue> tsType(thread_, tTable->Get(j));
            if (tsType->IsTSType()) {
                numOfTypes++;
            }
        }
        totalNumOfTypes += numOfTypes;
        LOG_COMPILER(DEBUG) << "module table: " << i << ", "
                            << "module name: " << name << ", "
                            << "number of types: " << numOfTypes;
    }
    LOG_COMPILER(DEBUG) << "total number of types: " << totalNumOfTypes;
}

JSHandle<EcmaString> TSModuleTable::GetModuleRequestByModuleId(JSThread *thread, int entry) const
{
    int amiOffset = GetModuleRequestOffset(entry);
    JSHandle<EcmaString> amiPath(thread, Get(amiOffset));
    return amiPath;
}

int TSModuleTable::GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath) const
{
    uint32_t length = GetNumberOfTSTypeTables();
    for (uint32_t i = 0; i < length; i++) {
        JSHandle<EcmaString> valueString = GetModuleRequestByModuleId(thread, i);
        if (EcmaStringAccessor::StringsAreEqual(*amiPath, *valueString)) {
            return i;
        }
    }
    return NOT_FOUND;
}
} // namespace panda::ecmascript
