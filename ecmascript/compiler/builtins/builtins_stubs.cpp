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

#include "ecmascript/compiler/builtins/builtins_stubs.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/builtins/builtins_array_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/builtins/builtins_function_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_number_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_vector_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_collection_stub_builder.h"
#include "ecmascript/compiler/builtins/builtins_object_stub_builder.h"
#include "ecmascript/compiler/codegen/llvm/llvm_ir_builder.h"
#include "ecmascript/compiler/typed_array_stub_builder.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_map.h"

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

GateRef BuiltinsStubBuilder::GetArg(GateRef numArgs, GateRef index)
{
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
        arg = Load(VariableType::JS_ANY(), argv, PtrMul(index, IntPtr(JSTaggedValue::TaggedTypeSize())));
        Jump(&exit);
    }
    Bind(&exit);
    GateRef ret = *arg;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStubBuilder::CallSlowPath(GateRef nativeCode, GateRef glue, GateRef thisValue,
                                          GateRef numArgs, GateRef func, GateRef newTarget, const char* comment)
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
        result = CallBuiltinRuntime(glue, args, false, comment);
        Jump(&exit);
    }
    Bind(&notcallThis0);
    {
        BRANCH(Int64Equal(numArgs, IntPtr(1)), &callThis1, &notcallThis1);
        Bind(&callThis1);
        {
            GateRef arg0 = GetCallArg0(numArgs);
            auto args = { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue, arg0 };
            result = CallBuiltinRuntime(glue, args, false, comment);
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
                result = CallBuiltinRuntime(glue, args, false, comment);
                Jump(&exit);
            }
            Bind(&callThis3);
            {
                GateRef arg0 = GetCallArg0(numArgs);
                GateRef arg1 = GetCallArg1(numArgs);
                GateRef arg2 = GetCallArg2(numArgs);
                auto args = { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue, arg0, arg1, arg2 };
                result = CallBuiltinRuntime(glue, args, false, comment);
                Jump(&exit);
            }
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

#define DECLARE_BUILTINS_WITH_STRING_STUB_BUILDER(method, resultVariableType, initValue)            \
DECLARE_BUILTINS(String##method)                                                                    \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, VariableType::resultVariableType(), initValue);                                \
    Label exit(env);                                                                                \
    Label slowPath(env);                                                                            \
    BuiltinsStringStubBuilder stringStubBuilder(this);                                              \
    stringStubBuilder.method(glue, thisValue, numArgs, &res, &exit, &slowPath);                     \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(String##method));                  \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());    \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

#define BUILTINS_WITH_STRING_STUB_BUILDER(V)                                        \
    V(CharAt,       JS_POINTER, Hole())                                             \
    V(FromCharCode, JS_ANY,     Hole())                                             \
    V(CharCodeAt,   JS_ANY,     DoubleToTaggedDoublePtr(Double(base::NAN_VALUE)))   \
    V(CodePointAt,  JS_ANY,     Undefined())                                        \
    V(IndexOf,      JS_ANY,     IntToTaggedPtr(Int32(-1)))                          \
    V(Substring,    JS_ANY,     IntToTaggedPtr(Int32(-1)))                          \
    V(Replace,      JS_ANY,     Undefined())                                        \
    V(Trim,         JS_ANY,     Undefined())                                        \
    V(Concat,       JS_ANY,     Undefined())                                        \
    V(Slice,        JS_ANY,     Undefined())                                        \
    V(ToLowerCase,  JS_ANY,     Undefined())                                        \
    V(StartsWith,   JS_ANY,     TaggedFalse())                                      \
    V(EndsWith,     JS_ANY,     TaggedFalse())

DECLARE_BUILTINS(LocaleCompare)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label slowPath(env);
    BuiltinsStringStubBuilder stringStubBuilder(this);
    stringStubBuilder.LocaleCompare(glue, thisValue, numArgs, &res, &exit, &slowPath);
    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(LocaleCompare));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(GetStringIterator)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label slowPath(env);
    BuiltinsStringStubBuilder stringStubBuilder(this);
    stringStubBuilder.GetStringIterator(glue, thisValue, numArgs, &res, &exit, &slowPath);
    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(GetStringIterator));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(STRING_ITERATOR_PROTO_NEXT)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label slowPath(env);
    BuiltinsStringStubBuilder stringStubBuilder(this);
    stringStubBuilder.StringIteratorNext(glue, thisValue, numArgs, &res, &exit, &slowPath);
    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(STRING_ITERATOR_PROTO_NEXT));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

BUILTINS_WITH_STRING_STUB_BUILDER(DECLARE_BUILTINS_WITH_STRING_STUB_BUILDER)

#undef DECLARE_BUILTINS_WITH_STRING_STUB_BUILDER
#undef BUILTINS_WITH_STRING_STUB_BUILDER

DECLARE_BUILTINS(FunctionPrototypeApply)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label slowPath(env);
    BuiltinsFunctionStubBuilder functionStubBuilder(this);
    functionStubBuilder.Apply(glue, thisValue, numArgs, &res, &exit, &slowPath);
    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(FunctionPrototypeApply));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

#define DECLARE_BUILTINS_WITH_CONTAINERS_STUB_BUILDER(StubName, Method, methodType, resultVariableType)     \
DECLARE_BUILTINS(StubName)                                                                                  \
{                                                                                                           \
    auto env = GetEnvironment();                                                                            \
    DEFVARIABLE(res, VariableType::resultVariableType(), Undefined());                                      \
    Label exit(env);                                                                                        \
    Label slowPath(env);                                                                                    \
    ContainersStubBuilder containersBuilder(this);                                                          \
    containersBuilder.Method(glue, thisValue, numArgs, &res, &exit, &slowPath, ContainersType::methodType); \
    Bind(&slowPath);                                                                                        \
    {                                                                                                       \
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(StubName));                                \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());            \
        Jump(&exit);                                                                                        \
    }                                                                                                       \
    Bind(&exit);                                                                                            \
    Return(*res);                                                                                           \
}

#define BUILTINS_WITH_CONTAINERS_STUB_BUILDER(V) \
    V(ArrayListForEach,            ContainersCommonFuncCall,  ARRAYLIST_FOREACH,            JS_POINTER) \
    V(DequeForEach,                DequeCommonFuncCall,       DEQUE_FOREACH,                JS_POINTER) \
    V(HashMapForEach,              ContainersHashCall,        HASHMAP_FOREACH,              JS_POINTER) \
    V(HashSetForEach,              ContainersHashCall,        HASHSET_FOREACH,              JS_POINTER) \
    V(LightWeightMapForEach,       ContainersLightWeightCall, LIGHTWEIGHTMAP_FOREACH,       JS_POINTER) \
    V(LightWeightSetForEach,       ContainersLightWeightCall, LIGHTWEIGHTSET_FOREACH,       JS_POINTER) \
    V(LinkedListForEach,           ContainersLinkedListCall,  LINKEDLIST_FOREACH,           JS_POINTER) \
    V(ListForEach,                 ContainersLinkedListCall,  LIST_FOREACH,                 JS_POINTER) \
    V(PlainArrayForEach,           ContainersCommonFuncCall,  PLAINARRAY_FOREACH,           JS_POINTER) \
    V(QueueForEach,                QueueCommonFuncCall,       QUEUE_FOREACH,                JS_POINTER) \
    V(StackForEach,                ContainersCommonFuncCall,  STACK_FOREACH,                JS_POINTER) \
    V(VectorForEach,               ContainersCommonFuncCall,  VECTOR_FOREACH,               JS_POINTER) \
    V(ArrayListReplaceAllElements, ContainersCommonFuncCall,  ARRAYLIST_REPLACEALLELEMENTS, JS_POINTER) \
    V(VectorReplaceAllElements,    ContainersCommonFuncCall,  VECTOR_REPLACEALLELEMENTS,    JS_POINTER)

BUILTINS_WITH_CONTAINERS_STUB_BUILDER(DECLARE_BUILTINS_WITH_CONTAINERS_STUB_BUILDER)

#undef DECLARE_BUILTINS_WITH_CONTAINERS_STUB_BUILDER
#undef BUILTINS_WITH_CONTAINERS_STUB_BUILDER

#define DECLARE_BUILTINS_WITH_ARRAY_STUB_BUILDER(Method, resultVariableType)                        \
DECLARE_BUILTINS(Array##Method)                                                                     \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, VariableType::resultVariableType(), Undefined());                              \
    Label exit(env);                                                                                \
    Label slowPath(env);                                                                            \
    BuiltinsArrayStubBuilder arrayStubBuilder(this);                                                \
    arrayStubBuilder.Method(glue, thisValue, numArgs, &res, &exit, &slowPath);                      \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(Array##Method));                   \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());    \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

#define BUILTINS_WITH_ARRAY_STUB_BUILDER(V) \
    V(Concat,       JS_ANY)                 \
    V(Filter,       JS_POINTER)             \
    V(Find,         JS_ANY)                 \
    V(FindIndex,    JS_ANY)                 \
    V(From,         JS_ANY)                 \
    V(Splice,       JS_ANY)                 \
    V(ForEach,      JS_ANY)                 \
    V(IndexOf,      JS_ANY)                 \
    V(LastIndexOf,  JS_ANY)                 \
    V(Pop,          JS_ANY)                 \
    V(Slice,        JS_POINTER)             \
    V(Reduce,       JS_ANY)                 \
    V(Reverse,      JS_POINTER)             \
    V(Push,         JS_ANY)                 \
    V(Values,       JS_POINTER)             \
    V(Includes,     JS_ANY)                 \
    V(CopyWithin,   JS_ANY)                 \
    V(Map,          JS_ANY)

DECLARE_BUILTINS(SORT)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    Label exit(env);
    Label slowPath(env);
    BuiltinsArrayStubBuilder arrayStubBuilder(this);
    arrayStubBuilder.Sort(glue, thisValue, numArgs, &res, &exit, &slowPath);
    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(SORT));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

BUILTINS_WITH_ARRAY_STUB_BUILDER(DECLARE_BUILTINS_WITH_ARRAY_STUB_BUILDER)

#undef DECLARE_BUILTINS_WITH_ARRAY_STUB_BUILDER
#undef BUILTINS_WITH_ARRAY_STUB_BUILDER

DECLARE_BUILTINS(BooleanConstructor)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsHeapObject(env);
    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label slowPath2(env);
    Label exit(env);

    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath1);
    Bind(&newTargetIsHeapObject);
    BRANCH(IsJSFunction(newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label intialHClassIsHClass(env);
        GateRef intialHClass = Load(VariableType::JS_ANY(), newTarget,
                                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        BRANCH(IsJSHClass(intialHClass), &intialHClassIsHClass, &slowPath2);
        Bind(&intialHClassIsHClass);
        {
            NewObjectStubBuilder newBuilder(this);
            newBuilder.SetParameters(glue, 0);
            Label afterNew(env);
            newBuilder.NewJSObject(&res, &afterNew, intialHClass);
            Bind(&afterNew);
            {
                GateRef valueOffset = IntPtr(JSPrimitiveRef::VALUE_OFFSET);
                GateRef value = GetArg(numArgs, IntPtr(0));
                Store(VariableType::INT64(), glue, *res, valueOffset, FastToBoolean(value));
                Jump(&exit);
            }
        }
        Bind(&slowPath2);
        {
            auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(BooleanConstructor));
            GateRef argv = GetArgv();
            auto args = { glue, nativeCode, func, thisValue, numArgs, argv, newTarget };
            res = CallBuiltinRuntimeWithNewTarget(glue, args, name.c_str());
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(BooleanConstructor));
        GateRef argv = GetArgv();
        auto args = { glue, nativeCode, func, thisValue, numArgs, argv };
        res = CallBuiltinRuntime(glue, args, true, name.c_str());
        Jump(&exit);
    }
    Bind(&slowPath1);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(BooleanConstructor));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(NumberConstructor)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(numberValue, VariableType::JS_ANY(), IntToTaggedPtr(IntPtr(0)));
    Label thisCollectionObj(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label slowPath2(env);
    Label exit(env);

    Label hasArg(env);
    Label numberCreate(env);
    Label newTargetIsHeapObject(env);
    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath1);
    Bind(&newTargetIsHeapObject);
    BRANCH(Int64GreaterThan(numArgs, IntPtr(0)), &hasArg, &numberCreate);
    Bind(&hasArg);
    {
        GateRef value = GetArgNCheck(Int32(0));
        Label number(env);
        BRANCH(TaggedIsNumber(value), &number, &slowPath);
        Bind(&number);
        {
            numberValue = value;
            res = value;
            Jump(&numberCreate);
        }
    }

    Bind(&numberCreate);
    Label newObj(env);
    Label newTargetIsJSFunction(env);
    BRANCH(TaggedIsUndefined(newTarget), &exit, &newObj);
    Bind(&newObj);
    {
        BRANCH(IsJSFunction(newTarget), &newTargetIsJSFunction, &slowPath);
        Bind(&newTargetIsJSFunction);
        {
            Label intialHClassIsHClass(env);
            GateRef intialHClass = Load(VariableType::JS_ANY(), newTarget,
                IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
            BRANCH(IsJSHClass(intialHClass), &intialHClassIsHClass, &slowPath2);
            Bind(&intialHClassIsHClass);
            {
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue, 0);
                Label afterNew(env);
                newBuilder.NewJSObject(&res, &afterNew, intialHClass);
                Bind(&afterNew);
                {
                    GateRef valueOffset = IntPtr(JSPrimitiveRef::VALUE_OFFSET);
                    Store(VariableType::INT64(), glue, *res, valueOffset, *numberValue);
                    Jump(&exit);
                }
            }
            Bind(&slowPath2);
            {
                auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(NumberConstructor));
                GateRef argv = GetArgv();
                res = CallBuiltinRuntimeWithNewTarget(glue,
                    { glue, nativeCode, func, thisValue, numArgs, argv, newTarget }, name.c_str());
                Jump(&exit);
            }
        }
    }

    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(NumberConstructor));
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue, { glue, nativeCode, func, thisValue, numArgs, argv }, true, name.c_str());
        Jump(&exit);
    }
    Bind(&slowPath1);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(NumberConstructor));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
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
    Label slowPath2(env);
    Label exit(env);

    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath1);
    Bind(&newTargetIsHeapObject);
    BRANCH(IsJSFunction(newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label intialHClassIsHClass(env);
        GateRef intialHClass = Load(VariableType::JS_ANY(), newTarget,
                                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        BRANCH(IsJSHClass(intialHClass), &intialHClassIsHClass, &slowPath2);
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
                GateRef value = GetArgNCheck(IntPtr(0));
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
                    GateRef year = GetArgNCheck(IntPtr(0));
                    GateRef month = GetArgNCheck(IntPtr(1));
                    GateRef day = GetArgNCheck(IntPtr(2));
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
        Bind(&slowPath2);
        {
            auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(DateConstructor));
            GateRef argv = GetArgv();
            res = CallBuiltinRuntimeWithNewTarget(glue, { glue, nativeCode, func, thisValue, numArgs, argv, newTarget },
                name.c_str());
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(DateConstructor));
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue, { glue, nativeCode, func, thisValue, numArgs, argv }, true, name.c_str());
        Jump(&exit);
    }
    Bind(&slowPath1);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(DateConstructor));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(ArrayConstructor)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsHeapObject(env);
    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label slowPath2(env);
    Label exit(env);

    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath1);
    Bind(&newTargetIsHeapObject);
    BRANCH(IsJSFunction(newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label fastGetHclass(env);
        Label intialHClassIsHClass(env);
        GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
        GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
        auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
        BRANCH(Equal(arrayFunc, newTarget), &fastGetHclass, &slowPath2);
        Bind(&fastGetHclass);
        GateRef intialHClass = Load(VariableType::JS_ANY(), newTarget, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        DEFVARIABLE(arrayLength, VariableType::INT64(), Int64(0));
        BRANCH(IsJSHClass(intialHClass), &intialHClassIsHClass, &slowPath2);
        Bind(&intialHClassIsHClass);
        {
            Label noArg(env);
            Label hasArg(env);
            Label arrayCreate(env);
            BRANCH(Int64Equal(numArgs, IntPtr(0)), &noArg, &hasArg);
            Bind(&noArg);
            {
                Jump(&arrayCreate);
            }
            Bind(&hasArg);
            {
                Label hasOneArg(env);
                BRANCH(Int64Equal(numArgs, IntPtr(1)), &hasOneArg, &slowPath);
                Bind(&hasOneArg);
                {
                    Label argIsNumber(env);
                    GateRef arg0 = GetArg(numArgs, IntPtr(0));
                    BRANCH(TaggedIsNumber(arg0), &argIsNumber, &slowPath);
                    Bind(&argIsNumber);
                    {
                        Label argIsInt(env);
                        Label argIsDouble(env);
                        BRANCH(TaggedIsInt(arg0), &argIsInt, &argIsDouble);
                        Bind(&argIsInt);
                        {
                            Label validIntLength(env);
                            GateRef intLen = GetInt64OfTInt(arg0);
                            GateRef isGEZero = Int64GreaterThanOrEqual(intLen, Int64(0));
                            GateRef isLEMaxLen = Int64LessThanOrEqual(intLen, Int64(JSArray::MAX_ARRAY_INDEX));
                            BRANCH(BoolAnd(isGEZero, isLEMaxLen), &validIntLength, &slowPath);
                            Bind(&validIntLength);
                            {
                                arrayLength = intLen;
                                Jump(&arrayCreate);
                            }
                        }
                        Bind(&argIsDouble);
                        {
                            Label validDoubleLength(env);
                            GateRef doubleLength = GetDoubleOfTDouble(arg0);
                            GateRef doubleToInt = DoubleToInt(glue, doubleLength);
                            GateRef intToDouble = CastInt64ToFloat64(SExtInt32ToInt64(doubleToInt));
                            GateRef doubleEqual = DoubleEqual(doubleLength, intToDouble);
                            GateRef doubleLEMaxLen =
                                DoubleLessThanOrEqual(doubleLength, Double(JSArray::MAX_ARRAY_INDEX));
                            BRANCH(BoolAnd(doubleEqual, doubleLEMaxLen), &validDoubleLength, &slowPath);
                            Bind(&validDoubleLength);
                            {
                                arrayLength = SExtInt32ToInt64(doubleToInt);
                                Jump(&arrayCreate);
                            }
                        }
                    }
                }
            }
            Bind(&arrayCreate);
            {
                Label lengthValid(env);
                BRANCH(Int64GreaterThan(*arrayLength, Int64(JSObject::MAX_GAP)), &slowPath, &lengthValid);
                Bind(&lengthValid);
                {
                    NewObjectStubBuilder newBuilder(this);
                    newBuilder.SetParameters(glue, 0);
                    res = newBuilder.NewJSArrayWithSize(intialHClass, *arrayLength);
                    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
                    Store(VariableType::INT32(), glue, *res, lengthOffset, TruncInt64ToInt32(*arrayLength));
                    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                              ConstantIndex::ARRAY_LENGTH_ACCESSOR);
                    SetPropertyInlinedProps(glue, *res, intialHClass, accessor,
                                            Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
                    SetExtensibleToBitfield(glue, *res, true);
                    Jump(&exit);
                }
            }
        }
        Bind(&slowPath2);
        {
            auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(ArrayConstructor));
            GateRef argv = GetArgv();
            res = CallBuiltinRuntimeWithNewTarget(glue, { glue, nativeCode, func, thisValue, numArgs, argv, newTarget },
                name.c_str());
            Jump(&exit);
        }
    }
    Bind(&slowPath);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(ArrayConstructor));
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue, { glue, nativeCode, func, thisValue, numArgs, argv }, true, name.c_str());
        Jump(&exit);
    }
    Bind(&slowPath1);
    {
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(ArrayConstructor));
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());
        Jump(&exit);
    }

    Bind(&exit);
    Return(*res);
}

#define DECLARE_BUILTINS_OBJECT_STUB_BUILDER(type, method, retType, retDefaultValue)                \
DECLARE_BUILTINS(type##method)                                                                      \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, retType, retDefaultValue);                                                     \
    Label thisCollectionObj(env);                                                                   \
    Label slowPath(env);                                                                            \
    Label exit(env);                                                                                \
    BuiltinsObjectStubBuilder builder(this, glue, thisValue, numArgs);                              \
    builder.method(&res, &exit, &slowPath);                                                         \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(type##method));                    \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());    \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

// Object.protetype.ToString
DECLARE_BUILTINS_OBJECT_STUB_BUILDER(Object, ToString, VariableType::JS_ANY(), Undefined());
// Object.protetype.Create
DECLARE_BUILTINS_OBJECT_STUB_BUILDER(Object, Create, VariableType::JS_ANY(), Undefined());
// Object.protetype.Assign
DECLARE_BUILTINS_OBJECT_STUB_BUILDER(Object, Assign, VariableType::JS_ANY(), Undefined());
// Object.protetype.HasOwnProperty
DECLARE_BUILTINS_OBJECT_STUB_BUILDER(Object, HasOwnProperty, VariableType::JS_ANY(), TaggedFalse());
// Object.protetype.Keys
DECLARE_BUILTINS_OBJECT_STUB_BUILDER(Object, Keys, VariableType::JS_ANY(), Undefined());
#undef DECLARE_BUILTINS_OBJECT_STUB_BUILDER

DECLARE_BUILTINS(MapConstructor)
{
    LinkedHashTableStubBuilder<LinkedHashMap, LinkedHashMapObject> hashTableBuilder(this, glue);
    hashTableBuilder.GenMapSetConstructor(nativeCode, func, newTarget, thisValue, numArgs);
}

DECLARE_BUILTINS(SetConstructor)
{
    LinkedHashTableStubBuilder<LinkedHashSet, LinkedHashSetObject> hashTableBuilder(this, glue);
    hashTableBuilder.GenMapSetConstructor(nativeCode, func, newTarget, thisValue, numArgs);
}

#define DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(type, method, retType, retDefaultValue)            \
DECLARE_BUILTINS(type##method)                                                                      \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, retType, retDefaultValue);                                                     \
    Label slowPath(env);                                                                            \
    Label exit(env);                                                                                \
    BuiltinsCollectionStubBuilder<JS##type> builder(this, glue, thisValue, numArgs);                \
    builder.method(&res, &exit, &slowPath);                                                         \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(type##method));                    \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());    \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

// Set.protetype.Clear
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Set, Clear, VariableType::JS_ANY(), Undefined());
// Set.protetype.Values
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Set, Values, VariableType::JS_ANY(), Undefined());
// Set.protetype.Entries
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Set, Entries, VariableType::JS_ANY(), Undefined());
// Set.protetype.ForEach
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Set, ForEach, VariableType::JS_ANY(), Undefined());
// Set.protetype.Add
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Set, Add, VariableType::JS_ANY(), Undefined());
// Set.protetype.Delete
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Set, Delete, VariableType::JS_ANY(), Undefined());
// Set.protetype.Has
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Set, Has, VariableType::JS_ANY(), Undefined());
// Map.protetype.Clear
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Map, Clear, VariableType::JS_ANY(), Undefined());
// Map.protetype.Values
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Map, Values, VariableType::JS_ANY(), Undefined());
// Map.protetype.Entries
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Map, Entries, VariableType::JS_ANY(), Undefined());
// Map.protetype.Keys
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Map, Keys, VariableType::JS_ANY(), Undefined());
// Map.protetype.ForEach
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Map, ForEach, VariableType::JS_ANY(), Undefined());
// Map.protetype.set
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Map, Set, VariableType::JS_ANY(), Undefined());
// Map.protetype.Delete
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Map, Delete, VariableType::JS_ANY(), Undefined());
// Map.protetype.Has
DECLARE_BUILTINS_COLLECTION_STUB_BUILDER(Map, Has, VariableType::JS_ANY(), Undefined());
#undef DECLARE_BUILTINS_COLLECTION_STUB_BUILDER

#define DECLARE_BUILTINS_NUMBER_STUB_BUILDER(type, method, retType, retDefaultValue)                \
DECLARE_BUILTINS(type##method)                                                                      \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, retType, retDefaultValue);                                                     \
    Label slowPath(env);                                                                            \
    Label exit(env);                                                                                \
    BuiltinsNumberStubBuilder builder(this, glue, thisValue, numArgs);                              \
    builder.method(&res, &exit, &slowPath);                                                         \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(type##method));                    \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());    \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

// Number.ParseFloat
DECLARE_BUILTINS_NUMBER_STUB_BUILDER(Number, ParseFloat, VariableType::JS_ANY(), Undefined());
#undef DECLARE_BUILTINS_NUMBER_STUB_BUILDER

#define DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER(type, method, retType, retDefaultValue)            \
DECLARE_BUILTINS(type##method)                                                                      \
{                                                                                                   \
    auto env = GetEnvironment();                                                                    \
    DEFVARIABLE(res, retType, retDefaultValue);                                                     \
    Label slowPath(env);                                                                            \
    Label exit(env);                                                                                \
    TypedArrayStubBuilder builder(this);                                                            \
    builder.method(glue, thisValue, numArgs, &res, &exit, &slowPath);                               \
    Bind(&slowPath);                                                                                \
    {                                                                                               \
        auto name = BuiltinsStubCSigns::GetName(BUILTINS_STUB_ID(type##method));                    \
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget, name.c_str());    \
        Jump(&exit);                                                                                \
    }                                                                                               \
    Bind(&exit);                                                                                    \
    Return(*res);                                                                                   \
}

// TypedArray.Subarray
DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER(TypedArray, SubArray, VariableType::JS_ANY(), Undefined());
DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER(TypedArray, GetByteLength, VariableType::JS_ANY(), Undefined());
DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER(TypedArray, GetByteOffset, VariableType::JS_ANY(), Undefined());
#undef DECLARE_BUILTINS_TYPEDARRAY_STUB_BUILDER
}  // namespace panda::ecmascript::kungfu
