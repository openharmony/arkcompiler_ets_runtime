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
#include "ecmascript/dfx/vmstat/opt_code_profiler.h"
#include "ecmascript/message_string.h"
#include "ecmascript/compiler/new_object_stub_builder.h"

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
        } else if (op == OpCode::CONST_DATA) {
            LowerConstPoolData(gate);
        } else if (op == OpCode::DEOPT_CHECK) {
            LowerDeoptCheck(gate);
        } else if (op == OpCode::CONSTRUCT) {
            LowerConstruct(gate);
        } else if (op == OpCode::UPDATE_HOTNESS) {
            LowerUpdateHotness(gate);
        }
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << " ";
        LOG_COMPILER(INFO) << "\033[34m" << "================="
                           << " After slowpath Lowering "
                           << "[" << GetMethodName() << "] "
                           << "=================" << "\033[0m";
        circuit_->PrintAllGatesWithBytecode();
        LOG_COMPILER(INFO) << "\033[34m" << "=========================== End ===========================" << "\033[0m";
    }
}

int32_t SlowPathLowering::ComputeCallArgc(GateRef gate, EcmaOpcode op)
{
    switch (op) {
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
        case EcmaOpcode::CALLTHIS0_IMM8_V8: {
            return acc_.GetNumValueIn(gate) + NUM_MANDATORY_JSFUNC_ARGS - 2; // 2: calltarget, this
        }
        default: {
            return acc_.GetNumValueIn(gate) + NUM_MANDATORY_JSFUNC_ARGS - 1; // 1: calltarget
        }
    }
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

void SlowPathLowering::ExceptionReturn(GateRef state, GateRef depend)
{
    auto constant = builder_.ExceptionConstant();
    builder_.Return(state, depend, constant);
}

void SlowPathLowering::ReplaceHirToSubCfg(GateRef hir, GateRef outir,
                                          const std::vector<GateRef> &successControl,
                                          const std::vector<GateRef> &exceptionControl,
                                          bool noThrow)
{
    if (outir != Circuit::NullGate()) {
        auto type = acc_.GetGateType(hir);
        if (!type.IsAnyType()) {
            acc_.SetGateType(outir, type);
        }
    }
    auto uses = acc_.Uses(hir);
    for (auto useIt = uses.begin(); useIt != uses.end();) {
        const OpCode op = acc_.GetOpCode(*useIt);
        if (acc_.IsStateIn(useIt)) {
            if (op == OpCode::IF_SUCCESS) {
                useIt = ReplaceHirControlGate(useIt, successControl[0]);
            } else if (op == OpCode::IF_EXCEPTION) {
                useIt = ReplaceHirControlGate(useIt, exceptionControl[0], noThrow);
            } else {
                if (!noThrow) {
                    ExceptionReturn(exceptionControl[0], exceptionControl[1]);
                }
                useIt = acc_.ReplaceIn(useIt, successControl[0]);
            }
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

void SlowPathLowering::ReplaceHirWithIfBranch(GateRef hirGate, GateRef callGate, GateRef ifBranch)
{
    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.IsStateIn(it)) {
            const OpCode op = acc_.GetOpCode(*it);
            if (op == OpCode::IF_SUCCESS) {
                acc_.SetMetaData(*it, circuit_->IfFalse());
                it = acc_.ReplaceIn(it, ifBranch);
            } else if (op == OpCode::IF_EXCEPTION) {
                acc_.SetMetaData(*it, circuit_->IfTrue());
                it = acc_.ReplaceIn(it, ifBranch);
            } else {
                GateRef ifTrue = builder_.IfTrue(ifBranch);
                GateRef ifFalse = builder_.IfFalse(ifBranch);
                ExceptionReturn(ifTrue, callGate);
                it = acc_.ReplaceIn(it, ifFalse);
            }
        } else {
            it = acc_.ReplaceIn(it, callGate);
        }
    }

    // delete old gate
    acc_.DeleteGate(hirGate);
}

void SlowPathLowering::ReplaceHirToJSCall(GateRef hirGate, GateRef callGate)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef exceptionOffset = builder_.IntPtr(JSThread::GlueData::GetExceptionOffset(false));
    GateRef exception = builder_.Load(VariableType::JS_ANY(), glue_, exceptionOffset);
    acc_.SetDep(exception, callGate);
    GateRef equal = builder_.NotEqual(exception, builder_.HoleConstant());
    GateRef ifBranch = builder_.Branch(stateInGate, equal);
    ReplaceHirWithIfBranch(hirGate, callGate, ifBranch);
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
        GateRef exceptionVal = builder_.ExceptionConstant();
        // compare with trampolines result
        GateRef equal = builder_.Equal(callGate, exceptionVal);
        ifBranch = builder_.Branch(stateInGate, equal);
    } else {
        ifBranch = builder_.Branch(stateInGate, builder_.Boolean(false));
    }
    ReplaceHirWithIfBranch(hirGate, callGate, ifBranch);
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
    ReplaceHirWithIfBranch(hirGate, callGate, ifBranch);
}

// labelmanager must be initialized
GateRef SlowPathLowering::LoadObjectFromConstPool(GateRef jsFunc, GateRef index)
{
    GateRef constPool = builder_.GetConstPool(jsFunc);
    return GetValueFromTaggedArray(constPool, index);
}

GateRef SlowPathLowering::GetProfileTypeInfo(GateRef jsFunc)
{
    GateRef method = builder_.GetMethodFromFunction(jsFunc);
    return builder_.Load(VariableType::JS_ANY(), method, builder_.IntPtr(Method::PROFILE_TYPE_INFO_OFFSET));
}

// labelmanager must be initialized
GateRef SlowPathLowering::GetHomeObjectFromJSFunction(GateRef jsFunc)
{
    GateRef offset = builder_.IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    return builder_.Load(VariableType::JS_ANY(), jsFunc, offset);
}

void SlowPathLowering::Lower(GateRef gate)
{
    GateRef newTarget = argAcc_.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    GateRef jsFunc = argAcc_.GetCommonArgGate(CommonArgIdx::FUNC);
    GateRef actualArgc = argAcc_.GetCommonArgGate(CommonArgIdx::ACTUAL_ARGC);

    EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    AddProfiling(gate);
    switch (ecmaOpcode) {
        case EcmaOpcode::CALLARG0_IMM8:
            LowerCallArg0(gate);
            break;
        case EcmaOpcode::CALLTHIS0_IMM8_V8:
            LowerCallthis0Imm8V8(gate);
            break;
        case EcmaOpcode::CALLARG1_IMM8_V8:
            LowerCallArg1Imm8V8(gate);
            break;
        case EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
            LowerWideCallrangePrefImm16V8(gate);
            break;
        case EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            LowerCallThisArg1(gate);
            break;
        case EcmaOpcode::CALLARGS2_IMM8_V8_V8:
            LowerCallargs2Imm8V8V8(gate);
            break;
        case EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            LowerCallthis2Imm8V8V8V8(gate);
            break;
        case EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
            LowerCallargs3Imm8V8V8(gate);
            break;
        case EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            LowerCallthis3Imm8V8V8V8V8(gate);
            break;
        case EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
            LowerCallthisrangeImm8Imm8V8(gate);
            break;
        case EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
            LowerWideCallthisrangePrefImm16V8(gate);
            break;
        case EcmaOpcode::APPLY_IMM8_V8_V8:
            LowerCallSpread(gate);
            break;
        case EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
            LowerCallrangeImm8Imm8V8(gate);
            break;
        case EcmaOpcode::GETUNMAPPEDARGS:
            LowerGetUnmappedArgs(gate, actualArgc);
            break;
        case EcmaOpcode::ASYNCFUNCTIONENTER:
            LowerAsyncFunctionEnter(gate);
            break;
        case EcmaOpcode::INC_IMM8:
            LowerInc(gate);
            break;
        case EcmaOpcode::DEC_IMM8:
            LowerDec(gate);
            break;
        case EcmaOpcode::GETPROPITERATOR:
            LowerGetPropIterator(gate);
            break;
        case EcmaOpcode::RESUMEGENERATOR:
            LowerResumeGenerator(gate);
            break;
        case EcmaOpcode::GETRESUMEMODE:
            LowerGetResumeMode(gate);
            break;
        case EcmaOpcode::CLOSEITERATOR_IMM8_V8:
        case EcmaOpcode::CLOSEITERATOR_IMM16_V8:
            LowerCloseIterator(gate);
            break;
        case EcmaOpcode::ADD2_IMM8_V8:
            LowerAdd2(gate);
            break;
        case EcmaOpcode::SUB2_IMM8_V8:
            LowerSub2(gate);
            break;
        case EcmaOpcode::MUL2_IMM8_V8:
            LowerMul2(gate);
            break;
        case EcmaOpcode::DIV2_IMM8_V8:
            LowerDiv2(gate);
            break;
        case EcmaOpcode::MOD2_IMM8_V8:
            LowerMod2(gate);
            break;
        case EcmaOpcode::EQ_IMM8_V8:
            LowerEq(gate);
            break;
        case EcmaOpcode::NOTEQ_IMM8_V8:
            LowerNotEq(gate);
            break;
        case EcmaOpcode::LESS_IMM8_V8:
            LowerLess(gate);
            break;
        case EcmaOpcode::LESSEQ_IMM8_V8:
            LowerLessEq(gate);
            break;
        case EcmaOpcode::GREATER_IMM8_V8:
            LowerGreater(gate);
            break;
        case EcmaOpcode::GREATEREQ_IMM8_V8:
            LowerGreaterEq(gate);
            break;
        case EcmaOpcode::CREATEITERRESULTOBJ_V8_V8:
            LowerCreateIterResultObj(gate);
            break;
        case EcmaOpcode::SUSPENDGENERATOR_V8:
            LowerSuspendGenerator(gate, jsFunc);
            break;
        case EcmaOpcode::ASYNCFUNCTIONAWAITUNCAUGHT_V8:
            LowerAsyncFunctionAwaitUncaught(gate);
            break;
        case EcmaOpcode::ASYNCFUNCTIONRESOLVE_V8:
            LowerAsyncFunctionResolve(gate);
            break;
        case EcmaOpcode::ASYNCFUNCTIONREJECT_V8:
            LowerAsyncFunctionReject(gate);
            break;
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        case EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16:
            LowerTryLdGlobalByName(gate, jsFunc);
            break;
        case EcmaOpcode::STGLOBALVAR_IMM16_ID16:
            LowerStGlobalVar(gate, jsFunc);
            break;
        case EcmaOpcode::GETITERATOR_IMM8:
        case EcmaOpcode::GETITERATOR_IMM16:
            LowerGetIterator(gate);
            break;
        case EcmaOpcode::GETASYNCITERATOR_IMM8:
            LowerGetAsyncIterator(gate);
            break;
        case EcmaOpcode::NEWOBJAPPLY_IMM8_V8:
        case EcmaOpcode::NEWOBJAPPLY_IMM16_V8:
            LowerNewObjApply(gate);
            break;
        case EcmaOpcode::THROW_PREF_NONE:
            LowerThrow(gate);
            break;
        case EcmaOpcode::TYPEOF_IMM8:
        case EcmaOpcode::TYPEOF_IMM16:
            LowerTypeof(gate);
            break;
        case EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8:
            LowerThrowConstAssignment(gate);
            break;
        case EcmaOpcode::THROW_NOTEXISTS_PREF_NONE:
            LowerThrowThrowNotExists(gate);
            break;
        case EcmaOpcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
            LowerThrowPatternNonCoercible(gate);
            break;
        case EcmaOpcode::THROW_IFNOTOBJECT_PREF_V8:
            LowerThrowIfNotObject(gate);
            break;
        case EcmaOpcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
            LowerThrowUndefinedIfHole(gate);
            break;
        case EcmaOpcode::THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16:
            LowerThrowUndefinedIfHoleWithName(gate, jsFunc);
            break;
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        case EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
            LowerThrowIfSuperNotCorrectCall(gate);
            break;
        case EcmaOpcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
            LowerThrowDeleteSuperProperty(gate);
            break;
        case EcmaOpcode::LDSYMBOL:
            LowerLdSymbol(gate);
            break;
        case EcmaOpcode::LDGLOBAL:
            LowerLdGlobal(gate);
            break;
        case EcmaOpcode::TONUMBER_IMM8:
            LowerToNumber(gate);
            break;
        case EcmaOpcode::NEG_IMM8:
            LowerNeg(gate);
            break;
        case EcmaOpcode::NOT_IMM8:
            LowerNot(gate);
            break;
        case EcmaOpcode::SHL2_IMM8_V8:
            LowerShl2(gate);
            break;
        case EcmaOpcode::SHR2_IMM8_V8:
            LowerShr2(gate);
            break;
        case EcmaOpcode::ASHR2_IMM8_V8:
            LowerAshr2(gate);
            break;
        case EcmaOpcode::AND2_IMM8_V8:
            LowerAnd2(gate);
            break;
        case EcmaOpcode::OR2_IMM8_V8:
            LowerOr2(gate);
            break;
        case EcmaOpcode::XOR2_IMM8_V8:
            LowerXor2(gate);
            break;
        case EcmaOpcode::DELOBJPROP_V8:
            LowerDelObjProp(gate);
            break;
        case EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
            LowerDefineMethod(gate, jsFunc);
            break;
        case EcmaOpcode::EXP_IMM8_V8:
            LowerExp(gate);
            break;
        case EcmaOpcode::ISIN_IMM8_V8:
            LowerIsIn(gate);
            break;
        case EcmaOpcode::INSTANCEOF_IMM8_V8:
            LowerInstanceof(gate);
            break;
        case EcmaOpcode::STRICTNOTEQ_IMM8_V8:
            LowerFastStrictNotEqual(gate);
            break;
        case EcmaOpcode::STRICTEQ_IMM8_V8:
            LowerFastStrictEqual(gate);
            break;
        case EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case EcmaOpcode::CREATEEMPTYARRAY_IMM16:
            LowerCreateEmptyArray(gate);
            break;
        case EcmaOpcode::CREATEEMPTYOBJECT:
            LowerCreateEmptyObject(gate);
            break;
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
            LowerCreateObjectWithBuffer(gate, jsFunc);
            break;
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            LowerCreateArrayWithBuffer(gate, jsFunc);
            break;
        case EcmaOpcode::STMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
            LowerStModuleVar(gate, jsFunc);
            break;
        case EcmaOpcode::SETGENERATORSTATE_IMM8:
            LowerSetGeneratorState(gate, jsFunc);
            break;
        case EcmaOpcode::GETTEMPLATEOBJECT_IMM8:
        case EcmaOpcode::GETTEMPLATEOBJECT_IMM16:
            LowerGetTemplateObject(gate);
            break;
        case EcmaOpcode::SETOBJECTWITHPROTO_IMM8_V8:
        case EcmaOpcode::SETOBJECTWITHPROTO_IMM16_V8:
            LowerSetObjectWithProto(gate);
            break;
        case EcmaOpcode::LDBIGINT_ID16:
            LowerLdBigInt(gate);
            break;
        case EcmaOpcode::TONUMERIC_IMM8:
            LowerToNumeric(gate);
            break;
        case EcmaOpcode::DYNAMICIMPORT:
            LowerDynamicImport(gate, jsFunc);
            break;
        case EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
            LowerExternalModule(gate, jsFunc);
            break;
        case EcmaOpcode::GETMODULENAMESPACE_IMM8:
        case EcmaOpcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
            LowerGetModuleNamespace(gate, jsFunc);
            break;
        case EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
        case EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
            LowerNewObjRange(gate);
            break;
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JEQZ_IMM32:
            LowerConditionJump(gate, true);
            break;
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
        case EcmaOpcode::JNEZ_IMM32:
            LowerConditionJump(gate, false);
            break;
        case EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            LowerSuperCall(gate, jsFunc, newTarget);
            break;
        case EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        case EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
            LowerSuperCallArrow(gate, newTarget);
            break;
        case EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
            LowerSuperCallSpread(gate, newTarget);
            break;
        case EcmaOpcode::ISTRUE:
            LowerIsTrueOrFalse(gate, true);
            break;
        case EcmaOpcode::ISFALSE:
            LowerIsTrueOrFalse(gate, false);
            break;
        case EcmaOpcode::GETNEXTPROPNAME_V8:
            LowerGetNextPropName(gate);
            break;
        case EcmaOpcode::COPYDATAPROPERTIES_V8:
            LowerCopyDataProperties(gate);
            break;
        case EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
        case EcmaOpcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
            LowerCreateObjectWithExcludedKeys(gate);
            break;
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
            LowerCreateRegExpWithLiteral(gate);
            break;
        case EcmaOpcode::STOWNBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOWNBYVALUE_IMM16_V8_V8:
            LowerStOwnByValue(gate);
            break;
        case EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
        case EcmaOpcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
            LowerStOwnByIndex(gate);
            break;
        case EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
            LowerStOwnByName(gate);
            break;
        case EcmaOpcode::NEWLEXENV_IMM8:
        case EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16:
            LowerNewLexicalEnv(gate);
            break;
        case EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
        case EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
            LowerNewLexicalEnvWithName(gate, jsFunc);
            break;
        case EcmaOpcode::POPLEXENV:
            LowerPopLexicalEnv(gate);
            break;
        case EcmaOpcode::LDSUPERBYVALUE_IMM8_V8:
        case EcmaOpcode::LDSUPERBYVALUE_IMM16_V8:
            LowerLdSuperByValue(gate, jsFunc);
            break;
        case EcmaOpcode::STSUPERBYVALUE_IMM16_V8_V8:
            LowerStSuperByValue(gate, jsFunc);
            break;
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        case EcmaOpcode::TRYSTGLOBALBYNAME_IMM16_ID16:
            LowerTryStGlobalByName(gate, jsFunc);
            break;
        case EcmaOpcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
            LowerStConstToGlobalRecord(gate, true);
            break;
        case EcmaOpcode::STTOGLOBALRECORD_IMM16_ID16:
            LowerStConstToGlobalRecord(gate, false);
            break;
        case EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
        case EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
            LowerStOwnByValueWithNameSet(gate);
            break;
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
            LowerStOwnByNameWithNameSet(gate);
            break;
        case EcmaOpcode::LDGLOBALVAR_IMM16_ID16:
            LowerLdGlobalVar(gate, jsFunc);
            break;
        case EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
            LowerLdObjByName(gate, jsFunc);
            break;
        case EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
            LowerStObjByName(gate, jsFunc, false);
            break;
        case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
            LowerDefineGetterSetterByValue(gate);
            break;
        case EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
        case EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
        case EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
            LowerLdObjByIndex(gate);
            break;
        case EcmaOpcode::STOBJBYINDEX_IMM8_V8_IMM16:
        case EcmaOpcode::STOBJBYINDEX_IMM16_V8_IMM16:
        case EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
            LowerStObjByIndex(gate);
            break;
        case EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
            LowerLdObjByValue(gate, jsFunc, false);
            break;
        case EcmaOpcode::LDTHISBYVALUE_IMM8:
        case EcmaOpcode::LDTHISBYVALUE_IMM16:
            LowerLdObjByValue(gate, jsFunc, true);
            break;
        case EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            LowerStObjByValue(gate, jsFunc, false);
            break;
        case EcmaOpcode::STTHISBYVALUE_IMM8_V8:
        case EcmaOpcode::STTHISBYVALUE_IMM16_V8:
            LowerStObjByValue(gate, jsFunc, true);
            break;
        case EcmaOpcode::LDSUPERBYNAME_IMM8_ID16:
        case EcmaOpcode::LDSUPERBYNAME_IMM16_ID16:
            LowerLdSuperByName(gate, jsFunc);
            break;
        case EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8:
        case EcmaOpcode::STSUPERBYNAME_IMM16_ID16_V8:
            LowerStSuperByName(gate, jsFunc);
            break;
        case EcmaOpcode::CREATEGENERATOROBJ_V8:
            LowerCreateGeneratorObj(gate);
            break;
        case EcmaOpcode::CREATEASYNCGENERATOROBJ_V8:
            LowerCreateAsyncGeneratorObj(gate);
            break;
        case EcmaOpcode::ASYNCGENERATORRESOLVE_V8_V8_V8:
            LowerAsyncGeneratorResolve(gate);
            break;
        case EcmaOpcode::ASYNCGENERATORREJECT_V8:
            LowerAsyncGeneratorReject(gate);
            break;
        case EcmaOpcode::STARRAYSPREAD_V8_V8:
            LowerStArraySpread(gate);
            break;
        case EcmaOpcode::LDLEXVAR_IMM4_IMM4:
        case EcmaOpcode::LDLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
            LowerLdLexVar(gate);
            break;
        case EcmaOpcode::STLEXVAR_IMM4_IMM4:
        case EcmaOpcode::STLEXVAR_IMM8_IMM8:
        case EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
            LowerStLexVar(gate);
            break;
        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        case EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
            LowerDefineClassWithBuffer(gate, jsFunc);
            break;
        case EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
            LowerDefineFunc(gate, jsFunc);
            break;
        case EcmaOpcode::COPYRESTARGS_IMM8:
        case EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16:
            LowerCopyRestArgs(gate, actualArgc);
            break;
        case EcmaOpcode::WIDE_LDPATCHVAR_PREF_IMM16:
            LowerWideLdPatchVar(gate);
            break;
        case EcmaOpcode::WIDE_STPATCHVAR_PREF_IMM16:
            LowerWideStPatchVar(gate);
            break;
        case EcmaOpcode::LDLOCALMODULEVAR_IMM8:
        case EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
            LowerLdLocalModuleVarByIndex(gate, jsFunc);
            break;
        case EcmaOpcode::DEBUGGER:
        case EcmaOpcode::JSTRICTEQZ_IMM8:
        case EcmaOpcode::JSTRICTEQZ_IMM16:
        case EcmaOpcode::JNSTRICTEQZ_IMM8:
        case EcmaOpcode::JNSTRICTEQZ_IMM16:
        case EcmaOpcode::JEQNULL_IMM8:
        case EcmaOpcode::JEQNULL_IMM16:
        case EcmaOpcode::JNENULL_IMM8:
        case EcmaOpcode::JNENULL_IMM16:
        case EcmaOpcode::JSTRICTEQNULL_IMM8:
        case EcmaOpcode::JSTRICTEQNULL_IMM16:
        case EcmaOpcode::JNSTRICTEQNULL_IMM8:
        case EcmaOpcode::JNSTRICTEQNULL_IMM16:
        case EcmaOpcode::JEQUNDEFINED_IMM8:
        case EcmaOpcode::JEQUNDEFINED_IMM16:
        case EcmaOpcode::JNEUNDEFINED_IMM8:
        case EcmaOpcode::JNEUNDEFINED_IMM16:
        case EcmaOpcode::JSTRICTEQUNDEFINED_IMM8:
        case EcmaOpcode::JSTRICTEQUNDEFINED_IMM16:
        case EcmaOpcode::JNSTRICTEQUNDEFINED_IMM8:
        case EcmaOpcode::JNSTRICTEQUNDEFINED_IMM16:
        case EcmaOpcode::JEQ_V8_IMM8:
        case EcmaOpcode::JEQ_V8_IMM16:
        case EcmaOpcode::JNE_V8_IMM8:
        case EcmaOpcode::JNE_V8_IMM16:
        case EcmaOpcode::JSTRICTEQ_V8_IMM8:
        case EcmaOpcode::JSTRICTEQ_V8_IMM16:
        case EcmaOpcode::JNSTRICTEQ_V8_IMM8:
        case EcmaOpcode::JNSTRICTEQ_V8_IMM16:
            break;
        case EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            LowerLdThisByName(gate, jsFunc);
            break;
        case EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case EcmaOpcode::STTHISBYNAME_IMM16_ID16:
            LowerStObjByName(gate, jsFunc, true);
            break;
        case EcmaOpcode::CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE:
            LowerNotifyConcurrentResult(gate);
            break;
        default:
            break;
    }
}

GateRef SlowPathLowering::LowerCallRuntime(int index, const std::vector<GateRef> &args, bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue_, index, Gate::InvalidGateRef, args);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue_, target, dependEntry_, args);
        return result;
    }
}

GateRef SlowPathLowering::LowerCallNGCRuntime(int index, const std::vector<GateRef> &args, bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallNGCRuntime(glue_, index, Gate::InvalidGateRef, args);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(index);
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue_, target, dependEntry_, args);
        return result;
    }
}

void SlowPathLowering::LowerAdd2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Add,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerCreateIterResultObj(GateRef gate)
{
    const int id = RTSTUB_ID(CreateIterResultObj);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

// When executing to SUSPENDGENERATOR instruction, save contextual information to GeneratorContext,
// including registers, acc, etc.
void SlowPathLowering::SaveFrameToContext(GateRef gate, GateRef jsFunc)
{
    GateRef genObj = acc_.GetValueIn(gate, 1);
    GateRef saveRegister = acc_.GetDep(gate);
    ASSERT(acc_.GetOpCode(saveRegister) == OpCode::SAVE_REGISTER);

    acc_.SetDep(gate, acc_.GetDep(saveRegister));
    builder_.SetDepend(acc_.GetDep(saveRegister));
    GateRef context =
        builder_.Load(VariableType::JS_POINTER(), genObj, builder_.IntPtr(JSGeneratorObject::GENERATOR_CONTEXT_OFFSET));
    // new tagged array
    auto method = methodLiteral_;
    const size_t arrLength = method->GetNumberVRegs() + 1; // 1: env vreg
    GateRef length = builder_.Int32(arrLength);
    GateRef taggedLength = builder_.ToTaggedInt(builder_.ZExtInt32ToInt64(length));
    const int arrayId = RTSTUB_ID(NewTaggedArray);
    GateRef taggedArray = LowerCallRuntime(arrayId, {taggedLength});
    // setRegsArrays
    auto hole = builder_.HoleConstant();
    size_t numVreg = acc_.GetNumValueIn(saveRegister);
    for (size_t idx = 0; idx < numVreg; idx++) {
        GateRef tmpGate = acc_.GetValueIn(saveRegister, idx);
        if (tmpGate != hole) {
            builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue_, taggedArray, builder_.Int32(idx), tmpGate);
        }
    }
    GateRef lexicalEnvGate = acc_.GetValueIn(saveRegister, numVreg - 1);
    acc_.DeleteGate(saveRegister);

    // setRegsArrays
    GateRef regsArrayOffset = builder_.IntPtr(GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET);
    builder_.Store(VariableType::JS_POINTER(), glue_, context, regsArrayOffset, taggedArray);

    // set this
    GateRef thisOffset = builder_.IntPtr(GeneratorContext::GENERATOR_THIS_OFFSET);
    GateRef thisObj = acc_.GetValueIn(gate, 3); // 3: this object
    builder_.Store(VariableType::JS_ANY(), glue_, context, thisOffset, thisObj);

    // set method
    GateRef methodOffset = builder_.IntPtr(GeneratorContext::GENERATOR_METHOD_OFFSET);
    builder_.Store(VariableType::JS_ANY(), glue_, context, methodOffset, jsFunc);

    // set acc
    GateRef accOffset = builder_.IntPtr(GeneratorContext::GENERATOR_ACC_OFFSET);
    GateRef curAccGate = acc_.GetValueIn(gate, acc_.GetNumValueIn(gate) - 1); // get current acc
    builder_.Store(VariableType::JS_ANY(), glue_, context, accOffset, curAccGate);

    // set generator object
    GateRef generatorObjectOffset = builder_.IntPtr(GeneratorContext::GENERATOR_GENERATOR_OBJECT_OFFSET);
    builder_.Store(VariableType::JS_ANY(), glue_, context, generatorObjectOffset, genObj);

    // set lexical env
    GateRef lexicalEnvOffset = builder_.IntPtr(GeneratorContext::GENERATOR_LEXICALENV_OFFSET);
    builder_.Store(VariableType::JS_ANY(), glue_, context, lexicalEnvOffset, lexicalEnvGate);

    // set nregs
    GateRef nregsOffset = builder_.IntPtr(GeneratorContext::GENERATOR_NREGS_OFFSET);
    builder_.Store(VariableType::INT32(), glue_, context, nregsOffset, length);

    // set bc size
    GateRef bcSizeOffset = builder_.IntPtr(GeneratorContext::GENERATOR_BC_OFFSET_OFFSET);
    GateRef bcSizeGate = acc_.GetValueIn(gate, 0); // saved bc_offset
    bcSizeGate = builder_.TruncInt64ToInt32(bcSizeGate);
    builder_.Store(VariableType::INT32(), glue_, context, bcSizeOffset, bcSizeGate);

    // set context to generator object
    GateRef contextOffset = builder_.IntPtr(JSGeneratorObject::GENERATOR_CONTEXT_OFFSET);
    builder_.Store(VariableType::JS_POINTER(), glue_, genObj, contextOffset, context);

    // set generator object to context
    builder_.Store(VariableType::JS_POINTER(), glue_, context, generatorObjectOffset, genObj);
}

void SlowPathLowering::LowerSuspendGenerator(GateRef gate, GateRef jsFunc)
{
    SaveFrameToContext(gate, jsFunc);
    acc_.SetDep(gate, builder_.GetDepend());
    AddProfiling(gate, false);
    const int id = RTSTUB_ID(OptSuspendGenerator);
    auto value = acc_.GetValueIn(gate, 2); // 2: acc
    auto genObj = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(id, { genObj, value });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionAwaitUncaught(GateRef gate)
{
    const int id = RTSTUB_ID(AsyncFunctionAwaitUncaught);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionResolve(GateRef gate)
{
    const int id = RTSTUB_ID(AsyncFunctionResolveOrReject);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef taggedTrue = builder_.TaggedTrue();
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), taggedTrue});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncFunctionReject(GateRef gate)
{
    const int id = RTSTUB_ID(AsyncFunctionResolveOrReject);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef taggedFalse = builder_.TaggedFalse();
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), taggedFalse});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLoadStr(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef newGate = builder_.GetObjectFromConstPool(glue_, jsFunc,
                                                      builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0)),
                                                      ConstPoolType::STRING);
    builder_.Branch(builder_.IsSpecial(newGate, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, newGate, successControl, failControl);
}

void SlowPathLowering::LowerTryLdGlobalByName(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef prop = acc_.GetValueIn(gate, 1);  // 1: the second parameter
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::TryLdGlobalByName,
        { glue_, prop, *profileTypeInfo, slotId });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStGlobalVar(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef prop = acc_.GetValueIn(gate, 1);  // 1: the second parameter
    GateRef value = acc_.GetValueIn(gate, 2);  // 2: the 2nd para is value
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::StGlobalVar,
        { glue_, prop, value, *profileTypeInfo, slotId });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerGetIterator(GateRef gate)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    auto result = LowerCallRuntime(RTSTUB_ID(GetIterator), {acc_.GetValueIn(gate, 0)}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerToJSCall(GateRef gate, const std::vector<GateRef> &args)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCall));
    GateRef target = builder_.IntPtr(RTSTUB_ID(JSCall));
    GateRef newGate = builder_.Call(cs, glue_, target, dependEntry_, args);
    ReplaceHirToJSCall(gate, newGate);
}

void SlowPathLowering::LowerGetAsyncIterator(GateRef gate)
{
    auto result = LowerCallRuntime(RTSTUB_ID(GetAsyncIterator), {acc_.GetValueIn(gate, 0)}, true);
    ReplaceHirToCall(gate, result);
}

void SlowPathLowering::LowerCallArg0(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);

    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLARG0_IMM8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef func = acc_.GetValueIn(gate, 0);
    GateRef env = builder_.Undefined();
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    LowerToJSCall(gate, {glue_, env, actualArgc, func, newTarget, thisObj, bcOffset});
}

void SlowPathLowering::LowerCallthisrangeImm8Imm8V8(GateRef gate)
{
    std::vector<GateRef> vec;
    // this
    size_t fixedInputsNum = 1;
    ASSERT(acc_.GetNumValueIn(gate) - fixedInputsNum >= 0);
    size_t numIns = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8));
    const size_t callTargetIndex = 1;  // 1: acc
    GateRef callTarget = acc_.GetValueIn(gate, numIns - callTargetIndex); // acc
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef newTarget = builder_.Undefined();
    GateRef env = builder_.Undefined();
    vec.emplace_back(glue_);
    vec.emplace_back(env);
    vec.emplace_back(actualArgc);
    vec.emplace_back(callTarget);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);
    // add common args
    for (size_t i = fixedInputsNum; i < numIns - callTargetIndex; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    vec.emplace_back(bcOffset);
    LowerToJSCall(gate, vec);
}

void SlowPathLowering::LowerWideCallthisrangePrefImm16V8(GateRef gate)
{
    // The first register input is thisobj, second is thisObj and other inputs are common args.
    size_t fixedInputsNum = 1; // 1: acc
    ASSERT(acc_.GetNumValueIn(gate) - fixedInputsNum >= 0);
    size_t numIns = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8));
    const size_t callTargetIndex = 1;
    GateRef callTarget = acc_.GetValueIn(gate, numIns - callTargetIndex);
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef newTarget = builder_.Undefined();
    GateRef env = builder_.Undefined();
    std::vector<GateRef> vec {glue_, env, actualArgc, callTarget, newTarget, thisObj};
    // add common args
    for (size_t i = fixedInputsNum; i < numIns - callTargetIndex; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    vec.emplace_back(bcOffset);
    LowerToJSCall(gate, vec);
}

void SlowPathLowering::LowerCallSpread(GateRef gate)
{
    // need to fixed in later
    const int id = RTSTUB_ID(CallSpread);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef newGate = LowerCallRuntime(id,
        {acc_.GetValueIn(gate, 2), acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCallrangeImm8Imm8V8(GateRef gate)
{
    std::vector<GateRef> vec;
    size_t numArgs = acc_.GetNumValueIn(gate);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLRANGE_IMM8_IMM8_V8));
    const size_t callTargetIndex = 1; // acc
    GateRef callTarget = acc_.GetValueIn(gate, numArgs - callTargetIndex);
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef env = builder_.Undefined();
    vec.emplace_back(glue_);
    vec.emplace_back(env);
    vec.emplace_back(actualArgc);
    vec.emplace_back(callTarget);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);

    for (size_t i = 0; i < numArgs - callTargetIndex; i++) { // 2: skip acc
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    vec.emplace_back(bcOffset);
    LowerToJSCall(gate, vec);
}

void SlowPathLowering::LowerNewObjApply(GateRef gate)
{
    const int id = RTSTUB_ID(NewObjApply);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(id,
        {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerThrow(GateRef gate)
{
    GateRef exception = acc_.GetValueIn(gate, 0);
    GateRef exceptionOffset = builder_.Int64(JSThread::GlueData::GetExceptionOffset(false));
    GateRef val = builder_.Int64Add(glue_, exceptionOffset);
    GateRef setException = circuit_->NewGate(circuit_->Store(), MachineType::NOVALUE,
        {dependEntry_, exception, val}, VariableType::INT64().GetGateType());
    ReplaceHirToThrowCall(gate, setException);
}

void SlowPathLowering::LowerThrowConstAssignment(GateRef gate)
{
    const int id = RTSTUB_ID(ThrowConstAssignment);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToThrowCall(gate, newGate);
}

void SlowPathLowering::LowerThrowThrowNotExists(GateRef gate)
{
    const int id = RTSTUB_ID(ThrowThrowNotExists);
    GateRef newGate = LowerCallRuntime(id, {});
    ReplaceHirToThrowCall(gate, newGate);
}

void SlowPathLowering::LowerThrowPatternNonCoercible(GateRef gate)
{
    const int id = RTSTUB_ID(ThrowPatternNonCoercible);
    GateRef newGate = LowerCallRuntime(id, {});
    ReplaceHirToThrowCall(gate, newGate);
}

void SlowPathLowering::LowerThrowIfNotObject(GateRef gate)
{
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
        LowerCallRuntime(RTSTUB_ID(ThrowIfNotObject), {}, true);
        builder_.Jump(&exceptionExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerThrowUndefinedIfHole(GateRef gate)
{
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
        LowerCallRuntime(RTSTUB_ID(ThrowUndefinedIfHole), {obj}, true);
        builder_.Jump(&exceptionExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerThrowUndefinedIfHoleWithName(GateRef gate, GateRef jsFunc)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef hole = acc_.GetValueIn(gate, 1);
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
        GateRef obj = builder_.GetObjectFromConstPool(glue_, jsFunc,
                                                      builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0)),
                                                      ConstPoolType::STRING);
        LowerCallRuntime(RTSTUB_ID(ThrowUndefinedIfHole), {obj}, true);
        builder_.Jump(&exceptionExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerThrowIfSuperNotCorrectCall(GateRef gate)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef result = LowerCallRuntime(RTSTUB_ID(ThrowIfSuperNotCorrectCall),
        {builder_.ToTaggedInt(acc_.GetValueIn(gate, 0)), acc_.GetValueIn(gate, 1)}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerThrowDeleteSuperProperty(GateRef gate)
{
    const int id = RTSTUB_ID(ThrowDeleteSuperProperty);
    GateRef newGate = LowerCallRuntime(id, {});
    ReplaceHirToThrowCall(gate, newGate);
}

void SlowPathLowering::LowerExceptionHandler(GateRef hirGate)
{
    GateRef depend = acc_.GetDep(hirGate);
    GateRef exceptionOffset = builder_.Int64(JSThread::GlueData::GetExceptionOffset(false));
    GateRef val = builder_.Int64Add(glue_, exceptionOffset);
    GateRef loadException = circuit_->NewGate(circuit_->Load(), VariableType::JS_ANY().GetMachineType(),
        { depend, val }, VariableType::JS_ANY().GetGateType());
    acc_.SetDep(loadException, depend);
    GateRef holeCst = builder_.HoleConstant();
    GateRef clearException = circuit_->NewGate(circuit_->Store(), MachineType::NOVALUE,
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

void SlowPathLowering::LowerLdSymbol(GateRef gate)
{
    const int id = RTSTUB_ID(GetSymbolFunction);
    GateRef newGate = LowerCallRuntime(id, {});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdGlobal(GateRef gate)
{
    GateRef offset = builder_.Int64(JSThread::GlueData::GetGlobalObjOffset(false));
    GateRef val = builder_.Int64Add(glue_, offset);
    GateRef newGate = circuit_->NewGate(circuit_->Load(), VariableType::JS_ANY().GetMachineType(),
        { dependEntry_, val }, VariableType::JS_ANY().GetGateType());
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSub2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Sub,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerMul2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Mul,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerDiv2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Div,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerMod2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Mod,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerEq(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Equal,
                                       { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerNotEq(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::NotEqual,
                                       { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerLess(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Less,
                                       { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerLessEq(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::LessEq,
                                       { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGreater(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Greater,
                                       { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGreaterEq(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::GreaterEq,
                                       { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGetPropIterator(GateRef gate)
{
    const int id = RTSTUB_ID(GetPropIterator);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCloseIterator(GateRef gate)
{
    const int id = RTSTUB_ID(CloseIterator);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerInc(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Inc,
        { glue_, acc_.GetValueIn(gate, 0) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerDec(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Dec,
        { glue_, acc_.GetValueIn(gate, 0) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerToNumber(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label notNumber(&builder_);
    Label checkResult(&builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), value);
    builder_.Branch(builder_.TaggedIsNumber(value), &checkResult, &notNumber);
    builder_.Bind(&notNumber);
    {
        result = LowerCallRuntime(RTSTUB_ID(ToNumber), { value }, true);
        builder_.Jump(&checkResult);
    }
    builder_.Bind(&checkResult);
    builder_.Branch(builder_.TaggedIsException(*result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, *result, successControl, failControl);
}

void SlowPathLowering::LowerNeg(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Neg,
        { glue_, acc_.GetValueIn(gate, 0) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerNot(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Not,
        { glue_, acc_.GetValueIn(gate, 0) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerShl2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Shl,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerShr2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Shr,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerAshr2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Ashr,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerAnd2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::And,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerOr2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Or,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerXor2(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Xor,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerDelObjProp(GateRef gate)
{
    const int id = RTSTUB_ID(DelObjProp);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerExp(GateRef gate)
{
    const int id = RTSTUB_ID(Exp);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIsIn(GateRef gate)
{
    const int id = RTSTUB_ID(IsIn);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerInstanceof(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::Instanceof,
        { glue_, acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1) });
    builder_.Branch(builder_.HasPendingException(glue_), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerFastStrictNotEqual(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label strictEqual(&builder_);
    Label notStrictEqual(&builder_);
    Label exit(&builder_);
    builder_.Branch(FastStrictEqual(acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)), &strictEqual,
        &notStrictEqual);
    builder_.Bind(&strictEqual);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&notStrictEqual);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

void SlowPathLowering::LowerFastStrictEqual(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    Label strictEqual(&builder_);
    Label notStrictEqual(&builder_);
    Label exit(&builder_);
    builder_.Branch(FastStrictEqual(acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)), &strictEqual,
        &notStrictEqual);
    builder_.Bind(&strictEqual);
    {
        result = builder_.TaggedTrue();
        builder_.Jump(&exit);
    }
    builder_.Bind(&notStrictEqual);
    {
        result = builder_.TaggedFalse();
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

GateRef SlowPathLowering::FastStrictEqual(GateRef left, GateRef right)
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
                doubleLeft = builder_.ChangeInt32ToFloat64(builder_.GetInt32OfTInt(left));
                builder_.Jump(&getRight);
            }
            builder_.Bind(&leftNotInt);
            {
                doubleLeft = builder_.GetDoubleOfTDouble(left);
                builder_.Jump(&getRight);
            }
            builder_.Bind(&getRight);
            {
                Label rightIsInt(&builder_);
                Label rightNotInt(&builder_);
                builder_.Branch(builder_.TaggedIsInt(right), &rightIsInt, &rightNotInt);
                builder_.Bind(&rightIsInt);
                {
                    doubleRight = builder_.ChangeInt32ToFloat64(builder_.GetInt32OfTInt(right));
                    builder_.Jump(&strictNumberEqualCheck);
                }
                builder_.Bind(&rightNotInt);
                {
                    doubleRight = builder_.GetDoubleOfTDouble(right);
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
            builder_.Equal(builder_.GetHashcodeFromString(glue_, left), builder_.GetHashcodeFromString(glue_, right)),
            &contentsCompare, &exit);
        builder_.Bind(&contentsCompare);
        {
            GateRef stringEqual = LowerCallRuntime(RTSTUB_ID(StringEqual), { left, right }, true);
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
                GateRef bigIntEqual = LowerCallRuntime(RTSTUB_ID(BigIntEqual), { left, right }, true);
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

void SlowPathLowering::LowerCreateEmptyArray(GateRef gate)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    Label afterNewJSArray(&builder_);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    GateRef glueGlobalEnvOffset =
        builder_.IntPtr(JSThread::GlueData::GetGlueGlobalEnvOffset(builder_.GetCompilationConfig()->Is32Bit()));
    GateRef glueGlobalEnv = builder_.Load(VariableType::NATIVE_POINTER(), glue_, glueGlobalEnvOffset);

    auto arrayFunc = builder_.GetGlobalEnvValue(VariableType::JS_ANY(), glueGlobalEnv, GlobalEnv::ARRAY_FUNCTION_INDEX);
    auto hclass = builder_.Load(VariableType::JS_POINTER(), arrayFunc,
                                builder_.IntPtr(JSFunction::PROTO_OR_DYNCLASS_OFFSET));

    GateRef size = builder_.GetObjectSizeFromHClass(hclass);
    auto emptyArray = builder_.GetGlobalConstantValue(VariableType::JS_POINTER(), glue_,
                                                      ConstantIndex::EMPTY_ARRAY_OBJECT_INDEX);
    auto env = builder_.GetCurrentEnvironment();
    env->SetCompilationConfig(builder_.GetCompilationConfig());
    {
        NewObjectStubBuilder stubBuilder(env);
        stubBuilder.SetParameters(glue_, size);
        stubBuilder.NewJSArrayLiteral(&result, &afterNewJSArray, RegionSpaceFlag::IN_YOUNG_SPACE, emptyArray, hclass,
                                      true);
    }
    builder_.Bind(&afterNewJSArray);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

void SlowPathLowering::LowerCreateEmptyObject(GateRef gate)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef result = LowerCallRuntime(RTSTUB_ID(CreateEmptyObject), {}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerCreateArrayWithBuffer(GateRef gate, GateRef jsFunc)
{
    Label afterNewJSArray(&builder_);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    GateRef index = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef method = builder_.GetMethodFromFunction(jsFunc);
    GateRef constPool = builder_.Load(VariableType::JS_ANY(), method, builder_.IntPtr(Method::CONSTANT_POOL_OFFSET));
    GateRef module = builder_.GetModuleFromFunction(jsFunc);

    auto obj = builder_.GetObjectFromConstPool(glue_, constPool, module, index, ConstPoolType::ARRAY_LITERAL);
    auto hclass = builder_.LoadHClass(obj);
    GateRef size = builder_.GetObjectSizeFromHClass(hclass);

    auto env = builder_.GetCurrentEnvironment();
    env->SetCompilationConfig(builder_.GetCompilationConfig());
    {
        NewObjectStubBuilder stubBuilder(env);
        stubBuilder.SetParameters(glue_, size);
        stubBuilder.NewJSArrayLiteral(&result, &afterNewJSArray, RegionSpaceFlag::IN_YOUNG_SPACE, obj, hclass, false);
    }
    builder_.Bind(&afterNewJSArray);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

void SlowPathLowering::LowerCreateObjectWithBuffer(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef obj = builder_.GetObjectFromConstPool(glue_, jsFunc, builder_.TruncInt64ToInt32(index),
                                                  ConstPoolType::OBJECT_LITERAL);
    GateRef lexEnv = acc_.GetValueIn(gate, 1);
    GateRef result = LowerCallRuntime(RTSTUB_ID(CreateObjectHavingMethod), { obj, lexEnv }, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStModuleVar(GateRef gate, GateRef jsFunc)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef index = builder_.ToTaggedInt(acc_.GetValueIn(gate, 0));
    LowerCallRuntime(RTSTUB_ID(StModuleVarByIndexOnJSFunc), {index, acc_.GetValueIn(gate, 1), jsFunc}, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    // StModuleVar will not be inValue to other hir gates, result will not be used to replace hirgate
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl, true);
}

void SlowPathLowering::LowerSetGeneratorState(GateRef gate, GateRef jsFunc)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef index = builder_.ToTaggedInt(acc_.GetValueIn(gate, 0));
    auto result = LowerCallRuntime(RTSTUB_ID(SetGeneratorState),
        {index, acc_.GetValueIn(gate, 1), jsFunc}, true);
    ReplaceHirToCall(gate, result, true);
}

void SlowPathLowering::LowerGetTemplateObject(GateRef gate)
{
    const int id = RTSTUB_ID(GetTemplateObject);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef literal = acc_.GetValueIn(gate, 0);
    GateRef newGate = LowerCallRuntime(id, { literal });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSetObjectWithProto(GateRef gate)
{
    const int id = RTSTUB_ID(SetObjectWithProto);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef proto = acc_.GetValueIn(gate, 0);
    GateRef obj = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(id, { proto, obj });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdBigInt(GateRef gate)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef numberBigInt = acc_.GetValueIn(gate, 0);
    GateRef result = LowerCallRuntime(RTSTUB_ID(LdBigInt), {numberBigInt}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerToNumeric(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label notNumber(&builder_);
    Label checkResult(&builder_);
    GateRef value = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), value);
    builder_.Branch(builder_.TaggedIsNumeric(value), &checkResult, &notNumber);
    builder_.Bind(&notNumber);
    {
        result = LowerCallRuntime(RTSTUB_ID(ToNumeric), { value }, true);
        builder_.Jump(&checkResult);
    }
    builder_.Bind(&checkResult);
    builder_.Branch(builder_.TaggedIsException(*result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, *result, successControl, failControl);
}

void SlowPathLowering::LowerDynamicImport(GateRef gate, GateRef jsFunc)
{
    const int id = RTSTUB_ID(DynamicImport);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), jsFunc});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdLocalModuleVarByIndex(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = builder_.ToTaggedInt(acc_.GetValueIn(gate, 0));
    GateRef result = LowerCallRuntime(RTSTUB_ID(LdLocalModuleVarByIndexOnJSFunc), {index, jsFunc}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerExternalModule(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef index = builder_.ToTaggedInt(acc_.GetValueIn(gate, 0));
    GateRef result = LowerCallRuntime(RTSTUB_ID(LdExternalModuleVarByIndexOnJSFunc), {index, jsFunc}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGetModuleNamespace(GateRef gate, GateRef jsFunc)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef index = builder_.ToTaggedInt(acc_.GetValueIn(gate, 0));
    GateRef result = LowerCallRuntime(RTSTUB_ID(GetModuleNamespaceByIndexOnJSFunc), {index, jsFunc}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerSuperCall(GateRef gate, GateRef func, GateRef newTarget)
{
    const int id = RTSTUB_ID(OptSuperCall);
    std::vector<GateRef> vec;
    ASSERT(acc_.GetNumValueIn(gate) >= 0);
    size_t numIns = acc_.GetNumValueIn(gate);
    vec.emplace_back(func);
    vec.emplace_back(newTarget);
    for (size_t i = 0; i < numIns; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef newGate = LowerCallRuntime(id, vec);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuperCallArrow(GateRef gate, GateRef newTarget)
{
    const int id = RTSTUB_ID(OptSuperCall);
    std::vector<GateRef> vec;
    ASSERT(acc_.GetNumValueIn(gate) > 0);
    size_t numIns = acc_.GetNumValueIn(gate);
    size_t funcIndex = numIns - 1;
    GateRef func = acc_.GetValueIn(gate, funcIndex);
    vec.emplace_back(func);
    vec.emplace_back(newTarget);
    for (size_t i = 0; i < funcIndex; i++) {
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef newGate = LowerCallRuntime(id, vec);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerSuperCallSpread(GateRef gate, GateRef newTarget)
{
    const int id = RTSTUB_ID(OptSuperCallSpread);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef func = acc_.GetValueIn(gate, 1);
    GateRef array = acc_.GetValueIn(gate, 0);
    GateRef newGate = LowerCallRuntime(id, { func, newTarget, array });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerIsTrueOrFalse(GateRef gate, bool flag)
{
    Label slowpath(&builder_);
    Label isTrue(&builder_);
    Label isFalse(&builder_);
    Label successExit(&builder_);
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    auto value = acc_.GetValueIn(gate, 0);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), value);
    result = builder_.CallStub(glue_, CommonStubCSigns::ToBoolean, { glue_, value });
    builder_.Branch(builder_.TaggedIsTrue(*result), &isTrue, &isFalse);
    builder_.Bind(&isTrue);
    {
        result = flag ? builder_.TaggedTrue() : builder_.TaggedFalse();
        builder_.Jump(&successExit);
    }
    builder_.Bind(&isFalse);
    {
        result = flag ? builder_.TaggedFalse() : builder_.TaggedTrue();
        builder_.Jump(&successExit);
    }
    builder_.Bind(&successExit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerNewObjRange(GateRef gate)
{
    Label fastPath(&builder_);
    Label slowPath(&builder_);
    Label threadCheck(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);

    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());

    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateRef thisObj = builder_.CallStub(glue_, CommonStubCSigns::NewThisObjectChecked, { glue_, ctor });
    builder_.Branch(builder_.TaggedIsHole(thisObj), &slowPath, &fastPath);
    builder_.Bind(&fastPath);
    {
        const int extra = 5; // 5: add glue, lexEnv, argc, new-target and this
        GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8));
        size_t range = acc_.GetNumValueIn(gate);
        std::vector<GateRef> args(range + extra);
        args[0] = glue_;
        args[1] = builder_.Undefined();
        args[2] = actualArgc;  // 2: argc
        args[3] = ctor;        // 3: call-target
        args[4] = ctor;        // 4: new-target
        args[5] = thisObj;     // 5: this
        for (size_t i = 1; i < range; ++i) {
            args[i + extra] = acc_.GetValueIn(gate, i);
        }
        result = LowerCallNGCRuntime(RTSTUB_ID(JSCallNew), args, true);
        result = builder_.CallStub(glue_, CommonStubCSigns::ConstructorCheck, { glue_, ctor, *result, thisObj });
        builder_.Jump(&threadCheck);
    }
    builder_.Bind(&slowPath);
    {
        size_t range = acc_.GetNumValueIn(gate);
        std::vector<GateRef> args(range);
        for (size_t i = 0; i < range; ++i) {
            args[i] = acc_.GetValueIn(gate, i);
        }
        result = LowerCallRuntime(RTSTUB_ID(OptNewObjRange), args, true);
        builder_.Jump(&threadCheck);
    }
    builder_.Bind(&threadCheck);
    builder_.Branch(builder_.HasPendingException(glue_), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, *result, successControl, failControl);
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
    auto mergeIntState = builder_.Merge(intFalseState);

    // (GET_ACC().IsDouble() && GET_ACC().GetDouble() == 0)
    std::vector<GateRef> doubleFalseState;
    ifBranch = builder_.Branch(mergeIntState, builder_.TaggedIsDouble(value));
    GateRef isDouble = builder_.IfTrue(ifBranch);
    GateRef notDouble = builder_.IfFalse(ifBranch);
    doubleFalseState.emplace_back(notDouble);
    condition = builder_.Equal(builder_.GetDoubleOfTDouble(value), builder_.Double(0));
    ifBranch = builder_.Branch(isDouble, condition);
    GateRef isDoubleZero = builder_.IfTrue(ifBranch);
    GateRef notDoubleZero = builder_.IfFalse(ifBranch);
    trueState.emplace_back(isEqualJump ? isDoubleZero : notDoubleZero);
    doubleFalseState.emplace_back(isEqualJump ? notDoubleZero : isDoubleZero);
    auto mergeFalseState = builder_.Merge(doubleFalseState);

    GateRef mergeTrueState = builder_.Merge(trueState);
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        if (acc_.GetOpCode(*it) == OpCode::IF_TRUE) {
            acc_.SetMetaData(*it, circuit_->OrdinaryBlock());
            it = acc_.ReplaceIn(it, mergeTrueState);
        } else if (acc_.GetOpCode(*it) == OpCode::IF_FALSE) {
            acc_.SetMetaData(*it, circuit_->OrdinaryBlock());
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

void SlowPathLowering::LowerGetNextPropName(GateRef gate)
{
    const int id = RTSTUB_ID(GetNextPropName);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef iter = acc_.GetValueIn(gate, 0);
    GateRef newGate = LowerCallRuntime(id, { iter });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCopyDataProperties(GateRef gate)
{
    const int id = RTSTUB_ID(CopyDataProperties);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef dst = acc_.GetValueIn(gate, 0);
    GateRef src = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(id, { dst, src });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateObjectWithExcludedKeys(GateRef gate)
{
    const int id = RTSTUB_ID(CreateObjectWithExcludedKeys);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef numKeys = acc_.GetValueIn(gate, 0);
    GateRef obj = acc_.GetValueIn(gate, 1);
    GateRef firstArgRegIdx = acc_.GetValueIn(gate, 2);
    auto args = { builder_.ToTaggedInt(numKeys), obj, builder_.ToTaggedInt(firstArgRegIdx) };
    GateRef newGate = LowerCallRuntime(id, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateRegExpWithLiteral(GateRef gate)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    const int id = RTSTUB_ID(CreateRegExpWithLiteral);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef pattern = acc_.GetValueIn(gate, 0);
    GateRef flags = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(id, { pattern, builder_.ToTaggedInt(flags) }, true);
    builder_.Branch(builder_.IsSpecial(newGate, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, newGate, successControl, failControl);
}

void SlowPathLowering::LowerStOwnByValue(GateRef gate)
{
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
        result = builder_.CallStub(glue_, CommonStubCSigns::SetPropertyByValueWithOwn,
            { glue_, receiver, propKey, accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = LowerCallRuntime(RTSTUB_ID(StOwnByValue), { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // stOwnByValue will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByIndex(GateRef gate)
{
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
        result = builder_.CallStub(glue_, CommonStubCSigns::SetPropertyByIndexWithOwn,
            { glue_, receiver, builder_.TruncInt64ToInt32(index), accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        auto args = {receiver, builder_.ToTaggedInt(index), accValue };
        result = LowerCallRuntime(RTSTUB_ID(StOwnByIndex), args, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // StOwnByIndex will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByName(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef propKey = acc_.GetValueIn(gate, 0);
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
        result = builder_.CallStub(glue_, CommonStubCSigns::SetPropertyByNameWithOwn,
            { glue_, receiver, propKey, accValue });
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = LowerCallRuntime(RTSTUB_ID(StOwnByName), {receiver, propKey, accValue }, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    // StOwnByName will not be inValue to other hir gates, result gate will be ignored
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerNewLexicalEnv(GateRef gate)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef lexEnv = acc_.GetValueIn(gate, 1);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::NewLexicalEnv,
        { glue_, lexEnv, builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0)) });
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerNewLexicalEnvWithName(GateRef gate, GateRef jsFunc)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef lexEnv = acc_.GetValueIn(gate, 2); // 2: Get current lexEnv
    auto args = { builder_.ToTaggedInt(acc_.GetValueIn(gate, 0)),
                  builder_.ToTaggedInt(acc_.GetValueIn(gate, 1)),
                  lexEnv, jsFunc };
    GateRef result = LowerCallRuntime(RTSTUB_ID(OptNewLexicalEnvWithName), args, true);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, failControl, true);
}

void SlowPathLowering::LowerPopLexicalEnv(GateRef gate)
{
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef currentEnv = acc_.GetValueIn(gate, 0);
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    GateRef parentEnv = builder_.GetValueFromTaggedArray(currentEnv, index);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, parentEnv, successControl, failControl, true);
}

void SlowPathLowering::LowerLdSuperByValue(GateRef gate, GateRef jsFunc)
{
    const int id = RTSTUB_ID(OptLdSuperByValue);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef newGate = LowerCallRuntime(id, { receiver, propKey, jsFunc });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStSuperByValue(GateRef gate, GateRef jsFunc)
{
    const int id = RTSTUB_ID(OptStSuperByValue);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef receiver = acc_.GetValueIn(gate, 0);
    GateRef propKey = acc_.GetValueIn(gate, 1);
    GateRef value = acc_.GetValueIn(gate, 2);
    GateRef newGate = LowerCallRuntime(id, { receiver, propKey, value, jsFunc});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerTryStGlobalByName(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef prop = acc_.GetValueIn(gate, 1);  // 1: the second parameter
    GateRef value = acc_.GetValueIn(gate, 2);  // 2: the 2nd para is value
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::TryStGlobalByName,
        { glue_, prop, value, *profileTypeInfo, slotId });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStConstToGlobalRecord(GateRef gate, bool isConst)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef propKey = acc_.GetValueIn(gate, 0);
    acc_.SetDep(gate, propKey);
    // 2 : number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    const int id = RTSTUB_ID(StGlobalRecord);
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef isConstGate = isConst ? builder_.TaggedTrue() : builder_.TaggedFalse();
    GateRef newGate = LowerCallRuntime(id, { propKey, value, isConstGate }, true);
    builder_.Branch(builder_.IsSpecial(newGate, JSTaggedValue::VALUE_EXCEPTION), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, newGate, successControl, failControl);
}

void SlowPathLowering::LowerStOwnByValueWithNameSet(GateRef gate)
{
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
                res = builder_.CallStub(glue_, CommonStubCSigns::DeprecatedSetPropertyByValue,
                    { glue_, receiver, propKey, accValue });
                builder_.Branch(builder_.IsSpecial(res, JSTaggedValue::VALUE_HOLE),
                    &slowPath, &notHole);
                builder_.Bind(&notHole);
                {
                    Label notexception(&builder_);
                    builder_.Branch(builder_.IsSpecial(res, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &notexception);
                    builder_.Bind(&notexception);
                    LowerCallRuntime(RTSTUB_ID(SetFunctionNameNoPrefix), { accValue, propKey }, true);
                    builder_.Jump(&successExit);
                }
            }
        }
    }
    builder_.Bind(&slowPath);
    {
        res = LowerCallRuntime(RTSTUB_ID(StOwnByValueWithNameSet), { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.IsSpecial(res, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerStOwnByNameWithNameSet(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef propKey = acc_.GetValueIn(gate, 0);
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
                result = builder_.CallStub(glue_, CommonStubCSigns::SetPropertyByNameWithOwn,
                    { glue_, receiver, propKey, accValue });
                builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE),
                    &notJSObject, &notHole);
                builder_.Bind(&notHole);
                {
                    Label notException(&builder_);
                    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
                        &exceptionExit, &notException);
                    builder_.Bind(&notException);
                    LowerCallRuntime(RTSTUB_ID(SetFunctionNameNoPrefix), {accValue, propKey}, true);
                    builder_.Jump(&successExit);
                }
            }
        }
    }
    builder_.Bind(&notJSObject);
    {
        result = LowerCallRuntime(RTSTUB_ID(StOwnByNameWithNameSet), { receiver, propKey, accValue }, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerLdGlobalVar(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef prop = acc_.GetValueIn(gate, 1);  // 1: the second parameter
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::LdGlobalVar,
        { glue_, prop, *profileTypeInfo, slotId });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit);
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerLdObjByName(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef prop = acc_.GetValueIn(gate, 1);  // 1: the second parameter
    GateRef receiver = acc_.GetValueIn(gate, 2);  // 2: the third parameter
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::GetPropertyByName,
        { glue_, receiver, prop, *profileTypeInfo, slotId });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStObjByName(GateRef gate, GateRef jsFunc, bool isThis)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);
    GateRef receiver;
    GateRef value;
    if (isThis) {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 3); // 3: this object
        value = acc_.GetValueIn(gate, 2);  // 2: the third para is value
    } else {
        // 4: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 2);  // 2: the third para is receiver
        value = acc_.GetValueIn(gate, 3);  // 3: the 4th para is value
    }
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef prop = acc_.GetValueIn(gate, 1);  // 1: the second parameter
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::SetPropertyByName,
        { glue_, receiver, prop, value, *profileTypeInfo, slotId });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerDefineGetterSetterByValue(GateRef gate)
{
    const int id = RTSTUB_ID(DefineGetterSetterByValue);
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef obj = acc_.GetValueIn(gate, 0);
    GateRef prop = acc_.GetValueIn(gate, 1);
    GateRef getter = acc_.GetValueIn(gate, 2);
    GateRef setter = acc_.GetValueIn(gate, 3);
    GateRef acc = acc_.GetValueIn(gate, 4);
    auto args = { obj, prop, getter, setter, acc };
    GateRef newGate = LowerCallRuntime(id, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdObjByIndex(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef holeConst = builder_.HoleConstant();
    DEFVAlUE(varAcc, (&builder_), VariableType::JS_ANY(), holeConst);
    GateRef result;
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef receiver = acc_.GetValueIn(gate, 1);

    Label fastPath(&builder_);
    Label slowPath(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    builder_.Branch(builder_.TaggedIsHeapObject(receiver), &fastPath, &slowPath);
    builder_.Bind(&fastPath);
    {
        varAcc = builder_.CallStub(glue_, CommonStubCSigns::GetPropertyByIndex,
            {glue_, receiver, builder_.TruncInt64ToInt32(index)});
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(*varAcc, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        GateRef undefined = builder_.Undefined();
        auto args = { receiver, builder_.ToTaggedInt(index), builder_.TaggedFalse(), undefined };
        varAcc = LowerCallRuntime(RTSTUB_ID(LdObjByIndex), args);
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

void SlowPathLowering::LowerStObjByIndex(GateRef gate)
{
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
        result = builder_.CallStub(glue_, CommonStubCSigns::SetPropertyByIndex,
            {glue_, receiver, builder_.TruncInt64ToInt32(index), accValue});
        Label notHole(&builder_);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_HOLE), &slowPath, &notHole);
        builder_.Bind(&notHole);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    builder_.Bind(&slowPath);
    {
        result = LowerCallRuntime(RTSTUB_ID(StObjByIndex),
            {receiver, builder_.ToTaggedInt(index), accValue}, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
            &exceptionExit, &successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerLdObjByValue(GateRef gate, GateRef jsFunc, bool isThis)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);

    GateRef receiver;
    GateRef propKey;
    if (isThis) {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = acc_.GetValueIn(gate, 2); // 2: this object
        propKey = acc_.GetValueIn(gate, 1);
    } else {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 3);
        receiver = acc_.GetValueIn(gate, 1);
        propKey = acc_.GetValueIn(gate, 2);  // 2: the third parameter
    }
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(
        glue_, CommonStubCSigns::GetPropertyByValue, {glue_, receiver, propKey, *profileTypeInfo, slotId});
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStObjByValue(GateRef gate, GateRef jsFunc, bool isThis)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);
    GateRef receiver;
    GateRef propKey;
    GateRef value;
    if (isThis) {
        // 3: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 3); // 3: this object
        propKey = acc_.GetValueIn(gate, 1);
        value = acc_.GetValueIn(gate, 2);  // 2: the third parameter
    } else {
        // 4: number of value inputs
        ASSERT(acc_.GetNumValueIn(gate) == 4);
        receiver = acc_.GetValueIn(gate, 1);
        propKey = acc_.GetValueIn(gate, 2);  // 2: the third parameter
        value = acc_.GetValueIn(gate, 3);  // 3: the 4th parameter
    }
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(
        glue_, CommonStubCSigns::SetPropertyByValue, {glue_, receiver, propKey, value, *profileTypeInfo, slotId});
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerLdSuperByName(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef prop = acc_.GetValueIn(gate, 0);
    GateRef result =
        LowerCallRuntime(RTSTUB_ID(OptLdSuperByValue), {acc_.GetValueIn(gate, 1), prop, jsFunc}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerStSuperByName(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef prop = acc_.GetValueIn(gate, 0);
    auto args2 = { acc_.GetValueIn(gate, 1), prop, acc_.GetValueIn(gate, 2), jsFunc };
    GateRef result = LowerCallRuntime(RTSTUB_ID(OptStSuperByValue), args2, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &successExit);
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, Circuit::NullGate(), successControl, failControl);
}

void SlowPathLowering::LowerCreateGeneratorObj(GateRef gate)
{
    const int id = RTSTUB_ID(CreateGeneratorObj);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCreateAsyncGeneratorObj(GateRef gate)
{
    int id = RTSTUB_ID(CreateAsyncGeneratorObj);
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncGeneratorResolve(GateRef gate)
{
    int id = RTSTUB_ID(AsyncGeneratorResolve);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef asyncGen = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    GateRef flag = acc_.GetValueIn(gate, 2);
    GateRef newGate = LowerCallRuntime(id, {asyncGen, value, flag});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerAsyncGeneratorReject(GateRef gate)
{
    int id = RTSTUB_ID(AsyncGeneratorReject);
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerStArraySpread(GateRef gate)
{
    const int id = RTSTUB_ID(StArraySpread);
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    auto args = { acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1), acc_.GetValueIn(gate, 2) };
    GateRef newGate = LowerCallRuntime(id, args);
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerLdLexVar(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef level = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef slot = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 1));
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;

    DEFVAlUE(currentEnv, (&builder_), VariableType::JS_ANY(), acc_.GetValueIn(gate, 2)); // 2: Get current lexEnv
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    Label exit(&builder_);
    uint64_t constLevel = acc_.TryGetValue(acc_.GetValueIn(gate, 0));
    if (constLevel == 0) {
        builder_.Jump(&exit);
    } else if (constLevel == 1) {
        currentEnv = builder_.GetValueFromTaggedArray(*currentEnv, index);
        builder_.Jump(&exit);
    } else {
        DEFVAlUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
        Label loopHead(&builder_);
        Label loopEnd(&builder_);
        builder_.Branch(builder_.Int32LessThan(*i, level), &loopHead, &exit);
        builder_.LoopBegin(&loopHead);
        currentEnv = builder_.GetValueFromTaggedArray(*currentEnv, index);
        i = builder_.Int32Add(*i, builder_.Int32(1));
        builder_.Branch(builder_.Int32LessThan(*i, level), &loopEnd, &exit);
        builder_.Bind(&loopEnd);
        builder_.LoopEnd(&loopHead);
    }
    builder_.Bind(&exit);
    GateRef valueIndex = builder_.Int32Add(slot, builder_.Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    GateRef result = builder_.GetValueFromTaggedArray(*currentEnv, valueIndex);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerStLexVar(GateRef gate)
{
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef level = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef slot = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 1));
    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    GateRef value = acc_.GetValueIn(gate, 3);
    DEFVAlUE(currentEnv, (&builder_), VariableType::JS_ANY(), acc_.GetValueIn(gate, 2)); // 2: Get current lexEnv
    GateRef index = builder_.Int32(LexicalEnv::PARENT_ENV_INDEX);
    Label exit(&builder_);
    uint64_t constLevel = acc_.TryGetValue(acc_.GetValueIn(gate, 0));
    if (constLevel == 0) {
        builder_.Jump(&exit);
    } else if (constLevel == 1) {
        currentEnv = builder_.GetValueFromTaggedArray(*currentEnv, index);
        builder_.Jump(&exit);
    } else {
        DEFVAlUE(i, (&builder_), VariableType::INT32(), builder_.Int32(0));
        Label loopHead(&builder_);
        Label loopEnd(&builder_);
        builder_.Branch(builder_.Int32LessThan(*i, level), &loopHead, &exit);
        builder_.LoopBegin(&loopHead);
        currentEnv = builder_.GetValueFromTaggedArray(*currentEnv, index);
        i = builder_.Int32Add(*i, builder_.Int32(1));
        builder_.Branch(builder_.Int32LessThan(*i, level), &loopEnd, &exit);
        builder_.Bind(&loopEnd);
        builder_.LoopEnd(&loopHead);
    }
    builder_.Bind(&exit);
    GateRef valueIndex = builder_.Int32Add(slot, builder_.Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    builder_.SetValueToTaggedArray(VariableType::JS_ANY(), glue_, *currentEnv, valueIndex, value);
    auto result = *currentEnv;
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    exceptionControl.emplace_back(Circuit::NullGate());
    exceptionControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, result, successControl, exceptionControl, true);
}

void SlowPathLowering::LowerDefineClassWithBuffer(GateRef gate, GateRef jsFunc)
{
    GateType type = acc_.GetGateType(gate);

    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef methodId = acc_.GetValueIn(gate, 0);
    GateRef proto = acc_.GetValueIn(gate, 3);
    GateRef literalId = acc_.GetValueIn(gate, 1);
    GateRef length = acc_.GetValueIn(gate, 2);  // 2: second arg
    GateRef lexicalEnv = acc_.GetValueIn(gate, 4); // 4: Get current env
    GateRef constpool = builder_.GetConstPool(jsFunc);
    GateRef module = builder_.GetModuleFromFunction(jsFunc);
    Label isException(&builder_);
    Label isNotException(&builder_);

    GateRef result;
    if (type.IsAnyType()) {
        auto args = { proto, lexicalEnv, constpool,
                      builder_.ToTaggedInt(methodId), builder_.ToTaggedInt(literalId), module };
        result = LowerCallRuntime(RTSTUB_ID(CreateClassWithBuffer), args, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION), &isException, &isNotException);
    } else {
        int index = tsManager_->GetHClassIndexByClassGateType(type);
        ASSERT(index != -1);
        GateRef ihcIndex = builder_.Int32(index);
        GateRef ihclass = builder_.GetObjectFromConstPool(glue_, jsFunc, ihcIndex, ConstPoolType::CLASS_LITERAL);

        auto args = { proto, lexicalEnv, constpool,
                      builder_.ToTaggedInt(methodId), builder_.ToTaggedInt(literalId), ihclass, module };
        result = LowerCallRuntime(RTSTUB_ID(CreateClassWithIHClass), args, true);
        builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION), &isException, &isNotException);
    }

    std::vector<GateRef> successControl;
    std::vector<GateRef> exceptionControl;
    builder_.Bind(&isNotException);
    {
        builder_.SetLexicalEnvToFunction(glue_, result, lexicalEnv);
        builder_.SetModuleToFunction(glue_, result, module);
        LowerCallRuntime(RTSTUB_ID(SetClassConstructorLength),
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

void SlowPathLowering::LowerDefineFunc(GateRef gate, GateRef jsFunc)
{
    GateRef methodId = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    GateRef length = acc_.GetValueIn(gate, 1);
    auto method = builder_.GetObjectFromConstPool(glue_, jsFunc, methodId, ConstPoolType::METHOD);

    Label defaultLabel(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = LowerCallRuntime(RTSTUB_ID(DefineFunc), { method });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &defaultLabel);
    builder_.Bind(&defaultLabel);
    {
        GateRef hClass = builder_.LoadHClass(result);
        builder_.SetPropertyInlinedProps(glue_, result, hClass, builder_.ToTaggedInt(length),
            builder_.Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        GateRef env = acc_.GetValueIn(gate, 2); // 2: Get current env
        builder_.SetLexicalEnvToFunction(glue_, result, env);
        builder_.SetModuleToFunction(glue_, result, builder_.GetModuleFromFunction(jsFunc));
        builder_.SetHomeObjectToFunction(glue_, result, builder_.GetHomeObjectFromFunction(jsFunc));
        builder_.Jump(&successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerAsyncFunctionEnter(GateRef gate)
{
    const int id = RTSTUB_ID(AsyncFunctionEnter);
    // 0: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 0);
    GateRef newGate = LowerCallRuntime(id, {});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerTypeof(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    GateRef obj = acc_.GetValueIn(gate, 0);
    Label entry(&builder_);
    Label exit(&builder_);

    GateRef gConstAddr = builder_.PtrAdd(glue_,
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
    GateRef offset = builder_.PtrMul(builder_.ZExtInt32ToPtr(indexOffset),
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
    AddProfiling(gate, false);
    GateRef contextOffset = builder_.IntPtr(JSGeneratorObject::GENERATOR_CONTEXT_OFFSET);
    GateRef contextGate = builder_.Load(VariableType::JS_POINTER(), obj, contextOffset);
    GateRef arrayOffset = builder_.IntPtr(GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET);
    GateRef arrayGate = builder_.Load(VariableType::JS_POINTER(), contextGate, arrayOffset);

    for (auto item : registerGates) {
        auto index = acc_.GetVirtualRegisterIndex(item);
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
    Label isAsyncGeneratorObj(&builder_);
    Label notAsyncGeneratorObj(&builder_);
    Label exit(&builder_);
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.HoleConstant());
    builder_.Branch(builder_.TaggedIsAsyncGeneratorObject(obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    builder_.Bind(&isAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = builder_.IntPtr(JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        result = builder_.Load(VariableType::JS_ANY(), obj, resumeResultOffset);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notAsyncGeneratorObj);
    {
        GateRef resumeResultOffset = builder_.IntPtr(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
        result = builder_.Load(VariableType::JS_ANY(), obj, resumeResultOffset);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

void SlowPathLowering::LowerGetResumeMode(GateRef gate)
{
    // 1: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 1);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    DEFVAlUE(result, (&builder_), VariableType::JS_ANY(), builder_.Undefined());
    Label isAsyncGeneratorObj(&builder_);
    Label notAsyncGeneratorObj(&builder_);
    Label exit(&builder_);
    GateRef obj = acc_.GetValueIn(gate, 0);
    builder_.Branch(builder_.TaggedIsAsyncGeneratorObject(obj), &isAsyncGeneratorObj, &notAsyncGeneratorObj);
    builder_.Bind(&isAsyncGeneratorObj);
    {
        GateRef bitFieldOffset = builder_.IntPtr(JSAsyncGeneratorObject::BIT_FIELD_OFFSET);
        GateRef bitField = builder_.Load(VariableType::INT32(), obj, bitFieldOffset);
        auto bitfieldlsr = builder_.Int32LSR(bitField,
                                             builder_.Int32(JSAsyncGeneratorObject::ResumeModeBits::START_BIT));
        GateRef modeBits = builder_.Int32And(bitfieldlsr,
                                             builder_.Int32((1LU << JSAsyncGeneratorObject::ResumeModeBits::SIZE) - 1));
        auto resumeMode = builder_.SExtInt32ToInt64(modeBits);
        result = builder_.ToTaggedIntPtr(resumeMode);
        builder_.Jump(&exit);
    }
    builder_.Bind(&notAsyncGeneratorObj);
    {
        GateRef bitFieldOffset = builder_.IntPtr(JSGeneratorObject::BIT_FIELD_OFFSET);
        GateRef bitField = builder_.Load(VariableType::INT32(), obj, bitFieldOffset);
        auto bitfieldlsr = builder_.Int32LSR(bitField, builder_.Int32(JSGeneratorObject::ResumeModeBits::START_BIT));
        GateRef modeBits = builder_.Int32And(bitfieldlsr,
                                             builder_.Int32((1LU << JSGeneratorObject::ResumeModeBits::SIZE) - 1));
        auto resumeMode = builder_.SExtInt32ToInt64(modeBits);
        result = builder_.ToTaggedIntPtr(resumeMode);
        builder_.Jump(&exit);
    }
    builder_.Bind(&exit);
    successControl.emplace_back(builder_.GetState());
    successControl.emplace_back(builder_.GetDepend());
    failControl.emplace_back(Circuit::NullGate());
    failControl.emplace_back(Circuit::NullGate());
    ReplaceHirToSubCfg(gate, *result, successControl, failControl, true);
}

void SlowPathLowering::LowerDefineMethod(GateRef gate, GateRef jsFunc)
{
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef methodId = builder_.TruncInt64ToInt32(acc_.GetValueIn(gate, 0));
    auto method = builder_.GetObjectFromConstPool(glue_, jsFunc, methodId, ConstPoolType::METHOD);
    GateRef length = acc_.GetValueIn(gate, 1);
    GateRef homeObject = acc_.GetValueIn(gate, 3);  // 3: third arg

    Label defaultLabel(&builder_);
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    GateRef result = LowerCallRuntime(RTSTUB_ID(DefineMethod), {method, homeObject}, true);
    builder_.Branch(builder_.IsSpecial(result, JSTaggedValue::VALUE_EXCEPTION),
        &exceptionExit, &defaultLabel);
    builder_.Bind(&defaultLabel);
    {
        GateRef hclass = builder_.LoadHClass(result);
        builder_.SetPropertyInlinedProps(glue_, result, hclass, builder_.ToTaggedInt(length),
            builder_.Int32(JSFunction::LENGTH_INLINE_PROPERTY_INDEX), VariableType::INT64());
        GateRef env = acc_.GetValueIn(gate, 2); // 2: Get current env
        builder_.SetLexicalEnvToFunction(glue_, result, env);
        builder_.SetModuleToFunction(glue_, result, builder_.GetModuleFromFunction(jsFunc));
        builder_.Jump(&successExit);
    }
    CREATE_DOUBLE_EXIT(successExit, exceptionExit)
    ReplaceHirToSubCfg(gate, result, successControl, failControl);
}

void SlowPathLowering::LowerGetUnmappedArgs(GateRef gate, GateRef actualArgc)
{
    GateRef newGate = builder_.CallStub(glue_, CommonStubCSigns::GetUnmapedArgs,
        { glue_, builder_.TruncInt64ToInt32(actualArgc) });
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerCopyRestArgs(GateRef gate, GateRef actualArgc)
{
    GateRef taggedArgc = builder_.ToTaggedInt(actualArgc);
    GateRef restIdx = acc_.GetValueIn(gate, 0);
    GateRef taggedRestIdx = builder_.ToTaggedInt(restIdx);

    const int id = RTSTUB_ID(OptCopyRestArgs);
    GateRef newGate = LowerCallRuntime(id, {taggedArgc, taggedRestIdx});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerWideLdPatchVar(GateRef gate)
{
    const int id = RTSTUB_ID(LdPatchVar);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef newGate = LowerCallRuntime(id, {builder_.ToTaggedInt(index)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::LowerWideStPatchVar(GateRef gate)
{
    const int id = RTSTUB_ID(StPatchVar);
    GateRef index = acc_.GetValueIn(gate, 0);
    GateRef newGate = LowerCallRuntime(id, {builder_.ToTaggedInt(index)});
    ReplaceHirToCall(gate, newGate);
}

void SlowPathLowering::AddProfiling(GateRef gate, bool skipGenerator)
{
    if (IsTraceBC()) {
        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        if ((ecmaOpcode == EcmaOpcode::SUSPENDGENERATOR_V8 || ecmaOpcode == EcmaOpcode::RESUMEGENERATOR) &&
            skipGenerator) {
            return;
        }
        auto ecmaOpcodeGate = builder_.Int32(static_cast<uint32_t>(ecmaOpcode));
        GateRef constOpcode = builder_.ToTaggedInt(builder_.ZExtInt32ToInt64(ecmaOpcodeGate));
        GateRef debugGate = builder_.CallRuntime(glue_, RTSTUB_ID(DebugAOTPrint), acc_.GetDep(gate), {constOpcode});
        acc_.SetDep(gate, debugGate);
    }

    if (IsProfiling()) {
        EcmaOpcode ecmaOpcode = acc_.GetByteCodeOpcode(gate);
        if ((ecmaOpcode == EcmaOpcode::SUSPENDGENERATOR_V8 || ecmaOpcode == EcmaOpcode::RESUMEGENERATOR) &&
            skipGenerator) {
            return;
        }
        auto ecmaOpcodeGate = builder_.Int32(static_cast<uint32_t>(ecmaOpcode));
        GateRef constOpcode = builder_.Int32ToTaggedInt(ecmaOpcodeGate);
        GateRef mode =
            builder_.Int32ToTaggedInt(builder_.Int32(static_cast<int32_t>(OptCodeProfiler::Mode::SLOW_PATH)));
        GateRef profiling = builder_.CallRuntime(glue_, RTSTUB_ID(ProfileOptimizedCode), acc_.GetDep(gate),
                                                 {constOpcode, mode});
        acc_.SetDep(gate, profiling);
    }
}

void SlowPathLowering::LowerCallthis0Imm8V8(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);

    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLTHIS0_IMM8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef func = acc_.GetValueIn(gate, 1);
    GateRef env = builder_.Undefined();
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    LowerToJSCall(gate, {glue_, env, actualArgc, func, newTarget, thisObj, bcOffset});
}

void SlowPathLowering::LowerCallArg1Imm8V8(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 2);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLARG1_IMM8_V8));

    GateRef newTarget = builder_.Undefined();
    GateRef a0Value = acc_.GetValueIn(gate, 0);
    GateRef thisObj = builder_.Undefined();
    GateRef func = acc_.GetValueIn(gate, 1); // acc
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    GateRef env = builder_.Undefined();
    LowerToJSCall(gate, {glue_, env, actualArgc, func, newTarget, thisObj, a0Value, bcOffset});
}

void SlowPathLowering::LowerWideCallrangePrefImm16V8(GateRef gate)
{
    std::vector<GateRef> vec;
    size_t numIns = acc_.GetNumValueIn(gate);
    size_t fixedInputsNum = 1; // 1: acc
    ASSERT(acc_.GetNumValueIn(gate) >= fixedInputsNum);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8));
    GateRef callTarget = acc_.GetValueIn(gate, numIns - fixedInputsNum); // acc
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef env = builder_.Undefined();

    vec.emplace_back(glue_);
    vec.emplace_back(env);
    vec.emplace_back(actualArgc);
    vec.emplace_back(callTarget);
    vec.emplace_back(newTarget);
    vec.emplace_back(thisObj);
    // add args
    for (size_t i = 0; i < numIns - fixedInputsNum; i++) { // skip acc
        vec.emplace_back(acc_.GetValueIn(gate, i));
    }
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    vec.emplace_back(bcOffset);
    LowerToJSCall(gate, vec);
}

void SlowPathLowering::LowerCallThisArg1(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLTHIS1_IMM8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef a0 = acc_.GetValueIn(gate, 1); // 1:first parameter
    GateRef func = acc_.GetValueIn(gate, 2); // 2:function
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    GateRef env = builder_.Undefined();
    LowerToJSCall(gate, {glue_, env, actualArgc, func, newTarget, thisObj, a0, bcOffset});
}

void SlowPathLowering::LowerCallargs2Imm8V8V8(GateRef gate)
{
    // 3: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 3);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLARGS2_IMM8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef a0 = acc_.GetValueIn(gate, 0);
    GateRef a1 = acc_.GetValueIn(gate, 1); // 1:first parameter
    GateRef func = acc_.GetValueIn(gate, 2); // 2:function
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    GateRef env = builder_.Undefined();

    LowerToJSCall(gate, {glue_, env, actualArgc, func, newTarget, thisObj, a0,
        a1, bcOffset});
}

void SlowPathLowering::LowerCallargs3Imm8V8V8(GateRef gate)
{
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = builder_.Undefined();
    GateRef a0 = acc_.GetValueIn(gate, 0);
    GateRef a1 = acc_.GetValueIn(gate, 1);
    GateRef a2 = acc_.GetValueIn(gate, 2);
    GateRef func = acc_.GetValueIn(gate, 3);
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    GateRef env = builder_.Undefined();

    LowerToJSCall(gate, {glue_, env, actualArgc, func, newTarget, thisObj, a0,
        a1, a2, bcOffset});
}

void SlowPathLowering::LowerCallthis2Imm8V8V8V8(GateRef gate)
{
    // 4: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 4);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef a0Value = acc_.GetValueIn(gate, 1);
    GateRef a1Value = acc_.GetValueIn(gate, 2);
    GateRef func = acc_.GetValueIn(gate, 3);  //acc
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    GateRef env = builder_.Undefined();

    LowerToJSCall(gate, {glue_, env, actualArgc, func, newTarget, thisObj, a0Value,
        a1Value, bcOffset});
}

void SlowPathLowering::LowerCallthis3Imm8V8V8V8V8(GateRef gate)
{
    // 5: number of value inputs
    ASSERT(acc_.GetNumValueIn(gate) == 5);
    GateRef actualArgc = builder_.Int64(ComputeCallArgc(gate, EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8));
    GateRef newTarget = builder_.Undefined();
    GateRef thisObj = acc_.GetValueIn(gate, 0);
    GateRef a0Value = acc_.GetValueIn(gate, 1);
    GateRef a1Value = acc_.GetValueIn(gate, 2);
    GateRef a2Value = acc_.GetValueIn(gate, 3);
    GateRef func = acc_.GetValueIn(gate, 4);
    GateRef env = builder_.Undefined();
    GateRef bcOffset = builder_.Int64(acc_.GetBytecodeIndex(gate));
    LowerToJSCall(gate, {glue_, env, actualArgc, func, newTarget, thisObj, a0Value,
        a1Value, a2Value, bcOffset});
}

void SlowPathLowering::LowerLdThisByName(GateRef gate, GateRef jsFunc)
{
    Label successExit(&builder_);
    Label exceptionExit(&builder_);
    Label updateProfileTypeInfo(&builder_);
    Label accessObject(&builder_);
    std::vector<GateRef> successControl;
    std::vector<GateRef> failControl;
    ASSERT(acc_.GetNumValueIn(gate) == 3);  // 2: number of parameter
    GateRef thisObj = acc_.GetValueIn(gate, 2); // 2: this object
    GateRef slotId = builder_.ZExtInt16ToInt32(acc_.GetValueIn(gate, 0));
    GateRef prop = acc_.GetValueIn(gate, 1);  // 1: the second parameter
    DEFVAlUE(profileTypeInfo, (&builder_), VariableType::JS_ANY(), GetProfileTypeInfo(jsFunc));
    builder_.Branch(builder_.TaggedIsUndefined(*profileTypeInfo), &updateProfileTypeInfo, &accessObject);
    builder_.Bind(&updateProfileTypeInfo);
    {
        profileTypeInfo = LowerCallRuntime(RTSTUB_ID(UpdateHotnessCounter), { jsFunc }, true);
        builder_.Jump(&accessObject);
    }
    builder_.Bind(&accessObject);
    GateRef result = builder_.CallStub(glue_, CommonStubCSigns::GetPropertyByName,
        { glue_, thisObj, prop, *profileTypeInfo, slotId });
    builder_.Branch(builder_.TaggedIsException(result), &exceptionExit, &successExit);
    builder_.Bind(&successExit);
    {
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

void SlowPathLowering::LowerConstPoolData(GateRef gate)
{
    Environment env(0, &builder_);
    GateRef jsFunc = argAcc_.GetCommonArgGate(CommonArgIdx::FUNC);
    ConstDataId dataId = acc_.GetConstDataId(gate);
    auto newGate = LoadObjectFromConstPool(jsFunc, builder_.Int32(dataId.GetId()));
    // replace newGate
    auto uses = acc_.Uses(gate);
    for (auto it = uses.begin(); it != uses.end();) {
        it = acc_.ReplaceIn(it, newGate);
    }

    // delete old gate
    acc_.DeleteGate(gate);
}

void SlowPathLowering::LowerDeoptCheck(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef condition = acc_.GetValueIn(gate, 0);
    GateRef frameState = acc_.GetValueIn(gate, 1);
    GateRef deoptType = acc_.GetValueIn(gate, 2);

    Label success(&builder_);
    Label fail(&builder_);
    builder_.Branch(condition, &success, &fail);
    builder_.Bind(&fail);
    {
        GateRef glue = acc_.GetGlueFromArgList();
        GateRef deoptCall = circuit_->NewGate(circuit_->Deopt(), {builder_.GetDepend(), frameState, glue, deoptType});
        builder_.SetDepend(deoptCall);
        builder_.Return(deoptCall);
    }
    builder_.Bind(&success);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void SlowPathLowering::LowerConstruct(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(JSCallNew));
    GateRef target = builder_.IntPtr(RTSTUB_ID(JSCallNew));
    size_t num = acc_.GetNumValueIn(gate);
    std::vector<GateRef> args(num);
    for (size_t i = 0; i < num; ++i) {
        args[i] = acc_.GetValueIn(gate, i);
    }
    auto depend = builder_.GetDepend();
    GateRef constructGate = builder_.Call(cs, glue_, target, depend, args);
    GateRef ctor = acc_.GetValueIn(gate, static_cast<size_t>(CommonArgIdx::FUNC));
    GateRef thisObj = acc_.GetValueIn(gate, static_cast<size_t>(CommonArgIdx::THIS_OBJECT));
    GateRef result = builder_.CallStub(
        glue_, CommonStubCSigns::ConstructorCheck, { glue_, ctor, constructGate, thisObj });
    ReplaceHirToJSCall(gate, result);
}

void SlowPathLowering::LowerUpdateHotness(GateRef gate)
{
    Environment env(gate, circuit_, &builder_);
    GateRef jsFunc = argAcc_.GetCommonArgGate(CommonArgIdx::FUNC);
    GateRef method = builder_.Load(VariableType::JS_ANY(), jsFunc, builder_.IntPtr(JSFunctionBase::METHOD_OFFSET));
    GateRef hotness = builder_.Load(VariableType::INT16(), method, builder_.IntPtr(Method::LITERAL_INFO_OFFSET));
    GateRef value = builder_.ZExtInt16ToInt32(hotness);
    GateRef offset = acc_.GetValueIn(gate, 0);
    GateRef newValue = builder_.Int32Add(value, offset);
    DEFVAlUE(newHotness, (&builder_), VariableType::INT16(), builder_.TruncInt32ToInt16(newValue));
    Label slowPath(&builder_);
    Label dispatch(&builder_);
    builder_.Branch(builder_.Int32LessThan(newValue, builder_.Int32(0)), &slowPath, &dispatch);
    builder_.Bind(&slowPath);
    {
        builder_.CallRuntime(glue_, RTSTUB_ID(UpdateHotnessCounter), Circuit::NullGate(), { jsFunc });
        newHotness = builder_.Int16(EcmaInterpreter::METHOD_HOTNESS_THRESHOLD);
        builder_.Jump(&dispatch);
    }
    builder_.Bind(&dispatch);
    builder_.Store(VariableType::VOID(), glue_, method, builder_.IntPtr(Method::LITERAL_INFO_OFFSET), *newHotness);
    acc_.ReplaceGate(gate, builder_.GetState(), builder_.GetDepend(), Circuit::NullGate());
}

void SlowPathLowering::LowerNotifyConcurrentResult(GateRef gate)
{
    const int id = RTSTUB_ID(NotifyConcurrentResult);

    GateRef newGate = LowerCallRuntime(id, {acc_.GetValueIn(gate, 0), acc_.GetValueIn(gate, 1)});
    ReplaceHirToCall(gate, newGate);
}
}  // namespace panda::ecmascript
