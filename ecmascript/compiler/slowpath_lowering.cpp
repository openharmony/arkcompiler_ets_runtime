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

#include "ecmascript/compiler/slowpath_lowering.h"
#include "ecmascript/message_string.h"

namespace panda::ecmascript::kungfu {
using UseIterator = GateAccessor::UseIterator;

#define CREATE_DOUBLE_EXIT(SuccessLabel, FailLabel)               \
    std::vector<GateRef> successControl;                          \
    std::vector<GateRef> failControl;                             \
    builder_.Bind(&SuccessLabel);                                 \
    {                                                             \
        successControl.emplace_back(builder_.GetState());         \
        successControl.emplace_back(builder_.GetDepend());        \
    }                                                             \
    builder_.Bind(&FailLabel);                                    \
    {                                                             \
        failControl.emplace_back(builder_.GetState());            \
        failControl.emplace_back(builder_.GetDepend());           \
    }

void SlowPathLowering::CallRuntimeLowering()
{
    std::vector<GateRef> gateList;
    circuit_->GetAllGates(gateList);
    for (const auto &gate : gateList) {
        auto op = acc_.GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            Lower(gate);
        } else if (op == OpCode::GET_EXCEPTION) {
            // initialize label manager
            Environment env(gate, circuit_, &builder_);
            LowerExceptionHandler(gate);
        }
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "=========================================================";
        circuit_->PrintAllGates(*bcBuilder_);
    }
}

int32_t SlowPathLowering::ComputeCallArgc(GateRef gate, EcmaBytecode op)
{
    if (op == EcmaBytecode::CALLITHISRANGEDYN_PREF_IMM16_V8) {
        return acc_.GetNumValueIn(gate) + NUM_MANDATORY_JSFUNC_ARGS - 3; // 3: calltarget, this and bcoffset
    }
    return acc_.GetNumValueIn(gate) + NUM_MANDATORY_JSFUNC_ARGS - 2; // 2: calltarget and bcoffset
}

UseIterator SlowPathLowering::ReplaceHirControlGate(const UseIterator &useIt, GateRef newGate, bool noThrow)
{
    ASSERT(acc_.GetOpCode(*useIt) == OpCode::IF_SUCCESS || acc_.GetOpCode(*useIt) == OpCode::IF_EXCEPTION);
    if (!noThrow) {
        auto firstUse = acc_.Uses(*useIt).begin();
        acc_.ReplaceIn(*firstUse, firstUse.GetIndex(), newGate);
    }
    auto next = acc_.DeleteGate(useIt);
    return next;
}

void SlowPathLowering::ReplaceHirToSubCfg(GateRef hir, GateRef outir,
                                          const std::vector<GateRef> &successControl,
                                          const std::vector<GateRef> &exceptionControl,
                                          bool noThrow)
{
    if (outir != Circuit::NullGate()) {
        auto type = acc_.GetGateType(hir);
        if (type.IsTSType()) {
            acc_.SetGateType(outir, type);
        }
    }
    auto uses = acc_.Uses(hir);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        const OpCode op = acc_.GetOpCode(*useIt);
        if (op == OpCode::IF_SUCCESS) {
            useIt = ReplaceHirControlGate(useIt, successControl[0]);
        } else if (op == OpCode::IF_EXCEPTION) {
            useIt = ReplaceHirControlGate(useIt, exceptionControl[0], noThrow);
        } else if (acc_.IsValueIn(useIt)) {
            useIt = acc_.ReplaceIn(useIt, outir);
        } else if (acc_.IsDependIn(useIt)) {
            if (acc_.IsExceptionState(useIt)) {
                useIt = noThrow ? acc_.DeleteExceptionDep(useIt)
                                : acc_.ReplaceIn(useIt, exceptionControl[1]);
            } else if (op == OpCode::DEPEND_RELAY && acc_.GetOpCode(acc_.GetIn(*useIt, 0)) == OpCode::IF_EXCEPTION) {
                useIt = acc_.ReplaceIn(useIt, exceptionControl[1]);
            } else {
                useIt = acc_.ReplaceIn(useIt, successControl[1]);
            }
        } else {
            UNREACHABLE();
        }
    }
    acc_.DeleteGate(hir);
}

void SlowPathLowering::ReplaceHirToJSCall(GateRef hirGate, GateRef callGate, GateRef glue)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef exceptionOffset = builder_.IntPtr(JSThread::GlueData::GetExceptionOffset(false));
    GateRef exception = builder_.Load(VariableType::JS_ANY(), glue, exceptionOffset);
    acc_.SetDep(exception, callGate);
    GateRef equal = builder_.NotEqual(exception, builder_.Int64(JSTaggedValue::VALUE_HOLE));
    GateRef ifBranch = builder_.Branch(stateInGate, equal);

    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            it = acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                it = acc_.ReplaceIn(it, ifBranch);
            } else {
                it = acc_.ReplaceIn(it, callGate);
            }
        }
    }

    // delete old gate
    acc_.DeleteGate(hirGate);
}

/*
 * lower to slowpath call like this pattern:
 * have throw:
 * res = Call(...);
 * if (res == VALUE_EXCEPTION) {
 *     goto exception_handle;
 * }
 * Set(res);
 *
 * no throw:
 * res = Call(...);
 * Set(res);
 */
void SlowPathLowering::ReplaceHirToCall(GateRef hirGate, GateRef callGate, bool noThrow)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef ifBranch;
    if (!noThrow) {
        // exception value
        GateRef exceptionVal = builder_.ExceptionConstant(GateType::TaggedNPointer());
        // compare with trampolines result
        GateRef equal = builder_.BinaryLogic(OpCode(OpCode::EQ), callGate, exceptionVal);
        ifBranch = builder_.Branch(stateInGate, equal);
    } else {
        ifBranch = builder_.Branch(stateInGate, builder_.Boolean(false));
    }

    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            it = acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                it = acc_.ReplaceIn(it, ifBranch);
            } else {
                it = acc_.ReplaceIn(it, callGate);
            }
        }
    }

    // delete old gate
    acc_.DeleteGate(hirGate);
}

/*
 * lower to throw call like this pattern:
 * Call(...);
 * goto exception_handle;
 *
 */
void SlowPathLowering::ReplaceHirToThrowCall(GateRef hirGate, GateRef callGate)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef ifBranch = builder_.Branch(stateInGate, builder_.Boolean(true));
    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            it = acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                it = acc_.ReplaceIn(it, ifBranch);
            } else {
                it = acc_.ReplaceIn(it, callGate);
            }
        }
    }
    acc_.DeleteGate(hirGate);
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetConstPool(GateRef jsFunc)
{
    GateRef method = builder_.GetMethodFromFunction(jsFunc);
    return builder_.Load(VariableType::JS_ANY(), method, builder_.IntPtr(Method::CONSTANT_POOL_OFFSET));
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constPool = GetConstPool(jsFunc);
    return builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), constPool, index);
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetHomeObjectFromJSFunction(GateRef jsFunc)
{
    GateRef offset = builder_.IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return builder_.Load(VariableType::JS_ANY(), jsFunc, offset);
}

void SlowPathLowering::Lower(GateRef gate)
{
    GateRef glue = argAcc_.GetCommonArgGate(CommonArgIdx::GLUE);
    GateRef newTarget = argAcc_.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    GateRef jsFunc = argAcc_.GetCommonArgGate(CommonArgIdx::FUNC);
    GateRef actualArgc = argAcc_.GetCommonArgGate(CommonArgIdx::ACTUAL_ARGC);

    auto pc = bcBuilder_->GetJSBytecode(gate);
    EcmaBytecode op = static_cast<EcmaBytecode>(*pc);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (op) {
        case LDA_STR_ID32:
            LowerLoadStr(gate, glue, jsFunc);
            break;
        case CALLARG0DYN_PREF_V8:
            LowerCallArg0(gate, glue);
            break;
        case CALLARG1DYN_PREF_V8_V8:
            LowerCallArg1(gate, glue);
            break;
        case CALLARGS2DYN_PREF_V8_V8_V8:
            LowerCallArgs2(gate, glue);
            break;
        case CALLARGS3DYN_PREF_V8_V8_V8_V8:
            LowerCallArgs3(gate, glue);
            break;
        case CALLITHISRANGEDYN_PREF_IMM16_V8:
            LowerCallThisRange(gate, glue);
            break;
        case CALLSPREADDYN_PREF_V8_V8_V8:
            LowerCallSpread(gate, glue);
            break;
        case CALLIRANGEDYN_PREF_IMM16_V8:
            LowerCallRange(gate, glue);
            break;
        case LDLEXENVDYN_PREF:
            LowerLexicalEnv(gate, glue);
            break;
        case GETUNMAPPEDARGS_PREF:
            LowerGetUnmappedArgs(gate, glue, actualArgc);
            break;
        case ASYNCFUNCTIONENTER_PREF:
            LowerAsyncFunctionEnter(gate, glue);
            break;
        case INCDYN_PREF_V8:
            LowerInc(gate, glue);
            break;
        case DECDYN_PREF_V8:
            LowerDec(gate, glue);
            break;
        case GETPROPITERATOR_PREF:
            LowerGetPropIterator(gate, glue);
            break;
        case RESUMEGENERATOR_PREF_V8:
            LowerResumeGenerator(gate);
            break;
        case GETRESUMEMODE_PREF_V8:
            LowerGetResumeMode(gate);
            break;
        case ITERNEXT_PREF_V8:
            LowerIterNext(gate, glue);
            break;
        case CLOSEITERATOR_PREF_V8:
            LowerCloseIterator(gate, glue);
            break;
        case ADD2DYN_PREF_V8:
            LowerAdd2(gate, glue);
            break;
        case SUB2DYN_PREF_V8:
            LowerSub2(gate, glue);
            break;
        case MUL2DYN_PREF_V8:
            LowerMul2(gate, glue);
            break;
        case DIV2DYN_PREF_V8:
            LowerDiv2(gate, glue);
            break;
        case MOD2DYN_PREF_V8:
            LowerMod2(gate, glue);
            break;
        case EQDYN_PREF_V8:
            LowerEq(gate, glue);
            break;
        case NOTEQDYN_PREF_V8:
            LowerNotEq(gate, glue);
            break;
        case LESSDYN_PREF_V8:
            LowerLess(gate, glue);
            break;
        case LESSEQDYN_PREF_V8:
            LowerLessEq(gate, glue);
            break;
        case GREATERDYN_PREF_V8:
            LowerGreater(gate, glue);
            break;
        case GREATEREQDYN_PREF_V8:
            LowerGreaterEq(gate, glue);
            break;
        case CREATEITERRESULTOBJ_PREF_V8_V8:
            LowerCreateIterResultObj(gate, glue);
            break;
        case SUSPENDGENERATOR_PREF_V8_V8:
            LowerSuspendGenerator(gate, glue, jsFunc);
            break;
        case ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8:
            LowerAsyncFunctionAwaitUncaught(gate, glue);
            break;
        case ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8:
            LowerAsyncFunctionResolve(gate, glue);
            break;
        case ASYNCFUNCTIONREJECT_PREF_V8_V8_V8:
            LowerAsyncFunctionReject(gate, glue);
            break;
        case TRYLDGLOBALBYNAME_PREF_ID32:
            LowerTryLdGlobalByName(gate, glue, jsFunc);
            break;
        case STGLOBALVAR_PREF_ID32:
            LowerStGlobalVar(gate, glue, jsFunc);
            break;
        case GETITERATOR_PREF:
            LowerGetIterator(gate, glue);
            break;
        case NEWOBJAPPLY_PREF_V8_V8:
            LowerNewObjApply(gate, glue);
            break;
        case THROWDYN_PREF:
            LowerThrow(gate, glue);
            break;
        case TYPEOFDYN_PREF:
            LowerTypeof(gate, glue);
            break;
        case THROWCONSTASSIGNMENT_PREF_V8:
            LowerThrowConstAssignment(gate, glue);
            break;
        case THROWTHROWNOTEXISTS_PREF:
            LowerThrowThrowNotExists(gate, glue);
            break;
        case THROWPATTERNNONCOERCIBLE_PREF:
            LowerThrowPatternNonCoercible(gate, glue);
            break;
        case THROWIFNOTOBJECT_PREF_V8:
            LowerThrowIfNotObject(gate, glue);
            break;
        case THROWUNDEFINEDIFHOLE_PREF_V8_V8:
            LowerThrowUndefinedIfHole(gate, glue);
            break;
        case THROWIFSUPERNOTCORRECTCALL_PREF_IMM16:
            LowerThrowIfSuperNotCorrectCall(gate, glue);
            break;
        case THROWDELETESUPERPROPERTY_PREF:
            LowerThrowDeleteSuperProperty(gate, glue);
            break;
        case LDGLOBALTHIS_PREF:
            LowerLdGlobal(gate, glue);
            break;
        case LDSYMBOL_PREF:
            LowerLdSymbol(gate, glue);
            break;
        case LDGLOBAL_PREF:
            LowerLdGlobal(gate, glue);
            break;
        case TONUMBER_PREF_V8:
            LowerToNumber(gate, glue);
            break;
        case NEGDYN_PREF_V8:
            LowerNeg(gate, glue);
            break;
        case NOTDYN_PREF_V8:
            LowerNot(gate, glue);
            break;
        case SHL2DYN_PREF_V8:
            LowerShl2(gate, glue);
            break;
        case SHR2DYN_PREF_V8:
            LowerShr2(gate, glue);
            break;
        case ASHR2DYN_PREF_V8:
            LowerAshr2(gate, glue);
            break;
        case AND2DYN_PREF_V8:
            LowerAnd2(gate, glue);
            break;
        case OR2DYN_PREF_V8:
            LowerOr2(gate, glue);
            break;
        case XOR2DYN_PREF_V8:
            LowerXor2(gate, glue);
            break;
        case DELOBJPROP_PREF_V8_V8:
            LowerDelObjProp(gate, glue);
            break;
        case DEFINEMETHOD_PREF_ID16_IMM16_V8:
            LowerDefineMethod(gate, glue, jsFunc);
            break;
        case EXPDYN_PREF_V8:
            LowerExp(gate, glue);
            break;
        case ISINDYN_PREF_V8:
            LowerIsIn(gate, glue);
            break;
        case INSTANCEOFDYN_PREF_V8:
            LowerInstanceof(gate, glue);
            break;
        case STRICTNOTEQDYN_PREF_V8:
            LowerFastStrictNotEqual(gate, glue);
            break;
        case STRICTEQDYN_PREF_V8:
            LowerFastStrictEqual(gate, glue);
            break;
        case CREATEEMPTYARRAY_PREF:
            LowerCreateEmptyArray(gate, glue);
            break;
        case CREATEEMPTYOBJECT_PREF:
            LowerCreateEmptyObject(gate, glue);
            break;
        case CREATEOBJECTWITHBUFFER_PREF_IMM16:
            LowerCreateObjectWithBuffer(gate, glue, jsFunc);
            break;
        case CREATEARRAYWITHBUFFER_PREF_IMM16:
            LowerCreateArrayWithBuffer(gate, glue, jsFunc);
            break;
        case STMODULEVAR_PREF_ID32:
            LowerStModuleVar(gate, glue, jsFunc);
            break;
        case GETTEMPLATEOBJECT_PREF_V8:
            LowerGetTemplateObject(gate, glue);
            break;
        case SETOBJECTWITHPROTO_PREF_V8_V8:
            LowerSetObjectWithProto(gate, glue);
            break;
        case LDBIGINT_PREF_ID32:
            LowerLdBigInt(gate, glue, jsFunc);
            break;
        case TONUMERIC_PREF_V8:
            LowerToNumeric(gate, glue);
            break;
        case DYNAMICIMPORT_PREF_V8:
            LowerDynamicImport(gate, glue, jsFunc);
            break;
        case LDMODULEVAR_PREF_ID32_IMM8:
            LowerLdModuleVar(gate, glue, jsFunc);
            break;
        case GETMODULENAMESPACE_PREF_ID32:
            LowerGetModuleNamespace(gate, glue, jsFunc);
            break;
        case NEWOBJDYNRANGE_PREF_IMM16_V8:
            LowerNewObjRange(gate, glue);
            break;
        case JEQZ_IMM8:
        case JEQZ_IMM16:
            LowerConditionJump(gate, true);
            break;
        case JNEZ_IMM8:
        case JNEZ_IMM16:
            LowerConditionJump(gate, false);
            break;
        case GETITERATORNEXT_PREF_V8_V8:
            LowerGetIteratorNext(gate, glue);
            break;
        case SUPERCALL_PREF_IMM16_V8:
            LowerSuperCall(gate, glue, newTarget);
            break;
        case SUPERCALLSPREAD_PREF_V8:
            LowerSuperCallSpread(gate, glue, newTarget);
            break;
        case ISTRUE_PREF:
            LowerIsTrueOrFalse(gate, glue, true);
            break;
        case ISFALSE_PREF:
            LowerIsTrueOrFalse(gate, glue, false);
            break;
        case GETNEXTPROPNAME_PREF_V8:
            LowerGetNextPropName(gate, glue);
            break;
        case COPYDATAPROPERTIES_PREF_V8_V8:
            LowerCopyDataProperties(gate, glue);
            break;
        case CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
            LowerCreateObjectWithExcludedKeys(gate, glue);
            break;
        case CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8:
            LowerCreateRegExpWithLiteral(gate, glue, jsFunc);
            break;
        case STOWNBYVALUE_PREF_V8_V8:
            LowerStOwnByValue(gate, glue);
            break;
        case STOWNBYINDEX_PREF_V8_IMM32:
            LowerStOwnByIndex(gate, glue);
            break;
        case STOWNBYNAME_PREF_ID32_V8:
            LowerStOwnByName(gate, glue, jsFunc);
            break;
        case NEWLEXENVDYN_PREF_IMM16:
            LowerNewLexicalEnv(gate, glue);
            break;
        case NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16:
            LowerNewLexicalEnvWithName(gate, glue, jsFunc);
            break;
        case POPLEXENVDYN_PREF:
            LowerPopLexicalEnv(gate, glue);
            break;
        case LDSUPERBYVALUE_PREF_V8_V8:
            LowerLdSuperByValue(gate, glue, jsFunc);
            break;
        case STSUPERBYVALUE_PREF_V8_V8:
            LowerStSuperByValue(gate, glue, jsFunc);
            break;
        case TRYSTGLOBALBYNAME_PREF_ID32:
            LowerTryStGlobalByName(gate, glue, jsFunc);
            break;
        case STCONSTTOGLOBALRECORD_PREF_ID32:
            LowerStConstToGlobalRecord(gate, glue, jsFunc);
            break;
        case STLETTOGLOBALRECORD_PREF_ID32:
            LowerStLetToGlobalRecord(gate, glue, jsFunc);
            break;
        case STCLASSTOGLOBALRECORD_PREF_ID32:
            LowerStClassToGlobalRecord(gate, glue, jsFunc);
            break;
        case STOWNBYVALUEWITHNAMESET_PREF_V8_V8:
            LowerStOwnByValueWithNameSet(gate, glue);
            break;
        case STOWNBYNAMEWITHNAMESET_PREF_ID32_V8:
            LowerStOwnByNameWithNameSet(gate, glue, jsFunc);
            break;
        case LDGLOBALVAR_PREF_ID32:
            LowerLdGlobalVar(gate, glue, jsFunc);
            break;
        case LDOBJBYNAME_PREF_ID32_V8:
            LowerLdObjByName(gate, glue, jsFunc);
            break;
        case STOBJBYNAME_PREF_ID32_V8:
            LowerStObjByName(gate, glue, jsFunc);
            break;
        case DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8:
            LowerDefineGetterSetterByValue(gate, glue);
            break;
        case LDOBJBYINDEX_PREF_V8_IMM32:
            LowerLdObjByIndex(gate, glue);
            break;
        case STOBJBYINDEX_PREF_V8_IMM32:
            LowerStObjByIndex(gate, glue);
            break;
        case LDOBJBYVALUE_PREF_V8_V8:
            LowerLdObjByValue(gate, glue);
            break;
        case STOBJBYVALUE_PREF_V8_V8:
            LowerStObjByValue(gate, glue);
            break;
        case LDSUPERBYNAME_PREF_ID32_V8:
            LowerLdSuperByName(gate, glue, jsFunc);
            break;
        case STSUPERBYNAME_PREF_ID32_V8:
            LowerStSuperByName(gate, glue, jsFunc);
            break;
        case CREATEGENERATOROBJ_PREF_V8:
            LowerCreateGeneratorObj(gate, glue);
            break;
        case CREATEASYNCGENERATOROBJ_PREF_V8:
            LowerCreateAsyncGeneratorObj(gate, glue);
            break;
        case ASYNCGENERATORRESOLVE_PREF_V8_V8_V8:
            LowerAsyncGeneratorResolve(gate, glue);
            break;
        case ASYNCGENERATORREJECT_PREF_V8_V8:
            LowerAsyncGeneratorReject(gate, glue);
            break;
        case STARRAYSPREAD_PREF_V8_V8:
            LowerStArraySpread(gate, glue);
            break;
        case LDLEXVARDYN_PREF_IMM4_IMM4:
        case LDLEXVARDYN_PREF_IMM8_IMM8:
        case LDLEXVARDYN_PREF_IMM16_IMM16:
            LowerLdLexVar(gate, glue);
            break;
        case STLEXVARDYN_PREF_IMM4_IMM4_V8:
        case STLEXVARDYN_PREF_IMM8_IMM8_V8:
        case STLEXVARDYN_PREF_IMM16_IMM16_V8:
            LowerStLexVar(gate, glue);
            break;
        case CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
            LowerCreateObjectHavingMethod(gate, glue, jsFunc);
            break;
        case LDHOMEOBJECT_PREF:
            LowerLdHomeObject(gate, jsFunc);
            break;
        case DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
            LowerDefineClassWithBuffer(gate, glue, jsFunc);
            break;
        case DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
            LowerDefineFunc(gate, glue, jsFunc);
            break;
        case COPYRESTARGS_PREF_IMM16:
            LowerCopyRestArgs(gate, glue, actualArgc);
            break;
        default:
            break;
    }
}

GateRef SlowPathLowering::LowerCallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args);
        return result;
    }
}

void SlowPathLowering::LowerAdd2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Add2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    auto args =  {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)};
    GateRef newGate = LowerCallRuntime(glue, id, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateIterResultObj(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(CreateIterResultObj);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

// When executing to SUSPENDGENERATOR instruction, save contextual information to GeneratorContext,
// including registers, acc, etc.
void SlowPathLowering::SaveFrameToContext(GateRef gate, GateRef glue, GateRef jsFunc)
{
    GateRef genObj = acc_.GetValueIn(gate, 1);
    GateRef saveRegister = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(saveRegister) == OpCode::SAVE_REGISTER);
    std::vector<GateRef> saveRegisterGates {};
    while (acc_.GetOpCode(saveRegister) == OpCode::SAVE_REGISTER) {
        saveRegisterGates.emplace_back(saveRegister);
        saveRegister = acc_.GetDep(saveRegister);
    }
    acc_.SetDep(gate, saveRegister);
    builder_.SetDepend(saveRegister);
    GateRef context =
        builder_.Load(VariableType::JS_POINTER(), genObj, builder_.IntPtr(JSGeneratorObject::GENERATOR_CONTEXT_OFFSET));
    // new tagged array
    auto method = bcBuilder_->GetMethod();
    auto jsPandaFile = bcBuilder_->GetJSPandaFile();
    const size_t arrLength = MethodLiteral::GetNumVregs(jsPandaFile, method) + method->GetNumArgs();
    GateRef length = builder_.Int32((arrLength));
    GateRef taggedLength = builder_.ToTaggedInt(builder_.ZExtInt32ToInt64(length));
    const int arrayId = RTSTUB_ID(NewTaggedArray);
    GateRef taggedArray = LowerCallRuntime(glue, arrayId, {taggedLength});
    // setRegsArrays
    for (auto item : saveRegisterGates) {
        auto index = acc_.GetBitField(item);
        auto valueGate = acc_.GetValueIn(item);
        builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue, taggedArray, builder_.Int32(index), valueGate);
        acc_.DeleteGate(item);
    }

    // setRegsArrays
    GateRef regsArrayOffset = builder_.IntPtr(GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET);
    builder_.Store(VariableType::JS_POINTER(), glue, context, regsArrayOffset, taggedArray);

    // set method
    GateRef methodOffset = builder_.IntPtr(GeneratorContext::GENERATOR_METHOD_OFFSET);
    builder_.Store(VariableType::JS_ANY(), glue, context, methodOffset, jsFunc);

    // set acc
    GateRef accOffset = builder_.IntPtr(GeneratorContext::GENERATOR_ACC_OFFSET);
    GateRef curAccGate = acc_.GetValueIn(gate, 3); // get current acc
    builder_.Store(VariableType::JS_ANY(), glue, context, accOffset, curAccGate);

    // set generator object
    GateRef generatorObjectOffset = builder_.IntPtr(GeneratorContext::GENERATOR_GENERATOR_OBJECT_OFFSET);
    builder_.Store(VariableType::JS_ANY(), glue, context, generatorObjectOffset, genObj);

    // set lexical env
    const int id = RTSTUB_ID(OptGetLexicalEnv);
    GateRef lexicalEnvGate = LowerCallRuntime(glue, id, {jsFunc});
    GateRef lexicalEnvOffset = builder_.IntPtr(GeneratorContext::GENERATOR_LEXICALENV_OFFSET);
    builder_.Store(VariableType::JS_ANY(), glue, context, lexicalEnvOffset, lexicalEnvGate);

    // set nregs
    GateRef nregsOffset = builder_.IntPtr(GeneratorContext::GENERATOR_NREGS_OFFSET);
    builder_.Store(VariableType::INT32(), glue, context, nregsOffset, length);

    // set bc size
    GateRef bcSizeOffset = builder_.IntPtr(GeneratorContext::GENERATOR_BC_OFFSET_OFFSET);
    GateRef bcSizeGate = acc_.GetValueIn(gate, 0); // saved bc_offset
    bcSizeGate = builder_.TruncInt64ToInt32(bcSizeGate);
    builder_.Store(VariableType::INT32(), glue, context, bcSizeOffset, bcSizeGate);

    // set context to generator object
    GateRef contextOffset = builder_.IntPtr(JSGeneratorObject::GENERATOR_CONTEXT_OFFSET);
    builder_.Store(VariableType::JS_POINTER(), glue, genObj, contextOffset, context);

    // set generator object to context
    builder_.Store(VariableType::JS_POINTER(), glue, context, generatorObjectOffset, genObj);
}

void SlowPathLowering::LowerSuspendGenerator(GateRef gate, GateRef glue, [[maybe_unused]]GateRef jsFunc)
{
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    SaveFrameToContext(gate, glue, jsFunc);
    acc_.SetDep(gate, builder_.GetDepend());
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(OptSuspendGenerator);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionAwaitUncaught(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(AsyncFunctionAwaitUncaught);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionResolve(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(AsyncFunctionResolveOrReject);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef taggedTrue = builder_.TaggedTrue();
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), taggedTrue});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionReject(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(AsyncFunctionResolveOrReject);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef taggedFalse = builder_.TaggedFalse();
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), taggedFalse});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLoadStr(GateRef gate, [[maybe_unused]] GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef newGate = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    builder_.Branch(builder_.IsSpecial(newGate, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, newGate, successControl, failControl);
}

void SlowPathLowering::LowerLexicalEnv(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(OptGetLexicalEnv);
    GateRef newGate = LowerCallRuntime(glue, id, {});
    ReplaceHirToCall(gate, newGate, true);
}

void SlowPathLowering::LowerTryLdGlobalByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), acc_.GetValueIn(gate, 0));
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef recordResult = LowerCallRuntime(glue, RTSTUB_ID(LdGlobalRecord), {prop}, true);
    Label isFound(&builder_);
    Label isNotFound(&builder_);
    Label success(&builder_);
    Label exception(&builder_);
    Label defaultLabel(&builder_);
    GateRef value = 0;
    builder_.Branch(builder_.TaggedIsUndefined(recordResult), &isNotFound, &isFound);
    builder_.Bind(&isNotFound);
    {
        Label notException(&builder_);
        result = LowerCallRuntime(glue, RTSTUB_ID(TryLdGlobalByName), {prop}, true);
        builder_.Branch(builder_.IsSpecial(*result, JSTaggedValue::VALUE_EXCEPTION),
            &exception, &notException);
        builder_.Bind(&notException);
        {
            builder_.Jump(&defaultLabel);
        }
    }
    builder_.Bind(&isFound);
    {
        result = builder_.Load(VariableType::JS_ANY(), recordResult, builder_.IntPtr(PropertyBox::VALUE_OFFSET));
        builder_.Jump(&defaultLabel);
    }
    builder_.Bind(&defaultLabel);
    {
        value = *result;
        builder_.Jump(&success);
    }
    CREATE_DOUBLE_EXIT(success, exception);
    ReplaceHirToSubCfg(gate, value, successControl, failControl);
}

void SlowPathLowering::LowerStGlobalVar(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    const int id = RTSTUB_ID(StGlobalVar);
    ASSERT(acc_.GetNumValueIn(gate) == 2); // 2: number of value inputs
    GateRef newGate = LowerCallRuntime(glue, id, {prop, acc_.GetValueIn(gate, 1)});
    builder_.Branch(builder_.IsSpecial(newGate, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, newGate, successControl, failControl);
}

void SlowPathLowering::LowerGetIterator(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    auto result = LowerCallRuntime(glue, RTSTUB_ID(GetIterator), {acc_.GetValueIn(gate, 0)}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerToJSCall(GateRef gate, GateRef glue, const std::vector<GateRef> &args)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCall));
    GateRef target = builder_.IntPtr(RTSTUB_ID(JSCall));
    GateRef newGate = builder_.Call(cs, glue, target, dependEntry_, args);
    ReplaceHirToJSCall(gate, newGate, glue);
}

void SlowPathLowering::LowerCallArg0(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);

    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaBytecode::CALLARG0DYN_PREF_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef func = acc_.GetValueIn(gate, 0);
    GateRef env = builder_.Undefined();
    GateRef bcOffset = acc_.GetValueIn(gate, 1);
    LowerToJSCall(gate, glue, {glue, env, actualArgc, func, newTarget, thisObj, bcOffset});
}

void SlowPathLowering::LowerCallArg1(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    // 2: func and bcoffset
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaBytecode::CALLARG1DYN_PREF_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef func = acc_.GetValueIn(gate, 0);
    GateRef env = builder_.Undefined();
    GateRef bcOffset = acc_.GetValueIn(gate, 2); // 2: bytecode offset
    LowerToJSCall(gate, glue, {glue, env, actualArgc, func, newTarget, thisObj, acc_.GetValueIn(gate, 1), bcOffset});
}

void SlowPathLowering::LowerCallArgs2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    // 2: func and bcoffset
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaBytecode::CALLARGS2DYN_PREF_V8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef func = acc_.GetValueIn(gate, 0);
    GateRef env = builder_.Undefined();
    GateRef bcOffset = acc_.GetValueIn(gate, 3); // 3: bytecode offset
    LowerToJSCall(gate, glue, {glue, env, actualArgc, func, newTarget, thisObj, acc_.GetValueIn(gate, 1),
        acc_.GetValueIn(gate, 2), bcOffset});
}

void SlowPathLowering::LowerCallArgs3(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaBytecode::CALLARGS3DYN_PREF_V8_V8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef func = acc_.GetValueIn(gate, 0);
    GateRef env = builder_.Undefined();
    GateRef bcOffset = acc_.GetValueIn(gate, 4); // 4: bytecode offset
    LowerToJSCall(gate, glue, {glue, env, actualArgc, func, newTarget, thisObj, acc_.GetValueIn(gate, 1),
        acc_.GetValueIn(gate, 2), acc_.GetValueIn(gate, 3), bcOffset});
}

void SlowPathLowering::LowerCallThisRange(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> vec;
    // The first register input is callTarget, second is thisObj and other inputs are common args.
    size_t fixedInputsNum = 2;
    ASSERT(acc_.GetNumValueIn(gate) - fixedInputsNum >= 0);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaBytecode::CALLITHISRANGEDYN_PREF_IMM16_V8));
    GateRef callTarget = acc_.GetValueIn(gate, 0);
    GateRef thisObj = acc_.GetValueIn(gate, 1);
    GateRef newTarget = builder_.Undefined();
    GateRef env = builder_.Undefined();
    vec.emplace_back(glue);
    vec.emplace_back(env);
    vec.emplace_back(actualArgc);
    vec.emplace_back(callTarget);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);
    // add common args
    size_t numIns = acc_.GetNumValueIn(gate);
    for (size_t i = fixedInputsNum; i < numIns; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    LowerToJSCall(gate, glue, vec);
}

void SlowPathLowering::LowerCallSpread(GateRef gate, GateRef glue)
{
    // need to fixed in later
    const int id = RTSTUB_ID(CallSpread);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = LowerCallRuntime(glue, id,
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallRange(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> vec;
    size_t numArgs = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaBytecode::CALLIRANGEDYN_PREF_IMM16_V8));
    GateRef callTarget = acc_.GetValueIn(gate, 0);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef env = builder_.Undefined();
    vec.emplace_back(glue);
    vec.emplace_back(env);
    vec.emplace_back(actualArgc);
    vec.emplace_back(callTarget);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);

    for (size_t i = 1; i < numArgs; i++) { // skip imm
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    LowerToJSCall(gate, glue, vec);
}

void SlowPathLowering::LowerNewObjApply(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(NewObjApply);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id,
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerThrow(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    GateRef exception = acc_.GetValueIn(gate, 0);
    GateRef exceptionOffset = builder_.Int64(JSThread::GlueData::GetExceptionOffset(false));
    GateRef val = builder_.Int64Add(glue, exceptionOffset);
    GateRef setException = circuit_->NewGate(OpCode(OpCode::STORE), 0,
        {dependEntry_, exception, val}, VariableType::INT64().GetGateType());
    ReplaceHirToThrowCall(gate, setException);
}

void SlowPathLowering::LowerThrowConstAssignment(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(ThrowConstAssignment);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToThrowCall(gate, newGate);
}

void SlowPathLowering::LowerThrowThrowNotExists(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(ThrowThrowNotExists);
    GateRef newGate = LowerCallRuntime(glue, id, {});
    ReplaceHirToThrowCall(gate, newGate);
}

void SlowPathLowering::LowerThrowPatternNonCoercible(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(ThrowPatternNonCoercible);
    GateRef newGate = LowerCallRuntime(glue, id, {});
    ReplaceHirToThrowCall(gate, newGate);
}

void SlowPathLowering::LowerThrowIfNotObject(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef value = acc_.GetValueIn(gate, 0);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label isEcmaObject(&builder_);
    Label notEcmaObject(&builder_);
    Label isHeapObject(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(value), &isHeapObject, &notEcmaObject);
    builder_.Bind(&isHeapObject);
    builder_.Branch(builder_.TaggedObjectIsEcmaObject(value), &isEcmaObject, &notEcmaObject);
    builder_.Bind(&isEcmaObject);
    {
        builder_.Jump(&successExit);
    }
    builder_.Bind(&notEcmaObject);
    {
        LowerCallRuntime(glue, RTSTUB_ID(ThrowIfNotObject), {}, true);
        builder_.Jump(&exceptionExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerThrowUndefinedIfHole(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef hole = acc_.GetValueIn(gate, 0);
    GateRef obj =  acc_.GetValueIn(gate, 1);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label isHole(&builder_);
    Label notHole(&builder_);
    builder_.Branch(builder_.TaggedIsHole(hole), &isHole, &notHole);
    builder_.Bind(&notHole);
    {
        builder_.Jump(&successExit);
    }
    builder_.Bind(&isHole);
    {
        LowerCallRuntime(glue, RTSTUB_ID(ThrowUndefinedIfHole), {obj}, true);
        builder_.Jump(&exceptionExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerThrowIfSuperNotCorrectCall(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        {builder_.ToTaggedInt(acc_.GetValueIn(gate, 0)), acc_.GetValueIn(gate, 1)}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerThrowDeleteSuperProperty(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(ThrowDeleteSuperProperty);
    GateRef newGate = LowerCallRuntime(glue, id, {});
    ReplaceHirToThrowCall(gate, newGate);
}

void SlowPathLowering::LowerExceptionHandler(GateRef hirGate)
{
    GateRef glue = argAcc_.GetCommonArgGate(CommonArgIdx::GLUE);
    GateRef depend = acc_.GetDep(hirGate);
    GateRef exceptionOffset = builder_.Int64(JSThread::GlueData::GetExceptionOffset(false));
    GateRef val = builder_.Int64Add(glue, exceptionOffset);
    GateRef loadException = circuit_->NewGate(OpCode(OpCode::LOAD), VariableType::JS_ANY().GetMachineType(),
        0, { depend, val }, VariableType::JS_ANY().GetGateType());
    acc_.SetDep(loadException, depend);
    GateRef holeCst = builder_.HoleConstant(VariableType::JS_ANY().GetGateType());
    GateRef clearException = circuit_->NewGate(OpCode(OpCode::STORE), 0,
        { loadException, holeCst, val }, VariableType::INT64().GetGateType());
    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetOpCode(*it) != OpCode::VALUE_SELECTOR && acc_.IsDependIn(it)) {
            it = acc_.ReplaceIn(it, clearException);
        } else {
            it = acc_.ReplaceIn(it, loadException);
        }
    }
    acc_.DeleteGate(hirGate);
}

void SlowPathLowering::LowerLdSymbol(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(GetSymbolFunction);
    GateRef newGate = LowerCallRuntime(glue, id, {});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdGlobal(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    GateRef offset = builder_.Int64(JSThread::GlueData::GetGlobalObjOffset(false));
    GateRef val = builder_.Int64Add(glue, offset);
    GateRef newGate = circuit_->NewGate(OpCode(OpCode::LOAD), VariableType::JS_ANY().GetMachineType(),
        0, { dependEntry_, val }, VariableType::JS_ANY().GetGateType());
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSub2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Sub2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerMul2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Mul2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDiv2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Div2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerMod2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Mod2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerEq(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Eq);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNotEq(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(NotEq);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLess(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Less);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLessEq(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(LessEq);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGreater(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Greater);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGreaterEq(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(GreaterEq);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerGetPropIterator(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(GetPropIterator);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIterNext(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(IterNext);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCloseIterator(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(CloseIterator);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerInc(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Inc);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDec(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Dec);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerToNumber(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(ToNumber);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNeg(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Neg);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerNot(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Not);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerShl2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Shl2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerShr2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Shr2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAshr2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Ashr2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAnd2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(And2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerOr2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Or2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerXor2(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Xor2);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDelObjProp(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(DelObjProp);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerExp(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(Exp);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIsIn(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(IsIn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerInstanceof(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(InstanceOf);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerFastStrictNotEqual(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label strictEqual(&builder_);
    Label notStrictEqual(&builder_);
    Label exit(&builder_);
    builder_.Branch(FastStrictEqual(glue, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)), &strictEqual,
        &notStrictEqual);
    builder_.Bind(&strictEqual);
    {
        result = builder_.Int64ToTaggedPtr(builder_.TaggedFalse());
        builder_.Jump(&exit);
    }
    builder_.Bind(&notStrictEqual);
    {
        result = builder_.Int64ToTaggedPtr(builder_.TaggedTrue());
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

void SlowPathLowering::LowerFastStrictEqual(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label strictEqual(&builder_);
    Label notStrictEqual(&builder_);
    Label exit(&builder_);
    builder_.Branch(FastStrictEqual(glue, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)), &strictEqual,
        &notStrictEqual);
    builder_.Bind(&strictEqual);
    {
        result = builder_.Int64ToTaggedPtr(builder_.TaggedTrue());
        builder_.Jump(&exit);
    }
    builder_.Bind(&notStrictEqual);
    {
        result = builder_.Int64ToTaggedPtr(builder_.TaggedFalse());
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

GateRef SlowPathLowering::FastStrictEqual(GateRef glue, GateRef left, GateRef right)
{
    auto env = builder_.GetCurrentEnvironment();
    Label entry(&builder_);
    env->SubCfgEntry(&entry);

    DEFVAlUE(result, (&builder_), VariableType::BOOL(), builder_.False());
    Label leftIsNumber(&builder_);
    Label leftNotNumber(&builder_);
    Label sameVariableCheck(&builder_);
    Label stringEqCheck(&builder_);
    Label stringCompare(&builder_);
    Label bigIntEqualCheck(&builder_);
    Label exit(&builder_);
    builder_.Branch(builder_.TaggedIsNumber(left), &leftIsNumber, &leftNotNumber);
    builder_.Bind(&leftIsNumber);
    {
        Label rightIsNumber(&builder_);
        builder_.Branch(builder_.TaggedIsNumber(right), &rightIsNumber, &exit);
        builder_.Bind(&rightIsNumber);
        {
            DEFVAlUE(doubleLeft, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
            DEFVAlUE(doubleRight, (&builder_), VariableType::FLOAT64(), builder_.Double(0.0));
            Label leftIsInt(&builder_);
            Label leftNotInt(&builder_);
            Label getRight(&builder_);
            Label strictNumberEqualCheck(&builder_);
            builder_.Branch(builder_.TaggedIsInt(left), &leftIsInt, &leftNotInt);
            builder_.Bind(&leftIsInt);
            {
                doubleLeft = builder_.ChangeInt32ToFloat64(builder_.TaggedCastToInt32(left));
                builder_.Jump(&getRight);
            }
            builder_.Bind(&leftNotInt);
            {
                doubleLeft = builder_.TaggedCastToDouble(left);
                builder_.Jump(&getRight);
            }
            builder_.Bind(&getRight);
            {
                Label rightIsInt(&builder_);
                Label rightNotInt(&builder_);
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightNotInt);
                builder_.Bind(&rightIsInt);
                {
                    doubleRight = builder_.ChangeInt32ToFloat64(builder_.TaggedCastToInt32(right));
                    builder_.Jump(&strictNumberEqualCheck);
                }
                builder_.Bind(&rightNotInt);
                {
                    doubleRight = builder_.TaggedCastToDouble(right);
                    builder_.Jump(&strictNumberEqualCheck);
                }
            }
            builder_.Bind(&strictNumberEqualCheck);
            {
                Label leftNotNan(&builder_);
                Label numberCheck(&builder_);
                builder_.Branch(builder_.DoubleIsNAN(*doubleLeft), &exit, &leftNotNan);
                builder_.Bind(&leftNotNan);
                {
                    Label rightNotNan(&builder_);
                    builder_.Branch(builder_.DoubleIsNAN(*doubleRight), &exit, &rightNotNan);
                    builder_.Bind(&rightNotNan);
                    {
                        result = builder_.Equal(*doubleLeft, *doubleRight);
                        builder_.Jump(&exit);
                    }
                }
            }
        }
    }
    builder_.Bind(&leftNotNumber);
    builder_.Branch(builder_.TaggedIsNumber(right), &exit, &sameVariableCheck);
    builder_.Bind(&sameVariableCheck);
    {
        Label strictEq(&builder_);
        builder_.Branch(builder_.Equal(left, right), &strictEq, &stringEqCheck);
        builder_.Bind(&strictEq);
        {
            result = builder_.True();
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&stringEqCheck);
    builder_.Branch(builder_.BothAreString(left, right), &stringCompare, &bigIntEqualCheck);
    builder_.Bind(&stringCompare);
    {
        Label lengthCompare(&builder_);
        Label hashcodeCompare(&builder_);
        Label contentsCompare(&builder_);
        builder_.Branch(builder_.Equal(builder_.ZExtInt1ToInt32(builder_.IsUtf16String(left)),
            builder_.ZExtInt1ToInt32(builder_.IsUtf16String(right))), &lengthCompare, &exit);
        builder_.Bind(&lengthCompare);
        builder_.Branch(builder_.Equal(builder_.GetLengthFromString(left), builder_.GetLengthFromString(right)),
            &hashcodeCompare, &exit);
        builder_.Bind(&hashcodeCompare);
        builder_.Branch(
            builder_.Equal(builder_.GetHashcodeFromString(glue, left), builder_.GetHashcodeFromString(glue, right)),
            &contentsCompare, &exit);
        builder_.Bind(&contentsCompare);
        {
            GateRef stringEqual = LowerCallRuntime(glue, RTSTUB_ID(StringEqual), { left, right }, true);
            result = builder_.Equal(stringEqual, builder_.TaggedTrue());
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&bigIntEqualCheck);
    {
        Label leftIsBigInt(&builder_);
        Label leftIsNotBigInt(&builder_);
        builder_.Branch(builder_.TaggedIsBigInt(left), &leftIsBigInt, &exit);
        builder_.Bind(&leftIsBigInt);
        {
            Label rightIsBigInt(&builder_);
            builder_.Branch(builder_.TaggedIsBigInt(right), &rightIsBigInt, &exit);
            builder_.Bind(&rightIsBigInt);
            {
                GateRef bigIntEqual = LowerCallRuntime(glue, RTSTUB_ID(BigIntEqual), { left, right }, true);
                result = builder_.Equal(bigIntEqual, builder_.TaggedTrue());
                builder_.Jump(&exit);
            }
        }
    }
    builder_.Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void SlowPathLowering::LowerCreateEmptyArray(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(CreateEmptyArray), {}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerCreateEmptyObject(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(CreateEmptyObject), {}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerCreateArrayWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = GetObjectFromConstPool(jsFunc, builder_.TruncInt64ToInt32(index));
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(CreateArrayWithBuffer), { obj }, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerCreateObjectWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = GetObjectFromConstPool(jsFunc, builder_.TruncInt64ToInt32(index));
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(CreateObjectWithBuffer), { obj }, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStModuleVar(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(StModuleVarOnJSFunc),
        { prop, acc_.GetValueIn(gate, 1), jsFunc }, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerGetTemplateObject(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(GetTemplateObject);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef literal = acc_.GetValueIn(gate, 0);
    GateRef newGate = LowerCallRuntime(glue, id, { literal });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSetObjectWithProto(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(SetObjectWithProto);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef proto = acc_.GetValueIn(gate, 0);
    GateRef obj = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(glue, id, { proto, obj });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdBigInt(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef numberBigInt = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(LdBigInt), {numberBigInt}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerToNumeric(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(ToNumeric);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerDynamicImport(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(DynamicImport);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), jsFunc});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdModuleVar(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef key = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef inner = builder_.ToTaggedInt(acc_.GetValueIn(gate, 1));
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(LdModuleVarOnJSFunc), {key, inner, jsFunc}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerGetModuleNamespace(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef localName = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(GetModuleNamespaceOnJSFunc), { localName, jsFunc }, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerGetIteratorNext(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(GetIteratorNext);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef method = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(glue, id, { obj, method });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuperCall(GateRef gate, GateRef glue, GateRef newTarget)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(OptSuperCall);
    std::vector<GateRef> vec;
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) > 0);
    size_t numIns = acc_.GetNumValueIn(gate);
    size_t funcIndex = numIns - 1;
    GateRef func = acc_.GetValueIn(gate, funcIndex);
    vec.emplace_back(func);
    vec.emplace_back(newTarget);
    for (size_t i = 0; i < funcIndex; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef newGate = LowerCallRuntime(glue, id, vec);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuperCallSpread(GateRef gate, GateRef glue, GateRef newTarget)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(OptSuperCallSpread);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef func = acc_.GetValueIn(gate, 1);
    GateRef array = acc_.GetValueIn(gate, 0);
    GateRef newGate = LowerCallRuntime(glue, id, { func, newTarget, array });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIsTrueOrFalse(GateRef gate, GateRef glue, bool flag)
{
    DebugPrintBC(gate, glue);
    Label isTrue(&builder_);
    Label isFalse(&builder_);
    Label successExit(&builder_);
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), acc_.GetValueIn(gate, 0));
    GateRef callResult = LowerCallRuntime(glue, RTSTUB_ID(ToBoolean), { *result }, true);
    builder_.Branch(builder_.IsSpecial(callResult, JSTaggedValue::VALUE_TRUE),
        &isTrue, &isFalse);
    builder_.Bind(&isTrue);
    {
        auto trueResult = flag ? builder_.TaggedTrue() : builder_.TaggedFalse();
        result = builder_.Int64ToTaggedPtr(trueResult);
        builder_.Jump(&successExit);
    }
    builder_.Bind(&isFalse);
    {
        auto falseResult = flag ? builder_.TaggedFalse() : builder_.TaggedTrue();
        result = builder_.Int64ToTaggedPtr(falseResult);
        builder_.Jump(&successExit);
    }
    builder_.Bind(&successExit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerNewObjRange(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(OptNewObjRange);
    size_t range = acc_.GetNumValueIn(gate);
    std::vector<GateRef> args(range);
    for (size_t i = 0; i < range; ++i) {
        args[i] = acc_.GetValueIn(gate, i);
    }
    GateRef newGate = LowerCallRuntime(glue, id, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerConditionJump(GateRef gate, bool isEqualJump)
{
    std::vector<GateRef> trueState;
    GateRef value = acc_.GetValueIn(gate, 0);
    // GET_ACC() == JSTaggedValue::False()
    GateRef condition = builder_.IsSpecial(value, JSTaggedValue::VALUE_FALSE);
    GateRef ifBranch = builder_.Branch(acc_.GetState(gate), condition);
    GateRef ifTrue = builder_.IfTrue(ifBranch);
    GateRef ifFalse = builder_.IfFalse(ifBranch);
    trueState.emplace_back(isEqualJump ? ifTrue : ifFalse);

    // (GET_ACC().IsInt() && GET_ACC().GetInt())
    std::vector<GateRef> intFalseState;
    ifBranch = isEqualJump ? builder_.Branch(ifFalse, builder_.TaggedIsInt(value))
        : builder_.Branch(ifTrue, builder_.TaggedIsInt(value));
    GateRef isInt = builder_.IfTrue(ifBranch);
    GateRef notInt = builder_.IfFalse(ifBranch);
    intFalseState.emplace_back(notInt);
    condition = builder_.Equal(builder_.TaggedGetInt(value), builder_.Int32(0));
    ifBranch = builder_.Branch(isInt, condition);
    GateRef isZero = builder_.IfTrue(ifBranch);
    GateRef notZero = builder_.IfFalse(ifBranch);
    trueState.emplace_back(isEqualJump ? isZero : notZero);
    intFalseState.emplace_back(isEqualJump ? notZero : isZero);
    auto mergeIntState = builder_.Merge(intFalseState.data(), intFalseState.size());

    // (GET_ACC().IsDouble() && GET_ACC().GetDouble() == 0)
    std::vector<GateRef> doubleFalseState;
    ifBranch = builder_.Branch(mergeIntState, builder_.TaggedIsDouble(value));
    GateRef isDouble = builder_.IfTrue(ifBranch);
    GateRef notDouble = builder_.IfFalse(ifBranch);
    doubleFalseState.emplace_back(notDouble);
    condition = builder_.Equal(builder_.TaggedCastToDouble(value), builder_.Double(0));
    ifBranch = builder_.Branch(isDouble, condition);
    GateRef isDoubleZero = builder_.IfTrue(ifBranch);
    GateRef notDoubleZero = builder_.IfFalse(ifBranch);
    trueState.emplace_back(isEqualJump ? isDoubleZero : notDoubleZero);
    doubleFalseState.emplace_back(isEqualJump ? notDoubleZero : isDoubleZero);
    auto mergeFalseState = builder_.Merge(doubleFalseState.data(), doubleFalseState.size());

    GateRef mergeTrueState = builder_.Merge(trueState.data(), trueState.size());
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetOpCode(*it) == OpCode::IF_TRUE) {
            acc_.SetOpCode(*it, OpCode::ORDINARY_BLOCK);
            it = acc_.ReplaceIn(it, mergeTrueState);
        } else if (acc_.GetOpCode(*it) == OpCode::IF_FALSE) {
            acc_.SetOpCode(*it, OpCode::ORDINARY_BLOCK);
            it = acc_.ReplaceIn(it, mergeFalseState);
        } else if (((acc_.GetOpCode(*it) == OpCode::DEPEND_SELECTOR) ||
                    (acc_.GetOpCode(*it) == OpCode::DEPEND_RELAY)) &&
                    (acc_.GetOpCode(acc_.GetIn(acc_.GetIn(*it, 0), it.GetIndex() - 1)) != OpCode::IF_EXCEPTION)) {
            it = acc_.ReplaceIn(it, acc_.GetDep(gate));
        } else {
            UNREACHABLE();
        }
    }
    // delete old gate
    acc_.DeleteGate(gate);
}

void SlowPathLowering::LowerGetNextPropName(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(GetNextPropName);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef iter = acc_.GetValueIn(gate, 0);
    GateRef newGate = LowerCallRuntime(glue, id, { iter });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCopyDataProperties(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(CopyDataProperties);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef dst = acc_.GetValueIn(gate, 0);
    GateRef src = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(glue, id, { dst, src });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateObjectWithExcludedKeys(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(CreateObjectWithExcludedKeys);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef numKeys = acc_.GetValueIn(gate, 0);
    GateRef obj = acc_.GetValueIn(gate, 1);
    GateRef firstArgRegIdx = acc_.GetValueIn(gate, 2);
    auto args = { builder_.ToTaggedInt(numKeys), obj, builder_.ToTaggedInt(firstArgRegIdx) };
    GateRef newGate = LowerCallRuntime(glue, id, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateRegExpWithLiteral(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    const int id = RTSTUB_ID(CreateRegExpWithLiteral);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef pattern = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef flags = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(glue, id, { pattern, builder_.ToTaggedInt(flags) });
    builder_.Branch(builder_.IsSpecial(newGate, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, newGate, successControl, failControl);
}

void SlowPathLowering::LowerStOwnByValue(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    // we do not need to merge outValueGate, so using GateRef directly instead of using Variable
    GateRef result;
    Label isHeapObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    builder_.Bind(&isHeapObject);
    Label notClassConstructor(&builder_);
    builder_.Branch(builder_.IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    builder_.Bind(&notClassConstructor);
    Label notClassPrototype(&builder_);
    builder_.Branch(builder_.IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    builder_.Bind(&notClassPrototype);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByValueWithOwn,
            { glue, receiver, propKey, accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = LowerCallRuntime(glue, RTSTUB_ID(StOwnByValue), { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // stOwnByValue will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByIndex(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    // we do not need to merge outValueGate, so using GateRef directly instead of using Variable
    GateRef result;
    Label isHeapObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    builder_.Bind(&isHeapObject);
    Label notClassConstructor(&builder_);
    builder_.Branch(builder_.IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    builder_.Bind(&notClassConstructor);
    Label notClassPrototype(&builder_);
    builder_.Branch(builder_.IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    builder_.Bind(&notClassPrototype);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByIndexWithOwn,
            { glue, receiver, builder_.TruncInt64ToInt32(index), accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        auto args = {receiver, builder_.ToTaggedInt(index), accValue };
        result = LowerCallRuntime(glue, RTSTUB_ID(StOwnByIndex), args, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // StOwnByIndex will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef propKey = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    // we do not need to merge outValueGate, so using GateRef directly instead of using Variable
    GateRef result;
    Label isJSObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.IsJSObject(receiver), &isJSObject, &slowPath);
    builder_.Bind(&isJSObject);
    Label notClassConstructor(&builder_);
    builder_.Branch(builder_.IsClassConstructor(receiver), &slowPath, &notClassConstructor);
    builder_.Bind(&notClassConstructor);
    Label notClassPrototype(&builder_);
    builder_.Branch(builder_.IsClassPrototype(receiver), &slowPath, &notClassPrototype);
    builder_.Bind(&notClassPrototype);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByNameWithOwn,
            { glue, receiver, propKey, accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = LowerCallRuntime(glue, RTSTUB_ID(StOwnByName), {receiver, propKey, accValue }, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // StOwnByName will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerNewLexicalEnv(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef lexEnv = LowerCallRuntime(glue, RTSTUB_ID(OptGetLexicalEnv), {}, true);
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(OptNewLexicalEnv),
        {builder_.ToTaggedInt(acc_.GetValueIn(gate, 0)), lexEnv}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerNewLexicalEnvWithName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef lexEnv = LowerCallRuntime(glue, RTSTUB_ID(OptGetLexicalEnv), {}, true);
    auto args = { builder_.ToTaggedInt(acc_.GetValueIn(gate, 0)),
                  builder_.ToTaggedInt(acc_.GetValueIn(gate, 1)),
                  lexEnv, jsFunc};
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(OptNewLexicalEnvWithName), args, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerPopLexicalEnv(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    LowerCallRuntime(glue, RTSTUB_ID(OptPopLexicalEnv), {}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl, true);
}

void SlowPathLowering::LowerLdSuperByValue(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(OptLdSuperByValue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(glue, id, { receiver, propKey, jsFunc });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStSuperByValue(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(OptStSuperByValue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateRef newGate = LowerCallRuntime(glue, id, { receiver, propKey, value, jsFunc});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerTryStGlobalByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    // order: 1. global record 2. global object
    DEFVAlUE(res, (&builder_), VariableType::JS_ANY(), builder_.Int64(JSTaggedValue::VALUE_HOLE));
    // 2 : number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef propKey = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    Label isUndefined(&builder_);
    Label notUndefined(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);

    // order: 1. global record 2. global object
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef recordInfo = LowerCallRuntime(glue, RTSTUB_ID(LdGlobalRecord), { propKey }, true);
    builder_.Branch(builder_.IsSpecial(recordInfo, JSTaggedValue::VALUE_UNDEFINED),
                    &isUndefined, &notUndefined);
    builder_.Bind(&notUndefined);
    {
        res = LowerCallRuntime(glue, RTSTUB_ID(TryUpdateGlobalRecord), { propKey, value }, true);
        builder_.Branch(builder_.IsSpecial(*res, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &successExit);
    }
    builder_.Bind(&isUndefined);
    {
        Label isHole(&builder_);
        Label notHole(&builder_);
        GateRef globalResult = LowerCallRuntime(glue, RTSTUB_ID(GetGlobalOwnProperty), { propKey }, true);
        builder_.Branch(builder_.IsSpecial(globalResult, JSTaggedValue::VALUE_HOLE), &isHole, &notHole);
        builder_.Bind(&isHole);
        {
            res = LowerCallRuntime(glue, RTSTUB_ID(ThrowReferenceError), { propKey }, true);
            builder_.Branch(builder_.IsSpecial(*res, JSTaggedValue::VALUE_EXCEPTION),
                            &exceptionExit, &successExit);
        }
        builder_.Bind(&notHole);
        {
            res = LowerCallRuntime(glue, RTSTUB_ID(StGlobalVar), { propKey, value }, true);
            builder_.Branch(builder_.IsSpecial(*res, JSTaggedValue::VALUE_EXCEPTION),
                            &exceptionExit, &successExit);
        }
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStConstToGlobalRecord(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef propKey = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    acc_.SetDep(gate, propKey);
    // 2 : number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    const int id = RTSTUB_ID(StGlobalRecord);
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef isConst = builder_.TaggedTrue();
    GateRef newGate = LowerCallRuntime(glue, id, { propKey, value, isConst });
    builder_.Branch(builder_.IsSpecial(newGate, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, newGate, successControl, failControl);
}

void SlowPathLowering::LowerStLetToGlobalRecord(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef taggedFalse = builder_.TaggedFalse();
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(StGlobalRecord),
        {prop,  acc_.GetValueIn(gate, 1), taggedFalse}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStClassToGlobalRecord(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef taggedFalse = builder_.TaggedFalse();
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(StGlobalRecord),
        {prop,  acc_.GetValueIn(gate, 1), taggedFalse}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByValueWithNameSet(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    Label isHeapObject(&builder_);
    Label slowPath(&builder_);
    Label notClassConstructor(&builder_);
    Label notClassPrototype(&builder_);
    Label notHole(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef res;
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    builder_.Bind(&isHeapObject);
    {
        builder_.Branch(builder_.IsClassConstructor(receiver), &slowPath, &notClassConstructor);
        builder_.Bind(&notClassConstructor);
        {
            builder_.Branch(builder_.IsClassPrototype(receiver), &slowPath, &notClassPrototype);
            builder_.Bind(&notClassPrototype);
            {
                res = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByValue,
                    { glue, receiver, propKey, accValue });
                builder_.Branch(builder_.IsSpecial(res, JSTaggedValue::VALUE_HOLE),
                    &slowPath, &notHole);
                builder_.Bind(&notHole);
                {
                    Label notexception(&builder_);
                    builder_.Branch(builder_.IsSpecial(res, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &notexception);
                    builder_.Bind(&notexception);
                    LowerCallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), { accValue, propKey }, true);
                    builder_.Jump(&successExit);
                }
            }
        }
    }
    builder_.Bind(&slowPath);
    {
        res = LowerCallRuntime(glue, RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.IsSpecial(res, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByNameWithNameSet(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef propKey = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    GateRef result;
    Label isJSObject(&builder_);
    Label notJSObject(&builder_);
    Label notClassConstructor(&builder_);
    Label notClassPrototype(&builder_);
    Label notHole(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.IsJSObject(receiver), &isJSObject, &notJSObject);
    builder_.Bind(&isJSObject);
    {
        builder_.Branch(builder_.IsClassConstructor(receiver), &notJSObject, &notClassConstructor);
        builder_.Bind(&notClassConstructor);
        {
            builder_.Branch(builder_.IsClassPrototype(receiver), &notJSObject, &notClassPrototype);
            builder_.Bind(&notClassPrototype);
            {
                result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByNameWithOwn,
                    { glue, receiver, propKey, accValue });
                builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE),
                    &notJSObject, &notHole);
                builder_.Bind(&notHole);
                {
                    Label notException(&builder_);
                    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &notException);
                    builder_.Bind(&notException);
                    LowerCallRuntime(glue, RTSTUB_ID(SetFunctionNameNoPrefix), {accValue, propKey}, true);
                    builder_.Jump(&successExit);
                }
            }
        }
    }
    builder_.Bind(&notJSObject);
    {
        result = LowerCallRuntime(glue, RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerLdGlobalVar(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label slowPath(&builder_);
    GateRef ret;
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef propKey = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef globalObject = builder_.GetGlobalObject(glue);
    result = LowerCallRuntime(glue, RTSTUB_ID(GetGlobalOwnProperty), { propKey }, true);
    builder_.Branch(builder_.IsSpecial(*result, JSTaggedValue::VALUE_HOLE),
        &slowPath, &successExit);
    builder_.Bind(&slowPath);
    {
        result = LowerCallRuntime(glue, RTSTUB_ID(LdGlobalVar), { globalObject, propKey }, true);
        builder_.Branch(builder_.IsSpecial(*result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&successExit);
    {
        ret = *result;
        successControl.emplace_back(builder_.GetState());
        successControl.emplace_back(builder_.GetDepend());
    }
    builder_.Bind(&exceptionExit);
    {
        failControl.emplace_back(builder_.GetState());
        failControl.emplace_back(builder_.GetDepend());
    }
    ReplaceHirToSubCfg(gate, ret, successControl, failControl);
}

void SlowPathLowering::LowerLdObjByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef holeConst = builder_.HoleConstant();
    DEFVAlUE(varAcc, (&builder_), VariableType::JS_ANY(), holeConst);
    GateRef result;
    Label receiverIsHeapObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    builder_.Bind(&receiverIsHeapObject);
    {
        varAcc = builder_.CallStub(glue, CommonStubCSigns::GetPropertyByName,
            {glue, receiver, prop});
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        GateRef undefined = builder_.UndefineConstant();
        varAcc = LowerCallRuntime(glue, RTSTUB_ID(LoadICByName), {undefined,
            receiver, prop, undefined}, true);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&successExit);
    {
        result = *varAcc;
        successControl.emplace_back(builder_.GetState());
        successControl.emplace_back(builder_.GetDepend());
    }
    builder_.Bind(&exceptionExit);
    {
        failControl.emplace_back(builder_.GetState());
        failControl.emplace_back(builder_.GetDepend());
    }
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStObjByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label receiverIsHeapObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef receiver = acc_.GetValueIn(gate, 1);
    GateRef result;
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    builder_.Bind(&receiverIsHeapObject);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByName,
            {glue, receiver, prop, acc_.GetValueIn(gate, 2)});
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        GateRef undefined = builder_.UndefineConstant();
        auto args = { undefined, receiver, prop, acc_.GetValueIn(gate, 2), undefined };
        result = LowerCallRuntime(glue, RTSTUB_ID(StoreICByName), args, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerDefineGetterSetterByValue(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(DefineGetterSetterByValue);
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef prop = acc_.GetValueIn(gate, 1);
    GateRef getter = acc_.GetValueIn(gate, 2);
    GateRef setter = acc_.GetValueIn(gate, 3);
    GateRef acc = acc_.GetValueIn(gate, 4);
    auto args = { obj, prop, getter, setter, acc };
    GateRef newGate = LowerCallRuntime(glue, id, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdObjByIndex(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef holeConst = builder_.HoleConstant();
    DEFVAlUE(varAcc, (&builder_), VariableType::JS_ANY(), holeConst);
    GateRef result;
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    Label fastPath(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    builder_.Bind(&fastPath);
    {
        varAcc = builder_.CallStub(glue, CommonStubCSigns::GetPropertyByIndex,
            {glue, receiver, builder_.TruncInt64ToInt32(index)});
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        GateRef undefined = builder_.UndefineConstant();
        auto args = { receiver, builder_.ToTaggedInt(index),
                      builder_.TaggedFalse(), builder_.ToTaggedInt(undefined) };
        varAcc = LowerCallRuntime(glue, RTSTUB_ID(LdObjByIndex), args);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&successExit);
    {
        result = *varAcc;
        successControl.emplace_back(builder_.GetState());
        successControl.emplace_back(builder_.GetDepend());
    }
    builder_.Bind(&exceptionExit);
    {
        failControl.emplace_back(builder_.GetState());
        failControl.emplace_back(builder_.GetDepend());
    }
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStObjByIndex(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label fastPath(&builder_);
    Label slowPath(&builder_);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef index = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    GateRef result;
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    builder_.Bind(&fastPath);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByIndex,
            {glue, receiver, builder_.TruncInt64ToInt32(index), accValue});
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = LowerCallRuntime(glue, RTSTUB_ID(StObjByIndex),
            {receiver, builder_.ToTaggedInt(index), accValue}, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerLdObjByValue(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef holeConst = builder_.HoleConstant();
    DEFVAlUE(varAcc, (&builder_), VariableType::JS_ANY(), holeConst);
    GateRef result;
    Label isHeapObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    builder_.Bind(&isHeapObject);
    {
        varAcc = builder_.CallStub(glue, CommonStubCSigns::GetPropertyByValue,
            {glue, receiver, propKey});
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        GateRef undefined = builder_.UndefineConstant();
        varAcc = LowerCallRuntime(glue, RTSTUB_ID(LoadICByValue), {undefined, receiver, propKey,
            builder_.ToTaggedInt(undefined)});
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&successExit);
    {
        result = *varAcc;
        successControl.emplace_back(builder_.GetState());
        successControl.emplace_back(builder_.GetDepend());
    }
    builder_.Bind(&exceptionExit);
    {
        failControl.emplace_back(builder_.GetState());
        failControl.emplace_back(builder_.GetDepend());
    }
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStObjByValue(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef accValue = acc_.GetValueIn(gate, 2);
    // we do not need to merge outValueGate, so using GateRef directly instead of using Variable
    GateRef result;
    Label isHeapObject(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &isHeapObject, &slowPath);
    builder_.Bind(&isHeapObject);
    {
        result = builder_.CallStub(glue, CommonStubCSigns::SetPropertyByValue, {glue, receiver, propKey, accValue});
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        GateRef undefined = builder_.UndefineConstant();
        result = LowerCallRuntime(glue, RTSTUB_ID(StoreICByValue), {undefined, receiver, propKey, accValue,
            builder_.ToTaggedInt(undefined)});
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // stObjByValue will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerLdSuperByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    GateRef result =
        LowerCallRuntime(glue, RTSTUB_ID(OptLdSuperByValue), {acc_.GetValueIn(gate, 1), prop, jsFunc}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStSuperByName(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef prop = GetObjectFromConstPool(jsFunc, acc_.GetValueIn(gate, 0));
    auto args2 = { acc_.GetValueIn(gate, 1), prop, acc_.GetValueIn(gate, 2), jsFunc };
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(OptStSuperByValue), args2, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerCreateGeneratorObj(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(CreateGeneratorObj);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateAsyncGeneratorObj(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    int id = RTSTUB_ID(CreateAsyncGeneratorObj);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncGeneratorResolve(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    int id = RTSTUB_ID(AsyncGeneratorResolve);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef asyncGen = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef flag = acc_.GetValueIn(gate, 2);
    GateRef newGate = LowerCallRuntime(glue, id, {asyncGen, value, flag});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncGeneratorReject(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    int id = RTSTUB_ID(AsyncGeneratorReject);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(glue, id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStArraySpread(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(StArraySpread);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    auto args = { acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2) };
    GateRef newGate = LowerCallRuntime(glue, id, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdLexVar(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef level = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef slot = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 1));
    DEFVAlUE(currentEnv, (&builder_), VariableType::JS_ANY(),
        LowerCallRuntime(glue, RTSTUB_ID(OptGetLexicalEnv), {}, true));
    DEFVAlUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;

    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label afterLoop(&builder_);
    builder_.Branch(builder_.Int32LessThan(*i, level), &loopHead, &afterLoop);
    builder_.LoopBegin(&loopHead);
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    currentEnv = builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), *currentEnv, index);
    i = builder_.Int32Add(*i, builder_.Int32(1));
    builder_.Branch(builder_.Int32LessThan(*i, level), &loopEnd, &afterLoop);
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);
    builder_.Bind(&afterLoop);
    GateRef valueIndex = builder_.Int32Add(slot, builder_.Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    GateRef result = builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), *currentEnv, valueIndex);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerStLexVar(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef level = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef slot = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 1));
    GateRef value = acc_.GetValueIn(gate, 2);
    DEFVAlUE(currentEnv, (&builder_), VariableType::JS_ANY(),
             LowerCallRuntime(glue, RTSTUB_ID(OptGetLexicalEnv), {}, true));
    DEFVAlUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    Label loopHead(&builder_);
    Label loopEnd(&builder_);
    Label afterLoop(&builder_);
    builder_.Branch(builder_.Int32LessThan(*i, level), &loopHead, &afterLoop);
    builder_.LoopBegin(&loopHead);
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    currentEnv = builder_.GetValueFromTaggedArray(VariableType::JS_ANY(), *currentEnv, index);
    i = builder_.Int32Add(*i, builder_.Int32(1));
    builder_.Branch(builder_.Int32LessThan(*i, level), &loopEnd, &afterLoop);
    builder_.Bind(&loopEnd);
    builder_.LoopEnd(&loopHead);
    builder_.Bind(&afterLoop);
    GateRef valueIndex = builder_.Int32Add(slot, builder_.Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue, *currentEnv, valueIndex, value);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    // StLexVar will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, exceptionControl, true);
}

void SlowPathLowering::LowerCreateObjectHavingMethod(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(CreateObjectHavingMethod);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef imm = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef literal = GetObjectFromConstPool(jsFunc, imm);
    GateRef env = acc_.GetValueIn(gate, 1);
    GateRef constpool = GetConstPool(jsFunc);
    GateRef result = LowerCallRuntime(glue, id, { literal, env, constpool });
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate,  result, successControl, failControl);
}

void SlowPathLowering::LowerLdHomeObject(GateRef gate, GateRef jsFunc)
{
    GateRef homeObj = GetHomeObjectFromJSFunction(jsFunc);
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, homeObj, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerDefineClassWithBuffer(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef methodId = builder_.SExtInt16ToInt64(acc_.GetValueIn(gate, 0));
    GateRef literalId = acc_.GetValueIn(gate, 1);
    GateRef length = acc_.GetValueIn(gate, 2);

    GateRef lexicalEnv = acc_.GetValueIn(gate, 3);
    GateRef proto = acc_.GetValueIn(gate, 4);
    GateRef constpool = GetConstPool(jsFunc);

    Label isException(&builder_);
    Label isNotException(&builder_);
    auto args = { proto, lexicalEnv, constpool,
        builder_.ToTaggedInt(methodId), builder_.ToTaggedInt(literalId) };
    GateRef result = LowerCallRuntime(glue, RTSTUB_ID(CreateClassWithBuffer), args, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &isException, &isNotException);
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    builder_.Bind(&isNotException);
    {
        GateRef newLexicalEnv = LowerCallRuntime(glue, RTSTUB_ID(OptGetLexicalEnv), {}, true);
        builder_.SetLexicalEnvToFunction(glue, result, newLexicalEnv);
        builder_.SetModuleToFunction(glue, result, builder_.GetModuleFromFunction(jsFunc));
        LowerCallRuntime(glue, RTSTUB_ID(SetClassConstructorLength),
            { result, builder_.ToTaggedInt(length) }, true);
        successControl.emplace_back(builder_.GetState());
        successControl.emplace_back(builder_.GetDepend());
    }
    builder_.Bind(&isException);
    {
        exceptionControl.emplace_back(builder_.GetState());
        exceptionControl.emplace_back(builder_.GetDepend());
    }
    ReplaceHirToSubCfg(gate, result, successControl, exceptionControl);
}

void SlowPathLowering::LowerDefineFunc(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    GateRef methodId = acc_.GetValueIn(gate, 0);
    GateRef length = acc_.GetValueIn(gate, 1);
    GateRef v0 = acc_.GetValueIn(gate, 2);
    DEFVAlUE(result, (&builder_),
        VariableType::JS_POINTER(), GetObjectFromConstPool(jsFunc, builder_.ZExtInt16ToInt32(methodId)));
    Label defaultLabel(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef ret;
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    {
        result = LowerCallRuntime(glue, RTSTUB_ID(DefineFunc), { *result });
        Label isException(&builder_);
        Label notException(&builder_);
        builder_.Branch(builder_.TaggedIsException(*result), &isException, &notException);
        builder_.Bind(&isException);
        {
            builder_.Jump(&exceptionExit);
        }
        builder_.Bind(&notException);
        {
            builder_.Jump(&defaultLabel);
        }
    }
    builder_.Bind(&defaultLabel);
    {
        GateRef hClass = builder_.LoadHClass(*result);
        builder_.SetPropertyInlinedProps(glue, *result, hClass, builder_.ToTaggedInt(length),
            builder_.Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        builder_.SetLexicalEnvToFunction(glue, *result, v0);
        builder_.SetModuleToFunction(glue, *result, builder_.GetModuleFromFunction(jsFunc));
        builder_.Jump(&successExit);
    }
    builder_.Bind(&successExit);
    {
        ret = *result;
        successControl.emplace_back(builder_.GetState());
        successControl.emplace_back(builder_.GetDepend());
    }
    builder_.Bind(&exceptionExit);
    {
        failControl.emplace_back(builder_.GetState());
        failControl.emplace_back(builder_.GetDepend());
    }
    ReplaceHirToSubCfg(gate, ret, successControl, failControl);
}

void SlowPathLowering::LowerAsyncFunctionEnter(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    const int id = RTSTUB_ID(AsyncFunctionEnter);
    // 0: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 0);
    GateRef newGate = LowerCallRuntime(glue, id, {});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerTypeof(GateRef gate, GateRef glue)
{
    DebugPrintBC(gate, glue);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef obj = acc_.GetValueIn(gate, 0);
    Label entry(&builder_);
    Label exit(&builder_);

    GateRef gConstAddr = builder_.PtrAdd(glue,
        builder_.IntPtr(JSThread::GlueData::GetGlobalConstOffset(builder_.GetCompilationConfig()->Is32Bit())));
    GateRef undefinedIndex = builder_.GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX);
    GateRef gConstUndefinedStr = builder_.Load(VariableType::JS_POINTER(), gConstAddr, undefinedIndex);
    DEFVAlUE(result, (&builder_), VariableType::JS_POINTER(), gConstUndefinedStr);
    Label objIsTrue(&builder_);
    Label objNotTrue(&builder_);
    Label defaultLabel(&builder_);
    GateRef gConstBooleanStr = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
        builder_.GetGlobalConstantString(ConstantIndex::BOOLEAN_STRING_INDEX));
    builder_.Branch(builder_.TaggedIsTrue(obj), &objIsTrue, &objNotTrue);
    builder_.Bind(&objIsTrue);
    {
        result = gConstBooleanStr;
        builder_.Jump(&exit);
    }
    builder_.Bind(&objNotTrue);
    {
        Label objIsFalse(&builder_);
        Label objNotFalse(&builder_);
        builder_.Branch(builder_.TaggedIsFalse(obj), &objIsFalse, &objNotFalse);
        builder_.Bind(&objIsFalse);
        {
            result = gConstBooleanStr;
            builder_.Jump(&exit);
        }
        builder_.Bind(&objNotFalse);
        {
            Label objIsNull(&builder_);
            Label objNotNull(&builder_);
            builder_.Branch(builder_.TaggedIsNull(obj), &objIsNull, &objNotNull);
            builder_.Bind(&objIsNull);
            {
                result = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
                    builder_.GetGlobalConstantString(ConstantIndex::OBJECT_STRING_INDEX));
                builder_.Jump(&exit);
            }
            builder_.Bind(&objNotNull);
            {
                Label objIsUndefined(&builder_);
                Label objNotUndefined(&builder_);
                builder_.Branch(builder_.TaggedIsUndefined(obj), &objIsUndefined, &objNotUndefined);
                builder_.Bind(&objIsUndefined);
                {
                    result = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
                        builder_.GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX));
                    builder_.Jump(&exit);
                }
                builder_.Bind(&objNotUndefined);
                builder_.Jump(&defaultLabel);
            }
        }
    }
    builder_.Bind(&defaultLabel);
    {
        Label objIsHeapObject(&builder_);
        Label objNotHeapObject(&builder_);
        builder_.Branch(builder_.TaggedIsHeapObject(obj), &objIsHeapObject, &objNotHeapObject);
        builder_.Bind(&objIsHeapObject);
        {
            Label objIsString(&builder_);
            Label objNotString(&builder_);
            builder_.Branch(builder_.IsJsType(obj, JSType::STRING), &objIsString, &objNotString);
            builder_.Bind(&objIsString);
            {
                result = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
                    builder_.GetGlobalConstantString(ConstantIndex::STRING_STRING_INDEX));
                builder_.Jump(&exit);
            }
            builder_.Bind(&objNotString);
            {
                Label objIsSymbol(&builder_);
                Label objNotSymbol(&builder_);
                builder_.Branch(builder_.IsJsType(obj, JSType::SYMBOL), &objIsSymbol, &objNotSymbol);
                builder_.Bind(&objIsSymbol);
                {
                    result = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
                        builder_.GetGlobalConstantString(ConstantIndex::SYMBOL_STRING_INDEX));
                    builder_.Jump(&exit);
                }
                builder_.Bind(&objNotSymbol);
                {
                    Label objIsCallable(&builder_);
                    Label objNotCallable(&builder_);
                    builder_.Branch(builder_.IsCallable(obj), &objIsCallable, &objNotCallable);
                    builder_.Bind(&objIsCallable);
                    {
                        result = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
                            builder_.GetGlobalConstantString(ConstantIndex::FUNCTION_STRING_INDEX));
                        builder_.Jump(&exit);
                    }
                    builder_.Bind(&objNotCallable);
                    {
                        Label objIsBigInt(&builder_);
                        Label objNotBigInt(&builder_);
                        builder_.Branch(builder_.IsJsType(obj, JSType::BIGINT), &objIsBigInt, &objNotBigInt);
                        builder_.Bind(&objIsBigInt);
                        {
                            result = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
                                builder_.GetGlobalConstantString(ConstantIndex::BIGINT_STRING_INDEX));
                            builder_.Jump(&exit);
                        }
                        builder_.Bind(&objNotBigInt);
                        {
                            result = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
                                builder_.GetGlobalConstantString(ConstantIndex::OBJECT_STRING_INDEX));
                            builder_.Jump(&exit);
                        }
                    }
                }
            }
        }
        builder_.Bind(&objNotHeapObject);
        {
            Label objIsNum(&builder_);
            Label objNotNum(&builder_);
            builder_.Branch(builder_.TaggedIsNumber(obj), &objIsNum, &objNotNum);
            builder_.Bind(&objIsNum);
            {
                result = builder_.Load(VariableType::JS_POINTER(), gConstAddr,
                    builder_.GetGlobalConstantString(ConstantIndex::NUMBER_STRING_INDEX));
                builder_.Jump(&exit);
            }
            builder_.Bind(&objNotNum);
            builder_.Jump(&exit);
        }
    }
    builder_.Bind(&exit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

GateRef SlowPathLowering::GetValueFromTaggedArray(GateRef arrayGate, GateRef indexOffset)
{
    GateRef offset = builder_.PtrMul(builder_.ChangeInt32ToIntPtr(indexOffset),
                                     builder_.IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = builder_.PtrAdd(offset, builder_.IntPtr(TaggedArray::DATA_OFFSET));
    GateRef value = builder_.Load(VariableType::JS_ANY(), arrayGate, dataOffset);
    return value;
}

void SlowPathLowering::LowerResumeGenerator(GateRef gate)
{
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef restoreGate = acc_.GetDep(gate);
    std::vector<GateRef> registerGates {};

    while (acc_.GetOpCode(restoreGate) == OpCode::RESTORE_REGISTER) {
        registerGates.emplace_back(restoreGate);
        restoreGate = acc_.GetDep(restoreGate);
    }

    acc_.SetDep(gate, restoreGate);
    builder_.SetDepend(restoreGate);
    GateRef contextOffset = builder_.IntPtr(JSGeneratorObject::GENERATOR_CONTEXT_OFFSET);
    GateRef contextGate = builder_.Load(VariableType::JS_POINTER(), obj, contextOffset);
    GateRef arrayOffset = builder_.IntPtr(GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET);
    GateRef arrayGate = builder_.Load(VariableType::JS_POINTER(), contextGate, arrayOffset);

    for (auto item : registerGates) {
        auto index = acc_.GetBitField(item);
        auto indexOffset = builder_.Int32(index);
        GateRef value = GetValueFromTaggedArray(arrayGate, indexOffset);
        auto uses = acc_.Uses(item);
        for (auto use = uses.begin(); use != uses.end();) {
            size_t valueStartIndex = acc_.GetStateCount(*use) + acc_.GetDependCount(*use);
            size_t valueEndIndex = valueStartIndex + acc_.GetInValueCount(*use);
            if (use.GetIndex() >= valueStartIndex && use.GetIndex() < valueEndIndex) {
                use = acc_.ReplaceIn(use, value);
            } else {
                use++;
            }
        }
        acc_.DeleteGate(item);
    }

    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef resumeResultOffset = builder_.IntPtr(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
    GateRef result = builder_.Load(VariableType::JS_ANY(), obj, resumeResultOffset);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerGetResumeMode(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef bitFieldOffset = builder_.IntPtr(JSGeneratorObject::BIT_FIELD_OFFSET);
    GateRef bitField = builder_.Load(VariableType::INT32(), obj, bitFieldOffset);
    auto bitfieldlsr = builder_.Int32LSR(bitField, builder_.Int32(JSGeneratorObject::ResumeModeBits::START_BIT));
    GateRef resumeModeBits = builder_.Int32And(bitfieldlsr,
                                               builder_.Int32((1LU << JSGeneratorObject::ResumeModeBits::SIZE) - 1));
    auto resumeMode = builder_.SExtInt32ToInt64(resumeModeBits);
    GateRef result = builder_.ToTaggedIntPtr(resumeMode);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerDefineMethod(GateRef gate, GateRef glue, GateRef jsFunc)
{
    DebugPrintBC(gate, glue);
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef methodId = acc_.GetValueIn(gate, 0);
    GateRef length = acc_.GetValueIn(gate, 1);
    GateRef env = acc_.GetValueIn(gate, 2);
    GateRef homeObject = acc_.GetValueIn(gate, 3);
    GateRef result;
    DEFVAlUE(method, (&builder_), VariableType::JS_POINTER(),
             GetObjectFromConstPool(jsFunc, builder_.ZExtInt16ToInt32(methodId)));
    Label defaultLabel(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    {
        method = LowerCallRuntime(glue, RTSTUB_ID(DefineMethod), {*method, homeObject});
        Label notException(&builder_);
        builder_.Branch(builder_.IsSpecial(*method, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &notException);
        builder_.Bind(&notException);
        {
            builder_.Jump(&defaultLabel);
        }
    }
    builder_.Bind(&defaultLabel);
    {
        GateRef hclass = builder_.LoadHClass(*method);
        builder_.SetPropertyInlinedProps(glue, *method, hclass, builder_.ToTaggedInt(length),
            builder_.Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        builder_.SetLexicalEnvToFunction(glue, *method, env);
        builder_.SetModuleToFunction(glue, *method, builder_.GetModuleFromFunction(jsFunc));
        result = *method;
        builder_.Jump(&successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGetUnmappedArgs(GateRef gate, GateRef glue, GateRef actualArgc)
{
    DebugPrintBC(gate, glue);
    GateRef taggedArgc = builder_.ToTaggedInt(builder_.ZExtInt32ToInt64(actualArgc));
    const int id = RTSTUB_ID(OptGetUnmapedArgs);
    GateRef newGate = LowerCallRuntime(glue, id, {taggedArgc});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCopyRestArgs(GateRef gate, GateRef glue, GateRef actualArgc)
{
    DebugPrintBC(gate, glue);
    GateRef taggedArgc = builder_.ToTaggedInt(builder_.ZExtInt32ToInt64(actualArgc));
    GateRef restIdx = acc_.GetValueIn(gate, 0);
    GateRef taggedRestIdx = builder_.ToTaggedInt(restIdx);

    const int id = RTSTUB_ID(OptCopyRestArgs);
    GateRef newGate = LowerCallRuntime(glue, id, {taggedArgc, taggedRestIdx});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::DebugPrintBC(GateRef gate, GateRef glue)
{
    if (enableBcTrace_) {
        auto pc = bcBuilder_->GetJSBytecode(gate);
        auto index = builder_.Int32(*pc);
        GateRef constIndex = builder_.ToTaggedInt(builder_.ZExtInt32ToInt64(index));
        GateRef debugGate = builder_.CallRuntime(glue,  RTSTUB_ID(DebugAOTPrint), acc_.GetDep(gate), {constIndex});
        acc_.SetDep(gate, debugGate);
    }
}
}  // namespace panda::ecmascript
