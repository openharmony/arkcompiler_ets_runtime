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

#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/dead_code_elimination.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
class DeadCodeEliminationTests : public testing::Test {
};
using ecmascript::kungfu::Chunk;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::DeadCodeElimination;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::MachineType;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::GateRef;
using ecmascript::kungfu::Verifier;
using ecmascript::NativeAreaAllocator;

HWTEST_F_L0(DeadCodeEliminationTests, CommonEliminationFramework)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder builder(&circuit);
    GateAccessor acc(&circuit);
    auto depend = acc.GetDependRoot();
    auto arg0 = builder.Arguments(0);
    auto dead = circuit.DeadGate();
    auto ifBranch = circuit.NewGate(circuit.IfBranch(0), {dead, arg0});
    auto ifTrue = circuit.NewGate(circuit.IfTrue(), {ifBranch});
    auto ifFalse = circuit.NewGate(circuit.IfFalse(), {ifBranch});
    auto trueReturn = circuit.NewGate(circuit.Return(), {ifTrue, depend, arg0, circuit.GetReturnRoot()});
    auto falseReturn = circuit.NewGate(circuit.Return(), {ifFalse, depend, arg0, circuit.GetReturnRoot()});
    Chunk chunk(&allocator);
    CombinedPassVisitor visitor(&circuit, false, "dead code elimination", &chunk);
    DeadCodeElimination elimination(&circuit, &visitor, &chunk);
    visitor.AddPass(&elimination);
    visitor.VisitGraph();
    EXPECT_TRUE(acc.IsNop(ifBranch));
    EXPECT_TRUE(acc.IsNop(ifTrue));
    EXPECT_TRUE(acc.IsNop(ifFalse));
    EXPECT_TRUE(acc.IsNop(trueReturn));
    EXPECT_TRUE(acc.IsNop(falseReturn));
}

HWTEST_F_L0(DeadCodeEliminationTests, MergeEliminationFramework)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    GateAccessor acc(&circuit);
    auto entry = acc.GetStateRoot();
    auto depend = acc.GetDependRoot();
    auto arg0 = builder.Arguments(0);
    auto arg1 = builder.Arguments(1);
    auto arg2 = builder.Arguments(2);
    auto dead = circuit.DeadGate();
    auto icmp = circuit.NewGate(circuit.Icmp(1), MachineType::I1, {arg0, arg1}, GateType::NJSValue());
    auto ifBranch = circuit.NewGate(circuit.IfBranch(0), {entry, icmp});
    circuit.NewGate(circuit.IfTrue(), {ifBranch});
    auto ifFalse = circuit.NewGate(circuit.IfFalse(), {ifBranch});
    auto merge = circuit.NewGate(circuit.Merge(2), {dead, ifFalse});
    auto valueSelector = circuit.NewGate(circuit.ValueSelector(2), {merge, arg1, arg2});
    auto circuitReturn = circuit.NewGate(circuit.Return(), {merge, depend, valueSelector, circuit.GetReturnRoot()});
    Chunk chunk(&allocator);
    CombinedPassVisitor visitor(&circuit, false, "dead code elimination", &chunk);
    DeadCodeElimination elimination(&circuit, &visitor, &chunk);
    visitor.AddPass(&elimination);
    visitor.VisitGraph();
    EXPECT_TRUE(acc.IsNop(merge));
    EXPECT_TRUE(acc.IsNop(valueSelector));
    EXPECT_TRUE(acc.GetValueIn(circuitReturn) == arg2);
}

HWTEST_F_L0(DeadCodeEliminationTests, MergeInputEliminationFramework)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    GateAccessor acc(&circuit);
    auto entry = acc.GetStateRoot();
    auto depend = acc.GetDependRoot();
    auto arg0 = builder.Arguments(0);
    auto arg1 = builder.Arguments(1);
    auto arg2 = builder.Arguments(2);
    auto dead = circuit.DeadGate();
    auto icmp = circuit.NewGate(circuit.Icmp(1), MachineType::I1, {arg0, arg1}, GateType::NJSValue());
    auto ifBranch = circuit.NewGate(circuit.IfBranch(0), {entry, icmp});
    auto ifTrue = circuit.NewGate(circuit.IfTrue(), {ifBranch});
    auto ifFalse = circuit.NewGate(circuit.IfFalse(), {ifBranch});
    auto ifBranch2 = circuit.NewGate(circuit.IfBranch(0), {ifTrue, icmp});
    auto ifTrue2 = circuit.NewGate(circuit.IfTrue(), {ifBranch2});
    circuit.NewGate(circuit.IfFalse(), {ifBranch2});
    auto merge = circuit.NewGate(circuit.Merge(3), {ifTrue2, dead, ifFalse});
    auto valueSelector = circuit.NewGate(circuit.ValueSelector(3), {merge, arg0, arg1, arg2});
    auto circuitReturn = circuit.NewGate(circuit.Return(), {merge, depend, valueSelector, circuit.GetReturnRoot()});
    Chunk chunk(&allocator);
    CombinedPassVisitor visitor(&circuit, false, "dead code elimination", &chunk);
    DeadCodeElimination elimination(&circuit, &visitor, &chunk);
    visitor.AddPass(&elimination);
    visitor.VisitGraph();
    EXPECT_TRUE(acc.GetOpCode(merge) == OpCode::MERGE);
    EXPECT_TRUE(acc.GetOpCode(valueSelector) == OpCode::VALUE_SELECTOR);
    EXPECT_TRUE(acc.GetValueIn(circuitReturn) == valueSelector);
    EXPECT_TRUE(acc.GetNumIns(merge) == 2);
    EXPECT_TRUE(acc.GetNumIns(valueSelector) == 3);
    EXPECT_TRUE(acc.GetIn(merge, 0) == ifTrue2);
    EXPECT_TRUE(acc.GetIn(merge, 1) == ifFalse);
    EXPECT_TRUE(acc.GetIn(valueSelector, 1) == arg0);
    EXPECT_TRUE(acc.GetIn(valueSelector, 2) == arg2);
}

}