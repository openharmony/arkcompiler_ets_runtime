/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_TYPED_HCR_LOWERING_H
#define ECMASCRIPT_COMPILER_TYPED_HCR_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/type_info_accessors.h"
#include "ecmascript/compiler/pass_manager.h"

namespace panda::ecmascript::kungfu {
// TypeHCRLowering Process
// SW: state wire, DW: depend wire, VW: value wire
// Before Type Lowering:
//                                    SW   DW   VW
//                                    |    |    |
//                                    |    |    |
//                                    v    v    v
//                                +-------------------+
//                                |       (HIR)       |    SW     +--------------+
//                            --DW|    JS_BYTECODE    |---------->| IF_EXCEPTION |
//                            |   +-------------------+           +--------------+
//                            |            SW       VW
//                            |            |        |
//                            |            v        |
//                            |    +--------------+ |
//                            |    |  IF_SUCCESS  | |
//                            |    +--------------+ |
//                            |            SW       |
//                            |            |        |
//                            |            v        v
//                            |   +-------------------+
//                            |   |       (HIR)       |
//                            --->|    JS_BYTECODE    |
//                                +-------------------+
//
// After Type Lowering:
//                                           SW
//                                           |
//                                           v
//                                 +-------------------+
//                                 |     IF_BRANCH     |
//                                 |    (Type Check)   |
//                                 +-------------------+
//                                    SW            SW
//                                    |             |
//                                    V             V
//                            +--------------+  +--------------+
//                            |    IF_TRUE   |  |   IF_FALSE   |
//                            +--------------+  +--------------+
//                 VW   DW          SW               SW                   DW   VW
//                 |    |           |                |                    |    |
//                 |    |           V                V                    |    |
//                 |    |  +---------------+     +---------------------+  |    |
//                 ------->|   FAST PATH   |     |        (HIR)        |<-------
//                         +---------------+     |     JS_BYTECODE     |
//                            VW  DW   SW        +---------------------+
//                            |   |    |               SW         VW  DW
//                            |   |    |               |          |   |
//                            |   |    |               v          |   |
//                            |   |    |         +--------------+ |   |
//                            |   |    |         |  IF_SUCCESS  | |   |
//                            |   |    |         +--------------+ |   |
//                            |   |    |                SW        |   |
//                            |   |    |                |         |   |
//                            |   |    v                v         |   |
//                            |   |  +---------------------+      |   |
//                            |   |  |        MERGE        |      |   |
//                            |   |  +---------------------+      |   |
//                            |   |    SW         SW    SW        |   |
//                            ----|----|----------|-----|--       |   |
//                             ---|----|----------|-----|-|-------|----
//                             |  |    |          |     | |       |
//                             v  v    v          |     v v       v
//                            +-----------------+ | +----------------+
//                            | DEPEND_SELECTOR | | | VALUE_SELECTOR |
//                            +-----------------+ | +----------------+
//                                    DW          |        VW
//                                    |           |        |
//                                    v           v        v
//                                  +------------------------+
//                                  |         (HIR)          |
//                                  |      JS_BYTECODE       |
//                                  +------------------------+

class TypedHCRLowering : public PassVisitor {
public:
    TypedHCRLowering(Circuit* circuit,
                     CompilationEnv *env,
                     RPOVisitor* visitor,
                     CompilationConfig* cmpCfg,
                     Chunk* chunk,
                     bool enableLoweringBuiltin)
        : PassVisitor(circuit, chunk, visitor),
          circuit_(circuit),
          compilationEnv_(env),
          acc_(circuit),
          builder_(circuit, cmpCfg),
          dependEntry_(circuit->GetDependRoot()),
          enableLoweringBuiltin_(enableLoweringBuiltin),
          traceBuiltins_(env != nullptr ? env->GetJSOptions().GetTraceBuiltins() : false)
    {
    }

    ~TypedHCRLowering() = default;

    GateRef VisitGate(GateRef gate) override;

private:
    void Lower(GateRef gate);
    void LowerType(GateRef gate);
    void LowerPrimitiveTypeCheck(GateRef gate);
    void LowerTypeConvert(GateRef gate);
    void LowerPrimitiveToNumber(GateRef dst, GateRef src, ParamType srcType);
    void LowerIntCheck(GateRef gate);
    void LowerDoubleCheck(GateRef gate);
    void LowerNumberCheck(GateRef gate);
    void LowerBooleanCheck(GateRef gate);
    void LowerIndexCheck(GateRef gate);
    void LowerObjectTypeCheck(GateRef gate);
    void LowerSimpleHClassCheck(GateRef gate);
    void LowerStableArrayCheck(GateRef gate);
    void LowerTypedArrayCheck(GateRef gate);
    void LowerEcmaStringCheck(GateRef gate);
    void LowerInternStringCheck(GateRef gate);
    void LowerEcmaMapCheck(GateRef gate);
    void LowerFlattenTreeStringCheck(GateRef gate, GateRef glue);
    void LowerLoadTypedArrayLength(GateRef gate);
    void LowerStringLength(GateRef gate);
    void LowerMapSize(GateRef gate);
    void LowerCallPrivateGetter(GateRef gate, GateRef glue);
    void LowerCallPrivateSetter(GateRef gate, GateRef glue);
    void LowerLoadProperty(GateRef gate);
    void LowerCallGetter(GateRef gate, GateRef glue);
    void LowerStoreProperty(GateRef gate);
    void LowerCallSetter(GateRef gate, GateRef glue);
    void LowerLoadArrayLength(GateRef gate);
    void LowerStoreElement(GateRef gate, GateRef glue);
    void LowerLoadElement(GateRef gate);
    void LowerLoadFromTaggedArray(GateRef gate);
    void LowerStoreToTaggedArray(GateRef gate, GateRef glue);
    void LowerRangeCheckPredicate(GateRef gate);
    void LowerBuiltinPrototypeHClassCheck(GateRef gate);
    void BuiltinInstanceStringTypeCheck(GateRef gate);
    void LowerLoadBuiltinObject(GateRef gate);
    void LowerTypedCreateObjWithBuffer(GateRef gate, GateRef glue);
    void LowerNumberToString(GateRef gate, GateRef glue);

    enum class ArrayState : uint8_t {
        PACKED = 0,
        HOLEY,
    };
    void LowerArrayLoadElement(GateRef gate, ArrayState arrayState, TypedLoadOp op);
    void LowerCowArrayCheck(GateRef gate, GateRef glue);
    void LowerTypedArrayLoadElement(GateRef gate, BuiltinTypeId id);
    void LowerStringLoadElement(GateRef gate);
    GateRef BuildOnHeapTypedArrayLoadElement(GateRef receiver, GateRef offset, VariableType type);
    GateRef BuildNotOnHeapTypedArrayLoadElement(GateRef receiver, GateRef offset, VariableType type);
    GateRef BuildTypedArrayLoadElement(GateRef receiver, GateRef offset, VariableType type, Label *isByteArray,
                                       Label *isArrayBuffer, Label *exit);
    void LowerArrayStoreElement(GateRef gate, GateRef glue, TypedStoreOp kind);
    void LowerTypedArrayStoreElement(GateRef gate, BuiltinTypeId id);
    void OptStoreElementByOnHeapMode(GateRef gate, GateRef receiver, GateRef offset, GateRef value, Label *isByteArray,
                                     Label *isArrayBuffer, Label *exit);
    void BuildOnHeapTypedArrayStoreElement(GateRef receiver, GateRef offset, GateRef value);
    void BuildNotOnHeapTypedArrayStoreElement(GateRef receiver, GateRef offset, GateRef value);
    void BuildTypedArrayStoreElement(GateRef receiver, GateRef offset, GateRef value, Label *isByteArray,
                                     Label *isArrayBuffer, Label *exit);
    void LowerUInt8ClampedArrayStoreElement(GateRef gate);
    void LowerTypedCallBuitin(GateRef gate);
    void LowerCallTargetCheck(GateRef gate);
    void LowerJSCallTargetCheck(GateRef gate);
    void LowerCallTargetIsCompiledCheck(GateRef gate);
    void CallTargetIsCompiledCheck(GateRef func, GateRef frameState, Label *checkAlreadyDeopt, Label *exit);
    void LowerJSCallTargetFromDefineFuncCheck(GateRef gate);
    void LowerJSCallTargetTypeCheck(GateRef gate);
    void LowerJSFastCallTargetTypeCheck(GateRef gate);
    void LowerJSCallThisTargetTypeCheck(GateRef gate);
    void LowerJSFastCallThisTargetTypeCheck(GateRef gate);
    void LowerJSNoGCCallThisTargetTypeCheck(GateRef gate);
    void LowerJSNoGCFastCallThisTargetTypeCheck(GateRef gate);
    void LowerJSNewObjRangeCallTargetCheck(GateRef gate);
    void LowerTypedNewAllocateThis(GateRef gate, GateRef glue);
    void LowerTypedSuperAllocateThis(GateRef gate, GateRef glue);
    void LowerGetSuperConstructor(GateRef gate);
    void LowerJSInlineTargetTypeCheck(GateRef gate);
    void LowerJSInlineTargetHeapConstantCheck(GateRef gate);
    void SetDeoptTypeInfo(JSType jstype, DeoptType &type, size_t &typedArrayRootHclassIndex,
        size_t &typedArrayRootHclassOnHeapIndex);
    void LowerLookupHolder(GateRef gate);
    void LowerLoadGetter(GateRef gate);
    void LowerLoadSetter(GateRef gate);
    void LowerPrototypeCheck(GateRef gate);
    void LowerStringEqual(GateRef gate, GateRef glue);
    void LowerTypeOfCheck(GateRef gate);
    void LowerTypeOf(GateRef gate, GateRef glue);
    void LowerArrayConstructorCheck(GateRef gate, GateRef glue);
    void NewArrayConstructorWithNoArgs(GateRef gate, GateRef glue);
    void LowerArrayConstructor(GateRef gate, GateRef glue);
    void LowerFloat32ArrayConstructorCheck(GateRef gate, GateRef glue);
    void NewFloat32ArrayConstructorWithNoArgs(GateRef gate, GateRef glue);
    void ConvertFloat32ArrayConstructorLength(GateRef len, Variable *arrayLength,
                                              Label *arrayCreate, Label *slowPath);
    void LowerFloat32ArrayConstructor(GateRef gate, GateRef glue);
    void LowerObjectConstructorCheck(GateRef gate, GateRef glue);
    void LowerObjectConstructor(GateRef gate, GateRef glue);
    void LowerBooleanConstructorCheck(GateRef gate, GateRef glue);
    void LowerBooleanConstructor(GateRef gate, GateRef glue);
    GateRef NewJSPrimitiveRef(PrimitiveType type, GateRef glue, GateRef value);
    void ReplaceGateWithPendingException(GateRef glue, GateRef gate, GateRef state, GateRef depend, GateRef value);
    void LowerOrdinaryHasInstance(GateRef gate, GateRef glue);
    void LowerProtoChangeMarkerCheck(GateRef gate);
    void LowerMonoCallGetterOnProto(GateRef gate, GateRef glue);
    void LowerMonoLoadPropertyOnProto(GateRef gate);
    void LowerMonoStorePropertyLookUpProto(GateRef gate, GateRef glue);
    void LowerMonoStoreProperty(GateRef gate, GateRef glue);
    void LowerStringFromSingleCharCode(GateRef gate, GateRef glue);
    void LowerMigrateArrayWithKind(GateRef gate);
    void LowerEcmaObjectCheck(GateRef gate);
    void LowerElementskindCheck(GateRef gate);

    GateRef LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                             bool useLabel = false);

    enum AccessorMode {
        GETTER,
        SETTER,
    };

    GateRef CallAccessor(GateRef glue, GateRef gate, GateRef function, GateRef receiver, AccessorMode mode,
                         GateRef value = Circuit::NullGate());
    void BuiltinInstanceHClassCheck(Environment *env, GateRef gate);
    void BuiltinPrototypeHClassCheck(Environment *env, GateRef gate);
    void ReplaceHirWithPendingException(GateRef hirGate, GateRef glue, GateRef state, GateRef depend, GateRef value);

    GateRef DoubleToTaggedDoublePtr(GateRef gate);
    GateRef ChangeInt32ToFloat64(GateRef gate);
    GateRef TruncDoubleToInt(GateRef gate);
    GateRef IntToTaggedIntPtr(GateRef x);
    GateType GetLeftType(GateRef gate);
    GateType GetRightType(GateRef gate);
    GateRef GetElementSize(BuiltinTypeId id);
    VariableType GetVariableType(BuiltinTypeId id);

    GateRef GetFrameState(GateRef gate) const
    {
        return acc_.GetFrameState(gate);
    }

    VariableType GetVarType(PropertyLookupResult plr);
    GateRef GetLengthFromSupers(GateRef supers);
    GateRef GetValueFromSupers(GateRef supers, size_t index);
    GateRef LoadFromTaggedArray(GateRef array, size_t index);
    GateRef LoadFromConstPool(GateRef unsharedConstPool, size_t index, size_t valVecType);
    GateRef GetLengthFromString(GateRef gate);
    GateRef LoadPropertyFromHolder(GateRef holder, PropertyLookupResult plr);
    void StorePropertyOnHolder(GateRef holder, GateRef value, PropertyLookupResult plr, bool needBarrier);

    Circuit *circuit_;
    CompilationEnv *compilationEnv_ {nullptr};
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_;
    bool enableLoweringBuiltin_ {false};
    bool traceBuiltins_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPED_HCR_LOWERING_H
