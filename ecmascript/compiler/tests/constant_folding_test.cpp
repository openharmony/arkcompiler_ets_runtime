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
#include "ecmascript/compiler/constant_folding.h"
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
} // namespace panda::test