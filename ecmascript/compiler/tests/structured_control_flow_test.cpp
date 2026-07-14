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

#include <algorithm>

#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/compiler/stub_builder-inl.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
using ecmascript::NativeAreaAllocator;
using namespace ecmascript::kungfu;

class StructuredControlFlowTest : public testing::Test {};

class StructuredControlFlowStubBuilder : public StubBuilder {
public:
    explicit StructuredControlFlowStubBuilder(Environment *env) : StubBuilder(env) {}
    ~StructuredControlFlowStubBuilder() override = default;

    void GenerateCircuit() override {}

    void BuildIfElseMerge(GateRef condition)
    {
        DEFVARIABLE(result, VariableType::INT32(), Int32(0));
        IR_IF (condition) {
            result = Int32(1);
        } IR_ELSE {
            result = Int32(2);
        }
        Return(*result);
    }

    void BuildTerminatedIfElse(GateRef condition)
    {
        IR_IF (condition) {
            Return(Int32(1));
        } IR_ELSE {
            Return(Int32(2));
        }
    }

    void BuildSingleIf(GateRef condition)
    {
        DEFVARIABLE(result, VariableType::INT32(), Int32(0));
        IR_IF (condition) {
            result = Int32(1);
        }
        Return(*result);
    }

    void BuildIfElseWithTerminatedTrueBranch(GateRef condition)
    {
        DEFVARIABLE(result, VariableType::INT32(), Int32(0));
        IR_IF (condition) {
            Return(Int32(1));
        } IR_ELSE {
            result = Int32(2);
        }
        Return(*result);
    }

    void BuildWhile()
    {
        constexpr int32_t loopLimit = 4;
        DEFVARIABLE(i, VariableType::INT32(), Int32(0));
        DEFVARIABLE(sum, VariableType::INT32(), Int32(0));
        IR_WHILE (Int32LessThan(*i, Int32(loopLimit))) {
            sum = Int32Add(*sum, *i);
            i = Int32Add(*i, Int32(1));
        }
        Return(*sum);
    }

    void BuildNestedLoopWithBreakAndContinue()
    {
        constexpr int32_t outerLoopLimit = 2;
        constexpr int32_t innerLoopLimit = 3;
        DEFVARIABLE(outer, VariableType::INT32(), Int32(0));
        DEFVARIABLE(inner, VariableType::INT32(), Int32(0));
        DEFVARIABLE(sum, VariableType::INT32(), Int32(0));
        IR_WHILE (Int32LessThan(*outer, Int32(outerLoopLimit))) {
            inner = Int32(0);
            IR_WHILE (Int32LessThan(*inner, Int32(innerLoopLimit))) {
                inner = Int32Add(*inner, Int32(1));
                IR_IF (Int32Equal(*inner, Int32(1))) {
                    IR_CONTINUE();
                }
                IR_IF (Int32Equal(*inner, Int32(2))) { // 2: break count
                    IR_BREAK();
                }
                sum = Int32Add(*sum, *inner);
            }
            outer = Int32Add(*outer, Int32(1));
        }
        Return(*sum);
    }

    void BuildContinueOnlyLoop()
    {
        constexpr int32_t loopLimit = 3;
        DEFVARIABLE(i, VariableType::INT32(), Int32(0));
        IR_WHILE (Int32LessThan(*i, Int32(loopLimit))) {
            i = Int32Add(*i, Int32(1));
            IR_CONTINUE();
        }
        Return(*i);
    }

    void BuildLoopWithBreak()
    {
        constexpr int32_t loopLimit = 3;
        DEFVARIABLE(i, VariableType::INT32(), Int32(0));
        DEFVARIABLE(count, VariableType::INT32(), Int32(0));
        IR_WHILE (Int32LessThan(*i, Int32(loopLimit))) {
            i = Int32Add(*i, Int32(1));
            IR_IF (Int32Equal(*i, Int32(1))) {
                IR_BREAK();
            }
            count = Int32Add(*count, Int32(1));
        }
        Return(*count);
    }
};

size_t CountOpCode(Circuit *circuit, OpCode opcode)
{
    GateAccessor accessor(circuit);
    std::vector<GateRef> gates;
    accessor.GetAllGates(gates);
    return std::count_if(gates.begin(), gates.end(), [&accessor, opcode](GateRef gate) {
        return accessor.GetOpCode(gate) == opcode;
    });
}

size_t CountLoopHeads(Circuit *circuit)
{
    GateAccessor accessor(circuit);
    std::vector<GateRef> gates;
    accessor.GetAllGates(gates);
    return std::count_if(gates.begin(), gates.end(), [&accessor](GateRef gate) {
        return accessor.IsLoopHead(gate);
    });
}

HWTEST_F_L0(StructuredControlFlowTest, IfElseCreatesMerge)
{
    NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder circuitBuilder(&circuit);
    Environment env(0, &circuitBuilder);
    circuitBuilder.SetEnvironment(&env);
    StructuredControlFlowStubBuilder builder(&env);

    builder.BuildIfElseMerge(circuitBuilder.Int32Equal(circuitBuilder.Int32(0), circuitBuilder.Int32(1)));

    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_GE(CountOpCode(&circuit, OpCode::MERGE), 1U);
    EXPECT_GE(CountOpCode(&circuit, OpCode::VALUE_SELECTOR), 1U);
}

HWTEST_F_L0(StructuredControlFlowTest, IfElseWithTerminatedBranchesDoesNotCreateMerge)
{
    NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder circuitBuilder(&circuit);
    Environment env(0, &circuitBuilder);
    circuitBuilder.SetEnvironment(&env);
    StructuredControlFlowStubBuilder builder(&env);

    builder.BuildTerminatedIfElse(circuitBuilder.Int32Equal(circuitBuilder.Int32(0), circuitBuilder.Int32(1)));

    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(CountOpCode(&circuit, OpCode::MERGE), 0U);
}

HWTEST_F_L0(StructuredControlFlowTest, SingleIfKeepsFalsePathReachable)
{
    NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder circuitBuilder(&circuit);
    Environment env(0, &circuitBuilder);
    circuitBuilder.SetEnvironment(&env);
    StructuredControlFlowStubBuilder builder(&env);

    builder.BuildSingleIf(circuitBuilder.Int32Equal(circuitBuilder.Int32(0), circuitBuilder.Int32(1)));

    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_GE(CountOpCode(&circuit, OpCode::MERGE), 1U);
    EXPECT_GE(CountOpCode(&circuit, OpCode::VALUE_SELECTOR), 1U);
}

HWTEST_F_L0(StructuredControlFlowTest, IfElseWithOneTerminatedBranchKeepsOtherBranchReachable)
{
    NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder circuitBuilder(&circuit);
    Environment env(0, &circuitBuilder);
    circuitBuilder.SetEnvironment(&env);
    StructuredControlFlowStubBuilder builder(&env);

    builder.BuildIfElseWithTerminatedTrueBranch(
        circuitBuilder.Int32Equal(circuitBuilder.Int32(0), circuitBuilder.Int32(1)));

    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(CountOpCode(&circuit, OpCode::RETURN), 2U);
}

HWTEST_F_L0(StructuredControlFlowTest, WhileCreatesLoopBackedge)
{
    NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder circuitBuilder(&circuit);
    Environment env(0, &circuitBuilder);
    circuitBuilder.SetEnvironment(&env);
    StructuredControlFlowStubBuilder builder(&env);

    builder.BuildWhile();

    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(CountLoopHeads(&circuit), 1U);
    EXPECT_GE(CountOpCode(&circuit, OpCode::LOOP_BACK), 1U);
}

HWTEST_F_L0(StructuredControlFlowTest, NestedLoopSupportsBreakAndContinue)
{
    NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder circuitBuilder(&circuit);
    Environment env(0, &circuitBuilder);
    circuitBuilder.SetEnvironment(&env);
    StructuredControlFlowStubBuilder builder(&env);

    builder.BuildNestedLoopWithBreakAndContinue();

    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(CountLoopHeads(&circuit), 2U);
    EXPECT_GE(CountOpCode(&circuit, OpCode::LOOP_BACK), 2U);
}

HWTEST_F_L0(StructuredControlFlowTest, ContinueOnlyLoopCreatesBackedge)
{
    NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder circuitBuilder(&circuit);
    Environment env(0, &circuitBuilder);
    circuitBuilder.SetEnvironment(&env);
    StructuredControlFlowStubBuilder builder(&env);

    builder.BuildContinueOnlyLoop();

    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(CountLoopHeads(&circuit), 1U);
    EXPECT_EQ(CountOpCode(&circuit, OpCode::LOOP_BACK), 1U);
}

HWTEST_F_L0(StructuredControlFlowTest, BreakExitsOuterLoop)
{
    NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder circuitBuilder(&circuit);
    Environment env(0, &circuitBuilder);
    circuitBuilder.SetEnvironment(&env);
    StructuredControlFlowStubBuilder builder(&env);

    builder.BuildLoopWithBreak();

    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(CountLoopHeads(&circuit), 1U);
    EXPECT_EQ(CountOpCode(&circuit, OpCode::LOOP_BACK), 1U);
}
}  // namespace panda::test
