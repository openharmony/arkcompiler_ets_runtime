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
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/lcr_gate_meta_data.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/share_gate_meta_data.h"
#include "ecmascript/compiler/share_opcodes.h"
#include "ecmascript/compiler/instruction_combine.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/gate_matchers.h"
#include "ecmascript/elements.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest.h"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace panda::test {
class InstructionCombineTests : public testing::Test {};
using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::EcmaOpcode;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::Float64BinopMatcher;
using ecmascript::kungfu::Float64Matcher;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::GateRef;
using ecmascript::kungfu::InstructionCombine;
using ecmascript::kungfu::Int32BinopMatcher;
using ecmascript::kungfu::Int64BinopMatcher;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::PGOSampleType;
using ecmascript::kungfu::Verifier;
// std::numeric_limits<T>::quiet_NaN().
template <class T> T SilenceNaN(T x)
{
    assert(std::isnan(x));
    // Do some calculation to make a signalling NaN quiet.
    return x - x;
}

HWTEST_F_L0(InstructionCombineTests, Int64AddTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // test  x + 0 => x
    {
        auto x = builder.Arguments(1);
        auto const_i64_0 = builder.Int64(0);
        auto test_x_add_0 = builder.Int64Add(x, const_i64_0);
        EXPECT_EQ(instcombie.VisitGate(test_x_add_0), x);
    }
    // test  1 + 2 => 3
    {
        auto const_i64_1 = builder.Int64(1);
        auto const_i64_2 = builder.Int64(2);
        auto result = instcombie.VisitGate(builder.Int64Add(const_i64_1, const_i64_2));
        EXPECT_EQ(acc.IsConstantValue(result, 3), true);
    }
    // Test for 64-bit integer wraparound: 9223372036854775807 + 1 => -9223372036854775808
    {
        auto const_i64_max = builder.Int64(9223372036854775807);
        auto const_i64_1 = builder.Int64(1);
        auto result = instcombie.VisitGate(builder.Int64Add(const_i64_max, const_i64_1));

        // Cast -9223372036854775808 to uint64_t for comparison
        EXPECT_EQ(acc.IsConstantValue(result, static_cast<uint64_t>(9223372036854775808ULL)), true);
    }

    // test ((y+max)+1)
    {
        auto y = builder.Arguments(2);
        auto const_i64_max = builder.Int64(9223372036854775807);
        auto const_i64_1 = builder.Int64(1);
        auto result = instcombie.VisitGate(builder.Int64Add(builder.Int64Add(y, const_i64_max), const_i64_1));
        Int64BinopMatcher result_m(result, &circuit);
        EXPECT_EQ(result_m.Left().Gate(), y);
        EXPECT_EQ(result_m.Right().ResolvedValue(), static_cast<uint64_t>(9223372036854775808ULL));
    }
    // test ((z+1)+2) => (z+3)  [z+1] not Owns
    {
        auto z = builder.Arguments(3);
        auto const_i64_1 = builder.Int64(1);
        auto const_i64_2 = builder.Int64(2);
        auto z_add_1 = builder.Int64Add(z, const_i64_1);
        auto intfer = builder.Int64Add(z_add_1, z_add_1);
        (void)intfer;
        auto result = instcombie.VisitGate(builder.Int64Add(z_add_1, const_i64_2));
        EXPECT_EQ(result, Circuit::NullGate());
    }
}


HWTEST_F_L0(InstructionCombineTests, Int32AddTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);
    // test  x + 0 => x
    {
        auto x = builder.Arguments(1);
        auto const_i32_0 = builder.Int32(0);
        auto test_x_add_0 = builder.Int32Add(x, const_i32_0);
        EXPECT_EQ(instcombie.VisitGate(test_x_add_0), x);
    }

    // test  1 + 2 => 3
    {
        auto const_i32_1 = builder.Int32(1);
        auto const_i32_2 = builder.Int32(2);
        auto result = instcombie.VisitGate(builder.Int32Add(const_i32_1, const_i32_2));
        EXPECT_EQ(acc.IsConstantValue(result, 3), true);
    }

    // Test for 32-bit integer wraparound: 2147483647 + 1 => -2147483648
    {
        auto const_i32_max = builder.Int32(2147483647);
        auto const_i32_1 = builder.Int32(1);
        auto result = instcombie.VisitGate(builder.Int32Add(const_i32_max, const_i32_1));

        // Check if the result is -2147483648, demonstrating wraparound behavior
        EXPECT_EQ(acc.IsConstantValue(result, static_cast<uint64_t>(-2147483648)), true);
    }


    // test (0 - x) + y => y - x
    {
        auto x = builder.Arguments(2);
        auto y = builder.Arguments(3);
        auto zero = builder.Int32(0);
        auto result = instcombie.VisitGate(builder.Int32Add(builder.Int32Sub(zero, x), y));
        Int32BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), y);
        EXPECT_EQ(m.Right().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
    }
    // test  y + (0 - x) => y - x
    {
        auto x = builder.Arguments(4);
        auto y = builder.Arguments(5);
        auto zero = builder.Int32(0);
        auto result = instcombie.VisitGate(builder.Int32Add(y, builder.Int32Sub(zero, x)));
        Int32BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), y);
        EXPECT_EQ(m.Right().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
    }

    // test ((y+1)+2) => (y+3)  [y+1] Owns
    {
        auto y = builder.Arguments(6);
        auto const_1 = builder.Int32(1);
        auto const_2 = builder.Int32(2);
        auto result = instcombie.VisitGate(builder.Int32Add(builder.Int32Add(y, const_1), const_2));
        Int32BinopMatcher result_m(result, &circuit);
        EXPECT_EQ(result_m.Left().Gate(), y);
        EXPECT_EQ(result_m.Right().ResolvedValue(), 3);
    }

    // test ((y+max)+1)
    {
        auto y = builder.Arguments(6);
        auto const_1 = builder.Int32(2147483647);
        auto const_2 = builder.Int32(1);
        auto result = instcombie.VisitGate(builder.Int32Add(builder.Int32Add(y, const_1), const_2));
        Int32BinopMatcher result_m(result, &circuit);
        EXPECT_EQ(result_m.Left().Gate(), y);
        EXPECT_EQ(result_m.Right().ResolvedValue(), static_cast<uint64_t>(-2147483648));
    }
}


HWTEST_F_L0(InstructionCombineTests, Int64SubTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // test  x - 0 => x
    {
        auto x = builder.Arguments(1);
        auto const_i64_0 = builder.Int64(0);
        auto test_x_add_0 = builder.Int64Sub(x, const_i64_0);
        EXPECT_EQ(instcombie.VisitGate(test_x_add_0), x);
    }
    // test  1 - 2 => -1
    {
        auto const_i64_1 = builder.Int64(1);
        auto const_i64_2 = builder.Int64(2);
        auto result = instcombie.VisitGate(builder.Int64Sub(const_i64_1, const_i64_2));
        EXPECT_EQ(acc.IsConstantValue(result, (int64_t)-1), true);
    }

    // Test for 64-bit integer wraparound subtraction: -9223372036854775808 - 1 => 9223372036854775807
    {
        auto const_i64_min = builder.Int64(-9223372036854775807 - 1); // -9223372036854775808
        auto const_i64_1 = builder.Int64(1);
        auto result = instcombie.VisitGate(builder.Int64Sub(const_i64_min, const_i64_1));

        // Expect the result to wrap around to 9223372036854775807
        // Casting to uint64_t for the comparison
        EXPECT_EQ(acc.IsConstantValue(result, static_cast<uint64_t>(9223372036854775807)), true);
    }


    // test x -x => 0
    {
        auto x = builder.Arguments(2);
        auto result = instcombie.VisitGate(builder.Int64Sub(x, x));
        EXPECT_EQ(acc.IsConstantValue(result, 0), true);
    }

    // test x - k = x + (-k)
    {
        auto x = builder.Arguments(3);
        auto const_1 = builder.Int64(-1);
        auto result = instcombie.VisitGate(builder.Int64Sub(x, const_1));
        Int64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), 1);
        EXPECT_EQ(m.Opcode(), OpCode::ADD);
    }
    // Test for x - k = x + (-k) when k is the minimum int64_t value
    {
        auto x = builder.Arguments(3);
        auto const_min = builder.Int64(INT64_MIN); // Minimum int64_t value
        auto result = instcombie.VisitGate(builder.Int64Sub(x, const_min));

        // Due to overflow, -k should wrap around to INT64_MIN.
        // The opcode should be ADD if the subtraction is reinterpreted as addition with -k.
        Int64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), INT64_MIN); // Here, we expect that -k has wrapped around to INT64_MIN
        EXPECT_EQ(m.Opcode(), OpCode::ADD);
    }
}

HWTEST_F_L0(InstructionCombineTests, Int32SubTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // test  x - 0 => x
    {
        auto x = builder.Arguments(1);
        auto const_i32_0 = builder.Int32(0);
        auto test_x_add_0 = builder.Int32Sub(x, const_i32_0);
        EXPECT_EQ(instcombie.VisitGate(test_x_add_0), x);
    }
    // test  1 - 2 => -1
    {
        auto const_i32_1 = builder.Int32(1);
        auto const_i32_2 = builder.Int32(2);
        auto result = instcombie.VisitGate(builder.Int32Sub(const_i32_1, const_i32_2));
        EXPECT_EQ(acc.IsConstantValue(result, (int32_t)-1), true);
    }

    // Test for 32-bit integer wraparound subtraction: -2147483648 - 1 => 2147483647
    {
        auto const_i32_min = builder.Int32(-2147483647 - 1); // -2147483648
        auto const_i32_1 = builder.Int32(1);
        auto result = instcombie.VisitGate(builder.Int32Sub(const_i32_min, const_i32_1));

        // Expect the result to wrap around to 2147483647
        // Casting to uint64_t for the comparison, assuming IsConstantValue accepts uint64_t
        EXPECT_EQ(acc.IsConstantValue(result, static_cast<uint64_t>(2147483647)), true);
    }

    // test x -x => 0
    {
        auto x = builder.Arguments(2);
        auto result = instcombie.VisitGate(builder.Int32Sub(x, x));
        EXPECT_EQ(acc.IsConstantValue(result, 0), true);
    }

    // test x - k = x + (-k)
    {
        auto x = builder.Arguments(3);
        auto const_1 = builder.Int32(-1);
        auto result = instcombie.VisitGate(builder.Int32Sub(x, const_1));
        Int32BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), 1);
        EXPECT_EQ(m.Opcode(), OpCode::ADD);
    }

    // Test for x - k = x + (-k) when k is the minimum int32_t value
    {
        auto x = builder.Arguments(3);
        auto const_min = builder.Int32(INT32_MIN); // Minimum int32_t value
        auto result = instcombie.VisitGate(builder.Int32Sub(x, const_min));

        // Due to overflow, -k should wrap around to INT32_MIN.
        // The opcode should be ADD if the subtraction is reinterpreted as addition with -k.
        Int32BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), INT32_MIN); // Here, we expect that -k has wrapped around to INT32_MIN
        EXPECT_EQ(m.Opcode(), OpCode::ADD);
    }
}


HWTEST_F_L0(InstructionCombineTests, Int64MulTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // test  x * 0 => 0
    {
        auto x = b.Arguments(1);
        auto const_0 = b.Int64(0);
        auto result = b.Int64Mul(x, const_0);
        EXPECT_EQ(instcombie.VisitGate(result), const_0);
    }
    // test  x * 1 => x
    {
        auto x = b.Arguments(1);
        auto const_1 = b.Int64(1);
        auto result = b.Int64Mul(x, const_1);
        EXPECT_EQ(instcombie.VisitGate(result), x);
    }

    // test  1 * 2 => 2
    {
        auto const_1 = b.Int64(1);
        auto const_2 = b.Int64(2);
        auto result = instcombie.VisitGate(b.Int64Mul(const_1, const_2));
        EXPECT_EQ(acc.IsConstantValue(result, (int64_t)2), true);
    }
    // Test for 64-bit integer wraparound multiplication: 9223372036854775807 * 2 => -2
    {
        auto const_i64_max = b.Int64(9223372036854775807); // Max int64_t value
        auto const_i64_2 = b.Int64(2);
        auto result = instcombie.VisitGate(b.Int64Mul(const_i64_max, const_i64_2));

        // Expect the result to wrap around to -2
        // Casting to uint64_t for the comparison
        EXPECT_EQ(acc.IsConstantValue(result, static_cast<uint64_t>(-2)), true);
    }

    // test x * -1 => 0 - X
    {
        auto x = b.Arguments(2);
        auto result = instcombie.VisitGate(b.Int64Mul(x, b.Int64(-1)));
        Int64BinopMatcher m(result, &circuit);

        EXPECT_EQ(m.Left().ResolvedValue(), 0);
        EXPECT_EQ(m.Right().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
    }

    // test x * 2^n => x << n
    {
        auto x = b.Arguments(3);
        auto const_4 = b.Int64(4);
        auto result = instcombie.VisitGate(b.Int64Mul(x, const_4));
        Int64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), 2);
        EXPECT_EQ(m.Opcode(), OpCode::LSL);
    }

    // (x * Int64Constant(a)) * Int64Constant(b)) => x * Int64Constant(a * b)
    {
        auto x = b.Arguments(4);
        auto const_4 = b.Int64(4);
        auto const_5 = b.Int64(5);
        auto result = instcombie.VisitGate(b.Int64Mul(b.Int64Mul(x, const_4), const_5));
        Int64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), 20);
        EXPECT_EQ(m.Opcode(), OpCode::MUL);
    }
    // Test for (x * Int64Constant(a)) * Int64Constant(b)) => x * Int64Constant(a * b) with overflow
    {
        auto x = b.Arguments(4);
        auto const_almost_max = b.Int64(INT64_MAX - 1); // INT64_MAX - 1
        auto const_3 = b.Int64(3);
        auto result = instcombie.VisitGate(b.Int64Mul(b.Int64Mul(x, const_almost_max), const_3));

        // Due to overflow, a * b should wrap around.
        // The opcode should still be MUL.
        Int64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), 9223372036854775802);

        EXPECT_EQ(m.Opcode(), OpCode::MUL);
    }
}

HWTEST_F_L0(InstructionCombineTests, Int32MulTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // test  x * 0 => 0
    {
        auto x = b.Arguments(1);
        auto const_0 = b.Int32(0);
        auto result = b.Int32Mul(x, const_0);
        EXPECT_EQ(instcombie.VisitGate(result), const_0);
    }
    // test  x * 1 => x
    {
        auto x = b.Arguments(1);
        auto const_1 = b.Int32(1);
        auto result = b.Int32Mul(x, const_1);
        EXPECT_EQ(instcombie.VisitGate(result), x);
    }


    // test  1 * 2 => 2
    {
        auto const_1 = b.Int32(1);
        auto const_2 = b.Int32(2);
        auto result = instcombie.VisitGate(b.Int32Mul(const_1, const_2));
        EXPECT_NE(result, Circuit::NullGate());
        EXPECT_EQ(acc.IsConstantValue(result, (int32_t)2), true);
    }

    // Test for 32-bit integer wraparound multiplication: 2147483647 * 2 => -2
    {
        auto const_i32_max = b.Int32(2147483647); // Max int32_t value
        auto const_i32_2 = b.Int32(2);
        auto result = instcombie.VisitGate(b.Int32Mul(const_i32_max, const_i32_2));

        // Expect the result to wrap around to -2
        // Casting to uint32_t for the comparison
        EXPECT_NE(result, Circuit::NullGate());
        EXPECT_EQ(acc.GetInt32FromConstant(result), -2);
    }


    // test x * -1 => 0 - X
    {
        auto x = b.Arguments(2);
        auto result = instcombie.VisitGate(b.Int32Mul(x, b.Int32(-1)));
        EXPECT_NE(result, Circuit::NullGate());
        Int32BinopMatcher m(result, &circuit);

        EXPECT_EQ(m.Left().ResolvedValue(), 0);
        EXPECT_EQ(m.Right().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
    }

    // test x * 2^n => x << n
    {
        auto x = b.Arguments(3);
        auto const_4 = b.Int32(4);
        auto result = instcombie.VisitGate(b.Int32Mul(x, const_4));
        EXPECT_NE(result, Circuit::NullGate());
        Int32BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), 2);
        EXPECT_EQ(m.Opcode(), OpCode::LSL);
    }

    // (x * Int32Constant(a)) * Int32Constant(b)) => x * Int32Constant(a * b)
    {
        auto x = b.Arguments(4);
        auto const_4 = b.Int32(4);
        auto const_5 = b.Int32(5);
        auto result = instcombie.VisitGate(b.Int32Mul(b.Int32Mul(x, const_4), const_5));
        EXPECT_NE(result, Circuit::NullGate());
        Int32BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().ResolvedValue(), 20);
        EXPECT_EQ(m.Opcode(), OpCode::MUL);
    }
    // Test for (x * Int32Constant(a)) * Int32Constant(b)) => x * Int32Constant(a * b) with overflow
    {
        auto x = b.Arguments(4);
        auto const_almost_max = b.Int32(INT32_MAX - 1); // INT32_MAX - 1
        auto const_3 = b.Int32(3);
        auto result = instcombie.VisitGate(b.Int32Mul(b.Int32Mul(x, const_almost_max), const_3));

        // Due to overflow, a * b should wrap around.
        // The opcode should still be MUL.
        Int32BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);

        EXPECT_EQ(m.Right().ResolvedValue(), static_cast<uint32_t>(2147483642));

        EXPECT_EQ(m.Opcode(), OpCode::MUL);
    }
}

HWTEST_F_L0(InstructionCombineTests, Int64DivTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // test  0/x => 0
    {
        auto x = b.Arguments(1);
        auto const_0 = b.Int64(0);
        auto result = b.Int64Div(const_0, x);
        EXPECT_EQ(instcombie.VisitGate(result), const_0);
    }


    // test  x/0 => 0
    {
        auto x = b.Arguments(1);
        auto const_0 = b.Int64(0);
        auto result = b.Int64Div(x, const_0);
        EXPECT_EQ(instcombie.VisitGate(result), const_0);
    }

    // test  x / 1 => x
    {
        auto x = b.Arguments(1);
        auto const_1 = b.Int64(1);
        auto result = b.Int64Div(x, const_1);
        EXPECT_EQ(instcombie.VisitGate(result), x);
    }

    // test  4 / 2 => 4
    {
        auto const_4 = b.Int64(4);
        auto const_2 = b.Int64(2);
        auto result = instcombie.VisitGate(b.Int64Div(const_4, const_2));
        EXPECT_NE(result, Circuit::NullGate());
        EXPECT_EQ(acc.IsConstantValue(result, (int64_t)2), true);
    }

    // test x / -1 => 0 - X
    {
        auto x = b.Arguments(2);
        auto result = instcombie.VisitGate(b.Int64Div(x, b.Int64(-1)));

        EXPECT_NE(result, Circuit::NullGate());
        Int64BinopMatcher m(result, &circuit);

        EXPECT_EQ(m.Left().ResolvedValue(), 0);
        EXPECT_EQ(m.Right().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
    }

    // test x / -5 => 0 - x / 5
    {
        auto x = b.Arguments(2);
        auto result = instcombie.VisitGate(b.Int64Div(x, b.Int64(-5)));
        EXPECT_NE(result, Circuit::NullGate());
        Int64BinopMatcher m(result, &circuit);

        EXPECT_EQ(m.Left().ResolvedValue(), 0);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
        Int64BinopMatcher m_right(m.Right().Gate(), &circuit);

        EXPECT_EQ(m_right.Left().Gate(), x);
        EXPECT_EQ(m_right.Opcode(), OpCode::SDIV);
        EXPECT_EQ(m_right.Right().ResolvedValue(), 5);
    }

    // test x / -9,223,372,036,854,775,808
    {
        auto x = b.Arguments(2);
        auto result = instcombie.VisitGate(b.Int64Div(x, b.Int64(std::numeric_limits<int64_t>::min())));
        EXPECT_EQ(result, Circuit::NullGate());
    }
}

HWTEST_F_L0(InstructionCombineTests, Int32DivTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // test  0/x => 0
    {
        auto x = b.Arguments(1);
        auto const_0 = b.Int32(0);
        auto result = b.Int32Div(const_0, x);
        EXPECT_EQ(instcombie.VisitGate(result), const_0);
    }


    // test  x/0 => 0
    {
        auto x = b.Arguments(1);
        auto const_0 = b.Int32(0);
        auto result = b.Int32Div(x, const_0);
        EXPECT_EQ(instcombie.VisitGate(result), const_0);
    }

    // test  x / 1 => x
    {
        auto x = b.Arguments(1);
        auto const_1 = b.Int32(1);
        auto result = b.Int32Div(x, const_1);
        EXPECT_EQ(instcombie.VisitGate(result), x);
    }

    // test  4 / 2 => 4
    {
        auto const_4 = b.Int32(4);
        auto const_2 = b.Int32(2);
        auto result = instcombie.VisitGate(b.Int32Div(const_4, const_2));

        EXPECT_NE(result, Circuit::NullGate());
        EXPECT_EQ(acc.IsConstantValue(result, (int32_t)2), true);
    }

    // test x / -1 => 0 - X
    {
        auto x = b.Arguments(2);
        auto result = instcombie.VisitGate(b.Int32Div(x, b.Int32(-1)));
        EXPECT_NE(result, Circuit::NullGate());
        Int32BinopMatcher m(result, &circuit);

        EXPECT_EQ(m.Left().ResolvedValue(), 0);
        EXPECT_EQ(m.Right().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
    }

    // test x / -5 => 0 - x / 5
    {
        auto x = b.Arguments(2);
        auto result = instcombie.VisitGate(b.Int32Div(x, b.Int32(-5)));
        EXPECT_NE(result, Circuit::NullGate());
        Int32BinopMatcher m(result, &circuit);

        EXPECT_EQ(m.Left().ResolvedValue(), 0);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
        Int32BinopMatcher m_right(m.Right().Gate(), &circuit);

        EXPECT_EQ(m_right.Left().Gate(), x);
        EXPECT_EQ(m_right.Opcode(), OpCode::SDIV);
        EXPECT_EQ(m_right.Right().ResolvedValue(), 5);
    }
}

HWTEST_F_L0(InstructionCombineTests, DoubleBinOptest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // test  x op NaN => NaN
    {
        auto x = b.Arguments(1);
        auto nan = b.NanValue();
        auto zero = b.Double(0);
        Float64Matcher nan_m(nan, &circuit);
        EXPECT_EQ(nan_m.HasResolvedValue(), true);


        EXPECT_EQ(instcombie.VisitGate(b.DoubleAdd(x, nan)), nan);
        EXPECT_EQ(instcombie.VisitGate(b.DoubleSub(x, nan)), nan);
        EXPECT_EQ(instcombie.VisitGate(b.DoubleMul(x, nan)), nan);
        EXPECT_EQ(instcombie.VisitGate(b.DoubleDiv(x, nan)), nan);
        // x % 0 => NaN
        EXPECT_EQ(instcombie.VisitGate(b.DoubleMod(x, zero)), nan);
    }
    // test  NaN op op => NaN
    {
        auto x = b.Arguments(1);
        auto nan = b.NanValue();
        EXPECT_EQ(instcombie.VisitGate(b.DoubleAdd(nan, x)), nan);
        EXPECT_EQ(instcombie.VisitGate(b.DoubleSub(nan, x)), nan);
        EXPECT_EQ(instcombie.VisitGate(b.DoubleDiv(nan, x)), nan);
        EXPECT_EQ(instcombie.VisitGate(b.DoubleMod(nan, x)), nan);
    }

    // test 10.4 op 5.2 => ?
    {
        auto value1 = b.Double(10.1);
        auto value2 = b.Double(5.2);
        auto result = instcombie.VisitGate(b.DoubleAdd(value1, value2));
        EXPECT_NE(result, Circuit::NullGate());
        EXPECT_EQ(acc.GetOpCode(result), OpCode::CONSTANT);

        result = instcombie.VisitGate(b.DoubleAdd(value1, value2));
        EXPECT_NE(result, Circuit::NullGate());
        EXPECT_EQ(acc.GetOpCode(result), OpCode::CONSTANT);

        result = instcombie.VisitGate(b.DoubleSub(value1, value2));
        EXPECT_NE(result, Circuit::NullGate());
        EXPECT_EQ(acc.GetOpCode(result), OpCode::CONSTANT);

        result = instcombie.VisitGate(b.DoubleDiv(value1, value2));
        EXPECT_NE(result, Circuit::NullGate());
        EXPECT_EQ(acc.GetOpCode(result), OpCode::CONSTANT);
    }

    // test x * -1.0 => -0.0 - x
    {
        auto x = b.Arguments(1);
        auto neg_one = b.Double(-1);
        auto result = instcombie.VisitGate(b.DoubleMul(x, neg_one));

        EXPECT_NE(result, Circuit::NullGate());
        Float64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Opcode(), OpCode::SUB);
        EXPECT_EQ(m.Left().ResolvedValue(), -0.0);
        EXPECT_EQ(m.Right().Gate(), x);
    }

    // test x * -1.0 => -0.0 - x
    {
        auto x = b.Arguments(1);
        auto two = b.Double(2.0);
        auto result = instcombie.VisitGate(b.DoubleMul(x, two));
        EXPECT_NE(result, Circuit::NullGate());
        Float64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Opcode(), OpCode::ADD);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Right().Gate(), x);
    }
}
HWTEST_F_L0(InstructionCombineTests, Int32Modtest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);
    {
        auto x = b.Arguments(1);
        auto zero = b.Int32(0);
        auto one = b.Int32(1);
        auto neg_one = b.Int32(-1);
        auto four = b.Int32(4);
        auto two = b.Int32(2);

        // 0 % x  => 0
        // x % 0  => 0
        // x % 1  => 0
        // x % -1 => 0
        // x % x  => 0
        EXPECT_EQ(instcombie.VisitGate(b.Int32Mod(x, zero)), zero);
        EXPECT_EQ(instcombie.VisitGate(b.Int32Mod(zero, x)), zero);
        EXPECT_EQ(instcombie.VisitGate(b.Int32Mod(x, one)), zero);
        EXPECT_EQ(instcombie.VisitGate(b.Int32Mod(x, neg_one)), zero);
        EXPECT_EQ(instcombie.VisitGate(b.Int32Mod(x, x)), zero);
        // 2%4 =>2
        EXPECT_EQ(instcombie.VisitGate(b.Int32Mod(two, four)), two);
    }
}

HWTEST_F_L0(InstructionCombineTests, Int32AddOverFlowtest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // IsFoldable overflow
    // 2147483647 + 1 =-2147483648
    {
        auto add_overflow = b.AddWithOverflow(b.Int32(2147483647), b.Int32(1));
        auto add_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, add_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, add_overflow, b.Int32(1));
        EXPECT_EQ(acc.GetInt32FromConstant(instcombie.VisitGate(add_result)), -2147483648);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(true));
    }

    // IsFoldable no overflow
    {
        auto add_overflow = b.AddWithOverflow(b.Int32(2147483646), b.Int32(1));
        auto add_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, add_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, add_overflow, b.Int32(1));
        EXPECT_EQ(acc.GetInt32FromConstant(instcombie.VisitGate(add_result)), 2147483647);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(false));
    }


    // x add 0 => 0
    // IsFoldable no overflow
    {
        auto x = b.Arguments(1);
        auto add_overflow = b.AddWithOverflow(x, b.Int32(0));
        auto add_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, add_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, add_overflow, b.Int32(1));
        EXPECT_EQ(instcombie.VisitGate(add_result), x);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(false));
    }
}

HWTEST_F_L0(InstructionCombineTests, Int32SubOverFlowTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // IsFoldable overflow
    // -2147483648 - 1 = 2147483647
    {
        auto sub_overflow = b.SubWithOverflow(b.Int32(-2147483648), b.Int32(1));
        auto sub_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, sub_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, sub_overflow, b.Int32(1));
        EXPECT_EQ(acc.GetInt32FromConstant(instcombie.VisitGate(sub_result)), 2147483647);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(true));
    }

    // IsFoldable no overflow
    {
        auto sub_overflow = b.SubWithOverflow(b.Int32(2147483647), b.Int32(1));
        auto sub_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, sub_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, sub_overflow, b.Int32(1));
        EXPECT_EQ(acc.GetInt32FromConstant(instcombie.VisitGate(sub_result)), 2147483646);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(false));
    }

    // x sub 0 => x
    // IsFoldable no overflow
    {
        auto x = b.Arguments(1);
        auto sub_overflow = b.SubWithOverflow(x, b.Int32(0));
        auto sub_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, sub_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, sub_overflow, b.Int32(1));
        EXPECT_EQ(instcombie.VisitGate(sub_result), x);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(false));
    }
}

HWTEST_F_L0(InstructionCombineTests, Int32MulOverFlowTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    // IsFoldable overflow
    // 2147483647 * 2 = -2
    {
        auto mul_overflow = b.MulWithOverflow(b.Int32(2147483647), b.Int32(2));
        auto mul_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, mul_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, mul_overflow, b.Int32(1));
        EXPECT_EQ(acc.GetInt32FromConstant(instcombie.VisitGate(mul_result)), -2);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(true));
    }

    // IsFoldable no overflow
    // 1000 * 2 = 2000
    {
        auto mul_overflow = b.MulWithOverflow(b.Int32(1000), b.Int32(2));
        auto mul_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, mul_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, mul_overflow, b.Int32(1));
        EXPECT_EQ(acc.GetInt32FromConstant(instcombie.VisitGate(mul_result)), 2000);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(false));
    }

    // x * 1 => x
    // IsFoldable no overflow
    {
        auto x = b.Arguments(1);
        auto mul_overflow = b.MulWithOverflow(x, b.Int32(1));
        auto mul_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, mul_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, mul_overflow, b.Int32(1));
        EXPECT_EQ(instcombie.VisitGate(mul_result), x);
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(false));
    }

    // x * 0 => 0
    // IsFoldable no overflow
    {
        auto x = b.Arguments(1);
        auto mul_overflow = b.MulWithOverflow(x, b.Int32(0));
        auto mul_result = b.ExtractValue(ecmascript::kungfu::MachineType::I32, mul_overflow, b.Int32(0));
        auto is_over = b.ExtractValue(ecmascript::kungfu::MachineType::I1, mul_overflow, b.Int32(1));
        EXPECT_EQ(instcombie.VisitGate(mul_result), b.Int32(0));
        EXPECT_EQ(instcombie.VisitGate(is_over), b.Boolean(false));
    }
}

HWTEST_F_L0(InstructionCombineTests, Int64_32AndTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);
    {
        auto x = b.Arguments(1);
        auto zero = b.Int64(0);
        auto neg_one = b.Int64(-1);
        auto Icmp = b.Equal(x, zero);
        auto one = b.Int64(1);
        auto two = b.Int64(2);
        EXPECT_EQ(instcombie.VisitGate(b.Int64And(x, zero)), zero);   // x & 0  => 0
        EXPECT_EQ(instcombie.VisitGate(b.Int64And(x, neg_one)), x);   // x & -1 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int64And(Icmp, one)), Icmp); // CMP & 1 => CMP
        EXPECT_EQ(instcombie.VisitGate(b.Int64And(two, one)), zero);  // K & K  => K
        EXPECT_EQ(instcombie.VisitGate(b.Int64And(x, x)), x);         // x & x => x
        // (x & 1) & 2 => x & 0
        auto result = instcombie.VisitGate(b.Int64And(b.Int64And(x, one), two));
        EXPECT_NE(result, Circuit::NullGate());
        Int64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::AND);
        EXPECT_EQ(m.Right().Gate(), zero);
    }

    {
        auto x = b.Arguments(2);
        auto zero = b.Int32(0);
        auto neg_one = b.Int32(-1);
        auto Icmp = b.Equal(x, zero);
        auto one = b.Int32(1);
        auto two = b.Int32(2);
        EXPECT_EQ(instcombie.VisitGate(b.Int32And(x, zero)), zero);   // x & 0  => 0
        EXPECT_EQ(instcombie.VisitGate(b.Int32And(x, neg_one)), x);   // x & -1 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int32And(Icmp, one)), Icmp); // CMP & 1 => CMP
        EXPECT_EQ(instcombie.VisitGate(b.Int32And(two, one)), zero);  // K & K  => K
        EXPECT_EQ(instcombie.VisitGate(b.Int32And(x, x)), x);         // x & x => x
        // (x & 1) & 2 => x & 0
        auto result = instcombie.VisitGate(b.Int32And(b.Int32And(x, one), two));
        EXPECT_NE(result, Circuit::NullGate());
        Int64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::AND);
        EXPECT_EQ(m.Right().Gate(), zero);
    }
}

HWTEST_F_L0(InstructionCombineTests, Int64_32OrTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);
    {
        auto x = b.Arguments(1);
        auto zero = b.Int64(0);
        auto neg_one = b.Int64(-1);
        auto one = b.Int64(1);
        auto two = b.Int64(2);
        auto three = b.Int64(3);
        EXPECT_EQ(instcombie.VisitGate(b.Int64Or(x, zero)), x);          // x | 0  => x
        EXPECT_EQ(instcombie.VisitGate(b.Int64Or(x, neg_one)), neg_one); // x | -1 => -1
        EXPECT_EQ(instcombie.VisitGate(b.Int64Or(two, one)), three);     // 2 | 1  => 3
        EXPECT_EQ(instcombie.VisitGate(b.Int64Or(x, x)), x);             // x | x => x
        // (x & K1) | K2 => x | K2 if K2 has ones for every zero bit in K1.
        auto result = instcombie.VisitGate(
            b.Int64Or(b.Int64And(x, b.Int64(-6148914691236517205L)), b.Int64(6148914691236517205L)));
        EXPECT_NE(result, Circuit::NullGate());
        Int64BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::OR);
        EXPECT_EQ(m.Right().Gate(), b.Int64(6148914691236517205L));
    }

    {
        auto x = b.Arguments(1);
        auto zero = b.Int32(0);
        auto neg_one = b.Int32(-1);
        auto one = b.Int32(1);
        auto two = b.Int32(2);
        auto three = b.Int32(3);
        EXPECT_EQ(instcombie.VisitGate(b.Int32Or(x, zero)), x);          // x | 0  => x
        EXPECT_EQ(instcombie.VisitGate(b.Int32Or(x, neg_one)), neg_one); // x | -1 => -1
        EXPECT_EQ(instcombie.VisitGate(b.Int32Or(two, one)), three);     // 2 | 1  => 3
        EXPECT_EQ(instcombie.VisitGate(b.Int32Or(x, x)), x);             // x | x => x
        // (x & K1) | K2 => x | K2 if K2 has ones for every zero bit in K1.
        auto result = instcombie.VisitGate(b.Int32Or(b.Int32And(x, b.Int32(-1431655765)), b.Int32(1431655765)));
        EXPECT_NE(result, Circuit::NullGate());
        Int32BinopMatcher m(result, &circuit);
        EXPECT_EQ(m.Left().Gate(), x);
        EXPECT_EQ(m.Opcode(), OpCode::OR);
        EXPECT_EQ(m.Right().Gate(), b.Int32(1431655765));
    }
}

HWTEST_F_L0(InstructionCombineTests, Int64_32XOrTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);
    {
        auto x = b.Arguments(1);
        auto zero = b.Int64(0);
        auto neg_one = b.Int64(-1);
        auto one = b.Int64(1);
        auto two = b.Int64(2);
        auto three = b.Int64(3);
        EXPECT_EQ(instcombie.VisitGate(b.Int64Xor(x, zero)), x);      // x ^ 0 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int64Xor(two, one)), three); // 2 | 1  => 3
        EXPECT_EQ(instcombie.VisitGate(b.Int64Xor(x, x)), zero);      // x ^ x => 0
        // (x ^ -1) ^ -1 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int64Xor(b.Int64Xor(x, neg_one), neg_one)), x);
    }

    {
        auto x = b.Arguments(1);
        auto zero = b.Int32(0);
        auto neg_one = b.Int32(-1);
        auto one = b.Int32(1);
        auto two = b.Int32(2);
        auto three = b.Int32(3);
        EXPECT_EQ(instcombie.VisitGate(b.Int32Xor(x, zero)), x);      // x ^ 0 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int32Xor(two, one)), three); // 2 | 1  => 3
        EXPECT_EQ(instcombie.VisitGate(b.Int32Xor(x, x)), zero);      // x ^ x => 0
        // (x ^ -1) ^ -1 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int32Xor(b.Int32Xor(x, neg_one), neg_one)), x);
    }
}

HWTEST_F_L0(InstructionCombineTests, Int64_32LsrTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    {
        auto x = b.Arguments(1);
        auto zero = b.Int64(0);
        auto two = b.Int64(2);
        EXPECT_EQ(instcombie.VisitGate(b.Int64LSR(x, zero)), x); // x >>> 0 => x
        // (u)-8 >> 2  => 4611686018427387902u
        // 8 >> 2  => 2
        EXPECT_EQ(instcombie.VisitGate(b.Int64LSR(b.Int64(-8), two)), b.Int64(4611686018427387902u));
        EXPECT_EQ(instcombie.VisitGate(b.Int64LSR(b.Int64(8), two)), b.Int64(2));
    }

    {
        auto x = b.Arguments(1);
        auto zero = b.Int32(0);
        auto two = b.Int32(2);
        EXPECT_EQ(instcombie.VisitGate(b.Int32LSR(x, zero)), x); // x >>> 0 => x
        // (u)-8 >> 2  => 1073741822u
        // 8 >> 2  => 2
        EXPECT_EQ(instcombie.VisitGate(b.Int32LSR(b.Int32(-8), two)), b.Int32(1073741822u));
        EXPECT_EQ(instcombie.VisitGate(b.Int32LSR(b.Int32(8), two)), b.Int32(2));
        // (x & 1023) >>> 10  => 0
        EXPECT_EQ(instcombie.VisitGate(b.Int32LSR(b.Int32And(x, b.Int32(1023)), b.Int32(10))), zero);
    }
}

HWTEST_F_L0(InstructionCombineTests, Int64_32AsrTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);
    {
        auto x = b.Arguments(1);
        auto zero = b.Int64(0);
        auto two = b.Int64(2);
        // x >> 0 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int64ASR(x, zero)), x);
        // -8 >> 2  => -2
        // 8 >> 2  => 2
        EXPECT_EQ(instcombie.VisitGate(b.Int64ASR(b.Int64(-8), two)), b.Int64(-2));
        EXPECT_EQ(instcombie.VisitGate(b.Int64ASR(b.Int64(8), two)), b.Int64(2));
    }
    {
        auto x = b.Arguments(1);
        auto zero = b.Int32(0);
        auto two = b.Int32(2);
        // x >> 0 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int32ASR(x, zero)), x);
        // -8 >> 2  => -2
        // 8 >> 2  => 2
        EXPECT_EQ(instcombie.VisitGate(b.Int32ASR(b.Int32(-8), two)), b.Int32(-2));
        EXPECT_EQ(instcombie.VisitGate(b.Int32ASR(b.Int32(8), two)), b.Int32(2));
    }
}

HWTEST_F_L0(InstructionCombineTests, Int64_32LslTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);
    {
        auto x = b.Arguments(1);
        auto zero = b.Int64(0);
        auto two = b.Int64(2);
        // x << 0 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int64LSL(x, zero)), x);
        // 1 << 2  => 4
        EXPECT_EQ(instcombie.VisitGate(b.Int64LSL(b.Int64(1), two)), b.Int64(4));
    }
    {
        auto x = b.Arguments(1);
        auto zero = b.Int32(0);
        auto two = b.Int32(2);
        // x << 0 => x
        EXPECT_EQ(instcombie.VisitGate(b.Int32LSL(x, zero)), x);
        // 1 << 2  => 4
        EXPECT_EQ(instcombie.VisitGate(b.Int32LSL(b.Int32(1), two)), b.Int32(4));
    }
}

HWTEST_F_L0(InstructionCombineTests, Int64EqualTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);

    //  Match {EQ ((x or constant1) , constant2)} {((constant1 || constant2) != constant2)} => false
    {
        auto x = b.Arguments(1);
        auto constant1 = b.Int64(5);
        auto constant2 = b.Int64(10);
        auto result = instcombie.VisitGate(b.Equal(b.Int64Or(x, constant1), constant2));
        EXPECT_EQ(result, b.False());
    }

    // Taggedalue
    {
        auto x = b.Arguments(1);
        auto constant1 = b.Int64(5);
        auto constant2 = b.Int64(10);
        auto result = instcombie.VisitGate(b.Equal(b.Int64ToTaggedPtr(b.Int64Or(x, constant1)), constant2));
        EXPECT_EQ(result, b.False());
    }
    // Match {EQ((X or constant1) & constant2, 0)} { (constan2 !=0 && constant1 & constant2 !=0) }=> false
    {
        auto x = b.Arguments(1);
        auto constant1 = b.Int64(15);
        auto constant2 = b.Int64(7);
        auto zero = b.Int64(0);
        auto result = instcombie.VisitGate(b.Equal(b.Int64And(b.Int64Or(x, constant1), constant2), zero));
        EXPECT_EQ(result, b.False());
    }
}

HWTEST_F_L0(InstructionCombineTests, ConvertTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder b(&circuit);
    Environment env(0, &b);
    b.SetEnvironment(&env);
    CombinedPassVisitor visitor(&circuit, false, "InstCombie", &chunk);
    InstructionCombine instcombie(&circuit, &visitor, &chunk);
    {
        auto x = b.Arguments(1);
        EXPECT_EQ(instcombie.VisitGate(b.ChangeFloat64ToInt32(b.ChangeInt32ToFloat64(x))), x);
    }
    {
        auto x = b.Arguments(2);
        EXPECT_NE(instcombie.VisitGate(b.ChangeInt32ToFloat64(b.ChangeFloat64ToInt32(x))), x);
    }
    {
        auto x = b.Arguments(3);
        EXPECT_NE(instcombie.VisitGate(b.Int64ToTaggedPtr(b.ChangeTaggedPointerToInt64(x))), x);
    }
}
} // namespace panda::test