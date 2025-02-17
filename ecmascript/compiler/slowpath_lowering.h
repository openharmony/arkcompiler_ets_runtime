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
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/new_object_stub_builder.h"
#include "ecmascript/compiler/pass_manager.h"
#include <cstddef>

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
                     PassContext *ctx, const MethodLiteral *methodLiteral,
                     bool enableLog, const std::string& name, const CString &recordName)
        : compilationEnv_(ctx->GetCompilationEnv()), methodLiteral_(methodLiteral),
          circuit_(circuit), acc_(circuit),
          argAcc_(circuit), builder_(circuit, cmpCfg),
          enableLog_(enableLog), methodName_(name), recordName_(recordName), glue_(acc_.GetGlueFromArgList())
    {
        traceBc_ = cmpCfg->IsTraceBC();
        profiling_ = cmpCfg->IsProfiling();
        stressDeopt_ = cmpCfg->IsStressDeopt();
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

    bool IsStressDeopt() const
    {
        return stressDeopt_;
    }

private:
    const std::string& GetMethodName() const
    {
        return methodName_;
    }
    bool enableMegaIC(GateRef gate);
    void ReplaceHirWithPendingException(GateRef hirGate, GateRef state, GateRef depend, GateRef value);
    void ReplaceHirWithValue(GateRef hirGate, GateRef value, bool noThrow = false);
    void ReplaceHirToThrowCall(GateRef hirGate, GateRef callGate);
    void LowerExceptionHandler(GateRef hirGate);
    void Lower(GateRef gate);
    void LowerAdd2(GateRef gate);
    void LowerCreateIterResultObj(GateRef gate);
    void SaveFrameToContext(GateRef gate);
    void LowerSuspendGenerator(GateRef gate);
    void LowerAsyncFunctionAwaitUncaught(GateRef gate);
    void LowerAsyncFunctionResolve(GateRef gate);
    void LowerAsyncFunctionReject(GateRef gate);
    void LowerStGlobalVar(GateRef gate);
    void LowerTryLdGlobalByName(GateRef gate);
    void LowerGetIterator(GateRef gate);
    void LowerGetAsyncIterator(GateRef gate);
    void LowerToJSCall(GateRef hirGate, const std::vector<GateRef> &args, const std::vector<GateRef> &argsFastCall);
    void LowerNewFastCall(GateRef gate, GateRef glue, GateRef func, bool needPushArgv,
        const std::vector<GateRef> &args, const std::vector<GateRef> &fastCallArgs,
        Variable *result, Label *exit);
    void LowerCallArg0Stub(GateRef gate);
    void LowerCallArg1Stub(GateRef gate);
    void LowerCallArg2Stub(GateRef gate);
    void LowerCallArg3Stub(GateRef gate);
    void LowerCallThis0Stub(GateRef gate);
    void LowerCallThis1Stub(GateRef gate);
    void LowerCallThis2Stub(GateRef gate);
    void LowerCallThis3Stub(GateRef gate);
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
    void LowerThrowUndefinedIfHoleWithName(GateRef Getgate);
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
    void LowerCreateArrayWithBuffer(GateRef gate);
    void LowerCreateObjectWithBuffer(GateRef gate);
    void LowerStModuleVar(GateRef gate);
    void LowerGetTemplateObject(GateRef gate);
    void LowerSetObjectWithProto(GateRef gate);
    void LowerLdBigInt(GateRef gate);
    void LowerToNumeric(GateRef gate);
    void LowerDynamicImport(GateRef gate);
    void LowerLdLocalModuleVarByIndex(GateRef gate);
    void LowerExternalModule(GateRef gate);
    void LowerGetModuleNamespace(GateRef gate);
    void LowerSendableExternalModule(GateRef gate);
    void LowerSendableLocalModule(GateRef gate);
    void LowerSuperCall(GateRef gate);
    void LowerSuperCallArrow(GateRef gate);
    void LowerSuperCallSpread(GateRef gate);
    void LowerSuperCallForwardAllArgs(GateRef gate);
    void CheckSuperAndNewTarget(NewObjectStubBuilder &objBuilder, GateRef super, Variable &newTarget,
                                Variable &thisObj, Label &fastPath, Label &slowPath);
    void CallNGCRuntimeWithCallTimer(int index, GateRef gate, GateRef func, Variable &result,
                                     const std::vector<GateRef> &args);
    GateRef IsAotOrFastCall(GateRef func, CircuitBuilder::JudgeMethodType type);
    void LowerFastSuperCallWithArgArray(GateRef array, const std::vector<GateRef> &args, bool isSpread,
                                        Variable &result, Label &exit);
    void LowerFastSuperCall(const std::vector<GateRef> &args, GateRef elementsPtr,
                            Variable &result, Label &exit);
    void GenerateSuperCallForwardAllArgsWithoutArgv(const std::vector<GateRef> &args, Variable &result, Label &exit);
    void LowerIsTrueOrFalse(GateRef gate, bool flag);
    void LowerNewObjRange(GateRef gate);
    bool IsDependIfStateMent(GateRef gate, size_t idx);
    void LowerConditionJump(GateRef gate, bool isEqualJump);
    void LowerGetNextPropName(GateRef gate);
    void LowerCopyDataProperties(GateRef gate);
    void LowerCreateObjectWithExcludedKeys(GateRef gate);
    void LowerCreateRegExpWithLiteral(GateRef gate);
    void LowerStOwnByValue(GateRef gate);
    void LowerStOwnByIndex(GateRef gate);
    void LowerStOwnByName(GateRef gate);
    void LowerDefineFunc(GateRef gate);
    void LowerNewLexicalEnv(GateRef gate);
    void LowerNewLexicalEnvWithName(GateRef gate);
    void LowerNewSendableEnv(GateRef gate);
    void LowerPopLexicalEnv(GateRef gate);
    void LowerLdSuperByValue(GateRef gate);
    void LowerStSuperByValue(GateRef gate);
    void LowerTryStGlobalByName(GateRef gate);
    void LowerStConstToGlobalRecord(GateRef gate, bool isConst);
    void LowerStOwnByValueWithNameSet(GateRef gate);
    void LowerStOwnByNameWithNameSet(GateRef gate);
    void LowerLdGlobalVar(GateRef gate);
    void LowerLdObjByName(GateRef gate);
    void LowerStObjByName(GateRef gate, bool isThis);
    void LowerLdSuperByName(GateRef gate);
    void LowerStSuperByName(GateRef gate);
    void LowerDefineGetterSetterByValue(GateRef gate);
    void LowerLdObjByIndex(GateRef gate);
    void LowerStObjByIndex(GateRef gate);
    void LowerLdObjByValue(GateRef gate, bool isThis);
    void LowerStObjByValue(GateRef gate, bool isThis);
    void LowerCreateGeneratorObj(GateRef gate);
    void LowerStArraySpread(GateRef gate);
    void LowerLdLexVar(GateRef gate);
    void LowerLdSendableVar(GateRef gate);
    void LowerStLexVar(GateRef gate);
    void LowerStSendableVar(GateRef gate);
    void LowerDefineClassWithBuffer(GateRef gate);
    void LowerAsyncFunctionEnter(GateRef gate);
    void LowerTypeof(GateRef gate);
    void LowerResumeGenerator(GateRef gate);
    void LowerStoreRegister(GateRef gate, GateRef arrayGate);
    void LowerGetResumeMode(GateRef gate);
    void LowerDefineMethod(GateRef gate);
    void LowerGetUnmappedArgs(GateRef gate);
    void LowerCopyRestArgs(GateRef gate);
    void LowerCallStubWithIC(GateRef gate, int sign, const std::vector<GateRef> &args);
    GateRef LowerCallRuntime(GateRef gate, int index, const std::vector<GateRef> &args, bool useLabel = false);
    GateRef LowerCallNGCRuntime(GateRef gate, int index, const std::vector<GateRef> &args, bool useLabel = false);
    void LowerCreateAsyncGeneratorObj(GateRef gate);
    void LowerAsyncGeneratorResolve(GateRef gate);
    void LowerAsyncGeneratorReject(GateRef gate);
    void LowerSetGeneratorState(GateRef gate);
    GateRef GetValueFromTaggedArray(GateRef arrayGate, GateRef indexOffset);
    GateRef GetTaggedArrayFromValueIn(Environment *env, GateRef gate, size_t length);
    GateRef LowerUpdateArrayHClassAtDefine(GateRef gate, GateRef array);
    void AddProfiling(GateRef gate, bool skipGenerator = true);
    GateRef FastStrictEqual(GateRef left, GateRef right);
    void LowerWideLdPatchVar(GateRef gate);
    void LowerWideStPatchVar(GateRef gate);
    void LowerLdThisByName(GateRef gate);
    bool IsFastCallArgs(size_t index);
    void LowerConstruct(GateRef gate);
    void LowerCallInternal(GateRef gate);
    void LowerCallNew(GateRef gate);
    void LowerTypedCall(GateRef gate);
    void LowerTypedFastCall(GateRef gate);
    void LowerCheckSafePointAndStackOver(GateRef gate);
    void LowerLdPrivateProperty(GateRef gate);
    void LowerStPrivateProperty(GateRef gate);
    void LowerTestIn(GateRef gate);
    void LowerNotifyConcurrentResult(GateRef gate);
    void LowerDefineFieldByName(GateRef gate);
    void LowerDefineFieldByValue(GateRef gate);
    void LowerDefineFieldByIndex(GateRef gate);
    void LowerToPropertyKey(GateRef gate);
    void LowerCreatePrivateProperty(GateRef gate);
    void LowerDefinePrivateProperty(GateRef gate);
    void LowerDefineSendableClass(GateRef gate);
    void LowerLdSendableClass(GateRef gate);
    void LowerGetEnv(GateRef gate);
    void DeleteLoopExit(GateRef gate);
    void DeleteLoopExitValue(GateRef gate);
    void LowerLdStr(GateRef gate);
    void LowerGetSharedConstPool(GateRef gate);
    void LowerGetUnsharedConstPool(GateRef gate);
    void LowerLdLazyExternalModuleVar(GateRef gate);
    void LowerLdLazySendableExternalModuleVar(GateRef gate);
    GateRef GetStringFromConstPool(GateRef gate, GateRef stringId, uint32_t stringIdIdx);

    CompilationEnv *compilationEnv_;
    const MethodLiteral *methodLiteral_ {nullptr};
    Circuit *circuit_;
    GateAccessor acc_;
    ArgumentAccessor argAcc_;
    CircuitBuilder builder_;
    bool enableLog_ {false};
    bool traceBc_ {false};
    bool profiling_ {false};
    bool stressDeopt_ {false};
    std::string methodName_;
    const CString &recordName_;
    GateRef glue_ {Circuit::NullGate()};
    CVector<GateRef> unsharedCP_;
};
}  // panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_SLOWPATH_LOWERING_H
