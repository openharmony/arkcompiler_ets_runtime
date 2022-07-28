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

#include "ecmascript/ts_types/ts_type_table.h"

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ts_types/ts_manager.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"
#include "ecmascript/ts_types/ts_type_parser.h"
#include "libpandafile/annotation_data_accessor.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript {
void TSTypeTable::Initialize(JSThread *thread, const JSPandaFile *jsPandaFile,
                             CVector<JSHandle<EcmaString>> &recordImportModules)
{
    EcmaVM *vm = thread->GetEcmaVM();
    TSManager *tsManager = vm->GetTSManager();
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<TSTypeTable> tsTypeTable = GenerateTypeTable(thread, jsPandaFile, recordImportModules);

    // Set TStypeTable -> GlobleModuleTable
    JSHandle<EcmaString> fileName = factory->NewFromUtf8(jsPandaFile->GetJSPandaFileDesc());
    tsManager->AddTypeTable(JSHandle<JSTaggedValue>(tsTypeTable), fileName);

    TSTypeTable::LinkClassType(thread, tsTypeTable);
    tsManager->GenerateStaticHClass(tsTypeTable);

    // management dependency module
    while (recordImportModules.size() > 0) {
        CString filename = ConvertToString(recordImportModules.back().GetTaggedValue());
        recordImportModules.pop_back();
        const JSPandaFile *moduleFile = JSPandaFileManager::GetInstance()->OpenJSPandaFile(filename.c_str());
        ASSERT(moduleFile != nullptr);
        TSTypeTable::Initialize(thread, moduleFile, recordImportModules);
    }
}

JSHandle<TSTypeTable> TSTypeTable::GenerateTypeTable(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                     CVector<JSHandle<EcmaString>> &recordImportModules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();

    // read type summary literal
    uint32_t summaryIndex = jsPandaFile->GetTypeSummaryIndex();
    JSHandle<TaggedArray> summaryLiteral = LiteralDataExtractor::GetDatasIgnoreType(thread, jsPandaFile, summaryIndex);
    ASSERT_PRINT(summaryLiteral->Get(TYPE_KIND_INDEX_IN_LITERAL).GetInt() == 0, "can not read type summary literal");

    uint32_t numTypes = static_cast<uint32_t>(summaryLiteral->Get(NUM_OF_TYPES_INDEX_IN_SUMMARY_LITREAL).GetInt());
    JSHandle<TSTypeTable> table = factory->NewTSTypeTable(numTypes);
    JSHandle<EcmaString> fileName = factory->NewFromUtf8(jsPandaFile->GetJSPandaFileDesc());
    uint32_t moduleId = tsManager->GetNextModuleId();

    TSTypeParser typeParser(thread->GetEcmaVM(), moduleId, fileName, recordImportModules);
    for (uint32_t idx = 1; idx <= numTypes; ++idx) {
        JSHandle<TaggedArray> typeLiteral = LiteralDataExtractor::GetDatasIgnoreType(thread, jsPandaFile,
                                                                                     idx + summaryIndex);
        JSHandle<JSTaggedValue> type = typeParser.ParseType(typeLiteral);
        typeParser.SetTypeRef(type, idx);
        table->Set(thread, idx, type);
    }
    recordImportModules = typeParser.GetImportModules();

    table->SetExportValueTable(thread, *jsPandaFile->GetPandaFile());
    return table;
}

GlobalTSTypeRef TSTypeTable::GetPropertyTypeGT(JSThread *thread, JSHandle<TSTypeTable> &table, TSTypeKind typeKind,
                                               uint32_t localtypeId, JSHandle<EcmaString> propName)
{
    GlobalTSTypeRef gt = GlobalTSTypeRef::Default();
    switch (typeKind) {
        case TSTypeKind::CLASS: {
            gt = TSClassType::GetPropTypeGT(thread, table, localtypeId, propName);
            break;
        }
        case TSTypeKind::CLASS_INSTANCE: {
            gt = TSClassInstanceType::GetPropTypeGT(thread, table, localtypeId, propName);
            break;
        }
        case TSTypeKind::OBJECT: {
            JSHandle<TSObjectType> ObjectType(thread, table->Get(localtypeId));
            gt = TSObjectType::GetPropTypeGT(ObjectType, propName);
            break;
        }
        default:
            UNREACHABLE();
    }
    return gt;
}

panda_file::File::EntityId TSTypeTable::GetFileId(const panda_file::File &pf)
{
    Span<const uint32_t> classIndexes = pf.GetClasses();
    panda_file::File::EntityId fileId {0};
    CString mainMethodName = CString(ENTRY_FUNC_NAME);
    panda_file::File::StringData sd = {static_cast<uint32_t>(mainMethodName.size()),
                                       reinterpret_cast<const uint8_t *>(mainMethodName.c_str())};

    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf.IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(pf, classId);
        cda.EnumerateMethods([&fileId, &pf, &sd](panda_file::MethodDataAccessor &mda) {
            if (pf.GetStringData(mda.GetNameId()) == sd) {
                fileId = mda.GetMethodId();
            }
        });
    }
    return fileId;
}

JSHandle<TaggedArray> TSTypeTable::GetExportTableFromPandFile(JSThread *thread, const panda_file::File &pf)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    panda_file::File::EntityId fileId = GetFileId(pf);
    panda_file::MethodDataAccessor mda(pf, fileId);

    CVector<CString> exportTable;
    const char *symbols;
    const char *symbolTypes;
    auto *fileName = pf.GetFilename().c_str();
    if (::strcmp(BUILTINS_TABLE_NAME, fileName) == 0) {
        symbols = DECLARED_SYMBOLS;
        symbolTypes = DECLARED_SYMBOL_TYPES;
    } else {
        symbols = EXPORTED_SYMBOLS;
        symbolTypes = EXPORTED_SYMBOL_TYPES;
    }

    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
    panda_file::AnnotationDataAccessor ada(pf, annotation_id);
    auto *annotationName = reinterpret_cast<const char *>(pf.GetStringData(ada.GetClassId()).data);
    ASSERT(annotationName != nullptr);
    if (::strcmp("L_ESTypeAnnotation;", annotationName) == 0) {
        uint32_t length = ada.GetCount();
        for (uint32_t i = 0; i < length; i++) {
            panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
            auto *elemName = reinterpret_cast<const char *>(pf.GetStringData(adae.GetNameId()).data);
            uint32_t elemCount = adae.GetArrayValue().GetCount();
            ASSERT(elemName != nullptr);
            if (::strcmp(symbols, elemName) == 0) { // symbols -> ["A", "B", "C"]
                for (uint32_t j = 0; j < elemCount; ++j) {
                    auto valueEntityId = adae.GetArrayValue().Get<panda_file::File::EntityId>(j);
                    auto *valueStringData = reinterpret_cast<const char *>(pf.GetStringData(valueEntityId).data);
                    CString target = ConvertToString(std::string(valueStringData));
                    exportTable.push_back(target);
                }
            }
            if (::strcmp(symbolTypes, elemName) == 0) { // symbolTypes -> [51, 52, 53]
                for (uint32_t k = 0; k < elemCount; ++k) {
                    auto value = adae.GetArrayValue().Get<panda_file::File::EntityId>(k).GetOffset();
                    CString typeId = ToCString(value);
                    exportTable.push_back(typeId);
                }
            }
        }
    }
    });

    uint32_t length = exportTable.size();
    JSHandle<TaggedArray> exportArray = factory->NewTaggedArray(length);
    for (uint32_t i = 0; i < length; i ++) {
        JSHandle<EcmaString> typeIdString = factory->NewFromUtf8(exportTable[i]);
        exportArray->Set(thread, i, typeIdString);
    }
    return exportArray;
}

JSHandle<TaggedArray> TSTypeTable::GetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable)
{
    int index = static_cast<int>(typeTable->GetLength()) - 1;
    JSHandle<TaggedArray> exportValueTable(thread, typeTable->Get(index));
    return exportValueTable;
}

void TSTypeTable::SetExportValueTable(JSThread *thread, const panda_file::File &pf)
{
    JSHandle<TaggedArray> exportValueTable = GetExportTableFromPandFile(thread, pf);
    if (exportValueTable->GetLength() != 0) { // add exprotValueTable to tSTypeTable if isn't empty
        Set(thread, GetLength() - 1, exportValueTable);
    }
}

void TSTypeTable::CheckModule(JSThread *thread, const TSManager* tsManager,  const JSHandle<EcmaString> target,
                              CVector<JSHandle<EcmaString>> &recordImportModules)
{
    int32_t entry = tsManager->GetTSModuleTable()->GetGlobalModuleID(thread, target);
    if (entry == -1) {
        bool flag = false;
        for (const auto it : recordImportModules) {
            if (EcmaString::StringsAreEqual(*it, *target)) {
                flag = true;
                break;
            }
        }
        if (!flag) {
            recordImportModules.push_back(target);
        }
    }
}

JSHandle<EcmaString> TSTypeTable::GenerateVarNameAndPath(JSThread *thread, JSHandle<EcmaString> importPath,
                                                         JSHandle<EcmaString> fileName,
                                                         CVector<JSHandle<EcmaString>> &recordImportModules)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    TSManager *tsManager = ecmaVm->GetTSManager();

    JSHandle<EcmaString> targetVarName = tsManager->GenerateImportVar(importPath); // #A#./A -> A
    JSHandle<EcmaString> relativePath = tsManager->GenerateImportRelativePath(importPath); // #A#./A -> ./A
    JSHandle<EcmaString> fullPathEcmaStr = tsManager->GenerateAmiPath(fileName, relativePath); // ./A -> XXX/XXX/A
    CheckModule(thread, tsManager, fullPathEcmaStr, recordImportModules);

    CString fullPath = ConvertToString(fullPathEcmaStr.GetTaggedValue());
    CString target = ConvertToString(targetVarName.GetTaggedValue());
    CString targetNameAndPath = "#" + target + "#" + fullPath; // #A#XXX/XXX/A

    JSHandle<EcmaString> targetNameAndPathEcmaStr = factory->NewFromUtf8(targetNameAndPath);
    return targetNameAndPathEcmaStr;
}

int TSTypeTable::GetTypeKindFromFileByLocalId(JSThread *thread, const JSPandaFile *jsPandaFile, int localId)
{
    JSHandle<TaggedArray> literal = LiteralDataExtractor::GetDatasIgnoreType(thread, jsPandaFile, localId);
    return literal->Get(TYPE_KIND_INDEX_IN_LITERAL).GetInt();
}

JSHandle<TSTypeTable> TSTypeTable::PushBackTypeToInferTable(JSThread *thread, JSHandle<TSTypeTable> &table,
                                                            const JSHandle<TSType> &type)
{
    uint32_t capacity = table->GetLength();  // can't be 0 due to RESERVE_TABLE_LENGTH
    uint32_t numberOfTypes = static_cast<uint32_t>(table->GetNumberOfTypes());
    if (UNLIKELY(capacity <= numberOfTypes + RESERVE_TABLE_LENGTH)) {
        table = JSHandle<TSTypeTable>(TaggedArray::SetCapacity(thread, JSHandle<TaggedArray>(table),
                                                               capacity * INCREASE_CAPACITY_RATE));
    }

    table->Set(thread, numberOfTypes + 1, type);
    table->SetNumberOfTypes(thread, numberOfTypes + 1);

    return table;
}

void TSTypeTable::LinkClassType(JSThread *thread, JSHandle<TSTypeTable> table)
{
    int numTypes = table->GetNumberOfTypes();
    JSMutableHandle<JSTaggedValue> type(thread, JSTaggedValue::Undefined());
    for (int i = 1; i <= numTypes; ++i) {
        type.Update(table->Get(i));
        if (!type->IsTSClassType()) {
            continue;
        }

        JSHandle<TSClassType> classType(type);
        if (classType->GetHasLinked()) {  // has linked
            continue;
        }

        JSHandle<TSClassType> extendClassType = classType->GetExtendClassType(thread);
        MergeClassFiled(thread, classType, extendClassType);
    }
}

void TSTypeTable::MergeClassFiled(JSThread *thread, JSHandle<TSClassType> classType,
                                  JSHandle<TSClassType> extendClassType)
{
    ASSERT(!classType->GetHasLinked());

    if (!extendClassType->GetHasLinked()) {
        MergeClassFiled(thread, extendClassType, extendClassType->GetExtendClassType(thread));
    }

    ASSERT(extendClassType->GetHasLinked());

    JSHandle<TSObjectType> field(thread, classType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> layout(thread, field->GetObjLayoutInfo());
    uint32_t numSelfTypes = layout->NumberOfElements();

    JSHandle<TSObjectType> extendField(thread, extendClassType->GetInstanceType());
    JSHandle<TSObjLayoutInfo> extendLayout(thread, extendField->GetObjLayoutInfo());
    uint32_t numExtendTypes = extendLayout->NumberOfElements();

    uint32_t numTypes = numSelfTypes + numExtendTypes;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TSObjLayoutInfo> newLayout = factory->CreateTSObjLayoutInfo(numTypes);

    uint32_t index = 0;
    while (index < numExtendTypes) {
        JSTaggedValue key = extendLayout->GetKey(index);
        JSTaggedValue type = extendLayout->GetTypeId(index);
        newLayout->SetKey(thread, index, key, type);
        index++;
    }

    index = 0;
    while (index < numSelfTypes) {
        JSTaggedValue key = layout->GetKey(index);
        JSTaggedValue type = layout->GetTypeId(index);
        newLayout->SetKey(thread, numExtendTypes + index, key, type);
        index++;
    }

    field->SetObjLayoutInfo(thread, newLayout);
    classType->SetHasLinked(true);
}
} // namespace panda::ecmascript
