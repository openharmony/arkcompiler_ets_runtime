/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/builtins_collection_stub_builder.h"

#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_iterator.h"

namespace panda::ecmascript::kungfu {

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::CheckCollectionObj(Label *thisCollectionObj, Label *slowPath)
{
    // check target obj
    auto jsType = std::is_same_v<CollectionType, JSSet> ? JSType::JS_SET : JSType::JS_MAP;
    GateRef isJsCollectionObj = IsJSObjectType(glue_, thisValue_, jsType);
    BRANCH(isJsCollectionObj, thisCollectionObj, slowPath);
}

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Clear(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisCollectionObj(env);
    // check target obj
    CheckCollectionObj(&thisCollectionObj, slowPath);

    Bind(&thisCollectionObj);
    GateRef linkedTable = GetLinked();
    GateRef res = Circuit::NullGate();
    if constexpr (std::is_same_v<CollectionType, JSMap>) {
        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.Clear(linkedTable);
    } else {
        LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.Clear(linkedTable);
    }

    Label exception(env);
    Label noException(env);
    BRANCH(TaggedIsException(res), &exception, &noException);
    Bind(&noException);
    SetLinked(res);
    Jump(exit);
    Bind(&exception);
    *result = res;
    Jump(exit);
}

template void BuiltinsCollectionStubBuilder<JSMap>::Clear(Variable *result, Label *exit, Label *slowPath);
template void BuiltinsCollectionStubBuilder<JSSet>::Clear(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::CreateIterator(Variable *result,
    Label *exit, Label *slowPath, GateRef kind)
{
    auto env = GetEnvironment();
    Label entry(env);
    Label thisCollectionObj(env);
    // check target obj
    CheckCollectionObj(&thisCollectionObj, slowPath);

    Bind(&thisCollectionObj);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetGlue(glue_);
    if constexpr (std::is_same_v<CollectionType, JSSet>) {
        newBuilder.CreateJSCollectionIterator<JSSetIterator, CollectionType>(result, exit, thisValue_, kind);
    } else {
        newBuilder.CreateJSCollectionIterator<JSMapIterator, CollectionType>(result, exit, thisValue_, kind);
    }
}

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Values(Variable *result, Label *exit, Label *slowPath)
{
    GateRef kind = Int32(static_cast<int32_t>(IterationKind::VALUE));
    CreateIterator(result, exit, slowPath, kind);
}

template void BuiltinsCollectionStubBuilder<JSMap>::Values(Variable *result, Label *exit, Label *slowPath);
template void BuiltinsCollectionStubBuilder<JSSet>::Values(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Entries(Variable *result, Label *exit, Label *slowPath)
{
    GateRef kind = Int32(static_cast<int32_t>(IterationKind::KEY_AND_VALUE));
    CreateIterator(result, exit, slowPath, kind);
}

template void BuiltinsCollectionStubBuilder<JSMap>::Entries(Variable *result, Label *exit, Label *slowPath);
template void BuiltinsCollectionStubBuilder<JSSet>::Entries(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Keys(Variable *result, Label *exit, Label *slowPath)
{
    GateRef kind = Int32(static_cast<int32_t>(IterationKind::KEY));
    CreateIterator(result, exit, slowPath, kind);
}

template void BuiltinsCollectionStubBuilder<JSMap>::Keys(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::ForEach(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisCollectionObj(env);
    // check target obj
    CheckCollectionObj(&thisCollectionObj, slowPath);

    Bind(&thisCollectionObj);
    GateRef callbackFnHandle = GetCallArg0(numArgs_);
    Label callable(env);
    // check heap obj
    Label heapObj(env);
    BRANCH(TaggedIsHeapObject(callbackFnHandle), &heapObj, slowPath);
    Bind(&heapObj);
    BRANCH(IsCallable(glue_, callbackFnHandle), &callable, slowPath);
    Bind(&callable);

    GateRef linkedTable = GetLinked();
    GateRef res = Circuit::NullGate();
    GateRef thisArg = GetCallArg1(numArgs_);
    if constexpr (std::is_same_v<CollectionType, JSMap>) {
        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.ForEach(thisValue_, linkedTable, callbackFnHandle, thisArg);
    } else {
        LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.ForEach(thisValue_, linkedTable, callbackFnHandle, thisArg);
    }

    Label exception(env);
    BRANCH(TaggedIsException(res), &exception, exit);
    Bind(&exception);
    *result = res;
    Jump(exit);
}

template void BuiltinsCollectionStubBuilder<JSMap>::ForEach(Variable *result, Label *exit, Label *slowPath);
template void BuiltinsCollectionStubBuilder<JSSet>::ForEach(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::MapSetOrSetAdd(
    Variable *result, Label *exit, Label *slowPath, bool isJsMapSet)
{
    auto env = GetEnvironment();
    Label thisCollectionObj(env);
    // check target obj
    CheckCollectionObj(&thisCollectionObj, slowPath);
    Bind(&thisCollectionObj);
    GateRef key = GetCallArg0(numArgs_);
    // check key
    Label keyNotHole(env);
    BRANCH(TaggedIsHole(key), slowPath, &keyNotHole);
    Bind(&keyNotHole);
    GateRef value = isJsMapSet ? GetCallArg1(numArgs_) : key;
    GateRef linkedTable = GetLinked();
    GateRef res = Circuit::NullGate();
    if constexpr (std::is_same_v<CollectionType, JSMap>) {
        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.Insert(linkedTable, key, value);
    } else {
        LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.Insert(linkedTable, key, value);
    }

    SetLinked(res);
    *result = thisValue_;
    Jump(exit);
}

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Set(Variable *result, Label *exit, Label *slowPath)
{
    MapSetOrSetAdd(result, exit, slowPath, true);
}

template void BuiltinsCollectionStubBuilder<JSMap>::Set(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Add(Variable *result, Label *exit, Label *slowPath)
{
    MapSetOrSetAdd(result, exit, slowPath, false);
}

template void BuiltinsCollectionStubBuilder<JSSet>::Add(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Delete(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisCollectionObj(env);
    // check target obj
    CheckCollectionObj(&thisCollectionObj, slowPath);

    Bind(&thisCollectionObj);
    GateRef key = GetCallArg0(numArgs_);
    GateRef linkedTable = GetLinked();
    GateRef res = Circuit::NullGate();
    if constexpr (std::is_same_v<CollectionType, JSMap>) {
        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.Delete(linkedTable, key);
    } else {
        LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.Delete(linkedTable, key);
    }
    *result = BooleanToTaggedBooleanPtr(res);
    Jump(exit);
}

template void BuiltinsCollectionStubBuilder<JSMap>::Delete(Variable *result, Label *exit, Label *slowPath);
template void BuiltinsCollectionStubBuilder<JSSet>::Delete(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Has(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisCollectionObj(env);
    // check target obj
    CheckCollectionObj(&thisCollectionObj, slowPath);

    Bind(&thisCollectionObj);
    GateRef key = GetCallArg0(numArgs_);
    GateRef linkedTable = GetLinked();
    GateRef res = Circuit::NullGate();
    if constexpr (std::is_same_v<CollectionType, JSMap>) {
        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.Has(linkedTable, key);
    } else {
        LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
        res = linkedHashTableStubBuilder.Has(linkedTable, key);
    }
    *result = BooleanToTaggedBooleanPtr(res);
    Jump(exit);
}

template void BuiltinsCollectionStubBuilder<JSMap>::Has(Variable *result, Label *exit, Label *slowPath);
template void BuiltinsCollectionStubBuilder<JSSet>::Has(Variable *result, Label *exit, Label *slowPath);

template <typename CollectionType>
void BuiltinsCollectionStubBuilder<CollectionType>::Get(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label thisCollectionObj(env);
    // check target obj
    CheckCollectionObj(&thisCollectionObj, slowPath);

    Bind(&thisCollectionObj);
    GateRef key = GetCallArg0(numArgs_);
    GateRef linkedTable = GetLinked();
    static_assert(std::is_same_v<CollectionType, JSMap>);
    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(this, glue_, GetCurrentGlobalEnv());
    *result = linkedHashTableStubBuilder.Get(linkedTable, key);
    Jump(exit);
}

template void BuiltinsCollectionStubBuilder<JSMap>::Get(Variable *result, Label *exit, Label *slowPath);

}  // namespace panda::ecmascript::kungfu
