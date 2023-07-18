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

namespace panda::ecmascript {
JSHandle<JSTaggedValue> TSTypeTable::GetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable)
{
    uint32_t index = typeTable->GetLength() - 1;
    JSHandle<JSTaggedValue> exportValueTable(thread, typeTable->Get(index));
    return exportValueTable;
}

void TSTypeTable::SetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable,
                                      JSHandle<TaggedArray> exportValueTable)
{
    if (exportValueTable->GetLength() != 0) { // add exprotValueTable to tSTypeTable if isn't empty
        typeTable->Set(thread, typeTable->GetLength() - 1, exportValueTable);
    }
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
