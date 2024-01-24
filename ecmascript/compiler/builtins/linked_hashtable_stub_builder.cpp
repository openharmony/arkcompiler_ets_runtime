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
#include "ecmascript/free_object.h"

namespace panda::ecmascript::kungfu {
template <typename LinkedHashTableType, typename LinkedHashTableObject>
void LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Rehash(
    GateRef linkedTable, GateRef newTable)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);

    GateRef numberOfAllElements = Int32Add(GetNumberOfElements(linkedTable),
        GetNumberOfDeletedElements(linkedTable));

    DEFVARIABLE(desEntry, VariableType::INT32(), Int32(0));
    DEFVARIABLE(currentDeletedElements, VariableType::INT32(), Int32(0));
    SetNextTable(linkedTable, newTable);

    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);

    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32LessThan(*i, numberOfAllElements), &next, &loopExit);
        Bind(&next);

        GateRef fromIndex = EntryToIndex(linkedTable, *i);
        DEFVARIABLE(key, VariableType::JS_ANY(), GetElement(linkedTable, fromIndex));
        Label hole(env);
        Label notHole(env);
        Branch(TaggedIsHole(*key), &hole, &notHole);
        Bind(&hole);
        {
            currentDeletedElements = Int32Add(*currentDeletedElements, Int32(1));
            SetDeletedNum(linkedTable, *i, *currentDeletedElements);
            Jump(&loopEnd);
        }
        Bind(&notHole);
        {
            Label weak(env);
            Label notWeak(env);
            Branch(TaggedIsWeak(*key), &weak, &notWeak);
            Bind(&weak);
            {
                key = RemoveTaggedWeakTag(*key);
                Jump(&notWeak);
            }
            Bind(&notWeak);

            GateRef hash = GetHash(*key);
            GateRef bucket = HashToBucket(newTable, hash);
            InsertNewEntry(newTable, bucket, *desEntry);
            GateRef desIndex = EntryToIndex(newTable, *desEntry);

            Label loopHead1(env);
            Label loopEnd1(env);
            Label next1(env);
            Label loopExit1(env);
            DEFVARIABLE(j, VariableType::INT32(), Int32(0));
            Jump(&loopHead1);
            LoopBegin(&loopHead1);
            {
                Branch(Int32LessThan(*j, Int32(LinkedHashTableObject::ENTRY_SIZE)), &next1, &loopExit1);
                Bind(&next1);
                GateRef ele = GetElement(linkedTable, Int32Add(fromIndex, *j));
                SetElement(newTable, Int32Add(desIndex, *j), ele);
                Jump(&loopEnd1);
            }
            Bind(&loopEnd1);
            j = Int32Add(*j, Int32(1));
            LoopEnd(&loopHead1);
            Bind(&loopExit1);
            desEntry = Int32Add(*desEntry, Int32(1));
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);

    SetNumberOfElements(newTable, GetNumberOfElements(linkedTable));
    SetNumberOfDeletedElements(newTable, Int32(0));
    env->SubCfgExit();
}

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::GrowCapacity(
    GateRef linkedTable, GateRef numberOfAddedElements)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    Label exit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), linkedTable);

    GateRef hasSufficient = HasSufficientCapacity(linkedTable, numberOfAddedElements);
    Label grow(env);
    Branch(hasSufficient, &exit, &grow);
    Bind(&grow);
    {
        GateRef newCapacity = ComputeCapacity(Int32Add(GetNumberOfElements(linkedTable), numberOfAddedElements));
        GateRef newTable = Create(newCapacity);
        Rehash(linkedTable, newTable);
        res = newTable;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::ComputeCapacity(
    GateRef atLeastSpaceFor)
{
    if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashMap>) {
        return TaggedGetInt(CallRuntime(glue_, RTSTUB_ID(LinkedHashMapComputeCapacity), {
        IntToTaggedInt(atLeastSpaceFor) }));
    } else {
        return TaggedGetInt(CallRuntime(glue_, RTSTUB_ID(LinkedHashSetComputeCapacity), {
        IntToTaggedInt(atLeastSpaceFor) }));
    }
}

template <typename LinkedHashTableType, typename LinkedHashTableObject>
void LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::RemoveEntry(
    GateRef linkedTable, GateRef entry)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    Label exit(env);
    env->SubCfgEntry(&entryLabel);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label loopExit(env);
    GateRef index = EntryToIndex(linkedTable, entry);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32LessThan(*i, Int32(LinkedHashTableObject::ENTRY_SIZE)), &next, &loopExit);
        Bind(&next);

        GateRef idx = Int32Add(index, *i);
        SetElement(linkedTable, idx, Hole());
        Jump(&loopEnd);
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEnd(&loopHead);
    Bind(&loopExit);

    GateRef newNofe = Int32Sub(GetNumberOfElements(linkedTable), Int32(1));
    SetNumberOfElements(linkedTable, newNofe);
    GateRef newNofd = Int32Add(GetNumberOfDeletedElements(linkedTable), Int32(1));
    SetNumberOfDeletedElements(linkedTable, newNofd);
    env->SubCfgExit();
}

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::HasSufficientCapacity(
    GateRef linkedTable, GateRef numOfAddElements)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    Label exit(env);
    env->SubCfgEntry(&entryLabel);
    DEFVARIABLE(res, VariableType::BOOL(), False());

    GateRef numberOfElements = GetNumberOfElements(linkedTable);
    GateRef numOfDelElements = GetNumberOfDeletedElements(linkedTable);
    GateRef nof = Int32Add(numberOfElements, numOfAddElements);
    GateRef capacity = GetCapacity(linkedTable);
    GateRef less = Int32LessThan(nof, capacity);
    GateRef half = Int32Div(Int32Sub(capacity, nof), Int32(2));
    GateRef lessHalf = Int32LessThanOrEqual(numOfDelElements, half);

    Label lessLable(env);
    Branch(BoolAnd(less, lessHalf), &lessLable, &exit);
    Bind(&lessLable);
    {
        Label need(env);
        Branch(Int32LessThanOrEqual(Int32Add(nof, Int32Div(nof, Int32(2))), capacity), &need, &exit);
        Bind(&need);
        {
            res = True();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::GetHash(GateRef key)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    Label exit(env);
    env->SubCfgEntry(&entryLabel);
    DEFVARIABLE(res, VariableType::INT32(), Int32(0));

    Label symbolKey(env);
    Label stringCheck(env);
    Branch(TaggedIsSymbol(key), &symbolKey, &stringCheck);
    Bind(&symbolKey);
    {
        res = Load(VariableType::INT32(), key, IntPtr(JSSymbol::HASHFIELD_OFFSET));
        Jump(&exit);
    }
    Bind(&stringCheck);
    Label stringKey(env);
    Label slowGetHash(env);
    Branch(TaggedIsString(key), &stringKey, &slowGetHash);
    Bind(&stringKey);
    {
        res = GetHashcodeFromString(glue_, key);
        Jump(&exit);
    }
    Bind(&slowGetHash);
    {
        // GetHash();
        GateRef hash = CallRuntime(glue_, RTSTUB_ID(GetLinkedHash), { key });
        res = GetInt32OfTInt(hash);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::HashObjectIsMatch(
    GateRef key, GateRef other)
{
    return SameValueZero(glue_, key, other);
}

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::FindElement(
    GateRef linkedTable, GateRef key, GateRef hash)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);

    DEFVARIABLE(res, VariableType::INT32(), Int32(-1));
    Label exit(env);
    Label isKey(env);
    Branch(IsKey(key), &isKey, &exit);
    Bind(&isKey);
    {
        GateRef bucket = HashToBucket(linkedTable, hash);
        GateRef index = BucketToIndex(bucket);
        DEFVARIABLE(entry, VariableType::JS_ANY(), GetElement(linkedTable, index));
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);

        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(TaggedIsHole(*entry), &loopExit, &next);
            Bind(&next);

            DEFVARIABLE(element, VariableType::JS_ANY(), GetKey(linkedTable, TaggedGetInt(*entry)));
            Label notHole(env);
            Branch(TaggedIsHole(*element), &loopEnd, &notHole);
            Bind(&notHole);
            {
                Label weak(env);
                Label notWeak(env);
                Branch(TaggedIsWeak(*element), &weak, &notWeak);
                Bind(&weak);
                {
                    element = RemoveTaggedWeakTag(*element);
                    Jump(&notWeak);
                }
                Bind(&notWeak);
                Label match(env);
                Branch(HashObjectIsMatch(key, *element), &match, &loopEnd);
                Bind(&match);
                {
                    res = TaggedGetInt(*entry);
                    Jump(&loopExit);
                }
            }
        }
        Bind(&loopEnd);
        entry = GetNextEntry(linkedTable, TaggedGetInt(*entry));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

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
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Create(GateRef numberOfElements)
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
        SetCapacity(array, numberOfElements);
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
    Label exit(env);
    env->SubCfgEntry(&entry);

    GateRef cap = GetCapacity(linkedTable);
    size_t newLength = LinkedHashTableType::GetLengthOfTable(LinkedHashTableType::MIN_CAPACITY);
    // Fill by Hole()
    DEFVARIABLE(index, VariableType::INT32(), Int32(LinkedHashTableType::ELEMENTS_START_INDEX));
    for (size_t i = LinkedHashTableType::ELEMENTS_START_INDEX; i < newLength; ++i) {
        SetValueToTaggedArray(VariableType::JS_NOT_POINTER(), glue_, linkedTable, *index, Hole());
        index = Int32Add(*index, Int32(1));
    }

    SetNumberOfElements(linkedTable, Int32(0));
    SetNumberOfDeletedElements(linkedTable, Int32(0));
    SetCapacity(linkedTable, Int32(LinkedHashTableType::MIN_CAPACITY));
    Label shrinkLabel(env);
    // if capacity equals to MIN_CAPACITY do nothing
    Branch(Int32Equal(cap, Int32(LinkedHashTableType::MIN_CAPACITY)), &exit, &shrinkLabel);
    Bind(&shrinkLabel);
    // Get here if capacity > MIN_CAPACITY.
    // Call Array::Trim
    CallNGCRuntime(glue_, RTSTUB_ID(ArrayTrim), {glue_, linkedTable, Int32(newLength)});
    Jump(&exit);

    Bind(&exit);
    env->SubCfgExit();
    return linkedTable;
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

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Insert(
    GateRef linkedTable, GateRef key, GateRef value)
{
    auto env = GetEnvironment();
    Label cfgEntry(env);
    env->SubCfgEntry(&cfgEntry);
    Label exit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), linkedTable);
    GateRef hash = GetHash(key);
    GateRef entry = FindElement(linkedTable, key, hash);
    Label findEntry(env);
    Label notFind(env);
    Branch(Int32Equal(entry, Int32(-1)), &notFind, &findEntry);
    Bind(&findEntry);
    {
        SetValue(linkedTable, entry, value);
        Jump(&exit);
    }
    Bind(&notFind);
    {
        GateRef newTable = GrowCapacity(linkedTable, Int32(1));
        res = newTable;
        GateRef bucket = HashToBucket(newTable, hash);
        GateRef numberOfElements = GetNumberOfElements(newTable);

        GateRef newEntry = Int32Add(numberOfElements, GetNumberOfDeletedElements(newTable));
        InsertNewEntry(newTable, bucket, newEntry);
        SetKey(newTable, newEntry, key);
        SetValue(newTable, newEntry, value);
        GateRef newNumberOfElements = Int32Add(numberOfElements, Int32(1));
        SetNumberOfElements(newTable, newNumberOfElements);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::Insert(
    GateRef linkedTable, GateRef key, GateRef value);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::Insert(
    GateRef linkedTable, GateRef key, GateRef value);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Delete(
    GateRef linkedTable, GateRef key)
{
    auto env = GetEnvironment();
    Label cfgEntry(env);
    env->SubCfgEntry(&cfgEntry);
    Label exit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), TaggedFalse());
    GateRef hash = GetHash(key);
    GateRef entry = FindElement(linkedTable, key, hash);
    Label findEntry(env);
    Branch(Int32Equal(entry, Int32(-1)), &exit, &findEntry);
    Bind(&findEntry);
    {
        RemoveEntry(linkedTable, entry);
        res = TaggedTrue();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::Delete(
    GateRef linkedTable, GateRef key);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::Delete(
    GateRef linkedTable, GateRef key);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Has(
    GateRef linkedTable, GateRef key)
{
    auto env = GetEnvironment();
    Label cfgEntry(env);
    env->SubCfgEntry(&cfgEntry);
    Label exit(env);
    Label nonEmpty(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), TaggedFalse());
    GateRef size = GetNumberOfElements(linkedTable);
    Branch(Int32Equal(size, Int32(0)), &exit, &nonEmpty);
    Bind(&nonEmpty);
    GateRef hash = GetHash(key);
    GateRef entry = FindElement(linkedTable, key, hash);
    Label findEntry(env);
    Branch(Int32Equal(entry, Int32(-1)), &exit, &findEntry);
    Bind(&findEntry);
    {
        res = TaggedTrue();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::Has(
    GateRef linkedTable, GateRef key);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::Has(
    GateRef linkedTable, GateRef key);
}  // namespace panda::ecmascript::kungfu
