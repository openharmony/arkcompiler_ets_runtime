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
#include "ecmascript/compiler/early_elimination.h"
#include "ecmascript/compiler/later_elimination.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
class CombinedPassVisitorTests : public testing::Test {
};
using ecmascript::kungfu::Chunk;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::DeadCodeElimination;
using ecmascript::kungfu::EarlyElimination;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::LaterElimination;
using ecmascript::kungfu::MachineType;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::GateRef;
using ecmascript::kungfu::Verifier;
using ecmascript::NativeAreaAllocator;

HWTEST_F_L0(CombinedPassVisitorTests, TSLoweringTestFramework)
{
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
    auto valueSelector = circuit.NewGate(circuit.ValueSelector(2), {merge, arg0, arg1});
    auto load1 = circuit.NewGate(circuit.LoadElement(2), MachineType::I64,
                                 {merge, depend, valueSelector, arg2}, GateType::AnyType());
    auto load2 = circuit.NewGate(circuit.LoadElement(2), MachineType::I64,
                                 {load1, load1, valueSelector, arg2}, GateType::AnyType());
    auto circuitReturn = circuit.NewGate(circuit.Return(), {load2, load2, load2, circuit.GetReturnRoot()});
    Chunk chunk(&allocator);
    CombinedPassVisitor visitor(&circuit, false, "combined pass visitor test", &chunk);
    DeadCodeElimination deadCodeElimination(&circuit, &visitor, &chunk);
    EarlyElimination earlyElimination(&circuit, &visitor, &chunk, true);
    visitor.AddPass(&deadCodeElimination);
    visitor.AddPass(&earlyElimination);
    visitor.VisitGraph();
    EXPECT_TRUE(acc.IsNop(merge));
    EXPECT_TRUE(acc.IsNop(valueSelector));
    EXPECT_TRUE(acc.GetValueIn(load1, 0) == arg1);
    EXPECT_TRUE(acc.IsNop(load2));
    EXPECT_TRUE(acc.GetState(circuitReturn) == load1);
}

}