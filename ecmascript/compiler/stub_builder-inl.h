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

#ifndef ECMASCRIPT_COMPILER_STUB_INL_H
#define ECMASCRIPT_COMPILER_STUB_INL_H

#include "ecmascript/compiler/stub_builder.h"

#include "ecmascript/accessor_data.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/message_string.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript::kungfu {
using JSFunction = panda::ecmascript::JSFunction;
using PropertyBox = panda::ecmascript::PropertyBox;

inline GateRef StubBuilder::Int8(int8_t value)
{
    return env_->GetBuilder()->Int8(value);
}

inline GateRef StubBuilder::Int16(int16_t value)
{
    return env_->GetBuilder()->Int16(value);
}

inline GateRef StubBuilder::Int32(int32_t value)
{
    return env_->GetBuilder()->Int32(value);
};

inline GateRef StubBuilder::Int64(int64_t value)
{
    return env_->GetBuilder()->Int64(value);
}

inline GateRef StubBuilder::StringPtr(const std::string &str)
{
    return env_->GetBuilder()->StringPtr(str);
}

inline GateRef StubBuilder::IntPtr(int64_t value)
{
    return env_->Is32Bit() ? Int32(value) : Int64(value);
};

inline GateRef StubBuilder::IntPtrSize()
{
    return env_->Is32Bit() ? Int32(sizeof(uint32_t)) : Int64(sizeof(uint64_t));
}

inline GateRef StubBuilder::True()
{
    return TruncInt32ToInt1(Int32(1));
}

inline GateRef StubBuilder::False()
{
    return TruncInt32ToInt1(Int32(0));
}

inline GateRef StubBuilder::Boolean(bool value)
{
    return env_->GetBuilder()->Boolean(value);
}

inline GateRef StubBuilder::Double(double value)
{
    return env_->GetBuilder()->Double(value);
}

inline GateRef StubBuilder::Undefined()
{
    return env_->GetBuilder()->UndefineConstant();
}

inline GateRef StubBuilder::Hole()
{
    return env_->GetBuilder()->HoleConstant();
}

inline GateRef StubBuilder::Null()
{
    return env_->GetBuilder()->NullConstant();
}

inline GateRef StubBuilder::Exception()
{
    return env_->GetBuilder()->ExceptionConstant();
}

inline GateRef StubBuilder::RelocatableData(uint64_t value)
{
    return env_->GetBuilder()->RelocatableData(value);
}

// parameter
inline GateRef StubBuilder::Argument(size_t index)
{
    return env_->GetArgument(index);
}

inline GateRef StubBuilder::Int1Argument(size_t index)
{
    return Argument(index);
}

inline GateRef StubBuilder::Int32Argument(size_t index)
{
    return Argument(index);
}

inline GateRef StubBuilder::Int64Argument(size_t index)
{
    return Argument(index);
}

inline GateRef StubBuilder::TaggedArgument(size_t index)
{
    return Argument(index);
}

inline GateRef StubBuilder::TaggedPointerArgument(size_t index)
{
    return Argument(index);
}

inline GateRef StubBuilder::PtrArgument(size_t index)
{
    return Argument(index);
}

inline GateRef StubBuilder::Float32Argument(size_t index)
{
    return Argument(index);
}

inline GateRef StubBuilder::Float64Argument(size_t index)
{
    return Argument(index);
}

inline GateRef StubBuilder::Alloca(int size)
{
    return env_->GetBuilder()->Alloca(size);
}

inline GateRef StubBuilder::Return(GateRef value)
{
    auto control = env_->GetCurrentLabel()->GetControl();
    auto depend = env_->GetCurrentLabel()->GetDepend();
    return env_->GetBuilder()->Return(control, depend, value);
}

inline GateRef StubBuilder::Return()
{
    auto control = env_->GetCurrentLabel()->GetControl();
    auto depend = env_->GetCurrentLabel()->GetDepend();
    return env_->GetBuilder()->ReturnVoid(control, depend);
}

inline void StubBuilder::Bind(Label *label)
{
    env_->GetBuilder()->Bind(label);
}

inline GateRef StubBuilder::CallRuntime(GateRef glue, int index, const std::initializer_list<GateRef>& args)
{
    SavePcIfNeeded(glue);
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    GateRef result = env_->GetBuilder()->CallRuntime(glue, index, Gate::InvalidGateRef, args,
                                                     Circuit::NullGate(), name.c_str());
    return result;
}

inline GateRef StubBuilder::CallRuntime(GateRef glue, int index, GateRef argc, GateRef argv)
{
    SavePcIfNeeded(glue);
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    GateRef result = env_->GetBuilder()->CallRuntimeVarargs(glue, index, argc, argv, name.c_str());
    return result;
}

inline GateRef StubBuilder::CallNGCRuntime(GateRef glue, int index, const std::initializer_list<GateRef>& args)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    GateRef result = env_->GetBuilder()->CallNGCRuntime(glue, index, Gate::InvalidGateRef, args,
                                                        Circuit::NullGate(), name.c_str());
    return result;
}

inline GateRef StubBuilder::FastCallOptimized(GateRef glue, GateRef code, const std::initializer_list<GateRef>& args)
{
    GateRef result = env_->GetBuilder()->FastCallOptimized(glue, code, Gate::InvalidGateRef, args, Circuit::NullGate());
    return result;
}

inline GateRef StubBuilder::CallOptimized(GateRef glue, GateRef code, const std::initializer_list<GateRef>& args)
{
    GateRef result = env_->GetBuilder()->CallOptimized(glue, code, Gate::InvalidGateRef, args, Circuit::NullGate());
    return result;
}

inline GateRef StubBuilder::GetAotCodeAddr(GateRef method)
{
    return env_->GetBuilder()->GetCodeAddr(method);
}

inline GateRef StubBuilder::CallStub(GateRef glue, int index, const std::initializer_list<GateRef>& args)
{
    SavePcIfNeeded(glue);
    const std::string name = CommonStubCSigns::GetName(index);
    GateRef result = env_->GetBuilder()->CallStub(glue, Circuit::NullGate(), index, args, name.c_str());
    return result;
}

inline GateRef StubBuilder::CallBuiltinRuntime(GateRef glue, const std::initializer_list<GateRef>& args,
                                               bool isNew, const char* comment)
{
    GateRef result = env_->GetBuilder()->CallBuiltinRuntime(glue, Gate::InvalidGateRef, args, isNew, comment);
    return result;
}

inline void StubBuilder::DebugPrint(GateRef glue, std::initializer_list<GateRef> args)
{
    CallNGCRuntime(glue, RTSTUB_ID(DebugPrint), args);
}

inline void StubBuilder::FatalPrint(GateRef glue, std::initializer_list<GateRef> args)
{
    CallNGCRuntime(glue, RTSTUB_ID(FatalPrint), args);
}

void StubBuilder::SavePcIfNeeded(GateRef glue)
{
    if (env_->IsAsmInterp()) {
        GateRef sp = Argument(static_cast<size_t>(InterpreterHandlerInputs::SP));
        GateRef pc = Argument(static_cast<size_t>(InterpreterHandlerInputs::PC));
        GateRef frame = PtrSub(sp,
            IntPtr(AsmInterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
        Store(VariableType::INT64(), glue, frame,
            IntPtr(AsmInterpretedFrame::GetPcOffset(GetEnvironment()->IsArch32Bit())), pc);
    }
}

void StubBuilder::SaveJumpSizeIfNeeded(GateRef glue, GateRef jumpSize)
{
    if (env_->IsAsmInterp()) {
        GateRef sp = Argument(static_cast<size_t>(InterpreterHandlerInputs::SP));
        GateRef frame = PtrSub(sp,
            IntPtr(AsmInterpretedFrame::GetSize(GetEnvironment()->IsArch32Bit())));
        Store(VariableType::INT64(), glue, frame,
            IntPtr(AsmInterpretedFrame::GetCallSizeOffset(GetEnvironment()->IsArch32Bit())), jumpSize);
    }
}

void StubBuilder::SetHotnessCounter(GateRef glue, GateRef method, GateRef value)
{
    auto env = GetEnvironment();
    GateRef newValue = env->GetBuilder()->TruncInt64ToInt16(value);
    Store(VariableType::INT16(), glue, method, IntPtr(Method::LITERAL_INFO_OFFSET), newValue);
}

void StubBuilder::SaveHotnessCounterIfNeeded(GateRef glue, GateRef sp, GateRef hotnessCounter, JSCallMode mode)
{
    if (env_->IsAsmInterp() && kungfu::AssemblerModule::IsJumpToCallCommonEntry(mode)) {
        ASSERT(hotnessCounter != Circuit::NullGate());
        GateRef frame = PtrSub(sp, IntPtr(AsmInterpretedFrame::GetSize(env_->IsArch32Bit())));
        GateRef function = Load(VariableType::JS_POINTER(), frame,
            IntPtr(AsmInterpretedFrame::GetFunctionOffset(env_->IsArch32Bit())));
        GateRef method = Load(VariableType::JS_ANY(), function, IntPtr(JSFunctionBase::METHOD_OFFSET));
        SetHotnessCounter(glue, method, hotnessCounter);
    }
}

// memory
inline GateRef StubBuilder::Load(VariableType type, GateRef base, GateRef offset)
{
    if (type == VariableType::NATIVE_POINTER()) {
        type = env_->IsArch64Bit() ? VariableType::INT64() : VariableType::INT32();
    }
    return env_->GetBuilder()->Load(type, base, offset);
}

inline GateRef StubBuilder::Load(VariableType type, GateRef base)
{
    return Load(type, base, IntPtr(0));
}

// arithmetic
inline GateRef StubBuilder::Int16Add(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int16Add(x, y);
}

inline GateRef StubBuilder::Int32Add(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Add(x, y);
}

inline GateRef StubBuilder::Int64Add(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64Add(x, y);
}

inline GateRef StubBuilder::DoubleAdd(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleAdd(x, y);
}

inline GateRef StubBuilder::PtrMul(GateRef x, GateRef y)
{
    return env_->GetBuilder()->PtrMul(x, y);
}

inline GateRef StubBuilder::PtrAdd(GateRef x, GateRef y)
{
    return env_->GetBuilder()->PtrAdd(x, y);
}

inline GateRef StubBuilder::PtrSub(GateRef x, GateRef y)
{
    return env_->GetBuilder()->PtrSub(x, y);
}

inline GateRef StubBuilder::IntPtrAnd(GateRef x, GateRef y)
{
    return env_->Is32Bit() ? Int32And(x, y) : Int64And(x, y);
}

inline GateRef StubBuilder::IntPtrEqual(GateRef x, GateRef y)
{
    if (env_->Is32Bit()) {
        return Int32Equal(x, y);
    }
    return Int64Equal(x, y);
}

inline GateRef StubBuilder::Int16Sub(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int16Sub(x, y);
}

inline GateRef StubBuilder::Int32Sub(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Sub(x, y);
}

inline GateRef StubBuilder::Int64Sub(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64Sub(x, y);
}

inline GateRef StubBuilder::DoubleSub(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleSub(x, y);
}

inline GateRef StubBuilder::Int32Mul(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Mul(x, y);
}

inline GateRef StubBuilder::Int64Mul(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64Mul(x, y);
}

inline GateRef StubBuilder::DoubleMul(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleMul(x, y);
}

inline GateRef StubBuilder::DoubleDiv(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleDiv(x, y);
}

inline GateRef StubBuilder::Int32Div(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Div(x, y);
}

inline GateRef StubBuilder::Int64Div(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64Div(x, y);
}

inline GateRef StubBuilder::IntPtrDiv(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrDiv(x, y);
}

inline GateRef StubBuilder::Int32Mod(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Mod(x, y);
}

inline GateRef StubBuilder::DoubleMod(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleMod(x, y);
}

// bit operation
inline GateRef StubBuilder::Int32Or(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Or(x, y);
}

inline GateRef StubBuilder::Int8And(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int8And(x, y);
}

inline GateRef StubBuilder::Int32And(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32And(x, y);
}

inline GateRef StubBuilder::BoolAnd(GateRef x, GateRef y)
{
    return env_->GetBuilder()->BoolAnd(x, y);
}

inline GateRef StubBuilder::BoolOr(GateRef x, GateRef y)
{
    return env_->GetBuilder()->BoolOr(x, y);
}

inline GateRef StubBuilder::Int32Not(GateRef x)
{
    return env_->GetBuilder()->Int32Not(x);
}

inline GateRef StubBuilder::IntPtrNot(GateRef x)
{
    return env_->Is32Bit() ? Int32Not(x) : Int64Not(x);
}

inline GateRef StubBuilder::BoolNot(GateRef x)
{
    return env_->GetBuilder()->BoolNot(x);
}

inline GateRef StubBuilder::Int64Or(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64Or(x, y);
}

inline GateRef StubBuilder::IntPtrOr(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrOr(x, y);
}

inline GateRef StubBuilder::Int64And(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64And(x, y);
}

inline GateRef StubBuilder::Int16LSL(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int16LSL(x, y);
}

inline GateRef StubBuilder::Int64Xor(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64Xor(x, y);
}

inline GateRef StubBuilder::Int32Xor(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Xor(x, y);
}

inline GateRef StubBuilder::Int8LSR(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int8LSR(x, y);
}

inline GateRef StubBuilder::Int64Not(GateRef x)
{
    return env_->GetBuilder()->Int64Not(x);
}

inline GateRef StubBuilder::Int32LSL(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32LSL(x, y);
}

inline GateRef StubBuilder::Int64LSL(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64LSL(x, y);
}

inline GateRef StubBuilder::IntPtrLSL(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrLSL(x, y);
}

inline GateRef StubBuilder::Int32ASR(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32ASR(x, y);
}

inline GateRef StubBuilder::Int32LSR(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32LSR(x, y);
}

inline GateRef StubBuilder::Int64LSR(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64LSR(x, y);
}

inline GateRef StubBuilder::IntPtrLSR(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrLSR(x, y);
}

template<OpCode Op, MachineType Type>
inline GateRef StubBuilder::BinaryOp(GateRef x, GateRef y)
{
    return env_->GetBuilder()->BinaryOp<Op, Type>(x, y);
}

template<OpCode Op, MachineType Type>
inline GateRef StubBuilder::BinaryOpWithOverflow(GateRef x, GateRef y)
{
    return env_->GetBuilder()->BinaryOpWithOverflow<Op, Type>(x, y);
}

inline GateRef StubBuilder::TaggedIsInt(GateRef x)
{
    return env_->GetBuilder()->TaggedIsInt(x);
}

inline GateRef StubBuilder::TaggedIsDouble(GateRef x)
{
    return BoolAnd(TaggedIsNumber(x), BoolNot(TaggedIsInt(x)));
}

inline GateRef StubBuilder::TaggedIsObject(GateRef x)
{
    return env_->GetBuilder()->TaggedIsObject(x);
}

inline GateRef StubBuilder::TaggedIsString(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsString(obj);
}

inline GateRef StubBuilder::TaggedIsStringOrSymbol(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsStringOrSymbol(obj);
}

inline GateRef StubBuilder::BothAreString(GateRef x, GateRef y)
{
    auto allHeapObject = BoolAnd(TaggedIsHeapObject(x), TaggedIsHeapObject(y));
    auto allString = env_->GetBuilder()->TaggedObjectBothAreString(x, y);
    return env_->GetBuilder()->LogicAnd(allHeapObject, allString);
}

inline GateRef StubBuilder::TaggedIsNumber(GateRef x)
{
    return BoolNot(TaggedIsObject(x));
}

inline GateRef StubBuilder::TaggedIsNumeric(GateRef x)
{
    return BoolOr(TaggedIsNumber(x), TaggedIsBigInt(x));
}

inline GateRef StubBuilder::TaggedIsHole(GateRef x)
{
    return env_->GetBuilder()->TaggedIsHole(x);
}

inline GateRef StubBuilder::TaggedIsNotHole(GateRef x)
{
    return env_->GetBuilder()->TaggedIsNotHole(x);
}

inline GateRef StubBuilder::TaggedIsUndefined(GateRef x)
{
    return env_->GetBuilder()->TaggedIsUndefined(x);
}

inline GateRef StubBuilder::TaggedIsException(GateRef x)
{
    return env_->GetBuilder()->TaggedIsException(x);
}

inline GateRef StubBuilder::TaggedIsSpecial(GateRef x)
{
    return env_->GetBuilder()->TaggedIsSpecial(x);
}

inline GateRef StubBuilder::TaggedIsHeapObject(GateRef x)
{
    return env_->GetBuilder()->TaggedIsHeapObject(x);
}

inline GateRef StubBuilder::TaggedIsGeneratorObject(GateRef x)
{
    return env_->GetBuilder()->TaggedIsGeneratorObject(x);
}

inline GateRef StubBuilder::TaggedIsJSArray(GateRef x)
{
    return env_->GetBuilder()->TaggedIsJSArray(x);
}

inline GateRef StubBuilder::TaggedIsAsyncGeneratorObject(GateRef x)
{
    return env_->GetBuilder()->TaggedIsAsyncGeneratorObject(x);
}

inline GateRef StubBuilder::TaggedIsJSGlobalObject(GateRef x)
{
    return env_->GetBuilder()->TaggedIsJSGlobalObject(x);
}

inline GateRef StubBuilder::TaggedIsWeak(GateRef x)
{
    return env_->GetBuilder()->TaggedIsWeak(x);
}

inline GateRef StubBuilder::TaggedIsPrototypeHandler(GateRef x)
{
    return env_->GetBuilder()->TaggedIsPrototypeHandler(x);
}

inline GateRef StubBuilder::TaggedIsStoreTSHandler(GateRef x)
{
    return env_->GetBuilder()->TaggedIsStoreTSHandler(x);
}

inline GateRef StubBuilder::TaggedIsTransWithProtoHandler(GateRef x)
{
    return env_->GetBuilder()->TaggedIsTransWithProtoHandler(x);
}

inline GateRef StubBuilder::TaggedIsTransitionHandler(GateRef x)
{
    return env_->GetBuilder()->TaggedIsTransitionHandler(x);
}

inline GateRef StubBuilder::GetNextPositionForHash(GateRef last, GateRef count, GateRef size)
{
    auto nextOffset = Int32LSR(Int32Mul(count, Int32Add(count, Int32(1))),
                               Int32(1));
    return Int32And(Int32Add(last, nextOffset), Int32Sub(size, Int32(1)));
}

inline GateRef StubBuilder::DoubleIsNAN(GateRef x)
{
    return env_->GetBuilder()->DoubleIsNAN(x);
}

inline GateRef StubBuilder::DoubleIsINF(GateRef x)
{
    return env_->GetBuilder()->DoubleIsINF(x);
}

inline GateRef StubBuilder::TaggedIsNull(GateRef x)
{
    return env_->GetBuilder()->TaggedIsNull(x);
}

inline GateRef StubBuilder::TaggedIsUndefinedOrNull(GateRef x)
{
    return env_->GetBuilder()->TaggedIsUndefinedOrNull(x);
}

inline GateRef StubBuilder::TaggedIsTrue(GateRef x)
{
    return env_->GetBuilder()->TaggedIsTrue(x);
}

inline GateRef StubBuilder::TaggedIsFalse(GateRef x)
{
    return env_->GetBuilder()->TaggedIsFalse(x);
}

inline GateRef StubBuilder::TaggedIsBoolean(GateRef x)
{
    return env_->GetBuilder()->TaggedIsBoolean(x);
}

inline GateRef StubBuilder::TaggedGetInt(GateRef x)
{
    return env_->GetBuilder()->TaggedGetInt(x);
}

inline GateRef StubBuilder::Int8ToTaggedInt(GateRef x)
{
    GateRef val = SExtInt8ToInt64(x);
    return env_->GetBuilder()->ToTaggedInt(val);
}

inline GateRef StubBuilder::Int16ToTaggedInt(GateRef x)
{
    GateRef val = SExtInt16ToInt64(x);
    return env_->GetBuilder()->ToTaggedInt(val);
}

inline GateRef StubBuilder::IntToTaggedPtr(GateRef x)
{
    GateRef val = SExtInt32ToInt64(x);
    return env_->GetBuilder()->ToTaggedIntPtr(val);
}

inline GateRef StubBuilder::IntToTaggedInt(GateRef x)
{
    GateRef val = SExtInt32ToInt64(x);
    return env_->GetBuilder()->ToTaggedInt(val);
}

inline GateRef StubBuilder::Int64ToTaggedInt(GateRef x)
{
    return env_->GetBuilder()->ToTaggedInt(x);
}

inline GateRef StubBuilder::DoubleToTaggedDoublePtr(GateRef x)
{
    return env_->GetBuilder()->DoubleToTaggedDoublePtr(x);
}

inline GateRef StubBuilder::TaggedPtrToTaggedDoublePtr(GateRef x)
{
    return DoubleToTaggedDoublePtr(CastInt64ToFloat64(ChangeTaggedPointerToInt64(x)));
}

inline GateRef StubBuilder::TaggedPtrToTaggedIntPtr(GateRef x)
{
    return IntToTaggedPtr(TruncInt64ToInt32(ChangeTaggedPointerToInt64(x)));
}

inline GateRef StubBuilder::CastDoubleToInt64(GateRef x)
{
    return env_->GetBuilder()->CastDoubleToInt64(x);
}

inline GateRef StubBuilder::TaggedTrue()
{
    return env_->GetBuilder()->TaggedTrue();
}

inline GateRef StubBuilder::TaggedFalse()
{
    return env_->GetBuilder()->TaggedFalse();
}

inline GateRef StubBuilder::TaggedUndefined()
{
    return env_->GetBuilder()->UndefineConstant();
}

// compare operation
inline GateRef StubBuilder::Int8Equal(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int8Equal(x, y);
}

inline GateRef StubBuilder::Equal(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Equal(x, y);
}

inline GateRef StubBuilder::Int32Equal(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Equal(x, y);
}

inline GateRef StubBuilder::Int32NotEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32NotEqual(x, y);
}

inline GateRef StubBuilder::Int64Equal(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64Equal(x, y);
}

inline GateRef StubBuilder::DoubleEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleEqual(x, y);
}

inline GateRef StubBuilder::DoubleNotEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleNotEqual(x, y);
}

inline GateRef StubBuilder::DoubleLessThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleLessThan(x, y);
}

inline GateRef StubBuilder::DoubleLessThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleLessThanOrEqual(x, y);
}

inline GateRef StubBuilder::DoubleGreaterThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleGreaterThan(x, y);
}

inline GateRef StubBuilder::DoubleGreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->DoubleGreaterThanOrEqual(x, y);
}

inline GateRef StubBuilder::Int64NotEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64NotEqual(x, y);
}

inline GateRef StubBuilder::Int32GreaterThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32GreaterThan(x, y);
}

inline GateRef StubBuilder::Int32LessThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32LessThan(x, y);
}

inline GateRef StubBuilder::Int32GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32GreaterThanOrEqual(x, y);
}

inline GateRef StubBuilder::Int32LessThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32LessThanOrEqual(x, y);
}

inline GateRef StubBuilder::Int32UnsignedGreaterThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32UnsignedGreaterThan(x, y);
}

inline GateRef StubBuilder::Int32UnsignedLessThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32UnsignedLessThan(x, y);
}

inline GateRef StubBuilder::Int32UnsignedGreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32UnsignedGreaterThanOrEqual(x, y);
}

inline GateRef StubBuilder::Int32UnsignedLessThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32UnsignedLessThanOrEqual(x, y);
}

inline GateRef StubBuilder::Int64GreaterThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64GreaterThan(x, y);
}

inline GateRef StubBuilder::Int64LessThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64LessThan(x, y);
}

inline GateRef StubBuilder::Int64LessThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64LessThanOrEqual(x, y);
}

inline GateRef StubBuilder::Int64GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64GreaterThanOrEqual(x, y);
}

inline GateRef StubBuilder::Int64UnsignedLessThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64UnsignedLessThanOrEqual(x, y);
}

inline GateRef StubBuilder::IntPtrGreaterThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrGreaterThan(x, y);
}

// cast operation
inline GateRef StubBuilder::TruncInt16ToInt8(GateRef val)
{
    return env_->GetBuilder()->TruncInt16ToInt8(val);
}

inline GateRef StubBuilder::ChangeInt64ToIntPtr(GateRef val)
{
    if (env_->IsArch32Bit()) {
        return TruncInt64ToInt32(val);
    }
    return val;
}

inline GateRef StubBuilder::ZExtInt32ToPtr(GateRef val)
{
    if (env_->IsArch32Bit()) {
        return val;
    }
    return ZExtInt32ToInt64(val);
}

inline GateRef StubBuilder::ChangeIntPtrToInt32(GateRef val)
{
    if (env_->IsArch32Bit()) {
        return val;
    }
    return TruncInt64ToInt32(val);
}

inline GateRef StubBuilder::GetSetterFromAccessor(GateRef accessor)
{
    GateRef setterOffset = IntPtr(AccessorData::SETTER_OFFSET);
    return Load(VariableType::JS_ANY(), accessor, setterOffset);
}

inline GateRef StubBuilder::GetElementsArray(GateRef object)
{
    return env_->GetBuilder()->GetElementsArray(object);
}

inline void StubBuilder::SetElementsArray(VariableType type, GateRef glue, GateRef object, GateRef elementsArray)
{
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    Store(type, glue, object, elementsOffset, elementsArray);
}

inline GateRef StubBuilder::GetPropertiesArray(GateRef object)
{
    GateRef propertiesOffset = IntPtr(JSObject::PROPERTIES_OFFSET);
    return Load(VariableType::JS_POINTER(), object, propertiesOffset);
}

// SetProperties in js_object.h
inline void StubBuilder::SetPropertiesArray(VariableType type, GateRef glue, GateRef object, GateRef propsArray)
{
    GateRef propertiesOffset = IntPtr(JSObject::PROPERTIES_OFFSET);
    Store(type, glue, object, propertiesOffset, propsArray);
}

inline void StubBuilder::SetHash(GateRef glue, GateRef object, GateRef hash)
{
    GateRef hashOffset = IntPtr(ECMAObject::HASH_OFFSET);
    Store(VariableType::INT64(), glue, object, hashOffset, hash);
}

inline GateRef StubBuilder::GetLengthOfTaggedArray(GateRef array)
{
    return Load(VariableType::INT32(), array, IntPtr(TaggedArray::LENGTH_OFFSET));
}

inline GateRef StubBuilder::IsJSHClass(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsJSHClass), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->IsJSHClass(obj);
    EXITENTRY();
    return res;
}

// object operation
inline GateRef StubBuilder::LoadHClass(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(LoadHClass), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->LoadHClass(object);
    EXITENTRY();
    return res;
}

inline void StubBuilder::StoreHClass(GateRef glue, GateRef object, GateRef hClass)
{
    return env_->GetBuilder()->StoreHClass(glue, object, hClass);
}

inline GateRef StubBuilder::GetObjectType(GateRef hClass)
{
    return env_->GetBuilder()->GetObjectType(hClass);
}

inline GateRef StubBuilder::IsDictionaryMode(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsDictionaryMode), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->IsDictionaryMode(object);
    EXITENTRY();
    return res;
}

inline GateRef StubBuilder::IsDictionaryModeByHClass(GateRef hClass)
{
    return env_->GetBuilder()->IsDictionaryModeByHClass(hClass);
}

inline GateRef StubBuilder::IsDictionaryElement(GateRef hClass)
{
    return env_->GetBuilder()->IsDictionaryElement(hClass);
}

inline GateRef StubBuilder::IsClassConstructorFromBitField(GateRef bitfield)
{
    // decode
    return env_->GetBuilder()->IsClassConstructorWithBitField(bitfield);
}

inline GateRef StubBuilder::IsClassConstructor(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsClassConstructor), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->IsClassConstructor(object);
    EXITENTRY();
    return res;
}

inline GateRef StubBuilder::IsClassPrototype(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsClassPrototype), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->IsClassPrototype(object);
    EXITENTRY();
    return res;
}

inline GateRef StubBuilder::IsExtensible(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsExtensible), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->IsExtensible(object);
    EXITENTRY();
    return res;
}

inline GateRef StubBuilder::TaggedObjectIsEcmaObject(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsEcmaObject), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->TaggedObjectIsEcmaObject(obj);
    EXITENTRY();
    return res;
}

inline GateRef StubBuilder::IsEcmaObject(GateRef obj)
{
    auto isHeapObject = TaggedIsHeapObject(obj);
    return env_->GetBuilder()->LogicAnd(isHeapObject, TaggedObjectIsEcmaObject(obj));
}

inline GateRef StubBuilder::IsJSObject(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsJSObject), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->IsJSObject(obj);
    EXITENTRY();
    return res;
}

inline GateRef StubBuilder::IsJSFunctionBase(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    GateRef greater = Int32GreaterThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_BASE)));
    GateRef less = Int32LessThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_BOUND_FUNCTION)));
    return BoolAnd(greater, less);
}

inline GateRef StubBuilder::IsConstructor(GateRef object)
{
    GateRef hClass = LoadHClass(object);
    GateRef bitfieldOffset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), hClass, bitfieldOffset);
    // decode
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::ConstructorBit::START_BIT)),
                 Int32((1LU << JSHClass::ConstructorBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsBase(GateRef func)
{
    return env_->GetBuilder()->IsBase(func);
}

inline GateRef StubBuilder::IsSymbol(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
}

inline GateRef StubBuilder::IsString(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsJSObject), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->TaggedObjectIsString(obj);
    EXITENTRY();
    return res;
}

inline GateRef StubBuilder::IsLineString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::LINE_STRING)));
}

inline GateRef StubBuilder::IsConstantString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::CONSTANT_STRING)));
}

inline GateRef StubBuilder::IsTreeString(GateRef obj)
{
    return env_->GetBuilder()->IsTreeString(obj);
}

inline GateRef StubBuilder::TreeStringIsFlat(GateRef string)
{
    return env_->GetBuilder()->TreeStringIsFlat(string);
}

inline GateRef StubBuilder::TaggedObjectIsBigInt(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::BIGINT)));
}

inline GateRef StubBuilder::IsJsProxy(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_PROXY)));
}

inline GateRef StubBuilder::IsJsArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_ARRAY)));
}

inline GateRef StubBuilder::IsByteArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::BYTE_ARRAY)));
}

inline GateRef StubBuilder::IsJSAPIVector(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_VECTOR)));
}

inline GateRef StubBuilder::IsJSAPIStack(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_STACK)));
}

inline GateRef StubBuilder::IsJSAPIPlainArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_PLAIN_ARRAY)));
}

inline GateRef StubBuilder::IsJSAPIQueue(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_QUEUE)));
}

inline GateRef StubBuilder::IsJSAPIDeque(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_DEQUE)));
}

inline GateRef StubBuilder::IsJSAPILightWeightMap(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_LIGHT_WEIGHT_MAP)));
}

inline GateRef StubBuilder::IsJSAPILightWeightSet(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_LIGHT_WEIGHT_SET)));
}

inline GateRef StubBuilder::IsLinkedNode(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::LINKED_NODE)));
}

inline GateRef StubBuilder::IsJSAPIHashMap(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_HASH_MAP)));
}

inline GateRef StubBuilder::IsJSAPIHashSet(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_HASH_SET)));
}

inline GateRef StubBuilder::IsJSAPILinkedList(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_LINKED_LIST)));
}

inline GateRef StubBuilder::IsJSAPIList(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_LIST)));
}

inline GateRef StubBuilder::IsJSAPIArrayList(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_API_ARRAY_LIST)));
}

inline GateRef StubBuilder::IsJSObjectType(GateRef obj, JSType jsType)
{
    GateRef isHeapObject = TaggedIsHeapObject(obj);
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return env_->GetBuilder()->LogicAnd(isHeapObject, Int32Equal(objectType, Int32(static_cast<int32_t>(jsType))));
}

inline GateRef StubBuilder::GetTarget(GateRef proxyObj)
{
    GateRef offset = IntPtr(JSProxy::TARGET_OFFSET);
    return Load(VariableType::JS_ANY(), proxyObj, offset);
}

inline GateRef StubBuilder::IsJsCOWArray(GateRef obj)
{
    // Elements of JSArray are shared and properties are not yet.
    GateRef elements = GetElementsArray(obj);
    GateRef objectType = GetObjectType(LoadHClass(elements));
    return env_->GetBuilder()->IsCOWArray(objectType);
}

inline GateRef StubBuilder::IsWritable(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            Int32LSR(attr, Int32(PropertyAttributes::WritableField::START_BIT)),
            Int32((1LLU << PropertyAttributes::WritableField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsAccessor(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(PropertyAttributes::IsAccessorField::START_BIT)),
            Int32((1LLU << PropertyAttributes::IsAccessorField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsInlinedProperty(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(PropertyAttributes::IsInlinedPropsField::START_BIT)),
            Int32((1LLU << PropertyAttributes::IsInlinedPropsField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::GetProtoCell(GateRef object)
{
    GateRef protoCellOffset = IntPtr(PrototypeHandler::PROTO_CELL_OFFSET);
    return Load(VariableType::JS_POINTER(), object, protoCellOffset);
}

inline GateRef StubBuilder::GetPrototypeHandlerHolder(GateRef object)
{
    GateRef holderOffset = IntPtr(PrototypeHandler::HOLDER_OFFSET);
    return Load(VariableType::JS_ANY(), object, holderOffset);
}

inline GateRef StubBuilder::GetPrototypeHandlerHandlerInfo(GateRef object)
{
    GateRef handlerInfoOffset = IntPtr(PrototypeHandler::HANDLER_INFO_OFFSET);
    return Load(VariableType::JS_ANY(), object, handlerInfoOffset);
}

inline GateRef StubBuilder::GetStoreTSHandlerHolder(GateRef object)
{
    GateRef holderOffset = IntPtr(StoreTSHandler::HOLDER_OFFSET);
    return Load(VariableType::JS_ANY(), object, holderOffset);
}

inline GateRef StubBuilder::GetStoreTSHandlerHandlerInfo(GateRef object)
{
    GateRef handlerInfoOffset = IntPtr(StoreTSHandler::HANDLER_INFO_OFFSET);
    return Load(VariableType::JS_ANY(), object, handlerInfoOffset);
}

inline GateRef StubBuilder::GetHasChanged(GateRef object)
{
    GateRef bitfieldOffset = IntPtr(ProtoChangeMarker::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), object, bitfieldOffset);
    GateRef mask = Int32(1LLU << (ProtoChangeMarker::HAS_CHANGED_BITS - 1));
    return Int32NotEqual(Int32And(bitfield, mask), Int32(0));
}

inline GateRef StubBuilder::HclassIsPrototypeHandler(GateRef hClass)
{
    return Int32Equal(GetObjectType(hClass),
        Int32(static_cast<int32_t>(JSType::PROTOTYPE_HANDLER)));
}

inline GateRef StubBuilder::HclassIsTransitionHandler(GateRef hClass)
{
    return Int32Equal(GetObjectType(hClass),
        Int32(static_cast<int32_t>(JSType::TRANSITION_HANDLER)));
}

inline GateRef StubBuilder::HclassIsPropertyBox(GateRef hClass)
{
    return Int32Equal(GetObjectType(hClass),
        Int32(static_cast<int32_t>(JSType::PROPERTY_BOX)));
}

inline GateRef StubBuilder::IsField(GateRef attr)
{
    return Int32Equal(
        Int32And(
            Int32LSR(attr, Int32(HandlerBase::KindBit::START_BIT)),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::FIELD));
}

inline GateRef StubBuilder::IsNonExist(GateRef attr)
{
    return Int32Equal(
        Int32And(
            Int32LSR(attr, Int32(HandlerBase::KindBit::START_BIT)),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::NON_EXIST));
}

inline GateRef StubBuilder::HandlerBaseIsAccessor(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(HandlerBase::AccessorBit::START_BIT)),
            Int32((1LLU << HandlerBase::AccessorBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::HandlerBaseIsJSArray(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(HandlerBase::IsJSArrayBit::START_BIT)),
            Int32((1LLU << HandlerBase::IsJSArrayBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::HandlerBaseIsInlinedProperty(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(HandlerBase::InlinedPropsBit::START_BIT)),
            Int32((1LLU << HandlerBase::InlinedPropsBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::HandlerBaseGetOffset(GateRef attr)
{
    return Int32And(Int32LSR(attr,
        Int32(HandlerBase::OffsetBit::START_BIT)),
        Int32((1LLU << HandlerBase::OffsetBit::SIZE) - 1));
}


inline GateRef StubBuilder::HandlerBaseGetAttrIndex(GateRef attr)
{
    return Int32And(Int32LSR(attr,
        Int32(HandlerBase::AttrIndexBit::START_BIT)),
        Int32((1LLU << HandlerBase::AttrIndexBit::SIZE) - 1));
}

inline GateRef StubBuilder::HandlerBaseGetRep(GateRef attr)
{
    return Int32And(Int32LSR(attr, Int32(HandlerBase::RepresentationBit::START_BIT)),
        Int32((1LLU << HandlerBase::RepresentationBit::SIZE) - 1));
}

inline GateRef StubBuilder::IsInternalAccessor(GateRef attr)
{
    return Int32NotEqual(
        Int32And(Int32LSR(attr,
            Int32(HandlerBase::InternalAccessorBit::START_BIT)),
            Int32((1LLU << HandlerBase::InternalAccessorBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsInvalidPropertyBox(GateRef obj)
{
    GateRef valueOffset = IntPtr(PropertyBox::VALUE_OFFSET);
    GateRef value = Load(VariableType::JS_ANY(), obj, valueOffset);
    return TaggedIsHole(value);
}

inline GateRef StubBuilder::GetValueFromPropertyBox(GateRef obj)
{
    GateRef valueOffset = IntPtr(PropertyBox::VALUE_OFFSET);
    return Load(VariableType::JS_ANY(), obj, valueOffset);
}

inline void StubBuilder::SetValueToPropertyBox(GateRef glue, GateRef obj, GateRef value)
{
    GateRef valueOffset = IntPtr(PropertyBox::VALUE_OFFSET);
    Store(VariableType::JS_ANY(), glue, obj, valueOffset, value);
}

inline GateRef StubBuilder::GetTransitionHClass(GateRef obj)
{
    GateRef transitionHClassOffset = IntPtr(TransitionHandler::TRANSITION_HCLASS_OFFSET);
    return Load(VariableType::JS_POINTER(), obj, transitionHClassOffset);
}

inline GateRef StubBuilder::GetTransitionHandlerInfo(GateRef obj)
{
    GateRef handlerInfoOffset = IntPtr(TransitionHandler::HANDLER_INFO_OFFSET);
    return Load(VariableType::JS_ANY(), obj, handlerInfoOffset);
}

inline GateRef StubBuilder::GetTransWithProtoHClass(GateRef obj)
{
    GateRef transitionHClassOffset = IntPtr(TransWithProtoHandler::TRANSITION_HCLASS_OFFSET);
    return Load(VariableType::JS_POINTER(), obj, transitionHClassOffset);
}

inline GateRef StubBuilder::GetTransWithProtoHandlerInfo(GateRef obj)
{
    GateRef handlerInfoOffset = IntPtr(TransWithProtoHandler::HANDLER_INFO_OFFSET);
    return Load(VariableType::JS_ANY(), obj, handlerInfoOffset);
}

inline GateRef StubBuilder::PropAttrGetOffset(GateRef attr)
{
    return Int32And(
        Int32LSR(attr, Int32(PropertyAttributes::OffsetField::START_BIT)),
        Int32((1LLU << PropertyAttributes::OffsetField::SIZE) - 1));
}

// SetDictionaryOrder func in property_attribute.h
inline GateRef StubBuilder::SetDictionaryOrderFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int32LSL(
        Int32((1LLU << PropertyAttributes::DictionaryOrderField::SIZE) - 1),
        Int32(PropertyAttributes::DictionaryOrderField::START_BIT));
    GateRef newVal = Int32Or(Int32And(attr, Int32Not(mask)),
        Int32LSL(value, Int32(PropertyAttributes::DictionaryOrderField::START_BIT)));
    return newVal;
}

inline GateRef StubBuilder::GetPrototypeFromHClass(GateRef hClass)
{
    GateRef protoOffset = IntPtr(JSHClass::PROTOTYPE_OFFSET);
    return Load(VariableType::JS_ANY(), hClass, protoOffset);
}

inline GateRef StubBuilder::GetLayoutFromHClass(GateRef hClass)
{
    GateRef attrOffset = IntPtr(JSHClass::LAYOUT_OFFSET);
    return Load(VariableType::JS_POINTER(), hClass, attrOffset);
}

inline GateRef StubBuilder::GetBitFieldFromHClass(GateRef hClass)
{
    GateRef offset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    return Load(VariableType::INT32(), hClass, offset);
}

inline GateRef StubBuilder::GetLengthFromString(GateRef value)
{
    GateRef len = Load(VariableType::INT32(), value, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32LSR(len, Int32(2));  // 2 : 2 means len must be right shift 2 bits
}

inline GateRef StubBuilder::GetFirstFromTreeString(GateRef string)
{
    return env_->GetBuilder()->GetFirstFromTreeString(string);
}

inline GateRef StubBuilder::GetSecondFromTreeString(GateRef string)
{
    return env_->GetBuilder()->GetSecondFromTreeString(string);
}

inline GateRef StubBuilder::GetIsAllTaggedPropFromHClass(GateRef hclass)
{
    GateRef bitfield = Load(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    return Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsAllTaggedPropBit::START_BIT)),
        Int32((1LLU << JSHClass::IsAllTaggedPropBit::SIZE) - 1));
}

inline void StubBuilder::SetBitFieldToHClass(GateRef glue, GateRef hClass, GateRef bitfield)
{
    GateRef offset = IntPtr(JSHClass::BIT_FIELD_OFFSET);
    Store(VariableType::INT32(), glue, hClass, offset, bitfield);
}

inline void StubBuilder::SetIsAllTaggedProp(GateRef glue, GateRef hclass, GateRef hasRep)
{
    GateRef bitfield1 = Load(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef mask = Int32LSL(
        Int32((1LU << JSHClass::IsAllTaggedPropBit::SIZE) - 1),
        Int32(JSHClass::IsAllTaggedPropBit::START_BIT));
    GateRef newVal = Int32Or(Int32And(bitfield1, Int32Not(mask)),
        Int32LSL(hasRep, Int32(JSHClass::IsAllTaggedPropBit::START_BIT)));
    Store(VariableType::INT32(), glue, hclass, IntPtr(JSHClass::BIT_FIELD1_OFFSET), newVal);
}

inline void StubBuilder::SetPrototypeToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef proto)
{
    GateRef offset = IntPtr(JSHClass::PROTOTYPE_OFFSET);
    Store(type, glue, hClass, offset, proto);
}

inline void StubBuilder::SetProtoChangeDetailsToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef protoChange)
{
    GateRef offset = IntPtr(JSHClass::PROTO_CHANGE_DETAILS_OFFSET);
    Store(type, glue, hClass, offset, protoChange);
}

inline void StubBuilder::SetLayoutToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef attr)
{
    GateRef offset = IntPtr(JSHClass::LAYOUT_OFFSET);
    Store(type, glue, hClass, offset, attr);
}

inline void StubBuilder::SetEnumCacheToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef key)
{
    GateRef offset = IntPtr(JSHClass::ENUM_CACHE_OFFSET);
    Store(type, glue, hClass, offset, key);
}

inline void StubBuilder::SetTransitionsToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef transition)
{
    GateRef offset = IntPtr(JSHClass::TRANSTIONS_OFFSET);
    Store(type, glue, hClass, offset, transition);
}

inline void StubBuilder::SetIsProtoTypeToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef oldValue = ZExtInt1ToInt32(value);
    GateRef bitfield = GetBitFieldFromHClass(hClass);
    GateRef mask = Int32LSL(
        Int32((1LU << JSHClass::IsPrototypeBit::SIZE) - 1),
        Int32(JSHClass::IsPrototypeBit::START_BIT));
    GateRef newVal = Int32Or(Int32And(bitfield, Int32Not(mask)),
        Int32LSL(oldValue, Int32(JSHClass::IsPrototypeBit::START_BIT)));
    SetBitFieldToHClass(glue, hClass, newVal);
}

inline GateRef StubBuilder::IsProtoTypeHClass(GateRef hClass)
{
    GateRef bitfield = GetBitFieldFromHClass(hClass);
    return TruncInt32ToInt1(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsPrototypeBit::START_BIT)),
        Int32((1LU << JSHClass::IsPrototypeBit::SIZE) - 1)));
}

inline void StubBuilder::SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
    GateRef value, GateRef attrOffset, VariableType type)
{
    ASM_ASSERT_WITH_GLUE(GET_MESSAGE_STRING_ID(IsNotDictionaryMode), BoolNot(IsDictionaryModeByHClass(hClass)), glue);
    GateRef bitfield = Load(VariableType::INT32(), hClass,
                            IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef inlinedPropsStart = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    GateRef propOffset = Int32Mul(
        Int32Add(inlinedPropsStart, attrOffset), Int32(JSTaggedValue::TaggedTypeSize()));

    // NOTE: need to translate MarkingBarrier
    Store(type, glue, obj, ZExtInt32ToPtr(propOffset), value);
    EXITENTRY();
}

inline GateRef StubBuilder::GetPropertyInlinedProps(GateRef obj, GateRef hClass,
    GateRef index)
{
    GateRef inlinedPropsStart = GetInlinedPropsStartFromHClass(hClass);
    GateRef propOffset = Int32Mul(
        Int32Add(inlinedPropsStart, index), Int32(JSTaggedValue::TaggedTypeSize()));
    return Load(VariableType::JS_ANY(), obj, ZExtInt32ToInt64(propOffset));
}

inline GateRef StubBuilder::GetInlinedPropOffsetFromHClass(GateRef hclass, GateRef index)
{
    GateRef inlinedPropsStart = GetInlinedPropsStartFromHClass(hclass);
    GateRef propOffset = Int32Mul(
        Int32Add(inlinedPropsStart, index), Int32(JSTaggedValue::TaggedTypeSize()));
    return ZExtInt32ToInt64(propOffset);
}

inline void StubBuilder::IncNumberOfProps(GateRef glue, GateRef hClass)
{
    GateRef propNums = GetNumberOfPropsFromHClass(hClass);
    SetNumberOfPropsToHClass(glue, hClass, Int32Add(propNums, Int32(1)));
}

inline GateRef StubBuilder::GetNumberOfPropsFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    return Int32And(Int32LSR(bitfield,
        Int32(JSHClass::NumberOfPropsBits::START_BIT)),
        Int32((1LLU << JSHClass::NumberOfPropsBits::SIZE) - 1));
}

inline GateRef StubBuilder::IsTSHClass(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    return Int32NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsTSBit::START_BIT)),
        Int32((1LU << JSHClass::IsTSBit::SIZE) - 1)),
        Int32(0));
}

inline void StubBuilder::SetNumberOfPropsToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef bitfield1 = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef oldWithMask = Int32And(bitfield1,
        Int32(~static_cast<uint32_t>(JSHClass::NumberOfPropsBits::Mask())));
    GateRef newValue = Int32LSR(value, Int32(JSHClass::NumberOfPropsBits::START_BIT));
    Store(VariableType::INT32(), glue, hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET),
        Int32Or(oldWithMask, newValue));
}

inline GateRef StubBuilder::GetInlinedPropertiesFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef objectSizeInWords = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ObjectSizeInWordsBits::START_BIT)),
        Int32((1LU << JSHClass::ObjectSizeInWordsBits::SIZE) - 1));
    GateRef inlinedPropsStart = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    return Int32Sub(objectSizeInWords, inlinedPropsStart);
}

inline GateRef StubBuilder::GetElementsKindFromHClass(GateRef hClass)
{
    return env_->GetBuilder()->GetElementsKindByHClass(hClass);
}

inline GateRef StubBuilder::GetObjectSizeFromHClass(GateRef hClass)
{
    return env_->GetBuilder()->GetObjectSizeFromHClass(hClass);
}

inline GateRef StubBuilder::GetInlinedPropsStartFromHClass(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    return Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
}

inline void StubBuilder::SetValueToTaggedArrayWithAttr(
    GateRef glue, GateRef array, GateRef index, GateRef key, GateRef val, GateRef attr)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    SetValueWithAttr(glue, array, dataOffset, key, val, attr);
}

inline void StubBuilder::SetValueToTaggedArrayWithRep(
    GateRef glue, GateRef array, GateRef index, GateRef val, GateRef rep, Label *repChange)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    SetValueWithRep(glue, array, dataOffset, val, rep, repChange);
}

inline void StubBuilder::SetValueToTaggedArray(VariableType valType, GateRef glue, GateRef array,
                                               GateRef index, GateRef val)
{
    // NOTE: need to translate MarkingBarrier
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    Store(valType, glue, array, dataOffset, val);
}

inline GateRef StubBuilder::GetValueFromTaggedArray(GateRef array, GateRef index)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(VariableType::JS_ANY(), array, dataOffset);
}

inline GateRef StubBuilder::IsSpecialIndexedObj(GateRef jsType)
{
    return Int32GreaterThan(jsType, Int32(static_cast<int32_t>(JSType::JS_ARRAY)));
}

inline GateRef StubBuilder::IsSpecialContainer(GateRef jsType)
{
    // arraylist and vector has fast pass now
    return BoolAnd(
        Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_API_ARRAY_LIST))),
        Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_API_VECTOR))));
}

inline GateRef StubBuilder::IsFastTypeArray(GateRef jsType)
{
    return BoolAnd(
            Int32GreaterThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_FIRST))),
            Int32LessThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_FLOAT64_ARRAY))));
}

inline GateRef StubBuilder::IsAccessorInternal(GateRef value)
{
    return Int32Equal(GetObjectType(LoadHClass(value)),
                      Int32(static_cast<int32_t>(JSType::INTERNAL_ACCESSOR)));
}

inline GateRef StubBuilder::GetPropAttrFromLayoutInfo(GateRef layout, GateRef entry)
{
    GateRef index = Int32Add(Int32LSL(entry, Int32(LayoutInfo::ELEMENTS_INDEX_LOG2)),
        Int32(LayoutInfo::ATTR_INDEX_OFFSET));
    return GetInt64OfTInt(GetValueFromTaggedArray(layout, index));
}

inline void StubBuilder::SetPropAttrToLayoutInfo(GateRef glue, GateRef layout, GateRef entry, GateRef attr)
{
    GateRef index = Int32Add(Int32LSL(entry, Int32(LayoutInfo::ELEMENTS_INDEX_LOG2)),
        Int32(LayoutInfo::ATTR_INDEX_OFFSET));
    GateRef taggedAttr = Int64ToTaggedInt(ZExtInt32ToInt64(attr));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, layout, index, taggedAttr);
}

inline GateRef StubBuilder::GetPropertyMetaDataFromAttr(GateRef attr)
{
    return Int32And(Int32LSR(attr, Int32(PropertyAttributes::PropertyMetaDataField::START_BIT)),
        Int32((1LLU << PropertyAttributes::PropertyMetaDataField::SIZE) - 1));
}

inline GateRef StubBuilder::GetKeyFromLayoutInfo(GateRef layout, GateRef entry)
{
    GateRef index = Int32LSL(entry, Int32(LayoutInfo::ELEMENTS_INDEX_LOG2));
    return GetValueFromTaggedArray(layout, index);
}

inline GateRef StubBuilder::GetPropertiesAddrFromLayoutInfo(GateRef layout)
{
    return PtrAdd(layout, IntPtr(TaggedArray::DATA_OFFSET));
}

inline GateRef StubBuilder::GetInt64OfTInt(GateRef x)
{
    return env_->GetBuilder()->GetInt64OfTInt(x);
}

inline GateRef StubBuilder::GetInt32OfTInt(GateRef x)
{
    return TruncInt64ToInt32(GetInt64OfTInt(x));
}

inline GateRef StubBuilder::TaggedCastToIntPtr(GateRef x)
{
    return env_->Is32Bit() ? TruncInt64ToInt32(GetInt64OfTInt(x)) : GetInt64OfTInt(x);
}

inline GateRef StubBuilder::GetDoubleOfTInt(GateRef x)
{
    return ChangeInt32ToFloat64(GetInt32OfTInt(x));
}

inline GateRef StubBuilder::GetDoubleOfTDouble(GateRef x)
{
    return env_->GetBuilder()->GetDoubleOfTDouble(x);
}

inline GateRef StubBuilder::GetDoubleOfTNumber(GateRef x)
{
    return env_->GetBuilder()->GetDoubleOfTNumber(x);
}

inline GateRef StubBuilder::LoadObjectFromWeakRef(GateRef x)
{
    return env_->GetBuilder()->LoadObjectFromWeakRef(x);
}

inline GateRef StubBuilder::ExtFloat32ToDouble(GateRef x)
{
    return env_->GetBuilder()->ExtFloat32ToDouble(x);
}

inline GateRef StubBuilder::ChangeInt32ToFloat32(GateRef x)
{
    return env_->GetBuilder()->ChangeInt32ToFloat32(x);
}

inline GateRef StubBuilder::ChangeInt32ToFloat64(GateRef x)
{
    return env_->GetBuilder()->ChangeInt32ToFloat64(x);
}

inline GateRef StubBuilder::ChangeUInt32ToFloat64(GateRef x)
{
    return env_->GetBuilder()->ChangeUInt32ToFloat64(x);
}

inline GateRef StubBuilder::ChangeFloat64ToInt32(GateRef x)
{
    return env_->GetBuilder()->ChangeFloat64ToInt32(x);
}

inline GateRef StubBuilder::ChangeTaggedPointerToInt64(GateRef x)
{
    return env_->GetBuilder()->ChangeTaggedPointerToInt64(x);
}

inline GateRef StubBuilder::Int64ToTaggedPtr(GateRef x)
{
    return env_->GetBuilder()->Int64ToTaggedPtr(x);
}

inline GateRef StubBuilder::CastInt32ToFloat32(GateRef x)
{
    return env_->GetBuilder()->CastInt32ToFloat32(x);
}

inline GateRef StubBuilder::CastInt64ToFloat64(GateRef x)
{
    return env_->GetBuilder()->CastInt64ToFloat64(x);
}

inline GateRef StubBuilder::SExtInt32ToInt64(GateRef x)
{
    return env_->GetBuilder()->SExtInt32ToInt64(x);
}

inline GateRef StubBuilder::SExtInt16ToInt64(GateRef x)
{
    return env_->GetBuilder()->SExtInt16ToInt64(x);
}

inline GateRef StubBuilder::SExtInt8ToInt64(GateRef x)
{
    return env_->GetBuilder()->SExtInt8ToInt64(x);
}

inline GateRef StubBuilder::SExtInt8ToInt32(GateRef x)
{
    return env_->GetBuilder()->SExtInt8ToInt32(x);
}

inline GateRef StubBuilder::SExtInt16ToInt32(GateRef x)
{
    return env_->GetBuilder()->SExtInt16ToInt32(x);
}

inline GateRef StubBuilder::SExtInt1ToInt64(GateRef x)
{
    return env_->GetBuilder()->SExtInt1ToInt64(x);
}

inline GateRef StubBuilder::SExtInt1ToInt32(GateRef x)
{
    return env_->GetBuilder()->SExtInt1ToInt32(x);
}

inline GateRef StubBuilder::ZExtInt8ToInt16(GateRef x)
{
    return env_->GetBuilder()->ZExtInt8ToInt16(x);
}

inline GateRef StubBuilder::ZExtInt32ToInt64(GateRef x)
{
    return env_->GetBuilder()->ZExtInt32ToInt64(x);
}

inline GateRef StubBuilder::ZExtInt1ToInt64(GateRef x)
{
    return env_->GetBuilder()->ZExtInt1ToInt64(x);
}

inline GateRef StubBuilder::ZExtInt1ToInt32(GateRef x)
{
    return env_->GetBuilder()->ZExtInt1ToInt32(x);
}

inline GateRef StubBuilder::ZExtInt8ToInt32(GateRef x)
{
    return env_->GetBuilder()->ZExtInt8ToInt32(x);
}

inline GateRef StubBuilder::ZExtInt8ToInt64(GateRef x)
{
    return env_->GetBuilder()->ZExtInt8ToInt64(x);
}

inline GateRef StubBuilder::ZExtInt8ToPtr(GateRef x)
{
    return env_->GetBuilder()->ZExtInt8ToPtr(x);
}

inline GateRef StubBuilder::ZExtInt16ToPtr(GateRef x)
{
    return env_->GetBuilder()->ZExtInt16ToPtr(x);
}

inline GateRef StubBuilder::SExtInt32ToPtr(GateRef x)
{
    return env_->GetBuilder()->SExtInt32ToPtr(x);
}

inline GateRef StubBuilder::ZExtInt16ToInt32(GateRef x)
{
    return env_->GetBuilder()->ZExtInt16ToInt32(x);
}

inline GateRef StubBuilder::ZExtInt16ToInt64(GateRef x)
{
    return env_->GetBuilder()->ZExtInt16ToInt64(x);
}

inline GateRef StubBuilder::TruncInt64ToInt32(GateRef x)
{
    return env_->GetBuilder()->TruncInt64ToInt32(x);
}

inline GateRef StubBuilder::TruncPtrToInt32(GateRef x)
{
    if (env_->Is32Bit()) {
        return x;
    }
    return TruncInt64ToInt32(x);
}

inline GateRef StubBuilder::TruncInt64ToInt1(GateRef x)
{
    return env_->GetBuilder()->TruncInt64ToInt1(x);
}

inline GateRef StubBuilder::TruncInt32ToInt1(GateRef x)
{
    return env_->GetBuilder()->TruncInt32ToInt1(x);
}

inline GateRef StubBuilder::GetObjectFromConstPool(GateRef constpool, GateRef index)
{
    return GetValueFromTaggedArray(constpool, index);
}

inline GateRef StubBuilder::GetGlobalConstantAddr(GateRef index)
{
    return Int64Mul(Int64(sizeof(JSTaggedValue)), index);
}

inline GateRef StubBuilder::GetGlobalConstantString(ConstantIndex index)
{
    if (env_->Is32Bit()) {
        return Int32Mul(Int32(sizeof(JSTaggedValue)), Int32(static_cast<int>(index)));
    } else {
        return Int64Mul(Int64(sizeof(JSTaggedValue)), Int64(static_cast<int>(index)));
    }
}

inline GateRef StubBuilder::IsCallableFromBitField(GateRef bitfield)
{
    return env_->GetBuilder()->IsCallableFromBitField(bitfield);
}

inline GateRef StubBuilder::IsCallable(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsCallable), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->IsCallable(obj);
    EXITENTRY();
    return res;
}

// GetOffset func in property_attribute.h
inline GateRef StubBuilder::GetOffsetFieldInPropAttr(GateRef attr)
{
    return Int32And(
        Int32LSR(attr, Int32(PropertyAttributes::OffsetField::START_BIT)),
        Int32((1LLU << PropertyAttributes::OffsetField::SIZE) - 1));
}

// SetOffset func in property_attribute.h
inline GateRef StubBuilder::SetOffsetFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int32LSL(
        Int32((1LLU << PropertyAttributes::OffsetField::SIZE) - 1),
        Int32(PropertyAttributes::OffsetField::START_BIT));
    GateRef newVal = Int32Or(Int32And(attr, Int32Not(mask)),
        Int32LSL(value, Int32(PropertyAttributes::OffsetField::START_BIT)));
    return newVal;
}

// SetIsInlinedProps func in property_attribute.h
inline GateRef StubBuilder::SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int32LSL(
        Int32((1LU << PropertyAttributes::IsInlinedPropsField::SIZE) - 1),
        Int32(PropertyAttributes::IsInlinedPropsField::START_BIT));
    GateRef newVal = Int32Or(Int32And(attr, Int32Not(mask)),
        Int32LSL(value, Int32(PropertyAttributes::IsInlinedPropsField::START_BIT)));
    return newVal;
}


inline GateRef StubBuilder::SetTrackTypeInPropAttr(GateRef attr, GateRef type)
{
    GateRef mask = Int32LSL(
        Int32((1LU << PropertyAttributes::TrackTypeField::SIZE) - 1),
        Int32(PropertyAttributes::TrackTypeField::START_BIT));
    GateRef newVal = Int32Or(Int32And(attr, Int32Not(mask)),
        Int32LSL(type, Int32(PropertyAttributes::TrackTypeField::START_BIT)));
    return newVal;
}

inline GateRef StubBuilder::GetTrackTypeInPropAttr(GateRef attr)
{
    return Int32And(
        Int32LSR(attr, Int32(PropertyAttributes::TrackTypeField::START_BIT)),
        Int32((1LLU << PropertyAttributes::TrackTypeField::SIZE) - 1));
}

inline GateRef StubBuilder::GetRepInPropAttr(GateRef attr)
{
    return Int32And(
        Int32LSR(attr, Int32(PropertyAttributes::RepresentationField::START_BIT)),
        Int32((1LLU << PropertyAttributes::RepresentationField::SIZE) - 1));
}

inline GateRef StubBuilder::IsIntRepInPropAttr(GateRef rep)
{
    return Int32Equal(rep, Int32(static_cast<int32_t>(Representation::INT)));
}

inline GateRef StubBuilder::IsDoubleRepInPropAttr(GateRef rep)
{
    return Int32Equal(rep, Int32(static_cast<int32_t>(Representation::DOUBLE)));
}

inline GateRef StubBuilder::SetTaggedRepInPropAttr(GateRef attr)
{
    GateRef mask = Int32LSL(
        Int32((1LU << PropertyAttributes::RepresentationField::SIZE) - 1),
        Int32(PropertyAttributes::RepresentationField::START_BIT));
    GateRef targetType = Int32(static_cast<int32_t>(Representation::TAGGED));
    GateRef newVal = Int32Or(Int32And(attr, Int32Not(mask)),
        Int32LSL(targetType, Int32(PropertyAttributes::RepresentationField::START_BIT)));
    return newVal;
}

inline void StubBuilder::SetHasConstructorToHClass(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    GateRef mask = Int32LSL(
        Int32((1LU << JSHClass::HasConstructorBits::SIZE) - 1),
        Int32(JSHClass::HasConstructorBits::START_BIT));
    GateRef newVal = Int32Or(Int32And(bitfield, Int32Not(mask)),
        Int32LSL(value, Int32(JSHClass::HasConstructorBits::START_BIT)));
    Store(VariableType::INT32(), glue, hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET), newVal);
}

inline GateRef StubBuilder::IntPtrEuqal(GateRef x, GateRef y)
{
    return env_->Is32Bit() ? Int32Equal(x, y) : Int64Equal(x, y);
}

inline GateRef StubBuilder::GetBitMask(GateRef bitoffset)
{
    // BIT_PER_WORD_MASK
    GateRef bitPerWordMask = Int32(GCBitset::BIT_PER_WORD_MASK);
    // IndexInWord(bitOffset) = bitOffset & BIT_PER_WORD_MASK
    GateRef indexInWord = Int32And(bitoffset, bitPerWordMask);
    // Mask(indeInWord) = 1 << index
    return Int32LSL(Int32(1), indexInWord);
}

inline GateRef StubBuilder::ObjectAddressToRange(GateRef x)
{
    // This function may cause GateRef x is not an object. GC may not mark this x object.
    return IntPtrAnd(TaggedCastToIntPtr(x), IntPtr(~panda::ecmascript::DEFAULT_REGION_MASK));
}

inline GateRef StubBuilder::InYoungGeneration(GateRef region)
{
    auto offset = Region::PackedData::GetFlagOffset(env_->Is32Bit());
    GateRef x = Load(VariableType::NATIVE_POINTER(), PtrAdd(IntPtr(offset), region),
        IntPtr(0));
    if (env_->Is32Bit()) {
        return Int32Equal(Int32And(x,
            Int32(RegionSpaceFlag::VALID_SPACE_MASK)), Int32(RegionSpaceFlag::IN_YOUNG_SPACE));
    } else {
        return Int64Equal(Int64And(x,
            Int64(RegionSpaceFlag::VALID_SPACE_MASK)), Int64(RegionSpaceFlag::IN_YOUNG_SPACE));
    }
}

inline GateRef StubBuilder::GetParentEnv(GateRef object)
{
    GateRef index = Int32(LexicalEnv::PARENT_ENV_INDEX);
    return GetValueFromTaggedArray(object, index);
}

inline GateRef StubBuilder::GetPropertiesFromLexicalEnv(GateRef object, GateRef index)
{
    GateRef valueIndex = Int32Add(index, Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    return GetValueFromTaggedArray(object, valueIndex);
}

inline void StubBuilder::SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value)
{
    GateRef valueIndex = Int32Add(index, Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, object, valueIndex, value);
}

inline GateRef StubBuilder::GetHomeObjectFromJSFunction(GateRef object)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return Load(VariableType::JS_ANY(), object, offset);
}

inline GateRef StubBuilder::GetMethodFromJSFunction(GateRef object)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    GateRef methodOffset;
    Label funcIsJSFunctionBase(env);
    Label funcIsJSProxy(env);
    Label getMethod(env);
    Branch(IsJSFunctionBase(object), &funcIsJSFunctionBase, &funcIsJSProxy);
    Bind(&funcIsJSFunctionBase);
    {
        methodOffset = IntPtr(JSFunctionBase::METHOD_OFFSET);
        Jump(&getMethod);
    }
    Bind(&funcIsJSProxy);
    {
        methodOffset = IntPtr(JSProxy::METHOD_OFFSET);
        Jump(&getMethod);
    }
    Bind(&getMethod);
    GateRef method = Load(VariableType::JS_ANY(), object, methodOffset);
    env->SubCfgExit();
    return method;
}

inline GateRef StubBuilder::GetCallFieldFromMethod(GateRef method)
{
    GateRef callFieldOffset = IntPtr(Method::CALL_FIELD_OFFSET);
    return Load(VariableType::INT64(), method, callFieldOffset);
}

inline void StubBuilder::SetLexicalEnvToFunction(GateRef glue, GateRef object, GateRef lexicalEnv)
{
    GateRef offset = IntPtr(JSFunction::LEXICAL_ENV_OFFSET);
    Store(VariableType::JS_ANY(), glue, object, offset, lexicalEnv);
}

inline GateRef StubBuilder::GetGlobalObject(GateRef glue)
{
    GateRef offset = IntPtr(JSThread::GlueData::GetGlobalObjOffset(env_->Is32Bit()));
    return Load(VariableType::JS_ANY(), glue, offset);
}

inline GateRef StubBuilder::GetMethodFromFunction(GateRef function)
{
    return env_->GetBuilder()->GetMethodFromFunction(function);
}

inline GateRef StubBuilder::GetModuleFromFunction(GateRef function)
{
    return env_->GetBuilder()->GetModuleFromFunction(function);
}

inline GateRef StubBuilder::GetEntryIndexOfGlobalDictionary(GateRef entry)
{
    return Int32Add(Int32(OrderTaggedHashTable<GlobalDictionary>::TABLE_HEADER_SIZE),
        Int32Mul(entry, Int32(GlobalDictionary::ENTRY_SIZE)));
}

inline GateRef StubBuilder::GetBoxFromGlobalDictionary(GateRef object, GateRef entry)
{
    GateRef index = GetEntryIndexOfGlobalDictionary(entry);
    GateRef offset = PtrAdd(ZExtInt32ToPtr(index),
        IntPtr(GlobalDictionary::ENTRY_VALUE_INDEX));
    return GetValueFromTaggedArray(object, offset);
}

inline GateRef StubBuilder::GetValueFromGlobalDictionary(GateRef object, GateRef entry)
{
    GateRef box = GetBoxFromGlobalDictionary(object, entry);
    return Load(VariableType::JS_ANY(), box, IntPtr(PropertyBox::VALUE_OFFSET));
}

inline GateRef StubBuilder::GetPropertiesFromJSObject(GateRef object)
{
    GateRef offset = IntPtr(JSObject::PROPERTIES_OFFSET);
    return Load(VariableType::JS_ANY(), object, offset);
}

inline GateRef StubBuilder::IsJSFunction(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    GateRef greater = Int32GreaterThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_FIRST)));
    GateRef less = Int32LessThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_LAST)));
    return BoolAnd(greater, less);
}

inline GateRef StubBuilder::IsBoundFunction(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_BOUND_FUNCTION)));
}

inline GateRef StubBuilder::IsNativeMethod(GateRef method)
{
    GateRef callFieldOffset = IntPtr(Method::CALL_FIELD_OFFSET);
    GateRef callfield = Load(VariableType::INT64(), method, callFieldOffset);
    return Int64NotEqual(
        Int64And(
            Int64LSR(callfield, Int64(MethodLiteral::IsNativeBit::START_BIT)),
            Int64((1LU << MethodLiteral::IsNativeBit::SIZE) - 1)),
        Int64(0));
}

inline GateRef StubBuilder::IsOptimizedWithBitField(GateRef bitfield)
{
    return env_->GetBuilder()->IsOptimizedWithBitField(bitfield);
}

inline GateRef StubBuilder::CanFastCallWithBitField(GateRef bitfield)
{
    return env_->GetBuilder()->CanFastCallWithBitField(bitfield);
}

inline GateRef StubBuilder::GetExpectedNumOfArgs(GateRef method)
{
    GateRef callFieldOffset = IntPtr(Method::CALL_FIELD_OFFSET);
    GateRef callfield = Load(VariableType::INT64(), method, callFieldOffset);
    return TruncInt64ToInt32(Int64And(
        Int64LSR(callfield, Int64(MethodLiteral::NumArgsBits::START_BIT)),
        Int64((1LU << MethodLiteral::NumArgsBits::SIZE) - 1)));
}

inline GateRef StubBuilder::GetMethodFromJSProxy(GateRef proxy)
{
    GateRef offset = IntPtr(JSProxy::METHOD_OFFSET);
    return Load(VariableType::JS_ANY(), proxy, offset);
}

inline GateRef StubBuilder::GetHandlerFromJSProxy(GateRef proxy)
{
    GateRef offset = IntPtr(JSProxy::HANDLER_OFFSET);
    return Load(VariableType::JS_ANY(), proxy, offset);
}

inline GateRef StubBuilder::GetTargetFromJSProxy(GateRef proxy)
{
    GateRef offset = IntPtr(JSProxy::TARGET_OFFSET);
    return Load(VariableType::JS_ANY(), proxy, offset);
}

inline GateRef StubBuilder::ComputeTaggedArraySize(GateRef length)
{
    return PtrAdd(IntPtr(TaggedArray::DATA_OFFSET),
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), length));
}

inline GateRef StubBuilder::GetGlobalConstantValue(VariableType type, GateRef glue, ConstantIndex index)
{
    GateRef gConstAddr = Load(VariableType::JS_ANY(), glue,
        IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_->Is32Bit())));
    auto constantIndex = IntPtr(JSTaggedValue::TaggedTypeSize() * static_cast<size_t>(index));
    return Load(type, gConstAddr, constantIndex);
}

inline GateRef StubBuilder::GetGlobalEnvValue(VariableType type, GateRef env, size_t index)
{
    auto valueIndex = IntPtr(GlobalEnv::HEADER_SIZE + JSTaggedValue::TaggedTypeSize() * index);
    return Load(type, env, valueIndex);
}

inline GateRef StubBuilder::HasPendingException(GateRef glue)
{
    GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env_->IsArch32Bit()));
    GateRef exception = Load(VariableType::JS_ANY(), glue, exceptionOffset);
    return TaggedIsNotHole(exception);
}

inline GateRef StubBuilder::DispatchBuiltins(GateRef glue, GateRef builtinsId,
                                             const std::initializer_list<GateRef>& args)
{
    GateRef target = PtrMul(ZExtInt32ToPtr(builtinsId), IntPtrSize());
    return env_->GetBuilder()->CallBuiltin(glue, target, args);
}

inline GateRef StubBuilder::DispatchBuiltinsWithArgv(GateRef glue, GateRef builtinsId,
                                                     const std::initializer_list<GateRef>& args)
{
    GateRef target = PtrMul(ZExtInt32ToPtr(builtinsId), IntPtrSize());
    return env_->GetBuilder()->CallBuiltinWithArgv(glue, target, args);
}

inline GateRef StubBuilder::GetBuiltinId(GateRef method)
{
    GateRef extraLiteralInfoOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef extraLiteralInfo = Load(VariableType::INT64(), method, extraLiteralInfoOffset);
    return TruncInt64ToInt32(Int64And(
        Int64LSR(extraLiteralInfo, Int64(MethodLiteral::BuiltinIdBits::START_BIT)),
        Int64((1LU << MethodLiteral::BuiltinIdBits::SIZE) - 1)));
}

inline GateRef StubBuilder::ComputeSizeUtf8(GateRef length)
{
    return PtrAdd(IntPtr(LineEcmaString::DATA_OFFSET), length);
}

inline GateRef StubBuilder::ComputeSizeUtf16(GateRef length)
{
    return PtrAdd(IntPtr(LineEcmaString::DATA_OFFSET), PtrMul(length, IntPtr(sizeof(uint16_t))));
}

inline GateRef StubBuilder::AlignUp(GateRef x, GateRef alignment)
{
    GateRef x1 = PtrAdd(x, PtrSub(alignment, IntPtr(1)));
    return IntPtrAnd(x1, IntPtrNot(PtrSub(alignment, IntPtr(1))));
}

inline void StubBuilder::SetLength(GateRef glue, GateRef str, GateRef length, bool compressed)
{
    GateRef len = Int32LSL(length, Int32(2));
    GateRef mixLength;
    if (compressed) {
        mixLength = Int32Or(len, Int32(EcmaString::STRING_COMPRESSED));
    } else {
        mixLength = Int32Or(len, Int32(EcmaString::STRING_UNCOMPRESSED));
    }
    Store(VariableType::INT32(), glue, str, IntPtr(EcmaString::MIX_LENGTH_OFFSET), mixLength);
}

inline void StubBuilder::SetRawHashcode(GateRef glue, GateRef str, GateRef rawHashcode)
{
    Store(VariableType::INT32(), glue, str, IntPtr(EcmaString::HASHCODE_OFFSET), rawHashcode);
}

inline GateRef StubBuilder::TryGetHashcodeFromString(GateRef string)
{
    return env_->GetBuilder()->TryGetHashcodeFromString(string);
}

inline void StubBuilder::SetExtensibleToBitfield(GateRef glue, GateRef obj, bool isExtensible)
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

inline void StubBuilder::Comment(GateRef glue, const std::string &str)
{
    CallNGCRuntime(glue, RTSTUB_ID(Comment), { StringPtr(str) });
}

inline GateRef StubBuilder::IsStableElements(GateRef hClass)
{
    return env_->GetBuilder()->IsStableElements(hClass);
}

inline GateRef StubBuilder::HasConstructorByHClass(GateRef hClass)
{
    return env_->GetBuilder()->HasConstructorByHClass(hClass);
}

inline GateRef StubBuilder::HasConstructor(GateRef object)
{
    return env_->GetBuilder()->HasConstructor(object);
}

inline GateRef StubBuilder::IsStableArguments(GateRef hClass)
{
    return env_->GetBuilder()->IsStableArguments(hClass);
}

inline GateRef StubBuilder::IsStableArray(GateRef hClass)
{
    return env_->GetBuilder()->IsStableArray(hClass);
}

inline GateRef StubBuilder::IsTypedArray(GateRef obj)
{
    GateRef jsHclass = LoadHClass(obj);
    GateRef jsType = GetObjectType(jsHclass);
    return BoolAnd(Int32GreaterThan(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_FIRST))),
                   Int32GreaterThanOrEqual(Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_LAST)), jsType));
}

inline GateRef StubBuilder::GetProfileTypeInfo(GateRef jsFunc)
{
    GateRef method = GetMethodFromFunction(jsFunc);
    return Load(VariableType::JS_POINTER(), method, IntPtr(Method::PROFILE_TYPE_INFO_OFFSET));
}

inline GateRef StubBuilder::LoadObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    return env_->GetBuilder()->LoadObjectFromConstPool(jsFunc, index);
}

inline GateRef StubBuilder::LoadPfHeaderFromConstPool(GateRef jsFunc)
{
    GateRef method = Load(VariableType::JS_ANY(), jsFunc, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef constPool = Load(VariableType::JS_ANY(), method, IntPtr(Method::CONSTANT_POOL_OFFSET));
    auto length = GetLengthOfTaggedArray(constPool);
    auto index = Int32Sub(length, Int32(ConstantPool::JS_PANDA_FILE_INDEX));
    auto jsPandaFile = GetValueFromTaggedArray(constPool, index);
    auto jsPfAddr = ChangeInt64ToIntPtr(ChangeTaggedPointerToInt64(jsPandaFile));
    auto pfAddr = Load(VariableType::NATIVE_POINTER(), jsPfAddr, Int32(JSPandaFile::PF_OFFSET));
    auto pfHeader = Load(VariableType::NATIVE_POINTER(), pfAddr, Int32(0));
    return pfHeader;
}

inline GateRef StubBuilder::LoadHCIndexFromConstPool(GateRef jsFunc, GateRef traceId)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    GateRef method = Load(VariableType::JS_ANY(), jsFunc, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef constPool = Load(VariableType::JS_ANY(), method, IntPtr(Method::CONSTANT_POOL_OFFSET));
    auto length = GetLengthOfTaggedArray(constPool);
    auto index = Int32Sub(length, Int32(ConstantPool::CONSTANT_INDEX_INFO_INDEX));
    auto constantIndexInfo = GetValueFromTaggedArray(constPool, index);
    auto indexInfoLength = GetLengthOfTaggedArray(constantIndexInfo);
    DEFVARIABLE(bcOffset, VariableType::INT32(), Int32(0));
    DEFVARIABLE(constantIndex, VariableType::INT32(),
        Int32(static_cast<int32_t>(ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX)));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label matchSuccess(env);
    Label afterUpdate(env);
    Branch(Int32LessThan(*i, indexInfoLength), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    bcOffset = GetInt32OfTInt(GetValueFromTaggedArray(constantIndexInfo, Int32(*i)));
    Branch(Int32Equal(*bcOffset, traceId), &matchSuccess, &afterUpdate);
    Bind(&matchSuccess);
    constantIndex = GetInt32OfTInt(GetValueFromTaggedArray(constantIndexInfo, Int32(*i + 1)));
    Jump(&afterLoop);
    Bind(&afterUpdate);
    i = Int32Add(*i, Int32(2)); // 2 : skip traceId and constantIndex
    Branch(Int32LessThan(*i, indexInfoLength), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    auto ret = *constantIndex;

    env->SubCfgExit();
    return ret;
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_STUB_INL_H
