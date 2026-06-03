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
#include "ecmascript/compiler/circuit_builder.h"
#ifdef ENABLE_BRANCH_ELIMINATION
#include "ecmascript/compiler/constant_folding2.h"
#else
#include "ecmascript/compiler/constant_folding.h"
#endif
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
class ConstantFoldingTest : public testing::Test {
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
using ecmascript::kungfu::LoopPeeling;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::TypedBinOp;
using ecmascript::kungfu::PGOSampleType;
using ecmascript::kungfu::GraphLinearizer;
using ecmascript::kungfu::ConstantFolding;
using ecmascript::kungfu::CompilationConfig;

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedBinOpTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    
    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(14));
    DEFVALUE(number2, (&builder), VariableType::INT32(), builder.Int32(7));

    auto sum = builder.Int32Add(*number1, *number2);
    auto convert = builder.ConvertInt32ToTaggedInt(sum);
    builder.Return(convert);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedBinOpTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedBinOpTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(convert, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetInt32FromConstant(acc.GetValueIn(convert, 0)) == 21);
}

#ifdef ENABLE_BRANCH_ELIMINATION
HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedFAddTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    constexpr double eps = 1e-6;

    builder.SetEnvironment(&env);

    DEFVALUE(double1, (&builder), VariableType::FLOAT64(), builder.Double(1.5));
    DEFVALUE(double2, (&builder), VariableType::FLOAT64(), builder.Double(1.2));

    auto sum = builder.DoubleAdd(*double1, *double2);
    auto res = builder.Return(sum);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedFAddTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedFAddTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(res, 0)), OpCode::CONSTANT);
    EXPECT_LE(acc.GetFloat64FromConstant(acc.GetValueIn(res, 0)), 2.7 + eps);
    EXPECT_GE(acc.GetFloat64FromConstant(acc.GetValueIn(res, 0)), 2.7 - eps);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedIntSubTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(int1, (&builder), VariableType::INT32(), builder.Int32(2));
    DEFVALUE(int2, (&builder), VariableType::INT32(), builder.Int32(1));

    auto sub = builder.Int32Sub(*int1, *int2);
    auto res = builder.Return(sub);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedIntSubTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedIntSubTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(res, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(res, 0)) == 1);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedFSubTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    constexpr double eps = 1e-6;

    builder.SetEnvironment(&env);

    DEFVALUE(double1, (&builder), VariableType::FLOAT64(), builder.Double(1.5));
    DEFVALUE(double2, (&builder), VariableType::FLOAT64(), builder.Double(1.2));

    auto sub = builder.DoubleSub(*double1, *double2);
    auto res = builder.Return(sub);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedFSubTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedFSubTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(res, 0)), OpCode::CONSTANT);
    EXPECT_LE(acc.GetFloat64FromConstant(acc.GetValueIn(res, 0)), 0.3 + eps);
    EXPECT_GE(acc.GetFloat64FromConstant(acc.GetValueIn(res, 0)), 0.3 - eps);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedIntMulTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(int1, (&builder), VariableType::INT32(), builder.Int32(2));
    DEFVALUE(int2, (&builder), VariableType::INT32(), builder.Int32(1));
    DEFVALUE(int3, (&builder), VariableType::INT32(), builder.Int32(-1));
    DEFVALUE(int4, (&builder), VariableType::INT32(), builder.Int32(-2));

    auto intMul1 = builder.Int32Mul(*int1, *int2);
    auto intMul2 = builder.Int32Mul(intMul1, *int3);
    auto intMul3 = builder.Int32Mul(intMul2, *int4);
    auto res = builder.Return(intMul3);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedIntMulTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedIntMulTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(res, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(res, 0)) == 4);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedDoubleMulTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    constexpr double eps = 1e-6;

    builder.SetEnvironment(&env);

    DEFVALUE(double1, (&builder), VariableType::FLOAT64(), builder.Double(1.3));
    DEFVALUE(double2, (&builder), VariableType::FLOAT64(), builder.Double(1.5));

    auto mul = builder.DoubleMul(*double1, *double2);
    auto res = builder.Return(mul);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedFMulTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedFMulTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(res, 0)), OpCode::CONSTANT);
    EXPECT_LE(acc.GetFloat64FromConstant(acc.GetValueIn(res, 0)), 1.95 + eps);
    EXPECT_GE(acc.GetFloat64FromConstant(acc.GetValueIn(res, 0)), 1.95 - eps);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedSDivTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(14));
    DEFVALUE(number2, (&builder), VariableType::INT32(), builder.Int32(7));

    auto div = builder.Int32Div(*number1, *number2);
    auto convert = builder.ConvertInt32ToTaggedInt(div);
    builder.Return(convert);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedSDivTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedSDivTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(convert, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetInt32FromConstant(acc.GetValueIn(convert, 0)) == 2);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedFDivTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    constexpr double eps = 1e-6;

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::FLOAT64(), builder.Double(4.2));
    DEFVALUE(number2, (&builder), VariableType::FLOAT64(), builder.Double(2.1));

    auto div = builder.DoubleDiv(*number1, *number2);
    auto ret = builder.Return(div);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedFDivTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedFDivTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_LE(acc.GetFloat64FromConstant(acc.GetValueIn(ret, 0)), 2 + eps);
    EXPECT_GE(acc.GetFloat64FromConstant(acc.GetValueIn(ret, 0)), 2 - eps);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedModTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(4));
    DEFVALUE(number2, (&builder), VariableType::INT32(), builder.Int32(3));

    auto mod = builder.Int32Mod(*number1, *number2);
    auto ret = builder.Return(mod);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedModTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedModTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(ret, 0)) == 1);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedMaxTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(4));
    DEFVALUE(number2, (&builder), VariableType::INT32(), builder.Int32(3));

    auto mmax = builder.Int32Max(*number1, *number2);
    auto ret = builder.Return(mmax);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedMaxTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedMaxTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(ret, 0)) == 4);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedMinTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(4));
    DEFVALUE(number2, (&builder), VariableType::INT32(), builder.Int32(3));

    auto mmin = builder.Int32Min(*number1, *number2);
    auto ret = builder.Return(mmin);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedMinTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedMinTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(ret, 0)) == 3);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedSqrtTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    constexpr double eps = 1e-6;

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::FLOAT64(), builder.Double(4));

    auto sqrt = builder.Sqrt(*number1);
    auto ret = builder.Return(sqrt);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedSqrtTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedSqrtTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_LE(acc.GetFloat64FromConstant(acc.GetValueIn(ret, 0)), 2 + eps);
    EXPECT_GE(acc.GetFloat64FromConstant(acc.GetValueIn(ret, 0)), 2 - eps);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedRevTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);
    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(3));
    auto rev = builder.Int32Not(*number1);
    auto ret = builder.Return(rev);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedRevTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedRevTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_EQ(acc.GetInt32FromConstant(acc.GetValueIn(ret, 0)), 0xFFFFFFFC);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedAbsTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(-3));

    auto abs = builder.Abs(*number1);
    auto ret = builder.Return(abs);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedAbsTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedAbsTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(ret, 0)) == 3);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedFloorTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::FLOAT64(), builder.Double(2.7));

    auto floor = builder.DoubleFloor(*number1);
    auto ret = builder.Return(floor);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedFloorTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedFloorTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_EQ(acc.GetFloat64FromConstant(acc.GetValueIn(ret, 0)), 2);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedCeilTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::FLOAT64(), builder.Double(2.7));

    auto ceil = builder.DoubleCeil(*number1);
    auto ret = builder.Return(ceil);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedCeilTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedCeilTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_EQ(acc.GetFloat64FromConstant(acc.GetValueIn(ret, 0)), 3);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedAndTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(2));
    DEFVALUE(number2, (&builder), VariableType::INT32(), builder.Int32(3));

    auto intAnd = builder.Int32And(*number1, *number2);
    auto ret = builder.Return(intAnd);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedAndTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedAndTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(ret, 0)) == 2);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedOrTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(2));
    DEFVALUE(number2, (&builder), VariableType::INT32(), builder.Int32(3));

    auto intOr = builder.Int32Or(*number1, *number2);
    auto ret = builder.Return(intOr);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedOrTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedOrTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(ret, 0)) == 3);
}

HWTEST_F_L0(ConstantFoldingTest, ConstantFoldingTypedXorTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);

    builder.SetEnvironment(&env);

    DEFVALUE(number1, (&builder), VariableType::INT32(), builder.Int32(2));
    DEFVALUE(number2, (&builder), VariableType::INT32(), builder.Int32(3));

    auto intXor = builder.Int32Xor(*number1, *number2);
    auto ret = builder.Return(intXor);
    CombinedPassVisitor constantFoldingVisitor(&circuit, false, "ConstantFoldingTypedXorTest", &chunk);
    CompilationConfig cmpCfg = CompilationConfig("x86_64-unknown-linux-gnu");
    ConstantFolding constantFolding(&circuit, &constantFoldingVisitor, &cmpCfg, false,
                                    "ConstantFoldingTypedXorTest", &chunk);
    constantFoldingVisitor.AddPass(&constantFolding);
    constantFoldingVisitor.VisitGraph();

    EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(ret, 0)), OpCode::CONSTANT);
    EXPECT_TRUE(acc.GetConstantValue(acc.GetValueIn(ret, 0)) == 1);
}
#endif
} // namespace panda::test
