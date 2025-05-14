/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/number_gate_info.h"
#include "ecmascript/compiler/number_speculative_lowering.h"
#include "ecmascript/compiler/number_speculative_runner.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ts_types/global_ts_type_ref.h"
#include "gtest/gtest.h"

namespace panda::test {
class NumberSpeculativeRetypeTests : public testing::Test {};
using ecmascript::ChunkVector;
using ecmascript::TSPrimitiveType;
using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::EcmaOpcode;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::MachineType;
using ecmascript::kungfu::NumberSpeculativeLowering;
using ecmascript::kungfu::NumberSpeculativeRetype;
using ecmascript::kungfu::NumberSpeculativeRetypeManager;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::RangeInfo;
using ecmascript::kungfu::TypeInfo;
using ecmascript::kungfu::Verifier;

HWTEST_F_L0(NumberSpeculativeRetypeTests, ConvertTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    // construct circuit
    auto value1 = builder.Int32(static_cast<uint64_t>(JSTaggedValue::VALUE_HOLE));
    auto is_undefined_or_hole_check_gate = circuit.NewGate(circuit.IsUndefinedOrHoleCheck(), MachineType::I1,
        {Circuit::NullGate(), Circuit::NullGate(), value1, Circuit::NullGate()}, GateType::NJSValue());
    auto is_not_undefined_or_hole_check_gate = circuit.NewGate(
        circuit.IsNotUndefinedOrHoleCheck(), MachineType::I1,
        {Circuit::NullGate(), Circuit::NullGate(), value1, Circuit::NullGate()}, GateType::NJSValue());
    auto value2 = builder.Int32(-1234567);
    auto ecma_object_check_gate = circuit.NewGate(circuit.EcmaObjectCheck(), MachineType::I1,
        {Circuit::NullGate(), Circuit::NullGate(), value2, Circuit::NullGate()}, GateType::NJSValue());

    ChunkVector<TypeInfo> typeInfo(&chunk);
    auto maxId = circuit.GetMaxGateId();
    typeInfo.resize(maxId + 1, TypeInfo::NONE);
    // retype phase
    NumberSpeculativeRetype retype(&circuit, &chunk, typeInfo);
    CombinedPassVisitor retypeVisitor(&circuit, false, "ConvertTest", &chunk);
    NumberSpeculativeRetypeManager retypePhase(&circuit, &retypeVisitor, &chunk,
                                            &retype,  NumberSpeculativeRetype::State::Retype);
    // convert phase
    NumberSpeculativeRetype convert(&circuit, &chunk, typeInfo);
    CombinedPassVisitor convertVisitor(&circuit, false, "ConvertTest", &chunk);
    NumberSpeculativeRetypeManager convertPhase(&circuit, &convertVisitor, &chunk,
                                            &convert, NumberSpeculativeRetype::State::Convert);

    {
        retypePhase.VisitGate(acc.GetValueIn(is_undefined_or_hole_check_gate, 0));
        convertPhase.VisitGate(is_undefined_or_hole_check_gate);
        convertPhase.VisitGate(is_not_undefined_or_hole_check_gate);
        auto input1 = acc.GetValueIn(is_undefined_or_hole_check_gate, 0);
        auto input2 = acc.GetValueIn(is_not_undefined_or_hole_check_gate, 0);
        EXPECT_EQ(acc.GetDstType(input1), ValueType::TAGGED_INT);
        EXPECT_EQ(acc.GetDstType(input2), ValueType::TAGGED_INT);
        EXPECT_EQ(acc.IsConstantValue(input1, static_cast<uint64_t>(JSTaggedValue::VALUE_HOLE)), false);
        EXPECT_EQ(acc.IsConstantValue(input2, static_cast<uint64_t>(JSTaggedValue::VALUE_HOLE)), false);
    }

    {
        retypePhase.VisitGate(acc.GetValueIn(ecma_object_check_gate, 0));
        convertPhase.VisitGate(ecma_object_check_gate);
        auto input = acc.GetValueIn(ecma_object_check_gate, 0);
        EXPECT_EQ(acc.GetMachineType(input), MachineType::I64);
    }
}

HWTEST_F_L0(NumberSpeculativeRetypeTests, ReTypeTest)
{
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    // construct circuit
    auto entry = acc.GetStateRoot();
    auto depend = acc.GetDependRoot();
    auto merge = circuit.NewGate(circuit.Merge(2), {entry, depend});
    circuit.NewGate(circuit.DependSelector(2), {merge, depend, depend});

    auto value = builder.Int32(1024);
    auto type_of_gate = circuit.NewGate(circuit.TypeOf(static_cast<uint64_t>(TSPrimitiveType::ANY)), MachineType::I64,
                                            {Circuit::NullGate(), depend}, GateType::AnyType());
    auto value_selector_gate = circuit.NewGate(circuit.ValueSelector(2), MachineType::I64,
                                            {merge, value, type_of_gate}, GateType::AnyType());

    ChunkVector<TypeInfo> typeInfo(&chunk);
    ChunkVector<RangeInfo> rangeInfos(&chunk);
    auto maxId = circuit.GetMaxGateId();
    typeInfo.resize(maxId + 1, TypeInfo::NONE);
    rangeInfos.resize(maxId + 1, RangeInfo::NONE());
    // retype phase
    NumberSpeculativeRetype retype(&circuit, &chunk, typeInfo);
    CombinedPassVisitor retypeVisitor(&circuit, false, "ReTypeTest", &chunk);
    NumberSpeculativeRetypeManager retypePhase(&circuit, &retypeVisitor, &chunk,
                                            &retype,  NumberSpeculativeRetype::State::Retype);
    // convert phase
    NumberSpeculativeRetype convert(&circuit, &chunk, typeInfo);
    CombinedPassVisitor convertVisitor(&circuit, false, "ReTypeTest", &chunk);
    NumberSpeculativeRetypeManager convertPhase(&circuit, &convertVisitor, &chunk,
                                            &convert, NumberSpeculativeRetype::State::Convert);

    // lowering phase
    NumberSpeculativeLowering lowering(&circuit, nullptr, &chunk, typeInfo, rangeInfos);

    {
        retypePhase.VisitGate(acc.GetValueIn(value_selector_gate, 0));
        retypePhase.VisitGate(acc.GetValueIn(value_selector_gate, 1));
        retypePhase.VisitGate(value_selector_gate);
        convertPhase.VisitGate(value_selector_gate);
        lowering.Run();
        EXPECT_EQ(acc.GetMachineType(value_selector_gate), acc.GetMachineType(acc.GetValueIn(value_selector_gate, 0)));
        EXPECT_EQ(acc.GetMachineType(value_selector_gate), acc.GetMachineType(acc.GetValueIn(value_selector_gate, 1)));
    }
}

} // namespace panda::test
