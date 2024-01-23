/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/circuit_builder.h"

#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/hcr_circuit_builder.h"
#include "ecmascript/compiler/lcr_circuit_builder.h"
#include "ecmascript/compiler/mcr_circuit_builder.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/method.h"
#include "ecmascript/js_array_iterator.h"

namespace panda::ecmascript::kungfu {

GateRef CircuitBuilder::Merge(const std::vector<GateRef> &inList)
{
    return circuit_->NewGate(circuit_->Merge(inList.size()), inList);
}

GateRef CircuitBuilder::Selector(OpCode opcode, MachineType machineType, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    if (values.size() == 0) {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(Circuit::NullGate());
        }
    } else {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(values[i]);
        }
    }
    ASSERT((opcode == OpCode::VALUE_SELECTOR) || (opcode == OpCode::DEPEND_SELECTOR));
    const GateMetaData* meta = (opcode == OpCode::DEPEND_SELECTOR) ?
        circuit_->DependSelector(valueCounts) : circuit_->ValueSelector(valueCounts);
    return circuit_->NewGate(meta, machineType, inList.size(), inList.data(), type.GetGateType());
}

GateRef CircuitBuilder::Selector(OpCode opcode, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    MachineType machineType = (opcode == OpCode::DEPEND_SELECTOR) ?
        MachineType::NOVALUE : MachineType::FLEX;
    return Selector(opcode, machineType, control, values, valueCounts, type);
}

GateRef CircuitBuilder::UndefineConstant()
{
    auto type = GateType::TaggedValue();
    return circuit_->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, type);
}

GateRef CircuitBuilder::Branch(GateRef state, GateRef condition, uint32_t trueWeight, uint32_t falseWeight,
                               const char* comment)
{
    auto value = BranchAccessor::ToValue(trueWeight, falseWeight);
    return circuit_->NewGate(circuit_->IfBranch(value), { state, condition }, comment);
}

GateRef CircuitBuilder::SwitchBranch(GateRef state, GateRef index, int caseCounts)
{
    return circuit_->NewGate(circuit_->SwitchBranch(caseCounts), { state, index });
}

GateRef CircuitBuilder::Return(GateRef state, GateRef depend, GateRef value)
{
    auto returnList = circuit_->GetReturnRoot();
    return circuit_->NewGate(circuit_->Return(), { state, depend, value, returnList });
}

GateRef CircuitBuilder::ReturnVoid(GateRef state, GateRef depend)
{
    auto returnList = circuit_->GetReturnRoot();
    return circuit_->NewGate(circuit_->ReturnVoid(), { state, depend, returnList });
}

GateRef CircuitBuilder::Goto(GateRef state)
{
    return circuit_->NewGate(circuit_->OrdinaryBlock(), { state });
}

GateRef CircuitBuilder::LoopBegin(GateRef state)
{
    auto nullGate = Circuit::NullGate();
    return circuit_->NewGate(circuit_->LoopBegin(2), { state, nullGate }); // 2: entry&back
}

GateRef CircuitBuilder::LoopEnd(GateRef state)
{
    return circuit_->NewGate(circuit_->LoopBack(), { state });
}

GateRef CircuitBuilder::LoopExit(GateRef state)
{
    return circuit_->NewGate(circuit_->LoopExit(), { state });
}

GateRef CircuitBuilder::LoopExitDepend(GateRef state, GateRef depend)
{
    return circuit_->NewGate(circuit_->LoopExitDepend(), { state, depend });
}

GateRef CircuitBuilder::LoopExitValue(GateRef state, GateRef value)
{
    auto machineType = acc_.GetMachineType(value);
    auto gateType = acc_.GetGateType(value);
    return circuit_->NewGate(circuit_->LoopExitValue(), machineType, { state, value }, gateType);
}

GateRef CircuitBuilder::IfTrue(GateRef ifBranch)
{
    return circuit_->NewGate(circuit_->IfTrue(), { ifBranch });
}

GateRef CircuitBuilder::IfFalse(GateRef ifBranch)
{
    return circuit_->NewGate(circuit_->IfFalse(), { ifBranch });
}

GateRef CircuitBuilder::SwitchCase(GateRef switchBranch, int64_t value)
{
    return circuit_->NewGate(circuit_->SwitchCase(value), { switchBranch });
}

GateRef CircuitBuilder::DefaultCase(GateRef switchBranch)
{
    return circuit_->NewGate(circuit_->DefaultCase(), { switchBranch });
}

GateRef CircuitBuilder::DependRelay(GateRef state, GateRef depend)
{
    return circuit_->NewGate(circuit_->DependRelay(), { state, depend });
}

GateRef CircuitBuilder::Arguments(size_t index)
{
    auto argListOfCircuit = circuit_->GetArgRoot();
    return GetCircuit()->NewArg(MachineType::I64, index, GateType::NJSValue(), argListOfCircuit);
}

GateRef CircuitBuilder::IsJsCOWArray(GateRef obj)
{
    // Elements of JSArray are shared and properties are not yet.
    GateRef elements = GetElementsArray(obj);
    GateRef objectType = GetObjectType(LoadHClass(elements));
    return IsCOWArray(objectType);
}

GateRef CircuitBuilder::IsCOWArray(GateRef objectType)
{
    return BoolOr(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::COW_TAGGED_ARRAY))),
                  Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::COW_MUTANT_TAGGED_ARRAY))));
}

GateRef CircuitBuilder::IsTaggedArray(GateRef object)
{
    GateRef objectType = GetObjectType(LoadHClass(object));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::TAGGED_ARRAY)));
}

GateRef CircuitBuilder::IsMutantTaggedArray(GateRef objectType)
{
    return BoolOr(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::MUTANT_TAGGED_ARRAY))),
                  Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::COW_MUTANT_TAGGED_ARRAY))));
}

GateRef CircuitBuilder::GetElementsArray(GateRef object)
{
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    return Load(VariableType::JS_POINTER(), object, elementsOffset);
}

GateRef CircuitBuilder::GetLengthOfTaggedArray(GateRef array)
{
    return Load(VariableType::INT32(), array, IntPtr(TaggedArray::LENGTH_OFFSET));
}

void CircuitBuilder::Jump(Label *label)
{
    ASSERT(label);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto jump = Goto(currentControl);
    currentLabel->SetControl(jump);
    label->AppendPredecessor(currentLabel);
    label->MergeControl(currentLabel->GetControl());
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::Branch(GateRef condition, Label *trueLabel, Label *falseLabel,
                            uint32_t trueWeight, uint32_t falseWeight)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = Branch(currentControl, condition, trueWeight, falseWeight);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = IfTrue(ifBranch);
    trueLabel->AppendPredecessor(GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = IfFalse(ifBranch);
    falseLabel->AppendPredecessor(GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        GateRef switchCase = SwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    GateRef defaultCase = DefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::LoopBegin(Label *loopHead)
{
    ASSERT(loopHead);
    auto loopControl = LoopBegin(loopHead->GetControl());
    loopHead->SetControl(loopControl);
    loopHead->SetPreControl(loopControl);
    loopHead->Bind();
    env_->SetCurrentLabel(loopHead);
}

void CircuitBuilder::LoopEnd(Label *loopHead)
{
    ASSERT(loopHead);
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto loopend = LoopEnd(currentControl);
    currentLabel->SetControl(loopend);
    loopHead->AppendPredecessor(currentLabel);
    loopHead->MergeControl(loopend);
    loopHead->Seal();
    loopHead->MergeAllControl();
    loopHead->MergeAllDepend();
    env_->SetCurrentLabel(nullptr);
}

// add loop exit info at begin of label (only support not merge label)
void CircuitBuilder::LoopExit(const std::vector<Variable *> &vars, size_t diff)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto loopExit = currentLabel->GetControl();
    auto loopExitDepend = currentLabel->GetDepend();
    std::vector<GateRef> loopExitValues;
    for (size_t i = 0; i < diff; ++i) {
        loopExit = LoopExit(loopExit);
        loopExitDepend = LoopExitDepend(loopExit, loopExitDepend);
        for (const auto &var : vars) {
            auto loopExitValue = LoopExitValue(loopExit, var->ReadVariable());
            var->WriteVariable(loopExitValue);
        }
    }
    currentLabel->SetControl(loopExit);
    currentLabel->SetDepend(loopExitDepend);
}

void CircuitBuilder::ClearConstantCache(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::CONSTANT);
    auto machineType = acc_.GetMachineType(gate);
    auto value = acc_.GetConstantValue(gate);
    auto gateType = acc_.GetGateType(gate);
    GetCircuit()->ClearConstantCache(machineType, value, gateType);
}

GateRef CircuitBuilder::DeoptCheck(GateRef condition, GateRef frameState, DeoptType type)
{
    std::string comment = Deoptimizier::DisplayItems(type);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    ASSERT(acc_.GetOpCode(frameState) == OpCode::FRAME_STATE);
    GateRef ret = GetCircuit()->NewGate(circuit_->DeoptCheck(),
        MachineType::I1, { currentControl, condition,
        frameState, Int64(static_cast<int64_t>(type))}, GateType::NJSValue(), comment.c_str());
    auto dependRelay = DependRelay(ret, currentDepend);
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(dependRelay);
    return ret;
}

GateRef CircuitBuilder::GetSuperConstructor(GateRef ctor)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto ret = GetCircuit()->NewGate(circuit_->GetSuperConstructor(), MachineType::ANYVALUE,
                                     { currentControl, currentDepend, ctor }, GateType::TaggedValue());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

GateRef CircuitBuilder::Int8(int8_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I8, val, GateType::NJSValue());
}

GateRef CircuitBuilder::Int16(int16_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I16, val, GateType::NJSValue());
}

GateRef CircuitBuilder::Int32(int32_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I32, static_cast<BitField>(val), GateType::NJSValue());
}

GateRef CircuitBuilder::Int64(int64_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, val, GateType::NJSValue());
}

GateRef CircuitBuilder::IntPtr(int64_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::ARCH, val, GateType::NJSValue());
}

GateRef CircuitBuilder::StringPtr(std::string_view str)
{
    return GetCircuit()->GetConstantStringGate(MachineType::ARCH, str, GateType::NJSValue());
}

GateRef CircuitBuilder::RelocatableData(uint64_t val)
{
    return GetCircuit()->NewGate(circuit_->RelocatableData(val),
        MachineType::ARCH, GateType::TaggedValue());
}

GateRef CircuitBuilder::Boolean(bool val)
{
    return GetCircuit()->GetConstantGate(MachineType::I1, val ? 1 : 0, GateType::NJSValue());
}

GateRef CircuitBuilder::Double(double val)
{
    return GetCircuit()->GetConstantGate(MachineType::F64, base::bit_cast<int64_t>(val), GateType::NJSValue());
}

GateRef CircuitBuilder::HoleConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_HOLE, type);
}

GateRef CircuitBuilder::SpecialHoleConstant()
{
    auto type = GateType::NJSValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, base::SPECIAL_HOLE, type);
}

GateRef CircuitBuilder::NullPtrConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, 0u, type);
}

GateRef CircuitBuilder::NullConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_NULL, type);
}

GateRef CircuitBuilder::ExceptionConstant()
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_EXCEPTION, type);
}

GateRef CircuitBuilder::NanValue()
{
    return Double(std::numeric_limits<double>::quiet_NaN());
}

GateRef CircuitBuilder::LoadObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constPool = GetConstPoolFromFunction(jsFunc);
    return GetValueFromTaggedArray(constPool, TruncInt64ToInt32(index));
}

GateRef CircuitBuilder::IsAccessorInternal(GateRef accessor)
{
    return Int32Equal(GetObjectType(LoadHClass(accessor)),
                      Int32(static_cast<int32_t>(JSType::INTERNAL_ACCESSOR)));
}

void CircuitBuilder::AppendFrameArgs(std::vector<GateRef> &args, GateRef hirGate)
{
    GateRef frameArgs = acc_.GetFrameArgs(hirGate);
    if (frameArgs == Circuit::NullGate()) {
        args.emplace_back(IntPtr(0));
    } else {
        args.emplace_back(frameArgs);
    }
}

GateRef CircuitBuilder::GetConstPool(GateRef jsFunc)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetConstPool(), MachineType::I64,
                                         { currentDepend, jsFunc },
                                         GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::GetGlobalEnv()
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetGlobalEnv(), MachineType::I64,
                                         { currentDepend },
                                         GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::GetGlobalEnvObj(GateRef env, size_t index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetGlobalEnvObj(index), MachineType::I64,
                                         { currentDepend, env },
                                         GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::GetGlobalEnvObjHClass(GateRef env, size_t index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetGlobalEnvObjHClass(index), MachineType::I64,
                                         { currentDepend, env },
                                         GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::GetGlobalConstantValue(ConstantIndex index)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentDepend = currentLabel->GetDepend();
    auto newGate = GetCircuit()->NewGate(circuit_->GetGlobalConstantValue(static_cast<size_t>(index)),
                                         MachineType::I64, { currentDepend }, GateType::AnyType());
    currentLabel->SetDepend(newGate);
    return newGate;
}

GateRef CircuitBuilder::HasPendingException(GateRef glue)
{
    GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env_->IsArch32Bit()));
    GateRef exception = Load(VariableType::JS_ANY(), glue, exceptionOffset);
    return TaggedIsNotHole(exception);
}

GateRef CircuitBuilder::IsUtf8String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32Equal(
        Int32And(len, Int32(EcmaString::STRING_COMPRESSED_BIT)),
        Int32(EcmaString::STRING_COMPRESSED));
}

GateRef CircuitBuilder::IsUtf16String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32Equal(
        Int32And(len, Int32(EcmaString::STRING_COMPRESSED_BIT)),
        Int32(EcmaString::STRING_UNCOMPRESSED));
}

GateRef CircuitBuilder::GetGlobalObject(GateRef glue)
{
    GateRef offset = IntPtr(JSThread::GlueData::GetGlobalObjOffset(cmpCfg_->Is32Bit()));
    return Load(VariableType::JS_ANY(), glue, offset);
}

GateRef CircuitBuilder::GetMethodFromFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunctionBase::METHOD_OFFSET);
    return Load(VariableType::JS_POINTER(), function, offset);
}

GateRef CircuitBuilder::GetModuleFromFunction(GateRef function)
{
    GateRef method = GetMethodFromFunction(function);
    GateRef offset = IntPtr(Method::ECMA_MODULE_OFFSET);
    return Load(VariableType::JS_POINTER(), method, offset);
}

GateRef CircuitBuilder::GetHomeObjectFromFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return Load(VariableType::JS_POINTER(), function, offset);
}

GateRef CircuitBuilder::GetConstPoolFromFunction(GateRef jsFunc)
{
    GateRef method = GetMethodFromFunction(jsFunc);
    return Load(VariableType::JS_ANY(), method, IntPtr(Method::CONSTANT_POOL_OFFSET));
}

GateRef CircuitBuilder::GetObjectFromConstPool(GateRef glue, GateRef hirGate, GateRef jsFunc, GateRef index,
                                               ConstPoolType type)
{
    GateRef constPool = GetConstPoolFromFunction(jsFunc);
    GateRef module = GetModuleFromFunction(jsFunc);
    return GetObjectFromConstPool(glue, hirGate, constPool, module, index, type);
}

GateRef CircuitBuilder::GetEmptyArray(GateRef glue)
{
    GateRef gConstAddr = Load(VariableType::JS_ANY(), glue,
        IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_->Is32Bit())));
    GateRef offset = GetGlobalConstantOffset(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    return Load(VariableType::JS_ANY(), gConstAddr, offset);
}

GateRef CircuitBuilder::GetPrototypeFromHClass(GateRef hClass)
{
    GateRef protoOffset = IntPtr(JSHClass::PROTOTYPE_OFFSET);
    return Load(VariableType::JS_ANY(), hClass, protoOffset);
}

GateRef CircuitBuilder::GetEnumCacheFromHClass(GateRef hClass)
{
    GateRef offset = IntPtr(JSHClass::ENUM_CACHE_OFFSET);
    return Load(VariableType::JS_ANY(), hClass, offset);
}

GateRef CircuitBuilder::GetProtoChangeMarkerFromHClass(GateRef hClass)
{
    GateRef offset = IntPtr(JSHClass::PROTO_CHANGE_MARKER_OFFSET);
    return Load(VariableType::JS_ANY(), hClass, offset);
}

GateRef CircuitBuilder::GetLengthFromForInIterator(GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::LENGTH_OFFSET);
    return Load(VariableType::INT32(), iter, offset);
}

GateRef CircuitBuilder::GetIndexFromForInIterator(GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::INDEX_OFFSET);
    return Load(VariableType::INT32(), iter, offset);
}

GateRef CircuitBuilder::GetKeysFromForInIterator(GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::KEYS_OFFSET);
    return Load(VariableType::JS_ANY(), iter, offset);
}

GateRef CircuitBuilder::GetObjectFromForInIterator(GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::OBJECT_OFFSET);
    return Load(VariableType::JS_ANY(), iter, offset);
}

GateRef CircuitBuilder::GetCachedHclassFromForInIterator(GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::CACHED_HCLASS_OFFSET);
    return Load(VariableType::JS_ANY(), iter, offset);
}

void CircuitBuilder::SetLengthOfForInIterator(GateRef glue, GateRef iter, GateRef length)
{
    GateRef offset = IntPtr(JSForInIterator::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, iter, offset, length);
}

void CircuitBuilder::SetIndexOfForInIterator(GateRef glue, GateRef iter, GateRef index)
{
    GateRef offset = IntPtr(JSForInIterator::INDEX_OFFSET);
    Store(VariableType::INT32(), glue, iter, offset, index);
}

void CircuitBuilder::SetKeysOfForInIterator(GateRef glue, GateRef iter, GateRef keys)
{
    GateRef offset = IntPtr(JSForInIterator::KEYS_OFFSET);
    Store(VariableType::JS_ANY(), glue, iter, offset, keys);
}

void CircuitBuilder::SetObjectOfForInIterator(GateRef glue, GateRef iter, GateRef object)
{
    GateRef offset = IntPtr(JSForInIterator::OBJECT_OFFSET);
    Store(VariableType::JS_ANY(), glue, iter, offset, object);
}

void CircuitBuilder::SetCachedHclassOfForInIterator(GateRef glue, GateRef iter, GateRef hclass)
{
    GateRef offset = IntPtr(JSForInIterator::CACHED_HCLASS_OFFSET);
    Store(VariableType::JS_ANY(), glue, iter, offset, hclass);
}

void CircuitBuilder::SetNextIndexOfArrayIterator(GateRef glue, GateRef iter, GateRef nextIndex)
{
    GateRef offset = IntPtr(JSArrayIterator::NEXT_INDEX_OFFSET);
    Store(VariableType::INT32(), glue, iter, offset, nextIndex);
}

void CircuitBuilder::SetIteratedArrayOfArrayIterator(GateRef glue, GateRef iter, GateRef iteratedArray)
{
    GateRef offset = IntPtr(JSArrayIterator::ITERATED_ARRAY_OFFSET);
    Store(VariableType::JS_ANY(), glue, iter, offset, iteratedArray);
}

void CircuitBuilder::SetBitFieldOfArrayIterator(GateRef glue, GateRef iter, GateRef kind)
{
    GateRef offset = IntPtr(JSArrayIterator::BIT_FIELD_OFFSET);
    Store(VariableType::INT32(), glue, iter, offset, kind);
}

void CircuitBuilder::IncreaseInteratorIndex(GateRef glue, GateRef iter, GateRef index)
{
    GateRef newIndex = Int32Add(index, Int32(1));
    GateRef offset = IntPtr(JSForInIterator::INDEX_OFFSET);
    Store(VariableType::INT32(), glue, iter, offset, newIndex);
}

GateRef CircuitBuilder::GetHasChanged(GateRef object)
{
    GateRef bitfieldOffset = IntPtr(ProtoChangeMarker::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), object, bitfieldOffset);
    GateRef mask = Int32(1LLU << (ProtoChangeMarker::HAS_CHANGED_BITS - 1));
    return Int32NotEqual(Int32And(bitfield, mask), Int32(0));
}

GateRef CircuitBuilder::GetAccessorHasChanged(GateRef object)
{
    GateRef bitfieldOffset = IntPtr(ProtoChangeMarker::BIT_FIELD_OFFSET);
    GateRef bitfield = Load(VariableType::INT32(), object, bitfieldOffset);
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(ProtoChangeMarker::AccessorHasChangedBits::START_BIT)),
                 Int32((1LLU << ProtoChangeMarker::AccessorHasChangedBits::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::HasDeleteProperty(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::HasDeletePropertyBit::START_BIT)),
                 Int32((1LLU << JSHClass::HasDeletePropertyBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsOnHeap(GateRef hClass)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::IsOnHeap::START_BIT)),
                 Int32((1LU << JSHClass::IsOnHeap::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsEcmaObject(GateRef obj)
{
    Label entryPass(env_);
    SubCfgEntry(&entryPass);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label heapObj(env_);
    Label exit(env_);
    GateRef isHeapObject = TaggedIsHeapObject(obj);
    Branch(isHeapObject, &heapObj, &exit);
    Bind(&heapObj);
    result = LogicAnd(isHeapObject, TaggedObjectIsEcmaObject(obj));
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetObjectFromConstPool(GateRef glue, GateRef hirGate, GateRef constPool, GateRef module,
                                               GateRef index, ConstPoolType type)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    Label cacheMiss(env_);
    Label cache(env_);

    // HirGate Can not be a nullGate in Aot
    if (GetCircuit()->IsOptimizedJSFunctionFrame() && hirGate == Circuit::NullGate()) {
        hirGate = index;
    }
    auto cacheValue = GetValueFromTaggedArray(constPool, index);
    DEFVALUE(result, env_, VariableType::JS_ANY(), cacheValue);
    Branch(BoolOr(TaggedIsHole(*result), TaggedIsNullPtr(*result)), &cacheMiss, &cache);
    Bind(&cacheMiss);
    {
        if (type == ConstPoolType::STRING) {
            result = CallRuntime(glue, RTSTUB_ID(GetStringFromCache), Gate::InvalidGateRef,
                { constPool, Int32ToTaggedInt(index) }, hirGate);
        } else if (type == ConstPoolType::ARRAY_LITERAL) {
            result = CallRuntime(glue, RTSTUB_ID(GetArrayLiteralFromCache), Gate::InvalidGateRef,
                { constPool, Int32ToTaggedInt(index), module }, hirGate);
        } else if (type == ConstPoolType::OBJECT_LITERAL) {
            result = CallRuntime(glue, RTSTUB_ID(GetObjectLiteralFromCache), Gate::InvalidGateRef,
                { constPool, Int32ToTaggedInt(index), module }, hirGate);
        } else {
            result = CallRuntime(glue, RTSTUB_ID(GetMethodFromCache), Gate::InvalidGateRef,
                { constPool, Int32ToTaggedInt(index), module }, hirGate);
        }
        Jump(&exit);
    }
    Bind(&cache);
    {
        if (type == ConstPoolType::METHOD) {
            Label isAOTLiteralInfo(env_);
            Branch(IsAOTLiteralInfo(*result), &isAOTLiteralInfo, &exit);
            Bind(&isAOTLiteralInfo);
            {
                result = CallRuntime(glue, RTSTUB_ID(GetMethodFromCache), Gate::InvalidGateRef,
                    { constPool, Int32ToTaggedInt(index), module }, hirGate);
                Jump(&exit);
            }
        } else if (type == ConstPoolType::ARRAY_LITERAL) {
            Label isAOTLiteralInfo(env_);
            Branch(IsAOTLiteralInfo(*result), &isAOTLiteralInfo, &exit);
            Bind(&isAOTLiteralInfo);
            {
                result = CallRuntime(glue, RTSTUB_ID(GetArrayLiteralFromCache), Gate::InvalidGateRef,
                    { constPool, Int32ToTaggedInt(index), module }, hirGate);
                Jump(&exit);
            }
        } else if (type == ConstPoolType::OBJECT_LITERAL) {
            Label isAOTLiteralInfo(env_);
            Branch(IsAOTLiteralInfo(*result), &isAOTLiteralInfo, &exit);
            Bind(&isAOTLiteralInfo);
            {
                result = CallRuntime(glue, RTSTUB_ID(GetObjectLiteralFromCache), Gate::InvalidGateRef,
                    { constPool, Int32ToTaggedInt(index), module }, hirGate);
                Jump(&exit);
            }
        } else {
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetFunctionLexicalEnv(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
}

void CircuitBuilder::SetLengthToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::LENGTH_OFFSET);
    Store(VariableType::INT32(), glue, function, offset, value);
}

void CircuitBuilder::SetLexicalEnvToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::LEXICAL_ENV_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

void CircuitBuilder::SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

GateRef CircuitBuilder::GetGlobalEnvValue(VariableType type, GateRef env, size_t index)
{
    auto valueIndex = IntPtr(GlobalEnv::HEADER_SIZE + JSTaggedValue::TaggedTypeSize() * index);
    return Load(type, env, valueIndex);
}

GateRef CircuitBuilder::GetCodeAddr(GateRef method)
{
    auto codeAddOffset = IntPtr(Method::CODE_ENTRY_OFFSET);
    return Load(VariableType::NATIVE_POINTER(), method, codeAddOffset);
}

GateRef CircuitBuilder::GetHClassGateFromIndex(GateRef gate, int32_t index)
{
    ArgumentAccessor argAcc(circuit_);
    GateRef jsFunc = argAcc.GetFrameArgsIn(gate, FrameArgIdx::FUNC);
    GateRef constPool = GetConstPool(jsFunc);
    return LoadHClassFromConstpool(constPool, index);
}

GateRef Variable::AddPhiOperand(GateRef val)
{
    ASSERT(GateAccessor(env_->GetCircuit()).IsValueSelector(val));
    Label label = env_->GetLabelFromSelector(val);
    size_t idx = 0;
    for (auto pred : label.GetPredecessors()) {
        auto preVal = pred.ReadVariable(this);
        ASSERT(!GateAccessor(env_->GetCircuit()).IsNop(preVal));
        idx++;
        val = AddOperandToSelector(val, idx, preVal);
    }
    return TryRemoveTrivialPhi(val);
}

GateRef Variable::AddOperandToSelector(GateRef val, size_t idx, GateRef in)
{
    GateAccessor(env_->GetCircuit()).NewIn(val, idx, in);
    return val;
}

GateRef Variable::TryRemoveTrivialPhi(GateRef phi)
{
    GateAccessor acc(GetCircuit());
    GateRef same = Gate::InvalidGateRef;
    const size_t inNum = acc.GetNumIns(phi);
    for (size_t i = 1; i < inNum; ++i) {
        GateRef phiIn = acc.GetIn(phi, i);
        if (phiIn == same || phiIn == phi) {
            continue;  // unique value or self-reference
        }
        if (same != Gate::InvalidGateRef) {
            return phi;  // the phi merges at least two valusses: not trivial
        }
        same = phiIn;
    }
    if (same == Gate::InvalidGateRef) {
        // the phi is unreachable or in the start block
        GateType type = acc.GetGateType(phi);
        same = env_->GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, type);
    }
    // remove the trivial phi
    // get all users of phi except self
    std::vector<GateRef> outs;
    auto uses = acc.Uses(phi);
    for (auto use = uses.begin(); use != uses.end();) {
        GateRef u = *use;
        if (u != phi) {
            outs.push_back(u);
            use = acc.ReplaceIn(use, same);
        } else {
            use++;
        }
    }
    acc.DeleteGate(phi);

    // try to recursiveby remove all phi users, which might have vecome trivial
    for (auto out : outs) {
        if (acc.IsValueSelector(out)) {
            auto result = TryRemoveTrivialPhi(out);
            if (same == out) {
                same = result;
            }
        }
    }
    return same;
}

GateRef CircuitBuilder::LoadBuiltinObject(size_t offset)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    auto frameState = acc_.FindNearestFrameState(currentDepend);
    GateRef ret = GetCircuit()->NewGate(circuit_->LoadBuiltinObject(offset),
                                        MachineType::I64,
                                        {currentControl, currentDepend, frameState},
                                        GateType::AnyType());
    currentLabel->SetControl(ret);
    currentLabel->SetDepend(ret);
    return ret;
}

}  // namespace panda::ecmascript::kungfu
