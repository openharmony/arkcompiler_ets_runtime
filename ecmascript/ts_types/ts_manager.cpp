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
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ts_types/ts_type_table.h"

namespace panda::ecmascript {
TSManager::TSManager(EcmaVM *vm) : vm_(vm), thread_(vm_->GetJSThread()), factory_(vm_->GetFactory()),
                                   assertTypes_(vm_->GetJSOptions().AssertTypes()),
                                   printAnyTypes_(vm_->GetJSOptions().PrintAnyTypes())
{
    JSHandle<TSModuleTable> mTable = factory_->NewTSModuleTable(TSModuleTable::DEFAULT_TABLE_CAPACITY);
    SetTSModuleTable(mTable);
}

void TSManager::Initialize()
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    TSModuleTable::Initialize(thread_, mTable);
    // Initialize module-table with 3 default type-tables
    SetTSModuleTable(mTable);
}

std::tuple<JSHandle<TSTypeTable>, uint32_t> TSManager::GenerateTSTypeTable(const JSPandaFile *jsPandaFile,
                                                                           const CString &recordName)
{
    JSHandle<EcmaString> amiPath = factory_->NewFromUtf8(recordName);
    JSHandle<TSTypeTable> table;
    uint32_t moduleId = 0;
    int moduleIdBaseOnFile = GetTSModuleTable()->GetGlobalModuleID(thread_, amiPath);
    if (moduleIdBaseOnFile != TSModuleTable::NOT_FOUND) {
        table = GetTSModuleTable()->GetTSTypeTable(thread_, moduleIdBaseOnFile);
        moduleId = moduleIdBaseOnFile;
    } else {
        // read type summary literal
        // struct of summaryLiteral: {numTypes, literalOffset0, literalOffset1, ...}
        panda_file::File::EntityId summaryOffset(jsPandaFile->GetTypeSummaryOffset(recordName));
        JSHandle<TaggedArray> summaryLiteral =
            LiteralDataExtractor::GetTypeLiteral(thread_, jsPandaFile, summaryOffset);
        uint32_t numIdx = static_cast<uint32_t>(BuiltinTypeId::NUM_INDEX_IN_SUMMARY);
        uint32_t numTypes = static_cast<uint32_t>(summaryLiteral->Get(numIdx).GetInt());

        // Initialize a empty TSTypeTable with length of (numTypes + RESERVE_TABLE_LENGTH)
        table = factory_->NewTSTypeTable(numTypes);
        moduleId = GetNextModuleId();
        table->SetNumberOfTypes(thread_, TSTypeTable::DEFAULT_NUM_TYPES);
        AddTypeTable(JSHandle<JSTaggedValue>(table), amiPath);
    }
    return std::make_tuple(table, moduleId);
}

void TSManager::AddTypeTable(JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath)
{
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    JSHandle<TSModuleTable> updateTable = TSModuleTable::AddTypeTable(thread_, table, typeTable, amiPath);
    SetTSModuleTable(updateTable);
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
    uint32_t numSelfTypes = layout->NumberOfElements();

    JSHandle<TSObjectType> extendField(thread_, extendClassType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> extendLayout(thread_, extendField->GetObjLayoutInfo());
    uint32_t numExtendTypes = extendLayout->NumberOfElements();

    uint32_t numTypes = numSelfTypes + numExtendTypes;

    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TSObjLayoutInfo> newLayout = factory->CreateTSObjLayoutInfo(numTypes);

    uint32_t index;
    for (index = 0; index < numExtendTypes; index++) {
        JSTaggedValue key = extendLayout->GetKey(index);
        JSTaggedValue type = extendLayout->GetTypeId(index);
        newLayout->SetKeyAndType(thread_, index, key, type);
    }

    for (index = 0; index < numSelfTypes; index++) {
        JSTaggedValue key = layout->GetKey(index);
        if (IsDuplicatedKey(extendLayout, key)) {
            continue;
        }
        JSTaggedValue type = layout->GetTypeId(index);
        newLayout->SetKeyAndType(thread_, numExtendTypes + index, key, type);
    }

    field->SetObjLayoutInfo(thread_, newLayout);
    classType->SetHasLinked(true);
}

bool TSManager::IsDuplicatedKey(JSHandle<TSObjLayoutInfo> extendLayout, JSTaggedValue key)
{
    ASSERT_PRINT(key.IsString(), "TS class field key is not a string");
    EcmaString *keyString = EcmaString::Cast(key.GetTaggedObject());

    uint32_t length = extendLayout->NumberOfElements();
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

int TSManager::GetHClassIndex(const kungfu::GateType &gateType)
{
    if (!IsClassInstanceTypeKind(gateType)) {
        return -1;
    }
    GlobalTSTypeRef instanceGT = gateType.GetGTRef();
    GlobalTSTypeRef classGT = GetClassType(instanceGT);
    auto it = classTypeIhcIndexMap_.find(classGT);
    if (it == classTypeIhcIndexMap_.end()) {
        return -1;
    } else {
        return it->second;
    }
}

JSTaggedValue TSManager::GetHClassFromCache(uint32_t index)
{
    const auto &hclassCache = snapshotRecordInfo_.GetHClassCache();
    JSHandle<ConstantPool> constantPool(GetSnapshotConstantPool());
    return JSTaggedValue(hclassCache[index - constantPool->GetCacheLength()]);
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
    if (static_cast<MTableIdx>(moduleId) != MTableIdx::PRIMITIVE) {
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

    return AddTSTypeToInferTable(JSHandle<TSType>(iteratorInstanceType));
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

    JSHandle<TSTypeTable> table = GetInferTypeTable();

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

GlobalTSTypeRef TSManager::AddTSTypeToInferTable(JSHandle<TSType> type) const
{
    JSHandle<TSTypeTable> iTable = GetInferTypeTable();
    JSHandle<TSTypeTable> newITable = TSTypeTable::PushBackTypeToTable(thread_, iTable, type);
    SetInferTypeTable(newITable);

    GlobalTSTypeRef gt = GlobalTSTypeRef(TSModuleTable::INFER_TABLE_ID, newITable->GetNumberOfTypes());
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
    for (int tableIndex = 0; tableIndex < mTable->GetNumberOfTSTypeTables(); ++tableIndex) {
        JSHandle<TSTypeTable> typeTable = mTable->GetTSTypeTable(thread_, tableIndex);
        GlobalTSTypeRef foundUnionRef = FindUnionInTypeTable(typeTable, unionType);
        if (!foundUnionRef.IsDefault()) {
            return foundUnionRef;
        }
    }

    return AddTSTypeToInferTable(JSHandle<TSType>(unionType));
}

void TSManager::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalModuleTable_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&snapshotConstantPool_)));
    snapshotRecordInfo_.Iterate(v);
}

JSHandle<TSTypeTable> TSManager::GetInferTypeTable() const
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    ASSERT(ConvertToString(mTable->GetAmiPathByModuleId(thread_, TSModuleTable::INFER_TABLE_ID).GetTaggedValue()) ==
           TSTypeTable::INFER_TABLE_NAME);

    uint32_t inferTableOffset = TSModuleTable::GetTSTypeTableOffset(TSModuleTable::INFER_TABLE_ID);
    JSHandle<TSTypeTable> inferTable(thread_, mTable->Get(inferTableOffset));
    return inferTable;
}

void TSManager::SetInferTypeTable(JSHandle<TSTypeTable> inferTable) const
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    ASSERT(ConvertToString(mTable->GetAmiPathByModuleId(thread_, TSModuleTable::INFER_TABLE_ID).GetTaggedValue()) ==
           TSTypeTable::INFER_TABLE_NAME);

    uint32_t inferTableOffset = TSModuleTable::GetTSTypeTableOffset(TSModuleTable::INFER_TABLE_ID);
    mTable->Set(thread_, inferTableOffset, inferTable);
}

JSHandle<TSTypeTable> TSManager::GetRuntimeTypeTable() const
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    ASSERT(ConvertToString(mTable->GetAmiPathByModuleId(thread_, TSModuleTable::RUNTIME_TABLE_ID).GetTaggedValue()) ==
           TSTypeTable::RUNTIME_TABLE_NAME);

    uint32_t runtimeTableOffset = TSModuleTable::GetTSTypeTableOffset(TSModuleTable::RUNTIME_TABLE_ID);
    JSHandle<TSTypeTable> runtimeTable(thread_, mTable->Get(runtimeTableOffset));
    return runtimeTable;
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

void TSManager::SetRuntimeTypeTable(JSHandle<TSTypeTable> runtimeTable)
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    ASSERT(ConvertToString(mTable->GetAmiPathByModuleId(thread_, TSModuleTable::RUNTIME_TABLE_ID).GetTaggedValue()) ==
           TSTypeTable::RUNTIME_TABLE_NAME);

    uint32_t runtimeTableOffset = TSModuleTable::GetTSTypeTableOffset(TSModuleTable::RUNTIME_TABLE_ID);
    mTable->Set(thread_, runtimeTableOffset, runtimeTable);
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
    JSHandle<TSTypeTable> iTable = GetInferTypeTable();
    JSHandle<TSTypeTable> newITable = TSTypeTable::PushBackTypeToTable(thread_, iTable,
        JSHandle<TSType>(classInstanceType));
    SetInferTypeTable(newITable);
    auto instanceGT = GlobalTSTypeRef(TSModuleTable::INFER_TABLE_ID, newITable->GetNumberOfTypes());
    classInstanceType->SetGT(instanceGT);
    ASSERT(IsClassInstanceTypeKind(instanceGT));
    return instanceGT;
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

void TSManager::GenerateStaticHClass(const JSPandaFile *jsPandaFile, JSHandle<TSClassType> classType)
{
    JSHandle<ConstantPool> constPool(thread_, vm_->FindConstpool(jsPandaFile, 0));
    JSHandle<TSObjectType> instanceType(thread_, classType->GetInstanceType());
    JSHClass *ihc = TSObjectType::GetOrCreateHClass(thread_, instanceType, TSObjectTypeKind::INSTANCE);
    JSHandle<TSObjectType> prototypeType(thread_, classType->GetPrototypeType());
    JSHClass *phc = TSObjectType::GetOrCreateHClass(thread_, prototypeType, TSObjectTypeKind::PROTOTYPE);
    JSHandle<JSHClass> phcHandle(thread_, JSTaggedValue(phc));
    JSHandle<JSObject> prototype = factory_->NewJSObject(phcHandle);
    ihc->SetProto(thread_, prototype);

    GlobalTSTypeRef gt = classType->GetGT();
    AddHClassInCompilePhase(gt, JSTaggedValue(ihc), constPool->GetCacheLength());
}

JSHandle<JSTaggedValue> TSManager::GetTSType(const GlobalTSTypeRef &gt) const
{
    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();

    if (moduleId == TSModuleTable::BUILTINS_TABLE_ID && !IsBuiltinsDTSEnabled()) {
        return JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
    }

    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    JSHandle<TSTypeTable> typeTable = mTable->GetTSTypeTable(thread_, moduleId);
    JSHandle<JSTaggedValue> tsType(thread_, typeTable->Get(localId));
    return tsType;
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
            if ((HasCreatedGT(GetBuiltinPandaFile(), GetBuiltinOffset(i))) &&
                (GetGTFromOffset(GetBuiltinPandaFile(), GetBuiltinOffset(i)) == classGT)) {
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

std::string TSManager::GetTypeStr(kungfu::GateType gateType) const
{
    GlobalTSTypeRef gt = gateType.GetGTRef();
    auto typeKind = GetTypeKind(gt);
    switch (typeKind) {
        case TSTypeKind::PRIMITIVE:
            return GetPrimitiveStr(gt);
        case TSTypeKind::CLASS:
            return "class";
        case TSTypeKind::CLASS_INSTANCE:
            return "class_instance";
        case TSTypeKind::FUNCTION:
            return "function";
        case TSTypeKind::UNION:
            return "union";
        case TSTypeKind::ARRAY:
            return "array";
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

void TSManager::SnapshotRecordInfo::Iterate(const RootVisitor &v)
{
    uint64_t hclassCacheSize = hclassCache_.size();
    for (uint64_t i = 0; i < hclassCacheSize; i++) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(hclassCache_.data()[i]))));
    }
}

void TSManager::InitSnapshotConstantPool(JSTaggedValue constantPool)
{
    JSHandle<ConstantPool> cp(thread_, constantPool);
    uint32_t constantPoolSize = cp->GetCacheLength();
    JSHandle<ConstantPool> newConstantPool = factory_->NewConstantPool(constantPoolSize);
    newConstantPool->SetJSPandaFile(cp->GetJSPandaFile());
    newConstantPool->SetIndexHeader(cp->GetIndexHeader());
    snapshotConstantPool_ = newConstantPool.GetTaggedValue();
}

void TSManager::AddHClassToSnapshotConstantPool()
{
    JSHandle<ConstantPool> cp(thread_, snapshotConstantPool_);
    uint32_t constantPoolSize = cp->GetCacheLength();
    const auto &hclassCache = snapshotRecordInfo_.GetHClassCache();
    uint32_t hclassCacheSize = snapshotRecordInfo_.GetHClassCacheSize();
    LOG_COMPILER(INFO) << "snapshot: constantPoolSize: " << constantPoolSize;
    LOG_COMPILER(INFO) << "snapshot: hclassCacheSize: " << hclassCacheSize;
    JSHandle<ConstantPool> newConstantPool = factory_->NewConstantPool(constantPoolSize + hclassCacheSize);
    for (uint32_t i = 0; i < constantPoolSize; ++i) {
        newConstantPool->SetObjectToCache(thread_, i, cp->GetObjectFromCache(i));
    }
    for (uint32_t i = 0; i < hclassCacheSize; ++i) {
        newConstantPool->SetObjectToCache(thread_, constantPoolSize + i, JSTaggedValue(hclassCache[i]));
    }
    newConstantPool->SetJSPandaFile(cp->GetJSPandaFile());
    newConstantPool->SetIndexHeader(cp->GetIndexHeader());
    snapshotConstantPool_ = newConstantPool.GetTaggedValue();
}

void TSManager::AddIndexOrSkippedMethodID(SnapshotInfoType type, uint32_t index, const CString &recordName)
{
    if (type != SnapshotInfoType::SKIPPED_METHOD && snapshotRecordInfo_.IsProcessed(index)) {
        return;
    }

    JSHandle<ConstantPool> snapshotConstantPool(GetSnapshotConstantPool());
    JSPandaFile *jsPandaFile = snapshotConstantPool->GetJSPandaFile();

    switch (type) {
        case SnapshotInfoType::STRING: {
            ConstantPool::GetStringFromCache(thread_, snapshotConstantPool_, index);
            break;
        }
        case SnapshotInfoType::METHOD: {
            panda_file::File::IndexHeader *indexHeader = snapshotConstantPool->GetIndexHeader();
            auto pf = jsPandaFile->GetPandaFile();
            Span<const panda_file::File::EntityId> indexs = pf->GetMethodIndex(indexHeader);
            panda_file::File::EntityId methodID = indexs[index];
            if (!snapshotRecordInfo_.IsSkippedMethod(methodID.GetOffset())) {
                snapshotRecordInfo_.AddMethodIndex(index);
                snapshotConstantPool->SetObjectToCache(thread_, index, JSTaggedValue(methodID.GetOffset()));
            }
            break;
        }
        case SnapshotInfoType::CLASS_LITERAL: {
            auto literalObj = ConstantPool::GetClassLiteralFromCache(thread_, snapshotConstantPool, index, recordName);
            JSHandle<TaggedArray> literalHandle(thread_, literalObj);
            CollectLiteralInfo(literalHandle, index, snapshotConstantPool);
            snapshotRecordInfo_.AddRecordLiteralIndex(index);
            break;
        }
        case SnapshotInfoType::OBJECT_LITERAL: {
            panda_file::File::EntityId id = snapshotConstantPool->GetEntityId(index);
            JSMutableHandle<TaggedArray> elements(thread_, JSTaggedValue::Undefined());
            JSMutableHandle<TaggedArray> properties(thread_, JSTaggedValue::Undefined());
            LiteralDataExtractor::ExtractObjectDatas(thread_, jsPandaFile,
                id, elements, properties, snapshotConstantPool, recordName);
            CollectLiteralInfo(properties, index, snapshotConstantPool);
            snapshotRecordInfo_.AddRecordLiteralIndex(index);
            break;
        }
        case SnapshotInfoType::ARRAY_LITERAL: {
            panda_file::File::EntityId id = snapshotConstantPool->GetEntityId(index);
            JSHandle<TaggedArray> literal = LiteralDataExtractor::GetDatasIgnoreType(
                thread_, jsPandaFile, id, snapshotConstantPool, recordName);
            CollectLiteralInfo(literal, index, snapshotConstantPool);
            snapshotRecordInfo_.AddRecordLiteralIndex(index);
            break;
        }
        case SnapshotInfoType::SKIPPED_METHOD: {
            snapshotRecordInfo_.AddSkippedMethodID(index);
            break;
        }
        default:
            UNREACHABLE();
    }
}

void TSManager::CollectLiteralInfo(JSHandle<TaggedArray> array, uint32_t constantPoolIndex,
                                   JSHandle<ConstantPool> snapshotConstantPool)
{
    JSMutableHandle<JSTaggedValue> valueHandle(thread_, JSTaggedValue::Undefined());
    uint32_t len = array->GetLength();
    std::vector<int> methodIDs;
    for (uint32_t i = 0; i < len; i++) {
        valueHandle.Update(array->Get(i));
        if (valueHandle->IsJSFunction()) {
            auto methodID = JSHandle<JSFunction>(valueHandle)->GetCallTarget()->GetMethodId().GetOffset();
            if (snapshotRecordInfo_.IsSkippedMethod(methodID)) {
                methodIDs.emplace_back(-1);
            } else {
                methodIDs.emplace_back(methodID);
            }
        }
    }

    uint32_t methodSize = methodIDs.size();
    JSHandle<AOTLiteralInfo> aotLiteralInfo = factory_->NewAOTLiteralInfo(methodSize);
    for (uint32_t i = 0; i < methodSize; ++i) {
        auto methodID = methodIDs[i];
        aotLiteralInfo->Set(thread_, i, JSTaggedValue(methodID));
    }

    snapshotConstantPool->SetObjectToCache(thread_, constantPoolIndex, aotLiteralInfo.GetTaggedValue());
}

void TSManager::ResolveSnapshotConstantPool(const std::map<uint32_t, uint32_t> &methodToEntryIndexMap)
{
    JSHandle<ConstantPool> snapshotConstantPool(GetSnapshotConstantPool());
    const auto &recordMethodIndex = snapshotRecordInfo_.GetRecordMethodIndex();
    const auto &recordLiteralIndex = snapshotRecordInfo_.GetRecordLiteralIndex();

    for (auto item: recordMethodIndex) {
        JSTaggedValue val = snapshotConstantPool->GetObjectFromCache(item);
        uint32_t methodID = static_cast<uint32_t>(val.GetInt());
        LOG_COMPILER(INFO) << "snapshot: resolve function method ID: " << methodID;
        uint32_t entryIndex = methodToEntryIndexMap.at(methodID);
        snapshotConstantPool->SetObjectToCache(thread_, item, JSTaggedValue(entryIndex));
    }

    for (auto item: recordLiteralIndex) {
        JSTaggedValue val = snapshotConstantPool->GetObjectFromCache(item);
        AOTLiteralInfo *aotLiteralInfo = AOTLiteralInfo::Cast(val.GetTaggedObject());
        uint32_t aotLiteralInfoLen = aotLiteralInfo->GetLength();
        for (uint32_t i = 0; i < aotLiteralInfoLen; ++i) {
            JSTaggedValue methodIDVal = aotLiteralInfo->Get(i);
            if (methodIDVal.GetInt() != -1) {
                uint32_t methodID = static_cast<uint32_t>(methodIDVal.GetInt());
                LOG_COMPILER(INFO) << "snapshot: resolve function method ID: " << methodID;
                uint32_t entryIndex = methodToEntryIndexMap.at(methodID);
                aotLiteralInfo->Set(thread_, i, JSTaggedValue(entryIndex));
            }
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
        auto gt = GetGTFromOffset(GetBuiltinPandaFile(), GetBuiltinOffset(idx));
        return (funcGT == gt);
    } else {
        uint32_t localId = funcGT.GetLocalId();
        return (localId == static_cast<uint32_t>(BuiltinTypeId::MATH));
    }
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

void TSModuleTable::Initialize(JSThread *thread, JSHandle<TSModuleTable> mTable)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
    mTable->SetNumberOfTSTypeTables(thread, DEFAULT_NUMBER_OF_TABLES);

    // set primitive type table
    JSHandle<EcmaString> primitiveTableName = factory->NewFromASCII(TSTypeTable::PRIMITIVE_TABLE_NAME);
    mTable->Set(thread, GetAmiPathOffset(PRIMITIVE_TABLE_ID), primitiveTableName);
    mTable->Set(thread, GetSortIdOffset(PRIMITIVE_TABLE_ID), JSTaggedValue(PRIMITIVE_TABLE_ID));
    JSHandle<TSTypeTable> primitiveTable = factory->NewTSTypeTable(0);
    mTable->Set(thread, GetTSTypeTableOffset(PRIMITIVE_TABLE_ID), primitiveTable);

    // set builtins type table
    JSHandle<EcmaString> builtinsTableName = factory->NewFromASCII(TSTypeTable::BUILTINS_TABLE_NAME);
    mTable->Set(thread, GetAmiPathOffset(BUILTINS_TABLE_ID), builtinsTableName);
    mTable->Set(thread, GetSortIdOffset(BUILTINS_TABLE_ID), JSTaggedValue(BUILTINS_TABLE_ID));
    JSHandle<TSTypeTable> builtinsTable;
    if (tsManager->IsBuiltinsDTSEnabled()) {
        GenerateBuiltinsTypeTable(thread);
    } else {
        builtinsTable = factory->NewTSTypeTable(0);
        mTable->Set(thread, GetTSTypeTableOffset(BUILTINS_TABLE_ID), builtinsTable);
    }

    // set infer type table
    JSHandle<EcmaString> inferTableName = factory->NewFromASCII(TSTypeTable::INFER_TABLE_NAME);
    mTable->Set(thread, GetAmiPathOffset(INFER_TABLE_ID), inferTableName);
    mTable->Set(thread, GetSortIdOffset(INFER_TABLE_ID), JSTaggedValue(INFER_TABLE_ID));
    JSHandle<TSTypeTable> inferTable = factory->NewTSTypeTable(0);
    mTable->Set(thread, GetTSTypeTableOffset(INFER_TABLE_ID), inferTable);

    // set runtime type table
    JSHandle<EcmaString> runtimeTableName = factory->NewFromASCII(TSTypeTable::RUNTIME_TABLE_NAME);
    mTable->Set(thread, GetAmiPathOffset(RUNTIME_TABLE_ID), runtimeTableName);
    mTable->Set(thread, GetSortIdOffset(RUNTIME_TABLE_ID), JSTaggedValue(RUNTIME_TABLE_ID));
    JSHandle<TSTypeTable> runtimeTable = factory->NewTSTypeTable(0);
    mTable->Set(thread, GetTSTypeTableOffset(RUNTIME_TABLE_ID), runtimeTable);
    AddRuntimeTypeTable(thread);
}

void TSModuleTable::AddRuntimeTypeTable(JSThread *thread)
{
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();
    JSHandle<TSTypeTable> runtimeTable = tsManager->GetRuntimeTypeTable();

    // add IteratorResult GT
    JSHandle<JSTaggedValue> valueString = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSTaggedValue> doneString = thread->GlobalConstants()->GetHandledDoneString();
    std::vector<JSHandle<JSTaggedValue>> prop = {valueString, doneString};
    std::vector<GlobalTSTypeRef> propType = { GlobalTSTypeRef(kungfu::GateType::AnyType().GetGTRef()),
        GlobalTSTypeRef(kungfu::GateType::BooleanType().GetGTRef()) };

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TSObjectType> iteratorResultType = factory->NewTSObjectType(prop.size());
    JSHandle<TSTypeTable> newRuntimeTable = TSTypeTable::PushBackTypeToTable(thread,
        runtimeTable, JSHandle<TSType>(iteratorResultType));
    GlobalTSTypeRef iteratorResultGt = GlobalTSTypeRef(TSModuleTable::RUNTIME_TABLE_ID,
        newRuntimeTable->GetNumberOfTypes());
    iteratorResultType->SetGT(iteratorResultGt);

    JSHandle<TSObjLayoutInfo> layoutInfo(thread, iteratorResultType->GetObjLayoutInfo());
    FillLayoutTypes(thread, layoutInfo, prop, propType);
    iteratorResultType->SetObjLayoutInfo(thread, layoutInfo);

    // add IteratorFunction GT
    JSHandle<TSFunctionType> iteratorFunctionType = factory->NewTSFunctionType(0);
    newRuntimeTable = TSTypeTable::PushBackTypeToTable(thread, runtimeTable, JSHandle<TSType>(iteratorFunctionType));
    GlobalTSTypeRef functiontGt = GlobalTSTypeRef(TSModuleTable::RUNTIME_TABLE_ID,
                                                  newRuntimeTable->GetNumberOfTypes());
    iteratorFunctionType->SetGT(functiontGt);
    iteratorFunctionType->SetReturnGT(iteratorResultGt);

    // add TSIterator GT
    JSHandle<JSTaggedValue> nextString = thread->GlobalConstants()->GetHandledNextString();
    JSHandle<JSTaggedValue> throwString = thread->GlobalConstants()->GetHandledThrowString();
    JSHandle<JSTaggedValue> returnString = thread->GlobalConstants()->GetHandledReturnString();
    std::vector<JSHandle<JSTaggedValue>> iteratorProp = {nextString, throwString, returnString};
    std::vector<GlobalTSTypeRef> iteratorPropType = {functiontGt, functiontGt, functiontGt};

    JSHandle<TSObjectType> iteratorType = factory->NewTSObjectType(iteratorProp.size());
    newRuntimeTable = TSTypeTable::PushBackTypeToTable(thread, runtimeTable, JSHandle<TSType>(iteratorType));
    GlobalTSTypeRef iteratorGt = GlobalTSTypeRef(TSModuleTable::RUNTIME_TABLE_ID, newRuntimeTable->GetNumberOfTypes());
    iteratorType->SetGT(iteratorGt);

    JSHandle<TSObjLayoutInfo> iteratorLayoutInfo(thread, iteratorType->GetObjLayoutInfo());
    FillLayoutTypes(thread, iteratorLayoutInfo, iteratorProp, iteratorPropType);
    iteratorType->SetObjLayoutInfo(thread, iteratorLayoutInfo);

    tsManager->SetRuntimeTypeTable(newRuntimeTable);
}

void TSModuleTable::FillLayoutTypes(JSThread *thread, JSHandle<TSObjLayoutInfo> &layOut,
    std::vector<JSHandle<JSTaggedValue>> &prop, std::vector<GlobalTSTypeRef> &propType)
{
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < prop.size(); index++) {
        key.Update(prop[index]);
        ASSERT(key->IsString());
        value.Update(JSTaggedValue(propType[index].GetType()));
        layOut->SetKeyAndType(thread, index, key.GetTaggedValue(), value.GetTaggedValue());
    }
}

JSHandle<EcmaString> TSModuleTable::GetAmiPathByModuleId(JSThread *thread, int entry) const
{
    int amiOffset = GetAmiPathOffset(entry);
    JSHandle<EcmaString> amiPath(thread, Get(amiOffset));
    return amiPath;
}

JSHandle<TSTypeTable> TSModuleTable::GetTSTypeTable(JSThread *thread, int entry) const
{
    uint32_t typeTableOffset = GetTSTypeTableOffset(entry);
    JSHandle<TSTypeTable> typeTable(thread, Get(typeTableOffset));

    return typeTable;
}

int TSModuleTable::GetGlobalModuleID(JSThread *thread, JSHandle<EcmaString> amiPath) const
{
    uint32_t length = GetNumberOfTSTypeTables();
    for (uint32_t i = 0; i < length; i ++) {
        JSHandle<EcmaString> valueString = GetAmiPathByModuleId(thread, i);
        if (EcmaStringAccessor::StringsAreEqual(*amiPath, *valueString)) {
            return i;
        }
    }
    return NOT_FOUND;
}

JSHandle<TSModuleTable> TSModuleTable::AddTypeTable(JSThread *thread, JSHandle<TSModuleTable> table,
                                                    JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath)
{
    int numberOfTSTypeTable = table->GetNumberOfTSTypeTables();
    if (GetTSTypeTableOffset(numberOfTSTypeTable) > table->GetLength()) {
        table = JSHandle<TSModuleTable>(TaggedArray::SetCapacity(thread, JSHandle<TaggedArray>(table),
                                                                 table->GetLength() * INCREASE_CAPACITY_RATE));
    }
    // add ts type table
    table->SetNumberOfTSTypeTables(thread, numberOfTSTypeTable + 1);
    table->Set(thread, GetAmiPathOffset(numberOfTSTypeTable), amiPath);
    table->Set(thread, GetSortIdOffset(numberOfTSTypeTable), JSTaggedValue(numberOfTSTypeTable));
    table->Set(thread, GetTSTypeTableOffset(numberOfTSTypeTable), typeTable);
    return table;
}

void TSModuleTable::GenerateBuiltinsTypeTable(JSThread *thread)
{
    auto tsManager = thread->GetEcmaVM()->GetTSManager();
    tsManager->GenerateBuiltinSummary();
    uint32_t numOfTypes = tsManager->GetBuiltinOffset(static_cast<uint32_t>(BuiltinTypeId::NUM_INDEX_IN_SUMMARY));
    JSHandle<TSTypeTable> table =
        thread->GetEcmaVM()->GetFactory()->NewTSTypeTable(numOfTypes);
    table->SetNumberOfTypes(thread, TSTypeTable::DEFAULT_NUM_TYPES);
    tsManager->GetTSModuleTable()->Set(thread, GetTSTypeTableOffset(BUILTINS_TABLE_ID), table);
}
} // namespace panda::ecmascript
