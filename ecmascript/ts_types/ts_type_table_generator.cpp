/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/ts_types/ts_type_table_generator.h"

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/type_literal_extractor.h"

namespace panda::ecmascript {
void TSTypeTableGenerator::GenerateDefaultTSTypeTables()
{
    JSHandle<EcmaString> primitiveTableName = factory_->NewFromASCII(TSTypeTable::PRIMITIVE_TABLE_NAME);
    AddTypeTable(primitiveTableName);

    GenerateBuiltinsTypeTable();

    JSHandle<EcmaString> inferTableName = factory_->NewFromASCII(TSTypeTable::INFER_TABLE_NAME);
    AddTypeTable(inferTableName);

    JSHandle<EcmaString> runtimeTableName = factory_->NewFromASCII(TSTypeTable::RUNTIME_TABLE_NAME);
    AddTypeTable(runtimeTableName);

    InitRuntimeTypeTable();

    JSHandle<EcmaString> genericsTableName = factory_->NewFromASCII(TSTypeTable::GENERICS_TABLE_NAME);
    AddTypeTable(genericsTableName);
}

void TSTypeTableGenerator::GenerateBuiltinsTypeTable()
{
    JSHandle<EcmaString> builtinsTableName = factory_->NewFromASCII(TSTypeTable::BUILTINS_TABLE_NAME);
    if (LIKELY(tsManager_->IsBuiltinsDTSEnabled())) {
        tsManager_->GenerateBuiltinSummary();
        uint32_t numOfTypes = tsManager_->GetBuiltinOffset(static_cast<uint32_t>(BuiltinTypeId::NUM_INDEX_IN_SUMMARY));
        AddTypeTable(builtinsTableName, numOfTypes);
    } else {
        AddTypeTable(builtinsTableName);
    }
}

JSHandle<TSTypeTable> TSTypeTableGenerator::GetOrGenerateTSTypeTable(const JSPandaFile *jsPandaFile,
                                                                     const CString &recordName,
                                                                     uint32_t moduleId)
{
    if (UNLIKELY(moduleId != static_cast<uint32_t>(GetNextModuleId()))) {
        return tsManager_->GetTSTypeTable(moduleId);
    }
    JSHandle<EcmaString> recordNameStr = factory_->NewFromUtf8(recordName);
    // when PGO is enabled, no matter whether the abc file is a '.js' or a '.ts' file, it may contain PGO GT
    uint32_t typeNum = tsManager_->GetPGOGTCountByRecordName(recordName);
    if (jsPandaFile->HasTSTypes(recordName)) {
        // only '.ts' file has type literal and can get number of type from it
        TypeSummaryExtractor summExtractor(jsPandaFile, recordName);
        typeNum += summExtractor.GetNumOfTypes();
    }
    JSHandle<TSTypeTable> table = AddTypeTable(recordNameStr, typeNum);
    return table;
}

uint32_t TSTypeTableGenerator::TryGetModuleId(const CString &recordName) const
{
    JSHandle<EcmaString> recordNameStr = factory_->NewFromUtf8(recordName);
    int moduleIdBaseOnFile = GetTSModuleTable()->GetGlobalModuleID(thread_, recordNameStr);
    if (moduleIdBaseOnFile != TSModuleTable::NOT_FOUND) {
        return moduleIdBaseOnFile;
    }
    return static_cast<uint32_t>(GetNextModuleId());
}

uint32_t TSTypeTableGenerator::TryGetLocalId(const JSHandle<TSTypeTable> &table) const
{
    return table->GetNumberOfTypes() + 1;
}

JSHandle<TSTypeTable> TSTypeTableGenerator::AddTypeTable(const JSHandle<EcmaString> &recordNameStr,
                                                         uint32_t numTypes)
{
    JSHandle<TSTypeTable> typeTable = factory_->NewTSTypeTable(numTypes);

    JSHandle<TSModuleTable> table = GetTSModuleTable();
    int numberOfTSTypeTable = table->GetNumberOfTSTypeTables();
    if (TSModuleTable::GetTSTypeTableOffset(numberOfTSTypeTable) > table->GetLength()) {
        table = JSHandle<TSModuleTable>(TaggedArray::SetCapacity(thread_, JSHandle<TaggedArray>(table),
            table->GetLength() * TSModuleTable::INCREASE_CAPACITY_RATE));
    }
    // add ts type table
    table->SetNumberOfTSTypeTables(thread_, numberOfTSTypeTable + 1);
    table->Set(thread_, TSModuleTable::GetModuleRequestOffset(numberOfTSTypeTable), recordNameStr);
    table->Set(thread_, TSModuleTable::GetSortIdOffset(numberOfTSTypeTable), JSTaggedValue(numberOfTSTypeTable));
    table->Set(thread_, TSModuleTable::GetTSTypeTableOffset(numberOfTSTypeTable), typeTable);
    tsManager_->SetTSModuleTable(table);
    return typeTable;
}

void TSTypeTableGenerator::InitRuntimeTypeTable()
{
    JSHandle<TSTypeTable> runtimeTable = tsManager_->GetRuntimeTable();

    // add IteratorResult GT
    JSHandle<JSTaggedValue> valueString = thread_->GlobalConstants()->GetHandledValueString();
    JSHandle<JSTaggedValue> doneString = thread_->GlobalConstants()->GetHandledDoneString();
    std::vector<JSHandle<JSTaggedValue>> prop { valueString, doneString };
    std::vector<GlobalTSTypeRef> propType { GlobalTSTypeRef::Default(), kungfu::GateType::BooleanType().GetGTRef() };

    JSHandle<TSObjectType> iteratorResultType = factory_->NewTSObjectType(prop.size());
    JSHandle<TSTypeTable> newRuntimeTable = TSTypeTable::PushBackTypeToTable(thread_,
        runtimeTable, JSHandle<TSType>(iteratorResultType));
    GlobalTSTypeRef iteratorResultGt(static_cast<uint32_t>(ModuleTableIdx::RUNTIME),
                                     newRuntimeTable->GetNumberOfTypes());
    iteratorResultType->SetGT(iteratorResultGt);

    JSHandle<TSObjLayoutInfo> layoutInfo(thread_, iteratorResultType->GetObjLayoutInfo());
    FillLayoutTypes(layoutInfo, prop, propType);

    // add IteratorFunction GT
    JSHandle<TSFunctionType> iteratorFunctionType = factory_->NewTSFunctionType(0);
    newRuntimeTable = TSTypeTable::PushBackTypeToTable(thread_, runtimeTable, JSHandle<TSType>(iteratorFunctionType));
    GlobalTSTypeRef functiontGt(static_cast<uint32_t>(ModuleTableIdx::RUNTIME),
                                                      newRuntimeTable->GetNumberOfTypes());
    iteratorFunctionType->SetGT(functiontGt);
    iteratorFunctionType->SetReturnGT(iteratorResultGt);

    // add TSIterator GT
    JSHandle<JSTaggedValue> nextString = thread_->GlobalConstants()->GetHandledNextString();
    JSHandle<JSTaggedValue> throwString = thread_->GlobalConstants()->GetHandledThrowString();
    JSHandle<JSTaggedValue> returnString = thread_->GlobalConstants()->GetHandledReturnString();
    std::vector<JSHandle<JSTaggedValue>> iteratorProp { nextString, throwString, returnString };
    std::vector<GlobalTSTypeRef> iteratorPropType { functiontGt, functiontGt, functiontGt };

    JSHandle<TSObjectType> iteratorType = factory_->NewTSObjectType(iteratorProp.size());
    newRuntimeTable = TSTypeTable::PushBackTypeToTable(thread_, runtimeTable, JSHandle<TSType>(iteratorType));
    GlobalTSTypeRef iteratorGt(static_cast<uint32_t>(ModuleTableIdx::RUNTIME), newRuntimeTable->GetNumberOfTypes());
    iteratorType->SetGT(iteratorGt);

    JSHandle<TSObjLayoutInfo> iteratorLayoutInfo(thread_, iteratorType->GetObjLayoutInfo());
    FillLayoutTypes(iteratorLayoutInfo, iteratorProp, iteratorPropType);

    tsManager_->SetRuntimeTable(newRuntimeTable);
}

void TSTypeTableGenerator::FillLayoutTypes(const JSHandle<TSObjLayoutInfo> &layout,
                                           const std::vector<JSHandle<JSTaggedValue>> &prop,
                                           const std::vector<GlobalTSTypeRef> &propType)
{
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread_, JSTaggedValue::Undefined());
    for (uint32_t index = 0; index < prop.size(); index++) {
        key.Update(prop[index]);
        ASSERT(key->IsString());
        value.Update(JSTaggedValue(propType[index].GetType()));
        layout->AddProperty(thread_, key.GetTaggedValue(), value.GetTaggedValue());
    }
}
}  // namespace panda::ecmascript
