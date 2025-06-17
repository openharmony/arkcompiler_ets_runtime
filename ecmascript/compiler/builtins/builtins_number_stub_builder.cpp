/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/builtins/builtins_number_stub_builder.h"

#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/js_primitive_ref.h"

namespace panda::ecmascript::kungfu {
void BuiltinsNumberStubBuilder::ParseFloat(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label definedMsg(env);
    Label undefinedMsg(env);
    GateRef msg = GetCallArg0(numArgs_);
    BRANCH(TaggedIsUndefined(msg), &undefinedMsg, &definedMsg);
    Bind(&undefinedMsg);
    {
        *result = DoubleToTaggedDoublePtr(Double(base::NAN_VALUE));
        Jump(exit);
    }
    Bind(&definedMsg);
    {
        Label heapObj(env);
        Label stringObj(env);
        BRANCH(TaggedIsHeapObject(msg), &heapObj, slowPath);
        Bind(&heapObj);
        BRANCH(IsString(glue_, msg), &stringObj, slowPath);
        Bind(&stringObj);
        {
            *result = CallNGCRuntime(glue_, RTSTUB_ID(NumberHelperStringToDouble), { glue_, msg });
            Jump(exit);
        }
    }
}

void BuiltinsNumberStubBuilder::ParseInt(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label msgIsString(env);
    Label radixIsSpecial(env);
    Label radixIsSpecialInt(env);

    DEFVARIABLE(radix, VariableType::INT32(), Int32(0));
    GateRef msg = GetCallArg0(numArgs_);
    GateRef arg2 = GetCallArg1(numArgs_);
    // ToString maybe throw exception.
    Branch(TaggedIsString(glue_, msg), &msgIsString, slowPath);
    Bind(&msgIsString);
    Branch(TaggedIsUndefined(arg2), &radixIsSpecialInt, &radixIsSpecial);

    Bind(&radixIsSpecial);
    {
        Label radixIsInt(env);
        // ToInt maybe throw exception.
        Branch(TaggedIsInt(arg2), &radixIsInt, slowPath);
        Bind(&radixIsInt);
        {
            radix = GetInt32OfTInt(arg2);
            Jump(&radixIsSpecialInt);
        }
    }
    Bind(&radixIsSpecialInt);
    {
        *result = CallNGCRuntime(glue_, RTSTUB_ID(StringToNumber), { glue_, msg, *radix });
        Jump(exit);
    }
}

void BuiltinsNumberStubBuilder::IsFinite(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef number = GetCallArg0(numArgs_);

    // In this method, we actually don't need slow path.
    // The following code is for passing the verification phase.
    Label noSlowPath(env);
    BRANCH(False(), slowPath, &noSlowPath);
    Bind(&noSlowPath);

    Label retTrue(env);
    Label retFalse(env);

    Label isNotInt(env);
    BRANCH(TaggedIsInt(number), &retTrue, &isNotInt);
    Bind(&isNotInt);
    {
        Label isDouble(env);
        BRANCH(TaggedIsDouble(number), &isDouble, &retFalse);
        Bind(&isDouble);
        {
            GateRef f = GetDoubleOfTDouble(number);
            BRANCH(DoubleIsNanOrInf(f), &retFalse, &retTrue);
        }
    }

    Bind(&retTrue);
    {
        *result = TaggedTrue();
        Jump(exit);
    }
    Bind(&retFalse);
    {
        *result = TaggedFalse();
        Jump(exit);
    }
}

void BuiltinsNumberStubBuilder::IsNaN(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef number = GetCallArg0(numArgs_);

    // In this method, we actually don't need slow path.
    // The following code is for passing the verification phase.
    Label noSlowPath(env);
    BRANCH(False(), slowPath, &noSlowPath);
    Bind(&noSlowPath);

    Label retTrue(env);
    Label retFalse(env);

    Label isDouble(env);
    BRANCH(TaggedIsDouble(number), &isDouble, &retFalse);
    Bind(&isDouble);
    BRANCH(DoubleIsNAN(GetDoubleOfTDouble(number)), &retTrue, &retFalse);

    Bind(&retTrue);
    {
        *result = TaggedTrue();
        Jump(exit);
    }
    Bind(&retFalse);
    {
        *result = TaggedFalse();
        Jump(exit);
    }
}

void BuiltinsNumberStubBuilder::IsInteger(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef number = GetCallArg0(numArgs_);

    // In this method, we actually don't need slow path.
    // The following code is for passing the verification phase.
    Label noSlowPath(env);
    BRANCH(False(), slowPath, &noSlowPath);
    Bind(&noSlowPath);

    Label retTrue(env);
    Label retFalse(env);

    Label isNotInt(env);
    BRANCH(TaggedIsInt(number), &retTrue, &isNotInt);
    Bind(&isNotInt);
    {
        Label isDouble(env);
        BRANCH(TaggedIsDouble(number), &isDouble, &retFalse);
        Bind(&isDouble);
        BRANCH(DoubleIsInteger(GetDoubleOfTDouble(number)), &retTrue, &retFalse);
    }

    Bind(&retTrue);
    {
        *result = TaggedTrue();
        Jump(exit);
    }
    Bind(&retFalse);
    {
        *result = TaggedFalse();
        Jump(exit);
    }
}

void BuiltinsNumberStubBuilder::IsSafeInteger(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    GateRef number = GetCallArg0(numArgs_);

    // In this method, we actually don't need slow path.
    // The following code is for passing the verification phase.
    Label noSlowPath(env);
    BRANCH(False(), slowPath, &noSlowPath);
    Bind(&noSlowPath);

    Label retTrue(env);
    Label retFalse(env);

    Label isNotInt(env);
    BRANCH(TaggedIsInt(number), &retTrue, &isNotInt);
    Bind(&isNotInt);
    {
        Label isDouble(env);
        BRANCH(TaggedIsDouble(number), &isDouble, &retFalse);
        Bind(&isDouble);
        {
            Label isNotNanOrInf(env);
            GateRef f = GetDoubleOfTDouble(number);
            BRANCH(DoubleIsNanOrInf(f), &retFalse, &isNotNanOrInf);
            Bind(&isNotNanOrInf);
            {
                Label checkSafe(env);
                GateRef truncated = ChangeInt32ToFloat64(TruncFloatToInt64(f));
                BRANCH(DoubleEqual(f, truncated), &checkSafe, &retFalse);
                Bind(&checkSafe);
                BRANCH(DoubleLessThanOrEqual(DoubleAbs(f), Double(base::MAX_SAFE_INTEGER)), &retTrue, &retFalse);
            }
        }
    }

    Bind(&retTrue);
    {
        *result = TaggedTrue();
        Jump(exit);
    }
    Bind(&retFalse);
    {
        *result = TaggedFalse();
        Jump(exit);
    }
}

void BuiltinsNumberStubBuilder::GenNumberConstructor(GateRef nativeCode, GateRef func, GateRef newTarget)
{
    auto env = GetEnvironment();
    DEFVARIABLE(res, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(numberValue, VariableType::JS_ANY(), IntToTaggedPtr(IntPtr(0)));
    Label thisCollectionObj(env);
    Label slowPath(env);
    Label slowPath1(env);
    Label exit(env);

    Label hasArg(env);
    Label numberCreate(env);
    Label newTargetIsHeapObject(env);
    BRANCH(TaggedIsHeapObject(newTarget), &newTargetIsHeapObject, &slowPath);
    Bind(&newTargetIsHeapObject);
    BRANCH(Int64GreaterThan(numArgs_, IntPtr(0)), &hasArg, &numberCreate);
    Bind(&hasArg);
    {
        GateRef value = GetArgFromArgv(glue_, Int32(0));
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
        BRANCH(IsJSFunction(glue_, newTarget), &newTargetIsJSFunction, &slowPath);
        Bind(&newTargetIsJSFunction);
        {
            Label intialHClassIsHClass(env);
            GateRef intialHClass = Load(VariableType::JS_ANY(), glue_, newTarget,
                IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
            BRANCH(IsJSHClass(glue_, intialHClass), &intialHClassIsHClass, &slowPath1);
            Bind(&intialHClassIsHClass);
            {
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue_, 0);
                Label afterNew(env);
                newBuilder.NewJSObject(&res, &afterNew, intialHClass);
                Bind(&afterNew);
                {
                    GateRef valueOffset = IntPtr(JSPrimitiveRef::VALUE_OFFSET);
                    Store(VariableType::INT64(), glue_, *res, valueOffset, *numberValue);
                    Jump(&exit);
                }
            }
            Bind(&slowPath1);
            {
                GateRef argv = GetArgv();
                res = CallBuiltinRuntimeWithNewTarget(glue_,
                    { glue_, nativeCode, func, thisValue_, numArgs_, argv, newTarget });
                Jump(&exit);
            }
        }
    }

    Bind(&slowPath);
    {
        GateRef argv = GetArgv();
        res = CallBuiltinRuntime(glue_, { glue_, nativeCode, func, thisValue_, numArgs_, argv }, true);
        Jump(&exit);
    }
    Bind(&exit);
    Return(*res);
}

void BuiltinsNumberStubBuilder::ToStringFunc(Variable *result, Label *exit, Label *slowPath)
{
    auto env = GetEnvironment();
    Label definedMsg(env);
    Label undefinedMsg(env);
    Label thisIsInt(env);
    Label msgIsInt(env);
    BRANCH(TaggedIsInt(thisValue_), &thisIsInt, slowPath);
    Bind(&thisIsInt);
    GateRef thisValueInt = GetInt32OfTInt(thisValue_);
    GateRef msg = GetCallArg0(numArgs_);
    BRANCH(TaggedIsUndefined(msg), &undefinedMsg, &definedMsg);
    Bind(&undefinedMsg);
    {
        *result = NumberToString(thisValueInt, Int32(10)); // 10: means radix
        Jump(exit);
    }
    Bind(&definedMsg);
    BRANCH(TaggedIsInt(msg), &msgIsInt, slowPath);
    Bind(&msgIsInt);
    {
        Label throwError(env);
        Label notThrowError(env);
        GateRef msgValue = GetInt32OfTInt(msg);
        GateRef outOfRange = BitOr(Int32LessThan(msgValue, Int32(base::MIN_RADIX)),
                                   Int32GreaterThan(msgValue, Int32(base::MAX_RADIX)));
        BRANCH(outOfRange, &throwError, &notThrowError);
        Bind(&throwError);
        {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(InvalidRadixLength));
            CallRuntime(glue_, RTSTUB_ID(ThrowRangeError), { IntToTaggedInt(taggedId) });
            Jump(exit);
        }
        Bind(&notThrowError);
        {
            *result = NumberToString(thisValueInt, msgValue);
            Jump(exit);
        }
    }
}

GateRef BuiltinsNumberStubBuilder::NumberToString(GateRef number, GateRef radix)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::JS_POINTER(), Hole());
    DEFVARIABLE(n, VariableType::INT32(), number);

    Label exit(env);
    Label numIsNegative(env);
    Label numNotNegative(env);
    Label afterFast(env);
    Label afterNew(env);
    GateRef isNegative = Int32LessThan(number, Int32(0));
    BRANCH(isNegative, &numIsNegative, &numNotNegative);
    Bind(&numIsNegative);
    {
        n = Int32Sub(Int32(0), *n);
        Jump(&afterFast);
    }
    Bind(&numNotNegative);
    {
        Label thisIsZero(env);
        Label thisNotZero(env);
        Label thisIsSingle(env);
        Label thisNotSingle(env);
        BRANCH(Int32Equal(number, Int32(0)), &thisIsZero, &thisNotZero);
        Bind(&thisIsZero);
        {
            result = GetGlobalConstantValue(VariableType::JS_POINTER(), glue_, ConstantIndex::ZERO_INDEX);
            Jump(&exit);
        }
        Bind(&thisNotZero);
        {
            BRANCH(Int32LessThan(number, radix), &thisIsSingle, &afterFast);
            Bind(&thisIsSingle);
            GateRef singleCharTable = GetSingleCharTable(glue_);
            GateRef index = ToCharCode(number);
            result = GetValueFromTaggedArray(glue_, singleCharTable, index);
            Jump(&exit);
        }
    }
    Bind(&afterFast);
    {
        DEFVARIABLE(temp, VariableType::INT32(), *n);
        DEFVARIABLE(length, VariableType::INT32(), Int32(0));
        Label lenAddOne(env);
        Label lenNotAddOne(env);
        BRANCH(isNegative, &lenAddOne, &lenNotAddOne);
        Bind(&lenAddOne);
        {
            length = Int32Add(*length, Int32(1));
            Jump(&lenNotAddOne);
        }
        Bind(&lenNotAddOne);
        {
            Label loopHead(env);
            Label loopEnd(env);
            Label next(env);
            Label loopExit(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                BRANCH(Int32GreaterThan(*temp, Int32(0)), &next, &loopExit);
                Bind(&next);
                {
                    temp = Int32Div(*temp, radix);
                    length = Int32Add(*length, Int32(1));
                    Jump(&loopEnd);
                }
            }
            Bind(&loopEnd);
            LoopEndWithCheckSafePoint(&loopHead, env, glue_);
            Bind(&loopExit);
            {
                NewObjectStubBuilder newBuilder(this);
                newBuilder.SetParameters(glue_, 0);
                newBuilder.AllocLineStringObject(&result, &afterNew, *length, true);
                Bind(&afterNew);
                {
                    GateRef dst = ChangeTaggedPointerToInt64(PtrAdd(*result, IntPtr(LineString::DATA_OFFSET)));
                    DEFVARIABLE(cursor, VariableType::INT32(), Int32Sub(*length, Int32(1)));
                    DEFVARIABLE(digit, VariableType::INT32(), Int32(0));
                    DEFVARIABLE(dstTmp, VariableType::NATIVE_POINTER(), dst);
                    dstTmp = PtrAdd(*dstTmp, PtrMul(ZExtInt32ToPtr(*cursor), IntPtr(sizeof(uint8_t))));
                    Label loopHead1(env);
                    Label loopEnd1(env);
                    Label next1(env);
                    Label loopExit1(env);
                    Jump(&loopHead1);
                    LoopBegin(&loopHead1);
                    {
                        BRANCH(Int32GreaterThan(*n, Int32(0)), &next1, &loopExit1);
                        Bind(&next1);
                        {
                            digit = Int32Mod(*n, radix);
                            n = Int32Div(*n, radix);
                            GateRef digitChar = ToCharCode(*digit);
                            Store(VariableType::INT8(), glue_, *dstTmp, IntPtr(0), TruncInt32ToInt8(digitChar));
                            cursor = Int32Sub(*cursor, Int32(1));
                            Jump(&loopEnd1);
                        }
                    }
                    Bind(&loopEnd1);
                    dstTmp = PtrSub(*dstTmp, IntPtr(sizeof(uint8_t)));
                    // Work with low level buffers, we can't call GC. Loop is simple, no more 32 iteration.
                    // Ability using GC at the end of loop require add additional calculate pointer to data of string
                    // on each iteration.
                    LoopEnd(&loopHead1);
                    Bind(&loopExit1);
                    {
                        Label strInsertSign(env);
                        Label strNotInsertSign(env);
                        BRANCH(isNegative, &strInsertSign, &exit);
                        Bind(&strInsertSign);
                        {
                            dstTmp = PtrSub(*dstTmp, IntPtr(sizeof(uint8_t)));
                            Store(VariableType::INT8(), glue_, dst, IntPtr(0), Int8(45)); // 45: means '-'
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
