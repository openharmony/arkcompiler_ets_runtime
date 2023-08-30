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

#ifndef ECMASCRIPT_COMPILER_TYPE_MCR_LOWERING_H
#define ECMASCRIPT_COMPILER_TYPE_MCR_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/combined_pass_visitor.h"

namespace panda::ecmascript::kungfu {
// TypeMCRLowering Process
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

class TypeMCRLowering : public PassVisitor {
public:
    TypeMCRLowering(Circuit *circuit, RPOVisitor *visitor,
                    CompilationConfig *cmpCfg, TSManager *tsManager, Chunk *chunk, bool onHeapCheck)
        : PassVisitor(circuit, chunk, visitor), circuit_(circuit), acc_(circuit), builder_(circuit, cmpCfg),
          dependEntry_(circuit->GetDependRoot()), tsManager_(tsManager), onHeapCheck_(onHeapCheck) {}

    ~TypeMCRLowering() = default;

    GateRef VisitGate(GateRef gate) override;

private:
    bool IsOnHeap() const
    {
        return onHeapCheck_;
    }
    void Lower(GateRef gate);
    void LowerType(GateRef gate);
    void LowerPrimitiveTypeCheck(GateRef gate);
    void LowerTypeConvert(GateRef gate);
    void LowerPrimitiveToNumber(GateRef dst, GateRef src, GateType srcType);
    void LowerIntCheck(GateRef gate);
    void LowerDoubleCheck(GateRef gate);
    void LowerNumberCheck(GateRef gate);
    void LowerBooleanCheck(GateRef gate);
    void LowerIndexCheck(GateRef gate);
    void LowerObjectTypeCheck(GateRef gate);
    void LowerSimpleHClassCheck(GateRef gate);
    void LowerTSSubtypingCheck(GateRef gate);
    void LowerObjectTypeCompare(GateRef gate);
    void LowerSimpleHClassCompare(GateRef gate);
    void LowerTSSubtypingCompare(GateRef gate);
    GateRef BuildCompareSubTyping(GateRef gate, GateRef frameState, Label *levelValid, Label *exit);
    GateRef BuildCompareHClass(GateRef gate, GateRef frameState);
    void BuildCompareSubTyping(GateRef gate);
    void LowerStableArrayCheck(GateRef gate);
    void LowerTypedArrayCheck(GateRef gate);
    void LowerEcmaStringCheck(GateRef gate);
    void LowerFlattenStringCheck(GateRef gate, GateRef glue);
    void LowerLoadTypedArrayLength(GateRef gate);
    void LowerStringLength(GateRef gate);
    void LowerLoadProperty(GateRef gate);
    void LowerCallGetter(GateRef gate, GateRef glue);
    void LowerStoreProperty(GateRef gate);
    void LowerCallSetter(GateRef gate, GateRef glue);
    void LowerLoadArrayLength(GateRef gate);
    void LowerStoreElement(GateRef gate, GateRef glue);
    void LowerLoadElement(GateRef gate);
    void LowerLoadFromTaggedArray(GateRef gate);
    void LowerStoreToTaggedArray(GateRef gate, GateRef glue);

    enum class ArrayState : uint8_t {
        PACKED = 0,
        HOLEY,
    };
    void LowerArrayLoadElement(GateRef gate, ArrayState arrayState);
    void LowerCowArrayCheck(GateRef gate, GateRef glue);
    void LowerTypedArrayLoadElement(GateRef gate, BuiltinTypeId id);
    void LowerStringLoadElement(GateRef gate);
    GateRef BuildOnHeapTypedArrayLoadElement(GateRef receiver, GateRef offset, VariableType type);
    GateRef BuildTypedArrayLoadElement(GateRef receiver, GateRef offset, VariableType type, Label *isByteArray,
                                       Label *isArrayBuffer, Label *exit);
    void LowerArrayStoreElement(GateRef gate, GateRef glue);
    void LowerTypedArrayStoreElement(GateRef gate, BuiltinTypeId id);
    void BuildOnHeapTypedArrayStoreElement(GateRef receiver, GateRef offset, GateRef value);
    void BuildTypedArrayStoreElement(GateRef receiver, GateRef offset, GateRef value, Label *isByteArray,
                                     Label *isArrayBuffer, Label *exit);
    void LowerUInt8ClampedArrayStoreElement(GateRef gate);
    void LowerTypedCallBuitin(GateRef gate);
    void LowerCallTargetCheck(GateRef gate);
    void LowerJSCallTargetCheck(GateRef gate);
    void LowerJSCallTargetFromDefineFuncCheck(GateRef gate);
    void LowerJSCallTargetTypeCheck(GateRef gate);
    void LowerJSFastCallTargetTypeCheck(GateRef gate);
    void LowerJSCallThisTargetTypeCheck(GateRef gate);
    void LowerJSFastCallThisTargetTypeCheck(GateRef gate);
    void LowerJSNoGCCallThisTargetTypeCheck(GateRef gate);
    void LowerJSNoGCFastCallThisTargetTypeCheck(GateRef gate);
    void LowerTypedNewAllocateThis(GateRef gate, GateRef glue);
    void LowerTypedSuperAllocateThis(GateRef gate, GateRef glue);
    void LowerGetSuperConstructor(GateRef gate);
    void LowerJSInlineTargetTypeCheck(GateRef gate);
    void SetDeoptTypeInfo(BuiltinTypeId id, DeoptType &type, size_t &funcIndex);
    void LowerLoadGetter(GateRef gate);
    void LowerLoadSetter(GateRef gate);
    void LowerInlineAccessorCheck(GateRef gate);

    GateRef LowerCallRuntime(GateRef glue, GateRef hirGate, int index, const std::vector<GateRef> &args,
                             bool useLabel = false);

    enum AccessorMode {
        GETTER,
        SETTER,
    };

    GateRef CallAccessor(GateRef glue, GateRef gate, GateRef function, GateRef receiver, AccessorMode mode,
                         GateRef value = Circuit::NullGate());
    void ReplaceHirWithPendingException(GateRef hirGate, GateRef glue, GateRef state, GateRef depend, GateRef value);

    GateRef DoubleToTaggedDoublePtr(GateRef gate);
    GateRef ChangeInt32ToFloat64(GateRef gate);
    GateRef TruncDoubleToInt(GateRef gate);
    GateRef IntToTaggedIntPtr(GateRef x);
    GateType GetLeftType(GateRef gate);
    GateType GetRightType(GateRef gate);
    GateRef GetObjectFromConstPool(GateRef jsFunc, GateRef index);
    GateRef GetElementSize(BuiltinTypeId id);
    VariableType GetVariableType(BuiltinTypeId id);

    GateRef GetFrameState(GateRef gate) const
    {
        return acc_.GetFrameState(gate);
    }

    VariableType GetVarType(PropertyLookupResult plr);
    GateRef LoadSupers(GateRef hclass);
    GateRef GetLengthFromSupers(GateRef supers);
    GateRef GetValueFromSupers(GateRef supers, size_t index);
    GateRef LoadFromTaggedArray(GateRef array, size_t index);
    GateRef LoadFromConstPool(GateRef jsFunc, size_t index);
    GateRef LoadFromVTable(GateRef receiver, size_t index);

    Circuit *circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef dependEntry_;
    [[maybe_unused]] TSManager *tsManager_ {nullptr};
    bool onHeapCheck_ {false};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_MCR_LOWERING_H
