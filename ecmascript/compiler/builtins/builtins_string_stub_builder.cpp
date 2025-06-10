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

#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_string_iterator.h"

namespace panda::ecmascript::kungfu {
void BuiltinsStringStubBuilder::FromCharCode(GateRef glue, [[maybe_unused]] GateRef thisValue,
    GateRef numArgs, Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(value, VariableType::INT16(), Int16(0));
    Label lengthIsZero(env);
    Label lengthNotZero(env);
    Label lengthIsOne(env);
    Label canBeCompress(env);
    Label isInt(env);
    Label notInt(env);
    Label newObj(env);
    Label canNotBeCompress(env);
    Label isPendingException(env);
    Label noPendingException(env);
    BRANCH(Int64Equal(IntPtr(0), numArgs), &lengthIsZero, &lengthNotZero);
    Bind(&lengthIsZero);
    res->WriteVariable(GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX));
    Jump(exit);
    Bind(&lengthNotZero);
    {
        BRANCH(Int64Equal(IntPtr(1), numArgs), &lengthIsOne, slowPath);
        Bind(&lengthIsOne);
        {
            GateRef codePointTag = GetCallArg0(numArgs);
            GateRef codePointValue = ToNumber(glue, codePointTag);
            BRANCH(HasPendingException(glue), &isPendingException, &noPendingException);
            Bind(&isPendingException);
            {
                res->WriteVariable(Exception());
                Jump(exit);
            }
            Bind(&noPendingException);
            {
                BRANCH(TaggedIsInt(codePointValue), &isInt, &notInt);
                Bind(&isInt);
                {
                    value = TruncInt32ToInt16(GetInt32OfTInt(codePointValue));
                    Jump(&newObj);
                }
                Bind(&notInt);
                {
                    value = TruncInt32ToInt16(DoubleToInt(glue, GetDoubleOfTDouble(codePointValue), base::INT16_BITS));
                    Jump(&newObj);
                }
                Bind(&newObj);
                BRANCH(IsASCIICharacter(ZExtInt16ToInt32(*value)), &canBeCompress, &canNotBeCompress);
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue, 0);
                Bind(&canBeCompress);
                {
                    GateRef singleCharTable = GetSingleCharTable(glue);
                    res->WriteVariable(GetValueFromTaggedArray(glue, singleCharTable, ZExtInt16ToInt32(*value)));
                    Jump(exit);
                }
                Bind(&canNotBeCompress);
                {
                    Label afterNew1(env);
                    newBuilder.AllocLineStringObject(res, &afterNew1, Int32(1), false);
                    Bind(&afterNew1);
                    {
                        GateRef dst = ChangeStringTaggedPointerToInt64(
                            PtrAdd(res->ReadVariable(), IntPtr(LineString::DATA_OFFSET)));
                        Store(VariableType::INT16(), glue, dst, IntPtr(0), *value);
                        Jump(exit);
                    }
                }
            }
        }
    }
}

void BuiltinsStringStubBuilder::CharAt(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
    DEFVARIABLE(doubleValue, VariableType::FLOAT64(), Double(0));
    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label next(env);
    Label posTagNotUndefined(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label isINF(env);
    Label isNotINF(env);
    Label posNotGreaterLen(env);
    Label posGreaterLen(env);
    Label posNotLessZero(env);
    Label posTagIsDouble(env);
    Label thisIsHeapobject(env);
    Label flattenFastPath(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            FlatStringStubBuilder thisFlat(this);
            thisFlat.FlattenString(glue, thisValue, &flattenFastPath);
            Bind(&flattenFastPath);
            GateRef thisLen = GetLengthFromString(thisValue);
            BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &posTagNotUndefined);
            Bind(&posTagNotUndefined);
            {
                GateRef posTag = GetCallArg0(numArgs);
                BRANCH(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                pos = GetInt32OfTInt(posTag);
                Jump(&next);
                Bind(&posTagNotInt);
                BRANCH(TaggedIsDouble(posTag), &posTagIsDouble, slowPath);
                Bind(&posTagIsDouble);
                doubleValue = GetDoubleOfTDouble(posTag);
                BRANCH(DoubleIsINF(*doubleValue), &posGreaterLen, &isNotINF);
                Bind(&isNotINF);
                pos = DoubleToInt(glue, GetDoubleOfTDouble(posTag));
                Jump(&next);
            }
            Bind(&next);
            {
                BRANCH(Int32GreaterThanOrEqual(*pos, thisLen), &posGreaterLen, &posNotGreaterLen);
                Bind(&posNotGreaterLen);
                {
                    BRANCH(Int32LessThan(*pos, Int32(0)), &posGreaterLen, &posNotLessZero);
                    Bind(&posNotLessZero);
                    {
                        StringInfoGateRef stringInfoGate(&thisFlat);
                        res->WriteVariable(CreateFromEcmaString(glue, *pos, stringInfoGate));
                        Jump(exit);
                    }
                }
                Bind(&posGreaterLen);
                {
                    res->WriteVariable(GetGlobalConstantValue(
                        VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX));
                    Jump(exit);
                }
            }
        }
    }
}

void BuiltinsStringStubBuilder::CharCodeAt(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    res->WriteVariable(DoubleToTaggedDoublePtr(Double(base::NAN_VALUE)));
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
    Label posIsValid(env);
    Label flattenFastPath(env);
    Label returnFirst(env);
    Label getNextChar(env);
    CheckParamsAndGetPosition(glue, thisValue, numArgs, &pos, exit, slowPath, &posIsValid);
    Bind(&posIsValid);
    {
        res->WriteVariable(FastStringCharCodeAt(glue, thisValue, *pos));
        Jump(exit);
    }
}

GateRef BuiltinsStringStubBuilder::FastStringCharCodeAt(GateRef glue, GateRef thisValue, GateRef pos)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(index, VariableType::INT32(), pos);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label exit(env);
    Label readyStringAt(env);
    FlatStringStubBuilder thisFlat(this);
    thisFlat.FlattenStringWithIndex(glue, thisValue, &index, &readyStringAt);
    Bind(&readyStringAt);
    {
        StringInfoGateRef stringInfoGate(&thisFlat);
        Label getCharByIndex(env);
        GateRef stringData = Circuit::NullGate();
        stringData = PtrAdd(stringInfoGate.GetString(), IntPtr(LineString::DATA_OFFSET));
        Jump(&getCharByIndex);
        Label isUtf16(env);
        Label isUtf8(env);
        Bind(&getCharByIndex);
        GateRef charPosition = Circuit::NullGate();
        BRANCH(IsUtf16String(stringInfoGate.GetString()), &isUtf16, &isUtf8);
        Bind(&isUtf16);
        {
            charPosition = PtrMul(ZExtInt32ToPtr(*index), IntPtr(sizeof(uint16_t)));
            result = Int64ToTaggedIntPtr((ZExtInt16ToInt64(
                LoadZeroOffsetPrimitive(VariableType::INT16(), PtrAdd(stringData, charPosition)))));
            Jump(&exit);
        }
        Bind(&isUtf8);
        {
            charPosition = PtrMul(ZExtInt32ToPtr(*index), IntPtr(sizeof(uint8_t)));
            result = Int64ToTaggedIntPtr((ZExtInt8ToInt64(
                LoadZeroOffsetPrimitive(VariableType::INT8(), PtrAdd(stringData, charPosition)))));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto res = *result;
    env->SubCfgExit();
    return res;
}

void BuiltinsStringStubBuilder::CodePointAt(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));
    Label posIsValid(env);
    Label flattenFastPath(env);
    Label returnFirst(env);
    Label getNextChar(env);
    CheckParamsAndGetPosition(glue, thisValue, numArgs, &pos, exit, slowPath, &posIsValid);
    Bind(&posIsValid);
    {
        FlatStringStubBuilder thisFlat(this);
        thisFlat.FlattenString(glue, thisValue, &flattenFastPath);
        Bind(&flattenFastPath);
        StringInfoGateRef stringInfoGate(&thisFlat);
        GateRef first = StringAt(glue, stringInfoGate, *pos);
        GateRef posVal = *pos;
        GateRef firstIsValid = LogicOrBuilder(env)
            .Or(Int32UnsignedLessThan(first, Int32(common::utf_helper::DECODE_LEAD_LOW)))
            .Or(Int32UnsignedGreaterThan(first, Int32(common::utf_helper::DECODE_LEAD_HIGH)))
            .Or(Int32Equal(Int32Add(posVal, Int32(1)), GetLengthFromString(thisValue)))
            .Done();
        BRANCH(firstIsValid, &returnFirst, &getNextChar);
        Bind(&getNextChar);
        GateRef second = StringAt(glue, stringInfoGate, Int32Add(*pos, Int32(1)));
        GateRef secondIsValid = BitOr(Int32UnsignedLessThan(second, Int32(common::utf_helper::DECODE_TRAIL_LOW)),
            Int32UnsignedGreaterThan(second, Int32(common::utf_helper::DECODE_TRAIL_HIGH)));
        BRANCH(secondIsValid, &returnFirst, slowPath);
        Bind(&returnFirst);
        res->WriteVariable(IntToTaggedPtr(first));
        Jump(exit);
    }
}

void BuiltinsStringStubBuilder::CheckParamsAndGetPosition(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* pos, Label *exit, Label *slowPath, Label *posIsValid)
{
    auto env = GetEnvironment();
    DEFVARIABLE(doubleValue, VariableType::FLOAT64(), Double(0));
    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label next(env);
    Label posTagNotUndefined(env);
    Label isINF(env);
    Label isNotINF(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label posNotGreaterLen(env);
    Label posNotLessZero(env);
    Label posTagIsDouble(env);
    Label thisIsHeapobject(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            GateRef thisLen = GetLengthFromString(thisValue);
            BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &posTagNotUndefined);
            Bind(&posTagNotUndefined);
            {
                GateRef posTag = GetCallArg0(numArgs);
                BRANCH(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                {
                    pos->WriteVariable(GetInt32OfTInt(posTag));
                    Jump(&next);
                }
                Bind(&posTagNotInt);
                {
                    BRANCH(TaggedIsDouble(posTag), &posTagIsDouble, slowPath);
                    Bind(&posTagIsDouble);
                    doubleValue = GetDoubleOfTDouble(posTag);
                    BRANCH(DoubleIsINF(*doubleValue), exit, &isNotINF);
                    Bind(&isNotINF);
                    pos->WriteVariable(DoubleToInt(glue, GetDoubleOfTDouble(posTag)));
                    Jump(&next);
                }
            }
            Bind(&next);
            {
                BRANCH(Int32UnsignedLessThan(pos->ReadVariable(), thisLen), posIsValid, exit);
            }
        }
    }
}

void BuiltinsStringStubBuilder::IndexOf(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));

    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label isSearchString(env);
    Label next(env);
    Label resPosGreaterZero(env);
    Label searchTagIsHeapObject(env);
    Label posTagNotUndefined(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label posTagIsDouble(env);
    Label nextCount(env);
    Label posNotLessThanLen(env);
    Label thisIsHeapobject(env);
    Label flattenFastPath(env);
    Label flattenFastPath1(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            GateRef searchTag = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(searchTag), &searchTagIsHeapObject, slowPath);
            Bind(&searchTagIsHeapObject);
            BRANCH(IsString(glue, searchTag), &isSearchString, slowPath);
            Bind(&isSearchString);
            {
                GateRef thisLen = GetLengthFromString(thisValue);
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &next, &posTagNotUndefined);
                Bind(&posTagNotUndefined);
                {
                    GateRef posTag = GetCallArg1(numArgs);
                    BRANCH(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                    Bind(&posTagIsInt);
                    pos = GetInt32OfTInt(posTag);
                    Jump(&next);
                    Bind(&posTagNotInt);
                    BRANCH(TaggedIsDouble(posTag), &posTagIsDouble, slowPath);
                    Bind(&posTagIsDouble);
                    pos = DoubleToInt(glue, GetDoubleOfTDouble(posTag));
                    Jump(&next);
                }
                Bind(&next);
                {
                    Label posGreaterThanZero(env);
                    Label posNotGreaterThanZero(env);
                    BRANCH(Int32GreaterThan(*pos, Int32(0)), &posGreaterThanZero, &posNotGreaterThanZero);
                    Bind(&posNotGreaterThanZero);
                    {
                        pos = Int32(0);
                        Jump(&nextCount);
                    }
                    Bind(&posGreaterThanZero);
                    {
                        BRANCH(Int32LessThanOrEqual(*pos, thisLen), &nextCount, &posNotLessThanLen);
                        Bind(&posNotLessThanLen);
                        {
                            pos = thisLen;
                            Jump(&nextCount);
                        }
                    }
                    Bind(&nextCount);
                    {
                        FlatStringStubBuilder thisFlat(this);
                        thisFlat.FlattenString(glue, thisValue, &flattenFastPath);
                        Bind(&flattenFastPath);
                        FlatStringStubBuilder searchFlat(this);
                        searchFlat.FlattenString(glue, searchTag, &flattenFastPath1);
                        Bind(&flattenFastPath1);
                        StringInfoGateRef thisStringInfoGate(&thisFlat);
                        StringInfoGateRef searchStringInfoGate(&searchFlat);
                        GateRef resPos = StringIndexOf(glue, thisStringInfoGate, searchStringInfoGate, *pos);
                        BRANCH(Int32GreaterThanOrEqual(resPos, Int32(0)), &resPosGreaterZero, exit);
                        Bind(&resPosGreaterZero);
                        {
                            Label resPosLessZero(env);
                            BRANCH(Int32LessThanOrEqual(resPos, thisLen), &resPosLessZero, exit);
                            Bind(&resPosLessZero);
                            {
                                res->WriteVariable(IntToTaggedPtr(resPos));
                                Jump(exit);
                            }
                        }
                    }
                }
            }
        }
    }
}

void BuiltinsStringStubBuilder::Substring(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(start, VariableType::INT32(), Int32(0));
    DEFVARIABLE(end, VariableType::INT32(), Int32(0));
    DEFVARIABLE(from, VariableType::INT32(), Int32(0));
    DEFVARIABLE(to, VariableType::INT32(), Int32(0));

    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label isSearchString(env);
    Label countStart(env);
    Label endTagIsUndefined(env);
    Label startNotGreatZero(env);
    Label countEnd(env);
    Label endNotGreatZero(env);
    Label countFrom(env);
    Label countRes(env);
    Label startTagNotUndefined(env);
    Label startTagIsNumber(env);
    Label startTagIsInt(env);
    Label endTagNotUndefined(env);
    Label endTagIsNumber(env);
    Label endTagIsInt(env);
    Label endGreatZero(env);
    Label endGreatLen(env);
    Label startGreatZero(env);
    Label startGreatEnd(env);
    Label startNotGreatEnd(env);
    Label thisIsHeapobject(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            Label next(env);
            GateRef thisLen = GetLengthFromString(thisValue);
            BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &startTagNotUndefined);
            Bind(&startTagNotUndefined);
            {
                GateRef startTag = GetCallArg0(numArgs);
                BRANCH(TaggedIsNumber(startTag), &startTagIsNumber, slowPath);
                Bind(&startTagIsNumber);
                BRANCH(TaggedIsInt(startTag), &startTagIsInt, slowPath);
                Bind(&startTagIsInt);
                {
                    start = GetInt32OfTInt(startTag);
                    Jump(&next);
                }
            }
            Bind(&next);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &endTagIsUndefined, &endTagNotUndefined);
                Bind(&endTagIsUndefined);
                {
                    end = thisLen;
                    Jump(&countStart);
                }
                Bind(&endTagNotUndefined);
                {
                    GateRef endTag = GetCallArg1(numArgs);
                    BRANCH(TaggedIsNumber(endTag), &endTagIsNumber, slowPath);
                    Bind(&endTagIsNumber);
                    BRANCH(TaggedIsInt(endTag), &endTagIsInt, slowPath);
                    Bind(&endTagIsInt);
                    {
                        end = GetInt32OfTInt(endTag);
                        Jump(&countStart);
                    }
                }
            }
            Bind(&countStart);
            {
                Label startGreatLen(env);
                BRANCH(Int32GreaterThan(*start, Int32(0)), &startGreatZero, &startNotGreatZero);
                Bind(&startNotGreatZero);
                {
                    start = Int32(0);
                    Jump(&countEnd);
                }
                Bind(&startGreatZero);
                {
                    BRANCH(Int32GreaterThan(*start, thisLen), &startGreatLen, &countEnd);
                    Bind(&startGreatLen);
                    {
                        start = thisLen;
                        Jump(&countEnd);
                    }
                }
            }
            Bind(&countEnd);
            {
                BRANCH(Int32GreaterThan(*end, Int32(0)), &endGreatZero, &endNotGreatZero);
                Bind(&endNotGreatZero);
                {
                    end = Int32(0);
                    Jump(&countFrom);
                }
                Bind(&endGreatZero);
                {
                    BRANCH(Int32GreaterThan(*end, thisLen), &endGreatLen, &countFrom);
                    Bind(&endGreatLen);
                    {
                        end = thisLen;
                        Jump(&countFrom);
                    }
                }
            }
            Bind(&countFrom);
            {
                BRANCH(Int32GreaterThan(*start, *end), &startGreatEnd, &startNotGreatEnd);
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
                GateRef len = Int32Sub(*to, *from);
                res->WriteVariable(GetSubString(glue, thisValue, *from, len));
                Jump(exit);
            }
        }
    }
}

void BuiltinsStringStubBuilder::SubStr(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(start, VariableType::INT32(), Int32(0));
    DEFVARIABLE(end, VariableType::INT32(), Int32(0));
    DEFVARIABLE(tempLength, VariableType::INT32(), Int32(0));
    DEFVARIABLE(resultLength, VariableType::INT32(), Int32(0));

    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label countStart(env);
    Label lengthTagIsUndefined(env);
    Label lengthTagNotUndefined(env);
    Label countResultLength(env);
    Label countResultLength1(env);
    Label countRes(env);
    Label intStartNotUndefined(env);
    Label intStartIsNumber(env);
    Label intStartIsInt(env);
    Label lengthTagIsNumber(env);
    Label lengthTagIsInt(env);
    Label thisIsHeapobject(env);
    Label endGreatZero(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            Label next(env);
            GateRef thisLen = GetLengthFromString(thisValue);
            BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &intStartNotUndefined);
            Bind(&intStartNotUndefined);
            {
                GateRef intStart = GetCallArg0(numArgs);
                BRANCH(TaggedIsNumber(intStart), &intStartIsNumber, slowPath);
                Bind(&intStartIsNumber);
                BRANCH(TaggedIsInt(intStart), &intStartIsInt, slowPath);
                Bind(&intStartIsInt);
                {
                    start = GetInt32OfTInt(intStart);
                    Jump(&next);
                }
            }
            Bind(&next);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &lengthTagIsUndefined, &lengthTagNotUndefined);
                Bind(&lengthTagIsUndefined);
                {
                    end = Int32(INT_MAX);
                    Jump(&countStart);
                }
                Bind(&lengthTagNotUndefined);
                {
                    GateRef lengthTag = GetCallArg1(numArgs);
                    BRANCH(TaggedIsNumber(lengthTag), &lengthTagIsNumber, slowPath);
                    Bind(&lengthTagIsNumber);
                    BRANCH(TaggedIsInt(lengthTag), &lengthTagIsInt, slowPath);
                    Bind(&lengthTagIsInt);
                    {
                        end = GetInt32OfTInt(lengthTag);
                        Jump(&countStart);
                    }
                }
            }
            Bind(&countStart);
            {
                Label startLessZero(env);
                Label newStartGreaterThanZero(env);
                Label newStartNotGreaterThanZero(env);
                BRANCH(Int32LessThan(*start, Int32(0)), &startLessZero, &countResultLength);
                Bind(&startLessZero);
                {
                    GateRef newStart = Int32Add(*start, thisLen);
                    BRANCH(Int32GreaterThan(newStart, Int32(0)), &newStartGreaterThanZero, &newStartNotGreaterThanZero);
                    Bind(&newStartGreaterThanZero);
                    {
                        start = newStart;
                        Jump(&countResultLength);
                    }
                    Bind(&newStartNotGreaterThanZero);
                    {
                        start = Int32(0);
                        Jump(&countResultLength);
                    }
                }
            }
            Bind(&countResultLength);
            {
                BRANCH(Int32GreaterThan(*end, Int32(0)), &endGreatZero, &countResultLength1);
                Bind(&endGreatZero);
                {
                    tempLength = *end;
                    Jump(&countResultLength1);
                }
            }
            Bind(&countResultLength1);
            {
                Label tempLenLessLength(env);
                Label tempLenNotLessLength(env);
                GateRef length = Int32Sub(thisLen, *start);
                BRANCH(Int32LessThan(*tempLength, length), &tempLenLessLength, &tempLenNotLessLength);
                Bind(&tempLenLessLength);
                {
                    resultLength = *tempLength;
                    Jump(&countRes);
                }
                Bind(&tempLenNotLessLength);
                {
                    resultLength = length;
                    Jump(&countRes);
                }
            }
            Bind(&countRes);
            {
                Label emptyString(env);
                Label fastSubString(env);

                BRANCH(Int32LessThanOrEqual(*resultLength, Int32(0)), &emptyString, &fastSubString);
                Bind(&emptyString);
                {
                    res->WriteVariable(GetGlobalConstantValue(
                        VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX));
                    Jump(exit);
                }
                Bind(&fastSubString);
                {
                    Label thisFlattenFastPath(env);
                    FlatStringStubBuilder thisFlat(this);
                    thisFlat.FlattenString(glue, thisValue, &thisFlattenFastPath);
                    Bind(&thisFlattenFastPath);
                    StringInfoGateRef stringInfoGate(&thisFlat);
                    GateRef result = FastSubString(glue, thisValue, *start, *resultLength, stringInfoGate);
                    res->WriteVariable(result);
                    Jump(exit);
                }
            }
        }
    }
}

GateRef BuiltinsStringStubBuilder::GetSubString(GateRef glue, GateRef thisValue, GateRef from, GateRef len)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label exit(env);
    Label flattenFastPath(env);
    Label sliceString(env);
    Label mayGetSliceString(env);
    Label fastSubstring(env);
    Label isUtf16(env);
    Label isUtf8(env);
    Label afterNew(env);
    Label isSingleChar(env);
    Label notSingleChar(env);
    Label getStringFromSingleCharTable(env);
    FlatStringStubBuilder thisFlat(this);
    thisFlat.FlattenString(glue, thisValue, &flattenFastPath);
    Bind(&flattenFastPath);
    {
        BRANCH(Int32Equal(len, Int32(1)), &isSingleChar, &notSingleChar);
        Bind(&isSingleChar);
        {
            StringInfoGateRef stringInfoGate1(&thisFlat);
            GateRef charCode = StringAt(glue, stringInfoGate1, from);
            GateRef canStoreAsUtf8 = IsASCIICharacter(charCode);
            BRANCH(canStoreAsUtf8, &getStringFromSingleCharTable, &fastSubstring);
            Bind(&getStringFromSingleCharTable);
            {
                GateRef singleCharTable = GetSingleCharTable(glue);
                result = GetValueFromTaggedArray(glue, singleCharTable, ZExtInt16ToInt32(charCode));
                Jump(&exit);
            }
        }
        Bind(&notSingleChar);
        BRANCH(Int32GreaterThanOrEqual(len, Int32(SlicedString::MIN_SLICED_STRING_LENGTH)),
            &mayGetSliceString, &fastSubstring);
        Bind(&mayGetSliceString);
        {
            BRANCH(IsUtf16String(thisValue), &isUtf16, &sliceString);
            Bind(&isUtf16);
            {
                StringInfoGateRef stringInfoGate(&thisFlat);
                GateRef fromOffset = PtrMul(ZExtInt32ToPtr(from), IntPtr(sizeof(uint16_t) / sizeof(uint8_t)));
                GateRef source = PtrAdd(GetNormalStringData(glue, stringInfoGate), fromOffset);
                GateRef canBeCompressed = CanBeCompressed(source, len, true);
                BRANCH(canBeCompressed, &isUtf8, &sliceString);
                Bind(&isUtf8);
                {
                    NewObjectStubBuilder newBuilder(this);
                    newBuilder.SetParameters(glue, 0);
                    newBuilder.AllocLineStringObject(&result, &afterNew, len, true);
                    Bind(&afterNew);
                    {
                        GateRef source1 = PtrAdd(GetNormalStringData(glue, stringInfoGate), fromOffset);
                        GateRef dst =
                            ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
                        CopyUtf16AsUtf8(glue, dst, source1, len);
                        Jump(&exit);
                    }
                }
            }
            Bind(&sliceString);
            {
                NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
                newBuilder.SetParameters(glue, 0);
                newBuilder.AllocSlicedStringObject(&result, &exit, from, len, &thisFlat);
            }
        }
        Bind(&fastSubstring);
        StringInfoGateRef stringInfoGate(&thisFlat);
        result = FastSubString(glue, thisValue, from, len, stringInfoGate);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::GetFastSubString(GateRef glue, GateRef thisValue, GateRef start, GateRef len)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label thisFlattenFastPath(env);
    FlatStringStubBuilder thisFlat(this);
    thisFlat.FlattenString(glue, thisValue, &thisFlattenFastPath);
    Bind(&thisFlattenFastPath);
    StringInfoGateRef stringInfoGate(&thisFlat);
    GateRef result = FastSubString(glue, thisValue, start, len, stringInfoGate);
    env->SubCfgExit();
    return result;
}

void BuiltinsStringStubBuilder::Replace(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();

    Label objNotUndefinedAndNull(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label tagsDefined(env);
        Label searchIsHeapObj(env);
        Label replaceIsHeapObj(env);

        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(Int64Equal(IntPtr(2), numArgs), &tagsDefined, slowPath); // 2: number of parameters. search & replace Tag
        Bind(&tagsDefined);
        {
            Label next(env);

            GateRef searchTag = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(searchTag), &searchIsHeapObj, slowPath);
            Bind(&searchIsHeapObj);
            GateRef replaceTag = GetCallArg1(numArgs);
            BRANCH(TaggedIsHeapObject(replaceTag), &replaceIsHeapObj, slowPath);
            Bind(&replaceIsHeapObj);
            BRANCH(LogicOrBuilder(env).Or(IsJSRegExp(glue, searchTag)).Or(IsEcmaObject(glue, searchTag)).Done(),
                slowPath, &next);
            Bind(&next);
            {
                Label allAreStrings(env);
                GateRef allIsString = LogicAndBuilder(env).And(IsString(glue, thisValue)).And(IsString(glue, searchTag))
                    .And(IsString(glue, replaceTag)).Done();
                BRANCH(allIsString, &allAreStrings, slowPath);
                Bind(&allAreStrings);
                {
                    Label replaceTagNotCallable(env);

                    GateRef replaceTagIsCallable = IsCallable(glue, replaceTag);

                    BRANCH(replaceTagIsCallable, slowPath, &replaceTagNotCallable);
                    Bind(&replaceTagNotCallable);
                    {
                        Label thisFlattenFastPath(env);
                        Label searchFlattenFastPath(env);
                        Label noReplace(env);
                        Label nextProcess(env);

                        FlatStringStubBuilder thisFlat(this);
                        thisFlat.FlattenString(glue, thisValue, &thisFlattenFastPath);
                        Bind(&thisFlattenFastPath);
                        StringInfoGateRef thisStringInfoGate(&thisFlat);
                        FlatStringStubBuilder searchFlat(this);
                        searchFlat.FlattenString(glue, searchTag, &searchFlattenFastPath);
                        Bind(&searchFlattenFastPath);
                        StringInfoGateRef searchStringInfoGate(&searchFlat);
                        GateRef pos = StringIndexOf(glue, thisStringInfoGate, searchStringInfoGate, Int32(-1));
                        BRANCH(Int32Equal(pos, Int32(-1)), &noReplace, &nextProcess);
                        Bind(&noReplace);
                        {
                            res->WriteVariable(thisValue);
                            Jump(exit);
                        }
                        Bind(&nextProcess);
                        {
                            Label functionalReplaceFalse(env);

                            BRANCH(replaceTagIsCallable, slowPath, &functionalReplaceFalse);
                            Bind(&functionalReplaceFalse);
                            {
                                Label replHandleIsString(env);

                                GateRef replHandle = GetSubstitution(glue, searchTag, thisValue, pos, replaceTag);
                                BRANCH(IsString(glue, replHandle), &replHandleIsString, slowPath);
                                Bind(&replHandleIsString);
                                {
                                    GateRef tailPos = Int32Add(pos, searchStringInfoGate.GetLength());
                                    GateRef prefixString = FastSubString(glue, thisValue, Int32(0),
                                        pos, thisStringInfoGate);
                                    GateRef thisLen = thisStringInfoGate.GetLength();
                                    GateRef suffixString = FastSubString(glue, thisValue, tailPos,
                                        Int32Sub(thisLen, tailPos), thisStringInfoGate);
                                    GateRef tempStr = StringConcat(glue, prefixString, replHandle);
                                    GateRef resultStr = StringConcat(glue, tempStr, suffixString);
                                    res->WriteVariable(resultStr);
                                    Jump(exit);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

GateRef BuiltinsStringStubBuilder::ConvertAndClampRelativeIndex(GateRef index, GateRef length)
{
    auto env = GetEnvironment();

    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(relativeIndex, VariableType::INT32(), Int32(-1));

    Label indexGreaterThanOrEqualZero(env);
    Label indexLessThanZero(env);
    Label next(env);

    BRANCH(Int32GreaterThanOrEqual(index, Int32(0)), &indexGreaterThanOrEqualZero, &indexLessThanZero);
    Bind(&indexGreaterThanOrEqualZero);
    {
        relativeIndex = index;
        Jump(&next);
    }
    Bind(&indexLessThanZero);
    {
        relativeIndex = Int32Add(index, length);
        Jump(&next);
    }
    Bind(&next);
    {
        Label relativeIndexLessThanZero(env);
        Label elseCheck(env);
        Label exit(env);

        BRANCH(Int32LessThan(*relativeIndex, Int32(0)), &relativeIndexLessThanZero, &elseCheck);
        Bind(&relativeIndexLessThanZero);
        {
            relativeIndex = Int32(0);
            Jump(&exit);
        }
        Bind(&elseCheck);
        {
            Label relativeIndexGreaterThanLength(env);

            BRANCH(Int32GreaterThan(*relativeIndex, length), &relativeIndexGreaterThanLength, &exit);
            Bind(&relativeIndexGreaterThanLength);
            {
                relativeIndex = length;
                Jump(&exit);
            }
        }
        Bind(&exit);
        auto ret = *relativeIndex;
        env->SubCfgExit();
        return ret;
    }
}

void BuiltinsStringStubBuilder::Slice(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();

    DEFVARIABLE(start, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(end, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(sliceLen, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label objNotUndefinedAndNull(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label isString(env);

        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            Label startTagDefined(env);

            BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), slowPath, &startTagDefined);
            Bind(&startTagDefined);
            {
                Label startTagIsInt(env);
                Label endTagUndefined(env);
                Label endTagDefined(env);
                Label endTagIsInt(env);
                Label next(env);

                GateRef startTag = GetCallArg0(numArgs);
                BRANCH(TaggedIsInt(startTag), &startTagIsInt, slowPath);
                Bind(&startTagIsInt);
                GateRef thisLen = GetLengthFromString(thisValue);
                start = ConvertAndClampRelativeIndex(GetInt32OfTInt(startTag), thisLen);
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &endTagUndefined, &endTagDefined);
                Bind(&endTagUndefined);
                {
                    end = thisLen;
                    Jump(&next);
                }
                Bind(&endTagDefined);
                {
                    GateRef endTag = GetCallArg1(numArgs);
                    BRANCH(TaggedIsInt(endTag), &endTagIsInt, slowPath);
                    Bind(&endTagIsInt);
                    end = ConvertAndClampRelativeIndex(GetInt32OfTInt(endTag), thisLen);
                    Jump(&next);
                }
                Bind(&next);
                {
                    Label emptyString(env);
                    Label fastSubString(env);
                    Label finish(env);

                    sliceLen = Int32Sub(*end, *start);
                    BRANCH(Int32LessThanOrEqual(*sliceLen, Int32(0)), &emptyString, &fastSubString);
                    Bind(&emptyString);
                    {
                        result = GetGlobalConstantValue(
                            VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
                        Jump(&finish);
                    }
                    Bind(&fastSubString);
                    {
                        Label thisFlattenFastPath(env);
                        FlatStringStubBuilder thisFlat(this);
                        thisFlat.FlattenString(glue, thisValue, &thisFlattenFastPath);
                        Bind(&thisFlattenFastPath);
                        StringInfoGateRef stringInfoGate(&thisFlat);
                        result = FastSubString(glue, thisValue, *start, *sliceLen, stringInfoGate);
                        Jump(&finish);
                    }
                    Bind(&finish);
                    res->WriteVariable(*result);
                    Jump(exit);
                }
            }
        }
    }
}

void BuiltinsStringStubBuilder::Trim(GateRef glue, GateRef thisValue, GateRef numArgs [[maybe_unused]],
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(start, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(end, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(sliceLen, VariableType::INT32(), Int32(-1));

    Label objNotUndefinedAndNull(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label thisIsString(env);

        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(IsString(glue, thisValue), &thisIsString, slowPath);
        Bind(&thisIsString);
        GateRef result = EcmaStringTrim(glue, thisValue, Int32(0)); // 0: mode = TrimMode::TRIM
        res->WriteVariable(result);
        Jump(exit);
    }
}

GateRef BuiltinsStringStubBuilder::StringAt(GateRef glue, const StringInfoGateRef &stringInfoGate, GateRef index)
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
    GateRef dataUtf16 = GetNormalStringData(glue, stringInfoGate);
    BRANCH(IsUtf16String(stringInfoGate.GetString()), &isUtf16, &isUtf8);
    Bind(&isUtf16);
    {
        result = ZExtInt16ToInt32(LoadZeroOffsetPrimitive(VariableType::INT16(), PtrAdd(dataUtf16,
            PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint16_t))))));
        Jump(&exit);
    }
    Bind(&isUtf8);
    {
        result = ZExtInt8ToInt32(LoadZeroOffsetPrimitive(VariableType::INT8(), PtrAdd(dataUtf16,
            PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint8_t))))));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::GetSingleCharCodeByIndex(GateRef glue, GateRef str, GateRef index)
{
    // Note: This method cannot handle treestring.
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));

    Label lineStringCheck(env);
    Label isLineString(env);
    Label slicedStringCheck(env);
    Label isSlicedString(env);
    Label exit(env);

    BRANCH(IsLineString(glue, str), &isLineString, &slicedStringCheck);
    Bind(&isLineString);
    {
        result = GetSingleCharCodeFromLineString(str, index);
        Jump(&exit);
    }
    Bind(&slicedStringCheck);
    BRANCH(IsSlicedString(glue, str), &isSlicedString, &exit);
    Bind(&isSlicedString);
    {
        result = GetSingleCharCodeFromSlicedString(glue, str, index);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::GetSingleCharCodeFromLineString(GateRef str, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef dataAddr = ChangeStringTaggedPointerToInt64(PtrAdd(str, IntPtr(LineString::DATA_OFFSET)));
    Label isUtf16(env);
    Label isUtf8(env);
    Label exit(env);
    BRANCH(IsUtf16String(str), &isUtf16, &isUtf8);
    Bind(&isUtf16);
    {
        result = ZExtInt16ToInt32(LoadZeroOffsetPrimitive(VariableType::INT16(), PtrAdd(dataAddr,
            PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint16_t))))));
        Jump(&exit);
    }
    Bind(&isUtf8);
    {
        result = ZExtInt8ToInt32(LoadZeroOffsetPrimitive(VariableType::INT8(), PtrAdd(dataAddr,
            PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint8_t))))));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::GetSingleCharCodeFromSlicedString(GateRef glue, GateRef str, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));

    GateRef parent = Load(VariableType::JS_POINTER(), glue, str, IntPtr(SlicedString::PARENT_OFFSET));
    GateRef startIndex = LoadStartIndex(str);
    result = GetSingleCharCodeFromLineString(parent, Int32Add(startIndex, index));
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::CreateStringBySingleCharCode(GateRef glue, GateRef charCode)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Hole());

    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);

    Label exit(env);
    Label utf8(env);
    Label utf16(env);
    Label afterNew(env);
    GateRef canStoreAsUtf8 = IsASCIICharacter(charCode);
    BRANCH(canStoreAsUtf8, &utf8, &utf16);
    Bind(&utf8);
    {
        GateRef singleCharTable = GetSingleCharTable(glue);
        result = GetValueFromTaggedArray(glue, singleCharTable, charCode);
        Jump(&exit);
    }
    Bind(&utf16);
    {
        newBuilder.AllocLineStringObject(&result, &afterNew, Int32(1), false);
    }
    Bind(&afterNew);
    {
        Label isUtf8Copy(env);
        Label isUtf16Copy(env);
        GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
        BRANCH(canStoreAsUtf8, &isUtf8Copy, &isUtf16Copy);
        Bind(&isUtf8Copy);
        {
            Store(VariableType::INT8(), glue, dst, IntPtr(0), TruncInt32ToInt8(charCode));
            Jump(&exit);
        }
        Bind(&isUtf16Copy);
        {
            Store(VariableType::INT16(), glue, dst, IntPtr(0), TruncInt32ToInt16(charCode));
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::CreateFromEcmaString(GateRef glue, GateRef index,
    const StringInfoGateRef &stringInfoGate)
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
    GateRef dataUtf = GetNormalStringData(glue, stringInfoGate);
    BRANCH(IsUtf16String(stringInfoGate.GetString()), &isUtf16, &isUtf8);
    Bind(&isUtf16);
    {
        GateRef dataAddr = PtrAdd(dataUtf, PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint16_t))));
        data = LoadZeroOffsetPrimitive(VariableType::INT16(), dataAddr);
        canBeCompressed = CanBeCompressed(dataAddr, Int32(1), true);
        Jump(&allocString);
    }
    Bind(&isUtf8);
    {
        GateRef dataAddr = PtrAdd(dataUtf, PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint8_t))));
        data = ZExtInt8ToInt16(LoadZeroOffsetPrimitive(VariableType::INT8(), dataAddr));
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
        BRANCH(*canBeCompressed, &isUtf8Next, &isUtf16Next);
        Bind(&isUtf8Next);
        {
            GateRef singleCharTable = GetSingleCharTable(glue);
            result = GetValueFromTaggedArray(glue, singleCharTable, ZExtInt16ToInt32(*data));
            Jump(&exit);
        }
        Bind(&isUtf16Next);
        {
            newBuilder.AllocLineStringObject(&result, &afterNew, Int32(1), false);
        }
        Bind(&afterNew);
        {
            Label isUtf8Copy(env);
            Label isUtf16Copy(env);
            GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
            BRANCH(*canBeCompressed, &isUtf8Copy, &isUtf16Copy);
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

GateRef BuiltinsStringStubBuilder::FastSubString(GateRef glue, GateRef thisValue, GateRef from,
    GateRef len, const StringInfoGateRef &stringInfoGate)
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

    BRANCH(Int32Equal(len, Int32(0)), &lenEqualZero, &lenNotEqualZero);
    Bind(&lenEqualZero);
    {
        result = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
        Jump(&exit);
    }
    Bind(&lenNotEqualZero);
    {
        BRANCH(Int32Equal(from, Int32(0)), &fromEqualZero, &next);
        Bind(&fromEqualZero);
        {
            GateRef thisLen = stringInfoGate.GetLength();
            BRANCH(Int32Equal(len, thisLen), &exit, &next);
        }
        Bind(&next);
        {
            BRANCH(IsUtf8String(thisValue), &isUtf8, &isUtf16);
            Bind(&isUtf8);
            {
                result = FastSubUtf8String(glue, from, len, stringInfoGate);
                Jump(&exit);
            }
            Bind(&isUtf16);
            {
                result = FastSubUtf16String(glue, from, len, stringInfoGate);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::FastSubUtf8String(GateRef glue, GateRef from, GateRef len,
    const StringInfoGateRef &stringInfoGate)
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
        GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
        GateRef source = PtrAdd(GetNormalStringData(glue, stringInfoGate), ZExtInt32ToPtr(from));
        CopyChars(glue, dst, source, len, IntPtr(sizeof(uint8_t)), VariableType::INT8());
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::FastSubUtf16String(GateRef glue, GateRef from, GateRef len,
    const StringInfoGateRef &stringInfoGate)
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
    GateRef source = PtrAdd(GetNormalStringData(glue, stringInfoGate), fromOffset);
    GateRef canBeCompressed = CanBeCompressed(source, len, true);
    NewObjectStubBuilder newBuilder(this);
    newBuilder.SetParameters(glue, 0);
    Label afterNew(env);
    BRANCH(canBeCompressed, &isUtf8, &isUtf16);
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
        GateRef source1 = PtrAdd(GetNormalStringData(glue, stringInfoGate), fromOffset);
        GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
        BRANCH(canBeCompressed, &isUtf8Next, &isUtf16Next);
        Bind(&isUtf8Next);
        {
            CopyUtf16AsUtf8(glue, dst, source1, len);
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

GateRef BuiltinsStringStubBuilder::GetSubstitution(GateRef glue, GateRef searchString, GateRef thisString,
    GateRef pos, GateRef replaceString)
{
    auto env = GetEnvironment();

    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label dollarFlattenFastPath(env);
    Label replaceFlattenFastPath(env);
    Label notFound(env);
    Label slowPath(env);
    Label exit(env);

    GateRef dollarString = GetGlobalConstantValue(VariableType::JS_POINTER(), glue, ConstantIndex::DOLLAR_INDEX);
    FlatStringStubBuilder dollarFlat(this);
    dollarFlat.FlattenString(glue, dollarString, &dollarFlattenFastPath);
    Bind(&dollarFlattenFastPath);
    StringInfoGateRef dollarStringInfoGate(&dollarFlat);
    FlatStringStubBuilder replaceFlat(this);
    replaceFlat.FlattenString(glue, replaceString, &replaceFlattenFastPath);
    Bind(&replaceFlattenFastPath);
    StringInfoGateRef replaceStringInfoGate(&replaceFlat);
    GateRef nextDollarIndex = StringIndexOf(glue, replaceStringInfoGate, dollarStringInfoGate, Int32(-1));
    BRANCH(Int32LessThan(nextDollarIndex, Int32(0)), &notFound, &slowPath);
    Bind(&notFound);
    {
        result = replaceString;
        Jump(&exit);
    }
    Bind(&slowPath);
    {
        result = CallRuntime(glue, RTSTUB_ID(RTSubstitution),
            {searchString, thisString, IntToTaggedInt(pos), replaceString});
        Jump(&exit);
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
    // Are used NATIVE_POINTERs instead JS_POINTERs, becasuse "CopyChars" have parameters "dst" and "source" which
    // already NATIVE_POINTER to buffer of strings. We can't track original string objects, so can't insert SafePoints
    // and call GC
    DEFVARIABLE(dstTmp, VariableType::NATIVE_POINTER(), dst);
    DEFVARIABLE(sourceTmp, VariableType::NATIVE_POINTER(), source);
    DEFVARIABLE(len, VariableType::INT32(), sourceLength);
    Label loopHead(env);
    Label loopEnd(env);
    Label storeTail(env);
    Label exit(env);
    uint8_t elemSize = 0;
    MachineType mt = type.GetMachineType();
    if (mt == I8) {
        elemSize = sizeof(int8_t);
    } else if (mt == I16) {
        elemSize = sizeof(int16_t);
    } else {
        LOG_COMPILER(FATAL) << "Unhandled VariableType: " << mt;
    };
    const constexpr int32_t batchBytes = 2 * sizeof(int64_t); // copy 16 bytes in one iterator.
    uint8_t elemInBatch = batchBytes / elemSize;

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        // loop copy, copy 16 bytes in every iteration. Until the remain is less than 16 bytes.
        Label body(env);
        BRANCH_NO_WEIGHT(Int32GreaterThanOrEqual(*len, Int32(elemInBatch)), &body, &storeTail); // len>=16
        Bind(&body);
        {
            len = Int32Sub(*len, Int32(elemInBatch));
            GateRef elem = LoadZeroOffsetPrimitive(VariableType::INT64(), *sourceTmp);
            Store(VariableType::INT64(), glue, *dstTmp, IntPtr(0), elem);
            elem = LoadZeroOffsetPrimitive(VariableType::INT64(), PtrAdd(*sourceTmp, IntPtr(sizeof(int64_t))));
            Store(VariableType::INT64(), glue, *dstTmp, IntPtr(sizeof(int64_t)), elem);
            Jump(&loopEnd);
        }
    }
    Bind(&loopEnd);
    sourceTmp = PtrAdd(*sourceTmp, IntPtr(batchBytes));
    dstTmp = PtrAdd(*dstTmp, IntPtr(batchBytes));
    // Work with low level buffers, we can't call GC. Loop is simple.
    LoopEnd(&loopHead);

    uint8_t elemInInt64 = sizeof(int64_t) / elemSize;
    Bind(&storeTail);
    {
        // If the remain larger than 8 bytes, copy once and make the remain less than 8 bytes.
        Label storeTail8_16(env);
        Label storeTail0_8(env);
        // 16 > len >= 8
        BRANCH_NO_WEIGHT(Int32GreaterThanOrEqual(*len, Int32(elemInInt64)), &storeTail8_16, &storeTail0_8);
        Bind(&storeTail8_16);
        {
            GateRef elem = LoadZeroOffsetPrimitive(VariableType::INT64(), *sourceTmp);
            Store(VariableType::INT64(), glue, *dstTmp, IntPtr(0), elem);
            len = Int32Sub(*len, Int32(elemInInt64));
            sourceTmp = PtrAdd(*sourceTmp, IntPtr(sizeof(int64_t)));
            dstTmp = PtrAdd(*dstTmp, IntPtr(sizeof(int64_t)));
            Jump(&storeTail0_8);
        }
        Bind(&storeTail0_8);
        {
            Label tailLoopHead(env);
            Label tailLoopEnd(env);
            Jump(&tailLoopHead);
            LoopBegin(&tailLoopHead);
            {
                Label body(env);
                // 8 > len > 0
                BRANCH_NO_WEIGHT(Int32GreaterThan(*len, Int32(0)), &body, &exit);
                Bind(&body);
                {
                    len = Int32Sub(*len, Int32(1));
                    GateRef i = LoadZeroOffset(type, glue, *sourceTmp);
                    Store(type, glue, *dstTmp, IntPtr(0), i);
                    Jump(&tailLoopEnd);
                }
            }
            Bind(&tailLoopEnd);
            sourceTmp = PtrAdd(*sourceTmp, size);
            dstTmp = PtrAdd(*dstTmp, size);
            // Work with low level buffers, we can't call GC. Loop is simple.
            LoopEnd(&tailLoopHead);
        }
    }
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
        BRANCH(Int32LessThan(*i, len), &nextCount, &exit);
        Bind(&nextCount);
        {
            if (isUtf16) {
                GateRef tmp = LoadPrimitive(VariableType::INT16(), data,
                    PtrMul(ZExtInt32ToPtr(*i), IntPtr(sizeof(uint16_t))));
                BRANCH(IsASCIICharacter(ZExtInt16ToInt32(tmp)), &loopEnd, &isNotASCIICharacter);
            } else {
                GateRef tmp = LoadPrimitive(VariableType::INT8(), data,
                    PtrMul(ZExtInt32ToPtr(*i), IntPtr(sizeof(uint8_t))));
                BRANCH(IsASCIICharacter(ZExtInt8ToInt32(tmp)), &loopEnd, &isNotASCIICharacter);
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

// source is utf8, dst is utf16
void BuiltinsStringStubBuilder::CopyUtf8AsUtf16(GateRef glue, GateRef dst, GateRef src,
    GateRef sourceLength)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    // Are used NATIVE_POINTERs instead JS_POINTERs, becasuse "CopyUtf8AsUtf16" have parameters "dst" and "src" which
    // already NATIVE_POINTER to buffer of strings. We can't track original string objects, so can't insert SafePoints
    // and call GC
    DEFVARIABLE(dstTmp, VariableType::NATIVE_POINTER(), dst);
    DEFVARIABLE(sourceTmp, VariableType::NATIVE_POINTER(), src);
    DEFVARIABLE(len, VariableType::INT32(), sourceLength);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label exit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32GreaterThan(*len, Int32(0)), &next, &exit);
        Bind(&next);
        {
            len = Int32Sub(*len, Int32(1));
            GateRef i = LoadZeroOffsetPrimitive(VariableType::INT8(), *sourceTmp);
            Store(VariableType::INT16(), glue, *dstTmp, IntPtr(0), ZExtInt8ToInt16(i));
            Jump(&loopEnd);
        }
    }

    Bind(&loopEnd);
    sourceTmp = PtrAdd(*sourceTmp, IntPtr(sizeof(uint8_t)));
    dstTmp = PtrAdd(*dstTmp, IntPtr(sizeof(uint16_t)));
    // Work with low level buffers, we can't call GC. Loop is simple.
    LoopEnd(&loopHead);

    Bind(&exit);
    env->SubCfgExit();
    return;
}

// source is utf16, dst is utf8
void BuiltinsStringStubBuilder::CopyUtf16AsUtf8(GateRef glue, GateRef dst, GateRef src,
    GateRef sourceLength)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    // Are used NATIVE_POINTERs instead JS_POINTERs, becasuse "CopyUtf8AsUtf16" have parameters "dst" and "src" which
    // already NATIVE_POINTER to buffer of strings. We can't track original string objects, so can't insert SafePoints
    // and call GC.
    DEFVARIABLE(dstTmp, VariableType::NATIVE_POINTER(), dst);
    DEFVARIABLE(sourceTmp, VariableType::NATIVE_POINTER(), src);
    DEFVARIABLE(len, VariableType::INT32(), sourceLength);
    Label loopHead(env);
    Label loopEnd(env);
    Label next(env);
    Label exit(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        BRANCH(Int32GreaterThan(*len, Int32(0)), &next, &exit);
        Bind(&next);
        {
            len = Int32Sub(*len, Int32(1));
            GateRef i = LoadZeroOffsetPrimitive(VariableType::INT16(), *sourceTmp);
            Store(VariableType::INT8(), glue, *dstTmp, IntPtr(0), TruncInt16ToInt8(i));
            Jump(&loopEnd);
        }
    }

    Bind(&loopEnd);
    sourceTmp = PtrAdd(*sourceTmp, IntPtr(sizeof(uint16_t)));
    dstTmp = PtrAdd(*dstTmp, IntPtr(sizeof(uint8_t)));
    // Work with low level buffers, we can't call GC. Loop is simple.
    LoopEnd(&loopHead);

    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef BuiltinsStringStubBuilder::GetUtf16Data(GateRef stringData, GateRef index)
{
    return ZExtInt16ToInt32(LoadZeroOffsetPrimitive(VariableType::INT16(), PtrAdd(stringData,
        PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint16_t))))));
}

GateRef BuiltinsStringStubBuilder::IsASCIICharacter(GateRef data)
{
    return Int32UnsignedLessThan(Int32Sub(data, Int32(1)), Int32(common::utf_helper::UTF8_1B_MAX));
}

GateRef BuiltinsStringStubBuilder::GetUtf8Data(GateRef stringData, GateRef index)
{
    return ZExtInt8ToInt32(LoadZeroOffsetPrimitive(VariableType::INT8(), PtrAdd(stringData,
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
        first = ZExtInt8ToInt32(LoadZeroOffsetPrimitive(VariableType::INT8(), rhsData));
    } else {
        first = ZExtInt16ToInt32(LoadZeroOffsetPrimitive(VariableType::INT16(), rhsData));
    }
    Jump(&loopHead);
    LoopBegin(&loopHead);
    BRANCH(Int32LessThanOrEqual(*i, max), &next, &exit);
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
        BRANCH(Int32NotEqual(lhsTemp, first), &nextCount1, &nextCount);
        Bind(&nextCount1);
        {
            i = Int32Add(*i, Int32(1));
            Jump(&loopHead1);
        }
        LoopBegin(&loopHead1);
        {
            BRANCH(Int32LessThanOrEqual(*i, max), &continueFor, &nextCount);
            Bind(&continueFor);
            {
                GateRef lhsTemp1;
                if (lhsIsUtf8) {
                    lhsTemp1 = GetUtf8Data(lhsData, *i);
                } else {
                    lhsTemp1 = GetUtf16Data(lhsData, *i);
                }
                BRANCH(Int32NotEqual(lhsTemp1, first), &lhsNotEqualFirst, &nextCount);
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
            BRANCH(Int32LessThanOrEqual(*i, max), &continueCount, &loopEnd);
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
                    BRANCH(Int32LessThan(*j, end), &lessEnd, &nextCount2);
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
                        BRANCH(Int32Equal(lhsTemp2, rhsTemp), &loopEnd2, &nextCount2);
                    }
                }
                Bind(&loopEnd2);
                j = Int32Add(*j, Int32(1));
                k = Int32Add(*k, Int32(1));
                LoopEnd(&loopHead2);
                Bind(&nextCount2);
                {
                    BRANCH(Int32Equal(*j, end), &equalEnd, &loopEnd);
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


void BuiltinsStringStubBuilder::StoreParent(GateRef glue, GateRef object, GateRef parent)
{
    Store(VariableType::JS_POINTER(), glue, object, IntPtr(SlicedString::PARENT_OFFSET), parent);
}

void BuiltinsStringStubBuilder::StoreStartIndexAndBackingStore(GateRef glue, GateRef object, GateRef startIndex,
                                                               GateRef hasBackingStore)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(InvalidStringLength),
               Int32LessThanOrEqual(startIndex, Int32(SlicedString::MAX_STRING_LENGTH)));
    GateRef encodedLen = Int32LSL(startIndex, Int32(SlicedString::StartIndexBits::START_BIT));
    GateRef newValue = Int32Or(encodedLen, ZExtInt1ToInt32(hasBackingStore));
    Store(VariableType::INT32(), glue, object, IntPtr(SlicedString::STARTINDEX_AND_FLAGS_OFFSET), newValue);
}

GateRef BuiltinsStringStubBuilder::LoadStartIndex(GateRef object)
{
    GateRef offset = IntPtr(SlicedString::STARTINDEX_AND_FLAGS_OFFSET);
    GateRef mixStartIndex = LoadPrimitive(VariableType::INT32(), object, offset);
    return Int32LSR(mixStartIndex, Int32(SlicedString::StartIndexBits::START_BIT));
}

GateRef BuiltinsStringStubBuilder::LoadHasBackingStore(GateRef object)
{
    GateRef offset = IntPtr(SlicedString::STARTINDEX_AND_FLAGS_OFFSET);
    GateRef mixStartIndex = LoadPrimitive(VariableType::INT32(), object, offset);
    return TruncInt32ToInt1(Int32And(mixStartIndex, Int32((1 << SlicedString::HasBackingStoreBit::SIZE) - 1)));
}

GateRef BuiltinsStringStubBuilder::StringIndexOf(GateRef glue, const StringInfoGateRef &lStringInfoGate,
    const StringInfoGateRef &rStringInfoGate, GateRef pos)
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

    GateRef lhsCount = lStringInfoGate.GetLength();
    GateRef rhsCount = rStringInfoGate.GetLength();

    BRANCH(Int32GreaterThan(pos, lhsCount), &exit, &nextCount);
    Bind(&nextCount);
    {
        BRANCH(Int32Equal(rhsCount, Int32(0)), &rhsCountEqualZero, &rhsCountNotEqualZero);
        Bind(&rhsCountEqualZero);
        {
            result = pos;
            Jump(&exit);
        }
        Bind(&rhsCountNotEqualZero);
        {
            BRANCH(Int32LessThan(pos, Int32(0)), &posLessZero, &posNotLessZero);
            Bind(&posLessZero);
            {
                posTag = Int32(0);
                Jump(&posNotLessZero);
            }
            Bind(&posNotLessZero);
            {
                GateRef max = Int32Sub(lhsCount, rhsCount);
                BRANCH(Int32LessThan(max, Int32(0)), &exit, &maxNotLessZero);
                Bind(&maxNotLessZero);
                {
                    GateRef posRMax = Int32Add(*posTag, rhsCount);
                    BRANCH(Int32GreaterThan(posRMax, lhsCount), &exit, &posRMaxNotGreaterLhs);
                    Bind(&posRMaxNotGreaterLhs);
                    GateRef rhsData = GetNormalStringData(glue, rStringInfoGate);
                    GateRef lhsData = GetNormalStringData(glue, lStringInfoGate);
                    BRANCH(IsUtf8String(rStringInfoGate.GetString()), &rhsIsUtf8, &rhsIsUtf16);
                    Bind(&rhsIsUtf8);
                    {
                        Label lhsIsUtf8(env);
                        Label lhsIsUtf16(env);
                        BRANCH(IsUtf8String(lStringInfoGate.GetString()), &lhsIsUtf8, &lhsIsUtf16);
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
                        BRANCH(IsUtf8String(lStringInfoGate.GetString()), &lhsIsUtf8, &lhsIsUtf16);
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

void FlatStringStubBuilder::FlattenString(GateRef glue, GateRef str, Label *fastPath)
{
    auto env = GetEnvironment();
    Label notLineString(env);
    Label exit(env);
    length_ = GetLengthFromString(str);
    BRANCH(IsLineString(glue, str), &exit, &notLineString);
    Bind(&notLineString);
    {
        Label isTreeString(env);
        Label notTreeString(env);
        Label isSlicedString(env);
        BRANCH(IsTreeString(glue, str), &isTreeString, &notTreeString);
        Bind(&isTreeString);
        {
            Label isFlat(env);
            Label notFlat(env);
            BRANCH(TreeStringIsFlat(glue, str), &isFlat, &notFlat);
            Bind(&isFlat);
            {
                flatString_.WriteVariable(GetFirstFromTreeString(glue, str));
                Jump(fastPath);
            }
            Bind(&notFlat);
            {
                flatString_.WriteVariable(CallRuntime(glue, RTSTUB_ID(SlowFlattenString), { str }));
                Jump(fastPath);
            }
        }
        Bind(&notTreeString);
        BRANCH(IsSlicedString(glue, str), &isSlicedString, &exit);
        Bind(&isSlicedString);
        {
            flatString_.WriteVariable(GetParentFromSlicedString(glue, str));
            startIndex_.WriteVariable(GetStartIndexFromSlicedString(str));
            Jump(fastPath);
        }
    }
    Bind(&exit);
    {
        flatString_.WriteVariable(str);
        Jump(fastPath);
    }
}

void FlatStringStubBuilder::FlattenStringWithIndex(GateRef glue, GateRef str, Variable *index, Label *fastPath)
{
    // Note this method modifies "index" variable for Sliced String
    auto env = GetEnvironment();
    Label notLineString(env);
    Label exit(env);
    BRANCH(IsLineString(glue, str), &exit, &notLineString);
    Bind(&notLineString);
    {
        Label isTreeString(env);
        Label notTreeString(env);
        Label isSlicedString(env);
        BRANCH(IsTreeString(glue, str), &isTreeString, &notTreeString);
        Bind(&isTreeString);
        {
            Label isFlat(env);
            Label notFlat(env);
            BRANCH(TreeStringIsFlat(glue, str), &isFlat, &notFlat);
            Bind(&isFlat);
            {
                flatString_.WriteVariable(GetFirstFromTreeString(glue, str));
                Jump(fastPath);
            }
            Bind(&notFlat);
            {
                flatString_.WriteVariable(CallRuntime(glue, RTSTUB_ID(SlowFlattenString), { str }));
                Jump(fastPath);
            }
        }
        Bind(&notTreeString);
        BRANCH(IsSlicedString(glue, str), &isSlicedString, &exit);
        Bind(&isSlicedString);
        {
            flatString_.WriteVariable(GetParentFromSlicedString(glue, str));
            startIndex_.WriteVariable(GetStartIndexFromSlicedString(str));
            index->WriteVariable(Int32Add(*startIndex_, index->ReadVariable()));
            Jump(fastPath);
        }
    }
    Bind(&exit);
    {
        flatString_.WriteVariable(str);
        Jump(fastPath);
    }
}

void BuiltinsStringStubBuilder::Concat(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label objNotUndefinedAndNull(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label isString(env);

        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            Label noPara(env);
            Label hasPara(env);
            BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &noPara, &hasPara);
            Bind(&noPara);
            {
                res->WriteVariable(thisValue);
                Jump(exit);
            }
            Bind(&hasPara);
            {
                Label writeRes(env);
                Label notArgc1(env);
                Label notArgc2(env);
                Label argc3(env);
                Label arg0IsValid(env);
                Label arg1IsValid(env);
                Label arg2IsValid(env);
                Label notException1(env);
                Label notException2(env);
                GateRef arg0 = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG0_OR_ARGV));
                BRANCH(TaggedIsString(glue, arg0), &arg0IsValid, slowPath);
                Bind(&arg0IsValid);
                {
                    result = StringConcat(glue, thisValue, arg0);
                    BRANCH(TaggedIsException(*result), &writeRes, &notException1);
                    Bind(&notException1);
                    BRANCH(Int64Equal(IntPtr(1), numArgs), &writeRes, &notArgc1);
                    Bind(&notArgc1);
                    {
                        GateRef arg1 = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG1));
                        BRANCH(TaggedIsString(glue, arg1), &arg1IsValid, slowPath);
                        Bind(&arg1IsValid);
                        result = StringConcat(glue, *result, arg1);
                        BRANCH(TaggedIsException(*result), &writeRes, &notException2);
                        Bind(&notException2);
                        BRANCH(Int64Equal(IntPtr(2), numArgs), &writeRes, &notArgc2); // 2: number of parameters.
                        Bind(&notArgc2);
                        GateRef arg2 = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG2));
                        BRANCH(TaggedIsString(glue, arg2), &arg2IsValid, slowPath);
                        Bind(&arg2IsValid);
                        BRANCH(Int64Equal(IntPtr(3), numArgs), &argc3, slowPath); // 3: number of parameters.
                        Bind(&argc3);
                        result = StringConcat(glue, *result, arg2);
                        Jump(&writeRes);
                    }
                    Bind(&writeRes);
                    res->WriteVariable(*result);
                    Jump(exit);
                }
            }
        }
    }
}

void BuiltinsStringStubBuilder::ToLowerCase(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label objNotUndefinedAndNull(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label isString(env);

        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            Label isUtf8(env);
            Label hasPara(env);
            Label isUtf8Next(env);
            Label flattenFastPath(env);
            BRANCH(IsUtf8String(thisValue), &isUtf8, slowPath);
            Bind(&isUtf8);
            {
                GateRef srcLength = GetLengthFromString(thisValue);
                DEFVARIABLE(len, VariableType::INT32(), srcLength);
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue, 0);
                newBuilder.AllocLineStringObject(res, &isUtf8Next, srcLength, true);
                Bind(&isUtf8Next);
                {
                    FlatStringStubBuilder thisFlat(this);
                    thisFlat.FlattenString(glue, thisValue, &flattenFastPath);
                    Bind(&flattenFastPath);
                    StringInfoGateRef stringInfoGate(&thisFlat);
                    GateRef dataUtf8 = GetNormalStringData(glue, stringInfoGate);
                    GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(res->ReadVariable(),
                        IntPtr(LineString::DATA_OFFSET)));
                    DEFVARIABLE(dstTmp, VariableType::NATIVE_POINTER(), dst);
                    DEFVARIABLE(sourceTmp, VariableType::NATIVE_POINTER(), dataUtf8);
                    Label loopHead(env);
                    Label loopEnd(env);
                    Label next(env);
                    Label toLower(env);
                    Label notLower(env);
                    Jump(&loopHead);
                    LoopBegin(&loopHead);
                    {
                        BRANCH(Int32GreaterThan(*len, Int32(0)), &next, exit);
                        Bind(&next);
                        {
                            len = Int32Sub(*len, Int32(1));
                            GateRef i = LoadZeroOffsetPrimitive(VariableType::INT8(), *sourceTmp);
                            // 65: means 'A', 90: means 'Z'
                            GateRef needLower = BitAnd(Int8GreaterThanOrEqual(i, Int8(65)),
                                Int8GreaterThanOrEqual(Int8(90), i));
                            BRANCH(needLower, &toLower, &notLower);
                            Bind(&toLower);
                            GateRef j = Int8Xor(i, Int8(1 << 5));
                            Store(VariableType::INT8(), glue, *dstTmp, IntPtr(0), j);
                            Jump(&loopEnd);
                            Bind(&notLower);
                            Store(VariableType::INT8(), glue, *dstTmp, IntPtr(0), i);
                            Jump(&loopEnd);
                        }
                    }
                    Bind(&loopEnd);
                    sourceTmp = PtrAdd(*sourceTmp, IntPtr(sizeof(uint8_t)));
                    dstTmp = PtrAdd(*dstTmp, IntPtr(sizeof(uint8_t)));
                    LoopEnd(&loopHead);
                }
            }
        }
    }
}

GateRef BuiltinsStringStubBuilder::StringAdd(GateRef glue, GateRef leftString, GateRef rightString, GateRef status)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    auto &builder_ = *env->GetBuilder();

    GateRef left = leftString;
    GateRef right = rightString;
    GateRef leftLength = GetLengthFromString(left);

    Label isFirstConcat(env);
    Label isNotFirstConcat(env);
    Label leftEmpty(env);
    Label leftNotEmpty(env);
    Label slowPath(env);
    Label exit(env);

    DEFVARIABLE(lineString, VariableType::JS_POINTER(), Undefined());
    DEFVARIABLE(slicedString, VariableType::JS_POINTER(), Undefined());
    DEFVARIABLE(newLeft, VariableType::JS_POINTER(), Undefined());
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    BRANCH_CIR(builder_.Equal(leftLength, builder_.Int32(0)), &leftEmpty, &leftNotEmpty);
    builder_.Bind(&leftEmpty);
    {
        result = right;
        builder_.Jump(&exit);
    }
    builder_.Bind(&leftNotEmpty);
    {
        Label rightEmpty(&builder_);
        Label rightNotEmpty(&builder_);
        GateRef rightLength = builder_.GetLengthFromString(right);
        BRANCH_CIR(builder_.Equal(rightLength, builder_.Int32(0)), &rightEmpty, &rightNotEmpty);
        builder_.Bind(&rightEmpty);
        {
            result = left;
            builder_.Jump(&exit);
        }
        builder_.Bind(&rightNotEmpty);
        {
            Label stringConcatOpt(&builder_);
            GateRef newLength = builder_.Int32Add(leftLength, rightLength);
            BRANCH_CIR(builder_.Int32LessThan(newLength,
                builder_.Int32(SlicedString::MIN_SLICED_STRING_LENGTH)), &slowPath, &stringConcatOpt);
            builder_.Bind(&stringConcatOpt);
            {
                GateRef backStoreLength =
                    builder_.Int32Mul(newLength, builder_.Int32(LineString::INIT_LENGTH_TIMES));
                GateRef leftIsUtf8 = builder_.IsUtf8String(left);
                GateRef rightIsUtf8 = builder_.IsUtf8String(right);
                GateRef canBeCompressed = builder_.BitAnd(leftIsUtf8, rightIsUtf8);
                GateRef isValidFirstOpt = IsFirstConcatInStringAdd(Equal(leftIsUtf8, rightIsUtf8), status);
                GateRef isValidOpt = ConcatIsInStringAdd(Equal(leftIsUtf8, rightIsUtf8), status);

                BRANCH_CIR(IsSpecialSlicedString(glue, left), &isNotFirstConcat, &isFirstConcat);
                builder_.Bind(&isFirstConcat);
                {
                    Label fastPath(&builder_);
                    Label canBeConcat(&builder_);
                    Label canBeCompress(&builder_);
                    Label canNotBeCompress(&builder_);
                    Label newSlicedStr(&builder_);
                    BRANCH_CIR(builder_.Int32LessThan(builder_.Int32(LineString::MAX_LENGTH),
                        backStoreLength), &slowPath, &fastPath);
                    builder_.Bind(&fastPath);
                    {
                        BRANCH_CIR(builder_.CanBeConcat(glue, left, right, isValidFirstOpt),
                            &canBeConcat, &slowPath);
                        builder_.Bind(&canBeConcat);
                        {
                            lineString = AllocateLineString(glue, backStoreLength, canBeCompressed);
                            GateRef leftSource = ChangeStringTaggedPointerToInt64(
                                PtrAdd(left, IntPtr(LineString::DATA_OFFSET)));
                            GateRef rightSource = ChangeStringTaggedPointerToInt64(
                                PtrAdd(right, IntPtr(LineString::DATA_OFFSET)));
                            GateRef leftDst = builder_.TaggedPointerToInt64(
                                builder_.PtrAdd(*lineString, builder_.IntPtr(LineString::DATA_OFFSET)));
                            BRANCH_CIR(canBeCompressed, &canBeCompress, &canNotBeCompress);
                            builder_.Bind(&canBeCompress);
                            {
                                GateRef rightDst = builder_.TaggedPointerToInt64(builder_.PtrAdd(leftDst,
                                    builder_.ZExtInt32ToPtr(leftLength)));
                                builder_.CopyChars(glue, leftDst, leftSource, leftLength,
                                                   builder_.IntPtr(sizeof(uint8_t)), VariableType::INT8());
                                builder_.CopyChars(glue, rightDst, rightSource, rightLength,
                                                   builder_.IntPtr(sizeof(uint8_t)), VariableType::INT8());
                                builder_.Jump(&newSlicedStr);
                            }
                            builder_.Bind(&canNotBeCompress);
                            {
                                Label leftIsUtf8L(&builder_);
                                Label leftIsUtf16L(&builder_);
                                Label rightIsUtf8L(&builder_);
                                Label rightIsUtf16L(&builder_);
                                GateRef rightDst = builder_.TaggedPointerToInt64(builder_.PtrAdd(leftDst,
                                    builder_.PtrMul(builder_.ZExtInt32ToPtr(leftLength),
                                    builder_.IntPtr(sizeof(uint16_t)))));
                                BRANCH_CIR(leftIsUtf8, &leftIsUtf8L, &leftIsUtf16L);
                                builder_.Bind(&leftIsUtf8L);
                                {
                                    // left is utf8, right string must utf16
                                    builder_.CopyUtf8AsUtf16(glue, leftDst, leftSource, leftLength);
                                    builder_.CopyChars(glue, rightDst, rightSource, rightLength,
                                        builder_.IntPtr(sizeof(uint16_t)), VariableType::INT16());
                                    builder_.Jump(&newSlicedStr);
                                }
                                builder_.Bind(&leftIsUtf16L);
                                {
                                    builder_.CopyChars(glue, leftDst, leftSource, leftLength,
                                        builder_.IntPtr(sizeof(uint16_t)), VariableType::INT16());
                                    BRANCH_CIR(rightIsUtf8, &rightIsUtf8L, &rightIsUtf16L);
                                    builder_.Bind(&rightIsUtf8L);
                                    builder_.CopyUtf8AsUtf16(glue, rightDst, rightSource, rightLength);
                                    builder_.Jump(&newSlicedStr);
                                    builder_.Bind(&rightIsUtf16L);
                                    builder_.CopyChars(glue, rightDst, rightSource, rightLength,
                                        builder_.IntPtr(sizeof(uint16_t)), VariableType::INT16());
                                    builder_.Jump(&newSlicedStr);
                                }
                            }
                            builder_.Bind(&newSlicedStr);
                            slicedString = AllocateSlicedString(glue, *lineString, newLength, canBeCompressed);
                            result = *slicedString;
                            builder_.Jump(&exit);
                        }
                    }
                }
                builder_.Bind(&isNotFirstConcat);
                {
                    Label fastPath(&builder_);
                    BRANCH_CIR(builder_.CanBackStore(glue, right, isValidOpt), &fastPath, &slowPath);
                    builder_.Bind(&fastPath);
                    {
                        // left string length means current length,
                        // max length means the field which was already initialized.
                        lineString = builder_.Load(VariableType::JS_POINTER(), glue, left,
                                                   builder_.IntPtr(SlicedString::PARENT_OFFSET));

                        GateRef maxLength = builder_.GetLengthFromString(*lineString);
                        Label needsRealloc(&builder_);
                        Label backingStore(&builder_);
                        BRANCH_CIR(builder_.Int32LessThan(maxLength, newLength), &needsRealloc, &backingStore);
                        builder_.Bind(&needsRealloc);
                        {
                            Label newLineStr(&builder_);
                            Label canBeCompress(&builder_);
                            Label canNotBeCompress(&builder_);
                            // The new backing store will have a length of min(2*length, LineString::MAX_LENGTH).
                            GateRef newBackStoreLength = builder_.Int32Mul(newLength, builder_.Int32(2));
                            BRANCH_CIR(builder_.Int32LessThan(newBackStoreLength,
                                builder_.Int32(LineString::MAX_LENGTH)), &newLineStr, &slowPath);
                            builder_.Bind(&newLineStr);
                            {
                                GateRef newBackingStore = AllocateLineString(glue, newBackStoreLength,
                                                                             canBeCompressed);
                                GateRef leftSource = ChangeStringTaggedPointerToInt64(
                                    PtrAdd(*lineString, IntPtr(LineString::DATA_OFFSET)));
                                GateRef rightSource = ChangeStringTaggedPointerToInt64(
                                    PtrAdd(right, IntPtr(LineString::DATA_OFFSET)));
                                GateRef leftDst = builder_.TaggedPointerToInt64(
                                    builder_.PtrAdd(newBackingStore,
                                    builder_.IntPtr(LineString::DATA_OFFSET)));
                                BRANCH_CIR(canBeCompressed, &canBeCompress, &canNotBeCompress);
                                builder_.Bind(&canBeCompress);
                                {
                                    GateRef rightDst = builder_.TaggedPointerToInt64(
                                        builder_.PtrAdd(leftDst, builder_.ZExtInt32ToPtr(leftLength)));
                                    builder_.CopyChars(glue, leftDst, leftSource, leftLength,
                                        builder_.IntPtr(sizeof(uint8_t)), VariableType::INT8());
                                    builder_.CopyChars(glue, rightDst, rightSource, rightLength,
                                        builder_.IntPtr(sizeof(uint8_t)), VariableType::INT8());
                                    newLeft = left;
                                    builder_.Store(VariableType::JS_POINTER(), glue, *newLeft,
                                        builder_.IntPtr(SlicedString::PARENT_OFFSET), newBackingStore);
                                    InitStringLengthAndFlags(glue, *newLeft, newLength, true);
                                    result = *newLeft;
                                    builder_.Jump(&exit);
                                }
                                builder_.Bind(&canNotBeCompress);
                                {
                                    GateRef rightDst = builder_.TaggedPointerToInt64(builder_.PtrAdd(leftDst,
                                        builder_.PtrMul(builder_.ZExtInt32ToPtr(leftLength),
                                        builder_.IntPtr(sizeof(uint16_t)))));
                                    builder_.CopyChars(glue, leftDst, leftSource, leftLength,
                                        builder_.IntPtr(sizeof(uint16_t)), VariableType::INT16());
                                    builder_.CopyChars(glue, rightDst, rightSource, rightLength,
                                        builder_.IntPtr(sizeof(uint16_t)), VariableType::INT16());
                                    newLeft = left;
                                    builder_.Store(VariableType::JS_POINTER(), glue, *newLeft,
                                        builder_.IntPtr(SlicedString::PARENT_OFFSET), newBackingStore);
                                    InitStringLengthAndFlags(glue, *newLeft, newLength, false);
                                    result = *newLeft;
                                    builder_.Jump(&exit);
                                }
                            }
                        }
                        builder_.Bind(&backingStore);
                        {
                            Label canBeCompress(&builder_);
                            Label canNotBeCompress(&builder_);
                            GateRef rightSource = ChangeStringTaggedPointerToInt64(
                                PtrAdd(right, IntPtr(LineString::DATA_OFFSET)));
                            GateRef leftDst = builder_.TaggedPointerToInt64(
                                builder_.PtrAdd(*lineString, builder_.IntPtr(LineString::DATA_OFFSET)));
                            BRANCH_CIR(canBeCompressed, &canBeCompress, &canNotBeCompress);
                            builder_.Bind(&canBeCompress);
                            {
                                GateRef rightDst = builder_.TaggedPointerToInt64(builder_.PtrAdd(leftDst,
                                    builder_.ZExtInt32ToPtr(leftLength)));
                                builder_.CopyChars(glue, rightDst, rightSource, rightLength,
                                    builder_.IntPtr(sizeof(uint8_t)), VariableType::INT8());
                                newLeft = left;
                                builder_.Store(VariableType::JS_POINTER(), glue, *newLeft,
                                    builder_.IntPtr(SlicedString::PARENT_OFFSET), *lineString);
                                InitStringLengthAndFlags(glue, *newLeft, newLength, true);
                                result = *newLeft;
                                builder_.Jump(&exit);
                            }
                            builder_.Bind(&canNotBeCompress);
                            {
                                GateRef rightDst = builder_.TaggedPointerToInt64(builder_.PtrAdd(leftDst,
                                    builder_.PtrMul(builder_.ZExtInt32ToPtr(leftLength),
                                    builder_.IntPtr(sizeof(uint16_t)))));
                                builder_.CopyChars(glue, rightDst, rightSource, rightLength,
                                    builder_.IntPtr(sizeof(uint16_t)), VariableType::INT16());
                                newLeft = left;
                                builder_.Store(VariableType::JS_POINTER(), glue, *newLeft,
                                    builder_.IntPtr(SlicedString::PARENT_OFFSET), *lineString);
                                InitStringLengthAndFlags(glue, *newLeft, newLength, false);
                                result = *newLeft;
                                builder_.Jump(&exit);
                            }
                        }
                    }
                }
                builder_.Bind(&slowPath);
                {
                    result = StringConcat(glue, leftString, rightString);
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::AllocateLineString(GateRef glue, GateRef length, GateRef canBeCompressed)
{
    auto env = GetEnvironment();
    auto &builder_ = *env->GetBuilder();
    Label subentry(&builder_);
    builder_.SubCfgEntry(&subentry);
    Label isUtf8(&builder_);
    Label isUtf16(&builder_);
    Label exit(&builder_);
    DEFVALUE(size, (&builder_), VariableType::INT64(), builder_.Int64(0));
    BRANCH_CIR(canBeCompressed, &isUtf8, &isUtf16);
    builder_.Bind(&isUtf8);
    {
        size = builder_.AlignUp(builder_.ComputeSizeUtf8(builder_.ZExtInt32ToPtr(length)),
            builder_.IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
        builder_.Jump(&exit);
    }
    builder_.Bind(&isUtf16);
    {
        size = builder_.AlignUp(builder_.ComputeSizeUtf16(builder_.ZExtInt32ToPtr(length)),
            builder_.IntPtr(static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(),
                                                 glue, ConstantIndex::LINE_STRING_CLASS_INDEX);
    builder_.StartAllocate();
    GateRef lineString =
        builder_.HeapAlloc(glue, *size, GateType::TaggedValue(), RegionSpaceFlag::IN_SHARED_OLD_SPACE);
    builder_.Store(VariableType::JS_POINTER(), glue, lineString,
                   builder_.IntPtr(0), stringClass, MemoryAttribute::NeedBarrierAndAtomic());
    InitStringLengthAndFlags(glue, lineString, length, canBeCompressed);
    builder_.Store(VariableType::INT32(), glue, lineString,
                   builder_.IntPtr(BaseString::RAW_HASHCODE_OFFSET), builder_.Int32(0));
    auto ret = builder_.FinishAllocate(lineString);
    builder_.SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::AllocateSlicedString(GateRef glue, GateRef flatString, GateRef length,
                                                        GateRef canBeCompressed)
{
    auto env = GetEnvironment();
    auto &builder_ = *env->GetBuilder();
    Label subentry(&builder_);
    builder_.SubCfgEntry(&subentry);

    GateRef stringClass = GetGlobalConstantValue(VariableType::JS_POINTER(),
                                                 glue, ConstantIndex::SLICED_STRING_CLASS_INDEX);
    GateRef size = builder_.IntPtr(panda::ecmascript::AlignUp(SlicedString::SIZE,
                                                              static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)));

    builder_.StartAllocate();
    GateRef slicedString = builder_.HeapAlloc(glue, size, GateType::TaggedValue(),
        RegionSpaceFlag::IN_SHARED_OLD_SPACE);
    builder_.Store(VariableType::JS_POINTER(), glue, slicedString,
                   builder_.IntPtr(0), stringClass, MemoryAttribute::NeedBarrierAndAtomic());
    InitStringLengthAndFlags(glue, slicedString, length, canBeCompressed);
    builder_.Store(VariableType::INT32(), glue, slicedString,
                   builder_.IntPtr(BaseString::RAW_HASHCODE_OFFSET), builder_.Int32(0));
    builder_.Store(VariableType::JS_POINTER(), glue, slicedString,
                   builder_.IntPtr(SlicedString::PARENT_OFFSET), flatString);
    StoreStartIndexAndBackingStore(glue, slicedString, builder_.Int32(0), builder_.Boolean(true));
    auto ret = builder_.FinishAllocate(slicedString);
    builder_.SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::IsSpecialSlicedString(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    auto &builder_ = *env->GetBuilder();
    GateRef objectType = GetObjectType(LoadHClass(glue, obj));
    GateRef isSlicedString = Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::SLICED_STRING)));
    Label entry(env);
    builder_.SubCfgEntry(&entry);
    Label exit(env);
    DEFVALUE(result, env, VariableType::BOOL(), builder_.False());
    Label isSlicedStr(env);
    BRANCH_CIR(isSlicedString, &isSlicedStr, &exit);
    builder_.Bind(&isSlicedStr);
    {
        result = LoadHasBackingStore(obj);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto ret = *result;
    builder_.SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::IsFirstConcatInStringAdd(GateRef init, GateRef status)
{
    auto env = GetEnvironment();
    auto judgeStatus = LogicOrBuilder(env)
                       .Or(Int32Equal(status, Int32(BaseString::BEGIN_STRING_ADD)))
                       .Or(Int32Equal(status, Int32(BaseString::IN_STRING_ADD)))
                       .Done();
    return LogicAndBuilder(env).And(init).And(judgeStatus).Done();
}

GateRef BuiltinsStringStubBuilder::ConcatIsInStringAdd(GateRef init, GateRef status)
{
    auto env = GetEnvironment();
    auto judgeStatus = LogicOrBuilder(env)
                       .Or(Int32Equal(status, Int32(BaseString::BEGIN_STRING_ADD)))
                       .Or(Int32Equal(status, Int32(BaseString::IN_STRING_ADD)))
                       .Or(Int32Equal(status, Int32(BaseString::CONFIRMED_IN_STRING_ADD)))
                       .Done();
    return LogicAndBuilder(env).And(init).And(judgeStatus).Done();
}

GateRef BuiltinsStringStubBuilder::StringConcat(GateRef glue, GateRef leftString, GateRef rightString)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());
    Label exit(env);
    Label equalZero(env);
    Label notEqualZero(env);
    Label lessThanMax(env);
    Label throwError(env);

    GateRef leftLength = GetLengthFromString(leftString);
    GateRef rightLength = GetLengthFromString(rightString);
    GateRef newLength = Int32Add(leftLength, rightLength);
    BRANCH(Int32UnsignedGreaterThanOrEqual(newLength, Int32(BaseString::MAX_STRING_LENGTH)), &throwError, &lessThanMax);
    Bind(&throwError);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(InvalidStringLength));
        CallRuntime(glue, RTSTUB_ID(ThrowRangeError), { IntToTaggedInt(taggedId) });
        result = Exception();
        Jump(&exit);
    }
    Bind(&lessThanMax);
    BRANCH(Int32Equal(newLength, Int32(0)), &equalZero, &notEqualZero);
    Bind(&equalZero);
    {
        result = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
        Jump(&exit);
    }
    Bind(&notEqualZero);
    {
        Label leftEqualZero(env);
        Label leftNotEqualZero(env);
        Label rightEqualZero(env);
        Label rightNotEqualZero(env);
        Label newLineString(env);
        Label newTreeString(env);
        BRANCH(Int32Equal(leftLength, Int32(0)), &leftEqualZero, &leftNotEqualZero);
        Bind(&leftEqualZero);
        {
            result = rightString;
            Jump(&exit);
        }
        Bind(&leftNotEqualZero);
        BRANCH(Int32Equal(rightLength, Int32(0)), &rightEqualZero, &rightNotEqualZero);
        Bind(&rightEqualZero);
        {
            result = leftString;
            Jump(&exit);
        }
        Bind(&rightNotEqualZero);
        {
            GateRef leftIsUtf8 = IsUtf8String(leftString);
            GateRef rightIsUtf8 = IsUtf8String(rightString);
            GateRef canBeCompressed = BitAnd(leftIsUtf8, rightIsUtf8);
            NewObjectStubBuilder newBuilder(this);
            newBuilder.SetParameters(glue, 0);
            GateRef isTreeOrSlicedString = Int32UnsignedLessThan(newLength,
                Int32(std::min(TreeString::MIN_TREE_STRING_LENGTH,
                               SlicedString::MIN_SLICED_STRING_LENGTH)));
            BRANCH(isTreeOrSlicedString, &newLineString, &newTreeString);
            Bind(&newLineString);
            {
                Label isUtf8(env);
                Label isUtf16(env);
                Label isUtf8Next(env);
                Label isUtf16Next(env);
                BRANCH(canBeCompressed, &isUtf8, &isUtf16);
                Bind(&isUtf8);
                {
                    newBuilder.AllocLineStringObject(&result, &isUtf8Next, newLength, true);
                }
                Bind(&isUtf16);
                {
                    newBuilder.AllocLineStringObject(&result, &isUtf16Next, newLength, false);
                }
                Bind(&isUtf8Next);
                {
                    GateRef leftSource = ChangeStringTaggedPointerToInt64(
                        PtrAdd(leftString, IntPtr(LineString::DATA_OFFSET)));
                    GateRef rightSource = ChangeStringTaggedPointerToInt64(
                        PtrAdd(rightString, IntPtr(LineString::DATA_OFFSET)));
                    GateRef leftDst = ChangeStringTaggedPointerToInt64(
                        PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
                    GateRef rightDst = ChangeStringTaggedPointerToInt64(PtrAdd(leftDst, ZExtInt32ToPtr(leftLength)));
                    CopyChars(glue, leftDst, leftSource, leftLength, IntPtr(sizeof(uint8_t)), VariableType::INT8());
                    CopyChars(glue, rightDst, rightSource, rightLength, IntPtr(sizeof(uint8_t)), VariableType::INT8());
                    Jump(&exit);
                }
                Bind(&isUtf16Next);
                {
                    Label leftIsUtf8L(env);
                    Label leftIsUtf16L(env);
                    Label rightIsUtf8L(env);
                    Label rightIsUtf16L(env);
                    GateRef leftSource = ChangeStringTaggedPointerToInt64(
                        PtrAdd(leftString, IntPtr(LineString::DATA_OFFSET)));
                    GateRef rightSource = ChangeStringTaggedPointerToInt64(
                        PtrAdd(rightString, IntPtr(LineString::DATA_OFFSET)));
                    GateRef leftDst = ChangeStringTaggedPointerToInt64(
                        PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
                    GateRef rightDst = ChangeStringTaggedPointerToInt64(
                        PtrAdd(leftDst, PtrMul(ZExtInt32ToPtr(leftLength), IntPtr(sizeof(uint16_t)))));
                    BRANCH(leftIsUtf8, &leftIsUtf8L, &leftIsUtf16L);
                    Bind(&leftIsUtf8L);
                    {
                        // left is utf8,right string must utf16
                        CopyUtf8AsUtf16(glue, leftDst, leftSource, leftLength);
                        CopyChars(glue, rightDst, rightSource, rightLength,
                            IntPtr(sizeof(uint16_t)), VariableType::INT16());
                        Jump(&exit);
                    }
                    Bind(&leftIsUtf16L);
                    {
                        CopyChars(glue, leftDst, leftSource, leftLength,
                            IntPtr(sizeof(uint16_t)), VariableType::INT16());
                        BRANCH(rightIsUtf8, &rightIsUtf8L, &rightIsUtf16L);
                        Bind(&rightIsUtf8L);
                        CopyUtf8AsUtf16(glue, rightDst, rightSource, rightLength);
                        Jump(&exit);
                        Bind(&rightIsUtf16L);
                        CopyChars(glue, rightDst, rightSource, rightLength,
                            IntPtr(sizeof(uint16_t)), VariableType::INT16());
                        Jump(&exit);
                    }
                }
            }
            Bind(&newTreeString);
            {
                Label isUtf8(env);
                Label isUtf16(env);
                BRANCH(canBeCompressed, &isUtf8, &isUtf16);
                Bind(&isUtf8);
                {
                    newBuilder.AllocTreeStringObject(&result, &exit, leftString, rightString, newLength, true);
                }
                Bind(&isUtf16);
                {
                    newBuilder.AllocTreeStringObject(&result, &exit, leftString, rightString, newLength, false);
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsStringStubBuilder::LocaleCompare(GateRef glue, GateRef thisValue, GateRef numArgs,
                                              [[maybe_unused]] Variable *res, [[maybe_unused]] Label *exit,
                                              Label *slowPath)
{
    auto env = GetEnvironment();

    Label thisIsHeapObj(env);
    BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
    Bind(&thisIsHeapObj);
    {
        Label thisValueIsString(env);
        Label fristArgIsString(env);
        Label arg0IsHeapObj(env);
        BRANCH(IsString(glue, thisValue), &thisValueIsString, slowPath);
        Bind(&thisValueIsString);
        GateRef arg0 = GetCallArg0(numArgs);
        BRANCH(TaggedIsHeapObject(arg0), &arg0IsHeapObj, slowPath);
        Bind(&arg0IsHeapObj);
        BRANCH(IsString(glue, arg0), &fristArgIsString, slowPath);
        Bind(&fristArgIsString);
#ifdef ARK_SUPPORT_INTL
        GateRef locales = GetCallArg1(numArgs);

        GateRef options = GetCallArg2(numArgs);
        GateRef localesIsUndefOrString =
            LogicOrBuilder(env).Or(TaggedIsUndefined(locales)).Or(TaggedIsString(glue, locales)).Done();
        GateRef cacheable = LogicAndBuilder(env).And(localesIsUndefOrString).And(TaggedIsUndefined(options)).Done();
        Label optionsIsString(env);
        Label cacheAble(env);
        Label uncacheable(env);

        BRANCH(cacheable, &cacheAble, &uncacheable);
        Bind(&cacheAble);
        {
            Label defvalue(env);
            GateRef resValue = CallRuntime(glue, RTSTUB_ID(LocaleCompareCacheable), {locales, thisValue, arg0});
            BRANCH(TaggedIsUndefined(resValue), &uncacheable, &defvalue);
            Bind(&defvalue);
            *res = resValue;
            Jump(exit);
        }
        Bind(&uncacheable);
        {
            res->WriteVariable(CallRuntime(glue, RTSTUB_ID(LocaleCompareWithGc), {locales, thisValue, arg0, options}));
            Jump(exit);
        }
#else
    Jump(slowPath);
#endif
    }
}

void BuiltinsStringStubBuilder::GetStringIterator(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
                                                  Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label thisIsHeapObj(env);
    BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
    Bind(&thisIsHeapObj);
    {
        Label thisValueIsString(env);
        BRANCH(IsString(glue, thisValue), &thisValueIsString, slowPath);
        Bind(&thisValueIsString);
        GateRef globalEnv = GetCurrentGlobalEnv();
        GateRef strIterClass = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                 GlobalEnv::STRING_ITERATOR_CLASS_INDEX);
        Label afterNew(env);
        NewObjectStubBuilder newBuilder(this);
        newBuilder.SetParameters(glue, 0);
        newBuilder.NewJSObject(&result, &afterNew, strIterClass);
        Bind(&afterNew);
        Store(VariableType::JS_POINTER(), glue, *result, IntPtr(JSStringIterator::ITERATED_STRING_OFFSET), thisValue);
        Store(VariableType::INT32(), glue, *result, IntPtr(JSStringIterator::STRING_ITERATOR_NEXT_INDEX_OFFSET),
              Int32(0));
        res->WriteVariable(*result);
        Jump(exit);
    }
}

void BuiltinsStringStubBuilder::StringIteratorNext(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
                                                   Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label thisIsHeapObj(env);
    Label thisIsStringIterator(env);
    Label strNotUndefined(env);
    Label strIsHeapObj(env);
    Label strIsString(env);
    Label iterDone(env);
    BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
    Bind(&thisIsHeapObj);
    BRANCH(TaggedIsStringIterator(glue, thisValue), &thisIsStringIterator, slowPath);
    Bind(&thisIsStringIterator);
    GateRef str = Load(VariableType::JS_POINTER(), glue, thisValue, IntPtr(JSStringIterator::ITERATED_STRING_OFFSET));
    BRANCH(TaggedIsUndefined(str), &iterDone, &strNotUndefined);
    Bind(&strNotUndefined);
    BRANCH(TaggedIsHeapObject(str), &strIsHeapObj, slowPath);
    Bind(&strIsHeapObj);
    BRANCH(TaggedIsString(glue, str), &strIsString, slowPath);
    Bind(&strIsString);
    {
        Label getFirst(env);
        Label afterFlat(env);
        Label getStringFromSingleCharTable(env);
        GateRef position = LoadPrimitive(VariableType::INT32(), thisValue,
            IntPtr(JSStringIterator::STRING_ITERATOR_NEXT_INDEX_OFFSET));
        GateRef len = GetLengthFromString(str);
        BRANCH(Int32GreaterThanOrEqual(position, len), &iterDone, &getFirst);
        Bind(&getFirst);
        FlatStringStubBuilder strFlat(this);
        strFlat.FlattenString(glue, str, &afterFlat);
        Bind(&afterFlat);
        StringInfoGateRef strInfo(&strFlat);
        GateRef first = StringAt(glue, strInfo, position);
        GateRef canStoreAsUtf8 = IsASCIICharacter(first);
        BRANCH(canStoreAsUtf8, &getStringFromSingleCharTable, slowPath);
        Bind(&getStringFromSingleCharTable);
        GateRef singleCharTable = GetSingleCharTable(glue);
        GateRef firstStr = GetValueFromTaggedArray(glue, singleCharTable, ZExtInt16ToInt32(first));
        Store(VariableType::INT32(), glue, thisValue, IntPtr(JSStringIterator::STRING_ITERATOR_NEXT_INDEX_OFFSET),
              Int32Add(position, Int32(1)));
        // CreateIterResultObject(firstStr, false)
        Label afterCreate(env);
        NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
        newBuilder.CreateJSIteratorResult(glue, &result, firstStr, TaggedFalse(), &afterCreate);
        Bind(&afterCreate);
        res->WriteVariable(*result);
        Jump(exit);
    }
    Bind(&iterDone);
    {
        Store(VariableType::JS_POINTER(), glue, thisValue, IntPtr(JSStringIterator::ITERATED_STRING_OFFSET),
              Undefined());
        // CreateIterResultObject(undefined, true)
        Label afterCreate(env);
        NewObjectStubBuilder newBuilder(this, GetCurrentGlobalEnv());
        newBuilder.CreateJSIteratorResult(glue, &result, Undefined(), TaggedTrue(), &afterCreate);
        Bind(&afterCreate);
        res->WriteVariable(*result);
        Jump(exit);
    }
}

GateRef BuiltinsStringStubBuilder::EcmaStringTrim(GateRef glue, GateRef thisValue, GateRef trimMode)
{
    auto env = GetEnvironment();

    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label emptyString(env);
    Label notEmpty(env);
    Label exit(env);

    GateRef srcLen = GetLengthFromString(thisValue);
    BRANCH(Int32Equal(srcLen, Int32(0)), &emptyString, &notEmpty);
    Bind(&emptyString);
    {
        result = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX);
        Jump(&exit);
    }
    Bind(&notEmpty);
    {
        Label srcFlattenFastPath(env);

        FlatStringStubBuilder srcFlat(this);
        srcFlat.FlattenString(glue, thisValue, &srcFlattenFastPath);
        Bind(&srcFlattenFastPath);
        StringInfoGateRef srcStringInfoGate(&srcFlat);
        result = EcmaStringTrimBody(glue, thisValue, srcStringInfoGate, trimMode, IsUtf8String(thisValue));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::EcmaStringTrimBody(GateRef glue, GateRef thisValue,
    StringInfoGateRef srcStringInfoGate, GateRef trimMode, GateRef isUtf8)
{
    auto env = GetEnvironment();

    Label entry(env);
    env->SubCfgEntry(&entry);

    GateRef srcLen = srcStringInfoGate.GetLength();
    GateRef srcString = srcStringInfoGate.GetString();
    GateRef startIndex = srcStringInfoGate.GetStartIndex();

    DEFVARIABLE(start, VariableType::INT32(), Int32(0));
    DEFVARIABLE(end, VariableType::INT32(), Int32Sub(srcLen, Int32(1)));

    Label trimOrTrimStart(env);
    Label notTrimStart(env);
    Label next(env);

    BRANCH(Int32GreaterThanOrEqual(trimMode, Int32(0)), &trimOrTrimStart, &notTrimStart);
    Bind(&trimOrTrimStart); // mode = TrimMode::TRIM or TrimMode::TRIM_START
    {
        start = CallNGCRuntime(glue, RTSTUB_ID(StringGetStart), {isUtf8, srcString, srcLen, startIndex});
        Jump(&notTrimStart);
    }
    Bind(&notTrimStart);
    {
        Label trimOrTrimEnd(env);
        BRANCH(Int32LessThanOrEqual(trimMode, Int32(0)), &trimOrTrimEnd, &next);
        Bind(&trimOrTrimEnd); // mode = TrimMode::TRIM or TrimMode::TRIM_END
        {
            end = CallNGCRuntime(glue, RTSTUB_ID(StringGetEnd), {isUtf8, srcString, *start, srcLen, startIndex});
            Jump(&next);
        }
    }
    Bind(&next);
    {
        auto ret = FastSubString(glue, thisValue, *start,
                                 Int32Add(Int32Sub(*end, *start), Int32(1)), srcStringInfoGate);
        env->SubCfgExit();
        return ret;
    }
}

GateRef BuiltinsStringStubBuilder::IsSubStringAt(GateRef lhsData, bool lhsIsUtf8, GateRef rhsData,
                                                 bool rhsIsUtf8, GateRef pos, GateRef rhsCount)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);

    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedTrue());

    Label exit(env);
    Label next(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label notEqual(env);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    BRANCH(Int32LessThan(*i, rhsCount), &next, &exit);
    Bind(&next);
    {
        GateRef lhsTemp;
        GateRef rhsTemp;
        if (lhsIsUtf8) {
            lhsTemp = GetUtf8Data(lhsData, Int32Add(*i, pos));
        } else {
            lhsTemp = GetUtf16Data(lhsData, Int32Add(*i, pos));
        }
        if (rhsIsUtf8) {
            rhsTemp = GetUtf8Data(rhsData, *i);
        } else {
            rhsTemp = GetUtf16Data(rhsData, *i);
        }
        BRANCH(Int32Equal(lhsTemp, rhsTemp), &loopEnd, &notEqual);
        Bind(&notEqual);
        {
            result = TaggedFalse();
            Jump(&exit);
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

GateRef BuiltinsStringStubBuilder::IsSubStringAt(GateRef glue, const StringInfoGateRef &lStringInfoGate,
                                                 const StringInfoGateRef &rStringInfoGate, GateRef pos)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label rhsIsUtf8(env);
    Label rhsIsUtf16(env);

    DEFVARIABLE(result, VariableType::JS_ANY(), TaggedFalse());
    GateRef rhsCount = rStringInfoGate.GetLength();
    GateRef rhsData = GetNormalStringData(glue, rStringInfoGate);
    GateRef lhsData = GetNormalStringData(glue, lStringInfoGate);
    BRANCH(IsUtf8String(rStringInfoGate.GetString()), &rhsIsUtf8, &rhsIsUtf16);
    Bind(&rhsIsUtf8);
    {
        Label lhsIsUtf8(env);
        Label lhsIsUtf16(env);
        BRANCH(IsUtf8String(lStringInfoGate.GetString()), &lhsIsUtf8, &lhsIsUtf16);
        Bind(&lhsIsUtf8);
        {
            result = IsSubStringAt(lhsData, true, rhsData, true, pos, rhsCount);
            Jump(&exit);
        }
        Bind(&lhsIsUtf16);
        {
            result = IsSubStringAt(lhsData, false, rhsData, true, pos, rhsCount);
            Jump(&exit);
        }
    }
    Bind(&rhsIsUtf16);
    {
        Label lhsIsUtf8(env);
        Label lhsIsUtf16(env);
        BRANCH(IsUtf8String(lStringInfoGate.GetString()), &lhsIsUtf8, &lhsIsUtf16);
        Bind(&lhsIsUtf8);
        {
            result = IsSubStringAt(lhsData, true, rhsData, false, pos, rhsCount);
            Jump(&exit);
        }
        Bind(&lhsIsUtf16);
        {
            result = IsSubStringAt(lhsData, false, rhsData, false, pos, rhsCount);
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsStringStubBuilder::StartsWith(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(pos, VariableType::INT32(), Int32(0));

    Label objNotUndefinedAndNull(env);
    Label thisIsHeapobject(env);
    Label isString(env);
    Label searchTagIsHeapObject(env);
    Label isSearchString(env);
    Label next(env);
    Label posTagNotUndefined(env);
    Label posTagIsInt(env);
    Label posTagNotInt(env);
    Label posTagIsDouble(env);
    Label posTagIsPositiveInfinity(env);
    Label posTagNotPositiveInfinity(env);

    Label posNotLessThanLen(env);
    Label flattenFastPath(env);
    Label flattenFastPath1(env);
    Label resPosEqualPos(env);
    Label resPosNotEqualPos(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            GateRef searchTag = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(searchTag), &searchTagIsHeapObject, slowPath);
            Bind(&searchTagIsHeapObject);
            BRANCH(IsString(glue, searchTag), &isSearchString, slowPath);
            Bind(&isSearchString);
            {
                GateRef thisLen = GetLengthFromString(thisValue);
                GateRef searchLen = GetLengthFromString(searchTag);
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &next, &posTagNotUndefined);
                Bind(&posTagNotUndefined);
                {
                    GateRef posTag = GetCallArg1(numArgs);
                    BRANCH(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                    Bind(&posTagIsInt);
                    pos = GetInt32OfTInt(posTag);
                    Jump(&next);
                    Bind(&posTagNotInt);
                    BRANCH(TaggedIsDouble(posTag), &posTagIsDouble, slowPath);
                    Bind(&posTagIsDouble);
                    BRANCH(DoubleEqual(GetDoubleOfTDouble(posTag), Double(builtins::BuiltinsNumber::POSITIVE_INFINITY)),
                        &posTagIsPositiveInfinity, &posTagNotPositiveInfinity);
                    Bind(&posTagIsPositiveInfinity);
                    pos = thisLen;
                    Jump(&next);
                    Bind(&posTagNotPositiveInfinity);
                    pos = DoubleToInt(glue, GetDoubleOfTDouble(posTag));
                    Jump(&next);
                }
                Bind(&next);
                {
                    Label posGreaterThanZero(env);
                    Label posNotGreaterThanZero(env);
                    Label nextCount(env);
                    BRANCH(Int32GreaterThan(*pos, Int32(0)), &posGreaterThanZero, &posNotGreaterThanZero);
                    Bind(&posNotGreaterThanZero);
                    {
                        pos = Int32(0);
                        Jump(&nextCount);
                    }
                    Bind(&posGreaterThanZero);
                    {
                        BRANCH(Int32LessThanOrEqual(*pos, thisLen), &nextCount, &posNotLessThanLen);
                        Bind(&posNotLessThanLen);
                        {
                            pos = thisLen;
                            Jump(&nextCount);
                        }
                    }
                    Bind(&nextCount);
                    {
                        Label notGreaterThanThisLen(env);
                        Label greaterThanThisLen(env);

                        GateRef posAddSearchLen = Int32Add(*pos, searchLen);
                        BRANCH(Int32GreaterThan(posAddSearchLen, thisLen), &greaterThanThisLen, &notGreaterThanThisLen);
                        Bind(&greaterThanThisLen);
                        {
                            res->WriteVariable(TaggedFalse());
                            Jump(exit);
                        }
                        Bind(&notGreaterThanThisLen);
                        FlatStringStubBuilder thisFlat(this);
                        thisFlat.FlattenString(glue, thisValue, &flattenFastPath);
                        Bind(&flattenFastPath);
                        FlatStringStubBuilder searchFlat(this);
                        searchFlat.FlattenString(glue, searchTag, &flattenFastPath1);
                        Bind(&flattenFastPath1);
                        {
                            StringInfoGateRef thisStringInfoGate(&thisFlat);
                            StringInfoGateRef searchStringInfoGate(&searchFlat);
                            GateRef result = IsSubStringAt(glue, thisStringInfoGate, searchStringInfoGate, *pos);
                            res->WriteVariable(result);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}

void BuiltinsStringStubBuilder::EndsWith(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(searchPos, VariableType::INT32(), Int32(0));
    DEFVARIABLE(startPos, VariableType::INT32(), Int32(0));
    DEFVARIABLE(endPos, VariableType::INT32(), Int32(0));
    Label thisExists(env);
    Label thisIsHeapObject(env);
    Label thisIsString(env);
    Label searchTagExists(env);
    Label searchTagIsHeapObject(env);
    Label searchTagIsString(env);
    Label posTagExists(env);
    Label posTagNotExists(env);
    Label posTagIsNumber(env);
    Label posTagIsInt(env);
    Label afterCallArg(env);
    Label endPosLessThanZero(env);
    Label endPosNotLessThanZero(env);
    Label endPosMoreThanThisLen(env);
    Label endPosNotMoreThanThisLen(env);
    Label startPosLessThanZero(env);
    Label startPosNotLessThanZero(env);
    Label flattenFastPath1(env);
    Label flattenFastPath2(env);
    Label resultIndexEqualStartPos(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &thisExists);
    Bind(&thisExists);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObject, slowPath);
        Bind(&thisIsHeapObject);
        BRANCH(IsString(glue, thisValue), &thisIsString, slowPath);
        Bind(&thisIsString);
        BRANCH(Int64GreaterThanOrEqual(IntPtr(0), numArgs), slowPath, &searchTagExists);
        Bind(&searchTagExists);
        {
            GateRef searchTag = GetCallArg0(numArgs);
            BRANCH(TaggedIsHeapObject(searchTag), &searchTagIsHeapObject, slowPath);
            Bind(&searchTagIsHeapObject);
            BRANCH(IsString(glue, searchTag), &searchTagIsString, slowPath);
            Bind(&searchTagIsString);
            {
                GateRef thisLen = GetLengthFromString(thisValue);
                GateRef searchLen = GetLengthFromString(searchTag);
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &posTagNotExists, &posTagExists);
                Bind(&posTagExists);
                {
                    GateRef posTag = GetCallArg1(numArgs);
                    BRANCH(TaggedIsNumber(posTag), &posTagIsNumber, slowPath);
                    Bind(&posTagIsNumber);
                    BRANCH(TaggedIsInt(posTag), &posTagIsInt, slowPath);
                    Bind(&posTagIsInt);
                    {
                        searchPos = GetInt32OfTInt(posTag);
                        Jump(&afterCallArg);
                    }
                }
                Bind(&posTagNotExists);
                {
                    searchPos = thisLen;
                    Jump(&afterCallArg);
                }
                Bind(&afterCallArg);
                {
                    endPos = *searchPos;
                    BRANCH(Int32GreaterThanOrEqual(*endPos, Int32(0)), &endPosNotLessThanZero, &endPosLessThanZero);
                    Bind(&endPosLessThanZero);
                    {
                        endPos = Int32(0);
                        Jump(&endPosNotLessThanZero);
                    }
                    Bind(&endPosNotLessThanZero);
                    {
                        BRANCH(Int32LessThanOrEqual(*endPos, thisLen), &endPosNotMoreThanThisLen,
                            &endPosMoreThanThisLen);
                        Bind(&endPosMoreThanThisLen);
                        {
                            endPos = thisLen;
                            Jump(&endPosNotMoreThanThisLen);
                        }
                        Bind(&endPosNotMoreThanThisLen);
                        {
                            startPos = Int32Sub(*endPos, searchLen);
                            BRANCH(Int32LessThan(*startPos, Int32(0)), &startPosLessThanZero,
                                &startPosNotLessThanZero);
                            Bind(&startPosNotLessThanZero);
                            {
                                FlatStringStubBuilder thisFlat(this);
                                thisFlat.FlattenString(glue, thisValue, &flattenFastPath1);
                                Bind(&flattenFastPath1);
                                FlatStringStubBuilder searchFlat(this);
                                searchFlat.FlattenString(glue, searchTag, &flattenFastPath2);
                                Bind(&flattenFastPath2);
                                {
                                    StringInfoGateRef thisStringInfoGate(&thisFlat);
                                    StringInfoGateRef searchStringInfoGate(&searchFlat);
                                    GateRef result =
                                        IsSubStringAt(glue, thisStringInfoGate, searchStringInfoGate, *startPos);
                                    res->WriteVariable(result);
                                    Jump(exit);
                                }
                            }
                            Bind(&startPosLessThanZero);
                            {
                                res->WriteVariable(TaggedFalse());
                                Jump(exit);
                            }
                        }
                    }
                }
            }
        }
    }
}

void BuiltinsStringStubBuilder::TrimStart(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label objNotUndefinedAndNull(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label thisIsString(env);
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(IsString(glue, thisValue), &thisIsString, slowPath);
        Bind(&thisIsString);
        GateRef result = EcmaStringTrim(glue, thisValue, Int32(1)); // 1: mode = TrimMode::start
        res->WriteVariable(result);
        Jump(exit);
    }
}

void BuiltinsStringStubBuilder::TrimEnd(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label objNotUndefinedAndNull(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label thisIsString(env);
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(IsString(glue, thisValue), &thisIsString, slowPath);
        Bind(&thisIsString);
        GateRef result = EcmaStringTrim(glue, thisValue, Int32(-1)); // -1: mode = TrimMode::end
        res->WriteVariable(result);
        Jump(exit);
    }
}

void BuiltinsStringStubBuilder::TrimLeft(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label objNotUndefinedAndNull(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label thisIsString(env);
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(IsString(glue, thisValue), &thisIsString, slowPath);
        Bind(&thisIsString);
        GateRef result = EcmaStringTrim(glue, thisValue, Int32(1)); // 1: mode = TrimMode::start
        res->WriteVariable(result);
        Jump(exit);
    }
}

void BuiltinsStringStubBuilder::TrimRight(GateRef glue, GateRef thisValue, [[maybe_unused]] GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label objNotUndefinedAndNull(env);
    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label thisIsString(env);
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        BRANCH(IsString(glue, thisValue), &thisIsString, slowPath);
        Bind(&thisIsString);
        GateRef result = EcmaStringTrim(glue, thisValue, Int32(-1)); // -1: mode = TrimMode::end
        res->WriteVariable(result);
        Jump(exit);
    }
}

void BuiltinsStringStubBuilder::PadStart(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(tempStringLength, VariableType::INT32(), Int32(0));
    DEFVARIABLE(newStringLength, VariableType::INT32(), Int32(0));
    DEFVARIABLE(tempStr, VariableType::JS_ANY(), Undefined());

    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label isPanString(env);
    Label next(env);
    Label padTagIsHeapObject(env);
    Label padStringNotUndefined(env);
    Label lengthIsInt(env);
    Label lengthNotInt(env);
    Label lengthIsDouble(env);
    Label thisIsHeapobject(env);
    Label newLengthIsNotNaN(env);
    Label newLengthIsNotINF(env);
    Label isSelf(env);
    Label isNotSelf(env);
    Label padStringNotEmpty(env);
    Label fillLessThanPad(env);
    Label fillNotLessThanPad(env);
    Label resultString(env);
    Label newLengthInRange(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            GateRef newLength = GetCallArg0(numArgs);
            BRANCH(TaggedIsInt(newLength), &lengthIsInt, &lengthNotInt);
            Bind(&lengthIsInt);
            {
                newStringLength = GetInt32OfTInt(newLength);
                BRANCH(Int32GreaterThanOrEqual(*newStringLength, Int32(BaseString::MAX_STRING_LENGTH)),
                    slowPath, &next);
            }
            Bind(&lengthNotInt);
            {
                BRANCH(TaggedIsDouble(newLength), &lengthIsDouble, slowPath);
                Bind(&lengthIsDouble);
                BRANCH(DoubleIsNAN(GetDoubleOfTDouble(newLength)), slowPath, &newLengthIsNotNaN);
                Bind(&newLengthIsNotNaN);
                BRANCH(DoubleIsINF(GetDoubleOfTDouble(newLength)), slowPath, &newLengthIsNotINF);
                Bind(&newLengthIsNotINF);
                BRANCH(DoubleGreaterThanOrEqual(GetDoubleOfTDouble(newLength), Double(BaseString::MAX_STRING_LENGTH)),
                    slowPath, &newLengthInRange);
                Bind(&newLengthInRange);
                newStringLength = DoubleToInt(glue, GetDoubleOfTDouble(newLength));
                Jump(&next);
            }
            Bind(&next);
            GateRef thisLen = GetLengthFromString(thisValue);
            BRANCH(Int32GreaterThanOrEqual(thisLen, *newStringLength), &isSelf, &isNotSelf);
            Bind(&isSelf);
            {
                res->WriteVariable(thisValue);
                Jump(exit);
            }
            Bind(&isNotSelf);
            {
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), slowPath, &padStringNotUndefined);
                Bind(&padStringNotUndefined);
                {
                    GateRef panTag = GetCallArg1(numArgs);
                    BRANCH(TaggedIsHeapObject(panTag), &padTagIsHeapObject, slowPath);
                    Bind(&padTagIsHeapObject);
                    BRANCH(IsString(glue, panTag), &isPanString, slowPath);
                    Bind(&isPanString);
                    GateRef padStringLen = GetLengthFromString(panTag);
                    BRANCH(Int32LessThanOrEqual(padStringLen, Int32(0)), slowPath, &padStringNotEmpty);
                    Bind(&padStringNotEmpty);
                    {
                        GateRef fillStringLen = Int32Sub(*newStringLength, thisLen);
                        BRANCH(Int32LessThan(fillStringLen, padStringLen), &fillLessThanPad, &fillNotLessThanPad);
                        Bind(&fillLessThanPad);
                        {
                            tempStr = GetSubString(glue, panTag, Int32(0), fillStringLen);
                            Jump(&resultString);
                        }
                        Bind(&fillNotLessThanPad);
                        {
                            tempStr = panTag;
                            tempStringLength = Int32Add(padStringLen, padStringLen);
                            Label loopHead(env);
                            Label loopEnd(env);
                            Label loopNext(env);
                            Label loopExit(env);
                            Jump(&loopHead);

                            LoopBegin(&loopHead);
                            {
                                BRANCH(Int32GreaterThan(*tempStringLength, fillStringLen), &loopExit, &loopNext);
                                Bind(&loopNext);
                                {
                                    tempStr = StringConcat(glue, panTag, *tempStr);
                                    Jump(&loopEnd);
                                }
                            }
                            Bind(&loopEnd);
                            tempStringLength = Int32Add(*tempStringLength, padStringLen);
                            LoopEndWithCheckSafePoint(&loopHead, env, glue);
                            Bind(&loopExit);
                            GateRef lastLen = Int32Sub(padStringLen, Int32Sub(*tempStringLength, fillStringLen));
                            GateRef lastPadString = GetSubString(glue, panTag, Int32(0), lastLen);
                            tempStr = StringConcat(glue, *tempStr, lastPadString);
                            Jump(&resultString);
                        }
                        Bind(&resultString);
                        {
                            tempStr = StringConcat(glue, *tempStr, thisValue);
                            res->WriteVariable(*tempStr);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}

void BuiltinsStringStubBuilder::PadEnd(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable* res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(tempStringLength, VariableType::INT32(), Int32(0));
    DEFVARIABLE(newStringLength, VariableType::INT32(), Int32(0));
    DEFVARIABLE(tempStr, VariableType::JS_ANY(), Undefined());

    Label objNotUndefinedAndNull(env);
    Label isString(env);
    Label isPanString(env);
    Label next(env);
    Label padTagIsHeapObject(env);
    Label padStringNotUndefined(env);
    Label lengthIsInt(env);
    Label lengthNotInt(env);
    Label lengthIsDouble(env);
    Label thisIsHeapobject(env);
    Label newLenthGreatZero(env);
    Label isSelf(env);
    Label isNotSelf(env);
    Label padLengthIsNotNaN(env);
    Label padLengthIsNotINF(env);
    Label newLengthInRange(env);

    BRANCH(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        BRANCH(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        BRANCH(IsString(glue, thisValue), &isString, slowPath);
        Bind(&isString);
        {
            GateRef newLength = GetCallArg0(numArgs);
            BRANCH(TaggedIsInt(newLength), &lengthIsInt, &lengthNotInt);
            Bind(&lengthIsInt);
            {
                newStringLength = GetInt32OfTInt(newLength);
                BRANCH(Int32GreaterThanOrEqual(*newStringLength, Int32(BaseString::MAX_STRING_LENGTH)),
                    slowPath, &next);
            }
            Bind(&lengthNotInt);
            {
                BRANCH(TaggedIsDouble(newLength), &lengthIsDouble, slowPath);
                Bind(&lengthIsDouble);
                BRANCH(DoubleIsNAN(GetDoubleOfTDouble(newLength)), slowPath, &padLengthIsNotNaN);
                Bind(&padLengthIsNotNaN);
                BRANCH(DoubleIsINF(GetDoubleOfTDouble(newLength)), slowPath, &padLengthIsNotINF);
                Bind(&padLengthIsNotINF);
                BRANCH(DoubleGreaterThanOrEqual(GetDoubleOfTDouble(newLength), Double(BaseString::MAX_STRING_LENGTH)),
                    slowPath, &newLengthInRange);
                Bind(&newLengthInRange);
                newStringLength = DoubleToInt(glue, GetDoubleOfTDouble(newLength));
                Jump(&next);
            }
            Bind(&next);
            GateRef thisLen = GetLengthFromString(thisValue);
            BRANCH(Int32GreaterThanOrEqual(thisLen, *newStringLength), &isSelf, &isNotSelf);
            Bind(&isSelf);
            {
                res->WriteVariable(thisValue);
                Jump(exit);
            }
            Bind(&isNotSelf);
            {
                tempStr = thisValue;
                BRANCH(Int64GreaterThanOrEqual(IntPtr(1), numArgs), slowPath, &padStringNotUndefined);
                Bind(&padStringNotUndefined);
                {
                    GateRef panTag = GetCallArg1(numArgs);
                    BRANCH(TaggedIsHeapObject(panTag), &padTagIsHeapObject, slowPath);
                    Bind(&padTagIsHeapObject);
                    BRANCH(IsString(glue, panTag), &isPanString, slowPath);
                    Bind(&isPanString);
                    {
                        GateRef padStringLen = GetLengthFromString(panTag);
                        BRANCH(Int32GreaterThanOrEqual(Int32(0), padStringLen), slowPath, &newLenthGreatZero);
                        Bind(&newLenthGreatZero);
                        {
                            tempStringLength = Int32Add(thisLen, padStringLen);
                            Label loopHead(env);
                            Label loopEnd(env);
                            Label loopNext(env);
                            Label loopExit(env);
                            Jump(&loopHead);

                            LoopBegin(&loopHead);
                            {
                                BRANCH(Int32GreaterThan(*tempStringLength, *newStringLength), &loopExit, &loopNext);
                                Bind(&loopNext);
                                {
                                    tempStr = StringConcat(glue, *tempStr, panTag);
                                    Jump(&loopEnd);
                                }
                            }
                            Bind(&loopEnd);
                            tempStringLength = Int32Add(*tempStringLength, padStringLen);
                            LoopEndWithCheckSafePoint(&loopHead, env, glue);
                            Bind(&loopExit);
                            GateRef lastLen = Int32Sub(padStringLen, Int32Sub(*tempStringLength, *newStringLength));
                            GateRef lastPadString = GetSubString(glue, panTag, Int32(0), lastLen);
                            tempStr = StringConcat(glue, *tempStr, lastPadString);
                            res->WriteVariable(*tempStr);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}

GateRef BuiltinsStringStubBuilder::StringToUint(GateRef glue, GateRef string, uint64_t maxValue)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT64(), Int64(-1));
    Label greatThanZero(env);
    Label inRange(env);
    Label flattenFastPath(env);
    auto len = GetLengthFromString(string);
    BRANCH_UNLIKELY(Int32Equal(len, Int32(0)), &exit, &greatThanZero);
    Bind(&greatThanZero);
    BRANCH_NO_WEIGHT(Int32GreaterThan(len, Int32(MAX_ELEMENT_INDEX_LEN)), &exit, &inRange);
    Bind(&inRange);
    {
        Label isUtf8(env);
        GateRef isUtf16String = IsUtf16String(string);
        BRANCH_NO_WEIGHT(isUtf16String, &exit, &isUtf8);
        Bind(&isUtf8);
        {
            FlatStringStubBuilder thisFlat(this);
            thisFlat.FlattenString(glue, string, &flattenFastPath);
            Bind(&flattenFastPath);
            StringInfoGateRef stringInfoGate(&thisFlat);
            GateRef dataUtf8 = GetNormalStringData(glue, stringInfoGate);
            result = StringDataToUint(dataUtf8, len, maxValue);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// length should be at least 1
GateRef BuiltinsStringStubBuilder::StringDataToUint(GateRef dataUtf8, GateRef len, uint64_t maxValue)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT64(), Int64(-1));
    DEFVARIABLE(c, VariableType::INT32(), Int32(0));
    c = ZExtInt8ToInt32(LoadZeroOffsetPrimitive(VariableType::INT8(), dataUtf8));
    Label isDigitZero(env);
    Label notDigitZero(env);
    BRANCH_NO_WEIGHT(Int32Equal(*c, Int32('0')), &isDigitZero, &notDigitZero);
    Bind(&isDigitZero);
    {
        Label lengthIsOne(env);
        BRANCH_NO_WEIGHT(Int32Equal(len, Int32(1)), &lengthIsOne, &exit);
        Bind(&lengthIsOne);
        {
            result = Int64(0);
            Jump(&exit);
        }
    }
    Bind(&notDigitZero);
    {
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        DEFVARIABLE(i, VariableType::INT32(), Int32(1));
        DEFVARIABLE(n, VariableType::INT64(), Int64Sub(ZExtInt32ToInt64(*c), Int64('0')));
        BRANCH_NO_WEIGHT(IsDigit(*c), &loopHead, &exit);
        LoopBegin(&loopHead);
        {
            Label doCheck(env);
            BRANCH_LIKELY(Int32UnsignedLessThan(*i, len), &doCheck, &afterLoop);
            Bind(&doCheck);
            c = ZExtInt8ToInt32(LoadPrimitive(VariableType::INT8(), dataUtf8, ZExtInt32ToPtr(*i)));
            Label isDigit2(env);
            BRANCH_NO_WEIGHT(IsDigit(*c), &isDigit2, &exit);
            Bind(&isDigit2);
            {
                // 10 means the base of digit is 10.
                n = Int64Add(Int64Mul(*n, Int64(10)), Int64Sub(ZExtInt32ToInt64(*c), Int64('0')));
                Jump(&loopEnd);
            }
        }
        Bind(&loopEnd);
        i = Int32Add(*i, Int32(1));
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            Label notGreatThanMaxIndex(env);
            BRANCH_UNLIKELY(Int64GreaterThan(*n, Int64(maxValue)), &exit, &notGreatThanMaxIndex);
            Bind(&notGreatThanMaxIndex);
            {
                result = *n;
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
