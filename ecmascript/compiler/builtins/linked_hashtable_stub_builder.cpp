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

#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/hash_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_map.h"

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
        BRANCH(Int32LessThan(*i, numberOfAllElements), &next, &loopExit);
        Bind(&next);

        GateRef fromIndex = EntryToIndex(linkedTable, *i);
        DEFVARIABLE(key, VariableType::JS_ANY(), GetElement(linkedTable, fromIndex));
        Label hole(env);
        Label notHole(env);
        BRANCH(TaggedIsHole(*key), &hole, &notHole);
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
            BRANCH(TaggedIsWeak(*key), &weak, &notWeak);
            Bind(&weak);
            {
                key = RemoveTaggedWeakTag(*key);
                Jump(&notWeak);
            }
            Bind(&notWeak);

            HashStubBuilder hashBuilder(this, glue_);
            GateRef hash = hashBuilder.GetHash(*key);
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
                BRANCH(Int32LessThan(*j, Int32(LinkedHashTableObject::ENTRY_SIZE)), &next1, &loopExit1);
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
    LoopEnd(&loopHead, env, glue_);
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
    BRANCH(hasSufficient, &exit, &grow);
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
        BRANCH(Int32LessThan(*i, Int32(LinkedHashTableObject::ENTRY_SIZE)), &next, &loopExit);
        Bind(&next);

        GateRef idx = Int32Add(index, *i);
        SetElement(linkedTable, idx, Hole());
        Jump(&loopEnd);
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEnd(&loopHead, env, glue_);
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
    GateRef isLess = LogicAndBuilder(env)
        .And(Int32LessThan(nof, capacity))
        .And(Int32LessThanOrEqual(numOfDelElements, Int32Div(Int32Sub(capacity, nof), Int32(2))))
        .Done();
    Label lessLable(env);
    BRANCH(isLess, &lessLable, &exit);
    Bind(&lessLable);
    {
        Label need(env);
        BRANCH(Int32LessThanOrEqual(Int32Add(nof, Int32Div(nof, Int32(2))), capacity), &need, &exit);  // 2: half
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
    BRANCH(IsKey(key), &isKey, &exit);
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
            BRANCH(TaggedIsHole(*entry), &loopExit, &next);
            Bind(&next);

            DEFVARIABLE(element, VariableType::JS_ANY(), GetKey(linkedTable, TaggedGetInt(*entry)));
            Label notHole(env);
            BRANCH(TaggedIsHole(*element), &loopEnd, &notHole);
            Bind(&notHole);
            {
                Label weak(env);
                Label notWeak(env);
                BRANCH(TaggedIsWeak(*element), &weak, &notWeak);
                Bind(&weak);
                {
                    element = RemoveTaggedWeakTag(*element);
                    Jump(&notWeak);
                }
                Bind(&notWeak);
                Label match(env);
                BRANCH(HashObjectIsMatch(key, *element), &match, &loopEnd);
                Bind(&match);
                {
                    res = TaggedGetInt(*entry);
                    Jump(&loopExit);
                }
            }
        }
        Bind(&loopEnd);
        entry = GetNextEntry(linkedTable, TaggedGetInt(*entry));
        LoopEnd(&loopHead, env, glue_);
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
    Label noNumberOfDeletedElements(env);

    BRANCH(Int32Equal(GetNumberOfDeletedElements(linkedTable), Int32(-1)), &noNumberOfDeletedElements, &loopHead);
    Bind(&noNumberOfDeletedElements);
    {
        res = entry;
        Jump(&exit);
    }

    LoopBegin(&loopHead);
    {
        BRANCH(Int32GreaterThanOrEqual(*currentEntry, Int32(0)), &next, &loopExit);
        Bind(&next);
        GateRef key = GetKey(linkedTable, *currentEntry);
        Label hole(env);
        BRANCH(TaggedIsHole(key), &hole, &loopEnd);
        Bind(&hole);
        {
            GateRef deletedNum = GetDeletedNum(linkedTable, *currentEntry);
            res = deletedNum;
            Jump(&exit);
        }
    }
    Bind(&loopEnd);
    currentEntry = Int32Sub(*currentEntry, Int32(1));
    LoopEnd(&loopHead, env, glue_);
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
    BRANCH(TaggedIsException(array), &exit, &noException);
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
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label setLinked(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), linkedTable);

    Label reuseExistingTable(env);
    Label createNewTable(env);
    GateRef cap = GetCapacity(linkedTable);
    GateRef minCapacity = Int32(LinkedHashTableType::MIN_CAPACITY);
    BRANCH(Equal(cap, minCapacity), &reuseExistingTable, &createNewTable);

    Bind(&reuseExistingTable);
    size_t length = static_cast<size_t>(LinkedHashTableType::GetLengthOfTable(LinkedHashTableType::MIN_CAPACITY));
    for (size_t i = LinkedHashTableType::ELEMENTS_START_INDEX; i < length; ++i) {
        SetValueToTaggedArray(VariableType::JS_NOT_POINTER(), glue_, linkedTable, Int32(i), Hole());
    }
    GateRef numberOfElements = GetNumberOfElements(linkedTable);
    GateRef numberOfDeletedElements = GetNumberOfDeletedElements(linkedTable);
    SetNumberOfElements(linkedTable, Int32(0));
    SetNumberOfDeletedElements(linkedTable, Int32Add(numberOfElements, numberOfDeletedElements));
    Jump(&exit);

    Bind(&createNewTable);
    GateRef newTable = Create(minCapacity);
    result = newTable;
    Label noException(env);
    BRANCH(TaggedIsException(newTable), &exit, &noException);
    Bind(&noException);

    Label capGreaterZero(env);
    BRANCH(Int32GreaterThan(cap, Int32(0)), &capGreaterZero, &exit);
    Bind(&capGreaterZero);
    {
        // NextTable
        SetNextTable(linkedTable, newTable);
        // SetNumberOfDeletedElements
        SetNumberOfDeletedElements(linkedTable, Int32(-1));
        Jump(&exit);
    }

    Bind(&exit);
    GateRef res = *result;
    env->SubCfgExit();
    return res;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::Clear(GateRef);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::Clear(GateRef);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::ForEach(GateRef thisValue,
    GateRef srcLinkedTable, GateRef callbackFnHandle, GateRef thisArg)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
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
        BRANCH(Int32LessThan(*index, *totalElements), &next, &loopExit);
        Bind(&next);
        GateRef valueIndex = *index;

        GateRef key = GetKey(*linkedTable, *index);
        index = Int32Add(*index, Int32(1));
        Label keyNotHole(env);
        BRANCH(TaggedIsHole(key), &loopEnd, &keyNotHole);
        Bind(&keyNotHole);

        GateRef value = key;
        if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashMap>) {
            value = GetValue(*linkedTable, valueIndex);
        }
        Label hasException(env);
        Label notHasException(env);
        JSCallArgs callArgs(JSCallMode::CALL_THIS_ARG3_WITH_RETURN);
        callArgs.callThisArg3WithReturnArgs = { thisArg, value, key, thisValue };
        CallStubBuilder callBuilder(this, glue_, callbackFnHandle, Int32(NUM_MANDATORY_JSFUNC_ARGS), 0, nullptr,
            Circuit::NullGate(), callArgs, ProfileOperation(), false);
        GateRef retValue = callBuilder.JSCallDispatch();
        BRANCH(HasPendingException(glue_), &hasException, &notHasException);
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
                BRANCH(TaggedIsHole(*nextTable), &loopExit1, &next1);
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
    GateRef linkedTable, GateRef callbackFnHandle, GateRef thisArg);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::ForEach(GateRef thisValue,
    GateRef linkedTable, GateRef callbackFnHandle, GateRef thisArg);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Insert(
    GateRef linkedTable, GateRef key, GateRef value)
{
    auto env = GetEnvironment();
    Label cfgEntry(env);
    env->SubCfgEntry(&cfgEntry);
    Label exit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), linkedTable);
    HashStubBuilder hashBuilder(this, glue_);
    GateRef hash = hashBuilder.GetHash(key);
    GateRef entry = FindElement(linkedTable, key, hash);
    Label findEntry(env);
    Label notFind(env);
    BRANCH(Int32Equal(entry, Int32(-1)), &notFind, &findEntry);
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
    DEFVARIABLE(res, VariableType::BOOL(), False());
    HashStubBuilder hashBuilder(this, glue_);
    GateRef hash = hashBuilder.GetHash(key);
    GateRef entry = FindElement(linkedTable, key, hash);
    Label findEntry(env);
    BRANCH(Int32Equal(entry, Int32(-1)), &exit, &findEntry);
    Bind(&findEntry);
    {
        RemoveEntry(linkedTable, entry);
        res = True();
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
    HashStubBuilder hashBuilder(this, glue_);
    GateRef hash = hashBuilder.GetHash(key);
    GateRef entry = FindElement(linkedTable, key, hash);
    GateRef ret = Int32NotEqual(entry, Int32(-1));
    env->SubCfgExit();
    return ret;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::Has(
    GateRef linkedTable, GateRef key);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::Has(
    GateRef linkedTable, GateRef key);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
void LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::StoreHashTableToNewObject(
    GateRef newTargetHClass, Variable& returnValue)
{
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.NewJSObject(glue_, newTargetHClass);
    returnValue.WriteVariable(res);
    GateRef table;
    if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashMap>) {
        table = Create(Int32(LinkedHashMap::MIN_CAPACITY));
        Store(VariableType::JS_ANY(), glue_, *returnValue, IntPtr(JSMap::LINKED_MAP_OFFSET), table);
    } else if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashSet>) {
        table = Create(Int32(LinkedHashSet::MIN_CAPACITY));
        Store(VariableType::JS_ANY(), glue_, *returnValue, IntPtr(JSSet::LINKED_SET_OFFSET), table);
    }
}

template void LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::StoreHashTableToNewObject(
    GateRef newTargetHClass, Variable& returnValue);
template void LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::StoreHashTableToNewObject(
    GateRef newTargetHClass, Variable& returnValue);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
void LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::GenMapSetConstructor(
    GateRef nativeCode, GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs, GateRef arg0, GateRef argv)
{
    auto env = GetEnvironment();
    DEFVARIABLE(returnValue, VariableType::JS_ANY(), Undefined());

    Label newTargetObject(env);
    Label newTargetNotObject(env);
    Label newTargetFunction(env);
    Label slowPath(env);
    Label exit(env);

    // 1.If NewTarget is undefined, throw a TypeError exception
    BRANCH(TaggedIsHeapObject(newTarget), &newTargetObject, &newTargetNotObject);

    Bind(&newTargetObject);
    BRANCH(IsJSFunction(newTarget), &newTargetFunction, &slowPath);

    Bind(&newTargetFunction);
    Label fastGetHClass(env);
    Label intialHClassIsHClass(env);
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue_, glueGlobalEnvOffset);
    GateRef mapOrSetFunc;
    if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashMap>) {
        mapOrSetFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                         GlobalEnv::BUILTINS_MAP_FUNCTION_INDEX);
    } else if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashSet>) {
        mapOrSetFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv,
                                         GlobalEnv::BUILTINS_SET_FUNCTION_INDEX);
    }
    GateRef newTargetHClass = Load(VariableType::JS_ANY(), newTarget, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    BRANCH(LogicAndBuilder(env).And(Equal(mapOrSetFunc, newTarget)).And(IsJSHClass(newTargetHClass)).Done(),
        &fastGetHClass, &slowPath);

    Bind(&fastGetHClass);
    Label isUndefinedOrNull(env);
    BRANCH(TaggedIsUndefinedOrNull(arg0), &isUndefinedOrNull, &slowPath);

    Bind(&isUndefinedOrNull);
    StoreHashTableToNewObject(newTargetHClass, returnValue);
    Jump(&exit);

    Bind(&newTargetNotObject);
    GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(InvalidNewTarget));
    CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), { IntToTaggedInt(taggedId) });
    returnValue = Exception();
    Jump(&exit);

    Bind(&slowPath);
    returnValue = CallBuiltinRuntimeWithNewTarget(glue_, {glue_, nativeCode, func, thisValue,
        numArgs, argv, newTarget});
    Jump(&exit);

    Bind(&exit);
    Return(*returnValue);
}

template void LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::GenMapSetConstructor(
    GateRef nativeCode, GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs,
    GateRef arg0, GateRef argv);
template void LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::GenMapSetConstructor(
    GateRef nativeCode, GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs,
    GateRef arg0, GateRef argv);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::GetLinked(GateRef jsThis)
{
    GateRef linkedTableOffset = GetLinkedOffset();
    return Load(VariableType::JS_ANY(), jsThis, linkedTableOffset);
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::GetLinked(
    GateRef jsThis);
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::GetLinked(
    GateRef jsThis);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
void LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::SetLinked(GateRef jsThis, GateRef newTable)
{
    GateRef linkedTableOffset = GetLinkedOffset();
    Store(VariableType::JS_ANY(), glue_, jsThis, linkedTableOffset, newTable);
}

template void LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::SetLinked(
    GateRef jsThis, GateRef newTable);
template void LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::SetLinked(
    GateRef jsThis, GateRef newTable);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Get(
    GateRef linkedTable, GateRef key)
{
    auto env = GetEnvironment();
    Label cfgEntry(env);
    env->SubCfgEntry(&cfgEntry);
    Label exit(env);
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    HashStubBuilder hashBuilder(this, glue_);
    GateRef hash = hashBuilder.GetHash(key);
    GateRef entry = FindElement(linkedTable, key, hash);
    Label findEntry(env);
    Branch(Int32Equal(entry, Int32(-1)), &exit, &findEntry);
    Bind(&findEntry);
    {
        res = GetValue(linkedTable, entry);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::Get(
    GateRef linkedTable, GateRef key);

template <typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef LinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::GetLinkedOffset()
{
    int32_t linkedTableOffset = 0;
    if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashMap>) {
        linkedTableOffset = JSMap::LINKED_MAP_OFFSET;
    } else {
        static_assert(std::is_same_v<LinkedHashTableType, LinkedHashSet>);
        linkedTableOffset = JSSet::LINKED_SET_OFFSET;
    }
    return IntPtr(linkedTableOffset);
}

template GateRef LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::GetLinkedOffset();
template GateRef LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::GetLinkedOffset();

}  // namespace panda::ecmascript::kungfu
