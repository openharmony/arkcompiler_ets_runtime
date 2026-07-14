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

#include "ecmascript/compiler/hash_stub_builder.h"

namespace panda::ecmascript::kungfu {

GateRef HashStubBuilder::GetHash(GateRef key)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    Label exit(env);
    Label slowGetHash(env);
    env->SubCfgEntry(&entryLabel);
    DEFVARIABLE(res, VariableType::INT32(), Int32(0));
    IR_IF (TaggedIsInt(key)) {
        res = TaggedGetInt(key);
    } IR_ELSE {
        IR_IF (TaggedIsSymbol(glue_, key)) {
            res = LoadPrimitive(VariableType::INT32(), key, IntPtr(JSSymbol::HASHFIELD_OFFSET));
        } IR_ELSE {
            IR_IF (TaggedIsString(glue_, key)) {
                res = GetHashcodeFromString(glue_, key);
            } IR_ELSE {
                IR_IF (TaggedIsHeapObject(key)) {
                    IR_IF (TaggedObjectIsEcmaObject(glue_, key)) {
                        CalcHashcodeForObject(glue_, key, &res);
                    } IR_ELSE {
                        Jump(&slowGetHash);
                    }
                } IR_ELSE {
                    IR_IF (TaggedIsNumber(key)) {
                        CalcHashcodeForNumber(key, &res);
                    } IR_ELSE {
                        Jump(&slowGetHash);
                    }
                }
            }
        }
    }
    Jump(&exit);

    Bind(&slowGetHash);
    res = GetInt32OfTInt(CallRuntime(glue_, RTSTUB_ID(GetLinkedHash), { key }));
    Jump(&exit);

    Bind(&exit);
    auto ret = *res;
    env->SubCfgExit();
    return ret;
}

void HashStubBuilder::CalcHashcodeForNumber(GateRef key, Variable *res)
{
    auto env = GetEnvironment();
    IR_IF (TaggedIsDouble(key)) {
        CalcHashcodeForDouble(key, res);
    } IR_ELSE {
        *res = CalcHashcodeForInt(key);
    }
}

void HashStubBuilder::CalcHashcodeForObject(GateRef glue, GateRef value, Variable *res)
{
    GateRef hash = StubBuilder::GetHash(glue, value);
    *res = hash;
    IR_IF (Int32Equal(**res, Int32(0))) {
        GateRef offset = IntPtr(JSThread::GlueData::GetRandomStatePtrOffset(GetEnvironment()->Is32Bit()));
        GateRef randomStatePtr = LoadPrimitive(VariableType::NATIVE_POINTER(), glue, offset);
        GateRef randomState = LoadPrimitive(VariableType::INT64(), randomStatePtr, IntPtr(0));
        GateRef k1 = Int64Xor(randomState, Int64LSR(randomState, Int64(base::RIGHT12)));
        GateRef k2 = Int64Xor(k1, Int64LSL(k1, Int64(base::LEFT25)));
        GateRef k3 = Int64Xor(k2, Int64LSR(k2, Int64(base::RIGHT27)));
        Store(VariableType::INT64(), glue, randomStatePtr, IntPtr(0), k3);
        GateRef k4 = Int64Mul(k3, Int64(base::GET_MULTIPLY));
        GateRef k5 = Int64LSR(k4, Int64(base::INT64_BITS - base::INT32_BITS));
        GateRef k6 = Int32And(TruncInt64ToInt32(k5), Int32(INT32_MAX));
        SetHash(glue, value, k6);
        *res = k6;
    }
}

void HashStubBuilder::CalcHashcodeForDouble(GateRef x, Variable *res)
{
    auto env = GetEnvironment();
    GateRef xInt64 = Int64Sub(ChangeTaggedPointerToInt64(x), Int64(JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    GateRef fractionBits = Int64And(xInt64, Int64(base::DOUBLE_SIGNIFICAND_MASK));
    GateRef expBits = Int64And(xInt64, Int64(base::DOUBLE_EXPONENT_MASK));
    GateRef isZero = BitAnd(
        Int64Equal(expBits, Int64(0)),
        Int64Equal(fractionBits, Int64(0)));

    IR_IF (isZero) {
        *res = Int32(0);
    } IR_ELSE {
        DEFVARIABLE(value, VariableType::JS_ANY(), x);
        GateRef exp = Int64Sub(
            Int64LSR(expBits, Int64(base::DOUBLE_SIGNIFICAND_SIZE)),
            Int64(base::DOUBLE_EXPONENT_BIAS));
        IR_IF (CanDoubleRepresentInt(exp, expBits, fractionBits)) {
            IR_IF (DoubleIsNAN(CastInt64ToFloat64(xInt64))) {
                *res = Int32(base::NAN_HASH);
            } IR_ELSE {
                *res = CalcHashcodeForInt(*value);
            }
        } IR_ELSE {
            *res = ChangeFloat64ToInt32(CastInt64ToFloat64(xInt64));
        }
    }
}
}  // namespace panda::ecmascript::kungfu
