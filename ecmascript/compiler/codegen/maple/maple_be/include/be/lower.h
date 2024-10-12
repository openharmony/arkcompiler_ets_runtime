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

#ifndef MAPLEBE_INCLUDE_BE_LOWERER_H
#define MAPLEBE_INCLUDE_BE_LOWERER_H
/* C++ headers. */
#include <vector>
#include <unordered_map>
#include <utility>
#include <cstddef>
#include <cstdarg>
#include <regex>
#include "intrinsics.h" /* For IntrinDesc. This includes 'intrinsic_op.h' as well */
#include "becommon.h"
#include "cg.h"
/* MapleIR headers. */
#include "mir_nodes.h"
#include "mir_module.h"
#include "mir_function.h"
#include "mir_lower.h"

namespace maplebe {
class CGLowerer {
    enum Option : uint64 {
        kUndefined = 0,
        kGenEh = 1ULL << 0,
        kVerboseCG = 1ULL << 1,
    };

    using BuiltinFunctionID = uint32;
    using OptionFlag = uint64;

public:
    CGLowerer(MIRModule &mod, BECommon &common, MIRFunction *func = nullptr) : mirModule(mod), beCommon(common)
    {
        SetOptions(kGenEh);
        mirBuilder = mod.GetMIRBuilder();
        SetCurrentFunc(func);
    }

    CGLowerer(MIRModule &mod, BECommon &common, bool genEh, bool verboseCG) : mirModule(mod), beCommon(common)
    {
        OptionFlag option = 0;
        if (genEh) {
            option |= kGenEh;
        }
        if (verboseCG) {
            option |= kVerboseCG;
        }
        SetOptions(option);
        mirBuilder = mod.GetMIRBuilder();
        SetCurrentFunc(nullptr);
    }

    ~CGLowerer()
    {
        mirBuilder = nullptr;
        currentBlock = nullptr;
    }

    void LowerFunc(MIRFunction &func);

    StmtNode *LowerDefaultIntrinsicCall(IntrinsiccallNode &intrinCall, MIRSymbol &st, MIRFunction &fn);

    StmtNode *LowerIntrinsiccall(IntrinsiccallNode &intrinCall, BlockNode &);

    MIRFunction *GetCurrentFunc() const
    {
        return mirModule.CurFunction();
    }

    BaseNode *LowerExpr(BaseNode &, BaseNode &, BlockNode &);

    BaseNode *LowerDread(DreadNode &dread, const BlockNode &block);

    BaseNode *LowerIread(IreadNode &iread)
    {
        /* use PTY_u8 for boolean type in dread/iread */
        if (iread.GetPrimType() == PTY_u1) {
            iread.SetPrimType(PTY_u8);
        }
        CHECK_FATAL(iread.GetFieldID() == 0, "fieldID must be 0");
        return &iread;
    }

    BaseNode *LowerCastExpr(BaseNode &expr);

    BaseNode *ExtractSymbolAddress(const StIdx &stIdx);
    BaseNode *LowerDreadToThreadLocal(BaseNode &expr, const BlockNode &block);
    StmtNode *LowerDassignToThreadLocal(StmtNode &stmt, const BlockNode &block);

    void LowerDassign(DassignNode &dassign, BlockNode &block);

    void LowerIassign(IassignNode &iassign, BlockNode &block);

    void LowerRegassign(RegassignNode &regAssign, BlockNode &block);

    BaseNode *LowerAddrof(AddrofNode &addrof) const
    {
        return &addrof;
    }

    DassignNode *SaveReturnValueInLocal(StIdx, uint16);
    BaseNode *NeedRetypeWhenLowerCallAssigned(PrimType pType);
    void LowerCallStmt(StmtNode &, StmtNode *&, BlockNode &, MIRType *retty = nullptr, bool uselvar = false,
                       bool isIntrinAssign = false);
    BlockNode *LowerIntrinsiccallAassignedToAssignStmt(IntrinsiccallNode &intrinsicCall);
    BlockNode *LowerCallAssignedStmt(StmtNode &stmt, bool uselvar = false);
    /* Intrinsiccall will processe return and vector as a call separately.
     * To be able to handle them in a unified manner, we lower intrinsiccall to Intrinsicsicop.
     */
    BlockNode *LowerIntrinsiccallToIntrinsicop(StmtNode &stmt);

    void LowerStmt(StmtNode &stmt, BlockNode &block);

    void LowerSwitchOpnd(StmtNode &stmt, BlockNode &block);

    BlockNode *LowerBlock(BlockNode &block);

#if TARGARM32 || TARGAARCH64 || TARGRISCV64 || TARGX86_64
    BlockNode *LowerReturnStructUsingFakeParm(NaryStmtNode &retNode);
#endif
    BlockNode *LowerReturn(NaryStmtNode &retNode);
    void LowerEntry(MIRFunction &func);

    void CleanupBranches(MIRFunction &func) const;

    void LowerTypePtr(BaseNode &expr) const;

    /* A pseudo register refers to a symbol when DreadNode is converted to RegreadNode. */
    StIdx GetSymbolReferredToByPseudoRegister(PregIdx regNO) const
    {
        (void)regNO;
        return StIdx();
    }

    void SetOptions(OptionFlag option)
    {
        options = option;
    }

    void SetCheckLoadStore(bool value)
    {
        checkLoadStore = value;
    }

    /* if it defines a built-in to use for the given intrinsic, return the name. otherwise, return nullptr */
    PUIdx GetBuiltinToUse(BuiltinFunctionID id) const;

    MIRModule &mirModule;
    BECommon &beCommon;
    BlockNode *currentBlock = nullptr; /* current block for lowered statements to be inserted to */
    bool checkLoadStore = false;
    int64 seed = 0;

    static constexpr PUIdx kFuncNotFound = PUIdx(-1);
    static constexpr int kThreeDimArray = 3;
    static constexpr int kNodeThirdOpnd = 2;
    static constexpr int kMCCSyncEnterFast0 = 0;
    static constexpr int kMCCSyncEnterFast1 = 1;
    static constexpr int kMCCSyncEnterFast2 = 2;
    static constexpr int kMCCSyncEnterFast3 = 3;

protected:
    /*
     * true if the lower level (e.g. mplcg) can handle the intrinsic directly.
     * For example, the INTRN_MPL_ATOMIC_EXCHANGE_PTR can be directly handled by mplcg,
     * and generate machine code sequences not containing any function calls.
     * Such intrinsics will bypass the lowering of "assigned",
     * and let mplcg handle the intrinsic results which are not return values.
     */
    bool IsIntrinsicCallHandledAtLowerLevel(MIRIntrinsicID intrinsic) const;

private:
    void SetCurrentFunc(MIRFunction *func)
    {
        mirModule.SetCurFunction(func);
    }

    bool ShouldAddAdditionalComment() const
    {
        return (options & kVerboseCG) != 0;
    }

    LabelIdx GetLabelIdx(MIRFunction &curFunc) const;
    StmtNode *GenCallNode(const StmtNode &stmt, PUIdx &funcCalled, CallNode &origCall);
    StmtNode *GenIntrinsiccallNode(const StmtNode &stmt, PUIdx &funcCalled, bool &handledAtLowerLevel,
                                   IntrinsiccallNode &origCall);
    StmtNode *GenIcallNode(PUIdx &funcCalled, IcallNode &origCall);
    BlockNode *GenBlockNode(StmtNode &newCall, const CallReturnVector &p2nRets, const Opcode &opcode,
                            const PUIdx &funcCalled, bool handledAtLowerLevel, bool uselvar);

    OptionFlag options = 0;
    bool needBranchCleanup = false;

    static std::vector<std::pair<BuiltinFunctionID, PUIdx>> builtinFuncIDs;
    MIRBuilder *mirBuilder = nullptr;
    uint32 labelIdx = 0;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_BE_LOWERER_H */