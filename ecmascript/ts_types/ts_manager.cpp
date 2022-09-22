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

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/ts_types/ts_type_parser.h"
#include "ecmascript/ts_types/ts_type_table.h"
#include "libpandafile/file-inl.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
void TSManager::DecodeTSTypes(const JSPandaFile *jsPandaFile)
{
    ASSERT_PRINT(jsPandaFile->HasTSTypes(), "the file has no ts type info");
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();

    JSHandle<EcmaString> queryFileName = factory_->NewFromUtf8(jsPandaFile->GetJSPandaFileDesc());
    int index = mTable->GetGlobalModuleID(thread_, queryFileName);
    if (index == TSModuleTable::NOT_FOUND) {
        CVector<JSHandle<EcmaString>> recordImportModules {};
        TSTypeTable::Initialize(thread_, jsPandaFile, recordImportModules);
        Link();
    }
}

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
    SetTSModuleTable(mTable);
}

void TSManager::AddTypeTable(JSHandle<JSTaggedValue> typeTable, JSHandle<EcmaString> amiPath)
{
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    JSHandle<TSModuleTable> updateTable = TSModuleTable::AddTypeTable(thread_, table, typeTable, amiPath);
    SetTSModuleTable(updateTable);
}

void TSManager::Link()
{
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    int length = table->GetNumberOfTSTypeTables();
    for (int i = 0; i < length; i++) {
        JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread_, i);
        LinkTSTypeTable(typeTable);
    }
}

void TSManager::LinkTSTypeTable(JSHandle<TSTypeTable> table)
{
    uint32_t length = table->GetLength();
    JSMutableHandle<TSImportType> importType(factory_->NewTSImportType());
    for (uint32_t i = 1; i < length && table->Get(i).IsTSImportType(); i++) {
        importType.Update(table->Get(i));
        RecursivelyResolveTargetType(importType);
        table->Set(thread_, i, importType);
    }
}

void TSManager::RecursivelyResolveTargetType(JSMutableHandle<TSImportType>& importType)
{
    if (!importType->GetTargetGT().IsDefault()) {
        return;
    }
    JSHandle<TSModuleTable> table = GetTSModuleTable();

    JSHandle<EcmaString> importVarAndPath(thread_, importType->GetImportPath());
    JSHandle<EcmaString> target = GenerateImportVar(importVarAndPath);
    JSHandle<EcmaString> amiPath = GenerateImportRelativePath(importVarAndPath);
    int index = table->GetGlobalModuleID(thread_, amiPath);
    ASSERT(index != TSModuleTable::NOT_FOUND);
    JSHandle<TSTypeTable> typeTable = table->GetTSTypeTable(thread_, index);
    JSHandle<TaggedArray> moduleExportTable = TSTypeTable::GetExportValueTable(thread_, typeTable);

    size_t typeId = GetTypeIndexFromExportTable(target, moduleExportTable);
    if (typeId < TSTypeParser::USER_DEFINED_TYPE_OFFSET) {
        importType->SetTargetGT(GlobalTSTypeRef(typeId));
        return;
    }
    int userDefId = typeId - TSTypeParser::USER_DEFINED_TYPE_OFFSET;

    JSHandle<TSType> bindType(thread_, typeTable->Get(static_cast<uint32_t>(userDefId)));
    if (bindType.GetTaggedValue().IsTSImportType()) {
        JSMutableHandle<TSImportType> redirectImportType(bindType);
        RecursivelyResolveTargetType(redirectImportType);
        typeTable->Set(thread_, static_cast<uint32_t>(userDefId), redirectImportType);
        importType->SetTargetGT(redirectImportType->GetTargetGT());
    } else {
        importType->SetTargetGT(bindType->GetGT());
    }
}

int TSManager::GetTypeIndexFromExportTable(JSHandle<EcmaString> target, JSHandle<TaggedArray> &exportTable) const
{
    uint32_t capacity = exportTable->GetLength();
    // capacity/2 -> ["A", "B", 51, 52] -> if target is "A", return 51, index = 0 + 4/2 = 2
    uint32_t length = capacity / 2 ;
    for (uint32_t i = 0; i < length; i++) {
        EcmaString *valueString = EcmaString::Cast(exportTable->Get(i).GetTaggedObject());
        if (EcmaString::StringsAreEqual(*target, valueString)) {
            EcmaString *localIdString = EcmaString::Cast(exportTable->Get(i + 1).GetTaggedObject());
            return CStringToULL(ConvertToString(localIdString));
        }
    }
    return -1;
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
    } else {
        LOG_COMPILER(ERROR) << "unsupport TSType GetPropType: "
                            << static_cast<uint8_t>(type->GetTaggedObject()->GetClass()->GetObjectType());
        return GlobalTSTypeRef::Default();
    }
}

GlobalTSTypeRef TSManager::GetPropType(GlobalTSTypeRef gt, const uint64_t key) const
{
    auto propertyName = factory_->NewFromStdString(std::to_string(key).c_str());
    return GetPropType(gt, propertyName);
}

uint32_t TSManager::GetUnionTypeLength(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::UNION);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSUnionType());
    JSHandle<TSUnionType> unionType = JSHandle<TSUnionType>(tsType);
    JSHandle<TaggedArray> unionTypeArray(thread_, unionType->GetComponents());
    return unionTypeArray->GetLength();
}

GlobalTSTypeRef TSManager::GetUnionTypeByIndex(GlobalTSTypeRef gt, int index) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::UNION);
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
                case JSType::TS_IMPORT_TYPE:
                    return TSTypeKind::IMPORT;
                case JSType::TS_INTERFACE_TYPE:
                    return TSTypeKind::INTERFACE_KIND;
                default:
                    UNREACHABLE();
            }
        } else {
            return TSTypeKind::UNKNOWN;
        }
    }
    return TSTypeKind::PRIMITIVE;
}

GlobalTSTypeRef TSManager::CreateGT(const panda_file::File &pf, const uint32_t typeId) const
{
    JSHandle<TSModuleTable> table = GetTSModuleTable();
    JSHandle<EcmaString> moduleName = factory_->NewFromStdString(pf.GetFilename());
    uint32_t moduleId = table->GetGlobalModuleID(thread_, moduleName);
    return TSTypeParser::CreateGT(moduleId, typeId);
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

JSHandle<EcmaString> TSManager::GenerateAmiPath(JSHandle<EcmaString> cur, JSHandle<EcmaString> rel) const
{
    CString currentAbcFile = ConvertToString(cur.GetTaggedValue());
    CString relativeAbcFile = ConvertToString(rel.GetTaggedValue());
    CString fullPath;

    if (relativeAbcFile.find("./") != 0 && relativeAbcFile.find("../") != 0) { // not start with "./" or "../"
        fullPath = relativeAbcFile + ".abc";
        return factory_->NewFromUtf8(fullPath); // not relative
    }
    auto slashPos = currentAbcFile.find_last_of('/');
    if (slashPos == std::string::npos) {
        fullPath.append(relativeAbcFile.substr(2, relativeAbcFile.size() - 2)); // 2: remove "./"
        fullPath.append(".abc"); // ".js" -> ".abc"
        return factory_->NewFromUtf8(fullPath);
    }

    fullPath.append(currentAbcFile.substr(0, slashPos + 1)); // 1: with "/"
    fullPath.append(relativeAbcFile.substr(2, relativeAbcFile.size() - 2)); // 2: remove "./"
    fullPath.append(".abc"); // ".js" -> ".abc"

    return factory_->NewFromUtf8(fullPath);
}

JSHandle<EcmaString> TSManager::GenerateImportVar(JSHandle<EcmaString> import) const
{
    // importNamePath #A#./A
    CString importVarNamePath = ConvertToString(import.GetTaggedValue());
    auto firstPos = importVarNamePath.find_first_of('#');
    auto lastPos = importVarNamePath.find_last_of('#');
    CString target = importVarNamePath.substr(firstPos + 1, lastPos - firstPos - 1);
    return factory_->NewFromUtf8(target); // #A#./A -> A
}

JSHandle<EcmaString> TSManager::GenerateImportRelativePath(JSHandle<EcmaString> importRel) const
{
    CString importNamePath = ConvertToString(importRel.GetTaggedValue());
    auto lastPos = importNamePath.find_last_of('#');
    CString path = importNamePath.substr(lastPos + 1, importNamePath.size() - lastPos - 1);
    return factory_->NewFromUtf8(path); // #A#./A -> ./A
}

GlobalTSTypeRef TSManager::AddUnionToInferTable(JSHandle<TSUnionType> unionType)
{
    JSHandle<TSTypeTable> iTable = GetInferTypeTable();
    JSHandle<TSTypeTable> newITable = TSTypeTable::PushBackTypeToInferTable(thread_, iTable,
                                                                            JSHandle<TSType>(unionType));
    SetInferTypeTable(newITable);

    GlobalTSTypeRef gt = GlobalTSTypeRef(TSModuleTable::INFER_TABLE_ID, newITable->GetNumberOfTypes());
    unionType->SetGT(gt);
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

    return AddUnionToInferTable(unionType);
}

void TSManager::Iterate(const RootVisitor &v)
{
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&globalModuleTable_)));
    v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&constantPoolInfos_)));

    uint64_t hclassCacheSize = GetHClassCacheSize();
    for (uint64_t i = 0; i < hclassCacheSize; i++) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(hclassCache_.data()[i]))));
    }
}

GlobalTSTypeRef TSManager::GetImportTypeTargetGT(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::IMPORT);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSImportType());
    JSHandle<TSImportType> importType = JSHandle<TSImportType>(tsType);
    return importType->GetTargetGT();
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

void TSManager::SetInferTypeTable(JSHandle<TSTypeTable> inferTable)
{
    JSHandle<TSModuleTable> mTable = GetTSModuleTable();
    ASSERT(ConvertToString(mTable->GetAmiPathByModuleId(thread_, TSModuleTable::INFER_TABLE_ID).GetTaggedValue()) ==
           TSTypeTable::INFER_TABLE_NAME);

    uint32_t inferTableOffset = TSModuleTable::GetTSTypeTableOffset(TSModuleTable::INFER_TABLE_ID);
    mTable->Set(thread_, inferTableOffset, inferTable);
}

uint32_t TSManager::GetFunctionTypeLength(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    return functionType->GetLength();
}

GlobalTSTypeRef TSManager::GetFuncParameterTypeGT(GlobalTSTypeRef gt, int index) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    return functionType->GetParameterTypeGT(index);
}

GlobalTSTypeRef TSManager::GetFuncThisGT(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType(tsType);
    return functionType->GetThisGT();
}

GlobalTSTypeRef TSManager::GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    return functionType->GetReturnGT();
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
    JSHandle<TSTypeTable> newITable = TSTypeTable::PushBackTypeToInferTable(thread_, iTable,
                                                                            JSHandle<TSType>(classInstanceType));
    SetInferTypeTable(newITable);
    auto instanceGT = GlobalTSTypeRef(TSModuleTable::INFER_TABLE_ID, newITable->GetNumberOfTypes());
    classInstanceType->SetGT(instanceGT);
    ASSERT(GetTypeKind(instanceGT) == TSTypeKind::CLASS_INSTANCE);
    return instanceGT;
}

GlobalTSTypeRef TSManager::GetClassType(GlobalTSTypeRef classInstanceGT) const
{
    ASSERT(GetTypeKind(classInstanceGT) == TSTypeKind::CLASS_INSTANCE);
    JSHandle<JSTaggedValue> tsType = GetTSType(classInstanceGT);
    ASSERT(tsType->IsTSClassInstanceType());
    JSHandle<TSClassInstanceType> instanceType(tsType);
    return instanceType->GetClassGT();
}

GlobalTSTypeRef TSManager::GetArrayParameterTypeGT(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::ARRAY);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSArrayType());
    JSHandle<TSArrayType> arrayType = JSHandle<TSArrayType>(tsType);
    return arrayType->GetElementGT();
}

void TSManager::GenerateStaticHClass(JSHandle<TSTypeTable> tsTypeTable, const JSPandaFile *jsPandaFile)
{
    JSHandle<ConstantPool> constPool(thread_, vm_->FindConstpool(jsPandaFile, 0));
    JSMutableHandle<TSClassType> classType(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<TSObjectType> instanceType(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<TSObjectType> prototypeType(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSHClass> phcHandle(thread_, JSTaggedValue::Undefined());

    for (int index = 1; index <= tsTypeTable->GetNumberOfTypes(); ++index) {
        JSTaggedValue type = tsTypeTable->Get(index);
        if (!type.IsTSClassType()) {
            continue;
        }
        classType.Update(type);

        instanceType.Update(classType->GetInstanceType());
        JSHClass *ihc = TSObjectType::GetOrCreateHClass(thread_, instanceType, TSObjectTypeKind::INSTANCE);
        prototypeType.Update(classType->GetPrototypeType());
        JSHClass *phc = TSObjectType::GetOrCreateHClass(thread_, prototypeType, TSObjectTypeKind::PROTOTYPE);
        phcHandle.Update(JSTaggedValue(phc));
        JSHandle<JSObject> prototype = thread_->GetEcmaVM()->GetFactory()->NewJSObject(phcHandle);
        ihc->SetProto(thread_, prototype);

        GlobalTSTypeRef gt = classType->GetGT();
        AddHClassInCompilePhase(gt, JSTaggedValue(ihc), constPool->GetCacheLength());
    }
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

std::string TSManager::GetTypeStr(kungfu::GateType gateType) const
{
    ASSERT(gateType.IsTSType());
    GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
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
        case TSPrimitiveType::BIG_INT:
            return "bigint";
        default:
            UNREACHABLE();
    }
}

void TSManager::CreateConstantPoolInfos(size_t size)
{
    ObjectFactory *factory = vm_->GetFactory();
    constantPoolInfosSize_ = 0;
    constantPoolInfos_ = factory->NewTaggedArray(size * CONSTANTPOOL_INFOS_ITEM_SIZE).GetTaggedValue();
}

void TSManager::CollectConstantPoolInfo(const JSPandaFile* jsPandaFile)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TaggedArray> constantPoolInfos = GetConstantPoolInfos();
    ASSERT(constantPoolInfosSize_ < constantPoolInfos->GetLength());

    std::string keyStr = std::to_string(jsPandaFile->GetFileUniqId());
    JSHandle<EcmaString> key = vm_->GetFactory()->NewFromStdString(keyStr);
    constantPoolInfos->Set(thread, constantPoolInfosSize_++, key);
    auto value = GenerateConstantPoolInfo(jsPandaFile);
    constantPoolInfos->Set(thread, constantPoolInfosSize_++, value);
}

JSTaggedValue TSManager::GenerateConstantPoolInfo(const JSPandaFile* jsPandaFile)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TaggedArray> constantPoolInfo = factory->NewTaggedArray(ComputeSizeOfConstantPoolInfo());
    JSHandle<ConstantPool> constantPool(thread, vm_->FindConstpool(jsPandaFile, 0));

    constantPoolInfo->Set(thread, NUM_OF_ORIGINAL_CONSTANTPOOL_DATA_INDEX,
                          JSTaggedValue(GetStringCacheSize() * ORIGINAL_CONSTANTPOOL_DATA_SIZE));
    constantPoolInfo->Set(thread, NUM_OF_HCLASS_INDEX, JSTaggedValue(GetHClassCacheSize()));

    uint32_t index = CONSTANTPOOL_INFO_DATA_OFFSET;
    IterateCaches(CacheKind::STRING_INDEX, [thread, &index, &constantPool, &constantPoolInfo]
    (uint32_t stringIndex) {
        JSTaggedValue str = constantPool->GetObjectFromCache(stringIndex);
        constantPoolInfo->Set(thread, index++, JSTaggedValue(stringIndex));
        constantPoolInfo->Set(thread, index++, str);
    });

    IterateCaches(CacheKind::HCLASS, [thread, &index, &constantPoolInfo]
    (JSTaggedType hclass) {
        constantPoolInfo->Set(thread, index++, JSTaggedValue(hclass));
    });

    if (ComputeSizeOfConstantPoolInfo() != index) {
        LOG_FULL(FATAL) << "constantpool info size incorrect";
    }

    return constantPoolInfo.GetTaggedValue();
}

void TSManager::SortConstantPoolInfos()
{
    JSHandle<TaggedArray> constantPoolInfos = GetConstantPoolInfos();
    uint32_t len = constantPoolInfos->GetLength();
    std::vector<std::pair<uint32_t, uint32_t>> indexTable;
    uint32_t tableLen = len / CONSTANTPOOL_INFOS_ITEM_SIZE;
    indexTable.reserve(tableLen);

    for (uint32_t i = 0; i < len; i += CONSTANTPOOL_INFOS_ITEM_SIZE) {
        EcmaString *key = EcmaString::Cast(constantPoolInfos->Get(i).GetTaggedObject());
        indexTable.emplace_back(std::make_pair(key->GetHashcode(), indexTable.size()));
    }

    std::sort(indexTable.begin(), indexTable.end(), [](std::pair<uint32_t, uint32_t> first,
    std::pair<uint32_t, uint32_t> second) {
        return first.first < second.first;
    });

    uint32_t nowIdx = 0;
    uint32_t changeIdx = 0;
    uint32_t tempIdx = 0;
    JSThread* thread = vm_->GetJSThread();
    for (uint32_t i = 0; i < tableLen; ++i) {
        nowIdx = i;
        JSTaggedValue tempKey = constantPoolInfos->Get(nowIdx * CONSTANTPOOL_INFOS_ITEM_SIZE);
        JSTaggedValue tempValue = constantPoolInfos->Get(nowIdx * CONSTANTPOOL_INFOS_ITEM_SIZE + 1);

        changeIdx = i;
        while (nowIdx != indexTable[changeIdx].second) {
            tempIdx = indexTable[changeIdx].second;
            constantPoolInfos->Set(thread, changeIdx * CONSTANTPOOL_INFOS_ITEM_SIZE,
                                   constantPoolInfos->Get(tempIdx * CONSTANTPOOL_INFOS_ITEM_SIZE));
            constantPoolInfos->Set(thread, changeIdx * CONSTANTPOOL_INFOS_ITEM_SIZE + 1,
                                   constantPoolInfos->Get(tempIdx * CONSTANTPOOL_INFOS_ITEM_SIZE + 1));
            indexTable[changeIdx].second = changeIdx;
            changeIdx = tempIdx;
        }
        constantPoolInfos->Set(thread, changeIdx * CONSTANTPOOL_INFOS_ITEM_SIZE, tempKey);
        constantPoolInfos->Set(thread, changeIdx * CONSTANTPOOL_INFOS_ITEM_SIZE + 1, tempValue);
        indexTable[changeIdx].second = changeIdx;
    }
}

JSHandle<ConstantPool> TSManager::RestoreConstantPool(const JSPandaFile* pf, uint32_t oldConstantPoolLen)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<TaggedArray> constantPoolInfos = GetConstantPoolInfos();
    std::string keyStr = std::to_string(pf->GetFileUniqId());
    JSHandle<EcmaString> key = vm_->GetFactory()->NewFromStdString(keyStr);
    uint32_t keyHash = key->GetHashcode();
    int leftBound = BinarySearch(keyHash, CONSTANTPOOL_INFOS_ITEM_SIZE);
    int rightBound = BinarySearch(keyHash, CONSTANTPOOL_INFOS_ITEM_SIZE, false);
    if (leftBound == -1 || rightBound == -1) {
        LOG_FULL(FATAL) << "restore constant pool fail";
    }

    JSMutableHandle<TaggedArray> valueArray(thread, JSTaggedValue::Undefined());
    while (leftBound <= rightBound) {
        uint32_t currentKeyIndex = leftBound * CONSTANTPOOL_INFOS_ITEM_SIZE;
        EcmaString *nowStr = EcmaString::Cast(constantPoolInfos->Get(currentKeyIndex).GetTaggedObject());
        if (EcmaString::StringsAreEqual(nowStr, *key)) {
            valueArray.Update(constantPoolInfos->Get(currentKeyIndex + 1));
        }
        leftBound++;
    }

    uint32_t originalConstantPoolDataStart = CONSTANTPOOL_INFO_DATA_OFFSET;
    uint32_t originalConstantPoolDataLen = static_cast<uint32_t>(valueArray->
                                           Get(NUM_OF_ORIGINAL_CONSTANTPOOL_DATA_INDEX).GetInt());
    uint32_t hclassStart = originalConstantPoolDataStart + originalConstantPoolDataLen;
    uint32_t hclassLen = static_cast<uint32_t>(valueArray->Get(NUM_OF_HCLASS_INDEX).GetInt());

    JSHandle<ConstantPool> constantPool = vm_->GetFactory()->NewConstantPool(oldConstantPoolLen + hclassLen);

    for (uint32_t i = 0; i < originalConstantPoolDataLen; i += ORIGINAL_CONSTANTPOOL_DATA_SIZE) {
        uint32_t currentIndex = originalConstantPoolDataStart + i;
        uint32_t constantPoolIndex = static_cast<uint32_t>(valueArray->Get(currentIndex).GetInt());
        JSTaggedValue constantPoolValue = valueArray->Get(currentIndex + 1);
        constantPool->SetObjectToCache(thread, constantPoolIndex, constantPoolValue);
    }

    for (uint32_t i = 0; i < hclassLen; ++i) {
        JSTaggedValue value = valueArray->Get(hclassStart + i);
        constantPool->SetObjectToCache(thread, oldConstantPoolLen + i, value);
    }

    return constantPool;
}

int TSManager::BinarySearch(uint32_t target, uint32_t itemSize, bool findLeftBound)
{
    JSHandle<TaggedArray> constantPoolInfos = GetConstantPoolInfos();
    int len = static_cast<int>(constantPoolInfos->GetLength()) / itemSize - 1;
    if (len < 0) {
        LOG_FULL(FATAL) << "constantPoolInfos should not be empty";
    }
    int left = 0;
    int right = len;

    while (left <= right) {
        int middle = left + (right - left) / 2;
        EcmaString *middleStr = EcmaString::Cast(constantPoolInfos->Get(middle * itemSize).GetTaggedObject());
        uint32_t nowHashCode = middleStr->GetHashcode();
        if (target < nowHashCode) {
            right = middle - 1;
        } else if (target > nowHashCode) {
            left = middle + 1;
        } else {
            if (findLeftBound) {
                right = middle - 1;
            } else {
                left = middle + 1;
            }
        }
    }

    int finalIdx = findLeftBound? left: right;
    if (finalIdx > len || finalIdx < 0) {
        return -1;
    }

    EcmaString *finalStr = EcmaString::Cast(constantPoolInfos->Get(finalIdx * itemSize).GetTaggedObject());
    uint32_t finalStrHashCode = finalStr->GetHashcode();

    return finalStrHashCode == target? finalIdx: -1;
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
        builtinsTable = GenerateBuiltinsTypeTable(thread);
    } else {
        builtinsTable = factory->NewTSTypeTable(0);
    }
    mTable->Set(thread, GetTSTypeTableOffset(BUILTINS_TABLE_ID), builtinsTable);
    TSTypeTable::LinkClassType(thread, builtinsTable);

    // set infer type table
    JSHandle<EcmaString> inferTableName = factory->NewFromASCII(TSTypeTable::INFER_TABLE_NAME);
    mTable->Set(thread, GetAmiPathOffset(INFER_TABLE_ID), inferTableName);
    mTable->Set(thread, GetSortIdOffset(INFER_TABLE_ID), JSTaggedValue(INFER_TABLE_ID));
    JSHandle<TSTypeTable> inferTable = factory->NewTSTypeTable(0);
    mTable->Set(thread, GetTSTypeTableOffset(INFER_TABLE_ID), inferTable);
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
        if (EcmaString::StringsAreEqual(*amiPath, *valueString)) {
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

JSHandle<TSTypeTable> TSModuleTable::GenerateBuiltinsTypeTable(JSThread *thread)
{
    CString builtinsDTSFileName = thread->GetEcmaVM()->GetTSManager()->GetBuiltinsDTS();
    JSPandaFile *jsPandaFile = JSPandaFileManager::GetInstance()->OpenJSPandaFile(builtinsDTSFileName);
    if (jsPandaFile == nullptr) {
        LOG_COMPILER(ERROR) << "load builtins.d.ts failed";
        return JSHandle<TSTypeTable>();
    }

    CVector<JSHandle<EcmaString>> vec;
    JSHandle<TSTypeTable> builtinsTypeTable = TSTypeTable::GenerateTypeTable(thread, jsPandaFile, BUILTINS_TABLE_ID,
                                                                             vec);
    return builtinsTypeTable;
}
} // namespace panda::ecmascript
