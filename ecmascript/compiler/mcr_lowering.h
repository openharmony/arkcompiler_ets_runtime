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

#ifndef ECMASCRIPT_COMPILER_MCR_LOWERING_H
#define ECMASCRIPT_COMPILER_MCR_LOWERING_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/combined_pass_visitor.h"
#include "ecmascript/compiler/compilation_env.h"
#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {
class MCRLowering : public PassVisitor {
public:
    MCRLowering(CompilationEnv* env, Circuit *circuit, RPOVisitor *visitor, CompilationConfig *cmpCfg, Chunk *chunk)
        : PassVisitor(circuit, chunk, visitor), env_(env), circuit_(circuit), acc_(circuit),
          builder_(circuit, cmpCfg), glue_(acc_.GetGlueFromArgList())
    {
    }
    ~MCRLowering() = default;

    GateRef VisitGate(GateRef gate) override;
    StateDepend LowerConvert(StateDepend stateDepend, GateRef gate);
private:

    void DeleteStateSplit(GateRef gate);
    void LowerArrayGuardianCheck(GateRef gate);
    void LowerHeapObjectCheck(GateRef gate);
    void LowerTaggedIsHeapObject(GateRef gate);
    void LowerIsMarkerCellValid(GateRef gate);
    void LowerIsSpecificObjectType(GateRef gate);
    void LowerHClassStableArrayCheck(GateRef gate);
    void LowerBuiltinHClassConsistencyCheck(GateRef gate);
    void LowerGetConstPool(GateRef gate);
    void LowerGetUnsharedConstpool(GateRef gate);
    void LowerLoadConstOffset(GateRef gate);
    void LowerLoadHClassConstOffset(GateRef gate);
    void LowerLoadHClassFromConstpool(GateRef gate);
    void LowerStoreConstOffset(GateRef gate);
    void LowerStoreHClassConstOffset(GateRef gate);
    void LowerConvertHoleAsUndefined(GateRef gate);
    void LowerCheckAndConvert(GateRef gate);
    void LowerCheckUInt32AndConvert(GateRef gate, GateRef frameState);
    void LowerCheckTaggedIntAndConvert(GateRef gate, GateRef frameState);
    void LowerCheckTaggedDoubleAndConvert(GateRef gate, GateRef frameState);
    void LowerCheckTaggedNumberAndConvert(GateRef gate, GateRef frameState, Label *exit);
    void LowerCheckTaggedBoolAndConvert(GateRef gate, GateRef frameState);
    void LowerCheckSpecialHoleAndConvert(GateRef gate, GateRef frameState);
    void LowerCheckSupportAndConvertBool(GateRef gate, GateRef frameState);
    void LowerGetGlobalEnv(GateRef gate);
    void LowerGetGlobalEnvObj(GateRef gate);
    void LowerGetGlobalEnvObjHClass(GateRef gate);
    void LowerGetGlobalConstantValue(GateRef gate);
    void LowerProductIsNegativeZero(GateRef gate);
    void LowerInt32CheckRightIsZero(GateRef gate);
    void LowerRemainderIsNegativeZero(GateRef gate);
    void LowerFloat64CheckRightIsZero(GateRef gate);
    void LowerValueCheckNegOverflow(GateRef gate);
    void LowerOverflowCheck(GateRef gate);
    void LowerInt32UnsignedUpperBoundCheck(GateRef gate);
    void LowerInt32DivWithCheck(GateRef gate);
    void LowerLexVarIsHoleCheck(GateRef gate);
    void LowerIsUndefinedOrHoleCheck(GateRef gate);
    void LowerIsNotUndefinedOrHoleCheck(GateRef gate);
    void LowerIsEcmaObjectCheck(GateRef gate);
    void LowerIsDataViewCheck(GateRef gate);
    void LowerStoreMemory(GateRef gate);
    void LowerCheckNullAndConvert(GateRef gate, GateRef frameState);
    void LowerUndefinedAndConvert(GateRef gate, GateRef frameState);
    void LowerMigrateFromRawValueToHeapValues(GateRef gate);
    void LowerMigrateFromHeapValueToRawValue(GateRef gate);
    void LowerMigrateFromHoleIntToHoleNumber(GateRef gate);
    void LowerMigrateFromHoleNumberToHoleInt(GateRef gate);
    void LowerHeapObjectIsEcmaObject(GateRef gate);
    void LowerIsCallableCheck(GateRef gate);
    void LowerCheckFloat64AndConvert(GateRef gate, GateRef frameState);
    void LowerStringAdd(GateRef gate);

    GateRef ConvertSpecialHoleIntToTagged(GateRef gate, Label* exit);
    GateRef ConvertSpecialHoleDoubleToTagged(GateRef gate, Label* exit);
    GateRef ConvertBoolToTaggedBoolean(GateRef gate);
    GateRef ConvertInt32ToFloat64(GateRef gate);
    GateRef ConvertUInt32ToFloat64(GateRef gate);
    GateRef ConvertInt32ToTaggedInt(GateRef gate);
    GateRef ConvertUInt32ToTaggedNumber(GateRef gate, Label *exit);
    GateRef ConvertFloat64ToBool(GateRef gate);
    GateRef ConvertFloat64ToInt32(GateRef glue, GateRef gate);
    GateRef ConvertFloat64ToTaggedDouble(GateRef gate);
    GateRef ConvertTaggedIntToInt32(GateRef gate);
    GateRef ConvertTaggedIntToFloat64(GateRef gate);
    GateRef ConvertTaggedDoubleToInt32(GateRef glue, GateRef gate);
    GateRef ConvertTaggedDoubleToFloat64(GateRef gate);
    GateRef ConvertTaggedNumberToBool(GateRef gate, Label *exit);
    GateRef ConvertTaggedNumberToInt32(GateRef gate, Label *exit);
    GateRef ConvertTaggedNumberToFloat64(GateRef gate, Label *exit);
    GateRef ConvertTaggedBooleanToBool(GateRef gate);
    void HeapAllocateInSOld(GateRef gate);
    void InitializeWithSpeicalValue(Label *exit, GateRef object, GateRef glue, GateRef value,
                                    GateRef start, GateRef end);
    CompilationEnv *env_;
    Circuit *circuit_;
    GateAccessor acc_;
    CircuitBuilder builder_;
    GateRef glue_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_MCR_LOWERING_H
