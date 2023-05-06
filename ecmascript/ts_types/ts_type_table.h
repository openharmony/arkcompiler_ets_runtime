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

#ifndef ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_H
#define ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_H

#include "ecmascript/ts_types/ts_type.h"

namespace panda::ecmascript {
class TSTypeTable : public TaggedArray {
public:
    static constexpr size_t RESERVE_TABLE_LENGTH = 2; // for reserve length and exportTable
    static constexpr size_t NUMBER_OF_TYPES_INDEX = 0;
    static constexpr size_t INCREASE_CAPACITY_RATE = 2;
    static constexpr const char* PRIMITIVE_TABLE_NAME = "primitiveTypes";
    static constexpr const char* BUILTINS_TABLE_NAME = "lib_ark_builtins.d";
    static constexpr const char* INFER_TABLE_NAME = "inferTypes";
    static constexpr const char* RUNTIME_TABLE_NAME = "runtimeTypes";
    static constexpr const char* GENERICS_TABLE_NAME = "genericsTypes";

    inline static TSTypeTable *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<TSTypeTable *>(object);
    }

    static JSHandle<JSTaggedValue> GetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable);

    static void SetExportValueTable(JSThread *thread, JSHandle<TSTypeTable> typeTable,
                                    JSHandle<TaggedArray> exportValueTable);

    inline int GetNumberOfTypes() const
    {
        return TaggedArray::Get(NUMBER_OF_TYPES_INDEX).GetInt();
    }

    inline void SetNumberOfTypes(JSThread *thread, uint32_t num = 0)
    {
        TaggedArray::Set(thread, NUMBER_OF_TYPES_INDEX, JSTaggedValue(num));
    }

    static JSHandle<TSTypeTable> PushBackTypeToTable(JSThread *thread, JSHandle<TSTypeTable> &table,
                                                     const JSHandle<TSType> &type);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_TABLE_H
