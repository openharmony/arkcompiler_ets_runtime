/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/hcr_circuit_builder.h"
#include "ecmascript/compiler/common_stub_csigns.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/compiler/circuit_builder-inl.h"

namespace panda::ecmascript::kungfu {

GateRef CircuitBuilder::NoLabelCallRuntime(GateRef glue, GateRef depend, size_t index, std::vector<GateRef> &args,
                                           GateRef hirGate)
{
    const std::string name = RuntimeStubCSigns::GetRTName(RTSTUB_ID(CallRuntime));
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
    GateRef target = IntPtr(index);
    std::vector<GateRef> inputs { depend, target, glue };
    inputs.insert(inputs.end(), args.begin(), args.end());
    auto numValuesIn = args.size() + 2; // 2: target & glue
    inputs.emplace_back(IntPtr(0));  // framestate slot
    numValuesIn += 1;
    GateRef pcOffset = Int64(acc_.TryGetPcOffset(hirGate));
    inputs.emplace_back(pcOffset);
    numValuesIn += 1;

    const GateMetaData* meta = circuit_->RuntimeCall(numValuesIn);
    MachineType machineType = cs->GetReturnType().GetMachineType();
    GateType type = cs->GetReturnType().GetGateType();
    GateRef result = circuit_->NewGate(meta, machineType, inputs.size(), inputs.data(), type, name.c_str());
    return result;
}

GateRef CircuitBuilder::ToLength(GateRef receiver)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->ToLength(), MachineType::I64,
                                     { currentControl, currentDepend, receiver }, GateType::NumberType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::CallStub(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                                 const char* comment)
{
    const CallSignature *cs = env_->IsBaselineBuiltin() ? BaselineStubCSigns::Get(index) :
                                                          CommonStubCSigns::Get(index);
    ASSERT(cs->IsCommonStub() || cs->IsBaselineStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result;
    if (GetCircuit()->IsOptimizedOrFastJit()) {
        ASSERT(hirGate != Circuit::NullGate());
        result = Call(cs, glue, target, depend, args, hirGate, comment);
    } else {
        result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    }
    return result;
}

GateRef CircuitBuilder::CallCommonStub(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                                       const char *comment)
{
    const CallSignature *cs = CommonStubCSigns::Get(index);
    ASSERT(cs->IsCommonStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result;
    if (GetCircuit()->IsOptimizedOrFastJit()) {
        ASSERT(hirGate != Circuit::NullGate());
        result = Call(cs, glue, target, depend, args, hirGate, comment);
    } else {
        result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    }
    return result;
}

GateRef CircuitBuilder::CallBuiltinRuntime(GateRef glue, GateRef depend, const std::vector<GateRef> &args, bool isNew)
{
    ASSERT(!GetCircuit()->IsOptimizedOrFastJit());
    int index = 0;
    if (!isNew) {
        index = static_cast<int>(RTSTUB_ID(PushCallArgsAndDispatchNative));
    } else {
        index = static_cast<int>(RTSTUB_ID(PushCallNewAndDispatchNative));
    }
    const std::string name = RuntimeStubCSigns::GetRTName(index);

    const CallSignature *cs = RuntimeStubCSigns::Get(index);
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), name.c_str());
    return result;
}

GateRef CircuitBuilder::CallBuiltinRuntimeWithNewTarget(GateRef glue, GateRef depend, const std::vector<GateRef> &args)
{
    ASSERT(!GetCircuit()->IsOptimizedOrFastJit());
    int index = 0;

    index = static_cast<int>(RTSTUB_ID(PushNewTargetAndDispatchNative));
    const std::string name = RuntimeStubCSigns::GetRTName(index);

    const CallSignature *cs = RuntimeStubCSigns::Get(index);
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), name.c_str());
    return result;
}

GateRef CircuitBuilder::Call(const CallSignature* cs, GateRef glue, GateRef target, GateRef depend,
                             const std::vector<GateRef> &args, GateRef hirGate, const char* comment)
{
    std::vector<GateRef> inputs { depend, target, glue };
    inputs.insert(inputs.end(), args.begin(), args.end());
    auto numValuesIn = args.size() + 2; // 2: target & glue
    if (GetCircuit()->IsOptimizedOrFastJit() && hirGate != Circuit::NullGate()) {
        AppendFrameArgs(inputs, hirGate);
        numValuesIn += 1;

        GateRef pcOffset = Int64(acc_.TryGetPcOffset(hirGate));
        inputs.emplace_back(pcOffset);
        numValuesIn += 1;
    }

    const GateMetaData* meta = nullptr;
    if (cs->IsCommonStub()) {
        meta = circuit_->Call(numValuesIn);
    } else if (cs->IsRuntimeVAStub()) {
        meta = circuit_->RuntimeCallWithArgv(numValuesIn);
    } else if (cs->IsRuntimeStub()) {
        meta = circuit_->RuntimeCall(numValuesIn);
    } else if (cs->IsBCDebuggerStub()) {
        meta = circuit_->DebuggerBytecodeCall(numValuesIn);
    } else if (cs->IsBCHandlerStub()) {
        meta = circuit_->BytecodeCall(numValuesIn);
    } else if (cs->IsBuiltinsStub()) {
        meta = circuit_->BuiltinsCall(numValuesIn);
    } else if (cs->IsBuiltinsWithArgvStub()) {
        meta = circuit_->BuiltinsCallWithArgv(numValuesIn);
    } else if (cs->IsRuntimeNGCStub()) {
        meta = circuit_->NoGcRuntimeCall(numValuesIn);
    } else if (cs->IsOptimizedStub()) {
        bool isNoGC = acc_.GetNoGCFlag(hirGate);
        meta = circuit_->CallOptimized(numValuesIn, isNoGC);
    } else if (cs->IsOptimizedFastCallStub()) {
        bool isNoGC = acc_.GetNoGCFlag(hirGate);
        meta = circuit_->FastCallOptimized(numValuesIn, isNoGC);
    } else if (cs->IsBaselineStub()) {
        meta = circuit_->BaselineCall(numValuesIn);
    } else if (cs->IsASMCallBarrierStub()) {
        meta = circuit_->ASMCallBarrier(numValuesIn);
    } else {
        LOG_ECMA(FATAL) << "unknown call operator";
        UNREACHABLE();
    }
    MachineType machineType = cs->GetReturnType().GetMachineType();
    GateType type = cs->GetReturnType().GetGateType();
    GateRef result = GetCircuit()->NewGate(meta, machineType, inputs.size(), inputs.data(), type, comment);
    auto label = GetCurrentLabel();
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallBCHandler(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                      const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedOrFastJit());
    const CallSignature *cs = BytecodeStubCSigns::BCHandler();
    ASSERT(cs->IsBCStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallBuiltin(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                    const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedOrFastJit());
    const CallSignature *cs = BuiltinsStubCSigns::BuiltinsCSign();
    ASSERT(cs->IsBuiltinsStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallBuiltinWithArgv(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                            const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedOrFastJit());
    const CallSignature *cs = BuiltinsStubCSigns::BuiltinsWithArgvCSign();
    ASSERT(cs->IsBuiltinsWithArgvStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallBCDebugger(GateRef glue, GateRef target, const std::vector<GateRef> &args,
                                       const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedOrFastJit());
    const CallSignature *cs = BytecodeStubCSigns::BCDebuggerHandler();
    ASSERT(cs->IsBCDebuggerStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args,
                                    GateRef hirGate, const char* comment)
{
    GateRef target = IntPtr(index);
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
    ASSERT(cs->IsRuntimeStub());
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef filteredHirGate = Circuit::NullGate();
    if (GetCircuit()->IsOptimizedOrFastJit()) {
        ASSERT(hirGate != Circuit::NullGate());
        filteredHirGate = hirGate;
    }
    GateRef result = Call(cs, glue, target, depend, args, filteredHirGate, comment);
    return result;
}

GateRef CircuitBuilder::CallRuntimeVarargs(GateRef glue, int index, GateRef argc, GateRef argv, const char* comment)
{
    ASSERT(!GetCircuit()->IsOptimizedOrFastJit());
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntimeWithArgv));
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    ASSERT(cs->IsRuntimeVAStub());
    GateRef result = Call(cs, glue, target, depend, {argc, argv}, Circuit::NullGate(), comment);
    return result;
}

GateRef CircuitBuilder::CallNGCRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args,
                                       GateRef hirGate, const char* comment)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(index);
    ASSERT(cs->IsRuntimeNGCStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef filteredHirGate = Circuit::NullGate();
    if (GetCircuit()->IsOptimizedOrFastJit() && RuntimeStubCSigns::IsAsmStub(index)) {
        ASSERT(hirGate != Circuit::NullGate());
        filteredHirGate = hirGate;
    }
    GateRef result = Call(cs, glue, target, depend, args, filteredHirGate, comment);
    return result;
}

GateRef CircuitBuilder::CallNGCRuntime(GateRef glue, GateRef gate, int index, const std::vector<GateRef> &args,
                                       bool useLabel)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    if (useLabel) {
        GateRef result = CallNGCRuntime(glue, index, Gate::InvalidGateRef, args, gate, name.c_str());
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(index);
        GateRef target = IntPtr(index);
        GateRef result = Call(cs, glue, target, GetDepend(), args, gate, name.c_str());
        return result;
    }
}

void CircuitBuilder::StartCallTimer(GateRef glue, GateRef gate, const std::vector<GateRef> &args, bool useLabel)
{
    (void)glue;
    (void)gate;
    (void)args;
    (void)useLabel;
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    CallNGCRuntime(glue, gate, RTSTUB_ID(StartCallTimer), args, useLabel);
#endif
}

void CircuitBuilder::EndCallTimer(GateRef glue, GateRef gate, const std::vector<GateRef> &args, bool useLabel)
{
    (void)glue;
    (void)gate;
    (void)args;
    (void)useLabel;
#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    CallNGCRuntime(glue, gate, RTSTUB_ID(EndCallTimer), args, useLabel);
#endif
}

GateRef CircuitBuilder::FastCallOptimized(GateRef glue, GateRef code, GateRef depend, const std::vector<GateRef> &args,
                                          GateRef hirGate)
{
    const CallSignature *cs = RuntimeStubCSigns::GetOptimizedFastCallSign();
    ASSERT(cs->IsOptimizedFastCallStub());
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef filteredHirGate = Circuit::NullGate();
    if (GetCircuit()->IsOptimizedOrFastJit()) {
        ASSERT(hirGate != Circuit::NullGate());
        filteredHirGate = hirGate;
    }
    GateRef result = Call(cs, glue, code, depend, args, filteredHirGate, "fastCallOptimized");
    return result;
}

GateRef CircuitBuilder::CallOptimized(GateRef glue, GateRef code, GateRef depend, const std::vector<GateRef> &args,
                                      GateRef hirGate)
{
    const CallSignature *cs = RuntimeStubCSigns::GetOptimizedCallSign();
    ASSERT(cs->IsOptimizedStub());
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef filteredHirGate = Circuit::NullGate();
    if (GetCircuit()->IsOptimizedOrFastJit()) {
        ASSERT(hirGate != Circuit::NullGate());
        filteredHirGate = hirGate;
    }
    GateRef result = Call(cs, glue, code, depend, args, filteredHirGate, "callOptimized");
    return result;
}

GateRef CircuitBuilder::GetCallBuiltinId(GateRef method)
{
    GateRef extraLiteralInfoOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef extraLiteralInfo = Load(VariableType::INT64(), method, extraLiteralInfoOffset);
    return Int64And(
        Int64LSR(extraLiteralInfo, Int64(MethodLiteral::BuiltinIdBits::START_BIT)),
        Int64((1LU << MethodLiteral::BuiltinIdBits::SIZE) - 1));
}

GateRef CircuitBuilder::CallPrivateGetter(GateRef hirGate, GateRef receiver, GateRef accessor, const char* comment)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);

    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    std::vector<GateRef> args = {currentControl, currentDepend, receiver, accessor};
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(circuit_->CallPrivateGetter(pcOffset),
                                          MachineType::I64,
                                          args.size(),
                                          args.data(),
                                          GateType::AnyType(),
                                          comment);
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::CallPrivateSetter(
    GateRef hirGate, GateRef receiver, GateRef accessor, GateRef value, const char* comment)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);

    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    std::vector<GateRef> args = {currentControl, currentDepend, receiver, accessor, value};
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(circuit_->CallPrivateSetter(pcOffset),
                                          MachineType::I64,
                                          args.size(),
                                          args.data(),
                                          GateType::AnyType(),
                                          comment);
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::CallGetter(GateRef hirGate, GateRef receiver, GateRef holder, GateRef propertyLookupResult,
                                   const char* comment)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);

    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    std::vector<GateRef> args = { currentControl, currentDepend, receiver, propertyLookupResult, holder };
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(circuit_->CallGetter(pcOffset),
                                          MachineType::I64,
                                          args.size(),
                                          args.data(),
                                          GateType::AnyType(),
                                          comment);
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::CallSetter(GateRef hirGate, GateRef receiver, GateRef holder, GateRef propertyLookupResult,
                                   GateRef value, const char* comment)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);

    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    std::vector<GateRef> args = { currentControl, currentDepend, receiver, propertyLookupResult, holder, value };
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(circuit_->CallSetter(pcOffset),
                                          MachineType::I64,
                                          args.size(),
                                          args.data(),
                                          GateType::AnyType(),
                                          comment);
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::Float32ArrayConstructor(GateRef hirGate, std::vector<GateRef> args)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t bitfield = args.size();
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);
    args.insert(args.begin(), currentDepend);
    args.insert(args.begin(), currentControl);
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(circuit_->Float32ArrayConstructor(bitfield, pcOffset),
        MachineType::I64, args.size(), args.data(), GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::Construct(GateRef hirGate, std::vector<GateRef> args)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE || acc_.GetOpCode(hirGate) == OpCode::REFLECT_CONSTRUCT);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t bitfield = args.size();
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    args.insert(args.begin(), currentDepend);
    args.insert(args.begin(), currentControl);
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(circuit_->Construct(bitfield, pcOffset), MachineType::I64,
                                          args.size(), args.data(), GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::CallInternal(GateRef hirGate, std::vector<GateRef> args, uint64_t pcOffset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t bitfield = args.size();
    ASSERT(pcOffset != 0);
    args.insert(args.begin(), currentDepend);
    args.insert(args.begin(), currentControl);
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(
        circuit_->CallInternal(bitfield, pcOffset), MachineType::I64, args.size(), args.data(), GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::CallNew(GateRef hirGate, std::vector<GateRef> args,
                                bool needPushArgv)
{
    ASSERT(acc_.GetOpCode(hirGate) == OpCode::JS_BYTECODE ||
           acc_.GetOpCode(hirGate) == OpCode::FLOAT32_ARRAY_CONSTRUCTOR);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    uint64_t bitfield = args.size();
    uint64_t pcOffset = acc_.TryGetPcOffset(hirGate);
    ASSERT(pcOffset != 0);
    args.insert(args.begin(), currentDepend);
    args.insert(args.begin(), currentControl);
    AppendFrameArgs(args, hirGate);
    auto callGate = GetCircuit()->NewGate(circuit_->CallNew(bitfield, pcOffset, needPushArgv),
                                          MachineType::I64, args.size(), args.data(), GateType::AnyType());
    currentLabel->SetControl(callGate);
    currentLabel->SetDepend(callGate);
    return callGate;
}

GateRef CircuitBuilder::CreateArray(ElementsKind kind, uint32_t arraySize,
                                    GateRef elementsLength, RegionSpaceFlag flag)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    ArrayMetaDataAccessor accessor(kind, ArrayMetaDataAccessor::Mode::CREATE, arraySize, flag);
    GateRef newGate = GetCircuit()->NewGate(circuit_->CreateArray(accessor.ToValue()), MachineType::I64,
                                            { currentControl, currentDepend, elementsLength },
                                            GateType::TaggedValue());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::CreateArrayWithBuffer(ElementsKind kind, ArrayMetaDataAccessor::Mode mode, GateRef cpId,
                                              GateRef constPoolIndex, RegionSpaceFlag flag)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    ArrayMetaDataAccessor accessor(kind, mode, 0, flag);
    GateRef newGate = GetCircuit()->NewGate(circuit_->CreateArrayWithBuffer(accessor.ToValue()),
                                            MachineType::I64,
                                            { currentControl, currentDepend, cpId, constPoolIndex, frameState },
                                            GateType::NJSValue());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::CreateArguments(ElementsKind kind, CreateArgumentsAccessor::Mode mode, GateRef restIdx)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    CreateArgumentsAccessor accessor(kind, mode);
    GateRef newGate = GetCircuit()->NewGate(circuit_->CreateArguments(accessor.ToValue()),
                                            MachineType::I64,
                                            { currentControl, currentDepend, restIdx, frameState },
                                            GateType::NJSValue());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

void CircuitBuilder::SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
    GateRef value, GateRef attrOffset, VariableType type)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef inlinedPropsStart = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    GateRef propOffset = Int32Mul(Int32Add(inlinedPropsStart, attrOffset),
        Int32(JSTaggedValue::TaggedTypeSize()));
    Store(type, glue, obj, ZExtInt32ToPtr(propOffset), value);
}

GateRef CircuitBuilder::StoreModuleVar(GateRef jsFunc, GateRef index, GateRef value)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef newGate = GetCircuit()->NewGate(circuit_->StoreModuleVar(), MachineType::I64,
        { currentControl, currentDepend, jsFunc, index, value }, GateType::TaggedValue());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::LdLocalModuleVar(GateRef jsFunc, GateRef index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef newGate = GetCircuit()->NewGate(circuit_->LdLocalModuleVar(), MachineType::I64,
                                            { currentControl, currentDepend, jsFunc, index}, GateType::TaggedValue());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::BuiltinConstructor(BuiltinsStubCSigns::ID id, GateRef gate)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef newGate = Circuit::NullGate();
    switch (id) {
        case BuiltinsStubCSigns::ID::ArrayConstructor: {
            if (acc_.GetNumValueIn(gate) == 1) {
                newGate = GetCircuit()->NewGate(circuit_->ArrayConstructor(1), MachineType::I64,
                                                { currentControl, currentDepend, acc_.GetValueIn(gate, 0)},
                                                GateType::TaggedValue());
            } else {
                ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: num value in
                newGate = GetCircuit()->NewGate(circuit_->ArrayConstructor(2), MachineType::I64,
                    { currentControl, currentDepend, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)},
                    GateType::TaggedValue());
            }
            break;
        }
        case BuiltinsStubCSigns::ID::ObjectConstructor: {
            if (acc_.GetNumValueIn(gate) == 1) {
                newGate = GetCircuit()->NewGate(circuit_->ObjectConstructor(1), MachineType::I64,
                                                { currentControl, currentDepend, acc_.GetValueIn(gate, 0)},
                                                GateType::TaggedValue());
            } else {
                ASSERT(acc_.GetNumValueIn(gate) >= 2); // 2: num value in
                newGate = GetCircuit()->NewGate(circuit_->ObjectConstructor(2), MachineType::I64,
                    { currentControl, currentDepend, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)},
                    GateType::TaggedValue());
            }
            break;
        }
        case BuiltinsStubCSigns::ID::BooleanConstructor: {
            if (acc_.GetNumValueIn(gate) == 1) {
                newGate = GetCircuit()->NewGate(circuit_->BooleanConstructor(1), MachineType::I64,
                                                { currentControl, currentDepend, acc_.GetValueIn(gate, 0)},
                                                GateType::TaggedValue());
            } else {
                ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: num value in
                newGate = GetCircuit()->NewGate(circuit_->BooleanConstructor(2), MachineType::I64,
                    { currentControl, currentDepend, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)},
                    GateType::TaggedValue());
            }
            break;
        }
        case BuiltinsStubCSigns::ID::Float32ArrayConstructor: {
            if (acc_.GetNumValueIn(gate) == 1) {
                newGate = Float32ArrayConstructor(gate, { acc_.GetValueIn(gate, 0)});
            } else {
                ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: num value in
                newGate = Float32ArrayConstructor(gate, { acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
            }
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

void CircuitBuilder::SetExtensibleToBitfield(GateRef glue, GateRef obj, bool isExtensible)
{
    GateRef jsHclass = LoadHClass(obj);
    GateRef bitfield = Load(VariableType::INT32(), jsHclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    GateRef boolVal = Boolean(isExtensible);
    GateRef boolToInt32 = ZExtInt1ToInt32(boolVal);
    GateRef encodeValue = Int32LSL(boolToInt32, Int32(JSHClass::ExtensibleBit::START_BIT));
    GateRef mask = Int32(((1LU << JSHClass::ExtensibleBit::SIZE) - 1) << JSHClass::ExtensibleBit::START_BIT);
    bitfield = Int32Or(Int32And(bitfield, Int32Not(mask)), encodeValue);
    Store(VariableType::INT32(), glue, jsHclass, IntPtr(JSHClass::BIT_FIELD_OFFSET), bitfield);
}

GateRef CircuitBuilder::OrdinaryHasInstance(GateRef obj, GateRef target)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef frameState = acc_.FindNearestFrameState(currentDepend);
    auto ret = GetCircuit()->NewGate(circuit_->OrdinaryHasInstance(),
                                     MachineType::I64,
                                     {currentControl, currentDepend, obj, target, frameState},
                                     GateType::TaggedNPointer());
    acc_.ReplaceInAfterInsert(currentControl, currentDepend, ret);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::MigrateArrayWithKind(GateRef receiver, GateRef oldElementsKind,
                                             GateRef newElementsKind)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef newGate = GetCircuit()->NewGate(circuit_->MigrateArrayWithKind(), MachineType::I64,
                                            { currentControl, currentDepend, receiver, oldElementsKind,
                                              newElementsKind },
                                            GateType::TaggedValue());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::IsLiteralString(GateRef string)
{
    GateRef objectType = GetObjectType(LoadHClass(string));
    return BitOr(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::LINE_STRING))),
                 Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::CONSTANT_STRING))));
}

// left and right both utf-8 or utf-16 and both linestring or constantstring can be concat.
GateRef CircuitBuilder::CanBeConcat(GateRef leftString, GateRef rightString, GateRef isValidOpt)
{
    return LogicAndBuilder(env_).And(isValidOpt).And(IsLiteralString(leftString))
        .And(IsLiteralString(rightString)).Done();
}

// left and right both utf-8 or utf-16 and right is linestring or constantstring can back store.
GateRef CircuitBuilder::CanBackStore(GateRef rightString, GateRef isValidOpt)
{
    return LogicAndBuilder(env_).And(isValidOpt).And(IsLiteralString(rightString)).Done();
}

GateRef CircuitBuilder::NumberToString(GateRef number)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    GateRef newGate = GetCircuit()->NewGate(circuit_->NumberToString(), MachineType::I64,
                                            { currentControl, currentDepend, number }, GateType::StringType());
    currentLabel->SetControl(newGate);
    currentLabel->SetDepend(newGate);
    return newGate;
}
}
