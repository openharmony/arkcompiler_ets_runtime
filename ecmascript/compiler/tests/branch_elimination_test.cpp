/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/branch_elimination.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/dead_code_elimination.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/graph_editor.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/share_opcodes.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/typed_bytecode_lowering.h"
#include "ecmascript/compiler/typed_hcr_lowering.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest-death-test.h"
#include "gtest/gtest.h"

namespace panda::test {
class BranchEliminationTest : public testing::Test {
};
using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::MachineType;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::Label;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::GateRef;
using ecmascript::kungfu::Variable;
using ecmascript::kungfu::VariableType;
using ecmascript::kungfu::Verifier;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::PGOSampleType;
using ecmascript::kungfu::BranchElimination;
using ecmascript::kungfu::DeadCodeElimination;

HWTEST_F_L0(BranchEliminationTest, TrueBranchEliminationTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    auto entry = acc.GetStateRoot();
    auto depend = acc.GetDependRoot();
    auto arg0 = builder.Arguments(0);
    auto arg1 = builder.Arguments(1);
    auto ifBranch = circuit.NewGate(circuit.IfBranch(0), {entry, builder.Boolean(true)});
    auto ifTrue = circuit.NewGate(circuit.IfTrue(), {ifBranch});
    auto ifFalse = circuit.NewGate(circuit.IfFalse(), {ifBranch});
    auto merge = circuit.NewGate(circuit.Merge(2), {ifTrue, ifFalse});
    auto valueSelector = circuit.NewGate(circuit.ValueSelector(2), {merge, arg0, arg1});

    auto circuitReturn = circuit.NewGate(circuit.Return(), {merge, depend, valueSelector, circuit.GetReturnRoot()});
    CombinedPassVisitor branchEliminationVisitor(&circuit, false, "TrueBranchEliminationTest", &chunk);
    BranchElimination branchElimination(&circuit, &branchEliminationVisitor, false,
                                        &chunk);
    branchEliminationVisitor.AddPass(&branchElimination);
    branchEliminationVisitor.VisitGraph();

    CombinedPassVisitor deadCodeEliminationVisitor(&circuit, false, "DeadCodeElimination", &chunk);
    DeadCodeElimination deadCodeElimination(&circuit, &deadCodeEliminationVisitor, &chunk);
    deadCodeEliminationVisitor.AddPass(&deadCodeElimination);
    deadCodeEliminationVisitor.VisitGraph();

    auto arg0HasOuts = acc.HasOuts(arg0);
    auto arg1HasOuts = acc.HasOuts(arg1);

    EXPECT_TRUE(acc.IsNop(ifBranch));
    EXPECT_TRUE(acc.IsNop(ifFalse));
    EXPECT_FALSE(acc.HasOuts(ifTrue));
    EXPECT_TRUE(acc.IsNop(merge));
    EXPECT_TRUE(acc.IsNop(valueSelector));
    EXPECT_TRUE(arg0HasOuts && !arg1HasOuts);
    Verifier::Run(&circuit);
}

HWTEST_F_L0(BranchEliminationTest, FalseBranchEliminationTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    auto entry = acc.GetStateRoot();
    auto depend = acc.GetDependRoot();
    auto arg0 = builder.Arguments(0);
    auto arg1 = builder.Arguments(1);
    auto ifBranch = circuit.NewGate(circuit.IfBranch(0), {entry, builder.Boolean(false)});
    auto ifTrue = circuit.NewGate(circuit.IfTrue(), {ifBranch});
    auto ifFalse = circuit.NewGate(circuit.IfFalse(), {ifBranch});
    auto merge = circuit.NewGate(circuit.Merge(2), {ifTrue, ifFalse});
    auto valueSelector = circuit.NewGate(circuit.ValueSelector(2), {merge, arg0, arg1});

    auto circuitReturn = circuit.NewGate(circuit.Return(), {merge, depend, valueSelector, circuit.GetReturnRoot()});
    CombinedPassVisitor branchEliminationVisitor(&circuit, false, "TrueBranchEliminationTest", &chunk);
    BranchElimination branchElimination(&circuit, &branchEliminationVisitor, false,
                                        &chunk);
    branchEliminationVisitor.AddPass(&branchElimination);
    branchEliminationVisitor.VisitGraph();

    CombinedPassVisitor deadCodeEliminationVisitor(&circuit, false, "DeadCodeElimination", &chunk);
    DeadCodeElimination deadCodeElimination(&circuit, &deadCodeEliminationVisitor, &chunk);
    deadCodeEliminationVisitor.AddPass(&deadCodeElimination);
    deadCodeEliminationVisitor.VisitGraph();

    auto arg0HasOuts = acc.HasOuts(arg0);
    auto arg1HasOuts = acc.HasOuts(arg1);

    EXPECT_TRUE(acc.IsNop(ifBranch));
    EXPECT_FALSE(acc.HasOuts(ifFalse));
    EXPECT_TRUE(acc.IsNop(ifTrue));
    EXPECT_TRUE(acc.IsNop(merge));
    EXPECT_TRUE(acc.IsNop(valueSelector));
    EXPECT_TRUE(!arg0HasOuts && arg1HasOuts);
    Verifier::Run(&circuit);
}

} // namespace panda::test
