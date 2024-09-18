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

#ifndef MAPLE_IR_INCLUDE_MIR_LOWER_H
#define MAPLE_IR_INCLUDE_MIR_LOWER_H
#include <iostream>
#include "mir_builder.h"
#include "opcodes.h"

namespace maple {
// The base value for branch probability notes and edge probabilities.
static constexpr int32 kProbAll = 10000;
static constexpr int32 kProbLikely = 9000;
static constexpr int32 kProbUnlikely = kProbAll - kProbLikely;
constexpr uint32 kNodeFirstOpnd = 0;
constexpr uint32 kNodeSecondOpnd = 1;
constexpr uint32 kNodeThirdOpnd = 2;
enum MirLowerPhase : uint8 { kLowerUnder, kLowerMe, kLowerExpandArray, kLowerBe, kLowerCG, kLowerLNO };

constexpr uint32 kShiftLowerMe = 1U << kLowerMe;
constexpr uint32 kShiftLowerBe = 1U << kLowerBe;
constexpr uint32 kShiftLowerCG = 1U << kLowerCG;
constexpr uint32 kShiftLowerLNO = 1U << kLowerLNO;
// check if a block node ends with an unconditional jump
inline bool OpCodeNoFallThrough(Opcode opCode)
{
    return opCode == OP_goto || opCode == OP_return || opCode == OP_switch;
}

inline bool IfStmtNoFallThrough(const IfStmtNode &ifStmt)
{
    return OpCodeNoFallThrough(ifStmt.GetThenPart()->GetLast()->GetOpCode());
}

class MIRLower {
public:
    static const std::set<std::string> kSetArrayHotFunc;

    MIRLower(MIRModule &mod, MIRFunction *f) : mirModule(mod), mirFunc(f) {}

    virtual ~MIRLower() = default;

    const MIRFunction *GetMirFunc() const
    {
        return mirFunc;
    }

    void SetMirFunc(MIRFunction *f)
    {
        mirFunc = f;
    }

    void Init()
    {
        mirBuilder = mirModule.GetMemPool()->New<MIRBuilder>(&mirModule);
    }

    virtual BlockNode *LowerIfStmt(IfStmtNode &ifStmt, bool recursive);
    BlockNode *LowerSwitchStmt(SwitchNode *switchNode);
    BlockNode *LowerBlock(BlockNode &);
    BaseNode *LowerEmbeddedCandCior(BaseNode *x, StmtNode *curstmt, BlockNode *block);
    void LowerCandCior(BlockNode &block);
    void LowerBuiltinExpect(BlockNode &block);
    void LowerFunc(MIRFunction &func);
    MIRFuncType *FuncTypeFromFuncPtrExpr(BaseNode *x);
    void SetLowerME()
    {
        lowerPhase |= kShiftLowerMe;
    }

    void SetLowerLNO()
    {
        lowerPhase |= kShiftLowerLNO;
    }

    void SetLowerBE()
    {
        lowerPhase |= kShiftLowerBe;
    }

    void SetLowerCG()
    {
        lowerPhase |= kShiftLowerCG;
    }

    uint8 GetOptLevel() const
    {
        return optLevel;
    }

    void SetOptLevel(uint8 optlvl)
    {
        optLevel = optlvl;
    }

    bool IsLowerME() const
    {
        return lowerPhase & kShiftLowerMe;
    }

    bool IsLowerLNO() const
    {
        return lowerPhase & kShiftLowerLNO;
    }

    bool IsLowerBE() const
    {
        return lowerPhase & kShiftLowerBe;
    }

    bool IsLowerCG() const
    {
        return lowerPhase & kShiftLowerCG;
    }

    static bool ShouldOptArrayMrt(const MIRFunction &func);

    virtual bool InLFO() const
    {
        return false;
    }

    GcovFuncInfo *GetFuncProfData()
    {
        return mirFunc->GetFuncProfData();
    }
    void CopyStmtFrequency(StmtNode *newStmt, StmtNode *oldStmt)
    {
        DEBUG_ASSERT(GetFuncProfData() != nullptr, "nullptr check");
        if (newStmt == oldStmt)
            return;
        int64_t freq = GetFuncProfData()->GetStmtFreq(oldStmt->GetStmtID());
        GetFuncProfData()->SetStmtFreq(newStmt->GetStmtID(), freq);
    }

protected:
    MIRModule &mirModule;

private:
    MIRFunction *mirFunc;
    MIRBuilder *mirBuilder = nullptr;
    uint32 lowerPhase = 0;
    uint8 optLevel = 0;
    LabelIdx CreateCondGotoStmt(Opcode op, BlockNode &blk, const IfStmtNode &ifStmt);
    void CreateBrFalseStmt(BlockNode &blk, const IfStmtNode &ifStmt);
    void CreateBrTrueStmt(BlockNode &blk, const IfStmtNode &ifStmt);
    void CreateBrFalseAndGotoStmt(BlockNode &blk, const IfStmtNode &ifStmt);
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_LOWER_H
