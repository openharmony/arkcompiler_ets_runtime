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
            return 0;
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            return 1;
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            return 2;
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            return 3;
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
            case BuiltinsStubCSigns::ID::MathCos:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathCos());
                break;
            case BuiltinsStubCSigns::ID::MathSin:
                TryInlineMathUnaryBuiltin(gate, argc, id, circuit_->MathSin());
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

void NativeInlineLowering::TryInlineMathUnaryBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id, const GateMetaData* op)
{
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + 1),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    // NOTE(schernykh): Add tracing
    if (argc == 0) {
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), builder_.NanValue());
        return;
    }
    auto param_check = builder_.TaggedIsNumber(acc_.GetValueIn(gate, 1));
    builder_.DeoptCheck(param_check, acc_.GetFrameState(gate), DeoptType::BUILTIN_INLINING_TYPE_GUARD);
    GateRef ret = builder_.BuildUnaryOp(op, acc_.GetValueIn(gate, 1));
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

}  // namespace panda::ecmascript
