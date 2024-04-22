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
#include "ecmascript/compiler/share_opcodes.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/typed_bytecode_lowering.h"
#include "ecmascript/compiler/typed_hcr_lowering.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "ecmascript/elements.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest.h"
#include <cstddef>

namespace panda::test {
class MetaDataEqualTests : public testing::Test {};

using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::EcmaOpcode;
using ecmascript::kungfu::ElementsKind;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::GateMetaData;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::JSBytecodeMetaData;
using ecmascript::kungfu::MachineType;
using ecmascript::kungfu::PGOTypeRef;
using ecmascript::kungfu::PGOSampleType;
using ecmascript::kungfu::TypedBinOp;
using ecmascript::kungfu::TypedCallTargetCheckOp;
using ecmascript::kungfu::ParamType;
using ecmascript::kungfu::TypedCallTargetCheckAccessor;

HWTEST_F_L0(MetaDataEqualTests, StringMetaDataEqualTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    auto stringGate1 = circuit.GetConstantStringGate(MachineType::ARCH, "test1", GateType::NJSValue());


    auto stringGate2 = circuit.GetConstantStringGate(MachineType::ARCH, "test2", GateType::NJSValue());

    EXPECT_FALSE(acc.MetaDataValueEqu(stringGate1, stringGate2));

    auto stringGate3 = circuit.GetConstantStringGate(MachineType::ARCH, "test1", GateType::NJSValue());

    EXPECT_TRUE(acc.MetaDataValueEqu(stringGate1, stringGate3));
}

HWTEST_F_L0(MetaDataEqualTests, ConstantMetaDataEqualTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);


    auto constantValue1 = circuit.GetConstantGate(MachineType::I64, 2, GateType::NJSValue());

    auto constantValue2 = circuit.GetConstantGate(MachineType::I64, 2, GateType::NJSValue());

    EXPECT_TRUE(acc.MetaDataValueEqu(constantValue1, constantValue2));

    auto constantValue3 = circuit.GetConstantGate(MachineType::I64, 2, GateType::NJSValue());

    auto constantValue4 = circuit.GetConstantGate(MachineType::I64, 3, GateType::NJSValue());

    EXPECT_FALSE(acc.MetaDataValueEqu(constantValue3, constantValue4));


    // MetaData is equal, but Gate is not equal
    auto constantValue5 = circuit.GetConstantGate(MachineType::I64, 2, GateType::NJSValue());

    auto constantValue6 = circuit.GetConstantGate(MachineType::I32, 2, GateType::NJSValue());

    EXPECT_TRUE(acc.MetaDataValueEqu(constantValue5, constantValue6));

    // MetaData is equal, but Gate is not equal
    auto ConstGateNJSValue2 = circuit.GetConstantGate(MachineType::I64, 2, GateType::NJSValue());

    auto ConstGateUndefined =
        circuit.GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, GateType::UndefinedType());

    EXPECT_TRUE(acc.MetaDataValueEqu(ConstGateNJSValue2, ConstGateUndefined));
}

HWTEST_F_L0(MetaDataEqualTests, TypeErrorMetaDataEqualTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    auto constantValue1 = circuit.GetConstantGate(MachineType::I64, 2, GateType::NJSValue());
    auto stringGate1 = circuit.GetConstantStringGate(MachineType::ARCH, "test1", GateType::NJSValue());
    EXPECT_FALSE(acc.MetaDataValueEqu(constantValue1, stringGate1));
    EXPECT_FALSE(acc.MetaDataValueEqu(stringGate1, constantValue1));
}

HWTEST_F_L0(MetaDataEqualTests, HCRMetaDataEqualTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    // JSBytecodeMetaData
    auto meta = circuit.JSBytecode(0, 0, EcmaOpcode::JEQZ_IMM8, 0, 0, true, false);
    auto gate =
        circuit.NewGate(meta, MachineType::I64, {Circuit::NullGate(), Circuit::NullGate()}, GateType::AnyType());
    auto meta2 = circuit.JSBytecode(0, 0, EcmaOpcode::JEQZ_IMM8, 0, 0, true, false);
    auto gate2 =
        circuit.NewGate(meta2, MachineType::I64, {Circuit::NullGate(), Circuit::NullGate()}, GateType::AnyType());

    EXPECT_TRUE(acc.MetaDataValueEqu(gate, gate2));

    static_cast<JSBytecodeMetaData *>(const_cast<GateMetaData *>(meta))->SetElementsKind(ElementsKind::NUMBER);
    static_cast<JSBytecodeMetaData *>(const_cast<GateMetaData *>(meta2))->SetElementsKind(ElementsKind::NUMBER);
    EXPECT_TRUE(acc.MetaDataValueEqu(gate, gate2));

    PGOSampleType type = PGOSampleType::CreateProfileType(0, 0);
    PGOSampleType type2 = PGOSampleType::CreateProfileType(0, 0);
    static_cast<JSBytecodeMetaData *>(const_cast<GateMetaData *>(meta))
        ->SetType(PGOTypeRef(static_cast<const PGOSampleType *>(&type)));
    static_cast<JSBytecodeMetaData *>(const_cast<GateMetaData *>(meta2))
        ->SetType(PGOTypeRef(static_cast<const PGOSampleType *>(&type2)));
    EXPECT_FALSE(acc.MetaDataValueEqu(gate, gate2));

    PGOSampleType type3 = PGOSampleType::CreateProfileType(0, 1);
    static_cast<JSBytecodeMetaData *>(const_cast<GateMetaData *>(meta))
        ->SetType(PGOTypeRef(static_cast<const PGOSampleType *>(&type)));
    static_cast<JSBytecodeMetaData *>(const_cast<GateMetaData *>(meta2))
        ->SetType(PGOTypeRef(static_cast<const PGOSampleType *>(&type3)));
    EXPECT_FALSE(acc.MetaDataValueEqu(gate, gate2));

    static_cast<JSBytecodeMetaData *>(const_cast<GateMetaData *>(meta))->SetElementsKind(ElementsKind::NUMBER);
    static_cast<JSBytecodeMetaData *>(const_cast<GateMetaData *>(meta2))->SetElementsKind(ElementsKind::HOLE_NUMBER);
    EXPECT_FALSE(acc.MetaDataValueEqu(gate, gate2));
}

HWTEST_F_L0(MetaDataEqualTests, MCRMetaDataEqualTest)
{
    // construct a circuit
    ecmascript::NativeAreaAllocator allocator;
    Circuit circuit(&allocator);
    ecmascript::Chunk chunk(&allocator);
    GateAccessor acc(&circuit);
    CircuitBuilder builder(&circuit);
    Environment env(0, &builder);
    builder.SetEnvironment(&env);

    // TypedCallMetaData
    auto callGate =
        circuit.NewGate(circuit.TypedCall(0, 0, true), MachineType::I64,
                        {Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate()}, GateType::AnyType());

    auto callGate2 =
        circuit.NewGate(circuit.TypedCall(0, 0, true), MachineType::I64,
                        {Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate()}, GateType::AnyType());

    EXPECT_TRUE(acc.MetaDataValueEqu(callGate, callGate2));
    EXPECT_TRUE(acc.MetaDataValueEqu(callGate2, callGate));

    // TypedCallTargetCheckMetaData
    auto callGate3 =
        circuit.NewGate(circuit.TypedCallTargetCheckOp(
            TypedCallTargetCheckAccessor::ToValue(TypedCallTargetCheckOp::JSCALLTHIS_FAST)), MachineType::I64,
            {Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate()},
            GateType::AnyType());
    auto callGate4 =
        circuit.NewGate(circuit.TypedCallTargetCheckOp(
            TypedCallTargetCheckAccessor::ToValue(TypedCallTargetCheckOp::JSCALLTHIS_FAST)), MachineType::I64,
            {Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate()},
            GateType::AnyType());

    EXPECT_TRUE(acc.MetaDataValueEqu(callGate3, callGate4));
    EXPECT_TRUE(acc.MetaDataValueEqu(callGate4, callGate3));

    // TypedBinaryMetaData
    uint64_t valueForType5 = ecmascript::kungfu::TypedBinaryAccessor::ToValue(ParamType::IntType(),
                                                                              TypedBinOp::TYPED_ADD);
    auto callGate5 = circuit.NewGate(
        circuit.TypedBinaryOp(valueForType5),
        MachineType::I64, { Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate() },
        GateType::AnyType());

    // TypedBinaryMetaData
    uint64_t valueForType6 = ecmascript::kungfu::TypedBinaryAccessor::ToValue(ParamType::IntType(),
                                                                              TypedBinOp::TYPED_ADD);
    auto callGate6 = circuit.NewGate(
        circuit.TypedBinaryOp(valueForType6),
        MachineType::I64, { Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate(), Circuit::NullGate() },
        GateType::AnyType());

    EXPECT_TRUE(acc.MetaDataValueEqu(callGate5, callGate6));
    EXPECT_TRUE(acc.MetaDataValueEqu(callGate6, callGate5));
}


} // namespace panda::test
