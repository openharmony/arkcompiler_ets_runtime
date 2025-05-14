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

#include "ecmascript/compiler/codegen/maple/litecg_ir_builder.h"


#include "ecmascript/deoptimizer/deoptimizer.h"

namespace panda::ecmascript::kungfu {
using FunctionBuilder = maple::litecg::LMIRBuilder::FunctionBuilder;
using SwitchBuilder = maple::litecg::LMIRBuilder::SwitchBuilder;
using Function = maple::litecg::Function;
using LMIRBuilder = maple::litecg::LMIRBuilder;
using BB = maple::litecg::BB;
using Expr = maple::litecg::Expr;
using Stmt = maple::litecg::Stmt;
using Const = maple::litecg::Const;
using LiteCGType = maple::litecg::Type;
using IntCmpCondition = maple::litecg::IntCmpCondition;
using FloatCmpCondition = maple::litecg::FloatCmpCondition;
using Var = maple::litecg::Var;
using PregIdx = maple::litecg::PregIdx;
using IntrinsicId = maple::litecg::IntrinsicId;
using maple::litecg::LiteCGValue;
using maple::litecg::LiteCGValueKind;

using StubIdType = std::variant<RuntimeStubCSigns::ID, CommonStubCSigns::ID, Expr>;

LiteCGIRBuilder::LiteCGIRBuilder(const std::vector<std::vector<GateRef>> *schedule, Circuit *circuit,
                                 LMIRModule *module, const CompilationConfig *cfg, CallSignature::CallConv callConv,
                                 bool enableLog, bool enableOptInlining,
                                 const panda::ecmascript::MethodLiteral *methodLiteral, const JSPandaFile *jsPandaFile,
                                 const std::string &funcName)
    : scheduledGates_(schedule),
      circuit_(circuit),
      lmirModule_(module),
      compCfg_(cfg),
      callConv_(callConv),
      enableLog_(enableLog),
      enableOptInlining_(enableOptInlining),
      methodLiteral_(methodLiteral),
      jsPandaFile_(jsPandaFile),
      funcName_(funcName),
      acc_(circuit),
      cf_(*module->GetModule())
{
    lmirBuilder_ = new LMIRBuilder(*module->GetModule());
    maple::theMIRModule = module->GetModule();
    ASSERT(compCfg_->Is64Bit());
    slotSize_ = sizeof(uint64_t);
    slotType_ = lmirBuilder_->i64Type;
    InitializeHandlers();
    if (cfg != nullptr) {
        maple::Triple::GetTriple().Init(cfg->IsAArch64());
    }
}

LiteCGIRBuilder::~LiteCGIRBuilder()
{
    delete lmirBuilder_;
}

void LiteCGIRBuilder::BuildInstID2BBIDMap()
{
    for (size_t bbIdx = 0; bbIdx < scheduledGates_->size(); bbIdx++) {
        const std::vector<GateRef> &bb = scheduledGates_->at(bbIdx);
        for (size_t instIdx = bb.size(); instIdx > 0; instIdx--) {
            if (!constantTableInfo.needConstantTable && acc_.GetOpCode(bb[instIdx - 1]) == OpCode::HEAP_CONSTANT) {
                constantTableInfo.needConstantTable = true;
            }
            GateId gateId = acc_.GetId(bb[instIdx - 1]);
            instID2bbID_[gateId] = static_cast<int>(bbIdx);
        }
    }
}

BB &LiteCGIRBuilder::GetOrCreateBB(int bbID)
{
    auto itr = bbID2BB_.find(bbID);
    if (itr != bbID2BB_.end()) {
        return *(itr->second);
    }
    BB &bb = lmirBuilder_->CreateBB();
    bbID2BB_[bbID] = &bb;
    return bb;
}

BB &LiteCGIRBuilder::GetFirstBB()
{
    // Obtain the first BB (i.e. the BB with id zero) for inserting prologue information
    return GetOrCreateBB(0);
}

BB &LiteCGIRBuilder::CreateBB()
{
    BB &bb = lmirBuilder_->CreateBB(false);
    return bb;
}

LiteCGType *LiteCGIRBuilder::ConvertLiteCGTypeFromGate(GateRef gate, bool isSigned) const
{
    if (acc_.IsGCRelated(gate)) {
        return lmirBuilder_->i64RefType;
    }

    MachineType t = acc_.GetMachineType(gate);
    switch (t) {
        case MachineType::NOVALUE:
            return lmirBuilder_->voidType;
        case MachineType::I1:
            return lmirBuilder_->u1Type;
        case MachineType::I8:
            return isSigned ? lmirBuilder_->i8Type : lmirBuilder_->u8Type;
        case MachineType::I16:
            return isSigned ? lmirBuilder_->i16Type : lmirBuilder_->u16Type;
        case MachineType::I32:
            return isSigned ? lmirBuilder_->i32Type : lmirBuilder_->u32Type;
        case MachineType::I64:
            return isSigned ? lmirBuilder_->i64Type : lmirBuilder_->u64Type;
        case MachineType::F32:
            return lmirBuilder_->f32Type;
        case MachineType::F64:
            return lmirBuilder_->f64Type;
        case MachineType::ARCH:
            return isSigned ? lmirBuilder_->i64Type : lmirBuilder_->u64Type;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

void LiteCGIRBuilder::AddFunc()
{
    // setup function type
    std::string funcName = lmirModule_->GetFuncName(methodLiteral_, jsPandaFile_);
    FunctionBuilder funcBuilder = lmirBuilder_->DefineFunction(funcName);
    funcBuilder.Param(lmirBuilder_->i64Type, "glue");
    if (circuit_->IsOsr()) {
        funcBuilder.Param(lmirBuilder_->i64PtrType, "interpSp");
    } else if (!methodLiteral_->IsFastCall()) {
        funcBuilder.Param(lmirBuilder_->i64Type, "actualArgc")
            .Param(lmirBuilder_->i64PtrType, "actualArgv")
            .Param(lmirBuilder_->i64RefType, "func")
            .Param(lmirBuilder_->i64RefType, "new_target")
            .Param(lmirBuilder_->i64RefType, "this_object");
        for (uint32_t i = 0; i < methodLiteral_->GetNumArgsWithCallField(); ++i) {
            funcBuilder.Param(lmirBuilder_->i64RefType, "param" + std::to_string(i));
        }
    } else {
        funcBuilder.Param(lmirBuilder_->i64RefType, "func").Param(lmirBuilder_->i64RefType, "this_object");
        for (uint32_t i = 0; i < methodLiteral_->GetNumArgsWithCallField(); ++i) {
            funcBuilder.Param(lmirBuilder_->i64RefType, "param" + std::to_string(i));
        }
    }

    funcBuilder.CallConvAttribute(ConvertCallAttr(callConv_));
    Function &function = funcBuilder.Return(lmirBuilder_->i64RefType).Done();
    lmirBuilder_->SetCurFunc(function);
    lmirBuilder_->RenameFormal2Preg(function);
    GenPrologue(function);
    auto offsetInPandaFile = methodLiteral_->GetMethodId().GetOffset();
    lmirModule_->SetFunction(offsetInPandaFile, funcName, methodLiteral_->IsFastCall());
}

// deal with derived reference
void LiteCGIRBuilder::CollectDerivedRefInfo()
{
    auto GetPregFromGate = [&](GateRef gate)->PregIdx {
        LiteCGValue value = gate2Expr_[gate];
        ASSERT(value.kind == LiteCGValueKind::kPregKind);
        return std::get<PregIdx>(value.data);
    };

    // collect base references for derived phi reference
    for (auto &pair : bbID2basePhis_) {
        for (auto &desc : pair.second) {
            Expr expr = GetExprFromGate(desc.operand);
            if (derivedPhiGate2BasePhiPreg_.find(desc.operand) != derivedPhiGate2BasePhiPreg_.end()) {
                expr = lmirBuilder_->Regread(derivedPhiGate2BasePhiPreg_[desc.operand]);
            }
            Stmt &tmpPhiAssign = lmirBuilder_->Regassign(expr, desc.phi);
            lmirBuilder_->AppendStmtBeforeBranch(GetOrCreateBB(desc.predBBId), tmpPhiAssign);
        }
    }

    std::set<PregIdx> baseRefSet;
    // set common derived reference
    for (auto it : derivedGate2BaseGate_) {
        if (GetExprFromGate(it.second).IsConstValue() || GetExprFromGate(it.first).IsConstValue()) {
            continue;
        }
        ASSERT(!GetExprFromGate(it.second).IsDread());
        PregIdx derivedIdx = GetPregFromGate(it.first);
        PregIdx baseIdx = GetPregFromGate(it.second);
        baseRefSet.insert(baseIdx);
        lmirBuilder_->SetFunctionDerived2BaseRef(derivedIdx, baseIdx);
    }

    // set phi derived reference
    for (auto it : derivedPhiGate2BasePhiPreg_) {
        PregIdx derivedIdx = GetPregFromGate(it.first);
        PregIdx baseIdx = it.second;
        if (baseRefSet.find(derivedIdx) != baseRefSet.end()) {
            LOG_COMPILER(FATAL) << "shouldn't occur nested derived reference" << std::endl;
            UNREACHABLE();
        }
        lmirBuilder_->SetFunctionDerived2BaseRef(derivedIdx, baseIdx);
    }

    bbID2basePhis_.clear();
    derivedPhiGate2BasePhiPreg_.clear();
    derivedGate2BaseGate_.clear();
    derivedGateCache_.clear();
}

void LiteCGIRBuilder::HandleBB(const std::vector<GateRef> &bb, std::unordered_set<OpCode> &usedOpcodeSet)
{
    for (size_t instIdx = bb.size(); instIdx > 0; instIdx--) {
        GateRef gate = bb[instIdx - 1];
        OpCode opcode = acc_.GetOpCode(gate);
        if (IsLogEnabled()) {
            lmirBuilder_->SetCurrentDebugComment(acc_.ToString(gate));
        }
        switch (opcode) {
            case OpCode::STATE_ENTRY:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::STATE_ENTRY);
                break;
            case OpCode::RETURN:
                HandleReturn(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::RETURN);
                break;
            case OpCode::RETURN_VOID:
                HandleReturnVoid(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::RETURN_VOID);
                break;
            case OpCode::IF_BRANCH:
                HandleBranch(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::IF_BRANCH);
                break;
            case OpCode::ORDINARY_BLOCK:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ORDINARY_BLOCK);
                break;
            case OpCode::IF_TRUE:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::IF_TRUE);
                break;
            case OpCode::IF_FALSE:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::IF_FALSE);
                break;
            case OpCode::SWITCH_BRANCH:
                HandleSwitch(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SWITCH_BRANCH);
                break;
            case OpCode::SWITCH_CASE:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SWITCH_CASE);
                break;
            case OpCode::MERGE:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::MERGE);
                break;
            case OpCode::DEFAULT_CASE:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::DEFAULT_CASE);
                break;
            case OpCode::LOOP_BEGIN:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::LOOP_BACK);
                break;
            case OpCode::LOOP_BACK:
                HandleGoto(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::LOOP_BEGIN);
                break;
            case OpCode::VALUE_SELECTOR:
                HandlePhi(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::VALUE_SELECTOR);
                break;
            case OpCode::RUNTIME_CALL:
                HandleRuntimeCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::RUNTIME_CALL);
                break;
            case OpCode::RUNTIME_CALL_WITH_ARGV:
                HandleRuntimeCallWithArgv(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::RUNTIME_CALL_WITH_ARGV);
                break;
            case OpCode::NOGC_RUNTIME_CALL:
                HandleCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::NOGC_RUNTIME_CALL);
                break;
            case OpCode::CALL_OPTIMIZED:
                HandleCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::CALL_OPTIMIZED);
                break;
            case OpCode::FAST_CALL_OPTIMIZED:
                HandleCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FAST_CALL_OPTIMIZED);
                break;
            case OpCode::CALL:
                HandleCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::CALL);
                break;
            case OpCode::BASELINE_CALL:
                HandleCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::BASELINE_CALL);
                break;
            case OpCode::BUILTINS_CALL:
                HandleCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::BUILTINS_CALL);
                break;
            case OpCode::BUILTINS_CALL_WITH_ARGV:
                HandleCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::BUILTINS_CALL_WITH_ARGV);
                break;
            case OpCode::ARG:
                HandleParameter(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ARG);
                break;
            case OpCode::CONSTANT:
                HandleConstant(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::CONSTANT);
                break;
            case OpCode::HEAP_CONSTANT:
                HandleHeapConstant(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::HEAP_CONSTANT);
                break;
            case OpCode::ZEXT:
                HandleZExtInt(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ZEXT);
                break;
            case OpCode::SEXT:
                HandleSExtInt(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SEXT);
                break;
            case OpCode::TRUNC:
                HandleCastIntXToIntY(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::TRUNC);
                break;
            case OpCode::FEXT:
                HandleFPExt(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FEXT);
                break;
            case OpCode::FTRUNC:
                HandleFPTrunc(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FTRUNC);
                break;
            case OpCode::REV:
                HandleIntRev(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::REV);
                break;
            case OpCode::ADD:
                HandleAdd(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ADD);
                break;
            case OpCode::SUB:
                HandleSub(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SUB);
                break;
            case OpCode::MUL:
                HandleMul(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::MUL);
                break;
            case OpCode::FDIV:
                HandleFloatDiv(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FDIV);
                break;
            case OpCode::SDIV:
                HandleIntDiv(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SDIV);
                break;
            case OpCode::UDIV:
                HandleUDiv(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::UDIV);
                break;
            case OpCode::AND:
                HandleIntAnd(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::AND);
                break;
            case OpCode::OR:
                HandleIntOr(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::OR);
                break;
            case OpCode::XOR:
                HandleIntXor(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::XOR);
                break;
            case OpCode::LSR:
                HandleIntLsr(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::LSR);
                break;
            case OpCode::ASR:
                HandleIntAsr(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ASR);
                break;
            case OpCode::ICMP:
                HandleCmp(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ICMP);
                break;
            case OpCode::FCMP:
                HandleCmp(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FCMP);
                break;
            case OpCode::LOAD_WITHOUT_BARRIER:
                HandleLoad(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::LOAD_WITHOUT_BARRIER);
                break;
            case OpCode::STORE_WITHOUT_BARRIER:
                HandleStore(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::STORE_WITHOUT_BARRIER);
                break;
            case OpCode::SIGNED_INT_TO_FLOAT:
                HandleChangeInt32ToDouble(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SIGNED_INT_TO_FLOAT);
                break;
            case OpCode::UNSIGNED_INT_TO_FLOAT:
                HandleChangeUInt32ToDouble(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::UNSIGNED_INT_TO_FLOAT);
                break;
            case OpCode::FLOAT_TO_SIGNED_INT:
                HandleChangeDoubleToInt32(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FLOAT_TO_SIGNED_INT);
                break;
            case OpCode::TAGGED_TO_INT64:
                HandleChangeTaggedPointerToInt64(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::TAGGED_TO_INT64);
                break;
            case OpCode::INT64_TO_TAGGED:
                HandleChangeInt64ToTagged(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::INT64_TO_TAGGED);
                break;
            case OpCode::BITCAST:
                HandleBitCast(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::BITCAST);
                break;
            case OpCode::LSL:
                HandleIntLsl(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::LSL);
                break;
            case OpCode::SMOD:
                HandleMod(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SMOD);
                break;
            case OpCode::FMOD:
                HandleMod(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FMOD);
                break;
            case OpCode::DEOPT_CHECK:
                HandleDeoptCheck(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::DEOPT_CHECK);
                break;
            case OpCode::TRUNC_FLOAT_TO_INT64:
                HandleTruncFloatToInt(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::TRUNC_FLOAT_TO_INT64);
                break;
            case OpCode::TRUNC_FLOAT_TO_INT32:
                HandleTruncFloatToInt(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::TRUNC_FLOAT_TO_INT32);
                break;
            case OpCode::ADD_WITH_OVERFLOW:
                HandleAddWithOverflow(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ADD_WITH_OVERFLOW);
                break;
            case OpCode::SUB_WITH_OVERFLOW:
                HandleSubWithOverflow(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SUB_WITH_OVERFLOW);
                break;
            case OpCode::MUL_WITH_OVERFLOW:
                HandleMulWithOverflow(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::MUL_WITH_OVERFLOW);
                break;
            case OpCode::EXP:
                HandleExp(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::EXP);
                break;
            case OpCode::ABS:
                HandleAbs(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ABS);
                break;
            case OpCode::MIN:
                HandleMin(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::MIN);
                break;
            case OpCode::MAX:
                HandleMax(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::MAX);
                break;
            case OpCode::CLZ32:
                HandleClz32(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::CLZ32);
                break;
            case OpCode::DOUBLE_TRUNC:
                HandleDoubleTrunc(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::DOUBLE_TRUNC);
                break;
            case OpCode::CEIL:
                HandleCeil(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::CEIL);
                break;
            case OpCode::FLOOR:
                HandleFloor(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FLOOR);
                break;
            case OpCode::EXTRACT_VALUE:
                HandleExtractValue(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::EXTRACT_VALUE);
                break;
            case OpCode::SQRT:
                HandleSqrt(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::SQRT);
                break;
            case OpCode::READSP:
                HandleReadSp(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::READSP);
                break;
            case OpCode::FINISH_ALLOCATE:
                HandleFinishAllocate(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::FINISH_ALLOCATE);
                break;
            case OpCode::INITVREG:
                HandleInitVreg(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::INITVREG);
                break;
            case OpCode::ASM_CALL_BARRIER:
                HandleCall(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, OpCode::ASM_CALL_BARRIER);
                break;
            default:
                if (illegalOpHandlers_.find(acc_.GetOpCode(gate)) == illegalOpHandlers_.end()) {
                    LOG_COMPILER(FATAL) << "can't process opcode: " << acc_.GetOpCode(gate) << std::endl;
                }
        }
        if (IsLogEnabled()) {
            lmirBuilder_->ClearCurrentDebugComment();
        }
    }
}
void LiteCGIRBuilder::Build()
{
    BuildInstID2BBIDMap();
    AddFunc();
    LOG_COMPILER(INFO) << "============== building litecg ir=======" << std::endl;

    std::unordered_set<OpCode> usedOpcodeSet;
    for (size_t bbIdx = 0; bbIdx < scheduledGates_->size(); bbIdx++) {
        const std::vector<GateRef> &bb = scheduledGates_->at(bbIdx);
        HandleBB(bb, usedOpcodeSet);
    }

    if (enableLog_) {
        for (auto &opcode : usedOpcodeSet) {
            LOG_COMPILER(INFO) << "OPCODE: " << opcode << std::endl;
        }
    }

    CollectDerivedRefInfo();

    std::map<int, std::vector<std::pair<PregIdx, PregIdx>>> bbID2phiAssign;
    for (auto &pair : bbID2unmergedPhis_) {
        for (auto &desc : pair.second) {
            Expr value = GetExprFromGate(desc.operand);
            PregIdx tmpPhiPregIdx = lmirBuilder_->CreatePreg(value.GetType());
            Stmt &tmpPhiAssign = lmirBuilder_->Regassign(value, tmpPhiPregIdx);
            lmirBuilder_->AppendStmtBeforeBranch(GetOrCreateBB(desc.predBBId), tmpPhiAssign);
            bbID2phiAssign[desc.predBBId].emplace_back(std::make_pair(tmpPhiPregIdx, desc.phi));
        }
    }

    for (auto &pair: bbID2phiAssign) {
        for (auto &expr: pair.second) {
            auto &stmt =  lmirBuilder_->Regassign(lmirBuilder_->Regread(expr.first), expr.second);
            lmirBuilder_->AppendStmtBeforeBranch(GetOrCreateBB(pair.first), stmt);
        }
    }
    bbID2unmergedPhis_.clear();
    bbID2phiAssign.clear();

    lmirBuilder_->AppendBB(lmirBuilder_->GetLastPosBB());
}

void LiteCGIRBuilder::AssistGenPrologue(const size_t reservedSlotsSize, FrameType frameType,
                                        maple::litecg::Function &function)
{
    lmirBuilder_->SetFuncFrameResverdSlot(reservedSlotsSize);
    if (circuit_->IsOsr()) {
        auto osrFrameType = methodLiteral_->IsFastCall() ? FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME : frameType;
        SaveFrameTypeOnFrame(function, osrFrameType);
        return;
    }
    auto ArgList = circuit_->GetArgRoot();
    auto uses = acc_.Uses(ArgList);
    for (auto useIt = uses.begin(); useIt != uses.end(); ++useIt) {
        int argth = static_cast<int>(acc_.TryGetValue(*useIt));
        Var &value = lmirBuilder_->GetParam(function, argth);
        int funcIndex = 0;
        if (methodLiteral_->IsFastCall()) {
            frameType = FrameType::FASTJIT_FAST_CALL_FUNCTION_FRAME;
            funcIndex = static_cast<int>(FastCallArgIdx::FUNC);
        } else {
            funcIndex = static_cast<int>(CommonArgIdx::FUNC);
        }
        if (argth == funcIndex) {
            SaveByteCodePcOnOptJSFuncFrame(value);
            SaveJSFuncOnOptJSFuncFrame(function, value, funcIndex);
            SaveFrameTypeOnFrame(function, frameType);
            LoadConstantTableIfNeeded(value);
        }
    }
}

void LiteCGIRBuilder::GenPrologue(maple::litecg::Function &function)
{
    auto frameType = circuit_->GetFrameType();
    if (IsInterpreted() || IsBaselineBuiltin()) {
        return;
    }
    lmirBuilder_->SetFuncFramePointer("all");
    size_t reservedSlotsSize = 0;
    if (frameType == FrameType::OPTIMIZED_FRAME) {
        reservedSlotsSize = OptimizedFrame::ComputeReservedSize(slotSize_);
        lmirBuilder_->SetFuncFrameResverdSlot(reservedSlotsSize);
        SaveFrameTypeOnFrame(function, frameType);
    } else if (frameType == FrameType::OPTIMIZED_JS_FUNCTION_FRAME) {
        reservedSlotsSize = OptimizedJSFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
        lmirBuilder_->SetFuncFrameResverdSlot(reservedSlotsSize);
        if (circuit_->IsOsr()) {
            auto osrFrameType = methodLiteral_->IsFastCall() ? FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME
                                                             : frameType;
            SaveFrameTypeOnFrame(function, osrFrameType);
            return;
        }
        auto ArgList = circuit_->GetArgRoot();
        auto uses = acc_.Uses(ArgList);
        for (auto useIt = uses.begin(); useIt != uses.end(); ++useIt) {
            int argth = static_cast<int>(acc_.TryGetValue(*useIt));
            Var &value = lmirBuilder_->GetParam(function, argth);
            int funcIndex = 0;
            if (methodLiteral_->IsFastCall()) {
                frameType = FrameType::OPTIMIZED_JS_FAST_CALL_FUNCTION_FRAME;
                funcIndex = static_cast<int>(FastCallArgIdx::FUNC);
            } else {
                funcIndex = static_cast<int>(CommonArgIdx::FUNC);
            }
            if (argth == funcIndex) {
                SaveJSFuncOnOptJSFuncFrame(function, value, funcIndex);
                SaveFrameTypeOnFrame(function, frameType);
                LoadConstantTableIfNeeded(value);
            }
        }
    } else if (frameType == FrameType::FASTJIT_FUNCTION_FRAME) {
        reservedSlotsSize = FASTJITFunctionFrame::ComputeReservedPcOffset(slotSize_);
        AssistGenPrologue(reservedSlotsSize, frameType, function);
    } else {
        LOG_COMPILER(FATAL) << "frameType interpret type error !";
        ASSERT_PRINT(static_cast<uintptr_t>(frameType), "is not support !");
    }
}

void LiteCGIRBuilder::SaveByteCodePcOnOptJSFuncFrame(maple::litecg::Var &value)
{
    ASSERT(circuit_->GetFrameType() == FrameType::FASTJIT_FUNCTION_FRAME);
    // load method
    Expr func = lmirBuilder_->Cvt(lmirBuilder_->i64PtrType, slotType_, lmirBuilder_->GenExprFromVar(value));
    Expr offsetMethod = lmirBuilder_->ConstVal(
        lmirBuilder_->CreateIntConst(lmirBuilder_->i64PtrType, JSFunctionBase::METHOD_OFFSET));
    Expr addrMethod = lmirBuilder_->Add(lmirBuilder_->i64PtrType, func, offsetMethod);
    Expr method = lmirBuilder_->Iread(
        lmirBuilder_->i64PtrType, addrMethod, lmirBuilder_->CreatePtrType(lmirBuilder_->i64PtrType));
    // load byteCodePc
    Expr offsetByteCodePc = lmirBuilder_->ConstVal(
        lmirBuilder_->CreateIntConst(lmirBuilder_->i64PtrType, Method::NATIVE_POINTER_OR_BYTECODE_ARRAY_OFFSET));
    Expr addrByteCodePc = lmirBuilder_->Add(lmirBuilder_->i64PtrType, method, offsetByteCodePc);
    Expr byteCodePc = lmirBuilder_->Iread(
        lmirBuilder_->i64PtrType, addrByteCodePc, lmirBuilder_->CreatePtrType(lmirBuilder_->i64PtrType));
    // push byteCodePc
    Expr fpAddr = CallingFp(false);
    Expr frameAddr = lmirBuilder_->Cvt(fpAddr.GetType(), lmirBuilder_->i64Type, fpAddr);
    size_t reservedOffset = FASTJITFunctionFrame::ComputeReservedPcOffset(slotSize_);
    Expr frameByteCodePcSlotAddr =
        lmirBuilder_->Sub(frameAddr.GetType(), frameAddr,
                          lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(slotType_, reservedOffset)));
    Expr byteCodePcAddr =
        lmirBuilder_->Cvt(frameByteCodePcSlotAddr.GetType(),
                          lmirBuilder_->CreatePtrType(slotType_), frameByteCodePcSlotAddr);
    auto &stmt = lmirBuilder_->Iassign(byteCodePc, byteCodePcAddr, byteCodePcAddr.GetType());
    lmirBuilder_->AppendStmt(GetFirstBB(), stmt);
}

void LiteCGIRBuilder::LoadConstantTableIfNeeded(maple::litecg::Var &value)
{
    if (!constantTableInfo.needConstantTable) {
        return;
    }
    auto machineCodeOffset = lmirBuilder_->ConstVal(
        lmirBuilder_->CreateIntConst(lmirBuilder_->i64PtrType, JSFunction::MACHINECODE_OFFSET));
    auto constantTableOffset = lmirBuilder_->ConstVal(
        lmirBuilder_->CreateIntConst(lmirBuilder_->i64PtrType, MachineCode::HEAP_CONSTANT_TABLE_ADDR_OFFSET));
    std::vector<Expr> args = { lmirBuilder_->GenExprFromVar(value), machineCodeOffset, constantTableOffset };
    auto intrinsicOp = lmirBuilder_->IntrinsicOp(
        IntrinsicId::INTRN_GET_HEAP_CONSTANT_TABLE, lmirBuilder_->i64PtrType, args);
    PregIdx pregIdx = lmirBuilder_->CreatePreg(lmirBuilder_->i64PtrType);
    lmirBuilder_->AppendStmt(GetFirstBB(), lmirBuilder_->Regassign(intrinsicOp, pregIdx));
    constantTableInfo.constTable = pregIdx;
}

void LiteCGIRBuilder::SaveJSFuncOnOptJSFuncFrame(maple::litecg::Function &function, maple::litecg::Var &value,
                                                 int funcIndex)
{
    ASSERT(IsOptimizedJSFunction());
    if (compCfg_->IsAArch64()) {
        auto frameType = circuit_->GetFrameType();
        if (frameType == FrameType::OPTIMIZED_JS_FUNCTION_FRAME) {
            function.SetFuncInfo(OptimizedJSFunctionFrame::GetFunctionToFpDelta(), true, funcIndex);
        } else if (frameType == FrameType::FASTJIT_FUNCTION_FRAME) {
            function.SetFuncInfo(FASTJITFunctionFrame::GetFunctionToFpDelta(), true, funcIndex);
        }
    } else {
        Expr fpAddr = CallingFp(false);
        Expr frameAddr = lmirBuilder_->Cvt(fpAddr.GetType(), lmirBuilder_->i64Type, fpAddr);
        size_t reservedOffset = 0;
        if (circuit_->GetFrameType() == FrameType::OPTIMIZED_JS_FUNCTION_FRAME) {
            reservedOffset = OptimizedJSFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
        } else {
            reservedOffset = FASTJITFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
        }

        Expr frameJSFuncSlotAddr =
            lmirBuilder_->Sub(frameAddr.GetType(), frameAddr,
                lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(slotType_, reservedOffset)));
        Expr jsFuncAddr =
            lmirBuilder_->Cvt(frameJSFuncSlotAddr.GetType(),
                lmirBuilder_->CreatePtrType(slotType_), frameJSFuncSlotAddr);
        Expr jsFuncValue = lmirBuilder_->Cvt(lmirBuilder_->i64PtrType, slotType_, lmirBuilder_->GenExprFromVar(value));
        auto &stmt = lmirBuilder_->Iassign(jsFuncValue, jsFuncAddr, jsFuncAddr.GetType());
        lmirBuilder_->AppendStmt(GetFirstBB(), stmt);
    }
}

void LiteCGIRBuilder::SaveFrameTypeOnFrame(maple::litecg::Function &function, FrameType frameType)
{
    if (compCfg_->IsAArch64()) {
        function.SetFrameTypeInfo(-slotSize_, true, static_cast<int>(frameType));
        return;
    }
    Expr fpAddr = CallingFp(false);
    Expr frameAddr = lmirBuilder_->Cvt(fpAddr.GetType(), lmirBuilder_->i64Type, fpAddr);
    Expr frameJSFuncSlotAddr = lmirBuilder_->Sub(
        frameAddr.GetType(), frameAddr, lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(slotType_, slotSize_)));
    Expr jsFuncAddr =
        lmirBuilder_->Cvt(frameJSFuncSlotAddr.GetType(), lmirBuilder_->CreatePtrType(slotType_), frameJSFuncSlotAddr);
    Expr liteFramType =
        lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->i64Type, static_cast<uintptr_t>(frameType)));
    auto &stmt = lmirBuilder_->Iassign(liteFramType, jsFuncAddr, jsFuncAddr.GetType());
    lmirBuilder_->AppendStmt(GetFirstBB(), stmt);
}

Expr LiteCGIRBuilder::GetGlue(const std::vector<GateRef> &inList)
{
    GateRef glueGate = inList[static_cast<size_t>(CallInputs::GLUE)];
    auto itr = gate2Expr_.find(glueGate);
    if (itr != gate2Expr_.end()) {
        return GetExprFromGate(glueGate);
    }
    Expr glue = lmirBuilder_->Dread(lmirBuilder_->GetLocalVar("glue"));
    SaveGate2Expr(glueGate, glue);
    return glue;
}

LiteCGValue LiteCGIRBuilder::ConstantFoldExpr(Expr expr, BB &curBB)
{
    if (expr.IsRegread()) {
        return {LiteCGValueKind::kPregKind, lmirBuilder_->GetPregIdxFromExpr(expr)};
    }
    auto *newNode = cf_.Fold(expr.GetNode());
    if (newNode == nullptr || !newNode->IsConstval()) {
        PregIdx pregIdx = lmirBuilder_->CreatePreg(expr.GetType());
        lmirBuilder_->AppendStmt(curBB, lmirBuilder_->Regassign(expr, pregIdx));
        return {LiteCGValueKind::kPregKind, pregIdx};
    }
    return {LiteCGValueKind::kConstKind, static_cast<maple::ConstvalNode*>(newNode)->GetConstVal()};
}

void LiteCGIRBuilder::SaveGate2Expr(GateRef gate, Expr expr, bool isGlueAdd)
{
    if (isGlueAdd) {
        gate2Expr_[gate] = {LiteCGValueKind::kGlueAdd, lmirBuilder_->GetConstFromExpr(expr)};
        return;
    } else if (expr.IsDread()) {
        gate2Expr_[gate] = {LiteCGValueKind::kSymbolKind, lmirBuilder_->GetLocalVarFromExpr(expr)};
        return;
    } else if (expr.IsRegread()) {
        gate2Expr_[gate] = {LiteCGValueKind::kPregKind, lmirBuilder_->GetPregIdxFromExpr(expr)};
        return;
    } else if (expr.IsConstValue()) {
        gate2Expr_[gate] = {LiteCGValueKind::kConstKind, lmirBuilder_->GetConstFromExpr(expr)};
        return;
    }
    gate2Expr_[gate] = ConstantFoldExpr(expr, GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]));
}

void LiteCGIRBuilder::SaveGate2Expr(GateRef gate, PregIdx pregIdx1, PregIdx pregIdx2)
{
    gate2Expr_[gate] = {LiteCGValueKind::kPregPairKind, std::make_pair(pregIdx1, pregIdx2)};
}

Expr LiteCGIRBuilder::GetConstant(GateRef gate)
{
    std::bitset<64> value = acc_.GetConstantValue(gate); // 64 for bit width
    auto machineType = acc_.GetMachineType(gate);
    if (machineType == MachineType::ARCH) {
        ASSERT(compCfg_->Is64Bit());
        machineType = MachineType::I64;
    }

    Const *constVal = nullptr;
    if (machineType == MachineType::I32) {
        constVal = &(lmirBuilder_->CreateIntConst(lmirBuilder_->i32Type, static_cast<int64_t>(value.to_ulong())));
    } else if (machineType == MachineType::I64) {
        constVal = &(lmirBuilder_->CreateIntConst(lmirBuilder_->i64Type, static_cast<int64_t>(value.to_ulong())));
        LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
        if (lmirBuilder_->LiteCGGetTypeKind(type) == maple::litecg::kLiteCGTypePointer) {
            Expr constExpr = lmirBuilder_->Cvt(lmirBuilder_->i64Type, type, lmirBuilder_->ConstVal(*constVal));
            return constExpr;
        } else if (lmirBuilder_->LiteCGGetTypeKind(type) == maple::litecg::kLiteCGTypeScalar) {
            // do nothing
        } else {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    } else if (machineType == MachineType::F64) {
        auto doubleValue = base::bit_cast<double>(value.to_ullong());  // actual double value
        constVal = &(lmirBuilder_->CreateDoubleConst(static_cast<double>(doubleValue)));
    } else if (machineType == MachineType::I8) {
        constVal = &(lmirBuilder_->CreateIntConst(lmirBuilder_->u8Type, static_cast<int64_t>(value.to_ulong())));
    } else if (machineType == MachineType::I16) {
        constVal = &(lmirBuilder_->CreateIntConst(lmirBuilder_->u16Type, static_cast<int64_t>(value.to_ulong())));
    } else if (machineType == MachineType::I1) {
        constVal = &(lmirBuilder_->CreateIntConst(lmirBuilder_->u1Type, static_cast<int64_t>(value.to_ulong())));
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    return lmirBuilder_->ConstVal(*constVal);
}

Expr LiteCGIRBuilder::CreateExprFromLiteCGValue(const LiteCGValue &value)
{
    if (value.kind == LiteCGValueKind::kSymbolKind) {
        return lmirBuilder_->Dread(*std::get<maple::MIRSymbol*>(value.data));
    } else if (value.kind == LiteCGValueKind::kConstKind) {
        return lmirBuilder_->ConstVal(*std::get<maple::MIRConst*>(value.data));
    } else if (value.kind == LiteCGValueKind::kGlueAdd) {
        auto glue = acc_.GetGlueFromArgList();
        return lmirBuilder_->Add(ConvertLiteCGTypeFromGate(glue), GetExprFromGate(glue),
            lmirBuilder_->ConstVal(*std::get<maple::MIRConst*>(value.data)));
    }
    ASSERT(value.kind == LiteCGValueKind::kPregKind);
    return lmirBuilder_->Regread(std::get<PregIdx>(value.data));
}

Expr LiteCGIRBuilder::GetExprFromGate(GateRef gate)
{
    if (acc_.GetOpCode(gate) == OpCode::CONSTANT) {
        return GetConstant(gate);
    }
    return CreateExprFromLiteCGValue(gate2Expr_[gate]);
}

Expr LiteCGIRBuilder::GetExprFromGate(GateRef gate, uint32_t index)
{
    LiteCGValue value = gate2Expr_[gate];
    ASSERT(value.kind == LiteCGValueKind::kPregPairKind);
    ASSERT(index == 0 || index == 1);
    std::pair<PregIdx, PregIdx> pair = std::get<std::pair<PregIdx, PregIdx>>(value.data);
    if (index == 0) {
        return lmirBuilder_->Regread(pair.first);
    }
    return lmirBuilder_->Regread(pair.second);
}

void LiteCGIRBuilder::InitializeHandlers()
{
    illegalOpHandlers_ = {OpCode::NOP,
                          OpCode::CIRCUIT_ROOT,
                          OpCode::DEPEND_ENTRY,
                          OpCode::DEAD,
                          OpCode::RETURN_LIST,
                          OpCode::ARG_LIST,
                          OpCode::THROW,
                          OpCode::DEPEND_SELECTOR,
                          OpCode::DEPEND_RELAY,
                          OpCode::FRAME_STATE,
                          OpCode::STATE_SPLIT,
                          OpCode::FRAME_ARGS,
                          OpCode::LOOP_EXIT_DEPEND,
                          OpCode::LOOP_EXIT,
                          OpCode::START_ALLOCATE,
                          OpCode::FINISH_ALLOCATE,
                          OpCode::FRAME_VALUES};
}

void LiteCGIRBuilder::HandleReturnVoid([[maybe_unused]] GateRef gate)
{
    return;
}

void LiteCGIRBuilder::HandleGoto(GateRef gate)
{
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    int block = instID2bbID_[acc_.GetId(gate)];
    int bbOut = instID2bbID_[acc_.GetId(outs[0])];
    switch (acc_.GetOpCode(gate)) {
        case OpCode::MERGE:
        case OpCode::LOOP_BEGIN: {
            for (const auto &out : outs) {
                bbOut = instID2bbID_[acc_.GetId(out)];
                VisitGoto(block, bbOut);
            }
            break;
        }
        default: {
            VisitGoto(block, bbOut);
            break;
        }
    }
}

void LiteCGIRBuilder::VisitGoto(int block, int bbOut)
{
    if (block == bbOut) {
        return;
    }
    BB &srcBB = GetOrCreateBB(block);
    BB &destBB = GetOrCreateBB(bbOut);

    lmirBuilder_->AppendStmt(srcBB, lmirBuilder_->Goto(destBB));
    lmirBuilder_->AppendBB(srcBB);
}

void LiteCGIRBuilder::HandleParameter(GateRef gate)
{
    return VisitParameter(gate);
}

void LiteCGIRBuilder::VisitParameter(GateRef gate)
{
    std::vector<GateRef> outs;
    acc_.GetOuts(gate, outs);
    if (outs.empty()) {
        return;
    }
    size_t argth = static_cast<size_t>(acc_.TryGetValue(gate));
    Var &param = lmirBuilder_->GetParam(lmirBuilder_->GetCurFunction(), argth);
    SaveGate2Expr(gate, lmirBuilder_->GenExprFromVar(param));
}

void LiteCGIRBuilder::HandleConstant(GateRef gate)
{
    // no need to deal with constant separately
    (void)gate;
    return;
}

void LiteCGIRBuilder::HandleHeapConstant(GateRef gate)
{
    std::bitset<64> value = acc_.GetConstantValue(gate); // 64: for bit width
    Const &constVal = lmirBuilder_->CreateIntConst(lmirBuilder_->u64Type, static_cast<uint64_t>(value.to_ulong()));
    ASSERT(constantTableInfo.needConstantTable);
    std::vector<Expr> args = { lmirBuilder_->Regread(constantTableInfo.constTable), lmirBuilder_->ConstVal(constVal) };
    auto intrinsicOp = lmirBuilder_->IntrinsicOp(IntrinsicId::INTRN_HEAP_CONSTANT, lmirBuilder_->i64RefType, args);
    SaveGate2Expr(gate, intrinsicOp);
}

void LiteCGIRBuilder::HandleAdd(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitAdd(gate, g0, g1);
}

Expr LiteCGIRBuilder::CanonicalizeToPtr(Expr expr, LiteCGType *type)
{
    if (lmirBuilder_->LiteCGGetTypeKind(expr.GetType()) == maple::litecg::kLiteCGTypePointer) {
        if (expr.GetType() == type) {
            return expr;
        }
        return lmirBuilder_->Cvt(expr.GetType(), type, expr);
    } else if (lmirBuilder_->LiteCGGetTypeKind(expr.GetType()) == maple::litecg::kLiteCGTypeScalar) {
        return lmirBuilder_->Cvt(lmirBuilder_->i64Type, type, expr);
    } else {
        LOG_COMPILER(FATAL) << "can't Canonicalize to Ptr: ";
        UNREACHABLE();
    }
    return expr;
}

void LiteCGIRBuilder::AddDerivedrefGate(GateRef gate, Expr result)
{
    auto e1 = acc_.GetIn(gate, 0);
    derivedrefGate.insert(std::pair<GateRef, Expr>(gate, result));
    bool shouldSkip = true;
    std::vector<GateRef> outGates;
    acc_.GetOuts(gate, outGates);
    for (GateRef outGate : outGates) {
        auto op = acc_.GetOpCode(outGate);
        if (op != OpCode::LOAD_WITHOUT_BARRIER && op != OpCode::STORE_WITHOUT_BARRIER) {
            shouldSkip = false;
            break;
        }
    }
    if (!shouldSkip) {
        SaveGate2Expr(gate, result);
        ASSERT(!GetExprFromGate(e1).IsDread());
        auto e1BaseIter = derivedGate2BaseGate_.find(e1);
        derivedGate2BaseGate_[gate] = (e1BaseIter == derivedGate2BaseGate_.end() ? e1 : e1BaseIter->second);
    }
}

Expr LiteCGIRBuilder::GetDerivedrefExpr(GateRef gate)
{
    auto derivedref = derivedrefGate.find(gate);
    return (derivedref == derivedrefGate.end()) ? GetExprFromGate(gate) : derivedref->second;
}

void LiteCGIRBuilder::VisitAdd(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    // save glue + offset
    if (e1 == acc_.GetGlueFromArgList() && acc_.GetOpCode(e2) == OpCode::CONSTANT) {
        SaveGate2Expr(gate, e2Value, true);
        return;
    }

    Expr result;
    /*
     *  If the first operand is pointer, special treatment is needed
     *  1) add, pointer, int
     *  2) add, vector{i8* x 2}, int
     */
    LiteCGType *returnType = ConvertLiteCGTypeFromGate(gate);
    auto machineType = acc_.GetMachineType(gate);
    if (IsAddIntergerType(machineType)) {
        auto e1Type = ConvertLiteCGTypeFromGate(e1);
        auto e1TypeKind = lmirBuilder_->LiteCGGetTypeKind(e1Type);
        auto e2Type = ConvertLiteCGTypeFromGate(e2);
        if (e1TypeKind == maple::litecg::kLiteCGTypePointer) {
            Expr tmp1 = lmirBuilder_->Cvt(e1Type, lmirBuilder_->i64Type, e1Value);
            Expr tmp2 =
                (e2Type == lmirBuilder_->i64Type) ? e2Value : lmirBuilder_->Cvt(e2Type, lmirBuilder_->i64Type, e2Value);
            Expr tmp3 = lmirBuilder_->Add(lmirBuilder_->i64Type, tmp1, tmp2);
            result = lmirBuilder_->Cvt(lmirBuilder_->i64Type, returnType, tmp3);
            if (e1Type == lmirBuilder_->i64RefType) {
                AddDerivedrefGate(gate, result);
            } else {
                SaveGate2Expr(gate, result);
            }
            return;
        } else {
            Expr tmp1Expr = (e1Type == returnType) ? e1Value : lmirBuilder_->Cvt(e1Type, returnType, e1Value);
            Expr tmp2Expr = (e2Type == returnType) ? e2Value : lmirBuilder_->Cvt(e2Type, returnType, e2Value);
            result = lmirBuilder_->Add(returnType, tmp1Expr, tmp2Expr);
        }
    } else if (machineType == MachineType::F64) {
        result = lmirBuilder_->Add(lmirBuilder_->f64Type, e1Value, e2Value);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleLoad(GateRef gate)
{
    VisitLoad(gate, acc_.GetIn(gate, 1));
}

void LiteCGIRBuilder::VisitLoad(GateRef gate, GateRef base)
{
    Expr baseAddr = GetDerivedrefExpr(base);

    LiteCGType *returnType = ConvertLiteCGTypeFromGate(gate);
    LiteCGType *memType = (lmirBuilder_->IsHeapPointerType(returnType)) ? lmirBuilder_->CreateRefType(returnType)
                                                                        : lmirBuilder_->CreatePtrType(returnType);
    baseAddr = CanonicalizeToPtr(baseAddr, memType);
    Expr result = lmirBuilder_->Iread(returnType, baseAddr, memType);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleCmp(GateRef gate)
{
    GateRef left = acc_.GetIn(gate, 0);
    GateRef right = acc_.GetIn(gate, 1);
    VisitCmp(gate, left, right);
}

IntCmpCondition LiteCGIRBuilder::ConvertLiteCGPredicateFromICMP(ICmpCondition cond) const
{
    switch (cond) {
        case ICmpCondition::SLT:
            return IntCmpCondition::kSLT;
        case ICmpCondition::SLE:
            return IntCmpCondition::kSLE;
        case ICmpCondition::SGT:
            return IntCmpCondition::kSGT;
        case ICmpCondition::SGE:
            return IntCmpCondition::kSGE;
        case ICmpCondition::ULT:
            return IntCmpCondition::kULT;
        case ICmpCondition::ULE:
            return IntCmpCondition::kULE;
        case ICmpCondition::UGT:
            return IntCmpCondition::kUGT;
        case ICmpCondition::UGE:
            return IntCmpCondition::kUGE;
        case ICmpCondition::NE:
            return IntCmpCondition::kNE;
        case ICmpCondition::EQ:
            return IntCmpCondition::kEQ;
        default:
            LOG_COMPILER(FATAL) << "unexpected cond!";
            UNREACHABLE();
    }
    return IntCmpCondition::kEQ;
}

FloatCmpCondition LiteCGIRBuilder::ConvertLiteCGPredicateFromFCMP(FCmpCondition cond) const
{
    switch (cond) {
        case FCmpCondition::OLT:
            return FloatCmpCondition::kOLT;
        case FCmpCondition::OLE:
            return FloatCmpCondition::kOLE;
        case FCmpCondition::OGT:
            return FloatCmpCondition::kOGT;
        case FCmpCondition::OGE:
            return FloatCmpCondition::kOGE;
        case FCmpCondition::ONE:
            return FloatCmpCondition::kONE;
        case FCmpCondition::OEQ:
            return FloatCmpCondition::kOEQ;
        default:
            LOG_COMPILER(FATAL) << "unexpected cond!";
            UNREACHABLE();
    }
    return FloatCmpCondition::kOEQ;
}

void LiteCGIRBuilder::VisitCmp(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *returnType = ConvertLiteCGTypeFromGate(gate);

    [[maybe_unused]] auto e1ValCode = acc_.GetMachineType(e1);
    [[maybe_unused]] auto e2ValCode = acc_.GetMachineType(e2);
    ASSERT((e1ValCode == e2ValCode) ||
           (compCfg_->Is64Bit() && (e1ValCode == MachineType::ARCH) && (e2ValCode == MachineType::I64)) ||
           (compCfg_->Is64Bit() && (e2ValCode == MachineType::ARCH) && (e1ValCode == MachineType::I64)));
    auto op = acc_.GetOpCode(gate);
    if (op == OpCode::ICMP) {
        auto cond = acc_.GetICmpCondition(gate);
        auto litecgCond = ConvertLiteCGPredicateFromICMP(cond);
        Expr result = lmirBuilder_->ICmp(returnType, e1Value, e2Value, litecgCond);
        SaveGate2Expr(gate, result);
    } else if (op == OpCode::FCMP) {
        auto cond = acc_.GetFCmpCondition(gate);
        auto litecgCond = ConvertLiteCGPredicateFromFCMP(cond);
        Expr result = lmirBuilder_->FCmp(returnType, e1Value, e2Value, litecgCond);
        SaveGate2Expr(gate, result);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void LiteCGIRBuilder::HandleBranch(GateRef gate)
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

void LiteCGIRBuilder::VisitBranch(GateRef gate, GateRef cmp, int btrue, int bfalse)
{
    if ((gate2Expr_.count(cmp) == 0) && (acc_.GetOpCode(cmp) != OpCode::CONSTANT)) {
        OPTIONAL_LOG_COMPILER(ERROR) << "Branch condition gate is nullptr!";
        return;
    }
    uint32_t trueWeight = 0;
    uint32_t falseWeight = 0;
    if (acc_.HasBranchWeight(gate)) {
        trueWeight = acc_.GetTrueWeight(gate);
        falseWeight = acc_.GetFalseWeight(gate);
    }
    BB &curBB = GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]);
    lmirBuilder_->AppendBB(curBB);
    BB &bb = CreateBB();
    BB &trueBB = GetOrCreateBB(btrue);
    BB &falseBB = GetOrCreateBB(bfalse);
    // we hope that branch with higher probability can be placed immediatly behind
    if (trueWeight < falseWeight) {
        Stmt &stmt = lmirBuilder_->Goto(falseBB);
        lmirBuilder_->AppendStmt(bb, stmt);
        lmirBuilder_->AppendBB(bb);
        Expr cond = GetExprFromGate(cmp);
        Stmt &condBR = lmirBuilder_->CondGoto(cond, trueBB, true);
        lmirBuilder_->AppendStmt(curBB, condBR);
        return;
    }
    Stmt &stmt = lmirBuilder_->Goto(trueBB);
    lmirBuilder_->AppendStmt(bb, stmt);
    lmirBuilder_->AppendBB(bb);
    Expr cond = GetExprFromGate(cmp);
    Stmt &condBR = lmirBuilder_->CondGoto(cond, falseBB, false);
    lmirBuilder_->AppendStmt(curBB, condBR);
}

void LiteCGIRBuilder::HandleReturn(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitReturn(gate, 1, ins);
}

void LiteCGIRBuilder::VisitReturn([[maybe_unused]] GateRef gate, [[maybe_unused]] GateRef popCount,
                                  const std::vector<GateRef> &operands)
{
    // [STATE] [DEPEND] [VALUE] [RETURN_LIST]
    GateRef operand = operands[2];  // 2: skip 2 in gate that are not data gate
    Expr returnValue = GetExprFromGate(operand);
    Stmt &returnNode = lmirBuilder_->Return(returnValue);
    BB &curBB = GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]);
    lmirBuilder_->AppendStmt(curBB, returnNode);
    lmirBuilder_->AppendBB(curBB);
}

Expr LiteCGIRBuilder::GetRTStubOffset(Expr glue, int index)
{
    size_t slotOffset = JSThread::GlueData::GetRTStubEntriesOffset(compCfg_->Is32Bit()) + index * slotSize_;
    Const &constVal = lmirBuilder_->CreateIntConst(glue.GetType(), static_cast<int64_t>(slotOffset));
    return lmirBuilder_->ConstVal(constVal);
}

Expr LiteCGIRBuilder::GetCoStubOffset(Expr glue, int index) const
{
    size_t offset =
        JSThread::GlueData::GetCOStubEntriesOffset(compCfg_->Is32Bit()) + static_cast<size_t>(index * slotSize_);
    Const &constVal = lmirBuilder_->CreateIntConst(glue.GetType(), static_cast<int64_t>(offset));
    return lmirBuilder_->ConstVal(constVal);
}

Expr LiteCGIRBuilder::GetBaselineStubOffset(Expr glue, int index) const
{
    size_t offset =
        JSThread::GlueData::GetBaselineStubEntriesOffset(compCfg_->Is32Bit()) + static_cast<size_t>(index * slotSize_);
    Const &constVal = lmirBuilder_->CreateIntConst(glue.GetType(), static_cast<int64_t>(offset));
    return lmirBuilder_->ConstVal(constVal);
}

void LiteCGIRBuilder::HandleRuntimeCall(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitRuntimeCall(gate, ins);
};

LiteCGType *LiteCGIRBuilder::ConvertLiteCGTypeFromVariableType(VariableType type) const
{
    std::map<VariableType, LiteCGType *> machineTypeMap = {
        {VariableType::VOID(), lmirBuilder_->voidType},
        {VariableType::BOOL(), lmirBuilder_->u1Type},
        {VariableType::INT8(), lmirBuilder_->i8Type},
        {VariableType::INT16(), lmirBuilder_->i16Type},
        {VariableType::INT32(), lmirBuilder_->i32Type},
        {VariableType::INT64(), lmirBuilder_->i64Type},
        {VariableType::FLOAT32(), lmirBuilder_->f32Type},
        {VariableType::FLOAT64(), lmirBuilder_->f64Type},
        {VariableType::NATIVE_POINTER(), lmirBuilder_->i64Type},
        {VariableType::JS_POINTER(), lmirBuilder_->i64RefType},
        {VariableType::JS_ANY(), lmirBuilder_->i64RefType},
    };
    return machineTypeMap[type];
}

LiteCGType *LiteCGIRBuilder::GenerateFuncType(const std::vector<Expr> &params, const CallSignature *stubDescriptor)
{
    LiteCGType *retType = ConvertLiteCGTypeFromVariableType(stubDescriptor->GetReturnType());
    std::vector<LiteCGType *> paramTys;
    for (auto value : params) {
        paramTys.emplace_back(value.GetType());
    }
    LiteCGType *functionType = lmirBuilder_->CreateFuncType(paramTys, retType, false);
    return functionType;
}

LiteCGType *LiteCGIRBuilder::GetFuncType(const CallSignature *stubDescriptor) const
{
    LiteCGType *returnType = ConvertLiteCGTypeFromVariableType(stubDescriptor->GetReturnType());
    std::vector<LiteCGType *> paramTys;
    auto paramCount = stubDescriptor->GetParametersCount();
    auto paramsType = stubDescriptor->GetParametersType();
    if (paramsType != nullptr) {
        LiteCGType *glueType = ConvertLiteCGTypeFromVariableType(paramsType[0]);
        paramTys.push_back(glueType);

        for (size_t i = 1; i < paramCount; i++) {
            paramTys.push_back(ConvertLiteCGTypeFromVariableType(paramsType[i]));
        }
    }
    auto funcType = lmirBuilder_->CreateFuncType(paramTys, returnType, stubDescriptor->IsVariadicArgs());
    return funcType;
}

Expr LiteCGIRBuilder::GetFunction(BB &bb, Expr glue, const CallSignature *signature, Expr rtbaseoffset,
                                  const std::string &realName) const
{
    LiteCGType *rtfuncType = GetFuncType(signature);
    LiteCGType *rtfuncTypePtr = lmirBuilder_->CreatePtrType(rtfuncType);
    LiteCGType *rtFuncTypePtrPtr = lmirBuilder_->CreatePtrType(rtfuncTypePtr);
    LiteCGType *glueType = (glue.GetType());
    LiteCGType *glueTypePtr = lmirBuilder_->CreatePtrType(glueType);
    Expr rtbaseAddr = lmirBuilder_->Cvt(rtbaseoffset.GetType(), glueTypePtr, rtbaseoffset);

    Expr funcAddr = lmirBuilder_->Iread(glueType, rtbaseAddr, glueTypePtr);
    Expr callee = lmirBuilder_->Cvt(glueType, rtFuncTypePtrPtr, funcAddr);

    std::string name = realName.empty() ? signature->GetName() : realName;
    Stmt &comment = lmirBuilder_->Comment("function: " + name);
    lmirBuilder_->AppendStmt(bb, comment);
    PregIdx funcPregIdx = lmirBuilder_->CreatePreg(callee.GetType());
    Stmt &funcAddrNode = lmirBuilder_->Regassign(callee, funcPregIdx);
    lmirBuilder_->AppendStmt(bb, funcAddrNode);

    return lmirBuilder_->Regread(funcPregIdx);
}

bool LiteCGIRBuilder::IsOptimizedJSFunction() const
{
    return circuit_->GetFrameType() == FrameType::OPTIMIZED_JS_FUNCTION_FRAME ||
        circuit_->GetFrameType() == FrameType::FASTJIT_FUNCTION_FRAME;
}

bool LiteCGIRBuilder::IsOptimized() const
{
    return circuit_->GetFrameType() == FrameType::OPTIMIZED_FRAME;
}

CallExceptionKind LiteCGIRBuilder::GetCallExceptionKind(OpCode op, size_t index) const
{
    ASSERT(op != OpCode::NOGC_RUNTIME_CALL || index != SIZE_MAX);
    bool hasPcOffset = IsOptimizedJSFunction() &&
                       ((op == OpCode::NOGC_RUNTIME_CALL && (kungfu::RuntimeStubCSigns::IsAsmStub(index))) ||
                        (op == OpCode::CALL) || (op == OpCode::RUNTIME_CALL) || (op == OpCode::BUILTINS_CALL));
    return hasPcOffset ? CallExceptionKind::HAS_PC_OFFSET : CallExceptionKind::NO_PC_OFFSET;
}

void LiteCGIRBuilder::VisitRuntimeCall(GateRef gate, const std::vector<GateRef> &inList)
{
    StubIdType stubId = RTSTUB_ID(CallRuntime);
    Expr glue = GetGlue(inList);
    int stubIndex = static_cast<int>(std::get<RuntimeStubCSigns::ID>(stubId));
    Expr rtoffset = GetRTStubOffset(glue, stubIndex);
    Expr rtbaseOffset = lmirBuilder_->Add(glue.GetType(), glue, rtoffset);
    const CallSignature *signature = RuntimeStubCSigns::Get(std::get<RuntimeStubCSigns::ID>(stubId));

    CallExceptionKind kind = GetCallExceptionKind(OpCode::RUNTIME_CALL);
    bool hasPCOffset = (kind == CallExceptionKind::HAS_PC_OFFSET);
    size_t actualNumArgs = hasPCOffset ? (inList.size() - 2) : inList.size();  // 2: pcOffset and frameArgs

    std::vector<Expr> params;
    params.push_back(glue);  // glue

    const int index = static_cast<int>(acc_.GetConstantValue(inList[static_cast<int>(CallInputs::TARGET)]));
    Expr indexValue =
        lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->u64Type, static_cast<uint64_t>(index)));
    params.push_back(indexValue);  // target

    const int64_t argc = actualNumArgs - static_cast<size_t>(CallInputs::FIRST_PARAMETER);
    Expr argcValue =
        lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->u64Type, static_cast<uint64_t>(argc)));
    params.push_back(argcValue);  // argc

    for (size_t paraIdx = static_cast<size_t>(CallInputs::FIRST_PARAMETER); paraIdx < actualNumArgs; ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params.push_back(GetExprFromGate(gateTmp));
    }

    std::string targetName = RuntimeStubCSigns::GetRTName(index);
    BB &bb = GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]);
    std::string name = targetName.empty() ? signature->GetName() : targetName;
    Expr callee = GetFunction(bb, glue, signature, rtbaseOffset, name);

    LiteCGType *returnType = ConvertLiteCGTypeFromVariableType(signature->GetReturnType());
    bool returnVoid = (returnType == lmirBuilder_->voidType);
    PregIdx returnPregIdx = returnVoid ? -1 : lmirBuilder_->CreatePreg(returnType);
    Stmt &callNode =
        returnVoid ? lmirBuilder_->ICall(callee, params) : lmirBuilder_->ICall(callee, params, returnPregIdx);

    if (kind == CallExceptionKind::HAS_PC_OFFSET) {
        std::unordered_map<int, LiteCGValue> deoptBundleInfo;
        auto frameArgs = inList.at(actualNumArgs);
        Expr pcOffset = hasPCOffset ? (GetExprFromGate(inList[actualNumArgs + 1]))
                                    : lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->i32Type, 0));
        CollectExraCallSiteInfo(deoptBundleInfo, pcOffset, frameArgs);
        lmirBuilder_->SetCallStmtDeoptBundleInfo(callNode, deoptBundleInfo);
    }
    lmirBuilder_->SetStmtCallConv(callNode, maple::litecg::Web_Kit_JS_Call);
    lmirBuilder_->AppendStmt(bb, callNode);
    if (!returnVoid) {
        SaveGate2Expr(gate, lmirBuilder_->Regread(returnPregIdx));
    }
}

void LiteCGIRBuilder::HandleZExtInt(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitZExtInt(gate, ins[0]);
}

void LiteCGIRBuilder::VisitZExtInt(GateRef gate, GateRef e1)
{
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(e1)) <=
           GetBitWidthFromMachineType(acc_.GetMachineType(gate)));
    LiteCGType *fromType = ConvertLiteCGTypeFromGate(e1);
    LiteCGType *toType = ConvertLiteCGTypeFromGate(gate, false);
    Expr result = lmirBuilder_->ZExt(fromType, toType, GetExprFromGate(e1));
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleIntDiv(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntDiv(gate, g0, g1);
}

void LiteCGIRBuilder::VisitIntDiv(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->SDiv(type, e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

Expr LiteCGIRBuilder::GetCallee(maple::litecg::BB &bb, const std::vector<GateRef> &inList,
                                const CallSignature *signature, const std::string &realName)
{
    LiteCGType *rtfuncType = GetFuncType(signature);
    LiteCGType *rtfuncTypePtr = lmirBuilder_->CreatePtrType(rtfuncType);
    LiteCGType *rtfuncTypePtrPtr = lmirBuilder_->CreatePtrType(rtfuncTypePtr);
    Expr code = GetExprFromGate(inList[static_cast<size_t>(CallInputs::TARGET)]);
    Expr callee = lmirBuilder_->Cvt(code.GetType(), rtfuncTypePtrPtr, code);

    std::string name = realName.empty() ? signature->GetName() : realName;
    Stmt &comment = lmirBuilder_->Comment("function: " + name);
    lmirBuilder_->AppendStmt(bb, comment);

    PregIdx funcPregIdx = lmirBuilder_->CreatePreg(callee.GetType());
    Stmt &funcAddrNode = lmirBuilder_->Regassign(callee, funcPregIdx);
    lmirBuilder_->AppendStmt(bb, funcAddrNode);
    return lmirBuilder_->Regread(funcPregIdx);
}

void LiteCGIRBuilder::HandleRuntimeCallWithArgv(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitRuntimeCallWithArgv(gate, ins);
}

void LiteCGIRBuilder::VisitRuntimeCallWithArgv(GateRef gate, const std::vector<GateRef> &inList)
{
    ASSERT(IsOptimized() == true);
    StubIdType stubId = RTSTUB_ID(CallRuntimeWithArgv);
    Expr glue = GetGlue(inList);
    int stubIndex = static_cast<int>(std::get<RuntimeStubCSigns::ID>(stubId));
    Expr rtoffset = GetRTStubOffset(glue, stubIndex);
    Expr rtbaseoffset = lmirBuilder_->Add(glue.GetType(), glue, rtoffset);
    const CallSignature *signature = RuntimeStubCSigns::Get(std::get<RuntimeStubCSigns::ID>(stubId));

    std::vector<Expr> params;
    params.push_back(glue);  // glue

    uint64_t index = acc_.GetConstantValue(inList[static_cast<size_t>(CallInputs::TARGET)]);
    auto targetId = lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->i64Type, index));
    params.push_back(targetId);  // target
    for (size_t paraIdx = static_cast<size_t>(CallInputs::FIRST_PARAMETER); paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params.push_back(GetExprFromGate(gateTmp));
    }

    BB &bb = GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]);
    std::string targetName = RuntimeStubCSigns::GetRTName(index);
    std::string name = targetName.empty() ? signature->GetName() : targetName;
    Expr callee = GetFunction(bb, glue, signature, rtbaseoffset, name);

    static uint32_t val = 0;
    std::string returnCallValName = name + "Ret" + std::to_string(val++);
    LiteCGType *returnType = ConvertLiteCGTypeFromVariableType(signature->GetReturnType());
    Var *returnVar = (returnType == lmirBuilder_->voidType)
                         ? nullptr
                         : &(lmirBuilder_->CreateLocalVar(returnType, returnCallValName));
    Stmt &callNode = lmirBuilder_->ICall(callee, params, returnVar);
    lmirBuilder_->AppendStmt(bb, callNode);
    if (returnVar != nullptr) {
        SaveGate2Expr(gate, lmirBuilder_->Dread(*returnVar));
    }
}

void LiteCGIRBuilder::HandleCall(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    OpCode callOp = acc_.GetOpCode(gate);
    if (callOp == OpCode::CALL || callOp == OpCode::NOGC_RUNTIME_CALL || callOp == OpCode::BUILTINS_CALL ||
        callOp == OpCode::BUILTINS_CALL_WITH_ARGV || callOp == OpCode::CALL_OPTIMIZED ||
        callOp == OpCode::FAST_CALL_OPTIMIZED || callOp == OpCode::BASELINE_CALL ||
        callOp == OpCode::ASM_CALL_BARRIER) {
        VisitCall(gate, ins, callOp);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

void LiteCGIRBuilder::VisitCall(GateRef gate, const std::vector<GateRef> &inList, OpCode op)
{
    size_t targetIndex = static_cast<size_t>(CallInputs::TARGET);
    static_assert(static_cast<size_t>(CallInputs::FIRST_PARAMETER) == 3);
    const CallSignature *calleeDescriptor = nullptr;
    Expr glue = GetGlue(inList);
    Expr callee;
    CallExceptionKind kind = CallExceptionKind::NO_PC_OFFSET;
    BB &bb = GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]);
    if (op == OpCode::CALL) {
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = CommonStubCSigns::Get(index);
        Expr rtoffset = GetCoStubOffset(glue, index);
        Expr rtbaseoffset = lmirBuilder_->Add(glue.GetType(), glue, rtoffset);
        callee = GetFunction(bb, glue, calleeDescriptor, rtbaseoffset);
        kind = GetCallExceptionKind(op);
    } else if (op == OpCode::NOGC_RUNTIME_CALL) {
        UpdateLeaveFrame(glue);
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = RuntimeStubCSigns::Get(index);
        Expr rtoffset = GetRTStubOffset(glue, index);
        Expr rtbaseoffset = lmirBuilder_->Add(glue.GetType(), glue, rtoffset);
        callee = GetFunction(bb, glue, calleeDescriptor, rtbaseoffset);
        kind = GetCallExceptionKind(op, index);
    } else if (op == OpCode::CALL_OPTIMIZED) {
        calleeDescriptor = RuntimeStubCSigns::GetOptimizedCallSign();
        callee = GetCallee(bb, inList, calleeDescriptor, calleeDescriptor->GetName());
        if (IsOptimizedJSFunction()) {
            kind = CallExceptionKind::HAS_PC_OFFSET;
        } else {
            kind = CallExceptionKind::NO_PC_OFFSET;
        }
    } else if (op == OpCode::FAST_CALL_OPTIMIZED) {
        calleeDescriptor = RuntimeStubCSigns::GetOptimizedFastCallSign();
        callee = GetCallee(bb, inList, calleeDescriptor, calleeDescriptor->GetName());
        if (IsOptimizedJSFunction()) {
            kind = CallExceptionKind::HAS_PC_OFFSET;
        } else {
            kind = CallExceptionKind::NO_PC_OFFSET;
        }
    } else if (op == OpCode::BASELINE_CALL) {
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = BaselineStubCSigns::Get(index);
        Expr rtoffset = GetBaselineStubOffset(glue, index);
        Expr rtbaseoffset = lmirBuilder_->Add(glue.GetType(), glue, rtoffset);
        callee = GetFunction(bb, glue, calleeDescriptor, rtbaseoffset);
        kind = GetCallExceptionKind(op);
    } else if (op == OpCode::ASM_CALL_BARRIER) {
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = RuntimeStubCSigns::Get(index);
        Expr rtoffset = GetRTStubOffset(glue, index);
        Expr rtbaseoffset = lmirBuilder_->Add(glue.GetType(), glue, rtoffset);
        callee = GetFunction(bb, glue, calleeDescriptor, rtbaseoffset);
    } else {
        ASSERT(op == OpCode::BUILTINS_CALL || op == OpCode::BUILTINS_CALL_WITH_ARGV);
        Expr opcodeOffset = GetExprFromGate(inList[targetIndex]);
        Expr rtoffset = GetBuiltinsStubOffset(glue);
        Expr offset = lmirBuilder_->Add(rtoffset.GetType(), rtoffset, opcodeOffset);
        Expr rtbaseoffset = lmirBuilder_->Add(glue.GetType(), glue, offset);
        if (op == OpCode::BUILTINS_CALL) {
            calleeDescriptor = BuiltinsStubCSigns::BuiltinsCSign();
        } else {
            calleeDescriptor = BuiltinsStubCSigns::BuiltinsWithArgvCSign();
        }
        callee = GetFunction(bb, glue, calleeDescriptor, rtbaseoffset);
        kind = GetCallExceptionKind(op);
    }

    std::vector<Expr> params;
    const size_t firstArg = static_cast<size_t>(CallInputs::FIRST_PARAMETER);
    GateRef glueGate = inList[firstArg];
    params.push_back(GetExprFromGate(glueGate));

    LiteCGType *calleeFuncType = lmirBuilder_->LiteCGGetPointedType(callee.GetType());
    std::vector<LiteCGType *> paramTypes = lmirBuilder_->LiteCGGetFuncParamTypes(calleeFuncType);

    bool hasPCOffset = (kind == CallExceptionKind::HAS_PC_OFFSET);
    size_t actualNumArgs = hasPCOffset ? (inList.size() - 2) : inList.size();  // 2: pcOffset and frameArgs

    // then push the actual parameter for js function call
    for (size_t paraIdx = firstArg + 1; paraIdx < actualNumArgs; ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        Expr gateExpr = GetExprFromGate(gateTmp);
        const auto gateTmpType = gateExpr.GetType();
        if (params.size() < paramTypes.size()) {  // this condition will be false for variadic arguments
            const auto paramType = paramTypes.at(params.size());
            // match parameter types and function signature types
            if (lmirBuilder_->IsHeapPointerType(paramType) && !lmirBuilder_->IsHeapPointerType(gateTmpType)) {
                Expr cvtI64Expr = lmirBuilder_->Cvt(gateTmpType, lmirBuilder_->i64Type, gateExpr);
                params.push_back(lmirBuilder_->Cvt(lmirBuilder_->i64Type, paramType, cvtI64Expr));
            } else {
                params.push_back(lmirBuilder_->Cvt(gateTmpType, paramType, gateExpr));
            }
        } else {
            params.push_back(gateExpr);
        }
    }

    LiteCGType *returnType = lmirBuilder_->LiteCGGetFuncReturnType(calleeFuncType);
    bool returnVoid = (returnType == lmirBuilder_->voidType);
    PregIdx returnPregIdx = returnVoid ? -1 : lmirBuilder_->CreatePreg(returnType);
    if (op == OpCode::ASM_CALL_BARRIER) {
        if (!returnVoid)  LOG_JIT(INFO) << "barrier has return use\n";
        Stmt &pureCall = lmirBuilder_->PureCall(callee, params);
        lmirBuilder_->AppendStmt(bb, pureCall);
        return;
    }
    Stmt &callNode =
        returnVoid ? lmirBuilder_->ICall(callee, params) : lmirBuilder_->ICall(callee, params, returnPregIdx);
    if (kind == CallExceptionKind::HAS_PC_OFFSET) {
        std::unordered_map<int, LiteCGValue> deoptBundleInfo;
        auto frameArgs = inList.at(actualNumArgs);
        Expr pcOffset = hasPCOffset ? (GetExprFromGate(inList[actualNumArgs + 1]))
                                    : lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->i32Type, 0));
        CollectExraCallSiteInfo(deoptBundleInfo, pcOffset, frameArgs);
        lmirBuilder_->SetCallStmtDeoptBundleInfo(callNode, deoptBundleInfo);
    }
    lmirBuilder_->SetStmtCallConv(callNode, ConvertCallAttr(calleeDescriptor->GetCallConv()));
    lmirBuilder_->AppendStmt(bb, callNode);
    if (!returnVoid) {
        SaveGate2Expr(gate, lmirBuilder_->Regread(returnPregIdx));
    }
}

void LiteCGIRBuilder::CollectExraCallSiteInfo(std::unordered_map<int, maple::litecg::LiteCGValue> &deoptBundleInfo,
                                              maple::litecg::Expr pcOffset, GateRef frameArgs)
{
    // pc offset
    auto pcIndex = static_cast<int>(SpecVregIndex::PC_OFFSET_INDEX);
    ASSERT(pcOffset.IsConstValue());
    deoptBundleInfo.insert(std::pair<int, LiteCGValue>(
        pcIndex, {LiteCGValueKind::kConstKind, lmirBuilder_->GetConstFromExpr(pcOffset)}));

    if (!enableOptInlining_) {
        return;
    }

    if (frameArgs == Circuit::NullGate()) {
        return;
    }
    if (acc_.GetOpCode(frameArgs) != OpCode::FRAME_ARGS) {
        return;
    }
    uint32_t maxDepth = acc_.GetFrameDepth(frameArgs, OpCode::FRAME_ARGS);
    if (maxDepth == 0) {
        return;
    }

    maxDepth = std::min(maxDepth, MAX_METHOD_OFFSET_NUM);
    size_t shift = Deoptimizier::ComputeShift(MAX_METHOD_OFFSET_NUM);
    ArgumentAccessor argAcc(const_cast<Circuit *>(circuit_));
    for (int32_t curDepth = static_cast<int32_t>(maxDepth - 1); curDepth >= 0; curDepth--) {
        ASSERT(acc_.GetOpCode(frameArgs) == OpCode::FRAME_ARGS);
        // method id
        uint32_t methodOffset = acc_.TryGetMethodOffset(frameArgs);
        frameArgs = acc_.GetFrameState(frameArgs);
        if (methodOffset == FrameStateOutput::INVALID_INDEX) {
            methodOffset = 0;
        }
        int32_t specCallTargetIndex = static_cast<int32_t>(SpecVregIndex::FIRST_METHOD_OFFSET_INDEX) - curDepth;
        int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(specCallTargetIndex, curDepth, shift);
        auto constMethodOffset = lmirBuilder_->GetConstFromExpr(
            lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->i32Type, methodOffset)));
        deoptBundleInfo.insert(
            std::pair<int, LiteCGValue>(encodeIndex, {LiteCGValueKind::kConstKind, constMethodOffset}));
    }
}

maple::litecg::ConvAttr LiteCGIRBuilder::ConvertCallAttr(const CallSignature::CallConv callConv)
{
    switch (callConv) {
        case CallSignature::CallConv::GHCCallConv: {
            return maple::litecg::GHC_Call;
        }
        case CallSignature::CallConv::WebKitJSCallConv: {
            return maple::litecg::Web_Kit_JS_Call;
        }
        default: {
            return maple::litecg::CCall;
        }
    }
}

Expr LiteCGIRBuilder::GetBuiltinsStubOffset(Expr glue)
{
    Const &constVal = lmirBuilder_->CreateIntConst(
        glue.GetType(), JSThread::GlueData::GetBuiltinsStubEntriesOffset(compCfg_->Is32Bit()));
    return lmirBuilder_->ConstVal(constVal);
}

void LiteCGIRBuilder::UpdateLeaveFrame(Expr glue)
{
    Expr leaveFrameOffset = GetLeaveFrameOffset(glue);
    Expr leaveFrameValue = lmirBuilder_->Add(glue.GetType(), glue, leaveFrameOffset);
    LiteCGType *glueType = glue.GetType();
    LiteCGType *glueTypePtr = lmirBuilder_->CreatePtrType(glueType);
    Expr leaveFrameAddr = lmirBuilder_->Cvt(leaveFrameValue.GetType(), glueTypePtr, leaveFrameValue);
    Expr fpAddr = CallingFp(true);
    Expr fp = lmirBuilder_->Cvt(fpAddr.GetType(), lmirBuilder_->i64Type, fpAddr);

    lmirBuilder_->Iassign(fp, leaveFrameAddr, fp.GetType());
}

bool LiteCGIRBuilder::IsInterpreted() const
{
    return circuit_->GetFrameType() == FrameType::ASM_INTERPRETER_FRAME;
}

bool LiteCGIRBuilder::IsBaselineBuiltin() const
{
    return circuit_->GetFrameType() == FrameType::BASELINE_BUILTIN_FRAME;
}

Expr LiteCGIRBuilder::CallingFp(bool /*isCaller*/)
{
    ASSERT(!IsInterpreted() && !IsBaselineBuiltin());
    /* 0:calling 1:its caller */
    Function &func = lmirBuilder_->GetCurFunction();
    return lmirBuilder_->LiteCGGetPregFP(func);
}

Expr LiteCGIRBuilder::GetLeaveFrameOffset(Expr glue)
{
    size_t slotOffset = JSThread::GlueData::GetLeaveFrameOffset(compCfg_->Is32Bit());
    Const &constVal = lmirBuilder_->CreateIntConst(glue.GetType(), static_cast<int>(slotOffset));
    return lmirBuilder_->ConstVal(constVal);
}

void LiteCGIRBuilder::HandleUDiv(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitUDiv(gate, g0, g1);
}

void LiteCGIRBuilder::VisitUDiv(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->UDiv(type, e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleIntAnd(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntAnd(gate, g0, g1);
}

void LiteCGIRBuilder::VisitIntAnd(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->And(type, e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleIntOr(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntOr(gate, g0, g1);
}

void LiteCGIRBuilder::VisitIntOr(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->Or(type, e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleIntXor(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntXor(gate, g0, g1);
}

void LiteCGIRBuilder::VisitIntXor(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->Xor(type, e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleIntLsr(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntLsr(gate, g0, g1);
}

void LiteCGIRBuilder::VisitIntLsr(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->LShr(type, e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleIntAsr(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntAsr(gate, g0, g1);
}

void LiteCGIRBuilder::VisitIntAsr(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->AShr(type, e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleBitCast(GateRef gate)
{
    VisitBitCast(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitBitCast(GateRef gate, GateRef e1)
{
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(gate)) ==
           GetBitWidthFromMachineType(acc_.GetMachineType(e1)));
    LiteCGType *fromType = ConvertLiteCGTypeFromGate(e1);
    LiteCGType *toType = ConvertLiteCGTypeFromGate(gate);
    Expr e1Value = GetExprFromGate(e1);
    Expr result = lmirBuilder_->BitCast(fromType, toType, e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleIntLsl(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitIntLsl(gate, g0, g1);
}

void LiteCGIRBuilder::VisitIntLsl(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->Shl(type, e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleMod(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitMod(gate, g0, g1);
}

void LiteCGIRBuilder::VisitMod(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    ASSERT(type == ConvertLiteCGTypeFromGate(e1));
    ASSERT(type == ConvertLiteCGTypeFromGate(e2));
    auto machineType = acc_.GetMachineType(gate);
    Expr result;
    if (machineType == MachineType::I32) {
        result = lmirBuilder_->SRem(type, e1Value, e2Value);
    } else if (machineType != MachineType::F64) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleFinishAllocate(GateRef gate)
{
    GateRef g0 = acc_.GetValueIn(gate, 0);
    VisitFinishAllocate(gate, g0);
}

void LiteCGIRBuilder::VisitFinishAllocate(GateRef gate, GateRef e1)
{
    Expr result = GetExprFromGate(e1);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleCastIntXToIntY(GateRef gate)
{
    VisitCastIntXToIntY(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitCastIntXToIntY(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(e1)) >=
           GetBitWidthFromMachineType(acc_.GetMachineType(gate)));
    auto e1Type = ConvertLiteCGTypeFromGate(e1);
    Expr result = lmirBuilder_->Cvt(e1Type, ConvertLiteCGTypeFromGate(gate), e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleChangeInt32ToDouble(GateRef gate)
{
    VisitChangeInt32ToDouble(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitChangeInt32ToDouble(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    auto e1Type = ConvertLiteCGTypeFromGate(e1);
    Expr result = lmirBuilder_->Cvt(e1Type, ConvertLiteCGTypeFromGate(gate), e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleChangeUInt32ToDouble(GateRef gate)
{
    VisitChangeUInt32ToDouble(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitChangeUInt32ToDouble(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    auto e1Type = ConvertLiteCGTypeFromGate(e1);
    if (e1Type != lmirBuilder_->u32Type) {
        e1Value = lmirBuilder_->Cvt(e1Type, lmirBuilder_->u32Type, e1Value);
    }
    Expr result = lmirBuilder_->Cvt(lmirBuilder_->u32Type, ConvertLiteCGTypeFromGate(gate), e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleChangeDoubleToInt32(GateRef gate)
{
    VisitChangeDoubleToInt32(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitChangeDoubleToInt32(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    auto e1Type = ConvertLiteCGTypeFromGate(e1);
    Expr result = lmirBuilder_->Cvt(e1Type, ConvertLiteCGTypeFromGate(gate), e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleChangeTaggedPointerToInt64(GateRef gate)
{
    VisitChangeTaggedPointerToInt64(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitChangeTaggedPointerToInt64(GateRef gate, GateRef e1)
{
    Expr result = CanonicalizeToInt(e1);
    SaveGate2Expr(gate, result);
}

Expr LiteCGIRBuilder::CanonicalizeToInt(GateRef gate)
{
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    Expr opnd = GetExprFromGate(gate);
    if (lmirBuilder_->LiteCGGetTypeKind(type) == maple::litecg::kLiteCGTypePointer) {
        return lmirBuilder_->Cvt(type, lmirBuilder_->i64Type, opnd);
    } else if (lmirBuilder_->LiteCGGetTypeKind(type) == maple::litecg::kLiteCGTypeScalar) {
        return opnd;
    } else {
        LOG_COMPILER(FATAL) << "can't Canonicalize to Int64: ";
        UNREACHABLE();
    }
}

void LiteCGIRBuilder::HandleChangeInt64ToTagged(GateRef gate)
{
    VisitChangeInt64ToTagged(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitChangeInt64ToTagged(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    ASSERT(lmirBuilder_->LiteCGGetTypeKind(ConvertLiteCGTypeFromGate(e1)) == maple::litecg::kLiteCGTypeScalar);
    Expr result = lmirBuilder_->Cvt(lmirBuilder_->i64Type, lmirBuilder_->i64RefType, e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleDoubleTrunc(GateRef gate)
{
    GateRef param = acc_.GetIn(gate, 0);
    VisitDoubleTrunc(gate, param);
}

void LiteCGIRBuilder::VisitDoubleTrunc(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    ASSERT(acc_.GetMachineType(e1) == acc_.GetMachineType(gate));
    Expr result = lmirBuilder_->Trunc(type, type, e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleSub(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitSub(gate, g0, g1);
}

void LiteCGIRBuilder::VisitSub(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    Expr result;
    LiteCGType *returnType = ConvertLiteCGTypeFromGate(gate);
    auto machineType = acc_.GetMachineType(gate);
    if (machineType == MachineType::I16 || machineType == MachineType::I32 || machineType == MachineType::I64 ||
        machineType == MachineType::ARCH || machineType == MachineType::F64) {
        result = lmirBuilder_->Sub(returnType, e1Value, e2Value);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleMul(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitMul(gate, g0, g1);
}

void LiteCGIRBuilder::VisitMul(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    Expr result;
    LiteCGType *returnType = ConvertLiteCGTypeFromGate(gate);
    auto machineType = acc_.GetMachineType(gate);
    if (IsMulIntergerType(machineType)) {
        result = lmirBuilder_->Mul(returnType, e1Value, e2Value);
    } else if (machineType == MachineType::F64) {
        result = lmirBuilder_->Mul(returnType, e1Value, e2Value);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleIntRev(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitIntRev(gate, ins[0]);
}

void LiteCGIRBuilder::VisitIntRev(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    ASSERT(type == ConvertLiteCGTypeFromGate(e1));
    Expr result;
    auto machineType = acc_.GetMachineType(gate);
    if (machineType <= MachineType::I64 && machineType >= MachineType::I1) {
        if (machineType == MachineType::I1) {
            result = lmirBuilder_->Lnot(type, e1Value);
        } else {
            result = lmirBuilder_->Bnot(type, e1Value);
        }
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleFloatDiv(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    auto g1 = acc_.GetIn(gate, 1);
    VisitFloatDiv(gate, g0, g1);
}

void LiteCGIRBuilder::VisitFloatDiv(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    Expr result = lmirBuilder_->SDiv(ConvertLiteCGTypeFromGate(gate), e1Value, e2Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleTruncFloatToInt(GateRef gate)
{
    auto g0 = acc_.GetIn(gate, 0);
    VisitTruncFloatToInt(gate, g0);
}

void LiteCGIRBuilder::VisitTruncFloatToInt(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    auto machineType = acc_.GetMachineType(e1);
    Expr result;
    if (machineType <= MachineType::F64 && machineType >= MachineType::F32) {
        result = lmirBuilder_->Trunc(ConvertLiteCGTypeFromGate(e1), lmirBuilder_->i64Type, e1Value);
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleAddWithOverflow(GateRef gate)
{
    auto in0 = acc_.GetIn(gate, 0);
    auto in1 = acc_.GetIn(gate, 1);
    ASSERT(acc_.GetMachineType(in0) == MachineType::I32);
    ASSERT(acc_.GetMachineType(in1) == MachineType::I32);
    VisitAddWithOverflow(gate, in0, in1);
}

void LiteCGIRBuilder::VisitAddWithOverflow(GateRef gate, GateRef e1, GateRef e2)
{
    VisitBinaryOpWithOverflow(gate, e1, e2, IntrinsicId::INTRN_ADD_WITH_OVERFLOW);
}

void LiteCGIRBuilder::HandleSubWithOverflow(GateRef gate)
{
    auto in0 = acc_.GetIn(gate, 0);
    auto in1 = acc_.GetIn(gate, 1);
    ASSERT(acc_.GetMachineType(in0) == MachineType::I32);
    ASSERT(acc_.GetMachineType(in1) == MachineType::I32);
    VisitSubWithOverflow(gate, in0, in1);
}

void LiteCGIRBuilder::VisitSubWithOverflow(GateRef gate, GateRef e1, GateRef e2)
{
    VisitBinaryOpWithOverflow(gate, e1, e2, IntrinsicId::INTRN_SUB_WITH_OVERFLOW);
}

void LiteCGIRBuilder::HandleMulWithOverflow(GateRef gate)
{
    auto in0 = acc_.GetIn(gate, 0);
    auto in1 = acc_.GetIn(gate, 1);
    ASSERT(acc_.GetMachineType(in0) == MachineType::I32);
    ASSERT(acc_.GetMachineType(in1) == MachineType::I32);
    VisitMulWithOverflow(gate, in0, in1);
}

void LiteCGIRBuilder::VisitMulWithOverflow(GateRef gate, GateRef e1, GateRef e2)
{
    VisitBinaryOpWithOverflow(gate, e1, e2, IntrinsicId::INTRN_MUL_WITH_OVERFLOW);
}

void LiteCGIRBuilder::VisitBinaryOpWithOverflow(GateRef gate, GateRef e1, GateRef e2, IntrinsicId intrinsicId)
{
    PregIdx preg1 = lmirBuilder_->CreatePreg(lmirBuilder_->i32Type);
    PregIdx preg2 = lmirBuilder_->CreatePreg(lmirBuilder_->u1Type);

    // generate function call
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    std::vector<Expr> args = {e1Value, e2Value};
    auto &call = lmirBuilder_->IntrinsicCall(intrinsicId, args, preg1, preg2);
    SaveGate2Expr(gate, preg1, preg2);
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), call);
}

void LiteCGIRBuilder::HandleSExtInt(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitSExtInt(gate, ins[0]);
}

void LiteCGIRBuilder::VisitSExtInt(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    LiteCGType *fromType = ConvertLiteCGTypeFromGate(e1);
    LiteCGType *toType = ConvertLiteCGTypeFromGate(gate);
    Expr result = lmirBuilder_->SExt(fromType, toType, e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleSqrt(GateRef gate)
{
    GateRef param = acc_.GetIn(gate, 0);
    VisitSqrt(gate, param);
}

void LiteCGIRBuilder::VisitSqrt(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    LiteCGType *type = ConvertLiteCGTypeFromGate(e1);
    Expr result;
    if (type == lmirBuilder_->f32Type || type == lmirBuilder_->f64Type) {
        result = lmirBuilder_->Sqrt(type, e1Value);
    } else {
        result = lmirBuilder_->Sqrt(lmirBuilder_->f64Type, lmirBuilder_->Cvt(type, lmirBuilder_->f64Type, e1Value));
    }
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleExp(GateRef gate)
{
    (void)gate;
    CHECK_FATAL(false, "not support exp !");
}

void LiteCGIRBuilder::HandleCeil(GateRef gate)
{
    VisitCeil(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitCeil(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    LiteCGType *type = ConvertLiteCGTypeFromGate(e1);
    Expr result = lmirBuilder_->Ceil(type, type, e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleAbs(GateRef gate)
{
    VisitAbs(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitAbs(GateRef gate, GateRef e1)
{
    auto machineType = acc_.GetMachineType(gate);
    ASSERT(acc_.GetMachineType(e1) == machineType);
    Expr result;
    LiteCGType *type = ConvertLiteCGTypeFromGate(e1);
    Expr e1Value = GetExprFromGate(e1);
    if (machineType == MachineType::I32 || machineType == MachineType::F64) {
        result = lmirBuilder_->Abs(type, e1Value);
    } else {
        LOG_ECMA(FATAL) << "`Abs` type should be untagged double or signed int";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
    return;
}

void LiteCGIRBuilder::HandleMin(GateRef gate)
{
    VisitMin(gate, acc_.GetIn(gate, 0), acc_.GetIn(gate, 1U));
}

void LiteCGIRBuilder::VisitMin(GateRef gate, GateRef e1, GateRef e2)
{
    auto machineType = acc_.GetMachineType(gate);
    ASSERT(acc_.GetMachineType(e1) == machineType);
    ASSERT(acc_.GetMachineType(e2) == machineType);
    Expr result;
    LiteCGType *type = ConvertLiteCGTypeFromGate(e1);
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    if (machineType == MachineType::I32 || machineType == MachineType::F64) {
        result = lmirBuilder_->Min(type, e1Value, e2Value);
    } else {
        LOG_ECMA(FATAL) << "`Min` type should be untagged double or signed int";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
    return;
}

void LiteCGIRBuilder::HandleMax(GateRef gate)
{
    VisitMax(gate, acc_.GetIn(gate, 0), acc_.GetIn(gate, 1U));
}

void LiteCGIRBuilder::VisitMax(GateRef gate, GateRef e1, GateRef e2)
{
    auto machineType = acc_.GetMachineType(gate);
    ASSERT(acc_.GetMachineType(e1) == machineType);
    ASSERT(acc_.GetMachineType(e2) == machineType);
    Expr result;
    LiteCGType *type = ConvertLiteCGTypeFromGate(e1);
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    if (machineType == MachineType::I32 || machineType == MachineType::F64) {
        result = lmirBuilder_->Max(type, e1Value, e2Value);
    } else {
        LOG_ECMA(FATAL) << "`Max` type should be untagged double or signed int";
        UNREACHABLE();
    }
    SaveGate2Expr(gate, result);
    return;
}

void LiteCGIRBuilder::HandleFloor(GateRef gate)
{
    VisitFloor(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitFloor(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    LiteCGType *type = ConvertLiteCGTypeFromGate(e1);
    Expr result = lmirBuilder_->Floor(type, type, e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleClz32(GateRef gate)
{
    VisitClz32(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitClz32(GateRef gate,  GateRef param)
{
    std::vector<Expr> params;
    params.push_back(GetExprFromGate(param));
    LiteCGType *type = ConvertLiteCGTypeFromGate(param);
    Expr result = lmirBuilder_->IntrinsicOp(IntrinsicId::INTRN_C_clz32, type, params);
    SaveGate2Expr(gate, result);
}


void LiteCGIRBuilder::HandleReadSp(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::READSP);
    VisitReadSp(gate);
}

void LiteCGIRBuilder::VisitReadSp(GateRef gate)
{
    Expr result = lmirBuilder_->LiteCGGetPregSP();
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleInitVreg(GateRef gate)
{
    ASSERT(acc_.GetOpCode(gate) == OpCode::INITVREG);
    VisitInitVreg(gate);
}

void LiteCGIRBuilder::VisitInitVreg(GateRef gate)
{
    size_t vregNumber = acc_.GetInitOffset(gate);
    BB &bb = GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]);
    LiteCGType *i64 = lmirBuilder_->i64Type;
    LiteCGType *i64Ptr = lmirBuilder_->i64PtrType;
    LiteCGType *i64Ref = lmirBuilder_->i64RefType;
    LiteCGType *i32 = lmirBuilder_->i32Type;
    switch (vregNumber) {
        case INIT_VRGE_GLUE: {
            // init glue
            Expr glue = lmirBuilder_->GenExprFromVar(
                lmirBuilder_->GetParam(lmirBuilder_->GetCurFunction(), 0));  // 0 : osr first param - glue
            SaveGate2Expr(gate, glue);
            return;
        }
        case INIT_VRGE_ARGS: {
            // init argc
            SaveGate2Expr(gate, lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(i64, 0)));
            return;
        }
        case INIT_VRGE_FUNCTION: {
            // init func
            PregIdx vreg = lmirBuilder_->CreatePreg(i64Ref);
            // load from frame
            Expr sp = lmirBuilder_->GenExprFromVar(
                lmirBuilder_->GetParam(lmirBuilder_->GetCurFunction(), 1));  // 1 : osr second param - sp
            Expr offsetFrame = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64Ptr, AsmInterpretedFrame::GetSize(!circuit_->IsArch64())));
            Expr frame = lmirBuilder_->Sub(i64Ptr, sp, offsetFrame);
            Expr offsetFunc = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64Ptr, AsmInterpretedFrame::GetFunctionOffset(!circuit_->IsArch64())));
            Expr addrFunc = lmirBuilder_->Add(i64Ptr, frame, offsetFunc);
            Expr ldrFunc = lmirBuilder_->Iread(i64Ref, addrFunc, lmirBuilder_->CreatePtrType(i64Ptr));
            lmirBuilder_->AppendStmt(bb, lmirBuilder_->Regassign(ldrFunc, vreg));
            if (IsOptimizedJSFunction()) {
                // reset jsfunc on OptJSFuncFrame
                Expr fpAddr = CallingFp(false);
                Expr frameAddr = lmirBuilder_->Cvt(fpAddr.GetType(), lmirBuilder_->i64Type, fpAddr);
                size_t reservedOffset = OptimizedJSFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
                Expr frameJSFuncSlotAddr =
                    lmirBuilder_->Sub(frameAddr.GetType(), frameAddr,
                                      lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(slotType_, reservedOffset)));
                Expr jsFuncAddr = lmirBuilder_->Cvt(frameJSFuncSlotAddr.GetType(),
                                                    lmirBuilder_->CreatePtrType(slotType_), frameJSFuncSlotAddr);
                Expr jsFuncValue = lmirBuilder_->Cvt(lmirBuilder_->i64PtrType, slotType_, lmirBuilder_->Regread(vreg));
                auto &stmt = lmirBuilder_->Iassign(jsFuncValue, jsFuncAddr, jsFuncAddr.GetType());
                lmirBuilder_->AppendStmt(bb, stmt);
            }
            SaveGate2Expr(gate, lmirBuilder_->Regread(vreg));
            return;
        }
        case INIT_VRGE_NEW_TARGET: {
            // init new_target
            PregIdx vreg = lmirBuilder_->CreatePreg(i64Ref);
            // load func from interpreter sp
            Expr sp = lmirBuilder_->GenExprFromVar(
                lmirBuilder_->GetParam(lmirBuilder_->GetCurFunction(), 1));  // 1 : osr second param - sp
            Expr offsetFrame = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64Ptr, AsmInterpretedFrame::GetSize(!circuit_->IsArch64())));
            Expr frame = lmirBuilder_->Sub(i64Ptr, sp, offsetFrame);
            Expr offsetFunc = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64Ptr, AsmInterpretedFrame::GetFunctionOffset(!circuit_->IsArch64())));
            Expr addrFunc = lmirBuilder_->Add(i64Ptr, frame, offsetFunc);
            Expr func = lmirBuilder_->Iread(i64Ref, addrFunc, lmirBuilder_->CreatePtrType(i64Ptr));
            // load method from func
            Expr offsetMethod = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(lmirBuilder_->i64PtrType, JSFunctionBase::METHOD_OFFSET));
            Expr addrMethod = lmirBuilder_->Add(i64Ptr, func, offsetMethod);
            Expr method = lmirBuilder_->Iread(i64Ptr, addrMethod, lmirBuilder_->CreatePtrType(i64Ptr));
            // load callField from method
            Expr offsetCF = lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(i64Ptr, Method::CALL_FIELD_OFFSET));
            Expr addrCF = lmirBuilder_->Add(i64Ptr, method, offsetCF);
            Expr cf = lmirBuilder_->Iread(i64, addrCF, lmirBuilder_->CreatePtrType(i64));
            // cal new target offset from callField
            Expr offsetNVSB =
                lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(i64, MethodLiteral::NumVregsBits::START_BIT));
            Expr offsetNVSZ = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64, (1LLU << MethodLiteral::NumVregsBits::SIZE) - 1));
            Expr offsetHFSB =
                lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(i64, MethodLiteral::HaveFuncBit::START_BIT));
            Expr offsetHFSZ = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64, (1LLU << MethodLiteral::HaveFuncBit::SIZE) - 1));
            Expr const0 = lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(i64, 0));
            Expr numVregs = lmirBuilder_->Cvt(
                i64, i32, lmirBuilder_->And(i64, lmirBuilder_->LShr(i64, cf, offsetNVSB), offsetNVSZ));
            Expr haveFunc = lmirBuilder_->Cvt(
                i64, i32,
                lmirBuilder_->ICmp(i64, lmirBuilder_->And(i64, lmirBuilder_->LShr(i64, cf, offsetHFSB), offsetHFSZ),
                                   const0, IntCmpCondition::kNE));
            Expr size = lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(i64Ptr, JSTaggedValue::TaggedTypeSize()));
            Expr offsetNewTarget = lmirBuilder_->Mul(
                i64Ptr, lmirBuilder_->ZExt(i32, i64Ptr, lmirBuilder_->Add(i32, numVregs, haveFunc)), size);
            // load new target from sp
            Expr addrNewTarget = lmirBuilder_->Add(i64Ptr, sp, offsetNewTarget);
            Expr ldrNewTarget = lmirBuilder_->Iread(i64Ref, addrNewTarget, lmirBuilder_->CreatePtrType(i64Ptr));
            lmirBuilder_->AppendStmt(bb, lmirBuilder_->Regassign(ldrNewTarget, vreg));
            SaveGate2Expr(gate, lmirBuilder_->Regread(vreg));
            return;
        }
        case INIT_VRGE_THIS_OBJECT: {
            // init this
            PregIdx vreg = lmirBuilder_->CreatePreg(i64Ref);
            // load from frame
            Expr sp = lmirBuilder_->GenExprFromVar(
                lmirBuilder_->GetParam(lmirBuilder_->GetCurFunction(), 1));  // 1 : osr second param - sp
            Expr offsetFrame = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64Ptr, AsmInterpretedFrame::GetSize(!circuit_->IsArch64())));
            Expr frame = lmirBuilder_->Sub(i64Ptr, sp, offsetFrame);
            Expr offsetThis = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64Ptr, AsmInterpretedFrame::GetThisOffset(!circuit_->IsArch64())));
            Expr addrThis = lmirBuilder_->Add(i64Ptr, frame, offsetThis);
            Expr ldrThis = lmirBuilder_->Iread(i64Ref, addrThis, lmirBuilder_->CreatePtrType(i64Ptr));
            lmirBuilder_->AppendStmt(bb, lmirBuilder_->Regassign(ldrThis, vreg));
            SaveGate2Expr(gate, lmirBuilder_->Regread(vreg));
            return;
        }
        case INIT_VRGE_NUM_ARGS: {
            // init numargs
            SaveGate2Expr(gate, lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(i64, 0)));
            return;
        }
        case INIT_VRGE_ENV: {
            // init env
            PregIdx vreg = lmirBuilder_->CreatePreg(i64Ref);
            // load from frame
            Expr sp = lmirBuilder_->GenExprFromVar(
                lmirBuilder_->GetParam(lmirBuilder_->GetCurFunction(), 1));  // 1 : osr second param - sp
            Expr offsetFrame = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64Ptr, AsmInterpretedFrame::GetSize(!circuit_->IsArch64())));
            Expr frame = lmirBuilder_->Sub(i64Ptr, sp, offsetFrame);
            Expr offsetEnv = lmirBuilder_->ConstVal(
                lmirBuilder_->CreateIntConst(i64Ptr, AsmInterpretedFrame::GetEnvOffset(!circuit_->IsArch64())));
            Expr addrEnv = lmirBuilder_->Add(i64Ptr, frame, offsetEnv);
            Expr ldrEnv = lmirBuilder_->Iread(i64Ref, addrEnv, lmirBuilder_->CreatePtrType(i64Ptr));
            lmirBuilder_->AppendStmt(bb, lmirBuilder_->Regassign(ldrEnv, vreg));
            SaveGate2Expr(gate, lmirBuilder_->Regread(vreg));
            return;
        }
        default: {
            // init vregs
            PregIdx vreg = lmirBuilder_->CreatePreg(i64Ref);
            // load from sp
            Expr sp = lmirBuilder_->GenExprFromVar(
                lmirBuilder_->GetParam(lmirBuilder_->GetCurFunction(), 1));  // 1 : osr second param - sp
            Expr offset =
                lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(i64Ptr, vregNumber * sizeof(JSTaggedType)));
            Expr addr = lmirBuilder_->Add(i64Ptr, sp, offset);
            Expr ldrVreg = lmirBuilder_->Iread(i64Ref, addr, lmirBuilder_->CreatePtrType(i64Ptr));
            lmirBuilder_->AppendStmt(bb, lmirBuilder_->Regassign(ldrVreg, vreg));
            SaveGate2Expr(gate, lmirBuilder_->Regread(vreg));
            return;
        }
    }
}

void LiteCGIRBuilder::HandleFPTrunc(GateRef gate)
{
    VisitFPTrunc(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitFPTrunc(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(e1)) >=
           GetBitWidthFromMachineType(acc_.GetMachineType(gate)));
    Expr result = lmirBuilder_->Cvt(ConvertLiteCGTypeFromGate(e1), ConvertLiteCGTypeFromGate(gate), e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleFPExt(GateRef gate)
{
    VisitFPExt(gate, acc_.GetIn(gate, 0));
}

void LiteCGIRBuilder::VisitFPExt(GateRef gate, GateRef e1)
{
    Expr e1Value = GetExprFromGate(e1);
    ASSERT(GetBitWidthFromMachineType(acc_.GetMachineType(e1)) <=
           GetBitWidthFromMachineType(acc_.GetMachineType(gate)));
    Expr result = lmirBuilder_->Cvt(ConvertLiteCGTypeFromGate(e1), ConvertLiteCGTypeFromGate(gate), e1Value);
    SaveGate2Expr(gate, result);
}

void LiteCGIRBuilder::HandleExtractValue(GateRef gate)
{
    GateRef pointer = acc_.GetIn(gate, 0);
    GateRef index = acc_.GetIn(gate, 1);
    VisitExtractValue(gate, pointer, index);
}

void LiteCGIRBuilder::VisitExtractValue(GateRef gate, GateRef e1, GateRef e2)
{
    ASSERT((acc_.GetOpCode(e2) == OpCode::CONSTANT) && acc_.GetMachineType(e2) == MachineType::I32);
    uint32_t index = static_cast<uint32_t>(acc_.GetConstantValue(e2));
    Expr expr = GetExprFromGate(e1, index);
    PregIdx pregIdx = lmirBuilder_->GetPregIdxFromExpr(expr);
    SaveGate2Expr(gate, lmirBuilder_->Regread(pregIdx));
}

void LiteCGIRBuilder::HandleStore(GateRef gate)
{
    GateRef addr = acc_.GetValueIn(gate, 0);
    GateRef value = acc_.GetValueIn(gate, 1);
    VisitStore(gate, addr, value);
}

void LiteCGIRBuilder::VisitStore(GateRef gate, GateRef base, GateRef value)
{
    Expr baseAddr = GetDerivedrefExpr(base);
    Expr data = GetExprFromGate(value);

    LiteCGType *returnType = ConvertLiteCGTypeFromGate(value);
    LiteCGType *memType = (lmirBuilder_->IsHeapPointerType(baseAddr.GetType()))
                              ? lmirBuilder_->CreateRefType(returnType)
                              : lmirBuilder_->CreatePtrType(returnType);
    baseAddr = CanonicalizeToPtr(baseAddr, memType);

    Stmt &store = lmirBuilder_->Iassign(data, baseAddr, memType);
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), store);
}

void LiteCGIRBuilder::HandlePhi(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitPhi(gate, ins);
}

void LiteCGIRBuilder::AddPhiDesc(int bbID, PhiDesc &desc, std::map<int, std::vector<PhiDesc>> &bbID2Phis)
{
    auto it = bbID2Phis.find(bbID);
    if (it == bbID2Phis.end()) {
        std::vector<PhiDesc> vec;
        vec.push_back(std::move(desc));
        bbID2Phis.insert(std::make_pair(bbID, vec));
    } else {
        it->second.push_back(std::move(desc));
    }
}

LiteCGIRBuilder::DerivedStatus LiteCGIRBuilder::CheckDerivedPhi(GateRef gate, std::set<GateRef> &vis)
{
    // if the gate status is cached with derived or base, doesn't need to go forward
    if (derivedGateCache_.find(gate) != derivedGateCache_.end()) {
        if (derivedGateCache_[gate]) {
            return DerivedStatus::IS_DERIVED;
        } else {
            return DerivedStatus::IS_BASE;
        }
    }
    // for the visited gate in the dfs, if not cached, its status is unknow
    if (vis.find(gate) != vis.end()) {
        return DerivedStatus::UNKNOW;
    }
    // cached gate doesn't need insert to visited set
    vis.insert(gate);
    DerivedStatus derivedStatus = DerivedStatus::IS_BASE;
    std::vector<GateRef> phiIns;
    acc_.GetIns(gate, phiIns);
    std::vector<GateRef> phiStates;
    acc_.GetIns(phiIns[0], phiStates);
    ASSERT(phiStates.size() + 1 == phiIns.size());
    for (int i = 1; i < static_cast<int>(phiIns.size()); i++) {
        auto op = acc_.GetOpCode(phiIns[i]);
        if (op == OpCode::ADD) {
            derivedStatus = DerivedStatus::IS_DERIVED;
            break;
        } else if (op == OpCode::VALUE_SELECTOR) {
            DerivedStatus status = CheckDerivedPhi(phiIns[i], vis);
            if (status == DerivedStatus::IS_DERIVED) {
                derivedStatus = DerivedStatus::IS_DERIVED;
                break;
            }
            if (status == DerivedStatus::UNKNOW) {
                derivedStatus = DerivedStatus::UNKNOW;
            }
        }
    }
    if (derivedStatus == DerivedStatus::IS_DERIVED) {
        derivedGateCache_[gate] = true;
    } else if (derivedStatus == DerivedStatus::IS_BASE) {
        derivedGateCache_[gate] = false;
    }

    return derivedStatus;
}

void LiteCGIRBuilder::FindBaseRefForPhi(GateRef gate, const std::vector<GateRef> &phiIns)
{
    int curBBId = instID2bbID_[acc_.GetId(gate)];
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    PregIdx basePregIdx = 0;
    bool isDerived = false;
    std::set<GateRef> baseIns;
    std::vector<PhiDesc> phiDescs;
    std::vector<GateRef> phiStates;
    acc_.GetIns(phiIns[0], phiStates);
    ASSERT(phiStates.size() + 1 == phiIns.size());
    for (int i = 1; i < static_cast<int>(phiIns.size()); i++) {
        int preBBId = LookupPredBB(phiStates[i - 1], curBBId);
        if (bbID2BB_.count(preBBId) != 0) {
            BB *preBB = bbID2BB_[preBBId];
            if (preBB == nullptr) {
                OPTIONAL_LOG_COMPILER(ERROR) << "FindBaseRef failed BasicBlock nullptr";
                return;
            }
        }
        auto op = acc_.GetOpCode(phiIns[i]);
        if (op == OpCode::ADD) {
            auto g0 = acc_.GetIn(phiIns[i], 0);
            baseIns.insert(g0);
            PhiDesc desc = {preBBId, g0, basePregIdx};
            phiDescs.push_back(desc);
            isDerived = true;
            ASSERT(ConvertLiteCGTypeFromGate(g0) == lmirBuilder_->i64RefType);
        } else if (op == OpCode::VALUE_SELECTOR) {
            std::set<GateRef> vis;
            if (CheckDerivedPhi(phiIns[i], vis) == DerivedStatus::IS_DERIVED) {
                isDerived = true;
            }
            baseIns.insert(phiIns[i]);
            PhiDesc desc = {preBBId, phiIns[i], basePregIdx};
            phiDescs.push_back(desc);
            ASSERT(ConvertLiteCGTypeFromGate(phiIns[i]) == lmirBuilder_->i64RefType);
        } else {
            baseIns.insert(phiIns[i]);
            PhiDesc desc = {preBBId, phiIns[i], basePregIdx};
            phiDescs.push_back(desc);
            ASSERT(ConvertLiteCGTypeFromGate(phiIns[i]) == lmirBuilder_->i64RefType);
        }
    }

    // use to catch the situation that the phi is derived
    if (isDerived) {
        LOG_COMPILER(FATAL) << "catch derived case!" << phiDescs.size() << std::endl;
        UNREACHABLE();
    }

    derivedGateCache_[gate] = isDerived;

    if (!isDerived) {
        return;
    }

    if (baseIns.size() == 1) {
        // only one base gate for the derived phi reference, doesn't need to insert a new phi
        derivedGate2BaseGate_[gate] = *baseIns.begin();
    } else {
        basePregIdx = lmirBuilder_->CreatePreg(type);
        derivedPhiGate2BasePhiPreg_[gate] = basePregIdx;
        for (PhiDesc desc : phiDescs) {
            desc.phi = basePregIdx;
            AddPhiDesc(curBBId, desc, bbID2basePhis_);
        }
    }
}

void LiteCGIRBuilder::VisitPhi(GateRef gate, const std::vector<GateRef> &phiIns)
{
    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    PregIdx phiPregIdx = lmirBuilder_->CreatePreg(type);

    if (phiIns.size() > 1) {
        SaveGate2Expr(gate, lmirBuilder_->Regread(phiPregIdx));
    }
    // Collect the states merges of this phi and note the 1-in is the merged states.
    std::vector<GateRef> phiStates;
    acc_.GetIns(phiIns[0], phiStates);
    ASSERT(phiStates.size() + 1 == phiIns.size());
    int curBBId = instID2bbID_[acc_.GetId(gate)];
    for (int i = 1; i < static_cast<int>(phiIns.size()); i++) {
        int preBBId = LookupPredBB(phiStates[i - 1], curBBId);
        // if bbID2BB_.count(preBBId) = 0 means bb with current bbIdx hasn't been created
        if (bbID2BB_.count(preBBId) != 0) {
            BB *preBB = bbID2BB_[preBBId];
            if (preBB == nullptr) {
                OPTIONAL_LOG_COMPILER(ERROR) << "VisitPhi failed BasicBlock nullptr";
                return;
            }
            PhiDesc desc = {preBBId, phiIns[i], phiPregIdx};
            AddPhiDesc(curBBId, desc, bbID2unmergedPhis_);
        } else {
            PhiDesc desc = {preBBId, phiIns[i], phiPregIdx};
            AddPhiDesc(curBBId, desc, bbID2unmergedPhis_);
        }
    }

    if (type == lmirBuilder_->i64RefType) {
        FindBaseRefForPhi(gate, phiIns);
    }
}

void LiteCGIRBuilder::HandleSwitch(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    VisitSwitch(gate, ins[1], outs);
}

void LiteCGIRBuilder::VisitSwitch(GateRef gate, GateRef input, const std::vector<GateRef> &outList)
{
    Expr cond = GetExprFromGate(input);
    int caseNum = static_cast<int>(outList.size());
    BB *defaultOutBB = nullptr;
    for (int i = 0; i < caseNum; i++) {
        if (acc_.GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            defaultOutBB = &GetOrCreateBB(instID2bbID_[acc_.GetId(outList[i])]);
        }
    }

    LiteCGType *type = ConvertLiteCGTypeFromGate(gate);
    CHECK_NULL_FATAL(defaultOutBB);
    SwitchBuilder builder = lmirBuilder_->Switch(type, cond, *defaultOutBB);
    for (int i = 0; i < caseNum; i++) {
        if (acc_.GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            continue;
        }
        BB &curOutBB = GetOrCreateBB(instID2bbID_[acc_.GetId(outList[i])]);
        builder.Case(acc_.TryGetValue(outList[i]), curOutBB);
    }
    Stmt &switchStmt = builder.Done();
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), switchStmt);
    lmirBuilder_->AppendBB(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]));
}

void LiteCGIRBuilder::HandleDeoptCheck(GateRef gate)
{
    VisitDeoptCheck(gate);
}

LiteCGType *LiteCGIRBuilder::GetExperimentalDeoptTy()
{
    std::vector<LiteCGType *> paramTys = {lmirBuilder_->i64Type, lmirBuilder_->i64RefType, lmirBuilder_->i64RefType};
    LiteCGType *functionType = lmirBuilder_->CreateFuncType(paramTys, lmirBuilder_->i64RefType, false);
    return functionType;
}

void LiteCGIRBuilder::SaveFrameTypeOnFrame(BB &bb, FrameType frameType)
{
    Expr llvmFpAddr = CallingFp(false);
    Expr frameAddr = lmirBuilder_->Cvt(llvmFpAddr.GetType(), slotType_, llvmFpAddr);
    Expr frameTypeSlotAddr = lmirBuilder_->Sub(
        slotType_, frameAddr, lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(slotType_, slotSize_)));
    LiteCGType *slotTypePtr = lmirBuilder_->CreatePtrType(slotType_);
    Expr addr = lmirBuilder_->Cvt(frameTypeSlotAddr.GetType(), slotTypePtr, frameTypeSlotAddr);
    Expr llvmFrameType =
        lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(slotType_, static_cast<uintptr_t>(frameType)));
    Stmt &stmt = lmirBuilder_->Iassign(llvmFrameType, addr, slotTypePtr);
    lmirBuilder_->AppendStmt(bb, stmt);
}

void LiteCGIRBuilder::GenDeoptEntry(std::string funcName)
{
    BB &bb = CreateBB();
    auto reservedSlotsSize = OptimizedFrame::ComputeReservedSize(slotSize_);
    lmirBuilder_->SetFuncFrameResverdSlot(reservedSlotsSize);
    SaveFrameTypeOnFrame(bb, FrameType::OPTIMIZED_FRAME);
    Function &func = lmirBuilder_->GetCurFunction();
    lmirModule_->SetFunction(LMIRModule::kDeoptEntryOffset, funcName, false);

    Expr glue = lmirBuilder_->GenExprFromVar(lmirBuilder_->GetParam(func, 0));          // 0: glue
    Expr deoptType = lmirBuilder_->GenExprFromVar(lmirBuilder_->GetParam(func, 1));     // 1: deopt_type
    Expr depth = lmirBuilder_->GenExprFromVar(lmirBuilder_->GetParam(func, 2));         // 2: depth

    StubIdType stubId = RTSTUB_ID(DeoptHandlerAsm);
    int stubIndex = static_cast<int>(std::get<RuntimeStubCSigns::ID>(stubId));
    Expr rtoffset = lmirBuilder_->Add(glue.GetType(), glue, GetRTStubOffset(glue, stubIndex));
    Expr patchAddr = lmirBuilder_->Cvt(glue.GetType(), lmirBuilder_->i64PtrType, rtoffset);
    Expr funcAddr = lmirBuilder_->Iread(rtoffset.GetType(), patchAddr, lmirBuilder_->i64PtrType);

    LiteCGType *funcType = GetExperimentalDeoptTy();
    LiteCGType *funcTypePtr = lmirBuilder_->CreatePtrType(funcType);
    LiteCGType *funcTypePtrPtr = lmirBuilder_->CreatePtrType(funcTypePtr);
    Expr callee = lmirBuilder_->Cvt(glue.GetType(), funcTypePtrPtr, funcAddr);
    PregIdx funcPregIdx = lmirBuilder_->CreatePreg(funcTypePtrPtr);
    lmirBuilder_->AppendStmt(bb, lmirBuilder_->Regassign(callee, funcPregIdx));

    LiteCGType *returnType = lmirBuilder_->LiteCGGetFuncReturnType(funcType);
    PregIdx pregIdx = lmirBuilder_->CreatePreg(returnType);
    maple::litecg::Args params = {glue, deoptType, depth};
    Stmt &callNode = lmirBuilder_->ICall(lmirBuilder_->Regread(funcPregIdx), params, pregIdx);
    lmirBuilder_->AppendStmt(bb, callNode);
    lmirBuilder_->AppendStmt(bb, lmirBuilder_->Return(lmirBuilder_->Regread(pregIdx)));
    lmirBuilder_->AppendBB(bb);
}

Function *LiteCGIRBuilder::GetExperimentalDeopt()
{
    /* 0:calling 1:its caller */
    std::string funcName = "litecg.experimental.deoptimize.p1i64";
    auto fn = lmirBuilder_->GetFunc(funcName);
    if (!fn) {
        // save previous func for restore env
        Function &preFunc = lmirBuilder_->GetCurFunction();
        auto fnTy = GetExperimentalDeoptTy();
        FunctionBuilder funcBuilder = lmirBuilder_->DefineFunction(funcName);
        // glue type depth
        funcBuilder.Param(lmirBuilder_->i64Type, "glue")
            .Param(lmirBuilder_->i64RefType, "deopt_type")
            .Param(lmirBuilder_->i64RefType, "max_depth");
        Function &curFunc = funcBuilder.Return(lmirBuilder_->LiteCGGetFuncReturnType(fnTy)).Done();
        funcBuilder.CallConvAttribute(maple::litecg::CCall);
        lmirBuilder_->SetCurFunc(curFunc);
        lmirBuilder_->RenameFormal2Preg(curFunc);
        GenDeoptEntry(funcName);
        fn = &curFunc;

        lmirBuilder_->SetCurFunc(preFunc);
    }
    return fn;
}

Expr LiteCGIRBuilder::ConvertToTagged(GateRef gate)
{
    auto machineType = acc_.GetMachineType(gate);
    switch (machineType) {
        case MachineType::I1:
            return ConvertBoolToTaggedBoolean(gate);
        case MachineType::I32:
            return ConvertInt32ToTaggedInt(GetExprFromGate(gate));
        case MachineType::F64:
            return ConvertFloat64ToTaggedDouble(gate);
        case MachineType::I64:
            break;
        default:
            LOG_COMPILER(FATAL) << "unexpected machineType!";
            UNREACHABLE();
            break;
    }
    return GetExprFromGate(gate);
}

Expr LiteCGIRBuilder::ConvertInt32ToTaggedInt(Expr value)
{
    Expr e1Value = lmirBuilder_->SExt(value.GetType(), lmirBuilder_->i64Type, value);
    Expr tagMask = lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->i64Type, JSTaggedValue::TAG_INT));
    Expr result = lmirBuilder_->Or(lmirBuilder_->i64Type, e1Value, tagMask);
    return lmirBuilder_->Cvt(lmirBuilder_->i64Type, lmirBuilder_->i64RefType, result);
}

Expr LiteCGIRBuilder::ConvertBoolToTaggedBoolean(GateRef gate)
{
    Expr value = GetExprFromGate(gate);
    Expr e1Value = lmirBuilder_->ZExt(value.GetType(), lmirBuilder_->u64Type, value);
    Expr tagMask =
        lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->i64Type, JSTaggedValue::TAG_BOOLEAN_MASK));
    Expr result = lmirBuilder_->Or(lmirBuilder_->u64Type, e1Value, tagMask);
    return lmirBuilder_->Cvt(lmirBuilder_->u64Type, lmirBuilder_->i64RefType, result);
}

Expr LiteCGIRBuilder::ConvertFloat64ToTaggedDouble(GateRef gate)
{
    Expr value = GetExprFromGate(gate);
    Expr e1Value = lmirBuilder_->BitCast(value.GetType(), lmirBuilder_->i64Type, value);
    Expr offset = lmirBuilder_->ConstVal(
        lmirBuilder_->CreateIntConst(lmirBuilder_->i64Type, JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    Expr result = lmirBuilder_->Add(lmirBuilder_->i64Type, e1Value, offset);
    return lmirBuilder_->Cvt(lmirBuilder_->i64Type, lmirBuilder_->i64RefType, result);
}

void LiteCGIRBuilder::SaveDeoptVregInfo(std::unordered_map<int, LiteCGValue> &deoptBundleInfo, BB &bb, int32_t index,
                                        size_t curDepth, size_t shift, GateRef gate)
{
    int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(index, curDepth, shift);
    auto deoptInfo = ConstantFoldExpr(ConvertToTagged(gate), bb);
    deoptBundleInfo.insert(std::pair<int, LiteCGValue>(encodeIndex, deoptInfo));
}

void LiteCGIRBuilder::SaveDeoptVregInfoWithI64(std::unordered_map<int, LiteCGValue> &deoptBundleInfo, BB &bb,
                                               int32_t index, size_t curDepth, size_t shift, GateRef gate)
{
    int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(index, curDepth, shift);
    Expr expr = GetExprFromGate(gate);
    Expr value = ConvertInt32ToTaggedInt(lmirBuilder_->Cvt(expr.GetType(), lmirBuilder_->i32Type, expr));
    auto deoptInfo = ConstantFoldExpr(value, bb);
    deoptBundleInfo.insert(std::pair<int, LiteCGValue>(encodeIndex, deoptInfo));
}

void LiteCGIRBuilder::VisitDeoptCheck(GateRef gate)
{
    auto deoptType = static_cast<uint32_t>(acc_.GetConstantValue(acc_.GetValueIn(gate, 2)));    // 2: deopt type
    auto &deoptBBInfo = GetOrCreateDeoptBBInfo(gate);
    auto getOrCreateDeoptFalseBB = [this, &deoptBBInfo, deoptType]() -> BB& {
        auto iter = deoptBBInfo.deoptType2BB.find(deoptType);
        if (iter != deoptBBInfo.deoptType2BB.end()) {
            return *iter->second;
        }
        BB &falseBB = lmirBuilder_->CreateBB();
        Expr constDeoptType = ConvertInt32ToTaggedInt(
            lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->u32Type, deoptType)));
        lmirBuilder_->AppendStmt(falseBB, lmirBuilder_->Regassign(constDeoptType, deoptBBInfo.deoptTypePreg));
        lmirBuilder_->AppendStmt(falseBB, lmirBuilder_->Goto(*deoptBBInfo.deoptBB));
        // deopt branch is not expected to be token as often,
        // just put them to the end of the function
        lmirBuilder_->AppendToLast(falseBB);

        deoptBBInfo.deoptType2BB.emplace(deoptType, &falseBB);
        return falseBB;
    };

    int block = instID2bbID_[acc_.GetId(gate)];
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    int bbOut = instID2bbID_[acc_.GetId(outs[0])];  // 0: output

    BB &trueBB = GetOrCreateBB(bbOut);
    BB &falseBB = getOrCreateDeoptFalseBB();
    GateRef cmp = acc_.GetValueIn(gate, 0);  // 0: cond
    Expr cond = GetExprFromGate(cmp);
    BB &curBB = GetOrCreateBB(block);
    lmirBuilder_->AppendStmt(curBB, lmirBuilder_->CondGoto(cond, falseBB, false));
    lmirBuilder_->AppendStmt(curBB, lmirBuilder_->Goto(trueBB));
    lmirBuilder_->AppendBB(curBB);
}

LiteCGIRBuilder::DeoptBBInfo &LiteCGIRBuilder::GetOrCreateDeoptBBInfo(GateRef gate)
{
    GateRef deoptFrameState = acc_.GetValueIn(gate, 1);  // 1: frame state
    ASSERT(acc_.GetOpCode(deoptFrameState) == OpCode::FRAME_STATE);
    auto iter = deoptFrameState2BB_.find(deoptFrameState);
    if (iter != deoptFrameState2BB_.end()) {
        return iter->second;
    }
    BB &bb = lmirBuilder_->CreateBB();  // deoptBB
    Expr glue = GetExprFromGate(acc_.GetGlueFromArgList());

    std::vector<Expr> params;
    params.push_back(glue);                        // glue
    auto deoptTypePreg = lmirBuilder_->CreatePreg(lmirBuilder_->i64RefType);
    params.push_back(lmirBuilder_->Regread(deoptTypePreg));  // deoptType

    std::unordered_map<int, LiteCGValue> deoptBundleInfo;
    size_t maxDepth = 0;
    GateRef frameState = acc_.GetFrameState(deoptFrameState);
    while ((acc_.GetOpCode(frameState) == OpCode::FRAME_STATE)) {
        maxDepth++;
        frameState = acc_.GetFrameState(frameState);
    }
    Expr constMaxDepth =
        lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->u32Type, static_cast<uint32_t>(maxDepth)));
    params.push_back(ConvertInt32ToTaggedInt(constMaxDepth));
    size_t shift = Deoptimizier::ComputeShift(maxDepth);
    frameState = deoptFrameState;
    ArgumentAccessor argAcc(const_cast<Circuit *>(circuit_));
    for (int32_t curDepth = static_cast<int32_t>(maxDepth); curDepth >= 0; curDepth--) {
        ASSERT(acc_.GetOpCode(frameState) == OpCode::FRAME_STATE);
        GateRef frameValues = acc_.GetValueIn(frameState, 1);  // 1: frame values
        const size_t numValueIn = acc_.GetNumValueIn(frameValues);
        const size_t envIndex = numValueIn - 2;  // 2: env valueIn index
        CHECK_FATAL(numValueIn > 0, "must not be zero");
        const size_t accIndex = numValueIn - 1;  // 1: acc valueIn index
        GateRef env = acc_.GetValueIn(frameValues, envIndex);
        GateRef acc = acc_.GetValueIn(frameValues, accIndex);
        auto pc = acc_.TryGetPcOffset(frameState);
        GateRef jsFunc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::FUNC);
        GateRef newTarget = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::NEW_TARGET);
        GateRef thisObj = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::THIS_OBJECT);
        GateRef actualArgc = argAcc.GetFrameArgsIn(frameState, FrameArgIdx::ACTUAL_ARGC);
        // vreg
        for (size_t i = 0; i < envIndex; i++) {
            GateRef vregValue = acc_.GetValueIn(frameValues, i);
            if (acc_.IsConstantTaggedValue(vregValue, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
                continue;
            }
            SaveDeoptVregInfo(deoptBundleInfo, bb, i, curDepth, shift, vregValue);
        }
        // env
        if (!acc_.IsConstantTaggedValue(env, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
            int32_t specEnvVregIndex = static_cast<int32_t>(SpecVregIndex::ENV_INDEX);
            SaveDeoptVregInfo(deoptBundleInfo, bb, specEnvVregIndex, curDepth, shift, env);
        }
        // acc
        if (!acc_.IsConstantTaggedValue(acc, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
            int32_t specAccVregIndex = static_cast<int32_t>(SpecVregIndex::ACC_INDEX);
            SaveDeoptVregInfo(deoptBundleInfo, bb, specAccVregIndex, curDepth, shift, acc);
        }
        // pc offset
        int32_t specPcOffsetIndex = static_cast<int32_t>(SpecVregIndex::PC_OFFSET_INDEX);
        int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(specPcOffsetIndex, curDepth, shift);
        Const &pcConst = lmirBuilder_->CreateIntConst(lmirBuilder_->u32Type, pc);
        deoptBundleInfo.insert(std::pair<int, LiteCGValue>(encodeIndex, {LiteCGValueKind::kConstKind, &pcConst}));

        // func
        int32_t specCallTargetIndex = static_cast<int32_t>(SpecVregIndex::FUNC_INDEX);
        SaveDeoptVregInfo(deoptBundleInfo, bb, specCallTargetIndex, curDepth, shift, jsFunc);
        // newTarget
        int32_t specNewTargetIndex = static_cast<int32_t>(SpecVregIndex::NEWTARGET_INDEX);
        SaveDeoptVregInfo(deoptBundleInfo, bb, specNewTargetIndex, curDepth, shift, newTarget);
        // this object
        int32_t specThisIndex = static_cast<int32_t>(SpecVregIndex::THIS_OBJECT_INDEX);
        SaveDeoptVregInfo(deoptBundleInfo, bb, specThisIndex, curDepth, shift, thisObj);
        int32_t specArgcIndex = static_cast<int32_t>(SpecVregIndex::ACTUAL_ARGC_INDEX);
        SaveDeoptVregInfoWithI64(deoptBundleInfo, bb, specArgcIndex, curDepth, shift, actualArgc);
        frameState = acc_.GetFrameState(frameState);
    }
    Function *callee = GetExperimentalDeopt();
    LiteCGType *funcType = GetExperimentalDeoptTy();
    LiteCGType *returnType = lmirBuilder_->LiteCGGetFuncReturnType(funcType);
    PregIdx returnPregIdx = lmirBuilder_->CreatePreg(returnType);
    Stmt &callNode = lmirBuilder_->Call(*callee, params, returnPregIdx);
    lmirBuilder_->SetCallStmtDeoptBundleInfo(callNode, deoptBundleInfo);
    lmirBuilder_->AppendStmt(bb, callNode);
    lmirBuilder_->AppendStmt(bb, lmirBuilder_->Return(lmirBuilder_->Regread(returnPregIdx)));
    lmirBuilder_->AppendToLast(bb);

    deoptFrameState2BB_.emplace(deoptFrameState, DeoptBBInfo(&bb, deoptTypePreg));
    return deoptFrameState2BB_.at(deoptFrameState);
}

int64_t LiteCGIRBuilder::GetBitWidthFromMachineType(MachineType machineType) const
{
    switch (machineType) {
        case NOVALUE:
            return 0;
        case ARCH:
            return 48;  // 48: Pointer representation in different architectures
        case I1:
            return 1;
        case I8:
            return 8;  // 8: bit width
        case I16:
            return 16;  // 16: bit width
        case I32:
            return 32;  // 32: bit width
        case I64:
            return 64;  // 64: bit width
        case F32:
            return 32;  // 32: bit width
        case F64:
            return 64;  // 64: bit width
        case FLEX:
        case ANYVALUE:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
}

int LiteCGIRBuilder::LookupPredBB(GateRef start, int bbID)
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

}  // namespace panda::ecmascript::kungfu
