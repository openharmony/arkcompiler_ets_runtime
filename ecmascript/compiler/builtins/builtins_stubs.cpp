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

#include "ecmascript/compiler/builtins/builtins_stubs-inl.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/interpreter_stub-inl.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/variable_type.h"

namespace panda::ecmascript::kungfu {
#if ECMASCRIPT_ENABLE_ASM_INTERPRETER_LOG
#define DECLARE_BUILTINS(name)                                                                      \
void name##StubBuilder::GenerateCircuit()                                                           \
{                                                                                                   \
    GateRef glue = PtrArgument(static_cast<size_t>(BuiltinsArgs::GLUE));                            \
    GateRef nativeCode = PtrArgument(static_cast<size_t>(BuiltinsArgs::NATIVECODE));                \
    GateRef func = TaggedArgument(static_cast<size_t>(BuiltinsArgs::FUNC));                         \
    GateRef thisValue = TaggedArgument(static_cast<size_t>(BuiltinsArgs::THISVALUE));               \
    GateRef numArgs = PtrArgument(static_cast<size_t>(BuiltinsArgs::NUMARGS));                      \
    GateRef argv = PtrArgument(static_cast<size_t>(BuiltinsArgs::ARGV));                            \
    DebugPrint(glue, { Int32(GET_MESSAGE_STRING_ID(name)) });                                       \
    GenerateCircuitImpl(glue, nativeCode, func, thisValue, numArgs, argv);                          \
}                                                                                                   \
void name##StubBuilder::GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func,         \
                                            GateRef thisValue, GateRef numArgs, GateRef argv)
#else
#define DECLARE_BUILTINS(name)                                                                      \
void name##StubBuilder::GenerateCircuit()                                                           \
{                                                                                                   \
    GateRef glue = PtrArgument(static_cast<size_t>(BuiltinsArgs::GLUE));                            \
    GateRef nativeCode = PtrArgument(static_cast<size_t>(BuiltinsArgs::NATIVECODE));                \
    GateRef func = TaggedArgument(static_cast<size_t>(BuiltinsArgs::FUNC));                         \
    GateRef thisValue = TaggedArgument(static_cast<size_t>(BuiltinsArgs::THISVALUE));               \
    GateRef numArgs = PtrArgument(static_cast<size_t>(BuiltinsArgs::NUMARGS));                      \
    GateRef argv = PtrArgument(static_cast<size_t>(BuiltinsArgs::ARGV));                            \
    GenerateCircuitImpl(glue, nativeCode, func, thisValue, numArgs, argv);                          \
}                                                                                                   \
void name##StubBuilder::GenerateCircuitImpl(GateRef glue, GateRef nativeCode, GateRef func,         \
                                            GateRef thisValue, GateRef numArgs, GateRef argv)
#endif

#define CALLSLOWPATH()                                                                              \
    CallNGCRuntime(glue, RTSTUB_ID(PushCallIRangeAndDispatchNative),                                \
                  { glue, nativeCode, func, thisValue, numArgs, argv })

GateRef BuiltinsStubBuilder::StringIndexOf(GateRef lhsData, bool lhsIsUtf8, GateRef rhsData, bool rhsIsUtf8,
                                           GateRef pos, GateRef max, GateRef rhsCount)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(i, VariableType::INT32(), pos);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(j, VariableType::INT32(), Int32(0));
    DEFVARIABLE(k, VariableType::INT32(), Int32(1));
    Label exit(env);
    Label next(env);
    Label next1(env);
    Label next2(env);
    Label next3(env);
    Label next4(env);
    Label next5(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label nextCount(env);
    Label nextCount1(env);
    Label nextCount2(env);
    GateRef first;
    if (rhsIsUtf8) {
        first = ZExtInt8ToInt32(Load(VariableType::INT8(), rhsData));
    } else {
        first = ZExtInt16ToInt32(Load(VariableType::INT16(), rhsData));
    }
    Jump(&loopHead);
    LoopBegin(&loopHead);
    Branch(Int32LessThanOrEqual(*i, max), &next, &exit);
    Bind(&next);
    {
        Label loopHead1(env);
        Label loopEnd1(env);
        GateRef lhsTemp;
        if (lhsIsUtf8) {
            lhsTemp = GetUtf8Date(lhsData, *i);
        } else {
            lhsTemp = GetUtf16Date(lhsData, *i);
        }
        Branch(Int32NotEqual(lhsTemp, first), &nextCount1, &nextCount);
        Bind(&nextCount1);
        {
            i = Int32Add(*i, Int32(1));
            Jump(&loopHead1);
        }
        LoopBegin(&loopHead1);
        {
            Branch(Int32LessThanOrEqual(*i, max), &next1, &nextCount);
            Bind(&next1);
            {
                GateRef lhsTemp1;
                if (lhsIsUtf8) {
                    lhsTemp1 = GetUtf8Date(lhsData, *i);
                } else {
                    lhsTemp1 = GetUtf16Date(lhsData, *i);
                }
                Branch(Int32NotEqual(lhsTemp1, first), &next2, &nextCount);
                Bind(&next2);
                {
                    i = Int32Add(*i, Int32(1));
                    Jump(&loopEnd1);
                }
            }
        }
        Bind(&loopEnd1);
        LoopEnd(&loopHead1);
        Bind(&nextCount);
        {
            Branch(Int32LessThanOrEqual(*i, max), &next3, &loopEnd);
            Bind(&next3);
            {
                Label loopHead2(env);
                Label loopEnd2(env);
                j = Int32Add(*i, Int32(1));
                GateRef end = Int32Sub(Int32Add(*j, rhsCount), Int32(1));
                k = Int32(1);
                Jump(&loopHead2);
                LoopBegin(&loopHead2);
                {
                    Branch(Int32LessThan(*j, end), &next4, &nextCount2);
                    Bind(&next4);
                    {
                        GateRef lhsTemp2;
                        if (lhsIsUtf8) {
                            lhsTemp2 = GetUtf8Date(lhsData, *j);
                        } else {
                            lhsTemp2 = GetUtf16Date(lhsData, *j);
                        }
                        GateRef rhsTemp;
                        if (rhsIsUtf8) {
                            rhsTemp = GetUtf8Date(rhsData, *k);
                        } else {
                            rhsTemp = GetUtf16Date(rhsData, *k);
                        }
                        Branch(Int32Equal(lhsTemp2, rhsTemp), &loopEnd2, &nextCount2);
                    }
                }
                Bind(&loopEnd2);
                j = Int32Add(*j, Int32(1));
                k = Int32Add(*k, Int32(1));
                LoopEnd(&loopHead2);
                Bind(&nextCount2);
                {
                    Branch(Int32Equal(*j, end), &next5, &loopEnd);
                    Bind(&next5);
                    result = *i;
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&loopEnd);
    i = Int32Add(*i, Int32(1));
    LoopEnd(&loopHead);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStubBuilder::StringIndexOf(GateRef lhs, GateRef rhs, GateRef pos)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(posTag, VariableType::INT32(), pos);
    Label exit(env);
    Label exit1(env);
    Label next1(env);
    Label next2(env);
    Label next3(env);
    Label next4(env);
    Label next5(env);
    Label rhsIsUtf8(env);
    Label rhsIsUtf16(env);

    GateRef lhsCount = GetLengthFromString(lhs);
    GateRef rhsCount = GetLengthFromString(rhs);

    Branch(Int32GreaterThan(pos, lhsCount), &exit, &next1);
    Bind(&next1);
    {
        Branch(Int32Equal(rhsCount, Int32(0)), &exit1, &next2);
        Bind(&exit1);
        {
            result = pos;
            Jump(&exit);
        }
        Bind(&next2);
        {
            Branch(Int32LessThan(pos, Int32(0)), &next3, &next4);
            Bind(&next3);
            {
                posTag = Int32(0);
                Jump(&next4);
            }
            Bind(&next4);
            {
                GateRef max = Int32Sub(lhsCount, rhsCount);
                Branch(Int32LessThan(max, Int32(0)), &exit, &next5);
                Bind(&next5);
                {
                    GateRef rhsData = PtrAdd(rhs, IntPtr(EcmaString::DATA_OFFSET));
                    GateRef lhsData = PtrAdd(lhs, IntPtr(EcmaString::DATA_OFFSET));
                    Branch(IsUtf8String(rhs), &rhsIsUtf8, &rhsIsUtf16);
                    Bind(&rhsIsUtf8);
                    {
                        Label lhsIsUtf8(env);
                        Label lhsIsUtf16(env);
                        Branch(IsUtf8String(lhs), &lhsIsUtf8, &lhsIsUtf16);
                        Bind(&lhsIsUtf8);
                        {
                            result = StringIndexOf(lhsData, true, rhsData, true, *posTag, max, rhsCount);
                            Jump(&exit);
                        }
                        Bind(&lhsIsUtf16);
                        {
                            result = StringIndexOf(lhsData, false, rhsData, true, *posTag, max, rhsCount);
                            Jump(&exit);
                        }
                    }
                    Bind(&rhsIsUtf16);
                    {
                        Label lhsIsUtf8(env);
                        Label lhsIsUtf16(env);
                        Branch(IsUtf8String(lhs), &lhsIsUtf8, &lhsIsUtf16);
                        Bind(&lhsIsUtf8);
                        {
                            result = StringIndexOf(lhsData, true, rhsData, false, *posTag, max, rhsCount);
                            Jump(&exit);
                        }
                        Bind(&lhsIsUtf16);
                        {
                            result = StringIndexOf(lhsData, false, rhsData, false, *posTag, max, rhsCount);
                            Jump(&exit);
                        }
                    }
                }
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
    DEFVARIABLE(res, VariableType::JS_ANY(), DoubleBuildTaggedWithNoGC(Double(base::NAN_VALUE)));
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

    Branch(TaggedIsUndefinedOrNull(thisValue), &slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(IsString(thisValue), &isString, &slowPath);
        Bind(&isString);
        {
            GateRef thisLen = GetLengthFromString(thisValue);
            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &posTagNotUndefined);
            Bind(&posTagNotUndefined);
            {
                GateRef posTag = GetCallArg(argv, IntPtr(0));
                Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                pos = TaggedCastToInt32(posTag);
                Jump(&next);
                Bind(&posTagNotInt);
                Branch(TaggedIsDouble(posTag), &posTagIsDouble, &slowPath);
                Bind(&posTagIsDouble);
                pos = DoubleToInt(glue, TaggedCastToDouble(posTag));
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
                        res = IntToTaggedNGC(StringAt(thisValue, *pos));
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&slowPath);
    {
        res = CALLSLOWPATH();
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

DECLARE_BUILTINS(IndexOf)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), IntToTaggedNGC(Int32(-1)));
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
    
    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label isSearchString(env);
    Label slowPath(env);
    Label next(env);
    Label next1(env);
    Label searchTagIsHeapObject(env);
    Label posTagNotUndefined(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label exit(env);
    Label posTagIsDouble(env);
    Label nextCount(env);
    Label posNotLessThanLen(env);

    Branch(TaggedIsUndefinedOrNull(thisValue), &slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(IsString(thisValue), &isString, &slowPath);
        Bind(&isString);
        {
            GateRef searchTag = GetCallArg(argv, IntPtr(0));
            Branch(TaggedIsHeapObject(searchTag), &searchTagIsHeapObject, &slowPath);
            Bind(&searchTagIsHeapObject);
            Branch(IsString(searchTag), &isSearchString, &slowPath);
            Bind(&isSearchString);
            {
                GateRef thisLen = GetLengthFromString(thisValue);
                Branch(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &next, &posTagNotUndefined);
                Bind(&posTagNotUndefined);
                {
                    GateRef posTag = GetCallArg(argv, IntPtr(1));
                    Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                    Bind(&posTagIsInt);
                    pos = TaggedCastToInt32(posTag);
                    Jump(&next);
                    Bind(&posTagNotInt);
                    Branch(TaggedIsDouble(posTag), &posTagIsDouble, &slowPath);
                    Bind(&posTagIsDouble);
                    pos = DoubleToInt(glue, TaggedCastToDouble(posTag));
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
                        GateRef resPos = StringIndexOf(thisValue, searchTag, *pos);
                        Branch(Int32GreaterThanOrEqual(resPos, Int32(0)), &next1, &exit);
                        Bind(&next1);
                        {
                            Label next2(env);
                            Branch(Int32LessThanOrEqual(resPos, thisLen), &next2, &exit);
                            Bind(&next2);
                            {
                                res = IntToTaggedNGC(resPos);
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
        res = CALLSLOWPATH();
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}
}  // namespace panda::ecmascript::kungfu