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

#ifndef ECMASCRIPT_COMPILER_SLOWPATH_LOWERING_H
#define ECMASCRIPT_COMPILER_SLOWPATH_LOWERING_H

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/gate_accessor.h"

namespace panda::ecmascript::kungfu {
// slowPath Lowering Process
// SW: state wire, DW: depend wire, VW: value wire
// Before lowering:
//                         SW        DW         VW
//                         |         |          |
//                         |         |          |
//                         v         v          v
//                     +-----------------------------+
//                     |            (HIR)            |
//                     |         JS_BYTECODE         |DW--------------------------------------
//                     |                             |                                       |
//                     +-----------------------------+                                       |
//                         SW                   SW                                           |
//                         |                     |                                           |
//                         |                     |                                           |
//                         |                     |                                           |
//                         v                     v                                           |
//                 +--------------+        +--------------+                                  |
//                 |  IF_SUCCESS  |        | IF_EXCEPTION |SW---------                       |
//                 +--------------+        +--------------+          |                       |
//                         SW                    SW                  |                       |
//                         |                     |                   |                       |
//                         v                     v                   |                       |
//     --------------------------------------------------------------|-----------------------|-------------------
//     catch processing                                              |                       |
//                                                                   |                       |
//                                                                   v                       V
//                                                            +--------------+       +-----------------+
//                                                            |    MERGE     |SW---->| DEPEND_SELECTOR |
//                                                            +--------------+       +-----------------+
//                                                                                          DW
//                                                                                          |
//                                                                                          v
//                                                                                   +-----------------+
//                                                                                   |  GET_EXCEPTION  |
//                                                                                   +-----------------+


// After lowering:
//         SW                                          DW      VW
//         |                                           |       |
//         |                                           |       |
//         |                                           v       v
//         |        +---------------------+         +------------------+
//         |        | CONSTANT(Exception) |         |       CALL       |DW---------------
//         |        +---------------------+         +------------------+                |
//         |                           VW            VW                                 |
//         |                           |             |                                  |
//         |                           |             |                                  |
//         |                           v             v                                  |
//         |                        +------------------+                                |
//         |                        |        EQ        |                                |
//         |                        +------------------+                                |
//         |                                VW                                          |
//         |                                |                                           |
//         |                                |                                           |
//         |                                v                                           |
//         |                        +------------------+                                |
//         ------------------------>|    IF_BRANCH     |                                |
//                                  +------------------+                                |
//                                   SW             SW                                  |
//                                   |              |                                   |
//                                   v              v                                   |
//                           +--------------+  +--------------+                         |
//                           |   IF_FALSE   |  |   IF_TRUE    |                         |
//                           |  (success)   |  |  (exception) |                         |
//                           +--------------+  +--------------+                         |
//                                 SW                SW   SW                            |
//                                 |                 |    |                             |
//                                 v                 v    |                             |
//     ---------------------------------------------------|-----------------------------|----------------------
//     catch processing                                   |                             |
//                                                        |                             |
//                                                        v                             v
//                                                 +--------------+             +-----------------+
//                                                 |    MERGE     |SW---------->| DEPEND_SELECTOR |
//                                                 +--------------+             +-----------------+
//                                                                                      DW
//                                                                                      |
//                                                                                      v
//                                                                              +-----------------+
//                                                                              |  GET_EXCEPTION  |
//                                                                              +-----------------+

class SlowPathLowering {
public:
    SlowPathLowering(BytecodeCircuitBuilder *bcBuilder, Circuit *circuit, CompilationConfig *cmpCfg,
                     TSManager *tsManager, bool enableLog, const std::string& name)
        : tsManager_(tsManager), bcBuilder_(bcBuilder), circuit_(circuit), acc_(circuit),
          argAcc_(circuit), builder_(circuit, cmpCfg),
          dependEntry_(Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY))),
          enableLog_(enableLog), methodName_(name)
    {
        enableBcTrace_ = cmpCfg->IsEnableByteCodeTrace();
    }
    ~SlowPathLowering() = default;
    void CallRuntimeLowering();

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    bool IsEnableByteCodeTrace() const
    {
        return enableBcTrace_;
    }

private:
    const std::string& GetMethodName() const
    {
        return methodName_;
    }

    GateAccessor::UseIterator ReplaceHirControlGate(const GateAccessor::UseIterator &useIt, GateRef newGate,
                                                    bool noThrow = false);
    void ReplaceHirToSubCfg(GateRef hir, GateRef outir,
                       const std::vector<GateRef> &successControl,
                       const std::vector<GateRef> &exceptionControl,
                       bool noThrow = false);
    void ReplaceHirToCall(GateRef hirGate, GateRef callGate, bool noThrow = false);
    void ReplaceHirToJSCall(GateRef hirGate, GateRef callGate, GateRef glue);
    void ReplaceHirToThrowCall(GateRef hirGate, GateRef callGate);
    void LowerExceptionHandler(GateRef hirGate);
    // environment must be initialized
    GateRef GetConstPool(GateRef jsFunc);
    GateRef LoadObjectFromConstPool(GateRef jsFunc, GateRef index);
    GateRef GetProfileTypeInfo(GateRef jsFunc);
    GateRef GetObjectFromConstPool(GateRef jsFunc, GateRef index);
    // environment must be initialized
    GateRef GetObjectFromConstPool(GateRef glue, GateRef jsFunc, GateRef index, ConstPoolType type);
    // environment must be initialized
    GateRef GetHomeObjectFromJSFunction(GateRef jsFunc);
    void Lower(GateRef gate);
    void LowerAdd2(GateRef gate, GateRef glue);
    void LowerCreateIterResultObj(GateRef gate, GateRef glue);
    void SaveFrameToContext(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerSuspendGenerator(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerAsyncFunctionAwaitUncaught(GateRef gate, GateRef glue);
    void LowerAsyncFunctionResolve(GateRef gate, GateRef glue);
    void LowerAsyncFunctionReject(GateRef gate, GateRef glue);
    void LowerLoadStr(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerStGlobalVar(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerTryLdGlobalByName(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerGetIterator(GateRef gate, GateRef glue);
    void LowerToJSCall(GateRef gate, GateRef glue, const std::vector<GateRef> &args);
    void LowerCallArg0(GateRef gate, GateRef glue);
    void LowerCallArg1Imm8V8(GateRef gate, GateRef glue);
    void LowerCallThisArg1(GateRef gate, GateRef glue);
    void LowerCallargs2Imm8V8V8(GateRef gate, GateRef glue);
    void LowerCallthis2Imm8V8V8V8(GateRef gate, GateRef glue);
    void LowerCallthis0Imm8V8(GateRef gate, GateRef glue);
    void LowerCallargs3Imm8V8V8(GateRef gate, GateRef glue);
    void LowerCallthis3Imm8V8V8V8V8(GateRef gate, GateRef glue);
    void LowerCallthisrangeImm8Imm8V8(GateRef gate, GateRef glue);
    void LowerWideCallthisrangePrefImm16V8(GateRef gate, GateRef glue);
    void LowerCallSpread(GateRef gate, GateRef glue);
    void LowerCallrangeImm8Imm8V8(GateRef gate, GateRef glue);
    void LowerWideCallrangePrefImm16V8(GateRef gate, GateRef glue);
    void LowerNewObjApply(GateRef gate, GateRef glue);
    void LowerThrow(GateRef gate, GateRef glue);
    void LowerThrowConstAssignment(GateRef gate, GateRef glue);
    void LowerThrowThrowNotExists(GateRef gate, GateRef glue);
    void LowerThrowPatternNonCoercible(GateRef gate, GateRef glue);
    void LowerThrowIfNotObject(GateRef gate, GateRef glue);
    void LowerThrowUndefinedIfHole(GateRef gate, GateRef glue);
    void LowerThrowIfSuperNotCorrectCall(GateRef gate, GateRef glue);
    void LowerThrowDeleteSuperProperty(GateRef gate, GateRef glue);
    void LowerLdSymbol(GateRef gate, GateRef glue);
    void LowerLdGlobal(GateRef gate, GateRef glue);
    void LowerSub2(GateRef gate, GateRef glue);
    void LowerMul2(GateRef gate, GateRef glue);
    void LowerDiv2(GateRef gate, GateRef glue);
    void LowerMod2(GateRef gate, GateRef glue);
    void LowerEq(GateRef gate, GateRef glue);
    void LowerNotEq(GateRef gate, GateRef glue);
    void LowerLess(GateRef gate, GateRef glue);
    void LowerLessEq(GateRef gate, GateRef glue);
    void LowerGreater(GateRef gate, GateRef glue);
    void LowerGreaterEq(GateRef gate, GateRef glue);
    void LowerGetPropIterator(GateRef gate, GateRef glue);
    void LowerCloseIterator(GateRef gate, GateRef glue);
    void LowerInc(GateRef gate, GateRef glue);
    void LowerDec(GateRef gate, GateRef glue);
    void LowerToNumber(GateRef gate, GateRef glue);
    void LowerNeg(GateRef gate, GateRef glue);
    void LowerNot(GateRef gate, GateRef glue);
    void LowerShl2(GateRef gate, GateRef glue);
    void LowerShr2(GateRef gate, GateRef glue);
    void LowerAshr2(GateRef gate, GateRef glue);
    void LowerAnd2(GateRef gate, GateRef glue);
    void LowerOr2(GateRef gate, GateRef glue);
    void LowerXor2(GateRef gate, GateRef glue);
    void LowerDelObjProp(GateRef gate, GateRef glue);
    void LowerExp(GateRef gate, GateRef glue);
    void LowerIsIn(GateRef gate, GateRef glue);
    void LowerInstanceof(GateRef gate, GateRef glue);
    void LowerFastStrictNotEqual(GateRef gate, GateRef glue);
    void LowerFastStrictEqual(GateRef gate, GateRef glue);
    void LowerCreateEmptyArray(GateRef gate, GateRef glue);
    void LowerCreateEmptyObject(GateRef gate, GateRef glue);
    void LowerCreateArrayWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerCreateObjectWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerStModuleVar(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerGetTemplateObject(GateRef gate, GateRef glue);
    void LowerSetObjectWithProto(GateRef gate, GateRef glue);
    void LowerLdBigInt(GateRef gate, GateRef glue);
    void LowerToNumeric(GateRef gate, GateRef glue);
    void LowerDynamicImport(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerLdLocalModuleVarByIndex(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerExternalModule(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerGetModuleNamespace(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerSuperCall(GateRef gate, GateRef glue, GateRef func, GateRef newTarget);
    void LowerSuperCallArrow(GateRef gate, GateRef glue, GateRef newTarget);
    void LowerSuperCallSpread(GateRef gate, GateRef glue, GateRef newTarget);
    void LowerIsTrueOrFalse(GateRef gate, GateRef glue, bool flag);
    void LowerNewObjRange(GateRef gate, GateRef glue);
    void LowerConditionJump(GateRef gate, bool isEqualJump);
    void LowerGetNextPropName(GateRef gate, GateRef glue);
    void LowerCopyDataProperties(GateRef gate, GateRef glue);
    void LowerCreateObjectWithExcludedKeys(GateRef gate, GateRef glue);
    void LowerCreateRegExpWithLiteral(GateRef gate, GateRef glue);
    void LowerStOwnByValue(GateRef gate, GateRef glue);
    void LowerStOwnByIndex(GateRef gate, GateRef glue);
    void LowerStOwnByName(GateRef gate, GateRef glue);
    void LowerDefineFunc(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerNewLexicalEnv(GateRef gate, GateRef glue);
    void LowerNewLexicalEnvWithName(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerPopLexicalEnv(GateRef gate, GateRef glue);
    void LowerLdSuperByValue(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerStSuperByValue(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerTryStGlobalByName(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerStConstToGlobalRecord(GateRef gate, GateRef glue, bool isConst);
    void LowerStOwnByValueWithNameSet(GateRef gate, GateRef glue);
    void LowerStOwnByNameWithNameSet(GateRef gate, GateRef glue);
    void LowerLdGlobalVar(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerLdObjByName(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerStObjByName(GateRef gate, GateRef glue, GateRef jsFunc, GateRef thisObj, bool isThis = false);
    void LowerLdSuperByName(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerStSuperByName(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerDefineGetterSetterByValue(GateRef gate, GateRef glue);
    void LowerLdObjByIndex(GateRef gate, GateRef glue);
    void LowerStObjByIndex(GateRef gate, GateRef glue);
    void LowerLdObjByValue(GateRef gate, GateRef glue, GateRef jsFunc, GateRef thisObj, bool useThis);
    void LowerStObjByValue(GateRef gate, GateRef glue, GateRef jsFunc, GateRef thisObj, bool useThis);
    void LowerCreateGeneratorObj(GateRef gate, GateRef glue);
    void LowerStArraySpread(GateRef gate, GateRef glue);
    void LowerLdLexVar(GateRef gate, GateRef glue);
    void LowerStLexVar(GateRef gate, GateRef glue);
    void LowerDefineClassWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerAsyncFunctionEnter(GateRef gate, GateRef glue);
    void LowerTypeof(GateRef gate, GateRef glue);
    void LowerResumeGenerator(GateRef gate);
    void LowerGetResumeMode(GateRef gate);
    void LowerDefineMethod(GateRef gate, GateRef glue, GateRef jsFunc);
    void LowerGetUnmappedArgs(GateRef gate, GateRef glue, GateRef actualArgc);
    void LowerCopyRestArgs(GateRef gate, GateRef glue, GateRef actualArgc);
    GateRef LowerCallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel = false);
    int32_t ComputeCallArgc(GateRef gate, EcmaOpcode op);
    void LowerCreateAsyncGeneratorObj(GateRef gate, GateRef glue);
    void LowerAsyncGeneratorResolve(GateRef gate, GateRef glue);
    void LowerAsyncGeneratorReject(GateRef gate, GateRef glue);
    GateRef GetValueFromTaggedArray(GateRef arrayGate, GateRef indexOffset);
    void DebugPrintBC(GateRef gate, GateRef glue);
    GateRef FastStrictEqual(GateRef glue, GateRef left, GateRef right);
    void LowerWideLdPatchVar(GateRef gate, GateRef glue);
    void LowerWideStPatchVar(GateRef gate, GateRef glue);
    void LowerLdThisByName(GateRef gate, GateRef glue, GateRef jsFunc, GateRef thisObj);
    void LowerConstPoolData(GateRef gate);

    TSManager *tsManager_ {nullptr};
    BytecodeCircuitBuilder *bcBuilder_;
    Circuit *circuit_;
    GateAccessor acc_;
    ArgumentAccessor argAcc_;
    CircuitBuilder builder_;
    GateRef dependEntry_;
    bool enableLog_ {false};
    bool enableBcTrace_ {false};
    std::string methodName_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_SLOWPATH_LOWERING_H