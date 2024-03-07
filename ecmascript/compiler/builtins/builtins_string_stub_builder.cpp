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
#include "ecmascript/compiler/builtins/builtins_stubs.h"
#include "ecmascript/compiler/new_object_stub_builder.h"

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
    Branch(Int64Equal(IntPtr(0), numArgs), &lengthIsZero, &lengthNotZero);
    Bind(&lengthIsZero);
    res->WriteVariable(GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::EMPTY_STRING_OBJECT_INDEX));
    Jump(exit);
    Bind(&lengthNotZero);
    {
        Branch(Int64Equal(IntPtr(1), numArgs), &lengthIsOne, slowPath);
        Bind(&lengthIsOne);
        {
            GateRef codePointTag = GetCallArg0(numArgs);
            GateRef codePointValue = ToNumber(glue, codePointTag);
            Branch(HasPendingException(glue), &isPendingException, &noPendingException);
            Bind(&isPendingException);
            {
                res->WriteVariable(Exception());
                Jump(exit);
            }
            Bind(&noPendingException);
            {
                Branch(TaggedIsInt(codePointValue), &isInt, &notInt);
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
                Branch(IsASCIICharacter(ZExtInt16ToInt32(*value)), &canBeCompress, &canNotBeCompress);
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue, 0);
                Bind(&canBeCompress);
                {
                    GateRef singleCharTable = GetSingleCharTable(glue);
                    res->WriteVariable(GetValueFromTaggedArray(singleCharTable, ZExtInt16ToInt32(*value)));
                    Jump(exit);
                }
                Bind(&canNotBeCompress);
                {
                    Label afterNew1(env);
                    newBuilder.AllocLineStringObject(res, &afterNew1, Int32(1), false);
                    Bind(&afterNew1);
                    {
                        GateRef dst = ChangeStringTaggedPointerToInt64(
                            PtrAdd(res->ReadVariable(), IntPtr(LineEcmaString::DATA_OFFSET)));
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

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, slowPath);
        Bind(&isString);
        {
            FlatStringStubBuilder thisFlat(this);
            thisFlat.FlattenString(glue, thisValue, &flattenFastPath);
            Bind(&flattenFastPath);
            GateRef thisLen = GetLengthFromString(thisValue);
            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &posTagNotUndefined);
            Bind(&posTagNotUndefined);
            {
                GateRef posTag = GetCallArg0(numArgs);
                Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                pos = GetInt32OfTInt(posTag);
                Jump(&next);
                Bind(&posTagNotInt);
                Branch(TaggedIsDouble(posTag), &posTagIsDouble, slowPath);
                Bind(&posTagIsDouble);
                doubleValue = GetDoubleOfTDouble(posTag);
                Branch(DoubleIsINF(*doubleValue), &posGreaterLen, &isNotINF);
                Bind(&isNotINF);
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
        res->WriteVariable(IntToTaggedPtr(StringAt(stringInfoGate, *pos)));
        Jump(exit);
    }
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
        GateRef first = StringAt(stringInfoGate, *pos);
        GateRef firstIsValid = BoolOr(Int32UnsignedLessThan(first, Int32(base::utf_helper::DECODE_LEAD_LOW)),
            Int32UnsignedGreaterThan(first, Int32(base::utf_helper::DECODE_LEAD_HIGH)));
        Branch(BoolOr(firstIsValid, Int32Equal(Int32Add(*pos, Int32(1)), GetLengthFromString(thisValue))),
            &returnFirst, &getNextChar);
        Bind(&getNextChar);
        GateRef second = StringAt(stringInfoGate, Int32Add(*pos, Int32(1)));
        GateRef secondIsValid = BoolOr(Int32UnsignedLessThan(second, Int32(base::utf_helper::DECODE_TRAIL_LOW)),
            Int32UnsignedGreaterThan(second, Int32(base::utf_helper::DECODE_TRAIL_HIGH)));
        Branch(secondIsValid, &returnFirst, slowPath);
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

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, slowPath);
        Bind(&isString);
        {
            GateRef thisLen = GetLengthFromString(thisValue);
            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &posTagNotUndefined);
            Bind(&posTagNotUndefined);
            {
                GateRef posTag = GetCallArg0(numArgs);
                Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                {
                    pos->WriteVariable(GetInt32OfTInt(posTag));
                    Jump(&next);
                }
                Bind(&posTagNotInt);
                {
                    Branch(TaggedIsDouble(posTag), &posTagIsDouble, slowPath);
                    Bind(&posTagIsDouble);
                    doubleValue = GetDoubleOfTDouble(posTag);
                    Branch(DoubleIsINF(*doubleValue), exit, &isNotINF);
                    Bind(&isNotINF);
                    pos->WriteVariable(DoubleToInt(glue, GetDoubleOfTDouble(posTag)));
                    Jump(&next);
                }
            }
            Bind(&next);
            {
                Branch(Int32GreaterThanOrEqual(pos->ReadVariable(), thisLen), exit, &posNotGreaterLen);
                Bind(&posNotGreaterLen);
                {
                    Branch(Int32LessThan(pos->ReadVariable(), Int32(0)), exit, posIsValid);
                }
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

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, slowPath);
        Bind(&isString);
        {
            GateRef searchTag = GetCallArg0(numArgs);
            Branch(TaggedIsHeapObject(searchTag), &searchTagIsHeapObject, slowPath);
            Bind(&searchTagIsHeapObject);
            Branch(IsString(searchTag), &isSearchString, slowPath);
            Bind(&isSearchString);
            {
                GateRef thisLen = GetLengthFromString(thisValue);
                Branch(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &next, &posTagNotUndefined);
                Bind(&posTagNotUndefined);
                {
                    GateRef posTag = GetCallArg1(numArgs);
                    Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                    Bind(&posTagIsInt);
                    pos = GetInt32OfTInt(posTag);
                    Jump(&next);
                    Bind(&posTagNotInt);
                    Branch(TaggedIsDouble(posTag), &posTagIsDouble, slowPath);
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
                        FlatStringStubBuilder thisFlat(this);
                        thisFlat.FlattenString(glue, thisValue, &flattenFastPath);
                        Bind(&flattenFastPath);
                        FlatStringStubBuilder searchFlat(this);
                        searchFlat.FlattenString(glue, searchTag, &flattenFastPath1);
                        Bind(&flattenFastPath1);
                        StringInfoGateRef thisStringInfoGate(&thisFlat);
                        StringInfoGateRef searchStringInfoGate(&searchFlat);
                        GateRef resPos = StringIndexOf(thisStringInfoGate, searchStringInfoGate, *pos);
                        Branch(Int32GreaterThanOrEqual(resPos, Int32(0)), &resPosGreaterZero, exit);
                        Bind(&resPosGreaterZero);
                        {
                            Label resPosLessZero(env);
                            Branch(Int32LessThanOrEqual(resPos, thisLen), &resPosLessZero, exit);
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
    Label posTagIsInt(env);
    Label posTagNotInt(env);
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

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, slowPath);
        Bind(&isString);
        {
            Label next(env);
            GateRef thisLen = GetLengthFromString(thisValue);
            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &next, &startTagNotUndefined);
            Bind(&startTagNotUndefined);
            {
                GateRef startTag = GetCallArg0(numArgs);
                Branch(TaggedIsInt(startTag), &posTagIsInt, &posTagNotInt);
                Bind(&posTagIsInt);
                start = GetInt32OfTInt(startTag);
                Jump(&next);
                Bind(&posTagNotInt);
                Branch(TaggedIsDouble(startTag), &posTagIsDouble, slowPath);
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
                    GateRef endTag = GetCallArg1(numArgs);
                    Branch(TaggedIsInt(endTag), &endTagIsInt, &endTagNotInt);
                    Bind(&endTagIsInt);
                    end = GetInt32OfTInt(endTag);
                    Jump(&countStart);
                    Bind(&endTagNotInt);
                    Branch(TaggedIsDouble(endTag), &endTagIsDouble, slowPath);
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
                GateRef len = Int32Sub(*to, *from);
                res->WriteVariable(GetSubString(glue, thisValue, *from, len));
                Jump(exit);
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
        Branch(Int32Equal(len, Int32(1)), &isSingleChar, &notSingleChar);
        Bind(&isSingleChar);
        {
            StringInfoGateRef stringInfoGate1(&thisFlat);
            GateRef charCode = StringAt(stringInfoGate1, from);
            GateRef canStoreAsUtf8 = IsASCIICharacter(charCode);
            Branch(canStoreAsUtf8, &getStringFromSingleCharTable, &fastSubstring);
            Bind(&getStringFromSingleCharTable);
            {
                GateRef singleCharTable = GetSingleCharTable(glue);
                result = GetValueFromTaggedArray(singleCharTable, ZExtInt16ToInt32(charCode));
                Jump(&exit);
            }
        }
        Bind(&notSingleChar);
        Branch(Int32GreaterThanOrEqual(len, Int32(SlicedString::MIN_SLICED_ECMASTRING_LENGTH)),
            &mayGetSliceString, &fastSubstring);
        Bind(&mayGetSliceString);
        {
            Branch(IsUtf16String(thisValue), &isUtf16, &sliceString);
            Bind(&isUtf16);
            {
                StringInfoGateRef stringInfoGate(&thisFlat);
                GateRef fromOffset = PtrMul(ZExtInt32ToPtr(from), IntPtr(sizeof(uint16_t) / sizeof(uint8_t)));
                GateRef source = PtrAdd(GetNormalStringData(stringInfoGate), fromOffset);
                GateRef canBeCompressed = CanBeCompressed(source, len, true);
                Branch(canBeCompressed, &isUtf8, &sliceString);
                Bind(&isUtf8);
                {
                    NewObjectStubBuilder newBuilder(this);
                    newBuilder.SetParameters(glue, 0);
                    newBuilder.AllocLineStringObject(&result, &afterNew, len, true);
                    Bind(&afterNew);
                    {
                        GateRef source1 = PtrAdd(GetNormalStringData(stringInfoGate), fromOffset);
                        GateRef dst =
                            ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET)));
                        CopyUtf16AsUtf8(glue, dst, source1, len);
                        Jump(&exit);
                    }
                }
            }
            Bind(&sliceString);
            {
                NewObjectStubBuilder newBuilder(this);
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

void BuiltinsStringStubBuilder::Replace(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();

    Label objNotUndefinedAndNull(env);

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label tagsDefined(env);
        Label searchIsHeapObj(env);
        Label replaceIsHeapObj(env);

        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        Branch(Int64Equal(IntPtr(2), numArgs), &tagsDefined, slowPath); // 2: number of parameters. search & replace Tag
        Bind(&tagsDefined);
        {
            Label next(env);

            GateRef searchTag = GetCallArg0(numArgs);
            Branch(TaggedIsHeapObject(searchTag), &searchIsHeapObj, slowPath);
            Bind(&searchIsHeapObj);
            GateRef replaceTag = GetCallArg1(numArgs);
            Branch(TaggedIsHeapObject(replaceTag), &replaceIsHeapObj, slowPath);
            Bind(&replaceIsHeapObj);
            Branch(BoolOr(IsJSRegExp(searchTag), IsEcmaObject(searchTag)), slowPath, &next);
            Bind(&next);
            {
                Label allAreStrings(env);
                GateRef thisIsString = IsString(thisValue);
                GateRef searchIsString = IsString(searchTag);
                GateRef replaceIsString = IsString(replaceTag);
                Branch(BoolAnd(BoolAnd(thisIsString, searchIsString), replaceIsString), &allAreStrings, slowPath);
                Bind(&allAreStrings);
                {
                    Label replaceTagNotCallable(env);

                    GateRef replaceTagIsCallable = IsCallable(replaceTag);

                    Branch(replaceTagIsCallable, slowPath, &replaceTagNotCallable);
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
                        GateRef pos = StringIndexOf(thisStringInfoGate, searchStringInfoGate, Int32(-1));
                        Branch(Int32Equal(pos, Int32(-1)), &noReplace, &nextProcess);
                        Bind(&noReplace);
                        {
                            res->WriteVariable(thisValue);
                            Jump(exit);
                        }
                        Bind(&nextProcess);
                        {
                            Label functionalReplaceFalse(env);

                            Branch(replaceTagIsCallable, slowPath, &functionalReplaceFalse);
                            Bind(&functionalReplaceFalse);
                            {
                                Label replHandleIsString(env);

                                GateRef replHandle = GetSubstitution(glue, searchTag, thisValue, pos, replaceTag);
                                Branch(IsString(replHandle), &replHandleIsString, slowPath);
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

    Branch(Int32GreaterThanOrEqual(index, Int32(0)), &indexGreaterThanOrEqualZero, &indexLessThanZero);
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

        Branch(Int32LessThan(*relativeIndex, Int32(0)), &relativeIndexLessThanZero, &elseCheck);
        Bind(&relativeIndexLessThanZero);
        {
            relativeIndex = Int32(0);
            Jump(&exit);
        }
        Bind(&elseCheck);
        {
            Label relativeIndexGreaterThanLength(env);

            Branch(Int32GreaterThan(*relativeIndex, length), &relativeIndexGreaterThanLength, &exit);
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

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label isString(env);

        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        Branch(IsString(thisValue), &isString, slowPath);
        Bind(&isString);
        {
            Label startTagDefined(env);

            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), slowPath, &startTagDefined);
            Bind(&startTagDefined);
            {
                Label startTagIsInt(env);
                Label endTagUndefined(env);
                Label endTagDefined(env);
                Label endTagIsInt(env);
                Label next(env);

                GateRef startTag = GetCallArg0(numArgs);
                Branch(TaggedIsInt(startTag), &startTagIsInt, slowPath);
                Bind(&startTagIsInt);
                GateRef thisLen = GetLengthFromString(thisValue);
                start = ConvertAndClampRelativeIndex(GetInt32OfTInt(startTag), thisLen);
                Branch(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &endTagUndefined, &endTagDefined);
                Bind(&endTagUndefined);
                {
                    end = thisLen;
                    Jump(&next);
                }
                Bind(&endTagDefined);
                {
                    GateRef endTag = GetCallArg1(numArgs);
                    Branch(TaggedIsInt(endTag), &endTagIsInt, slowPath);
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
                    Branch(Int32LessThanOrEqual(*sliceLen, Int32(0)), &emptyString, &fastSubString);
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

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label thisIsString(env);

        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        Branch(IsString(thisValue), &thisIsString, slowPath);
        Bind(&thisIsString);
        GateRef result = EcmaStringTrim(glue, thisValue, Int32(0)); // 0: mode = TrimMode::TRIM
        res->WriteVariable(result);
        Jump(exit);
    }
}

GateRef BuiltinsStringStubBuilder::StringAt(const StringInfoGateRef &stringInfoGate, GateRef index)
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
    GateRef dataUtf16 = GetNormalStringData(stringInfoGate);
    Branch(IsUtf16String(stringInfoGate.GetString()), &isUtf16, &isUtf8);
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

GateRef BuiltinsStringStubBuilder::GetSingleCharCodeByIndex(GateRef str, GateRef index)
{
    // Note: This method cannot handle treestring.
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));

    Label isConstantString(env);
    Label lineStringCheck(env);
    Label isLineString(env);
    Label slicedStringCheck(env);
    Label isSlicedString(env);
    Label exit(env);

    Branch(IsConstantString(str), &isConstantString, &lineStringCheck);
    Bind(&isConstantString);
    {
        result = GetSingleCharCodeFromConstantString(str, index);
        Jump(&exit);
    }
    Bind(&lineStringCheck);
    Branch(IsLineString(str), &isLineString, &slicedStringCheck);
    Bind(&isLineString);
    {
        result = GetSingleCharCodeFromLineString(str, index);
        Jump(&exit);
    }
    Bind(&slicedStringCheck);
    Branch(IsSlicedString(str), &isSlicedString, &exit);
    Bind(&isSlicedString);
    {
        result = GetSingleCharCodeFromSlicedString(str, index);
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::GetSingleCharCodeFromConstantString(GateRef str, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    GateRef offset = ChangeStringTaggedPointerToInt64(PtrAdd(str, IntPtr(ConstantString::CONSTANT_DATA_OFFSET)));
    GateRef dataAddr = Load(VariableType::NATIVE_POINTER(), offset, IntPtr(0));
    GateRef result = ZExtInt8ToInt32(Load(VariableType::INT8(), PtrAdd(dataAddr,
        PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint8_t))))));
    env->SubCfgExit();
    return result;
}

GateRef BuiltinsStringStubBuilder::GetSingleCharCodeFromLineString(GateRef str, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef dataAddr = ChangeStringTaggedPointerToInt64(PtrAdd(str, IntPtr(LineEcmaString::DATA_OFFSET)));
    Label isUtf16(env);
    Label isUtf8(env);
    Label exit(env);
    Branch(IsUtf16String(str), &isUtf16, &isUtf8);
    Bind(&isUtf16);
    {
        result = ZExtInt16ToInt32(Load(VariableType::INT16(), PtrAdd(dataAddr,
            PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint16_t))))));
        Jump(&exit);
    }
    Bind(&isUtf8);
    {
        result = ZExtInt8ToInt32(Load(VariableType::INT8(), PtrAdd(dataAddr,
            PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(uint8_t))))));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef BuiltinsStringStubBuilder::GetSingleCharCodeFromSlicedString(GateRef str, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    Label isLineString(env);
    Label notLineString(env);
    Label exit(env);

    GateRef parent = Load(VariableType::JS_POINTER(), str, IntPtr(SlicedString::PARENT_OFFSET));
    GateRef startIndex = Load(VariableType::INT32(), str, IntPtr(SlicedString::STARTINDEX_OFFSET));
    Branch(IsLineString(parent), &isLineString, &notLineString);
    Bind(&isLineString);
    {
        result = GetSingleCharCodeFromLineString(parent, Int32Add(startIndex, index));
        Jump(&exit);
    }
    Bind(&notLineString);
    {
        result = GetSingleCharCodeFromConstantString(parent, Int32Add(startIndex, index));
        Jump(&exit);
    }
    Bind(&exit);
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
    Branch(canStoreAsUtf8, &utf8, &utf16);
    Bind(&utf8);
    {
        newBuilder.AllocLineStringObject(&result, &afterNew, Int32(1), true);
    }
    Bind(&utf16);
    {
        newBuilder.AllocLineStringObject(&result, &afterNew, Int32(1), false);
    }
    Bind(&afterNew);
    {
        Label isUtf8Copy(env);
        Label isUtf16Copy(env);
        GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET)));
        Branch(canStoreAsUtf8, &isUtf8Copy, &isUtf16Copy);
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
    GateRef dataUtf = GetNormalStringData(stringInfoGate);
    Branch(IsUtf16String(stringInfoGate.GetString()), &isUtf16, &isUtf8);
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
            GateRef singleCharTable = GetSingleCharTable(glue);
            result = GetValueFromTaggedArray(singleCharTable, ZExtInt16ToInt32(*data));
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
            GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET)));
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
            GateRef thisLen = stringInfoGate.GetLength();
            Branch(Int32Equal(len, thisLen), &exit, &next);
        }
        Bind(&next);
        {
            Branch(IsUtf8String(thisValue), &isUtf8, &isUtf16);
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
        GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET)));
        GateRef source = PtrAdd(GetNormalStringData(stringInfoGate), ZExtInt32ToPtr(from));
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
    GateRef source = PtrAdd(GetNormalStringData(stringInfoGate), fromOffset);
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
        GateRef source1 = PtrAdd(GetNormalStringData(stringInfoGate), fromOffset);
        GateRef dst = ChangeStringTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET)));
        Branch(canBeCompressed, &isUtf8Next, &isUtf16Next);
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
    GateRef nextDollarIndex = StringIndexOf(replaceStringInfoGate, dollarStringInfoGate, Int32(-1));
    Branch(Int32LessThan(nextDollarIndex, Int32(0)), &notFound, &slowPath);
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
    DEFVARIABLE(dstTmp, VariableType::NATIVE_POINTER(), dst);
    DEFVARIABLE(sourceTmp, VariableType::NATIVE_POINTER(), source);
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

// source is utf8, dst is utf16
void BuiltinsStringStubBuilder::CopyUtf8AsUtf16(GateRef glue, GateRef dst, GateRef src,
    GateRef sourceLength)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
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
        Branch(Int32GreaterThan(*len, Int32(0)), &next, &exit);
        Bind(&next);
        {
            len = Int32Sub(*len, Int32(1));
            GateRef i = Load(VariableType::INT8(), *sourceTmp);
            Store(VariableType::INT16(), glue, *dstTmp, IntPtr(0), ZExtInt8ToInt16(i));
            Jump(&loopEnd);
        }
    }

    Bind(&loopEnd);
    sourceTmp = PtrAdd(*sourceTmp, IntPtr(sizeof(uint8_t)));
    dstTmp = PtrAdd(*dstTmp, IntPtr(sizeof(uint16_t)));
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
    return Int32UnsignedLessThan(Int32Sub(data, Int32(1)), Int32(base::utf_helper::UTF8_1B_MAX));
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


void BuiltinsStringStubBuilder::StoreParent(GateRef glue, GateRef object, GateRef parent)
{
    Store(VariableType::JS_POINTER(), glue, object, IntPtr(SlicedString::PARENT_OFFSET), parent);
}

void BuiltinsStringStubBuilder::StoreStartIndex(GateRef glue, GateRef object, GateRef startIndex)
{
    Store(VariableType::INT32(), glue, object, IntPtr(SlicedString::STARTINDEX_OFFSET), startIndex);
}

void BuiltinsStringStubBuilder::StoreHasBackingStore(GateRef glue, GateRef object, GateRef hasBackingStore)
{
    Store(VariableType::INT32(), glue, object, IntPtr(SlicedString::BACKING_STORE_FLAG), hasBackingStore);
}

GateRef BuiltinsStringStubBuilder::StringIndexOf(const StringInfoGateRef &lStringInfoGate,
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
                    GateRef rhsData = GetNormalStringData(rStringInfoGate);
                    GateRef lhsData = GetNormalStringData(lStringInfoGate);
                    Branch(IsUtf8String(rStringInfoGate.GetString()), &rhsIsUtf8, &rhsIsUtf16);
                    Bind(&rhsIsUtf8);
                    {
                        Label lhsIsUtf8(env);
                        Label lhsIsUtf16(env);
                        Branch(IsUtf8String(lStringInfoGate.GetString()), &lhsIsUtf8, &lhsIsUtf16);
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
                        Branch(IsUtf8String(lStringInfoGate.GetString()), &lhsIsUtf8, &lhsIsUtf16);
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
    Branch(BoolOr(IsLineString(str), IsConstantString(str)), &exit, &notLineString);
    Bind(&notLineString);
    {
        Label isTreeString(env);
        Label notTreeString(env);
        Label isSlicedString(env);
        Branch(IsTreeString(str), &isTreeString, &notTreeString);
        Bind(&isTreeString);
        {
            Label isFlat(env);
            Label notFlat(env);
            Branch(TreeStringIsFlat(str), &isFlat, &notFlat);
            Bind(&isFlat);
            {
                flatString_.WriteVariable(GetFirstFromTreeString(str));
                Jump(fastPath);
            }
            Bind(&notFlat);
            {
                flatString_.WriteVariable(CallRuntime(glue, RTSTUB_ID(SlowFlattenString), { str }));
                Jump(fastPath);
            }
        }
        Bind(&notTreeString);
        Branch(IsSlicedString(str), &isSlicedString, &exit);
        Bind(&isSlicedString);
        {
            flatString_.WriteVariable(GetParentFromSlicedString(str));
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

GateRef BuiltinsStringStubBuilder::GetStringDataFromLineOrConstantString(GateRef str)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isConstantString(env);
    Label isLineString(env);
    DEFVARIABLE(result, VariableType::NATIVE_POINTER(), IntPtr(0));
    Branch(IsConstantString(str), &isConstantString, &isLineString);
    Bind(&isConstantString);
    {
        GateRef address = ChangeStringTaggedPointerToInt64(PtrAdd(str, IntPtr(ConstantString::CONSTANT_DATA_OFFSET)));
        result = Load(VariableType::NATIVE_POINTER(), address, IntPtr(0));
        Jump(&exit);
    }
    Bind(&isLineString);
    {
        result = ChangeStringTaggedPointerToInt64(PtrAdd(str, IntPtr(LineEcmaString::DATA_OFFSET)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void BuiltinsStringStubBuilder::Concat(GateRef glue, GateRef thisValue, GateRef numArgs,
    Variable *res, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    DEFVARIABLE(result, VariableType::JS_POINTER(), Undefined());

    Label objNotUndefinedAndNull(env);

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Label thisIsHeapObj(env);
        Label isString(env);

        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
        Bind(&thisIsHeapObj);
        Branch(IsString(thisValue), &isString, slowPath);
        Bind(&isString);
        {
            Label noPara(env);
            Label hasPara(env);
            Branch(Int64GreaterThanOrEqual(IntPtr(0), numArgs), &noPara, &hasPara);
            Bind(&noPara);
            {
                res->WriteVariable(thisValue);
                Jump(exit);
            }
            Bind(&hasPara);
            {
                Label argc1(env);
                Label notArgc1(env);
                Label argc2(env);
                Label notArgc2(env);
                Label argc3(env);
                Label notArgc3(env);
                Label arg0IsValid(env);
                Label arg1IsValid(env);
                Label arg2IsValid(env);
                Label next(env);
                Label next1(env);
                GateRef arg0 = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG0_OR_ARGV));
                Branch(TaggedIsString(arg0), &arg0IsValid, slowPath);
                Bind(&arg0IsValid);
                {
                    Branch(Int64Equal(IntPtr(1), numArgs), &argc1, &notArgc1);
                    Bind(&argc1);
                    {
                        res->WriteVariable(StringConcat(glue, thisValue, arg0));
                        Jump(exit);
                    }
                    Bind(&notArgc1);
                    {
                        result = StringConcat(glue, thisValue, arg0);
                        Branch(TaggedIsException(*result), slowPath, &next);
                        Bind(&next);
                        GateRef arg1 = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG1));
                        Branch(TaggedIsString(arg1), &arg1IsValid, slowPath);
                        Bind(&arg1IsValid);
                        Branch(Int64Equal(IntPtr(2), numArgs), &argc2, &notArgc2); // 2: number of parameters.
                        Bind(&argc2);
                        {
                            res->WriteVariable(StringConcat(glue, *result, arg1));
                            Jump(exit);
                        }
                        Bind(&notArgc2);
                        result = StringConcat(glue, *result, arg1);
                        Branch(TaggedIsException(*result), slowPath, &next1);
                        Bind(&next1);
                        GateRef arg2 = TaggedArgument(static_cast<size_t>(BuiltinsArgs::ARG2));
                        Branch(TaggedIsString(arg2), &arg2IsValid, slowPath);
                        Bind(&arg2IsValid);
                        Branch(Int64Equal(IntPtr(3), numArgs), &argc3, slowPath); // 3: number of parameters.
                        Bind(&argc3);
                        {
                            res->WriteVariable(StringConcat(glue, *result, arg2));
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
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
    Branch(Int32GreaterThanOrEqual(newLength, Int32(EcmaString::MAX_STRING_LENGTH)), &throwError, &lessThanMax);
    Bind(&throwError);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(InvalidStringLength));
        CallRuntime(glue, RTSTUB_ID(ThrowRangeError), { IntToTaggedInt(taggedId) });
        Jump(&exit);
    }
    Bind(&lessThanMax);
    Branch(Int32Equal(newLength, Int32(0)), &equalZero, &notEqualZero);
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
        Branch(Int32Equal(leftLength, Int32(0)), &leftEqualZero, &leftNotEqualZero);
        Bind(&leftEqualZero);
        {
            result = rightString;
            Jump(&exit);
        }
        Bind(&leftNotEqualZero);
        Branch(Int32Equal(rightLength, Int32(0)), &rightEqualZero, &rightNotEqualZero);
        Bind(&rightEqualZero);
        {
            result = leftString;
            Jump(&exit);
        }
        Bind(&rightNotEqualZero);
        {
            GateRef leftIsUtf8 = IsUtf8String(leftString);
            GateRef rightIsUtf8 = IsUtf8String(rightString);
            GateRef canBeCompressed = BoolAnd(leftIsUtf8, rightIsUtf8);
            NewObjectStubBuilder newBuilder(this);
            newBuilder.SetParameters(glue, 0);
            GateRef isTreeOrSlicedString = Int32LessThan(newLength,
                Int32(std::min(TreeEcmaString::MIN_TREE_ECMASTRING_LENGTH,
                               SlicedString::MIN_SLICED_ECMASTRING_LENGTH)));
            Branch(isTreeOrSlicedString, &newLineString, &newTreeString);
            Bind(&newLineString);
            {
                Label isUtf8(env);
                Label isUtf16(env);
                Label isUtf8Next(env);
                Label isUtf16Next(env);
                Branch(canBeCompressed, &isUtf8, &isUtf16);
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
                    GateRef leftSource = GetStringDataFromLineOrConstantString(leftString);
                    GateRef rightSource = GetStringDataFromLineOrConstantString(rightString);
                    GateRef leftDst = ChangeStringTaggedPointerToInt64(
                        PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET)));
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
                    GateRef leftSource = GetStringDataFromLineOrConstantString(leftString);
                    GateRef rightSource = GetStringDataFromLineOrConstantString(rightString);
                    GateRef leftDst = ChangeStringTaggedPointerToInt64(
                        PtrAdd(*result, IntPtr(LineEcmaString::DATA_OFFSET)));
                    GateRef rightDst = ChangeStringTaggedPointerToInt64(
                        PtrAdd(leftDst, PtrMul(ZExtInt32ToPtr(leftLength), IntPtr(sizeof(uint16_t)))));
                    Branch(leftIsUtf8, &leftIsUtf8L, &leftIsUtf16L);
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
                        Branch(rightIsUtf8, &rightIsUtf8L, &rightIsUtf16L);
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
                Branch(canBeCompressed, &isUtf8, &isUtf16);
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

void BuiltinsStringStubBuilder::LocaleCompare([[maybe_unused]] GateRef glue, GateRef thisValue, GateRef numArgs,
                                              [[maybe_unused]] Variable *res, [[maybe_unused]] Label *exit,
                                              Label *slowPath)
{
    auto env = GetEnvironment();

    Label thisIsHeapObj(env);
    Branch(TaggedIsHeapObject(thisValue), &thisIsHeapObj, slowPath);
    Bind(&thisIsHeapObj);
    {
        Label thisValueIsString(env);
        Label fristArgIsString(env);
        Label arg0IsHeapObj(env);
        Branch(IsString(thisValue), &thisValueIsString, slowPath);
        Bind(&thisValueIsString);
        GateRef arg0 = GetCallArg0(numArgs);
        Branch(TaggedIsHeapObject(arg0), &arg0IsHeapObj, slowPath);
        Bind(&arg0IsHeapObj);
        Branch(IsString(arg0), &fristArgIsString, slowPath);
        Bind(&fristArgIsString);
#ifdef ARK_SUPPORT_INTL
        GateRef locales = GetCallArg1(numArgs);

        GateRef options = GetCallArg2(numArgs);
        GateRef localesIsUndef = TaggedIsUndefined(locales);
        GateRef optionsIsUndef = TaggedIsUndefined(options);
        GateRef cacheable = BoolAnd(BoolOr(localesIsUndef, TaggedIsString(locales)), optionsIsUndef);
        Label optionsIsString(env);
        Label cacheAble(env);
        Label uncacheable(env);

        Branch(cacheable, &cacheAble, &uncacheable);
        Bind(&cacheAble);
        {
            Label defvalue(env);
            GateRef resValue = CallNGCRuntime(glue, RTSTUB_ID(LocaleCompareNoGc), {glue, locales, thisValue, arg0});
            Branch(TaggedIsUndefined(resValue), slowPath, &defvalue);
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
    Branch(Int32Equal(srcLen, Int32(0)), &emptyString, &notEmpty);
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

    Branch(Int32GreaterThanOrEqual(trimMode, Int32(0)), &trimOrTrimStart, &notTrimStart);
    Bind(&trimOrTrimStart); // mode = TrimMode::TRIM or TrimMode::TRIM_START
    {
        start = CallNGCRuntime(glue, RTSTUB_ID(StringGetStart), {isUtf8, srcString, srcLen, startIndex});
        Jump(&notTrimStart);
    }
    Bind(&notTrimStart);
    {
        Label trimOrTrimEnd(env);
        Branch(Int32LessThanOrEqual(trimMode, Int32(0)), &trimOrTrimEnd, &next);
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
    Branch(Int32LessThan(*i, rhsCount), &next, &exit);
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
        Branch(Int32Equal(lhsTemp, rhsTemp), &loopEnd, &notEqual);
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

GateRef BuiltinsStringStubBuilder::IsSubStringAt(const StringInfoGateRef &lStringInfoGate,
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
    GateRef rhsData = GetNormalStringData(rStringInfoGate);
    GateRef lhsData = GetNormalStringData(lStringInfoGate);
    Branch(IsUtf8String(rStringInfoGate.GetString()), &rhsIsUtf8, &rhsIsUtf16);
    Bind(&rhsIsUtf8);
    {
        Label lhsIsUtf8(env);
        Label lhsIsUtf16(env);
        Branch(IsUtf8String(lStringInfoGate.GetString()), &lhsIsUtf8, &lhsIsUtf16);
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
        Branch(IsUtf8String(lStringInfoGate.GetString()), &lhsIsUtf8, &lhsIsUtf16);
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

    Branch(TaggedIsUndefinedOrNull(thisValue), slowPath, &objNotUndefinedAndNull);
    Bind(&objNotUndefinedAndNull);
    {
        Branch(TaggedIsHeapObject(thisValue), &thisIsHeapobject, slowPath);
        Bind(&thisIsHeapobject);
        Branch(IsString(thisValue), &isString, slowPath);
        Bind(&isString);
        {
            GateRef searchTag = GetCallArg0(numArgs);
            Branch(TaggedIsHeapObject(searchTag), &searchTagIsHeapObject, slowPath);
            Bind(&searchTagIsHeapObject);
            Branch(IsString(searchTag), &isSearchString, slowPath);
            Bind(&isSearchString);
            {
                GateRef thisLen = GetLengthFromString(thisValue);
                GateRef searchLen = GetLengthFromString(searchTag);
                Branch(Int64GreaterThanOrEqual(IntPtr(1), numArgs), &next, &posTagNotUndefined);
                Bind(&posTagNotUndefined);
                {
                    GateRef posTag = GetCallArg1(numArgs);
                    Branch(TaggedIsInt(posTag), &posTagIsInt, &posTagNotInt);
                    Bind(&posTagIsInt);
                    pos = GetInt32OfTInt(posTag);
                    Jump(&next);
                    Bind(&posTagNotInt);
                    Branch(TaggedIsDouble(posTag), &posTagIsDouble, slowPath);
                    Bind(&posTagIsDouble);
                    Branch(DoubleEqual(GetDoubleOfTDouble(posTag), Double(builtins::BuiltinsNumber::POSITIVE_INFINITY)),
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
                        Label notGreaterThanThisLen(env);
                        Label greaterThanThisLen(env);

                        GateRef posAddSearchLen = Int32Add(*pos, searchLen);
                        Branch(Int32GreaterThan(posAddSearchLen, thisLen), &greaterThanThisLen, &notGreaterThanThisLen);
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
                            GateRef result = IsSubStringAt(thisStringInfoGate, searchStringInfoGate, *pos);
                            res->WriteVariable(result);
                            Jump(exit);
                        }
                    }
                }
            }
        }
    }
}
}  // namespace panda::ecmascript::kungfu
