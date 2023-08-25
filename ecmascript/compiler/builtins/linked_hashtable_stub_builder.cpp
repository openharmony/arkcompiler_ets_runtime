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
template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::GetDeletedElementsAt(
    GateRef linkedTable, GateRef entry)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    Label exit(env);
    DEFVARIABLE(res, VariableType::INT32(), Int32(0));
    DEFVARIABLE(currentEntry, VariableType::INT32(), Int32Sub(entry, Int32(1)));
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32GreaterThanOrEqual(*currentEntry, Int32(0)), &next, &loopExit);
        Bind(&next);
        GateRef key = GetKey(linkedTable, *currentEntry);
        Label hole(env);
        Branch(TaggedIsHole(key), &hole, &loopEnd);
        Bind(&hole);
        {
            GateRef deletedNum = GetDeletedNum(linkedTable, *currentEntry);
            res = deletedNum;
            Jump(&exit);
        }
    }
    Bind(&loopEnd);
    currentEntry = Int32Sub(*currentEntry, Int32(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);
    Jump(&exit);
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

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

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::ForEach(GateRef thisValue,
    GateRef srcLinkedTable, GateRef numArgs)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    // caller checked callbackFnHandle callable
    GateRef callbackFnHandle = GetCallArg0(numArgs);
    GateRef thisArg = GetCallArg1(numArgs);
    DEFVARIABLE(linkedTable, VariableType::JS_ANY(), srcLinkedTable);

    GateRef numberOfElements = GetNumberOfElements(*linkedTable);
    GateRef numberOfDeletedElements = GetNumberOfDeletedElements(*linkedTable);
    GateRef tmpTotalElements = Int32Add(numberOfElements, numberOfDeletedElements);
    DEFVARIABLE(totalElements, VariableType::INT32(), tmpTotalElements);
    DEFVARIABLE(index, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32LessThan(*index, *totalElements), &next, &loopExit);
        Bind(&next);
        GateRef valueIndex = *index;

        GateRef key = GetKey(*linkedTable, *index);
        index = Int32Add(*index, Int32(1));
        Label keyNotHole(env);
        Branch(TaggedIsHole(key), &loopEnd, &keyNotHole);
        Bind(&keyNotHole);

        GateRef value = key;
        if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashMap>) {
            value = GetValue(*linkedTable, valueIndex);
        }
        Label hasException(env);
        Label notHasException(env);
        GateRef retValue = JSCallDispatch(glue_, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0,
            Circuit::NullGate(), JSCallMode::CALL_THIS_ARG3_WITH_RETURN, { thisArg, value, key, thisValue });
        Branch(HasPendingException(glue_), &hasException, &notHasException);
        Bind(&hasException);
        {
            res = retValue;
            Jump(&exit);
        }
        Bind(&notHasException);
        {
            // Maybe add or delete, get next table
            GateRef tmpNextTable = GetNextTable(*linkedTable);
            DEFVARIABLE(nextTable, VariableType::JS_ANY(), tmpNextTable);
            Label loopHead1(env);
            Label loopEnd1(env);
            Label next1(env);
            Label loopExit1(env);
            Jump(&loopHead1);
            LoopBegin(&loopHead1);
            {
                Branch(TaggedIsHole(*nextTable), &loopExit1, &next1);
                Bind(&next1);
                GateRef deleted = GetDeletedElementsAt(*linkedTable, *index);
                index = Int32Sub(*index, deleted);
                linkedTable = *nextTable;
                nextTable = GetNextTable(*linkedTable);
                Jump(&loopEnd1);
            }
            Bind(&loopEnd1);
            LoopEnd(&loopHead1);
            Bind(&loopExit1);
            // update totalElements
            GateRef numberOfEle = GetNumberOfElements(*linkedTable);
            GateRef numberOfDeletedEle = GetNumberOfDeletedElements(*linkedTable);
            totalElements = Int32Add(numberOfEle, numberOfDeletedEle);
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&loopExit);
    Jump(&exit);

    Bind(&exit);
    env->SubCfgExit();
    return *res;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::ForEach(GateRef thisValue,
    GateRef linkedTable, GateRef numArgs);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::ForEach(GateRef thisValue,
    GateRef linkedTable, GateRef numArgs);
}  // namespace panda::ecmascript::kungfu
