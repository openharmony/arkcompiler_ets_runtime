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

#include "ecmascript/compiler/litecg_ir_builder.h"

#include <cmath>
#include <cstdint>

#include "ecmascript/compiler/argument_accessor.h"
#include "ecmascript/compiler/bc_call_signature.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/debug_info.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/method.h"
#include "lmir_builder.h"

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
      acc_(circuit)
{
    lmirBuilder_ = new LMIRBuilder(*module->GetModule());
    ASSERT(compCfg_->Is64Bit());
    slotSize_ = sizeof(uint64_t);
    slotType_ = lmirBuilder_->i64Type;
    InitializeHandlers();
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
    if (!methodLiteral_->IsFastCall()) {
        funcBuilder.Param(lmirBuilder_->i64Type, "actualArgc")
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
        return value.pregIdx;
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
        if (acc_.GetOpCode(it.second) == OpCode::CONSTANT) {
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

void LiteCGIRBuilder::Build()
{
    BuildInstID2BBIDMap();
    AddFunc();
    LOG_COMPILER(INFO) << "============== building litecg ir=======" << std::endl;

    std::unordered_set<OpCode> usedOpcodeSet;
    for (size_t bbIdx = 0; bbIdx < scheduledGates_->size(); bbIdx++) {
        const std::vector<GateRef> &bb = scheduledGates_->at(bbIdx);

        for (size_t instIdx = bb.size(); instIdx > 0; instIdx--) {
            GateRef gate = bb[instIdx - 1];
            auto found = opHandlers_.find(acc_.GetOpCode(gate));
            if (found != opHandlers_.end()) {
                (this->*(found->second))(gate);
                InsertUsedOpcodeSet(usedOpcodeSet, found->first);
                continue;
            }
            if (illegalOpHandlers_.find(acc_.GetOpCode(gate)) == illegalOpHandlers_.end()) {
                LOG_COMPILER(FATAL) << "can't process opcode: " << acc_.GetOpCode(gate) << std::endl;
            }
        }
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

void LiteCGIRBuilder::GenPrologue(maple::litecg::Function &function)
{
    auto frameType = circuit_->GetFrameType();
    if (IsInterpreted()) {
        return;
    }
    lmirBuilder_->SetFuncFramePointer("all");
    size_t reservedSlotsSize = 0;
    if (frameType == FrameType::OPTIMIZED_FRAME) {
        reservedSlotsSize = OptimizedFrame::ComputeReservedSize(slotSize_);
        lmirBuilder_->SetFuncFrameResverdSlot(reservedSlotsSize);
        SaveFrameTypeOnFrame(frameType);
    } else if (frameType == FrameType::OPTIMIZED_JS_FUNCTION_FRAME) {
        reservedSlotsSize = OptimizedJSFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
        lmirBuilder_->SetFuncFrameResverdSlot(reservedSlotsSize);
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
                SaveJSFuncOnOptJSFuncFrame(value);
                SaveFrameTypeOnFrame(frameType);
            }
        }
    } else {
        LOG_COMPILER(FATAL) << "frameType interpret type error !";
        ASSERT_PRINT(static_cast<uintptr_t>(frameType), "is not support !");
    }
}

void LiteCGIRBuilder::SaveJSFuncOnOptJSFuncFrame(maple::litecg::Var &value)
{
    ASSERT(circuit_->GetFrameType() == FrameType::OPTIMIZED_JS_FUNCTION_FRAME);
    Expr fpAddr = CallingFp(false);
    Expr frameAddr = lmirBuilder_->Cvt(fpAddr.GetType(), lmirBuilder_->i64Type, fpAddr);
    size_t reservedOffset = OptimizedJSFunctionFrame::ComputeReservedJSFuncOffset(slotSize_);
    Expr frameJSFuncSlotAddr =
        lmirBuilder_->Sub(frameAddr.GetType(), frameAddr,
                          lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(slotType_, reservedOffset)));
    Expr jsFuncAddr =
        lmirBuilder_->Cvt(frameJSFuncSlotAddr.GetType(), lmirBuilder_->CreatePtrType(slotType_), frameJSFuncSlotAddr);
    Expr jsFuncValue = lmirBuilder_->Cvt(lmirBuilder_->i64PtrType, slotType_, lmirBuilder_->GenExprFromVar(value));
    auto &stmt = lmirBuilder_->Iassign(jsFuncValue, jsFuncAddr, jsFuncAddr.GetType());
    lmirBuilder_->AppendStmt(GetFirstBB(), stmt);
}

void LiteCGIRBuilder::SaveFrameTypeOnFrame(FrameType frameType)
{
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

void LiteCGIRBuilder::SaveGate2Expr(GateRef gate, Expr expr)
{
    if (expr.IsDread()) {
        LiteCGValue value;
        value.kind = LiteCGValueKind::kSymbolKind;
        value.symbol = lmirBuilder_->GetLocalVarFromExpr(expr);
        gate2Expr_[gate] = value;
        return;
    }
    if (expr.IsRegread()) {
        LiteCGValue value;
        value.kind = LiteCGValueKind::kPregKind;
        value.pregIdx = lmirBuilder_->GetPregIdxFromExpr(expr);
        gate2Expr_[gate] = value;
        return;
    }
    // check expr is not agg
    BB &curBB = GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]);
    PregIdx pregIdx = lmirBuilder_->CreatePreg(expr.GetType());
    lmirBuilder_->AppendStmt(curBB, lmirBuilder_->Regassign(expr, pregIdx));
    LiteCGValue value;
    value.kind = LiteCGValueKind::kPregKind;
    value.pregIdx = pregIdx;
    gate2Expr_[gate] = value;
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

Expr LiteCGIRBuilder::GetExprFromGate(GateRef gate)
{
    if (acc_.GetOpCode(gate) == OpCode::CONSTANT) {
        return GetConstant(gate);
    }
    LiteCGValue value = gate2Expr_[gate];
    if (value.kind == LiteCGValueKind::kSymbolKind) {
        return lmirBuilder_->Dread(*value.symbol);
    }
    ASSERT(value.kind == LiteCGValueKind::kPregKind);
    return lmirBuilder_->Regread(value.pregIdx);
}

void LiteCGIRBuilder::InitializeHandlers()
{
    opHandlers_ = {
        {OpCode::STATE_ENTRY, &LiteCGIRBuilder::HandleGoto},
        {OpCode::RETURN, &LiteCGIRBuilder::HandleReturn},
        {OpCode::RETURN_VOID, &LiteCGIRBuilder::HandleReturnVoid},
        {OpCode::IF_BRANCH, &LiteCGIRBuilder::HandleBranch},
        {OpCode::ORDINARY_BLOCK, &LiteCGIRBuilder::HandleGoto},
        {OpCode::IF_TRUE, &LiteCGIRBuilder::HandleGoto},
        {OpCode::IF_FALSE, &LiteCGIRBuilder::HandleGoto},
        {OpCode::SWITCH_CASE, &LiteCGIRBuilder::HandleGoto},
        {OpCode::MERGE, &LiteCGIRBuilder::HandleGoto},
        {OpCode::DEFAULT_CASE, &LiteCGIRBuilder::HandleGoto},
        {OpCode::LOOP_BEGIN, &LiteCGIRBuilder::HandleGoto},
        {OpCode::LOOP_BACK, &LiteCGIRBuilder::HandleGoto},
        {OpCode::VALUE_SELECTOR, &LiteCGIRBuilder::HandlePhi},
        {OpCode::RUNTIME_CALL, &LiteCGIRBuilder::HandleRuntimeCall},
        {OpCode::RUNTIME_CALL_WITH_ARGV, &LiteCGIRBuilder::HandleRuntimeCallWithArgv},
        {OpCode::NOGC_RUNTIME_CALL, &LiteCGIRBuilder::HandleCall},
        {OpCode::CALL_OPTIMIZED, &LiteCGIRBuilder::HandleCall},
        {OpCode::FAST_CALL_OPTIMIZED, &LiteCGIRBuilder::HandleCall},
        {OpCode::CALL, &LiteCGIRBuilder::HandleCall},
        {OpCode::BUILTINS_CALL, &LiteCGIRBuilder::HandleCall},
        {OpCode::BUILTINS_CALL_WITH_ARGV, &LiteCGIRBuilder::HandleCall},
        {OpCode::ARG, &LiteCGIRBuilder::HandleParameter},
        {OpCode::CONSTANT, &LiteCGIRBuilder::HandleConstant},
        {OpCode::ZEXT, &LiteCGIRBuilder::HandleZExtInt},
        {OpCode::SEXT, &LiteCGIRBuilder::HandleSExtInt},
        {OpCode::TRUNC, &LiteCGIRBuilder::HandleCastIntXToIntY},
        {OpCode::FEXT, &LiteCGIRBuilder::HandleFPExt},
        {OpCode::FTRUNC, &LiteCGIRBuilder::HandleFPTrunc},
        {OpCode::REV, &LiteCGIRBuilder::HandleIntRev},
        {OpCode::ADD, &LiteCGIRBuilder::HandleAdd},
        {OpCode::SUB, &LiteCGIRBuilder::HandleSub},
        {OpCode::MUL, &LiteCGIRBuilder::HandleMul},
        {OpCode::FDIV, &LiteCGIRBuilder::HandleFloatDiv},
        {OpCode::SDIV, &LiteCGIRBuilder::HandleIntDiv},
        {OpCode::UDIV, &LiteCGIRBuilder::HandleUDiv},
        {OpCode::AND, &LiteCGIRBuilder::HandleIntAnd},
        {OpCode::OR, &LiteCGIRBuilder::HandleIntOr},
        {OpCode::XOR, &LiteCGIRBuilder::HandleIntXor},
        {OpCode::LSR, &LiteCGIRBuilder::HandleIntLsr},
        {OpCode::ASR, &LiteCGIRBuilder::HandleIntAsr},
        {OpCode::ICMP, &LiteCGIRBuilder::HandleCmp},
        {OpCode::FCMP, &LiteCGIRBuilder::HandleCmp},
        {OpCode::LOAD, &LiteCGIRBuilder::HandleLoad},
        {OpCode::STORE, &LiteCGIRBuilder::HandleStore},
        {OpCode::SIGNED_INT_TO_FLOAT, &LiteCGIRBuilder::HandleChangeInt32ToDouble},
        {OpCode::UNSIGNED_INT_TO_FLOAT, &LiteCGIRBuilder::HandleChangeUInt32ToDouble},
        {OpCode::FLOAT_TO_SIGNED_INT, &LiteCGIRBuilder::HandleChangeDoubleToInt32},
        {OpCode::TAGGED_TO_INT64, &LiteCGIRBuilder::HandleChangeTaggedPointerToInt64},
        {OpCode::INT64_TO_TAGGED, &LiteCGIRBuilder::HandleChangeInt64ToTagged},
        {OpCode::BITCAST, &LiteCGIRBuilder::HandleBitCast},
        {OpCode::LSL, &LiteCGIRBuilder::HandleIntLsl},
        {OpCode::SMOD, &LiteCGIRBuilder::HandleMod},
        {OpCode::FMOD, &LiteCGIRBuilder::HandleMod},
        {OpCode::DEOPT_CHECK, &LiteCGIRBuilder::HandleDeoptCheck},
        {OpCode::TRUNC_FLOAT_TO_INT64, &LiteCGIRBuilder::HandleTruncFloatToInt},
        {OpCode::TRUNC_FLOAT_TO_INT32, &LiteCGIRBuilder::HandleTruncFloatToInt},
        {OpCode::ADD_WITH_OVERFLOW, &LiteCGIRBuilder::HandleAddWithOverflow},
        {OpCode::SUB_WITH_OVERFLOW, &LiteCGIRBuilder::HandleSubWithOverflow},
        {OpCode::MUL_WITH_OVERFLOW, &LiteCGIRBuilder::HandleMulWithOverflow},
        {OpCode::EXTRACT_VALUE, &LiteCGIRBuilder::HandleExtractValue},
        {OpCode::SQRT, &LiteCGIRBuilder::HandleSqrt},
        {OpCode::READSP, &LiteCGIRBuilder::HandleReadSp},
        {OpCode::FINISH_ALLOCATE, &LiteCGIRBuilder::HandleFinishAllocate},
    };
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

void LiteCGIRBuilder::VisitAdd(GateRef gate, GateRef e1, GateRef e2)
{
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);

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
            SaveGate2Expr(gate, result);
            // set the base reference of derived reference
            if (e1Type == lmirBuilder_->i64RefType) {
                ASSERT(!e1Value.IsDread());
                derivedGate2BaseGate_[gate] = e1;
            }
            return;
        } else {
            Expr tmp1Expr = (e1Type == returnType) ? e1Value : lmirBuilder_->Cvt(e1Type, returnType, e1Value);
            Expr tmp2Expr = (e2Type == returnType) ? e2Value : lmirBuilder_->Cvt(e2Type, returnType, e2Value);
            result = lmirBuilder_->Add(returnType, tmp1Expr, tmp2Expr);
        }
    } else if (machineType == MachineType::F64) {
        result = lmirBuilder_->Add(returnType, e1Value, e2Value);
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
    Expr baseAddr = GetExprFromGate(base);

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
    int offset =
        JSThread::GlueData::GetCOStubEntriesOffset(compCfg_->Is32Bit()) + static_cast<size_t>(index * slotSize_);
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
    return circuit_->GetFrameType() == FrameType::OPTIMIZED_JS_FUNCTION_FRAME;
}

bool LiteCGIRBuilder::IsOptimized() const
{
    return circuit_->GetFrameType() == FrameType::OPTIMIZED_FRAME;
}

CallExceptionKind LiteCGIRBuilder::GetCallExceptionKind(size_t index, OpCode op) const
{
    bool hasPcOffset = IsOptimizedJSFunction() &&
                       ((op == OpCode::NOGC_RUNTIME_CALL && (kungfu::RuntimeStubCSigns::IsAsmStub(index))) ||
                        (op == OpCode::CALL) || (op == OpCode::RUNTIME_CALL));
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

    CallExceptionKind kind = GetCallExceptionKind(stubIndex, OpCode::RUNTIME_CALL);
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
        callOp == OpCode::FAST_CALL_OPTIMIZED) {
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
        kind = GetCallExceptionKind(index, op);
    } else if (op == OpCode::NOGC_RUNTIME_CALL) {
        UpdateLeaveFrame(glue);
        const size_t index = acc_.GetConstantValue(inList[targetIndex]);
        calleeDescriptor = RuntimeStubCSigns::Get(index);
        Expr rtoffset = GetRTStubOffset(glue, index);
        Expr rtbaseoffset = lmirBuilder_->Add(glue.GetType(), glue, rtoffset);
        callee = GetFunction(bb, glue, calleeDescriptor, rtbaseoffset);
        kind = GetCallExceptionKind(index, op);
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
        pcIndex, {0, nullptr, lmirBuilder_->GetConstFromExpr(pcOffset), LiteCGValueKind::kConstKind}));

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
            std::pair<int, LiteCGValue>(encodeIndex, {0, nullptr, constMethodOffset, LiteCGValueKind::kConstKind}));
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

Expr LiteCGIRBuilder::CallingFp(bool /*isCaller*/)
{
    ASSERT(!IsInterpreted());
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
    // need use different symbol name?
    // get return type {i32 res, u1 carry}
    auto *retType = lmirBuilder_->GetStructType("overflow_internal@i32");
    retType = retType ? retType
                      : lmirBuilder_->CreateStructType("overflow_internal@i32")
                            .Field("res", lmirBuilder_->i32Type)
                            .Field("carry", lmirBuilder_->u1Type)
                            .Done();
    static uint32_t val = 0;
    std::string retVarName = "add_overflow_ret@i32" + std::to_string(val++);
    Var &retVar = lmirBuilder_->CreateLocalVar(retType, retVarName);

    // generate function call
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    std::vector<Expr> args = {e1Value, e2Value};
    auto &call = lmirBuilder_->IntrinsicCall(IntrinsicId::INTRN_ADD_WITH_OVERFLOW, args, &retVar);
    SaveGate2Expr(gate, lmirBuilder_->Dread(retVar));
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), call);
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
    // need use different symbol name?
    // get return type {i32 res, u1 carry}
    auto *retType = lmirBuilder_->GetStructType("overflow_internal@i32");
    retType = retType ? retType
                      : lmirBuilder_->CreateStructType("overflow_internal@i32")
                            .Field("res", lmirBuilder_->i32Type)
                            .Field("carry", lmirBuilder_->u1Type)
                            .Done();
    static uint32_t val = 0;
    std::string retVarName = "sub_overflow_ret@i32" + std::to_string(val++);
    Var &retVar = lmirBuilder_->CreateLocalVar(retType, retVarName);

    // generate function call
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    std::vector<Expr> args = {e1Value, e2Value};
    auto &call = lmirBuilder_->IntrinsicCall(IntrinsicId::INTRN_SUB_WITH_OVERFLOW, args, &retVar);
    SaveGate2Expr(gate, lmirBuilder_->Dread(retVar));
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), call);
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
    // need use different symbol name?
    // get return type {i32 res, u1 carry}
    auto *retType = lmirBuilder_->GetStructType("overflow_internal@i32");
    retType = retType ? retType
                      : lmirBuilder_->CreateStructType("overflow_internal@i32")
                            .Field("res", lmirBuilder_->i32Type)
                            .Field("carry", lmirBuilder_->u1Type)
                            .Done();
    static uint32_t val = 0;
    std::string retVarName = "mul_overflow_ret@i32" + std::to_string(val++);
    Var &retVar = lmirBuilder_->CreateLocalVar(retType, retVarName);

    // generate function call
    Expr e1Value = GetExprFromGate(e1);
    Expr e2Value = GetExprFromGate(e2);
    std::vector<Expr> args = {e1Value, e2Value};
    auto &call = lmirBuilder_->IntrinsicCall(IntrinsicId::INTRN_MUL_WITH_OVERFLOW, args, &retVar);
    SaveGate2Expr(gate, lmirBuilder_->Dread(retVar));
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
    Expr e1Value = GetExprFromGate(e1);
    ASSERT((acc_.GetOpCode(e2) == OpCode::CONSTANT) && acc_.GetMachineType(e2) == MachineType::I32);
    uint32_t index = static_cast<uint32_t>(acc_.GetConstantValue(e2));
    Var *baseVar = lmirBuilder_->GetLocalVarFromExpr(e1Value);
    ASSERT(baseVar != nullptr);
    // in maple type system, field 0 means the agg itself and field index start from 1
    Expr rhs = lmirBuilder_->DreadWithField(*baseVar, index + 1);
    PregIdx pregIdx = lmirBuilder_->CreatePreg(rhs.GetType());
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), lmirBuilder_->Regassign(rhs, pregIdx));
    SaveGate2Expr(gate, lmirBuilder_->Regread(pregIdx));
}

void LiteCGIRBuilder::HandleStore(GateRef gate)
{
    VisitStore(gate, acc_.GetIn(gate, 2), acc_.GetIn(gate, 1));  // 2:baseAddr gate, 1:data gate
}

void LiteCGIRBuilder::VisitStore(GateRef gate, GateRef base, GateRef value)
{
    Expr baseAddr = GetExprFromGate(base);
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
    SwitchBuilder builder = lmirBuilder_->Switch(type, cond, *defaultOutBB);
    for (int i = 0; i < caseNum; i++) {
        if (acc_.GetOpCode(outList[i]) == OpCode::DEFAULT_CASE) {
            continue;
        }
        BB &curOutBB = GetOrCreateBB(instID2bbID_[acc_.GetId(outList[i])]);
        builder.Case(i, curOutBB);
    }
    Stmt &switchStmt = builder.Done();
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), switchStmt);
    lmirBuilder_->AppendBB(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]));
}

void LiteCGIRBuilder::HandleBytecodeCall(GateRef gate)
{
    std::vector<GateRef> ins;
    acc_.GetIns(gate, ins);
    VisitBytecodeCall(gate, ins);
}

Expr LiteCGIRBuilder::GetBaseOffset(GateRef gate, Expr glue)
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

Expr LiteCGIRBuilder::GetBCStubOffset(Expr glue)
{
    return lmirBuilder_->ConstVal(
        lmirBuilder_->CreateIntConst(glue.GetType(), JSThread::GlueData::GetBCStubEntriesOffset(compCfg_->Is32Bit())));
}

Expr LiteCGIRBuilder::GetBCDebugStubOffset(Expr glue)
{
    return lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(
        glue.GetType(), JSThread::GlueData::GetBCDebuggerStubEntriesOffset(compCfg_->Is32Bit())));
}

void LiteCGIRBuilder::VisitBytecodeCall(GateRef gate, const std::vector<GateRef> &inList)
{
    size_t paraStartIndex = static_cast<size_t>(CallInputs::FIRST_PARAMETER);
    size_t targetIndex = static_cast<size_t>(CallInputs::TARGET);
    size_t glueIndex = static_cast<size_t>(CallInputs::GLUE);
    Expr opcodeOffset = GetExprFromGate(inList[targetIndex]);

    // start index of bytecode handler csign
    Expr glue = GetExprFromGate(inList[glueIndex]);
    Expr baseOffset = GetBaseOffset(gate, glue);
    Expr offset = lmirBuilder_->Add(baseOffset.GetType(), baseOffset, opcodeOffset);
    Expr rtbaseoffset = lmirBuilder_->Add(glue.GetType(), glue, offset);
    const CallSignature *signature = BytecodeStubCSigns::BCHandler();
    BB &bb = GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]);
    Expr callee = GetFunction(bb, glue, signature, rtbaseoffset);

    std::vector<Expr> params;
    for (size_t paraIdx = paraStartIndex; paraIdx < inList.size(); ++paraIdx) {
        GateRef gateTmp = inList[paraIdx];
        params.push_back(GetExprFromGate(gateTmp));
    }

    LiteCGType *funcType = GenerateFuncType(params, signature);
    LiteCGType *returnType = lmirBuilder_->LiteCGGetFuncReturnType(funcType);
    static uint32_t retNo = 0;
    std::string retName = signature->GetName() + "Ret" + std::to_string(retNo++);
    Var *returnVar =
        (returnType == lmirBuilder_->voidType) ? nullptr : &(lmirBuilder_->CreateLocalVar(returnType, retName));
    Stmt &callNode = lmirBuilder_->ICall(callee, params, returnVar);
    lmirBuilder_->AppendStmt(bb, callNode);
    if (returnVar != nullptr) {
        SaveGate2Expr(gate, lmirBuilder_->Dread(*returnVar));
    }

    lmirBuilder_->SetStmtCallConv(callNode, maple::litecg::GHC_Call);
}

void LiteCGIRBuilder::HandleDeoptCheck(GateRef gate)
{
    int block = instID2bbID_[acc_.GetId(gate)];
    std::vector<GateRef> outs;
    acc_.GetOutStates(gate, outs);
    int bbOut = instID2bbID_[acc_.GetId(outs[0])];  // 0: output

    BB &trueBB = GetOrCreateBB(bbOut);
    BB &falseBB = lmirBuilder_->CreateBB();
    GateRef cmp = acc_.GetValueIn(gate, 0);  // 0: cond
    Expr cond = GetExprFromGate(cmp);
    BB &curBB = GetOrCreateBB(block);
    lmirBuilder_->AppendStmt(curBB, lmirBuilder_->CondGoto(cond, falseBB, false));
    lmirBuilder_->AppendStmt(curBB, lmirBuilder_->Goto(trueBB));
    lmirBuilder_->AppendBB(curBB);
    // deopt branch is not expected to be token as often,
    // just put them to the end of the function
    lmirBuilder_->AppendToLast(falseBB);

    VisitDeoptCheck(gate);
    Expr returnValue = GetExprFromGate(gate);
    lmirBuilder_->AppendStmt(falseBB, lmirBuilder_->Return(returnValue));
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

    Expr glue = lmirBuilder_->Dread(lmirBuilder_->GetParam(func, 0));
    Expr check = lmirBuilder_->Dread(lmirBuilder_->GetParam(func, 1));
    Expr depth = lmirBuilder_->Dread(lmirBuilder_->GetParam(func, 2));

    StubIdType stubId = RTSTUB_ID(DeoptHandlerAsm);
    int stubIndex = static_cast<int>(std::get<RuntimeStubCSigns::ID>(stubId));
    Expr rtoffset = lmirBuilder_->Add(glue.GetType(), glue, GetRTStubOffset(glue, stubIndex));
    Expr patchAddr = lmirBuilder_->Cvt(glue.GetType(), lmirBuilder_->i64PtrType, rtoffset);
    Expr funcAddr = lmirBuilder_->Iread(rtoffset.GetType(), patchAddr, lmirBuilder_->i64PtrType);

    LiteCGType *funcType = GetExperimentalDeoptTy();
    LiteCGType *funcTypePtr = lmirBuilder_->CreatePtrType(funcType);
    LiteCGType *funcTypePtrPtr = lmirBuilder_->CreatePtrType(funcTypePtr);
    Expr callee = lmirBuilder_->Cvt(glue.GetType(), funcTypePtrPtr, funcAddr);

    Var &funcVar = lmirBuilder_->CreateLocalVar(callee.GetType(), "DeoptimizeSubFunc");
    Stmt &funcAddrNode = lmirBuilder_->Dassign(callee, funcVar);
    lmirBuilder_->AppendStmt(bb, funcAddrNode);

    LiteCGType *returnType = lmirBuilder_->LiteCGGetFuncReturnType(funcType);
    PregIdx pregIdx = lmirBuilder_->CreatePreg(returnType);
    std::vector<Expr> params = {glue, check, depth};
    Stmt &callNode = lmirBuilder_->ICall(lmirBuilder_->Dread(funcVar), params, pregIdx);
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
    Expr value = ConvertToTagged(gate);
    PregIdx pregIdx = lmirBuilder_->CreatePreg(value.GetType());
    lmirBuilder_->AppendStmt(bb, lmirBuilder_->Regassign(value, pregIdx));
    LiteCGValue deoptVal;
    deoptVal.kind = LiteCGValueKind::kPregKind;
    deoptVal.pregIdx = pregIdx;
    deoptBundleInfo.insert(std::pair<int, LiteCGValue>(encodeIndex, deoptVal));
}
void LiteCGIRBuilder::SaveDeoptVregInfoWithI64(std::unordered_map<int, LiteCGValue> &deoptBundleInfo, BB &bb,
                                               int32_t index, size_t curDepth, size_t shift, GateRef gate)
{
    int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(index, curDepth, shift);
    Expr expr = GetExprFromGate(gate);
    Expr value = ConvertInt32ToTaggedInt(lmirBuilder_->Cvt(expr.GetType(), lmirBuilder_->i32Type, expr));
    PregIdx pregIdx = lmirBuilder_->CreatePreg(value.GetType());
    lmirBuilder_->AppendStmt(bb, lmirBuilder_->Regassign(value, pregIdx));
    LiteCGValue deoptVal;
    deoptVal.kind = LiteCGValueKind::kPregKind;
    deoptVal.pregIdx = pregIdx;
    deoptBundleInfo.insert(std::pair<int, LiteCGValue>(encodeIndex, deoptVal));
}

void LiteCGIRBuilder::VisitDeoptCheck(GateRef gate)
{
    BB &bb = lmirBuilder_->GetLastAppendedBB();  // falseBB of deopt check
    Expr glue = GetExprFromGate(acc_.GetGlueFromArgList());
    GateRef deoptFrameState = acc_.GetValueIn(gate, 1);  // 1: frame state
    ASSERT(acc_.GetOpCode(deoptFrameState) == OpCode::FRAME_STATE);
    std::vector<Expr> params;
    params.push_back(glue);                        // glue
    GateRef deoptType = acc_.GetValueIn(gate, 2);  // 2: deopt type
    uint64_t v = acc_.GetConstantValue(deoptType);
    Expr constV = lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->u32Type, static_cast<uint32_t>(v)));
    params.push_back(ConvertInt32ToTaggedInt(constV));  // deoptType
    Function *callee = GetExperimentalDeopt();
    LiteCGType *funcType = GetExperimentalDeoptTy();

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
            if (acc_.IsConstantValue(vregValue, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
                continue;
            }
            SaveDeoptVregInfo(deoptBundleInfo, bb, i, curDepth, shift, vregValue);
        }
        // env
        if (!acc_.IsConstantValue(env, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
            int32_t specEnvVregIndex = static_cast<int32_t>(SpecVregIndex::ENV_INDEX);
            SaveDeoptVregInfo(deoptBundleInfo, bb, specEnvVregIndex, curDepth, shift, env);
        }
        // acc
        if (!acc_.IsConstantValue(acc, JSTaggedValue::VALUE_OPTIMIZED_OUT)) {
            int32_t specAccVregIndex = static_cast<int32_t>(SpecVregIndex::ACC_INDEX);
            SaveDeoptVregInfo(deoptBundleInfo, bb, specAccVregIndex, curDepth, shift, acc);
        }
        // pc offset
        int32_t specPcOffsetIndex = static_cast<int32_t>(SpecVregIndex::PC_OFFSET_INDEX);
        int32_t encodeIndex = Deoptimizier::EncodeDeoptVregIndex(specPcOffsetIndex, curDepth, shift);
        Const &pcConst = lmirBuilder_->CreateIntConst(lmirBuilder_->u32Type, pc);
        LiteCGValue deoptVal;
        deoptVal.kind = LiteCGValueKind::kConstKind;
        deoptVal.constVal = &pcConst;
        deoptBundleInfo.insert(std::pair<int, LiteCGValue>(encodeIndex, deoptVal));

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
    LiteCGType *returnType = lmirBuilder_->LiteCGGetFuncReturnType(funcType);

    bool returnVoid = (returnType == lmirBuilder_->voidType);
    PregIdx returnPregIdx = returnVoid ? -1 : lmirBuilder_->CreatePreg(returnType);

    Stmt &callNode = lmirBuilder_->Call(*callee, params, returnPregIdx);
    lmirBuilder_->SetCallStmtDeoptBundleInfo(callNode, deoptBundleInfo);

    lmirBuilder_->AppendStmt(bb, callNode);
    if (!returnVoid) {
        SaveGate2Expr(gate, lmirBuilder_->Regread(returnPregIdx));
    }
}

void LiteCGIRBuilder::HandleConstString(GateRef gate)
{
    const ChunkVector<char> &str = acc_.GetConstantString(gate);  // 64: bit width
    VisitConstString(gate, str);
}

void LiteCGIRBuilder::VisitConstString(GateRef gate, const ChunkVector<char> &str)  // 64: bit width
{
    ASSERT(acc_.GetMachineType(gate) == MachineType::ARCH);

    Expr value = lmirBuilder_->ConstVal(lmirBuilder_->CreateStrConst(std::string(str.data(), str.size())));
    static uint32_t val = 0;
    std::string name = "ConstStringVar" + std::to_string(val++);
    Var &var = lmirBuilder_->CreateLocalVar(value.GetType(), name);
    Stmt &stmt = lmirBuilder_->Dassign(value, var);
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), stmt);
    SaveGate2Expr(gate, lmirBuilder_->Addrof(var));
}

void LiteCGIRBuilder::HandleRelocatableData(GateRef gate)
{
    uint64_t value = acc_.TryGetValue(gate);
    VisitRelocatableData(gate, value);
}

void LiteCGIRBuilder::VisitRelocatableData(GateRef gate, uint64_t value)
{
    Var &var = lmirBuilder_->CreateGlobalVar(lmirBuilder_->i64Type, "G", maple::litecg::GlobalVarAttr::VAR_internal);
    Expr constVal = lmirBuilder_->ConstVal(lmirBuilder_->CreateIntConst(lmirBuilder_->i64Type, value));
    Stmt &stmt = lmirBuilder_->Dassign(constVal, var);
    lmirBuilder_->AppendStmt(GetOrCreateBB(instID2bbID_[acc_.GetId(gate)]), stmt);
    SaveGate2Expr(gate, lmirBuilder_->Dread(var));
}

void LiteCGIRBuilder::HandleAlloca(GateRef gate)
{
    return VisitAlloca(gate);
}

void LiteCGIRBuilder::VisitAlloca(GateRef gate)
{
    uint64_t machineRep = acc_.TryGetValue(gate);
    LiteCGType *dataType = GetMachineRepType(static_cast<MachineRep>(machineRep));

    static uint32_t val = 0;
    std::string name = "AllocaVar" + std::to_string(val++);
    Var &var = lmirBuilder_->CreateLocalVar(dataType, name);
    Expr addr = lmirBuilder_->Addrof(var);
    Expr addrCvt = lmirBuilder_->Cvt(addr.GetType(), ConvertLiteCGTypeFromGate(gate), addr);
    SaveGate2Expr(gate, addrCvt);
}

LiteCGType *LiteCGIRBuilder::GetMachineRepType(MachineRep rep) const
{
    LiteCGType *dstType;
    switch (rep) {
        case MachineRep::K_BIT:
            dstType = lmirBuilder_->u1Type;
            break;
        case MachineRep::K_WORD8:
            dstType = lmirBuilder_->i8Type;
            break;
        case MachineRep::K_WORD16:
            dstType = lmirBuilder_->i16Type;
            break;
        case MachineRep::K_WORD32:
            dstType = lmirBuilder_->i32Type;
            break;
        case MachineRep::K_FLOAT64:
            dstType = lmirBuilder_->f64Type;
            break;
        case MachineRep::K_WORD64:
            dstType = lmirBuilder_->i64Type;
            break;
        case MachineRep::K_PTR_1:
            dstType = lmirBuilder_->i64RefType;
            break;
        case MachineRep::K_META:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return dstType;
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
