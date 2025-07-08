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

#include "ecmascript/compiler/typed_hcr_lowering.h"

#include "ecmascript/compiler/builtins_lowering.h"
#include "ecmascript/compiler/deopt_type.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/linked_hash_table.h"

namespace panda::ecmascript::kungfu {
GateRef TypedHCRLowering::VisitGate(GateRef gate)
{
    GateRef glue = glue_;
    auto op = acc_.GetOpCode(gate);
    [[maybe_unused]] auto scopedGate = circuit_->VisitGateBegin(gate);
    switch (op) {
        case OpCode::PRIMITIVE_TYPE_CHECK:
            LowerPrimitiveTypeCheck(gate);
            break;
        case OpCode::BUILTIN_INSTANCE_HCLASS_CHECK:
            LowerBuiltinInstanceHClassCheck(gate);
            break;
        case OpCode::BUILTIN_PROTOTYPE_HCLASS_CHECK:
            LowerBuiltinPrototypeHClassCheck(gate);
            break;
        case OpCode::STABLE_ARRAY_CHECK:
            LowerStableArrayCheck(glue, gate);
            break;
        case OpCode::TYPED_ARRAY_CHECK:
            LowerTypedArrayCheck(glue, gate);
            break;
        case OpCode::STRING_KEY_CHECK:
            LowerStringKeyCheck(gate, glue);
            break;
        case OpCode::INTERN_STRING_KEY_CHECK:
            LowerInternStringKeyCheck(gate, glue);
            break;
        case OpCode::ECMA_STRING_CHECK:
            LowerEcmaStringCheck(gate, glue);
            break;
        case OpCode::INTERN_STRING_CHECK:
            LowerInternStringCheck(gate, glue);
            break;
        case OpCode::ECMA_MAP_CHECK:
            LowerEcmaMapCheck(glue, gate);
            break;
        case OpCode::FLATTEN_TREE_STRING_CHECK:
            LowerFlattenTreeStringCheck(gate, glue);
            break;
        case OpCode::LOAD_STRING_LENGTH:
            LowerStringLength(gate);
            break;
        case OpCode::LOAD_MAP_SIZE:
            LowerMapSize(gate);
            break;
        case OpCode::LOAD_TYPED_ARRAY_LENGTH:
            LowerLoadTypedArrayLength(gate);
            break;
        case OpCode::OBJECT_TYPE_CHECK:
            LowerObjectTypeCheck(glue, gate);
            break;
        case OpCode::RANGE_CHECK_PREDICATE:
            LowerRangeCheckPredicate(gate);
            break;
        case OpCode::INDEX_CHECK:
            LowerIndexCheck(gate);
            break;
        case OpCode::TYPED_CALLTARGETCHECK_OP:
            LowerJSCallTargetCheck(gate, glue);
            break;
        case OpCode::CALL_TARGET_IS_COMPILED_CHECK:
            LowerCallTargetIsCompiledCheck(gate);
            break;
        case OpCode::TYPED_CALL_CHECK:
            LowerCallTargetCheck(gate);
            break;
        case OpCode::JSINLINETARGET_TYPE_CHECK:
            LowerJSInlineTargetTypeCheck(gate, glue);
            break;
        case OpCode::JSINLINETARGET_HEAPCONSTANT_CHECK:
            LowerJSInlineTargetHeapConstantCheck(gate);
            break;
        case OpCode::TYPE_CONVERT:
            LowerTypeConvert(gate);
            break;
        case OpCode::LOAD_PROPERTY:
            LowerLoadProperty(gate);
            break;
        case OpCode::CALL_PRIVATE_GETTER:
            LowerCallPrivateGetter(gate, glue);
            break;
        case OpCode::CALL_PRIVATE_SETTER:
            LowerCallPrivateSetter(gate, glue);
            break;
        case OpCode::CALL_GETTER:
            LowerCallGetter(gate, glue);
            break;
        case OpCode::STORE_PROPERTY:
        case OpCode::STORE_PROPERTY_NO_BARRIER:
            LowerStoreProperty(gate);
            break;
        case OpCode::CALL_SETTER:
            LowerCallSetter(gate, glue);
            break;
        case OpCode::LOAD_ARRAY_LENGTH:
            LowerLoadArrayLength(gate);
            break;
        case OpCode::LOAD_ELEMENT:
            LowerLoadElement(gate, glue);
            break;
        case OpCode::STORE_ELEMENT:
            LowerStoreElement(gate, glue);
            break;
        case OpCode::TYPED_CALL_BUILTIN:
        case OpCode::TYPED_CALL_BUILTIN_SIDE_EFFECT:
            LowerTypedCallBuitin(gate);
            break;
        case OpCode::TYPED_NEW_ALLOCATE_THIS:
            LowerTypedNewAllocateThis(gate, glue);
            break;
        case OpCode::TYPED_SUPER_ALLOCATE_THIS:
            LowerTypedSuperAllocateThis(gate, glue);
            break;
        case OpCode::GET_SUPER_CONSTRUCTOR:
            LowerGetSuperConstructor(glue, gate);
            break;
        case OpCode::COW_ARRAY_CHECK:
            LowerCowArrayCheck(gate, glue);
            break;
        case OpCode::LOOK_UP_HOLDER:
            LowerLookupHolder(glue, gate);
            break;
        case OpCode::LOAD_GETTER:
            LowerLoadGetter(gate, glue);
            break;
        case OpCode::LOAD_SETTER:
            LowerLoadSetter(gate, glue);
            break;
        case OpCode::PROTOTYPE_CHECK:
            LowerPrototypeCheck(glue, gate);
            break;
        case OpCode::STRING_EQUAL:
            LowerStringEqual(gate, glue);
            break;
        case OpCode::TYPE_OF_CHECK:
            LowerTypeOfCheck(gate);
            break;
        case OpCode::TYPE_OF:
            LowerTypeOf(gate, glue);
            break;
        case OpCode::TYPED_CONSTRUCTOR_CHECK:
            LowerTypedConstructorCheck(gate, glue);
            break;
        case OpCode::ARRAY_CONSTRUCTOR_CHECK:
            LowerArrayConstructorCheck(gate, glue);
            break;
        case OpCode::ARRAY_CONSTRUCTOR:
            LowerArrayConstructor(gate, glue);
            break;
        case OpCode::FLOAT32_ARRAY_CONSTRUCTOR_CHECK:
            LowerFloat32ArrayConstructorCheck(gate, glue);
            break;
        case OpCode::FLOAT32_ARRAY_CONSTRUCTOR:
            LowerFloat32ArrayConstructor(gate, glue);
            break;
        case OpCode::LOAD_BUILTIN_OBJECT:
            LowerLoadBuiltinObject(gate);
            break;
        case OpCode::OBJECT_CONSTRUCTOR_CHECK:
            LowerObjectConstructorCheck(gate, glue);
            break;
        case OpCode::OBJECT_CONSTRUCTOR:
            LowerObjectConstructor(gate, glue);
            break;
        case OpCode::BOOLEAN_CONSTRUCTOR_CHECK:
            LowerBooleanConstructorCheck(gate, glue);
            break;
        case OpCode::BOOLEAN_CONSTRUCTOR:
            LowerBooleanConstructor(gate, glue);
            break;
        case OpCode::ORDINARY_HAS_INSTANCE:
            if (compilationEnv_->IsJitCompiler()) {
                LowerOrdinaryHasInstanceForJIT(gate, glue);
            } else {
                LowerOrdinaryHasInstance(gate, glue);
            }
            break;
        case OpCode::PROTO_CHANGE_MARKER_CHECK:
            LowerProtoChangeMarkerCheck(glue, gate);
            break;
        case OpCode::PRIMTYPE_PROTO_CHANGE_MARKER_CHECK:
            LowerPrimitiveTypeProtoChangeMarkerCheck(glue, gate);
            break;
        case OpCode::MONO_CALL_GETTER_ON_PROTO:
            LowerMonoCallGetterOnProto(gate, glue);
            break;
        case OpCode::MONO_LOAD_PROPERTY_ON_PROTO:
            LowerMonoLoadPropertyOnProto(glue, gate);
            break;
        case OpCode::MONO_STORE_PROPERTY_LOOK_UP_PROTO:
            LowerMonoStorePropertyLookUpProto(gate, glue);
            break;
        case OpCode::MONO_STORE_PROPERTY:
            LowerMonoStoreProperty(gate, glue);
            break;
        case OpCode::TYPED_CREATE_OBJ_WITH_BUFFER:
            LowerTypedCreateObjWithBuffer(gate, glue);
            break;
        case OpCode::STRING_FROM_SINGLE_CHAR_CODE:
            LowerStringFromSingleCharCode(gate, glue);
            break;
        case OpCode::MIGRATE_ARRAY_WITH_KIND:
            LowerMigrateArrayWithKind(gate, glue);
            break;
        case OpCode::NUMBER_TO_STRING:
            LowerNumberToString(gate, glue);
            break;
        case OpCode::ECMA_OBJECT_CHECK:
            LowerEcmaObjectCheck(gate);
            break;
        case OpCode::ELEMENTSKIND_CHECK:
            LowerElementskindCheck(glue, gate);
            break;
        case OpCode::INLINE_SUPER_CTOR_CHECK:
            LowerInlineSuperCtorCheck(gate, glue);
            break;
        case OpCode::CHECK_CONSTRUCTOR:
            LowerCheckConstructor(gate, glue);
            break;
        default:
            break;
    }
    return Circuit::NullGate();
}

void TypedHCRLowering::LowerJSCallTargetCheck(GateRef gate, GateRef glue)
{
    TypedCallTargetCheckOp Op = acc_.GetTypedCallTargetCheckOp(gate);
    switch (Op) {
        case TypedCallTargetCheckOp::JSCALL: {
            LowerJSCallTargetTypeCheck(gate, glue);
            break;
        }
        case TypedCallTargetCheckOp::JSCALL_FAST: {
            LowerJSFastCallTargetTypeCheck(gate, glue);
            break;
        }
        case TypedCallTargetCheckOp::JSCALLTHIS: {
            LowerJSCallThisTargetTypeCheck(gate, glue);
            break;
        }
        case TypedCallTargetCheckOp::JSCALLTHIS_FAST: {
            LowerJSFastCallThisTargetTypeCheck(gate, glue);
            break;
        }
        case TypedCallTargetCheckOp::JSCALLTHIS_NOGC: {
            LowerJSNoGCCallThisTargetTypeCheck(gate, glue);
            break;
        }
        case TypedCallTargetCheckOp::JSCALLTHIS_FAST_NOGC: {
            LowerJSNoGCFastCallThisTargetTypeCheck(gate, glue);
            break;
        }
        case TypedCallTargetCheckOp::JS_NEWOBJRANGE: {
            LowerJSNewObjRangeCallTargetCheck(gate, glue);
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void TypedHCRLowering::LowerPrimitiveTypeCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto type = acc_.GetParamGateType(gate);
    if (type.IsIntType()) {
        LowerIntCheck(gate);
    } else if (type.IsDoubleType()) {
        LowerDoubleCheck(gate);
    } else if (type.IsNumberType()) {
        LowerNumberCheck(gate);
    } else if (type.IsBooleanType()) {
        LowerBooleanCheck(gate);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void TypedHCRLowering::LowerIntCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsInt(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTINT6);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerDoubleCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsDouble(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTDOUBLE3);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerNumberCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsNumber(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTNUMBER2);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerBooleanCheck(GateRef gate)
{
    GateRef frameState = GetFrameState(gate);

    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedIsBoolean(value);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::NOTBOOL2);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerStableArrayCheck(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);

    GateRef receiverHClass = builder_.LoadHClassByConstOffset(glue, receiver);
    ArrayMetaDataAccessor accessor = acc_.GetArrayMetaDataAccessor(gate);
    builder_.HClassStableArrayCheck(receiverHClass, frameState, accessor);
    builder_.ArrayGuardianCheck(frameState);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::SetDeoptTypeInfo(JSType jstype, DeoptType &type, size_t &typedArrayRootHclassIndex,
    size_t &typedArrayRootHclassOnHeapIndex)
{
    type = DeoptType::NOTARRAY1;
    switch (jstype) {
        case JSType::JS_INT8_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::INT8_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::INT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_UINT8_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::UINT8_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::UINT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::UINT8_CLAMPED_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::UINT8_CLAMPED_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_INT16_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::INT16_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::INT16_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_UINT16_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::UINT16_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::UINT16_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_INT32_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::INT32_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::INT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_UINT32_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::UINT32_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::UINT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_FLOAT32_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::FLOAT32_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::FLOAT32_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_FLOAT64_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::FLOAT64_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::FLOAT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_BIGINT64_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::BIGINT64_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::BIGINT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        case JSType::JS_BIGUINT64_ARRAY:
            typedArrayRootHclassIndex = GlobalEnv::BIGUINT64_ARRAY_ROOT_HCLASS_INDEX;
            typedArrayRootHclassOnHeapIndex = GlobalEnv::BIGUINT64_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void TypedHCRLowering::LowerTypedArrayCheck(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    TypedArrayMetaDataAccessor accessor = acc_.GetTypedArrayMetaDataAccessor(gate);
    size_t typedArrayRootHclassIndex = GlobalEnv::INT8_ARRAY_ROOT_HCLASS_INDEX;
    size_t typedArrayRootHclassOnHeapIndex = GlobalEnv::INT8_ARRAY_ROOT_HCLASS_ON_HEAP_INDEX;
    auto deoptType = DeoptType::NONE;
    ParamType paramType = accessor.GetParamType();
    ASSERT(paramType.IsBuiltinType());
    auto builtinType = paramType.GetBuiltinType();
    SetDeoptTypeInfo(static_cast<JSType>(builtinType), deoptType, typedArrayRootHclassIndex,
                     typedArrayRootHclassOnHeapIndex);

    GateRef frameState = GetFrameState(gate);
    GateRef globalEnv = circuit_->GetGlobalEnvCache();
    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);
    GateRef receiverHClass = builder_.LoadHClassByConstOffset(glue, receiver);

    OnHeapMode onHeapMode = accessor.GetOnHeapMode();
    if (OnHeap::IsOnHeap(onHeapMode)) {
        GateRef rootOnHeapHclass = builder_.GetGlobalEnvObj(globalEnv, typedArrayRootHclassOnHeapIndex);
        GateRef check = builder_.Equal(receiverHClass, rootOnHeapHclass);
        builder_.DeoptCheck(check, frameState, deoptType);
    } else if (OnHeap::IsNotOnHeap(onHeapMode)) {
        GateRef rootHclass = builder_.GetGlobalEnvObj(globalEnv, typedArrayRootHclassIndex);
        GateRef check = builder_.Equal(receiverHClass, rootHclass);
        builder_.DeoptCheck(check, frameState, deoptType);
    } else {
        GateRef rootHclass = builder_.GetGlobalEnvObj(globalEnv, typedArrayRootHclassIndex);
        GateRef rootOnHeapHclass = builder_.GetGlobalEnvObj(globalEnv, typedArrayRootHclassOnHeapIndex);
        GateRef check1 = builder_.Equal(receiverHClass, rootHclass);
        GateRef check2 = builder_.Equal(receiverHClass, rootOnHeapHclass);
        builder_.DeoptCheck(builder_.BitOr(check1, check2), frameState, deoptType);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerStringKeyCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef key = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    builder_.DeoptCheck(builder_.TaggedIsString(glue, key), frameState, DeoptType::NOTSTRING1);
    builder_.DeoptCheck(builder_.StringEqual(key, value),
                        frameState, DeoptType::KEYMISSMATCH);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerInternStringKeyCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef key = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    builder_.HeapObjectCheck(key, frameState);
    GateRef isString = builder_.TaggedObjectIsString(glue, key, compilationEnv_);
    builder_.DeoptCheck(isString, frameState, DeoptType::NOTSTRING1);
    GateRef isInternString = builder_.IsInternString(key);
    builder_.DeoptCheck(isInternString, frameState, DeoptType::NOTINTERNSTRING1);
    GateRef isEqual = builder_.Equal(value, key);
    builder_.DeoptCheck(isEqual, frameState, DeoptType::KEYMISSMATCH);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerEcmaStringCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);
    GateRef isString = builder_.TaggedObjectIsString(glue, receiver, compilationEnv_);
    builder_.DeoptCheck(isString, frameState, DeoptType::NOTSTRING1);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerInternStringCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);
    GateRef isString = builder_.TaggedObjectIsString(glue, receiver, compilationEnv_);
    builder_.DeoptCheck(isString, frameState, DeoptType::NOTSTRING1);
    GateRef isInternString = builder_.IsInternString(receiver);
    builder_.DeoptCheck(isInternString, frameState, DeoptType::NOTINTERNSTRING1);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerEcmaMapCheck(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);

    GateRef hclass = builder_.LoadHClassByConstOffset(glue, receiver);

    size_t mapHclassIndex = GlobalEnv::MAP_CLASS_INDEX;
    GateRef globalEnv = circuit_->GetGlobalEnvCache();
    GateRef mapHclass = builder_.GetGlobalEnvObj(globalEnv, mapHclassIndex);
    GateRef isMap = builder_.Equal(hclass, mapHclass, "Check HClass");

    builder_.DeoptCheck(isMap, frameState, DeoptType::ISNOTMAP);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerFlattenTreeStringCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef str = acc_.GetValueIn(gate, 0);
    DEFVALUE(result, (&builder_), VariableType::JS_POINTER(), str);
    Label isTreeString(&builder_);
    Label exit(&builder_);

    BRANCH_CIR(builder_.IsTreeString(glue, str), &isTreeString, &exit);
    builder_.Bind(&isTreeString);
    {
        Label isFlat(&builder_);
        Label needFlat(&builder_);
        BRANCH_CIR(builder_.TreeStringIsFlat(glue, str), &isFlat, &needFlat);
        builder_.Bind(&isFlat);
        {
            result = builder_.GetFirstFromTreeString(glue, str);
            builder_.Jump(&exit);
        }
        builder_.Bind(&needFlat);
        {
            result = LowerCallRuntime(glue, gate, RTSTUB_ID(SlowFlattenString), { str }, true);
            builder_.Jump(&exit);
        }
    }

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

GateRef TypedHCRLowering::GetLengthFromString(GateRef gate)
{
    GateRef shiftCount = builder_.Int32(BaseString::LengthBits::START_BIT);
    return builder_.Int32LSR(
        builder_.LoadConstOffset(VariableType::INT32(), gate, BaseString::LENGTH_AND_FLAGS_OFFSET), shiftCount);
}

void TypedHCRLowering::LowerStringLength(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef length = GetLengthFromString(receiver);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), length);
}

void TypedHCRLowering::LowerMapSize(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);

    GateRef linkedMap = builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, JSMap::LINKED_MAP_OFFSET);
    GateRef mapSizeTagged = builder_.LoadFromTaggedArray(linkedMap, LinkedHashMap::NUMBER_OF_ELEMENTS_INDEX);
    GateRef mapSize = builder_.TaggedGetInt(mapSizeTagged);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), mapSize);
}

void TypedHCRLowering::LowerLoadTypedArrayLength(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef length = builder_.LoadConstOffset(VariableType::INT32(), receiver, JSTypedArray::ARRAY_LENGTH_OFFSET);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), length);
}

void TypedHCRLowering::LowerObjectTypeCheck(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    size_t typeCount = acc_.GetNumValueIn(gate) - 1;
    if (typeCount == 1) {
        GateRef frameState = GetFrameState(gate);
        GateRef compare = BuildCompareHClass(glue, gate, frameState);
        builder_.DeoptCheck(compare, frameState, DeoptType::INCONSISTENTHCLASS6);
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
        return;
    }
    LowerSimpleHClassCheck(glue, gate);
}

GateRef TypedHCRLowering::BuildCompareHClass(GateRef glue, GateRef gate, GateRef frameState)
{
    GateRef receiver = acc_.GetValueIn(gate, 0);
    bool isHeapObject = acc_.GetObjectTypeAccessor(gate).IsHeapObject();
    if (!isHeapObject) {
        builder_.HeapObjectCheck(receiver, frameState);
    }
    GateRef aotHCIndex = acc_.GetValueIn(gate, 1);
    auto hclassIndex = acc_.GetConstantValue(aotHCIndex);
    ArgumentAccessor *argAcc = circuit_->GetArgumentAccessor();
    GateRef unsharedConstPool = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::UNSHARED_CONST_POOL);
    GateRef aotHCGate = builder_.LoadHClassFromConstpool(unsharedConstPool, hclassIndex);
    GateRef receiverHClass = builder_.LoadHClassByConstOffset(glue, receiver);
    return builder_.Equal(aotHCGate, receiverHClass, "checkHClass");
}

void TypedHCRLowering::LowerSimpleHClassCheck(GateRef glue, GateRef gate)
{
    GateRef frameState = GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    bool isHeapObject = acc_.GetObjectTypeAccessor(gate).IsHeapObject();
    if (!isHeapObject) {
        builder_.HeapObjectCheck(receiver, frameState);
    }
    DEFVALUE(result, (&builder_), VariableType::BOOL(), builder_.Boolean(false));
    ArgumentAccessor *argAcc = circuit_->GetArgumentAccessor();
    GateRef unsharedConstPool = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::UNSHARED_CONST_POOL);
    GateRef receiverHClass = builder_.LoadHClassByConstOffset(glue, receiver);
    std::vector<Label> ifFalse;
    Label resultIsTrue(&builder_);
    Label resultIsFalse(&builder_);
    Label exit(&builder_);
    size_t typeCount = acc_.GetNumValueIn(gate) - 1;
    ASSERT(typeCount > 0);
    for (size_t i = 0; i < typeCount - 1; i++) {
        ifFalse.emplace_back(Label(&builder_));
    }
    for (size_t i = 0; i < typeCount; i++) {
        GateRef aotHCIndex = acc_.GetValueIn(gate, i + 1);
        auto hclassIndex = acc_.GetConstantValue(aotHCIndex);
        GateRef aotHCGate = builder_.LoadHClassFromConstpool(unsharedConstPool, hclassIndex);

        if (i != typeCount - 1) {
            BRANCH_CIR(builder_.Equal(receiverHClass, aotHCGate), &resultIsTrue, &ifFalse[i]);
            builder_.Bind(&ifFalse[i]);
        } else {
            BRANCH_CIR(builder_.Equal(receiverHClass, aotHCGate), &resultIsTrue, &resultIsFalse);
        }
    }
    builder_.Bind(&resultIsFalse);
    {
        result = builder_.Boolean(false);
        builder_.Jump(&exit);
    }
    builder_.Bind(&resultIsTrue);
    {
        result = builder_.Boolean(true);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    auto finalResult = *result;
    builder_.DeoptCheck(finalResult, frameState, DeoptType::INCONSISTENTHCLASS6);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerRangeCheckPredicate(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto deoptType = DeoptType::NOTARRAY1;
    GateRef frameState = GetFrameState(gate);
    GateRef x = acc_.GetValueIn(gate, 0);
    GateRef y = acc_.GetValueIn(gate, 1);
    TypedBinaryAccessor accessor = acc_.GetTypedBinaryAccessor(gate);
    TypedBinOp cond = accessor.GetTypedBinOp();
    GateRef check = Circuit::NullGate();
    // check the condition
    switch (cond) {
        case TypedBinOp::TYPED_GREATER:
            check = builder_.Int32GreaterThan(x, y);
            break;
        case TypedBinOp::TYPED_GREATEREQ:
            check = builder_.Int32GreaterThanOrEqual(x, y);
            break;
        case TypedBinOp::TYPED_LESS:
            check = builder_.Int32LessThan(x, y);
            break;
        case TypedBinOp::TYPED_LESSEQ:
            check = builder_.Int32LessThanOrEqual(x, y);
            break;
        default:
            UNREACHABLE();
            break;
    }
    builder_.DeoptCheck(check, frameState, deoptType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::BuiltinInstanceHClassCheck(Environment *env, GateRef gate)
{
    BuiltinPrototypeHClassAccessor accessor = acc_.GetBuiltinHClassAccessor(gate);
    BuiltinTypeId type = accessor.GetBuiltinTypeId();
    ElementsKind kind = accessor.GetElementsKind();
    GateRef frameState = GetFrameState(gate);
    GateRef glue = glue_;
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef ihcMatches = Circuit::NullGate();
    if (type == BuiltinTypeId::ARRAY) {
        if (Elements::IsGeneric(kind)) {
            auto index = compilationEnv_->GetArrayHClassIndex(kind, false);
            auto protoIndex = compilationEnv_->GetArrayHClassIndex(kind, true);
            GateRef receiverHClass = builder_.LoadHClassByConstOffset(glue, receiver);
            // If the Elements kind is Generic, hclass comparison is required. Other kinds can ensure that hclass has
            // not been modified.
            GateRef globalEnv = circuit_->GetGlobalEnvCache();
            ihcMatches = LogicOrBuilder(env)
                .Or(builder_.Equal(receiverHClass,
                                   builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                              static_cast<size_t>(index))))
                .Or(builder_.Equal(receiverHClass,
                                   builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv,
                                                              static_cast<size_t>(protoIndex))))
                .Done();
            GateRef elementsKind = builder_.GetElementsKindByHClass(receiverHClass);
            ihcMatches = LogicOrBuilder(env)
                .Or(ihcMatches)
                .Or(builder_.NotEqual(elementsKind, builder_.Int32(Elements::ToUint(ElementsKind::GENERIC))))
                .Done();
        } else {
            GateRef receiverHClass = builder_.LoadHClassByConstOffset(glue, receiver);
            GateRef elementsKind = builder_.GetElementsKindByHClass(receiverHClass);
            ihcMatches =
                builder_.NotEqual(elementsKind, builder_.Int32(Elements::ToUint(ElementsKind::GENERIC)));
        }
    } else {
        size_t ihcOffset = JSThread::GlueData::GetBuiltinInstanceHClassOffset(type, env->IsArch32Bit());
        GateRef initialIhcAddress = builder_.LoadConstOffset(VariableType::JS_POINTER(), glue, ihcOffset);
        GateRef receiverHClass = builder_.LoadHClassByConstOffset(glue, receiver);
        if (IsTypedArrayType(type)) {
             // check IHC onHeap hclass
            size_t ihcOnHeapOffset = JSThread::GlueData::GetBuiltinExtraHClassOffset(type, env->IsArch32Bit());
            ihcMatches = LogicOrBuilder(env)
                .Or(builder_.Equal(receiverHClass, initialIhcAddress))
                .Or(builder_.Equal(receiverHClass,
                                   builder_.LoadConstOffset(VariableType::JS_POINTER(), glue, ihcOnHeapOffset)))
                .Done();
        } else {
            ihcMatches = builder_.Equal(receiverHClass, initialIhcAddress);
        }
    }
    // De-opt if HClass of x changed where X is the current builtin object.
    builder_.DeoptCheck(ihcMatches, frameState, DeoptType::BUILTININSTANCEHCLASSMISMATCH);
}

void TypedHCRLowering::BuiltinPrototypeHClassCheck(Environment *env, GateRef gate)
{
    BuiltinPrototypeHClassAccessor accessor = acc_.GetBuiltinHClassAccessor(gate);
    BuiltinTypeId type = accessor.GetBuiltinTypeId();
    bool isPrototypeOfPrototype = accessor.IsPrototypeOfPrototype();
    GateRef frameState = GetFrameState(gate);
    GateRef glue = glue_;
    GateRef receiver = acc_.GetValueIn(gate, 0);
    // Only HClasses recorded in the JSThread during builtin initialization are available
    [[maybe_unused]] JSHClass *initialPrototypeHClass = compilationEnv_->GetBuiltinPrototypeHClass(type);
    ASSERT(initialPrototypeHClass != nullptr);

    // Phc = PrototypeHClass
    size_t phcOffset = JSThread::GlueData::GetBuiltinPrototypeHClassOffset(type, env->IsArch32Bit());
    GateRef receiverPhcAddress = builder_.LoadPrototypeHClass(glue, receiver);
    GateRef initialPhcAddress = builder_.LoadConstOffset(VariableType::JS_POINTER(), glue, phcOffset);
    GateRef phcMatches = builder_.Equal(receiverPhcAddress, initialPhcAddress);
    // De-opt if HClass of X.prototype changed where X is the current builtin object.
    builder_.DeoptCheck(phcMatches, frameState, DeoptType::BUILTINPROTOHCLASSMISMATCH1);

    // array.Iterator should compare PrototypeOfPrototypeHClass.
    if (isPrototypeOfPrototype) {
        size_t pphcOffset = JSThread::GlueData::GetBuiltinPrototypeOfPrototypeHClassOffset(type, env->IsArch32Bit());
        GateRef receiverPPhcAddress = builder_.LoadPrototypeOfPrototypeHClass(glue, receiver);
        GateRef initialPPhcAddress = builder_.LoadConstOffset(VariableType::JS_POINTER(), glue, pphcOffset);
        GateRef pphcMatches = builder_.Equal(receiverPPhcAddress, initialPPhcAddress);
        // De-opt if HClass of X.prototype.prototype changed where X is the current builtin object.
        builder_.DeoptCheck(pphcMatches, frameState, DeoptType::BUILTINPROTOHCLASSMISMATCH2);
    }
}

void TypedHCRLowering::BuiltinInstanceStringTypeCheck(GateRef gate)
{
    BuiltinPrototypeHClassAccessor accessor = acc_.GetBuiltinHClassAccessor(gate);
    [[maybe_unused]] BuiltinTypeId type = accessor.GetBuiltinTypeId();
    ASSERT(type == BuiltinTypeId::STRING);
    GateRef frameState = GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef typeCheck = builder_.TaggedObjectIsString(glue_, receiver, compilationEnv_);
    builder_.DeoptCheck(typeCheck, frameState, DeoptType::BUILTININSTANCEHCLASSMISMATCH2);
}

void TypedHCRLowering::LowerBuiltinInstanceHClassCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);
    BuiltinPrototypeHClassAccessor accessor = acc_.GetBuiltinHClassAccessor(gate);
    BuiltinTypeId type = accessor.GetBuiltinTypeId();
    // BuiltinTypeId::STRING represents primitive string, only need to check the type of hclass here.
    if (type == BuiltinTypeId::STRING) {
        BuiltinInstanceStringTypeCheck(gate);
    } else {
        BuiltinInstanceHClassCheck(&env, gate); // check IHC
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerBuiltinPrototypeHClassCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(receiver, frameState);
    BuiltinPrototypeHClassAccessor accessor = acc_.GetBuiltinHClassAccessor(gate);
    BuiltinTypeId type = accessor.GetBuiltinTypeId();
    // BuiltinTypeId::STRING represents primitive string, only need to check the type of hclass here.
    if (type == BuiltinTypeId::STRING) {
        BuiltinInstanceStringTypeCheck(gate);
    } else {
        BuiltinInstanceHClassCheck(&env, gate); // check IHC
        BuiltinPrototypeHClassCheck(&env, gate); // check PHC
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerIndexCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    auto deoptType = DeoptType::NOTLEGALIDX1;

    GateRef frameState = GetFrameState(gate);
    GateRef length = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    ASSERT(acc_.GetGateType(length).IsNJSValueType());
    // UnsignedLessThan can check both lower and upper bounds
    GateRef lengthCheck = builder_.Int32UnsignedLessThan(index, length);
    builder_.DeoptCheck(lengthCheck, frameState, deoptType);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), index);
}

GateRef TypedHCRLowering::LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                                           bool useLabel)
{
    const std::string name = RuntimeStubCSigns::GetRTName(index);
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args, hirGate, name.c_str());
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args, hirGate, name.c_str());
        return result;
    }
}

void TypedHCRLowering::LowerTypeConvert(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    TypeConvertAccessor accessor(acc_.TryGetValue(gate));
    ParamType leftType = accessor.GetLeftType();
    GateType rightType = accessor.GetRightType();
    if (rightType.IsNumberType()) {
        GateRef value = acc_.GetValueIn(gate, 0);
        // NOTICE-PGO: wx support undefined/null/boolean type:
        if (leftType.HasNumberType()) {
            LowerPrimitiveToNumber(gate, value, leftType);
        }
        return;
    }
}

void TypedHCRLowering::LowerPrimitiveToNumber(GateRef dst, GateRef src, ParamType srcType)
{
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    if (srcType.IsBooleanType()) {
        Label exit(&builder_);
        Label isTrue(&builder_);
        Label isFalse(&builder_);
        BRANCH_CIR(builder_.TaggedIsTrue(src), &isTrue, &isFalse);
        builder_.Bind(&isTrue);
        {
            result = IntToTaggedIntPtr(builder_.Int32(1));
            builder_.Jump(&exit);
        }
        builder_.Bind(&isFalse);
        {
            result = IntToTaggedIntPtr(builder_.Int32(0));
            builder_.Jump(&exit);
        }
        builder_.Bind(&exit);
    } else if (srcType.IsUndefinedType()) {
        result = DoubleToTaggedDoublePtr(builder_.Double(base::NAN_VALUE));
    } else if (srcType.IsBigIntType() || srcType.HasNumberType()) {
        ASSERT(!srcType.IsIntOverflowType());
        result = src;
    } else if (srcType.IsNullType()) {
        result = IntToTaggedIntPtr(builder_.Int32(0));
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    acc_.ReplaceGate(dst, builder_.GetState(), builder_.GetDepend(), *result);
}

GateRef TypedHCRLowering::LoadFromConstPool(GateRef glue, GateRef unsharedConstPool, size_t index, size_t valVecType)
{
    GateRef constPoolSize = builder_.GetLengthOfTaggedArray(unsharedConstPool);
    GateRef valVecIndex = builder_.Int32Sub(constPoolSize, builder_.Int32(valVecType));
    GateRef valVec = builder_.GetValueFromTaggedArray(glue, unsharedConstPool, valVecIndex);
    return builder_.LoadFromTaggedArray(valVec, index);
}

void TypedHCRLowering::LowerLoadProperty(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: receiver, plr
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsLocal() || plr.IsFunction());

    GateRef result = LoadPropertyFromHolder(receiver, plr);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedHCRLowering::LowerCallPrivateGetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: receiver, accessor
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef accessor = acc_.GetValueIn(gate, 1);

    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    result = CallAccessor(glue, gate, accessor, receiver, AccessorMode::GETTER);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedHCRLowering::LowerCallPrivateSetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3); // 3: receiver, accessor, value
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef accessor = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);

    CallAccessor(glue, gate, accessor, receiver, AccessorMode::SETTER, value);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerCallGetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3);  // 3: receiver, holder, plr
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    GateRef holder = acc_.GetValueIn(gate, 2);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));

    GateRef accessor = LoadPropertyFromHolder(holder, plr);

    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    Label isInternalAccessor(&builder_);
    Label notInternalAccessor(&builder_);
    Label callGetter(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(builder_.IsAccessorInternal(glue, accessor), &isInternalAccessor, &notInternalAccessor);
    {
        builder_.Bind(&isInternalAccessor);
        {
            result = builder_.CallRuntime(glue, RTSTUB_ID(CallInternalGetter),
                Gate::InvalidGateRef, { accessor, holder }, gate);
            builder_.Jump(&exit);
        }
        builder_.Bind(&notInternalAccessor);
        {
            GateRef getter = builder_.LoadConstOffset(VariableType::JS_ANY(), accessor, AccessorData::GETTER_OFFSET);
            BRANCH_CIR(builder_.IsSpecial(getter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callGetter);
            builder_.Bind(&callGetter);
            {
                result = CallAccessor(glue, gate, getter, receiver, AccessorMode::GETTER);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedHCRLowering::LowerStoreProperty(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3);  // 3: receiver, plr, value
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2); // 2: value
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsLocal());
    auto op = OpCode(acc_.GetOpCode(gate));
    if (op == OpCode::STORE_PROPERTY) {
        if (plr.IsInlinedProps()) {
            builder_.StoreConstOffset(VariableType::JS_ANY(), receiver, plr.GetOffset(), value);
        } else {
            auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, JSObject::PROPERTIES_OFFSET);
            builder_.SetValueToTaggedArray(
                VariableType::JS_ANY(), glue_, properties, builder_.Int32(plr.GetOffset()), value);
        }
    } else if (op == OpCode::STORE_PROPERTY_NO_BARRIER) {
        if (plr.IsInlinedProps()) {
            builder_.StoreConstOffset(GetVarType(plr), receiver, plr.GetOffset(), value);
        } else {
            auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, JSObject::PROPERTIES_OFFSET);
            builder_.SetValueToTaggedArray(
                GetVarType(plr), glue_, properties, builder_.Int32(plr.GetOffset()), value);
        }
    } else {
        UNREACHABLE();
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerCallSetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 4);  // 4: receiver, holder, plr, value
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    GateRef holder = acc_.GetValueIn(gate, 2);
    GateRef value = acc_.GetValueIn(gate, 3);

    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());
    GateRef accessor = Circuit::NullGate();
    if (plr.IsNotHole()) {
        ASSERT(plr.IsLocal());
        if (plr.IsInlinedProps()) {
            accessor = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, plr.GetOffset());
        } else {
            auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, JSObject::PROPERTIES_OFFSET);
            accessor = builder_.GetValueFromTaggedArray(glue, properties, builder_.Int32(plr.GetOffset()));
        }
    } else if (plr.IsLocal()) {
        if (plr.IsInlinedProps()) {
            accessor = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, plr.GetOffset());
        } else {
            auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, JSObject::PROPERTIES_OFFSET);
            accessor = builder_.GetValueFromTaggedArray(glue, properties, builder_.Int32(plr.GetOffset()));
        }
        accessor = builder_.ConvertHoleAsUndefined(accessor);
    } else {
        UNREACHABLE();
    }
    Label isInternalAccessor(&builder_);
    Label notInternalAccessor(&builder_);
    Label callSetter(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(builder_.IsAccessorInternal(glue, accessor), &isInternalAccessor, &notInternalAccessor);
    {
        builder_.Bind(&isInternalAccessor);
        {
            builder_.CallRuntime(glue, RTSTUB_ID(CallInternalSetter),
                Gate::InvalidGateRef, { receiver, accessor, value }, gate);
            builder_.Jump(&exit);
        }
        builder_.Bind(&notInternalAccessor);
        {
            GateRef setter = builder_.LoadConstOffset(VariableType::JS_ANY(), accessor, AccessorData::SETTER_OFFSET);
            BRANCH_CIR(builder_.IsSpecial(setter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callSetter);
            builder_.Bind(&callSetter);
            {
                CallAccessor(glue, gate, setter, receiver, AccessorMode::SETTER, value);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerLoadArrayLength(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef array = acc_.GetValueIn(gate, 0);
    GateRef result = builder_.LoadConstOffset(VariableType::INT32(), array, JSArray::LENGTH_OFFSET);
    acc_.SetGateType(gate, GateType::NJSValue());
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

GateRef TypedHCRLowering::GetElementSize(BuiltinTypeId id)
{
    GateRef elementSize = Circuit::NullGate();
    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
        case BuiltinTypeId::UINT8_ARRAY:
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            elementSize = builder_.Int32(sizeof(uint8_t));
            break;
        case BuiltinTypeId::INT16_ARRAY:
        case BuiltinTypeId::UINT16_ARRAY:
            elementSize = builder_.Int32(sizeof(uint16_t));
            break;
        case BuiltinTypeId::INT32_ARRAY:
        case BuiltinTypeId::UINT32_ARRAY:
        case BuiltinTypeId::FLOAT32_ARRAY:
            elementSize = builder_.Int32(sizeof(uint32_t));
            break;
        case BuiltinTypeId::FLOAT64_ARRAY:
            elementSize = builder_.Int32(sizeof(double));
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return elementSize;
}

VariableType TypedHCRLowering::GetVariableType(BuiltinTypeId id)
{
    VariableType type = VariableType::JS_ANY();
    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
        case BuiltinTypeId::UINT8_ARRAY:
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            type = VariableType::INT8();
            break;
        case BuiltinTypeId::INT16_ARRAY:
        case BuiltinTypeId::UINT16_ARRAY:
            type = VariableType::INT16();
            break;
        case BuiltinTypeId::INT32_ARRAY:
        case BuiltinTypeId::UINT32_ARRAY:
            type = VariableType::INT32();
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            type = VariableType::FLOAT32();
            break;
        case BuiltinTypeId::FLOAT64_ARRAY:
            type = VariableType::FLOAT64();
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return type;
}

void TypedHCRLowering::LowerLoadElement(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    LoadElementAccessor accessor = acc_.GetLoadElementAccessor(gate);
    TypedLoadOp op = accessor.GetTypedLoadOp();
    switch (op) {
        case TypedLoadOp::ARRAY_LOAD_INT_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_DOUBLE_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_OBJECT_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_TAGGED_ELEMENT:
            LowerArrayLoadElement(gate, ArrayState::PACKED, op);
            break;
        case TypedLoadOp::ARRAY_LOAD_HOLE_TAGGED_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_HOLE_INT_ELEMENT:
        case TypedLoadOp::ARRAY_LOAD_HOLE_DOUBLE_ELEMENT:
            LowerArrayLoadElement(gate, ArrayState::HOLEY, op);
            break;
        case TypedLoadOp::INT8ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::INT8_ARRAY);
            break;
        case TypedLoadOp::UINT8ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::UINT8_ARRAY);
            break;
        case TypedLoadOp::UINT8CLAMPEDARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::UINT8_CLAMPED_ARRAY);
            break;
        case TypedLoadOp::INT16ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::INT16_ARRAY);
            break;
        case TypedLoadOp::UINT16ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::UINT16_ARRAY);
            break;
        case TypedLoadOp::INT32ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::INT32_ARRAY);
            break;
        case TypedLoadOp::UINT32ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::UINT32_ARRAY);
            break;
        case TypedLoadOp::FLOAT32ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::FLOAT32_ARRAY);
            break;
        case TypedLoadOp::FLOAT64ARRAY_LOAD_ELEMENT:
            LowerTypedArrayLoadElement(gate, BuiltinTypeId::FLOAT64_ARRAY);
            break;
        case TypedLoadOp::STRING_LOAD_ELEMENT:
            LowerStringLoadElement(gate, glue);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void TypedHCRLowering::LowerCowArrayCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    Label notCOWArray(&builder_);
    Label isCOWArray(&builder_);
    GateRef needCOW = builder_.IsJsCOWArray(glue, receiver, compilationEnv_);
    BRANCH_CIR(needCOW, &isCOWArray, &notCOWArray);
    builder_.Bind(&isCOWArray);
    {
        LowerCallRuntime(glue, gate, RTSTUB_ID(CheckAndCopyArray), {receiver}, true);
        builder_.Jump(&notCOWArray);
    }
    builder_.Bind(&notCOWArray);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

// for JSArray
void TypedHCRLowering::LowerArrayLoadElement(GateRef gate, ArrayState arrayState, TypedLoadOp op)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = glue_;
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef element = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSObject::ELEMENTS_OFFSET);
    GateRef result = Circuit::NullGate();
    if (arrayState == ArrayState::HOLEY) {
        if (op == TypedLoadOp::ARRAY_LOAD_HOLE_INT_ELEMENT ||
            op == TypedLoadOp::ARRAY_LOAD_HOLE_DOUBLE_ELEMENT) {
            result = builder_.GetValueFromJSArrayWithElementsKind(VariableType::INT64(), element, index);
        } else {
            result = builder_.GetValueFromTaggedArray(glue, element, index);
            result = builder_.ConvertHoleAsUndefined(result);
        }
    } else {
        // When elementsKind swith on, we should get corresponding raw value for Int and Double kind.
        result = builder_.GetValueFromTaggedArray(glue, element, index);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedHCRLowering::LowerTypedArrayLoadElement(GateRef gate, BuiltinTypeId id)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = glue_;
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef elementSize = GetElementSize(id);
    GateRef offset = builder_.PtrMul(index, elementSize);
    VariableType type = GetVariableType(id);

    GateRef result = Circuit::NullGate();
    LoadElementAccessor accessor = acc_.GetLoadElementAccessor(gate);
    OnHeapMode onHeapMode = accessor.GetOnHeapMode();

    switch (onHeapMode) {
        case OnHeapMode::ON_HEAP: {
            result = BuildOnHeapTypedArrayLoadElement(glue, receiver, offset, type);
            break;
        }
        case OnHeapMode::NOT_ON_HEAP: {
            result = BuildNotOnHeapTypedArrayLoadElement(glue, receiver, offset, type);
            break;
        }
        default: {
            Label isByteArray(&builder_);
            Label isArrayBuffer(&builder_);
            Label exit(&builder_);
            result = BuildTypedArrayLoadElement(glue, receiver, offset, type, &isByteArray, &isArrayBuffer, &exit);
            break;
        }
    }

    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
            result = builder_.SExtInt8ToInt32(result);
            break;
        case BuiltinTypeId::UINT8_ARRAY:
        case BuiltinTypeId::UINT8_CLAMPED_ARRAY:
            result = builder_.ZExtInt8ToInt32(result);
            break;
        case BuiltinTypeId::INT16_ARRAY:
            result = builder_.SExtInt16ToInt32(result);
            break;
        case BuiltinTypeId::UINT16_ARRAY:
            result = builder_.ZExtInt16ToInt32(result);
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            result = builder_.ExtFloat32ToDouble(result);
            break;
        default:
            break;
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

GateRef TypedHCRLowering::BuildOnHeapTypedArrayLoadElement(GateRef glue, GateRef receiver, GateRef offset,
    VariableType type)
{
    GateRef byteArray =
        builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    GateRef data = builder_.PtrAdd(byteArray, builder_.IntPtr(ByteArray::DATA_OFFSET));
    GateRef result = builder_.Load(type, glue, data, offset);
    return result;
}

GateRef TypedHCRLowering::BuildNotOnHeapTypedArrayLoadElement(GateRef glue, GateRef receiver, GateRef offset,
    VariableType type)
{
    GateRef arrayBuffer =
        builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);

    GateRef data =
        builder_.Load(VariableType::JS_POINTER(), glue, arrayBuffer, builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
    GateRef block =
        builder_.Load(VariableType::JS_ANY(), glue, data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));
    GateRef byteOffset = builder_.LoadWithoutBarrier(VariableType::INT32(), receiver,
                                                     builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
    GateRef result = builder_.Load(type, glue, block, builder_.PtrAdd(offset, byteOffset));
    return result;
}

GateRef TypedHCRLowering::BuildTypedArrayLoadElement(GateRef glue, GateRef receiver, GateRef offset, VariableType type,
                                                     Label *isByteArray, Label *isArrayBuffer, Label *exit)
{
    GateRef byteArrayOrArrayBuffer =
        builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    DEFVALUE(data, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    DEFVALUE(result, (&builder_), type, builder_.Double(0));

    GateRef isOnHeap = builder_.IsOnHeap(builder_.LoadHClassByConstOffset(glue, receiver));
    BRANCH_CIR(isOnHeap, isByteArray, isArrayBuffer);
    builder_.Bind(isByteArray);
    {
        data = builder_.PtrAdd(byteArrayOrArrayBuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));
        result = builder_.Load(type, glue, *data, offset);
        builder_.Jump(exit);
    }
    builder_.Bind(isArrayBuffer);
    {
        data = builder_.Load(VariableType::JS_POINTER(), glue, byteArrayOrArrayBuffer,
                             builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
        GateRef block =
            builder_.Load(VariableType::JS_ANY(), glue, *data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));
        GateRef byteOffset =
            builder_.LoadWithoutBarrier(VariableType::INT32(), receiver,
                                        builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
        result = builder_.Load(type, glue, block, builder_.PtrAdd(offset, byteOffset));
        builder_.Jump(exit);
    }
    builder_.Bind(exit);

    return *result;
}

void TypedHCRLowering::LowerStringLoadElement(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef result = Circuit::NullGate();
    if (compilationEnv_->IsJitCompiler()) {
        result = builder_.CallStub(glue, gate, CommonStubCSigns::StringLoadElement,
                                   {glue, receiver, index, circuit_->GetGlobalEnvCache()}, "StringLoadElement stub");
    } else {
        BuiltinsStringStubBuilder builder(&env, circuit_->GetGlobalEnvCache());
        result = builder.GetSingleCharCodeByIndex(glue, receiver, index);
    }

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedHCRLowering::LowerStoreElement(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    StoreElementAccessor accessor = acc_.GetStoreElementAccessor(gate);
    TypedStoreOp op = accessor.GetTypedStoreOp();
    switch (op) {
        case TypedStoreOp::ARRAY_STORE_ELEMENT:
        case TypedStoreOp::ARRAY_STORE_INT_ELEMENT:
        case TypedStoreOp::ARRAY_STORE_DOUBLE_ELEMENT:
            LowerArrayStoreElement(gate, glue, op);
            break;
        case TypedStoreOp::INT8ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::INT8_ARRAY);
            break;
        case TypedStoreOp::UINT8ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::UINT8_ARRAY);
            break;
        case TypedStoreOp::UINT8CLAMPEDARRAY_STORE_ELEMENT:
            LowerUInt8ClampedArrayStoreElement(gate);
            break;
        case TypedStoreOp::INT16ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::INT16_ARRAY);
            break;
        case TypedStoreOp::UINT16ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::UINT16_ARRAY);
            break;
        case TypedStoreOp::INT32ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::INT32_ARRAY);
            break;
        case TypedStoreOp::UINT32ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::UINT32_ARRAY);
            break;
        case TypedStoreOp::FLOAT32ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::FLOAT32_ARRAY);
            break;
        case TypedStoreOp::FLOAT64ARRAY_STORE_ELEMENT:
            LowerTypedArrayStoreElement(gate, BuiltinTypeId::FLOAT64_ARRAY);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

// for JSArray
void TypedHCRLowering::LowerArrayStoreElement(GateRef gate, GateRef glue, TypedStoreOp op)
{
    Environment env(gate, circuit_, &builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);  // 0: receiver
    GateRef index = acc_.GetValueIn(gate, 1);     // 1: index
    GateRef value = acc_.GetValueIn(gate, 2);     // 2: value

    GateRef element = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver, JSObject::ELEMENTS_OFFSET);
    // Because at retype stage, we have set output according to op
    // there is not need to consider about the convertion here.
    if (op == TypedStoreOp::ARRAY_STORE_INT_ELEMENT) {
        GateRef convertedValue = builder_.ZExtInt32ToInt64(value);
        builder_.SetValueToTaggedArray(VariableType::INT64(), glue, element, index, convertedValue);
    } else if (op == TypedStoreOp::ARRAY_STORE_DOUBLE_ELEMENT) {
        builder_.SetValueToTaggedArray(VariableType::FLOAT64(), glue, element, index, value);
    } else {
        builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue, element, index, value);
    }

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

// for JSTypedArray
void TypedHCRLowering::LowerTypedArrayStoreElement(GateRef gate, BuiltinTypeId id)
{
    Environment env(gate, circuit_, &builder_);
    GateRef glue = glue_;
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);

    GateRef elementSize = GetElementSize(id);
    GateRef offset = builder_.PtrMul(index, elementSize);
    switch (id) {
        case BuiltinTypeId::INT8_ARRAY:
        case BuiltinTypeId::UINT8_ARRAY:
            value = builder_.TruncInt32ToInt8(value);
            break;
        case BuiltinTypeId::INT16_ARRAY:
        case BuiltinTypeId::UINT16_ARRAY:
            value = builder_.TruncInt32ToInt16(value);
            break;
        case BuiltinTypeId::FLOAT32_ARRAY:
            value = builder_.TruncDoubleToFloat32(value);
            break;
        default:
            break;
    }

    Label isByteArray(&builder_);
    Label isArrayBuffer(&builder_);
    Label exit(&builder_);
    OptStoreElementByOnHeapMode(gate, glue, receiver, offset, value, &isByteArray, &isArrayBuffer, &exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::OptStoreElementByOnHeapMode(GateRef gate, GateRef glue, GateRef receiver, GateRef offset,
    GateRef value, Label *isByteArray, Label *isArrayBuffer, Label *exit)
{
    StoreElementAccessor accessor = acc_.GetStoreElementAccessor(gate);
    OnHeapMode onHeapMode = accessor.GetOnHeapMode();
    switch (onHeapMode) {
        case OnHeapMode::ON_HEAP: {
            BuildOnHeapTypedArrayStoreElement(receiver, offset, value);
            break;
        }
        case OnHeapMode::NOT_ON_HEAP: {
            BuildNotOnHeapTypedArrayStoreElement(glue, receiver, offset, value);
            break;
        }
        default: {
            BuildTypedArrayStoreElement(glue, receiver, offset, value, isByteArray, isArrayBuffer, exit);
            break;
        }
    }
}

void TypedHCRLowering::BuildOnHeapTypedArrayStoreElement(GateRef receiver, GateRef offset, GateRef value)
{
    GateRef byteArray = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver,
                                                 JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    GateRef data = builder_.PtrAdd(byteArray, builder_.IntPtr(ByteArray::DATA_OFFSET));

    builder_.StoreMemory(MemoryType::ELEMENT_TYPE, VariableType::VOID(), data, offset, value);
}

void TypedHCRLowering::BuildNotOnHeapTypedArrayStoreElement(GateRef glue, GateRef receiver,
                                                            GateRef offset, GateRef value)
{
    GateRef arrayBuffer = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver,
                                                   JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    GateRef data =
        builder_.Load(VariableType::JS_POINTER(), glue, arrayBuffer, builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
    GateRef block = builder_.Load(VariableType::JS_ANY(), glue, data,
                                  builder_.IntPtr(JSNativePointer::POINTER_OFFSET));
    GateRef byteOffset =
        builder_.LoadWithoutBarrier(VariableType::INT32(), receiver,
                                    builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
    builder_.StoreMemory(MemoryType::ELEMENT_TYPE, VariableType::VOID(), block,
                         builder_.PtrAdd(offset, byteOffset), value);
}

void TypedHCRLowering::BuildTypedArrayStoreElement(GateRef glue, GateRef receiver, GateRef offset, GateRef value,
                                                   Label *isByteArray, Label *isArrayBuffer, Label *exit)
{
    GateRef byteArrayOrArrayBuffer = builder_.LoadConstOffset(VariableType::JS_POINTER(), receiver,
                                                              JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET);
    GateRef isOnHeap = builder_.IsOnHeap(builder_.LoadHClassByConstOffset(glue, receiver));
    DEFVALUE(data, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    BRANCH_CIR(isOnHeap, isByteArray, isArrayBuffer);
    builder_.Bind(isByteArray);
    {
        data = builder_.PtrAdd(byteArrayOrArrayBuffer, builder_.IntPtr(ByteArray::DATA_OFFSET));
        builder_.StoreMemory(MemoryType::ELEMENT_TYPE, VariableType::VOID(), *data, offset, value);
        builder_.Jump(exit);
    }
    builder_.Bind(isArrayBuffer);
    {
        data = builder_.Load(VariableType::JS_POINTER(), glue, byteArrayOrArrayBuffer,
                             builder_.IntPtr(JSArrayBuffer::DATA_OFFSET));
        GateRef block =
            builder_.Load(VariableType::JS_ANY(), glue, *data, builder_.IntPtr(JSNativePointer::POINTER_OFFSET));
        GateRef byteOffset =
            builder_.LoadWithoutBarrier(VariableType::INT32(), receiver,
                                        builder_.IntPtr(JSTypedArray::BYTE_OFFSET_OFFSET));
        builder_.StoreMemory(MemoryType::ELEMENT_TYPE, VariableType::VOID(), block,
                             builder_.PtrAdd(offset, byteOffset), value);
        builder_.Jump(exit);
    }
    builder_.Bind(exit);
}

// for UInt8ClampedArray
void TypedHCRLowering::LowerUInt8ClampedArrayStoreElement(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);

    GateRef glue = glue_;
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef elementSize = builder_.Int32(sizeof(uint8_t));
    GateRef offset = builder_.PtrMul(index, elementSize);
    GateRef value = acc_.GetValueIn(gate, 2);

    DEFVALUE(result, (&builder_), VariableType::INT32(), value);
    GateRef topValue = builder_.Int32(static_cast<uint32_t>(UINT8_MAX));
    GateRef bottomValue = builder_.Int32(static_cast<uint32_t>(0));
    Label isOverFlow(&builder_);
    Label notOverFlow(&builder_);
    Label exit(&builder_);
    Label isByteArray(&builder_);
    Label isArrayBuffer(&builder_);
    Label quit(&builder_);
    BRANCH_CIR(builder_.Int32GreaterThan(value, topValue), &isOverFlow, &notOverFlow);
    builder_.Bind(&isOverFlow);
    {
        result = topValue;
        builder_.Jump(&exit);
    }
    builder_.Bind(&notOverFlow);
    {
        Label isUnderSpill(&builder_);
        BRANCH_CIR(builder_.Int32LessThan(value, bottomValue), &isUnderSpill, &exit);
        builder_.Bind(&isUnderSpill);
        {
            result = bottomValue;
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    value = builder_.TruncInt32ToInt8(*result);
    OptStoreElementByOnHeapMode(gate, glue, receiver, offset, value, &isByteArray, &isArrayBuffer, &quit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

GateRef TypedHCRLowering::DoubleToTaggedDoublePtr(GateRef gate)
{
    return builder_.DoubleToTaggedDoublePtr(gate);
}

GateRef TypedHCRLowering::ChangeInt32ToFloat64(GateRef gate)
{
    return builder_.ChangeInt32ToFloat64(gate);
}

GateRef TypedHCRLowering::TruncDoubleToInt(GateRef gate)
{
    return builder_.TruncInt64ToInt32(builder_.TruncFloatToInt64(gate));
}

GateRef TypedHCRLowering::IntToTaggedIntPtr(GateRef x)
{
    GateRef val = builder_.SExtInt32ToInt64(x);
    return builder_.ToTaggedIntPtr(val);
}

void TypedHCRLowering::LowerTypedCallBuitin(GateRef gate)
{
    BuiltinLowering lowering(circuit_, builder_.GetCompilationConfig(), traceBuiltins_);
    lowering.LowerTypedCallBuitin(gate);
}

void TypedHCRLowering::CallTargetIsCompiledCheck(GateRef func, GateRef frameState,
                                                 Label *checkAlreadyDeopt, Label *exit)
{
    GateRef isCompiled = builder_.JudgeAotAndFastCall(func, CircuitBuilder::JudgeMethodType::HAS_AOT);
    BRANCH_CIR_LIKELY(isCompiled, exit, checkAlreadyDeopt);
    builder_.Bind(checkAlreadyDeopt);
    {
        GateRef glue = glue_;
        GateRef method = builder_.GetMethodFromFunction(glue, func);
        GateRef hasDeopt = builder_.AlreadyDeopt(method);
        builder_.DeoptCheck(hasDeopt, frameState, DeoptType::CALLTARGETNOTCOMPILED);
        builder_.Jump(exit);
    }
    builder_.Bind(exit);
}

void TypedHCRLowering::LowerCallTargetIsCompiledCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    Label checkAlreadyDeopt(&builder_);
    Label exit(&builder_);
    GateRef func = acc_.GetValueIn(gate, 0);
    GateRef frameState = acc_.GetFrameState(gate);
    builder_.IsCallableCheck(func, frameState);
    CallTargetIsCompiledCheck(func, frameState, &checkAlreadyDeopt, &exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSCallTargetTypeCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label checkAlreadyDeopt(&builder_);
    Label exit(&builder_);
    ArgumentAccessor *argAcc = circuit_->GetArgumentAccessor();
    GateRef frameState = GetFrameState(gate);
    GateRef sharedConstPool = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::SHARED_CONST_POOL);
    auto func = acc_.GetValueIn(gate, 0);
    auto methodIndex = acc_.GetValueIn(gate, 1);
    builder_.IsCallableCheck(func, frameState);
    CallTargetIsCompiledCheck(func, frameState, &checkAlreadyDeopt, &exit);

    GateRef funcMethodTarget = builder_.GetMethodFromFunction(glue, func);
    GateRef methodTarget = builder_.GetValueFromTaggedArray(glue, sharedConstPool, methodIndex);
    GateRef check = builder_.Equal(funcMethodTarget, methodTarget);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT2);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSFastCallTargetTypeCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label checkAlreadyDeopt(&builder_);
    Label exit(&builder_);
    ArgumentAccessor *argAcc = circuit_->GetArgumentAccessor();
    GateRef frameState = GetFrameState(gate);
    GateRef sharedConstPool = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::SHARED_CONST_POOL);
    auto func = acc_.GetValueIn(gate, 0);
    auto methodIndex = acc_.GetValueIn(gate, 1);
    builder_.IsCallableCheck(func, frameState);
    CallTargetIsCompiledCheck(func, frameState, &checkAlreadyDeopt, &exit);

    GateRef funcMethodTarget = builder_.GetMethodFromFunction(glue, func);
    GateRef methodTarget = builder_.GetValueFromTaggedArray(glue, sharedConstPool, methodIndex);
    GateRef check = builder_.Equal(funcMethodTarget, methodTarget);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTJSFASTCALLTGT1);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSCallThisTargetTypeCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label checkAlreadyDeopt(&builder_);
    Label exit(&builder_);
    GateRef frameState = GetFrameState(gate);
    auto func = acc_.GetValueIn(gate, 0);
    builder_.IsCallableCheck(func, frameState);
    CallTargetIsCompiledCheck(func, frameState, &checkAlreadyDeopt, &exit);

    GateRef methodId = builder_.GetMethodId(glue, func);
    GateRef check = builder_.Equal(methodId, acc_.GetValueIn(gate, 1));
    builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT3);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSNoGCCallThisTargetTypeCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label checkAlreadyDeopt(&builder_);
    Label exit(&builder_);
    GateRef frameState = GetFrameState(gate);
    auto func = acc_.GetValueIn(gate, 0);
    builder_.IsCallableCheck(func, frameState);
    CallTargetIsCompiledCheck(func, frameState, &checkAlreadyDeopt, &exit);

    GateRef methodId = builder_.GetMethodId(glue, func);
    GateRef check = builder_.Equal(methodId, acc_.GetValueIn(gate, 1));
    builder_.DeoptCheck(check, frameState, DeoptType::NOTJSCALLTGT4);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSFastCallThisTargetTypeCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label checkAlreadyDeopt(&builder_);
    Label exit(&builder_);
    GateRef frameState = GetFrameState(gate);
    auto func = acc_.GetValueIn(gate, 0);
    builder_.IsCallableCheck(func, frameState);
    CallTargetIsCompiledCheck(func, frameState, &checkAlreadyDeopt, &exit);

    GateRef methodId = builder_.GetMethodId(glue, func);
    GateRef check = builder_.Equal(methodId, acc_.GetValueIn(gate, 1));
    builder_.DeoptCheck(check, frameState, DeoptType::NOTJSFASTCALLTGT2);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSNoGCFastCallThisTargetTypeCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label checkAlreadyDeopt(&builder_);
    Label exit(&builder_);
    GateRef frameState = GetFrameState(gate);
    auto func = acc_.GetValueIn(gate, 0);
    builder_.IsCallableCheck(func, frameState);
    CallTargetIsCompiledCheck(func, frameState, &checkAlreadyDeopt, &exit);

    GateRef methodId = builder_.GetMethodId(glue, func);
    GateRef check = builder_.Equal(methodId, acc_.GetValueIn(gate, 1));
    builder_.DeoptCheck(check, frameState, DeoptType::NOTJSFASTCALLTGT3);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSNewObjRangeCallTargetCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    auto ctor = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(ctor, frameState);
    GateRef isJsFunc = builder_.IsJSFunction(glue, ctor);
    builder_.DeoptCheck(isJsFunc, frameState, DeoptType::NOTJSNEWCALLTGT);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerCallTargetCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);

    BuiltinLowering lowering(circuit_, builder_.GetCompilationConfig(), traceBuiltins_);
    GateRef funcheck = lowering.LowerCallTargetCheck(&env, gate);
    GateRef check = lowering.CheckPara(gate, funcheck);
    builder_.DeoptCheck(check, frameState, DeoptType::NOTCALLTGT1);

    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSInlineTargetTypeCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    auto func = acc_.GetValueIn(gate, 0);
    builder_.HeapObjectCheck(func, frameState);
    GateRef isFunction = builder_.IsJSFunction(glue, func);
    builder_.DeoptCheck(isFunction, frameState, DeoptType::NOTJSFUNCTION);
    GateRef check = builder_.Equal(builder_.GetMethodId(glue, func), acc_.GetValueIn(gate, 1));
    builder_.DeoptCheck(check, frameState, DeoptType::INLINEFAIL1);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerJSInlineTargetHeapConstantCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    auto func = acc_.GetValueIn(gate, 0);
    auto heapConstant = acc_.GetValueIn(gate, 1); // 1: the second value, heapConstant
    builder_.DeoptCheck(builder_.Equal(func, heapConstant), frameState, DeoptType::NOTCALLTARGETHEAPOBJECT);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerInlineSuperCtorCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    auto func = acc_.GetValueIn(gate, 0);
    GateRef newTarget = acc_.GetValueIn(gate, 2); // 2: newTarget
    GateRef constant = acc_.GetValueIn(gate, 1); // 1: Heap constant or method ID
    GateRef check = Circuit::NullGate();
    if (acc_.GetOpCode(constant) == OpCode::HEAP_CONSTANT) {
        check = LogicAndBuilder(&env)
            .And(builder_.Equal(func, constant))
            .And(builder_.BoolNot(builder_.TaggedIsUndefined(newTarget)))
            .Done();
    } else {
        builder_.HeapObjectCheck(func, frameState);
        check = LogicAndBuilder(&env)
            .And(builder_.IsJSFunction(glue, func))
            .And(builder_.IsConstructor(glue, func))
            .And(builder_.Equal(builder_.GetMethodId(glue, func), acc_.GetValueIn(gate, 1)))
            .And(builder_.BoolNot(builder_.TaggedIsUndefined(newTarget)))
            .Done();
    }
    builder_.DeoptCheck(check, frameState, DeoptType::INLINESUPERFAIL);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerTypedNewAllocateThis(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ArgumentAccessor *argAcc = circuit_->GetArgumentAccessor();
    GateRef ctor = acc_.GetValueIn(gate, 0); // 0: 1st argument
    GateRef ihclass = acc_.GetValueIn(gate, 1); // 1: 2nd argument
    GateRef size = acc_.GetValueIn(gate, 2); // 2: 3rd argument
    Label isBase(&builder_);
    Label exit(&builder_);
    DEFVALUE(thisObj, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    BRANCH_CIR(builder_.IsBase(glue, ctor), &isBase, &exit);
    builder_.Bind(&isBase);
    if (compilationEnv_->IsJitCompiler()) {
        thisObj = builder_.CallStub(glue, gate, CommonStubCSigns::NewJSObject, {glue, ihclass, size});
        builder_.Jump(&exit);
    } else {
        NewObjectStubBuilder newBuilder(builder_.GetCurrentEnvironment());
        newBuilder.SetParameters(glue, 0);
        newBuilder.NewJSObject(&thisObj, &exit, ihclass, size);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *thisObj);
}

void TypedHCRLowering::LowerTypedSuperAllocateThis(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef superCtor = acc_.GetValueIn(gate, 0);
    GateRef newTarget = acc_.GetValueIn(gate, 1);

    DEFVALUE(thisObj, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label allocate(&builder_);
    Label exit(&builder_);

    GateRef isBase = builder_.IsBase(glue, superCtor);
    BRANCH_CIR(isBase, &allocate, &exit);
    builder_.Bind(&allocate);
    {
        thisObj = builder_.CallStub(glue, gate, CommonStubCSigns::FastSuperAllocateThis,
            { glue, superCtor, newTarget });
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *thisObj);
}

void TypedHCRLowering::LowerGetSuperConstructor(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateRef hclass = builder_.LoadHClassByConstOffset(glue, ctor);
    GateRef superCtor = builder_.LoadPrototype(glue_, hclass);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), superCtor);
}

VariableType TypedHCRLowering::GetVarType(PropertyLookupResult plr)
{
    if (plr.GetRepresentation() == Representation::DOUBLE) {
        return kungfu::VariableType::FLOAT64();
    } else if (plr.GetRepresentation() == Representation::INT) {
        return kungfu::VariableType::INT32();
    } else {
        return kungfu::VariableType::INT64();
    }
}

GateRef TypedHCRLowering::GetLengthFromSupers(GateRef supers)
{
    return builder_.LoadConstOffset(VariableType::INT32(), supers, TaggedArray::EXTRA_LENGTH_OFFSET);
}

GateRef TypedHCRLowering::GetValueFromSupers(GateRef supers, size_t index)
{
    GateRef val = builder_.LoadFromTaggedArray(supers, index);
    return builder_.LoadObjectFromWeakRef(val);
}

GateRef TypedHCRLowering::CallAccessor(GateRef glue, GateRef gate, GateRef function, GateRef receiver,
    AccessorMode mode, GateRef value)
{
    if (g_isEnableCMCGC) {
        builder_.CallNGCRuntime(glue, RTSTUB_ID(CopyCallTarget), Gate::InvalidGateRef, {glue, function}, glue);
    }
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCall));
    GateRef target = builder_.IntPtr(RTSTUB_ID(JSCall));
    GateRef newTarget = builder_.Undefined();
    GateRef argc = builder_.Int64(NUM_MANDATORY_JSFUNC_ARGS + (mode == AccessorMode::SETTER ? 1 : 0));  // 1: value
    GateRef argv = builder_.IntPtr(0);
    std::vector<GateRef> args { glue, argc, argv, function, newTarget, receiver };
    if (mode == AccessorMode::SETTER) {
        args.emplace_back(value);
    }

    return builder_.Call(cs, glue, target, builder_.GetDepend(), args, gate);
}

void TypedHCRLowering::ReplaceHirWithPendingException(GateRef hirGate, GateRef glue, GateRef state, GateRef depend,
                                                      GateRef value)
{
    auto condition = builder_.HasPendingException(glue, compilationEnv_);
    GateRef ifBranch = builder_.Branch(state, condition, 1, BranchWeight::DEOPT_WEIGHT, "checkException");
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    GateRef eDepend = builder_.DependRelay(ifTrue, depend);
    GateRef sDepend = builder_.DependRelay(ifFalse, depend);

    StateDepend success(ifFalse, sDepend);
    StateDepend exception(ifTrue, eDepend);
    acc_.ReplaceHirWithIfBranch(hirGate, success, exception, value);
}

void TypedHCRLowering::LowerLookupHolder(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 3);  // 3: receiver, holderHC, jsFunc
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef holderHCIndex = acc_.GetValueIn(gate, 1);
    GateRef unsharedConstPool = acc_.GetValueIn(gate, 2);  // 2: constpool
    GateRef holderHC = builder_.LoadHClassFromConstpool(unsharedConstPool,
        acc_.GetConstantValue(holderHCIndex));
    GateRef frameState = acc_.GetFrameState(gate);
    DEFVALUE(holder, (&builder_), VariableType::JS_ANY(), receiver);
    Label loopHead(&builder_);
    Label exit(&builder_);
    Label lookUpProto(&builder_);
    builder_.Jump(&loopHead);

    builder_.LoopBegin(&loopHead);
    builder_.DeoptCheck(builder_.TaggedIsNotNull(*holder), frameState, DeoptType::INCONSISTENTHCLASS13);
    auto curHC = builder_.LoadHClassByConstOffset(glue, *holder);
    BRANCH_CIR(builder_.Equal(curHC, holderHC), &exit, &lookUpProto);

    builder_.Bind(&lookUpProto);
    holder = builder_.LoadPrototype(glue_, curHC);
    builder_.LoopEnd(&loopHead);

    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *holder);
}

void TypedHCRLowering::LowerLoadGetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: holder, plr
    GateRef holder = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());

    GateRef getter;
    if (plr.IsInlinedProps()) {
        auto acceessorData = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, plr.GetOffset());
        getter = builder_.LoadConstOffset(VariableType::JS_ANY(), acceessorData, AccessorData::GETTER_OFFSET);
    } else {
        auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, JSObject::PROPERTIES_OFFSET);
        auto acceessorData = builder_.GetValueFromTaggedArray(glue, properties, builder_.Int32(plr.GetOffset()));
        getter = builder_.LoadConstOffset(VariableType::JS_ANY(), acceessorData, AccessorData::GETTER_OFFSET);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), getter);
}

void TypedHCRLowering::LowerLoadSetter(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 2);  // 2: holder, plr
    GateRef holder = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1);
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    ASSERT(plr.IsAccessor());

    GateRef setter;
    if (plr.IsInlinedProps()) {
        auto acceessorData = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, plr.GetOffset());
        setter = builder_.LoadConstOffset(VariableType::JS_ANY(), acceessorData, AccessorData::SETTER_OFFSET);
    } else {
        auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, JSObject::PROPERTIES_OFFSET);
        auto acceessorData = builder_.GetValueFromTaggedArray(glue, properties, builder_.Int32(plr.GetOffset()));
        setter = builder_.LoadConstOffset(VariableType::JS_ANY(), acceessorData, AccessorData::SETTER_OFFSET);
    }
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), setter);
}

void TypedHCRLowering::LowerPrototypeCheck(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef unsharedConstPool = acc_.GetValueIn(gate, 0);
    GateRef frameState = acc_.GetFrameState(gate);

    uint32_t hclassIndex = acc_.GetHClassIndex(gate);
    auto expectedReceiverHC = builder_.LoadHClassFromConstpool(unsharedConstPool, hclassIndex);

    auto prototype = builder_.LoadPrototype(glue_, expectedReceiverHC);
    auto protoHClass = builder_.LoadHClassByConstOffset(glue, prototype);
    auto marker = builder_.LoadProtoChangeMarker(glue, protoHClass);
    builder_.DeoptCheck(builder_.TaggedIsNotNull(marker), frameState, DeoptType::PROTOTYPECHANGED1);
    auto check = LogicAndBuilder(&env)
        .And(builder_.BoolNot(builder_.GetHasChanged(marker)))
        .And(builder_.BoolNot(builder_.GetAccessorHasChanged(marker)))
        .Done();
    builder_.DeoptCheck(check, frameState, DeoptType::INLINEFAIL2);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
    return;
}

void TypedHCRLowering::LowerStringEqual(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef left = acc_.GetValueIn(gate, 0);
    GateRef right = acc_.GetValueIn(gate, 1);
    GateRef leftLength = GetLengthFromString(left);
    GateRef rightLength = GetLengthFromString(right);

    DEFVALUE(result, (&builder_), VariableType::BOOL(), builder_.False());
    Label lenEqual(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(builder_.Equal(leftLength, rightLength), &lenEqual, &exit);
    builder_.Bind(&lenEqual);
    {
        result = builder_.CallStub(glue, gate, CommonStubCSigns::FastStringEqual,
                                   {glue, left, right, circuit_->GetGlobalEnvCache()});
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedHCRLowering::LowerTypeOfCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef glue = glue_;
    GateRef value = acc_.GetValueIn(gate, 0);
    GateTypeAccessor accessor(acc_.TryGetValue(gate));
    ParamType type = accessor.GetParamType();
    GateRef check = Circuit::NullGate();
    if (type.IsNumberType()) {
        check = builder_.TaggedIsNumber(value);
    } else if (type.IsBooleanType()) {
        check = builder_.TaggedIsBoolean(value);
    } else if (type.IsNullType()) {
        check = builder_.TaggedIsNull(value);
    } else if (type.IsUndefinedType()) {
        check = builder_.TaggedIsUndefined(value);
    } else {
        // NOTICE-PGO: wx add support for builtin(Function Object ArrayKind)
        builder_.DeoptCheck(builder_.TaggedIsHeapObject(value, compilationEnv_),
                            frameState, DeoptType::INCONSISTENTTYPE1);
        if (type.IsStringType()) {
            check = builder_.TaggedIsString(glue, value);
        } else if (type.IsBigIntType()) {
            check = builder_.IsJsType(glue, value, JSType::BIGINT);
        } else if (type.IsSymbolType()) {
            check = builder_.IsJsType(glue, value, JSType::SYMBOL);
        } else {
            UNREACHABLE();
        }
    }

    builder_.DeoptCheck(check, frameState, DeoptType::INCONSISTENTTYPE1);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerTypeOf(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateTypeAccessor accessor(acc_.TryGetValue(gate));
    ParamType type = accessor.GetParamType();
    GateRef gConstAddr = builder_.LoadWithoutBarrier(VariableType::JS_POINTER(), glue,
        builder_.IntPtr(JSThread::GlueData::GetGlobalConstOffset(builder_.GetCompilationConfig()->Is32Bit())));
    ConstantIndex index;
    // NOTICE-PGO: wx add support for builtin(Function Object ArrayKind)
    if (type.IsNumberType()) {
        index = ConstantIndex::NUMBER_STRING_INDEX;
    } else if (type.IsBooleanType()) {
        index = ConstantIndex::BOOLEAN_STRING_INDEX;
    } else if (type.IsNullType()) {
        index = ConstantIndex::OBJECT_STRING_INDEX;
    } else if (type.IsUndefinedType()) {
        index = ConstantIndex::UNDEFINED_STRING_INDEX;
    } else if (type.IsStringType()) {
        index = ConstantIndex::STRING_STRING_INDEX;
    } else if (type.IsBigIntType()) {
        index = ConstantIndex::BIGINT_STRING_INDEX;
    } else if (type.IsSymbolType()) {
        index = ConstantIndex::SYMBOL_STRING_INDEX;
    } else {
        UNREACHABLE();
    }

    GateRef result = builder_.LoadWithoutBarrier(VariableType::JS_POINTER(), gConstAddr,
                                                 builder_.GetGlobalConstantOffset(index));
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedHCRLowering::LowerTypedConstructorCheck(GateRef gate, GateRef glue)
{
    static const std::map<size_t, DeoptType> deoptType = {
        {GlobalEnv::ARRAY_FUNCTION_INDEX, DeoptType::NEWBUILTINCTORARRAY},
        {GlobalEnv::OBJECT_FUNCTION_INDEX, DeoptType::NEWBUILTINCTOROBJECT},
        {GlobalEnv::FLOAT32_ARRAY_FUNCTION_INDEX, DeoptType::NEWBUILTINCTORFLOAT32ARRAY},
        {GlobalEnv::BOOLEAN_FUNCTION_INDEX, DeoptType::NEWBUILTINCTORBOOLEAN},
        {GlobalEnv::BUILTINS_MAP_FUNCTION_INDEX, DeoptType::NEWBUILTINCTORMAP},
        {GlobalEnv::BUILTINS_SET_FUNCTION_INDEX, DeoptType::NEWBUILTINCTORSET},
        {GlobalEnv::DATE_FUNCTION_INDEX, DeoptType::NEWBUILTINCTORDATE},
        {GlobalEnv::ERROR_FUNCTION_INDEX, DeoptType::NEWBUILTINCTORERROR},
    };
    Environment env(gate, circuit_, &builder_);
    size_t functionIndex = static_cast<size_t>(acc_.TryGetValue(gate));
    GateRef frameState = GetFrameState(gate);
    GateRef newTarget = acc_.GetValueIn(gate, 0);
    GateRef globalEnv = circuit_->GetGlobalEnvCache();
    GateRef builtinFunc = builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, functionIndex);
    GateRef check = builder_.Equal(builtinFunc, newTarget);
    auto type =
        deoptType.find(functionIndex) != deoptType.end() ? deoptType.at(functionIndex) : DeoptType::NEWBUILTINCTORFAIL1;
    builder_.DeoptCheck(check, frameState, type);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerArrayConstructorCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef newTarget = acc_.GetValueIn(gate, 0);
    Label isHeapObject(&builder_);
    Label exit(&builder_);
    DEFVALUE(check, (&builder_), VariableType::BOOL(), builder_.True());
    check = builder_.TaggedIsHeapObject(newTarget, compilationEnv_);
    BRANCH_CIR(*check, &isHeapObject, &exit);
    builder_.Bind(&isHeapObject);
    {
        Label isJSFunction(&builder_);
        check = builder_.IsJSFunction(glue, newTarget);
        BRANCH_CIR(*check, &isJSFunction, &exit);
        builder_.Bind(&isJSFunction);
        {
            Label getHclass(&builder_);
            GateRef globalEnv = circuit_->GetGlobalEnvCache();
            GateRef arrayFunc =
                builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
            check = builder_.Equal(arrayFunc, newTarget);
            BRANCH_CIR(*check, &getHclass, &exit);
            builder_.Bind(&getHclass);
            {
                GateRef intialHClass = builder_.Load(VariableType::JS_ANY(), glue, newTarget,
                                                     builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
                check = builder_.IsJSHClass(glue, intialHClass);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    builder_.DeoptCheck(*check, frameState, DeoptType::NEWBUILTINCTORARRAY);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerArrayConstructor(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    if (acc_.GetNumValueIn(gate) == 1) {
        NewArrayConstructorWithNoArgs(gate, glue);
        return;
    }
    ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: new target and arg0
    DEFVALUE(res, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label slowPath(&builder_);
    Label exit(&builder_);
    GateRef newTarget = acc_.GetValueIn(gate, 0);
    GateRef arg0 = acc_.GetValueIn(gate, 1);
    GateRef intialHClass =
        builder_.Load(VariableType::JS_ANY(), glue, newTarget, builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    DEFVALUE(arrayLength, (&builder_), VariableType::INT64(), builder_.Int64(0));
    Label argIsNumber(&builder_);
    Label arrayCreate(&builder_);
    BRANCH_CIR(builder_.TaggedIsNumber(arg0), &argIsNumber, &slowPath);
    builder_.Bind(&argIsNumber);
    {
        Label argIsInt(&builder_);
        Label argIsDouble(&builder_);
        BRANCH_CIR(builder_.TaggedIsInt(arg0), &argIsInt, &argIsDouble);
        builder_.Bind(&argIsInt);
        {
            Label validIntLength(&builder_);
            GateRef intLen = builder_.GetInt64OfTInt(arg0);
            GateRef isGEZero = builder_.Int64GreaterThanOrEqual(intLen, builder_.Int64(0));
            GateRef isLEMaxLen = builder_.Int64LessThanOrEqual(intLen, builder_.Int64(JSArray::MAX_ARRAY_INDEX));
            BRANCH_CIR(builder_.BitAnd(isGEZero, isLEMaxLen), &validIntLength, &slowPath);
            builder_.Bind(&validIntLength);
            {
                arrayLength = intLen;
                builder_.Jump(&arrayCreate);
            }
        }
        builder_.Bind(&argIsDouble);
        {
            Label validDoubleLength(&builder_);
            GateRef doubleLength = builder_.GetDoubleOfTDouble(arg0);
            GateRef doubleToInt = builder_.TruncDoubleToInt(glue, doubleLength, base::INT32_BITS);
            GateRef intToDouble = builder_.CastInt64ToFloat64(builder_.SExtInt32ToInt64(doubleToInt));
            GateRef doubleEqual = builder_.DoubleEqual(doubleLength, intToDouble);
            GateRef doubleLEMaxLen =
                builder_.DoubleLessThanOrEqual(doubleLength, builder_.Double(JSArray::MAX_ARRAY_INDEX));
            BRANCH_CIR(builder_.BitAnd(doubleEqual, doubleLEMaxLen), &validDoubleLength, &slowPath);
            builder_.Bind(&validDoubleLength);
            {
                arrayLength = builder_.SExtInt32ToInt64(doubleToInt);
                builder_.Jump(&arrayCreate);
            }
        }
    }
    builder_.Bind(&arrayCreate);
    {
        Label lengthValid(&builder_);
        BRANCH_CIR(
            builder_.Int64GreaterThan(*arrayLength, builder_.Int64(JSObject::MAX_GAP)), &slowPath, &lengthValid);
        builder_.Bind(&lengthValid);
        {
            NewObjectStubBuilder newBuilder(builder_.GetCurrentEnvironment(), circuit_->GetGlobalEnvCache());
            newBuilder.SetParameters(glue, 0);
            res = newBuilder.NewJSArrayWithSize(intialHClass, *arrayLength);
            GateRef lengthOffset = builder_.IntPtr(JSArray::LENGTH_OFFSET);
            builder_.Store(VariableType::INT32(), glue, *res, lengthOffset, builder_.TruncInt64ToInt32(*arrayLength));
            GateRef accessor = builder_.GetGlobalConstantValue(ConstantIndex::ARRAY_LENGTH_ACCESSOR);
            builder_.SetPropertyInlinedProps(glue, *res, intialHClass, accessor,
                builder_.Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_ANY());
            builder_.SetExtensibleToBitfield(glue, *res, true);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&slowPath);
    {
        size_t range = acc_.GetNumValueIn(gate);
        std::vector<GateRef> args(range);
        for (size_t i = 0; i < range; ++i) {
            args[i] = acc_.GetValueIn(gate, i);
        }
        res = LowerCallRuntime(glue, gate, RTSTUB_ID(OptNewObjRange), args, true);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    ReplaceGateWithPendingException(glue, gate, builder_.GetState(), builder_.GetDepend(), *res);
}

void TypedHCRLowering::LowerFloat32ArrayConstructorCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef newTarget = acc_.GetValueIn(gate, 0);
    GateRef globalEnv = circuit_->GetGlobalEnvCache();
    GateRef arrayFunc =
        builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::FLOAT32_ARRAY_FUNCTION_INDEX);
    GateRef check = builder_.Equal(arrayFunc, newTarget);
    builder_.DeoptCheck(check, frameState, DeoptType::NEWBUILTINCTORFLOAT32ARRAY);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::NewFloat32ArrayConstructorWithNoArgs(GateRef gate, GateRef glue)
{
    NewObjectStubBuilder newBuilder(builder_.GetCurrentEnvironment(), circuit_->GetGlobalEnvCache());
    newBuilder.SetParameters(glue, 0);
    GateRef res = newBuilder.NewFloat32ArrayWithSize(glue, builder_.Int32(0));
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), res);
}

void TypedHCRLowering::ConvertFloat32ArrayConstructorLength(GateRef len, Variable *arrayLength,
                                                            Label *arrayCreate, Label *slowPath)
{
    Label argIsNumber(&builder_);
    BRANCH_CIR(builder_.TaggedIsNumber(len), &argIsNumber, slowPath);
    builder_.Bind(&argIsNumber);
    {
        Label argIsInt(&builder_);
        Label argIsDouble(&builder_);
        BRANCH_CIR(builder_.TaggedIsInt(len), &argIsInt, &argIsDouble);
        builder_.Bind(&argIsInt);
        {
            Label validIntLength(&builder_);
            GateRef intLen = builder_.GetInt64OfTInt(len);
            GateRef isGEZero = builder_.Int64GreaterThanOrEqual(intLen, builder_.Int64(0));
            GateRef isLEMaxLen = builder_.Int64LessThanOrEqual(intLen, builder_.Int64(JSObject::MAX_GAP));
            BRANCH_CIR(builder_.BitAnd(isGEZero, isLEMaxLen), &validIntLength, slowPath);
            builder_.Bind(&validIntLength);
            {
                *arrayLength = intLen;
                builder_.Jump(arrayCreate);
            }
        }
        builder_.Bind(&argIsDouble);
        {
            Label validDoubleLength(&builder_);
            GateRef doubleLength = builder_.GetDoubleOfTDouble(len);
            GateRef doubleToInt = builder_.TruncDoubleToInt(glue_, doubleLength, base::INT32_BITS);
            GateRef intToDouble = builder_.CastInt64ToFloat64(builder_.SExtInt32ToInt64(doubleToInt));
            GateRef doubleEqual = builder_.DoubleEqual(doubleLength, intToDouble);
            GateRef doubleLEMaxLen =
                builder_.DoubleLessThanOrEqual(doubleLength, builder_.Double(JSObject::MAX_GAP));
            BRANCH_CIR(builder_.BitAnd(doubleEqual, doubleLEMaxLen), &validDoubleLength, slowPath);
            builder_.Bind(&validDoubleLength);
            {
                *arrayLength = builder_.SExtInt32ToInt64(doubleToInt);
                builder_.Jump(arrayCreate);
            }
        }
    }
}

void TypedHCRLowering::LowerFloat32ArrayConstructor(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    if (compilationEnv_->IsJitCompiler()) {
        GateRef res = Circuit::NullGate();
        if (acc_.GetNumValueIn(gate) == 1) {
            res = builder_.CallStub(glue, gate, CommonStubCSigns::NewFloat32ArrayWithNoArgs,
                                    {glue, circuit_->GetGlobalEnvCache()}, "NewFloat32ArrayWithNoArgs stub");
        } else {
            ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: new target and arg0
            res = builder_.CallStub(
                glue, gate, CommonStubCSigns::NewFloat32Array,
                {glue, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), circuit_->GetGlobalEnvCache()},
                "NewFloat32Array stub");
        }
        acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), res);
        return;
    }
    if (acc_.GetNumValueIn(gate) == 1) {
        NewFloat32ArrayConstructorWithNoArgs(gate, glue);
        return;
    }
    ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: new target and arg0
    DEFVALUE(res, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label slowPath(&builder_);
    Label exit(&builder_);
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateRef arg0 = acc_.GetValueIn(gate, 1);
    DEFVALUE(arrayLength, (&builder_), VariableType::INT64(), builder_.Int64(0));
    Label arrayCreateByLength(&builder_);
    Label arrayCreate(&builder_);
    ConvertFloat32ArrayConstructorLength(arg0, &arrayLength, &arrayCreateByLength, &arrayCreate);
    builder_.Bind(&arrayCreateByLength);
    {
        NewObjectStubBuilder newBuilder(builder_.GetCurrentEnvironment(), circuit_->GetGlobalEnvCache());
        newBuilder.SetParameters(glue, 0);
        GateRef truncedLength = builder_.TruncInt64ToInt32(*arrayLength);
        res = newBuilder.NewFloat32ArrayWithSize(glue, truncedLength);
        builder_.Jump(&exit);
    }
    builder_.Bind(&arrayCreate);
    {
        NewObjectStubBuilder newBuilder(builder_.GetCurrentEnvironment(), circuit_->GetGlobalEnvCache());
        newBuilder.SetParameters(glue, 0);
        GateRef thisObj = newBuilder.NewFloat32ArrayObj(glue);
        GateRef argc = builder_.Int64(4); // 4: means func newtarget thisObj arg0
        GateRef argv = builder_.IntPtr(0);
        std::vector<GateRef> args { glue, argc, argv, ctor, ctor, thisObj, arg0 };
        res = builder_.CallNew(gate, args);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *res);
}

void TypedHCRLowering::NewArrayConstructorWithNoArgs(GateRef gate, GateRef glue)
{
    GateRef newTarget = acc_.GetValueIn(gate, 0);
    GateRef intialHClass =
        builder_.Load(VariableType::JS_ANY(), glue, newTarget, builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    GateRef arrayLength = builder_.Int64(0);
    NewObjectStubBuilder newBuilder(builder_.GetCurrentEnvironment(), circuit_->GetGlobalEnvCache());
    newBuilder.SetParameters(glue, 0);
    GateRef res = newBuilder.NewJSArrayWithSize(intialHClass, arrayLength);
    GateRef lengthOffset = builder_.IntPtr(JSArray::LENGTH_OFFSET);
    builder_.Store(VariableType::INT32(), glue, res, lengthOffset, builder_.TruncInt64ToInt32(arrayLength));
    GateRef accessor = builder_.GetGlobalConstantValue(ConstantIndex::ARRAY_LENGTH_ACCESSOR);
    builder_.SetPropertyInlinedProps(glue, res, intialHClass, accessor,
                                     builder_.Int32(JSArray::LENGTH_INLINE_PROPERTY_INDEX), VariableType::JS_ANY());
    builder_.SetExtensibleToBitfield(glue, res, true);
    ReplaceGateWithPendingException(glue, gate, builder_.GetState(), builder_.GetDepend(), res);
}

void TypedHCRLowering::LowerObjectConstructorCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef newTarget = acc_.GetValueIn(gate, 0);
    Label isHeapObject(&builder_);
    Label exit(&builder_);
    DEFVALUE(check, (&builder_), VariableType::BOOL(), builder_.True());
    check = builder_.TaggedIsHeapObject(newTarget, compilationEnv_);
    BRANCH_CIR(*check, &isHeapObject, &exit);
    builder_.Bind(&isHeapObject);
    {
        Label isJSFunction(&builder_);
        check = builder_.IsJSFunction(glue, newTarget);
        BRANCH_CIR(*check, &isJSFunction, &exit);
        builder_.Bind(&isJSFunction);
        {
            Label getHclass(&builder_);
            GateRef globalEnv = circuit_->GetGlobalEnvCache();
            GateRef targetFunc =
                builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::OBJECT_FUNCTION_INDEX);
            check = builder_.Equal(targetFunc, newTarget);
            BRANCH_CIR(*check, &getHclass, &exit);
            builder_.Bind(&getHclass);
            {
                GateRef intialHClass = builder_.Load(VariableType::JS_ANY(), glue, newTarget,
                                                     builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
                check = builder_.IsJSHClass(glue, intialHClass);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    builder_.DeoptCheck(*check, frameState, DeoptType::NEWBUILTINCTOROBJECT);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerObjectConstructor(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef value = builder_.Undefined();
    ASSERT(acc_.GetNumValueIn(gate) <= 2); // 2: new target and arg0
    if (acc_.GetNumValueIn(gate) > 1) {
        value = acc_.GetValueIn(gate, 1);
    }
    DEFVALUE(res, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label slowPath(&builder_);
    Label exit(&builder_);

    Label isHeapObj(&builder_);
    Label notHeapObj(&builder_);
    BRANCH_CIR(builder_.TaggedIsHeapObject(value, compilationEnv_), &isHeapObj, &notHeapObj);
    builder_.Bind(&isHeapObj);
    {
        Label isEcmaObj(&builder_);
        Label notEcmaObj(&builder_);
        BRANCH_CIR(builder_.TaggedObjectIsEcmaObject(glue, value), &isEcmaObj, &notEcmaObj);
        builder_.Bind(&isEcmaObj);
        {
            res = value;
            builder_.Jump(&exit);
        }
        builder_.Bind(&notEcmaObj);
        {
            Label isSymbol(&builder_);
            Label notSymbol(&builder_);
            BRANCH_CIR(builder_.TaggedIsSymbol(glue, value), &isSymbol, &notSymbol);
            builder_.Bind(&isSymbol);
            {
                res = NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_SYMBOL, glue, value);
                builder_.Jump(&exit);
            }
            builder_.Bind(&notSymbol);
            {
                Label isBigInt(&builder_);
                BRANCH_CIR(builder_.TaggedIsBigInt(glue, value), &isBigInt, &slowPath);
                builder_.Bind(&isBigInt);
                {
                    res = NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BIGINT, glue, value);
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&notHeapObj);
    {
        Label isNumber(&builder_);
        Label notNumber(&builder_);
        BRANCH_CIR(builder_.TaggedIsNumber(value), &isNumber, &notNumber);
        builder_.Bind(&isNumber);
        {
            res = NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_NUMBER, glue, value);
            builder_.Jump(&exit);
        }
        builder_.Bind(&notNumber);
        {
            Label isBoolean(&builder_);
            Label notBoolean(&builder_);
            BRANCH_CIR(builder_.TaggedIsBoolean(value), &isBoolean, &notBoolean);
            builder_.Bind(&isBoolean);
            {
                res = NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BOOLEAN, glue, value);
                builder_.Jump(&exit);
            }
            builder_.Bind(&notBoolean);
            {
                Label isNullOrUndefined(&builder_);
                BRANCH_CIR(builder_.TaggedIsUndefinedOrNull(value), &isNullOrUndefined, &slowPath);
                builder_.Bind(&isNullOrUndefined);
                {
                    GateRef globalEnv = circuit_->GetGlobalEnvCache();
                    GateRef objectFunctionPrototype = builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue,
                        globalEnv, GlobalEnv::OBJECT_FUNCTION_PROTOTYPE_INDEX);
                    res = builder_.OrdinaryNewJSObjectCreate(glue, objectFunctionPrototype);
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&slowPath);
    {
        size_t range = acc_.GetNumValueIn(gate);
        std::vector<GateRef> args(range);
        for (size_t i = 0; i < range; ++i) {
            args[i] = acc_.GetValueIn(gate, i);
        }
        res = LowerCallRuntime(glue, gate, RTSTUB_ID(OptNewObjRange), args, true);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *res);
}

void TypedHCRLowering::LowerBooleanConstructorCheck(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = GetFrameState(gate);
    GateRef newTarget = acc_.GetValueIn(gate, 0);
    Label isHeapObject(&builder_);
    Label exit(&builder_);
    DEFVALUE(check, (&builder_), VariableType::BOOL(), builder_.True());
    check = builder_.TaggedIsHeapObject(newTarget, compilationEnv_);
    BRANCH_CIR(*check, &isHeapObject, &exit);
    builder_.Bind(&isHeapObject);
    {
        Label isJSFunction(&builder_);
        check = builder_.IsJSFunction(glue, newTarget);
        BRANCH_CIR(*check, &isJSFunction, &exit);
        builder_.Bind(&isJSFunction);
        {
            Label getHclass(&builder_);
            GateRef globalEnv = circuit_->GetGlobalEnvCache();
            GateRef booleanFunc =
                builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::BOOLEAN_FUNCTION_INDEX);
            check = builder_.Equal(booleanFunc, newTarget);
            BRANCH_CIR(*check, &getHclass, &exit);
            builder_.Bind(&getHclass);
            {
                GateRef intialHClass = builder_.Load(VariableType::JS_ANY(), glue, newTarget,
                                                     builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
                check = builder_.IsJSHClass(glue, intialHClass);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    builder_.DeoptCheck(*check, frameState, DeoptType::NEWBUILTINCTORBOOLEAN);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerBooleanConstructor(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef value = builder_.Undefined();
    ASSERT(acc_.GetNumValueIn(gate) <= 2); // 2: new target and arg0
    if (acc_.GetNumValueIn(gate) > 1) {
        value = acc_.GetValueIn(gate, 1);
    }
    DEFVALUE(res, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label isBoolean(&builder_);
    Label slowPath(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(builder_.TaggedIsBoolean(value), &isBoolean, &slowPath);
    builder_.Bind(&isBoolean);
    {
        res = NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BOOLEAN, glue, value);
        builder_.Jump(&exit);
    }
    builder_.Bind(&slowPath);
    {
        size_t range = acc_.GetNumValueIn(gate);
        std::vector<GateRef> args(range);
        for (size_t i = 0; i < range; ++i) {
            args[i] = acc_.GetValueIn(gate, i);
        }
        res = LowerCallRuntime(glue, gate, RTSTUB_ID(OptNewObjRange), args, true);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *res);
}

GateRef TypedHCRLowering::NewJSPrimitiveRef(PrimitiveType type, GateRef glue, GateRef value)
{
    GateRef globalEnv = circuit_->GetGlobalEnvCache();
    GateRef ctor = Circuit::NullGate();
    switch (type) {
        case PrimitiveType::PRIMITIVE_NUMBER: {
            ctor =
                builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::NUMBER_FUNCTION_INDEX);
            break;
        }
        case PrimitiveType::PRIMITIVE_SYMBOL: {
            ctor =
                builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::SYMBOL_FUNCTION_INDEX);
            break;
        }
        case PrimitiveType::PRIMITIVE_BOOLEAN: {
            ctor =
                builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::BOOLEAN_FUNCTION_INDEX);
            break;
        }
        case PrimitiveType::PRIMITIVE_BIGINT: {
            ctor =
                builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glue, globalEnv, GlobalEnv::BIGINT_FUNCTION_INDEX);
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable " << static_cast<int>(type);
            UNREACHABLE();
        }
    }
    GateRef hclass =
        builder_.Load(VariableType::JS_ANY(), glue, ctor, builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));
    NewObjectStubBuilder newBuilder(builder_.GetCurrentEnvironment());
    GateRef res = newBuilder.NewJSObject(glue, hclass);
    GateRef valueOffset = builder_.IntPtr(JSPrimitiveRef::VALUE_OFFSET);
    builder_.Store(VariableType::JS_ANY(), glue, res, valueOffset, value);
    return res;
}

void TypedHCRLowering::ReplaceGateWithPendingException(GateRef glue, GateRef gate, GateRef state, GateRef depend,
                                                       GateRef value)
{
    auto condition = builder_.HasPendingException(glue, compilationEnv_);
    GateRef ifBranch = builder_.Branch(state, condition, 1, BranchWeight::DEOPT_WEIGHT, "checkException");
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    GateRef eDepend = builder_.DependRelay(ifTrue, depend);
    GateRef sDepend = builder_.DependRelay(ifFalse, depend);

    StateDepend success(ifFalse, sDepend);
    StateDepend exception(ifTrue, eDepend);
    acc_.ReplaceHirWithIfBranch(gate, success, exception, value);
}

void TypedHCRLowering::LowerLoadBuiltinObject(GateRef gate)
{
    if (!enableLoweringBuiltin_) {
        return;
    }
    Environment env(gate, circuit_, &builder_);
    auto frameState = GetFrameState(gate);
    GateRef glue = glue_;
    auto builtinEntriesOffset = JSThread::GlueData::GetBuiltinEntriesOffset(false);
    size_t index = acc_.GetIndex(gate);
    auto boxOffset = builtinEntriesOffset + BuiltinIndex::GetInstance().GetBuiltinBoxOffset(index);
    GateRef box = builder_.LoadConstOffset(VariableType::JS_POINTER(), glue, boxOffset);
    GateRef builtin = builder_.LoadConstOffset(VariableType::JS_POINTER(), box, PropertyBox::VALUE_OFFSET);
    auto builtinIsNotHole = builder_.TaggedIsNotHole(builtin);
    // attributes on globalThis may change, it will cause renew a PropertyBox, the old box will be abandoned
    // so we need deopt
    builder_.DeoptCheck(builtinIsNotHole, frameState, DeoptType::BUILTINISHOLE1);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), builtin);
}

void TypedHCRLowering::LowerOrdinaryHasInstanceForJIT(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef target = acc_.GetValueIn(gate, 1);
    auto result = builder_.CallStub(glue, gate, CommonStubCSigns::OrdinaryHasInstance,
                                    {glue, obj, target, circuit_->GetGlobalEnvCache()}, "OrdinaryHasInstance stub");
    ReplaceGateWithPendingException(glue, gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedHCRLowering::LowerOrdinaryHasInstance(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef target = acc_.GetValueIn(gate, 1);

    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.TaggedFalse());
    DEFVALUE(object, (&builder_), VariableType::JS_ANY(), obj);
    Label exit(&builder_);

    Label targetIsBoundFunction(&builder_);
    Label targetNotBoundFunction(&builder_);
    // 2. If C has a [[BoundTargetFunction]] internal slot, then
    //    a. Let BC be the value of C's [[BoundTargetFunction]] internal slot.
    //    b. Return InstanceofOperator(O,BC)  (see 12.9.4).
    BRANCH_CIR(builder_.TaggedIsBoundFunction(glue, target), &targetIsBoundFunction, &targetNotBoundFunction);
    builder_.Bind(&targetIsBoundFunction);
    {
        GateRef boundTarget = builder_.LoadConstOffset(VariableType::JS_ANY(), target,
                                                       JSBoundFunction::BOUND_TARGET_OFFSET);
        result = builder_.CallRuntime(glue, RTSTUB_ID(InstanceOf), Gate::InvalidGateRef,
                                      { obj, boundTarget }, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&targetNotBoundFunction);

    // 3. If Type(O) is not Object, return false
    Label objIsHeapObject(&builder_);
    Label objIsEcmaObject(&builder_);
    Label objNotEcmaObject(&builder_);
    BRANCH_CIR(builder_.TaggedIsHeapObject(obj, compilationEnv_), &objIsHeapObject, &objNotEcmaObject);
    builder_.Bind(&objIsHeapObject);
    BRANCH_CIR(builder_.TaggedObjectIsEcmaObject(glue, obj), &objIsEcmaObject, &objNotEcmaObject);
    builder_.Bind(&objNotEcmaObject);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&objIsEcmaObject);
    {
        // 4. Let P be Get(C, "prototype").
        Label getCtorProtoSlowPath(&builder_);
        Label ctorIsJSFunction(&builder_);
        Label gotCtorPrototype(&builder_);
        Label isHeapObject(&builder_);
        DEFVALUE(constructorPrototype, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
        BRANCH_CIR(builder_.IsJSFunction(glue, target), &ctorIsJSFunction, &getCtorProtoSlowPath);
        builder_.Bind(&ctorIsJSFunction);
        {
            Label getCtorProtoFastPath(&builder_);
            GateRef ctorProtoOrHC = builder_.LoadConstOffset(VariableType::JS_POINTER(), target,
                                                             JSFunction::PROTO_OR_DYNCLASS_OFFSET);

            BRANCH_CIR(builder_.TaggedIsHole(ctorProtoOrHC), &getCtorProtoSlowPath, &getCtorProtoFastPath);
            builder_.Bind(&getCtorProtoFastPath);
            {
                Label isHClass(&builder_);
                Label isPrototype(&builder_);
                BRANCH_CIR(builder_.TaggedIsHeapObject(ctorProtoOrHC, compilationEnv_),
                           &isHeapObject, &getCtorProtoSlowPath);
                builder_.Bind(&isHeapObject);
                BRANCH_CIR(builder_.IsJSHClass(glue, ctorProtoOrHC), &isHClass, &isPrototype);
                builder_.Bind(&isHClass);
                {
                    constructorPrototype = builder_.LoadPrototype(glue_, ctorProtoOrHC);
                    builder_.Jump(&gotCtorPrototype);
                }
                builder_.Bind(&isPrototype);
                {
                    constructorPrototype = ctorProtoOrHC;
                    builder_.Jump(&gotCtorPrototype);
                }
            }
        }
        builder_.Bind(&getCtorProtoSlowPath);
        {
            auto prototypeString = builder_.GetGlobalConstantValue(ConstantIndex::PROTOTYPE_STRING_INDEX);
            constructorPrototype = builder_.CallRuntime(glue, RTSTUB_ID(GetPropertyByName), Gate::InvalidGateRef,
                                                        { target, prototypeString }, gate);
            builder_.Jump(&gotCtorPrototype);
        }
        // 6. If Type(P) is not Object, throw a TypeError exception.
        Label prototypeIsEcmaObj(&builder_);
        Label prototypeNotEcmaObj(&builder_);
        builder_.Bind(&gotCtorPrototype);
        {
            BRANCH_CIR(builder_.IsEcmaObject(glue, *constructorPrototype), &prototypeIsEcmaObj, &prototypeNotEcmaObj);
            builder_.Bind(&prototypeNotEcmaObj);
            {
                GateRef taggedId = builder_.Int32(GET_MESSAGE_STRING_ID(TargetTypeNotObject));
                builder_.CallRuntime(glue, RTSTUB_ID(ThrowTypeError), Gate::InvalidGateRef,
                                     { builder_.Int32ToTaggedInt(taggedId) }, gate);
                result = builder_.ExceptionConstant();
                builder_.Jump(&exit);
            }
        }
        builder_.Bind(&prototypeIsEcmaObj);
        // 7. Repeat
        //    a.Let O be O.[[GetPrototypeOf]]().
        //    b.ReturnIfAbrupt(O).
        //    c.If O is null, return false.
        //    d.If SameValue(P, O) is true, return true.
        Label loopHead(&builder_);
        Label loopEnd(&builder_);
        Label afterLoop(&builder_);
        Label strictEqual1(&builder_);
        Label notStrictEqual1(&builder_);
        Label shouldReturn(&builder_);
        Label shouldContinue(&builder_);

        BRANCH_CIR(builder_.TaggedIsNull(*object), &afterLoop, &loopHead);
        builder_.LoopBegin(&loopHead);
        {
            GateRef isEqual = builder_.Equal(*object, *constructorPrototype);

            BRANCH_CIR(isEqual, &strictEqual1, &notStrictEqual1);
            builder_.Bind(&strictEqual1);
            {
                result = builder_.TaggedTrue();
                builder_.Jump(&exit);
            }
            builder_.Bind(&notStrictEqual1);
            {
                Label objectIsHeapObject(&builder_);
                Label objectIsEcmaObject(&builder_);
                Label objectNotEcmaObject(&builder_);

                BRANCH_CIR(builder_.TaggedIsHeapObject(*object, compilationEnv_),
                           &objectIsHeapObject, &objectNotEcmaObject);
                builder_.Bind(&objectIsHeapObject);
                BRANCH_CIR(builder_.TaggedObjectIsEcmaObject(glue, *object), &objectIsEcmaObject, &objectNotEcmaObject);
                builder_.Bind(&objectNotEcmaObject);
                {
                    GateRef taggedId = builder_.Int32(GET_MESSAGE_STRING_ID(CanNotGetNotEcmaObject));
                    builder_.CallRuntime(glue, RTSTUB_ID(ThrowTypeError), Gate::InvalidGateRef,
                                         { builder_.Int32ToTaggedInt(taggedId) }, gate);
                    result = builder_.ExceptionConstant();
                    builder_.Jump(&exit);
                }
                builder_.Bind(&objectIsEcmaObject);
                {
                    Label objectIsJsProxy(&builder_);
                    Label objectNotIsJsProxy(&builder_);
                    BRANCH_CIR(builder_.IsJsProxy(glue, *object), &objectIsJsProxy, &objectNotIsJsProxy);
                    builder_.Bind(&objectIsJsProxy);
                    {
                        object = builder_.CallRuntime(glue, RTSTUB_ID(CallGetPrototype), Gate::InvalidGateRef,
                                                      { *object }, gate);
                        builder_.Jump(&shouldContinue);
                    }
                    builder_.Bind(&objectNotIsJsProxy);
                    {
                        GateRef objHClass = builder_.LoadHClassByConstOffset(glue, *object);
                        object = builder_.LoadPrototype(glue_, objHClass);
                        builder_.Jump(&shouldContinue);
                    }
                }
            }
            builder_.Bind(&shouldContinue);
            BRANCH_CIR(builder_.TaggedIsNull(*object), &afterLoop, &loopEnd);
        }
        builder_.Bind(&loopEnd);
        builder_.LoopEnd(&loopHead);
        builder_.Bind(&afterLoop);
        {
            result = builder_.TaggedFalse();
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    ReplaceGateWithPendingException(glue, gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedHCRLowering::LowerProtoChangeMarkerCheck(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    auto hclass = builder_.LoadHClassByConstOffset(glue, receiver);
    auto prototype = builder_.LoadPrototype(glue_, hclass);
    auto protoHClass = builder_.LoadHClassByConstOffset(glue, prototype);
    auto marker = builder_.LoadProtoChangeMarker(glue_, protoHClass);
    auto notNull = builder_.TaggedIsNotNull(marker);
    builder_.DeoptCheck(notNull, frameState, DeoptType::PROTOTYPECHANGED2);
    auto hasChanged = builder_.GetHasChanged(marker);
    builder_.DeoptCheck(builder_.BoolNot(hasChanged), frameState, DeoptType::PROTOTYPECHANGED2);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerPrimitiveTypeProtoChangeMarkerCheck(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef prototype = acc_.GetValueIn(gate, 0);
    auto protoHClass = builder_.LoadHClassByConstOffset(glue, prototype);
    auto marker = builder_.LoadProtoChangeMarker(glue_, protoHClass);
    auto notNull = builder_.TaggedIsNotNull(marker);
    builder_.DeoptCheck(notNull, frameState, DeoptType::PRIMTYPEPROTOTYPECHANGED);
    auto hasChanged = builder_.GetHasChanged(marker);
    builder_.DeoptCheck(builder_.BoolNot(hasChanged), frameState, DeoptType::PRIMTYPEPROTOTYPECHANGED);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerMonoLoadPropertyOnProto(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1); // 1: propertyLookupResult
    GateRef hclassIndex = acc_.GetValueIn(gate, 2); // 2: hclassIndex
    GateRef unsharedConstPool = acc_.GetValueIn(gate, 3); // 3: constPool
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    GateRef result = Circuit::NullGate();
    ASSERT(plr.IsLocal() || plr.IsFunction());

    Label loadHolder(&builder_);
    DEFVALUE(current, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    GetPropertyHolderFromProtoChain(glue, { receiver, hclassIndex, unsharedConstPool, frameState },
                                    &loadHolder, &current, DeoptType::INCONSISTENTHCLASS8);
    builder_.Bind(&loadHolder);
    result = LoadPropertyFromHolder(*current, plr);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), result);
}

void TypedHCRLowering::GetPropertyHolderFromProtoChain(
    GateRef glue, ProtoTypeHolderInfo holderInfo, Label *loadHolder, Variable *current, DeoptType deoptType)
{
    if (compilationEnv_->SupportHeapConstant()) {
        auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
        const auto &holderHClassIndex2HeapConstantIndex = jitCompilationEnv->GetHolderHClassIndex2HeapConstantIndex();
        auto holderHClassIndex = acc_.GetConstantValue(holderInfo.holderHClassIndex);
        auto itr = holderHClassIndex2HeapConstantIndex.find(holderHClassIndex);
        if (itr != holderHClassIndex2HeapConstantIndex.end()) {
            *current = builder_.HeapConstant(itr->second);
            builder_.Jump(loadHolder);
            return;
        }
    }
    auto receiverHC = builder_.LoadHClassByConstOffset(glue, holderInfo.receiver);
    *current = builder_.LoadPrototype(glue_, receiverHC);

    // lookup from receiver for holder
    auto holderHC = builder_.LoadHClassFromConstpool(
        holderInfo.unsharedConstPool, acc_.GetConstantValue(holderInfo.holderHClassIndex));
    Label loopHead(&builder_);
    Label lookUpProto(&builder_);
    builder_.Jump(&loopHead);

    builder_.LoopBegin(&loopHead);
    builder_.DeoptCheck(builder_.TaggedIsNotNull(current->ReadVariable()), holderInfo.frameState, deoptType);
    auto curHC = builder_.LoadHClassByConstOffset(glue, current->ReadVariable());
    BRANCH_CIR(builder_.Equal(curHC, holderHC), loadHolder, &lookUpProto);

    builder_.Bind(&lookUpProto);
    *current = builder_.LoadPrototype(glue_, curHC);
    builder_.LoopEnd(&loopHead);
}

void TypedHCRLowering::LowerMonoCallGetterOnProto(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1); // 1: propertyLookupResult
    GateRef hclassIndex = acc_.GetValueIn(gate, 2); // 2: hclassIndex
    GateRef unsharedConstPool = acc_.GetValueIn(gate, 3); // 3: constPool
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    GateRef accessor = Circuit::NullGate();
    GateRef holder = Circuit::NullGate();

    Label loadHolder(&builder_);
    DEFVALUE(current, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    GetPropertyHolderFromProtoChain(glue, { receiver, hclassIndex, unsharedConstPool, frameState },
                                    &loadHolder, &current, DeoptType::INCONSISTENTHCLASS9);

    builder_.Bind(&loadHolder);
    holder = *current;
    accessor = LoadPropertyFromHolder(holder, plr);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    Label isInternalAccessor(&builder_);
    Label notInternalAccessor(&builder_);
    Label callGetter(&builder_);
    Label exit(&builder_);
    BRANCH_CIR(builder_.IsAccessorInternal(glue, accessor), &isInternalAccessor, &notInternalAccessor);
    {
        builder_.Bind(&isInternalAccessor);
        {
            result = builder_.CallRuntime(glue, RTSTUB_ID(CallInternalGetter),
                Gate::InvalidGateRef, { accessor, holder }, gate);
            builder_.Jump(&exit);
        }
        builder_.Bind(&notInternalAccessor);
        {
            GateRef getter = builder_.LoadConstOffset(VariableType::JS_ANY(), accessor, AccessorData::GETTER_OFFSET);
            BRANCH_CIR(builder_.IsSpecial(getter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callGetter);
            builder_.Bind(&callGetter);
            {
                result = CallAccessor(glue, gate, getter, receiver, AccessorMode::GETTER);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), *result);
}

GateRef TypedHCRLowering::LoadPropertyFromHolder(GateRef holder, PropertyLookupResult plr)
{
    GateRef result = Circuit::NullGate();
    GateRef glue = glue_;
    // while loading from iterator result, it may be the case that traversing an array including holes using 'for of',
    // need to convert hole to undefined
    if (plr.IsNotHole() || (compilationEnv_->IsJitCompiler() && !plr.IsLoadFromIterResult())) {
        ASSERT(plr.IsLocal());
        if (plr.IsInlinedProps()) {
            result = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, plr.GetOffset());
        } else {
            auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, JSObject::PROPERTIES_OFFSET);
            result = builder_.GetValueFromTaggedArray(glue, properties, builder_.Int32(plr.GetOffset()));
        }
    } else if (plr.IsLocal()) {
        if (plr.IsInlinedProps()) {
            result = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, plr.GetOffset());
        } else {
            auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, JSObject::PROPERTIES_OFFSET);
            result = builder_.GetValueFromTaggedArray(glue, properties, builder_.Int32(plr.GetOffset()));
        }
        result = builder_.ConvertHoleAsUndefined(result);
    } else {
        UNREACHABLE();
    }
    return result;
}

void TypedHCRLowering::LowerMonoStorePropertyLookUpProto(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1); // 1: propertyLookupResult
    GateRef hclassIndex = acc_.GetValueIn(gate, 2); // 2: hclassIndex
    GateRef unsharedConstPool = acc_.GetValueIn(gate, 3); // 3: constpool
    GateRef value = acc_.GetValueIn(gate, 4); // 4: value
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    bool noBarrier = acc_.IsNoBarrier(gate);

    Label loadHolder(&builder_);
    DEFVALUE(current, (&builder_), VariableType::JS_ANY(), builder_.UndefineConstant());
    GetPropertyHolderFromProtoChain(glue, { receiver, hclassIndex, unsharedConstPool, frameState },
                                    &loadHolder, &current, DeoptType::INCONSISTENTHCLASS10);
    builder_.Bind(&loadHolder);
    Label exit(&builder_);

    builder_.Bind(&loadHolder);
    if (!plr.IsAccessor()) {
        StorePropertyOnHolder(*current, value, plr, noBarrier);
        builder_.Jump(&exit);
    } else {
        GateRef accessor = LoadPropertyFromHolder(*current, plr);
        Label isInternalAccessor(&builder_);
        Label notInternalAccessor(&builder_);
        Label callSetter(&builder_);
        BRANCH_CIR(builder_.IsAccessorInternal(glue, accessor), &isInternalAccessor, &notInternalAccessor);
        {
            builder_.Bind(&isInternalAccessor);
            {
                builder_.CallRuntime(glue, RTSTUB_ID(CallInternalSetter),
                    Gate::InvalidGateRef, { receiver, accessor, value }, gate);
                builder_.Jump(&exit);
            }
            builder_.Bind(&notInternalAccessor);
            {
                GateRef setter =
                    builder_.LoadConstOffset(VariableType::JS_ANY(), accessor, AccessorData::SETTER_OFFSET);
                BRANCH_CIR(builder_.IsSpecial(setter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callSetter);
                builder_.Bind(&callSetter);
                {
                    CallAccessor(glue, gate, setter, receiver, AccessorMode::SETTER, value);
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerMonoStoreProperty(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propertyLookupResult = acc_.GetValueIn(gate, 1); // 1: propertyLookupResult
    GateRef hclassIndex = acc_.GetValueIn(gate, 2); // 2: hclassIndex
    GateRef unsharedConstPool = acc_.GetValueIn(gate, 3); // 3: constPool
    GateRef value = acc_.GetValueIn(gate, 4); // 4: value
    GateRef keyIndex = acc_.GetValueIn(gate, 5); // 5: keyIndex
    bool isPrototype = acc_.TryGetValue(acc_.GetValueIn(gate, 6)); // 6: proto flag Index
    PropertyLookupResult plr(acc_.TryGetValue(propertyLookupResult));
    bool noBarrier = acc_.IsNoBarrier(gate);
    auto receiverHC = builder_.LoadHClassByConstOffset(glue, receiver);
    // transition happened
    Label exit(&builder_);
    Label notProto(&builder_);
    Label isProto(&builder_);
    auto newHolderHC = builder_.LoadHClassFromConstpool(unsharedConstPool, acc_.GetConstantValue(hclassIndex));
    if (compilationEnv_->IsAotCompiler()) {
        auto prototype = builder_.LoadPrototype(glue_, receiverHC);
        builder_.StorePrototype(glue_, newHolderHC, prototype);
    }
    if (isPrototype) {
        builder_.Branch(builder_.IsPrototypeHClass(receiverHC), &isProto, &notProto,
            BranchWeight::ONE_WEIGHT, BranchWeight::DEOPT_WEIGHT, "isPrototypeHClass");
        builder_.Bind(&isProto);

        uint32_t heapConstantIndex = JitCompilationEnv::INVALID_HEAP_CONSTANT_INDEX;
        GateRef propKey = 0;
        if (compilationEnv_->SupportHeapConstant()) {
            auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv_);
            const auto &gate2HeapConstantIndex = jitCompilationEnv->GetGate2HeapConstantIndex();
            auto itr = gate2HeapConstantIndex.find(gate);
            if (itr != gate2HeapConstantIndex.end()) {
                heapConstantIndex = itr->second;
                propKey = builder_.HeapConstant(heapConstantIndex);
            }
        }
        if (heapConstantIndex == JitCompilationEnv::INVALID_HEAP_CONSTANT_INDEX) {
            propKey = builder_.GetObjectByIndexFromConstPool(glue, gate, frameState, keyIndex, ConstPoolType::STRING);
        }
        builder_.CallRuntime(glue, RTSTUB_ID(UpdateAOTHClass), Gate::InvalidGateRef,
            { receiverHC, newHolderHC, propKey }, gate);
        builder_.Jump(&notProto);
        builder_.Bind(&notProto);
    }
    MemoryAttribute mAttr = MemoryAttribute::NeedBarrierAndAtomic();
    builder_.TransitionHClass(glue, receiver, newHolderHC, mAttr);
    if (!plr.IsInlinedProps()) {
        auto properties =
            builder_.LoadConstOffset(VariableType::JS_ANY(), receiver, JSObject::PROPERTIES_OFFSET);
        auto capacity = builder_.LoadConstOffset(VariableType::INT32(), properties, TaggedArray::LENGTH_OFFSET);
        auto index = builder_.Int32(plr.GetOffset());
        Label needExtend(&builder_);
        Label notExtend(&builder_);
        BRANCH_CIR(builder_.Int32UnsignedLessThan(index, capacity), &notExtend, &needExtend);
        builder_.Bind(&notExtend);
        {
            if (!plr.IsAccessor()) {
                StorePropertyOnHolder(receiver, value, plr, noBarrier);
                builder_.Jump(&exit);
            } else {
                GateRef accessor = LoadPropertyFromHolder(receiver, plr);
                Label isInternalAccessor(&builder_);
                Label notInternalAccessor(&builder_);
                Label callSetter(&builder_);
                BRANCH_CIR(builder_.IsAccessorInternal(glue, accessor), &isInternalAccessor, &notInternalAccessor);
                {
                    builder_.Bind(&isInternalAccessor);
                    {
                        builder_.CallRuntime(glue, RTSTUB_ID(CallInternalSetter),
                            Gate::InvalidGateRef, { receiver, accessor, value }, gate);
                        builder_.Jump(&exit);
                    }
                    builder_.Bind(&notInternalAccessor);
                    {
                        GateRef setter =
                            builder_.LoadConstOffset(VariableType::JS_ANY(), accessor, AccessorData::SETTER_OFFSET);
                        BRANCH_CIR(builder_.IsSpecial(setter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callSetter);
                        builder_.Bind(&callSetter);
                        {
                            CallAccessor(glue, gate, setter, receiver, AccessorMode::SETTER, value);
                            builder_.Jump(&exit);
                        }
                    }
                }
            }
        }
        builder_.Bind(&needExtend);
        {
            builder_.CallRuntime(glue,
                RTSTUB_ID(PropertiesSetValue),
                Gate::InvalidGateRef,
                { receiver, value, properties, builder_.Int32ToTaggedInt(capacity),
                builder_.Int32ToTaggedInt(index) }, gate);
            builder_.Jump(&exit);
        }
    } else {
        if (!plr.IsAccessor()) {
            StorePropertyOnHolder(receiver, value, plr, noBarrier);
            builder_.Jump(&exit);
        } else {
            GateRef accessor = LoadPropertyFromHolder(receiver, plr);
            Label isInternalAccessor(&builder_);
            Label notInternalAccessor(&builder_);
            Label callSetter(&builder_);
            BRANCH_CIR(builder_.IsAccessorInternal(glue, accessor), &isInternalAccessor, &notInternalAccessor);
            {
                builder_.Bind(&isInternalAccessor);
                {
                    builder_.CallRuntime(glue, RTSTUB_ID(CallInternalSetter),
                        Gate::InvalidGateRef, { receiver, accessor, value }, gate);
                    builder_.Jump(&exit);
                }
                builder_.Bind(&notInternalAccessor);
                {
                    GateRef setter =
                        builder_.LoadConstOffset(VariableType::JS_ANY(), accessor, AccessorData::SETTER_OFFSET);
                    BRANCH_CIR(builder_.IsSpecial(setter, JSTaggedValue::VALUE_UNDEFINED), &exit, &callSetter);
                    builder_.Bind(&callSetter);
                    {
                        CallAccessor(glue, gate, setter, receiver, AccessorMode::SETTER, value);
                        builder_.Jump(&exit);
                    }
                }
            }
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::StorePropertyOnHolder(GateRef holder, GateRef value, PropertyLookupResult plr, bool noBarrier)
{
    if (!noBarrier) {
        if (plr.IsInlinedProps()) {
            builder_.StoreConstOffset(VariableType::JS_ANY(), holder, plr.GetOffset(), value);
        } else {
            auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, JSObject::PROPERTIES_OFFSET);
            builder_.SetValueToTaggedArray(
                VariableType::JS_ANY(), glue_, properties, builder_.Int32(plr.GetOffset()), value);
        }
    } else {
        if (plr.IsInlinedProps()) {
            builder_.StoreConstOffset(GetVarType(plr), holder, plr.GetOffset(), value);
        } else {
            auto properties = builder_.LoadConstOffset(VariableType::JS_ANY(), holder, JSObject::PROPERTIES_OFFSET);
            builder_.SetValueToTaggedArray(
                GetVarType(plr), glue_, properties, builder_.Int32(plr.GetOffset()), value);
        }
    }
}

static GateRef GetOldObject(CompilationEnv *compilationEnv, CircuitBuilder &builder,
    const GateAccessor &acc, GateRef glue, GateRef gate)
{
    if (compilationEnv->SupportHeapConstant()) {
        auto *jitCompilationEnv = static_cast<JitCompilationEnv*>(compilationEnv);
        const auto &gate2HeapConstantIndex = jitCompilationEnv->GetGate2HeapConstantIndex();
        auto itr = gate2HeapConstantIndex.find(gate);
        // object must be found in unshared constpool,
        // which is ensured by TypedBytecodeLowering::LowerCreateObjectWithBuffer
        ASSERT(itr != gate2HeapConstantIndex.end());
        return builder.HeapConstant(itr->second);
    }
    GateRef frameState = acc.GetFrameState(gate);
    GateRef index = acc.GetValueIn(gate, 1); // 1: index
    return builder.GetObjectByIndexFromConstPool(
        glue, gate, frameState, builder.TruncInt64ToInt32(index), ConstPoolType::OBJECT_LITERAL);
}

void TypedHCRLowering::LowerTypedCreateObjWithBuffer(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    DEFVALUE(ret, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label equal(&builder_);
    Label notEqual(&builder_);
    Label exit(&builder_);
    GateRef objSize = acc_.GetValueIn(gate, 0); // 0: objSize
    GateRef lexEnv = acc_.GetValueIn(gate, 3); // 3: lexenv
    size_t numValueIn = acc_.GetNumValueIn(gate);

    GateRef oldObj = GetOldObject(compilationEnv_, builder_, acc_, glue, gate);
    GateRef hclass = builder_.LoadHClassByConstOffset(glue, oldObj);
    GateRef emptyArray = builder_.GetGlobalConstantValue(ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    GateRef objectSize = builder_.GetObjectSizeFromHClass(hclass);
    BRANCH_CIR(builder_.Equal(objectSize, objSize), &equal, &notEqual);
    builder_.Bind(&equal);
    {
        builder_.StartAllocate();
        GateRef newObj = builder_.HeapAlloc(glue, objSize, GateType::TaggedValue(), RegionSpaceFlag::IN_YOUNG_SPACE);
        builder_.StoreHClass(glue, newObj, hclass, MemoryAttribute::NeedBarrierAndAtomic());
        builder_.StoreConstOffset(VariableType::INT64(), newObj,
            JSObject::HASH_OFFSET, builder_.Int64(JSTaggedValue(0).GetRawData()));
        builder_.StoreConstOffset(VariableType::INT64(), newObj, JSObject::PROPERTIES_OFFSET, emptyArray);
        builder_.StoreConstOffset(VariableType::INT64(), newObj, JSObject::ELEMENTS_OFFSET, emptyArray);
        size_t fixedNumValueIn = 4; // objSize, index, hclass, lexEnv
        for (uint32_t i = 0; i < numValueIn - fixedNumValueIn; i += 2) { // 2 : value, offset
            builder_.StoreConstOffset(VariableType::INT64(), newObj,
                acc_.GetConstantValue(acc_.GetValueIn(gate, i + fixedNumValueIn + 1)),
                acc_.GetValueIn(gate, i + fixedNumValueIn));
        }
        ret = builder_.FinishAllocate(newObj);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notEqual);
    {
        ret = builder_.CallRuntime(glue, RTSTUB_ID(CreateObjectHavingMethod),
            Gate::InvalidGateRef, { oldObj, lexEnv }, gate);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *ret);
}

void TypedHCRLowering::LowerStringFromSingleCharCode(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    DEFVALUE(value, (&builder_), VariableType::INT16(), builder_.Int16(0));
    DEFVALUE(res, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label isPendingException(&builder_);
    Label noPendingException(&builder_);
    Label exit(&builder_);
    Label isInt(&builder_);
    Label notInt(&builder_);
    Label newObj(&builder_);
    Label canBeCompress(&builder_);
    Label canNotBeCompress(&builder_);
    GateRef codePointTag = acc_.GetValueIn(gate);
    GateRef codePointValue = builder_.ToNumber(gate, codePointTag, glue);
    BRANCH_CIR(builder_.HasPendingException(glue, compilationEnv_), &isPendingException, &noPendingException);
    builder_.Bind(&isPendingException);
    {
        res = builder_.ExceptionConstant();
        builder_.Jump(&exit);
    }
    builder_.Bind(&noPendingException);
    {
        BRANCH_CIR(builder_.TaggedIsInt(codePointValue), &isInt, &notInt);
        builder_.Bind(&isInt);
        {
            value = builder_.TruncInt32ToInt16(builder_.GetInt32OfTInt(codePointValue));
            builder_.Jump(&newObj);
        }
        builder_.Bind(&notInt);
        {
            value = builder_.TruncInt32ToInt16(
                builder_.TruncDoubleToInt(glue, builder_.GetDoubleOfTDouble(codePointValue), base::INT16_BITS));
            builder_.Jump(&newObj);
        }
        builder_.Bind(&newObj);
        BRANCH_CIR(builder_.IsASCIICharacter(builder_.ZExtInt16ToInt32(*value)), &canBeCompress, &canNotBeCompress);
        NewObjectStubBuilder newBuilder(&env);
        newBuilder.SetParameters(glue, 0);
        builder_.Bind(&canBeCompress);
        {
            GateRef singleCharTable = builder_.GetGlobalConstantValue(ConstantIndex::SINGLE_CHAR_TABLE_INDEX);
            res = builder_.GetValueFromTaggedArray(glue, singleCharTable, builder_.ZExtInt16ToInt32(*value));
            builder_.Jump(&exit);
        }
        builder_.Bind(&canNotBeCompress);
        {
            Label afterNew1(&builder_);
            newBuilder.AllocLineStringObject(&res, &afterNew1, builder_.Int32(1), false);
            builder_.Bind(&afterNew1);
            {
                GateRef dst = builder_.ChangeTaggedPointerToInt64(
                    builder_.PtrAdd(*res, builder_.IntPtr(LineString::DATA_OFFSET)));
                builder_.Store(VariableType::INT16(), glue, dst, builder_.IntPtr(0), *value);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *res);
}

void TypedHCRLowering::LowerMigrateArrayWithKind(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    Label exit(&builder_);
    GateRef object = acc_.GetValueIn(gate, 0);
    GateRef oldKind = acc_.GetValueIn(gate, 1); // 1: objSize
    GateRef newKind = acc_.GetValueIn(gate, 2); // 2: index

    DEFVALUE(newElements, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label doMigration(&builder_);
    Label migrateFromInt(&builder_);
    Label migrateOtherKinds(&builder_);
    GateRef noNeedMigration = LogicOrBuilder(&env)
        .Or(builder_.Int32Equal(oldKind, newKind))
        .Or(builder_.BitAnd(builder_.Int32Equal(oldKind, builder_.Int32(Elements::ToUint(ElementsKind::INT))),
            builder_.Int32Equal(newKind, builder_.Int32(Elements::ToUint(ElementsKind::HOLE_INT)))))
        .Or(builder_.BitAnd(builder_.Int32Equal(oldKind, builder_.Int32(Elements::ToUint(ElementsKind::NUMBER))),
            builder_.Int32Equal(newKind, builder_.Int32(Elements::ToUint(ElementsKind::HOLE_NUMBER)))))
        .Done();
    BRANCH_CIR(noNeedMigration, &exit, &doMigration);
    builder_.Bind(&doMigration);

    GateRef needCOW = builder_.IsJsCOWArray(glue, object, compilationEnv_);
    BRANCH_CIR(builder_.ElementsKindIsIntOrHoleInt(oldKind), &migrateFromInt, &migrateOtherKinds);
    builder_.Bind(&migrateFromInt);
    {
        Label migrateToHeapValuesFromInt(&builder_);
        Label migrateToRawValuesFromInt(&builder_);
        Label migrateToNumbersFromInt(&builder_);
        BRANCH_CIR(builder_.ElementsKindIsHeapKind(newKind),
                   &migrateToHeapValuesFromInt, &migrateToRawValuesFromInt);
        builder_.Bind(&migrateToHeapValuesFromInt);
        {
            newElements = builder_.MigrateFromRawValueToHeapValues(object, needCOW, builder_.True());
            builder_.StoreConstOffset(VariableType::JS_ANY(), object, JSObject::ELEMENTS_OFFSET, *newElements);
            builder_.Jump(&exit);
        }
        builder_.Bind(&migrateToRawValuesFromInt);
        {
            BRANCH_CIR(builder_.ElementsKindIsNumOrHoleNum(newKind), &migrateToNumbersFromInt, &exit);
            builder_.Bind(&migrateToNumbersFromInt);
            {
                builder_.MigrateFromHoleIntToHoleNumber(object);
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&migrateOtherKinds);
    {
        Label migrateFromNumber(&builder_);
        Label migrateToHeapValuesFromNum(&builder_);
        Label migrateToRawValuesFromNum(&builder_);
        Label migrateToIntFromNum(&builder_);
        Label migrateToRawValueFromTagged(&builder_);
        BRANCH_CIR(builder_.ElementsKindIsNumOrHoleNum(oldKind), &migrateFromNumber, &migrateToRawValueFromTagged);
        builder_.Bind(&migrateFromNumber);
        {
            BRANCH_CIR(builder_.ElementsKindIsHeapKind(newKind),
                       &migrateToHeapValuesFromNum, &migrateToRawValuesFromNum);
            builder_.Bind(&migrateToHeapValuesFromNum);
            {
                Label migrateToTaggedFromNum(&builder_);
                BRANCH_CIR(builder_.ElementsKindIsHeapKind(newKind), &migrateToTaggedFromNum, &exit);
                builder_.Bind(&migrateToTaggedFromNum);
                {
                    newElements = builder_.MigrateFromRawValueToHeapValues(object, needCOW, builder_.False());
                    builder_.StoreConstOffset(VariableType::JS_ANY(), object, JSObject::ELEMENTS_OFFSET, *newElements);
                    builder_.Jump(&exit);
                }
            }
            builder_.Bind(&migrateToRawValuesFromNum);
            {
                BRANCH_CIR(builder_.ElementsKindIsIntOrHoleInt(newKind), &migrateToIntFromNum, &exit);
                builder_.Bind(&migrateToIntFromNum);
                {
                    builder_.MigrateFromHoleNumberToHoleInt(object);
                    builder_.Jump(&exit);
                }
            }
        }
        builder_.Bind(&migrateToRawValueFromTagged);
        {
            Label migrateToIntFromTagged(&builder_);
            Label migrateToOthersFromTagged(&builder_);
            BRANCH_CIR(builder_.ElementsKindIsIntOrHoleInt(newKind),
                       &migrateToIntFromTagged, &migrateToOthersFromTagged);
            builder_.Bind(&migrateToIntFromTagged);
            {
                newElements = builder_.MigrateFromHeapValueToRawValue(object, needCOW, builder_.True());
                builder_.StoreConstOffset(VariableType::JS_ANY(), object, JSObject::ELEMENTS_OFFSET, *newElements);
                builder_.Jump(&exit);
            }
            builder_.Bind(&migrateToOthersFromTagged);
            {
                Label migrateToNumFromTagged(&builder_);
                BRANCH_CIR(builder_.ElementsKindIsNumOrHoleNum(newKind), &migrateToNumFromTagged, &exit);
                builder_.Bind(&migrateToNumFromTagged);
                {
                    newElements = builder_.MigrateFromHeapValueToRawValue(object, needCOW, builder_.False());
                    builder_.StoreConstOffset(VariableType::JS_ANY(), object, JSObject::ELEMENTS_OFFSET, *newElements);
                    builder_.Jump(&exit);
                }
            }
        }
    }
    builder_.Bind(&exit);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerNumberToString(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    DEFVALUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    GateRef number = acc_.GetValueIn(gate, 0);
    result = builder_.CallRuntime(glue, RTSTUB_ID(NumberToString),
        Gate::InvalidGateRef, { number }, gate);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), *result);
}

void TypedHCRLowering::LowerEcmaObjectCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    GateRef frameState = acc_.GetFrameState(gate);
    builder_.HeapObjectCheck(value, frameState);
    builder_.HeapObjectIsEcmaObjectCheck(value, frameState);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerElementskindCheck(GateRef glue, GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef frameState = acc_.GetFrameState(gate);
    ArrayMetaDataAccessor accessor = acc_.GetArrayMetaDataAccessor(gate);
    ElementsKind kind = accessor.GetElementsKind();
    GateRef check = Circuit::NullGate();
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef hclass = builder_.LoadHClassByConstOffset(glue, receiver);

    if (Elements::IsComplex(kind)) {
        check = builder_.Int32GreaterThanOrEqual(builder_.Int32(Elements::ToUint(kind)),
                                                 builder_.GetElementsKindByHClass(hclass));
    } else {
        check = builder_.Equal(builder_.Int32(Elements::ToUint(kind)), builder_.GetElementsKindByHClass(hclass));
    }
    builder_.DeoptCheck(check, frameState, DeoptType::INCONSISTENTELEMENTSKIND);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void TypedHCRLowering::LowerCheckConstructor(GateRef gate, GateRef glue)
{
    Environment env(gate, circuit_, &builder_);
    NewObjectStubBuilder objBuilder(&env);
    GateRef superFunc = acc_.GetValueIn(gate, 0); // 0 : Super func
    GateRef ctorResult = acc_.GetValueIn(gate, 1); // 1: Constructor result
    GateRef thisObj = acc_.GetValueIn(gate, 2); // 2: This object
    GateRef result = objBuilder.ConstructorCheck(glue, superFunc, ctorResult, thisObj);
    ReplaceHirWithPendingException(gate, glue, builder_.GetState(), builder_.GetDepend(), result);
}
}  // namespace panda::ecmascript::kungfu
