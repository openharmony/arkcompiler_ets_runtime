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

#include "ecmascript/js_tagged_value.h"
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
JSHandle<TSTypeTable> TSTypeTable::GenerateTypeTable(JSThread *thread, const JSPandaFile *jsPandaFile,
                                                     const CString &recordName, uint32_t moduleId,
                                                     CVector<JSHandle<EcmaString>> &recordImportModules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // read type summary literal
    // struct of summaryLiteral: {numTypes, literalOffset0, literalOffset1, ...}
    panda_file::File::EntityId summaryOffset(jsPandaFile->GetTypeSummaryOffset(recordName));
    JSHandle<TaggedArray> summaryLiteral = LiteralDataExtractor::GetTypeLiteral(thread, jsPandaFile, summaryOffset);
    uint32_t numTypes = static_cast<uint32_t>(summaryLiteral->Get(NUM_OF_TYPES_INDEX_IN_SUMMARY_LITREAL).GetInt());

    FillLiteralOffsetGTMap(thread, jsPandaFile, moduleId, summaryLiteral, numTypes);

    // Initialize a empty TSTypeTable with length of (numTypes + RESERVE_TABLE_LENGTH)
    JSHandle<TSTypeTable> table = factory->NewTSTypeTable(numTypes);
    TSTypeParser typeParser(thread->GetEcmaVM(), jsPandaFile, recordImportModules);
    for (uint32_t idx = 1; idx <= numTypes; ++idx) {
        panda_file::File::EntityId offset(summaryLiteral->Get(idx).GetInt());
        JSHandle<TaggedArray> typeLiteral = LiteralDataExtractor::GetTypeLiteral(thread, jsPandaFile, offset);
        if (typeLiteral->GetLength() == 0) {  // typeLiteral maybe hole in d.abc
            continue;
        }

        JSHandle<JSTaggedValue> type = typeParser.ParseType(typeLiteral);
        typeParser.SetTypeGT(type, moduleId, idx);
        table->Set(thread, idx, type);
    }

    // Get amiPaths of all of the modules/files imported by current module/file
    recordImportModules = typeParser.GetImportModules();
    // Generate export-table and set to the last position of this type-table
    SetExportValueTable(thread, jsPandaFile, table);
    return table;
}

void TSTypeTable::FillLiteralOffsetGTMap(JSThread *thread, const JSPandaFile *jsPandaFile, uint32_t moduleId,
                                         JSHandle<TaggedArray> summaryLiteral, uint32_t numTypes)
{
    TSManager *tsManager = thread->GetEcmaVM()->GetTSManager();

    for (uint32_t idx = 1; idx <= numTypes; ++idx) {
        panda_file::File::EntityId offset(summaryLiteral->Get(idx).GetInt());
        GlobalTSTypeRef gt = GlobalTSTypeRef(moduleId, idx);
        tsManager->AddElementToLiteralOffsetGTMap(jsPandaFile, offset, gt);
    }
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

JSHandle<TaggedArray> TSTypeTable::GenerateExportTableFromPandaFile(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();

    // Read export-data from annotation of the .abc File
    JSHandle<TaggedArray> exportTable = GetExportDataFromPandaFile(thread, jsPandaFile);
    uint32_t length = exportTable->GetLength();

    // Replace typeIds with GT
    JSTaggedValue target;
    for (uint32_t i = 1; i < length; i += ITEM_SIZE) {
        target = exportTable->Get(i);
        // Create GT based on typeId, and wrapped it into a JSTaggedValue
        uint32_t typeId = static_cast<uint32_t>(target.GetInt());
        GlobalTSTypeRef typeGT = TSTypeParser::CreateGT(ecmaVm, jsPandaFile, typeId);
        // Set the wrapped GT to exportTable
        exportTable->Set(thread, i, JSTaggedValue(typeGT.GetType()));
    }

    return exportTable;
}

JSHandle<TaggedArray> TSTypeTable::GetExportDataFromPandaFile(JSThread *thread, const JSPandaFile *jsPandaFile)
{
    const panda_file::File &pf = *jsPandaFile->GetPandaFile();
    panda_file::File::EntityId fileId = GetFileId(pf);
    panda_file::MethodDataAccessor mda(pf, fileId);

    const char *symbolTypes;
    auto *fileName = pf.GetFilename().c_str();
    if (::strcmp(BUILTINS_TABLE_NAME, fileName) == 0) {
        symbolTypes = DECLARED_SYMBOL_TYPES;
    } else {
        symbolTypes = EXPORTED_SYMBOL_TYPES;
    }

    JSHandle<TaggedArray> typeOfExportedSymbols(thread, thread->GlobalConstants()->GetEmptyArray());
    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
        panda_file::AnnotationDataAccessor ada(pf, annotation_id);
        auto *annotationName = reinterpret_cast<const char *>(pf.GetStringData(ada.GetClassId()).data);
        ASSERT(annotationName != nullptr);
        if (::strcmp("L_ESTypeAnnotation;", annotationName) != 0) {
            return;
        }
        uint32_t length = ada.GetCount();
        for (uint32_t i = 0; i < length; i++) {
            panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
            auto *elemName = reinterpret_cast<const char *>(pf.GetStringData(adae.GetNameId()).data);
            ASSERT(elemName != nullptr);

            if (::strcmp(symbolTypes, elemName) != 0) {
                continue;
            }

            panda_file::ScalarValue sv = adae.GetScalarValue();
            panda_file::File::EntityId literalOffset(sv.GetValue());
            typeOfExportedSymbols = LiteralDataExtractor::GetTypeLiteral(thread, jsPandaFile, literalOffset);
            // typeOfExprotedSymbols: "symbol0", "type0", "symbol1", "type1" ...
        }
    });

    return typeOfExportedSymbols;
}

JSHandle<TaggedArray> TSTypeTable::GetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable)
{
    int index = static_cast<int>(typeTable->GetLength()) - 1;
    JSHandle<TaggedArray> exportValueTable(thread, typeTable->Get(index));
    return exportValueTable;
}

void TSTypeTable::SetExportValueTable(JSThread *thread, const JSPandaFile *jsPandaFile, JSHandle<TSTypeTable> typeTable)
{
    // Get export-type-info from specified file, and put them into an tagged array
    JSHandle<TaggedArray> exportValueTable = GenerateExportTableFromPandaFile(thread, jsPandaFile);

    if (exportValueTable->GetLength() != 0) { // add exprotValueTable to tSTypeTable if isn't empty
        typeTable->Set(thread, typeTable->GetLength() - 1, exportValueTable);
    }
}

void TSTypeTable::CheckModule(JSThread *thread, const TSManager* tsManager, const JSHandle<EcmaString> target,
                              CVector<JSHandle<EcmaString>> &recordImportModules)
{
    int32_t entry = tsManager->GetTSModuleTable()->GetGlobalModuleID(thread, target);
    if (entry == -1) {
        bool flag = false;
        for (const auto it : recordImportModules) {
            if (EcmaStringAccessor::StringsAreEqual(*it, *target)) {
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
    // Check and fill recordImportModules
    CheckModule(thread, tsManager, fullPathEcmaStr, recordImportModules);

    CString fullPath = ConvertToString(fullPathEcmaStr.GetTaggedValue());
    CString target = ConvertToString(targetVarName.GetTaggedValue());
    CString targetNameAndPath = "#" + target + "#" + fullPath; // #A#XXX/XXX/A

    JSHandle<EcmaString> targetNameAndPathEcmaStr = factory->NewFromUtf8(targetNameAndPath);
    return targetNameAndPathEcmaStr;
}

JSHandle<TSTypeTable> TSTypeTable::PushBackTypeToTable(JSThread *thread, JSHandle<TSTypeTable> &table,
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
} // namespace panda::ecmascript
