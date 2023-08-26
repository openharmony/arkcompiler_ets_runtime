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

#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/jspandafile/type_literal_extractor.h"
#include "ecmascript/ts_types/ts_type_table_generator.h"
#include "ecmascript/ts_types/ts_type_parser.h"
#include "ecmascript/vtable.h"

namespace panda::ecmascript {
TSManager::TSManager(EcmaVM *vm) : vm_(vm), thread_(vm_->GetJSThread()), factory_(vm_->GetFactory()),
                                   assertTypes_(vm_->GetJSOptions().AssertTypes()),
                                   typeThreshold_(vm_->GetJSOptions().GetTypeThreshold())
{
    JSHandle<TSModuleTable> mTable = factory_->NewTSModuleTable(TSModuleTable::INITIAL_CAPACITY);
    SetTSModuleTable(mTable);
}

void TSManager::Initialize()
{
    TSTypeTableGenerator tableGenerator(this);
    tableGenerator.GenerateDefaultTSTypeTables();
}

int TSManager::GetElementsIndexByArrayType(const kungfu::GateType &gateType,
                                           const panda_file::File::EntityId id)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    if (!IsArrayTypeKind(gateType)) {
        return -1;
    }
    return GetElementsIndex(id);
}

int TSManager::GetElementsKindIndexByArrayType(const kungfu::GateType &gateType,
                                               const panda_file::File::EntityId id)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    if (!IsArrayTypeKind(gateType)) {
        return -1;
    }
    return GetElementsKindIndex(id);
}

int TSManager::GetHClassIndexByObjectType(const kungfu::GateType &gateType)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    if (!IsObjectTypeKind(gateType)) {
        return -1;
    }
    GlobalTSTypeRef objectGT = gateType.GetGTRef();
    return GetHClassIndex(objectGT);
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

int TSManager::GetConstructorHClassIndexByClassGateType(const kungfu::GateType &gateType)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    if (!IsClassTypeKind(gateType)) {
        return -1;
    }
    GlobalTSTypeRef classGT = gateType.GetGTRef();
    return GetHClassIndex(classGT, true);
}

int TSManager::GetElementsIndex(panda_file::File::EntityId id)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    auto elementMap = jsArrayData_.GetElmMap();
    auto iter = elementMap.find(id);
    auto endIter = elementMap.end();
    if (iter == endIter) {
        return -1;
    } else {
        std::unordered_map<int32_t, uint32_t> &cpIndexMap = iter->second.GetCPIndexMap();
        auto indexIter = cpIndexMap.find(curCPID_);
        if (indexIter == cpIndexMap.end()) {
            // This elm is used in the current constantpool, but has not yet been recorded
            return RecordElmToVecAndIndexMap(iter->second);
        }
        return indexIter->second;
    }
}

uint32_t TSManager::RecordElmToVecAndIndexMap(ElementData &elmData)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    JSHandle<ConstantPool> constantPool(GetConstantPool());
    CVector<JSTaggedType> &elmVec = snapshotData_.GetSnapshotValVector(curCPID_);
    elmVec.emplace_back(elmData.GetELM());

    uint32_t index = constantPool->GetCacheLength() + elmVec.size() - 1;
    std::unordered_map<int32_t, uint32_t> &cpIndexMap = elmData.GetCPIndexMap();
    cpIndexMap[curCPID_] = index;

    return index;
}

int TSManager::GetElementsKindIndex(panda_file::File::EntityId id)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    auto elementKindMap = jsArrayData_.GetElmKindMap();
    auto iter = elementKindMap.find(id);
    auto endIter = elementKindMap.end();
    if (iter == endIter) {
        return -1;
    } else {
        std::unordered_map<int32_t, uint32_t> &cpIndexMap = iter->second.GetCPIndexMap();
        auto indexIter = cpIndexMap.find(curCPID_);
        if (indexIter == cpIndexMap.end()) {
            // This elmKind is used in the current constantpool, but has not yet been recorded
            return RecordElmKindToVecAndIndexMap(iter->second);
        }
        return indexIter->second;
    }
}

uint32_t TSManager::RecordElmKindToVecAndIndexMap(ElementKindData &elmKindData)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    JSHandle<ConstantPool> constantPool(GetConstantPool());
    CVector<JSTaggedType> &kindVec = snapshotData_.GetSnapshotValVector(curCPID_);
    kindVec.emplace_back(elmKindData.GetElmKind());

    uint32_t index = constantPool->GetCacheLength() + kindVec.size() - 1;
    std::unordered_map<int32_t, uint32_t> &cpIndexMap = elmKindData.GetCPIndexMap();
    cpIndexMap[curCPID_] = index;

    return index;
}

int TSManager::GetHClassIndex(GlobalTSTypeRef classGT, bool isConstructor)
{
    if (HasOffsetFromGT(classGT)) {
        uint32_t literalOffset = 0;
        CString recordName = "";
        std::tie(recordName, literalOffset) = GetOffsetFromGt(classGT);
        GetCompilationDriver()->AddResolvedMethod(recordName, literalOffset);
    }
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    std::map<GlobalTSTypeRef, IHClassData>::iterator iter;
    std::map<GlobalTSTypeRef, IHClassData>::iterator endIter;
    if (isConstructor) {
        iter = gtConstructorhcMap_.find(classGT);
        endIter = gtConstructorhcMap_.end();
    } else {
        iter = gtIhcMap_.find(classGT);
        endIter = gtIhcMap_.end();
    }
    if (iter == endIter) {
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
    CVector<JSTaggedType> &hcVec = snapshotData_.GetSnapshotValVector(curCPID_);
    hcVec.emplace_back(ihcData.GetIHC());

    uint32_t index = constantPool->GetCacheLength() + hcVec.size() - 1;
    std::unordered_map<int32_t, uint32_t> &cpIndexMap = ihcData.GetCPIndexMap();
    cpIndexMap[curCPID_] = index;

    return index;
}

JSTaggedValue TSManager::GetValueFromCache(uint32_t index)
{
    // make sure already setting correct curCP_ and curCPID_ before calling this method
    JSHandle<ConstantPool> constantPool(GetConstantPool());
    const CVector<JSTaggedType> &valVec = snapshotData_.GetSnapshotValVector(curCPID_);
    return JSTaggedValue(valVec[index - constantPool->GetCacheLength()]);
}

JSHandle<TSClassType> TSManager::GetExtendedClassType(JSHandle<TSClassType> classType) const
{
    ASSERT(classType.GetTaggedValue().IsTSClassType());
    // Get extended type of classType based on ExtensionGT
    GlobalTSTypeRef extensionGT = classType->GetExtensionGT();
    JSHandle<JSTaggedValue> extendClassType = GetTSType(extensionGT);

    ASSERT(extendClassType->IsTSClassType());
    return JSHandle<TSClassType>(extendClassType);
}

TSClassType *TSManager::GetExtendedClassType(const TSClassType *classType) const
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(JSTaggedValue(classType).IsTSClassType());
    // Get extended type of classType based on ExtensionGT
    GlobalTSTypeRef extensionGT = classType->GetExtensionGT();
    JSHandle<JSTaggedValue> extendClassType = GetTSType(extensionGT);

    ASSERT(extendClassType->IsTSClassType());
    return TSClassType::Cast(extendClassType->GetTaggedObject());
}

GlobalTSTypeRef TSManager::GetPropType(GlobalTSTypeRef gt, JSHandle<JSTaggedValue> propertyName) const
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
        return TSObjectType::GetPropTypeGT(thread, objectType, propertyName);
    } else if (type->IsTSIteratorInstanceType()) {
        JSHandle<TSIteratorInstanceType> iteratorInstance(type);
        return TSIteratorInstanceType::GetPropTypeGT(thread, iteratorInstance, propertyName);
    } else if (type->IsTSInterfaceType()) {
        JSHandle<TSInterfaceType> interfaceType(type);
        return TSInterfaceType::GetPropTypeGT(thread, interfaceType, propertyName);
    } else if (type->IsTSNamespaceType()) {
        JSHandle<TSNamespaceType> namespaceType(type);
        return TSNamespaceType::GetPropTypeGT(thread, namespaceType, propertyName);
    }
    return GlobalTSTypeRef::Default();
}


GlobalTSTypeRef TSManager::GetIndexSignType(GlobalTSTypeRef objType, kungfu::GateType indexType) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> type = GetTSType(objType);
    ASSERT(type->IsTSType());

    uint32_t typeId = indexType.Value();
    if (type->IsTSClassInstanceType()) {
        JSHandle<TSClassInstanceType> classInstanceType(type);
        return TSClassInstanceType::GetIndexSignType(thread, classInstanceType, typeId);
    } else if (type->IsTSObjectType()) {
        JSHandle<TSObjectType> objectType(type);
        return TSObjectType::GetIndexSignType(thread, objectType, typeId);
    } else if (type->IsTSInterfaceType()) {
        JSHandle<TSInterfaceType> interfaceType(type);
        return TSInterfaceType::GetIndexSignType(thread, interfaceType, typeId);
    }
    LOG_COMPILER(DEBUG) << "Unsupport TSType GetIndexSignType: "
                        << static_cast<uint32_t>(type->GetTaggedObject()->GetClass()->GetObjectType());
    return GlobalTSTypeRef::Default();
}

bool TSManager::IsStaticFunc(GlobalTSTypeRef gt) const
{
    ASSERT(IsFunctionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetStatic();
}

bool TSManager::GetSuperGateType(kungfu::GateType &gateType) const
{
    JSHandle<JSTaggedValue> type = GetTSType(gateType.GetGTRef());
    if (type->IsTSObjectType()) {
        return false;
    }
    if (type->IsTSClassType()) {
        JSHandle<TSClassType> classType(type);
        if (classType->IsBaseClassType()) {
            return false;
        }
        gateType = kungfu::GateType(classType->GetExtensionGT());
        return true;
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

GlobalTSTypeRef TSManager::GetSuperPropType(GlobalTSTypeRef gt, JSHandle<JSTaggedValue> propertyName,
                                            PropertyType propType) const
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> type = GetTSType(gt);
    if (type->IsTSClassType()) {
        JSHandle<TSClassType> classType(type);
        return TSClassType::GetSuperPropTypeGT(thread, classType, propertyName, propType);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

GlobalTSTypeRef TSManager::GetSuperPropType(GlobalTSTypeRef gt, const uint64_t key, PropertyType propType) const
{
    JSTaggedValue keyValue = JSTaggedValue(key);
    JSMutableHandle<JSTaggedValue> propertyName(thread_, JSTaggedValue::Undefined());
    if (keyValue.IsInt() || keyValue.IsDouble()) {
        propertyName.Update(keyValue);
    } else {
        propertyName.Update(factory_->NewFromStdString(std::to_string(key).c_str()));
    }
    return GetSuperPropType(gt, propertyName, propType);
}

GlobalTSTypeRef TSManager::GetPropType(GlobalTSTypeRef gt, const uint64_t key) const
{
    JSTaggedValue keyValue = JSTaggedValue(key);
    JSMutableHandle<JSTaggedValue> propertyName(thread_, JSTaggedValue::Undefined());
    if (keyValue.IsInt() || keyValue.IsDouble()) {
        propertyName.Update(keyValue);
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
    if (!gt.IsPrimitiveModule()) {
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
                    return TSTypeKind::INTERFACE;
                case JSType::TS_ITERATOR_INSTANCE_TYPE:
                    return TSTypeKind::ITERATOR_INSTANCE;
                case JSType::TS_NAMESPACE_TYPE:
                    return TSTypeKind::NAMESPACE;
                default:
                    LOG_ECMA(FATAL) << "this branch is unreachable";
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
    GlobalTSTypeRef kindGT =
        GlobalTSTypeRef(static_cast<uint32_t>(ModuleTableIdx::RUNTIME), static_cast<int>(runtimeType));
    GlobalTSTypeRef foundTypeRef = FindIteratorInstanceInInferTable(kindGT, elementGt);
    if (!foundTypeRef.IsDefault()) {
        return foundTypeRef;
    }

    JSHandle<TSIteratorInstanceType> iteratorInstanceType = factory_->NewTSIteratorInstanceType();
    iteratorInstanceType->SetKindGT(kindGT);
    iteratorInstanceType->SetElementGT(elementGt);

    return AddTSTypeToInferredTable(JSHandle<TSType>(iteratorInstanceType));
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

    JSHandle<TSTypeTable> table = GetInferredTable();

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
    if (UNLIKELY(!GlobalTSTypeRef::IsValidLocalId(iTable->GetNumberOfTypes() + 1))) {
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
    uint32_t numOfTables = static_cast<uint32_t>(mTable->GetNumberOfTSTypeTables());
    for (uint32_t tableIndex = 0; tableIndex < numOfTables; ++tableIndex) {
        JSHandle<TSTypeTable> typeTable = GetTSTypeTable(tableIndex);
        GlobalTSTypeRef foundUnionRef = FindUnionInTypeTable(typeTable, unionType);
        if (!foundUnionRef.IsDefault()) {
            return foundUnionRef;
        }
    }

    return AddTSTypeToInferredTable(JSHandle<TSType>(unionType));
}

void TSManager::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalModuleTable_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&curCP_)));
    snapshotData_.Iterate(v);
    jsArrayData_.Iterate(v);
    for (auto iter : gtIhcMap_) {
        iter.second.Iterate(v);
    }
    for (auto &exportTable : resolvedExportTable_) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&resolvedExportTable_.at(exportTable.first))));
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

int TSManager::GetMethodIndex(GlobalTSTypeRef gt) const
{
    ASSERT(IsFunctionTypeKind(gt));
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    uint32_t methodOffset = functionType->GetMethodOffset();
    JSHandle<ConstantPool> constantPool(GetConstantPool());
    return constantPool->GetMethodIndexByEntityId(panda_file::File::EntityId(methodOffset));
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

bool TSManager::CanFastCall(GlobalTSTypeRef gt) const
{
    if (!IsFunctionTypeKind(gt)) {
        return false;
    }
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetIsFastCall();
}

bool TSManager::IsNoGC(GlobalTSTypeRef gt) const
{
    if (!IsFunctionTypeKind(gt)) {
        return false;
    }
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetIsNoGC();
}

bool TSManager::MethodOffsetIsVaild(GlobalTSTypeRef gt) const
{
    if (!IsFunctionTypeKind(gt)) {
        return false;
    }
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetIsMethodOffsetVaild();
}

bool TSManager::FastCallFlagIsVaild(GlobalTSTypeRef gt) const
{
    if (!IsFunctionTypeKind(gt)) {
        return false;
    }
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetIsFastCallVaild();
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
    return AddTSTypeToInferredTable(JSHandle<TSType>(classInstanceType));
}

GlobalTSTypeRef TSManager::CreateArrayType()
{
    JSHandle<TSArrayType> arrayType = factory_->NewTSArrayType();
    return AddTSTypeToInferredTable(JSHandle<TSType>(arrayType));
}

GlobalTSTypeRef TSManager::CreateNamespaceType()
{
    JSHandle<TSNamespaceType> namespaceType = factory_->NewTSNamespaceType();
    return AddTSTypeToInferredTable(JSHandle<TSType>(namespaceType));
}

void TSManager::AddNamespacePropType(kungfu::GateType objType, JSTaggedValue name, kungfu::GateType valueType)
{
    JSHandle<JSTaggedValue> tsType = GetTSType(GlobalTSTypeRef(objType.Value()));
    JSHandle<TSNamespaceType> namespaceType(tsType);
    JSHandle<JSTaggedValue> key(thread_, name);
    JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(valueType.Value()));
    TSNamespaceType::AddKeyAndValue(thread_, namespaceType, key, value);
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

void TSManager::AddArrayTSElements(panda_file::File::EntityId id, JSTaggedValue elements)
{
    JSHandle<TaggedArray> elementsHandle(thread_, elements);
    ElementData elementData = ElementData(elementsHandle.GetTaggedType());
    jsArrayData_.AddElmMap(id, elementData);
}

void TSManager::AddArrayTSElementsKind(panda_file::File::EntityId id, JSTaggedValue kind)
{
    JSHandle<JSTaggedValue> kindHandle(thread_, kind);
    ElementKindData elementKindData = ElementKindData(kindHandle.GetTaggedType());
    jsArrayData_.AddElmKindMap(id, elementKindData);
}

void TSManager::AddArrayTSConstantIndex(uint64_t bcAbsoluteOffset, JSTaggedValue index)
{
    JSHandle<JSTaggedValue> indexHandle(thread_, index);
    ConstantIndexData constantIndexData = ConstantIndexData(indexHandle.GetTaggedType());
    jsArrayData_.AddOffConstIndexMap(bcAbsoluteOffset, constantIndexData);
}

void TSManager::AddInstanceTSHClass(GlobalTSTypeRef gt, JSHandle<JSHClass> &ihclass)
{
    IHClassData ihcData = IHClassData(ihclass.GetTaggedType());
    gtIhcMap_.insert({gt, ihcData});
}

void TSManager::AddConstructorTSHClass(GlobalTSTypeRef gt, JSHandle<JSHClass> &constructorHClass)
{
    IHClassData ihcData = IHClassData(constructorHClass.GetTaggedType());
    gtConstructorhcMap_.insert({gt, ihcData});
}

JSTaggedValue TSManager::GetTSHClass(const kungfu::GateType &gateType) const
{
    if (!IsClassTypeKind(gateType) && !IsObjectTypeKind(gateType)) {
        return JSTaggedValue::Hole();
    }
    GlobalTSTypeRef classGT = gateType.GetGTRef();
    auto iter = gtIhcMap_.find(classGT);
    if (iter != gtIhcMap_.end()) {
        return JSTaggedValue(iter->second.GetIHC());
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue TSManager::GetInstanceTSHClass(const JSHandle<TSClassType> &classType) const
{
    GlobalTSTypeRef gt = classType->GetGT();
    IHClassData data = gtIhcMap_.at(gt);
    return JSTaggedValue(data.GetIHC());
}

bool TSManager::HasTSHClass(const JSHandle<TSClassType> &classType) const
{
    GlobalTSTypeRef gt = classType->GetGT();
    return gtIhcMap_.find(gt) != gtIhcMap_.end();
}

bool TSManager::HasTSHClass(const TSClassType *classType) const
{
    GlobalTSTypeRef gt = classType->GetGT();
    return gtIhcMap_.find(gt) != gtIhcMap_.end();
}

JSHandle<JSTaggedValue> TSManager::GetTSType(const GlobalTSTypeRef &gt) const
{
    uint32_t moduleId = gt.GetModuleId();
    uint32_t localId = gt.GetLocalId();

    if ((gt.IsBuiltinModule() && !IsBuiltinsDTSEnabled()) || gt.IsPrimitiveModule()) {
        return thread_->GlobalConstants()->GetHandledUndefined();
    }

    JSHandle<TSTypeTable> typeTable = GetTSTypeTable(moduleId);
    JSHandle<JSTaggedValue> tsType(thread_, typeTable->Get(localId));
    return tsType;
}

bool TSManager::IsBuiltinClassType(BuiltinTypeId id, kungfu::GateType gateType) const
{
    if (!IsClassTypeKind(gateType)) {
        return false;
    }
    auto classGT = gateType.GetGTRef();
    if (UNLIKELY(!IsBuiltinsDTSEnabled())) {
        uint32_t localId = classGT.GetLocalId();
        return classGT.IsBuiltinModule() && (localId == static_cast<uint32_t>(id));
    }

    const JSPandaFile *builtinPandaFile = GetBuiltinPandaFile();
    uint32_t typeOffset = GetBuiltinOffset(static_cast<uint32_t>(id));
    GlobalTypeID gId(builtinPandaFile, typeOffset);
    if (HasCreatedGT(gId)) {
        auto gt = GetGTByGlobalTypeID(gId);
        return (gt == classGT);
    }
    return false;
}

bool TSManager::IsBuiltinInstanceType(BuiltinTypeId id, kungfu::GateType gateType) const
{
    if (!IsClassInstanceTypeKind(gateType)) {
        return false;
    }
    auto classGT = GetClassType(gateType.GetGTRef());
    if (UNLIKELY(!IsBuiltinsDTSEnabled())) {
        uint32_t localId = classGT.GetLocalId();
        return classGT.IsBuiltinModule() && (localId == static_cast<uint32_t>(id));
    }

    const JSPandaFile *builtinPandaFile = GetBuiltinPandaFile();
    uint32_t typeOffset = GetBuiltinOffset(static_cast<uint32_t>(id));
    GlobalTypeID gId(builtinPandaFile, typeOffset);
    if (HasCreatedGT(gId)) {
        auto gt = GetGTByGlobalTypeID(gId);
        return (gt == classGT);
    }
    return false;
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
            GlobalTypeID gId(GetBuiltinPandaFile(), GetBuiltinOffset(i));
            bool hasCreatedGT = HasCreatedGT(gId);
            if (hasCreatedGT && (GetGTByGlobalTypeID(gId) == classGT)) {
                return true;
            }
        }
        return false;
    }
    uint32_t l = classGT.GetLocalId();
    return classGT.IsBuiltinModule() &&
           (l >= static_cast<uint32_t>(BuiltinTypeId::TYPED_ARRAY_FIRST)) &&
           (l <= static_cast<uint32_t>(BuiltinTypeId::TYPED_ARRAY_LAST));
}

const std::vector<BuiltinTypeId> &TSManager::GetValidTypedArrayIds()
{
    static const std::vector<BuiltinTypeId> validTypedArrayIds = {
        BuiltinTypeId::INT8_ARRAY,
        BuiltinTypeId::UINT8_ARRAY,
        BuiltinTypeId::UINT8_CLAMPED_ARRAY,
        BuiltinTypeId::INT16_ARRAY,
        BuiltinTypeId::UINT16_ARRAY,
        BuiltinTypeId::INT32_ARRAY,
        BuiltinTypeId::UINT32_ARRAY,
        BuiltinTypeId::FLOAT32_ARRAY,
        BuiltinTypeId::FLOAT64_ARRAY
    };
    return validTypedArrayIds;
}

BuiltinTypeId TSManager::GetBuiltinTypeIdByJSType(JSType jsType)
{
    static const std::map<JSType, BuiltinTypeId> jsTypeToBultinTypeID = {
        {JSType::JS_INT8_ARRAY, BuiltinTypeId::INT8_ARRAY},
        {JSType::JS_UINT8_ARRAY, BuiltinTypeId::UINT8_ARRAY},
        {JSType::JS_UINT8_CLAMPED_ARRAY, BuiltinTypeId::UINT8_CLAMPED_ARRAY},
        {JSType::JS_INT16_ARRAY, BuiltinTypeId::INT16_ARRAY},
        {JSType::JS_UINT16_ARRAY, BuiltinTypeId::UINT16_ARRAY},
        {JSType::JS_INT32_ARRAY, BuiltinTypeId::INT32_ARRAY},
        {JSType::JS_UINT32_ARRAY, BuiltinTypeId::UINT32_ARRAY},
        {JSType::JS_FLOAT32_ARRAY, BuiltinTypeId::FLOAT32_ARRAY},
        {JSType::JS_FLOAT64_ARRAY, BuiltinTypeId::FLOAT64_ARRAY},
    };

    auto it = jsTypeToBultinTypeID.find(jsType);
    if (it != jsTypeToBultinTypeID.end()) {
        return it->second;
    }

    return BuiltinTypeId::NUM_INDEX_IN_SUMMARY;
}

const kungfu::GateType TSManager::GetBuiltinsGateTypeByPt(ClassType pgoType)
{
    ASSERT(pgoType.IsBuiltinsType());
    JSType jsType = static_cast<JSType>(pgoType.GetId());

    auto it = pgoBuiltinGTCache_.find(jsType);
    if (it != pgoBuiltinGTCache_.end()) {
        return kungfu::GateType(it->second);
    }

    BuiltinTypeId bTypeId = GetBuiltinTypeIdByJSType(jsType);
    // If some builtin types sampled by pgo are not supported in AOT, AnyType will be returned
    if (bTypeId == BuiltinTypeId::NUM_INDEX_IN_SUMMARY) {
        return kungfu::GateType::AnyType();
    }

    TSTypeParser parser(this);
    GlobalTSTypeRef gt = parser.CreateGT(GetBuiltinPandaFile(), GetBuiltinRecordName(),
        static_cast<uint32_t>(bTypeId));
    GlobalTSTypeRef instanceGT;

    if (UNLIKELY(!IsBuiltinsDTSEnabled())) {
        JSHandle<TSClassInstanceType> classInstanceType = factory_->NewTSClassInstanceType();
        classInstanceType->SetClassGT(gt);
        instanceGT = AddTSTypeToInferredTable(JSHandle<TSType>(classInstanceType));
    } else {
        instanceGT = CreateClassInstanceType(gt);
    }

    pgoBuiltinGTCache_.insert({jsType, instanceGT});
    return kungfu::GateType(instanceGT);
}

BuiltinTypeId TSManager::GetTypedArrayBuiltinId(kungfu::GateType gateType) const
{
    if (!IsClassInstanceTypeKind(gateType)) {
        return BuiltinTypeId::NUM_INDEX_IN_SUMMARY;
    }
    const GlobalTSTypeRef gateGT = GlobalTSTypeRef(gateType.Value());
    GlobalTSTypeRef classGT = GetClassType(gateGT);
    const auto pandaFile = GetBuiltinPandaFile();
    for (uint32_t i = static_cast<uint32_t>(BuiltinTypeId::TYPED_ARRAY_FIRST);
            i <= static_cast<uint32_t>(BuiltinTypeId::TYPED_ARRAY_LAST); i++) {
        if (IsBuiltinsDTSEnabled()) {
            const auto offset = GetBuiltinOffset(i);
            GlobalTypeID gId(pandaFile, offset);
            if ((HasCreatedGT(gId)) &&
               (GetGTByGlobalTypeID(gId) == classGT)) {
                return static_cast<BuiltinTypeId>(i);
            }
        }
        uint32_t l = classGT.GetLocalId();
        if (classGT.IsBuiltinModule() && (l == i)) {
            return static_cast<BuiltinTypeId>(i);
        }
    }
    return BuiltinTypeId::NUM_INDEX_IN_SUMMARY;
}

bool TSManager::IsValidTypedArrayType(kungfu::GateType gateType) const
{
    std::vector<BuiltinTypeId> ids = GetValidTypedArrayIds();
    for (const auto &id : ids) {
        if (IsBuiltinInstanceType(id, gateType)) {
            return true;
        }
    }
    return false;
}

bool TSManager::IsIntTypedArrayType(kungfu::GateType gateType) const
{
    return IsBuiltinInstanceType(BuiltinTypeId::INT8_ARRAY, gateType) ||
           IsBuiltinInstanceType(BuiltinTypeId::UINT8_ARRAY, gateType) ||
           IsBuiltinInstanceType(BuiltinTypeId::UINT8_CLAMPED_ARRAY, gateType) ||
           IsBuiltinInstanceType(BuiltinTypeId::INT16_ARRAY, gateType) ||
           IsBuiltinInstanceType(BuiltinTypeId::UINT16_ARRAY, gateType) ||
           IsBuiltinInstanceType(BuiltinTypeId::INT32_ARRAY, gateType);
}

bool TSManager::IsDoubleTypedArrayType(kungfu::GateType gateType) const
{
    return IsBuiltinInstanceType(BuiltinTypeId::FLOAT32_ARRAY, gateType) ||
           IsBuiltinInstanceType(BuiltinTypeId::FLOAT64_ARRAY, gateType);
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
    ASSERT(builtinGT.IsBuiltinModule());
    if (IsBuiltinsDTSEnabled()) {
        for (uint32_t idx = static_cast<uint32_t>(BuiltinTypeId::FUNCTION);
            idx <= static_cast<uint32_t>(BuiltinTypeId::INTL); idx++) {
            GlobalTypeID gId(GetBuiltinPandaFile(), GetBuiltinOffset(idx));
            if ((HasCreatedGT(gId)) &&
               (GetGTByGlobalTypeID(gId) == builtinGT)) {
                   return idx;
               }
        }
    }
    return builtinGT.GetLocalId();
}

std::string TSManager::GetClassTypeStr(GlobalTSTypeRef gt) const
{
    if (gt.IsBuiltinModule()) {
        uint32_t index = GetBuiltinIndex(gt);
        return GetBuiltinsName(index);
    }

    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    if (tsType->IsUndefined()) {
        return "any";
    }
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
        case TSTypeKind::INTERFACE:
            return "interface";
        case TSTypeKind::ITERATOR_INSTANCE:
            return "iterator_instance";
        case TSTypeKind::UNKNOWN:
            return "unknown";
        case TSTypeKind::NAMESPACE:
            return "namespace";
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
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
            return "unknown";
    }
}

void TSManager::SetCurConstantPool(const JSPandaFile *jsPandaFile, uint32_t methodOffset)
{
    curCPID_ = GetConstantPoolIDByMethodOffset(jsPandaFile, methodOffset);
    curCP_ = vm_->GetJSThread()->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, curCPID_);
}

int32_t TSManager::GetConstantPoolIDByMethodOffset(const JSPandaFile *jsPandaFile, uint32_t methodOffset)
{
    panda_file::IndexAccessor indexAccessor(*jsPandaFile->GetPandaFile(),
                                            panda_file::File::EntityId(methodOffset));
    return static_cast<int32_t>(indexAccessor.GetHeaderIndex());
}

JSHandle<ConstantPool> TSManager::GetSnapshotConstantPool(uint32_t cpListIndex)
{
    JSHandle<TaggedArray> snapshotCPList(thread_, snapshotData_.GetSnapshotCPList());
    return JSHandle<ConstantPool>(thread_, snapshotCPList->Get(cpListIndex));
}

void TSManager::ProcessSnapshotConstantPool(kungfu::BytecodeInfoCollector *bcInfoCollector)
{
    const CMap<int32_t, JSTaggedValue> &oldCPValues = vm_->GetJSThread()->GetCurrentEcmaContext()->FindConstpools(
        bcInfoCollector->GetJSPandaFile()).value();
    std::map<int32_t, uint32_t> cpListIndexMap;

    GenerateSnapshotConstantPoolList(cpListIndexMap, oldCPValues);
    FillSnapshotConstantPoolList(cpListIndexMap, bcInfoCollector);
    AddValueToSnapshotConstantPoolList(cpListIndexMap, bcInfoCollector);
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
        const CVector<JSTaggedType> &valVec = snapshotData_.GetSnapshotValVector(oldCPID);
        uint32_t valVecSize = valVec.size();
        if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
            LOG_COMPILER(INFO) << "[aot-snapshot] constantPoolID: " << oldCPID;
            LOG_COMPILER(INFO) << "[aot-snapshot] constantPoolSize: " << cpSize;
            LOG_COMPILER(INFO) << "[aot-snapshot] valueSize: " << valVecSize;
        }
        // 1: constIndexInfo TaggedArray
        JSHandle<ConstantPool> newCp = factory_->NewConstantPool(cpSize + valVecSize);

        auto offConstIndexMap = jsArrayData_.GetOffConstIndexMap();
        // 2: each item need store (bcAbsoluteOffset, constIndex)
        JSHandle<TaggedArray> constIndexInfoList =
            factory_->NewTaggedArray(offConstIndexMap.size() * SnapshotData::SNAPSHOT_CP_LIST_ITEM_SIZE);
        uint32_t idx = 0;
        for (auto &x : offConstIndexMap) {
            constIndexInfoList->Set(thread_, idx++, JSTaggedValue(static_cast<int32_t>(x.first)));
            constIndexInfoList->Set(thread_, idx++, JSTaggedValue(x.second.GetIndex()));
        }
        newCp->SetObjectToCache(thread_, cpSize + valVecSize, constIndexInfoList.GetTaggedValue());

        snapshotCPList->Set(thread_, pos++, JSTaggedValue(oldCPID));
        cpListIndexMap[oldCPID] = pos;
        snapshotCPList->Set(thread_, pos++, newCp.GetTaggedValue());
    }
}

void TSManager::TryGetIhcAndChc(GlobalTSTypeRef gt, JSHandle<JSTaggedValue> &ihc, JSHandle<JSTaggedValue> &chc)
{
    if (gt.IsDefault()) {
        return;
    }
    auto ihcIt = gtIhcMap_.find(gt);
    if (ihcIt != gtIhcMap_.end()) {
        IHClassData ihcData = ihcIt->second;
        ihc = JSHandle<JSTaggedValue>(thread_, JSTaggedValue(ihcData.GetIHC()));
    }
    auto chcIt = gtConstructorhcMap_.find(gt);
    if (chcIt != gtConstructorhcMap_.end()) {
        IHClassData chcData = chcIt->second;
        chc = JSHandle<JSTaggedValue>(thread_, JSTaggedValue(chcData.GetIHC()));
    }
}

void TSManager::FillSnapshotConstantPoolList(const std::map<int32_t, uint32_t> &cpListIndexMap,
                                             kungfu::BytecodeInfoCollector *bcInfoCollector)
{
    const JSPandaFile *jsPandaFile = bcInfoCollector->GetJSPandaFile();

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::STRING,
        [this, jsPandaFile, &cpListIndexMap] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSTaggedValue oldCP = vm_->GetJSThread()->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, oldCPID);

        JSTaggedValue str = ConstantPool::GetStringFromCache(thread_, oldCP, data.index);

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        newCP->SetObjectToCache(thread_, data.index, str);
    });

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::METHOD,
        [this, jsPandaFile, &cpListIndexMap, bcInfoCollector] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSHandle<ConstantPool> oldCP(thread_,
            vm_->GetJSThread()->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, oldCPID));

        uint32_t methodOffset = oldCP->GetEntityId(data.index).GetOffset();

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        if (!bcInfoCollector->IsSkippedMethod(methodOffset)) {
            snapshotData_.AddIndexInfoToRecordInfo(SnapshotData::RecordType::METHOD,
                                                   std::make_pair(cpListIndex, data.index));
            newCP->SetObjectToCache(thread_, data.index, JSTaggedValue(methodOffset));
        }
    });

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::CLASS_LITERAL,
        [this, jsPandaFile, &cpListIndexMap, bcInfoCollector]
        (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSHandle<ConstantPool> oldCP(thread_,
            vm_->GetJSThread()->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, oldCPID));

        auto literalObj = ConstantPool::GetClassLiteralFromCache(thread_, oldCP, data.index, *data.recordName);
        JSHandle<ClassLiteral> classLiteral(thread_, literalObj);
        JSHandle<TaggedArray> arrayHandle(thread_, classLiteral->GetArray());

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        JSHandle<JSTaggedValue> ihc = thread_->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> chc = thread_->GlobalConstants()->GetHandledUndefined();
        TypeLocation loc(jsPandaFile, data.outerMethodOffset, data.bcIndex);
        GlobalTSTypeRef gt = GetLiteralGT(loc);
        ASSERT(gt.IsDefault() || IsUserDefinedClassTypeKind(gt));
        TryGetIhcAndChc(gt, ihc, chc);
        CollectLiteralInfo(arrayHandle, data.index, newCP, bcInfoCollector, ihc, chc);
        snapshotData_.AddIndexInfoToRecordInfo(SnapshotData::RecordType::LITERAL,
                                               std::make_pair(cpListIndex, data.index));
    });

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::OBJECT_LITERAL,
        [this, jsPandaFile, &cpListIndexMap, bcInfoCollector] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSHandle<ConstantPool> oldCP(thread_,
            vm_->GetJSThread()->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, oldCPID));

        panda_file::File::EntityId id = oldCP->GetEntityId(data.index);
        JSMutableHandle<TaggedArray> elements(thread_, JSTaggedValue::Undefined());
        JSMutableHandle<TaggedArray> properties(thread_, JSTaggedValue::Undefined());
        LiteralDataExtractor::ExtractObjectDatas(thread_, jsPandaFile, id, elements,
                                                 properties, oldCP, *data.recordName);

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        JSHandle<JSTaggedValue> ihc = thread_->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> chc = thread_->GlobalConstants()->GetHandledUndefined();
        TypeLocation loc(jsPandaFile, data.outerMethodOffset, data.bcIndex);
        GlobalTSTypeRef gt = GetLiteralGT(loc);
        ASSERT(gt.IsDefault() || IsObjectTypeKind(gt));
        TryGetIhcAndChc(gt, ihc, chc);
        CollectLiteralInfo(properties, data.index, newCP, bcInfoCollector, ihc, chc);
        snapshotData_.AddIndexInfoToRecordInfo(SnapshotData::RecordType::LITERAL,
                                               std::make_pair(cpListIndex, data.index));
    });

    bcInfoCollector->IterateConstantPoolInfo(kungfu::ConstantPoolInfo::ItemType::ARRAY_LITERAL,
        [this, jsPandaFile, &cpListIndexMap, bcInfoCollector] (const kungfu::ConstantPoolInfo::ItemData &data) {
        int32_t oldCPID = GetConstantPoolIDByMethodOffset(jsPandaFile, data.outerMethodOffset);
        JSHandle<ConstantPool> oldCP(thread_,
            vm_->GetJSThread()->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, oldCPID));

        panda_file::File::EntityId id = oldCP->GetEntityId(data.index);
        JSHandle<TaggedArray> literal = LiteralDataExtractor::GetDatasIgnoreType(
            thread_, jsPandaFile, id, oldCP, *data.recordName);

        uint32_t cpListIndex = cpListIndexMap.at(oldCPID);
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);
        JSHandle<JSTaggedValue> ihc = thread_->GlobalConstants()->GetHandledUndefined();
        JSHandle<JSTaggedValue> chc = thread_->GlobalConstants()->GetHandledUndefined();
        CollectLiteralInfo(literal, data.index, newCP, bcInfoCollector, ihc, chc);
        snapshotData_.AddIndexInfoToRecordInfo(SnapshotData::RecordType::LITERAL,
                                               std::make_pair(cpListIndex, data.index));
    });
}

void TSManager::AddValueToSnapshotConstantPoolList(const std::map<int32_t, uint32_t> &cpListIndexMap,
                                                   kungfu::BytecodeInfoCollector *bcInfoCollector)
{
    const JSPandaFile *jsPandaFile = bcInfoCollector->GetJSPandaFile();
    JSMutableHandle<ConstantPool> oldCP(thread_, thread_->GlobalConstants()->GetUndefined());
    JSMutableHandle<ConstantPool> newCP(thread_, thread_->GlobalConstants()->GetUndefined());
    for (auto &iter : cpListIndexMap) {
        int32_t oldCPID = iter.first;
        oldCP.Update(vm_->GetJSThread()->GetCurrentEcmaContext()->FindConstpool(jsPandaFile, oldCPID));
        uint32_t constantPoolSize = oldCP->GetCacheLength();

        uint32_t cpListIndex = iter.second;
        newCP.Update(GetSnapshotConstantPool(cpListIndex));

        const CVector<JSTaggedType> &valVec = snapshotData_.GetSnapshotValVector(oldCPID);
        uint32_t valVecSize = valVec.size();
        for (uint32_t i = 0; i < valVecSize; ++i) {
            newCP->SetObjectToCache(thread_, constantPoolSize + i, JSTaggedValue(valVec[i]));
        }
    }
}

void TSManager::CollectLiteralInfo(JSHandle<TaggedArray> array, uint32_t constantPoolIndex,
                                   JSHandle<ConstantPool> snapshotConstantPool,
                                   kungfu::BytecodeInfoCollector *bcInfoCollector,
                                   JSHandle<JSTaggedValue> ihc, JSHandle<JSTaggedValue> chc)
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
        aotLiteralInfo->SetObjectToCache(thread_, i, JSTaggedValue(methodOffset));
    }

    if (!ihc->IsUndefined()) {
        aotLiteralInfo->SetIhc(ihc.GetTaggedValue());
    }

    if (!chc->IsUndefined()) {
        aotLiteralInfo->SetChc(chc.GetTaggedValue());
    }

    snapshotConstantPool->SetObjectToCache(thread_, constantPoolIndex, aotLiteralInfo.GetTaggedValue());
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
        if (methodToEntryIndexMap.find(methodOffset) != methodToEntryIndexMap.end()) {
            uint32_t entryIndex = methodToEntryIndexMap.at(methodOffset);
            newCP->SetObjectToCache(thread_, methodIndex, JSTaggedValue(entryIndex));
        }
    }

    auto &recordLiteralInfo = snapshotData_.GetRecordInfo(SnapshotData::RecordType::LITERAL);
    for (auto &item: recordLiteralInfo) {
        uint32_t cpListIndex = item.first;
        uint32_t literalIndex = item.second;
        JSHandle<ConstantPool> newCP = GetSnapshotConstantPool(cpListIndex);

        JSTaggedValue val = newCP->GetObjectFromCache(literalIndex);
        AOTLiteralInfo *aotLiteralInfo = AOTLiteralInfo::Cast(val.GetTaggedObject());
        uint32_t aotLiteralInfoLen = aotLiteralInfo->GetCacheLength();
        for (uint32_t i = 0; i < aotLiteralInfoLen; ++i) {
            JSTaggedValue methodOffsetVal = aotLiteralInfo->GetObjectFromCache(i);
            if (methodOffsetVal.GetInt() == -1) {
                continue;
            }
            uint32_t methodOffset = static_cast<uint32_t>(methodOffsetVal.GetInt());
            if (vm_->GetJSOptions().IsEnableCompilerLogSnapshot()) {
                LOG_COMPILER(INFO) << "[aot-snapshot] store AOT entry index of method (offset: "
                                   << methodOffset << ") ";
            }
            uint32_t entryIndex = methodToEntryIndexMap.at(methodOffset);
            aotLiteralInfo->SetObjectToCache(thread_, i, JSTaggedValue(entryIndex));
        }
    }
}

kungfu::GateType TSManager::TryNarrowUnionType(kungfu::GateType gateType)
{
    // for unionType like A|null B|undefined, even A|null|undefined, we will narrow it to A or B at some situations.
    // such as ldobjbyname, stobjbyvalue etc.
    if (IsUnionTypeKind(gateType)) {
        JSHandle<JSTaggedValue> type = GetTSType(gateType.GetGTRef());
        JSHandle<TSUnionType> unionType(type);
        JSHandle<TaggedArray> components(thread_, unionType->GetComponents());
        auto length = components->GetLength();
        if (length > 3) { // the Maximum shrinkable union size is 3 in extreme scenario like A | null | undefined.
            return gateType;
        }
        auto actualTypeNum = 0;
        kungfu::GateType actualType = kungfu::GateType::AnyType();
        for (uint32_t index = 0; index < length; index++) {
            kungfu::GateType typeGt = kungfu::GateType(components->Get(index).GetInt());
            if (!typeGt.IsNullType() && !typeGt.IsUndefinedType()) {
                actualTypeNum++;
                actualType = typeGt;
                if (actualTypeNum > 1) { // if there isn't a single actual type, we can't narrow the union type.
                    break;
                }
            }
        }
        if (actualTypeNum == 1) {
            return actualType;
        }
    }
    return gateType;
}

JSTaggedValue TSManager::GetStringFromConstantPool(const uint16_t stringId) const
{
    JSHandle<ConstantPool> constantPool(GetConstantPool());
    return ConstantPool::GetStringFromCache(thread_, constantPool.GetTaggedValue(), stringId);
}

JSHandle<TaggedArray> TSManager::GetExportTableFromLiteral(const JSPandaFile *jsPandaFile, const CString &recordName)
{
    // To compare with the exportTable, we need to parse the literalbuffer in abc TypeAnnotation.
    // If the exportTable already exist, we will use it directly. OtherWise, we will parse and store it.
    // In type system parser at a later stage, we will also use these arrays to avoid duplicate parsing.
    if (HasResolvedExportTable(jsPandaFile, recordName)) {
        JSTaggedValue exportTypeTable = GetResolvedExportTable(jsPandaFile, recordName);
        JSHandle<TaggedArray> table(vm_->GetJSThread(), exportTypeTable);
        return table;
    }

    JSHandle<TaggedArray> typeOfExportedSymbols = GenerateExportTableFromLiteral(jsPandaFile, recordName);
    AddResolvedExportTable(jsPandaFile, recordName, typeOfExportedSymbols.GetTaggedValue());
    return typeOfExportedSymbols;
}

JSHandle<TaggedArray> TSManager::GenerateExportTableFromLiteral(const JSPandaFile *jsPandaFile,
                                                                const CString &recordName)
{
    bool isBuiltinTable = (std::strcmp(recordName.c_str(), TSTypeTable::BUILTINS_TABLE_NAME) == 0);
    ExportTypeTableExtractor exTableExtractor(jsPandaFile, recordName, isBuiltinTable);
    uint32_t length = exTableExtractor.GetLength();
    JSHandle<TaggedArray> typeOfExportedSymbols = factory_->NewOldSpaceTaggedArray(length);
    uint32_t index = 0;
    exTableExtractor.EnumerateModuleTypes(
        [this, &typeOfExportedSymbols, &index](const CString &name, const uint32_t typeId) {
            JSHandle<EcmaString> str = factory_->NewFromUtf8(name);
            typeOfExportedSymbols->Set(thread_, index++, JSTaggedValue(str.GetTaggedValue()));
            typeOfExportedSymbols->Set(thread_, index++, JSTaggedValue(typeId));
        });
    return typeOfExportedSymbols;
}

bool TSManager::IsBuiltinObjectMethod(BuiltinTypeId id, kungfu::GateType funcType) const
{
    DISALLOW_GARBAGE_COLLECTION;
    GlobalTSTypeRef funcGT = funcType.GetGTRef();
    if (!funcGT.IsBuiltinModule()) {
        return false;
    }

    if (IsBuiltinsDTSEnabled()) {
        uint32_t idx = static_cast<uint32_t>(id);
        const JSPandaFile *builtinPandaFile = GetBuiltinPandaFile();
        uint32_t builtinOffset = GetBuiltinOffset(idx);
        GlobalTypeID gId(builtinPandaFile, builtinOffset);
        bool hasCreatedGT = HasCreatedGT(gId);
        if (hasCreatedGT) {
            JSHandle<JSTaggedValue> funcTsType = GetTSType(funcGT);
            ASSERT(funcTsType->IsTSFunctionType());
            JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(funcTsType);
            auto name = functionType->GetName();

            auto gt = GetGTByGlobalTypeID(gId);
            auto tsType = GetTSType(gt);
            ASSERT(tsType->IsTSClassType());
            JSHandle<TSClassType> classType(tsType);
            JSHandle<TSObjectType> constructorType(thread_, classType->GetConstructorType());
            JSTaggedValue layout = constructorType->GetObjLayoutInfo();
            TSObjLayoutInfo *itLayout = TSObjLayoutInfo::Cast(layout.GetTaggedObject());
            int index = itLayout->GetElementIndexByKey(name);
            if (index != -1) {
                auto builtinFuncGt = GlobalTSTypeRef(itLayout->GetTypeId(index).GetInt());
                return builtinFuncGt == funcGT;
            }
            JSHandle<TSObjectType> prototypeType(thread_, classType->GetPrototypeType());
            JSTaggedValue prototypeLayout = prototypeType->GetObjLayoutInfo();
            TSObjLayoutInfo *pPrototypeLayout = TSObjLayoutInfo::Cast(prototypeLayout.GetTaggedObject());
            index = pPrototypeLayout->GetElementIndexByKey(name);
            if (index != -1) {
                auto builtinFuncGt = GlobalTSTypeRef(pPrototypeLayout->GetTypeId(index).GetInt());
                return builtinFuncGt == funcGT;
            }
        }
    }
    return false;
}

void TSManager::GenerateBuiltinSummary()
{
    ASSERT(IsBuiltinsDTSEnabled());
    CString builtinsDTSFileName = GetBuiltinsDTS();
    std::shared_ptr<JSPandaFile> jsPandaFile = JSPandaFileManager::GetInstance()->OpenJSPandaFile(builtinsDTSFileName);
    if (jsPandaFile == nullptr) {
        LOG_COMPILER(FATAL) << "load lib_ark_builtins.d.ts failed";
    }
    JSPandaFileManager::GetInstance()->AddJSPandaFileVm(vm_, jsPandaFile);
    SetBuiltinPandaFile(jsPandaFile.get());
    CString builtinsRecordName(TSTypeTable::BUILTINS_TABLE_NAME);
    SetBuiltinRecordName(builtinsRecordName);
    TypeSummaryExtractor summExtractor(jsPandaFile.get(), builtinsRecordName);
    summExtractor.EnumerateTypeOffsets(static_cast<uint32_t>(BuiltinTypeId::NUM_OF_BUILTIN_TYPES),
        [this](const uint32_t typeOffset) {
            builtinOffsets_.emplace_back(typeOffset);
        });
}

void TSManager::PrintNumOfTypes() const
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    uint32_t length = static_cast<uint32_t>(mTable->GetNumberOfTSTypeTables());
    uint32_t totalNumOfTypes = 0;
    for (uint32_t i = 0; i < length; i++) {
        JSHandle<EcmaString> valueString = mTable->GetModuleRequestByModuleId(thread_, i);
        std::string name = EcmaStringAccessor(valueString).ToStdString();
        JSHandle<TSTypeTable> tTable = GetTSTypeTable(i);
        uint32_t numOfExpectedTypes = static_cast<uint32_t>(tTable->GetNumberOfTypes());
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

void TSManager::PrintTypeInfo(const JSPandaFile *jsPandaFile) const
{
    if (!(vm_->GetJSOptions().PrintTypeInfo())) {
        return;
    }

    LOG_COMPILER(INFO) << "====================================================================";
    LOG_COMPILER(INFO) << "start printing type info in file " << jsPandaFile->GetFileName();
    const auto &records = jsPandaFile->GetJSRecordInfo();
    for (const auto &it : records) {
        const auto &recordName = it.first;
        LOG_COMPILER(INFO) << "====================================================================";
        LOG_COMPILER(INFO) << "In record " << recordName;
        if (jsPandaFile->HasTypeSummaryOffset(recordName)) {
            LOG_COMPILER(INFO) << "[TypeLiterals]";
            TypeSummaryExtractor(jsPandaFile, recordName).Print();
        }
        ExportTypeTableExtractor(jsPandaFile, recordName, false).Print();
    }
    LOG_COMPILER(INFO) << "====================================================================";
    LOG_COMPILER(INFO) << "end of printing type info";
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
