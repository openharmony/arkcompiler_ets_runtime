/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/ts_type_lowering.h"
#include "ecmascript/compiler/type_lowering.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
class LoweringRelateGateTests : public testing::Test {
};
using ecmascript::GlobalEnvConstants;
using ecmascript::ConstantIndex;
using ecmascript::RegionSpaceFlag;
using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::MachineType;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::Verifier;
using ecmascript::kungfu::TypedBinOp;
using ecmascript::kungfu::Label;
using ecmascript::kungfu::Variable;
using ecmascript::kungfu::VariableType;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::TypeLowering;
using ecmascript::kungfu::TSTypeLowering;
using ecmascript::kungfu::CompilationConfig;

HWTEST_F_L0(LoweringRelateGateTests, TypeCheckFramework)
{
    // construct a circuit
    Circuit circuit;
    CircuitBuilder builder(&circuit);
    auto entry = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
    auto depend = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
    auto arg0 = builder.Arguments(0);
    auto check = builder.TypeCheck(GateType::NumberType(), arg0);
    builder.Return(entry, depend, check);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CompilationConfig config("x86_64-unknown-linux-gnu", false);
    TypeLowering typeLowering(nullptr, &circuit, &config, nullptr, false, "TypeCheckFramework");
    typeLowering.RunTypeLowering();
    EXPECT_TRUE(Verifier::Run(&circuit));
}

HWTEST_F_L0(LoweringRelateGateTests, TypedBinaryOperatorAddFramework)
{
    // construct a circuit
    Circuit circuit;
    CircuitBuilder builder(&circuit);
    auto entry = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
    auto depend = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
    auto arg0 = builder.Arguments(0);
    auto arg1 = builder.Arguments(1);
    auto nadd = builder.TypedBinaryOperator(MachineType::I64, TypedBinOp::TYPED_ADD,
                                            GateType::NumberType(), GateType::NumberType(),
                                            {entry, depend, arg0, arg1}, GateType::NumberType());
    builder.Return(nadd, nadd, nadd);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CompilationConfig config("x86_64-unknown-linux-gnu", false);
    TypeLowering typeLowering(nullptr, &circuit, &config, nullptr, false, "TypedBinaryOperatorAddFramework");
    typeLowering.RunTypeLowering();
    EXPECT_TRUE(Verifier::Run(&circuit));
}

HWTEST_F_L0(LoweringRelateGateTests, TypedBinaryOperatorLessFramework)
{
    // construct a circuit
    Circuit circuit;
    CircuitBuilder builder(&circuit);
    auto entry = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
    auto depend = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
    auto arg0 = builder.Arguments(0);
    auto arg1 = builder.Arguments(1);
    auto nless = builder.TypedBinaryOperator(MachineType::I64, TypedBinOp::TYPED_LESS,
                                            GateType::NumberType(), GateType::NumberType(),
                                            {entry, depend, arg0, arg1}, GateType::BooleanType());
    builder.Return(nless, nless, nless);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CompilationConfig config("x86_64-unknown-linux-gnu", false);
    TypeLowering typeLowering(nullptr, &circuit, &config, nullptr, false, "TypedBinaryOperatorLessFramework");
    typeLowering.RunTypeLowering();
    EXPECT_TRUE(Verifier::Run(&circuit));
}

HWTEST_F_L0(LoweringRelateGateTests, TypeConvertFramework)
{
    // construct a circuit
    Circuit circuit;
    CircuitBuilder builder(&circuit);
    auto entry = Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY));
    auto depend = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
    auto arg0 = builder.Arguments(0);
    auto convert = builder.TypeConvert(MachineType::I64, GateType::NJSValue(), GateType::NumberType(),
                                       {entry, depend, arg0});
    builder.Return(convert, convert, convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CompilationConfig config("x86_64-unknown-linux-gnu", false);
    TypeLowering typeLowering(nullptr, &circuit, &config, nullptr, false, "TypeConvertFramework");
    typeLowering.RunTypeLowering();
    EXPECT_TRUE(Verifier::Run(&circuit));
}

HWTEST_F_L0(LoweringRelateGateTests, TypeOpCodeFramework)
{
    // construct a circuit
    Circuit circuit;
    CircuitBuilder builder(&circuit);
    Environment env(2, &builder);
    builder.SetEnvironment(&env);
    Label isNumber(&builder);
    Label notNumber(&builder);
    Label exit(&builder);
    VariableType arg1Type(MachineType::I64, GateType::BooleanType());
    CompilationConfig config("x86_64-unknown-linux-gnu", false);
    TypeLowering typeLowering(nullptr, &circuit, &config, nullptr, false, "TypeOpCodeFramework");

    auto arg0 = builder.Arguments(0);
    auto arg1 = builder.Arguments(1);

    DEFVAlUE(result, (&builder), VariableType::JS_ANY(), builder.Int32ToTaggedPtr(builder.Int32(1)));
    builder.Branch(builder.TypeCheck(GateType::NumberType(), arg0), &isNumber, &notNumber);
    builder.Bind(&isNumber);
    auto convert = builder.PrimitiveToNumber(arg1, arg1Type);
    result = builder.NumberBinaryOp<TypedBinOp::TYPED_ADD>(arg0, convert);
    builder.Jump(&exit);
    builder.Bind(&notNumber);
    builder.Jump(&exit);
    builder.Bind(&exit);
    builder.Return(*result);
    EXPECT_TRUE(Verifier::Run(&circuit));
    typeLowering.RunTypeLowering();
    EXPECT_TRUE(Verifier::Run(&circuit));
}

HWTEST_F_L0(LoweringRelateGateTests, HeapAllocTest)
{
    // construct a circuit
    Circuit circuit;
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    auto glue = builder.Arguments(0);
    auto arg0 = builder.Arguments(1);
    auto arg1 = builder.Arguments(2);
    auto array = builder.HeapAlloc(arg0, GateType::AnyType(), RegionSpaceFlag::IN_YOUNG_SPACE);

    auto offset = builder.Int64(JSThread::GlueData::GetGlueGlobalConstOffset(false));
    auto globalEnv = builder.Load(VariableType::JS_POINTER(), glue, offset);
    auto lenthOffset = builder.IntPtr(GlobalEnvConstants::GetOffsetOfLengthString());
    auto lengthString = builder.Load(VariableType::JS_POINTER(), globalEnv, lenthOffset);

    builder.Store(VariableType::JS_POINTER(), glue, array, builder.IntPtr(0), arg1);
    builder.StoreElement(array, builder.IntPtr(0), builder.ToTaggedInt(builder.Int64(0)));
    builder.StoreElement(array, builder.IntPtr(1), builder.ToTaggedInt(builder.Int64(1)));
    builder.StoreProperty(array, lengthString, builder.ToTaggedInt(builder.Int64(2)));
    auto length = builder.LoadProperty(array, lengthString);
    Label less2(&builder);
    Label notLess2(&builder);
    auto condtion = builder.TaggedIsTrue(builder.NumberBinaryOp<TypedBinOp::TYPED_LESS>(length,
        builder.ToTaggedInt(builder.Int64(2))));
    builder.Branch(condtion, &less2, &notLess2);
    builder.Bind(&less2);
    auto ret = builder.LoadElement(array,  builder.IntPtr(1));
    builder.Return(ret);
    builder.Bind(&notLess2);
    builder.Return(builder.Int64(-1));
    EXPECT_TRUE(Verifier::Run(&circuit));
}
} // namespace panda::test