/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/builtins_collection_iterator_stub_builder.h"

#include "ecmascript/compiler/builtins/linked_hashtable_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/linked_hash_table.h"

namespace panda::ecmascript::kungfu {

template <typename IteratorType>
void BuiltinsCollectionIteratorStubBuilder<IteratorType>::CheckCollectionIteratorObj(Label *updateIter,
                                                                                     Label *throwException)
{
    // check target obj
    auto jsType = std::is_same_v<IteratorType, JSSetIterator> ? JSType::JS_SET_ITERATOR : JSType::JS_MAP_ITERATOR;
    GateRef isJsCollectionIteratorObj = IsJSObjectType(glue_, thisValue_, jsType);
    BRANCH(isJsCollectionIteratorObj, updateIter, throwException);
}

template<typename IteratorType>
GateRef BuiltinsCollectionIteratorStubBuilder<IteratorType>::CreateIterValueForEntry(GateRef key, GateRef value)
{
    NewObjectStubBuilder newBuilder(this);
    GateRef elements = newBuilder.NewTaggedArray(glue_, Int32(2));    // 2: length of array to store [key, value] is 2
    SetValueToTaggedArray(VariableType::JS_ANY(), glue_, elements, Int32(0), key);   // 0: key
    SetValueToTaggedArray(VariableType::JS_ANY(), glue_, elements, Int32(1), value); // 1: value
    return newBuilder.CreateArrayFromList(glue_, elements, Int32(Elements::ToUint(ElementsKind::TAGGED)));
}

template <typename IteratorType>
void BuiltinsCollectionIteratorStubBuilder<IteratorType>::Next(Variable *result,
                                                               Label *exit)
{
    auto env = GetEnvironment();
    Label updateIter(env);
    Label checkLinked(env);
    Label getNext(env);
    Label iterDone(env);
    Label throwException(env);

    DEFVARIABLE(res, VariableType::JS_ANY(), TaggedUndefined());
    DEFVARIABLE(iterValue, VariableType::JS_ANY(), TaggedUndefined());
    DEFVARIABLE(isDone, VariableType::JS_ANY(), TaggedTrue());
    DEFVARIABLE(iteratedLinkedObj, VariableType::JS_ANY(), TaggedUndefined());
    DEFVARIABLE(itemKind, VariableType::INT32(), GetIterationKind());
    DEFVARIABLE(totalElements, VariableType::INT32(), Int32(0));
    DEFVARIABLE(key, VariableType::JS_ANY(), TaggedUndefined());

    // check target obj
    CheckCollectionIteratorObj(&updateIter, &throwException);
    Bind(&updateIter);
    {
        UpdateIter(&checkLinked);
    }

    Bind(&checkLinked);
    {
        iteratedLinkedObj = GetIteratedLinkedObj();
        BRANCH(TaggedIsUndefined(*iteratedLinkedObj), &iterDone, &getNext);
    }

    Bind(&getNext);
    {
        DEFVARIABLE(index, VariableType::INT32(), GetNextIndex());
        if constexpr (std::is_same_v<IteratorType, JSMapIterator>) {
            LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this, glue_);
            totalElements = Int32Add(linkedHashTableStubBuilder.GetNumberOfElements(*iteratedLinkedObj),
                                     linkedHashTableStubBuilder.GetNumberOfDeletedElements(*iteratedLinkedObj));
        } else {
            LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this, glue_);
            totalElements = Int32Add(linkedHashTableStubBuilder.GetNumberOfElements(*iteratedLinkedObj),
                                     linkedHashTableStubBuilder.GetNumberOfDeletedElements(*iteratedLinkedObj));
        }
        Label keyIsNotHole(env);
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH(Int32LessThan(*index, *totalElements), &next, &loopExit);
            Bind(&next);
            {
                if constexpr (std::is_same_v<IteratorType, JSMapIterator>) {
                    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this,
                                                                                                              glue_);
                    key = linkedHashTableStubBuilder.GetKey(*iteratedLinkedObj, *index);
                } else {
                    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this,
                                                                                                              glue_);
                    key = linkedHashTableStubBuilder.GetKey(*iteratedLinkedObj, *index);
                }
                BRANCH(TaggedIsHole(*key), &loopEnd, &keyIsNotHole);
                Bind(&keyIsNotHole);
                {
                    SetNextIndex(Int32Add(*index, Int32(1)));
                    Label defaultLabel(env);
                    const uint32_t TOTAL_ITERATOR_KINDS = 3; // 3: iterator has 3 kinds [key, value, key_and_value]
                    Label labelBuffer[TOTAL_ITERATOR_KINDS] = {Label(env), Label(env), Label(env)};
                    int64_t valueBuffer[TOTAL_ITERATOR_KINDS] = {static_cast<int64_t>(IterationKind::KEY),
                                                                 static_cast<int64_t>(IterationKind::VALUE),
                                                                 static_cast<int64_t>(IterationKind::KEY_AND_VALUE)};
                    // 3 : this switch has 3 case
                    Switch(*itemKind, &defaultLabel, valueBuffer, labelBuffer, TOTAL_ITERATOR_KINDS);

                    // ITERATION_KIND_KEY
                    // 0 : index of this buffer
                    Bind(&labelBuffer[0]);
                    {
                        isDone = TaggedFalse();
                        iterValue = *key;
                        Jump(&iterDone);
                    }

                    // ITERATION_KIND_VALUE
                    // 1 : index of this buffer
                    Bind(&labelBuffer[1]);
                    {
                        isDone = TaggedFalse();
                        if constexpr (std::is_same_v<IteratorType, JSMapIterator>) {
                            LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(
                                this, glue_);
                            iterValue = linkedHashTableStubBuilder.GetValue(*iteratedLinkedObj, *index);
                        } else {
                            iterValue = *key;
                        }
                        Jump(&iterDone);
                    }

                    // ITERATION_KIND_KEY_AND_VALUE
                    // 2 : index of this buffer
                    Bind(&labelBuffer[2]);
                    {
                        Label afterCreate(env);
                        DEFVARIABLE(value, VariableType::JS_ANY(), TaggedUndefined());
                        if constexpr (std::is_same_v<IteratorType, JSMapIterator>) {
                            LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(
                                this, glue_);
                            value = linkedHashTableStubBuilder.GetValue(*iteratedLinkedObj, *index);
                        } else {
                            value = *key;
                        }
                        iterValue = CreateIterValueForEntry(*key, *value);
                        isDone = TaggedFalse();
                        Jump(&iterDone);
                    }

                    Bind(&defaultLabel);
                    {
                        FatalPrint(glue_, {Int32(GET_MESSAGE_STRING_ID(ThisBranchIsUnreachable))});
                        Jump(exit);
                    }
                }
            }
        }
        Bind(&loopEnd);
        index = Int32Add(*index, Int32(1));
        LoopEnd(&loopHead);
        Bind(&loopExit);
        SetIteratedLinkedObj(TaggedUndefined());
        Jump(&iterDone);
    }

    Bind(&iterDone);
    {
        Label afterCreate(env);
        NewObjectStubBuilder newBuilder(this);
        newBuilder.CreateJSIteratorResult(glue_, &res, *iterValue, *isDone, &afterCreate);
        Bind(&afterCreate);
        result->WriteVariable(*res);
        Jump(exit);
    }

    Bind(&throwException);
    {
        if constexpr (std::is_same_v<IteratorType, JSMapIterator>) {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(MapIteratorTypeError));
            CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
        } else {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetIteratorTypeError));
            CallRuntime(glue_, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
        }
        result->WriteVariable(Exception());
        Jump(exit);
    }
}

template void BuiltinsCollectionIteratorStubBuilder<JSMapIterator>::Next(Variable *result,
                                                                         Label *exit);
template void BuiltinsCollectionIteratorStubBuilder<JSSetIterator>::Next(Variable *result,
                                                                         Label *exit);

template <typename IteratorType>
void BuiltinsCollectionIteratorStubBuilder<IteratorType>::UpdateIter(Label *iterUpdated)
{
    auto env = GetEnvironment();
    Label checkNextTable(env);
    Label getNext(env);
    DEFVARIABLE(nextTable, VariableType::JS_ANY(), Circuit::NullGate());
    DEFVARIABLE(iteratedLinkedObj, VariableType::JS_ANY(), GetIteratedLinkedObj());

    BRANCH(TaggedIsUndefined(*iteratedLinkedObj), iterUpdated, &checkNextTable);
    
    Bind(&checkNextTable);
    {
        nextTable = GetNextTable(*iteratedLinkedObj);
        BRANCH(TaggedIsHole(*nextTable), iterUpdated, &getNext);
    }

    Bind(&getNext);
    {
        DEFVARIABLE(index, VariableType::INT32(), GetNextIndex());
        Label loopHead(env);
        Label loopEnd(env);
        Label next(env);
        Label loopExit(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            BRANCH(TaggedIsHole(*nextTable), &loopExit, &next);
            Bind(&next);
            {
                DEFVARIABLE(DeletedElementsAt, VariableType::INT32(), Int32(0));
                if constexpr (std::is_same_v<IteratorType, JSMapIterator>) {
                    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this,
                                                                                                              glue_);
                    DeletedElementsAt = linkedHashTableStubBuilder.GetDeletedElementsAt(*iteratedLinkedObj, *index);
                } else {
                    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this,
                                                                                                              glue_);
                    DeletedElementsAt = linkedHashTableStubBuilder.GetDeletedElementsAt(*iteratedLinkedObj, *index);
                }
                index = Int32Sub(*index, *DeletedElementsAt);
                iteratedLinkedObj = *nextTable;
                nextTable = GetNextTable(*iteratedLinkedObj);
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&loopExit);
        SetIteratedLinkedObj(*iteratedLinkedObj);
        SetNextIndex(*index);
        Jump(iterUpdated);
    }
}

template void BuiltinsCollectionIteratorStubBuilder<JSMapIterator>::UpdateIter(Label *iterUpdated);
template void BuiltinsCollectionIteratorStubBuilder<JSSetIterator>::UpdateIter(Label *iterUpdated);

}  // namespace panda::ecmascript::kungfu
