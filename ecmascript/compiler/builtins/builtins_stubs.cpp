/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/builtins_stubs.h"

#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_dataview_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_function_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_regexp_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_number_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_proxy_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_reflect_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_typedarray_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_arraylist_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_deque_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_hashmap_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_hashset_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_lightweightmap_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_lightweightset_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_linkedlist_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_list_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_plainarray_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_queue_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_stack_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_vector_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_collator_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_collection_iterator_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_collection_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_object_stub_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/hash_stub_builder.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/linked_hash_table.h"

namespace panda::ecmascript::kungfu {
#if ECMASCRIPT_ENABLE_BUILTIN_LOG
#define DECLARE_BUILTINS(name)                                                                      \
void name##StubBuilder::GenerateCircuit()                                                           \
{                                                                                                   \
    GateRef glue = PtrArgument(static_cast<size_t>(BuiltinsArgs::GLUE));                            \
    GateRef nativeCode = PtrArgument(static_cast<size_t>(BuiltinsArgs::NATIVECODE));                \
    GateRef func = TaggedArgument(static_cast<size_t>(BuiltinsArgs::FUNC));                         \
    GateRef newTarget = TaggedArgument(static_cast<size_t>(BuiltinsArgs::NEWTARGET));               \
    GateRef thisValue = TaggedArgument(static_cast<size_t>(BuiltinsArgs::THISVALUE));               \
    GateRef numArgs = PtrArgument(static_cast<size_t>(BuiltinsArgs::NUMARGS));                      \
    DebugPrint(glue, { Int32(GET_MESSAGE_STRING_ID(name)) });                                       \
    GenerateCircuitImpl(glue, nativeCode, func, newTarget, thisValue, numArgs);                     \
}                                                                                                   \
void name##StubBuilder::GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func,         \
                                            GateRef newTarget, GateRef thisValue, GateRef numArgs)
#else
#ifndef NDEBUG
#define DECLARE_BUILTINS(name)                                                                      \
void name##StubBuilder::GenerateCircuit()                                                           \
{                                                                                                   \
    GateRef glue = PtrArgument(static_cast<size_t>(BuiltinsArgs::GLUE));                            \
    GateRef nativeCode = PtrArgument(static_cast<size_t>(BuiltinsArgs::NATIVECODE));                \
    GateRef func = TaggedArgument(static_cast<size_t>(BuiltinsArgs::FUNC));                         \
    GateRef newTarget = TaggedArgument(static_cast<size_t>(BuiltinsArgs::NEWTARGET));               \
    GateRef thisValue = TaggedArgument(static_cast<size_t>(BuiltinsArgs::THISVALUE));               \
    GateRef numArgs = PtrArgument(static_cast<size_t>(BuiltinsArgs::NUMARGS));                      \
    CallRuntime(glue, RTSTUB_ID(ForceGC), {});                                                      \
    GenerateCircuitImpl(glue, nativeCode, func, newTarget, thisValue, numArgs);                     \
}                                                                                                   \
void name##StubBuilder::GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func,         \
                                            GateRef newTarget, GateRef thisValue, GateRef numArgs)
#else
#define DECLARE_BUILTINS(name)                                                                      \
void name##StubBuilder::GenerateCircuit()                                                           \
{                                                                                                   \
    GateRef glue = PtrArgument(static_cast<size_t>(BuiltinsArgs::GLUE));                            \
    GateRef nativeCode = PtrArgument(static_cast<size_t>(BuiltinsArgs::NATIVECODE));                \
    GateRef func = TaggedArgument(static_cast<size_t>(BuiltinsArgs::FUNC));                         \
    GateRef newTarget = TaggedArgument(static_cast<size_t>(BuiltinsArgs::NEWTARGET));               \
    GateRef thisValue = TaggedArgument(static_cast<size_t>(BuiltinsArgs::THISVALUE));               \
    GateRef numArgs = PtrArgument(static_cast<size_t>(BuiltinsArgs::NUMARGS));                      \
    GenerateCircuitImpl(glue, nativeCode, func, newTarget, thisValue, numArgs);                     \
}                                                                                                   \
void name##StubBuilder::GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func,         \
                                            GateRef newTarget, GateRef thisValue, GateRef numArgs)
#endif
#endif

GateRef BuiltinsStubBuilder::GetCallArg0(GateRef numArg)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label isValid(env);
    Label exit(env);
    BRANCH(Int32GreaterThan(TruncPtrToInt32(numArg), Int32(0)), &isValid, &exit);
    Bind(&isValid);
    {
        result = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG0_OR_ARGV));
        Jump(&exit);
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

GateRef BuiltinsStubBuilder::GetCallArg1(GateRef numArg)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label isValid(env);
    Label exit(env);
    BRANCH(Int32GreaterThan(TruncPtrToInt32(numArg), Int32(1)), &isValid, &exit);
    Bind(&isValid);
    {
        result = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG1));
        Jump(&exit);
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

GateRef BuiltinsStubBuilder::GetCallArg2(GateRef numArg)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    Label isValid(env);
    Label exit(env);
    // 2: 2 args
    BRANCH(Int32GreaterThan(TruncPtrToInt32(numArg), Int32(2)), &isValid, &exit);
    Bind(&isValid);
    {
        result = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG2));
        Jump(&exit);
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

GateRef BuiltinsStubBuilder::GetGlobalEnvFromFunction(GateRef glue, GateRef func) {
    auto env0 = GetEnvironment();
    Label subentry(env0);
    env0->SubCfgEntry(&subentry);
    Label exit(env0);
    Label isUndefined(env0);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef lexicalEnv = GetFunctionLexicalEnv(glue, func);
    {
        ASM_ASSERT(GET_MESSAGE_STRING_ID(LexicalEnvIsUndefined),
                   BitOr(TaggedIsSharedObj(glue, func),
                       LogicAndBuilder(env0).
                       And(TaggedIsHeapObject(lexicalEnv)).
                       And(IsGlobalEnv(glue, lexicalEnv)).
                       Done()));
    }
    result = lexicalEnv;
    BRANCH_UNLIKELY(TaggedIsUndefined(lexicalEnv), &isUndefined, &exit);
    Bind(&isUndefined);
    {
        result = GetGlobalEnv(glue);
        Jump(&exit);
    }
    Bind(&exit);
    auto res = *result;
    env0->SubCfgExit();
    return res;
}

GateRef BuiltinsStubBuilder::GetArgFromArgv(GateRef glue, GateRef index, GateRef numArgs, bool needCheck)
{
    if (!needCheck) {
        GateRef argv = GetArgv();
        return Load(VariableType::JS_ANY(), glue, argv, PtrMul(index, IntPtr(JSTaggedValue::TaggedTypeSize())));
    }
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(arg, VariableType::JS_ANY(), Undefined());
    Label validIndex(env);
    Label exit(env);
    BRANCH(IntPtrGreaterThan(numArgs, index), &validIndex, &exit);
    Bind(&validIndex);
    {
        GateRef argv = GetArgv();
        arg = Load(VariableType::JS_ANY(), glue, argv, PtrMul(index, IntPtr(JSTaggedValue::TaggedTypeSize())));
        Jump(&exit);
    }
    Bind(&exit);
    GateRef ret = *arg;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStubBuilder::CallSlowPath(GateRef nativeCode, GateRef glue, GateRef thisValue,
                                          GateRef numArgs, GateRef func, GateRef newTarget)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label callThis0(env);
    Label notcallThis0(env);
    Label notcallThis1(env);
    Label callThis1(env);
    Label callThis2(env);
    Label callThis3(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef runtimeCallInfoArgs = PtrAdd(numArgs, IntPtr(NUM_MANDATORY_JSFUNC_ARGS));
    BRANCH(Int64Equal(numArgs, IntPtr(0)), &callThis0, &notcallThis0);
    Bind(&callThis0);
    {
        auto args = { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue };
        result = CallBuiltinRuntime(glue, args, false);
        Jump(&exit);
    }
    Bind(&notcallThis0);
    {
        BRANCH(Int64Equal(numArgs, IntPtr(1)), &callThis1, &notcallThis1);
        Bind(&callThis1);
        {
            GateRef arg0 = GetCallArg0(numArgs);
            auto args = { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue, arg0 };
            result = CallBuiltinRuntime(glue, args, false);
            Jump(&exit);
        }
        Bind(&notcallThis1);
        {
            BRANCH(Int64Equal(numArgs, IntPtr(2)), &callThis2, &callThis3); // 2: args2
            Bind(&callThis2);
            {
                GateRef arg0 = GetCallArg0(numArgs);
                GateRef arg1 = GetCallArg1(numArgs);
                auto args = { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue, arg0, arg1 };
                result = CallBuiltinRuntime(glue, args, false);
                Jump(&exit);
            }
            Bind(&callThis3);
            {
                GateRef arg0 = GetCallArg0(numArgs);
                GateRef arg1 = GetCallArg1(numArgs);
                GateRef arg2 = GetCallArg2(numArgs);
                auto args = { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue, arg0, arg1, arg2 };
                result = CallBuiltinRuntime(glue, args, false);
                Jump(&exit);
            }
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

#define DECLARE_BUILTINS_STUB_BUILDER(method, type, initValue, ...)                                 \
DECLARE_BUILTINS(type##method)                                                                      \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, VariableType::JS_ANY(), initValue);                                            \
    Label exit(env);                                                                                \
    Label slowPath(env);                                                                            \
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);                                       \
    Builtins##type##StubBuilder builder(this, globalEnv);                                           \
    builder.method(glue, thisValue, numArgs, &res, &exit, &slowPath);                               \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);                  \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

#define DECLARE_BUILTINS_STUB_BUILDER1(method, type, initValue, ...)                                \
DECLARE_BUILTINS(type##method)                                                                      \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, VariableType::JS_ANY(), initValue);                                            \
    Label thisCollectionObj(env);                                                                   \
    Label slowPath(env);                                                                            \
    Label exit(env);                                                                                \
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);                                       \
    Builtins##type##StubBuilder builder(this, glue, thisValue, numArgs, globalEnv);                 \
    builder.method(&res, &exit, &slowPath);                                                         \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);                  \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

// map and set stub function
#define DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(method, type, retDefaultValue, ...)                \
DECLARE_BUILTINS(type##method)                                                                      \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, VariableType::JS_ANY(), retDefaultValue);                                      \
    Label slowPath(env);                                                                            \
    Label exit(env);                                                                                \
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);                                       \
    BuiltinsCollectionStubBuilder<JS##type> builder(this, glue, thisValue, numArgs, globalEnv);     \
    builder.method(&res, &exit, &slowPath);                                                         \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);                  \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

// map and set stub function
#define DECLARE_BUILTINS_DATAVIEW_STUB_BUILDER(method, type, numType, function, retDefaultValue, ...) \
DECLARE_BUILTINS(type##method)                                                                        \
{                                                                                                     \
    auto env = GetEnvironment();                                                                      \
    DEFVARIABLE(res, VariableType::JS_ANY(), retDefaultValue);                                        \
    Label slowPath(env);                                                                              \
    Label exit(env);                                                                                  \
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);                                         \
    BuiltinsDataViewStubBuilder builder(this, globalEnv);                                             \
    builder.function<DataViewType::numType>(glue, thisValue, numArgs, &res, &exit, &slowPath);        \
    Bind(&slowPath);                                                                                  \
    {                                                                                                 \
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(type##method));                      \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);                    \
        Jump(&exit);                                                                                  \
    }                                                                                                 \
    Bind(&exit);                                                                                      \
    Return(*res);                                                                                     \
}

BUILTINS_METHOD_STUB_LIST(DECLARE_BUILTINS_STUB_BUILDER, DECLARE_BUILTINS_STUB_BUILDER1,
                          DECLARE_BUILTINS_COLLECTION_STUB_BUILDER, DECLARE_BUILTINS_DATAVIEW_STUB_BUILDER)
#undef DECLARE_BUILTINS_STUB_BUILDER
#undef DECLARE_BUILTINS_STUB_BUILDER1
#undef DECLARE_BUILTINS_COLLECTION_STUB_BUILDER
#undef DECLARE_BUILTINS_DATAVIEW_STUB_BUILDER

DECLARE_BUILTINS(ArkToolsHashCode)
{
    (void) nativeCode;
    (void) func;
    (void) newTarget;
    (void) thisValue;
    auto env = GetEnvironment();
    GateRef key = GetCallArg0(numArgs);

    Label irHash(env);
    Label rtHash(env);
    BRANCH(IntPtrEqual(numArgs, IntPtr(1)), &irHash, &rtHash);
    Bind(&irHash);
    {
        HashStubBuilder hashBuilder(this, glue);
        GateRef hash = hashBuilder.GetHash(key);
        Return(env->GetBuilder()->Int32ToTaggedPtr(hash));
    }
    Bind(&rtHash);
    Return(CallRuntime(glue, RTSTUB_ID(GetLinkedHash), { key }));
}

// aot and builtins public stub function
#define DECLARE_AOT_AND_BUILTINS_STUB_BUILDER(stubName, method, type, initValue)                    \
DECLARE_BUILTINS(stubName)                                                                          \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, VariableType::JS_ANY(), initValue);                                            \
    Label exit(env);                                                                                \
    Label slowPath(env);                                                                            \
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);                                       \
    Builtins##type##StubBuilder builder(this, globalEnv);                                           \
    builder.method(glue, thisValue, numArgs, &res, &exit, &slowPath);                               \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);                  \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

// map and set iterator stub function
#define DECLARE_BUILTINS_COLLECTION_ITERATOR_STUB_BUILDER(stubName, method, type, retDefaultValue)             \
DECLARE_BUILTINS(stubName)                                                                                     \
{                                                                                                              \
    (void) nativeCode;                                                                                         \
    (void) func;                                                                                               \
    (void) newTarget;                                                                                          \
    auto env = GetEnvironment();                                                                               \
    DEFVARIABLE(res, VariableType::JS_ANY(), retDefaultValue);                                                 \
    Label exit(env);                                                                                           \
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);                                                  \
    BuiltinsCollectionIteratorStubBuilder<JS##type> builder(this, glue, thisValue, numArgs, globalEnv);        \
    builder.method(&res, &exit);                                                                               \
    Bind(&exit);                                                                                               \
    Return(*res);                                                                                              \
}

#if ENABLE_NEXT_OPTIMIZATION
    #define NEXT_AOT_AND_BUILTINS_STUB_LIST_WITH_METHOD(V, D)                                        \
        V(ArrayIteratorProtoNext,  ArrayIteratorNext,   Array,            Undefined())               \
        D(MapIteratorProtoNext,    Next,                MapIterator,      Undefined())               \
        D(SetIteratorProtoNext,    Next,                SetIterator,      Undefined())
#else
    #define NEXT_AOT_AND_BUILTINS_STUB_LIST_WITH_METHOD(V, D)
#endif

#define AOT_AND_BUILTINS_STUB_LIST_WITH_METHOD(V, D)                                              \
    V(StringLocaleCompare,           LocaleCompare,        String,   Undefined())                 \
    V(StringIteratorProtoNext,       StringIteratorNext,   String,   Undefined())                 \
    V(ArraySort,                     Sort,                 Array,    Undefined())                 \
    NEXT_AOT_AND_BUILTINS_STUB_LIST_WITH_METHOD(V, D)

AOT_AND_BUILTINS_STUB_LIST_WITH_METHOD(DECLARE_AOT_AND_BUILTINS_STUB_BUILDER,
                                       DECLARE_BUILTINS_COLLECTION_ITERATOR_STUB_BUILDER)
#undef NEXT_AOT_AND_BUILTINS_STUB_LIST_WITH_METHOD
#undef AOT_AND_BUILTINS_STUB_LIST
#undef DECLARE_AOT_AND_BUILTINS_STUB_BUILDER
#undef DECLARE_BUILTINS_COLLECTION_ITERATOR_STUB_BUILDER

#define DECLARE_CONTAINERS_STUB_BUILDER(method, type, initValue)                                    \
DECLARE_BUILTINS(type##method)                                                                      \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, VariableType::JS_ANY(), initValue);                                            \
    Label exit(env);                                                                                \
    Label slowPath(env);                                                                            \
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);                                       \
    Containers##type##StubBuilder builder(this, globalEnv);                                         \
    builder.method(glue, thisValue, numArgs, &res, &exit, &slowPath);                               \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);                  \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

BUILTINS_CONTAINERS_STUB_BUILDER(DECLARE_CONTAINERS_STUB_BUILDER)
#undef DECLARE_CONTAINERS_STUB_BUILDER

DECLARE_BUILTINS(BooleanConstructor)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsHeapObject(env);
    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label exit(env);

    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath);
    Bind(&newTargetIsHeapObject);
    BRANCH(IsJSFunction(glue, newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label intialHClassIsHClass(env);
        GateRef intialHClass = Load(VariableType::JS_ANY(), glue, newTarget,
                                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        BRANCH(IsJSHClass(glue, intialHClass), &intialHClassIsHClass, &slowPath1);
        Bind(&intialHClassIsHClass);
        {
            NewObjectStubBuilder newBuilder(this);
            newBuilder.SetParameters(glue, 0);
            Label afterNew(env);
            newBuilder.NewJSObject(&res, &afterNew, intialHClass);
            Bind(&afterNew);
            {
                GateRef valueOffset = IntPtr(JSPrimitiveRef::VALUE_OFFSET);
                GateRef value = GetArgFromArgv(glue, IntPtr(0), numArgs, true);
                Store(VariableType::INT64(), glue, *res, valueOffset, FastToBoolean(glue, value));
                Jump(&exit);
            }
        }
        Bind(&slowPath1);
        {
            GateRef argv = GetArgv();
            auto args = { glue, nativeCode, func, thisValue, numArgs, argv, newTarget };
            res = CallBuiltinRuntimeWithNewTarget(glue, args);
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        GateRef argv = GetArgv();
        auto args = { glue, nativeCode, func, thisValue, numArgs, argv };
        res = CallBuiltinRuntime(glue, args, true);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(DateConstructor)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsHeapObject(env);
    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label exit(env);

    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath);
    Bind(&newTargetIsHeapObject);
    BRANCH(IsJSFunction(glue, newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label intialHClassIsHClass(env);
        GateRef intialHClass = Load(VariableType::JS_ANY(), glue, newTarget,
                                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        BRANCH(IsJSHClass(glue, intialHClass), &intialHClassIsHClass, &slowPath1);
        Bind(&intialHClassIsHClass);
        {
            Label oneArg(env);
            Label notOneArg(env);
            Label newJSDate(env);
            DEFVARIABLE(timeValue, VariableType::FLOAT64(), Double(0));
            BRANCH(Int64Equal(numArgs, IntPtr(1)), &oneArg, &notOneArg);
            Bind(&oneArg);
            {
                Label valueIsNumber(env);
                GateRef value = GetArgFromArgv(glue, IntPtr(0));
                BRANCH(TaggedIsNumber(value), &valueIsNumber, &slowPath);
                Bind(&valueIsNumber);
                {
                    timeValue = CallNGCRuntime(glue, RTSTUB_ID(TimeClip), {GetDoubleOfTNumber(value)});
                    Jump(&newJSDate);
                }
            }
            Bind(&notOneArg);
            {
                Label threeArgs(env);
                BRANCH(Int64Equal(numArgs, IntPtr(3)), &threeArgs, &slowPath);  // 3: year month day
                Bind(&threeArgs);
                {
                    Label numberYearMonthDay(env);
                    GateRef year = GetArgFromArgv(glue, IntPtr(0));
                    GateRef month = GetArgFromArgv(glue, IntPtr(1));
                    GateRef day = GetArgFromArgv(glue, IntPtr(2));
                    BRANCH(IsNumberYearMonthDay(year, month, day), &numberYearMonthDay, &slowPath);
                    Bind(&numberYearMonthDay);
                    {
                        GateRef y = GetDoubleOfTNumber(year);
                        GateRef m = GetDoubleOfTNumber(month);
                        GateRef d = GetDoubleOfTNumber(day);
                        timeValue = CallNGCRuntime(glue, RTSTUB_ID(SetDateValues), {y, m, d});
                        Jump(&newJSDate);
                    }
                }
            }
            Bind(&newJSDate);
            {
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue, 0);
                Label afterNew(env);
                newBuilder.NewJSObject(&res, &afterNew, intialHClass);
                Bind(&afterNew);
                {
                    GateRef timeValueOffset = IntPtr(JSDate::TIME_VALUE_OFFSET);
                    Store(VariableType::JS_NOT_POINTER(), glue, *res, timeValueOffset,
                          DoubleToTaggedDoublePtr(*timeValue));
                    Jump(&exit);
                }
            }
        }
        Bind(&slowPath1);
        {
            GateRef argv = GetArgv();
            res = CallBuiltinRuntimeWithNewTarget(glue,
                { glue, nativeCode, func, thisValue, numArgs, argv, newTarget });
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue, { glue, nativeCode, func, thisValue, numArgs, argv }, true);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(NumberConstructor)
{
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);
    BuiltinsNumberStubBuilder builder(this, glue, thisValue, numArgs, globalEnv);
    builder.GenNumberConstructor(nativeCode, func, newTarget);
}

DECLARE_BUILTINS(ProxyConstructor)
{
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);
    BuiltinsProxyStubBuilder builder(this, glue, thisValue, numArgs, globalEnv);
    builder.GenProxyConstructor(nativeCode, func, newTarget);
}

DECLARE_BUILTINS(ArrayConstructor)
{
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);
    BuiltinsArrayStubBuilder builder(this, globalEnv);
    builder.GenArrayConstructor(glue, nativeCode, func, newTarget, thisValue, numArgs);
}

DECLARE_BUILTINS(MapConstructor)
{
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);
    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> hashTableBuilder(this, glue, globalEnv);
    GateRef arg0 = GetArgFromArgv(glue, IntPtr(0), numArgs, true);
    hashTableBuilder.GenMapSetConstructor(nativeCode, func, newTarget, thisValue, numArgs, arg0, GetArgv());
}

DECLARE_BUILTINS(SetConstructor)
{
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);
    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> hashTableBuilder(this, glue, globalEnv);
    GateRef arg0 = GetArgFromArgv(glue, IntPtr(0), numArgs, true);
    hashTableBuilder.GenMapSetConstructor(nativeCode, func, newTarget, thisValue, numArgs, arg0, GetArgv());
}

#define DECLARE_BUILTINS_TYPED_ARRAY_STUB_BUILDER(TYPE, type, index)                                \
DECLARE_BUILTINS(type##ArrayConstructor)                                                            \
{                                                                                                   \
    GateRef ctorName = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,                     \
                                              ConstantIndex::TYPE##_ARRAY_STRING_INDEX);            \
    GateRef globalEnv = GetGlobalEnvFromFunction(glue, func);                                       \
    BuiltinsTypedArrayStubBuilder builtinsTypedArrayStubBuilder(this, globalEnv);                   \
    builtinsTypedArrayStubBuilder.GenTypedArrayConstructor(glue, nativeCode, func, newTarget,       \
        thisValue, numArgs, ctorName, DataViewType::TYPE);                                          \
}
BUILTIN_COMPILER_TYPED_ARRAY_TYPES(DECLARE_BUILTINS_TYPED_ARRAY_STUB_BUILDER)
#undef DECLARE_BUILTINS_TYPED_ARRAY_STUB_BUILDER
}  // namespace panda::ecmascript::kungfu
