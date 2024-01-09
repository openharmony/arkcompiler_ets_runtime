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
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/share_opcodes.h"
#include "ecmascript/compiler/value_numbering.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/typed_bytecode_lowering.h"
#include "ecmascript/compiler/typed_hcr_lowering.h"
#include "ecmascript/elements.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest.h"
#include <cstddef>
#include <vector>

namespace panda::test {
class GlobalValueNumberingTests : public testing::Test {};

using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::EcmaOpcode;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::GateRef;
using ecmascript::kungfu::PGOSampleType;
using ecmascript::kungfu::ValueNumbering;
using ecmascript::kungfu::Verifier;


HWTEST_F_L0(GlobalValueNumberingTests, AllInputsCheckedTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    auto x = builder.Arguments(1);
    auto y = builder.Arguments(2);
    auto z = builder.Arguments(3);
    auto add1 = builder.Int64Add(x, y);
    auto add2 = builder.Int64Add(x, y);
    auto add3 = builder.Int64Add(x, z);

    CombinedPassVisitor visitor(&circuit, false, "ValueNumbering", &chunk);
    ValueNumbering valuenumber(&circuit, &visitor, &chunk, true, false);

    EXPECT_EQ(valuenumber.VisitGate(add1), Circuit::NullGate());
    EXPECT_EQ(valuenumber.VisitGate(add2), add1);
    EXPECT_EQ(valuenumber.VisitGate(add3), Circuit::NullGate());
}


HWTEST_F_L0(GlobalValueNumberingTests, DeadNodesTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    auto x = builder.Arguments(1);
    auto y = builder.Arguments(2);

    auto add1 = builder.Int64Add(x, y);
    auto add2 = builder.Int64Add(x, y);

    CombinedPassVisitor visitor(&circuit, false, "ValueNumbering", &chunk);
    ValueNumbering valuenumber(&circuit, &visitor, &chunk, true, false);

    EXPECT_EQ(valuenumber.VisitGate(add1), Circuit::NullGate());
    acc.DeleteGate(add1);
    EXPECT_EQ(valuenumber.VisitGate(add2), Circuit::NullGate());
}

HWTEST_F_L0(GlobalValueNumberingTests, WontReplaceNodeWithItself)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    auto x = builder.Arguments(1);
    auto y = builder.Arguments(2);

    auto add1 = builder.Int64Add(x, y);

    CombinedPassVisitor visitor(&circuit, false, "ValueNumbering", &chunk);
    ValueNumbering valuenumber(&circuit, &visitor, &chunk, true, false);

    EXPECT_EQ(valuenumber.VisitGate(add1), Circuit::NullGate());
    EXPECT_EQ(valuenumber.VisitGate(add1), Circuit::NullGate());
}


HWTEST_F_L0(GlobalValueNumberingTests, E2ESimpleAddTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    auto x = builder.Arguments(1);
    auto y = builder.Arguments(2);


    auto add1 = builder.Int64Add(x, y);
    auto add2 = builder.Int64Add(x, y);
    auto add3 = builder.Int64Add(add1, add2);

    builder.Return(add3);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "ValueNumbering", &chunk);
    ValueNumbering valuenumber(&circuit, &visitor, &chunk, true, false);
    visitor.AddPass(&valuenumber);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_TRUE(acc.GetValueIn(add3, 0) == acc.GetValueIn(add3, 1));
    EXPECT_EQ(valuenumber.GetoptimizedGateCount(), 1);
}

HWTEST_F_L0(GlobalValueNumberingTests, GrowStressTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    std::vector<GateRef> results;
    auto x = builder.Arguments(1);
    auto y = builder.Arguments(2);

    for (int i = 0; i < 10000; i++) {
        auto add1 = builder.Int64Add(x, y);
        results.push_back(add1);
    }

    GateRef before = results[0];
    for (int i = 1; i < 10000; i++) {
        before = builder.Int64Add(before, results[i]);
    }

    builder.Return(before);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "ValueNumbering", &chunk);
    ValueNumbering valuenumber(&circuit, &visitor, &chunk, true, false);
    visitor.AddPass(&valuenumber);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(valuenumber.GetoptimizedGateCount(), 9999);
}


HWTEST_F_L0(GlobalValueNumberingTests, ComplexAddTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    auto x = builder.Arguments(1);
    auto y = builder.Arguments(2);

    auto add1 = builder.Int64Add(x, y);
    auto add2 = builder.Int64Add(x, y);
    auto add3 = builder.Int64Add(add1, add2);

    auto add4 = builder.Int64Add(x, y);
    auto add5 = builder.Int64Add(x, y);
    auto add6 = builder.Int64Add(add4, add5);

    auto add7 = builder.Int64Add(add3, add6);
    builder.Return(add7);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "ValueNumbering", &chunk);
    ValueNumbering valuenumber(&circuit, &visitor, &chunk, true, false);
    visitor.AddPass(&valuenumber);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(valuenumber.GetoptimizedGateCount(), 4);
}

} // namespace panda::test