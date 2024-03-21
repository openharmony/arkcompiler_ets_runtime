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
#include "ecmascript/compiler/native_inline_lowering.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/message_string.h"

namespace panda::ecmascript::kungfu {

std::optional<std::pair<size_t, bool>> NativeInlineLowering::GetCallInfo(GateRef gate)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::CALLARG0_IMM8:
            return {{0U, false}};
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
            return {{0U, true}};
        case EcmaOpcode::CALLARG1_IMM8_V8:
            return {{1U, false}};
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            return {{1U, true}};
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
            return {{2U, false}};
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            return {{2U, true}};
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
            return {{3U, false}};
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            return {{3U, true}};
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8: {
            CallRangeTypeInfoAccessor tia(thread_, circuit_, gate);
            return {{tia.GetArgc(), false}};
        }
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8: {
            CallThisRangeTypeInfoAccessor tia(thread_, circuit_, gate);
            return {{tia.GetArgc(), true}};
        }
        default:
            return std::nullopt;
    }
}

void NativeInlineLowering::RunNativeInlineLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op != OpCode::JS_BYTECODE) {
            continue;
        }
        auto optCallInfo = GetCallInfo(gate);
        if (!optCallInfo) {
            continue;
        }
        auto [argc, skipThis] = optCallInfo.value();
        CallTypeInfoAccessor ctia(thread_, circuit_, gate);
        BuiltinsStubCSigns::ID id = ctia.TryGetPGOBuiltinId();
        switch (id) {
            case BuiltinsStubCSigns::ID::StringFromCharCode:
                TryInlineStringFromCharCode(gate, argc, skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathAcos:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAcos(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathAcosh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAcosh(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathAsin:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAsin(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathAsinh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAsinh(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathAtan:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAtan(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathAtan2:
                TryInlineMathBinaryBuiltin(gate, argc, id, circuit_->MathAtan2(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathAtanh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAtanh(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathCos:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathCos(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathCosh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathCosh(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathSign:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathSign(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathSin:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathSin(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathSinh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathSinh(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathSqrt:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathSqrt(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathTan:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathTan(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathTanh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathTanh(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathTrunc:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathTrunc(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathAbs:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAbs(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathLog:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathLog(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathLog2:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathLog2(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathLog10:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathLog10(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathLog1p:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathLog1p(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathExp:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathExp(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathExpm1:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathExpm1(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathClz32:
                TryInlineMathClz32Builtin(gate, argc, skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathPow:
                TryInlineMathBinaryBuiltin(gate, argc, id, circuit_->MathPow(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathCbrt:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathCbrt(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathImul:
                TryInlineMathImulBuiltin(gate, argc, id, circuit_->MathImul(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::GlobalIsFinite:
                TryInlineGlobalFiniteBuiltin(gate, argc, id, circuit_->GlobalIsFinite(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::GlobalIsNan:
                TryInlineGlobalNanBuiltin(gate, argc, id, circuit_->GlobalIsNan(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathMin:
                TryInlineMathMinMaxBuiltin(gate, argc, id, circuit_->MathMin(), base::POSITIVE_INFINITY, skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathMax:
                TryInlineMathMinMaxBuiltin(gate, argc, id, circuit_->MathMax(), -base::POSITIVE_INFINITY, skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathRound:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathRound(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathFRound:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathFRound(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathCeil:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathCeil(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MathFloor:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathFloor(), skipThis);
                break;
            default:
                break;
        }
    }

    if (EnableLog()) {
        LOG_COMPILER(INFO) << " ";
        LOG_COMPILER(INFO) << "\033[34m" << "================="
                           << " After Native Inline Lowering "
                           << "[" << GetMethodName() << "] "
                           << "=================" << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
    }
}

void NativeInlineLowering::TryInlineStringFromCharCode(GateRef gate, size_t argc, bool skipThis)
{
    if (!skipThis) {
        return;
    }
    if (argc != 1) {
        return;
    }
    CallThis1TypeInfoAccessor tacc(thread_, circuit_, gate);
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, tacc.GetFunc(),
                                 builder_.IntPtr(static_cast<int64_t>(BuiltinsStubCSigns::ID::StringFromCharCode)),
                                 {tacc.GetArg0()});
    }
    GateRef ret = builder_.StringFromSingleCharCode(tacc.GetArg0());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

void NativeInlineLowering::TryInlineMathUnaryBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                     const GateMetaData* op, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    // NOTE(schernykh): Add tracing
    if (argc == 0) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.NanValue());
        return;
    }
    GateRef ret = builder_.BuildMathBuiltinOp(op, {acc_.GetValueIn(gate, firstParam)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineMathClz32Builtin(GateRef gate, size_t argc, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(BuiltinsStubCSigns::ID::MathClz32)));
    }
    // NOTE(schernykh): Add tracing
    if (argc == 0) {
        const int32_t defaultValue = 32;
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.Int32(defaultValue));
        return;
    }
    GateRef ret = builder_.BuildMathBuiltinOp(circuit_->MathClz32(), {acc_.GetValueIn(gate, firstParam)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineGlobalFiniteBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                        const GateMetaData* op, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    // NOTE(schernykh): Add tracing
    if (argc == 0) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.Boolean(false));
        return;
    }
    GateRef ret = builder_.BuildMathBuiltinOp(op, {acc_.GetValueIn(gate, firstParam)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineGlobalNanBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                     const GateMetaData* op, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    // NOTE(schernykh): Add tracing
    if (argc == 0) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.Boolean(true));
        return;
    }
    GateRef ret = builder_.BuildMathBuiltinOp(op, {acc_.GetValueIn(gate, firstParam)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineMathImulBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                    const GateMetaData* op, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    // NOTE(schernykh): Add tracing
    if (argc < 2U) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.Int32(0));
        return;
    }
    GateRef ret = builder_.BuildMathBuiltinOp(op, {acc_.GetValueIn(gate, firstParam),
                                              acc_.GetValueIn(gate, firstParam + 1)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
    return;
}

void NativeInlineLowering::TryInlineMathBinaryBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                      const GateMetaData* op, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    // NOTE(schernykh): Add tracing
    if (argc < 2U) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.NanValue());
        return;
    }
    GateRef ret = builder_.BuildMathBuiltinOp(op, {acc_.GetValueIn(gate, firstParam),
                                              acc_.GetValueIn(gate, firstParam + 1)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
    return;
}

void NativeInlineLowering::TryInlineMathMinMaxBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                      const GateMetaData* op, double defaultValue, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    if (argc == 0) {
        GateRef ret = builder_.DoubleToTaggedDoublePtr(builder_.Double(defaultValue));
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
        return;
    }
    GateRef ret = acc_.GetValueIn(gate, firstParam);
    if (argc == 1) {
        auto param_check = builder_.TaggedIsNumber(ret);
        builder_.DeoptCheck(param_check, acc_.GetFrameState(gate), DeoptType::BUILTIN_INLINING_TYPE_GUARD);
        if (acc_.GetGateType(ret).IsAnyType()) {
            acc_.SetGateType(ret, GateType::NumberType());
        }
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
        return;
    }
    for (size_t i = 1; i < argc; i++) {
        auto param = acc_.GetValueIn(gate, i + firstParam);
        ret = builder_.BuildMathBuiltinOp(op, {ret, param});
    }
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

}  // namespace panda::ecmascript
