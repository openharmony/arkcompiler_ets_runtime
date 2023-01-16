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
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_vector_stub_builder.h"
#include "ecmascript/compiler/builtins/containers_stub_builder.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_primitive_ref.h"

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
    Branch(IntPtrGreaterThan(numArgs, index), &validIndex, &exit);
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
    Branch(Int64Equal(numArgs, IntPtr(0)), &callThis0, &notcallThis0);
    Bind(&callThis0);
    {
        result = CallBuiltinRuntime(glue, { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue });
        Jump(&exit);
    }
    Bind(&notcallThis0);
    {
        Branch(Int64Equal(numArgs, IntPtr(1)), &callThis1, &notcallThis1);
        Bind(&callThis1);
        {
            GateRef arg0 = GetCallArg0();
            result =
                CallBuiltinRuntime(glue, { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue, arg0 });
            Jump(&exit);
        }
        Bind(&notcallThis1);
        {
            Branch(Int64Equal(numArgs, IntPtr(2)), &callThis2, &callThis3); // 2: args2
            Bind(&callThis2);
            {
                GateRef arg0 = GetCallArg0();
                GateRef arg1 = GetCallArg1();
                result = CallBuiltinRuntime(glue,
                    { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue, arg0, arg1 });
                Jump(&exit);
            }
            Bind(&callThis3);
            {
                GateRef arg0 = GetCallArg0();
                GateRef arg1 = GetCallArg1();
                GateRef arg2 = GetCallArg2();
                result = CallBuiltinRuntime(glue,
                    { nativeCode, glue, runtimeCallInfoArgs, func, newTarget, thisValue, arg0, arg1, arg2 });
                Jump(&exit);
            }
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

DECLARE_BUILTINS(CharCodeAt)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), DoubleToTaggedDoublePtr(Double(base::NAN_VALUE)));
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));

    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label slowPath(env);
    Label next(env);
    Label posTagNotUndefined(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label posNotGreaterLen(env);
    Label posNotLessZero(env);
    Label exit(env);
    Label posTagIsDouble(env);
    Label thisIsHeapobject(env);

    Branch(TaggedIsUndefinedOrNull(thisValue), &slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, &slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, &slowPath);
        Bind(&isString);
        {
            GateRef thisLen = GetLengthFromString(thisValue);
            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &posTagNotUndefined);
            Bind(&posTagNotUndefined);
            {
                GateRef posTag = GetCallArg0();
                Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                pos = GetInt32OfTInt(posTag);
                Jump(&next);
                Bind(&posTagNotInt);
                Branch(TaggedIsDouble(posTag), &posTagIsDouble, &slowPath);
                Bind(&posTagIsDouble);
                pos = DoubleToInt(glue, GetDoubleOfTDouble(posTag));
                Jump(&next);
            }
            Bind(&next);
            {
                Branch(Int32GreaterThanOrEqual(*pos, thisLen), &exit, &posNotGreaterLen);
                Bind(&posNotGreaterLen);
                {
                    Branch(Int32LessThan(*pos, Int32(0)), &exit, &posNotLessZero);
                    Bind(&posNotLessZero);
                    {
                        BuiltinsStringStubBuilder stringBuilder(this);
                        res = IntToTaggedPtr(stringBuilder.StringAt(thisValue, *pos));
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(IndexOf)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), IntToTaggedPtr(Int32(-1)));
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
    
    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label isSearchString(env);
    Label slowPath(env);
    Label next(env);
    Label resPosGreaterZero(env);
    Label searchTagIsHeapObject(env);
    Label posTagNotUndefined(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label exit(env);
    Label posTagIsDouble(env);
    Label nextCount(env);
    Label posNotLessThanLen(env);
    Label thisIsHeapobject(env);

    Branch(TaggedIsUndefinedOrNull(thisValue), &slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, &slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, &slowPath);
        Bind(&isString);
        {
            GateRef searchTag = GetCallArg0();
            Branch(TaggedIsHeapObject(searchTag), &searchTagIsHeapObject, &slowPath);
            Bind(&searchTagIsHeapObject);
            Branch(IsString(searchTag), &isSearchString, &slowPath);
            Bind(&isSearchString);
            {
                GateRef thisLen = GetLengthFromString(thisValue);
                Branch(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &next, &posTagNotUndefined);
                Bind(&posTagNotUndefined);
                {
                    GateRef posTag = GetCallArg1();
                    Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                    Bind(&posTagIsInt);
                    pos = GetInt32OfTInt(posTag);
                    Jump(&next);
                    Bind(&posTagNotInt);
                    Branch(TaggedIsDouble(posTag), &posTagIsDouble, &slowPath);
                    Bind(&posTagIsDouble);
                    pos = DoubleToInt(glue, GetDoubleOfTDouble(posTag));
                    Jump(&next);
                }
                Bind(&next);
                {
                    Label posGreaterThanZero(env);
                    Label posNotGreaterThanZero(env);
                    Branch(Int32GreaterThan(*pos, Int32(0)), &posGreaterThanZero, &posNotGreaterThanZero);
                    Bind(&posNotGreaterThanZero);
                    {
                        pos = Int32(0);
                        Jump(&nextCount);
                    }
                    Bind(&posGreaterThanZero);
                    {
                        Branch(Int32LessThanOrEqual(*pos, thisLen), &nextCount, &posNotLessThanLen);
                        Bind(&posNotLessThanLen);
                        {
                            pos = thisLen;
                            Jump(&nextCount);
                        }
                    }
                    Bind(&nextCount);
                    {
                        BuiltinsStringStubBuilder stringBuilder(this);
                        GateRef resPos = stringBuilder.StringIndexOf(thisValue, searchTag, *pos);
                        Branch(Int32GreaterThanOrEqual(resPos, Int32(0)), &resPosGreaterZero, &exit);
                        Bind(&resPosGreaterZero);
                        {
                            Label resPosLessZero(env);
                            Branch(Int32LessThanOrEqual(resPos, thisLen), &resPosLessZero, &exit);
                            Bind(&resPosLessZero);
                            {
                                res = IntToTaggedPtr(resPos);
                                Jump(&exit);
                            }
                        }
                    }
                }
            }
        }
    }
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(Substring)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), IntToTaggedPtr(Int32(-1)));
    DEFVARIABLE(start, VariableType::INT32(), Int32(0));
    DEFVARIABLE(end, VariableType::INT32(), Int32(0));
    DEFVARIABLE(from, VariableType::INT32(), Int32(0));
    DEFVARIABLE(to, VariableType::INT32(), Int32(0));

    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label isSearchString(env);
    Label slowPath(env);
    Label countStart(env);
    Label endTagIsUndefined(env);
    Label startNotGreatZero(env);
    Label countEnd(env);
    Label endNotGreatZero(env);
    Label countFrom(env);
    Label countRes(env);
    Label startTagNotUndefined(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label exit(env);
    Label posTagIsDouble(env);
    Label endTagNotUndefined(env);
    Label endTagIsInt(env);
    Label endTagNotInt(env);
    Label endTagIsDouble(env);
    Label endGreatZero(env);
    Label endGreatLen(env);
    Label startGreatZero(env);
    Label startGreatEnd(env);
    Label startNotGreatEnd(env);
    Label thisIsHeapobject(env);

    Branch(TaggedIsUndefinedOrNull(thisValue), &slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, &slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, &slowPath);
        Bind(&isString);
        {
            Label next(env);
            GateRef thisLen = GetLengthFromString(thisValue);
            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &startTagNotUndefined);
            Bind(&startTagNotUndefined);
            {
                GateRef startTag = GetCallArg0();
                Branch(TaggedIsInt(startTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                start = GetInt32OfTInt(startTag);
                Jump(&next);
                Bind(&posTagNotInt);
                Branch(TaggedIsDouble(startTag), &posTagIsDouble, &slowPath);
                Bind(&posTagIsDouble);
                start = DoubleToInt(glue, GetDoubleOfTDouble(startTag));
                Jump(&next);
            }
            Bind(&next);
            {
                Branch(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &endTagIsUndefined, &endTagNotUndefined);
                Bind(&endTagIsUndefined);
                {
                    end = thisLen;
                    Jump(&countStart);
                }
                Bind(&endTagNotUndefined);
                {
                    GateRef endTag = GetCallArg1();
                    Branch(TaggedIsInt(endTag), &endTagIsInt, &endTagNotInt);
                    Bind(&endTagIsInt);
                    end = GetInt32OfTInt(endTag);
                    Jump(&countStart);
                    Bind(&endTagNotInt);
                    Branch(TaggedIsDouble(endTag), &endTagIsDouble, &slowPath);
                    Bind(&endTagIsDouble);
                    end = DoubleToInt(glue, GetDoubleOfTDouble(endTag));
                    Jump(&countStart);
                }
            }
            Bind(&countStart);
            {
                Label startGreatLen(env);
                Branch(Int32GreaterThan(*start, Int32(0)), &startGreatZero, &startNotGreatZero);
                Bind(&startNotGreatZero);
                {
                    start = Int32(0);
                    Jump(&countEnd);
                }
                Bind(&startGreatZero);
                {
                    Branch(Int32GreaterThan(*start, thisLen), &startGreatLen, &countEnd);
                    Bind(&startGreatLen);
                    {
                        start = thisLen;
                        Jump(&countEnd);
                    }
                }
            }
            Bind(&countEnd);
            {
                Branch(Int32GreaterThan(*end, Int32(0)), &endGreatZero, &endNotGreatZero);
                Bind(&endNotGreatZero);
                {
                    end = Int32(0);
                    Jump(&countFrom);
                }
                Bind(&endGreatZero);
                {
                    Branch(Int32GreaterThan(*end, thisLen), &endGreatLen, &countFrom);
                    Bind(&endGreatLen);
                    {
                        end = thisLen;
                        Jump(&countFrom);
                    }
                }
            }
            Bind(&countFrom);
            {
                Branch(Int32GreaterThan(*start, *end), &startGreatEnd, &startNotGreatEnd);
                Bind(&startGreatEnd);
                {
                    from = *end;
                    to = *start;
                    Jump(&countRes);
                }
                Bind(&startNotGreatEnd);
                {
                    from = *start;
                    to = *end;
                    Jump(&countRes);
                }
            }
            Bind(&countRes);
            {
                BuiltinsStringStubBuilder stringBuilder(this);
                GateRef len = Int32Sub(*to, *from);
                Label isUtf8(env);
                Label isUtf16(env);
                Branch(IsUtf8String(thisValue), &isUtf8, &isUtf16);
                Bind(&isUtf8);
                {
                    res = stringBuilder.FastSubUtf8String(glue, thisValue, *from, len);
                    Jump(&exit);
                }
                Bind(&isUtf16);
                {
                    res = stringBuilder.FastSubUtf16String(glue, thisValue, *from, len);
                    Jump(&exit);
                }
            }
        }
    }

    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(CharAt)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Hole());
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));

    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label slowPath(env);
    Label next(env);
    Label posTagNotUndefined(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label posNotGreaterLen(env);
    Label posGreaterLen(env);
    Label posNotLessZero(env);
    Label exit(env);
    Label posTagIsDouble(env);
    Label thisIsHeapobject(env);

    Branch(TaggedIsUndefinedOrNull(thisValue), &slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, &slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, &slowPath);
        Bind(&isString);
        {
            GateRef thisLen = GetLengthFromString(thisValue);
            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &posTagNotUndefined);
            Bind(&posTagNotUndefined);
            {
                GateRef posTag = GetCallArg0();
                Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                pos = GetInt32OfTInt(posTag);
                Jump(&next);
                Bind(&posTagNotInt);
                Branch(TaggedIsDouble(posTag), &posTagIsDouble, &slowPath);
                Bind(&posTagIsDouble);
                pos = DoubleToInt(glue, GetDoubleOfTDouble(posTag));
                Jump(&next);
            }
            Bind(&next);
            {
                Branch(Int32GreaterThanOrEqual(*pos, thisLen), &posGreaterLen, &posNotGreaterLen);
                Bind(&posNotGreaterLen);
                {
                    Branch(Int32LessThan(*pos, Int32(0)), &posGreaterLen, &posNotLessZero);
                    Bind(&posNotLessZero);
                    {
                        BuiltinsStringStubBuilder stringBuilder(this);
                        res = stringBuilder.CreateFromEcmaString(glue, thisValue, *pos);
                        Jump(&exit);
                    }
                }
                Bind(&posGreaterLen);
                {
                    res = GetGlobalConstantValue(
                        VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(VectorForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);
    
    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersCommonFuncCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::VECTOR_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(VectorReplaceAllElements)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);
    
    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersCommonFuncCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::VECTOR_REPLACEALLELEMENTS);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(StackForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);
    
    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersCommonFuncCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::STACK_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(PlainArrayForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);
    
    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersCommonFuncCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::PLAINARRAY_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(QueueForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);
    
    ContainersStubBuilder containersBuilder(this);
    containersBuilder.QueueCommonFuncCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::QUEUE_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(DequeForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);
    
    ContainersStubBuilder containersBuilder(this);
    containersBuilder.DequeCommonFuncCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::DEQUE_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(LightWeightMapForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);

    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersLightWeightCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::LIGHTWEIGHTMAP_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(LightWeightSetForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);

    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersLightWeightCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::LIGHTWEIGHTSET_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(HashMapForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);

    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersHashCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::HASHMAP_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(HashSetForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);

    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersHashCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::HASHSET_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(LinkedListForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);

    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersLinkedListCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::LINKEDLIST_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(ListForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);

    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersLinkedListCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::LIST_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(ArrayListForEach)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);
    
    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersCommonFuncCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::ARRAYLIST_FOREACH);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(ArrayListReplaceAllElements)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label slowPath(env);
    
    ContainersStubBuilder containersBuilder(this);
    containersBuilder.ContainersCommonFuncCall(glue, thisValue, numArgs, &res, &exit,
        &slowPath, ContainersType::ARRAYLIST_REPLACEALLELEMENTS);
    Bind(&slowPath);
    {
        res = CallSlowPath(nativeCode, glue, thisValue, numArgs, func, newTarget);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(BooleanConstructor)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label exit(env);

    Branch(IsJSFunction(newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label intialHClassIsHClass(env);
        GateRef intialHClass = Load(VariableType::JS_ANY(), newTarget,
                                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        Branch(IsJSHClass(intialHClass), &intialHClassIsHClass, &slowPath);
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

DECLARE_BUILTINS(DateConstructor)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label exit(env);

    Branch(IsJSFunction(newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label intialHClassIsHClass(env);
        GateRef intialHClass = Load(VariableType::JS_ANY(), newTarget,
                                    IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        Branch(IsJSHClass(intialHClass), &intialHClassIsHClass, &slowPath);
        Bind(&intialHClassIsHClass);
        {
            Label oneArg(env);
            Label notOneArg(env);
            Label newJSDate(env);
            DEFVARIABLE(timeValue, VariableType::FLOAT64(), Double(0));
            Branch(Int64Equal(numArgs, IntPtr(1)), &oneArg, &notOneArg);
            Bind(&oneArg);
            {
                Label valueIsNumber(env);
                GateRef value = GetArgNCheck(IntPtr(0));
                Branch(TaggedIsNumber(value), &valueIsNumber, &slowPath);
                Bind(&valueIsNumber);
                {
                    timeValue = CallNGCRuntime(glue, RTSTUB_ID(TimeClip), {GetDoubleOfTNumber(value)});
                    Jump(&newJSDate);
                }
            }
            Bind(&notOneArg);
            {
                Label threeArgs(env);
                Branch(Int64Equal(numArgs, IntPtr(3)), &threeArgs, &slowPath);  // 3: year month day
                Bind(&threeArgs);
                {
                    Label numberYearMonthDay(env);
                    GateRef year = GetArgNCheck(IntPtr(0));
                    GateRef month = GetArgNCheck(IntPtr(1));
                    GateRef day = GetArgNCheck(IntPtr(2));
                    Branch(IsNumberYearMonthDay(year, month, day), &numberYearMonthDay, &slowPath);
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

DECLARE_BUILTINS(ArrayConstructor)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());

    Label newTargetIsJSFunction(env);
    Label slowPath(env);
    Label exit(env);

    Branch(IsJSFunction(newTarget), &newTargetIsJSFunction, &slowPath);
    Bind(&newTargetIsJSFunction);
    {
        Label fastGetHclass(env);
        Label intialHClassIsHClass(env);
        GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env->Is32Bit()));
        GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
        auto arrayFunc = GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
        Branch(Equal(ChangeTaggedPointerToInt64(arrayFunc), ChangeTaggedPointerToInt64(newTarget)), &fastGetHclass,
            &slowPath);
        Bind(&fastGetHclass);
        GateRef intialHClass = Load(VariableType::JS_ANY(), newTarget, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
        DEFVARIABLE(arrayLength, VariableType::INT64(), Int64(0));
        Branch(IsJSHClass(intialHClass), &intialHClassIsHClass, &slowPath);
        Bind(&intialHClassIsHClass);
        {
            Label noArg(env);
            Label hasArg(env);
            Label arrayCreate(env);
            Branch(Int64Equal(numArgs, IntPtr(0)), &noArg, &hasArg);
            Bind(&noArg);
            {
                Jump(&arrayCreate);
            }
            Bind(&hasArg);
            {
                Label hasOneArg(env);
                Branch(Int64Equal(numArgs, IntPtr(1)), &hasOneArg, &slowPath);
                Bind(&hasOneArg);
                {
                    Label argIsNumber(env);
                    GateRef arg0 = GetArg(numArgs, IntPtr(0));
                    Branch(TaggedIsNumber(arg0), &argIsNumber, &slowPath);
                    Bind(&argIsNumber);
                    {
                        Label argIsInt(env);
                        Label argIsDouble(env);
                        Branch(TaggedIsInt(arg0), &argIsInt, &argIsDouble);
                        Bind(&argIsInt);
                        {
                            Label validIntLength(env);
                            GateRef intLen = GetInt64OfTInt(arg0);
                            GateRef isGEZero = Int64GreaterThanOrEqual(intLen, Int64(0));
                            GateRef isLEMaxLen = Int64LessThanOrEqual(intLen, Int64(JSArray::MAX_ARRAY_INDEX));
                            Branch(BoolAnd(isGEZero, isLEMaxLen), &validIntLength, &slowPath);
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
                            Branch(BoolAnd(doubleEqual, doubleLEMaxLen), &validDoubleLength, &slowPath);
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
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue, 0);
                Label afterNew(env);
                newBuilder.NewJSObject(&res, &afterNew, intialHClass);
                Bind(&afterNew);
                {
                    GateRef lengthOffset = IntPtr(JSArray::LENGTH_OFFSET);
                    Store(VariableType::JS_ANY(), glue, *res, lengthOffset, Int64ToTaggedInt(*arrayLength));
                    GateRef accessor = GetGlobalConstantValue(VariableType::JS_ANY(), glue,
                                                              ConstantIndex::ARRAY_LENGTH_ACCESSOR);
                    SetPropertyInlinedProps(glue, *res, intialHClass, accessor,
                                            Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
                    SetExtensibleToBitfield(glue, *res, true);
                    Jump(&exit);
                }
            }
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
}  // namespace panda::ecmascript::kungfu