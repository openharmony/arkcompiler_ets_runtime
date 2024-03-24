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

std::optional<size_t> NativeInlineLowering::GetArgc(GateRef gate)
{
    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
            return 0U;
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            return 1U;
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            return 2U;
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            return 3U;
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8: {
            CallThisRangeTypeInfoAccessor tia(thread_, circuit_, gate);
            return tia.GetArgc();
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
        std::optional<size_t> opt_argc = GetArgc(gate);
        if (!opt_argc) {
            continue;
        }
        size_t argc = opt_argc.value();
        CallTypeInfoAccessor ctia(thread_, circuit_, gate);
        BuiltinsStubCSigns::ID id = ctia.TryGetPGOBuiltinId();
        switch (id) {
            case BuiltinsStubCSigns::ID::StringFromCharCode:
                TryInlineStringFromCharCode(gate, argc);
                break;
            case BuiltinsStubCSigns::ID::MathAcos:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAcos());
                break;
            case BuiltinsStubCSigns::ID::MathAcosh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAcosh());
                break;
            case BuiltinsStubCSigns::ID::MathAsin:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAsin());
                break;
            case BuiltinsStubCSigns::ID::MathAsinh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAsinh());
                break;
            case BuiltinsStubCSigns::ID::MathAtan:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAtan());
                break;
            case BuiltinsStubCSigns::ID::MathAtan2:
                TryInlineMathBinaryBuiltin(gate, argc, id, circuit_->MathAtan2());
                break;
            case BuiltinsStubCSigns::ID::MathAtanh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAtanh());
                break;
            case BuiltinsStubCSigns::ID::MathCos:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathCos());
                break;
            case BuiltinsStubCSigns::ID::MathCosh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathCosh());
                break;
            case BuiltinsStubCSigns::ID::MathSin:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathSin());
                break;
            case BuiltinsStubCSigns::ID::MathSinh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathSinh());
                break;
            case BuiltinsStubCSigns::ID::MathTan:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathTan());
                break;
            case BuiltinsStubCSigns::ID::MathTanh:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathTanh());
                break;
            case BuiltinsStubCSigns::ID::MathAbs:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathAbs());
                break;
            case BuiltinsStubCSigns::ID::MathLog:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathLog());
                break;
            case BuiltinsStubCSigns::ID::MathLog2:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathLog2());
                break;
            case BuiltinsStubCSigns::ID::MathLog10:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathLog10());
                break;
            case BuiltinsStubCSigns::ID::MathLog1p:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathLog1p());
                break;
            case BuiltinsStubCSigns::ID::MathExp:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathExp());
                break;
            case BuiltinsStubCSigns::ID::MathExpm1:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathExpm1());
                break;
            case BuiltinsStubCSigns::ID::MathPow:
                TryInlineMathBinaryBuiltin(gate, argc, id, circuit_->MathPow());
                break;
            case BuiltinsStubCSigns::ID::MathCbrt:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathCbrt());
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

void NativeInlineLowering::TryInlineStringFromCharCode(GateRef gate, size_t argc)
{
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
                                                     const GateMetaData* op)
{
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + 1),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    // NOTE(schernykh): Add tracing
    if (argc == 0) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.NanValue());
        return;
    }
    GateRef ret = builder_.BuildMathBuiltinOp(op, {acc_.GetValueIn(gate, 1)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineMathBinaryBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                      const GateMetaData* op)
{
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + 1),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    // NOTE(schernykh): Add tracing
    if (argc < 2U) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.NanValue());
        return;
    }
    GateRef ret = builder_.BuildMathBuiltinOp(op, {acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
    return;
}

}  // namespace panda::ecmascript
