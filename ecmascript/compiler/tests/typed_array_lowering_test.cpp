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
#include "ecmascript/compiler/type.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/compiler/typed_bytecode_lowering.h"
#include "ecmascript/compiler/typed_hcr_lowering.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest-death-test.h"
#include "gtest/gtest.h"

namespace panda::test {
class TypedArrayLoweringTests : public testing::Test {
};
using ecmascript::kungfu::Circuit;
using ecmascript::kungfu::GateAccessor;
using ecmascript::kungfu::OpCode;
using ecmascript::kungfu::GateType;
using ecmascript::kungfu::MachineType;
using ecmascript::kungfu::CircuitBuilder;
using ecmascript::kungfu::Verifier;
using ecmascript::kungfu::Environment;
using ecmascript::kungfu::CombinedPassVisitor;
using ecmascript::kungfu::TypedHCRLowering;
using ecmascript::OnHeapMode;
using ecmascript::kungfu::ParamType;

HWTEST_F_L0(TypedArrayLoweringTests, LoadTypedArrayLength)
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
    auto loadLength = builder.LoadTypedArrayLength(array, ParamType::AnyType());
    acc.SetMachineType(loadLength, MachineType::I32);
    acc.SetGateType(loadLength, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadLength);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "LoadTypedArrayLength", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadLength), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::LOAD_CONST_OFFSET);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::I32);
}

HWTEST_F_L0(TypedArrayLoweringTests, Int32ArrayLoadElement)
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
    auto index = builder.Int32(1);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::INT32ARRAY_LOAD_ELEMENT>(array, index);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadElement);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Int32ArrayLoadElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadElement), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::VALUE_SELECTOR);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::I32);
    auto numIn = acc.GetNumValueIn(result);
    EXPECT_EQ(numIn, 2);
    for (size_t i = 0; i < numIn; ++i) {
        EXPECT_EQ(acc.GetOpCode(acc.GetValueIn(result, i)), OpCode::LOAD);
    }
}

HWTEST_F_L0(TypedArrayLoweringTests, Int32OnHeapArrayLoadElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::INT32ARRAY_LOAD_ELEMENT>(array, index,
        OnHeapMode::ON_HEAP);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadElement);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Int32OnHeapArrayLoadElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadElement), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::LOAD);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::I32);
}

HWTEST_F_L0(TypedArrayLoweringTests, Float64OnHeapArrayLoadElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::FLOAT64ARRAY_LOAD_ELEMENT>(array, index,
        OnHeapMode::ON_HEAP);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadElement);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Float64OnHeapArrayLoadElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadElement), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::LOAD);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::F64);
}

HWTEST_F_L0(TypedArrayLoweringTests, FLOAT32OnHeapArrayLoadElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT>(array,
        index, OnHeapMode::ON_HEAP);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadElement);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "FLOAT32OnHeapArrayLoadElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadElement), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::FEXT);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::F64);
    auto load = acc.GetValueIn(result, 0);
    EXPECT_EQ(acc.GetOpCode(load), OpCode::LOAD);
    EXPECT_EQ(acc.GetMachineType(load), MachineType::F32);
}

HWTEST_F_L0(TypedArrayLoweringTests, Int8OnHeapArrayLoadElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::INT8ARRAY_LOAD_ELEMENT>(array, index,
        OnHeapMode::ON_HEAP);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadElement);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Int8OnHeapArrayLoadElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadElement), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::SEXT);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::I32);
    auto load = acc.GetValueIn(result, 0);
    EXPECT_EQ(acc.GetOpCode(load), OpCode::LOAD);
    EXPECT_EQ(acc.GetMachineType(load), MachineType::I8);
}

HWTEST_F_L0(TypedArrayLoweringTests, UInt8OnHeapArrayLoadElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::UINT8ARRAY_LOAD_ELEMENT>(array, index,
        OnHeapMode::ON_HEAP);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadElement);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "UInt8OnHeapArrayLoadElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadElement), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::ZEXT);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::I32);
    auto load = acc.GetValueIn(result, 0);
    EXPECT_EQ(acc.GetOpCode(load), OpCode::LOAD);
    EXPECT_EQ(acc.GetMachineType(load), MachineType::I8);
}

HWTEST_F_L0(TypedArrayLoweringTests, Int16OnHeapArrayLoadElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::INT16ARRAY_LOAD_ELEMENT>(array, index,
        OnHeapMode::ON_HEAP);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadElement);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Int16OnHeapArrayLoadElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadElement), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::SEXT);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::I32);
    auto load = acc.GetValueIn(result, 0);
    EXPECT_EQ(acc.GetOpCode(load), OpCode::LOAD);
    EXPECT_EQ(acc.GetMachineType(load), MachineType::I16);
}

HWTEST_F_L0(TypedArrayLoweringTests, UInt16OnHeapArrayLoadElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto loadElement = builder.LoadElement<ecmascript::kungfu::TypedLoadOp::UINT16ARRAY_LOAD_ELEMENT>(array, index,
        OnHeapMode::ON_HEAP);
    acc.SetMachineType(loadElement, MachineType::I32);
    acc.SetGateType(loadElement, GateType::NJSValue());
    auto convert = builder.ConvertInt32ToTaggedInt(loadElement);
    builder.Return(convert);
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "UInt16OnHeapArrayLoadElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(loadElement), OpCode::NOP);
    auto result = acc.GetValueIn(convert, 0);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::ZEXT);
    EXPECT_EQ(acc.GetMachineType(result), MachineType::I32);
    auto load = acc.GetValueIn(result, 0);
    EXPECT_EQ(acc.GetOpCode(load), OpCode::LOAD);
    EXPECT_EQ(acc.GetMachineType(load), MachineType::I16);
}

HWTEST_F_L0(TypedArrayLoweringTests, Int32ArrayStoreElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto value = builder.Int32(2);
    auto storeElement =
        builder.StoreElement<ecmascript::kungfu::TypedStoreOp::INT32ARRAY_STORE_ELEMENT>(array, index, value);
    auto ret = builder.Return(builder.Undefined());
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Int32ArrayStoreElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(storeElement), OpCode::NOP);
    auto result = acc.GetDep(ret);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::DEPEND_SELECTOR);
    auto numIn = acc.GetDependCount(result);
    EXPECT_EQ(numIn, 2);
    for (size_t i = 0; i < numIn; ++i) {
        auto input = acc.GetDep(result, i);
        EXPECT_EQ(acc.GetOpCode(input), OpCode::STORE_MEMORY);
        EXPECT_EQ(value, acc.GetValueIn(input, 2));
    }
}

HWTEST_F_L0(TypedArrayLoweringTests, Int32OnHeapArrayStoreElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto value = builder.Int32(2);
    auto storeElement =
        builder.StoreElement<ecmascript::kungfu::TypedStoreOp::INT32ARRAY_STORE_ELEMENT>(array, index, value,
                                                                                         OnHeapMode::ON_HEAP);
    auto ret = builder.Return(builder.Undefined());
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Int32OnHeapArrayStoreElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(storeElement), OpCode::NOP);
    auto result = acc.GetDep(ret);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::STORE_MEMORY);
    auto storeValue = acc.GetValueIn(result, 2);
    EXPECT_EQ(value, storeValue);
}

HWTEST_F_L0(TypedArrayLoweringTests, Float64OnHeapArrayStoreElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto value = builder.Double(1.5);
    auto storeElement =
        builder.StoreElement<ecmascript::kungfu::TypedStoreOp::FLOAT64ARRAY_STORE_ELEMENT>(array, index, value,
                                                                                         OnHeapMode::ON_HEAP);
    auto ret = builder.Return(builder.Undefined());
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Float64OnHeapArrayStoreElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(storeElement), OpCode::NOP);
    auto result = acc.GetDep(ret);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::STORE_MEMORY);
    auto storeValue = acc.GetValueIn(result, 2);
    EXPECT_EQ(value, storeValue);
}

HWTEST_F_L0(TypedArrayLoweringTests, Int8OnHeapArrayStoreElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto value = builder.Int32(2);
    auto storeElement =
        builder.StoreElement<ecmascript::kungfu::TypedStoreOp::INT8ARRAY_STORE_ELEMENT>(array, index, value,
                                                                                         OnHeapMode::ON_HEAP);
    auto ret = builder.Return(builder.Undefined());
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Int8OnHeapArrayStoreElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(storeElement), OpCode::NOP);
    auto result = acc.GetDep(ret);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::STORE_MEMORY);
    auto storeValue = acc.GetValueIn(result, 2);
    EXPECT_EQ(value, acc.GetValueIn(storeValue));
    EXPECT_EQ(acc.GetOpCode(storeValue), OpCode::TRUNC);
    EXPECT_EQ(acc.GetMachineType(storeValue), MachineType::I8);
}

HWTEST_F_L0(TypedArrayLoweringTests, UInt8OnHeapArrayStoreElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto value = builder.Int32(2);
    auto storeElement =
        builder.StoreElement<ecmascript::kungfu::TypedStoreOp::UINT8ARRAY_STORE_ELEMENT>(array, index, value,
                                                                                         OnHeapMode::ON_HEAP);
    auto ret = builder.Return(builder.Undefined());
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "UInt8OnHeapArrayStoreElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(storeElement), OpCode::NOP);
    auto result = acc.GetDep(ret);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::STORE_MEMORY);
    auto storeValue = acc.GetValueIn(result, 2);
    EXPECT_EQ(value, acc.GetValueIn(storeValue));
    EXPECT_EQ(acc.GetOpCode(storeValue), OpCode::TRUNC);
    EXPECT_EQ(acc.GetMachineType(storeValue), MachineType::I8);
}

HWTEST_F_L0(TypedArrayLoweringTests, Int16OnHeapArrayStoreElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto value = builder.Int32(2);
    auto storeElement =
        builder.StoreElement<ecmascript::kungfu::TypedStoreOp::INT16ARRAY_STORE_ELEMENT>(array, index, value,
                                                                                         OnHeapMode::ON_HEAP);
    auto ret = builder.Return(builder.Undefined());
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Int16OnHeapArrayStoreElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(storeElement), OpCode::NOP);
    auto result = acc.GetDep(ret);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::STORE_MEMORY);
    auto storeValue = acc.GetValueIn(result, 2);
    EXPECT_EQ(value, acc.GetValueIn(storeValue));
    EXPECT_EQ(acc.GetOpCode(storeValue), OpCode::TRUNC);
    EXPECT_EQ(acc.GetMachineType(storeValue), MachineType::I16);
}

HWTEST_F_L0(TypedArrayLoweringTests, UInt16OnHeapArrayStoreElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto value = builder.Int32(2);
    auto storeElement =
        builder.StoreElement<ecmascript::kungfu::TypedStoreOp::UINT16ARRAY_STORE_ELEMENT>(array, index, value,
                                                                                         OnHeapMode::ON_HEAP);
    auto ret = builder.Return(builder.Undefined());
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "UInt16OnHeapArrayStoreElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(storeElement), OpCode::NOP);
    auto result = acc.GetDep(ret);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::STORE_MEMORY);
    auto storeValue = acc.GetValueIn(result, 2);
    EXPECT_EQ(value, acc.GetValueIn(storeValue));
    EXPECT_EQ(acc.GetOpCode(storeValue), OpCode::TRUNC);
    EXPECT_EQ(acc.GetMachineType(storeValue), MachineType::I16);
}

HWTEST_F_L0(TypedArrayLoweringTests, Float32OnHeapArrayStoreElement)
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
    acc.SetGateType(array, GateType::AnyType());
    auto index = builder.Int32(1);
    auto value = builder.Double(1.5);
    auto storeElement =
        builder.StoreElement<ecmascript::kungfu::TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT>(array, index, value,
                                                                                           OnHeapMode::ON_HEAP);
    auto ret = builder.Return(builder.Undefined());
    EXPECT_TRUE(Verifier::Run(&circuit));
    CombinedPassVisitor visitor(&circuit, false, "Float32OnHeapArrayStoreElement", &chunk);
    TypedHCRLowering lowering(&circuit, nullptr, &visitor, nullptr, &chunk, false);
    visitor.AddPass(&lowering);
    visitor.VisitGraph();
    EXPECT_TRUE(Verifier::Run(&circuit));
    EXPECT_EQ(acc.GetOpCode(storeElement), OpCode::NOP);
    auto result = acc.GetDep(ret);
    EXPECT_EQ(acc.GetOpCode(result), OpCode::STORE_MEMORY);
    auto storeValue = acc.GetValueIn(result, 2);
    EXPECT_EQ(value, acc.GetValueIn(storeValue));
    EXPECT_EQ(acc.GetOpCode(storeValue), OpCode::FTRUNC);
    EXPECT_EQ(acc.GetMachineType(storeValue), MachineType::F32);
}
} // namespace panda::test
