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
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/ts_hcr_lowering.h"
#include "ecmascript/compiler/type_mcr_lowering.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/pgo_profiler/pgo_profiler_type.h"
#include "ecmascript/tests/test_helper.h"

namespace panda::test {
class LoweringRelateGateTests : public testing::Test {
};
using ecmascript::GlobalEnvConstants;
using ecmascript::ConstantIndex;
using ecmascript::RegionSpaceFlag;
using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::GateAccessor;
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
using ecmascript::kungfu::TypeMCRLowering;
using ecmascript::kungfu::TSHCRLowering;
using ecmascript::kungfu::CompilationConfig;

HWTEST_F_L0(LoweringRelateGateTests, TypeCheckFramework)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder builder(&circuit);
    GateAccessor acc(&circuit);
    Environment env(1, &builder);
    builder.SetEnvironment(&env);
    auto depend = acc.GetDependRoot();
    auto state = acc.GetStateRoot();
    auto arg0 = builder.Arguments(0);
    auto pcGate = circuit.GetConstantGate(MachineType::I64, 0, GateType::NJSValue());
    auto frameArgs = circuit.NewGate(
        circuit.FrameArgs(), {builder.Arguments(3), builder.Arguments(4), builder.Arguments(5), builder.Arguments(2)});
    auto frameState = circuit.NewGate(circuit.FrameState(1), {pcGate, frameArgs});
    auto stateSplit = circuit.NewGate(circuit.StateSplit(), {state, depend, frameState});
    builder.SetDepend(stateSplit);
    auto check = builder.TryPrimitiveTypeCheck(GateType::NumberType(), arg0);
    builder.ReturnVoid(check, depend);

    CompilationConfig config(TARGET_X64);
    TypeMCRLowering typeMCRLowering(&circuit, &config, nullptr, false, "TypeCheckFramework");
    typeMCRLowering.RunTypeMCRLowering();
    EXPECT_TRUE(Verifier::Run(&circuit));
}

HWTEST_F_L0(LoweringRelateGateTests, TypeConvertFramework)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder builder(&circuit);
    GateAccessor acc(&circuit);
    auto entry = acc.GetStateRoot();
    auto depend = acc.GetDependRoot();
    auto arg0 = builder.Arguments(0);
    auto convert = builder.TypeConvert(MachineType::I64, GateType::NJSValue(), GateType::NumberType(),
                                       {entry, depend, arg0});
    builder.Return(convert, convert, convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CompilationConfig config(TARGET_X64);
    TypeMCRLowering typeMCRLowering(&circuit, &config, nullptr, false, "TypeConvertFramework");
    typeMCRLowering.RunTypeMCRLowering();
    EXPECT_TRUE(Verifier::Run(&circuit));
}

HWTEST_F_L0(LoweringRelateGateTests, HeapAllocTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);
    auto glue = builder.Arguments(0);
    auto arg0 = builder.Arguments(1);
    auto arg1 = builder.Arguments(2);
    auto array = builder.HeapAlloc(arg0, GateType::AnyType(), RegionSpaceFlag::IN_YOUNG_SPACE);

    auto offset = builder.Int64(JSThread::GlueData::GetGlobalConstOffset(false));
    auto globalEnv = builder.Load(VariableType::JS_POINTER(), glue, offset);
    auto lenthOffset = builder.IntPtr(GlobalEnvConstants::GetOffsetOfLengthString());
    auto lengthString = builder.Load(VariableType::JS_POINTER(), globalEnv, lenthOffset);

    builder.Store(VariableType::JS_POINTER(), glue, array, builder.IntPtr(0), arg1);
    builder.StoreElement<ecmascript::kungfu::TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(array, builder.IntPtr(0),
        builder.ToTaggedInt(builder.Int64(0)), builder.Undefined());
    builder.StoreElement<ecmascript::kungfu::TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(array, builder.IntPtr(1),
        builder.ToTaggedInt(builder.Int64(1)), builder.Undefined());
    builder.StoreProperty(array, lengthString, builder.ToTaggedInt(builder.Int64(2)));
    auto length = builder.LoadProperty(array, lengthString, false);
    Label less2(&builder);
    Label notLess2(&builder);
    auto condtion = builder.TaggedIsTrue(builder.TypedBinaryOp<TypedBinOp::TYPED_LESS>(length,
        builder.ToTaggedInt(builder.Int64(2))), GateType::NumberType(), GateType::NumberType(),
        GateType::AnyType(), PGOSampleType::NoneType());
    builder.Branch(condtion, &less2, &notLess2);
    builder.Bind(&less2);
    auto ret = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT>(array, builder.IntPtr(1),
                                                                                               builder.Undefined());
    builder.Return(ret);
    builder.Bind(&notLess2);
    builder.Return(builder.Int64(-1));
    EXPECT_TRUE(Verifier::Run(&circuit));
}
} // namespace panda::test
