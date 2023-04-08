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
    SlowPathLowering(Circuit *circuit, CompilationConfig *cmpCfg,
                     TSManager *tsManager, const MethodLiteral *methodLiteral,
                     bool enableLog, const std::string& name)
        : tsManager_(tsManager), methodLiteral_(methodLiteral),
          circuit_(circuit), acc_(circuit),
          argAcc_(circuit), builder_(circuit, cmpCfg),
          dependEntry_(circuit->GetDependRoot()),
          enableLog_(enableLog), methodName_(name), glue_(acc_.GetGlueFromArgList())
    {
        traceBc_ = cmpCfg->IsTraceBC();
        profiling_ = cmpCfg->IsProfiling();
    }
    ~SlowPathLowering() = default;
    void CallRuntimeLowering();

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    bool IsTraceBC() const
    {
        return traceBc_;
    }

    bool IsProfiling() const
    {
        return profiling_;
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
    void ExceptionReturn(GateRef state, GateRef depend);
    void ReplaceHirWithIfBranch(GateRef hirGate, GateRef callGate, GateRef ifBranch);
    void ReplaceHirToCall(GateRef hirGate, GateRef callGate, bool noThrow = false);
    void ReplaceHirToJSCall(GateRef hirGate, GateRef callGate);
    void ReplaceHirToThrowCall(GateRef hirGate, GateRef callGate);
    void LowerExceptionHandler(GateRef hirGate);
    // environment must be initialized
    GateRef LoadObjectFromConstPool(GateRef jsFunc, GateRef index);
    GateRef GetProfileTypeInfo(GateRef jsFunc);
    // environment must be initialized
    GateRef GetHomeObjectFromJSFunction(GateRef jsFunc);
    void Lower(GateRef gate);
    void LowerAdd2(GateRef gate);
    void LowerCreateIterResultObj(GateRef gate);
    void SaveFrameToContext(GateRef gate, GateRef jsFunc);
    void LowerSuspendGenerator(GateRef gate, GateRef jsFunc);
    void LowerAsyncFunctionAwaitUncaught(GateRef gate);
    void LowerAsyncFunctionResolve(GateRef gate);
    void LowerAsyncFunctionReject(GateRef gate);
    void LowerLoadStr(GateRef gate, GateRef jsFunc);
    void LowerStGlobalVar(GateRef gate, GateRef jsFunc);
    void LowerTryLdGlobalByName(GateRef gate, GateRef jsFunc);
    void LowerGetIterator(GateRef gate);
    void LowerGetAsyncIterator(GateRef gate);
    void LowerToJSCall(GateRef gate, const std::vector<GateRef> &args);
    void LowerCallArg0(GateRef gate);
    void LowerCallArg1Imm8V8(GateRef gate);
    void LowerCallThisArg1(GateRef gate);
    void LowerCallargs2Imm8V8V8(GateRef gate);
    void LowerCallthis2Imm8V8V8V8(GateRef gate);
    void LowerCallthis0Imm8V8(GateRef gate);
    void LowerCallargs3Imm8V8V8(GateRef gate);
    void LowerCallthis3Imm8V8V8V8V8(GateRef gate);
    void LowerCallthisrangeImm8Imm8V8(GateRef gate);
    void LowerWideCallthisrangePrefImm16V8(GateRef gate);
    void LowerCallSpread(GateRef gate);
    void LowerCallrangeImm8Imm8V8(GateRef gate);
    void LowerWideCallrangePrefImm16V8(GateRef gate);
    void LowerNewObjApply(GateRef gate);
    void LowerThrow(GateRef gate);
    void LowerThrowConstAssignment(GateRef gate);
    void LowerThrowThrowNotExists(GateRef gate);
    void LowerThrowPatternNonCoercible(GateRef gate);
    void LowerThrowIfNotObject(GateRef gate);
    void LowerThrowUndefinedIfHole(GateRef gate);
    void LowerThrowUndefinedIfHoleWithName(GateRef gate, GateRef jsFunc);
    void LowerThrowIfSuperNotCorrectCall(GateRef gate);
    void LowerThrowDeleteSuperProperty(GateRef gate);
    void LowerLdSymbol(GateRef gate);
    void LowerLdGlobal(GateRef gate);
    void LowerSub2(GateRef gate);
    void LowerMul2(GateRef gate);
    void LowerDiv2(GateRef gate);
    void LowerMod2(GateRef gate);
    void LowerEq(GateRef gate);
    void LowerNotEq(GateRef gate);
    void LowerLess(GateRef gate);
    void LowerLessEq(GateRef gate);
    void LowerGreater(GateRef gate);
    void LowerGreaterEq(GateRef gate);
    void LowerGetPropIterator(GateRef gate);
    void LowerCloseIterator(GateRef gate);
    void LowerInc(GateRef gate);
    void LowerDec(GateRef gate);
    void LowerToNumber(GateRef gate);
    void LowerNeg(GateRef gate);
    void LowerNot(GateRef gate);
    void LowerShl2(GateRef gate);
    void LowerShr2(GateRef gate);
    void LowerAshr2(GateRef gate);
    void LowerAnd2(GateRef gate);
    void LowerOr2(GateRef gate);
    void LowerXor2(GateRef gate);
    void LowerDelObjProp(GateRef gate);
    void LowerExp(GateRef gate);
    void LowerIsIn(GateRef gate);
    void LowerInstanceof(GateRef gate);
    void LowerFastStrictNotEqual(GateRef gate);
    void LowerFastStrictEqual(GateRef gate);
    void LowerCreateEmptyArray(GateRef gate);
    void LowerCreateEmptyObject(GateRef gate);
    void LowerCreateArrayWithBuffer(GateRef gate, GateRef jsFunc);
    void LowerCreateObjectWithBuffer(GateRef gate, GateRef jsFunc);
    void LowerStModuleVar(GateRef gate, GateRef jsFunc);
    void LowerGetTemplateObject(GateRef gate);
    void LowerSetObjectWithProto(GateRef gate);
    void LowerLdBigInt(GateRef gate);
    void LowerToNumeric(GateRef gate);
    void LowerDynamicImport(GateRef gate, GateRef jsFunc);
    void LowerLdLocalModuleVarByIndex(GateRef gate, GateRef jsFunc);
    void LowerExternalModule(GateRef gate, GateRef jsFunc);
    void LowerGetModuleNamespace(GateRef gate, GateRef jsFunc);
    void LowerSuperCall(GateRef gate, GateRef func, GateRef newTarget);
    void LowerSuperCallArrow(GateRef gate, GateRef newTarget);
    void LowerSuperCallSpread(GateRef gate, GateRef newTarget);
    void LowerIsTrueOrFalse(GateRef gate, bool flag);
    void LowerNewObjRange(GateRef gate);
    void LowerConditionJump(GateRef gate, bool isEqualJump);
    void LowerGetNextPropName(GateRef gate);
    void LowerCopyDataProperties(GateRef gate);
    void LowerCreateObjectWithExcludedKeys(GateRef gate);
    void LowerCreateRegExpWithLiteral(GateRef gate);
    void LowerStOwnByValue(GateRef gate);
    void LowerStOwnByIndex(GateRef gate);
    void LowerStOwnByName(GateRef gate);
    void LowerDefineFunc(GateRef gate, GateRef jsFunc);
    void LowerNewLexicalEnv(GateRef gate);
    void LowerNewLexicalEnvWithName(GateRef gate, GateRef jsFunc);
    void LowerPopLexicalEnv(GateRef gate);
    void LowerLdSuperByValue(GateRef gate, GateRef jsFunc);
    void LowerStSuperByValue(GateRef gate, GateRef jsFunc);
    void LowerTryStGlobalByName(GateRef gate, GateRef jsFunc);
    void LowerStConstToGlobalRecord(GateRef gate, bool isConst);
    void LowerStOwnByValueWithNameSet(GateRef gate);
    void LowerStOwnByNameWithNameSet(GateRef gate);
    void LowerLdGlobalVar(GateRef gate, GateRef jsFunc);
    void LowerLdObjByName(GateRef gate, GateRef jsFunc);
    void LowerStObjByName(GateRef gate, GateRef jsFunc, bool isThis);
    void LowerLdSuperByName(GateRef gate, GateRef jsFunc);
    void LowerStSuperByName(GateRef gate, GateRef jsFunc);
    void LowerDefineGetterSetterByValue(GateRef gate);
    void LowerLdObjByIndex(GateRef gate);
    void LowerStObjByIndex(GateRef gate);
    void LowerLdObjByValue(GateRef gate, GateRef jsFunc, bool isThis);
    void LowerStObjByValue(GateRef gate, GateRef jsFunc, bool isThis);
    void LowerCreateGeneratorObj(GateRef gate);
    void LowerStArraySpread(GateRef gate);
    void LowerLdLexVar(GateRef gate);
    void LowerStLexVar(GateRef gate);
    void LowerDefineClassWithBuffer(GateRef gate, GateRef jsFunc);
    void LowerAsyncFunctionEnter(GateRef gate);
    void LowerTypeof(GateRef gate);
    void LowerResumeGenerator(GateRef gate);
    void LowerGetResumeMode(GateRef gate);
    void LowerDefineMethod(GateRef gate, GateRef jsFunc);
    void LowerGetUnmappedArgs(GateRef gate, GateRef actualArgc);
    void LowerCopyRestArgs(GateRef gate, GateRef actualArgc);
    GateRef LowerCallRuntime(int index, const std::vector<GateRef> &args, bool useLabel = false);
    GateRef LowerCallNGCRuntime(int index, const std::vector<GateRef> &args, bool useLabel = false);
    int32_t ComputeCallArgc(GateRef gate, EcmaOpcode op);
    void LowerCreateAsyncGeneratorObj(GateRef gate);
    void LowerAsyncGeneratorResolve(GateRef gate);
    void LowerAsyncGeneratorReject(GateRef gate);
    void LowerSetGeneratorState(GateRef gate, GateRef jsFunc);
    GateRef GetValueFromTaggedArray(GateRef arrayGate, GateRef indexOffset);
    void AddProfiling(GateRef gate, bool skipGenerator = true);
    GateRef FastStrictEqual(GateRef left, GateRef right);
    void LowerWideLdPatchVar(GateRef gate);
    void LowerWideStPatchVar(GateRef gate);
    void LowerLdThisByName(GateRef gate, GateRef jsFunc);
    void LowerConstPoolData(GateRef gate);
    void LowerDeoptCheck(GateRef gate);
    void LowerConstruct(GateRef gate);
    void LowerUpdateHotness(GateRef gate);
    void LowerNotifyConcurrentResult(GateRef gate);

    TSManager *tsManager_ {nullptr};
    const MethodLiteral *methodLiteral_ {nullptr};
    Circuit *circuit_;
    GateAccessor acc_;
    ArgumentAccessor argAcc_;
    CircuitBuilder builder_;
    GateRef dependEntry_;
    bool enableLog_ {false};
    bool traceBc_ {false};
    bool profiling_ {false};
    std::string methodName_;
    GateRef glue_ {Circuit::NullGate()};
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_SLOWPATH_LOWERING_H
