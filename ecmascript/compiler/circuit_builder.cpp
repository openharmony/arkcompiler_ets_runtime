/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/enum_cache.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/lexical_env.h"

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

GateRef CircuitBuilder::Nop()
{
    return circuit_->NewGate(circuit_->Nop(), {});
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

GateRef CircuitBuilder::IsJsCOWArray(GateRef glue, GateRef obj)
{
    // Elements of JSArray are shared and properties are not yet.
    GateRef elements = GetElementsArray(glue, obj);
    GateRef objectType = GetObjectType(LoadHClass(glue, elements));
    return IsCOWArray(objectType);
}

GateRef CircuitBuilder::IsJsCOWArray(GateRef glue, GateRef obj, [[maybe_unused]] const CompilationEnv *compilationEnv)
{
#ifdef IMPOSSIBLE
    if (compilationEnv != nullptr && compilationEnv->SupportIntrinsic()) {
        std::string comment = "is_js_cow_array";
        auto currentLabel = env_->GetCurrentLabel();
        auto currentDepend = currentLabel->GetDepend();
        GateRef elementsOffset = Int32(JSObject::ELEMENTS_OFFSET);
        GateRef bitfieldOffset = Int32(JSHClass::BIT_FIELD_OFFSET);
        GateRef cowTaggedArray = Int32(static_cast<int32_t>(JSType::COW_MUTANT_TAGGED_ARRAY));
        GateRef cowArrayLast = Int32(static_cast<int32_t>(JSType::COW_TAGGED_ARRAY));
        GateRef isJsCowArray = GetCircuit()->NewGate(circuit_->IsJsCOWArrayIntrinsic(),
            MachineType::I1, { currentDepend, glue, obj, elementsOffset, bitfieldOffset, cowTaggedArray, cowArrayLast},
            GateType::NJSValue(), comment.c_str());
        currentLabel->SetDepend(isJsCowArray);
        return isJsCowArray;
    }
#endif
    return IsJsCOWArray(glue, obj);
}

GateRef CircuitBuilder::IsCOWArray(GateRef objectType)
{
    return BitOr(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::COW_TAGGED_ARRAY))),
                 Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::COW_MUTANT_TAGGED_ARRAY))));
}

GateRef CircuitBuilder::IsTaggedArray(GateRef glue, GateRef object)
{
    GateRef objectType = GetObjectType(LoadHClass(glue, object));
    return Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::TAGGED_ARRAY)));
}

GateRef CircuitBuilder::IsMutantTaggedArray(GateRef objectType)
{
    return BitOr(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::MUTANT_TAGGED_ARRAY))),
                 Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::COW_MUTANT_TAGGED_ARRAY))));
}

GateRef CircuitBuilder::GetElementsArray(GateRef glue, GateRef object)
{
    GateRef elementsOffset = IntPtr(JSObject::ELEMENTS_OFFSET);
    return Load(VariableType::JS_POINTER(), glue, object, elementsOffset);
}

GateRef CircuitBuilder::GetLengthOfTaggedArray(GateRef array)
{
    return LoadWithoutBarrier(VariableType::INT32(), array, IntPtr(TaggedArray::LENGTH_OFFSET));
}

GateRef CircuitBuilder::GetLengthOfJSTypedArray(GateRef array)
{
    return LoadWithoutBarrier(VariableType::INT32(), array, IntPtr(JSTypedArray::ARRAY_LENGTH_OFFSET));
}

GateRef CircuitBuilder::GetDataOfTaggedArray(GateRef array)
{
    return PtrAdd(array, Int64(TaggedArray::DATA_OFFSET));
}

GateRef CircuitBuilder::GetLengthOfJSArray(GateRef array)
{
    return LoadWithoutBarrier(VariableType::INT32(), array, IntPtr(JSArray::LENGTH_OFFSET));
}

GateRef CircuitBuilder::IsTypedArray(GateRef glue, GateRef array)
{
    GateRef hclass = LoadHClass(glue, array);
    GateRef type = GetObjectType(hclass);
    return BitAnd(Int32GreaterThan(type, Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_FIRST))),
                  Int32GreaterThanOrEqual(Int32(static_cast<int32_t>(JSType::JS_TYPED_ARRAY_LAST)), type));
}

GateRef CircuitBuilder::IsSharedTypedArray(GateRef glue, GateRef array)
{
    GateRef hclass = LoadHClass(glue, array);
    GateRef type = GetObjectType(hclass);
    return BitAnd(Int32GreaterThan(type, Int32(static_cast<int32_t>(JSType::JS_SHARED_TYPED_ARRAY_FIRST))),
                  Int32GreaterThanOrEqual(Int32(static_cast<int32_t>(JSType::JS_SHARED_TYPED_ARRAY_LAST)), type));
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
                            uint32_t trueWeight, uint32_t falseWeight, const char* comment)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = Branch(currentControl, condition, trueWeight, falseWeight, comment);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = IfTrue(ifBranch);
    trueLabel->AppendPredecessor(GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = IfFalse(ifBranch);
    falseLabel->AppendPredecessor(GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_->SetCurrentLabel(nullptr);
}

template <class LabelPtrGetter>
void CircuitBuilder::SwitchGeneric(GateRef index, Label *defaultLabel, Span<const int64_t> keysValue,
                                   LabelPtrGetter getIthLabelFn)
{
    static_assert(std::is_invocable_r_v<Label*, LabelPtrGetter, size_t>, "Invalid call signature.");
    size_t numberOfKeys = keysValue.Size();
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (size_t i = 0; i < numberOfKeys; i++) {
        GateRef switchCase = SwitchCase(switchBranch, keysValue[i]);
        Label *curLabel = std::invoke(getIthLabelFn, i);
        curLabel->AppendPredecessor(currentLabel);
        curLabel->MergeControl(switchCase);
    }

    GateRef defaultCase = DefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::Switch(GateRef index, Label *defaultLabel,
                            const int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    return SwitchGeneric(index, defaultLabel, {keysValue, numberOfKeys}, [keysLabel](size_t i) {
        return &keysLabel[i];
    });
}

void CircuitBuilder::Switch(GateRef index, Label *defaultLabel,
                            const int64_t *keysValue, Label * const *keysLabel, int numberOfKeys)
{
    return SwitchGeneric(index, defaultLabel, {keysValue, numberOfKeys}, [keysLabel](size_t i) {
        return keysLabel[i];
    });
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

void CircuitBuilder::DeoptCheck(GateRef condition, GateRef frameState, DeoptType type)
{
    std::string comment = Deoptimizier::DisplayItems(type);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto currentDepend = currentLabel->GetDepend();
    ASSERT(acc_.GetOpCode(frameState) == OpCode::FRAME_STATE);
    GateRef deoptCheck = GetCircuit()->NewGate(circuit_->DeoptCheck(),
        MachineType::I1, { currentControl, currentDepend, condition,
        frameState, Int64(static_cast<int64_t>(type))}, GateType::NJSValue(), comment.c_str());
    // Add a state output to avoid schedule a phi node to deoptCheck's BB by mistake
    GateRef trueBB = circuit_->NewGate(circuit_->OrdinaryBlock(), { deoptCheck });
    auto dependRelay = DependRelay(trueBB, currentDepend);
    currentLabel->SetControl(trueBB);
    currentLabel->SetDepend(dependRelay);
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

GateRef CircuitBuilder::HeapConstant(uint32_t val)
{
    return GetCircuit()->GetHeapConstantGate(val);
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

GateRef CircuitBuilder::TaggedValueConstant(JSTaggedValue taggedValue)
{
    auto type = GateType::TaggedValue();
    return GetCircuit()->GetConstantGate(MachineType::I64, taggedValue.GetRawData(), type);
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

GateRef CircuitBuilder::LoadObjectFromConstPool(GateRef glue, GateRef constPool, GateRef index)
{
    return GetValueFromTaggedArray(glue, constPool, TruncInt64ToInt32(index));
}

GateRef CircuitBuilder::IsAccessorInternal(GateRef glue, GateRef accessor)
{
    return Int32Equal(GetObjectType(LoadHClass(glue, accessor)),
                      Int32(static_cast<int32_t>(JSType::INTERNAL_ACCESSOR)));
}

void CircuitBuilder::AppendFrameState(std::vector<GateRef> &args, GateRef hirGate)
{
    if (acc_.HasFrameState(hirGate)) {
        GateRef frameState = acc_.GetFrameState(hirGate);
        args.emplace_back(frameState);
    } else {
        args.emplace_back(IntPtr(0));
    }
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
    GateRef exception = LoadWithoutBarrier(VariableType::JS_ANY(), glue, exceptionOffset);
    return TaggedIsNotHole(exception);
}

GateRef CircuitBuilder::HasPendingException(GateRef glue, [[maybe_unused]] const CompilationEnv *compilationEnv)
{
    if (compilationEnv != nullptr && compilationEnv->SupportIntrinsic()) {
        std::string comment = "HasPendingExceptionIntrinsic";
        auto currentLabel = env_->GetCurrentLabel();
        auto currentDepend = currentLabel->GetDepend();
        GateRef exceptionOffset = IntPtr(JSThread::GlueData::GetExceptionOffset(env_->IsArch32Bit()));
        GateRef hasPendingException = GetCircuit()->NewGate(circuit_->HasPendingExceptionIntrinsic(),
            MachineType::I1, { currentDepend, glue, exceptionOffset, Int64(JSTaggedValue::VALUE_HOLE) },
            GateType::NJSValue(), comment.c_str());
        currentLabel->SetDepend(hasPendingException);
        return hasPendingException;
    }
    return HasPendingException(glue);
}

GateRef CircuitBuilder::IsUtf8String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = LoadWithoutBarrier(VariableType::INT32(), string, IntPtr(BaseString::LENGTH_AND_FLAGS_OFFSET));
    return Int32Equal(
        Int32And(len, Int32((1 << BaseString::CompressedStatusBit::SIZE) - 1)),
        Int32(BaseString::STRING_COMPRESSED));
}

GateRef CircuitBuilder::IsUtf16String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = LoadWithoutBarrier(VariableType::INT32(), string, IntPtr(BaseString::LENGTH_AND_FLAGS_OFFSET));
    return Int32Equal(
        Int32And(len, Int32((1 << BaseString::CompressedStatusBit::SIZE) - 1)),
        Int32(BaseString::STRING_UNCOMPRESSED));
}

GateRef CircuitBuilder::IsInternString(GateRef string)
{
    GateRef len = LoadWithoutBarrier(VariableType::INT32(), string, IntPtr(BaseString::LENGTH_AND_FLAGS_OFFSET));
    return Int32NotEqual(Int32And(len, Int32(1 << BaseString::IsInternBit::START_BIT)), Int32(0));
}

GateRef CircuitBuilder::GetGlobalEnv(GateRef glue)
{
    GateRef globalEnvOffset = IntPtr(JSThread::GlueData::GetCurrentEnvOffset(env_->IsArch32Bit()));
    return Load(VariableType::JS_ANY(), glue, glue, globalEnvOffset);
}

GateRef CircuitBuilder::GetGlobalObject(GateRef glue, GateRef globalEnv)
{
    return GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::JS_GLOBAL_OBJECT_INDEX);
}

GateRef CircuitBuilder::GetMethodFromFunction(GateRef glue, GateRef function)
{
    GateRef offset = IntPtr(JSFunctionBase::METHOD_OFFSET);
    return Load(VariableType::JS_POINTER(), glue, function, offset);
}

GateRef CircuitBuilder::GetModuleFromFunction(GateRef glue, GateRef function)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    return Load(VariableType::JS_POINTER(), glue, function, offset);
}

GateRef CircuitBuilder::GetSendableEnvFromModule(GateRef glue, GateRef module)
{
    return Load(VariableType::JS_POINTER(), glue, module, IntPtr(SourceTextModule::SENDABLE_ENV_OFFSET));
}

void CircuitBuilder::SetSendableEnvToModule(GateRef glue, GateRef module, GateRef value)
{
    GateRef offset = IntPtr(SourceTextModule::SENDABLE_ENV_OFFSET);
    Store(VariableType::JS_POINTER(), glue, module, offset, value);
}

GateRef CircuitBuilder::GetHomeObjectFromFunction(GateRef glue, GateRef function)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return Load(VariableType::JS_POINTER(), glue, function, offset);
}

GateRef CircuitBuilder::GetConstPoolFromFunction(GateRef glue, GateRef jsFunc)
{
    GateRef method = GetMethodFromFunction(glue, jsFunc);
    return Load(VariableType::JS_ANY(), glue, method, IntPtr(Method::CONSTANT_POOL_OFFSET));
}

GateRef CircuitBuilder::GetUnsharedConstpoolFromGlue(GateRef glue, GateRef constpool)
{
    Label entryPass(env_);
    SubCfgEntry(&entryPass);
    DEFVALUE(result, env_, VariableType::JS_ANY(), Hole());
    Label canGetUnsharedCp(env_);
    Label exit(env_);
    GateRef unshareIdx = GetUnsharedConstpoolIndex(glue, constpool);
    GateRef unsharedCpArrayLen = GetUnsharedConstpoolArrayLen(glue);
    GateRef indexLessThanUnsharedCpArrayLen = Int32LessThan(TaggedGetInt(unshareIdx), unsharedCpArrayLen);
    BRANCH(indexLessThanUnsharedCpArrayLen, &canGetUnsharedCp, &exit);
    Bind(&canGetUnsharedCp);
    {
        GateRef unshareCpOffset =
            static_cast<int32_t>(JSThread::GlueData::GetUnSharedConstpoolsOffset(env_->Is32Bit()));
        GateRef unshareCpAddr = LoadWithoutBarrier(VariableType::NATIVE_POINTER(), glue, IntPtr(unshareCpOffset));
        result = GetUnsharedConstpool(glue, unshareCpAddr, unshareIdx);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetUnsharedConstpoolArrayLen(GateRef glue)
{
    GateRef unshareCpArrayLenOffset = static_cast<int32_t>(
        JSThread::GlueData::GetUnSharedConstpoolsArrayLenOffset(env_->Is32Bit()));
    return LoadWithoutBarrier(VariableType::INT32(), glue, IntPtr(unshareCpArrayLenOffset));
}

GateRef CircuitBuilder::GetUnsharedConstpoolIndex(GateRef glue, GateRef constpool)
{
    GateRef constPoolSize = GetLengthOfTaggedArray(constpool);
    GateRef unshareIdx = Int32Sub(constPoolSize, Int32(ConstantPool::UNSHARED_CONSTPOOL_INDEX));
    return GetValueFromTaggedArray(glue, constpool, unshareIdx);
}

GateRef CircuitBuilder::GetUnsharedConstpool(GateRef glue, GateRef arrayAddr, GateRef index)
{
    GateRef dataOffset = PtrAdd(arrayAddr,
                                PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()), ZExtInt32ToPtr(TaggedGetInt(index))));
    return Load(VariableType::JS_ANY(), glue, dataOffset, IntPtr(0));
}

GateRef CircuitBuilder::GetEmptyArray(GateRef glue)
{
    GateRef gConstAddr = LoadWithoutBarrier(VariableType::JS_ANY(), glue,
        IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_->Is32Bit())));
    GateRef offset = GetGlobalConstantOffset(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    return LoadWithoutBarrier(VariableType::JS_ANY(), gConstAddr, offset);
}

GateRef CircuitBuilder::IsCompositeHClass(GateRef hClass)
{
    GateRef objectType = GetObjectType(hClass);
    return BitAnd(
        Int32LessThanOrEqual(objectType, Int32(static_cast<int32_t>(common::CommonType::LAST_OBJECT_TYPE))),
        Int32GreaterThanOrEqual(objectType, Int32(static_cast<int32_t>(common::CommonType::FIRST_OBJECT_TYPE))));
}

void CircuitBuilder::CheckHClassFieldInvalidAccess([[maybe_unused]]GateRef glue, [[maybe_unused]] GateRef hClass)
{
#ifndef NDEBUG
    Label entryPass(env_);
    SubCfgEntry(&entryPass);
    Label exit(env_);
    Label failed(env_);
    BRANCH_UNLIKELY(IsCompositeHClass(hClass), &failed, &exit);
    Bind(&failed);
    {
        CallNGCRuntime(glue, RTSTUB_ID(FatalPrint), Gate::InvalidGateRef,
                       {Int32(GET_MESSAGE_STRING_ID(AccessCompositeClassField))}, glue);
        Jump(&exit);
    }
    Bind(&exit);
    SubCfgExit();
#endif
}

void CircuitBuilder::CheckHClassAddrInvalid([[maybe_unused]]GateRef glue, [[maybe_unused]] GateRef hClass)
{
#ifndef NDEBUG
    Label entryPass(env_);
    SubCfgEntry(&entryPass);
    Label exit(env_);
    Label failed(env_);
    constexpr uint64_t highHClassMask = 0xFFFFFFFF00000000ul;
    GateRef highHClass = Int64And(TaggedPointerToInt64(hClass), Int64(highHClassMask));
    GateRef expectHighHClass = TaggedPointerToInt64(LoadWithoutBarrier(VariableType::JS_POINTER(), glue,
        IntPtr(JSThread::GlueData::GetBaseAddressOffset(env_->Is32Bit()))));
    BRANCH_UNLIKELY(Int64NotEqual(expectHighHClass, highHClass), &failed, &exit);
    Bind(&failed);
    {
        CallNGCRuntime(glue, RTSTUB_ID(FatalPrint), Gate::InvalidGateRef,
                       {Int32(GET_MESSAGE_STRING_ID(HClassAddressIsInvalid))}, glue);
        Jump(&exit);
    }
    Bind(&exit);
    SubCfgExit();
#endif
}

GateRef CircuitBuilder::GetPrototypeFromHClass(GateRef glue, GateRef hClass)
{
    CheckHClassFieldInvalidAccess(glue, hClass);
    GateRef protoOffset = IntPtr(JSHClass::PROTOTYPE_OFFSET);
    return Load(VariableType::JS_ANY(), glue, hClass, protoOffset);
}

GateRef CircuitBuilder::GetEnumCacheFromHClass(GateRef glue, GateRef hClass)
{
    CheckHClassFieldInvalidAccess(glue, hClass);
    GateRef offset = IntPtr(JSHClass::ENUM_CACHE_OFFSET);
    return Load(VariableType::JS_ANY(), glue, hClass, offset);
}

GateRef CircuitBuilder::GetProtoChangeMarkerFromHClass(GateRef glue, GateRef hClass)
{
    CheckHClassFieldInvalidAccess(glue, hClass);
    GateRef offset = IntPtr(JSHClass::PROTO_CHANGE_MARKER_OFFSET);
    return Load(VariableType::JS_ANY(), glue, hClass, offset);
}

GateRef CircuitBuilder::GetCacheKindFromForInIterator(GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::CACHE_KIND_OFFSET);
    return LoadWithoutBarrier(VariableType::INT32(), iter, offset);
}

GateRef CircuitBuilder::GetLengthFromForInIterator(GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::LENGTH_OFFSET);
    return LoadWithoutBarrier(VariableType::INT32(), iter, offset);
}

GateRef CircuitBuilder::GetIndexFromForInIterator(GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::INDEX_OFFSET);
    return LoadWithoutBarrier(VariableType::INT32(), iter, offset);
}

GateRef CircuitBuilder::GetKeysFromForInIterator(GateRef glue, GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::KEYS_OFFSET);
    return Load(VariableType::JS_ANY(), glue, iter, offset);
}

GateRef CircuitBuilder::GetObjectFromForInIterator(GateRef glue, GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::OBJECT_OFFSET);
    return Load(VariableType::JS_ANY(), glue, iter, offset);
}

GateRef CircuitBuilder::GetCachedHClassFromForInIterator(GateRef glue, GateRef iter)
{
    GateRef offset = IntPtr(JSForInIterator::CACHED_HCLASS_OFFSET);
    return Load(VariableType::JS_ANY(), glue, iter, offset);
}

GateRef CircuitBuilder::GetArrayIterationKind(GateRef iter)
{
    static_assert(JSArrayIterator::SIZE - JSArrayIterator::BIT_FIELD_OFFSET <= sizeof(uint32_t));
    GateRef bitfield = LoadWithoutBarrier(VariableType::INT32(), iter, IntPtr(JSArrayIterator::BIT_FIELD_OFFSET));
    GateRef mask = Int32((1LLU << JSArrayIterator::ITERATION_KIND_BITS) - 1);
    return Int32And(bitfield, mask);
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

void CircuitBuilder::SetCacheKindForInIterator(GateRef glue, GateRef iter, GateRef cacheKind)
{
    GateRef offset = IntPtr(JSForInIterator::CACHE_KIND_OFFSET);
    Store(VariableType::INT32(), glue, iter, offset, cacheKind);
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

void CircuitBuilder::SetCachedHClassOfForInIterator(GateRef glue, GateRef iter, GateRef hclass)
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

void CircuitBuilder::IncreaseArrayIteratorIndex(GateRef glue, GateRef iter, GateRef index)
{
    static_assert(JSArrayIterator::BIT_FIELD_OFFSET - JSArrayIterator::NEXT_INDEX_OFFSET <= sizeof(uint32_t));
    GateRef newIndex = Int32Add(index, Int32(1));
    GateRef offset = IntPtr(JSArrayIterator::NEXT_INDEX_OFFSET);
    Store(VariableType::INT32(), glue, iter, offset, newIndex);
}

void CircuitBuilder::IncreaseIteratorIndex(GateRef glue, GateRef iter, GateRef index)
{
    GateRef newIndex = Int32Add(index, Int32(1));
    GateRef offset = IntPtr(JSForInIterator::INDEX_OFFSET);
    Store(VariableType::INT32(), glue, iter, offset, newIndex);
}

GateRef CircuitBuilder::GetHasChanged(GateRef object)
{
    GateRef bitfieldOffset = IntPtr(ProtoChangeMarker::BIT_FIELD_OFFSET);
    GateRef bitfield = LoadWithoutBarrier(VariableType::INT32(), object, bitfieldOffset);
    GateRef mask = Int32(1LLU << (ProtoChangeMarker::HAS_CHANGED_BITS - 1));
    return Int32NotEqual(Int32And(bitfield, mask), Int32(0));
}

GateRef CircuitBuilder::GetNotFoundHasChanged(GateRef object)
{
    GateRef bitfieldOffset = IntPtr(ProtoChangeMarker::BIT_FIELD_OFFSET);
    GateRef bitfield = LoadWithoutBarrier(VariableType::INT32(), object, bitfieldOffset);
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(ProtoChangeMarker::NotFoundHasChangedBits::START_BIT)),
                 Int32((1LLU << ProtoChangeMarker::NotFoundHasChangedBits::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::GetAccessorHasChanged(GateRef object)
{
    GateRef bitfieldOffset = IntPtr(ProtoChangeMarker::BIT_FIELD_OFFSET);
    GateRef bitfield = LoadWithoutBarrier(VariableType::INT32(), object, bitfieldOffset);
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(ProtoChangeMarker::AccessorHasChangedBits::START_BIT)),
                 Int32((1LLU << ProtoChangeMarker::AccessorHasChangedBits::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::HasDeleteProperty(GateRef hClass)
{
    GateRef bitfield = LoadWithoutBarrier(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::HasDeletePropertyBit::START_BIT)),
                 Int32((1LLU << JSHClass::HasDeletePropertyBit::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsOnHeap(GateRef hClass)
{
    GateRef bitfield = LoadWithoutBarrier(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
    return Int32NotEqual(
        Int32And(Int32LSR(bitfield, Int32(JSHClass::IsOnHeap::START_BIT)),
                 Int32((1LU << JSHClass::IsOnHeap::SIZE) - 1)),
        Int32(0));
}

GateRef CircuitBuilder::IsEcmaObject(GateRef glue, GateRef obj)
{
    return LogicAndBuilder(env_).And(TaggedIsHeapObject(obj)).And(TaggedObjectIsEcmaObject(glue, obj)).Done();
}

GateRef CircuitBuilder::CheckJSType(GateRef glue, GateRef object, JSType jsType)
{
    Label entryPass(env_);
    SubCfgEntry(&entryPass);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    Label heapObj(env_);
    Label exit(env_);
    GateRef isHeapObject = TaggedIsHeapObject(object);
    BRANCH(isHeapObject, &heapObj, &exit);
    Bind(&heapObj);
    {
        GateRef objectType = GetObjectType(LoadHClass(glue, object));
        result = Int32Equal(objectType, Int32(static_cast<int32_t>(jsType)));
                Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetObjectByIndexFromConstPool(GateRef glue, GateRef hirGate, GateRef frameState, GateRef index,
                                                      ConstPoolType type)
{
    ArgumentAccessor *argAcc = circuit_->GetArgumentAccessor();
    GateRef jsFunc = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
    GateRef module = GetModuleFromFunction(glue, jsFunc);
    GateRef sharedConstpool = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::SHARED_CONST_POOL);
    GateRef unsharedConstPool = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::UNSHARED_CONST_POOL);
    GateRef obj = GetObjectFromConstPool(glue, hirGate, sharedConstpool, unsharedConstPool, module, index, type);
    return obj;
}

GateRef CircuitBuilder::GetObjectFromConstPool(GateRef glue, GateRef hirGate, GateRef sharedConstPool,
                                               GateRef unsharedConstPool, GateRef module, GateRef index,
                                               ConstPoolType type)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    Label cacheMiss(env_);
    Label cache(env_);
    Label unshareCpHit(env_);
    Label unshareCpMiss(env_);

    // HirGate Can not be a nullGate in Aot
    if (GetCircuit()->IsOptimizedOrFastJit() && hirGate == Circuit::NullGate()) {
        hirGate = index;
    }
    // Call runtime to create unshared constpool when current context's cache is hole in multi-thread.
    DEFVALUE(cacheValue, env_, VariableType::JS_ANY(), Hole());
    if (type == ConstPoolType::ARRAY_LITERAL || type == ConstPoolType::OBJECT_LITERAL) {
        BRANCH(TaggedIsNotHole(unsharedConstPool), &unshareCpHit, &unshareCpMiss);
        Bind(&unshareCpHit);
        {
            cacheValue = GetValueFromTaggedArray(glue, unsharedConstPool, index);
            Jump(&unshareCpMiss);
        }
    } else {
        cacheValue = GetValueFromTaggedArray(glue, sharedConstPool, index);
        Jump(&unshareCpMiss);
    }
    Bind(&unshareCpMiss);
    DEFVALUE(result, env_, VariableType::JS_ANY(), *cacheValue);
    BRANCH(BitOr(TaggedIsHole(*result), TaggedIsNullPtr(*result)), &cacheMiss, &cache);
    Bind(&cacheMiss);
    {
        if (type == ConstPoolType::STRING) {
            result = CallRuntime(glue, RTSTUB_ID(GetStringFromCache), Gate::InvalidGateRef,
                { sharedConstPool, Int32ToTaggedInt(index) }, hirGate);
        } else if (type == ConstPoolType::ARRAY_LITERAL) {
            result = CallRuntime(glue, RTSTUB_ID(GetArrayLiteralFromCache), Gate::InvalidGateRef,
                { sharedConstPool, Int32ToTaggedInt(index), module }, hirGate);
        } else if (type == ConstPoolType::OBJECT_LITERAL) {
            result = CallRuntime(glue, RTSTUB_ID(GetObjectLiteralFromCache), Gate::InvalidGateRef,
                { sharedConstPool, Int32ToTaggedInt(index), module }, hirGate);
        } else {
            result = CallRuntime(glue, RTSTUB_ID(GetMethodFromCache), Gate::InvalidGateRef,
                { sharedConstPool, Int32ToTaggedInt(index) }, hirGate);
        }
        Jump(&exit);
    }
    Bind(&cache);
    {
        if (type == ConstPoolType::METHOD) {
            Label isHeapObj(env_);
            Label checkInteger(env_);
            BRANCH(TaggedIsHeapObject(*result), &isHeapObj, &checkInteger);
            Bind(&isHeapObj);
            {
                Label isAOTLiteralInfo(env_);
                BRANCH(IsAOTLiteralInfo(glue, *result), &isAOTLiteralInfo, &exit);
                Bind(&isAOTLiteralInfo);
                {
                    result = CallRuntime(glue, RTSTUB_ID(GetMethodFromCache), Gate::InvalidGateRef,
                        { sharedConstPool, Int32ToTaggedInt(index) }, hirGate);
                    Jump(&exit);
                }
            }
            Bind(&checkInteger);
            {
                Label isInteger(env_);
                BRANCH(TaggedIsInt(*result), &isInteger, &exit);
                Bind(&isInteger);
                {
                    result = CallRuntime(glue, RTSTUB_ID(GetMethodFromCache), Gate::InvalidGateRef,
                        { sharedConstPool, Int32ToTaggedInt(index) }, hirGate);
                    Jump(&exit);
                }
            }
        } else if (type == ConstPoolType::ARRAY_LITERAL) {
            Label isHeapObj(env_);
            BRANCH(TaggedIsHeapObject(*result), &isHeapObj, &exit);
            Bind(&isHeapObj);
            {
                Label isAOTLiteralInfo(env_);
                BRANCH(IsAOTLiteralInfo(glue, *result), &isAOTLiteralInfo, &exit);
                Bind(&isAOTLiteralInfo);
                {
                    result = CallRuntime(glue, RTSTUB_ID(GetArrayLiteralFromCache), Gate::InvalidGateRef,
                        { sharedConstPool, Int32ToTaggedInt(index), module }, hirGate);
                    Jump(&exit);
                }
            }
        } else if (type == ConstPoolType::OBJECT_LITERAL) {
            Label isHeapObj(env_);
            BRANCH(TaggedIsHeapObject(*result), &isHeapObj, &exit);
            Bind(&isHeapObj);
            {
                Label isAOTLiteralInfo(env_);
                BRANCH(IsAOTLiteralInfo(glue, *result), &isAOTLiteralInfo, &exit);
                Bind(&isAOTLiteralInfo);
                {
                    result = CallRuntime(glue, RTSTUB_ID(GetObjectLiteralFromCache), Gate::InvalidGateRef,
                        { sharedConstPool, Int32ToTaggedInt(index), module }, hirGate);
                    Jump(&exit);
                }
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

GateRef CircuitBuilder::GetFunctionLexicalEnv(GateRef glue, GateRef function)
{
    return Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
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

void CircuitBuilder::SetModuleToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, function, offset, value);
}

GateRef CircuitBuilder::GetGlobalEnvValue(VariableType type, [[maybe_unused]] GateRef glue, GateRef env, size_t index)
{
    auto valueIndex = IntPtr(GlobalEnv::HEADER_SIZE + JSTaggedValue::TaggedTypeSize() * index);
    return LoadWithoutBarrier(type, env, valueIndex);
}

GateRef CircuitBuilder::GetCodeAddr(GateRef jsFunc)
{
    auto codeAddOffset = IntPtr(JSFunction::CODE_ENTRY_OFFSET);
    return LoadWithoutBarrier(VariableType::NATIVE_POINTER(), jsFunc, codeAddOffset);
}

GateRef CircuitBuilder::GetBaselineCodeAddr(GateRef baselineCode)
{
    auto codeAddrOffset = IntPtr(MachineCode::FUNCADDR_OFFSET);
    return LoadWithoutBarrier(VariableType::NATIVE_POINTER(), baselineCode, codeAddrOffset);
}

GateRef CircuitBuilder::GetHClassGateFromIndex(GateRef gate, int32_t index)
{
    ArgumentAccessor *argAcc = circuit_->GetArgumentAccessor();
    GateRef unsharedConstpool = argAcc->GetFrameArgsIn(gate, FrameArgIdx::UNSHARED_CONST_POOL);
    return LoadHClassFromConstpool(unsharedConstpool, index);
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

GateRef CircuitBuilder::ElementsKindIsInt(GateRef kind)
{
    return Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::INT)));
}

GateRef CircuitBuilder::ElementsKindIsIntOrHoleInt(GateRef kind)
{
    GateRef kindIsInt = Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::INT)));
    GateRef kindIsHoleInt = Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::HOLE_INT)));
    return BitOr(kindIsInt, kindIsHoleInt);
}

GateRef CircuitBuilder::ElementsKindIsNumber(GateRef kind)
{
    return Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::NUMBER)));
}

GateRef CircuitBuilder::ElementsKindIsNumOrHoleNum(GateRef kind)
{
    GateRef kindIsNum = Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::NUMBER)));
    GateRef kindIsHoleNum = Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::HOLE_NUMBER)));
    return BitOr(kindIsNum, kindIsHoleNum);
}

GateRef CircuitBuilder::ElementsKindIsString(GateRef kind)
{
    return Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::STRING)));
}

GateRef CircuitBuilder::ElementsKindIsStringOrHoleString(GateRef kind)
{
    GateRef kindIsString = Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::STRING)));
    GateRef kindIsHoleString = Int32Equal(kind, Int32(Elements::ToUint(ElementsKind::HOLE_STRING)));
    return BitOr(kindIsString, kindIsHoleString);
}

GateRef CircuitBuilder::ElementsKindIsHeapKind(GateRef kind)
{
    GateRef overString = Int32GreaterThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::STRING)));
    GateRef isHoleOrNone = Int32LessThanOrEqual(kind, Int32(Elements::ToUint(ElementsKind::HOLE)));
    return BitOr(overString, isHoleOrNone);
}

GateRef CircuitBuilder::ElementsKindHasHole(GateRef kind)
{
    return Int32NotEqual(Int32And(kind, Int32(Elements::ToUint(ElementsKind::HOLE))), Int32(0));
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

GateRef CircuitBuilder::GetKeyFromLexivalEnv(GateRef glue, GateRef lexicalEnv, GateRef levelIndex, GateRef slotIndex)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    Label loopHead(env_);
    Label loopEnd(env_);
    Label afterLoop(env_);

    DEFVALUE(result, env_, VariableType::JS_ANY(), Hole());
    DEFVALUE(currentEnv, env_, VariableType::JS_ANY(), lexicalEnv);
    DEFVALUE(i, env_, VariableType::INT32(), Int32(0));

    Branch(Int32LessThan(*i, levelIndex), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    {
        currentEnv = GetParentEnv(glue, *currentEnv);
        i = Int32Add(*i, Int32(1));
        Branch(Int32LessThan(*i, levelIndex), &loopEnd, &afterLoop);
        Bind(&loopEnd);
        LoopEnd(&loopHead);
    }
    Bind(&afterLoop);
    {
        result = GetPropertiesFromLexicalEnv(glue, *currentEnv, slotIndex);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetParentEnv(GateRef glue, GateRef object)
{
    GateRef index = Int32(LexicalEnv::PARENT_ENV_INDEX);
    return GetValueFromTaggedArray(glue, object, index);
}

GateRef CircuitBuilder::GetSendableParentEnv(GateRef glue, GateRef object)
{
    GateRef index = Int32(SendableEnv::SENDABLE_PARENT_ENV_INDEX);
    return GetValueFromTaggedArray(glue, object, index);
}

GateRef CircuitBuilder::GetPropertiesFromLexicalEnv(GateRef glue, GateRef object, GateRef index)
{
    GateRef valueIndex = Int32Add(index, Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    return GetValueFromTaggedArray(glue, object, valueIndex);
}

GateRef CircuitBuilder::NewJSPrimitiveRef(GateRef glue, GateRef globalEnv, size_t index, GateRef obj)
{
    GateRef func = GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, index);
    GateRef protoOrHclass = Load(VariableType::JS_ANY(), glue, func, IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    NewObjectStubBuilder newBuilder(env_);
    GateRef newObj  = newBuilder.NewJSObject(glue, protoOrHclass);
    GateRef valueOffset = IntPtr(JSPrimitiveRef::VALUE_OFFSET);
    Store(VariableType::JS_ANY(), glue, newObj, valueOffset, obj);
    return newObj;
}

GateRef CircuitBuilder::ToObject(GateRef glue, GateRef globalEnv, GateRef obj)
{
    Label entry(env_);
    env_->SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::JS_ANY(), obj);
    DEFVALUE(taggedId, env_, VariableType::INT32(), Int32(0));
    Label isNumber(env_);
    Label notNumber(env_);
    Label isBoolean(env_);
    Label notBoolean(env_);
    Label isString(env_);
    Label notString(env_);
    Label isECMAObject(env_);
    Label notIsECMAObject(env_);
    Label isSymbol(env_);
    Label notSymbol(env_);
    Label isUndefined(env_);
    Label notIsUndefined(env_);
    Label isNull(env_);
    Label notIsNull(env_);
    Label isHole(env_);
    Label notIsHole(env_);
    Label isBigInt(env_);
    Label notIsBigInt(env_);
    Label throwError(env_);
    BRANCH(IsEcmaObject(glue, obj), &isECMAObject, &notIsECMAObject);
    Bind(&isECMAObject);
    {
        result = obj;
        Jump(&exit);
    }
    Bind(&notIsECMAObject);
    BRANCH(TaggedIsNumber(obj), &isNumber, &notNumber);
    Bind(&isNumber);
    {
        result = NewJSPrimitiveRef(glue, globalEnv, GlobalEnv::NUMBER_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notNumber);
    BRANCH(TaggedIsBoolean(obj), &isBoolean, &notBoolean);
    Bind(&isBoolean);
    {
        result = NewJSPrimitiveRef(glue, globalEnv, GlobalEnv::BOOLEAN_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notBoolean);
    BRANCH(TaggedIsString(glue, obj), &isString, &notString);
    Bind(&isString);
    {
        result = NewJSPrimitiveRef(glue, globalEnv, GlobalEnv::STRING_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notString);
    BRANCH(TaggedIsSymbol(glue, obj), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        result = NewJSPrimitiveRef(glue, globalEnv, GlobalEnv::SYMBOL_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notSymbol);
    BRANCH(TaggedIsUndefined(obj), &isUndefined, &notIsUndefined);
    Bind(&isUndefined);
    {
        taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotUndefinedObject));
        Jump(&throwError);
    }
    Bind(&notIsUndefined);
    BRANCH(TaggedIsHole(obj), &isHole, &notIsHole);
    Bind(&isHole);
    {
        taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotHoleObject));
        Jump(&throwError);
    }
    Bind(&notIsHole);
    BRANCH(TaggedIsNull(obj), &isNull, &notIsNull);
    Bind(&isNull);
    {
        taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotNullObject));
        Jump(&throwError);
    }
    Bind(&notIsNull);
    BRANCH(TaggedIsBigInt(glue, obj), &isBigInt, &notIsBigInt);
    Bind(&isBigInt);
    {
        result = NewJSPrimitiveRef(glue, globalEnv, GlobalEnv::BIGINT_FUNCTION_INDEX, obj);
        Jump(&exit);
    }
    Bind(&notIsBigInt);
    {
        taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotConvertNotNullObject));
        Jump(&throwError);
    }
    Bind(&throwError);
    {
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), Gate::InvalidGateRef, { Int32ToTaggedInt(*taggedId) }, glue);
        result = ExceptionConstant();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetPrototype(GateRef glue, GateRef object)
{
    Label entry(env_);
    env_->SubCfgEntry(&entry);
    DEFVALUE(result, env_, VariableType::JS_ANY(), Hole());
    Label exit(env_);
    Label objectIsHeapObject(env_);
    Label objectIsEcmaObject(env_);
    Label objectNotEcmaObject(env_);

    BRANCH(TaggedIsHeapObject(object), &objectIsHeapObject, &objectNotEcmaObject);
    Bind(&objectIsHeapObject);
    BRANCH(TaggedObjectIsEcmaObject(glue, object), &objectIsEcmaObject, &objectNotEcmaObject);
    Bind(&objectNotEcmaObject);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotGetNotEcmaObject));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), Gate::InvalidGateRef, { Int32ToTaggedInt(taggedId) }, glue);
        result = ExceptionConstant();
        Jump(&exit);
    }
    Bind(&objectIsEcmaObject);
    {
        Label objectIsJsProxy(env_);
        Label objectNotIsJsProxy(env_);
        BRANCH(IsJsProxy(glue, object), &objectIsJsProxy, &objectNotIsJsProxy);
        Bind(&objectIsJsProxy);
        {
            result = CallRuntime(glue, RTSTUB_ID(CallGetPrototype), Gate::InvalidGateRef, { object }, glue);
            Jump(&exit);
        }
        Bind(&objectNotIsJsProxy);
        {
            result = GetPrototypeFromHClass(glue, LoadHClass(glue, object));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetGlobalConstantValue(VariableType type, GateRef glue, ConstantIndex index)
{
    GateRef gConstAddr = LoadWithoutBarrier(VariableType::JS_ANY(), glue,
                                            IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_->Is32Bit())));
    auto constantIndex = IntPtr(JSTaggedValue::TaggedTypeSize() * static_cast<size_t>(index));
    return LoadWithoutBarrier(type, gConstAddr, constantIndex);
}

GateRef CircuitBuilder::TransProtoWithoutLayout(GateRef glue, GateRef hClass, GateRef proto)
{
    Label entry(env_);
    env_->SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::JS_ANY(), Undefined());

    GateRef key = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
        ConstantIndex::PROTOTYPE_STRING_INDEX);
    GateRef newClass = CallNGCRuntime(glue, RTSTUB_ID(JSHClassFindProtoTransitions), Gate::InvalidGateRef,
                                      { glue, hClass, key, proto }, glue);
    Label undef(env_);
    Label find(env_);
    BRANCH(IntPtrEqual(TaggedCastToIntPtr(newClass), IntPtr(0)), &undef, &find);
    Bind(&find);
    {
        result = newClass;
        Jump(&exit);
    }
    Bind(&undef);
    {
        result = CallRuntime(glue, RTSTUB_ID(HClassCloneWithAddProto), Gate::InvalidGateRef,
                             { hClass, key, proto }, glue);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}


GateRef CircuitBuilder::OrdinaryNewJSObjectCreate(GateRef glue, GateRef proto)
{
    Label entry(env_);
    env_->SubCfgEntry(&entry);
    Label exit(env_);
    DEFVALUE(result, env_, VariableType::JS_ANY(), Undefined());

    GateRef hClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                            ConstantIndex::OBJECT_HCLASS_INDEX);
    GateRef newClass = TransProtoWithoutLayout(glue, hClass, proto);
    Label exception(env_);
    Label noexception(env_);
    BRANCH(TaggedIsException(newClass), &exception, &noexception);
    Bind(&exception);
    {
        result = ExceptionConstant();
        Jump(&exit);
    }
    Bind(&noexception);
    NewObjectStubBuilder newBuilder(env_);
    GateRef newObj = newBuilder.NewJSObject(glue, newClass);
    Label exceptionNewObj(env_);
    Label noexceptionNewObj(env_);
    BRANCH(TaggedIsException(newObj), &exceptionNewObj, &noexceptionNewObj);
    Bind(&exceptionNewObj);
    {
        result = ExceptionConstant();
        Jump(&exit);
    }
    Bind(&noexceptionNewObj);
    {
        SetExtensibleToBitfield(glue, newObj, True());
        result = newObj;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetPropertiesFromSendableEnv(GateRef glue, GateRef object, GateRef index)
{
    GateRef valueIndex = Int32Add(index, Int32(SendableEnv::SENDABLE_RESERVED_ENV_LENGTH));
    return GetValueFromTaggedArray(glue, object, valueIndex);
}

GateRef CircuitBuilder::GetProfileTypeInfo(GateRef glue, GateRef function)
{
    GateRef raw = Load(VariableType::JS_POINTER(), glue, function, IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET));
    return Load(VariableType::JS_POINTER(), glue, raw, IntPtr(ProfileTypeInfoCell::VALUE_OFFSET));
}

void CircuitBuilder::SetRawProfileTypeInfoToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

void CircuitBuilder::UpdateProfileTypeInfoCellToFunction(GateRef glue, GateRef function,
                                                         GateRef profileTypeInfo, GateRef slotId)
{
    Label subEntry(env_);
    env_->SubCfgEntry(&subEntry);

    Label profileTypeInfoNotUndefined(env_);
    Label slotValueUpdate(env_);
    Label slotValueNotUndefined(env_);
    Label profileTypeInfoEnd(env_);
    NewObjectStubBuilder newBuilder(env_);
    BRANCH(TaggedIsUndefined(profileTypeInfo), &profileTypeInfoEnd, &profileTypeInfoNotUndefined);
    Bind(&profileTypeInfoNotUndefined);
    {
        GateRef slotValue = GetValueFromTaggedArray(glue, profileTypeInfo, slotId);
        BRANCH(TaggedIsUndefined(slotValue), &slotValueUpdate, &slotValueNotUndefined);
        Bind(&slotValueUpdate);
        {
            GateRef newProfileTypeInfoCell = newBuilder.NewProfileTypeInfoCell(glue, Undefined());
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, profileTypeInfo, slotId, newProfileTypeInfoCell);
            SetRawProfileTypeInfoToFunction(glue, function, newProfileTypeInfoCell);
            Jump(&profileTypeInfoEnd);
        }
        Bind(&slotValueNotUndefined);
        UpdateProfileTypeInfoCellType(glue, slotValue);
        SetRawProfileTypeInfoToFunction(glue, function, slotValue);
        Jump(&profileTypeInfoEnd);
    }
    Bind(&profileTypeInfoEnd);

    env_->SubCfgExit();
}

void CircuitBuilder::UpdateProfileTypeInfoCellType(GateRef glue, GateRef profileTypeInfoCell)
{
    Label subEntry(env_);
    env_->SubCfgEntry(&subEntry);

    // ProfileTypeInfoCell0 -> Cell1 -> CellN
    Label isProfileTypeInfoCell0(env_);
    Label notProfileTypeInfoCell0(env_);
    Label isProfileTypeInfoCell1(env_);
    Label endProfileTypeInfoCellType(env_);
    GateRef objectType = GetObjectType(LoadHClass(glue, profileTypeInfoCell));
    BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::PROFILE_TYPE_INFO_CELL_0))),
           &isProfileTypeInfoCell0, &notProfileTypeInfoCell0);
    Bind(&isProfileTypeInfoCell0);
    {
        auto profileTypeInfoCell1Class = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                                ConstantIndex::PROFILE_TYPE_INFO_CELL_1_CLASS_INDEX);
        TransitionHClass(glue, profileTypeInfoCell, profileTypeInfoCell1Class, MemoryAttribute::NoBarrier());
        Jump(&endProfileTypeInfoCellType);
    }
    Bind(&notProfileTypeInfoCell0);
    BRANCH(Int32Equal(objectType, Int32(static_cast<int32_t>(JSType::PROFILE_TYPE_INFO_CELL_1))),
           &isProfileTypeInfoCell1, &endProfileTypeInfoCellType);
    Bind(&isProfileTypeInfoCell1);
    {
        auto profileTypeInfoCellNClass = GetGlobalConstantValue(VariableType::JS_POINTER(), glue,
                                                                ConstantIndex::PROFILE_TYPE_INFO_CELL_N_CLASS_INDEX);
        TransitionHClass(glue, profileTypeInfoCell, profileTypeInfoCellNClass, MemoryAttribute::NoBarrier());
        Jump(&endProfileTypeInfoCellType);
    }
    Bind(&endProfileTypeInfoCellType);
    env_->SubCfgExit();
}

GateRef CircuitBuilder::FastToBoolean(GateRef glue, GateRef value)
{
    Label entry(env_);
    env_->SubCfgEntry(&entry);
    DEFVALUE(result, env_, VariableType::JS_ANY(), HoleConstant());
    Label exit(env_);

    Label isSpecial(env_);
    Label notSpecial(env_);
    Label isNumber(env_);
    Label isInt(env_);
    Label isDouble(env_);
    Label notNumber(env_);
    Label notNan(env_);
    Label isString(env_);
    Label notString(env_);
    Label isBigint(env_);
    Label lengthIsOne(env_);
    Label returnTrue(env_);
    Label returnFalse(env_);

    BRANCH(TaggedIsSpecial(value), &isSpecial, &notSpecial);
    Bind(&isSpecial);
    {
        BRANCH(TaggedIsTrue(value), &returnTrue, &returnFalse);
    }
    Bind(&notSpecial);
    {
        BRANCH(TaggedIsNumber(value), &isNumber, &notNumber);
        Bind(&notNumber);
        {
            BRANCH(TaggedIsString(glue, value), &isString, &notString);
            Bind(&isString);
            {
                auto len = GetLengthFromString(value);
                BRANCH(Int32Equal(len, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&notString);
            BRANCH(TaggedIsBigInt(glue, value), &isBigint, &returnTrue);
            Bind(&isBigint);
            {
                auto len = LoadWithoutBarrier(VariableType::INT32(), value, IntPtr(BigInt::LENGTH_OFFSET));
                BRANCH(Int32Equal(len, Int32(1)), &lengthIsOne, &returnTrue);
                Bind(&lengthIsOne);
                {
                    auto data = PtrAdd(value, IntPtr(BigInt::DATA_OFFSET));
                    auto data0 = LoadWithoutBarrier(VariableType::INT32(), data, Int32(0));
                    BRANCH(Int32Equal(data0, Int32(0)), &returnFalse, &returnTrue);
                }
            }
        }
        Bind(&isNumber);
        {
            BRANCH(TaggedIsInt(value), &isInt, &isDouble);
            Bind(&isInt);
            {
                auto intValue = GetInt32OfTInt(value);
                BRANCH(Int32Equal(intValue, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&isDouble);
            {
                auto doubleValue = GetDoubleOfTDouble(value);
                BRANCH(DoubleIsNAN(doubleValue), &returnFalse, &notNan);
                Bind(&notNan);
                BRANCH(DoubleEqual(doubleValue, Double(0.0)), &returnFalse, &returnTrue);
            }
        }
    }
    Bind(&returnTrue);
    {
        result = TaggedTrue();
        Jump(&exit);
    }
    Bind(&returnFalse);
    {
        result = TaggedFalse();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::IsStableArrayLengthWriteable(GateRef glue, GateRef array)
{
    Label entry(env_);
    env_->SubCfgEntry(&entry);
    DEFVALUE(result, env_, VariableType::BOOL(), False());
    GateRef hClass = LoadHClassByConstOffset(glue, array);
    CheckHClassFieldInvalidAccess(glue, hClass);
    GateRef attrOffset = IntPtr(JSHClass::LAYOUT_OFFSET);
    GateRef layout = Load(VariableType::JS_POINTER(), glue, hClass, attrOffset);
    GateRef entryHandler = Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX);
    GateRef index =
        Int32Add(Int32LSL(entryHandler, Int32(LayoutInfo::ELEMENTS_INDEX_LOG2)), Int32(LayoutInfo::ATTR_INDEX_OFFSET));
    GateRef attr = GetInt64OfTInt(GetValueFromTaggedArray(glue, layout, index));
    GateRef writeableField =
        Int32And(TruncInt64ToInt32(Int64LSR(attr, Int64(PropertyAttributes::WritableField::START_BIT))),
                 Int32((1LLU << PropertyAttributes::WritableField::SIZE) - 1));
    result = Int32NotEqual(writeableField, Int32(0));
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetStageOfHotReload(GateRef glue)
{
    GateRef stageOfColdReloadOffset = IntPtr(JSThread::GlueData::GetStageOfHotReloadOffset(env_->Is32Bit()));
    return LoadWithoutBarrier(VariableType::INT32(), glue, stageOfColdReloadOffset);
}

GateRef CircuitBuilder::IsNotLdEndExecPatchMain(GateRef glue)
{
    return Int32NotEqual(GetStageOfHotReload(glue),
                         Int32(static_cast<int>(StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN)));
}

GateRef CircuitBuilder::FloatArrayElementConvert(GateRef value, bool isFloat32)
{
    Label entry(env_);
    Label exit(env_);
    env_->SubCfgEntry(&entry);
    DEFVALUE(result, env_, VariableType::FLOAT64(), Double(base::NAN_VALUE));
    Label ResultIsNan(env_);
    Label ResultIsNumber(env_);

    GateRef tmpResult = isFloat32 ? ExtFloat32ToDouble(value) : value;
    BRANCH_UNLIKELY(DoubleIsImpureNaN(tmpResult), &ResultIsNan, &ResultIsNumber);
    Bind(&ResultIsNan);
    {
        result = Double(base::NAN_VALUE);
        Jump(&exit);
    }
    Bind(&ResultIsNumber);
    {
        result = tmpResult;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env_->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
