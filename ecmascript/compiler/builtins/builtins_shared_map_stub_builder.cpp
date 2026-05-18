/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/builtins_shared_map_stub_builder.h"

#include "ecmascript/compiler/builtins/linked_hashtable_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/js_shared_map.h"


namespace panda::ecmascript::kungfu {

// @ref panda::ecmascript::builtins::BuiltinsSharedMap::Keys()
void BuiltinsSharedMapStubBuilder::Keys(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isSharedMap(env);
    BRANCH(IsJSObjectType(glue, thisValue, JSType::JS_SHARED_MAP), &isSharedMap, slowPath);
    Bind(&isSharedMap);
    {
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, IntPtr(JSSharedMapIterator::SIZE));
        GateRef kind = Int32(static_cast<int32_t>(IterationKind::KEY));
        newBuilder.CreateJSSharedMapIterator(result, exit, thisValue, kind);
    }
}

// @ref panda::ecmascript::builtins::BuiltinsSharedMap::Values()
void BuiltinsSharedMapStubBuilder::Values(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isSharedMap(env);
    BRANCH(IsJSObjectType(glue, thisValue, JSType::JS_SHARED_MAP), &isSharedMap, slowPath);
    Bind(&isSharedMap);
    {
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, IntPtr(JSSharedMapIterator::SIZE));
        GateRef kind = Int32(static_cast<int32_t>(IterationKind::VALUE));
        newBuilder.CreateJSSharedMapIterator(result, exit, thisValue, kind);
    }
}

// @ref panda::ecmascript::builtins::BuiltinsSharedMap::Set()
void BuiltinsSharedMapStubBuilder::Set(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isSharedMap(env);

    // Step 1: Check this is JSSharedMap
    BRANCH(IsJSObjectType(glue, thisValue, JSType::JS_SHARED_MAP), &isSharedMap, slowPath);
    Bind(&isSharedMap);

    // Step 2: Get key and value arguments
    GateRef key = GetCallArg0(numArgs);
    GateRef value = GetCallArg1(numArgs);

    // Step 3: JSSharedMap::Set()
    // Step 3.1: check sharedType
    Label keyIsSharedType(env);
    Label valueIsSharedType(env);
    BRANCH(TaggedIsSharedType(glue, key), &keyIsSharedType, slowPath);
    Bind(&keyIsSharedType);

    BRANCH(TaggedIsSharedType(glue, value), &valueIsSharedType, slowPath);
    Bind(&valueIsSharedType);

    // Step 3.2: SetImpl
    SetImpl(glue, thisValue, key, value);
    *result = thisValue;
    Jump(exit);
}

// @ref panda::ecmascript::JSSharedMap::Set() : Assuming that key and value are sharedType
void BuiltinsSharedMapStubBuilder::SetImpl(GateRef glue, GateRef thisValue, GateRef key, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    Label canWriteExit(env);
    Label writeDoneExit(env);
    Label exit(env);
    Label hasScopeException(env);
    Label notScopeException(env);
    DEFVARIABLE(scopeEntered, VariableType::BOOL(), False());

    ASM_ASSERT(GET_MESSAGE_STRING_ID(SharedMapSet), TaggedIsSharedType(glue, key));
    ASM_ASSERT(GET_MESSAGE_STRING_ID(SharedMapSet), TaggedIsSharedType(glue, value));

    // Step 3.3: Create concurrentApiScope
    ConcurrentApiScopeCanWrite<JSSharedMap>(glue, thisValue, scopeEntered, &canWriteExit);
    Bind(&canWriteExit);

    // Step 3.4: Check hasPendingException
    BRANCH(HasPendingException(glue), &hasScopeException, &notScopeException);
    Bind(&hasScopeException);
    {
        Jump(&exit);
    }
    Bind(&notScopeException);
    {
        // Step 3.5: Set impl by LinkedHashMap::Set
        GateRef linkedMapOffset = IntPtr(JSSharedMap::LINKED_MAP_OFFSET);
        GateRef linkedTable = Load(VariableType::JS_ANY(), glue, thisValue, linkedMapOffset);

        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(
            this, glue, GetCurrentGlobalEnv(), true);
        GateRef newTable = linkedHashTableStubBuilder.Insert(linkedTable, key, value);

        // Step 3.6: Set new table to JSSharedMap
        Store(VariableType::JS_ANY(), glue, thisValue, linkedMapOffset, newTable);

        Jump(&exit);
    }
    Bind(&exit);
    ConcurrentApiScopeWriteDone<JSSharedMap>(glue, thisValue, scopeEntered, &writeDoneExit);
    Bind(&writeDoneExit);
    env->SubCfgExit();
}

// @ref panda::ecmascript::builtins::BuiltinsSharedMap::Get()
void BuiltinsSharedMapStubBuilder::Get(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isSharedMap(env);

    // Step 1: Check this is JSSharedMap
    BRANCH(IsJSObjectType(glue, thisValue, JSType::JS_SHARED_MAP), &isSharedMap, slowPath);
    Bind(&isSharedMap);

    // Step 2: Get key argument
    GateRef key = GetCallArg0(numArgs);

    // Step 3: JSSharedMap::Get()
    GateRef value = GetImpl(glue, thisValue, key);
    *result = value;
    Jump(exit);
}

// @ref panda::ecmascript::JSSharedMap::Get()
GateRef BuiltinsSharedMapStubBuilder::GetImpl(GateRef glue, GateRef thisValue, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label hasException(env);
    Label notException(env);

    Label canReadExit(env);
    Label readDoneExit(env);
    DEFVARIABLE(expectModRecord, VariableType::INT32(), Int32(0));
    DEFVARIABLE(desiredModRecord, VariableType::INT32(), Int32(0));
    DEFVARIABLE(scopeEntered, VariableType::BOOL(), False());

    ConcurrentApiScopeCanRead<JSSharedMap>(glue, thisValue, expectModRecord,
        desiredModRecord, scopeEntered, &canReadExit);
    Bind(&canReadExit);
    BRANCH(HasPendingException(glue), &hasException, &notException);
    Bind(&hasException);
    {
        Jump(&exit);
    }
    Bind(&notException);
    {
        GateRef linkedMapOffset = IntPtr(JSSharedMap::LINKED_MAP_OFFSET);
        GateRef linkedTable = Load(VariableType::JS_ANY(), glue, thisValue, linkedMapOffset);
        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(
            this, glue, GetCurrentGlobalEnv(), true);
        result = linkedHashTableStubBuilder.Get(linkedTable, key);

        Jump(&exit);
    }
    Bind(&exit);
    ConcurrentApiScopeReadDone<JSSharedMap>(glue, thisValue, expectModRecord,
        desiredModRecord, scopeEntered, &readDoneExit);
    Bind(&readDoneExit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

// @ref panda::ecmascript::builtins::BuiltinsSharedMap::Has()
void BuiltinsSharedMapStubBuilder::Has(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isSharedMap(env);

    // Step 1: Check this is JSSharedMap
    BRANCH(IsJSObjectType(glue, thisValue, JSType::JS_SHARED_MAP), &isSharedMap, slowPath);
    Bind(&isSharedMap);

    // Step 2: Get key argument
    GateRef key = GetCallArg0(numArgs);

    // Step 3: JSSharedMap::Has()
    GateRef ret = HasImpl(glue, thisValue, key);
    *result = ret;
    Jump(exit);
}

// @ref panda::ecmascript::JSSharedMap::Has()
GateRef BuiltinsSharedMapStubBuilder::HasImpl(GateRef glue, GateRef thisValue, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());
    Label exit(env);
    Label hasException(env);
    Label notException(env);

    Label canReadExit(env);
    Label readDoneExit(env);
    DEFVARIABLE(expectModRecord, VariableType::INT32(), Int32(0));
    DEFVARIABLE(desiredModRecord, VariableType::INT32(), Int32(0));
    DEFVARIABLE(scopeEntered, VariableType::BOOL(), False());

    ConcurrentApiScopeCanRead<JSSharedMap>(glue, thisValue, expectModRecord,
        desiredModRecord, scopeEntered, &canReadExit);
    Bind(&canReadExit);
    BRANCH(HasPendingException(glue), &hasException, &notException);
    Bind(&hasException);
    {
        Jump(&exit);
    }
    Bind(&notException);
    {
        GateRef linkedMapOffset = IntPtr(JSSharedMap::LINKED_MAP_OFFSET);
        GateRef linkedTable = Load(VariableType::JS_ANY(), glue, thisValue, linkedMapOffset);
        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(
            this, glue, GetCurrentGlobalEnv(), true);
        result = BooleanToTaggedBooleanPtr(linkedHashTableStubBuilder.Has(linkedTable, key));

        Jump(&exit);
    }
    Bind(&exit);
    ConcurrentApiScopeReadDone<JSSharedMap>(glue, thisValue, expectModRecord,
        desiredModRecord, scopeEntered, &readDoneExit);
    Bind(&readDoneExit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

// @ref panda::ecmascript::builtins::BuiltinsSharedMap::Delete()
void BuiltinsSharedMapStubBuilder::Delete(GateRef glue, GateRef thisValue,
    GateRef numArgs, Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label isSharedMap(env);

    // Step 1: Check this is JSSharedMap
    BRANCH(IsJSObjectType(glue, thisValue, JSType::JS_SHARED_MAP), &isSharedMap, slowPath);
    Bind(&isSharedMap);

    // Step 2: Get key argument
    GateRef key = GetCallArg0(numArgs);

    // Step 3: JSSharedMap::Delete()
    GateRef ret = DeleteImpl(glue, thisValue, key);
    *result = ret;
    Jump(exit);
}

// @ref panda::ecmascript::JSSharedMap::Delete()
GateRef BuiltinsSharedMapStubBuilder::DeleteImpl(GateRef glue, GateRef thisValue, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());
    Label canWriteExit(env);
    Label writeDoneExit(env);
    Label exit(env);
    Label hasException(env);
    Label notException(env);
    DEFVARIABLE(scopeEntered, VariableType::BOOL(), False());

    ConcurrentApiScopeCanWrite<JSSharedMap>(glue, thisValue, scopeEntered, &canWriteExit);
    Bind(&canWriteExit);
    BRANCH(HasPendingException(glue), &hasException, &notException);
    Bind(&hasException);
    {
        Jump(&exit);
    }
    Bind(&notException);
    {
        GateRef linkedMapOffset = IntPtr(JSSharedMap::LINKED_MAP_OFFSET);
        GateRef linkedTable = Load(VariableType::JS_ANY(), glue, thisValue, linkedMapOffset);
        LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> linkedHashTableStubBuilder(
            this, glue, GetCurrentGlobalEnv(), true);
        result = BooleanToTaggedBooleanPtr(linkedHashTableStubBuilder.Delete(linkedTable, key));
        Jump(&exit);
    }
    Bind(&exit);
    ConcurrentApiScopeWriteDone<JSSharedMap>(glue, thisValue, scopeEntered, &writeDoneExit);
    Bind(&writeDoneExit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

}  // namespace panda::ecmascript::kungfu
