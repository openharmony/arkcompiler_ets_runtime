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

#include "ecmascript/compiler/builtins/shared_linked_hashtable_stub_builder.h"

#include "ecmascript/compiler/call_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_map.h"

namespace panda::ecmascript::kungfu {

template<typename LinkedHashTableType, typename LinkedHashTableObject>
GateRef SharedLinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::Create(GateRef numberOfElements)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    // new LinkedHashTable
    GateRef length = CalNewTaggedArrayLength(numberOfElements);
    NewObjectStubBuilder newBuilder(this);
    GateRef array = newBuilder.NewSTaggedArray(glue_, length);

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

template <typename LinkedHashTableType, typename LinkedHashTableObject>
void SharedLinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::StoreHashTableToNewObject(
    GateRef newTargetHClass, Variable& returnValue)
{
    NewObjectStubBuilder newBuilder(this);
    GateRef res = newBuilder.NewSObject(glue_, newTargetHClass);
    returnValue.WriteVariable(res);
    GateRef table;
    if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashMap>) {
        table = Create(Int32(LinkedHashMap::MIN_CAPACITY));
        Store(VariableType::JS_ANY(), glue_, *returnValue, IntPtr(JSSharedMap::LINKED_MAP_OFFSET), table);
        GateRef modRecordOffset = IntPtr(JSSharedMap::MOD_RECORD_OFFSET);
        Store(VariableType::INT32(), glue_, *returnValue, modRecordOffset, Int32(0));
    } else if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashSet>) {
        table = Create(Int32(LinkedHashSet::MIN_CAPACITY));
        Store(VariableType::JS_ANY(), glue_, *returnValue, IntPtr(JSSharedSet::LINKED_SET_OFFSET), table);
        GateRef modRecordOffset = IntPtr(JSSharedSet::MOD_RECORD_OFFSET);
        Store(VariableType::INT32(), glue_, *returnValue, modRecordOffset, Int32(0));
    }
}

template void SharedLinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::StoreHashTableToNewObject(
    GateRef newTargetHClass, Variable& returnValue);
template void SharedLinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::StoreHashTableToNewObject(
    GateRef newTargetHClass, Variable& returnValue);

// @ref panda::ecmascript::builtins::BuiltinsSharedMap::Constructor()
// @ref panda::ecmascript::builtins::BuiltinsSharedSet::Constructor()
template <typename LinkedHashTableType, typename LinkedHashTableObject>
void SharedLinkedHashTableStubBuilder<LinkedHashTableType, LinkedHashTableObject>::GenSharedMapSetConstructor(
    GateRef nativeCode, GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs, GateRef arg0, GateRef argv)
{
    auto env = GetEnvironment();
    DEFVARIABLE(returnValue, VariableType::JS_ANY(), Undefined());

    Label newTargetObject(env);
    Label newTargetFunction(env);
    Label slowPath(env);
    Label exit(env);

    Label isUndefinedOrNull(env);
    BRANCH(TaggedIsUndefinedOrNull(arg0), &isUndefinedOrNull, &slowPath);

    // 1. If NewTarget is undefined, throw exception in slowPath
    Bind(&isUndefinedOrNull);
    BRANCH(TaggedIsHeapObject(newTarget), &newTargetObject, &slowPath);

    Bind(&newTargetObject);
    BRANCH(IsJSFunction(glue_, newTarget), &newTargetFunction, &slowPath);

    Bind(&newTargetFunction);
    Label fastGetHClass(env);
    GateRef mapOrSetFunc;
    if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashMap>) {
        mapOrSetFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue_, GetCurrentGlobalEnv(),
                                         GlobalEnv::SHARED_BUILTIN_MAP_FUNCTION_INDEX);
    } else if constexpr (std::is_same_v<LinkedHashTableType, LinkedHashSet>) {
        mapOrSetFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glue_, GetCurrentGlobalEnv(),
                                         GlobalEnv::SHARED_BUILTIN_SET_FUNCTION_INDEX);
    }

    GateRef newTargetHClass =
        Load(VariableType::JS_ANY(), glue_, newTarget, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    BRANCH(LogicAndBuilder(env).And(Equal(mapOrSetFunc, newTarget)).And(IsJSHClass(glue_, newTargetHClass)).Done(),
        &fastGetHClass, &slowPath);

    Bind(&fastGetHClass);
    StoreHashTableToNewObject(newTargetHClass, returnValue);
    Jump(&exit);

    Bind(&slowPath);
    returnValue = CallBuiltinRuntime(glue_, { glue_, nativeCode, func, thisValue, numArgs, argv }, true);
    Jump(&exit);

    Bind(&exit);
    Return(*returnValue);
}

template void SharedLinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject>::GenSharedMapSetConstructor(
    GateRef nativeCode, GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs,
    GateRef arg0, GateRef argv);
template void SharedLinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject>::GenSharedMapSetConstructor(
    GateRef nativeCode, GateRef func, GateRef newTarget, GateRef thisValue, GateRef numArgs,
    GateRef arg0, GateRef argv);
}  // namespace panda::ecmascript::kungfu
