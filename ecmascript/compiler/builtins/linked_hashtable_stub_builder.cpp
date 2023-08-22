/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/linked_hashtable_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_map.h"

namespace panda::ecmascript::kungfu {

template<typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Create(int32_t numberOfElements)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    // new LinkedHashTable
    GateRef length = CalNewTaggedArrayLength(numberOfElements);
    NewObjectStubBuilder newBuilder(this);
    GateRef array = newBuilder.NewTaggedArray(glue_, length);

    Label noException(env);
    Branch(TaggedIsException(array), &exit, &noException);
    Bind(&noException);
    {
        // SetNumberOfElements
        SetNumberOfElements(array, Int32(0));
        // SetNumberOfDeletedElements
        SetNumberOfDeletedElements(array, Int32(0));
        // SetCapacity
        SetCapacity(array, Int32(numberOfElements));
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
    return array;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::Create(int32_t);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::Create(int32_t);

template<typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Clear(GateRef linkedTable)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label setLinked(env);

    GateRef newTable = Create(LinkedHashTableType::MIN_CAPACITY);
    Label noException(env);
    Branch(TaggedIsException(newTable), &exit, &noException);
    Bind(&noException);

    GateRef cap = GetCapacity(linkedTable);
    Label capGreaterZero(env);
    Branch(Int32GreaterThan(cap, Int32(0)), &capGreaterZero, &exit);
    Bind(&capGreaterZero);
    {
        // NextTable
        SetNextTable(linkedTable, newTable);
        // SetNumberOfDeletedElements
        SetNumberOfDeletedElements(linkedTable, Int32(-1));
        Jump(&exit);
    }

    Bind(&exit);
    env->SubCfgExit();
    return newTable;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::Clear(GateRef);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::Clear(GateRef);
}  // namespace panda::ecmascript::kungfu
