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
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_iterator.h"
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
            CallRangeTypeInfoAccessor tia(compilationEnv_, circuit_, gate);
            return {{tia.GetArgc(), false}};
        }
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8: {
            CallThisRangeTypeInfoAccessor tia(compilationEnv_, circuit_, gate);
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
        CallTypeInfoAccessor ctia(compilationEnv_, circuit_, gate);
        BuiltinsStubCSigns::ID id = ctia.TryGetPGOBuiltinMethodId();
        if (IS_INVALID_ID(id)) {
            continue;
        }
        switch (id) {
            case BuiltinsStubCSigns::ID::StringFromCharCode:
                TryInlineStringFromCharCode(gate, argc, skipThis);
                break;
            case BuiltinsStubCSigns::ID::NumberIsFinite:
                TryInlineNumberIsFinite(gate, argc, skipThis);
                break;
            case BuiltinsStubCSigns::ID::NumberIsInteger:
                TryInlineNumberIsInteger(gate, argc, skipThis);
                break;
            case BuiltinsStubCSigns::ID::NumberIsNaN:
                TryInlineNumberIsNaN(gate, argc, skipThis);
                break;
            case BuiltinsStubCSigns::ID::NumberIsSafeInteger:
                TryInlineNumberIsSafeInteger(gate, argc, skipThis);
                break;
            case BuiltinsStubCSigns::ID::TypedArrayEntries:
                TryInlineTypedArrayIteratorBuiltin(gate, id, circuit_->TypedArrayEntries(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::TypedArrayKeys:
                TryInlineTypedArrayIteratorBuiltin(gate, id, circuit_->TypedArrayKeys(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::TypedArrayValues:
                TryInlineTypedArrayIteratorBuiltin(gate, id, circuit_->TypedArrayValues(), skipThis);
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
            case BuiltinsStubCSigns::ID::DateGetTime:
                TryInlineDateGetTime(gate, argc, skipThis);
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
            case BuiltinsStubCSigns::ID::ArrayBufferIsView:
                TryInlineArrayBufferIsView(gate, argc, id, skipThis);
                break;
            case BuiltinsStubCSigns::ID::DataViewGetFloat32:
            case BuiltinsStubCSigns::ID::DataViewGetFloat64:
            case BuiltinsStubCSigns::ID::DataViewGetInt8:
            case BuiltinsStubCSigns::ID::DataViewGetInt16:
            case BuiltinsStubCSigns::ID::DataViewGetInt32:
            case BuiltinsStubCSigns::ID::DataViewGetUint16:
            case BuiltinsStubCSigns::ID::DataViewGetUint32:
            case BuiltinsStubCSigns::ID::DataViewGetUint8:
                TryInlineDataViewGet(gate, argc, id);
                break;
            case BuiltinsStubCSigns::ID::DataViewSetFloat32:
            case BuiltinsStubCSigns::ID::DataViewSetFloat64:
            case BuiltinsStubCSigns::ID::DataViewSetInt8:
            case BuiltinsStubCSigns::ID::DataViewSetInt16:
            case BuiltinsStubCSigns::ID::DataViewSetInt32:
            case BuiltinsStubCSigns::ID::DataViewSetUint8:
            case BuiltinsStubCSigns::ID::DataViewSetUint16:
            case BuiltinsStubCSigns::ID::DataViewSetUint32:
                TryInlineDataViewSet(gate, argc, id);
                break;
            case BuiltinsStubCSigns::ID::BigIntAsIntN:
            case BuiltinsStubCSigns::ID::BigIntAsUintN:
                TryInlineBigIntAsIntN(gate, argc, id, skipThis);
                break;
            case BuiltinsStubCSigns::ID::MapGet:
                InlineStubBuiltin(gate, 1U, argc, id, circuit_->MapGet(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MapHas:
                InlineStubBuiltin(gate, 1U, argc, id, circuit_->MapHas(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MapKeys:
                InlineStubBuiltin(gate, 0U, argc, id, circuit_->MapKeys(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MapValues:
                InlineStubBuiltin(gate, 0U, argc, id, circuit_->MapValues(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MapEntries:
                InlineStubBuiltin(gate, 0U, argc, id, circuit_->MapEntries(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::SetHas:
                InlineStubBuiltin(gate, 1U, argc, id, circuit_->SetHas(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::DateNow:
                TryInlineWhitoutParamBuiltin(gate, argc, id, circuit_->DateNow(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::MapDelete:
                InlineStubBuiltin(gate, 1U, argc, id, circuit_->MapDelete(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::SetDelete:
                InlineStubBuiltin(gate, 1U, argc, id, circuit_->SetDelete(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::SetValues:
                InlineStubBuiltin(gate, 0U, argc, id, circuit_->SetValues(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::SetEntries:
                InlineStubBuiltin(gate, 0U, argc, id, circuit_->SetEntries(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::BigIntConstructor:
                TryInlineBigIntConstructor(gate, argc, skipThis);
                break;
            case BuiltinsStubCSigns::ID::MapClear:
                InlineStubBuiltin(gate, 0U, argc, id, circuit_->MapClear(), skipThis);
                break;
            case BuiltinsStubCSigns::ID::SetClear:
                InlineStubBuiltin(gate, 0U, argc, id, circuit_->SetClear(), skipThis);
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

void NativeInlineLowering::AddTraceLogs(GateRef gate, BuiltinsStubCSigns::ID id)
{
    size_t index = RTSTUB_ID(AotInlineBuiltinTrace);

    GateRef frameState = acc_.GetFrameState(gate);
    GateRef frameArgs = acc_.GetValueIn(frameState);
    GateRef callerFunc = acc_.GetValueIn(frameArgs, 0);
    std::vector<GateRef> args{callerFunc, builder_.Int32(id)};

    builder_.CallRuntime(glue_, index, Gate::InvalidGateRef, args, gate);
}

void NativeInlineLowering::TryInlineStringFromCharCode(GateRef gate, size_t argc, bool skipThis)
{
    if (!skipThis) {
        return;
    }
    if (argc != 1) {
        return;
    }
    CallThis1TypeInfoAccessor tacc(compilationEnv_, circuit_, gate);
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, tacc.GetFunc(),
                                 builder_.IntPtr(static_cast<int64_t>(BuiltinsStubCSigns::ID::StringFromCharCode)),
                                 {tacc.GetArg0()});
    }

    if (EnableTrace()) {
        AddTraceLogs(gate, BuiltinsStubCSigns::ID::StringFromCharCode);
    }

    GateRef ret = builder_.StringFromSingleCharCode(tacc.GetArg0());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

void NativeInlineLowering::TryInlineNumberIsFinite(GateRef gate, size_t argc, bool skipThis)
{
    if (!skipThis) {
        return;
    }
    if (argc != 1) {
        return;
    }
    CallThis1TypeInfoAccessor tacc(compilationEnv_, circuit_, gate);
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, tacc.GetFunc(),
                                 builder_.IntPtr(static_cast<int64_t>(BuiltinsStubCSigns::ID::NumberIsFinite)));
    }
    GateRef ret = builder_.NumberIsFinite(tacc.GetArg0());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

void NativeInlineLowering::TryInlineNumberIsInteger(GateRef gate, size_t argc, bool skipThis)
{
    if (!skipThis) {
        return;
    }
    if (argc != 1) {
        return;
    }
    CallThis1TypeInfoAccessor tacc(compilationEnv_, circuit_, gate);
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, tacc.GetFunc(),
                                 builder_.IntPtr(static_cast<int64_t>(BuiltinsStubCSigns::ID::NumberIsInteger)));
    }
    GateRef ret = builder_.NumberIsInteger(tacc.GetArg0());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

void NativeInlineLowering::TryInlineNumberIsNaN(GateRef gate, size_t argc, bool skipThis)
{
    if (!skipThis) {
        return;
    }
    if (argc != 1) {
        return;
    }
    CallThis1TypeInfoAccessor tacc(compilationEnv_, circuit_, gate);
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, tacc.GetFunc(),
                                 builder_.IntPtr(static_cast<int64_t>(BuiltinsStubCSigns::ID::NumberIsNaN)));
    }
    GateRef ret = builder_.NumberIsNaN(tacc.GetArg0());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

void NativeInlineLowering::TryInlineNumberIsSafeInteger(GateRef gate, size_t argc, bool skipThis)
{
    if (!skipThis) {
        return;
    }
    if (argc != 1) {
        return;
    }
    CallThis1TypeInfoAccessor tacc(compilationEnv_, circuit_, gate);
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, tacc.GetFunc(),
                                 builder_.IntPtr(static_cast<int64_t>(BuiltinsStubCSigns::ID::NumberIsSafeInteger)));
    }
    GateRef ret = builder_.NumberIsSafeInteger(tacc.GetArg0());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), ret);
}

void NativeInlineLowering::TryInlineBigIntAsIntN(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                 bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (argc < 2U) {
        return;
    }
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }
    GateRef bits = acc_.GetValueIn(gate, firstParam);
    GateRef bigint = acc_.GetValueIn(gate, firstParam + 1);
    GateRef frameState = acc_.GetFrameState(gate);
    bool isUnsigned = (id == BuiltinsStubCSigns::ID::BigIntAsUintN);
    const auto* op = isUnsigned ? circuit_->BigIntAsUintN() : circuit_->BigIntAsIntN();
    GateRef ret = builder_.BuildBigIntAsIntN(op, {bits, bigint, frameState});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineTypedArrayIteratorBuiltin(GateRef gate,
                                                              BuiltinsStubCSigns::ID id,
                                                              const GateMetaData* op, bool skipThis)
{
    if (!skipThis) {
        return;
    }

    CallThis0TypeInfoAccessor tacc(compilationEnv_, circuit_, gate);
    Environment env(gate, circuit_, &builder_);

    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, tacc.GetFunc(), builder_.IntPtr(static_cast<int64_t>(id)), {tacc.GetThisObj()});
    }

    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }

    GateRef ret = builder_.BuildTypedArrayIterator(acc_.GetValueIn(gate, 0), op);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
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

    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }

    if (argc == 0) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.NanValue());
        return;
    }
    GateRef ret = builder_.BuildControlDependOp(op, {acc_.GetValueIn(gate, firstParam)});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineWhitoutParamBuiltin(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id,
                                                        const GateMetaData* op, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }

    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }

    GateRef ret = builder_.BuildControlDependOp(op, {});
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
    if (EnableTrace()) {
        AddTraceLogs(gate, BuiltinsStubCSigns::ID::MathClz32);
    }
    if (argc == 0) {
        const int32_t defaultValue = 32;
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.Int32(defaultValue));
        return;
    }
    GateRef ret = builder_.BuildControlDependOp(circuit_->MathClz32(), {acc_.GetValueIn(gate, firstParam)});
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
    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }
    if (argc == 0) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.Boolean(false));
        return;
    }
    GateRef ret = builder_.BuildControlDependOp(op, {acc_.GetValueIn(gate, firstParam)});
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
    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }
    if (argc == 0) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.Boolean(true));
        return;
    }
    GateRef ret = builder_.BuildControlDependOp(op, {acc_.GetValueIn(gate, firstParam)});
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
    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }
    if (argc < 2U) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.Int32(0));
        return;
    }
    GateRef ret = builder_.BuildControlDependOp(op, {acc_.GetValueIn(gate, firstParam),
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
    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }
    if (argc < 2U) {
        acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), builder_.NanValue());
        return;
    }
    GateRef ret = builder_.BuildControlDependOp(op, {acc_.GetValueIn(gate, firstParam),
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
    if (EnableTrace()) {
        AddTraceLogs(gate, id);
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
        ret = builder_.BuildControlDependOp(op, {ret, param});
    }
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineArrayBufferIsView(GateRef gate,
                                                      size_t argc,
                                                      BuiltinsStubCSigns::ID id,
                                                      bool skipThis)
{
    if (!skipThis) {
        return;
    }
    if (argc != 1) {
        return;
    }
    CallThis1TypeInfoAccessor tacc(compilationEnv_, circuit_, gate);
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, tacc.GetFunc(), builder_.IntPtr(static_cast<int64_t>(id)), {tacc.GetArg0()});
    }
    GateRef arg0 = tacc.GetArg0();
    GateRef ret = builder_.ArrayBufferIsView(arg0);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineDataViewGet(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id)
{
    if (argc != 1 && argc != 2) { // number of args must be 1/2
        return;
    }
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + 1), builder_.IntPtr(static_cast<int64_t>(id)));
    }
    GateRef thisObj = acc_.GetValueIn(gate, 0); // 0: this
    builder_.IsEcmaObjectCheck(thisObj);
    builder_.IsDataViewCheck(thisObj);
    GateRef dataViewCallID = builder_.Int32(id);
    GateRef index = acc_.GetValueIn(gate, 1); // 1: index of dataView
    GateRef ret = Circuit::NullGate();
    GateRef frameState = acc_.GetFrameState(gate);
    if (argc == 1) { // if not provide isLittleEndian, default use big endian
        ret = builder_.DataViewGet(thisObj, index, dataViewCallID, builder_.TaggedFalse(), frameState);
    } else if (argc == 2) { // 2: provide isLittleEndian
        GateRef isLittleEndian = acc_.GetValueIn(gate, 2); // 2: is little endian mode
        builder_.IsTaggedBooleanCheck(isLittleEndian);
        ret = builder_.DataViewGet(thisObj, index, dataViewCallID, isLittleEndian, frameState);
    }
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::TryInlineDataViewSet(GateRef gate, size_t argc, BuiltinsStubCSigns::ID id)
{
    if (argc != 1 && argc != 2 && argc != 3) { // number of args must be 1/2/3
        return;
    }
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + 1), builder_.IntPtr(static_cast<int64_t>(id)));
    }
    GateRef thisObj = acc_.GetValueIn(gate, 0); // 0: this
    builder_.IsEcmaObjectCheck(thisObj);
    builder_.IsDataViewCheck(thisObj);
    GateRef dataViewCallID = builder_.Int32(id);
    GateRef index = acc_.GetValueIn(gate, 1); // 1: index

    GateRef ret = Circuit::NullGate();
    GateRef frameState = acc_.GetFrameState(gate);
    if (argc == 1) { // arg counts is 1
        ret = builder_.DataViewSet(
            thisObj, index, builder_.Double(base::NAN_VALUE), dataViewCallID, builder_.TaggedFalse(), frameState);
    } else if (argc == 2) { // arg counts is 2
        GateRef value = acc_.GetValueIn(gate, 2); // 2: value
        ret = builder_.DataViewSet(thisObj, index, value, dataViewCallID, builder_.TaggedFalse(), frameState);
    } else if (argc == 3) { // arg counts is 3
        GateRef value = acc_.GetValueIn(gate, 2); // 2: value
        GateRef isLittleEndian = acc_.GetValueIn(gate, 3); // 3: is little endian mode
        builder_.IsTaggedBooleanCheck(isLittleEndian);
        ret = builder_.DataViewSet(thisObj, index, value, dataViewCallID, isLittleEndian, frameState);
    }
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::InlineStubBuiltin(GateRef gate, size_t builtinArgc, size_t realArgc,
    BuiltinsStubCSigns::ID id, const GateMetaData* op, bool skipThis)
{
    if (!skipThis) {
        return;
    }
    Environment env(gate, circuit_, &builder_);
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, realArgc + 1U),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }

    std::vector<GateRef> args {};
    for (size_t i = 0; i <= builtinArgc; i++) {
        args.push_back(i <= realArgc ? acc_.GetValueIn(gate, i) : builder_.Undefined());
    }
    GateRef ret = builder_.BuildControlDependOp(op, args);
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

void NativeInlineLowering::ReplaceGateWithPendingException(GateRef hirGate, GateRef value)
{
    GateRef state = builder_.GetState();
    // copy depend-wire of hirGate to value
    GateRef depend = builder_.GetDepend();
    // exception value
    GateRef exceptionVal = builder_.ExceptionConstant();
    // compare with trampolines result
    GateRef equal = builder_.Equal(value, exceptionVal);
    auto ifBranch = builder_.Branch(state, equal, 1, BranchWeight::DEOPT_WEIGHT, "checkException");

    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    GateRef eDepend = builder_.DependRelay(ifTrue, depend);
    GateRef sDepend = builder_.DependRelay(ifFalse, depend);
    StateDepend success(ifFalse, sDepend);
    StateDepend exception(ifTrue, eDepend);
    acc_.ReplaceHirWithIfBranch(hirGate, success, exception, value);

}

void NativeInlineLowering::TryInlineBigIntConstructor(GateRef gate, size_t argc, bool skipThis)
{
    Environment env(gate, circuit_, &builder_);
    bool firstParam = skipThis ? 1 : 0;
    auto id = BuiltinsStubCSigns::ID::BigIntConstructor;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(id)));
    }
    if (EnableTrace()) {
        AddTraceLogs(gate, id);
    }

    auto param = builder_.Undefined();
    if (argc > 0) {
        param = acc_.GetValueIn(gate, firstParam);
    }

    GateRef ret = builder_.BuildControlDependOp(circuit_->BigIntConstructor(), {param});
    ReplaceGateWithPendingException(gate, ret);
    return;
}

void NativeInlineLowering::TryInlineDateGetTime(GateRef gate, size_t argc, bool skipThis)
{
    // Always shout be "this", we can't inline this function without instance of object
    if (!skipThis) {
        return;
    }
    Environment env(gate, circuit_, &builder_);
    // We are sure, that "this" is passed to this function, so always need to do +1
    bool firstParam = 1;
    if (!Uncheck()) {
        builder_.CallTargetCheck(gate, acc_.GetValueIn(gate, argc + firstParam),
                                 builder_.IntPtr(static_cast<int64_t>(BuiltinsStubCSigns::ID::DateGetTime)));
    }
    if (EnableTrace()) {
        AddTraceLogs(gate, BuiltinsStubCSigns::ID::DateGetTime);
    }
    // Take object using "this"
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef ret = builder_.BuildControlDependOp(circuit_->DateGetTime(), {obj});
    acc_.ReplaceHirAndDeleteIfException(gate, builder_.GetStateDepend(), ret);
}

}  // namespace panda::ecmascript
