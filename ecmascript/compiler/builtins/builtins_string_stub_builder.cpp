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

#include "ecmascript/compiler/builtins/builtins_string_stub_builder.h"

#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/new_object_stub_builder.h"

namespace panda::ecmascript::kungfu {
GateRef BuiltinsStringStubBuilder::StringAt(GateRef obj, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));

    Label exit(env);
    Label isUtf16(env);
    Label isUtf8(env);
    Label doIntOp(env);
    Label leftIsNumber(env);
    Label rightIsNumber(env);
    GateRef dataUtf16 = GetNormalStringData(obj);
    Branch(IsUtf16String(obj), &isUtf16, &isUtf8);
    Bind(&isUtf16);
    {
        result = ZExtInt16ToInt32(Load(VariableType::INT16(), PtrAdd(dataUtf16,
            PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint16_t))))));
        Jump(&exit);
    }
    Bind(&isUtf8);
    {
        result = ZExtInt8ToInt32(Load(VariableType::INT8(), PtrAdd(dataUtf16,
            PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint8_t))))));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::CreateFromEcmaString(GateRef glue, GateRef obj, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Hole());
    DEFVARIABLE(canBeCompressed, VariableType::BOOL(), False());
    DEFVARIABLE(data, VariableType::INT16(), Int32(0));

    Label exit(env);
    Label isUtf16(env);
    Label isUtf8(env);
    Label allocString(env);
    GateRef dataUtf = GetNormalStringData(obj);
    Branch(IsUtf16String(obj), &isUtf16, &isUtf8);
    Bind(&isUtf16);
    {
        GateRef dataAddr = PtrAdd(dataUtf, PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint16_t))));
        data = Load(VariableType::INT16(), dataAddr);
        canBeCompressed = CanBeCompressed(dataAddr, Int32(1), true);
        Jump(&allocString);
    }
    Bind(&isUtf8);
    {
        GateRef dataAddr = PtrAdd(dataUtf, PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint8_t))));
        data = ZExtInt8ToInt16(Load(VariableType::INT8(), dataAddr));
        canBeCompressed = CanBeCompressed(dataAddr, Int32(1), false);
        Jump(&allocString);
    }
    Bind(&allocString);
    {
        Label afterNew(env);
        Label isUtf8Next(env);
        Label isUtf16Next(env);
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, 0);
        Branch(*canBeCompressed, &isUtf8Next, &isUtf16Next);
        Bind(&isUtf8Next);
        {
            newBuilder.AllocLineStringObject(&result, &afterNew, Int32(1), true);
        }
        Bind(&isUtf16Next);
        {
            newBuilder.AllocLineStringObject(&result, &afterNew, Int32(1), false);
        }
        Bind(&afterNew);
        {
            Label isUtf8Copy(env);
            Label isUtf16Copy(env);
            GateRef dst = PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET));
            Branch(*canBeCompressed, &isUtf8Copy, &isUtf16Copy);
            Bind(&isUtf8Copy);
            {
                Store(VariableType::INT8(), glue, dst, IntPtr(0), TruncInt16ToInt8(*data));
                Jump(&exit);
            }
            Bind(&isUtf16Copy);
            {
                Store(VariableType::INT16(), glue, dst, IntPtr(0), *data);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::FastSubString(GateRef glue, GateRef thisValue, GateRef from, GateRef len)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), thisValue);

    Label exit(env);
    Label lenEqualZero(env);
    Label lenNotEqualZero(env);
    Label fromEqualZero(env);
    Label next(env);
    Label isUtf8(env);
    Label isUtf16(env);

    Branch(Int32Equal(len, Int32(0)), &lenEqualZero, &lenNotEqualZero);
    Bind(&lenEqualZero);
    {
        result = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
        Jump(&exit);
    }
    Bind(&lenNotEqualZero);
    {
        Branch(Int32Equal(from, Int32(0)), &fromEqualZero, &next);
        Bind(&fromEqualZero);
        {
            GateRef thisLen = GetLengthFromString(thisValue);
            Branch(Int32Equal(len, thisLen), &exit, &next);
        }
        Bind(&next);
        {
            Branch(IsUtf8String(thisValue), &isUtf8, &isUtf16);
            Bind(&isUtf8);
            {
                result = FastSubUtf8String(glue, thisValue, from, len);
                Jump(&exit);
            }
            Bind(&isUtf16);
            {
                result = FastSubUtf16String(glue, thisValue, from, len);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::FastSubUtf8String(GateRef glue, GateRef thisValue, GateRef from, GateRef len)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());
    Label exit(env);

    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterNew(env);
    newBuilder.AllocLineStringObject(&result, &afterNew, len, true);
    Bind(&afterNew);
    {
        GateRef dst = PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET));
        GateRef source = PtrAdd(GetNormalStringData(thisValue), ZExtInt32ToPtr(from));
        CopyChars(glue, dst, source, len, IntPtr(sizeof(uint8_t)), VariableType::INT8());
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::FastSubUtf16String(GateRef glue, GateRef thisValue, GateRef from, GateRef len)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label isUtf16(env);
    Label isUtf8(env);
    Label isUtf8Next(env);
    Label isUtf16Next(env);

    GateRef fromOffset = PtrMul(ZExtInt32ToPtr(from), IntPtr(sizeof(uint16_t) / sizeof(uint8_t)));
    GateRef source = PtrAdd(GetNormalStringData(thisValue), fromOffset);
    GateRef canBeCompressed = CanBeCompressed(source, len, true);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterNew(env);
    Branch(canBeCompressed, &isUtf8, &isUtf16);
    Bind(&isUtf8);
    {
        newBuilder.AllocLineStringObject(&result, &afterNew, len, true);
    }
    Bind(&isUtf16);
    {
        newBuilder.AllocLineStringObject(&result, &afterNew, len, false);
    }
    Bind(&afterNew);
    {
        GateRef source1 = PtrAdd(GetNormalStringData(thisValue), fromOffset);
        GateRef dst = PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET));
        Branch(canBeCompressed, &isUtf8Next, &isUtf16Next);
        Bind(&isUtf8Next);
        {
            CopyUtf16AsUtf8(glue, source1, dst, len);
            Jump(&exit);
        }
        Bind(&isUtf16Next);
        {
            CopyChars(glue, dst, source1, len, IntPtr(sizeof(uint16_t)), VariableType::INT16());
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsStringStubBuilder::CopyChars(GateRef glue, GateRef dst, GateRef source,
    GateRef sourceLength, GateRef size, VariableType type)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(dstTmp, VariableType::JS_ANY(), dst);
    DEFVARIABLE(sourceTmp, VariableType::JS_ANY(), source);
    DEFVARIABLE(len, VariableType::INT32(), sourceLength);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label exit(env);
    Jump(&loopHead);

    LoopBegin(&loopHead);
    {
        Branch(Int32GreaterThan(*len, Int32(0)), &next, &exit);
        Bind(&next);
        {
            len = Int32Sub(*len, Int32(1));
            GateRef i = Load(type, *sourceTmp);
            Store(type, glue, *dstTmp, IntPtr(0), i);
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    sourceTmp = PtrAdd(*sourceTmp, size);
    dstTmp = PtrAdd(*dstTmp, size);
    LoopEnd(&loopHead);

    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef BuiltinsStringStubBuilder::CanBeCompressed(GateRef data, GateRef len, bool isUtf16)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label loopHead(env);
    Label loopEnd(env);
    Label nextCount(env);
    Label isNotASCIICharacter(env);
    Label exit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32LessThan(*i, len), &nextCount, &exit);
        Bind(&nextCount);
        {
            if (isUtf16) {
                GateRef tmp = Load(VariableType::INT16(), data,
                    PtrMul(ZExtInt32ToPtr(*i), IntPtr(sizeof(uint16_t))));
                Branch(IsASCIICharacter(ZExtInt16ToInt32(tmp)), &loopEnd, &isNotASCIICharacter);
            } else {
                GateRef tmp = Load(VariableType::INT8(), data,
                    PtrMul(ZExtInt32ToPtr(*i), IntPtr(sizeof(uint8_t))));
                Branch(IsASCIICharacter(ZExtInt8ToInt32(tmp)), &loopEnd, &isNotASCIICharacter);
            }
            Bind(&isNotASCIICharacter);
            {
                result = False();
                Jump(&exit);
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

void BuiltinsStringStubBuilder::CopyUtf16AsUtf8(GateRef glue, GateRef src, GateRef dst,
    GateRef sourceLength)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(dstTmp, VariableType::JS_ANY(), dst);
    DEFVARIABLE(sourceTmp, VariableType::JS_ANY(), src);
    DEFVARIABLE(len, VariableType::INT32(), sourceLength);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label exit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(Int32GreaterThan(*len, Int32(0)), &next, &exit);
        Bind(&next);
        {
            len = Int32Sub(*len, Int32(1));
            GateRef i = Load(VariableType::INT16(), *sourceTmp);
            Store(VariableType::INT8(), glue, *dstTmp, IntPtr(0), TruncInt16ToInt8(i));
            Jump(&loopEnd);
        }
    }

    Bind(&loopEnd);
    sourceTmp = PtrAdd(*sourceTmp, IntPtr(sizeof(uint16_t)));
    dstTmp = PtrAdd(*dstTmp, IntPtr(sizeof(uint8_t)));
    LoopEnd(&loopHead);

    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef BuiltinsStringStubBuilder::GetUtf16Data(GateRef stringData, GateRef index)
{
    return ZExtInt16ToInt32(Load(VariableType::INT16(), PtrAdd(stringData,
        PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint16_t))))));
}

GateRef BuiltinsStringStubBuilder::IsASCIICharacter(GateRef data)
{
    return Int32LessThan(Int32Sub(data, Int32(1)), Int32(base::utf_helper::UTF8_1B_MAX));
}

GateRef BuiltinsStringStubBuilder::GetUtf8Data(GateRef stringData, GateRef index)
{
    return ZExtInt8ToInt32(Load(VariableType::INT8(), PtrAdd(stringData,
        PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint8_t))))));
}

GateRef BuiltinsStringStubBuilder::StringIndexOf(GateRef lhsData, bool lhsIsUtf8, GateRef rhsData, bool rhsIsUtf8,
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
    Label continueFor(env);
    Label lhsNotEqualFirst(env);
    Label continueCount(env);
    Label lessEnd(env);
    Label equalEnd(env);
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
            lhsTemp = GetUtf8Data(lhsData, *i);
        } else {
            lhsTemp = GetUtf16Data(lhsData, *i);
        }
        Branch(Int32NotEqual(lhsTemp, first), &nextCount1, &nextCount);
        Bind(&nextCount1);
        {
            i = Int32Add(*i, Int32(1));
            Jump(&loopHead1);
        }
        LoopBegin(&loopHead1);
        {
            Branch(Int32LessThanOrEqual(*i, max), &continueFor, &nextCount);
            Bind(&continueFor);
            {
                GateRef lhsTemp1;
                if (lhsIsUtf8) {
                    lhsTemp1 = GetUtf8Data(lhsData, *i);
                } else {
                    lhsTemp1 = GetUtf16Data(lhsData, *i);
                }
                Branch(Int32NotEqual(lhsTemp1, first), &lhsNotEqualFirst, &nextCount);
                Bind(&lhsNotEqualFirst);
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
            Branch(Int32LessThanOrEqual(*i, max), &continueCount, &loopEnd);
            Bind(&continueCount);
            {
                Label loopHead2(env);
                Label loopEnd2(env);
                j = Int32Add(*i, Int32(1));
                GateRef end = Int32Sub(Int32Add(*j, rhsCount), Int32(1));
                k = Int32(1);
                Jump(&loopHead2);
                LoopBegin(&loopHead2);
                {
                    Branch(Int32LessThan(*j, end), &lessEnd, &nextCount2);
                    Bind(&lessEnd);
                    {
                        GateRef lhsTemp2;
                        if (lhsIsUtf8) {
                            lhsTemp2 = GetUtf8Data(lhsData, *j);
                        } else {
                            lhsTemp2 = GetUtf16Data(lhsData, *j);
                        }
                        GateRef rhsTemp;
                        if (rhsIsUtf8) {
                            rhsTemp = GetUtf8Data(rhsData, *k);
                        } else {
                            rhsTemp = GetUtf16Data(rhsData, *k);
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
                    Branch(Int32Equal(*j, end), &equalEnd, &loopEnd);
                    Bind(&equalEnd);
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

GateRef BuiltinsStringStubBuilder::StringIndexOf(GateRef lhs, GateRef rhs, GateRef pos)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(posTag, VariableType::INT32(), pos);
    Label exit(env);
    Label rhsCountEqualZero(env);
    Label nextCount(env);
    Label rhsCountNotEqualZero(env);
    Label posLessZero(env);
    Label posNotLessZero(env);
    Label maxNotLessZero(env);
    Label rhsIsUtf8(env);
    Label rhsIsUtf16(env);
    Label posRMaxNotGreaterLhs(env);

    GateRef lhsCount = GetLengthFromString(lhs);
    GateRef rhsCount = GetLengthFromString(rhs);

    Branch(Int32GreaterThan(pos, lhsCount), &exit, &nextCount);
    Bind(&nextCount);
    {
        Branch(Int32Equal(rhsCount, Int32(0)), &rhsCountEqualZero, &rhsCountNotEqualZero);
        Bind(&rhsCountEqualZero);
        {
            result = pos;
            Jump(&exit);
        }
        Bind(&rhsCountNotEqualZero);
        {
            Branch(Int32LessThan(pos, Int32(0)), &posLessZero, &posNotLessZero);
            Bind(&posLessZero);
            {
                posTag = Int32(0);
                Jump(&posNotLessZero);
            }
            Bind(&posNotLessZero);
            {
                GateRef max = Int32Sub(lhsCount, rhsCount);
                Branch(Int32LessThan(max, Int32(0)), &exit, &maxNotLessZero);
                Bind(&maxNotLessZero);
                {
                    GateRef posRMax = Int32Add(*posTag, rhsCount);
                    Branch(Int32GreaterThan(posRMax, lhsCount), &exit, &posRMaxNotGreaterLhs);
                    Bind(&posRMaxNotGreaterLhs);
                    GateRef rhsData = GetNormalStringData(rhs);
                    GateRef lhsData = GetNormalStringData(lhs);
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
}  // namespace panda::ecmascript::kungfu
