/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/stub_builder.h"

#include "ecmascript/accessor_data.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/baseline/baseline_call_signature.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/enum_cache.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/mega_ic_cache.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/message_string.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript::kungfu {
using JSFunction = panda::ecmascript::JSFunction;
using PropertyBox = panda::ecmascript::PropertyBox;
inline int StubBuilder::NextVariableId()
{
    return env_->NextVariableId();
}

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
}

inline GateRef StubBuilder::Int64(int64_t value)
{
    return env_->GetBuilder()->Int64(value);
}

inline GateRef StubBuilder::TaggedInt(int32_t value)
{
    JSTaggedType value64 = static_cast<JSTaggedType>(value) | JSTaggedValue::TAG_INT;
    return env_->GetBuilder()->Int64(value64);
}

inline GateRef StubBuilder::StringPtr(std::string_view str)
{
    return env_->GetBuilder()->StringPtr(str);
}

inline GateRef StubBuilder::IntPtr(int64_t value)
{
    return env_->Is32Bit() ? Int32(value) : Int64(value);
}

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

inline GateRef StubBuilder::SpecialHole()
{
    return env_->GetBuilder()->SpecialHoleConstant();
}

inline GateRef StubBuilder::Null()
{
    return env_->GetBuilder()->NullConstant();
}

inline GateRef StubBuilder::NullPtr()
{
    return env_->GetBuilder()->NullPtrConstant();
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

inline GateRef StubBuilder::CallRuntime(GateRef glue, int index, const std::vector<GateRef>& args)
{
    SavePcIfNeeded(glue);
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    GateRef result = env_->GetBuilder()->CallRuntime(glue, index, Gate::InvalidGateRef, args,
                                                     glue, name.c_str());
    return result;
}

inline GateRef StubBuilder::CallRuntime(GateRef glue, int index, GateRef argc, GateRef argv)
{
    SavePcIfNeeded(glue);
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    GateRef result = env_->GetBuilder()->CallRuntimeVarargs(glue, index, argc, argv, name.c_str());
    return result;
}

inline GateRef StubBuilder::CallNGCRuntime(GateRef glue, int index,
                                           const std::vector<GateRef>& args, GateRef hir)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    GateRef result;
    if (env_->GetCircuit()->IsOptimizedOrFastJit()) {
        result = env_->GetBuilder()->CallNGCRuntime(glue, index, Gate::InvalidGateRef, args,
                                                    hir, name.c_str());
    } else {
        result = env_->GetBuilder()->CallNGCRuntime(glue, index, Gate::InvalidGateRef, args,
                                                    Circuit::NullGate(), name.c_str());
    }
    return result;
}

inline GateRef StubBuilder::FastCallOptimized(GateRef glue, GateRef code, const std::vector<GateRef>& args, GateRef hir)
{
    GateRef result;
    if (env_->GetCircuit()->IsOptimizedOrFastJit()) {
        result = env_->GetBuilder()->FastCallOptimized(glue, code, Gate::InvalidGateRef, args, hir);
    } else {
        result = env_->GetBuilder()->FastCallOptimized(glue, code, Gate::InvalidGateRef, args, Circuit::NullGate());
    }
    return result;
}

inline GateRef StubBuilder::CallOptimized(GateRef glue, GateRef code, const std::vector<GateRef>& args, GateRef hir)
{
    GateRef result;
    if (env_->GetCircuit()->IsOptimizedOrFastJit()) {
        result = env_->GetBuilder()->CallOptimized(glue, code, Gate::InvalidGateRef, args, hir);
    } else {
        result = env_->GetBuilder()->CallOptimized(glue, code, Gate::InvalidGateRef, args, Circuit::NullGate());
    }
    return result;
}

inline GateRef StubBuilder::GetAotCodeAddr(GateRef jsFunc)
{
    return env_->GetBuilder()->GetCodeAddr(jsFunc);
}

inline GateRef StubBuilder::GetBaselineCodeAddr(GateRef baselineCode)
{
    return env_->GetBuilder()->GetBaselineCodeAddr(baselineCode);
}

inline GateRef StubBuilder::CallStub(GateRef glue, int index, const std::initializer_list<GateRef>& args)
{
    SavePcIfNeeded(glue);
    const std::string name = CommonStubCSigns::GetName(index);
    GateRef result = env_->GetBuilder()->CallStub(glue, Circuit::NullGate(), index, args, name.c_str());
    return result;
}

inline GateRef StubBuilder::CallCommonStub(GateRef glue, int index, const std::initializer_list<GateRef>& args,
                                           GateRef hir)
{
    SavePcIfNeeded(glue);
    const std::string name = CommonStubCSigns::GetName(index);
    GateRef result = env_->GetBuilder()->CallCommonStub(glue, hir, index, args, name.c_str());
    return result;
}

inline GateRef StubBuilder::CallBuiltinRuntime(GateRef glue, const std::initializer_list<GateRef>& args, bool isNew)
{
    return env_->GetBuilder()->CallBuiltinRuntime(glue, Gate::InvalidGateRef, args, isNew);
}

inline GateRef StubBuilder::CallBuiltinRuntimeWithNewTarget(GateRef glue, const std::initializer_list<GateRef>& args)
{
    return env_->GetBuilder()->CallBuiltinRuntimeWithNewTarget(glue, Gate::InvalidGateRef, args);
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
    if (env_->IsAsmInterp() || env_->IsBaselineBuiltin()) {
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
    if ((env_->IsAsmInterp() || env_->IsBaselineBuiltin())
         && kungfu::AssemblerModule::IsJumpToCallCommonEntry(mode)) {
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
    return env_->GetBuilder()->IntPtrEqual(x, y);
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

inline GateRef StubBuilder::Int32Min(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Min(x, y);
}

inline GateRef StubBuilder::Int32Max(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Max(x, y);
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

inline GateRef StubBuilder::Int16Or(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int16Or(x, y);
}

inline GateRef StubBuilder::Int16And(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int16And(x, y);
}

inline GateRef StubBuilder::Int32Or(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32Or(x, y);
}

inline GateRef StubBuilder::Int8And(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int8And(x, y);
}

inline GateRef StubBuilder::Int8Xor(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int8Xor(x, y);
}

inline GateRef StubBuilder::Int32And(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int32And(x, y);
}

inline GateRef StubBuilder::BitAnd(GateRef x, GateRef y)
{
    return env_->GetBuilder()->BitAnd(x, y);
}

inline GateRef StubBuilder::BitOr(GateRef x, GateRef y)
{
    return env_->GetBuilder()->BitOr(x, y);
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

inline GateRef StubBuilder::Int16LSR(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int16LSR(x, y);
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
    return env_->GetBuilder()->TaggedIsDouble(x);
}

inline GateRef StubBuilder::TaggedIsObject(GateRef x)
{
    return env_->GetBuilder()->TaggedIsObject(x);
}

inline GateRef StubBuilder::TaggedIsString(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsString(obj);
}

inline GateRef StubBuilder::TaggedIsStringIterator(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsStringIterator(obj);
}

inline GateRef StubBuilder::TaggedIsSharedObj(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsSharedObj(obj);
}

inline GateRef StubBuilder::TaggedIsStringOrSymbol(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsStringOrSymbol(obj);
}

inline GateRef StubBuilder::TaggedIsSymbol(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsSymbol(obj);
}

inline GateRef StubBuilder::TaggedIsArrayBuffer(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_ARRAY_BUFFER)));
}

inline GateRef StubBuilder::TaggedIsArrayIterator(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsArrayIterator(obj);
}

inline GateRef StubBuilder::BothAreString(GateRef x, GateRef y)
{
    return env_->GetBuilder()->BothAreString(x, y);
}

inline GateRef StubBuilder::TaggedIsNumber(GateRef x)
{
    return BoolNot(TaggedIsObject(x));
}

inline GateRef StubBuilder::TaggedIsNumeric(GateRef x)
{
    return env_->GetBuilder()->TaggedIsNumeric(x);
}

inline GateRef StubBuilder::TaggedIsHole(GateRef x)
{
    return env_->GetBuilder()->TaggedIsHole(x);
}

inline GateRef StubBuilder::TaggedIsNotHole(GateRef x)
{
    return env_->GetBuilder()->TaggedIsNotHole(x);
}

inline GateRef StubBuilder::ValueIsSpecialHole(GateRef x)
{
    return env_->GetBuilder()->IsSpecialHole(x);
}

inline GateRef StubBuilder::ElementsKindIsInt(GateRef kind)
{
    return env_->GetBuilder()->ElementsKindIsInt(kind);
}

inline GateRef StubBuilder::ElementsKindIsIntOrHoleInt(GateRef kind)
{
    return env_->GetBuilder()->ElementsKindIsIntOrHoleInt(kind);
}

inline GateRef StubBuilder::ElementsKindIsNumber(GateRef kind)
{
    return env_->GetBuilder()->ElementsKindIsNumber(kind);
}

inline GateRef StubBuilder::ElementsKindIsNumOrHoleNum(GateRef kind)
{
    return env_->GetBuilder()->ElementsKindIsNumOrHoleNum(kind);
}

inline GateRef StubBuilder::ElementsKindIsString(GateRef kind)
{
    return env_->GetBuilder()->ElementsKindIsString(kind);
}

inline GateRef StubBuilder::ElementsKindIsStringOrHoleString(GateRef kind)
{
    return env_->GetBuilder()->ElementsKindIsStringOrHoleString(kind);
}

inline GateRef StubBuilder::ElementsKindIsHeapKind(GateRef kind)
{
    return env_->GetBuilder()->ElementsKindIsHeapKind(kind);
}

inline GateRef StubBuilder::ElementsKindHasHole(GateRef kind)
{
    return env_->GetBuilder()->ElementsKindHasHole(kind);
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

inline GateRef StubBuilder::TaggedIsRegularObject(GateRef x)
{
    GateRef objectType = GetObjectType(LoadHClass(x));
    return Int32LessThan(objectType, Int32(static_cast<int32_t>(JSType::JS_API_ARRAY_LIST)));
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

inline GateRef StubBuilder::IsTaggedArray(GateRef x)
{
    return env_->GetBuilder()->IsTaggedArray(x);
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

inline GateRef StubBuilder::TaggedIsStoreAOTHandler(GateRef x)
{
    return env_->GetBuilder()->TaggedIsStoreAOTHandler(x);
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

inline GateRef StubBuilder::DoubleIsNanOrInf(GateRef x)
{
    return env_->GetBuilder()->DoubleIsNanOrInf(x);
}

inline GateRef StubBuilder::DoubleAbs(GateRef x)
{
    return env_->GetBuilder()->FAbs(x);
}

inline GateRef StubBuilder::DoubleIsInteger(GateRef x)
{
    GateRef notInteger = LogicOrBuilder(env_).Or(DoubleIsNAN(x)).Or(DoubleIsINF(x))
        .Or(BoolNot(DoubleEqual(x, DoubleTrunc(x)))).Done();
    return BoolNot(notInteger);
}

inline GateRef StubBuilder::DoubleIsWithinInt32(GateRef x)
{
    return env_->GetBuilder()->DoubleIsWithinInt32(x);
}

inline GateRef StubBuilder::DoubleTrunc(GateRef x)
{
    if (env_->IsAArch64()) {
        return env_->GetBuilder()->DoubleTrunc(x);
    }

    Label entry(env_);
    env_->SubCfgEntry(&entry);

    DEFVARIABLE(result, VariableType::FLOAT64(), x);
    Label exit(env_);

    constexpr int64_t DOUBLE_FRACTION_BITS = 52;
    constexpr int64_t DOUBLE_EXP_MASK = 0x7ff;
    constexpr int64_t DOUBLE_EXP_OFFSET = 0x3ff;
    GateRef bits = CastDoubleToInt64(x);
    GateRef exp = Int64Sub(Int64And(Int64LSR(bits, Int64(DOUBLE_FRACTION_BITS)), Int64(DOUBLE_EXP_MASK)),
                           Int64(DOUBLE_EXP_OFFSET));

    Label trunc(env_);
    BRANCH(Int64GreaterThanOrEqual(exp, Int64(DOUBLE_FRACTION_BITS)), &exit, &trunc);
    Bind(&trunc);
    {
        Label zero(env_);
        Label nonZero(env_);
        BRANCH(Int64LessThan(exp, Int64(0)), &zero, &nonZero);
        Bind(&zero);
        {
            constexpr int64_t DOUBLE_SIGN_SHIFT = 63;
            constexpr int64_t mask = static_cast<int64_t>(1) << DOUBLE_SIGN_SHIFT;
            result = CastInt64ToFloat64(Int64And(bits, Int64(mask)));
            Jump(&exit);
        }
        Bind(&nonZero);
        {
            constexpr int64_t DOUBLE_NON_FRACTION_BITS = 12;
            constexpr int64_t NEG_ONE = -1;
            GateRef mask = Int64LSR(Int64(NEG_ONE), Int64Add(exp, Int64(DOUBLE_NON_FRACTION_BITS)));
            result = CastInt64ToFloat64(Int64And(bits, Int64Not(mask)));
            Jump(&exit);
        }
    }

    Bind(&exit);
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}

inline GateRef StubBuilder::TaggedIsNull(GateRef x)
{
    return env_->GetBuilder()->TaggedIsNull(x);
}

inline GateRef StubBuilder::TaggedIsNotNull(GateRef x)
{
    return env_->GetBuilder()->TaggedIsNotNull(x);
}

inline GateRef StubBuilder::TaggedIsUndefinedOrNull(GateRef x)
{
    return env_->GetBuilder()->TaggedIsUndefinedOrNull(x);
}

inline GateRef StubBuilder::TaggedIsUndefinedOrNullOrHole(GateRef x)
{
    return env_->GetBuilder()->TaggedIsUndefinedOrNullOrHole(x);
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

inline GateRef StubBuilder::TaggedIsNativePointer(GateRef x)
{
    return env_->GetBuilder()->TaggedIsNativePointer(x);
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

inline GateRef StubBuilder::Int64ToTaggedIntPtr(GateRef x)
{
    return env_->GetBuilder()->ToTaggedIntPtr(x);
}

inline GateRef StubBuilder::DoubleToTaggedDouble(GateRef x)
{
    return env_->GetBuilder()->DoubleToTaggedDouble(x);
}

inline GateRef StubBuilder::DoubleToTaggedDoublePtr(GateRef x)
{
    return env_->GetBuilder()->DoubleToTaggedDoublePtr(x);
}

inline GateRef StubBuilder::BooleanToTaggedBooleanPtr(GateRef x)
{
    return env_->GetBuilder()->BooleanToTaggedBooleanPtr(x);
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

inline GateRef StubBuilder::CastFloat32ToInt32(GateRef x)
{
    return env_->GetBuilder()->CastFloat32ToInt32(x);
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

inline GateRef StubBuilder::Int8GreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int8GreaterThanOrEqual(x, y);
}

inline GateRef StubBuilder::Equal(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Equal(x, y);
}

inline GateRef StubBuilder::NotEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->NotEqual(x, y);
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

inline GateRef StubBuilder::Int64UnsignedGreaterThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64UnsignedGreaterThan(x, y);
}

inline GateRef StubBuilder::Int64UnsignedGreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->Int64UnsignedGreaterThanOrEqual(x, y);
}

inline GateRef StubBuilder::IntPtrLessThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrLessThan(x, y);
}

inline GateRef StubBuilder::IntPtrLessThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrLessThanOrEqual(x, y);
}

inline GateRef StubBuilder::IntPtrGreaterThan(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrGreaterThan(x, y);
}

inline GateRef StubBuilder::IntPtrGreaterThanOrEqual(GateRef x, GateRef y)
{
    return env_->GetBuilder()->IntPtrGreaterThanOrEqual(x, y);
}

// cast operation
inline GateRef StubBuilder::TruncInt16ToInt8(GateRef val)
{
    return env_->GetBuilder()->TruncInt16ToInt8(val);
}

inline GateRef StubBuilder::TruncInt32ToInt16(GateRef val)
{
    return env_->GetBuilder()->TruncInt32ToInt16(val);
}

inline GateRef StubBuilder::TruncInt32ToInt8(GateRef val)
{
    return env_->GetBuilder()->TruncInt32ToInt8(val);
}

inline GateRef StubBuilder::TruncFloatToInt64(GateRef val)
{
    return env_->GetBuilder()->TruncFloatToInt64(val);
}

inline void StubBuilder::CanNotConvertNotValidObject([[maybe_unused]] GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(CanNotConvertNotValidObject), IsEcmaObject(obj));
}

inline void StubBuilder::IsNotPropertyKey([[maybe_unused]] GateRef flag)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsNotPropertyKey), flag);
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

inline void StubBuilder::SetElementsArray(VariableType type, GateRef glue, GateRef object, GateRef elementsArray,
                                          MemoryAttribute mAttr)
{
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    Store(type, glue, object, elementsOffset, elementsArray, mAttr);
}

inline GateRef StubBuilder::GetPropertiesArray(GateRef object)
{
    GateRef propertiesOffset = IntPtr(JSObject::PROPERTIES_OFFSET);
    return Load(VariableType::JS_POINTER(), object, propertiesOffset);
}

// SetProperties in js_object.h
inline void StubBuilder::SetPropertiesArray(VariableType type, GateRef glue, GateRef object, GateRef propsArray,
                                            MemoryAttribute mAttr)
{
    GateRef propertiesOffset = IntPtr(JSObject::PROPERTIES_OFFSET);
    Store(type, glue, object, propertiesOffset, propsArray, mAttr);
}

inline GateRef StubBuilder::GetLengthOfTaggedArray(GateRef array)
{
    return Load(VariableType::INT32(), array, IntPtr(TaggedArray::LENGTH_OFFSET));
}

inline GateRef StubBuilder::GetLengthOfJSTypedArray(GateRef array)
{
    return Load(VariableType::INT32(), array, IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET));
}

inline GateRef StubBuilder::GetExtraLengthOfTaggedArray(GateRef array)
{
    return Load(VariableType::INT32(), array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET));
}

inline void StubBuilder::SetExtraLengthOfTaggedArray(GateRef glue, GateRef array, GateRef len)
{
    return Store(VariableType::INT32(), glue, array, IntPtr(TaggedArray::EXTRA_LENGTH_OFFSET),
                 len, MemoryAttribute::NoBarrier());
}

inline GateRef StubBuilder::IsJSHClass(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsJSHClass), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->IsJSHClass(obj);
    return res;
}

// object operation
inline GateRef StubBuilder::LoadHClass(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(LoadHClass), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->LoadHClass(object);
    return res;
}

inline void StubBuilder::StoreHClass(GateRef glue, GateRef object, GateRef hClass)
{
    return env_->GetBuilder()->StoreHClass(glue, object, hClass);
}

inline void StubBuilder::StoreHClassWithoutBarrier(GateRef glue, GateRef object, GateRef hClass)
{
    return env_->GetBuilder()->StoreHClassWithoutBarrier(glue, object, hClass);
}

inline void StubBuilder::StoreBuiltinHClass(GateRef glue, GateRef object, GateRef hClass)
{
    return env_->GetBuilder()->StoreHClassWithoutBarrier(glue, object, hClass);
}

inline void StubBuilder::StorePrototype(GateRef glue, GateRef hclass, GateRef prototype)
{
    return env_->GetBuilder()->StorePrototype(glue, hclass, prototype);
}

inline GateRef StubBuilder::GetObjectType(GateRef hClass)
{
    return env_->GetBuilder()->GetObjectType(hClass);
}

inline GateRef StubBuilder::IsDictionaryMode(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsDictionaryMode), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->IsDictionaryMode(object);
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

inline GateRef StubBuilder::IsJSArrayPrototypeModified(GateRef hClass)
{
    return env_->GetBuilder()->IsJSArrayPrototypeModified(hClass);
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
    return res;
}

inline GateRef StubBuilder::IsClassPrototype(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsClassPrototype), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->IsClassPrototype(object);
    return res;
}

inline GateRef StubBuilder::IsExtensible(GateRef object)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsExtensible), TaggedIsHeapObject(object));
    GateRef res = env_->GetBuilder()->IsExtensible(object);
    return res;
}

inline GateRef StubBuilder::IsSendableFunctionModule(GateRef module)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsSendableFunctionModule), IsSourceTextModule(module));
    GateRef bitfieldOffset = Int32(SourceTextModule::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), module, bitfieldOffset);
    return Equal(Int32And(Int32LSR(bitfield,
        Int32(SourceTextModule::SharedTypeBits::START_BIT)),
        Int32((1LU << SourceTextModule::SharedTypeBits::SIZE) - 1)),
        Int32(static_cast<int32_t>(SharedTypes::SENDABLE_FUNCTION_MODULE)));
}

inline GateRef StubBuilder::TaggedObjectIsEcmaObject(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsEcmaObject), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->TaggedObjectIsEcmaObject(obj);
    return res;
}

inline GateRef StubBuilder::IsEcmaObject(GateRef obj)
{
    return env_->GetBuilder()->IsEcmaObject(obj);
}

inline GateRef StubBuilder::IsJSObject(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsJSObject), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->IsJSObject(obj);
    return res;
}

inline GateRef StubBuilder::IsJSFunctionBase(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    GateRef greater = Int32GreaterThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_BASE)));
    GateRef less = Int32LessThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_BOUND_FUNCTION)));
    return BitAnd(greater, less);
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

inline GateRef StubBuilder::IsDerived(GateRef func)
{
    return env_->GetBuilder()->IsDerived(func);
}

inline GateRef StubBuilder::IsSymbol(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
}

inline GateRef StubBuilder::IsDataView(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_DATA_VIEW)));
}

inline GateRef StubBuilder::IsString(GateRef obj)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(IsJSObject), TaggedIsHeapObject(obj));
    GateRef res = env_->GetBuilder()->TaggedObjectIsString(obj);
    return res;
}

inline GateRef StubBuilder::IsLineString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::LINE_STRING)));
}

inline GateRef StubBuilder::IsSlicedString(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::SLICED_STRING)));
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

inline GateRef StubBuilder::IsJSShared(GateRef obj)
{
    return TaggedIsSharedObj(obj);
}

inline GateRef StubBuilder::IsProfileTypeInfoCell0(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::PROFILE_TYPE_INFO_CELL_0)));
}

inline GateRef StubBuilder::IsJSGlobalObject(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_GLOBAL_OBJECT)));
}

inline GateRef StubBuilder::IsNativeModuleFailureInfo(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::NATIVE_MODULE_FAILURE_INFO)));
}

inline GateRef StubBuilder::IsNativePointer(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_NATIVE_POINTER)));
}

inline GateRef StubBuilder::IsModuleNamespace(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_MODULE_NAMESPACE)));
}

inline GateRef StubBuilder::IsSourceTextModule(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::SOURCE_TEXT_MODULE_RECORD)));
}

inline GateRef StubBuilder::IsSpecialContainer(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return BitAnd(
        Int32GreaterThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::JS_API_ARRAY_LIST))),
        Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(JSType::JS_API_QUEUE))));
}

inline GateRef StubBuilder::IsJSPrimitiveRef(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_PRIMITIVE_REF)));
}

inline GateRef StubBuilder::IsJsArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_ARRAY)));
}

inline GateRef StubBuilder::IsJsSArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_SHARED_ARRAY)));
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

inline GateRef StubBuilder::IsJSCollator(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_COLLATOR)));
}

inline GateRef StubBuilder::IsJSObjectType(GateRef obj, JSType jsType)
{
    return LogicAndBuilder(env_)
        .And(TaggedIsHeapObject(obj))
        .And(Int32Equal(GetObjectType(LoadHClass(obj)), Int32(static_cast<int32_t>(jsType))))
        .Done();
}

inline GateRef StubBuilder::IsJSRegExp(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_REG_EXP)));
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

inline GateRef StubBuilder::IsCOWArray(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return env_->GetBuilder()->IsCOWArray(objectType);
}

inline GateRef StubBuilder::IsMutantTaggedArray(GateRef elements)
{
    GateRef objectType = GetObjectType(LoadHClass(elements));
    return env_->GetBuilder()->IsMutantTaggedArray(objectType);
}

inline GateRef StubBuilder::IsWritable(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::WritableField::START_BIT))),
            Int32((1LLU << PropertyAttributes::WritableField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsDefaultAttribute(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::DefaultAttributesField::START_BIT))),
            Int32((1LLU << PropertyAttributes::DefaultAttributesField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsConfigable(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::ConfigurableField::START_BIT))),
            Int32((1LLU << PropertyAttributes::ConfigurableField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsAccessor(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::IsAccessorField::START_BIT))),
            Int32((1LLU << PropertyAttributes::IsAccessorField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsEnumerable(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::EnumerableField::START_BIT))),
            Int32((1LLU << PropertyAttributes::EnumerableField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsInlinedProperty(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::IsInlinedPropsField::START_BIT))),
            Int32((1LLU << PropertyAttributes::IsInlinedPropsField::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::GetPrototypeHandlerProtoCell(GateRef object)
{
    GateRef protoCellOffset = IntPtr(PrototypeHandler::PROTO_CELL_OFFSET);
    return Load(VariableType::JS_POINTER(), object, protoCellOffset);
}

inline GateRef StubBuilder::GetTransWithProtoHandlerProtoCell(GateRef object)
{
    GateRef protoCellOffset = IntPtr(TransWithProtoHandler::PROTO_CELL_OFFSET);
    return Load(VariableType::JS_POINTER(), object, protoCellOffset);
}

inline GateRef StubBuilder::GetStoreAOTHandlerProtoCell(GateRef object)
{
    GateRef protoCellOffset = IntPtr(StoreAOTHandler::PROTO_CELL_OFFSET);
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

inline void StubBuilder::SetPrototypeHandlerHandlerInfo(GateRef glue, GateRef obj, GateRef value)
{
    GateRef handlerInfoOffset = IntPtr(PrototypeHandler::HANDLER_INFO_OFFSET);
    Store(VariableType::JS_ANY(), glue, obj, handlerInfoOffset, value);
}

inline GateRef StubBuilder::GetStoreAOTHandlerHolder(GateRef object)
{
    GateRef holderOffset = IntPtr(StoreAOTHandler::HOLDER_OFFSET);
    return Load(VariableType::JS_ANY(), object, holderOffset);
}

inline GateRef StubBuilder::GetStoreAOTHandlerHandlerInfo(GateRef object)
{
    GateRef handlerInfoOffset = IntPtr(StoreAOTHandler::HANDLER_INFO_OFFSET);
    return Load(VariableType::JS_ANY(), object, handlerInfoOffset);
}

inline GateRef StubBuilder::GetHasChanged(GateRef object)
{
    return env_->GetBuilder()->GetHasChanged(object);
}

inline GateRef StubBuilder::GetNotFoundHasChanged(GateRef object)
{
    return env_->GetBuilder()->GetNotFoundHasChanged(object);
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

inline GateRef StubBuilder::TaggedIsProtoChangeMarker(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsProtoChangeMarker(obj);
}

inline GateRef StubBuilder::GetEmptyArray(GateRef glue)
{
    return env_->GetBuilder()->GetEmptyArray(glue);
}

inline GateRef StubBuilder::GetCacheKindFromForInIterator(GateRef iter)
{
    return env_->GetBuilder()->GetCacheKindFromForInIterator(iter);
}

inline GateRef StubBuilder::GetLengthFromForInIterator(GateRef iter)
{
    return env_->GetBuilder()->GetLengthFromForInIterator(iter);
}

inline GateRef StubBuilder::GetIndexFromForInIterator(GateRef iter)
{
    return env_->GetBuilder()->GetIndexFromForInIterator(iter);
}

inline GateRef StubBuilder::GetKeysFromForInIterator(GateRef iter)
{
    return env_->GetBuilder()->GetKeysFromForInIterator(iter);
}

inline GateRef StubBuilder::GetObjectFromForInIterator(GateRef iter)
{
    return env_->GetBuilder()->GetObjectFromForInIterator(iter);
}

inline GateRef StubBuilder::GetCachedHClassFromForInIterator(GateRef iter)
{
    return env_->GetBuilder()->GetCachedHClassFromForInIterator(iter);
}

inline void StubBuilder::SetLengthOfForInIterator(GateRef glue, GateRef iter, GateRef length)
{
    env_->GetBuilder()->SetLengthOfForInIterator(glue, iter, length);
}

inline void StubBuilder::SetIndexOfForInIterator(GateRef glue, GateRef iter, GateRef index)
{
    env_->GetBuilder()->SetIndexOfForInIterator(glue, iter, index);
}

inline void StubBuilder::SetCacheKindForInIterator(GateRef glue, GateRef iter, GateRef cacheKind)
{
    env_->GetBuilder()->SetCacheKindForInIterator(glue, iter, cacheKind);
}

inline void StubBuilder::SetKeysOfForInIterator(GateRef glue, GateRef iter, GateRef keys)
{
    env_->GetBuilder()->SetKeysOfForInIterator(glue, iter, keys);
}

inline void StubBuilder::SetObjectOfForInIterator(GateRef glue, GateRef iter, GateRef object)
{
    env_->GetBuilder()->SetObjectOfForInIterator(glue, iter, object);
}

inline void StubBuilder::SetCachedHClassOfForInIterator(GateRef glue, GateRef iter, GateRef hclass)
{
    env_->GetBuilder()->SetCachedHClassOfForInIterator(glue, iter, hclass);
}

inline void StubBuilder::IncreaseIteratorIndex(GateRef glue, GateRef iter, GateRef index)
{
    env_->GetBuilder()->IncreaseIteratorIndex(glue, iter, index);
}

inline void StubBuilder::IncreaseArrayIteratorIndex(GateRef glue, GateRef iter, GateRef index)
{
    env_->GetBuilder()->IncreaseArrayIteratorIndex(glue, iter, index);
}

inline void StubBuilder::SetNextIndexOfArrayIterator(GateRef glue, GateRef iter, GateRef nextIndex)
{
    env_->GetBuilder()->SetNextIndexOfArrayIterator(glue, iter, nextIndex);
}

inline void StubBuilder::SetIteratedArrayOfArrayIterator(GateRef glue, GateRef iter, GateRef iteratedArray)
{
    env_->GetBuilder()->SetIteratedArrayOfArrayIterator(glue, iter, iteratedArray);
}

inline void StubBuilder::SetBitFieldOfArrayIterator(GateRef glue, GateRef iter, GateRef kind)
{
    env_->GetBuilder()->SetBitFieldOfArrayIterator(glue, iter, kind);
}

inline GateRef StubBuilder::GetArrayIterationKind(GateRef iter)
{
    return env_->GetBuilder()->GetArrayIterationKind(iter);
}

inline GateRef StubBuilder::IsField(GateRef attr)
{
    return Int32Equal(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::KindBit::START_BIT))),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::FIELD));
}

inline GateRef StubBuilder::IsNonSharedStoreField(GateRef attr)
{
    return Int32Equal(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::SWholeKindBit::START_BIT))),
            Int32((1LLU << HandlerBase::SWholeKindBit::SIZE) - 1)),
        Int32(HandlerBase::StoreHandlerKind::S_FIELD));
}

inline GateRef StubBuilder::IsStoreShared(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::SSharedBit::START_BIT))),
            Int32((1LLU << HandlerBase::SSharedBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::IsElement(GateRef attr)
{
    return Int32Equal(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::KindBit::START_BIT))),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::ELEMENT));
}

inline GateRef StubBuilder::IsStringElement(GateRef attr)
{
    return Int32Equal(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::KindBit::START_BIT))),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::STRING));
}

inline GateRef StubBuilder::IsNumber(GateRef attr)
{
    return Int32Equal(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::KindBit::START_BIT))),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::NUMBER));
}

inline GateRef StubBuilder::IsStringLength(GateRef attr)
{
    return Int32Equal(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::KindBit::START_BIT))),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::STRING_LENGTH));
}

inline GateRef StubBuilder::IsTypedArrayElement(GateRef attr)
{
    return Int32Equal(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::KindBit::START_BIT))),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::TYPED_ARRAY));
}

inline GateRef StubBuilder::IsNonExist(GateRef attr)
{
    return Int32Equal(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::KindBit::START_BIT))),
            Int32((1LLU << HandlerBase::KindBit::SIZE) - 1)),
        Int32(HandlerBase::HandlerKind::NON_EXIST));
}

inline GateRef StubBuilder::HandlerBaseIsAccessor(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::AccessorBit::START_BIT))),
            Int32((1LLU << HandlerBase::AccessorBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::HandlerBaseIsJSArray(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::IsJSArrayBit::START_BIT))),
            Int32((1LLU << HandlerBase::IsJSArrayBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::HandlerBaseIsInlinedProperty(GateRef attr)
{
    return Int32NotEqual(
        Int32And(
            TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::InlinedPropsBit::START_BIT))),
            Int32((1LLU << HandlerBase::InlinedPropsBit::SIZE) - 1)),
        Int32(0));
}

inline GateRef StubBuilder::HandlerBaseGetOffset(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::OffsetBit::START_BIT))),
        Int32((1LLU << HandlerBase::OffsetBit::SIZE) - 1));
}


inline GateRef StubBuilder::HandlerBaseGetAttrIndex(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::AttrIndexBit::START_BIT))),
        Int32((1LLU << HandlerBase::AttrIndexBit::SIZE) - 1));
}

inline GateRef StubBuilder::HandlerBaseGetRep(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::RepresentationBit::START_BIT))),
        Int32((1LLU << HandlerBase::RepresentationBit::SIZE) - 1));
}

inline GateRef StubBuilder::IsInvalidPropertyBox(GateRef obj)
{
    GateRef valueOffset = IntPtr(PropertyBox::VALUE_OFFSET);
    GateRef value = Load(VariableType::JS_ANY(), obj, valueOffset);
    return TaggedIsHole(value);
}

inline GateRef StubBuilder::IsAccessorPropertyBox(GateRef obj)
{
    GateRef valueOffset = IntPtr(PropertyBox::VALUE_OFFSET);
    GateRef value = Load(VariableType::JS_ANY(), obj, valueOffset);
    return TaggedIsAccessor(value);
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
        TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::OffsetField::START_BIT))),
        Int32((1LLU << PropertyAttributes::OffsetField::SIZE) - 1));
}

inline GateRef StubBuilder::GetSortedIndex(GateRef attr)
{
    return TruncInt64ToInt32(Int64And(
        Int64LSR(attr, Int64(PropertyAttributes::SortedIndexField::START_BIT)),
        Int64((1LLU << PropertyAttributes::SortedIndexField::SIZE) - 1)));
}

// SetDictionaryOrder func in property_attribute.h
inline GateRef StubBuilder::SetDictionaryOrderFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int64LSL(
        Int64((1LLU << PropertyAttributes::DictionaryOrderField::SIZE) - 1),
        Int64(PropertyAttributes::DictionaryOrderField::START_BIT));
    GateRef newVal = Int64Or(Int64And(attr, Int64Not(mask)),
        Int64LSL(value, Int64(PropertyAttributes::DictionaryOrderField::START_BIT)));
    return newVal;
}

inline GateRef StubBuilder::GetPrototypeFromHClass(GateRef hClass)
{
    return env_->GetBuilder()->GetPrototypeFromHClass(hClass);
}

inline GateRef StubBuilder::GetEnumCacheFromHClass(GateRef hClass)
{
    return env_->GetBuilder()->GetEnumCacheFromHClass(hClass);
}

inline GateRef StubBuilder::GetProtoChangeMarkerFromHClass(GateRef hClass)
{
    return env_->GetBuilder()->GetProtoChangeMarkerFromHClass(hClass);
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
    return Int32LSR(len, Int32(EcmaString::STRING_LENGTH_SHIFT_COUNT));
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

inline void StubBuilder::SetProtoChangeDetailsToHClass(VariableType type,
    GateRef glue, GateRef hClass, GateRef protoChange)
{
    GateRef offset = IntPtr(JSHClass::PROTO_CHANGE_DETAILS_OFFSET);
    Store(type, glue, hClass, offset, protoChange);
}

inline GateRef StubBuilder::GetProtoChangeDetails(GateRef hClass)
{
    GateRef offset = IntPtr(JSHClass::PROTO_CHANGE_DETAILS_OFFSET);
    return Load(VariableType::JS_ANY(), hClass, offset);
}

inline void StubBuilder::SetLayoutToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef attr,
                                           MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSHClass::LAYOUT_OFFSET);
    Store(type, glue, hClass, offset, attr, mAttr);
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

inline void StubBuilder::SetParentToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef parent)
{
    GateRef offset = IntPtr(JSHClass::PARENT_OFFSET);
    Store(type, glue, hClass, offset, parent);
}

inline void StubBuilder::SetIsPrototypeToHClass(GateRef glue, GateRef hClass, GateRef value)
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

inline void StubBuilder::SetIsAOT(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef oldValue = ZExtInt1ToInt32(value);
    GateRef bitfield = GetBitFieldFromHClass(hClass);
    GateRef mask = Int32LSL(
        Int32((1LU << JSHClass::IsAOTBit::SIZE) - 1),
        Int32(JSHClass::IsAOTBit::START_BIT));
    GateRef newVal = Int32Or(Int32And(bitfield, Int32Not(mask)),
        Int32LSL(oldValue, Int32(JSHClass::IsAOTBit::START_BIT)));
    SetBitFieldToHClass(glue, hClass, newVal);
}

inline GateRef StubBuilder::IsPrototypeHClass(GateRef hClass)
{
    GateRef bitfield = GetBitFieldFromHClass(hClass);
    return TruncInt32ToInt1(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsPrototypeBit::START_BIT)),
        Int32((1LU << JSHClass::IsPrototypeBit::SIZE) - 1)));
}

inline void StubBuilder::SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass, GateRef value,
                                                 GateRef attrOffset, VariableType type, MemoryAttribute mAttr)
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
    Store(type, glue, obj, ZExtInt32ToPtr(propOffset), value, mAttr);
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

inline GateRef StubBuilder::IsObjSizeTrackingInProgress(GateRef hclass)
{
    GateRef count = GetConstructionCounter(hclass);
    return Int32NotEqual(count, Int32(0));
}

inline GateRef StubBuilder::GetConstructionCounter(GateRef hclass)
{
    GateRef bitfield = Load(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    return Int32And(Int32LSR(bitfield,
        Int32(JSHClass::ConstructionCounterBits::START_BIT)),
        Int32((1LU << JSHClass::ConstructionCounterBits::SIZE) - 1));
}

inline void StubBuilder::SetConstructionCounter(GateRef glue, GateRef hclass, GateRef count)
{
    GateRef bitfield = Load(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    GateRef encodeValue = Int32LSL(count, Int32(JSHClass::ConstructionCounterBits::START_BIT));
    GateRef mask =
        Int32(((1LU << JSHClass::ConstructionCounterBits::SIZE) - 1) << JSHClass::ConstructionCounterBits::START_BIT);
    bitfield = Int32Or(Int32And(bitfield, Int32Not(mask)), encodeValue);
    Store(VariableType::INT32(), glue, hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET), bitfield);
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

inline GateRef StubBuilder::HasDeleteProperty(GateRef hClass)
{
    return env_->GetBuilder()->HasDeleteProperty(hClass);
}

inline GateRef StubBuilder::IsAOTHClass(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    return Int32NotEqual(Int32And(Int32LSR(bitfield,
        Int32(JSHClass::IsAOTBit::START_BIT)),
        Int32((1LU << JSHClass::IsAOTBit::SIZE) - 1)),
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

inline void StubBuilder::SetElementsKindToTrackInfo(GateRef glue, GateRef trackInfo, GateRef elementsKind)
{
    GateRef bitfield = Load(VariableType::INT32(), trackInfo, IntPtr(TrackInfo::BIT_FIELD_OFFSET));
    GateRef oldWithMask = Int32And(bitfield,
        Int32(~static_cast<uint32_t>(TrackInfo::ElementsKindBits::Mask())));
    GateRef newValue = Int32LSR(elementsKind, Int32(TrackInfo::ElementsKindBits::START_BIT));
    GateRef newBitfield = Int32Or(oldWithMask, newValue);
    Store(VariableType::INT32(), glue, trackInfo, IntPtr(TrackInfo::BIT_FIELD_OFFSET), newBitfield);
}

inline void StubBuilder::SetSpaceFlagToTrackInfo(GateRef glue, GateRef trackInfo, GateRef spaceFlag)
{
    GateRef bitfield = Load(VariableType::INT32(), trackInfo, IntPtr(TrackInfo::BIT_FIELD_OFFSET));
    GateRef oldWithMask = Int32And(bitfield,
        Int32(~static_cast<uint32_t>(TrackInfo::SpaceFlagBits::Mask())));
    GateRef newValue = Int32LSL(spaceFlag, Int32(TrackInfo::SpaceFlagBits::START_BIT));
    GateRef newBitfield = Int32Or(oldWithMask, newValue);
    Store(VariableType::INT32(), glue, trackInfo, IntPtr(TrackInfo::BIT_FIELD_OFFSET), newBitfield);
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
                                               GateRef index, GateRef val, MemoryAttribute mAttr)
{
    // NOTE: need to translate MarkingBarrier
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    Store(valType, glue, array, dataOffset, val, mAttr);
}

inline GateRef StubBuilder::GetValueFromTaggedArray(GateRef array, GateRef index)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(VariableType::JS_ANY(), array, dataOffset);
}

inline GateRef StubBuilder::GetDataPtrInTaggedArray(GateRef array)
{
    return PtrAdd(array, IntPtr(TaggedArray::DATA_OFFSET));
}

inline GateRef StubBuilder::GetDataPtrInTaggedArray(GateRef array, GateRef index)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return PtrAdd(array, dataOffset);
}

inline GateRef StubBuilder::GetUnsharedConstpoolIndex(GateRef constpool)
{
    GateRef constPoolSize = GetLengthOfTaggedArray(constpool);
    GateRef unshareIdx = Int32Sub(constPoolSize, Int32(ConstantPool::UNSHARED_CONSTPOOL_INDEX));
    return GetValueFromTaggedArray(constpool, unshareIdx);
}

inline GateRef StubBuilder::GetUnsharedConstpoolFromGlue(GateRef glue, GateRef constpool)
{
    return env_->GetBuilder()->GetUnsharedConstpoolFromGlue(glue, constpool);
}

inline GateRef StubBuilder::GetUnsharedConstpool(GateRef arrayAddr, GateRef index)
{
    GateRef dataOffset =
        PtrAdd(arrayAddr, PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), ZExtInt32ToPtr(TaggedGetInt(index))));
    return Load(VariableType::JS_ANY(), dataOffset);
}

inline GateRef StubBuilder::GetValueFromMutantTaggedArray(GateRef elements, GateRef index)
{
    GateRef offset = PtrMul(ZExtInt32ToPtr(index), IntPtr(sizeof(int64_t)));
    GateRef dataOffset = PtrAdd(offset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(VariableType::INT64(), elements, dataOffset);
}

inline GateRef StubBuilder::IsSpecialIndexedObj(GateRef jsType)
{
    return Int32GreaterThan(jsType, Int32(static_cast<int32_t>(JSType::JS_ARRAY)));
}

inline void StubBuilder::SharedObjectStoreBarrierWithTypeCheck(bool isDicMode, Variable *result, GateRef glue,
    GateRef attr, GateRef value, Variable *newValue, Label *executeSharedSetProp, Label *exit)
{
    auto *env = GetEnvironment();
    Label barrier(env);
    Label typeMismatch(env);
    GateRef fieldType = isDicMode ? GetDictSharedFieldTypeInPropAttr(attr) : GetSharedFieldTypeInPropAttr(attr);
    SharedObjectStoreBarrierWithTypeCheck(result, glue, fieldType, value, newValue, executeSharedSetProp, exit);
}

inline void StubBuilder::SharedObjectStoreBarrierWithTypeCheck(bool isDicMode, Variable *result, GateRef glue,
    GateRef attr, GateRef value, Variable *newValue, Label *executeSharedSetProp, Label *exit,
    GateRef SCheckModelIsCHECK)
{
    auto *env = GetEnvironment();
    Label checkFieldTypeAndStoreBarrier(env);
    Label storeBarrierOnly(env);
    BRANCH(SCheckModelIsCHECK, &checkFieldTypeAndStoreBarrier, &storeBarrierOnly);
    Bind(&checkFieldTypeAndStoreBarrier);
    {
        SharedObjectStoreBarrierWithTypeCheck(isDicMode, result, glue, attr, value, newValue,
            executeSharedSetProp, exit);
    }
    Bind(&storeBarrierOnly);
    {
        SharedObjectStoreBarrier(glue, value, newValue, executeSharedSetProp);
    }
}

inline void StubBuilder::SharedObjectStoreBarrierWithTypeCheck(Variable *result, GateRef glue, GateRef fieldType,
    GateRef value, Variable *newValue, Label *executeSharedSetProp, Label *exit)
{
    auto *env = GetEnvironment();
    Label barrier(env);
    Label typeMismatch(env);
    MatchFieldType(glue, fieldType, value, &barrier, &typeMismatch);
    Bind(&typeMismatch);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetTypeMismatchedSharedProperty));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), {IntToTaggedInt(taggedId)});
        *result = Exception();
        Jump(exit);
    }
    Bind(&barrier);
    {
        SharedObjectStoreBarrier(glue, value, newValue, executeSharedSetProp);
    }
}

inline void StubBuilder::SharedObjectStoreBarrier(GateRef glue, GateRef value, Variable *newValue, Label *exit)
{
    auto *env = GetEnvironment();
    Label checkIsTreeString(env);
    BRANCH(TaggedIsHeapObject(value), &checkIsTreeString, exit);
    Bind(&checkIsTreeString);
    {
        Label needFlatten(env);
        BRANCH(IsTreeString(value), &needFlatten, exit);
        Bind(&needFlatten);
        {
            *newValue = CallRuntime(glue, RTSTUB_ID(SlowSharedObjectStoreBarrier), { value });
            Jump(exit);
        }
    }
}

inline GateRef StubBuilder::GetFieldTypeFromHandler(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(HandlerBase::SFieldTypeBit::START_BIT))),
        Int32((1LLU << HandlerBase::SFieldTypeBit::SIZE) - 1));
}

inline GateRef StubBuilder::ClearSharedStoreKind(GateRef handlerInfo)
{
    return Int64And(handlerInfo, Int64Not(Int64(HandlerBase::SSharedBit::Mask())));
}

inline GateRef StubBuilder::UpdateSOutOfBoundsForHandler(GateRef handlerInfo)
{
    return Int64Or(handlerInfo, Int64(HandlerBase::SOutOfBoundsBit::Mask()));
}

inline GateRef StubBuilder::IsArrayListOrVector(GateRef jsType)
{
    // arraylist and vector has fast pass now
    return BitOr(Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_API_ARRAY_LIST))),
                 Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_API_VECTOR))));
}

inline GateRef StubBuilder::IsSharedArray(GateRef jsType)
{
    return Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_SHARED_ARRAY)));
}

inline GateRef StubBuilder::IsFastTypeArray(GateRef jsType)
{
    return BitAnd(
        Int32GreaterThan(jsType, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_FIRST))),
        Int32LessThanOrEqual(jsType, Int32(static_cast<int32_t>(JSType::JS_FLOAT64_ARRAY))));
}

inline GateRef StubBuilder::IsJSProxy(GateRef jsType)
{
    return Int32Equal(jsType, Int32(static_cast<int32_t>(JSType::JS_PROXY)));
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

inline GateRef StubBuilder::GetIhcFromAOTLiteralInfo(GateRef info)
{
    auto len = GetLengthOfTaggedArray(info);
    GateRef aotIhcIndex = Int32Sub(len, Int32(AOTLiteralInfo::AOT_IHC_INDEX));
    GateRef ihcOffset = Int32Mul(aotIhcIndex, Int32(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(ihcOffset, IntPtr(TaggedArray::DATA_OFFSET));
    return Load(VariableType::JS_ANY(), info, dataOffset);
}

inline void StubBuilder::UpdateFieldType(GateRef glue, GateRef hclass, GateRef attr)
{
    CallNGCRuntime(glue, RTSTUB_ID(UpdateFieldType), { hclass, attr });
}

inline GateRef StubBuilder::GetPropertyMetaDataFromAttr(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::PropertyMetaDataField::START_BIT))),
        Int32((1LLU << PropertyAttributes::PropertyMetaDataField::SIZE) - 1));
}

inline GateRef StubBuilder::TranslateToRep(GateRef value)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    Label intLabel(env);
    Label nonIntLabel(env);
    Label doubleLabel(env);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(static_cast<int32_t>(Representation::TAGGED)));
    Branch(TaggedIsInt(value), &intLabel, &nonIntLabel);
    Bind(&intLabel);
    {
        result = Int32(static_cast<int32_t>(Representation::INT));
        Jump(&exit);
    }
    Bind(&nonIntLabel);
    {
        Branch(TaggedIsDouble(value), &doubleLabel, &exit);
    }
    Bind(&doubleLabel);
    {
        result = Int32(static_cast<int32_t>(Representation::DOUBLE));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
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

inline GateRef StubBuilder::IsInternalString(GateRef string)
{
    return env_->GetBuilder()->IsInternString(string);
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

inline GateRef StubBuilder::GetInt32OfTNumber(GateRef x)
{
    return env_->GetBuilder()->GetInt32OfTNumber(x);
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

inline GateRef StubBuilder::TruncDoubleToFloat32(GateRef x)
{
    return env_->GetBuilder()->TruncDoubleToFloat32(x);
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

inline GateRef StubBuilder::GetGlobalConstantOffset(ConstantIndex index)
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
    return res;
}

inline GateRef StubBuilder::TaggedIsCallable(GateRef obj)
{
    return LogicAndBuilder(env_)
        .And(TaggedIsHeapObject(obj))
        .And(env_->GetBuilder()->IsCallable(obj))
        .Done();
}

// GetOffset func in property_attribute.h
inline GateRef StubBuilder::GetOffsetFieldInPropAttr(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::OffsetField::START_BIT))),
        Int32((1LLU << PropertyAttributes::OffsetField::SIZE) - 1));
}

// SetOffset func in property_attribute.h
inline GateRef StubBuilder::SetOffsetFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int64LSL(
        Int64((1LLU << PropertyAttributes::OffsetField::SIZE) - 1),
        Int64(PropertyAttributes::OffsetField::START_BIT));
    GateRef newVal = Int64Or(Int64And(attr, Int64Not(mask)),
        Int64LSL(ZExtInt32ToInt64(value), Int64(PropertyAttributes::OffsetField::START_BIT)));
    return newVal;
}

// SetIsInlinedProps func in property_attribute.h
inline GateRef StubBuilder::SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value)
{
    GateRef mask = Int64LSL(
        Int64((1LU << PropertyAttributes::IsInlinedPropsField::SIZE) - 1),
        Int64(PropertyAttributes::IsInlinedPropsField::START_BIT));
    GateRef newVal = Int64Or(Int64And(attr, Int64Not(mask)),
        Int64LSL(ZExtInt32ToInt64(value), Int64(PropertyAttributes::IsInlinedPropsField::START_BIT)));
    return newVal;
}


inline GateRef StubBuilder::SetTrackTypeInPropAttr(GateRef attr, GateRef type)
{
    GateRef mask = Int64LSL(
        Int64((1LU << PropertyAttributes::TrackTypeField::SIZE) - 1),
        Int64(PropertyAttributes::TrackTypeField::START_BIT));
    GateRef newVal = Int64Or(Int64And(attr, Int64Not(mask)),
        Int64LSL(ZExtInt32ToInt64(type), Int64(PropertyAttributes::TrackTypeField::START_BIT)));
    return newVal;
}

inline GateRef StubBuilder::GetSharedFieldTypeInPropAttr(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::SharedFieldTypeField::START_BIT))),
        Int32((1LLU << PropertyAttributes::SharedFieldTypeField::SIZE) - 1));
}

inline GateRef StubBuilder::GetTrackTypeInPropAttr(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::TrackTypeField::START_BIT))),
        Int32((1LLU << PropertyAttributes::TrackTypeField::SIZE) - 1));
}

inline GateRef StubBuilder::GetDictSharedFieldTypeInPropAttr(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::DictSharedFieldTypeField::START_BIT))),
        Int32((1LLU << PropertyAttributes::DictSharedFieldTypeField::SIZE) - 1));
}

inline GateRef StubBuilder::GetRepInPropAttr(GateRef attr)
{
    return Int32And(
        TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::RepresentationField::START_BIT))),
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

inline GateRef StubBuilder::IsTaggedRepInPropAttr(GateRef attr)
{
    GateRef rep = GetRepInPropAttr(attr);
    return BitAnd(BoolNot(IsDoubleRepInPropAttr(rep)), BoolNot(IsIntRepInPropAttr(rep)));
}

inline GateRef StubBuilder::SetTaggedRepInPropAttr(GateRef attr)
{
    GateRef mask = Int64LSL(
        Int64((1LU << PropertyAttributes::RepresentationField::SIZE) - 1),
        Int64(PropertyAttributes::RepresentationField::START_BIT));
    GateRef targetType = Int32(static_cast<uint32_t>(Representation::TAGGED));
    GateRef newVal = Int64Or(Int64And(attr, Int64Not(mask)),
        Int64LSL(ZExtInt32ToInt64(targetType), Int64(PropertyAttributes::RepresentationField::START_BIT)));
    return newVal;
}

template<class T>
void StubBuilder::SetHClassBit(GateRef glue, GateRef hClass, GateRef value)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    GateRef mask = Int32LSL(
        Int32((1LU << T::SIZE) - 1),
        Int32(T::START_BIT));
    GateRef newVal = Int32Or(Int32And(bitfield, Int32Not(mask)),
        Int32LSL(value, Int32(T::START_BIT)));
    Store(VariableType::INT32(), glue, hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET), newVal);
}

inline GateRef StubBuilder::IntPtrEuqal(GateRef x, GateRef y)
{
    return env_->Is32Bit() ? Int32Equal(x, y) : Int64Equal(x, y);
}

inline GateRef StubBuilder::IntPtrNotEqual(GateRef x, GateRef y)
{
    return env_->Is32Bit() ? Int32NotEqual(x, y) : Int64NotEqual(x, y);
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

inline GateRef StubBuilder::RegionInSpace(GateRef region, RegionSpaceFlag space)
{
    auto offset = Region::PackedData::GetFlagsOffset(env_->Is32Bit());
    GateRef x = Load(VariableType::NATIVE_POINTER(), PtrAdd(IntPtr(offset), region),
        IntPtr(0));
    if (env_->Is32Bit()) {
        return Int32Equal(Int32And(x,
            Int32(RegionSpaceFlag::VALID_SPACE_MASK)), Int32(space));
    } else {
        return Int64Equal(Int64And(x,
            Int64(RegionSpaceFlag::VALID_SPACE_MASK)), Int64(space));
    }
}

inline GateRef StubBuilder::InYoungGeneration(GateRef region)
{
    return RegionInSpace(region, RegionSpaceFlag::IN_YOUNG_SPACE);
}

inline GateRef StubBuilder::RegionInSpace(GateRef region, RegionSpaceFlag spaceBegin, RegionSpaceFlag spaceEnd)
{
    auto offset = Region::PackedData::GetFlagsOffset(env_->Is32Bit());
    GateRef x = Load(VariableType::NATIVE_POINTER(), PtrAdd(IntPtr(offset), region),
        IntPtr(0));
    if (env_->Is32Bit()) {
        GateRef spaceType = Int32And(x, Int32(RegionSpaceFlag::VALID_SPACE_MASK));
        GateRef greater = Int32GreaterThanOrEqual(spaceType, Int32(spaceBegin));
        GateRef less = Int32LessThanOrEqual(spaceType, Int32(spaceEnd));
        return BitAnd(greater, less);
    } else {
        GateRef spaceType = Int64And(x, Int64(RegionSpaceFlag::VALID_SPACE_MASK));
        GateRef greater = Int64GreaterThanOrEqual(spaceType, Int64(spaceBegin));
        GateRef less = Int64LessThanOrEqual(spaceType, Int64(spaceEnd));
        return BitAnd(greater, less);
    }
}

inline GateRef StubBuilder::InGeneralOldGeneration(GateRef region)
{
    return RegionInSpace(region, RegionSpaceFlag::GENERAL_OLD_BEGIN, RegionSpaceFlag::GENERAL_OLD_END);
}

inline GateRef StubBuilder::InSharedHeap(GateRef region)
{
    auto offset = Region::PackedData::GetFlagsOffset(env_->Is32Bit());
    GateRef x = Load(VariableType::NATIVE_POINTER(), PtrAdd(IntPtr(offset), region),
        IntPtr(0));
    if (env_->Is32Bit()) {
        GateRef spaceType = Int32And(x, Int32(RegionSpaceFlag::VALID_SPACE_MASK));
        GateRef greater = Int32GreaterThanOrEqual(spaceType, Int32(RegionSpaceFlag::SHARED_SPACE_BEGIN));
        GateRef less = Int32LessThanOrEqual(spaceType, Int32(RegionSpaceFlag::SHARED_SPACE_END));
        return BitAnd(greater, less);
    } else {
        GateRef spaceType = Int64And(x, Int64(RegionSpaceFlag::VALID_SPACE_MASK));
        GateRef greater = Int64GreaterThanOrEqual(spaceType, Int64(RegionSpaceFlag::SHARED_SPACE_BEGIN));
        GateRef less = Int64LessThanOrEqual(spaceType, Int64(RegionSpaceFlag::SHARED_SPACE_END));
        return BitAnd(greater, less);
    }
}

inline GateRef StubBuilder::InSharedSweepableSpace(GateRef region)
{
    auto offset = Region::PackedData::GetFlagsOffset(env_->Is32Bit());
    GateRef x = Load(VariableType::NATIVE_POINTER(), PtrAdd(IntPtr(offset), region),
        IntPtr(0));
    if (env_->Is32Bit()) {
        GateRef spaceType = Int32And(x, Int32(RegionSpaceFlag::VALID_SPACE_MASK));
        GateRef greater = Int32GreaterThanOrEqual(spaceType, Int32(RegionSpaceFlag::SHARED_SWEEPABLE_SPACE_BEGIN));
        GateRef less = Int32LessThanOrEqual(spaceType, Int32(RegionSpaceFlag::SHARED_SWEEPABLE_SPACE_END));
        return BitAnd(greater, less);
    } else {
        GateRef spaceType = Int64And(x, Int64(RegionSpaceFlag::VALID_SPACE_MASK));
        GateRef greater = Int64GreaterThanOrEqual(spaceType, Int64(RegionSpaceFlag::SHARED_SWEEPABLE_SPACE_BEGIN));
        GateRef less = Int64LessThanOrEqual(spaceType, Int64(RegionSpaceFlag::SHARED_SWEEPABLE_SPACE_END));
        return BitAnd(greater, less);
    }
}

inline GateRef StubBuilder::GetParentEnv(GateRef object)
{
    return env_->GetBuilder()->GetParentEnv(object);
}

inline GateRef StubBuilder::GetSendableParentEnv(GateRef object)
{
    return env_->GetBuilder()->GetSendableParentEnv(object);
}

inline GateRef StubBuilder::GetPropertiesFromLexicalEnv(GateRef object, GateRef index)
{
    return env_->GetBuilder()->GetPropertiesFromLexicalEnv(object, index);
}

inline GateRef StubBuilder::GetPropertiesFromSendableEnv(GateRef object, GateRef index)
{
    return env_->GetBuilder()->GetPropertiesFromSendableEnv(object, index);
}

inline GateRef StubBuilder::GetKeyFromLexivalEnv(GateRef lexicalEnv, GateRef levelIndex, GateRef slotIndex)
{
    return env_->GetBuilder()->GetKeyFromLexivalEnv(lexicalEnv, levelIndex, slotIndex);
}

inline void StubBuilder::SetPropertiesToSendableEnv(GateRef glue, GateRef object, GateRef index, GateRef value)
{
    GateRef valueIndex = Int32Add(index, Int32(SendableEnv::SENDABLE_RESERVED_ENV_LENGTH));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, object, valueIndex, value);
}

inline GateRef StubBuilder::GetSendableEnvFromModule(GateRef module)
{
    return env_->GetBuilder()->GetSendableEnvFromModule(module);
}

inline void StubBuilder::SetSendableEnvToModule(GateRef glue, GateRef module, GateRef value, MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(SourceTextModule::SENDABLE_ENV_OFFSET);
    mAttr.SetShare(MemoryAttribute::SHARED);
    Store(VariableType::JS_POINTER(), glue, module, offset, value, mAttr);
}

inline GateRef StubBuilder::GetHomeObjectFromJSFunction(GateRef object)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return Load(VariableType::JS_ANY(), object, offset);
}

inline GateRef StubBuilder::GetMethodFromJSFunctionOrProxy(GateRef object)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);

    GateRef methodOffset;
    Label funcIsJSFunctionBase(env);
    Label funcIsJSProxy(env);
    Label getMethod(env);
    BRANCH(IsJSFunctionBase(object), &funcIsJSFunctionBase, &funcIsJSProxy);
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

inline void StubBuilder::SetLexicalEnvToFunction(GateRef glue, GateRef object, GateRef lexicalEnv,
                                                 MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunction::LEXICAL_ENV_OFFSET);
    Store(VariableType::JS_ANY(), glue, object, offset, lexicalEnv, mAttr);
}

inline void StubBuilder::SetProtoOrHClassToFunction(GateRef glue, GateRef function, GateRef value,
                                                    MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value, mAttr);
}

inline GateRef StubBuilder::GetProtoOrHClass(GateRef function)
{
    GateRef offset = IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET);
    return Load(VariableType::JS_ANY(), function, offset);
}

inline void StubBuilder::SetTypedArrayName(GateRef glue, GateRef typedArray, GateRef name,
                                           MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSTypedArray::TYPED_ARRAY_NAME_OFFSET);
    Store(VariableType::JS_ANY(), glue, typedArray, offset, name, mAttr);
}

inline void StubBuilder::SetContentType(GateRef glue, GateRef typedArray, GateRef type)
{
    GateRef offset = IntPtr(JSTypedArray::CONTENT_TYPE_OFFSET);
    Store(VariableType::INT8(), glue, typedArray, offset, type, MemoryAttribute::NoBarrier());
}

inline void StubBuilder::SetViewedArrayBufferOrByteArray(GateRef glue, GateRef typedArray, GateRef data,
    MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    Store(VariableType::JS_ANY(), glue, typedArray, offset, data, mAttr);
}

inline GateRef StubBuilder::GetViewedArrayBufferOrByteArray(GateRef typedArray)
{
    GateRef offset = IntPtr(JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    return Load(VariableType::JS_ANY(), typedArray, offset);
}

inline void StubBuilder::SetByteLength(GateRef glue, GateRef typedArray, GateRef byteLength)
{
    GateRef offset = IntPtr(JSTypedArray::BYTE_LENGTH_OFFSET);
    Store(VariableType::INT8(), glue, typedArray, offset, byteLength, MemoryAttribute::NoBarrier());
}

inline void StubBuilder::SetByteOffset(GateRef glue, GateRef typedArray, GateRef byteOffset)
{
    GateRef offset = IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET);
    Store(VariableType::INT8(), glue, typedArray, offset, byteOffset, MemoryAttribute::NoBarrier());
}

inline void StubBuilder::SetTypedArrayLength(GateRef glue, GateRef typedArray, GateRef arrayLength)
{
    GateRef offset = IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET);
    Store(VariableType::INT8(), glue, typedArray, offset, arrayLength, MemoryAttribute::NoBarrier());
}

inline void StubBuilder::SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value,
                                                 MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value, mAttr);
}

inline void StubBuilder::SetModuleToFunction(GateRef glue, GateRef function, GateRef value,
                                             MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, function, offset, value, mAttr);
}

inline void StubBuilder::SetWorkNodePointerToFunction(GateRef glue, GateRef function, GateRef value,
                                                      MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunction::WORK_NODE_POINTER_OFFSET);
    Store(VariableType::NATIVE_POINTER(), glue, function, offset, value, mAttr);
}

inline void StubBuilder::SetMethodToFunction(GateRef glue, GateRef function, GateRef value,
                                             MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunctionBase::METHOD_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value, mAttr);
}

inline void StubBuilder::SetCodeEntryToFunctionFromMethod(GateRef glue, GateRef function, GateRef method)
{
    GateRef codeEntryOffset = IntPtr(Method::CODEENTRY_LITERAL_OFFSET);
    GateRef codeEntry = Load(VariableType::NATIVE_POINTER(), method, codeEntryOffset);
    GateRef funcOffset = IntPtr(JSFunctionBase::CODE_ENTRY_OFFSET);
    Store(VariableType::NATIVE_POINTER(), glue, function, funcOffset, codeEntry);
}

inline void StubBuilder::SetCodeEntryToFunctionFromFuncEntry(GateRef glue, GateRef function, GateRef codeEntry)
{
    GateRef funcOffset = IntPtr(JSFunctionBase::CODE_ENTRY_OFFSET);
    Store(VariableType::NATIVE_POINTER(), glue, function, funcOffset, codeEntry);
}

inline void StubBuilder::SetNativePointerToFunctionFromMethod(GateRef glue, GateRef function, GateRef method)
{
    GateRef nativePointerOffset = IntPtr(Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET);
    GateRef nativePointer = Load(VariableType::NATIVE_POINTER(), method, nativePointerOffset);
    GateRef funcOffset = IntPtr(JSFunctionBase::CODE_ENTRY_OFFSET);
    Store(VariableType::NATIVE_POINTER(), glue, function, funcOffset, nativePointer);
}

inline void StubBuilder::SetLengthToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunctionBase::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, function, offset, value, MemoryAttribute::NoBarrier());
}

inline GateRef StubBuilder::GetLengthFromFunction(GateRef function)
{
    return Load(VariableType::JS_NOT_POINTER(), function, IntPtr(JSFunctionBase::LENGTH_OFFSET));
}

inline void StubBuilder::SetRawProfileTypeInfoToFunction(GateRef glue, GateRef function, GateRef value,
                                                         MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value, mAttr);
}

inline void StubBuilder::SetValueToProfileTypeInfoCell(GateRef glue, GateRef profileTypeInfoCell, GateRef value)
{
    GateRef offset = IntPtr(ProfileTypeInfoCell::VALUE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, profileTypeInfoCell, offset, value);
}

inline void StubBuilder::UpdateProfileTypeInfoCellType(GateRef glue, GateRef profileTypeInfoCell)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    // ProfileTypeInfoCell0 -> Cell1 -> CellN
    Label isProfileTypeInfoCell0(env);
    Label notProfileTypeInfoCell0(env);
    Label isProfileTypeInfoCell1(env);
    Label endProfileTypeInfoCellType(env);
    GateRef objectType = GetObjectType(LoadHClass(profileTypeInfoCell));
    BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::PROFILE_TYPE_INFO_CELL_0))),
           &isProfileTypeInfoCell0, &notProfileTypeInfoCell0);
    Bind(&isProfileTypeInfoCell0);
    {
        auto profileTypeInfoCell1Class = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                                ConstantIndex::PROFILE_TYPE_INFO_CELL_1_CLASS_INDEX);
        StoreHClassWithoutBarrier(glue, profileTypeInfoCell, profileTypeInfoCell1Class);
        Jump(&endProfileTypeInfoCellType);
    }
    Bind(&notProfileTypeInfoCell0);
    BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::PROFILE_TYPE_INFO_CELL_1))),
           &isProfileTypeInfoCell1, &endProfileTypeInfoCellType);
    Bind(&isProfileTypeInfoCell1);
    {
        auto profileTypeInfoCellNClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                                ConstantIndex::PROFILE_TYPE_INFO_CELL_N_CLASS_INDEX);
        StoreHClassWithoutBarrier(glue, profileTypeInfoCell, profileTypeInfoCellNClass);
        Jump(&endProfileTypeInfoCellType);
    }
    Bind(&endProfileTypeInfoCellType);
    env->SubCfgExit();
}

inline void StubBuilder::SetJSObjectTaggedField(GateRef glue, GateRef object, size_t offset, GateRef value)
{
    Store(VariableType::JS_ANY(), glue, object, IntPtr(offset), value);
}

inline void StubBuilder::SetCompiledCodeFlagToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef bitFieldOffset = IntPtr(JSFunctionBase::BIT_FIELD_OFFSET);
    GateRef oldVal = Load(VariableType::INT32(), function, bitFieldOffset);

    GateRef mask = Int32(JSFunctionBase::COMPILED_CODE_FASTCALL_BITS << JSFunctionBase::IsCompiledCodeBit::START_BIT);
    GateRef newVal = Int32Or(Int32And(oldVal, Int32Not(mask)), value);
    Store(VariableType::INT32(), glue, function, bitFieldOffset, newVal, MemoryAttribute::NoBarrier());
}

inline void StubBuilder::SetCompiledFuncEntry(GateRef glue, GateRef jsFunc, GateRef codeEntry, GateRef isFastCall)
{
    SetCodeEntryToFunctionFromFuncEntry(glue, jsFunc, codeEntry);
    GateRef compiledCodeFastCallBits = Int32Add(Int32(1),
                                                Int32LSL(isFastCall, Int32(JSFunctionBase::IsFastCallBit::START_BIT)));
    SetCompiledCodeFlagToFunction(glue, jsFunc, compiledCodeFastCallBits);
}

inline GateRef StubBuilder::GetIsFastCall(GateRef machineCode)
{
    GateRef bitfield = Load(VariableType::INT32(), machineCode, IntPtr(MachineCode::BIT_FIELD_OFFSET));
    return Int32And(Int32LSR(bitfield, Int32(MachineCode::IsFastCallBits::START_BIT)),
                    Int32((1LU << MachineCode::IsFastCallBits::SIZE) - 1));
}

inline void StubBuilder::SetTaskConcurrentFuncFlagToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef bitFieldOffset = IntPtr(JSFunctionBase::BIT_FIELD_OFFSET);
    GateRef oldVal = Load(VariableType::INT32(), function, bitFieldOffset);
    GateRef mask = Int32LSL(
        Int32((1LU << JSFunctionBase::TaskConcurrentFuncFlagBit::SIZE) - 1),
        Int32(JSFunctionBase::TaskConcurrentFuncFlagBit::START_BIT));
    GateRef newVal = Int32Or(Int32And(oldVal, Int32Not(mask)),
        Int32LSL(ZExtInt1ToInt32(value), Int32(JSFunctionBase::TaskConcurrentFuncFlagBit::START_BIT)));
    Store(VariableType::INT32(), glue, function, bitFieldOffset, newVal);
}

inline void StubBuilder::SetBitFieldToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef bitFieldOffset = IntPtr(JSFunctionBase::BIT_FIELD_OFFSET);
    Store(VariableType::INT32(), glue, function, bitFieldOffset, value);
}

inline void StubBuilder::SetMachineCodeToFunction(GateRef glue, GateRef function, GateRef value, MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunction::MACHINECODE_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value, mAttr);
}

inline void StubBuilder::SetBaselineJitCodeToFunction(GateRef glue, GateRef function, GateRef value,
    MemoryAttribute mAttr)
{
    GateRef offset = IntPtr(JSFunction::BASELINECODE_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value, mAttr);
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

inline GateRef StubBuilder::GetHomeObjectFromFunction(GateRef function)
{
    return env_->GetBuilder()->GetHomeObjectFromFunction(function);
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
    return BitAnd(greater, less);
}

inline GateRef StubBuilder::IsBoundFunction(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::JS_BOUND_FUNCTION)));
}

inline GateRef StubBuilder::IsJSOrBoundFunction(GateRef obj)
{
    GateRef objectType = GetObjectType(LoadHClass(obj));
    GateRef greater = Int32GreaterThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_FUNCTION_FIRST)));
    GateRef less = Int32LessThanOrEqual(objectType,
        Int32(static_cast<int32_t>(JSType::JS_BOUND_FUNCTION)));
    return BitAnd(greater, less);
}

inline GateRef StubBuilder::IsAOTLiteralInfo(GateRef info)
{
    return env_->GetBuilder()->IsAOTLiteralInfo(info);
}

inline GateRef StubBuilder::IsAotWithCallField(GateRef method)
{
    GateRef callFieldOffset = IntPtr(Method::CALL_FIELD_OFFSET);
    GateRef callfield = Load(VariableType::INT64(), method, callFieldOffset);
    return Int64NotEqual(
        Int64And(
            Int64LSR(callfield, Int64(Method::IsAotCodeBit::START_BIT)),
            Int64((1LU << Method::IsAotCodeBit::SIZE) - 1)),
        Int64(0));
}

inline GateRef StubBuilder::IsFastCall(GateRef method)
{
    GateRef callFieldOffset = IntPtr(Method::CALL_FIELD_OFFSET);
    GateRef callfield = Load(VariableType::INT64(), method, callFieldOffset);
    return Int64NotEqual(
        Int64And(
            Int64LSR(callfield, Int64(Method::IsFastCallBit::START_BIT)),
            Int64((1LU << Method::IsFastCallBit::SIZE) - 1)),
        Int64(0));
}

inline GateRef StubBuilder::HasPrototype(GateRef kind)
{
    return LogicAndBuilder(env_)
        .And(Int32NotEqual(kind, Int32(static_cast<int32_t>(FunctionKind::BUILTIN_PROXY_CONSTRUCTOR))))
        .And(Int32GreaterThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::BASE_CONSTRUCTOR))))
        .And(Int32LessThanOrEqual(kind, Int32(static_cast<int32_t>(FunctionKind::ASYNC_GENERATOR_FUNCTION))))
        .Done();
}

inline GateRef StubBuilder::HasAccessor(GateRef kind)
{
    GateRef greater = Int32GreaterThanOrEqual(kind,
        Int32(static_cast<int32_t>(FunctionKind::NORMAL_FUNCTION)));
    GateRef less = Int32LessThanOrEqual(kind,
        Int32(static_cast<int32_t>(FunctionKind::ASYNC_FUNCTION)));
    return BitAnd(greater, less);
}

inline GateRef StubBuilder::IsClassConstructorKind(GateRef kind)
{
    GateRef left = Int32Equal(kind, Int32(static_cast<int32_t>(FunctionKind::CLASS_CONSTRUCTOR)));
    GateRef right = Int32Equal(kind, Int32(static_cast<int32_t>(FunctionKind::DERIVED_CONSTRUCTOR)));
    return BitOr(left, right);
}

inline GateRef StubBuilder::IsGeneratorKind(GateRef kind)
{
    GateRef left = Int32Equal(kind, Int32(static_cast<int32_t>(FunctionKind::ASYNC_GENERATOR_FUNCTION)));
    GateRef right = Int32Equal(kind, Int32(static_cast<int32_t>(FunctionKind::GENERATOR_FUNCTION)));
    return BitOr(left, right);
}

inline GateRef StubBuilder::IsBaseKind(GateRef kind)
{
    GateRef val = Int32Equal(kind, Int32(static_cast<int32_t>(FunctionKind::BASE_CONSTRUCTOR)));
    return BitOr(val, IsGeneratorKind(kind));
}

inline GateRef StubBuilder::IsBaseConstructorKind(GateRef kind)
{
    return Int32Equal(kind, Int32(static_cast<int32_t>(FunctionKind::BASE_CONSTRUCTOR)));
}

inline GateRef StubBuilder::IsSendableFunction(GateRef method)
{
    GateRef fieldOffset = IntPtr(Method::EXTRA_LITERAL_INFO_OFFSET);
    GateRef literalField = Load(VariableType::INT64(), method, fieldOffset);
    return Int64NotEqual(
        Int64And(
            Int64LSR(literalField, Int64(Method::IsSharedBit::START_BIT)),
            Int64((1LU << Method::IsSharedBit::SIZE) - 1)),
        Int64(0));
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

inline GateRef StubBuilder::JudgeAotAndFastCall(GateRef jsFunc, CircuitBuilder::JudgeMethodType type)
{
    return env_->GetBuilder()->JudgeAotAndFastCall(jsFunc, type);
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
    return env_->GetBuilder()->GetGlobalConstantValue(type, glue, index);
}

inline GateRef StubBuilder::GetGlobalConstantValue(VariableType type, GateRef glue, GateRef index)
{
    GateRef gConstAddr = Load(VariableType::JS_ANY(), glue,
                              IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_->Is32Bit())));
    auto constantIndex = PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), index);
    return Load(type, gConstAddr, constantIndex);
}

inline GateRef StubBuilder::GetSingleCharTable(GateRef glue)
{
    return GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::SINGLE_CHAR_TABLE_INDEX);
}

inline GateRef StubBuilder::IsEnableMutantArray(GateRef glue)
{
    GateRef offset = IntPtr(JSThread::GlueData::GetIsEnableMutantArrayOffset(env_->Is32Bit()));
    return Load(VariableType::BOOL(), glue, offset);
}

inline GateRef StubBuilder::IsEnableElementsKind(GateRef glue)
{
    GateRef offset = IntPtr(JSThread::GlueData::GetIsEnableElementsKindOffset(env_->Is32Bit()));
    return Load(VariableType::BOOL(), glue, offset);
}

inline GateRef StubBuilder::TaggedIsEnumCache(GateRef obj)
{
    return env_->GetBuilder()->TaggedIsEnumCache(obj);
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

inline GateRef StubBuilder::DispatchBuiltins(GateRef glue, GateRef builtinsId, const std::vector<GateRef>& args,
                                             GateRef hir)
{
    GateRef target = PtrMul(ZExtInt32ToPtr(builtinsId), IntPtrSize());
    return env_->GetBuilder()->CallBuiltin(glue, target, args, hir);
}

inline GateRef StubBuilder::DispatchBuiltinsWithArgv(GateRef glue, GateRef builtinsId, const std::vector<GateRef>& args)
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

inline GateRef StubBuilder::AlignDown(GateRef x, GateRef alignment)
{
    return IntPtrAnd(x, IntPtrNot(PtrSub(alignment, IntPtr(1))));
}

inline void StubBuilder::SetLength(GateRef glue, GateRef str, GateRef length, bool compressed)
{
    GateRef len = Int32LSL(length, Int32(EcmaString::STRING_LENGTH_SHIFT_COUNT));
    GateRef mixLength;
    if (compressed) {
        mixLength = Int32Or(len, Int32(EcmaString::STRING_COMPRESSED));
    } else {
        mixLength = Int32Or(len, Int32(EcmaString::STRING_UNCOMPRESSED));
    }
    Store(VariableType::INT32(), glue, str, IntPtr(EcmaString::MIX_LENGTH_OFFSET), mixLength);
}

inline void StubBuilder::SetLength(GateRef glue, GateRef str, GateRef length, GateRef isCompressed)
{
    GateRef len = Int32LSL(length, Int32(EcmaString::STRING_LENGTH_SHIFT_COUNT));
    GateRef mixLength = Int32Or(len, isCompressed);
    Store(VariableType::INT32(), glue, str, IntPtr(EcmaString::MIX_LENGTH_OFFSET), mixLength);
}

inline GateRef StubBuilder::IsIntegerString(GateRef string)
{
    return env_->GetBuilder()->IsIntegerString(string);
}

inline GateRef StubBuilder::GetRawHashFromString(GateRef value)
{
    return env_->GetBuilder()->GetRawHashFromString(value);
}

inline void StubBuilder::SetRawHashcode(GateRef glue, GateRef str, GateRef rawHashcode, GateRef isInteger)
{
    env_->GetBuilder()->SetRawHashcode(glue, str, rawHashcode, isInteger);
}

inline GateRef StubBuilder::TryGetHashcodeFromString(GateRef string)
{
    return env_->GetBuilder()->TryGetHashcodeFromString(string);
}

inline GateRef StubBuilder::GetMixHashcode(GateRef string)
{
    return Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_HASHCODE_OFFSET));
}

inline void StubBuilder::SetElementsKindToJSHClass(GateRef glue, GateRef jsHclass, GateRef elementsKind)
{
    GateRef bitfield = Load(VariableType::INT32(), jsHclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    GateRef encodeValue = Int32LSL(elementsKind, Int32(JSHClass::ElementsKindBits::START_BIT));
    GateRef mask = Int32(((1LU << JSHClass::ElementsKindBits::SIZE) - 1) << JSHClass::ElementsKindBits::START_BIT);
    bitfield = Int32Or(Int32And(bitfield, Int32Not(mask)), encodeValue);
    Store(VariableType::INT32(), glue, jsHclass, IntPtr(JSHClass::BIT_FIELD_OFFSET), bitfield);
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
    Store(VariableType::INT32(), glue, jsHclass, IntPtr(JSHClass::BIT_FIELD_OFFSET), bitfield,
        MemoryAttribute::NoBarrier());
}

inline void StubBuilder::SetCallableToBitfield(GateRef glue, GateRef obj, bool isCallable)
{
    GateRef jsHclass = LoadHClass(obj);
    GateRef bitfield = Load(VariableType::INT32(), jsHclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    GateRef boolVal = Boolean(isCallable);
    GateRef boolToInt32 = ZExtInt1ToInt32(boolVal);
    GateRef encodeValue = Int32LSL(boolToInt32, Int32(JSHClass::CallableBit::START_BIT));
    GateRef mask = Int32(((1LU << JSHClass::CallableBit::SIZE) - 1) << JSHClass::CallableBit::START_BIT);
    bitfield = Int32Or(Int32And(bitfield, Int32Not(mask)), encodeValue);
    Store(VariableType::INT32(), glue, jsHclass, IntPtr(JSHClass::BIT_FIELD_OFFSET), bitfield,
        MemoryAttribute::NoBarrier());
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
    // only array and typedArray use this bitfield
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
    return env_->GetBuilder()->IsTypedArray(obj);
}

inline GateRef StubBuilder::IsSharedTypedArray(GateRef obj)
{
    return env_->GetBuilder()->IsSharedTypedArray(obj);
}

inline GateRef StubBuilder::GetProfileTypeInfo(GateRef jsFunc)
{
    GateRef raw = Load(VariableType::JS_POINTER(), jsFunc, IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    return Load(VariableType::JS_POINTER(), raw, IntPtr(ProfileTypeInfoCell::VALUE_OFFSET));
}

inline void StubBuilder::CheckDetectorName(GateRef glue, GateRef key, Label *fallthrough, Label *slow)
{
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env_->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    GateRef keyAddr = ChangeTaggedPointerToInt64(key);
    GateRef firstDetectorName = GetGlobalEnvValue(
        VariableType::INT64(), glueGlobalEnv, GlobalEnv::FIRST_DETECTOR_SYMBOL_INDEX);
    GateRef lastDetectorName = GetGlobalEnvValue(
        VariableType::INT64(), glueGlobalEnv, GlobalEnv::LAST_DETECTOR_SYMBOL_INDEX);
    GateRef isDetectorName = BitAnd(Int64UnsignedLessThanOrEqual(firstDetectorName, keyAddr),
                                    Int64UnsignedLessThanOrEqual(keyAddr, lastDetectorName));
    Label checkCommonDetector(env_);
    BRANCH(isDetectorName, slow, &checkCommonDetector);
    Bind(&checkCommonDetector);
    auto gFlagsStr = GetGlobalConstantValue(
        VariableType::JS_POINTER(), glue, ConstantIndex::FLAGS_INDEX);
    GateRef isFlagsStr = Equal(key, gFlagsStr);
    BRANCH(isFlagsStr, slow, fallthrough);
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

inline GateRef StubBuilder::LoadHCIndexInfosFromConstPool(GateRef jsFunc)
{
    GateRef method = Load(VariableType::JS_ANY(), jsFunc, IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef constPool = Load(VariableType::JS_ANY(), method, IntPtr(Method::CONSTANT_POOL_OFFSET));
    auto length = GetLengthOfTaggedArray(constPool);
    auto index = Int32Sub(length, Int32(ConstantPool::CONSTANT_INDEX_INFO_INDEX));
    return GetValueFromTaggedArray(constPool, index);
}

inline GateRef StubBuilder::LoadHCIndexFromConstPool(
    GateRef cachedArray, GateRef cachedLength, GateRef traceId, Label *miss)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);

    DEFVARIABLE(bcOffset, VariableType::INT32(), Int32(0));
    DEFVARIABLE(constantIndex, VariableType::INT32(),
        Int32(static_cast<int32_t>(ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX)));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label matchSuccess(env);
    Label afterUpdate(env);
    BRANCH(Int32LessThan(*i, cachedLength), &loopHead, miss);
    LoopBegin(&loopHead);
    bcOffset = GetInt32OfTInt(GetValueFromTaggedArray(cachedArray, *i));
    BRANCH(Int32Equal(*bcOffset, traceId), &matchSuccess, &afterUpdate);
    Bind(&matchSuccess);
    constantIndex = GetInt32OfTInt(GetValueFromTaggedArray(cachedArray, Int32Add(*i, Int32(1))));
    Jump(&afterLoop);
    Bind(&afterUpdate);
    i = Int32Add(*i, Int32(2)); // 2 : skip traceId and constantIndex
    BRANCH(Int32LessThan(*i, cachedLength), &loopEnd, miss);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    auto ret = *constantIndex;

    env->SubCfgExit();
    return ret;
}

inline GateRef StubBuilder::RemoveTaggedWeakTag(GateRef weak)
{
    return Int64ToTaggedPtr(IntPtrAnd(ChangeTaggedPointerToInt64(weak), IntPtr(~JSTaggedValue::TAG_WEAK)));
}

inline GateRef StubBuilder::GetAttrIndex(GateRef index)
{
    return Int32Add(Int32LSL(index, Int32(LayoutInfo::ELEMENTS_INDEX_LOG2)), Int32(LayoutInfo::ATTR_INDEX_OFFSET));
}

inline GateRef StubBuilder::GetKeyIndex(GateRef index)
{
    return Int32LSL(index, Int32(LayoutInfo::ELEMENTS_INDEX_LOG2));
}

inline GateRef StubBuilder::GetAttr(GateRef layoutInfo, GateRef index)
{
    GateRef fixedIdx = GetAttrIndex(index);
    return GetInt64OfTInt(GetValueFromTaggedArray(layoutInfo, fixedIdx));
}

inline GateRef StubBuilder::GetKey(GateRef layoutInfo, GateRef index)
{
    GateRef fixedIdx = GetKeyIndex(index);
    return GetValueFromTaggedArray(layoutInfo, fixedIdx);
}

inline GateRef StubBuilder::IsMarkerCellValid(GateRef cell)
{
    return env_->GetBuilder()->IsMarkerCellValid(cell);
}

inline GateRef StubBuilder::GetAccessorHasChanged(GateRef obj)
{
    return env_->GetBuilder()->GetAccessorHasChanged(obj);
}

inline GateRef StubBuilder::ComputeTaggedTypedArraySize(GateRef elementSize, GateRef length)
{
    return PtrAdd(IntPtr(ByteArray::DATA_OFFSET), PtrMul(elementSize, length));
}

inline GateRef StubBuilder::GetAccGetter(GateRef accesstor)
{
    return Load(VariableType::JS_ANY(), accesstor, IntPtr(AccessorData::GETTER_OFFSET));
}

inline GateRef StubBuilder::GetAccSetter(GateRef accesstor)
{
    return Load(VariableType::JS_ANY(), accesstor, IntPtr(AccessorData::SETTER_OFFSET));
}

inline GateRef StubBuilder::GetViewedArrayBuffer(GateRef dataView)
{
    return Load(VariableType::JS_ANY(), dataView,
                IntPtr(JSDataView::VIEW_ARRAY_BUFFER_OFFSET));
}

inline GateRef StubBuilder::GetByteOffset(GateRef dataView)
{
    return Load(VariableType::INT32(), dataView,
                IntPtr(JSDataView::BYTE_OFFSET_OFFSET));
}

inline GateRef StubBuilder::GetByteLength(GateRef dataView)
{
    return Load(VariableType::INT32(), dataView,
                IntPtr(JSDataView::BYTE_LENGTH_OFFSET));
}

inline GateRef StubBuilder::GetArrayBufferData(GateRef buffer)
{
    GateRef offset = IntPtr(JSArrayBuffer::DATA_OFFSET);
    return Load(VariableType::JS_ANY(), buffer, offset);
}

inline GateRef StubBuilder::GetArrayBufferByteLength(GateRef buffer)
{
    GateRef offset = IntPtr(JSArrayBuffer::BYTE_LENGTH_OFFSET);
    return Load(VariableType::INT32(), buffer, offset);
}

inline void StubBuilder::SetArrayBufferByteLength(GateRef glue, GateRef buffer, GateRef length)
{
    GateRef offset = IntPtr(JSArrayBuffer::BYTE_LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, buffer, offset, length);
}

inline GateRef StubBuilder::GetCurrentEcmaContext(GateRef glue)
{
    GateRef currentContextOffset = IntPtr(JSThread::GlueData::GetCurrentContextOffset(env_->Is32Bit()));
    return Load(VariableType::NATIVE_POINTER(), glue, currentContextOffset);
}

inline GateRef StubBuilder::GetPropertiesCache(GateRef glue)
{
    GateRef propertiesCacheOffset = IntPtr(JSThread::GlueData::GetPropertiesCacheOffset(env_->Is32Bit()));
    return Load(VariableType::NATIVE_POINTER(), glue, propertiesCacheOffset);
}

inline GateRef StubBuilder::GetMegaICCache(GateRef glue, MegaICCache::MegaICKind kind)
{
    GateRef megaICCache_Offset;
    if (kind == MegaICCache::Load) {
        megaICCache_Offset = IntPtr(JSThread::GlueData::GetLoadMegaICCacheOffset(env_->Is32Bit()));
    } else {
        megaICCache_Offset = IntPtr(JSThread::GlueData::GetStoreMegaICCacheOffset(env_->Is32Bit()));
    }
    return Load(VariableType::NATIVE_POINTER(), glue, megaICCache_Offset);
}

inline GateRef StubBuilder::GetModuleLogger(GateRef glue)
{
    GateRef moduleLoggerOffset = IntPtr(JSThread::GlueData::GetModuleLoggerOffset(env_->Is32Bit()));
    return Load(VariableType::NATIVE_POINTER(), glue, moduleLoggerOffset);
}

inline GateRef StubBuilder::GetStageOfHotReload(GateRef glue)
{
    GateRef stageOfColdReloadOffset = IntPtr(JSThread::GlueData::GetStageOfHotReloadOffset(env_->Is32Bit()));
    return Load(VariableType::INT32(), glue, stageOfColdReloadOffset);
}

inline GateRef StubBuilder::GetModuleManager(GateRef glue)
{
    GateRef moduleManagerOffset = IntPtr(JSThread::GlueData::GetModuleManagerOffset(env_->Is32Bit()));
    return Load(VariableType::NATIVE_POINTER(), glue, moduleManagerOffset);
}

inline void StubBuilder::IncMegaProbeCount([[maybe_unused]]GateRef glue)
{
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
    GateRef megaProbeCountOffset = IntPtr(JSThread::GlueData::GetMegaProbesCountOffset(env_->Is32Bit()));
    GateRef before = Load(VariableType::INT64(), glue, megaProbeCountOffset);
    Store(VariableType::INT64(), glue, glue, megaProbeCountOffset, Int64Add(before, Int64(1)));
#endif
}
inline void StubBuilder::IncMegaHitCount([[maybe_unused]]GateRef glue)
{
#if ECMASCRIPT_ENABLE_MEGA_PROFILER
    GateRef megaHitCountOffset = IntPtr(JSThread::GlueData::GetMegaHitCountOffset(env_->Is32Bit()));
    GateRef before = Load(VariableType::INT64(), glue, megaHitCountOffset);
    Store(VariableType::INT64(), glue, glue, megaHitCountOffset, Int64Add(before, Int64(1)));
#endif
}

inline GateRef StubBuilder::GetSortedKey(GateRef layoutInfo, GateRef index)
{
    GateRef fixedIdx = GetSortedIndex(layoutInfo, index);
    return GetKey(layoutInfo, fixedIdx);
}

inline GateRef StubBuilder::GetSortedIndex(GateRef layoutInfo, GateRef index)
{
    return GetSortedIndex(GetAttr(layoutInfo, index));
}

inline void StubBuilder::SetToPropertiesCache(GateRef glue, GateRef cache, GateRef cls, GateRef key, GateRef result,
                                              GateRef hir)
{
    GateRef hash = HashFromHclassAndKey(glue, cls, key, hir);
    GateRef prop =
        PtrAdd(cache, PtrMul(ZExtInt32ToPtr(hash), IntPtr(PropertiesCache::PropertyKey::GetPropertyKeySize())));
    StoreWithoutBarrier(VariableType::JS_POINTER(), prop, IntPtr(PropertiesCache::PropertyKey::GetHclassOffset()), cls);
    StoreWithoutBarrier(VariableType::JS_ANY(), prop, IntPtr(PropertiesCache::PropertyKey::GetKeyOffset()), key);
    StoreWithoutBarrier(VariableType::INT32(), prop, IntPtr(PropertiesCache::PropertyKey::GetResultsOffset()), result);
}

inline void StubBuilder::StoreWithoutBarrier(VariableType type, GateRef base, GateRef offset, GateRef value)
{
    GateRef addr = PtrAdd(base, offset);
    env_->GetBuilder()->StoreWithoutBarrier(type, addr, value);
}

inline GateRef StubBuilder::HashFromHclassAndKey(GateRef glue, GateRef cls, GateRef key, GateRef hir)
{
    GateRef clsHash = Int32LSR(ChangeIntPtrToInt32(TaggedCastToIntPtr(cls)), Int32(3));  // skip 8bytes
    GateRef keyHash = GetKeyHashCode(glue, key, hir);
    return Int32And(Int32Xor(clsHash, keyHash), Int32(PropertiesCache::CACHE_LENGTH_MASK));
}
inline GateRef StubBuilder::GetLastLeaveFrame(GateRef glue)
{
    bool isArch32 = GetEnvironment()->Is32Bit();
    GateRef spOffset = IntPtr(JSThread::GlueData::GetLeaveFrameOffset(isArch32));
    return Load(VariableType::NATIVE_POINTER(), glue, spOffset);
}

// HashFromHclassAndStringKey only supports String and does not support Symbol. Additionally, it is necessary to ensure
// that the hash value of the String exists.
inline GateRef StubBuilder::HashFromHclassAndStringKey([[maybe_unused]] GateRef glue, GateRef cls, GateRef key)
{
    GateRef clsHash =
        Int32LSR(ChangeIntPtrToInt32(TaggedCastToIntPtr(cls)), Int32(MegaICCache::HCLASS_SHIFT)); // skip 8bytes
    GateRef keyHash = Load(VariableType::INT32(), key, IntPtr(EcmaString::MIX_HASHCODE_OFFSET));
    GateRef temp = Int32Xor(Int32Xor(Int32Mul(clsHash, Int32(31)), Int32Mul(keyHash, Int32(0x9e3779b9))),
                            Int32LSR(keyHash, Int32(16)));
    return Int32And(temp, Int32(MegaICCache::CACHE_LENGTH_MASK));
}

inline GateRef StubBuilder::OrdinaryNewJSObjectCreate(GateRef glue, GateRef proto)
{
    return env_->GetBuilder()->OrdinaryNewJSObjectCreate(glue, proto);
}

inline GateRef StubBuilder::NewJSPrimitiveRef(GateRef glue, size_t index, GateRef obj)
{
    return env_->GetBuilder()->NewJSPrimitiveRef(glue, index, obj);
}

inline GateRef StubBuilder::ToObject(GateRef glue, GateRef obj)
{
    return env_->GetBuilder()->ToObject(glue, obj);
}

inline GateRef StubBuilder::GetPrototype(GateRef glue, GateRef object)
{
    return env_->GetBuilder()->GetPrototype(glue, object);
}

inline GateRef StubBuilder::GetLengthOfJSArray(GateRef array)
{
    return env_->GetBuilder()->GetLengthOfJSArray(array);
}

inline void StubBuilder::ResolvedModuleMustBeSourceTextModule([[maybe_unused]] GateRef resolvedModule)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(CurrentModuleNotSourceTextModule), IsSourceTextModule(resolvedModule));
}

inline void StubBuilder::ModuleEnvMustBeDefined([[maybe_unused]] GateRef curEnv)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(ModuleEnvMustBeDefined), BoolNot(TaggedIsUndefined(curEnv)));
}

inline void StubBuilder::CheckIsResolvedIndexBinding([[maybe_unused]] GateRef resolution)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(CheckIsResolvedIndexBinding), IsResolvedIndexBinding(resolution));
}

inline void StubBuilder::RecordNameMustBeString([[maybe_unused]] GateRef recordName)
{
    ASM_ASSERT(GET_MESSAGE_STRING_ID(RecordNameMustBeString), TaggedIsString(recordName));
}

inline GateRef StubBuilder::GetNameDictionary(GateRef module)
{
    GateRef nameDictionaryOffset = IntPtr(SourceTextModule::NAME_DICTIONARY_OFFSET);
    return Load(VariableType::JS_ANY(), module, nameDictionaryOffset);
}

inline GateRef StubBuilder::GetCurrentModuleEnv(GateRef curModule)
{
    GateRef sourceTextModuleOffset = IntPtr(SourceTextModule::SOURCE_TEXT_MODULE_OFFSET);
    return Load(VariableType::JS_ANY(), curModule, sourceTextModuleOffset);
}

inline GateRef StubBuilder::GetBitFieldFromSourceTextModule(GateRef curModule)
{
    GateRef bitFieldOffset = IntPtr(SourceTextModule::BIT_FIELD_OFFSET);
    return Load(VariableType::INT32(), curModule, bitFieldOffset);
}

inline GateRef StubBuilder::GetResolveModuleFromResolvedIndexBinding(GateRef resolvedBinding)
{
    GateRef moduleOffset = IntPtr(ResolvedIndexBinding::MODULE_OFFSET);
    return Load(VariableType::JS_ANY(), resolvedBinding, moduleOffset);
}

inline GateRef StubBuilder::GetResolveModuleFromResolvedBinding(GateRef resolvedBinding)
{
    GateRef moduleOffset = IntPtr(ResolvedBinding::MODULE_OFFSET);
    return Load(VariableType::JS_ANY(), resolvedBinding, moduleOffset);
}

inline GateRef StubBuilder::GetIdxOfResolvedIndexBinding(GateRef resolvedBinding)
{
    GateRef indexOffset = IntPtr(ResolvedIndexBinding::INDEX_OFFSET);
    return Load(VariableType::INT32(), resolvedBinding, indexOffset);
}

inline GateRef StubBuilder::GetIdxOfResolvedRecordIndexBinding(GateRef resolvedBinding)
{
    GateRef indexOffset = IntPtr(ResolvedRecordIndexBinding::INDEX_OFFSET);
    return Load(VariableType::INT32(), resolvedBinding, indexOffset);
}

inline GateRef StubBuilder::GetModuleRecord(GateRef resolvedBinding)
{
    GateRef moduleRecordIdxOffset = IntPtr(ResolvedRecordIndexBinding::MODULE_RECORD_INDEX_OFFSET);
    return Load(VariableType::JS_ANY(), resolvedBinding, moduleRecordIdxOffset);
}

inline GateRef StubBuilder::GetBindingName(GateRef resolvedBinding)
{
    GateRef bindingNameOffset = IntPtr(ResolvedRecordBinding::BINDING_NAME_OFFSET);
    return Load(VariableType::JS_ANY(), resolvedBinding, bindingNameOffset);
}

inline GateRef StubBuilder::IsResolvedIndexBinding(GateRef resolvedBinding)
{
    GateRef objectType = GetObjectType(LoadHClass(resolvedBinding));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::RESOLVEDINDEXBINDING_RECORD)));
}

inline GateRef StubBuilder::IsResolvedBinding(GateRef resolvedBinding)
{
    GateRef objectType = GetObjectType(LoadHClass(resolvedBinding));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::RESOLVEDBINDING_RECORD)));
}

inline GateRef StubBuilder::IsResolvedRecordIndexBinding(GateRef resolvedBinding)
{
    GateRef objectType = GetObjectType(LoadHClass(resolvedBinding));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::RESOLVEDRECORDINDEXBINDING_RECORD)));
}

inline GateRef StubBuilder::IsResolvedRecordBinding(GateRef resolvedBinding)
{
    GateRef objectType = GetObjectType(LoadHClass(resolvedBinding));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::RESOLVEDRECORDBINDING_RECORD)));
}

inline GateRef StubBuilder::IsLdEndExecPatchMain(GateRef glue)
{
    return Int32Equal(GetStageOfHotReload(glue),
                      Int32(static_cast<int>(StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN)));
}

inline GateRef StubBuilder::GetModuleType(GateRef module)
{
    GateRef bitfield = GetBitFieldFromSourceTextModule(module);
    return Int32And(Int32LSR(bitfield, Int32(SourceTextModule::STATUS_BITS)),
                    Int32((1LU << SourceTextModule::MODULE_TYPE_BITS) - 1));
}

inline GateRef StubBuilder::IsNativeModule(GateRef module)
{
    GateRef moduleType = GetModuleType(module);
    return LogicOrBuilder(env_)
        .Or(Int32Equal(moduleType, Int32(static_cast<int>(ModuleTypes::OHOS_MODULE))))
        .Or(Int32Equal(moduleType, Int32(static_cast<int>(ModuleTypes::APP_MODULE))))
        .Or(Int32Equal(moduleType, Int32(static_cast<int>(ModuleTypes::NATIVE_MODULE))))
        .Or(Int32Equal(moduleType, Int32(static_cast<int>(ModuleTypes::INTERNAL_MODULE))))
        .Done();
}

inline GateRef StubBuilder::IsCjsModule(GateRef module)
{
    GateRef moduleType = GetModuleType(module);
    return Int32Equal(moduleType, Int32(static_cast<int>(ModuleTypes::CJS_MODULE)));
}

inline GateRef StubBuilder::GetCjsModuleFunction(GateRef glue)
{
    GateRef glueGlobalEnvOffset = IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(env_->Is32Bit()));
    GateRef glueGlobalEnv = Load(VariableType::NATIVE_POINTER(), glue, glueGlobalEnvOffset);
    return GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::CJS_MODULE_FUNCTION_INDEX);
}

inline GateRef StubBuilder::GetArrayElementsGuardians(GateRef env)
{
    GateRef offset = IntPtr(GlobalEnv::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), env, offset);
    return TruncInt32ToInt1(Int32And(Int32LSR(bitfield,
        Int32(GlobalEnv::ArrayPrototypeChangedGuardiansBits::START_BIT)),
        Int32((1LU << GlobalEnv::ArrayPrototypeChangedGuardiansBits::SIZE) - 1)));
}

inline void StubBuilder::SetArrayElementsGuardians(GateRef glue, GateRef env, GateRef value)
{
    GateRef oldValue = ZExtInt1ToInt32(value);
    GateRef offset = IntPtr(GlobalEnv::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), env, offset);
    GateRef mask = Int32LSL(
        Int32((1LU <<GlobalEnv::ArrayPrototypeChangedGuardiansBits::SIZE) - 1),
        Int32(GlobalEnv::ArrayPrototypeChangedGuardiansBits::START_BIT));
    GateRef newVal = Int32Or(Int32And(bitfield, Int32Not(mask)),
        Int32LSL(oldValue, Int32(GlobalEnv::ArrayPrototypeChangedGuardiansBits::START_BIT)));
    Store(VariableType::INT32(), glue, env, offset, newVal);
}
} //  namespace panda::ecmascript::kungfu
#endif // ECMASCRIPT_COMPILER_STUB_INL_H
