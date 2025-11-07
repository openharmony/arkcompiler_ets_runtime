/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/codegen/llvm/llvm_ir_builder.h"


#include "ecmascript/deoptimizer/deoptimizer.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "llvm/IR/IRBuilder.h"

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif


namespace panda::ecmascript::kungfu {
LLVMIRBuilder::LLVMIRBuilder(const std::vector<std::vector<GateRef>> *schedule, Circuit *circuit, LLVMModule *module,
                             LLVMValueRef function, const CompilationConfig *cfg, CallSignature::CallConv callConv,
                             bool enableLog, bool isFastCallAot, const std::string &funcName, bool enableOptDirectCall,
                             bool enableOptInlining, bool enableBranchProfiling, bool isStwCopyStub)
    : compCfg_(cfg),
      scheduledGates_(schedule),
      circuit_(circuit),
      acc_(circuit),
      glue_(acc_.GetGlueFromArgList()),
      module_(module->GetModule()),
      function_(function),
      llvmModule_(module),
      callConv_(callConv),
      enableLog_(enableLog),
      isFastCallAot_(isFastCallAot),
      enableOptDirectCall_(enableOptDirectCall),
      enableOptInlining_(enableOptInlining),
      enableOptBranchProfiling_(enableBranchProfiling),
      isStwCopyStub_(isStwCopyStub)
{
    ASSERT(compCfg_->Is64Bit());
    context_ = module->GetContext();
    builder_ = LLVMCreateBuilderInContext(context_);
    bbID2BB_.clear();
    SetFunctionCallConv();
    InitializeHandlers();

    LLVMSetGC(function_, "statepoint-example");
    slotSize_ = sizeof(uint64_t);
    slotType_ = GetInt64T();
    LLVMMetadataRef dFile = llvmModule_->GetDFileMD();
    LLVMMetadataRef funcTyMD = GetFunctionTypeMD(dFile);
    size_t funcOffset = 0;
    if (IsLogEnabled()) {
        dFuncMD_ = LLVMDIBuilderCreateFunction(GetDIBuilder(), dFile, funcName.c_str(), funcName.size(),
                                               funcName.c_str(), funcName.size(), dFile, funcOffset,
                                               funcTyMD, true, true, 0, LLVMDIFlags::LLVMDIFlagZero, false);
        LLVMSetSubprogram(function_, dFuncMD_);
    }
    std::string triple = LLVMGetTarget(module->GetModule());
    ASSERT(GlobalTargetBuilders().count(triple) && "unsupported target");
    targetBuilder_ = GlobalTargetBuilders()[triple]();
    ASMBarrierCall_ = targetBuilder_->GetASMBarrierCall(module, enableOptDirectCall_);
    const char* attrName = "no-builtin-memset";
    const char* attrValue = "";
    LLVMAddAttributeAtIndex(
        function_,
        LLVMAttributeFunctionIndex,
        LLVMCreateStringAttribute(context_, attrName, strlen(attrName), attrValue, strlen(attrValue)));
}

LLVMMetadataRef LLVMIRBuilder::GetFunctionTypeMD(LLVMMetadataRef dFile)
{
    LLVMDIBuilderRef builder = GetDIBuilder();
    LLVMMetadataRef Int64Ty = LLVMDIBuilderCreateBasicType(builder, "Int64", 5, 64, 0, LLVMDIFlags::LLVMDIFlagZero);
    LLVMMetadataRef paramT[] = { nullptr, Int64Ty }; // need to compute the real types for parameters in the future.
    LLVMMetadataRef funcTy = LLVMDIBuilderCreateSubroutineType(builder, dFile, paramT, 2, LLVMDIFlags::LLVMDIFlagZero);
    return funcTy;
}

LLVMIRBuilder::~LLVMIRBuilder()
{
    if (builder_ != nullptr) {
        LLVMDisposeBuilder(builder_);
        builder_ = nullptr;
    }
    delete targetBuilder_;
}

void LLVMIRBuilder::SetFunctionCallConv()
{
    switch (callConv_) {
        case CallSignature::CallConv::GHCCallConv:
            LLVMSetFunctionCallConv(function_, LLVMGHCCallConv);
            break;
        case CallSignature::CallConv::WebKitJSCallConv: {
            LLVMSetFunctionCallConv(function_, LLVMWebKitJSCallConv);
            break;
        }
        default: {
            LLVMSetFunctionCallConv(function_, LLVMCCallConv);
            callConv_ = CallSignature::CallConv::CCallConv;
            break;
        }
    }
}

int LLVMIRBuilder::FindBasicBlock(GateRef gate) const
{
    for (size_t bbIdx = 0; bbIdx < scheduledGates_->size(); bbIdx++) {
        const std::vector<GateRef>& bb = scheduledGates_->at(bbIdx);
        for (size_t instIdx = bb.size(); instIdx > 0; instIdx--) {
            GateRef tmp = bb[instIdx - 1];
            if (tmp == gate) {
                return bbIdx;
            }
        }
    }
    return -1;
}

void LLVMIRBuilder::InitializeHandlers()
{
    opHandlers_ = {
        {OpCode::STATE_ENTRY, &LLVMIRBuilder::HandleGoto},
        {OpCode::RETURN, &LLVMIRBuilder::HandleReturn},
        {OpCode::RETURN_VOID, &LLVMIRBuilder::HandleReturnVoid},
        {OpCode::IF_BRANCH, &LLVMIRBuilder::HandleBranch},
        {OpCode::SWITCH_BRANCH, &LLVMIRBuilder::HandleSwitch},
        {OpCode::ORDINARY_BLOCK, &LLVMIRBuilder::HandleGoto},
        {OpCode::IF_TRUE, &LLVMIRBuilder::HandleGoto},
        {OpCode::IF_FALSE, &LLVMIRBuilder::HandleGoto},
        {OpCode::SWITCH_CASE, &LLVMIRBuilder::HandleGoto},
        {OpCode::MERGE, &LLVMIRBuilder::HandleGoto},
        {OpCode::DEFAULT_CASE, &LLVMIRBuilder::HandleGoto},
        {OpCode::LOOP_BEGIN, &LLVMIRBuilder::HandleGoto},
        {OpCode::LOOP_BACK, &LLVMIRBuilder::HandleGoto},
        {OpCode::VALUE_SELECTOR, &LLVMIRBuilder::HandlePhi},
        {OpCode::ASM_CALL_BARRIER, &LLVMIRBuilder::HandleCall},
        {OpCode::RUNTIME_CALL, &LLVMIRBuilder::HandleRuntimeCall},
        {OpCode::RUNTIME_CALL_WITH_ARGV, &LLVMIRBuilder::HandleRuntimeCallWithArgv},
        {OpCode::NOGC_RUNTIME_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::CALL_OPTIMIZED, &LLVMIRBuilder::HandleCall},
        {OpCode::FAST_CALL_OPTIMIZED, &LLVMIRBuilder::HandleCall},
        {OpCode::CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::BASELINE_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::BYTECODE_CALL, &LLVMIRBuilder::HandleBytecodeCall},
        {OpCode::DEBUGGER_BYTECODE_CALL, &LLVMIRBuilder::HandleBytecodeCall},
        {OpCode::BUILTINS_CALL, &LLVMIRBuilder::HandleCall},
        {OpCode::BUILTINS_CALL_WITH_ARGV, &LLVMIRBuilder::HandleCall},
        {OpCode::ALLOCA, &LLVMIRBuilder::HandleAlloca},
        {OpCode::ARG, &LLVMIRBuilder::HandleParameter},
        {OpCode::CONSTANT, &LLVMIRBuilder::HandleConstant},
        {OpCode::CONSTSTRING, &LLVMIRBuilder::HandleConstString},
        {OpCode::RELOCATABLE_DATA, &LLVMIRBuilder::HandleRelocatableData},
        {OpCode::ZEXT, &LLVMIRBuilder::HandleZExtInt},
        {OpCode::SEXT, &LLVMIRBuilder::HandleSExtInt},
        {OpCode::TRUNC, &LLVMIRBuilder::HandleCastIntXToIntY},
        {OpCode::FEXT, &LLVMIRBuilder::HandleFPExt},
        {OpCode::FTRUNC, &LLVMIRBuilder::HandleFPTrunc},
        {OpCode::REV, &LLVMIRBuilder::HandleIntRev},
        {OpCode::ADD, &LLVMIRBuilder::HandleAdd},
        {OpCode::SUB, &LLVMIRBuilder::HandleSub},
        {OpCode::MUL, &LLVMIRBuilder::HandleMul},
        {OpCode::FDIV, &LLVMIRBuilder::HandleFloatDiv},
        {OpCode::SDIV, &LLVMIRBuilder::HandleIntDiv},
        {OpCode::UDIV, &LLVMIRBuilder::HandleUDiv},
        {OpCode::AND, &LLVMIRBuilder::HandleIntAnd},
        {OpCode::OR, &LLVMIRBuilder::HandleIntOr},
        {OpCode::FETCH_OR, &LLVMIRBuilder::HandleFetchOr},
        {OpCode::XOR, &LLVMIRBuilder::HandleIntXor},
        {OpCode::LSR, &LLVMIRBuilder::HandleIntLsr},
        {OpCode::ASR, &LLVMIRBuilder::HandleIntAsr},
        {OpCode::ICMP, &LLVMIRBuilder::HandleCmp},
        {OpCode::FCMP, &LLVMIRBuilder::HandleCmp},
        {OpCode::LOAD_WITHOUT_BARRIER, &LLVMIRBuilder::HandleLoad},
        {OpCode::STORE_WITHOUT_BARRIER, &LLVMIRBuilder::HandleStore},
        {OpCode::SIGNED_INT_TO_FLOAT, &LLVMIRBuilder::HandleChangeInt32ToDouble},
        {OpCode::UNSIGNED_INT_TO_FLOAT, &LLVMIRBuilder::HandleChangeUInt32ToDouble},
        {OpCode::FLOAT_TO_SIGNED_INT, &LLVMIRBuilder::HandleChangeDoubleToInt32},
        {OpCode::TAGGED_TO_INT64, &LLVMIRBuilder::HandleChangeTaggedPointerToInt64},
        {OpCode::INT64_TO_TAGGED, &LLVMIRBuilder::HandleChangeInt64ToTagged},
        {OpCode::BITCAST, &LLVMIRBuilder::HandleBitCast},
        {OpCode::LSL, &LLVMIRBuilder::HandleIntLsl},
        {OpCode::SMOD, &LLVMIRBuilder::HandleMod},
        {OpCode::FMOD, &LLVMIRBuilder::HandleMod},
        {OpCode::DEOPT_CHECK, &LLVMIRBuilder::HandleDeoptCheck},
        {OpCode::TRUNC_FLOAT_TO_INT64, &LLVMIRBuilder::HandleTruncFloatToInt},
        {OpCode::TRUNC_FLOAT_TO_INT32, &LLVMIRBuilder::HandleTruncFloatToInt},
        {OpCode::ADD_WITH_OVERFLOW, &LLVMIRBuilder::HandleAddWithOverflow},
        {OpCode::SUB_WITH_OVERFLOW, &LLVMIRBuilder::HandleSubWithOverflow},
        {OpCode::MUL_WITH_OVERFLOW, &LLVMIRBuilder::HandleMulWithOverflow},
        {OpCode::EXTRACT_VALUE, &LLVMIRBuilder::HandleExtractValue},
        {OpCode::SQRT, &LLVMIRBuilder::HandleSqrt},
        {OpCode::EXP, &LLVMIRBuilder::HandleExp},
        {OpCode::ABS, &LLVMIRBuilder::HandleAbs},
        {OpCode::MIN, &LLVMIRBuilder::HandleMin},
        {OpCode::MAX, &LLVMIRBuilder::HandleMax},
        {OpCode::CLZ32, &LLVMIRBuilder::HandleClz32},
        {OpCode::DOUBLE_TRUNC, &LLVMIRBuilder::HandleDoubleTrunc},
        {OpCode::CEIL, &LLVMIRBuilder::HandleCeil},
        {OpCode::FLOOR, &LLVMIRBuilder::HandleFloor},
        {OpCode::READSP, &LLVMIRBuilder::HandleReadSp},
        {OpCode::BITREV, &LLVMIRBuilder::HandleBitRev},
        {OpCode::FINISH_ALLOCATE, &LLVMIRBuilder::HandleFinishAllocate},
    };
    illegalOpHandlers_ = {
        OpCode::NOP, OpCode::CIRCUIT_ROOT, OpCode::DEPEND_ENTRY,
        OpCode::DEAD, OpCode::RETURN_LIST,
        OpCode::ARG_LIST, OpCode::THROW,
        OpCode::DEPEND_SELECTOR, OpCode::DEPEND_RELAY,
        OpCode::FRAME_STATE, OpCode::STATE_SPLIT, OpCode::FRAME_ARGS,
        OpCode::LOOP_EXIT_DEPEND, OpCode::LOOP_EXIT,
        OpCode::START_ALLOCATE, OpCode::FRAME_VALUES
    };
}

std::string LLVMIRBuilder::LLVMValueToString(LLVMValueRef val) const
{
    char* msg = LLVMPrintValueToString(val);
    std::string str(msg);
    LLVMDisposeMessage(msg);
    return str;
}

void LLVMIRBuilder::Build()
{
    for (size_t bbIdx = 0; bbIdx < scheduledGates_->size(); bbIdx++) {
        const std::vector<GateRef>& bb = scheduledGates_->at(bbIdx);
        for (size_t instIdx = bb.size(); instIdx > 0; instIdx--) {
            GateId gateId = acc_.GetId(bb[instIdx - 1]);
            instID2bbID_[gateId] = static_cast<int>(bbIdx);
        }
    }

    for (size_t bbIdx = 0; bbIdx < scheduledGates_->size(); bbIdx++) {
        const std::vector<GateRef>& bb = scheduledGates_->at(bbIdx);
        OperandsVector predecessors;
        auto ins = acc_.Ins(bb[0]);
        for (auto i = ins.begin(); i != ins.end(); i++) {
            GateRef r = *i;
            if (!acc_.IsState(r)) {
                continue;
            }
            predecessors.insert(instID2bbID_[acc_.GetId(r)]);
        }
        LinkToLLVMCfg(bbIdx, predecessors);

        for (size_t instIdx = bb.size(); instIdx > 0; instIdx--) {
            GateRef gate = bb[instIdx - 1];
            auto found = opHandlers_.find(acc_.GetOpCode(gate));
            if (found != opHandlers_.end()) {
                (this->*(found->second))(gate);
                continue;
            }
            if (illegalOpHandlers_.find(acc_.GetOpCode(gate)) == illegalOpHandlers_.end()) {
                acc_.Print(gate);
                LOG_COMPILER(FATAL) << "The gate below need to be translated ";
                UNREACHABLE();
            }
        }
    }
    Finish();
}

BasicBlock *LLVMIRBuilder::EnsureBB(int id)
{
    BasicBlock *bb = nullptr;
    if (bbID2BB_.count(id) == 0) {
        auto newBB = std::make_unique<BasicBlock>(id);
        bb = newBB.get();
        bbID2BB_[id] = std::move(newBB);
    } else {
        bb = bbID2BB_[id].get();
    }
    return bb;
}

void LLVMIRBuilder::SetToCfg(BasicBlock *bb) const
{
    EnsureLBB(bb);
    BasicBlockImpl *impl = bb->GetImpl<BasicBlockImpl>();
    if ((impl == nullptr) || (impl->lBB_ == nullptr)) {
        LOG_COMPILER(ERROR) << "SetToCfg failed ";
        return;
    }
    impl->started = true;
    bb->SetImpl(impl);
    LLVMPositionBuilderAtEnd(builder_, impl->lBB_);
}

void LLVMIRBuilder::ProcessPhiWorkList()
{
    for (BasicBlock *bb : phiRebuildWorklist_) {
        auto impl = bb->GetImpl<BasicBlockImpl>();
        for (auto &e : impl->unmergedPhis_) {
            ASSERT(bbID2BB_.count(e.predBBId) > 0);
            BasicBlock *pred = bbID2BB_[e.predBBId].get();
            if (!impl->started) {
                OPTIONAL_LOG_COMPILER(ERROR) << " ProcessPhiWorkList error hav't start ";
                return;
            }
            LLVMValueRef value = GetLValue(e.operand);
            if (LLVMTypeOf(value) != LLVMTypeOf(e.phi)) {
                OPTIONAL_LOG_COMPILER(ERROR) << " ProcessPhiWorkList LLVMTypeOf don't match error ";
            }
            LLVMBasicBlockRef llvmBB = EnsureLBB(pred);
            LLVMAddIncoming(e.phi, &value, &llvmBB, 1);
        }
        impl->unmergedPhis_.clear();
    }
    phiRebuildWorklist_.clear();
}

void LLVMIRBuilder::EndCurrentBlock() const
{
    BasicBlockImpl *impl = currentBb_->GetImpl<BasicBlockImpl>();
    impl->ended = true;
}

void LLVMIRBuilder::Finish()
{
    ASSERT(!!currentBb_);
    EndCurrentBlock();
    ProcessPhiWorkList();
    for (auto &it : bbID2BB_) {
        it.second->ResetImpl<BasicBlockImpl>();
    }
    LLVMDIBuilderFinalize(GetDIBuilder());
}

BasicBlockImpl *LLVMIRBuilder::EnsureBBImpl(BasicBlock *bb) const
{
    if (bb->GetImpl<BasicBlockImpl>()) {
        return bb->GetImpl<BasicBlockImpl>();
    }
    auto impl = std::make_unique<BasicBlockImpl>();
    bb->SetImpl(impl.release());
    return bb->GetImpl<BasicBlockImpl>();
}

void LLVMIRBuilder::AssistGenPrologue(const size_t reservedSlotsSize, FrameType frameType)
{
    LLVMAddTargetDependentFunctionAttr(function_, "frame-reserved-slots",
                                       std::to_string(reservedSlotsSize).c_str());
    auto ArgList = circuit_->GetArgRoot();
    auto uses = acc_.Uses(ArgList);
    for (auto useIt = uses.begin(); useIt != uses.end(); ++useIt) {
        int argth = static_cast<int>(acc_.TryGetValue(*useIt));
        LLVMValueRef value = LLVMGetParam(function_, argth);
        int funcIndex = 0;
        if (isFastCallAot_) {
            frameType = FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME;
            funcIndex = static_cast<int>(FastCallArgIdx::FUNC);
        } else {
            funcIndex = static_cast<int>(CommonArgIdx::FUNC);
        }
        if (argth == funcIndex) {
            SaveByteCodePcOnOptJSFuncFrame(value);
            SaveJSFuncOnOptJSFuncFrame(value);
            SaveFrameTypeOnFrame(frameType, builder_);
        }
    }
}

void LLVMIRBuilder::GenPrologue()
{
    auto frameType = circuit_->GetFrameType();
    if (IsInterpreted()) {
        return;
    }
    LLVMAddTargetDependentFunctionAttr(function_, "frame-pointer", "all");

    size_t reservedSlotsSize = 0;
    if (frameType == FrameType::OPTIMIZED_FRAME) {
        reservedSlotsSize = OptimizedFrame::ComputeReservedSize(slotSize_);
        LLVMAddTargetDependentFunctionAttr(function_, "frame-reserved-slots",
                                           std::to_string(reservedSlotsSize).c_str());
        SaveFrameTypeOnFrame(frameType, builder_);
    } else if (frameType == FrameType::BASELINE_BUILTIN_FRAME) {
        reservedSlotsSize = BaselineBuiltinFrame::ComputeReservedSize(slotSize_);
        LLVMAddTargetDependentFunctionAttr(function_, "frame-reserved-slots",
                                           std::to_string(reservedSlotsSize).c_str());
        SaveFrameTypeOnFrame(frameType, builder_);
    } else if (frameType == FrameType::OPTIMIZED_JS_FUNCTION_FRAME) {
        reservedSlotsSize = OptimizedJSFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
        LLVMAddTargetDependentFunctionAttr(function_, "frame-reserved-slots",
                                           std::to_string(reservedSlotsSize).c_str());
        auto ArgList = circuit_->GetArgRoot();
        auto uses = acc_.Uses(ArgList);
        for (auto useIt = uses.begin(); useIt != uses.end(); ++useIt) {
            int argth = static_cast<int>(acc_.TryGetValue(*useIt));
            LLVMValueRef value = LLVMGetParam(function_, argth);
            int funcIndex = 0;
            if (isFastCallAot_) {
                frameType = FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME;
                funcIndex = static_cast<int>(FastCallArgIdx::FUNC);
            } else {
                funcIndex = static_cast<int>(CommonArgIdx::FUNC);
            }
            if (argth == funcIndex) {
                SaveJSFuncOnOptJSFuncFrame(value);
                SaveFrameTypeOnFrame(frameType, builder_);
            }
        }
    } else if (frameType == FrameType::FASTJIT_FUNCTION_FRAME) {
        reservedSlotsSize = FASTJITFunctionFrame::ComputeReservedPcOffset(slotSize_);
        AssistGenPrologue(reservedSlotsSize, frameType);
    } else {
        LOG_COMPILER(FATAL) << "frameType interpret type error !";
        ASSERT_PRINT(static_cast<uintptr_t>(frameType), "is not support !");
    }
}

void LLVMIRBuilder::SaveByteCodePcOnOptJSFuncFrame(LLVMValueRef value)
{
    ASSERT(circuit_->GetFrameType() == FrameType::FASTJIT_FUNCTION_FRAME);
    // load method
    LLVMValueRef func = LLVMBuildPtrToInt(builder_, value, slotType_, "cast_to_i64");
    LLVMValueRef offsetMethod = LLVMConstInt(GetInt64T(), JSFunctionBase::METHOD_OFFSET, false);
    LLVMValueRef addrMethod = LLVMBuildAdd(builder_, func, offsetMethod, "");
    LLVMValueRef methodPtr = LLVMBuildIntToPtr(builder_, addrMethod, GetTaggedPtrT(), "");
    LLVMValueRef method = LLVMBuildLoad(builder_, methodPtr, "");
    // load byteCodePc
    LLVMValueRef offsetByteCodePc = LLVMConstInt(GetInt64T(), Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET, false);
    LLVMValueRef addrByteCodePc = LLVMBuildAdd(builder_, method, offsetByteCodePc, "");
    LLVMValueRef byteCodePcPtr = LLVMBuildIntToPtr(builder_, addrByteCodePc, GetTaggedPtrT(), "");
    LLVMValueRef byteCodePc = LLVMBuildLoad(builder_, byteCodePcPtr, "");
    // push byteCodePc
    LLVMValueRef llvmFpAddr = CallingFp(module_, builder_, false);
    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder_, llvmFpAddr, slotType_, "cast_int_t");
    size_t reservedOffset = FASTJITFunctionFrame::ComputeReservedPcOffset(slotSize_);
    LLVMValueRef byteCodePcSlotAddr = LLVMBuildSub(builder_, frameAddr, LLVMConstInt(slotType_,
        reservedOffset, false), "");
    LLVMValueRef byteCodePcAddr = LLVMBuildIntToPtr(builder_, byteCodePcSlotAddr,
        LLVMPointerType(slotType_, 0), "byteCodePc.Addr");
    LLVMBuildStore(builder_, byteCodePc, byteCodePcAddr);
}

void LLVMIRBuilder::SaveFrameTypeOnFrame(FrameType frameType, LLVMBuilderRef builder)
{
    LLVMValueRef llvmFpAddr = CallingFp(module_, builder, false);

    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder, llvmFpAddr, slotType_, "cast_int_t");
    LLVMValueRef frameTypeSlotAddr = LLVMBuildSub(builder, frameAddr, LLVMConstInt(slotType_, slotSize_, false), "");
    LLVMValueRef addr = LLVMBuildIntToPtr(builder, frameTypeSlotAddr, LLVMPointerType(slotType_, 0), "frameType.Addr");
    LLVMValueRef llvmFrameType = LLVMConstInt(slotType_, static_cast<uintptr_t>(frameType), 0);
    LLVMBuildStore(builder, llvmFrameType, addr);
}

LLVMValueRef LLVMIRBuilder::CallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder, bool isCaller)
{
    if (IsInterpreted()) {
        return LLVMGetParam(function_, static_cast<unsigned>(InterpreterHandlerInputs::SP));
    }
    /* 0:calling 1:its caller */
    std::vector<LLVMValueRef> args = {LLVMConstInt(GetInt32T(), 0, isCaller)};
    auto fn = LLVMGetNamedFunction(module, "llvm.frameaddress.p0i8");
    if (!fn) {
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = { GetInt32T() };
        auto fnTy = LLVMFunctionType(GetRawPtrT(), paramTys1, 1, 0);
        fn = LLVMAddFunction(module, "llvm.frameaddress.p0i8", fnTy);
    }
    LLVMValueRef fAddrRet = LLVMBuildCall(builder, fn, args.data(), 1, "");
    return fAddrRet;
}

LLVMValueRef LLVMIRBuilder::ReadRegister(LLVMModuleRef &module, [[maybe_unused]] LLVMBuilderRef &builder,
    LLVMMetadataRef meta)
{
    std::vector<LLVMValueRef> args = {LLVMMetadataAsValue(context_, meta)};
    auto fn = LLVMGetNamedFunction(module, "llvm.read_register.i64");
    if (!fn) {
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = {
            GetMachineRepType(MachineRep::K_META),
        };
        auto fnTy = LLVMFunctionType(GetInt64T(), paramTys1, 1, 0);
        fn = LLVMAddFunction(module, "llvm.read_register.i64", fnTy);
    }
    LLVMValueRef fAddrRet = LLVMBuildCall(builder_, fn, args.data(), 1, "");
    return fAddrRet;
}

LLVMBasicBlockRef LLVMIRBuilder::EnsureLBB(BasicBlock *bb) const
{
    BasicBlockImpl *impl = EnsureBBImpl(bb);
    ASSERT(impl != nullptr);
    if (impl->lBB_) {
        return impl->lBB_;
    }

    std::string buf = "B" + std::to_string(bb->GetId());
    LLVMBasicBlockRef llvmBB = LLVMAppendBasicBlockInContext(context_, function_, buf.c_str());
    impl->lBB_ = llvmBB;
    impl->continuation = llvmBB;
    bb->SetImpl(impl);
    return llvmBB;
}

LLVMTypeRef LLVMIRBuilder::GetMachineRepType(MachineRep rep) const
{
    LLVMTypeRef dstType;
    switch (rep) {
        case MachineRep::K_BIT:
            dstType = GetInt1T();
            break;
        case MachineRep::K_WORD8:
            dstType = GetInt8T();
            break;
        case MachineRep::K_WORD16:
            dstType = GetInt16T();
            break;
        case MachineRep::K_WORD32:
            dstType = GetInt32T();
            break;
        case MachineRep::K_FLOAT64:
            dstType = GetDoubleT();
            break;
        case MachineRep::K_WORD64:
            dstType = GetInt64T();
            break;
        case MachineRep::K_PTR_1:
            dstType = GetTaggedHPtrT();
            break;
        case MachineRep::K_META:
            dstType = LLVMMetadataTypeInContext(context_);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return dstType;
}

void LLVMIRBuilder::HandleReadSp(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::READSP);
    VisitReadSp(gate);
}

void LLVMIRBuilder::HandleBitRev(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::BITREV);
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitBitRev(gate, ins[0]);
}

void LLVMIRBuilder::VisitBitRev(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    std::vector<LLVMValueRef> args = { e1Value };
    std::string intrinsic;
    switch (acc_.GetMachineType(e1)) {
        case I8:
            intrinsic = "llvm.bitreverse.i8";
            break;
        case I16:
            intrinsic = "llvm.bitreverse.i16";
            break;
        case I32:
            intrinsic = "llvm.bitreverse.i32";
            break;
        case I64:
            intrinsic = "llvm.bitreverse.i64";
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
    }
    auto fn = LLVMGetNamedFunction(module_, intrinsic.c_str());
    if (!fn) {
        LLVMTypeRef type = ConvertLLVMTypeFromGate(e1);
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = { type };
        auto fnTy = LLVMFunctionType(type, paramTys1, 1, 0);
        fn = LLVMAddFunction(module_, intrinsic.c_str(), fnTy);
    }
    LLVMValueRef result = LLVMBuildCall(builder_, fn, args.data(), 1, "bitreverse");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleCall(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    OpCode callOp = acc_.GetOpCode(gate);
    if (callOp == OpCode::CALL || callOp == OpCode::NOGC_RUNTIME_CALL ||
        callOp == OpCode::BUILTINS_CALL || callOp == OpCode::BUILTINS_CALL_WITH_ARGV ||
        callOp == OpCode::CALL_OPTIMIZED || callOp == OpCode::FAST_CALL_OPTIMIZED ||
        callOp == OpCode::BASELINE_CALL || callOp == OpCode::ASM_CALL_BARRIER) {
        VisitCall(gate, ins, callOp);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void LLVMIRBuilder::HandleBytecodeCall(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitBytecodeCall(gate, ins);
}

void LLVMIRBuilder::HandleRuntimeCall(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitRuntimeCall(gate, ins);
}

LLVMValueRef LLVMIRBuilder::GetFunction(LLVMValueRef glue, const CallSignature *signature,
                                        LLVMValueRef rtbaseoffset, const std::string &realName) const
{
    LLVMTypeRef rtfuncType = llvmModule_->GetFuncType(signature);
    LLVMTypeRef rtfuncTypePtr = LLVMPointerType(rtfuncType, 0);
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    LLVMValueRef rtbaseAddr = LLVMBuildIntToPtr(builder_, rtbaseoffset, LLVMPointerType(glueType, 0), "");
    std::string name = realName.empty()
            ? signature->GetName()
            : realName;
    LLVMValueRef llvmAddr = LLVMBuildLoad(builder_, rtbaseAddr, name.c_str());
    LLVMValueRef callee = LLVMBuildIntToPtr(builder_, llvmAddr, rtfuncTypePtr, (name + "-cast").c_str());
    ASSERT(callee != nullptr);
    return callee;
}

LLVMValueRef LLVMIRBuilder::GetOrDeclareFunction(const CallSignature *signature) const
{
    LLVMValueRef callee = LLVMGetNamedFunction(module_, signature->GetName().c_str());
    if (callee != nullptr) {
        return callee;
    }
    auto funcType = llvmModule_->GetFuncType(signature);
    return LLVMAddFunction(module_, signature->GetName().c_str(), funcType);
}

LLVMValueRef LLVMIRBuilder::GetFunctionFromGlobalValue([[maybe_unused]] LLVMValueRef glue,
    const CallSignature *signature, LLVMValueRef reloc) const
{
    LLVMTypeRef rtfuncType = llvmModule_->GetFuncType(signature);
    LLVMTypeRef rtfuncTypePtr = LLVMPointerType(rtfuncType, 0);
    LLVMValueRef callee = LLVMBuildIntToPtr(builder_, reloc, rtfuncTypePtr, "cast");
    assert(callee != nullptr);
    return callee;
}

bool LLVMIRBuilder::IsInterpreted() const
{
    return circuit_->GetFrameType() == FrameType::ASM_INTERPRETER_FRAME;
}

bool LLVMIRBuilder::IsBaselineBuiltin() const
{
    return circuit_->GetFrameType() == FrameType::BASELINE_BUILTIN_FRAME;
}

bool LLVMIRBuilder::IsOptimized() const
{
    return circuit_->GetFrameType() == FrameType::OPTIMIZED_FRAME;
}

bool LLVMIRBuilder::IsOptimizedJSFunction() const
{
    return circuit_->GetFrameType() == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
        circuit_->GetFrameType() == FrameType::FASTJIT_FUNCTION_FRAME;
}

void LLVMIRBuilder::VisitRuntimeCall(GateRef gate, const std::vector<GateRef> &inList)
{
    ASSERT(llvmModule_ != nullptr);
    StubIdType stubId = RTSTUB_ID(CallRuntime);
    LLVMValueRef glue = GetGlue(inList);
    const CallSignature *signature = RuntimeStubCSigns::Get(std::get<RuntimeStubCSigns::ID>(stubId));

    auto kind = GetCallInfoKind(OpCode::RUNTIME_CALL, inList);

    size_t actualNumArgs = 0;
    GateRef frameState = Circuit::NullGate();
    ComputeArgCountAndExtraInfo(actualNumArgs, frameState, inList, kind);

    std::vector<LLVMValueRef> params{glue};
    const int index = static_cast<int>(acc_.GetConstantValue(inList[static_cast<int>(CallInputs::TARGET)]));
    params.push_back(LLVMConstInt(GetInt64T(), index, 0)); // target
    params.push_back(LLVMConstInt(GetInt64T(),
        actualNumArgs - static_cast<size_t>(CallInputs::FIRST_PARAMETER), 0)); // argc
    for (size_t paraIdx = static_cast<size_t>(CallInputs::FIRST_PARAMETER); paraIdx < actualNumArgs; ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params.push_back(GetLValue(gateTmp));
    }

    LLVMTypeRef funcType = llvmModule_->GenerateFuncType(params, signature);
    LLVMValueRef callee;
    if (enableOptDirectCall_) {
        callee = GetOrDeclareFunction(signature);
    } else {
        int stubIndex = static_cast<int>(std::get<RuntimeStubCSigns::ID>(stubId));
        LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, glue, GetRTStubOffset(glue, stubIndex), "");
        callee = GetFunction(glue, signature, rtbaseoffset, RuntimeStubCSigns::GetRTName(index));
    }
    callee = LLVMBuildPointerCast(builder_, callee, LLVMPointerType(funcType, 0), "");
    LLVMValueRef runtimeCall = nullptr;
    if (kind == CallInfoKind::HAS_FRAME_STATE) {
        std::vector<LLVMValueRef> values;
        GetDeoptBundleInfo(frameState, values);
        runtimeCall = LLVMBuildCall3(builder_, funcType, callee, params.data(), actualNumArgs,
                                     "", values.data(), values.size());
    } else {
        runtimeCall = LLVMBuildCall2(builder_, funcType, callee, params.data(), actualNumArgs, "");
    }
    LLVMSetInstructionCallConv(runtimeCall, LLVMWebKitJSCallConv);
    if (RuntimeStubCSigns::IsCold(index)) {
        unsigned ColdAttrKind = LLVMGetEnumAttributeKindForName(COLD_ATTR.data(), COLD_ATTR.size());
        LLVMAttributeRef ColdAttribute = LLVMCreateEnumAttribute(context_, ColdAttrKind, 0);
        LLVMAddCallSiteAttribute(runtimeCall, LLVMAttributeFunctionIndex, ColdAttribute);
    }
    Bind(gate, runtimeCall);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, runtimeCall);
    }
}

bool LLVMIRBuilder::SetDebugInfo(GateRef g, LLVMValueRef r)
{
    if (r != nullptr) {
        LLVMValueKind k = LLVMGetValueKind(r);
        if (k == LLVMInstructionValueKind) {
            std::string comment = acc_.ToString(g);
            circuit_->AddComment(g, std::move(comment));
            size_t index = 0;
            circuit_->GetDebugInfo(g, index);
            LLVMMetadataRef loc = LLVMDIBuilderCreateDebugLocation(context_, index + 1, 0, dFuncMD_, NULL);
            LLVMInstructionSetDebugLoc(r, loc);
            return true;
        }
    }
    return false;
}

void LLVMIRBuilder::HandleRuntimeCallWithArgv(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitRuntimeCallWithArgv(gate, ins);
}

void LLVMIRBuilder::VisitRuntimeCallWithArgv(GateRef gate, const std::vector<GateRef> &inList)
{
    ASSERT(IsOptimized() == true);
    StubIdType stubId = RTSTUB_ID(CallRuntimeWithArgv);
    LLVMValueRef glue = GetGlue(inList);
    const CallSignature *signature = RuntimeStubCSigns::Get(std::get<RuntimeStubCSigns::ID>(stubId));
    std::vector<LLVMValueRef> params;
    params.push_back(glue); // glue

    uint64_t index = acc_.GetConstantValue(inList[static_cast<size_t>(CallInputs::TARGET)]);
    auto targetId = LLVMConstInt(GetInt64T(), index, 0);
    params.push_back(targetId); // target
    for (size_t paraIdx = static_cast<size_t>(CallInputs::FIRST_PARAMETER); paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params.push_back(GetLValue(gateTmp));
    }

    LLVMTypeRef funcType = llvmModule_->GenerateFuncType(params, signature);
    LLVMValueRef callee;
    if (enableOptDirectCall_) {
        callee = GetOrDeclareFunction(signature);
    } else {
        int stubIndex = static_cast<int>(std::get<RuntimeStubCSigns::ID>(stubId));
        LLVMValueRef rtoffset = GetRTStubOffset(glue, stubIndex);
        LLVMValueRef rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
        callee = GetFunction(glue, signature, rtbaseoffset);
    }
    callee = LLVMBuildPointerCast(builder_, callee, LLVMPointerType(funcType, 0), "");
    LLVMValueRef runtimeCall = LLVMBuildCall2(builder_, funcType, callee, params.data(), inList.size() - 1, "");
    Bind(gate, runtimeCall);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, runtimeCall);
    }
}

LLVMValueRef LLVMIRBuilder::GetCurrentSP()
{
    LLVMMetadataRef meta;
    if (compCfg_->IsAmd64()) {
        meta = LLVMMDStringInContext2(context_, "rsp", 4);   // 4 : 4 means len of "rsp"
    } else {
        meta = LLVMMDStringInContext2(context_, "sp", 3);   // 3 : 3 means len of "sp"
    }
    LLVMMetadataRef metadataNode = LLVMMDNodeInContext2(context_, &meta, 1);
    LLVMValueRef spValue = ReadRegister(module_, builder_, metadataNode);
    return spValue;
}

LLVMValueRef LLVMIRBuilder::GetCurrentFrameType(LLVMValueRef currentSpFrameAddr)
{
    LLVMValueRef tmp = LLVMBuildSub(builder_, currentSpFrameAddr, LLVMConstInt(slotType_, slotSize_, 1), "");
    LLVMValueRef frameTypeAddr =
        LLVMBuildIntToPtr(builder_, tmp, LLVMPointerType(GetInt64T(), 0), "");
    LLVMValueRef frameType = LLVMBuildLoad(builder_, frameTypeAddr, "");
    return frameType;
}

void LLVMIRBuilder::SetGCLeafFunction(LLVMValueRef call)
{
    const char *attrName = "gc-leaf-function";
    const char *attrValue = "true";
    LLVMAttributeRef llvmAttr = LLVMCreateStringAttribute(context_, attrName, strlen(attrName), attrValue,
                                                          strlen(attrValue));
    LLVMAddCallSiteAttribute(call, LLVMAttributeFunctionIndex, llvmAttr);
}

void LLVMIRBuilder::SetCallConvAttr(const CallSignature *calleeDescriptor, LLVMValueRef call)
{
    ASSERT(calleeDescriptor != nullptr);
    if (calleeDescriptor->GetCallConv() == CallSignature::CallConv::GHCCallConv) {
        LLVMSetTailCall(call, true);
        LLVMSetInstructionCallConv(call, LLVMGHCCallConv);
    } else if (calleeDescriptor->GetCallConv() == CallSignature::CallConv::WebKitJSCallConv) {
        LLVMSetInstructionCallConv(call, LLVMWebKitJSCallConv);
    }
    if (calleeDescriptor->IsNoTailCall()) {
        if (llvm::CallInst *ci = llvm::dyn_cast<llvm::CallInst>(llvm::unwrap(call))) {
            ci->setTailCallKind(llvm::CallInst::TCK_NoTail);
        }
    } else if (calleeDescriptor->GetTailCall()) {
        LLVMSetTailCall(call, true);
    }
}

void LLVMIRBuilder::SetCallSiteFunctionAttr(CallSiteAttribute attr, LLVMValueRef call)
{
    if (attr.readOnly) {
        unsigned readOnlyAttrKind = LLVMGetEnumAttributeKindForName(READONLY_ATTR.data(), READONLY_ATTR.size());
        LLVMAttributeRef readOnlyAttribute = LLVMCreateEnumAttribute(context_, readOnlyAttrKind, 0);
        LLVMAddCallSiteAttribute(call, LLVMAttributeFunctionIndex, readOnlyAttribute);
    }
    if (attr.cold) {
        unsigned coldAttrKind = LLVMGetEnumAttributeKindForName(COLD_ATTR.data(), COLD_ATTR.size());
        LLVMAttributeRef coldAttribute = LLVMCreateEnumAttribute(context_, coldAttrKind, 0);
        LLVMAddCallSiteAttribute(call, LLVMAttributeFunctionIndex, coldAttribute);
    }
}

bool LLVMIRBuilder::IsHeapPointerType(LLVMTypeRef valueType)
{
    return LLVMGetTypeKind(valueType) == LLVMPointerTypeKind && LLVMGetPointerAddressSpace(valueType) > 0;
}

LLVMValueRef LLVMIRBuilder::GetGlue(const std::vector<GateRef> &inList)
{
    auto g = inList.at(static_cast<size_t>(CallInputs::GLUE));
    return GetLValue(g);
}

LLVMValueRef LLVMIRBuilder::GetLeaveFrameOffset(LLVMValueRef glue)
{
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    return LLVMConstInt(glueType,
        static_cast<int>(JSThread::GlueData::GetLeaveFrameOffset(compCfg_->Is32Bit())), 0);
}

LLVMValueRef LLVMIRBuilder::GetRTStubOffset(LLVMValueRef glue, int index)
{
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    return LLVMConstInt(glueType,
        static_cast<int>(JSThread::GlueData::GetRTStubEntriesOffset(compCfg_->Is32Bit())) + index * slotSize_, 0);
}

LLVMValueRef LLVMIRBuilder::GetCoStubOffset(LLVMValueRef glue, int index)
{
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    return LLVMConstInt(glueType, JSThread::GlueData::GetCOStubEntriesOffset(compCfg_->Is32Bit()) +
        static_cast<size_t>(index * slotSize_), 0);
}

LLVMValueRef LLVMIRBuilder::GetBaselineStubOffset(LLVMValueRef glue, int index)
{
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    return LLVMConstInt(glueType, JSThread::GlueData::GetBaselineStubEntriesOffset(compCfg_->Is32Bit()) +
                        static_cast<size_t>(index * slotSize_), 0);
}

LLVMValueRef LLVMIRBuilder::GetBCStubOffset(LLVMValueRef glue)
{
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    return LLVMConstInt(glueType, JSThread::GlueData::GetBCStubEntriesOffset(compCfg_->Is32Bit()), 0);
}

LLVMValueRef LLVMIRBuilder::GetBCDebugStubOffset(LLVMValueRef glue)
{
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    return LLVMConstInt(glueType, JSThread::GlueData::GetBCDebuggerStubEntriesOffset(compCfg_->Is32Bit()), 0);
}

LLVMValueRef LLVMIRBuilder::GetBuiltinsStubOffset(LLVMValueRef glue)
{
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    return LLVMConstInt(glueType, JSThread::GlueData::GetBuiltinsStubEntriesOffset(compCfg_->Is32Bit()), 0);
}

void LLVMIRBuilder::ComputeArgCountAndExtraInfo(size_t &actualNumArgs, GateRef &frameState,
                                                const std::vector<GateRef> &inList, CallInfoKind kind)
{
    if (kind == CallInfoKind::HAS_FRAME_STATE) {
        actualNumArgs = inList.size() - 1;  // 1: frameState
        frameState = inList.at(actualNumArgs);
    } else {
        actualNumArgs = inList.size();
    }
}

void LLVMIRBuilder::UpdateLeaveFrame(LLVMValueRef glue)
{
    LLVMValueRef leaveFrameOffset = GetLeaveFrameOffset(glue);
    LLVMValueRef leaveFrameValue = LLVMBuildAdd(builder_, glue, leaveFrameOffset, "");
    LLVMTypeRef glueType = LLVMTypeOf(glue);
    LLVMValueRef leaveFrameAddr = LLVMBuildIntToPtr(builder_, leaveFrameValue, LLVMPointerType(glueType, 0), "");
    LLVMValueRef llvmFpAddr = CallingFp(module_, builder_, true);
    LLVMValueRef fp = LLVMBuildPtrToInt(builder_, llvmFpAddr, GetInt64T(), "cast_int64_t");
    LLVMBuildStore(builder_, fp, leaveFrameAddr);
}

LLVMValueRef LLVMIRBuilder::GetCallee(const std::vector<GateRef> &inList, const CallSignature *signature,
    const std::string &realName)
{
    LLVMTypeRef rtfuncType = llvmModule_->GetFuncType(signature);
    LLVMTypeRef rtfuncTypePtr = LLVMPointerType(rtfuncType, 0);

    std::string name = realName.empty()
            ? signature->GetName()
            : realName;
    LLVMValueRef code = GetLValue(inList.at(static_cast<size_t>(CallInputs::TARGET)));
    LLVMValueRef callee = LLVMBuildIntToPtr(builder_, code, rtfuncTypePtr, (name + "-cast").c_str());
    ASSERT(callee != nullptr);
    return callee;
}

void LLVMIRBuilder::VisitReadSp(GateRef gate)
{
    LLVMValueRef spValue = GetCurrentSP();
    Bind(gate, spValue);
}

CallInfoKind LLVMIRBuilder::GetCallInfoKind(OpCode op, const std::vector<GateRef> &inList) const
{
    if (!IsOptimizedJSFunction()) {
        return CallInfoKind::NO_FRAME_STATE;
    }
    CallInfoKind kind;
    switch (op) {
        case OpCode::NOGC_RUNTIME_CALL: {
            size_t targetIndex = static_cast<size_t>(CallInputs::TARGET);
            const size_t index = acc_.GetConstantValue(inList[targetIndex]);
            if (kungfu::RuntimeStubCSigns::IsAsmStub(index)) {
                kind = CallInfoKind::HAS_FRAME_STATE;
            } else {
                kind = CallInfoKind::NO_FRAME_STATE;
            }
            break;
        }
        case OpCode::CALL:
        case OpCode::CALL_OPTIMIZED:
        case OpCode::FAST_CALL_OPTIMIZED:
        case OpCode::BUILTINS_CALL:
        case OpCode::RUNTIME_CALL:
            kind = CallInfoKind::HAS_FRAME_STATE;
            break;
        case OpCode::BASELINE_CALL:
        case OpCode::ASM_CALL_BARRIER:
        case OpCode::BUILTINS_CALL_WITH_ARGV:
            kind = CallInfoKind::NO_FRAME_STATE;
            break;
        default:
            kind = CallInfoKind::NO_FRAME_STATE;
            UNREACHABLE();
    }
    return kind;
}

bool LLVMIRBuilder::GetGCState(GateRef gate, OpCode op, const CallSignature *calleeDescriptor) const
{
    bool isNoGC = false;
    switch (op) {
        case OpCode::CALL:
        case OpCode::NOGC_RUNTIME_CALL:
        case OpCode::BASELINE_CALL:
        case OpCode::BUILTINS_CALL:
        case OpCode::BUILTINS_CALL_WITH_ARGV:
            isNoGC = false;
            break;
        case OpCode::CALL_OPTIMIZED:
        case OpCode::FAST_CALL_OPTIMIZED:
            isNoGC = acc_.IsNoGC(gate);
            break;
        case OpCode::ASM_CALL_BARRIER:
            isNoGC = true;
            break;
        default:
            isNoGC = false;
            UNREACHABLE();
    }
    if (calleeDescriptor->GetGCLeafFunction() ||
        calleeDescriptor->GetCallConv() == CallSignature::CallConv::GHCCallConv) {
        isNoGC = true;
    }
    return isNoGC;
}

void LLVMIRBuilder::VisitCall(GateRef gate, const std::vector<GateRef> &inList, OpCode op)
{
    size_t targetIndex = static_cast<size_t>(CallInputs::TARGET);
    static_assert(static_cast<size_t>(CallInputs::FIRST_PARAMETER) == 3);
    const CallSignature *calleeDescriptor = nullptr;
    LLVMValueRef glue = GetGlue(inList);
    LLVMValueRef rtoffset;
    LLVMValueRef rtbaseoffset;
    LLVMValueRef callee;
    CallSiteAttribute attr;
    if (op == OpCode::CALL) {
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = CommonStubCSigns::Get(index);
        if (enableOptDirectCall_) {
            if (isStwCopyStub_) {
                calleeDescriptor = CommonStubCSigns::Get(index + CommonStubCSigns::NUM_OF_ALL_NORMAL_STUBS);
            }
            callee = GetOrDeclareFunction(calleeDescriptor);
        } else {
            rtoffset = GetCoStubOffset(glue, index);
            rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
            callee = GetFunction(glue, calleeDescriptor, rtbaseoffset);
        }
        attr.readOnly = CommonStubCSigns::IsReadOnly(index);
        attr.cold = CommonStubCSigns::IsCold(index);
    } else if (op == OpCode::NOGC_RUNTIME_CALL) {
        // enableOptDirectCall_ optimization can be used for this case if the callee is asm stub.
        UpdateLeaveFrame(glue);
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = RuntimeStubCSigns::Get(index);
        if (enableOptDirectCall_ && RuntimeStubCSigns::IsAsmStub(index)) {
            callee = GetOrDeclareFunction(calleeDescriptor);
        } else {
            rtoffset = GetRTStubOffset(glue, index);
            rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
            callee = GetFunction(glue, calleeDescriptor, rtbaseoffset);
        }
    } else if (op == OpCode::CALL_OPTIMIZED) {
        calleeDescriptor = RuntimeStubCSigns::GetOptimizedCallSign();
        callee = GetCallee(inList, calleeDescriptor);
    } else if (op == OpCode::FAST_CALL_OPTIMIZED) {
        calleeDescriptor = RuntimeStubCSigns::GetOptimizedFastCallSign();
        callee = GetCallee(inList, calleeDescriptor);
    } else if (op == OpCode::BASELINE_CALL) {
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = BaselineStubCSigns::Get(index);
        rtoffset = GetBaselineStubOffset(glue, index);
        rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
        callee = GetFunction(glue, calleeDescriptor, rtbaseoffset);
    } else if (op == OpCode::ASM_CALL_BARRIER) {
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = RuntimeStubCSigns::Get(index);
        if (enableOptDirectCall_) {
            callee = GetOrDeclareFunction(calleeDescriptor);
        } else {
            rtoffset = GetRTStubOffset(glue, index);
            rtbaseoffset = LLVMBuildAdd(builder_, glue, rtoffset, "");
            callee = GetFunction(glue, calleeDescriptor, rtbaseoffset);
        }
    } else {
        ASSERT(op == OpCode::BUILTINS_CALL || op == OpCode::BUILTINS_CALL_WITH_ARGV);
        LLVMValueRef opcodeOffset = GetLValue(inList.at(targetIndex));
        rtoffset = GetBuiltinsStubOffset(glue);
        rtbaseoffset = LLVMBuildAdd(
            builder_, glue, LLVMBuildAdd(builder_, rtoffset, opcodeOffset, ""), "");
        if (op == OpCode::BUILTINS_CALL) {
            calleeDescriptor = BuiltinsStubCSigns::BuiltinsCSign();
        } else {
            calleeDescriptor = BuiltinsStubCSigns::BuiltinsWithArgvCSign();
        }
        callee = GetFunction(glue, calleeDescriptor, rtbaseoffset);
    }

    CallInfoKind kind = GetCallInfoKind(op, inList);
    bool isNoGC = GetGCState(gate, op, calleeDescriptor);

    std::vector<LLVMValueRef> params;
    const size_t firstArg = static_cast<size_t>(CallInputs::FIRST_PARAMETER);
    GateRef glueGate = inList[firstArg];
    params.push_back(GetLValue(glueGate));

    // get parameter types
    LLVMTypeRef calleeFuncType = LLVMGetElementType(LLVMTypeOf(callee));
    std::vector<LLVMTypeRef> paramTypes(LLVMCountParamTypes(calleeFuncType));
    LLVMGetParamTypes(calleeFuncType, paramTypes.data());

    int extraParameterCnt = 0;
    size_t actualNumArgs = 0;
    GateRef frameState = Circuit::NullGate();
    ComputeArgCountAndExtraInfo(actualNumArgs, frameState, inList, kind);
    std::vector<CallSignature::ParamAttr> *paramAttr = calleeDescriptor->GetParamAttr();
    // then push the actual parameter for js function call
    for (size_t paraIdx = firstArg + 1; paraIdx < actualNumArgs; ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        const auto gateTmpType = LLVMTypeOf(GetLValue(gateTmp));
        if (paramAttr != nullptr && params.size() < paramAttr->size() &&
            paramAttr->at(params.size()) == CallSignature::ParamAttr::Dead) {
            params.push_back(LLVMGetUndef(gateTmpType));
            continue;
        }
        if (params.size() < paramTypes.size()) {  // this condition will be false for variadic arguments
            const auto paramType = paramTypes.at(params.size());
            // match parameter types and function signature types
            if (IsHeapPointerType(paramType) && !IsHeapPointerType(gateTmpType)) {
#ifndef ARK_USE_SATB_BARRIER
                params.push_back(LLVMBuildIntToPtr(builder_,
                                                   LLVMBuildBitCast(builder_, GetLValue(gateTmp), GetInt64T(), ""),
                                                   paramType, ""));
#else
                params.push_back(LLVMBuildIntToPtr(builder_,
                    LLVMBuildZExtOrBitCast(builder_, GetLValue(gateTmp), GetInt64T(), ""),
                paramType, ""));
#endif
            } else {
#ifndef ARK_USE_SATB_BARRIER
                params.push_back(LLVMBuildBitCast(builder_, GetLValue(gateTmp), paramType, ""));
#else
                params.push_back(LLVMBuildZExtOrBitCast(builder_, GetLValue(gateTmp), paramType, ""));
#endif
            }
        } else {
            params.push_back(GetLValue(gateTmp));
        }
    }

    LLVMValueRef call = nullptr;
    if (op == OpCode::ASM_CALL_BARRIER) {
        if (!enableOptDirectCall_) {
            callee = LLVMBuildPointerCast(builder_, callee, llvmModule_->GetRawPtrT(), "");
            params.insert(params.begin(), callee);
        }
        call = LLVMBuildCall(builder_, ASMBarrierCall_, params.data(), params.size(), "");
    } else {
        LLVMTypeRef funcType = llvmModule_->GenerateFuncType(params, calleeDescriptor);
        callee = LLVMBuildPointerCast(builder_, callee, LLVMPointerType(funcType, 0), "");
        // In the LLVM backend, when we set the gcleaf option,
        // the analysis of stack liveness will not be carried out.
        // As a result, we shouldn't call llvmcall3 with the gcleaf option enabled.
        bool useFrameState = (kind == CallInfoKind::HAS_FRAME_STATE && !isNoGC);
        if (useFrameState) {
            std::vector<LLVMValueRef> values;
            GetDeoptBundleInfo(frameState, values);
            call = LLVMBuildCall3(builder_, funcType, callee, params.data(), actualNumArgs - firstArg +
                extraParameterCnt, "", values.data(), values.size());
        } else {
            call = LLVMBuildCall2(builder_, funcType, callee, params.data(), actualNumArgs - firstArg +
                extraParameterCnt, "");
        }
        SetCallConvAttr(calleeDescriptor, call);
        SetCallSiteFunctionAttr(attr, call);
    }
    if (isNoGC) {
        SetGCLeafFunction(call);
    }
    Bind(gate, call);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, call);
    }
}

void LLVMIRBuilder::VisitBytecodeCall(GateRef gate, const std::vector<GateRef> &inList)
{
    size_t paraStartIndex = static_cast<size_t>(CallInputs::FIRST_PARAMETER);
    size_t targetIndex = static_cast<size_t>(CallInputs::TARGET);
    size_t glueIndex = static_cast<size_t>(CallInputs::GLUE);
    LLVMValueRef opcodeOffset = GetLValue(inList.at(targetIndex));
    ASSERT(llvmModule_ != nullptr);

    // start index of bytecode handler csign in llvmModule
    LLVMValueRef glue = GetLValue(inList.at(glueIndex));
    LLVMValueRef baseOffset = GetBaseOffset(gate, glue);
    LLVMValueRef rtbaseoffset = LLVMBuildAdd(
        builder_, glue, LLVMBuildAdd(builder_, baseOffset, opcodeOffset, ""), "");
    const CallSignature *signature = BytecodeStubCSigns::BCHandler();
    LLVMValueRef callee = GetFunction(glue, signature, rtbaseoffset);

    std::vector<LLVMValueRef> params;
    for (size_t paraIdx = paraStartIndex; paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params.push_back(GetLValue(gateTmp));
    }

    LLVMTypeRef funcType = llvmModule_->GenerateFuncType(params, signature);
    callee = LLVMBuildPointerCast(builder_, callee, LLVMPointerType(funcType, 0), "");
    LLVMValueRef call = LLVMBuildCall2(builder_, funcType, callee, params.data(), inList.size() - paraStartIndex, "");
    SetGCLeafFunction(call);
    LLVMSetTailCall(call, true);
    LLVMSetInstructionCallConv(call, LLVMGHCCallConv);
    Bind(gate, call);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, call);
    }
}

LLVMValueRef LLVMIRBuilder::GetBaseOffset(GateRef gate, LLVMValueRef glue)
{
    switch (acc_.GetOpCode(gate)) {
        case OpCode::BYTECODE_CALL:
            return GetBCStubOffset(glue);
        case OpCode::DEBUGGER_BYTECODE_CALL:
            return GetBCDebugStubOffset(glue);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void LLVMIRBuilder::HandleAlloca(GateRef gate)
{
    return VisitAlloca(gate);
}

void LLVMIRBuilder::VisitAlloca(GateRef gate)
{
    uint64_t machineRep = acc_.TryGetValue(gate);
    LLVMTypeRef dataType = GetMachineRepType(static_cast<MachineRep>(machineRep));
    auto lv = LLVMBuildPtrToInt(builder_,
                                LLVMBuildAlloca(builder_, dataType, ""),
                                ConvertLLVMTypeFromGate(gate), "");
    Bind(gate, lv);
}

void LLVMIRBuilder::HandlePhi(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitPhi(gate, ins);
}

int LLVMIRBuilder::LookupPredBB(GateRef start, int bbID)
{
    GateId gateId = acc_.GetId(start);
    int owner = instID2bbID_[gateId];
    if (owner != bbID) {
        return owner;
    }
    GateRef pred = start;
    while (owner == bbID) {
        pred = acc_.GetState(pred);
        auto id = acc_.GetId(pred);
        owner = instID2bbID_[id];
    }
    return owner;
}

void LLVMIRBuilder::VisitPhi(GateRef gate, const std::vector<GateRef> &phiIns)
{
    LLVMTypeRef type = ConvertLLVMTypeFromGate(gate);
    LLVMValueRef phi = LLVMBuildPhi(builder_, type, "");
    if (phiIns.size() > 1) {
        Bind(gate, phi);
    }
    // Collect the states merges of this phi and note the 1-in is the merged states.
    std::vector<GateRef> phiStates;
    acc_.GetIns(phiIns.at(0), phiStates);
    ASSERT(phiStates.size() + 1 == phiIns.size());
    for (int i = 1; i < static_cast<int>(phiIns.size()); i++) {
        int bbIdx = LookupPredBB(phiStates.at(i - 1), currentBb_->GetId());

        int cnt = static_cast<int>(bbID2BB_.count(bbIdx));
        // if cnt = 0 means bb with current bbIdx hasn't been created
        if (cnt > 0) {
            BasicBlock *bb = bbID2BB_[bbIdx].get();
            if (bb == nullptr) {
                OPTIONAL_LOG_COMPILER(ERROR) << "VisitPhi failed BasicBlock nullptr";
                return;
            }
            BasicBlockImpl *impl = bb->GetImpl<BasicBlockImpl>();
            if (impl == nullptr) {
                OPTIONAL_LOG_COMPILER(ERROR) << "VisitPhi failed impl nullptr";
                return;
            }
            LLVMBasicBlockRef llvmBB = EnsureLBB(bb);  // The llvm bb
            LLVMValueRef value = GetLValue(phiIns.at(i));

            if (impl->started) {
                LLVMAddIncoming(phi, &value, &llvmBB, 1);
            } else {
                impl = currentBb_->GetImpl<BasicBlockImpl>();
                NotMergedPhiDesc d = { bbIdx, phiIns.at(i), phi };
                impl->unmergedPhis_.emplace_back(d);
                phiRebuildWorklist_.push_back(currentBb_);
            }
        } else {
            BasicBlockImpl* impl = currentBb_->GetImpl<BasicBlockImpl>();
            NotMergedPhiDesc d = { bbIdx, phiIns.at(i), phi };
            impl->unmergedPhis_.emplace_back(d);
            phiRebuildWorklist_.push_back(currentBb_);
        }
    }
}

void LLVMIRBuilder::VisitReturn([[maybe_unused]] GateRef gate, [[maybe_unused]] GateRef popCount,
                                const std::vector<GateRef> &operands)
{
    // [STATE] [DEPEND] [VALUE] [RETURN_LIST]
    GateRef operand = operands[2];  // 2: skip 2 in gate that are not data gate
    LLVMValueRef returnValue = GetLValue(operand);
    LLVMBuildRet(builder_, returnValue);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, returnValue);
    }
}

void LLVMIRBuilder::HandleReturn(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitReturn(gate, 1, ins);
}

void LLVMIRBuilder::VisitReturnVoid([[maybe_unused]] GateRef gate)
{
    // [STATE] [DEPEND] [VALUE] [RETURN_LIST]
    LLVMBuildRetVoid(builder_);
}

void LLVMIRBuilder::HandleReturnVoid(GateRef gate)
{
    VisitReturnVoid(gate);
}

void LLVMIRBuilder::LinkToLLVMCfg(int bbId, const OperandsVector &predecessors)
{
    BasicBlock *bb = EnsureBB(bbId);
    if (bb == nullptr) {
        OPTIONAL_LOG_COMPILER(ERROR) << " block create failed ";
        return;
    }
    currentBb_ = bb;
    LLVMBasicBlockRef lBB = EnsureLBB(bb);
    SetToCfg(bb);
    for (int predecessor : predecessors) {
        BasicBlock *pre = EnsureBB(predecessor);
        if (pre == nullptr) {
            OPTIONAL_LOG_COMPILER(ERROR) << " block setup failed, predecessor:%d nullptr" << predecessor;
            return;
        }
        LLVMBasicBlockRef preLBB = EnsureLBB(pre);
        LLVMMoveBasicBlockBefore(preLBB, lBB);
    }
    if (IsPrologue(bbId)) {
        GenPrologue();
    }
}

void LLVMIRBuilder::HandleGoto(GateRef gate)
{
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    int block = instID2bbID_[acc_.GetId(gate)];
    switch (acc_.GetOpCode(gate)) {
        case OpCode::MERGE:
        case OpCode::LOOP_BEGIN: {
            for (const auto &out : outs) {
                int bbOut = instID2bbID_[acc_.GetId(out)];
                VisitGoto(block, bbOut);
            }
            break;
        }
        default: {
            int bbOut = instID2bbID_[acc_.GetId(outs[0])];
            VisitGoto(block, bbOut);
            break;
        }
    }
}

void LLVMIRBuilder::VisitGoto(int block, int bbOut)
{
    if (block == bbOut) {
        return;
    }
    BasicBlock *bb = EnsureBB(bbOut);
    if (bb == nullptr) {
        OPTIONAL_LOG_COMPILER(ERROR) << " block is nullptr ";
        return;
    }
    llvm::BasicBlock *self = llvm::unwrap(EnsureLBB(bbID2BB_[block].get()));
    llvm::BasicBlock *out = llvm::unwrap(EnsureLBB(bbID2BB_[bbOut].get()));
    llvm::BranchInst::Create(out, self);
    EndCurrentBlock();
}

void LLVMIRBuilder::HandleConstant(GateRef gate)
{
    std::bitset<64> value = acc_.GetConstantValue(gate); // 64: bit width
    VisitConstant(gate, value);
}

void LLVMIRBuilder::VisitConstant(GateRef gate, std::bitset<64> value) // 64: bit width
{
    LLVMValueRef llvmValue = nullptr;
    auto machineType = acc_.GetMachineType(gate);
    if (machineType == MachineType::ARCH) {
        ASSERT(compCfg_->Is64Bit());
        machineType = MachineType::I64;
    }
    if (machineType == MachineType::I32) {
        llvmValue = LLVMConstInt(GetInt32T(), value.to_ulong(), 0);
    } else if (machineType == MachineType::I64) {
        llvmValue = LLVMConstInt(GetInt64T(), value.to_ullong(), 0);
        LLVMTypeRef type = ConvertLLVMTypeFromGate(gate);
        if (LLVMGetTypeKind(type) == LLVMPointerTypeKind) {
            llvmValue = LLVMBuildIntToPtr(builder_, llvmValue, type, "");
        } else if (LLVMGetTypeKind(type) == LLVMIntegerTypeKind) {
            // do nothing
        } else {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    } else if (machineType == MachineType::F64) {
        auto doubleValue = base::bit_cast<double>(value.to_ullong()); // actual double value
        llvmValue = LLVMConstReal(GetDoubleT(), doubleValue);
    } else if (machineType == MachineType::I8) {
        llvmValue = LLVMConstInt(GetInt8T(), value.to_ulong(), 0);
    } else if (machineType == MachineType::I16) {
        llvmValue = LLVMConstInt(GetInt16T(), value.to_ulong(), 0);
    } else if (machineType == MachineType::I1) {
        llvmValue = LLVMConstInt(GetInt1T(), value.to_ulong(), 0);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    Bind(gate, llvmValue);
}

void LLVMIRBuilder::HandleConstString(GateRef gate)
{
    const ChunkVector<char> &str = acc_.GetConstantString(gate); // 64: bit width
    VisitConstString(gate, str);
}

void LLVMIRBuilder::VisitConstString(GateRef gate, const ChunkVector<char> &str) // 64: bit width
{
    ASSERT(acc_.GetMachineType(gate) == MachineType::ARCH);
    LLVMValueRef llvmValue1 = LLVMConstStringInContext(context_, str.data(), str.size(), 0);
    LLVMValueRef addr = LLVMBuildAlloca(builder_, LLVMTypeOf(llvmValue1), "");
    LLVMBuildStore(builder_, llvmValue1, addr);
    Bind(gate, addr);
}

void LLVMIRBuilder::HandleRelocatableData(GateRef gate)
{
    uint64_t value = acc_.TryGetValue(gate);
    VisitRelocatableData(gate, value);
}

void LLVMIRBuilder::VisitRelocatableData(GateRef gate, uint64_t value)
{
    LLVMValueRef globalValue = LLVMAddGlobal(module_, GetInt64T(), "G");
    LLVMSetInitializer(globalValue, LLVMConstInt(GetInt64T(), value, 0));
    Bind(gate, globalValue);
}

void LLVMIRBuilder::HandleZExtInt(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitZExtInt(gate, ins[0]);
}

void LLVMIRBuilder::HandleSExtInt(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitSExtInt(gate, ins[0]);
}

void LLVMIRBuilder::HandleParameter(GateRef gate)
{
    return VisitParameter(gate);
}

void LLVMIRBuilder::VisitParameter(GateRef gate)
{
    int argth = static_cast<int>(acc_.TryGetValue(gate));
    LLVMValueRef value = LLVMGetParam(function_, argth);
    ASSERT(LLVMTypeOf(value) == ConvertLLVMTypeFromGate(gate));
    Bind(gate, value);
    // NOTE: caller put args, otherwise crash
    ASSERT(value != nullptr);
}

void LLVMIRBuilder::SaveJSFuncOnOptJSFuncFrame(LLVMValueRef value)
{
    ASSERT(IsOptimizedJSFunction());
    size_t reservedOffset = 0;
    LLVMValueRef llvmFpAddr = CallingFp(module_, builder_, false);
    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder_, llvmFpAddr, slotType_, "cast_int_t");
    if (circuit_->GetFrameType() == FrameType::OPTIMIZED_JS_FUNCTION_FRAME) {
        reservedOffset = OptimizedJSFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
    } else {
        reservedOffset = FASTJITFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
    }
    LLVMValueRef frameJSFuncSlotAddr = LLVMBuildSub(builder_, frameAddr, LLVMConstInt(slotType_,
        reservedOffset, false), "");
    LLVMValueRef jsFuncAddr = LLVMBuildIntToPtr(builder_, frameJSFuncSlotAddr,
        LLVMPointerType(slotType_, 0), "jsfunc.Addr");
    LLVMValueRef jsFuncValue = LLVMBuildPtrToInt(builder_, value, slotType_, "cast_to_i64");
    LLVMBuildStore(builder_, jsFuncValue, jsFuncAddr);
}

void LLVMIRBuilder::HandleBranch(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    GateRef bTrue = (acc_.GetOpCode(outs[0]) == OpCode::IF_TRUE) ? outs[0] : outs[1];
    GateRef bFalse = (acc_.GetOpCode(outs[0]) == OpCode::IF_FALSE) ? outs[0] : outs[1];
    int bbTrue = instID2bbID_[acc_.GetId(bTrue)];
    int bbFalse = instID2bbID_[acc_.GetId(bFalse)];
    VisitBranch(gate, ins[1], bbTrue, bbFalse);
}

void LLVMIRBuilder::HandleMod(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitMod(gate, g0, g1);
}

void LLVMIRBuilder::VisitMod(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = nullptr;
    ASSERT(ConvertLLVMTypeFromGate(gate) == ConvertLLVMTypeFromGate(e1));
    ASSERT(ConvertLLVMTypeFromGate(gate) == ConvertLLVMTypeFromGate(e2));
    auto machineType = acc_.GetMachineType(gate);
    if (machineType == MachineType::I32 || machineType == MachineType::I64) {
        result = LLVMBuildSRem(builder_, e1Value, e2Value, "");
    } else if (machineType == MachineType::F64) {
        result = LLVMBuildFRem(builder_, e1Value, e2Value, "");
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleFinishAllocate(GateRef gate)
{
    auto g0 = acc_.GetValueIn(gate, 0);
    VisitFinishAllocate(gate, g0);
}

void LLVMIRBuilder::VisitFinishAllocate(GateRef gate, GateRef e1)
{
    LLVMValueRef result = GetLValue(e1);
    Bind(gate, result);
    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitBranch(GateRef gate, GateRef cmp, int btrue, int bfalse)
{
    if (gate2LValue_.count(cmp) == 0) {
        OPTIONAL_LOG_COMPILER(ERROR) << "Branch condition gate is nullptr!";
        return;
    }
    LLVMValueRef cond = GetLValue(cmp);

    BasicBlock *trueBB = EnsureBB(btrue);
    BasicBlock *falseBB = EnsureBB(bfalse);
    EnsureLBB(trueBB);
    EnsureLBB(falseBB);

    LLVMBasicBlockRef llvmTrueBB = trueBB->GetImpl<BasicBlockImpl>()->lBB_;
    LLVMBasicBlockRef llvmFalseBB = falseBB->GetImpl<BasicBlockImpl>()->lBB_;
    LLVMValueRef result = LLVMBuildCondBr(builder_, cond, llvmTrueBB, llvmFalseBB);
    EndCurrentBlock();

    if (enableOptBranchProfiling_ && acc_.HasBranchWeight(gate)) {
        auto trueWeight = acc_.GetTrueWeight(gate);
        auto falseWeight = acc_.GetFalseWeight(gate);
        LLVMMetadataRef branch_weights = LLVMMDStringInContext2(context_, "branch_weights", 14);
        LLVMMetadataRef weight1 = LLVMValueAsMetadata(LLVMConstInt(LLVMIntType(32), trueWeight, 0));
        LLVMMetadataRef weight2 = LLVMValueAsMetadata(LLVMConstInt(LLVMIntType(32), falseWeight, 0));
        LLVMMetadataRef mds[] = {branch_weights, weight1, weight2};
        LLVMMetadataRef metadata = LLVMMDNodeInContext2(context_, mds, 3);
        LLVMValueRef metadata_value = LLVMMetadataAsValue(context_, metadata);
        LLVMSetMetadata(result, LLVMGetMDKindID("prof", 4), metadata_value); // 4: length of "prof"
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleSwitch(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    VisitSwitch(gate, ins[1], outs);
}

void LLVMIRBuilder::VisitSwitch(GateRef gate, GateRef input, const std::vector<GateRef> &outList)
{
    LLVMValueRef cond = GetLValue(input);
    int caseNum = static_cast<int>(outList.size());
    BasicBlock *curOutBB = nullptr;
    LLVMBasicBlockRef llvmDefaultOutBB = nullptr;
    for (int i = 0; i < caseNum; i++) {
        curOutBB = EnsureBB(instID2bbID_[acc_.GetId(outList[i])]);
        EnsureLBB(curOutBB);
        if (acc_.GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            llvmDefaultOutBB = curOutBB->GetImpl<BasicBlockImpl>()->lBB_;
        }
    }
    LLVMValueRef result = LLVMBuildSwitch(builder_, cond, llvmDefaultOutBB, static_cast<uint32_t>(caseNum - 1));
    LLVMBasicBlockRef llvmCurOutBB = nullptr;
    for (int i = 0; i < caseNum; i++) {
        if (acc_.GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            continue;
        }
        curOutBB = EnsureBB(instID2bbID_[acc_.GetId(outList[i])]);
        llvmCurOutBB = curOutBB->GetImpl<BasicBlockImpl>()->lBB_;
        LLVMAddCase(result, LLVMConstInt(ConvertLLVMTypeFromGate(input), acc_.TryGetValue(outList[i]), 0),
                    llvmCurOutBB);
    }
    EndCurrentBlock();
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

unsigned LLVMIRBuilder::GetPtrAddressSpace(LLVMValueRef v) const
{
    auto ty = LLVMTypeOf(v);
    if (LLVMGetTypeKind(ty) == LLVMPointerTypeKind) {
        return LLVMGetPointerAddressSpace(ty);
    }
    return 0;
}

void LLVMIRBuilder::VisitLoad(GateRef gate, GateRef base)
{
    LLVMValueRef baseAddr = GetLValue(base);

    LLVMTypeRef returnType = ConvertLLVMTypeFromGate(gate);
    LLVMTypeRef memType = LLVMPointerType(returnType, GetPtrAddressSpace(baseAddr));
    baseAddr = CanonicalizeToPtr(baseAddr, memType);

    LLVMValueRef result = LLVMBuildLoad(builder_, baseAddr, "");
    auto order = acc_.GetMemoryAttribute(gate);
    switch (order.GetOrder()) {
        case MemoryAttribute::MEMORY_ORDER_RELEASE: {
            LLVMSetOrdering(result, LLVMAtomicOrderingRelease);
            break;
        }
        case MemoryAttribute::NOT_ATOMIC: {
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitStore(GateRef gate, GateRef base, GateRef value)
{
    LLVMValueRef baseAddr = GetLValue(base);
    LLVMValueRef data = GetLValue(value);

    LLVMTypeRef returnType = ConvertLLVMTypeFromGate(value);
    LLVMTypeRef ptrType = LLVMPointerType(returnType, GetPtrAddressSpace(baseAddr));
    baseAddr = CanonicalizeToPtr(baseAddr, ptrType);

    LLVMValueRef result = LLVMBuildStore(builder_, data, baseAddr);
    auto order = acc_.GetMemoryAttribute(gate);
    switch (order.GetOrder()) {
        case MemoryAttribute::MEMORY_ORDER_RELEASE: {
            LLVMSetOrdering(result, LLVMAtomicOrderingRelease);
            break;
        }
        case MemoryAttribute::NOT_ATOMIC: {
            break;
        }
        default: {
            UNREACHABLE();
            break;
        }
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

LLVMValueRef LLVMIRBuilder::CanonicalizeToInt(LLVMValueRef value) const
{
    if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMPointerTypeKind) {
        return LLVMBuildPtrToInt(builder_, value, GetInt64T(), "");
    } else if (LLVMGetTypeKind(LLVMTypeOf(value)) == LLVMIntegerTypeKind) {
        return value;
    } else {
        LOG_COMPILER(FATAL) << "can't Canonicalize to Int64: ";
        UNREACHABLE();
    }
}

LLVMValueRef LLVMIRBuilder::CanonicalizeToPtr(LLVMValueRef value, LLVMTypeRef ptrType) const
{
    LLVMTypeRef valueT = LLVMTypeOf(value);
    if (LLVMGetTypeKind(valueT) == LLVMPointerTypeKind) {
        if (valueT != ptrType) {
            return LLVMBuildPointerCast(builder_, value, ptrType, "");
        }
    } else if (LLVMGetTypeKind(valueT) == LLVMIntegerTypeKind) {
        LLVMValueRef result = LLVMBuildIntToPtr(builder_, value, ptrType, "");
        return result;
    } else {
        LOG_COMPILER(FATAL) << "can't Canonicalize to Ptr: ";
        UNREACHABLE();
    }
    return value;
}

LLVMValueRef LLVMIRBuilder::CanonicalizeToPtr(LLVMValueRef value) const
{
    LLVMTypeRef valueT = LLVMTypeOf(value);
    if (LLVMGetTypeKind(valueT) == LLVMPointerTypeKind) {
        auto ptrType = LLVMPointerType(GetInt8T(), GetPtrAddressSpace(value));
        return LLVMBuildPointerCast(builder_, value, ptrType, "");
    } else if (LLVMGetTypeKind(valueT) == LLVMIntegerTypeKind) {
        LLVMValueRef result = LLVMBuildIntToPtr(builder_, value, GetRawPtrT(), "");
        return result;
    } else {
        LOG_COMPILER(FATAL) << "can't Canonicalize to Ptr: ";
        UNREACHABLE();
    }
}

void LLVMIRBuilder::HandleIntRev(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitIntRev(gate, ins[0]);
}

void LLVMIRBuilder::VisitIntRev(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    ASSERT(ConvertLLVMTypeFromGate(gate) == ConvertLLVMTypeFromGate(e1));
    auto machineType = acc_.GetMachineType(gate);
    LLVMValueRef result = nullptr;
    if (machineType <= MachineType::I64 && machineType >= MachineType::I1) {
        result = LLVMBuildNot(builder_, e1Value, "");
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

bool LLVMIRBuilder::IsLInteger(LLVMValueRef v) const
{
    LLVMTypeRef r = LLVMTypeOf(v);
    return LLVMGetTypeKind(r) == LLVMIntegerTypeKind;
}

bool LLVMIRBuilder::IsLPointer(LLVMValueRef v) const
{
    LLVMTypeRef r = LLVMTypeOf(v);
    return LLVMGetTypeKind(r) == LLVMPointerTypeKind;
}

LLVMValueRef LLVMIRBuilder::PointerAdd(LLVMValueRef baseAddr, LLVMValueRef offsetInByte, LLVMTypeRef rep)
{
    LLVMValueRef ptr = CanonicalizeToPtr(baseAddr);
    LLVMValueRef dstRef8 = LLVMBuildGEP(builder_, ptr, &offsetInByte, 1, "");
    LLVMValueRef result = LLVMBuildPointerCast(builder_, dstRef8, rep, "");
    return result;
}

LLVMTypeRef LLVMIRBuilder::ConvertLLVMTypeFromGate(GateRef gate) const
{
    if (acc_.IsGCRelated(gate)) {
        return GetTaggedHPtrT();
    }
    MachineType t = acc_.GetMachineType(gate);
    switch (t) {
        case MachineType::NOVALUE:
            return GetVoidT();
        case MachineType::I1:
            return GetInt1T();
        case MachineType::I8:
            return GetInt8T();
        case MachineType::I16:
            return GetInt16T();
        case MachineType::I32:
            return GetInt32T();
        case MachineType::I64:
            return GetInt64T();
        case MachineType::F32:
            return GetFloatT();
        case MachineType::F64:
            return GetDoubleT();
        case MachineType::ARCH: {
            return GetInt64T();
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

int64_t LLVMIRBuilder::GetBitWidthFromMachineType(MachineType machineType) const
{
    switch (machineType) {
        case NOVALUE:
            return 0;
        case ARCH:
            return 48;  // 48: Pointer representation in different architectures
        case I1:
            return 1;
        case I8:
            return 8; // 8: bit width
        case I16:
            return 16; // 16: bit width
        case I32:
            return 32; // 32: bit width
        case I64:
            return 64; // 64: bit width
        case F32:
            return 32; // 32: bit width
        case F64:
            return 64; // 64: bit width
        case FLEX:
        case ANYVALUE:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void LLVMIRBuilder::HandleAdd(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitAdd(gate, g0, g1);
}

void LLVMIRBuilder::HandleTruncFloatToInt(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    VisitTruncFloatToInt(gate, g0);
}

void LLVMIRBuilder::VisitTruncFloatToInt(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    auto machineType = acc_.GetMachineType(e1);
    LLVMValueRef result = nullptr;
    if (machineType <= MachineType::F64 && machineType >= MachineType::F32) {
        result = LLVMBuildFPToSI(builder_, e1Value, ConvertLLVMTypeFromGate(gate), "");
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitAdd(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = nullptr;

    LLVMTypeRef returnType = ConvertLLVMTypeFromGate(gate);
    auto machineType = acc_.GetMachineType(gate);
    if (IsAddIntergerType(machineType)) {
        auto e1Type = LLVMGetTypeKind(ConvertLLVMTypeFromGate(e1));
        if (e1Type == LLVMPointerTypeKind) {
            result = PointerAdd(e1Value, e2Value, returnType);
        } else {
            LLVMValueRef tmp1Value = LLVMBuildIntCast2(builder_, e1Value, returnType, 0, "");
            LLVMValueRef tmp2Value = LLVMBuildIntCast2(builder_, e2Value, returnType, 0, "");
            result = LLVMBuildAdd(builder_, tmp1Value, tmp2Value, "");
            if (LLVMTypeOf(tmp1Value) != LLVMTypeOf(tmp2Value)) {
                ASSERT(LLVMTypeOf(tmp1Value) == LLVMTypeOf(tmp2Value));
            }
        }
    } else if (machineType == MachineType::F64) {
        result = LLVMBuildFAdd(builder_, e1Value, e2Value, "");
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleSub(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitSub(gate, g0, g1);
}

void LLVMIRBuilder::VisitSub(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = nullptr;
    auto machineType = acc_.GetMachineType(gate);
    if (machineType == MachineType::I16 || machineType == MachineType::I32 ||
        machineType == MachineType::I64 || machineType == MachineType::ARCH) {
        result = LLVMBuildSub(builder_, e1Value, e2Value, "");
    } else if (machineType == MachineType::F64) {
        result = LLVMBuildFSub(builder_, e1Value, e2Value, "");
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleMul(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitMul(gate, g0, g1);
}

void LLVMIRBuilder::VisitMul(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = nullptr;
    auto machineType = acc_.GetMachineType(gate);
    if (IsMulIntergerType(machineType)) {
        result = LLVMBuildMul(builder_, e1Value, e2Value, "");
    } else if (machineType == MachineType::F64) {
        result = LLVMBuildFMul(builder_, e1Value, e2Value, "");
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleFloatDiv(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitFloatDiv(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntDiv(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntDiv(gate, g0, g1);
}

void LLVMIRBuilder::HandleUDiv(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitUDiv(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntOr(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntOr(gate, g0, g1);
}

void LLVMIRBuilder::HandleFetchOr(GateRef gate)
{
    auto g0 = acc_.GetValueIn(gate, 0);
    auto g1 = acc_.GetValueIn(gate, 1);
    VisitFetchOr(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntXor(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntXor(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntLsr(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntLsr(gate, g0, g1);
}

void LLVMIRBuilder::HandleIntAsr(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntAsr(gate, g0, g1);
}

void LLVMIRBuilder::HandleCmp(GateRef gate)
{
    GateRef left = acc_.GetIn(gate, 0);
    GateRef right = acc_.GetIn(gate, 1);
    VisitCmp(gate, left, right);
}

void LLVMIRBuilder::HandleAddWithOverflow(GateRef gate)
{
    GateRef left = acc_.GetIn(gate, 0);
    GateRef right = acc_.GetIn(gate, 1);
    ASSERT(acc_.GetMachineType(left) == MachineType::I32);
    ASSERT(acc_.GetMachineType(right) == MachineType::I32);
    VisitAddWithOverflow(gate, left, right);
}

void LLVMIRBuilder::VisitAddWithOverflow(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    std::vector<LLVMValueRef> args = { e1Value, e2Value };
    auto fn = LLVMGetNamedFunction(module_, "llvm.sadd.with.overflow.i32");
    if (!fn) {
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = { GetInt32T(), GetInt32T() };
        LLVMTypeRef structTys[] = { GetInt32T(), GetInt1T() };
        LLVMTypeRef returnType = LLVMStructTypeInContext(context_, structTys, 2, 0);
        auto fnTy = LLVMFunctionType(returnType, paramTys1, 2, 0);
        fn = LLVMAddFunction(module_, "llvm.sadd.with.overflow.i32", fnTy);
    }
    LLVMValueRef result = LLVMBuildCall(builder_, fn, args.data(), 2, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleSubWithOverflow(GateRef gate)
{
    GateRef left = acc_.GetIn(gate, 0);
    GateRef right = acc_.GetIn(gate, 1);
    ASSERT(acc_.GetMachineType(left) == MachineType::I32);
    ASSERT(acc_.GetMachineType(right) == MachineType::I32);
    VisitSubWithOverflow(gate, left, right);
}

void LLVMIRBuilder::VisitSubWithOverflow(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    std::vector<LLVMValueRef> args = { e1Value, e2Value };
    auto fn = LLVMGetNamedFunction(module_, "llvm.ssub.with.overflow.i32");
    if (!fn) {
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = { GetInt32T(), GetInt32T() };
        LLVMTypeRef structTys[] = { GetInt32T(), GetInt1T() };
        LLVMTypeRef returnType = LLVMStructTypeInContext(context_, structTys, 2, 0);
        auto fnTy = LLVMFunctionType(returnType, paramTys1, 2, 0);
        fn = LLVMAddFunction(module_, "llvm.ssub.with.overflow.i32", fnTy);
    }
    LLVMValueRef result = LLVMBuildCall(builder_, fn, args.data(), 2, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleMulWithOverflow(GateRef gate)
{
    GateRef left = acc_.GetIn(gate, 0);
    GateRef right = acc_.GetIn(gate, 1);
    ASSERT(acc_.GetMachineType(left) == MachineType::I32);
    ASSERT(acc_.GetMachineType(right) == MachineType::I32);
    VisitMulWithOverflow(gate, left, right);
}

void LLVMIRBuilder::VisitMulWithOverflow(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    std::vector<LLVMValueRef> args = { e1Value, e2Value };
    auto fn = LLVMGetNamedFunction(module_, "llvm.smul.with.overflow.i32");
    if (!fn) {
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = { GetInt32T(), GetInt32T() };
        LLVMTypeRef structTys[] = { GetInt32T(), GetInt1T() };
        LLVMTypeRef returnType = LLVMStructTypeInContext(context_, structTys, 2, 0);
        auto fnTy = LLVMFunctionType(returnType, paramTys1, 2, 0);
        fn = LLVMAddFunction(module_, "llvm.smul.with.overflow.i32", fnTy);
    }
    LLVMValueRef result = LLVMBuildCall(builder_, fn, args.data(), 2, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleExtractValue(GateRef gate)
{
    GateRef pointer = acc_.GetIn(gate, 0);
    GateRef index = acc_.GetIn(gate, 1);
    VisitExtractValue(gate, pointer, index);
}

void LLVMIRBuilder::VisitExtractValue(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    ASSERT((acc_.GetOpCode(e2) == OpCode::CONSTANT) && acc_.GetMachineType(e2) == MachineType::I32);
    uint32_t index = static_cast<uint32_t>(acc_.GetConstantValue(e2));
    LLVMValueRef result = LLVMBuildExtractValue(builder_, e1Value, index, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleSqrt(GateRef gate)
{
    GateRef param = acc_.GetIn(gate, 0);
    VisitSqrt(gate, param);
}

void LLVMIRBuilder::VisitSqrt(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    std::vector<LLVMValueRef> args = { e1Value };
    auto fn = LLVMGetNamedFunction(module_, "llvm.sqrt.f64");
    if (!fn) {
        /* init instrinsic function declare */
        LLVMTypeRef paramTys1[] = { GetDoubleT() };
        auto fnTy = LLVMFunctionType(GetDoubleT(), paramTys1, 1, 0);
        fn = LLVMAddFunction(module_, "llvm.sqrt.f64", fnTy);
    }
    LLVMValueRef result = LLVMBuildCall(builder_, fn, args.data(), 1, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleExp(GateRef gate)
{
    GateRef base = acc_.GetIn(gate, 0U);
    GateRef power = acc_.GetIn(gate, 1U);
    VisitExp(gate, base, power);
}

void LLVMIRBuilder::VisitExp([[maybe_unused]] GateRef gate, [[maybe_unused]] GateRef e1, [[maybe_unused]] GateRef e2)
{
#ifdef SUPPORT_LLVM_INTRINSICS_WITH_CALLS
    llvm::Value *e1Value = llvm::unwrap(GetLValue(e1));
    llvm::Value *e2Value = llvm::unwrap(GetLValue(e2));

    [[maybe_unused]] auto machineType = acc_.GetMachineType(gate);
    ASSERT(machineType == MachineType::F64);
    ASSERT(acc_.GetMachineType(e1) == machineType);
    ASSERT(acc_.GetMachineType(e2) == machineType);

    llvm::Value *result = nullptr;

    constexpr double one = 1.0;
    if (acc_.IsConstant(e1) && acc_.GetFloat64FromConstant(e1) == std::exp(one)) {
        llvm::Intrinsic::ID llvmId = llvm::Intrinsic::exp;
        result = llvm::unwrap(builder_)->CreateUnaryIntrinsic(llvmId, e2Value);
    } else {
        llvm::Intrinsic::ID llvmId = llvm::Intrinsic::pow;
        result = llvm::unwrap(builder_)->CreateBinaryIntrinsic(llvmId, e1Value, e2Value);
    }
#else
    UNREACHABLE();
#endif
}

void LLVMIRBuilder::HandleCeil(GateRef gate)
{
    GateRef param = acc_.GetIn(gate, 0);
    VisitCeil(gate, param);
}

void LLVMIRBuilder::VisitCeil(GateRef gate, GateRef e1)
{
    llvm::Value *e1Value = llvm::unwrap(GetLValue(e1));
    ASSERT(acc_.GetMachineType(e1) == acc_.GetMachineType(gate));
    llvm::Intrinsic::ID llvmId = llvm::Intrinsic::ceil;
    llvm::Value *result = llvm::unwrap(builder_)->CreateUnaryIntrinsic(llvmId, e1Value);
    Bind(gate, llvm::wrap(result));

    if (IsLogEnabled()) {
        SetDebugInfo(gate, llvm::wrap(result));
    }
}

template<typename... Ts>
static llvm::CallInst *BuildLLVMIntrinsic(llvm::IRBuilder<> *builder, llvm::Intrinsic::ID llvmId, Ts... inputs)
{
    static_assert((std::is_same_v<Ts, llvm::Value *> && ...));
    if constexpr (sizeof...(inputs) == 1) {
        return builder->CreateUnaryIntrinsic(llvmId, inputs...);
    } else {
        static_assert(sizeof...(inputs) == 2);
        return builder->CreateBinaryIntrinsic(llvmId, inputs...);
    }
}

void LLVMIRBuilder::HandleAbs(GateRef gate)
{
    VisitAbs(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::VisitAbs(GateRef gate, GateRef e1)
{
    auto machineType = acc_.GetMachineType(gate);
    ASSERT(acc_.GetMachineType(e1) == machineType);
    llvm::Intrinsic::ID llvmId = 0;
    auto *builder = llvm::unwrap(builder_);
    llvm::Value *value = llvm::unwrap(GetLValue(e1));
    LLVMValueRef result;
    if (machineType == MachineType::I32) {
        llvmId = llvm::Intrinsic::abs;
        llvm::Type *type = llvm::Type::getInt1Ty(*llvm::unwrap(context_));
        llvm::Value *poison = llvm::Constant::getIntegerValue(type, llvm::APInt(1, 0));
        result = llvm::wrap(BuildLLVMIntrinsic(builder, llvmId, value, poison));
    } else if (machineType == MachineType::F64) {
        llvmId = llvm::Intrinsic::fabs;
        result = llvm::wrap(BuildLLVMIntrinsic(builder, llvmId, value));
    } else {
        LOG_ECMA(FATAL) << "`Abs` type should be untagged double or signed int";
        UNREACHABLE();
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleMin(GateRef gate)
{
    VisitMin(gate, acc_.GetIn(gate, 0), acc_.GetIn(gate, 1U));
}

void LLVMIRBuilder::VisitMin(GateRef gate, GateRef e1, GateRef e2)
{
    auto machineType = acc_.GetMachineType(gate);
    ASSERT(acc_.GetMachineType(e1) == machineType);
    ASSERT(acc_.GetMachineType(e2) == machineType);
    llvm::Intrinsic::ID llvmId = 0;
    if (machineType == MachineType::I32) {
        llvmId = llvm::Intrinsic::smin;
    } else if (machineType == MachineType::F64) {
        llvmId = llvm::Intrinsic::minimum;
    } else {
        LOG_ECMA(FATAL) << "`Min` type should be untagged double or signed int";
        UNREACHABLE();
    }
    VisitIntrinsic(gate, llvmId, e1, e2);
}

void LLVMIRBuilder::HandleMax(GateRef gate)
{
    VisitMax(gate, acc_.GetIn(gate, 0), acc_.GetIn(gate, 1U));
}

void LLVMIRBuilder::VisitMax(GateRef gate, GateRef e1, GateRef e2)
{
    auto machineType = acc_.GetMachineType(gate);
    ASSERT(acc_.GetMachineType(e1) == machineType);
    ASSERT(acc_.GetMachineType(e2) == machineType);
    llvm::Intrinsic::ID llvmId = 0;
    if (machineType == MachineType::I32) {
        llvmId = llvm::Intrinsic::smax;
    } else if (machineType == MachineType::F64) {
        llvmId = llvm::Intrinsic::maximum;
    } else {
        LOG_ECMA(FATAL) << "`Max` type should be untagged double or signed int";
        UNREACHABLE();
    }
    VisitIntrinsic(gate, llvmId, e1, e2);
}

void LLVMIRBuilder::HandleFloor(GateRef gate)
{
    VisitIntrinsic(gate, llvm::Intrinsic::floor, acc_.GetIn(gate, 0));
}

template<typename... Ts>
void LLVMIRBuilder::VisitIntrinsic(GateRef gate, llvm::Intrinsic::ID llvmId, Ts... inputs)
{
    static_assert((std::is_same_v<Ts, GateRef> && ...));

    auto *builder = llvm::unwrap(builder_);
    LLVMValueRef result = llvm::wrap(BuildLLVMIntrinsic(builder, llvmId, llvm::unwrap(GetLValue(inputs))...));
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

LLVMIntPredicate LLVMIRBuilder::ConvertLLVMPredicateFromICMP(ICmpCondition cond)
{
    switch (cond) {
        case ICmpCondition::SLT:
            return LLVMIntSLT;
        case ICmpCondition::SLE:
            return LLVMIntSLE;
        case ICmpCondition::SGT:
            return LLVMIntSGT;
        case ICmpCondition::SGE:
            return LLVMIntSGE;
        case ICmpCondition::ULT:
            return LLVMIntULT;
        case ICmpCondition::ULE:
            return LLVMIntULE;
        case ICmpCondition::UGT:
            return LLVMIntUGT;
        case ICmpCondition::UGE:
            return LLVMIntUGE;
        case ICmpCondition::NE:
            return LLVMIntNE;
        case ICmpCondition::EQ:
            return LLVMIntEQ;
        default:
            LOG_COMPILER(FATAL) << "unexpected cond!";
            UNREACHABLE();
    }
    return LLVMIntEQ;
}

LLVMRealPredicate LLVMIRBuilder::ConvertLLVMPredicateFromFCMP(FCmpCondition cond)
{
    switch (cond) {
        case FCmpCondition::OLT:
            return LLVMRealOLT;
        case FCmpCondition::OLE:
            return LLVMRealOLE;
        case FCmpCondition::OGT:
            return LLVMRealOGT;
        case FCmpCondition::OGE:
            return LLVMRealOGE;
        case FCmpCondition::ONE:
            return LLVMRealONE;
        case FCmpCondition::OEQ:
            return LLVMRealOEQ;
        default:
            LOG_COMPILER(FATAL) << "unexpected cond!";
            UNREACHABLE();
    }
    return LLVMRealOEQ;
}

void LLVMIRBuilder::VisitCmp(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = nullptr;
    [[maybe_unused]] auto e1ValCode = acc_.GetMachineType(e1);
    [[maybe_unused]] auto e2ValCode = acc_.GetMachineType(e2);
    ASSERT((e1ValCode == e2ValCode) ||
        (compCfg_->Is64Bit() && (e1ValCode == MachineType::ARCH) && (e2ValCode == MachineType::I64)) ||
        (compCfg_->Is64Bit() && (e2ValCode == MachineType::ARCH) && (e1ValCode == MachineType::I64)));
    LLVMIntPredicate intOpcode = LLVMIntEQ;
    LLVMRealPredicate realOpcode = LLVMRealPredicateFalse;
    auto op = acc_.GetOpCode(gate);
    if (op == OpCode::ICMP) {
        auto cond = acc_.GetICmpCondition(gate);
        intOpcode = ConvertLLVMPredicateFromICMP(cond);
        result = LLVMBuildICmp(builder_, intOpcode, e1Value, e2Value, "");
    } else if (op == OpCode::FCMP) {
        auto cond = acc_.GetFCmpCondition(gate);
        realOpcode = ConvertLLVMPredicateFromFCMP(cond);
        result = LLVMBuildFCmp(builder_, realOpcode, e1Value, e2Value, "");
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleLoad(GateRef gate)
{
    VisitLoad(gate, acc_.GetIn(gate, 1));
}

void LLVMIRBuilder::HandleStore(GateRef gate)
{
    GateRef addr = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    VisitStore(gate, addr, value);
}

void LLVMIRBuilder::HandleChangeInt32ToDouble(GateRef gate)
{
    VisitChangeInt32ToDouble(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::HandleChangeUInt32ToDouble(GateRef gate)
{
    VisitChangeUInt32ToDouble(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::HandleChangeDoubleToInt32(GateRef gate)
{
    VisitChangeDoubleToInt32(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::HandleChangeTaggedPointerToInt64(GateRef gate)
{
    VisitChangeTaggedPointerToInt64(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::HandleChangeInt64ToTagged(GateRef gate)
{
    VisitChangeInt64ToTagged(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::HandleDoubleTrunc(GateRef gate)
{
    GateRef param = acc_.GetIn(gate, 0);
    VisitDoubleTrunc(gate, param);
}

void LLVMIRBuilder::VisitDoubleTrunc(GateRef gate, GateRef e1)
{
    llvm::Value *e1Value = llvm::unwrap(GetLValue(e1));
    ASSERT(acc_.GetMachineType(e1) == acc_.GetMachineType(gate));
    llvm::Intrinsic::ID llvmId = llvm::Intrinsic::trunc;
    llvm::Value *result = llvm::unwrap(builder_)->CreateUnaryIntrinsic(llvmId, e1Value);
    Bind(gate, llvm::wrap(result));

    if (IsLogEnabled()) {
        SetDebugInfo(gate, llvm::wrap(result));
    }
}

void LLVMIRBuilder::VisitIntDiv(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = LLVMBuildSDiv(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitUDiv(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = LLVMBuildUDiv(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitFloatDiv(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);

    LLVMValueRef result = LLVMBuildFDiv(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitIntOr(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = LLVMBuildOr(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitFetchOr(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    e1Value = CanonicalizeToPtr(e1Value, LLVMPointerType(LLVMTypeOf(e2Value), 0));
    auto order = acc_.GetMemoryAttribute(gate);
    LLVMAtomicOrdering atomic_order = LLVMAtomicOrderingSequentiallyConsistent;
    switch (order.GetOrder()) {
        case MemoryAttribute::NOT_ATOMIC: {
            atomic_order = LLVMAtomicOrderingMonotonic;
            break;
        }
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
    LLVMValueRef result = LLVMBuildAtomicRMW(builder_, LLVMAtomicRMWBinOpOr, e1Value, e2Value, atomic_order, false);
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleIntAnd(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntAnd(gate, g0, g1);
}

void LLVMIRBuilder::VisitIntAnd(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = LLVMBuildAnd(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitIntXor(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = LLVMBuildXor(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitIntLsr(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = LLVMBuildLShr(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitIntAsr(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = LLVMBuildAShr(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleIntLsl(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntLsl(gate, g0, g1);
}

void LLVMIRBuilder::VisitIntLsl(GateRef gate, GateRef e1, GateRef e2)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef e2Value = GetLValue(e2);
    LLVMValueRef result = LLVMBuildShl(builder_, e1Value, e2Value, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitZExtInt(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(e1)) <=
           GetBitWidthFromMachineType(acc_.GetMachineType(gate)));
    LLVMValueRef result = LLVMBuildZExt(builder_, e1Value, ConvertLLVMTypeFromGate(gate), "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitSExtInt(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef result = LLVMBuildSExt(builder_, e1Value, ConvertLLVMTypeFromGate(gate), "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleCastIntXToIntY(GateRef gate)
{
    VisitCastIntXToIntY(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::VisitCastIntXToIntY(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(e1)) >=
           GetBitWidthFromMachineType(acc_.GetMachineType(gate)));
    LLVMValueRef result = LLVMBuildIntCast2(builder_, e1Value, ConvertLLVMTypeFromGate(gate), 1, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleFPExt(GateRef gate)
{
    VisitFPExt(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::VisitFPExt(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(e1)) <=
           GetBitWidthFromMachineType(acc_.GetMachineType(gate)));
    LLVMValueRef result = LLVMBuildFPExt(builder_, e1Value, ConvertLLVMTypeFromGate(gate), "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleFPTrunc(GateRef gate)
{
    VisitFPTrunc(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::VisitFPTrunc(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(e1)) >=
           GetBitWidthFromMachineType(acc_.GetMachineType(gate)));
    LLVMValueRef result = LLVMBuildFPTrunc(builder_, e1Value, ConvertLLVMTypeFromGate(gate), "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitChangeInt32ToDouble(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef result = LLVMBuildSIToFP(builder_, e1Value, ConvertLLVMTypeFromGate(gate), "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitChangeUInt32ToDouble(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef result = LLVMBuildUIToFP(builder_, e1Value, GetDoubleT(), "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitChangeDoubleToInt32(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef result = LLVMBuildFPToSI(builder_, e1Value, GetInt32T(), "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitChangeTaggedPointerToInt64(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    LLVMValueRef result = CanonicalizeToInt(e1Value);
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::VisitChangeInt64ToTagged(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    ASSERT(LLVMGetTypeKind(LLVMTypeOf(e1Value)) == LLVMIntegerTypeKind);
    LLVMValueRef result = LLVMBuildIntToPtr(builder_, e1Value, GetTaggedHPtrT(), "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleBitCast(GateRef gate)
{
    VisitBitCast(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::VisitBitCast(GateRef gate, GateRef e1)
{
    LLVMValueRef e1Value = GetLValue(e1);
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(gate)) ==
           GetBitWidthFromMachineType(acc_.GetMachineType(e1)));
    auto returnType = ConvertLLVMTypeFromGate(gate);
    LLVMValueRef result = LLVMBuildBitCast(builder_, e1Value, returnType, "");
    Bind(gate, result);

    if (IsLogEnabled()) {
        SetDebugInfo(gate, result);
    }
}

void LLVMIRBuilder::HandleDeoptCheck(GateRef gate)
{
    int block = instID2bbID_[acc_.GetId(gate)];
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    int bbOut = instID2bbID_[acc_.GetId(outs[0])]; // 0: output

    BasicBlock *trueBB = EnsureBB(bbOut);
    LLVMBasicBlockRef llvmTrueBB = EnsureLBB(trueBB);
    std::string buf = "deopt if false B" + std::to_string(block);
    LLVMBasicBlockRef llvmFalseBB = LLVMAppendBasicBlock(function_, buf.c_str());
    GateRef cmp = acc_.GetValueIn(gate, 0); // 0: cond
    LLVMValueRef cond = GetLValue(cmp);
    LLVMValueRef result = LLVMBuildCondBr(builder_, cond, llvmTrueBB, llvmFalseBB);

    if (enableOptBranchProfiling_) {
        LLVMMetadataRef branch_weights = LLVMMDStringInContext2(context_, "branch_weights", 14);
        LLVMMetadataRef weight1 = LLVMValueAsMetadata(LLVMConstInt(LLVMIntType(32), BranchWeight::DEOPT_WEIGHT, 0));
        LLVMMetadataRef weight2 = LLVMValueAsMetadata(LLVMConstInt(LLVMIntType(32), BranchWeight::ONE_WEIGHT, 0));
        LLVMMetadataRef mds[] = {branch_weights, weight1, weight2};
        LLVMMetadataRef metadata = LLVMMDNodeInContext2(context_, mds, 3);  // 3: size of mds
        LLVMValueRef metadata_value = LLVMMetadataAsValue(context_, metadata);
        LLVMSetMetadata(result, LLVMGetMDKindID("prof", 4), metadata_value);    // 4: length of "prof"
    }

    EndCurrentBlock();

    LLVMPositionBuilderAtEnd(builder_, llvmFalseBB);
    LLVMBasicBlockRef preLBB = EnsureLBB(EnsureBB(block));
    LLVMMoveBasicBlockBefore(preLBB, llvmFalseBB);

    VisitDeoptCheck(gate);
    LLVMValueRef returnValue = GetLValue(gate);
    if (IsLogEnabled()) {
        SetDebugInfo(gate, returnValue);
    }
    LLVMBuildRet(builder_, returnValue);
    Bind(gate, result);
}

void LLVMIRBuilder::HandleClz32(GateRef gate)
{
    VisitClz32(gate, acc_.GetIn(gate, 0));
}

void LLVMIRBuilder::VisitClz32(GateRef gate,  GateRef param)
{
    LLVMValueRef value = GetLValue(param);
    LLVMValueRef trueConst = LLVMConstInt(GetInt1T(), 0, true);

    llvm::CallInst *result = llvm::unwrap(builder_)->CreateBinaryIntrinsic(llvm::Intrinsic::ctlz,
                                                                           llvm::unwrap(value),
                                                                           llvm::unwrap(trueConst));
    Bind(gate, llvm::wrap(result));

    if (IsLogEnabled()) {
        SetDebugInfo(gate, value);
    }
}

LLVMTypeRef LLVMIRBuilder::GetExperimentalDeoptTy()
{
    auto fnTy = LLVMFunctionType(GetTaggedHPtrT(), nullptr, 0, 1);
    return fnTy;
}

LLVMValueRef LLVMModule::GetDeoptFunction()
{
    auto fn = LLVMGetNamedFunction(module_, Deoptimizier::GetLLVMDeoptRelocateSymbol());
    return fn;
}

void LLVMIRBuilder::GenDeoptEntry(LLVMModuleRef &module)
{
    // glue type maybeAcc
    std::vector<LLVMTypeRef> paramTys = { GetInt64T(), GetInt64T(), GetInt64T() };
    auto funcType = LLVMFunctionType(GetInt64T(), paramTys.data(),  paramTys.size(), 0);
    auto function = LLVMAddFunction(module, Deoptimizier::GetLLVMDeoptRelocateSymbol(), funcType);
    LLVMSetFunctionCallConv(function, LLVMCCallConv);
    llvmModule_->SetFunction(LLVMModule::kDeoptEntryOffset, function, false);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context_, function, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilderInContext(context_);
    LLVMPositionBuilderAtEnd(builder, entry);

    auto reservedSlotsSize = OptimizedFrame::ComputeReservedSize(slotSize_);
    LLVMAddTargetDependentFunctionAttr(function, "frame-reserved-slots", std::to_string(reservedSlotsSize).c_str());
    SaveFrameTypeOnFrame(FrameType::OPTIMIZED_FRAME, builder);

    LLVMValueRef glue = LLVMGetParam(function, 0);
    LLVMValueRef check = LLVMGetParam(function, 1);
    LLVMValueRef maybeAcc = LLVMGetParam(function, 2);  // 2: maybeAcc

    StubIdType stubId = RTSTUB_ID(DeoptHandlerAsm);
    int stubIndex = static_cast<int>(std::get<RuntimeStubCSigns::ID>(stubId));
    LLVMValueRef rtoffset = LLVMBuildAdd(builder, glue, GetRTStubOffset(glue, stubIndex), "");
    LLVMValueRef patchAddr = LLVMBuildIntToPtr(builder, rtoffset, GetTaggedPtrT(), "");
    LLVMValueRef llvmAddr = LLVMBuildLoad(builder, patchAddr, "");
    LLVMTypeRef rtfuncTypePtr = LLVMPointerType(funcType, 0);
    LLVMValueRef callee = LLVMBuildIntToPtr(builder, llvmAddr, rtfuncTypePtr, "");
    std::vector<LLVMValueRef> params = {glue, check, maybeAcc};
    LLVMValueRef runtimeCall = LLVMBuildCall2(builder, funcType, callee, params.data(), params.size(), "");
    LLVMBuildRet(builder, runtimeCall);
    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMDisposeBuilder(builder);
}

LLVMValueRef LLVMIRBuilder::GetExperimentalDeopt(LLVMModuleRef &module)
{
    /* 0:calling 1:its caller */
    auto fn = LLVMGetNamedFunction(module, "llvm.experimental.deoptimize.p1i64");
    if (!fn) {
        auto fnTy = GetExperimentalDeoptTy();
        fn = LLVMAddFunction(module, "llvm.experimental.deoptimize.p1i64", fnTy);
        GenDeoptEntry(module);
    }
    return fn;
}

LLVMValueRef LLVMIRBuilder::ConvertBoolToTaggedBoolean(GateRef gate)
{
    LLVMValueRef value = GetLValue(gate);
    LLVMValueRef e1Value = LLVMBuildZExt(builder_, value, GetInt64T(), "");
    auto tagMask = LLVMConstInt(GetInt64T(), JSTaggedValue::TAG_BOOLEAN_MASK, 0);
    LLVMValueRef result = LLVMBuildOr(builder_, e1Value, tagMask, "");
    return LLVMBuildIntToPtr(builder_, result, GetTaggedHPtrT(), "");
}

LLVMValueRef LLVMIRBuilder::ConvertInt32ToTaggedInt(GateRef gate)
{
    LLVMValueRef value = GetLValue(gate);
    return ConvertInt32ToTaggedInt(value);
}

LLVMValueRef LLVMIRBuilder::ConvertInt32ToTaggedInt(LLVMValueRef value)
{
    LLVMValueRef e1Value = LLVMBuildSExt(builder_, value, GetInt64T(), "");
    auto tagMask = LLVMConstInt(GetInt64T(), JSTaggedValue::TAG_INT, 0);
    LLVMValueRef result = LLVMBuildOr(builder_, e1Value, tagMask, "");
    return LLVMBuildIntToPtr(builder_, result, GetTaggedHPtrT(), "");
}

LLVMValueRef LLVMIRBuilder::ConvertFloat64ToTaggedDouble(GateRef gate)
{
    LLVMValueRef value = GetLValue(gate);
    LLVMValueRef e1Value = LLVMBuildBitCast(builder_, value, GetInt64T(), "");
    auto offset = LLVMConstInt(GetInt64T(), JSTaggedValue::DOUBLE_ENCODE_OFFSET, 0);
    LLVMValueRef result = LLVMBuildAdd(builder_, e1Value, offset, "");
    return LLVMBuildIntToPtr(builder_, result, GetTaggedHPtrT(), "");
}

LLVMValueRef LLVMIRBuilder::ConvertToTagged(GateRef gate)
{
    auto machineType = acc_.GetMachineType(gate);
    switch (machineType) {
        case MachineType::I1:
            return ConvertBoolToTaggedBoolean(gate);
        case MachineType::I32:
            return ConvertInt32ToTaggedInt(gate);
        case MachineType::F64:
            return ConvertFloat64ToTaggedDouble(gate);
        case MachineType::I64:
            break;
        default:
            LOG_COMPILER(FATAL) << "unexpected machineType!";
            UNREACHABLE();
            break;
    }
    return gate2LValue_.at(gate);
}

void LLVMIRBuilder::SaveDeoptVregInfo(std::vector<LLVMValueRef> &values, int32_t index, size_t curDepth, size_t shift,
                                      GateRef gate)
{
    int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(index, curDepth, shift);
    values.emplace_back(LLVMConstInt(GetInt32T(), encodeIndex, false));
    values.emplace_back(ConvertToTagged(gate));
}

void LLVMIRBuilder::SaveDeoptVregInfoWithI64(std::vector<LLVMValueRef> &values, int32_t index, size_t curDepth,
                                             size_t shift, GateRef gate)
{
    LLVMValueRef value = LLVMBuildIntCast2(builder_, gate2LValue_.at(gate), GetInt32T(), 1, "");
    int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(index, curDepth, shift);
    values.emplace_back(LLVMConstInt(GetInt32T(), encodeIndex, false));
    values.emplace_back(ConvertInt32ToTaggedInt(value));
}

void LLVMIRBuilder::GetDeoptBundleInfo(GateRef deoptFrameState, std::vector<LLVMValueRef> &values)
{
    if (acc_.GetOpCode(deoptFrameState) != OpCode::FRAME_STATE) {
        return;
    }

    // inline depth
    size_t maxDepth = acc_.GetFrameDepth(deoptFrameState, OpCode::FRAME_STATE);
    uint32_t specInlineDepthIndex = static_cast<uint32_t>(SpecVregIndex::INLINE_DEPTH);
    LLVMValueRef depthValue = LLVMConstInt(GetInt32T(), maxDepth, false);
    values.emplace_back(LLVMConstInt(GetInt32T(), static_cast<uint64_t>(specInlineDepthIndex), false));
    values.emplace_back(depthValue);

    size_t shift = Deoptimizier::ComputeShift(maxDepth);
    GateRef frameState = deoptFrameState;
    ArgumentAccessor *argAcc = const_cast<Circuit *>(circuit_)->GetArgumentAccessor();
    for (int32_t curDepth = static_cast<int32_t>(maxDepth); curDepth >= 0; curDepth--) {
        ASSERT(acc_.GetOpCode(frameState) == OpCode::FRAME_STATE);
        GateRef frameValues = acc_.GetValueIn(frameState, 1); // 1: frame values
        const size_t numValueIn = acc_.GetNumValueIn(frameValues);
        ASSERT(numValueIn > 1);
        const size_t envIndex = numValueIn - 2; // 2: env valueIn index
        const size_t accIndex = numValueIn - 1; // 1: acc valueIn index
        GateRef env = acc_.GetValueIn(frameValues, envIndex);
        GateRef acc = acc_.GetValueIn(frameValues, accIndex);
        auto pc = acc_.TryGetPcOffset(frameState);
        GateRef jsFunc = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
        GateRef newTarget = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::NEW_TARGET);
        GateRef thisObj = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::THIS_OBJECT);
        GateRef actualArgc = argAcc->GetFrameArgsIn(frameState, FrameArgIdx::ACTUAL_ARGC);
        // vreg
        for (size_t i = 0; i < envIndex; i++) {
            GateRef vregValue = acc_.GetValueIn(frameValues, i);
            if (acc_.IsConstantTaggedValue(vregValue, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
                continue;
            }
            SaveDeoptVregInfo(values, i, curDepth, shift, vregValue);
        }
        // env
        if (!acc_.IsConstantTaggedValue(env, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
            int32_t specEnvVregIndex = static_cast<int32_t>(SpecVregIndex::ENV_INDEX);
            SaveDeoptVregInfo(values, specEnvVregIndex, curDepth, shift, env);
        }
        // acc
        if (!acc_.IsConstantTaggedValue(acc, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
            int32_t specAccVregIndex = static_cast<int32_t>(SpecVregIndex::ACC_INDEX);
            SaveDeoptVregInfo(values, specAccVregIndex, curDepth, shift, acc);
        }
        // pc offset
        int32_t specPcOffsetIndex = static_cast<int32_t>(SpecVregIndex::PC_OFFSET_INDEX);
        int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(specPcOffsetIndex, curDepth, shift);
        values.emplace_back(LLVMConstInt(GetInt32T(), encodeIndex, false));
        values.emplace_back(LLVMConstInt(GetInt32T(), pc, false));
        // func
        int32_t specCallTargetIndex = static_cast<int32_t>(SpecVregIndex::FUNC_INDEX);
        SaveDeoptVregInfo(values, specCallTargetIndex, curDepth, shift, jsFunc);
        // newTarget
        int32_t specNewTargetIndex = static_cast<int32_t>(SpecVregIndex::NEWTARGET_INDEX);
        SaveDeoptVregInfo(values, specNewTargetIndex, curDepth, shift, newTarget);
        // this object
        int32_t specThisIndex = static_cast<int32_t>(SpecVregIndex::THIS_OBJECT_INDEX);
        SaveDeoptVregInfo(values, specThisIndex, curDepth, shift, thisObj);
        int32_t specArgcIndex = static_cast<int32_t>(SpecVregIndex::ACTUAL_ARGC_INDEX);
        SaveDeoptVregInfoWithI64(values, specArgcIndex, curDepth, shift, actualArgc);
        frameState = acc_.GetFrameState(frameState);
    }
}

void LLVMIRBuilder::VisitDeoptCheck(GateRef gate)
{
    LLVMValueRef glue = gate2LValue_.at(glue_);
    GateRef deoptFrameState = acc_.GetValueIn(gate, 1); // 1: frame state
    ASSERT(acc_.GetOpCode(deoptFrameState) == OpCode::FRAME_STATE);
    std::vector<LLVMValueRef> params;
    params.push_back(glue); // glue
    GateRef deoptType = acc_.GetValueIn(gate, 2); // 2: deopt type
    uint64_t v = acc_.GetConstantValue(deoptType);
    params.push_back(LLVMConstInt(GetInt32T(), static_cast<uint32_t>(v), false)); // deoptType
    LLVMValueRef undefined = LLVMConstInt(GetInt64T(), JSTaggedValue::VALUE_UNDEFINED, false);
    params.push_back(LLVMBuildIntToPtr(builder_, undefined, GetTaggedHPtrT(), "")); // maybeAcc
    LLVMValueRef callee = GetExperimentalDeopt(module_);
    LLVMTypeRef funcType = GetExperimentalDeoptTy();

    std::vector<LLVMValueRef> values;
    GetDeoptBundleInfo(deoptFrameState, values);
    LLVMValueRef runtimeCall =
        LLVMBuildCall3(builder_, funcType, callee, params.data(), params.size(), "", values.data(), values.size());
    Bind(gate, runtimeCall);
}

LLVMModule::LLVMModule(NativeAreaAllocator* allocator, const std::string &name, bool logDbg, const std::string &triple)
    : IRModule(allocator, logDbg, triple)
{
    context_ = LLVMContextCreate();
    module_ = LLVMModuleCreateWithNameInContext(name.c_str(), context_);
    LLVMSetTarget(module_, triple.c_str());
    dBuilder_ = LLVMCreateDIBuilder(module_);
    dFileMD_ = LLVMDIBuilderCreateFile(dBuilder_, name.c_str(), name.size(), ".", 1);
    dUnitMD_ = LLVMDIBuilderCreateCompileUnit(dBuilder_, LLVMDWARFSourceLanguageC_plus_plus, dFileMD_, "ArkCompiler",
                                              0, 0, NULL, 0, 0, NULL, 0, LLVMDWARFEmissionFull,
                                              0, 0, 0, "/", 1, "", 0);

    voidT_ = LLVMVoidTypeInContext(context_);
    int1T_ = LLVMInt1TypeInContext(context_);
    int8T_ = LLVMInt8TypeInContext(context_);
    int16T_ = LLVMInt16TypeInContext(context_);
    int32T_ = LLVMInt32TypeInContext(context_);
    int64T_ = LLVMInt64TypeInContext(context_);
    floatT_ = LLVMFloatTypeInContext(context_);
    doubleT_ = LLVMDoubleTypeInContext(context_);
    taggedHPtrT_ = LLVMPointerType(LLVMInt64TypeInContext(context_), 1);
    taggedPtrT_ = LLVMPointerType(LLVMInt64TypeInContext(context_), 0);
    rawPtrT_ = LLVMPointerType(LLVMInt8TypeInContext(context_), 0);
}

LLVMModule::~LLVMModule()
{
    if (module_ != nullptr) {
        LLVMDisposeModule(module_);
        module_ = nullptr;
    }
    if (context_ != nullptr) {
        LLVMContextDispose(context_);
        context_ = nullptr;
    }
    if (dBuilder_ != nullptr) {
        LLVMDisposeDIBuilder(dBuilder_);
        dBuilder_ = nullptr;
    }
}

void LLVMModule::InitialLLVMFuncTypeAndFuncByModuleCSigns()
{
    for (size_t i = 0; i < callSigns_.size(); i++) {
        const CallSignature* cs = callSigns_[i];
        ASSERT(!cs->GetName().empty());
        LLVMValueRef value = AddAndGetFunc(cs);
        SetFunction(i, value, false);
    }
}

void LLVMModule::SetUpForCommonStubs()
{
    CommonStubCSigns::GetCSigns(callSigns_);
    InitialLLVMFuncTypeAndFuncByModuleCSigns();
}

void LLVMModule::SetUpForBytecodeHandlerStubs()
{
    BytecodeStubCSigns::GetNormalCSigns(callSigns_);
    InitialLLVMFuncTypeAndFuncByModuleCSigns();
}

void LLVMModule::SetUpForBytecodeStwCopyHandlerStubs()
{
    BytecodeStubCSigns::GetStwCopyCSigns(callSigns_);
    InitialLLVMFuncTypeAndFuncByModuleCSigns();
}

void LLVMModule::SetUpForBuiltinsStubs()
{
    BuiltinsStubCSigns::GetCSigns(callSigns_);
    InitialLLVMFuncTypeAndFuncByModuleCSigns();
}

void LLVMModule::SetUpForBaselineStubs()
{
    BaselineStubCSigns::GetCSigns(callSigns_);
    InitialLLVMFuncTypeAndFuncByModuleCSigns();
}

LLVMValueRef LLVMModule::AddAndGetFunc(const CallSignature *stubDescriptor)
{
    auto funcType = GetFuncType(stubDescriptor);
    return LLVMAddFunction(module_, stubDescriptor->GetName().c_str(), funcType);
}

LLVMTypeRef LLVMModule::GetFuncType(const CallSignature *stubDescriptor)
{
    LLVMTypeRef returnType = ConvertLLVMTypeFromVariableType(stubDescriptor->GetReturnType());
    std::vector<LLVMTypeRef> paramTys;
    auto paramCount = stubDescriptor->GetParametersCount();
    int extraParameterCnt = 0;
    auto paramsType = stubDescriptor->GetParametersType();
    if (paramsType != nullptr) {
        LLVMTypeRef glueType = ConvertLLVMTypeFromVariableType(paramsType[0]);
        paramTys.push_back(glueType);

        for (size_t i = 1; i < paramCount; i++) {
            paramTys.push_back(ConvertLLVMTypeFromVariableType(paramsType[i]));
        }
    }
    auto functype = LLVMFunctionType(returnType, paramTys.data(), paramCount + extraParameterCnt,
        stubDescriptor->IsVariadicArgs());
    return functype;
}

LLVMTypeRef LLVMModule::GenerateFuncType(const std::vector<LLVMValueRef> &params, const CallSignature *stubDescriptor)
{
    LLVMTypeRef returnType = ConvertLLVMTypeFromVariableType(stubDescriptor->GetReturnType());
    std::vector<LLVMTypeRef> paramTys;
    for (auto value : params) {
        paramTys.emplace_back(LLVMTypeOf(value));
    }
    auto functionType = LLVMFunctionType(returnType, paramTys.data(), paramTys.size(), false);
    return functionType;
}

LLVMTypeRef LLVMModule::ConvertLLVMTypeFromVariableType(VariableType type)
{
    std::map<VariableType, LLVMTypeRef> machineTypeMap = {
        {VariableType::VOID(), GetVoidT() },
        {VariableType::BOOL(), GetInt1T() },
        {VariableType::INT8(), GetInt8T() },
        {VariableType::INT16(), GetInt16T() },
        {VariableType::INT32(), GetInt32T() },
        {VariableType::INT64(), GetInt64T() },
        {VariableType::INT8(), GetInt8T() },
        {VariableType::INT16(), GetInt16T() },
        {VariableType::INT32(), GetInt32T() },
        {VariableType::INT64(), GetInt64T() },
        {VariableType::FLOAT32(), GetFloatT() },
        {VariableType::FLOAT64(), GetDoubleT() },
        {VariableType::NATIVE_POINTER(), GetInt64T() },
        {VariableType::JS_POINTER(), GetTaggedHPtrT() },
        {VariableType::JS_ANY(), GetTaggedHPtrT()},
    };
    return machineTypeMap[type];
}

LLVMValueRef LLVMModule::AddFunc(const panda::ecmascript::MethodLiteral *methodLiteral, const JSPandaFile *jsPandaFile)
{
    LLVMTypeRef returnType = NewLType(MachineType::I64, GateType::TaggedValue());  // possibly get it for circuit
    LLVMTypeRef glue = NewLType(MachineType::I64, GateType::NJSValue());
    uint32_t paramCount = 0;
    std::vector<LLVMTypeRef> paramTys = { glue };
    if (!methodLiteral->IsFastCall()) {
        LLVMTypeRef actualArgc = NewLType(MachineType::I64, GateType::NJSValue());
        LLVMTypeRef actualArgv = NewLType(MachineType::I64, GateType::NJSValue());
        paramTys.emplace_back(actualArgc);
        paramTys.emplace_back(actualArgv);
        auto funcIndex = static_cast<uint32_t>(CommonArgIdx::FUNC);
        auto numOfComArgs = static_cast<uint32_t>(CommonArgIdx::NUM_OF_ARGS);
        paramCount = methodLiteral->GetNumArgsWithCallField() + numOfComArgs;
        auto numOfRestArgs = paramCount - funcIndex;
        paramTys.insert(paramTys.end(), numOfRestArgs, NewLType(MachineType::I64, GateType::TaggedValue()));
    } else {
        auto funcIndex = static_cast<uint32_t>(FastCallArgIdx::FUNC);
        auto numOfComArgs = static_cast<uint32_t>(FastCallArgIdx::NUM_OF_ARGS);
        paramCount = methodLiteral->GetNumArgsWithCallField() + numOfComArgs;
        auto numOfRestArgs = paramCount - funcIndex;
        paramTys.insert(paramTys.end(), numOfRestArgs, NewLType(MachineType::I64, GateType::TaggedValue()));
    }
    auto funcType = LLVMFunctionType(returnType, paramTys.data(), paramCount, false); // not variable args

    std::string name = GetFuncName(methodLiteral, jsPandaFile);
    auto offsetInPandaFile = methodLiteral->GetMethodId().GetOffset();
    auto function = LLVMAddFunction(module_, name.c_str(), funcType);
    ASSERT(offsetInPandaFile != LLVMModule::kDeoptEntryOffset);
    SetFunction(offsetInPandaFile, function, methodLiteral->IsFastCall());

    return function;
}

LLVMTypeRef LLVMModule::NewLType(MachineType machineType, GateType gateType)
{
    VariableType vType(machineType, gateType);
    return ConvertLLVMTypeFromVariableType(vType);
}
}  // namespace panda::ecmascript::kungfu
