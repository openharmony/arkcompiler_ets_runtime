/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/arksteed/arksteed_graph_builder.h"

#include "ecmascript/arksteed/arksteed_bytecode_analysis.h"
#include "ecmascript/arksteed/arksteed_bytecode_analysis_new.h"
#include "ecmascript/arksteed/arksteed_bytecode_iterator.h"
#include "ecmascript/arksteed/arksteed_bytecode_preprocessor_new.h"
#include "ecmascript/arksteed/arksteed_opcode.h"
#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/global_env.h"

namespace panda::ecmascript::arksteed {
namespace kungfu = panda::ecmascript::kungfu;

namespace {
VRegIDType CheckedSubGraphVariableCount(int variableCount)
{
    ASSERT(variableCount >= 0);
    return static_cast<VRegIDType>(variableCount);
}

const char *ValueRepresentationName(ValueRepresentation repr)
{
    switch (repr) {
        case ValueRepresentation::TAGGED:
            return "tagged";
        case ValueRepresentation::INT32:
            return "int32";
        case ValueRepresentation::UINT32:
            return "uint32";
        case ValueRepresentation::FLOAT64:
            return "float64";
        case ValueRepresentation::HOLEY_FLOAT64:
            return "holey_float64";
        case ValueRepresentation::INT_PTR:
            return "intptr";
        case ValueRepresentation::NONE:
            return "none";
    }
    return "unknown";
}

const char *MachineTypeName(kungfu::MachineType machineType)
{
    switch (machineType) {
        case kungfu::MachineType::NOVALUE:
            return "novalue";
        case kungfu::MachineType::ANYVALUE:
            return "anyvalue";
        case kungfu::MachineType::ARCH:
            return "arch";
        case kungfu::MachineType::FLEX:
            return "flex";
        case kungfu::MachineType::I1:
            return "i1";
        case kungfu::MachineType::I8:
            return "i8";
        case kungfu::MachineType::I16:
            return "i16";
        case kungfu::MachineType::I32:
            return "i32";
        case kungfu::MachineType::I64:
            return "i64";
        case kungfu::MachineType::F32:
            return "f32";
        case kungfu::MachineType::F64:
            return "f64";
    }
    return "unknown";
}

bool IsTaggedCallType(kungfu::VariableType type)
{
    kungfu::GateType gateType = type.GetGateType();
    return gateType == kungfu::GateType::TaggedValue() || gateType == kungfu::GateType::TaggedPointer() ||
           gateType == kungfu::GateType::TaggedNPointer();
}

bool MatchesCallSignatureType(const ValueVertex *value, kungfu::VariableType type)
{
    if (IsTaggedCallType(type)) {
        return value->IsTagged();
    }

    switch (type.GetMachineType()) {
        case kungfu::MachineType::NOVALUE:
        case kungfu::MachineType::ANYVALUE:
        case kungfu::MachineType::FLEX:
            return true;
        case kungfu::MachineType::ARCH:
        case kungfu::MachineType::I64:
            return value->IsIntPtr();
        case kungfu::MachineType::I1:
        case kungfu::MachineType::I8:
        case kungfu::MachineType::I16:
        case kungfu::MachineType::I32:
            return value->IsInt32() || value->IsUint32() || value->IsIntPtr();
        case kungfu::MachineType::F32:
        case kungfu::MachineType::F64:
            return value->IsAnyFloat64();
    }
    return true;
}
}  // namespace

const LivenessBitSet *ArkSteedGraphBuilder::GetInLivenessFor(uint32_t index) const
{
    ASSERT(bytecodeAnalysis_ != nullptr);
    return &bytecodeAnalysis_->GetInLiveness(index);
}

bool ArkSteedGraphBuilder::Build()
{
    InitializeGraph();
    // Start basic block
    startBlock_ = BB::New(GetChunk());
    SetCurrentBlock(startBlock_);
    InitializeGlobalsAndParameters();
    InitializeCurrentFrameState();
    BuildMergeStates();
    if (options_.printMethodName) {
        ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
        NewVertex<CallRuntimeVertex>({jsFunc}, RTSTUB_ID(PrintMethodName));
    }
    // Finishes the basic block
    MergeCurrentFrameStateTo(FinishBlock<JumpVertex>({}, &jumpTargets_[entryPoint_]), entryPoint_);
    BuildBody();
#ifndef NDEBUG
    ValidateAfterBuilding();
#endif
    return true;
}

void ArkSteedGraphBuilder::InitializeGraph()
{
    method_ = env_->GetMethodLiteral();

    const char *recordName = method_->GetRecordNameWithSymbol(env_->GetJSPandaFile(), method_->GetMethodId());
    const char *methodName = method_->GetMethodName(env_->GetJSPandaFile(), method_->GetMethodId());
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Starts compiling " << recordName << " :: " << methodName;
#endif

    options_ = ArkSteedCompilationOptions(env_->GetJSOptions());
    bytecodeContext_.Initialize(method_, env_, &options_);

    iterator_ = BytecodeIterator(&bytecodeContext_);
    jumpTargets_.resize(bytecodeContext_.GetBytecodeCount());

    numParams_ = method_->GetNumArgsForArkSteed();
    numLocal_ = method_->GetNumVregsWithCallField();

    bytecodeAnalysis_ = GetChunk()->New<BytecodeAnalysis>(GetChunk(), bytecodeContext_, numLocal_, numParams_);
    bytecodeAnalysis_->AnalyzeLivenessAndAssignments(&bytecodeContext_, &options_);
}

void ArkSteedGraphBuilder::InitializeGlobalsAndParameters()
{
    ASSERT(glueAddr_ != 0);
    glue_ = GetIntPtrConstant(static_cast<intptr_t>(glueAddr_));

    // caller argument area (in fp-slot words): +2 argc, +3 call-target, +4 new-target, +5 this, +6... user args.
    const int32_t kCallTargetFpSlotIndex = 3;

    for (uint32_t i = 0; i < numParams_; i++) {
        ValueVertex *v = NewVertexNoInput<InitialValueVertex>(static_cast<int32_t>(i + kCallTargetFpSlotIndex));
        GetGraph()->AddParameter(v);
    }
}

// Precondition: Param vertices have been added to the graph.
void ArkSteedGraphBuilder::InitializeCurrentFrameState()
{
    currentFrameState_ = GetChunk()->New<InterpreterFrameState>(numLocal_, numParams_, GetChunk());
    for (VRegIDType i = 0; i < numParams_; i++) {
        currentFrameState_->SetParam(i, GetGraph()->GetParameter(i));
    }
    ValueVertex *undefinedValue = GetRootConstant(RootConstantVertex::RootIndex::UNDEFINED);
    for (VRegIDType i = 0; i < numLocal_; i++) {
        currentFrameState_->SetLocal(i, undefinedValue);
    }
    // Fixed header lexicalEnv is at [fp - 24], i.e. slot -3 in word units.
    currentFrameState_->SetEnv(NewVertexNoInput<InitialValueVertex>(-3));
    currentFrameState_->SetAcc(undefinedValue);
}

ValueVertex *ArkSteedGraphBuilder::NewCallStubWithIC(const CommonStubCSigns::ID stubId,
                                                     const std::vector<ValueVertex *> &args)
{
    std::vector<ValueVertex *> allArgs;
    allArgs.reserve(args.size() + 3);  // 3: glue + jsFunc + slotId
    allArgs.push_back(GetGlue());
    allArgs.insert(allArgs.end(), args.begin(), args.end());
    allArgs.push_back(currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX));
    allArgs.push_back(GetInt32Constant(static_cast<int>(GetICSlotId(0))));

    ValidateCommonStubCallArgs(stubId, allArgs);
    return NewVertex<CallCommonStubVertex>(allArgs, stubId);
}

void ArkSteedGraphBuilder::LowerCallStubWithIC(const CommonStubCSigns::ID stubId,
                                               const std::vector<ValueVertex *> &args)
{
    currentFrameState_->SetAcc(NewCallStubWithIC(stubId, args));
}

void ArkSteedGraphBuilder::LowerCallStubWithICPreserveAcc(const CommonStubCSigns::ID stubId,
                                                          const std::vector<ValueVertex *> &args)
{
    NewCallStubWithIC(stubId, args);
}

ValueVertex *ArkSteedGraphBuilder::NewCommonStubCall(std::initializer_list<ValueVertex *> args,
                                                     const CommonStubCSigns::ID stubId)
{
    std::vector<ValueVertex *> allArgs(args);
    ValidateCommonStubCallArgs(stubId, allArgs);
    return NewVertex<CallCommonStubVertex>(allArgs, stubId);
}

void ArkSteedGraphBuilder::ValidateCommonStubCallArgs(const CommonStubCSigns::ID stubId,
                                                      const std::vector<ValueVertex *> &args) const
{
    const CallSignature *signature = CommonStubCSigns::Get(stubId);
    size_t actualCount = args.size();
    size_t expectedCount = signature->GetParametersCount();
    if (actualCount != expectedCount) {
        LOG_ECMA(FATAL) << "ArkSteed CommonStub argument count mismatch, stub: " << signature->GetName()
                        << ", expected: " << expectedCount << ", actual: " << actualCount;
        UNREACHABLE();
    }

    kungfu::VariableType *params = signature->GetParametersType();
    if (params == nullptr) {
        return;
    }
    for (size_t i = 0; i < expectedCount; ++i) {
        if (!MatchesCallSignatureType(args[i], params[i])) {
            LOG_ECMA(FATAL) << "ArkSteed CommonStub argument type mismatch, stub: " << signature->GetName()
                            << ", index: " << i
                            << ", expected machine type: " << MachineTypeName(params[i].GetMachineType())
                            << ", actual representation: "
                            << ValueRepresentationName(args[i]->GetValueRepresentation());
            UNREACHABLE();
        }
    }
}

void ArkSteedGraphBuilder::BuildMergeStates()
{
    auto &jumpLoop = bytecodeContext_.GetJumpLoop();
    size_t n = bytecodeContext_.GetBytecodeCount();

    mergeStates_.assign(n, nullptr);
    predecessorCountReductions_.assign(n, 0);
    for (iterator_.GotoStart(); !iterator_.Done(); ++iterator_) {
        auto curIndex = iterator_.Index();
        if (!jumpLoop[curIndex]) {
            continue;
        }
        auto targetIndex = iterator_.GetJumpTargetBcIndex();
        ASSERT(mergeStates_[targetIndex] == nullptr && "Unexpected assignment before.");
        mergeStates_[targetIndex] =
            MergePointFrameState::NewForLoop(targetIndex,
                                             bytecodeContext_.GetPredecessorCount()[targetIndex],
                                             &bytecodeAnalysis_->GetInLiveness(targetIndex),
                                             &bytecodeAnalysis_->GetLoopInfo(targetIndex),
                                             chunk_);
    }
}

void ArkSteedGraphBuilder::BuildBody()
{
    auto bytecodeCount = bytecodeContext_.GetBytecodeCount();
    for (iterator_.GotoStart(); !iterator_.Done(); ++iterator_) {
        uint32_t index = iterator_.Index();
        auto bytecodeInfo = iterator_.GetCurrentBytecodeInfo();

        if (mergeStates_[index] != nullptr) {
            if (mergeStates_[index]->PredecessorCount() == 0) {
                ASSERT(CurrentBlock() == nullptr);
                MarkDeadPredecessorsForSuccessors(index, bytecodeInfo);
                continue;
            }
            if (CurrentBlock() != nullptr) {
                // Previous basic block was NOT finished (fallthrough).
                MergeCurrentFrameStateTo(FinishBlock<JumpVertex>({}, &jumpTargets_[index]), index);
            }
            if (mergeStates_[index]->IsUnmergedUnreachableLoop()) {
                ASSERT(CurrentBlock() == nullptr);
                MarkDeadPredecessorsForSuccessors(index, bytecodeInfo);
                continue;
            }
            StartNewBlockWithMergeState(index);
        } else {
            if (CurrentBlock() == nullptr) {
                MarkDeadPredecessorsForSuccessors(index, bytecodeInfo);
                continue;
            }
        }
        ProcessBytecode();
        if (bytecodeInfo.needFallThrough()) {
            uint32_t fallThroughIndex = iterator_.NextIndex();
            if (fallThroughIndex >= bytecodeCount) {
                LOG_COMPILER(WARN) << "Malformed bytecode #" << index << ": the last bytecode goes fallthrough.";
                continue;
            }
            uint32_t nextRPOIndex = iterator_.NextRPOIndex();
            if (fallThroughIndex != nextRPOIndex) {
                BB *block = FinishBlock<JumpVertex>({}, &jumpTargets_[fallThroughIndex]);
                MergeCurrentFrameStateTo(block, fallThroughIndex);
            }
        }
    }
    // The last bytecode shall finish the last basic block.
    ASSERT(currentBlock_ == nullptr);
}

void ArkSteedGraphBuilder::ValidateAfterBuilding()
{
    for (iterator_.GotoStart(); !iterator_.Done(); ++iterator_) {
        uint32_t index = iterator_.Index();
        MergePointFrameState *mergeState = mergeStates_[index];
        if (mergeState == nullptr) {
            continue;
        }
        ValidateMergeStateAfterBuilding(index, mergeState);
    }
}

void ArkSteedGraphBuilder::ValidateMergeStateAfterBuilding(uint32_t index, MergePointFrameState *mergeState)
{
    if (mergeState->PredecessorCount() == 0) {
        if (mergeState->PredecessorsSoFar() != 0) {
            LOG_COMPILER(FATAL) << "INVALID merge state for bytecode #" << index << ": unreachable merge point has "
                                << mergeState->PredecessorsSoFar() << " predecessors merged.";
        }
        return;
    }
    if (mergeState->PredecessorsSoFar() != mergeState->PredecessorCount()) {
        LOG_COMPILER(FATAL) << "INVALID merge state for bytecode #" << index << ": PredecessorsSoFar() which is "
                            << mergeState->PredecessorsSoFar() << " does not match PredecessorCount() which is "
                            << mergeState->PredecessorCount();
    }
    LOG_COMPILER(DEBUG) << "OK: merge state for bytecode #" << index;
}

ArkSteedGraphBuilder::BranchResult ArkSteedGraphBuilder::BuildBranchIfTrue(BranchBuilder &builder, ValueVertex *vertex)
{
    auto emitUnconditionalBytecodeBranch = [this, &builder](bool conditionTrue) {
        bool takeJumpTarget =
            builder.GetCurrentBranchType() == BranchType::TRUE_BRANCH ? conditionTrue : !conditionTrue;
        ASSERT(builder.GetMode() == BranchBuilder::JUMP_BYTECODE_TARGET);

        uint32_t destIndex = takeJumpTarget ? builder.JumpTargetBcIndex() : builder.FallthroughBcIndex();
        uint32_t trimmedIndex = takeJumpTarget ? builder.FallthroughBcIndex() : builder.JumpTargetBcIndex();
        if (trimmedIndex != destIndex) {
            ReduceBytecodePredecessorCount(trimmedIndex);
        }
        BB *block = FinishBlock<JumpVertex>({}, &jumpTargets_[destIndex]);
        MergeCurrentFrameStateTo(block, destIndex);
    };

    if (RootConstantVertex *root = vertex->TryCast<RootConstantVertex>()) {
        switch (root->GetIndex()) {
            case RootConstantVertex::RootIndex::TRUE_VALUE:
                if (builder.GetMode() == BranchBuilder::JUMP_LABEL_TARGET) {
                    return BranchResult::ALWAYS_TRUE;
                }
                emitUnconditionalBytecodeBranch(true);
                return BranchResult::ALWAYS_TRUE;
            case RootConstantVertex::RootIndex::FALSE_VALUE:
            case RootConstantVertex::RootIndex::NULL_VALUE:
            case RootConstantVertex::RootIndex::UNDEFINED:
                if (builder.GetMode() == BranchBuilder::JUMP_LABEL_TARGET) {
                    return BranchResult::ALWAYS_FALSE;
                }
                emitUnconditionalBytecodeBranch(false);
                return BranchResult::ALWAYS_FALSE;
            default:
                break;
        }
    }

    builder.Build<BranchIfTrueVertex>({vertex});
    return BranchResult::DEFAULT;
}

void ArkSteedGraphBuilder::ProcessBytecode()
{
    auto info = iterator_.GetCurrentBytecodeInfo();
    auto opcode = info.GetOpcode();
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Processing bytecode #" << iterator_.Index() << ": " << GetEcmaOpcodeStr(opcode);
#endif

    // All unsupported opcodes should be filtered out by IsArkSteedSupportedBytecode
    ASSERT(kungfu::IsArkSteedSupportedOpcode(opcode));

    switch (opcode) {
        // Nop: Nothing to do
        case kungfu::EcmaOpcode::NOP:
            break;
        // Call Instructions
        case kungfu::EcmaOpcode::CALLARG0_IMM8:
            LowerCallArg0();
            break;
        case kungfu::EcmaOpcode::CALLARG1_IMM8_V8:
            LowerCallArg1();
            break;
        case kungfu::EcmaOpcode::CALLARGS2_IMM8_V8_V8:
            LowerCallArgs2();
            break;
        case kungfu::EcmaOpcode::CALLARGS3_IMM8_V8_V8_V8:
            LowerCallArgs3();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_CALLINIT_PREF_IMM8_V8:
            LowerCallThis0();
            break;
        case kungfu::EcmaOpcode::CALLTHIS0_IMM8_V8:
            LowerCallThis0();
            break;
        case kungfu::EcmaOpcode::CALLTHIS1_IMM8_V8_V8:
            LowerCallThis1();
            break;
        case kungfu::EcmaOpcode::CALLTHIS2_IMM8_V8_V8_V8:
            LowerCallThis2();
            break;
        case kungfu::EcmaOpcode::CALLTHIS3_IMM8_V8_V8_V8_V8:
            LowerCallThis3();
            break;
        case kungfu::EcmaOpcode::CALLRANGE_IMM8_IMM8_V8:
        case kungfu::EcmaOpcode::WIDE_CALLRANGE_PREF_IMM16_V8:
            LowerCallRange();
            break;
        case kungfu::EcmaOpcode::CALLTHISRANGE_IMM8_IMM8_V8:
        case kungfu::EcmaOpcode::WIDE_CALLTHISRANGE_PREF_IMM16_V8:
            LowerCallThisRange();
            break;
        case kungfu::EcmaOpcode::APPLY_IMM8_V8_V8:
            LowerCallSpread();
            break;
        case kungfu::EcmaOpcode::GETUNMAPPEDARGS:
            LowerGetUnmappedArgs();
            break;
        case kungfu::EcmaOpcode::INC_IMM8:
            LowerInc();
            break;
        case kungfu::EcmaOpcode::DEC_IMM8:
            LowerDec();
            break;
        case kungfu::EcmaOpcode::GETPROPITERATOR:
            LowerGetPropIterator();
            break;
        case kungfu::EcmaOpcode::CLOSEITERATOR_IMM8_V8:
        case kungfu::EcmaOpcode::CLOSEITERATOR_IMM16_V8:
            LowerCloseIterator();
            break;
        case kungfu::EcmaOpcode::ADD2_IMM8_V8:
            LowerAdd2();
            break;
        case kungfu::EcmaOpcode::SUB2_IMM8_V8:
            LowerSub2();
            break;
        case kungfu::EcmaOpcode::MUL2_IMM8_V8:
            LowerMul2();
            break;
        case kungfu::EcmaOpcode::DIV2_IMM8_V8:
            LowerDiv2();
            break;
        case kungfu::EcmaOpcode::MOD2_IMM8_V8:
            LowerMod2();
            break;
        case kungfu::EcmaOpcode::EQ_IMM8_V8:
            LowerEq();
            break;
        case kungfu::EcmaOpcode::NOTEQ_IMM8_V8:
            LowerNotEq();
            break;
        case kungfu::EcmaOpcode::LESS_IMM8_V8:
            LowerLess();
            break;
        case kungfu::EcmaOpcode::LESSEQ_IMM8_V8:
            LowerLessEq();
            break;
        case kungfu::EcmaOpcode::GREATER_IMM8_V8:
            LowerGreater();
            break;
        case kungfu::EcmaOpcode::GREATEREQ_IMM8_V8:
            LowerGreaterEq();
            break;
        case kungfu::EcmaOpcode::CREATEITERRESULTOBJ_V8_V8:
            LowerCreateIterResultObj();
            break;
        case kungfu::EcmaOpcode::TRYLDGLOBALBYNAME_IMM8_ID16:
        case kungfu::EcmaOpcode::TRYLDGLOBALBYNAME_IMM16_ID16:
            LowerTryLdGlobalByName();
            break;
        case kungfu::EcmaOpcode::STGLOBALVAR_IMM16_ID16:
            LowerStGlobalVar();
            break;
        case kungfu::EcmaOpcode::GETITERATOR_IMM8:
        case kungfu::EcmaOpcode::GETITERATOR_IMM16:
            LowerGetIterator();
            break;
        case kungfu::EcmaOpcode::NEWOBJAPPLY_IMM8_V8:
        case kungfu::EcmaOpcode::NEWOBJAPPLY_IMM16_V8:
            LowerNewObjApply();
            break;
        case kungfu::EcmaOpcode::THROW_PREF_NONE:
            LowerThrow();
            break;
        case kungfu::EcmaOpcode::TYPEOF_IMM8:
        case kungfu::EcmaOpcode::TYPEOF_IMM16:
            LowerTypeOf();
            break;
        case kungfu::EcmaOpcode::THROW_CONSTASSIGNMENT_PREF_V8:
            LowerThrowConstAssignment();
            break;
        case kungfu::EcmaOpcode::THROW_NOTEXISTS_PREF_NONE:
            LowerThrowNotExists();
            break;
        case kungfu::EcmaOpcode::THROW_PATTERNNONCOERCIBLE_PREF_NONE:
            LowerThrowPatternNonCoercible();
            break;
        case kungfu::EcmaOpcode::THROW_IFNOTOBJECT_PREF_V8:
            LowerThrowIfNotObject();
            break;
        case kungfu::EcmaOpcode::THROW_UNDEFINEDIFHOLE_PREF_V8_V8:
            LowerThrowUndefinedIfHole();
            break;
        case kungfu::EcmaOpcode::THROW_UNDEFINEDIFHOLEWITHNAME_PREF_ID16:
            LowerThrowUndefinedIfHoleWithName();
            break;
        case kungfu::EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM8:
        case kungfu::EcmaOpcode::THROW_IFSUPERNOTCORRECTCALL_PREF_IMM16:
            LowerThrowIfSuperNotCorrectCall();
            break;
        case kungfu::EcmaOpcode::THROW_DELETESUPERPROPERTY_PREF_NONE:
            LowerThrowDeleteSuperProperty();
            break;
        case kungfu::EcmaOpcode::LDSYMBOL:
            LowerLdSymbol();
            break;
        case kungfu::EcmaOpcode::LDGLOBAL:
            LowerLdGlobal();
            break;
        case kungfu::EcmaOpcode::TONUMBER_IMM8:
            LowerToNumber();
            break;
        case kungfu::EcmaOpcode::NEG_IMM8:
            LowerNeg();
            break;
        case kungfu::EcmaOpcode::NOT_IMM8:
            LowerNot();
            break;
        case kungfu::EcmaOpcode::SHL2_IMM8_V8:
            LowerShl2();
            break;
        case kungfu::EcmaOpcode::SHR2_IMM8_V8:
            LowerShr2();
            break;
        case kungfu::EcmaOpcode::ASHR2_IMM8_V8:
            LowerAshr2();
            break;
        case kungfu::EcmaOpcode::AND2_IMM8_V8:
            LowerAnd2();
            break;
        case kungfu::EcmaOpcode::OR2_IMM8_V8:
            LowerOr2();
            break;
        case kungfu::EcmaOpcode::XOR2_IMM8_V8:
            LowerXor2();
            break;
        case kungfu::EcmaOpcode::DELOBJPROP_V8:
            LowerDelObjProp();
            break;
        case kungfu::EcmaOpcode::DEFINEMETHOD_IMM8_ID16_IMM8:
        case kungfu::EcmaOpcode::DEFINEMETHOD_IMM16_ID16_IMM8:
            LowerDefineMethod();
            break;
        case kungfu::EcmaOpcode::EXP_IMM8_V8:
            LowerExp();
            break;
        case kungfu::EcmaOpcode::ISIN_IMM8_V8:
            LowerIsIn();
            break;
        case kungfu::EcmaOpcode::INSTANCEOF_IMM8_V8:
            LowerInstanceOf();
            break;
        case kungfu::EcmaOpcode::STRICTNOTEQ_IMM8_V8:
            LowerStrictNotEq();
            break;
        case kungfu::EcmaOpcode::STRICTEQ_IMM8_V8:
            LowerStrictEq();
            break;
        case kungfu::EcmaOpcode::CREATEEMPTYARRAY_IMM8:
        case kungfu::EcmaOpcode::CREATEEMPTYARRAY_IMM16:
            LowerCreateEmptyArray();
            break;
        case kungfu::EcmaOpcode::CREATEEMPTYOBJECT:
            LowerCreateEmptyObject();
            break;
        case kungfu::EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM8_ID16:
        case kungfu::EcmaOpcode::CREATEOBJECTWITHBUFFER_IMM16_ID16:
            LowerCreateObjectWithBuffer();
            break;
        case kungfu::EcmaOpcode::CREATEARRAYWITHBUFFER_IMM8_ID16:
        case kungfu::EcmaOpcode::CREATEARRAYWITHBUFFER_IMM16_ID16:
            LowerCreateArrayWithBuffer();
            break;
        case kungfu::EcmaOpcode::STMODULEVAR_IMM8:
        case kungfu::EcmaOpcode::WIDE_STMODULEVAR_PREF_IMM16:
            LowerStModuleVar();
            break;
        case kungfu::EcmaOpcode::GETTEMPLATEOBJECT_IMM8:
        case kungfu::EcmaOpcode::GETTEMPLATEOBJECT_IMM16:
            LowerGetTemplateObject();
            break;
        case kungfu::EcmaOpcode::SETOBJECTWITHPROTO_IMM8_V8:
        case kungfu::EcmaOpcode::SETOBJECTWITHPROTO_IMM16_V8:
            LowerSetObjectWithProto();
            break;
        case kungfu::EcmaOpcode::LDBIGINT_ID16:
            LowerLoadBigInt();
            break;
        case kungfu::EcmaOpcode::TONUMERIC_IMM8:
            LowerToNumeric();
            break;
        case kungfu::EcmaOpcode::DYNAMICIMPORT:
            LowerDynamicImport();
            break;
        case kungfu::EcmaOpcode::LDEXTERNALMODULEVAR_IMM8:
        case kungfu::EcmaOpcode::WIDE_LDEXTERNALMODULEVAR_PREF_IMM16:
            LowerLdExternalModuleVar();
            break;
        case kungfu::EcmaOpcode::GETMODULENAMESPACE_IMM8:
        case kungfu::EcmaOpcode::WIDE_GETMODULENAMESPACE_PREF_IMM16:
            LowerGetModuleNamespace();
            break;
        case kungfu::EcmaOpcode::NEWOBJRANGE_IMM8_IMM8_V8:
        case kungfu::EcmaOpcode::NEWOBJRANGE_IMM16_IMM8_V8:
            LowerNewObjRange();
            break;
        case kungfu::EcmaOpcode::WIDE_NEWOBJRANGE_PREF_IMM16_V8:
            LowerNewObjRange();
            break;
        case kungfu::EcmaOpcode::JEQZ_IMM8:
        case kungfu::EcmaOpcode::JEQZ_IMM16:
        case kungfu::EcmaOpcode::JEQZ_IMM32:
            LowerJumpIfFalse();
            break;
        case kungfu::EcmaOpcode::JNEZ_IMM8:
        case kungfu::EcmaOpcode::JNEZ_IMM16:
        case kungfu::EcmaOpcode::JNEZ_IMM32:
            LowerJumpIfTrue();
            break;
        case kungfu::EcmaOpcode::SUPERCALLTHISRANGE_IMM8_IMM8_V8:
        case kungfu::EcmaOpcode::WIDE_SUPERCALLTHISRANGE_PREF_IMM16_V8:
            LowerSuperCallThisRange();
            break;
        case kungfu::EcmaOpcode::SUPERCALLARROWRANGE_IMM8_IMM8_V8:
        case kungfu::EcmaOpcode::WIDE_SUPERCALLARROWRANGE_PREF_IMM16_V8:
            LowerSuperCallArrowRange();
            break;
        case kungfu::EcmaOpcode::SUPERCALLSPREAD_IMM8_V8:
            LowerSuperCallSpread();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_SUPERCALLFORWARDALLARGS_PREF_V8:
            LowerSuperCallForwardAllArgs();
            break;
        case kungfu::EcmaOpcode::ISTRUE:
        case kungfu::EcmaOpcode::CALLRUNTIME_ISTRUE_PREF_IMM8:
            LowerIsTrueOrFalse(true);
            break;
        case kungfu::EcmaOpcode::ISFALSE:
        case kungfu::EcmaOpcode::CALLRUNTIME_ISFALSE_PREF_IMM8:
            LowerIsTrueOrFalse(false);
            break;
        case kungfu::EcmaOpcode::GETNEXTPROPNAME_V8:
            LowerGetNextPropName();
            break;
        case kungfu::EcmaOpcode::COPYDATAPROPERTIES_V8:
            LowerCopyDataProperties();
            break;
        case kungfu::EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8:
        case kungfu::EcmaOpcode::WIDE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8:
            LowerCreateObjectWithExcludedKeys();
            break;
        case kungfu::EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM8_ID16_IMM8:
        case kungfu::EcmaOpcode::CREATEREGEXPWITHLITERAL_IMM16_ID16_IMM8:
            LowerCreateRegExpWithLiteral();
            break;
        case kungfu::EcmaOpcode::STOWNBYVALUE_IMM8_V8_V8:
        case kungfu::EcmaOpcode::STOWNBYVALUE_IMM16_V8_V8:
            LowerStOwnByValue();
            break;
        case kungfu::EcmaOpcode::STOWNBYINDEX_IMM8_V8_IMM16:
        case kungfu::EcmaOpcode::STOWNBYINDEX_IMM16_V8_IMM16:
        case kungfu::EcmaOpcode::WIDE_STOWNBYINDEX_PREF_V8_IMM32:
            LowerStOwnByIndex();
            break;
        case kungfu::EcmaOpcode::STOWNBYNAME_IMM8_ID16_V8:
        case kungfu::EcmaOpcode::STOWNBYNAME_IMM16_ID16_V8:
            LowerStOwnByName();
            break;
        case kungfu::EcmaOpcode::NEWLEXENV_IMM8:
        case kungfu::EcmaOpcode::WIDE_NEWLEXENV_PREF_IMM16:
            LowerNewLexicalEnv();
            break;
        case kungfu::EcmaOpcode::NEWLEXENVWITHNAME_IMM8_ID16:
        case kungfu::EcmaOpcode::WIDE_NEWLEXENVWITHNAME_PREF_IMM16_ID16:
            LowerNewLexicalEnvWithName();
            break;
        case kungfu::EcmaOpcode::POPLEXENV:
            LowerPopLexicalEnv();
            break;
        case kungfu::EcmaOpcode::LDSUPERBYVALUE_IMM8_V8:
        case kungfu::EcmaOpcode::LDSUPERBYVALUE_IMM16_V8:
            LowerLdSuperByValue();
            break;
        case kungfu::EcmaOpcode::STSUPERBYVALUE_IMM16_V8_V8:
        case kungfu::EcmaOpcode::STSUPERBYVALUE_IMM8_V8_V8:
            LowerStSuperByValue();
            break;
        case kungfu::EcmaOpcode::TRYSTGLOBALBYNAME_IMM8_ID16:
        case kungfu::EcmaOpcode::TRYSTGLOBALBYNAME_IMM16_ID16:
            LowerTryStGlobalByName();
            break;
        case kungfu::EcmaOpcode::STCONSTTOGLOBALRECORD_IMM16_ID16:
            LowerStConstToGlobalRecord(true);
            break;
        case kungfu::EcmaOpcode::STTOGLOBALRECORD_IMM16_ID16:
            LowerStConstToGlobalRecord(false);
            break;
        case kungfu::EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM8_V8_V8:
        case kungfu::EcmaOpcode::STOWNBYVALUEWITHNAMESET_IMM16_V8_V8:
            LowerStOwnByValueWithNameSet();
            break;
        case kungfu::EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM8_ID16_V8:
        case kungfu::EcmaOpcode::STOWNBYNAMEWITHNAMESET_IMM16_ID16_V8:
            LowerStOwnByNameWithNameSet();
            break;
        case kungfu::EcmaOpcode::LDGLOBALVAR_IMM16_ID16:
            LowerLdGlobalVar();
            break;
        case kungfu::EcmaOpcode::LDOBJBYNAME_IMM8_ID16:
        case kungfu::EcmaOpcode::LDOBJBYNAME_IMM16_ID16:
            LowerLoadObjByName();
            break;
        case kungfu::EcmaOpcode::STOBJBYNAME_IMM8_ID16_V8:
        case kungfu::EcmaOpcode::STOBJBYNAME_IMM16_ID16_V8:
            LowerStoreObjByName();
            break;
        case kungfu::EcmaOpcode::DEFINEGETTERSETTERBYVALUE_V8_V8_V8_V8:
            LowerDefineGetterSetterByValue();
            break;
        case kungfu::EcmaOpcode::LDOBJBYINDEX_IMM8_IMM16:
        case kungfu::EcmaOpcode::LDOBJBYINDEX_IMM16_IMM16:
            LowerLdObjByIndex();
            break;
        case kungfu::EcmaOpcode::WIDE_LDOBJBYINDEX_PREF_IMM32:
            LowerLdObjByIndex();
            break;
        case kungfu::EcmaOpcode::STOBJBYINDEX_IMM8_V8_IMM16:
        case kungfu::EcmaOpcode::STOBJBYINDEX_IMM16_V8_IMM16:
            LowerStObjByIndex();
            break;
        case kungfu::EcmaOpcode::WIDE_STOBJBYINDEX_PREF_V8_IMM32:
            LowerStObjByIndex();
            break;
        case kungfu::EcmaOpcode::LDOBJBYVALUE_IMM8_V8:
        case kungfu::EcmaOpcode::LDOBJBYVALUE_IMM16_V8:
            LowerLoadObjByValue();
            break;
        case kungfu::EcmaOpcode::LDTHISBYVALUE_IMM8:
        case kungfu::EcmaOpcode::LDTHISBYVALUE_IMM16:
            LowerLdThisByValue();
            break;
        case kungfu::EcmaOpcode::STOBJBYVALUE_IMM8_V8_V8:
        case kungfu::EcmaOpcode::STOBJBYVALUE_IMM16_V8_V8:
            LowerStoreObjByValue();
            break;
        case kungfu::EcmaOpcode::STTHISBYVALUE_IMM8_V8:
        case kungfu::EcmaOpcode::STTHISBYVALUE_IMM16_V8:
            LowerStThisByValue();
            break;
        case kungfu::EcmaOpcode::LDSUPERBYNAME_IMM8_ID16:
        case kungfu::EcmaOpcode::LDSUPERBYNAME_IMM16_ID16:
            LowerLdSuperByName();
            break;
        case kungfu::EcmaOpcode::STSUPERBYNAME_IMM8_ID16_V8:
        case kungfu::EcmaOpcode::STSUPERBYNAME_IMM16_ID16_V8:
            LowerStSuperByName();
            break;
        case kungfu::EcmaOpcode::STARRAYSPREAD_V8_V8:
            LowerStoreArraySpread();
            break;
        case kungfu::EcmaOpcode::LDLEXVAR_IMM4_IMM4:
        case kungfu::EcmaOpcode::LDLEXVAR_IMM8_IMM8:
        case kungfu::EcmaOpcode::WIDE_LDLEXVAR_PREF_IMM16_IMM16:
            LowerLdLexVar();
            break;
        case kungfu::EcmaOpcode::STLEXVAR_IMM4_IMM4:
        case kungfu::EcmaOpcode::STLEXVAR_IMM8_IMM8:
        case kungfu::EcmaOpcode::WIDE_STLEXVAR_PREF_IMM16_IMM16:
            LowerStLexVar();
            break;
        case kungfu::EcmaOpcode::DEFINECLASSWITHBUFFER_IMM8_ID16_ID16_IMM16_V8:
        case kungfu::EcmaOpcode::DEFINECLASSWITHBUFFER_IMM16_ID16_ID16_IMM16_V8:
            LowerDefineClassWithBuffer();
            break;
        case kungfu::EcmaOpcode::DEFINEFUNC_IMM8_ID16_IMM8:
        case kungfu::EcmaOpcode::DEFINEFUNC_IMM16_ID16_IMM8:
            LowerDefineFunc();
            break;
        case kungfu::EcmaOpcode::COPYRESTARGS_IMM8:
        case kungfu::EcmaOpcode::WIDE_COPYRESTARGS_PREF_IMM16:
            LowerCopyRestArgs();
            break;
        case kungfu::EcmaOpcode::WIDE_LDPATCHVAR_PREF_IMM16:
            LowerLdPatchVar();
            break;
        case kungfu::EcmaOpcode::WIDE_STPATCHVAR_PREF_IMM16:
            LowerStPatchVar();
            break;
        case kungfu::EcmaOpcode::LDLOCALMODULEVAR_IMM8:
        case kungfu::EcmaOpcode::WIDE_LDLOCALMODULEVAR_PREF_IMM16:
            LowerLdLocalModuleVar();
            break;
        case kungfu::EcmaOpcode::LDTHISBYNAME_IMM8_ID16:
        case kungfu::EcmaOpcode::LDTHISBYNAME_IMM16_ID16:
            LowerLdThisByName();
            break;
        case kungfu::EcmaOpcode::STTHISBYNAME_IMM8_ID16:
        case kungfu::EcmaOpcode::STTHISBYNAME_IMM16_ID16:
            LowerStThisByName();
            break;
        case kungfu::EcmaOpcode::LDPRIVATEPROPERTY_IMM8_IMM16_IMM16:
            LowerLdPrivateProperty();
            break;
        case kungfu::EcmaOpcode::STPRIVATEPROPERTY_IMM8_IMM16_IMM16_V8:
            LowerStPrivateProperty();
            break;
        case kungfu::EcmaOpcode::TESTIN_IMM8_IMM16_IMM16:
            LowerTestIn();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_NOTIFYCONCURRENTRESULT_PREF_NONE:
            LowerNotifyConcurrentResult();
            break;
        case kungfu::EcmaOpcode::DEFINEPROPERTYBYNAME_IMM8_ID16_V8:
            LowerDefinePropertyByName();
            break;
        case kungfu::EcmaOpcode::DEFINEFIELDBYNAME_IMM8_ID16_V8:
            LowerDefineFieldByName();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYVALUE_PREF_IMM8_V8_V8:
            LowerDefineFieldByValue();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEFIELDBYINDEX_PREF_IMM8_IMM32_V8:
            LowerDefineFieldByIndex();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_TOPROPERTYKEY_PREF_NONE:
            LowerToPropertyKey();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_CREATEPRIVATEPROPERTY_PREF_IMM16_ID16:
            LowerCreatePrivateProperty();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_DEFINEPRIVATEPROPERTY_PREF_IMM8_IMM16_IMM16_V8:
            LowerDefinePrivateProperty();
            break;
        case kungfu::EcmaOpcode::CALLRUNTIME_LDLAZYMODULEVAR_PREF_IMM8:
        case kungfu::EcmaOpcode::CALLRUNTIME_WIDELDLAZYMODULEVAR_PREF_IMM16:
            LowerLdExternalModuleVar();
            break;
        case kungfu::EcmaOpcode::LDA_STR_ID16:
            LowerLoadString();
            break;
        case kungfu::EcmaOpcode::JMP_IMM8:
        case kungfu::EcmaOpcode::JMP_IMM16:
        case kungfu::EcmaOpcode::JMP_IMM32:
            LowerJumpConstant();
            break;
        case kungfu::EcmaOpcode::LDNAN:
        case kungfu::EcmaOpcode::LDINFINITY:
        case kungfu::EcmaOpcode::LDUNDEFINED:
        case kungfu::EcmaOpcode::LDNULL:
        case kungfu::EcmaOpcode::LDTRUE:
        case kungfu::EcmaOpcode::LDFALSE:
        case kungfu::EcmaOpcode::LDHOLE:
        case kungfu::EcmaOpcode::LDAI_IMM32:
        case kungfu::EcmaOpcode::FLDAI_IMM64:
        case kungfu::EcmaOpcode::LDFUNCTION:
        case kungfu::EcmaOpcode::LDNEWTARGET:
        case kungfu::EcmaOpcode::LDTHIS:
            LowerLoadConst(info, opcode);
            break;
        case kungfu::EcmaOpcode::MOV_V4_V4:
        case kungfu::EcmaOpcode::MOV_V8_V8:
        case kungfu::EcmaOpcode::MOV_V16_V16:
        case kungfu::EcmaOpcode::STA_V8:
        case kungfu::EcmaOpcode::LDA_V8:
            LowerMoveValues(info);
            break;
        case kungfu::EcmaOpcode::RETURNUNDEFINED:
        case kungfu::EcmaOpcode::RETURN:
            LowerReturn(opcode);
            break;
        default:
            UNREACHABLE();
    }
}

void ArkSteedGraphBuilder::LowerLoadString()
{
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *res = GetStringFromConstPool(stringId);
    currentFrameState_->SetAcc(res);
}

void ArkSteedGraphBuilder::LowerLoadBigInt()
{
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *numberBigInt = GetStringFromConstPool(stringId);
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({numberBigInt}, RTSTUB_ID(LdBigInt)));
}

void ArkSteedGraphBuilder::LowerLoadConst(const BytecodeInfo &info, kungfu::EcmaOpcode opcode)
{
    ValueVertex *vertex = nullptr;
    switch (opcode) {
        case EcmaOpcode::LDNAN:
            vertex = GetTaggedConstant(base::NumberHelper::GetNaN());
            break;
        case EcmaOpcode::LDINFINITY:
            vertex = GetTaggedConstant(base::NumberHelper::GetPositiveInfinity());
            break;
        case EcmaOpcode::LDUNDEFINED:
            vertex = GetRootConstant(RootConstantVertex::RootIndex::UNDEFINED);
            break;
        case EcmaOpcode::LDNULL:
            vertex = GetRootConstant(RootConstantVertex::RootIndex::NULL_VALUE);
            break;
        case EcmaOpcode::LDTRUE:
            vertex = GetRootConstant(RootConstantVertex::RootIndex::TRUE_VALUE);
            break;
        case EcmaOpcode::LDFALSE:
            vertex = GetRootConstant(RootConstantVertex::RootIndex::FALSE_VALUE);
            break;
        case EcmaOpcode::LDHOLE:
            vertex = GetTaggedConstant(JSTaggedValue::VALUE_HOLE);
            break;
        case EcmaOpcode::LDAI_IMM32:
            vertex = GetTaggedConstant(std::get<Immediate>(info.inputs[0]).ToJSTaggedValueInt());
            break;
        case EcmaOpcode::FLDAI_IMM64:
            vertex = GetTaggedConstant(std::get<Immediate>(info.inputs[0]).ToJSTaggedValueDouble());
            break;
        case EcmaOpcode::LDFUNCTION:
            vertex = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
            break;
        case EcmaOpcode::LDNEWTARGET:
            vertex = currentFrameState_->GetParam(NEW_TARGET_PARAM_INDEX);
            break;
        case EcmaOpcode::LDTHIS:
            vertex = currentFrameState_->GetParam(THIS_OBJECT_PARAM_INDEX);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    currentFrameState_->SetAcc(vertex);
}

void ArkSteedGraphBuilder::LowerMoveValues(const BytecodeInfo &info)
{
    ValueVertex *vertex = nullptr;
    // Get input value
    if (info.AccIn()) {
        vertex = currentFrameState_->GetAcc();
    } else if (!info.inputs.empty()) {
        vertex = LoadRegister(0);
    } else {
        UNREACHABLE();
    }
    // Set output value
    if (info.AccOut()) {
        currentFrameState_->SetAcc(vertex);
    } else if (!info.vregOut.empty()) {
        currentFrameState_->Set(VirtualRegister(info.vregOut[0]), vertex);
    } else {
        UNREACHABLE();
    }
}

void ArkSteedGraphBuilder::LowerCreateEmptyObject()
{
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({}, RTSTUB_ID(CreateEmptyObject)));
}

void ArkSteedGraphBuilder::LowerCreateEmptyArray()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, globalEnv}, CommonStubCSigns::CreateEmptyArray));
}

void ArkSteedGraphBuilder::LowerCreateObjectWithBuffer()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *index = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *obj = GetObjectFromConstPool(index);
    ValueVertex *lexEnv = LoadRegister(1);
    currentFrameState_->SetAcc(NewCommonStubCall({glue, obj, lexEnv}, CommonStubCSigns::CreateObjectHavingMethod));
}

void ArkSteedGraphBuilder::LowerCreateArrayWithBuffer()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *index = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *slotId = GetInt32Constant(static_cast<int>(GetICSlotId(1)));
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(
        NewCommonStubCall({glue, index, jsFunc, slotId, globalEnv}, CommonStubCSigns::CreateArrayWithBuffer));
}

void ArkSteedGraphBuilder::LowerCreateObjectWithExcludedKeys()
{
    uint32_t inputSize = GetInputSize();
    std::vector<ValueVertex *> args;
    for (uint32_t idx = 0; idx < inputSize; idx++) {
        args.push_back(LoadRegister(idx));
    }
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>(args, RTSTUB_ID(OptCreateObjectWithExcludedKeys)));
}

void ArkSteedGraphBuilder::LowerCreateRegExpWithLiteral()
{
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *pattern = GetStringFromConstPool(stringId);
    ValueVertex *flags = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(1)));  // 1: flags operand index
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({pattern, flags}, RTSTUB_ID(CreateRegExpWithLiteral)));
}

void ArkSteedGraphBuilder::LowerNewObjRange()
{
    uint32_t inputSize = GetInputSize();
    std::vector<ValueVertex *> args;
    for (uint32_t idx = 0; idx < inputSize; idx++) {
        args.push_back(LoadRegister(idx));
    }
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>(args, RTSTUB_ID(OptNewObjRange)));
}

void ArkSteedGraphBuilder::LowerAdd2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Add));
}

void ArkSteedGraphBuilder::LowerSub2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Sub));
}

void ArkSteedGraphBuilder::LowerMul2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Mul));
}

void ArkSteedGraphBuilder::LowerDiv2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Div));
}

void ArkSteedGraphBuilder::LowerMod2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Mod));
}

void ArkSteedGraphBuilder::LowerExp()
{
    ValueVertex *left = LoadRegister(0);
    ValueVertex *right = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({left, right}, RTSTUB_ID(Exp)));
}

void ArkSteedGraphBuilder::LowerNeg()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x}, CommonStubCSigns::Neg));
}

void ArkSteedGraphBuilder::LowerInc()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x}, CommonStubCSigns::Inc));
}

void ArkSteedGraphBuilder::LowerDec()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x}, CommonStubCSigns::Dec));
}

void ArkSteedGraphBuilder::LowerShl2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Shl));
}

void ArkSteedGraphBuilder::LowerShr2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Shr));
}

void ArkSteedGraphBuilder::LowerAshr2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Ashr));
}

void ArkSteedGraphBuilder::LowerAnd2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::And));
}

void ArkSteedGraphBuilder::LowerOr2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Or));
}

void ArkSteedGraphBuilder::LowerXor2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Xor));
}

void ArkSteedGraphBuilder::LowerNot()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x}, CommonStubCSigns::Not));
}

void ArkSteedGraphBuilder::LowerEq()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Equal));
}

void ArkSteedGraphBuilder::LowerNotEq()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::NotEqual));
}

void ArkSteedGraphBuilder::LowerLess()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Less));
}

void ArkSteedGraphBuilder::LowerLessEq()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::LessEq));
}

void ArkSteedGraphBuilder::LowerGreater()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::Greater));
}

void ArkSteedGraphBuilder::LowerGreaterEq()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::GreaterEq));
}

void ArkSteedGraphBuilder::LowerStrictEq()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::StrictEqual));
}

void ArkSteedGraphBuilder::LowerStrictNotEq()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *x = LoadRegister(0);
    ValueVertex *y = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, x, y, globalEnv}, CommonStubCSigns::StrictNotEqual));
}

void ArkSteedGraphBuilder::LowerTypeOf()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *obj = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, obj}, CommonStubCSigns::TypeOf));
}

void ArkSteedGraphBuilder::LowerToNumber()
{
    ValueVertex *value = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({value}, RTSTUB_ID(ToNumber)));
}

void ArkSteedGraphBuilder::LowerToNumeric()
{
    ValueVertex *value = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({value}, RTSTUB_ID(ToNumeric)));
}

void ArkSteedGraphBuilder::LowerIsIn()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *prop = LoadRegister(0);
    ValueVertex *obj = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, prop, obj, globalEnv}, CommonStubCSigns::IsIn));
}

void ArkSteedGraphBuilder::LowerInstanceOf()
{
    ValueVertex *object = LoadRegister(1);
    ValueVertex *target = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithIC(CommonStubCSigns::Instanceof, {object, target, globalEnv});
}

void ArkSteedGraphBuilder::LowerTestIn()
{
    ValueVertex *levelIndex = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(1)));  // 1: level operand index
    ValueVertex *slotIndex = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(2)));  // 2: slot operand index
    ValueVertex *lexicalEnv = LoadRegister(3);  // 3: lexicalEnv register index
    ValueVertex *obj = currentFrameState_->GetAcc();

    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({lexicalEnv, levelIndex, slotIndex, obj}, RTSTUB_ID(TestIn)));
}

void ArkSteedGraphBuilder::LowerLoadObjByName()
{
    ValueVertex *receiver = currentFrameState_->GetAcc();
    ValueVertex *id = GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithIC(CommonStubCSigns::GetPropertyByName, {receiver, id, globalEnv});
}

void ArkSteedGraphBuilder::LowerCallArg0()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *result = NewCommonStubCall({glue, func}, CommonStubCSigns::CallArg0Stub);
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallArg1()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *a0Value = LoadRegister(0);
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *result = NewCommonStubCall({glue, func, a0Value}, CommonStubCSigns::CallArg1Stub);
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallArgs2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *a0Value = LoadRegister(0);
    ValueVertex *a1Value = LoadRegister(1);
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *result = NewCommonStubCall({glue, func, a0Value, a1Value}, CommonStubCSigns::CallArg2Stub);
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallArgs3()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *a0Value = LoadRegister(0);
    ValueVertex *a1Value = LoadRegister(1);
    ValueVertex *a2Value = LoadRegister(2);  // 2: third argument register index
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *result = NewCommonStubCall({glue, func, a0Value, a1Value, a2Value}, CommonStubCSigns::CallArg3Stub);
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallRange()
{
    uint32_t inputSize = GetInputSize();
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *taggedArray = GetTaggedArrayFromValueIn(inputSize);
    ValueVertex *taggedLength = GetTaggedLength(inputSize);

    ValueVertex *result = NewVertex<CallRuntimeVertex>({func, taggedArray, taggedLength}, RTSTUB_ID(CallRange));
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerReturn(kungfu::EcmaOpcode opcode)
{
    ValueVertex *value = nullptr;
    if (opcode == EcmaOpcode::RETURNUNDEFINED) {
        value = GetRootConstant(RootConstantVertex::RootIndex::UNDEFINED);
    } else {
        value = currentFrameState_->GetAcc();
    }
    FinishBlock<ReturnVertex>({value});
}

void ArkSteedGraphBuilder::LowerThrow()
{
    ValueVertex *exception = currentFrameState_->GetAcc();
    BuildThrow(RTSTUB_ID(Throw), exception);
}

void ArkSteedGraphBuilder::LowerThrowConstAssignment()
{
    ValueVertex *value = LoadRegister(0);
    BuildThrow(RTSTUB_ID(ThrowConstAssignment), value);
}

void ArkSteedGraphBuilder::LowerThrowNotExists()
{
    BuildThrow(RTSTUB_ID(ThrowThrowNotExists), nullptr);
}

void ArkSteedGraphBuilder::LowerThrowPatternNonCoercible()
{
    BuildThrow(RTSTUB_ID(ThrowPatternNonCoercible), nullptr);
}

void ArkSteedGraphBuilder::LowerThrowIfNotObject()
{
    // to do: Implement the throw path after subgraph is supported.
}

void ArkSteedGraphBuilder::LowerThrowUndefinedIfHole()
{
    // to do: Implement the throw path after subgraph is supported.
}

void ArkSteedGraphBuilder::LowerThrowUndefinedIfHoleWithName()
{
    // to do: Implement the throw path after subgraph is supported.
}

void ArkSteedGraphBuilder::LowerThrowIfSuperNotCorrectCall()
{
    ValueVertex *index = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    ValueVertex *thisValue = currentFrameState_->GetAcc();
    NewVertex<ThrowIfSuperNotCorrectCallVertex>({index, thisValue}, RTSTUB_ID(ThrowIfSuperNotCorrectCall));
}

void ArkSteedGraphBuilder::LowerThrowDeleteSuperProperty()
{
    BuildThrow(RTSTUB_ID(ThrowDeleteSuperProperty), nullptr);
}

void ArkSteedGraphBuilder::LowerStoreObjByName()
{
    ValueVertex *receiver = LoadRegister(2);  // 2: receiver register index
    ValueVertex *id = GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithICPreserveAcc(CommonStubCSigns::SetPropertyByName, {receiver, id, value, globalEnv});
}

void ArkSteedGraphBuilder::LowerLoadObjByValue()
{
    ValueVertex *receiver = LoadRegister(1);
    ValueVertex *key = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithIC(CommonStubCSigns::GetPropertyByValue, {receiver, key, globalEnv});
}

void ArkSteedGraphBuilder::LowerStoreObjByValue()
{
    ValueVertex *receiver = LoadRegister(1);
    ValueVertex *key = LoadRegister(2);  // 2: key register index
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithICPreserveAcc(CommonStubCSigns::SetPropertyByValue, {receiver, key, value, globalEnv});
}

void ArkSteedGraphBuilder::LowerLdObjByIndex()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *receiver = LoadRegister(1);
    ValueVertex *index = GetInt32Constant(static_cast<int>(GetImmediate(0)));
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, receiver, index, globalEnv}, CommonStubCSigns::LdObjByIndex));
}

void ArkSteedGraphBuilder::LowerStObjByIndex()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *receiver = LoadRegister(0);
    ValueVertex *index = GetInt32Constant(static_cast<int>(GetImmediate(1)));
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, receiver, index, value, globalEnv}, CommonStubCSigns::StObjByIndex);
}

void ArkSteedGraphBuilder::LowerGetIterator()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *obj = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, obj, globalEnv}, CommonStubCSigns::GetIterator));
}

void ArkSteedGraphBuilder::LowerGetPropIterator()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *object = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, object, globalEnv}, CommonStubCSigns::Getpropiterator));
}

void ArkSteedGraphBuilder::LowerCloseIterator()
{
    ValueVertex *iterator = LoadRegister(0);
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({iterator}, RTSTUB_ID(CloseIterator)));
}

void ArkSteedGraphBuilder::LowerGetNextPropName()
{
    ValueVertex *iterator = LoadRegister(0);
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({iterator}, RTSTUB_ID(GetNextPropNameSlowpath)));
}

void ArkSteedGraphBuilder::LowerGetTemplateObject()
{
    ValueVertex *value = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({value}, RTSTUB_ID(GetTemplateObject)));
}

void ArkSteedGraphBuilder::LowerStoreArraySpread()
{
    ValueVertex *array = LoadRegister(0);
    ValueVertex *index = LoadRegister(1);
    ValueVertex *value = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({array, index, value}, RTSTUB_ID(StArraySpread)));
}

void ArkSteedGraphBuilder::LowerCallThis0()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *thisObj = LoadRegister(0);
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *result = NewCommonStubCall({glue, func, thisObj}, CommonStubCSigns::CallThis0Stub);
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallThis1()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *thisObj = LoadRegister(0);
    ValueVertex *a0Value = LoadRegister(1);
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *result = NewCommonStubCall({glue, func, thisObj, a0Value}, CommonStubCSigns::CallThis1Stub);
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallThis2()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *thisObj = LoadRegister(0);
    ValueVertex *a0Value = LoadRegister(1);
    ValueVertex *a1Value = LoadRegister(2);  // 2: second argument register index
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *result = NewCommonStubCall({glue, func, thisObj, a0Value, a1Value}, CommonStubCSigns::CallThis2Stub);
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallThis3()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *thisObj = LoadRegister(0);
    ValueVertex *a0Value = LoadRegister(1);
    ValueVertex *a1Value = LoadRegister(2);  // 2: second argument register index
    ValueVertex *a2Value = LoadRegister(3);  // 3: third argument register index
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *result =
        NewCommonStubCall({glue, func, thisObj, a0Value, a1Value, a2Value}, CommonStubCSigns::CallThis3Stub);
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallThisRange()
{
    uint32_t inputSize = GetInputSize();
    ASSERT(inputSize > 0);
    uint32_t argc = inputSize - 1;  // Skip the receiver.
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *thisObj = LoadRegister(0);
    ValueVertex *taggedArray = GetTaggedArrayFromValueIn(argc, 1);
    ValueVertex *taggedLength = GetTaggedLength(argc);

    ValueVertex *result =
        NewVertex<CallRuntimeVertex>({thisObj, func, taggedArray, taggedLength}, RTSTUB_ID(CallThisRange));
    currentFrameState_->SetAcc(result);
}

void ArkSteedGraphBuilder::LowerCallSpread()
{
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *thisArg = LoadRegister(0);
    ValueVertex *argsArray = LoadRegister(1);
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({func, thisArg, argsArray}, RTSTUB_ID(CallSpread)));
}

void ArkSteedGraphBuilder::LowerGetUnmappedArgs()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *argv = GetIntPtrConstant(0);
    ValueVertex *numArgs = GetActualArgc();
    ValueVertex *argvTaggedArray = GetRootConstant(RootConstantVertex::RootIndex::UNDEFINED);
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(
        NewCommonStubCall({glue, argv, numArgs, argvTaggedArray, globalEnv}, CommonStubCSigns::GetUnmappedArgs));
}

void ArkSteedGraphBuilder::LowerCreateIterResultObj()
{
    ValueVertex *value = LoadRegister(0);
    ValueVertex *done = LoadRegister(1);
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({value, done}, RTSTUB_ID(CreateIterResultObj)));
}

void ArkSteedGraphBuilder::LowerTryLdGlobalByName()
{
    ValueVertex *id = GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithIC(CommonStubCSigns::TryLdGlobalByName, {id, globalEnv});
}

void ArkSteedGraphBuilder::LowerStGlobalVar()
{
    ValueVertex *id = GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithICPreserveAcc(CommonStubCSigns::StGlobalVar, {id, value, globalEnv});
}

void ArkSteedGraphBuilder::LowerNewObjApply()
{
    ValueVertex *target = LoadRegister(0);
    ValueVertex *args = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({target, args}, RTSTUB_ID(NewObjApply)));
}

void ArkSteedGraphBuilder::LowerLdSymbol()
{
    ValueVertex *globalEnv = GetGlobalEnv();
    // Calculate offset: HEADER_SIZE + SYMBOL_FUNCTION_INDEX * TaggedTypeSize
    int32_t offset = static_cast<int32_t>(GlobalEnv::HEADER_SIZE +
                                          GlobalEnv::SYMBOL_FUNCTION_INDEX * JSTaggedValue::TaggedTypeSize());
    currentFrameState_->SetAcc(NewVertex<LoadTaggedFieldVertex>({globalEnv}, offset));
}

void ArkSteedGraphBuilder::LowerLdGlobal()
{
    ValueVertex *globalEnv = GetGlobalEnv();
    // Calculate offset: HEADER_SIZE + JS_GLOBAL_OBJECT_INDEX * TaggedTypeSize
    int32_t offset = static_cast<int32_t>(GlobalEnv::HEADER_SIZE +
                                          GlobalEnv::JS_GLOBAL_OBJECT_INDEX * JSTaggedValue::TaggedTypeSize());
    currentFrameState_->SetAcc(NewVertex<LoadTaggedFieldVertex>({globalEnv}, offset));
}

void ArkSteedGraphBuilder::LowerDelObjProp()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *object = LoadRegister(0);
    ValueVertex *prop = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    currentFrameState_->SetAcc(
        NewCommonStubCall({glue, object, prop, globalEnv}, CommonStubCSigns::DeleteObjectProperty));
    // to do: IsSpecial
}

void ArkSteedGraphBuilder::LowerDefineMethod()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *methodId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *taggedMethodId = NewVertex<ToTaggedIntVertex>({methodId});
    ValueVertex *length = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(1)));
    ValueVertex *env = LoadRegister(2);  // 2: env register index
    ValueVertex *homeObject = currentFrameState_->GetAcc();
    ValueVertex *module = GetModuleFromFunction();
    ValueVertex *method = GetMethodFromConstPool(taggedMethodId);
    ValueVertex *slotId = NewTaggedVertexFromRawInt32(static_cast<int>(GetICSlotId(3)));  // 3: slotId operand index

    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>(
#if ECMASCRIPT_ENABLE_IC
        {method, homeObject, length, env, module, slotId, jsFunc},
#else
        {method, homeObject, length, env, module},
#endif
        RTSTUB_ID(DefineMethod)));
}

void ArkSteedGraphBuilder::LowerStModuleVar()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *index = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    ValueVertex *value = currentFrameState_->GetAcc();
    NewVertex<CallRuntimeVertex>({index, value, jsFunc}, RTSTUB_ID(StModuleVarByIndexOnJSFunc));
}

void ArkSteedGraphBuilder::LowerSetObjectWithProto()
{
    ValueVertex *proto = LoadRegister(0);
    ValueVertex *obj = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({proto, obj}, RTSTUB_ID(SetObjectWithProto)));
}

void ArkSteedGraphBuilder::LowerDynamicImport()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *specifier = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({specifier, jsFunc}, RTSTUB_ID(DynamicImport)));
}

void ArkSteedGraphBuilder::LowerLdExternalModuleVar()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *index = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({index, jsFunc}, RTSTUB_ID(LdExternalModuleVarByIndexOnJSFunc)));
}

void ArkSteedGraphBuilder::LowerGetModuleNamespace()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *index = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({index, jsFunc}, RTSTUB_ID(GetModuleNamespaceByIndexOnJSFunc)));
}

void ArkSteedGraphBuilder::LowerSuperCallThisRange()
{
    uint32_t inputSize = GetInputSize();
    ValueVertex *thisFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *newTarget = currentFrameState_->GetParam(NEW_TARGET_PARAM_INDEX);
    ValueVertex *taggedArray = GetTaggedArrayFromValueIn(inputSize);
    ValueVertex *taggedLength = NewTaggedVertexFromRawInt32(static_cast<int>(inputSize));

    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({thisFunc, newTarget, taggedArray, taggedLength}, RTSTUB_ID(OptSuperCall)));
}

void ArkSteedGraphBuilder::LowerSuperCallArrowRange()
{
    uint32_t inputSize = GetInputSize();
    uint32_t argc = inputSize - 1;

    ValueVertex *func = LoadRegister(argc);
    ValueVertex *newTarget = currentFrameState_->GetParam(NEW_TARGET_PARAM_INDEX);
    ValueVertex *taggedArray = GetTaggedArrayFromValueIn(argc);
    ValueVertex *taggedLength = NewTaggedVertexFromRawInt32(static_cast<int>(argc));

    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({func, newTarget, taggedArray, taggedLength}, RTSTUB_ID(OptSuperCall)));
}

void ArkSteedGraphBuilder::LowerSuperCallSpread()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *globalEnv = GetGlobalEnv();
    ValueVertex *array = LoadRegister(0);
    ValueVertex *func = currentFrameState_->GetAcc();
    ValueVertex *newTarget = currentFrameState_->GetParam(NEW_TARGET_PARAM_INDEX);

    ValueVertex *argsArray = NewCommonStubCall({glue, array, globalEnv}, CommonStubCSigns::GetCallSpreadArgs);

    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({func, newTarget, argsArray}, RTSTUB_ID(OptSuperCallSpread)));
}

void ArkSteedGraphBuilder::LowerSuperCallForwardAllArgs()
{
    ValueVertex *func = LoadRegister(0);
    ValueVertex *superFunc = NewCommonStubCall({GetGlue(), func}, CommonStubCSigns::GetPrototype);
    ValueVertex *newTarget = currentFrameState_->GetParam(NEW_TARGET_PARAM_INDEX);
    ValueVertex *actualArgc = NewVertex<ToTaggedIntVertex>({GetActualArgc()});
    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({superFunc, newTarget, actualArgc}, RTSTUB_ID(OptSuperCallForwardAllArgs)));
}

void ArkSteedGraphBuilder::LowerJumpConstant()
{
    uint32_t index = iterator_.Index();
    uint32_t targetBcIndex = iterator_.GetJumpTargetBcIndex();
    BB *block = nullptr;
    if (bytecodeContext_.GetJumpLoop()[index]) {
        block = FinishBlock<JumpLoopVertex>({}, &jumpTargets_[targetBcIndex]);
        // For JumpLoop, set the predecessor id to PredecessorsSoFar() of the target loop header
        block->SetPredecessorId(mergeStates_[targetBcIndex]->PredecessorsSoFar());
    } else {
        block = FinishBlock<JumpVertex>({}, &jumpTargets_[targetBcIndex]);
    }
    MergeCurrentFrameStateTo(block, targetBcIndex);
}

void ArkSteedGraphBuilder::LowerJumpIfTrue()
{
    auto branchBuilder = CreateBranchBuilder(BranchType::TRUE_BRANCH);
    BuildBranchIfTrue(branchBuilder, currentFrameState_->GetAcc());
}

void ArkSteedGraphBuilder::LowerJumpIfFalse()
{
    auto branchBuilder = CreateBranchBuilder(BranchType::FALSE_BRANCH);
    BuildBranchIfTrue(branchBuilder, currentFrameState_->GetAcc());
}

void ArkSteedGraphBuilder::LowerIsTrueOrFalse(bool isTrue)
{
    ValueVertex *glue = GetGlue();
    ValueVertex *value = currentFrameState_->GetAcc();
    if (isTrue) {
        currentFrameState_->SetAcc(NewCommonStubCall({glue, value}, CommonStubCSigns::ToBooleanTrue));
    } else {
        currentFrameState_->SetAcc(NewCommonStubCall({glue, value}, CommonStubCSigns::ToBooleanFalse));
    }
}

void ArkSteedGraphBuilder::LowerCopyDataProperties()
{
    ValueVertex *target = LoadRegister(0);
    ValueVertex *source = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({target, source}, RTSTUB_ID(CopyDataProperties)));
}

void ArkSteedGraphBuilder::LowerStOwnByValue()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *receiver = LoadRegister(0);
    ValueVertex *key = LoadRegister(1);
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, receiver, key, value, globalEnv}, CommonStubCSigns::StOwnByValue);
}

void ArkSteedGraphBuilder::LowerStOwnByIndex()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *receiver = LoadRegister(0);
    ValueVertex *index = GetInt32Constant(static_cast<int>(GetImmediate(1)));
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, receiver, index, value, globalEnv}, CommonStubCSigns::StOwnByIndex);
}

void ArkSteedGraphBuilder::LowerStOwnByName()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *propKey = GetStringFromConstPool(stringId);
    ValueVertex *receiver = LoadRegister(1);
    ValueVertex *accValue = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, receiver, propKey, accValue, globalEnv}, CommonStubCSigns::StOwnByName);
}

void ArkSteedGraphBuilder::LowerNewLexicalEnv()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *parent = LoadRegister(1);
    ValueVertex *scope = GetInt32Constant(static_cast<int>(GetImmediate(0)));
    ValueVertex *newEnv = NewCommonStubCall({glue, parent, scope}, CommonStubCSigns::NewLexicalEnv);
    currentFrameState_->SetAcc(newEnv);
    currentFrameState_->SetEnv(newEnv);
}

void ArkSteedGraphBuilder::LowerNewLexicalEnvWithName()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *level = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    ValueVertex *slotId = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(1)));
    ValueVertex *newEnv = NewVertex<CallRuntimeVertex>(
        {level, slotId, LoadRegister(2), jsFunc},  // 2: env register index
        RTSTUB_ID(OptNewLexicalEnvWithName));
    currentFrameState_->SetAcc(newEnv);
    currentFrameState_->SetEnv(newEnv);
}

void ArkSteedGraphBuilder::LowerPopLexicalEnv()
{
    ValueVertex *currentEnv = LoadRegister(0);
    ValueVertex *parentEnv = GetValueFromTaggedArray(currentEnv, LexicalEnv::PARENT_ENV_INDEX);
    currentFrameState_->SetAcc(parentEnv);
    currentFrameState_->SetEnv(parentEnv);
}

void ArkSteedGraphBuilder::LowerLdSuperByValue()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *thisObj = LoadRegister(0);
    ValueVertex *propKey = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({thisObj, propKey, jsFunc}, RTSTUB_ID(OptLdSuperByValue)));
}

void ArkSteedGraphBuilder::LowerStSuperByValue()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *thisObj = LoadRegister(0);
    ValueVertex *propKey = LoadRegister(1);
    ValueVertex *value = currentFrameState_->GetAcc();
    NewVertex<CallRuntimeVertex>({thisObj, propKey, value, jsFunc}, RTSTUB_ID(OptStSuperByValue));
}

void ArkSteedGraphBuilder::LowerTryStGlobalByName()
{
    ValueVertex *id = GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithICPreserveAcc(CommonStubCSigns::TryStGlobalByName, {id, value, globalEnv});
}

void ArkSteedGraphBuilder::LowerStConstToGlobalRecord(bool isConst)
{
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *propKey = GetStringFromConstPool(stringId);
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *isConstGate = isConst ? GetTaggedConstant(JSTaggedValue::True().GetRawData())
                                       : GetTaggedConstant(JSTaggedValue::False().GetRawData());
    NewVertex<CallRuntimeVertex>({propKey, value, isConstGate}, RTSTUB_ID(StGlobalRecord));
}

void ArkSteedGraphBuilder::LowerStOwnByValueWithNameSet()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *receiver = LoadRegister(0);
    ValueVertex *propKey = LoadRegister(1);
    ValueVertex *accValue = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, receiver, propKey, accValue, globalEnv}, CommonStubCSigns::StOwnByValueWithNameSet);
}

void ArkSteedGraphBuilder::LowerStOwnByNameWithNameSet()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *propKey = GetStringFromConstPool(stringId);
    ValueVertex *receiver = LoadRegister(1);
    ValueVertex *accValue = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, receiver, propKey, accValue, globalEnv}, CommonStubCSigns::StOwnByNameWithNameSet);
}

void ArkSteedGraphBuilder::LowerLdGlobalVar()
{
    ValueVertex *id = GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithIC(CommonStubCSigns::LdGlobalVar, {id, globalEnv});
}

void ArkSteedGraphBuilder::LowerDefineGetterSetterByValue()
{
    ValueVertex *obj = LoadRegister(0);
    ValueVertex *prop = LoadRegister(1);
    ValueVertex *getter = LoadRegister(2);  // 2: getter register index
    ValueVertex *setter = LoadRegister(3);  // 3: setter register index
    ValueVertex *acc = currentFrameState_->GetAcc();
    ValueVertex *undefinedValue = GetRootConstant(RootConstantVertex::RootIndex::UNDEFINED);
    ValueVertex *taggedOne = NewTaggedVertexFromRawInt32(1);
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({obj, prop, getter, setter, acc, undefinedValue, taggedOne},
                                                            RTSTUB_ID(DefineGetterSetterByValue)));
}

void ArkSteedGraphBuilder::LowerLdThisByValue()
{
    ValueVertex *receiver = currentFrameState_->GetParam(THIS_OBJECT_PARAM_INDEX);
    ValueVertex *key = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithIC(CommonStubCSigns::GetPropertyByValue, {receiver, key, globalEnv});
}

void ArkSteedGraphBuilder::LowerStThisByValue()
{
    ValueVertex *receiver = currentFrameState_->GetParam(THIS_OBJECT_PARAM_INDEX);
    ValueVertex *key = LoadRegister(1);
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithICPreserveAcc(CommonStubCSigns::SetPropertyByValue, {receiver, key, value, globalEnv});
}

void ArkSteedGraphBuilder::LowerLdSuperByName()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *thisObj = currentFrameState_->GetAcc();
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *prop = GetStringFromConstPool(stringId);
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({thisObj, prop, jsFunc}, RTSTUB_ID(OptLdSuperByValue)));
}

void ArkSteedGraphBuilder::LowerStSuperByName()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *thisObj = LoadRegister(1);
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(0)));
    ValueVertex *prop = GetStringFromConstPool(stringId);
    NewVertex<CallRuntimeVertex>({thisObj, prop, value, jsFunc}, RTSTUB_ID(OptStSuperByValue));
}

void ArkSteedGraphBuilder::LowerLdLexVar()
{
    ValueVertex *level = GetInt32Constant(static_cast<int>(GetImmediate(0)));
    ValueVertex *slot = GetInt32Constant(static_cast<int>(GetImmediate(1)));
    ValueVertex *lexicalEnv = LoadRegister(2);  // 2: lexicalEnv register index
    ValueVertex *glue = GetGlue();
    currentFrameState_->SetAcc(NewCommonStubCall({glue, level, slot, lexicalEnv}, CommonStubCSigns::LdLexVar));
}

void ArkSteedGraphBuilder::LowerStLexVar()
{
    ValueVertex *level = GetInt32Constant(static_cast<int>(GetImmediate(0)));
    ValueVertex *slot = GetInt32Constant(static_cast<int>(GetImmediate(1)));
    ValueVertex *lexicalEnv = LoadRegister(2);  // 2: lexicalEnv register index
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *glue = GetGlue();
    NewCommonStubCall({glue, level, slot, lexicalEnv, value}, CommonStubCSigns::StLexVar);
}

void ArkSteedGraphBuilder::LowerDefineClassWithBuffer()
{
    // Bytecode format: ID16_ID16_ID16_IMM16_V8 (methodId, literalId, length, proto, lexicalEnv, slotId)
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *methodId = NewTaggedVertexFromRawInt32(static_cast<int>(GetConstDataId(0)));
    ValueVertex *literalId = NewTaggedVertexFromRawInt32(static_cast<int>(GetConstDataId(1)));
    ValueVertex *length = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(2)));  // 2: length operand index
    ValueVertex *proto = LoadRegister(3);  // 3: proto register index
    ValueVertex *lexicalEnv = LoadRegister(4);  // 4: lexicalEnv register index
    ValueVertex *slotId = NewTaggedVertexFromRawInt32(static_cast<int>(GetICSlotId(5)));  // 5: slotId operand index
    ValueVertex *sharedConstPool = GetSharedConstPool();
    ValueVertex *module = GetModuleFromFunction();

    std::vector<ValueVertex *> args = {
        proto,
        lexicalEnv,
        sharedConstPool,
        methodId,
        literalId,
        module,
        length,
#if ECMASCRIPT_ENABLE_IC
        slotId,
        jsFunc
#endif
    };
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>(args, RTSTUB_ID(CreateClassWithBuffer)));
}

void ArkSteedGraphBuilder::LowerDefineFunc()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *slotId = GetInt32Constant(static_cast<int>(GetICSlotId(0)));
    ValueVertex *methodId = GetInt32Constant(static_cast<int>(GetConstDataId(1)));
    ValueVertex *length = GetInt32Constant(static_cast<int>(GetImmediate(2)));  // 2: length operand index
    ValueVertex *lexicalEnv = LoadRegister(3);  // 3: lexicalEnv register index
    ValueVertex *globalEnv = GetGlobalEnv();

    currentFrameState_->SetAcc(NewCommonStubCall({glue, jsFunc, methodId, length, lexicalEnv, slotId, globalEnv},
                                                 CommonStubCSigns::Definefunc));
}

void ArkSteedGraphBuilder::LowerCopyRestArgs()
{
    ValueVertex *actualArgc = GetActualArgc();
    ValueVertex *taggedArgc = NewVertex<ToTaggedIntVertex>({actualArgc});
    ValueVertex *taggedRestIdx = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({taggedArgc, taggedRestIdx}, RTSTUB_ID(OptCopyRestArgs)));
}

void ArkSteedGraphBuilder::LowerLdPatchVar()
{
    ValueVertex *index = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({index}, RTSTUB_ID(LdPatchVar)));
}

void ArkSteedGraphBuilder::LowerStPatchVar()
{
    ValueVertex *index = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    ValueVertex *value = currentFrameState_->GetAcc();
    NewVertex<CallRuntimeVertex>({index, value}, RTSTUB_ID(StPatchVar));
}

void ArkSteedGraphBuilder::LowerLdLocalModuleVar()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *index = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({index, jsFunc}, RTSTUB_ID(LdLocalModuleVarByIndexOnJSFunc)));
}

void ArkSteedGraphBuilder::LowerLdThisByName()
{
    ValueVertex *receiver = currentFrameState_->GetParam(THIS_OBJECT_PARAM_INDEX);
    ValueVertex *id = GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithIC(CommonStubCSigns::GetPropertyByName, {receiver, id, globalEnv});
}

void ArkSteedGraphBuilder::LowerStThisByName()
{
    ValueVertex *receiver = currentFrameState_->GetParam(THIS_OBJECT_PARAM_INDEX);
    ValueVertex *id = GetIntPtrConstant(static_cast<intptr_t>(GetConstDataId(1)));
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    LowerCallStubWithICPreserveAcc(CommonStubCSigns::SetPropertyByName, {receiver, id, value, globalEnv});
}

void ArkSteedGraphBuilder::LowerLdPrivateProperty()
{
    ValueVertex *levelIndex = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(1)));
    ValueVertex *slotIndex = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(2)));  // 2: slot operand index
    ValueVertex *lexicalEnv = LoadRegister(3);  // 3: lexicalEnv register index
    ValueVertex *obj = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(
        NewVertex<CallRuntimeVertex>({lexicalEnv, levelIndex, slotIndex, obj}, RTSTUB_ID(LdPrivateProperty)));
}

void ArkSteedGraphBuilder::LowerStPrivateProperty()
{
    ValueVertex *levelIndex = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(1)));
    ValueVertex *slotIndex = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(2)));  // 2: slot operand index
    ValueVertex *obj = LoadRegister(3);  // 3: obj register index
    ValueVertex *lexicalEnv = LoadRegister(4);  // 4: lexicalEnv register index
    ValueVertex *value = currentFrameState_->GetAcc();
    NewVertex<CallRuntimeVertex>({lexicalEnv, levelIndex, slotIndex, obj, value}, RTSTUB_ID(StPrivateProperty));
}

void ArkSteedGraphBuilder::LowerNotifyConcurrentResult()
{
    ValueVertex *jsFunc = currentFrameState_->GetParam(CALL_TARGET_PARAM_INDEX);
    ValueVertex *result = currentFrameState_->GetAcc();
    NewVertex<CallRuntimeVertex>({result, jsFunc}, RTSTUB_ID(NotifyConcurrentResult));
}

void ArkSteedGraphBuilder::LowerDefinePropertyByName()
{
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(1)));
    ValueVertex *prop = GetStringFromConstPool(stringId);
    ValueVertex *obj = LoadRegister(2);  // 2: obj register index
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *glue = GetGlue();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, obj, prop, value, globalEnv}, CommonStubCSigns::DefineField);
}

void ArkSteedGraphBuilder::LowerDefineFieldByName()
{
    ValueVertex *stringId = GetInt32Constant(static_cast<int>(GetConstDataId(1)));
    ValueVertex *prop = GetStringFromConstPool(stringId);
    ValueVertex *obj = LoadRegister(2);  // 2: obj register index
    ValueVertex *value = currentFrameState_->GetAcc();
    ValueVertex *glue = GetGlue();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, obj, prop, value, globalEnv}, CommonStubCSigns::DefineField);
}

void ArkSteedGraphBuilder::LowerDefineFieldByValue()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *receiver = LoadRegister(1);
    ValueVertex *propKey = LoadRegister(0);
    ValueVertex *acc = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, receiver, propKey, acc, globalEnv}, CommonStubCSigns::DefineField);
}

void ArkSteedGraphBuilder::LowerDefineFieldByIndex()
{
    ValueVertex *glue = GetGlue();
    ValueVertex *receiver = LoadRegister(1);
    ValueVertex *propKey = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    ValueVertex *acc = currentFrameState_->GetAcc();
    ValueVertex *globalEnv = GetGlobalEnv();
    NewCommonStubCall({glue, receiver, propKey, acc, globalEnv}, CommonStubCSigns::DefineField);
}

void ArkSteedGraphBuilder::LowerToPropertyKey()
{
    ValueVertex *value = currentFrameState_->GetAcc();
    currentFrameState_->SetAcc(NewVertex<CallRuntimeVertex>({value}, RTSTUB_ID(ToPropertyKey)));
}

void ArkSteedGraphBuilder::LowerCreatePrivateProperty()
{
    ValueVertex *count = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    ValueVertex *literalId = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(1)));
    ValueVertex *lexicalEnv = LoadRegister(2);  // 2: lexicalEnv register index
    ValueVertex *constpool = GetSharedConstPool();
    ValueVertex *module = GetModuleFromFunction();

    NewVertex<CallRuntimeVertex>({lexicalEnv, count, constpool, literalId, module}, RTSTUB_ID(CreatePrivateProperty));
}

void ArkSteedGraphBuilder::LowerDefinePrivateProperty()
{
    ValueVertex *levelIndex = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(0)));
    ValueVertex *slotIndex = NewTaggedVertexFromRawInt32(static_cast<int>(GetImmediate(1)));
    ValueVertex *obj = LoadRegister(2);  // 2: obj register index
    ValueVertex *lexicalEnv = LoadRegister(3);  // 3: lexicalEnv register index
    ValueVertex *value = currentFrameState_->GetAcc();
    NewVertex<CallRuntimeVertex>({lexicalEnv, levelIndex, slotIndex, obj, value}, RTSTUB_ID(DefinePrivateProperty));
}

void ArkSteedGraphBuilder::MergeCurrentFrameStateTo(BB *predecessor, uint32_t destIndex)
{
    if (mergeStates_[destIndex] == nullptr) {
        const LivenessBitSet *liveness = GetInLivenessFor(destIndex);
        uint32_t predCount = PredecessorCount(destIndex);
        ASSERT(predCount > 0);
        mergeStates_[destIndex] = MergePointFrameState::New(destIndex, predCount, liveness, GetChunk());
    }
    mergeStates_[destIndex]->MergeFrom(*currentFrameState_, predecessor);
}

BB *ArkSteedGraphBuilder::StartNewBlockWithMergeState(MergePointFrameState *mergeState, BBRef *ref, BB **blockSlot)
{
    ASSERT(mergeState != nullptr);
    ASSERT(ref != nullptr);
    ASSERT(blockSlot != nullptr);

    BB *block = *blockSlot;
    if (block == nullptr) {
        uint32_t numPreds = mergeState->PredecessorCount();
        block = BB::New(GetChunk());
        block->SetPredecessorCount(numPreds);
        block->SetRegisterMergeState(&mergeState->RegisterState());
        if (mergeState->IsLoopHeader()) {
            block->SetLoopHeader(true);
        }
        if (mergeState->IsExceptionHandler()) {
            block->SetExceptionHandler(true);
        }
        *blockSlot = block;
        ref->Bind(block);
    }

    SetCurrentBlock(block);
    for (PhiVertex *phi : mergeState->Phis()) {
        if (phi->Vertex::GetOwner() == nullptr) {
            phi->SetOwner(block);
            block->GetPhis().push_back(phi);
            RegisterVertexWithLabeller(phi);
        }
    }
    return block;
}

ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::Label::Label(ArkSteedSubGraphBuilder *subBuilder,
                                                            uint32_t predecessorCount)
    : subBuilder_(subBuilder),
      predecessorCount_(predecessorCount),
      mergeLiveSet_(subBuilder->builder_->GetChunk()->New<LivenessBitSet>(
          subBuilder->builder_->GetChunk(), subBuilder->numLocal_, subBuilder->numParams_))
{
}

ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::Label::Label(ArkSteedSubGraphBuilder *subBuilder,
                                                            uint32_t predecessorCount,
                                                            std::initializer_list<SubGraphVariable *> liveVariables)
    : Label(subBuilder, predecessorCount)
{
    for (SubGraphVariable *var : liveVariables) {
        ASSERT(var != nullptr);
        mergeLiveSet_->Set(var->pseudoRegister_);
    }
}

ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::ArkSteedSubGraphBuilder(ArkSteedGraphBuilder *builder, int variableCount)
    : builder_(builder),
      numLocal_(CheckedSubGraphVariableCount(variableCount)),
      numParams_(0),
      subGraphFrame_(builder->GetChunk()->New<InterpreterFrameState>(numLocal_, numParams_, builder->GetChunk()))
{
    InterpreterFrameState *parentFrame = builder_->CurrentFrameState();
    ASSERT(parentFrame != nullptr);
    subGraphFrame_->SetEnv(parentFrame->GetEnv());
    subGraphFrame_->SetAcc(parentFrame->GetAcc());
}

void ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::MergeIntoLabel(Label *label, BB *predecessor)
{
    ASSERT(label != nullptr);
    ASSERT(label->subBuilder_ == this);
    ASSERT(predecessor != nullptr);

    if (label->variableMergeState_ == nullptr) {
        label->variableMergeState_ = MergePointFrameState::New(
            BytecodeContext::INVALID_BC_INDEX, label->predecessorCount_, label->mergeLiveSet_, builder_->GetChunk());
    }
    label->variableMergeState_->MergeFrom(*subGraphFrame_, predecessor);
}

void ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::TrimUnmergedPredecessors(Label *label, uint32_t num)
{
    ASSERT(label != nullptr);
    ASSERT(label->subBuilder_ == this);
    ASSERT(num <= label->predecessorCount_);

    if (num == 0) {
        return;
    }

    label->predecessorCount_ -= num;
    if (label->variableMergeState_ != nullptr) {
        label->variableMergeState_->ReducePredecessorCount(num);
    }
}

void ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::Goto(Label *label)
{
    ASSERT(builder_->CurrentBlock() != nullptr);
    BB *predecessor = builder_->FinishBlock<JumpVertex>({}, &label->ref_);
    MergeIntoLabel(label, predecessor);
}

void ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::GotoOrTrim(Label *label)
{
    if (builder_->CurrentBlock() == nullptr) {
        TrimUnmergedPredecessors(label);
        return;
    }
    Goto(label);
}

void ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::Bind(Label *label)
{
    ASSERT(label != nullptr);
    ASSERT(label->subBuilder_ == this);
    ASSERT(builder_->CurrentBlock() == nullptr);
    ASSERT(label->variableMergeState_ != nullptr);
    ASSERT(label->variableMergeState_->PredecessorsSoFar() == label->predecessorCount_);

    subGraphFrame_->CopyFrom(*label->variableMergeState_);

    builder_->StartNewBlockWithMergeState(label->variableMergeState_, &label->ref_, &label->block_);
    builder_->ProcessMergePointPredecessors(label->variableMergeState_, &label->ref_, label->block_);
}

ArkSteedGraphBuilder::ReduceResult ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::TrimPredecessorsAndBind(Label *label)
{
    ASSERT(label != nullptr);

    uint32_t predecessorsSoFar =
        label->variableMergeState_ == nullptr ? 0 : label->variableMergeState_->PredecessorsSoFar();
    if (predecessorsSoFar == 0) {
        builder_->SetCurrentBlock(nullptr);
        return ReduceResult::DoneWithAbort();
    }

    ASSERT(predecessorsSoFar <= label->predecessorCount_);
    builder_->SetCurrentBlock(nullptr);
    TrimUnmergedPredecessors(label, label->predecessorCount_ - predecessorsSoFar);
    Bind(label);
    return ReduceResult::Done();
}

ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::LoopLabel ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::BeginLoop(
    std::initializer_list<SubGraphVariable *> loopVars)
{
    constexpr uint32_t kLoopHeaderPredecessorCount = 2;
    Chunk *chunk = builder_->GetChunk();
    auto *loopHeaderRef = chunk->New<BBRef>();
    auto *loopInfo = chunk->New<LoopInfo>(chunk, BytecodeContext::INVALID_BC_INDEX, BytecodeContext::INVALID_BC_INDEX,
                                          numLocal_, numParams_);
    auto *loopHeaderLiveness = chunk->New<LivenessBitSet>(chunk, numLocal_, numParams_);
    for (SubGraphVariable *var : loopVars) {
        ASSERT(var != nullptr);
        loopHeaderLiveness->Set(var->pseudoRegister_);
        loopInfo->AddDef(var->pseudoRegister_);
    }

    BB *loopPredecessor = builder_->FinishBlock<JumpVertex>({}, loopHeaderRef);
    MergePointFrameState *loopState = MergePointFrameState::NewForLoop(
        BytecodeContext::INVALID_BC_INDEX, kLoopHeaderPredecessorCount, loopHeaderLiveness, loopInfo, chunk);
    loopState->MergeFrom(*subGraphFrame_, loopPredecessor);
    BB *loopHeaderBlock = nullptr;
    builder_->StartNewBlockWithMergeState(loopState, loopHeaderRef, &loopHeaderBlock);
    builder_->ProcessMergePointPredecessors(loopState, loopHeaderRef, loopHeaderBlock);
    subGraphFrame_->CopyFrom(*loopState);
    return LoopLabel(this, loopState, loopHeaderRef, loopHeaderBlock);
}

void ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::EndLoop(LoopLabel *loopLabel)
{
    ASSERT(loopLabel != nullptr);
    ASSERT(loopLabel->subBuilder_ == this);
    ASSERT(loopLabel->loopHeaderBlock_ != nullptr);

    if (builder_->CurrentBlock() == nullptr) {
        loopLabel->mergeState_->ReducePredecessorCount();
        loopLabel->mergeState_->ClearIsLoop();
        loopLabel->mergeState_->ClearLoopInfo();
        return;
    }

    BB *loopBackedge = builder_->FinishBlock<JumpLoopVertex>({}, loopLabel->loopHeaderRef_);
    loopLabel->mergeState_->MergeFrom(*subGraphFrame_, loopBackedge);
    ASSERT(loopLabel->mergeState_->PredecessorsSoFar() == loopLabel->mergeState_->PredecessorCount());
    loopBackedge->SetPredecessorId(loopLabel->mergeState_->PredecessorCount() - 1);
}

ArkSteedGraphBuilder::ReduceResult ArkSteedGraphBuilder::ArkSteedSubGraphBuilder::Branch(
    std::initializer_list<SubGraphVariable *> vars, CallbackRef<BranchResult(BranchBuilder &)> cond,
    CallbackRef<ReduceResult()> ifTrue, CallbackRef<ReduceResult()> ifFalse)
{
    constexpr uint32_t kBinaryBranchPredecessorCount = 2;

    Label elseBranch(this, 1);
    BranchBuilder branchBuilder(builder_, this, BranchType::FALSE_BRANCH, &elseBranch);
    BranchResult branchResult = cond(branchBuilder);
    switch (branchResult) {
        case BranchResult::ALWAYS_TRUE:
            return ifTrue();
        case BranchResult::ALWAYS_FALSE:
            return ifFalse();
        case BranchResult::ABORT:
            return ReduceResult::DoneWithAbort();
        case BranchResult::DEFAULT:
            break;
    }

    Label done(this, kBinaryBranchPredecessorCount, vars);
    ReduceResult trueResult = ifTrue();
    GotoOrTrim(&done);

    Bind(&elseBranch);
    ReduceResult falseResult = ifFalse();
    if (trueResult.IsDoneWithAbort() && falseResult.IsDoneWithAbort()) {
        return ReduceResult::DoneWithAbort();
    }

    GotoOrTrim(&done);
    ReduceResult bindResult = TrimPredecessorsAndBind(&done);
    if (bindResult.IsDoneWithAbort()) {
        return bindResult;
    }
    return ReduceResult::Done();
}

ArkSteedGraphBuilder::ReduceResult ArkSteedGraphBuilder::Select(CallbackRef<BranchResult(BranchBuilder &)> cond,
                                                                CallbackRef<ReduceResult()> ifTrue,
                                                                CallbackRef<ReduceResult()> ifFalse)
{
    constexpr uint32_t kBinaryBranchPredecessorCount = 2;

    ArkSteedSubGraphBuilder subGraph(this, 1);
    ArkSteedSubGraphBuilder::SubGraphVariable resultVar(0);
    ArkSteedSubGraphBuilder::Label elseBranch(&subGraph, 1);
    BranchBuilder branchBuilder(this, &subGraph, BranchType::FALSE_BRANCH, &elseBranch);
    BranchResult branchResult = cond(branchBuilder);
    switch (branchResult) {
        case BranchResult::ALWAYS_TRUE:
            return ifTrue();
        case BranchResult::ALWAYS_FALSE:
            return ifFalse();
        case BranchResult::ABORT:
            return ReduceResult::DoneWithAbort();
        case BranchResult::DEFAULT:
            break;
    }

    ArkSteedSubGraphBuilder::Label done(&subGraph, kBinaryBranchPredecessorCount, {&resultVar});
    auto completeArm = [&subGraph, &resultVar, &done](const ReduceResult &result) {
        if (result.IsDoneWithValue()) {
            subGraph.Set(resultVar, result.Value());
        }
        subGraph.GotoOrTrim(&done);
        return result.IsDoneWithAbort();
    };

    bool truePathAborted = completeArm(ifTrue());
    subGraph.Bind(&elseBranch);
    bool falsePathAborted = completeArm(ifFalse());
    if (truePathAborted && falsePathAborted) {
        return ReduceResult::DoneWithAbort();
    }
    subGraph.Bind(&done);
    return ReduceResult::Done(subGraph.Get(resultVar));
}

uint32_t ArkSteedGraphBuilder::PredecessorCount(uint32_t index) const
{
    ASSERT(index < static_cast<uint32_t>(bytecodeContext_.GetPredecessorCount().size()));
    ASSERT(index < static_cast<uint32_t>(predecessorCountReductions_.size()));
    uint32_t staticPredecessorCount = bytecodeContext_.GetPredecessorCount()[index];
    ASSERT(predecessorCountReductions_[index] <= staticPredecessorCount);
    return staticPredecessorCount - predecessorCountReductions_[index];
}

void ArkSteedGraphBuilder::ReduceBytecodePredecessorCount(uint32_t index, uint32_t num)
{
    if (num == 0) {
        return;
    }

    ASSERT(index < static_cast<uint32_t>(predecessorCountReductions_.size()));
    ASSERT(num <= PredecessorCount(index));

    predecessorCountReductions_[index] += num;
    if (mergeStates_[index] != nullptr) {
        mergeStates_[index]->ReducePredecessorCount(num);
    }
}

void ArkSteedGraphBuilder::MarkDeadLoopBackedge(uint32_t index)
{
    if (mergeStates_[index] == nullptr && PredecessorCount(index) == 0) {
        return;
    }

    ASSERT(PredecessorCount(index) > 0);
    if (mergeStates_[index] != nullptr) {
        ASSERT(mergeStates_[index]->IsLoopHeader());
        ASSERT(mergeStates_[index]->PredecessorCount() == PredecessorCount(index));
        ASSERT(mergeStates_[index]->PredecessorsSoFar() + 1 == mergeStates_[index]->PredecessorCount());
        mergeStates_[index]->ReducePredecessorCount();
        mergeStates_[index]->ClearIsLoop();
        mergeStates_[index]->ClearLoopInfo();
    }
    predecessorCountReductions_[index]++;
}

void ArkSteedGraphBuilder::MarkDeadPredecessorsForSuccessors(uint32_t index, const BytecodeInfo &bytecodeInfo)
{
    if (bytecodeInfo.IsCondJump()) {
        uint32_t jumpTarget = bytecodeContext_.GetJumpTargetBcIndex(index);
        uint32_t fallthrough = index + 1;
        ReduceBytecodePredecessorCount(jumpTarget);
        if (fallthrough < bytecodeContext_.GetBytecodeCount() && fallthrough != jumpTarget) {
            ReduceBytecodePredecessorCount(fallthrough);
        }
        return;
    }

    if (bytecodeInfo.IsJump()) {
        uint32_t jumpTarget = bytecodeContext_.GetJumpTargetBcIndex(index);
        if (bytecodeContext_.GetJumpLoop()[index]) {
            MarkDeadLoopBackedge(jumpTarget);
        } else {
            ReduceBytecodePredecessorCount(jumpTarget);
        }
        return;
    }

    if (bytecodeInfo.needFallThrough()) {
        uint32_t nextIndex = index + 1;
        if (nextIndex < bytecodeContext_.GetBytecodeCount()) {
            ReduceBytecodePredecessorCount(nextIndex);
        }
    }
}

void ArkSteedGraphBuilder::StartNewBlockWithMergeState(uint32_t index)
{
#ifndef NDEBUG
    LOG_COMPILER(DEBUG) << "Creating new basic block at bytecode #" << index << " (with merge state)";
#endif
    ASSERT(mergeStates_[index] != nullptr);

    BB *current = nullptr;
    StartNewBlockWithMergeState(mergeStates_[index], &jumpTargets_[index], &current);

    // For all virtual registers not in LiveIn(B) where B is current basic block,
    // it is either defined in B (which will be updated to currentFrameState_ during bytecode visiting) or
    // ignored.
    mergeStates_[index]->FrameState().ForEach([this](ValueVertex *vertex, VirtualRegister reg) {
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "\tLive-in virtual register " << VRegDisplayString(reg) << " -> Vertex "
                            << GetCurrentGraphLabeller()->GetVertexInputLabel(vertex);
#endif
        currentFrameState_->Set(reg, vertex);
    });

    ProcessMergePointPredecessors(mergeStates_[index], &jumpTargets_[index], current);
}

void ArkSteedGraphBuilder::ProcessMergePointPredecessors(MergePointFrameState *mergeState, BBRef *ref, BB *mergeBlock)
{
    ASSERT(mergeState != nullptr);
    ASSERT(ref != nullptr);
    ASSERT(mergeBlock != nullptr);

    uint32_t numPreds = mergeState->PredecessorCount();
    if (numPreds <= 1) {
        return;
    }
    if (mergeState->IsExceptionHandler()) {
        LOG_COMPILER(WARN) << "to do: Exception handlers are not supported now.";
    } else if (mergeState->IsLoopHeader()) {
        // 1 : The loop edge shall be guaranteed to be an unconditional jump.
        ASSERT(mergeState->PredecessorsSoFar() == numPreds - 1);
        for (uint32_t i = 0; i + 1 < numPreds; i++) {
            TrySplitCriticalEdge(mergeState, ref, mergeBlock, i);
        }
    } else {
        ASSERT(mergeState->PredecessorsSoFar() == numPreds);
        for (uint32_t i = 0; i < numPreds; i++) {
            TrySplitCriticalEdge(mergeState, ref, mergeBlock, i);
        }
    }
}

void ArkSteedGraphBuilder::TrySplitCriticalEdge(MergePointFrameState *mergeState, BBRef *ref, BB *mergeBlock,
                                                uint32_t predIndex)
{
    ASSERT(mergeState != nullptr);
    ASSERT(ref != nullptr);
    ASSERT(mergeBlock != nullptr);

    BB *predecessor = mergeState->PredecessorAt(predIndex);
    BranchIfTrueVertex *branchIf = predecessor->GetControlVertex()->TryCast<BranchIfTrueVertex>();
    if (branchIf == nullptr) {
        predecessor->SetPredecessorId(predIndex);
        return;  // A basic block has multiple successors (excluding catch blocks) only if it's
                 // BranchIfTrueVertex.
    }
    // to do: State initialization
    RegisterMergeState *state = GetChunk()->New<RegisterMergeState>();
    BB *splitBlock = BB::New(GetChunk());
    splitBlock->SetPredecessorCount(1);
    splitBlock->SetRegisterMergeState(state);
    GetGraph()->Add(splitBlock);

    JumpVertex *splitJump = Vertex::New<JumpVertex>(GetChunk(), {}, ref->BlockRef());
    splitJump->SetOwner(splitBlock);
    splitBlock->SetControlVertex(splitJump);
    RegisterVertexWithLabeller(splitJump);

    splitBlock->AddPredecessor(predecessor);
    splitBlock->SetPredecessorId(predIndex);
    mergeState->SetPredecessorAt(predIndex, splitBlock);
    ASSERT(!!(branchIf->IfTrue() == mergeBlock) + !!(branchIf->IfFalse() == mergeBlock) == 1);
    if (branchIf->IfTrue() == mergeBlock) {
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "Creates edge-split block at predecessor #" << predIndex << " of merge block #"
                            << mergeBlock->GetId() << " (splitting true-branch)";
#endif
        branchIf->SetIfTrue(splitBlock);
    } else if (branchIf->IfFalse() == mergeBlock) {
#ifndef NDEBUG
        LOG_COMPILER(DEBUG) << "Creates edge-split block at predecessor #" << predIndex << " of merge block #"
                            << mergeBlock->GetId() << " (splitting false-branch)";
#endif
        branchIf->SetIfFalse(splitBlock);
    }
}

void ArkSteedGraphBuilder::BranchBuilder::StartFallthroughBlock(BB *predecessor)
{
    switch (GetMode()) {
        case Mode::JUMP_BYTECODE_TARGET: {
            auto &data = data_.bytecodeTarget;
            builder_->MergeCurrentFrameStateTo(predecessor, data.jumpTargetBcIndex);
            builder_->MergeCurrentFrameStateTo(predecessor, data.fallthroughBcIndex);
            break;
        }
        case Mode::JUMP_LABEL_TARGET: {
            auto &data = data_.labelTarget;
            subBuilder_->MergeIntoLabel(data.jumpLabel, predecessor);
            if (data.fallthroughBlock == nullptr) {
                data.fallthroughBlock = BB::New(builder_->GetChunk());
                data.fallthroughBlock->AddPredecessor(predecessor);
                data.fallthroughTarget.Bind(data.fallthroughBlock);
            }
            builder_->SetCurrentBlock(data.fallthroughBlock);
            break;
        }
    }
}

BBRef *ArkSteedGraphBuilder::BranchBuilder::JumpTarget()
{
    switch (GetMode()) {
        case Mode::JUMP_BYTECODE_TARGET: {
            return &builder_->jumpTargets_[data_.bytecodeTarget.jumpTargetBcIndex];
        }
        case Mode::JUMP_LABEL_TARGET:
            return &data_.labelTarget.jumpLabel->ref_;
    }
}

BBRef *ArkSteedGraphBuilder::BranchBuilder::FallThrough()
{
    switch (GetMode()) {
        case Mode::JUMP_BYTECODE_TARGET: {
            return &builder_->jumpTargets_[data_.bytecodeTarget.fallthroughBcIndex];
        }
        case Mode::JUMP_LABEL_TARGET:
            return &data_.labelTarget.fallthroughTarget;
    }
}

BBRef *ArkSteedGraphBuilder::BranchBuilder::TrueTarget()
{
    return GetCurrentBranchType() == BranchType::TRUE_BRANCH ? JumpTarget() : FallThrough();
}

BBRef *ArkSteedGraphBuilder::BranchBuilder::FalseTarget()
{
    return GetCurrentBranchType() == BranchType::FALSE_BRANCH ? JumpTarget() : FallThrough();
}

void ArkSteedGraphBuilder::BuildThrow(kungfu::RuntimeStubCSigns::ID id, ValueVertex *input)
{
    bool hasInput = (input != nullptr);
    if (!hasInput) {
        input = GetInt32Constant(0);
    }
    FinishBlock<ThrowVertex>({input}, id, hasInput);
}
}  // namespace panda::ecmascript::arksteed
