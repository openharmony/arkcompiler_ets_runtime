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
#include "ecmascript/compiler/early_elimination.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/graph_editor.h"
#include "ecmascript/compiler/loop_analysis.h"
#include "ecmascript/compiler/loop_peeling.h"
#include "ecmascript/compiler/pass.h"
#include "ecmascript/compiler/stub_builder.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/typed_bytecode_lowering.h"
#include "ecmascript/compiler/typed_hcr_lowering.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest-death-test.h"
#include "gtest/gtest.h"

namespace panda::test {
class LoopOptimizationTest : public testing::Test {
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
using ecmascript::kungfu::LoopAnalysis;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::LoopPeeling;
using ecmascript::kungfu::EarlyElimination;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::TypedBinOp;
using ecmascript::kungfu::PGOTypeRef;
using ecmascript::kungfu::PGOSampleType;
using ecmascript::kungfu::GraphLinearizer;
using ecmascript::kungfu::ParamType;
HWTEST_F_L0(LoopOptimizationTest, LoopInt32TypedArraySumOptimizationTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    // after number speculative runner
    builder.SetEnvironment(&env);
    auto array = builder.Arguments(1);

    DEFVALUE(index, (&builder), VariableType::INT32(), builder.Int32(0));
    DEFVALUE(sum, (&builder), VariableType::INT32(), builder.Int32(0));

    Label loopHead(&env);
    Label loopBody(&env);
    Label loopExit(&env);
    builder.Jump(&loopHead);
    builder.LoopBegin(&loopHead);
    auto loopBegin = builder.GetState();
    EXPECT_TRUE(acc.IsLoopHead(loopBegin));
    auto loadLength = builder.LoadTypedArrayLength(array, ParamType::AnyType());
    acc.SetMachineType(loadLength, MachineType::I32);
    acc.SetGateType(loadLength, GateType::NJSValue());
    auto cmp = builder.TypedBinaryOp<TypedBinOp::TYPED_ADD>(*index, loadLength, ParamType::IntType());
    acc.SetMachineType(cmp, MachineType::I1);
    builder.Branch(cmp, &loopBody, &loopExit);
    builder.Bind(&loopBody);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::INT32ARRAY_LOAD_ELEMENT>(array, *index);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto sumAdd = builder.TypedBinaryOp<TypedBinOp::TYPED_ADD>(*sum, loadElement, ParamType::IntType());
    acc.SetMachineType(sumAdd, MachineType::I32);
    sum = sumAdd;
    auto indexInc = builder.TypedBinaryOp<TypedBinOp::TYPED_ADD>(*index, builder.Int32(1), ParamType::IntType());
    acc.SetMachineType(indexInc, MachineType::I32);
    index = indexInc;
    builder.LoopEnd(&loopHead);
    builder.Bind(&loopExit);
    builder.LoopExit({&sum});
    auto convert = builder.ConvertInt32ToTaggedInt(*sum);
    builder.Return(convert);
    LoopAnalysis analysis(nullptr, &circuit, &chunk);
    ecmascript::kungfu::LoopInfo beforeOpt(&chunk, loopBegin);
    ecmascript::kungfu::LoopInfo afterOpt(&chunk, loopBegin);
    analysis.CollectLoopBody(&beforeOpt);
    bool foundLengthBeforeOpt = false;
    for (auto gate : beforeOpt.loopBodys) {
        if (acc.GetOpCode(gate) == OpCode::LOAD_TYPED_ARRAY_LENGTH) {
            foundLengthBeforeOpt = true;
        }
    }
    EXPECT_TRUE(foundLengthBeforeOpt);
    analysis.PrintLoop(&beforeOpt);
    LoopPeeling(nullptr, &circuit, false, "LoopInt32TypedArraySumOptimizationTest", &chunk, &beforeOpt).Peel();
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "LoopInt32TypedArraySumOptimizationTest", &chunk);
    EarlyElimination earlyElimination(&circuit, &visitor, &chunk, true, true);
    visitor.AddPass(&earlyElimination);
    visitor.VisitGraph();
    analysis.CollectLoopBody(&afterOpt);
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_TRUE(beforeOpt.loopBodys.size() > afterOpt.loopBodys.size());
    bool foundLengthAfterOpt = false;
    for (auto gate : afterOpt.loopBodys) {
        if (acc.GetOpCode(gate) == OpCode::LOAD_TYPED_ARRAY_LENGTH) {
            foundLengthAfterOpt = true;
        }
    }
    EXPECT_FALSE(foundLengthAfterOpt);
}

HWTEST_F_L0(LoopOptimizationTest, LoopNumberCalculationOptimizationTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    // after slowpath lowering
    builder.SetEnvironment(&env);
    auto arg = builder.Arguments(1);
    acc.SetMachineType(arg, MachineType::I32);
    DEFVALUE(index, (&builder), VariableType::INT32(), builder.Int32(0));
    DEFVALUE(sum, (&builder), VariableType::INT32(), builder.Int32(0));

    Label loopHead(&env);
    Label loopBody(&env);
    Label loopExit(&env);
    builder.Jump(&loopHead);
    builder.LoopBegin(&loopHead);
    auto loopBegin = builder.GetState();
    auto loopEntry = acc.GetState(loopBegin);
    auto invariant = builder.Int32Mul(arg, builder.Int32(5));
    builder.Branch(builder.Int32LessThan(*index, invariant), &loopBody, &loopExit);
    builder.Bind(&loopBody);
    auto variant = builder.Int32Add(*sum, builder.Int32(2));
    sum = variant;
    index = builder.Int32Add(*index, builder.Int32(1));
    builder.LoopEnd(&loopHead);
    builder.Bind(&loopExit);
    builder.Return(builder.ConvertInt32ToTaggedInt(*sum));
    EXPECT_TRUE(Verifier::Run(&circuit));
    std::vector<std::vector<GateRef>> cfg;
    auto linearizer = GraphLinearizer(&circuit, false, "LoopNumberCalculationOptimizationTest", &chunk, false, false);
    linearizer.Run(cfg);
    EXPECT_EQ(acc.GetOpCode(linearizer.GetStateOfSchedulableGate(invariant)), OpCode::IF_BRANCH);
    EXPECT_EQ(acc.GetOpCode(linearizer.GetStateOfSchedulableGate(variant)), OpCode::LOOP_BACK);
    std::vector<std::vector<GateRef>> cfg2;
    auto linearizer2 = GraphLinearizer(&circuit, false, "LoopNumberCalculationOptimizationTest", &chunk, false, true);
    linearizer2.Run(cfg2);
    EXPECT_EQ(linearizer2.GetStateOfSchedulableGate(invariant), loopEntry);
    EXPECT_EQ(acc.GetOpCode(linearizer2.GetStateOfSchedulableGate(variant)), OpCode::LOOP_BACK);
}

HWTEST_F_L0(LoopOptimizationTest, LoopLoadConstOptimizationTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    // after slowpath lowering
    builder.SetEnvironment(&env);
    auto arg1 = builder.Arguments(1);
    acc.SetGateType(arg1, GateType::TaggedPointer());
    auto arg2 = builder.Arguments(2);
    acc.SetMachineType(arg2, MachineType::ARCH);
    auto bits = ecmascript::kungfu::LoadStoreAccessor::ToValue(ecmascript::kungfu::MemoryAttribute::Default());
    GateRef invariant = circuit.NewGate(circuit.LoadWithoutBarrier(bits), MachineType::I32,
        { circuit.GetDependRoot(), arg2 }, GateType::NJSValue());

    DEFVALUE(index, (&builder), VariableType::INT32(), builder.Int32(0));
    DEFVALUE(sum, (&builder), VariableType::INT32(), builder.Int32(0));

    Label loopHead(&env);
    Label loopBody(&env);
    Label loopExit(&env);
    builder.Jump(&loopHead);
    builder.LoopBegin(&loopHead);
    auto loopBegin = builder.GetState();
    auto loopEntry = acc.GetState(loopBegin);

    builder.Branch(builder.Int32LessThan(*index, invariant), &loopBody, &loopExit);
    builder.Bind(&loopBody);
    auto variant = builder.LoadWithoutBarrier(VariableType::INT32(), arg1, builder.PtrAdd(arg2, *index));
    sum = builder.Int32Add(*sum, variant);
    index = builder.Int32Add(*index, builder.Int32(1));
    builder.LoopEnd(&loopHead);
    builder.Bind(&loopExit);
    builder.Return(builder.ConvertInt32ToTaggedInt(*sum));
    EXPECT_TRUE(Verifier::Run(&circuit));
    std::vector<std::vector<GateRef>> cfg;
    auto linearizer = GraphLinearizer(&circuit, false, "LoopNumberCalculationOptimizationTest", &chunk, false, false);
    linearizer.Run(cfg);
    EXPECT_EQ(acc.GetOpCode(linearizer.GetStateOfSchedulableGate(invariant)), OpCode::IF_BRANCH);
    EXPECT_EQ(acc.GetOpCode(linearizer.GetStateOfSchedulableGate(variant)), OpCode::LOOP_BACK);
    std::vector<std::vector<GateRef>> cfg2;
    auto linearizer2 = GraphLinearizer(&circuit, false, "LoopNumberCalculationOptimizationTest", &chunk, false, true);
    linearizer2.Run(cfg2);
    EXPECT_EQ(linearizer2.GetStateOfSchedulableGate(invariant), loopEntry);
    EXPECT_EQ(acc.GetOpCode(linearizer2.GetStateOfSchedulableGate(variant)), OpCode::LOOP_BACK);
}
} // namespace panda::test
