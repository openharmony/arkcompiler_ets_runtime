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

#include "ecmascript/compiler/ts_hcr_opt_pass.h"

namespace panda::ecmascript::kungfu {

GateRef TSHCROptPass::VisitGate(GateRef gate)
{
    auto opcode = acc_.GetOpCode(gate);
    switch (opcode) {
        case OpCode::TYPED_BINARY_OP:
            return VisitTypedBinaryOp(gate);
        default:
            break;
    }
    return Circuit::NullGate();
}

GateRef TSHCROptPass::VisitTypedBinaryOp(GateRef gate)
{
    if (acc_.HasStringType(gate)) {
        return VisitStringBinOp(gate);
    }
    return Circuit::NullGate();
}

GateRef TSHCROptPass::VisitStringBinOp(GateRef gate)
{
    TypedBinOp op = acc_.GetTypedBinaryOp(gate);
    switch (op) {
        case TypedBinOp::TYPED_EQ:
            return VisitStringEqual(gate);
        default:
            return Circuit::NullGate();
    }
}

GateRef TSHCROptPass::VisitStringEqual(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    if (acc_.IsConstString(left) && acc_.IsConstString(right)) {
        return ConvertStringEqualToConst(left, right);
    }

    if (IsSingleCharString(left) && IsSingleCharString(right)) {
        return ConvertToSingleCharComparison(left, right);
    }

    if (IsNotLoadStrOrStringLoadElement(left) || IsNotLoadStrOrStringLoadElement(right)) {
        return Circuit::NullGate();
    }

    if (IsSingleCharString(left) || IsSingleCharString(right)) {
        return builder_.Boolean(false);
    }

    return Circuit::NullGate();
}

GateRef TSHCROptPass::ConvertStringEqualToConst(GateRef left, GateRef right)
{
    uint32_t leftId = acc_.GetStringIdFromLdaStrGate(left);
    uint32_t rightId = acc_.GetStringIdFromLdaStrGate(right);

    auto leftMethodOffset = acc_.TryGetMethodOffset(left);
    auto rightMethodOffset = acc_.TryGetMethodOffset(right);
    JSHandle<EcmaString> leftStr(thread_, GetStringFromConstantPool(leftMethodOffset, leftId));
    JSHandle<EcmaString> rightStr(thread_, GetStringFromConstantPool(rightMethodOffset, rightId));
    bool isEqual = EcmaStringAccessor::StringsAreEqual(thread_->GetEcmaVM(), leftStr, rightStr);
    if (isEqual) {
        return builder_.Boolean(true);
    }
    return builder_.Boolean(false);
}

bool TSHCROptPass::IsSingleCharString(GateRef gate)
{
    if (acc_.IsConstString(gate)) {
        uint32_t strId = acc_.GetStringIdFromLdaStrGate(gate);
        auto methodOffset = acc_.TryGetMethodOffset(gate);
        JSTaggedValue str = GetStringFromConstantPool(methodOffset, strId);
        return EcmaStringAccessor(str).GetLength() == 1;
    }
    return acc_.IsSingleCharGate(gate);
}

bool TSHCROptPass::IsNotLoadStrOrStringLoadElement(GateRef gate)
{
    OpCode op = acc_.GetOpCode(gate);
    if (op == OpCode::JS_BYTECODE) {
        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        return ecmaOpcode != EcmaOpcode::LDA_STR_ID16;
    }
    if (op == OpCode::LOAD_ELEMENT) {
        return acc_.GetTypedLoadOp(gate) != TypedLoadOp::STRING_LOAD_ELEMENT;
    }
    return true;
}

GateRef TSHCROptPass::ConvertConstSingleCharToInt32(GateRef gate)
{
    ASSERT(acc_.IsConstString(gate));
    uint32_t strId = acc_.GetStringIdFromLdaStrGate(gate);
    auto methodOffset = acc_.TryGetMethodOffset(gate);
    JSTaggedValue str = GetStringFromConstantPool(methodOffset, strId);
    ASSERT(EcmaStringAccessor(str).GetLength() == 1);
    uint16_t strToInt = EcmaStringAccessor(str).Get(0);
    return builder_.Int32(strToInt);
}

GateRef TSHCROptPass::ConvertToSingleCharComparison(GateRef left, GateRef right)
{
    ASSERT(!acc_.IsConstString(left) || !acc_.IsConstString(right));
    if (acc_.IsConstString(left)) {
        left = ConvertConstSingleCharToInt32(left);
    } else if (acc_.IsConstString(right)) {
        right = ConvertConstSingleCharToInt32(right);
    }
    // change string binary operator to int binary operator
    return builder_.TypedBinaryOp<TypedBinOp::TYPED_EQ>(left, right, ParamType::IntType());
}
}  // namespace panda::ecmascript::kungfu
