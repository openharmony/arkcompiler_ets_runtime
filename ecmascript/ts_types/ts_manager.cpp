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

TSManager::TSManager(EcmaVM *vm) : vm_(vm), thread_(vm_->GetJSThread()), factory_(vm_->GetFactory())
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

    int localId = newITable->GetNumberOfTypes() - 1;
    GlobalTSTypeRef gt = GlobalTSTypeRef(TSModuleTable::INFER_TABLE_ID, localId);
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
    uint64_t length = constantStringTable_.size();
    for (uint64_t i = 0; i < length; i++) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(constantStringTable_.data()[i]))));
    }

    uint64_t hclassTableLength = staticHClassTable_.size();
    for (uint64_t i = 0; i < hclassTableLength; i++) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(staticHClassTable_.data()[i]))));
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

int TSManager::GetFunctionTypLength(GlobalTSTypeRef gt) const
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

GlobalTSTypeRef TSManager::GetFuncReturnValueTypeGT(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::FUNCTION);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSFunctionType());
    JSHandle<TSFunctionType> functionType = JSHandle<TSFunctionType>(tsType);
    return functionType->GetReturnGT();
}

GlobalTSTypeRef TSManager::GetArrayParameterTypeGT(GlobalTSTypeRef gt) const
{
    ASSERT(GetTypeKind(gt) == TSTypeKind::ARRAY);
    JSHandle<JSTaggedValue> tsType = GetTSType(gt);
    ASSERT(tsType->IsTSArrayType());
    JSHandle<TSArrayType> arrayType = JSHandle<TSArrayType>(tsType);
    return arrayType->GetElementGT();
}

size_t TSManager::AddConstString(JSTaggedValue string)
{
    auto it = std::find(constantStringTable_.begin(), constantStringTable_.end(), string.GetRawData());
    if (it != constantStringTable_.end()) {
        return it - constantStringTable_.begin();
    } else {
        constantStringTable_.emplace_back(string.GetRawData());
        return constantStringTable_.size() - 1;
    }
}

// add string to constantstringtable and get its index
size_t TSManager::GetStringIdx(JSHandle<JSTaggedValue> constPool, const uint16_t id)
{
    JSHandle<ConstantPool> newConstPool(thread_, constPool.GetTaggedValue());
    auto str = newConstPool->GetObjectFromCache(id);
    return AddConstString(str);
}

bool TSManager::IsTypeVerifyEnabled() const
{
    return vm_->GetJSOptions().EnableTypeInferVerify();
}

std::string TSManager::GetStdStringById(size_t index) const
{
    std::string str = GetStringById(index)->GetCString().get();
    return str;
}

void TSManager::GenerateStaticHClass(JSHandle<TSTypeTable> tsTypeTable)
{
    JSMutableHandle<TSObjectType> instanceType(thread_, JSTaggedValue::Undefined());
    for (int index = 1; index <= tsTypeTable->GetNumberOfTypes(); ++index) {
        JSTaggedValue type = tsTypeTable->Get(index);
        if (!type.IsTSClassType()) {
            continue;
        }

        TSClassType *classType = TSClassType::Cast(type.GetTaggedObject());
        instanceType.Update(classType->GetInstanceType());
        GlobalTSTypeRef gt = classType->GetGT();
        JSTaggedValue ihc = JSTaggedValue(TSObjectType::GetOrCreateHClass(thread_, instanceType));
        AddStaticHClassInCompilePhase(gt, ihc);
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
    GlobalTSTypeRef gt = GlobalTSTypeRef(gateType.GetType());
    ASSERT(gt.GetFlag() == 0);
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
    }

    CVector<JSHandle<EcmaString>> vec;
    JSHandle<TSTypeTable> builtinsTypeTable = TSTypeTable::GenerateTypeTable(thread, jsPandaFile, BUILTINS_TABLE_ID,
                                                                             vec);
    return builtinsTypeTable;
}
} // namespace panda::ecmascript
