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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OBJ_EMIT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OBJ_EMIT_H

#include "obj_emit.h"
#include "aarch64_insn.h"
#include "aarch64_cg.h"

namespace maplebe {
enum AArch64FixupKind {
    kAArch64PCRelAdrImm21 = kFirstTargetFixupKind,
    kAArch64PCRelAdrpImm21,
    kAArch64LoadPCRelImm19,
    kAArch64CondBranchPCRelImm19,
    kAArch64UnCondBranchPCRelImm26,
    kAArch64CompareBranchPCRelImm19,
    kAArch64TestBranchPCRelImm14,
    kAArch64CallPCRelImm26,
    kAArch64AddPCRelLo12,
    kAArch64LdrPCRelLo12,
};

class AArch64ObjFuncEmitInfo : public ObjFuncEmitInfo {
public:
    AArch64ObjFuncEmitInfo(CGFunc &func, MemPool &memPool) : ObjFuncEmitInfo(func, memPool) {}
    ~AArch64ObjFuncEmitInfo() = default;
    void HandleLocalBranchFixup(const std::vector<uint32> &label2Offset,
                                const std::vector<uint32> &symbol2Offset) override;
};

class AArch64ObjEmitter : public ObjEmitter {
public:
    AArch64ObjEmitter(CG &cg, const std::string &objFileName) : ObjEmitter(cg, objFileName) {}
    ~AArch64ObjEmitter() = default;

    void EncodeInstruction(const Insn &insn, const std::vector<uint32> &label2Offset,
                           ObjFuncEmitInfo &objFuncEmitInfo) override
    {
        uint32 binInsn = GetBinaryCodeForInsn(insn, label2Offset, objFuncEmitInfo);
        objFuncEmitInfo.AppendTextData(binInsn, k4ByteSize);
        if (insn.GetMachineOpcode() == MOP_xbl || insn.GetMachineOpcode() == MOP_xblr) {
            if (insn.GetStackMap() == nullptr) {
                return;
            }
            objFuncEmitInfo.RecordOffset2StackMapInfo(objFuncEmitInfo.GetTextDataSize(),
                                                      insn.GetStackMap()->GetReferenceMap().SerializeInfo(),
                                                      insn.GetStackMap()->GetDeoptInfo().SerializeInfo());
        }
    }

    uint32 GetInsnSize(const Insn &insn) const override
    {
        (void)insn;
        return k4ByteSize;
    }

    FuncEmitInfo &CreateFuncEmitInfo(CGFunc &cgFunc)
    {
        MemPool *memPool = cgFunc.GetCG()->GetMIRModule()->GetMemPool();
        AArch64ObjFuncEmitInfo *content = memPool->New<AArch64ObjFuncEmitInfo>(cgFunc, *memPool);
        contents.insert(contents.begin() + cgFunc.GetFunction().GetPuidxOrigin(), content);
        return *content;
    }

    void HandleTextSectionGlobalFixup() override;
    void AppendTextSectionData() override;
    void AppendGlobalLabel() override;
    void AppendSymsToSymTabSec() override;
    void InitSections() override;
    void LayoutSections() override;
    void UpdateMachineAndFlags(FileHeader &header) override;
    void EmitDataToDynamic();
    void EmitDataToHash();
    void EmitIntrinsicInsn(const Insn &insn, const std::vector<uint32_t> &label2Offset,
                           ObjFuncEmitInfo &objFuncEmitInfo) override;
    void EmitSpinIntrinsicInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) override;

    uint32 GetBinaryCodeForInsn(const Insn &insn, const std::vector<uint32> &label2Offset,
                                ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GetOpndMachineValue(const Operand &opnd) const;
    uint32 GetAdrLabelOpndValue(const Insn &insn, const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GetLoadLiteralOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GetCondBranchOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GetUnCondBranchOpndValue(const Operand &opnd, const std::vector<uint32> &label2Offset,
                                    ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GetCallFuncOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GetTestBranchOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GetCompareBranchOpndValue(const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GetLo12LitrealOpndValue(MOperator mOp, const Operand &opnd, ObjFuncEmitInfo &objFuncEmitInfo) const;
    void InsertNopInsn(ObjFuncEmitInfo &objFuncEmitInfo) const override;

private:
    uint32 GenAddSubExtendRegInsn(const Insn &insn) const;
    uint32 GenAddSubImmInsn(const Insn &insn) const;
    uint32 GenAddSubShiftImmInsn(const Insn &insn) const;
    uint32 GenAddSubRegInsn(const Insn &insn) const;
    uint32 GenAddSubShiftRegInsn(const Insn &insn) const;
    uint32 GenBitfieldInsn(const Insn &insn) const;
    uint32 GenExtractInsn(const Insn &insn) const;
    uint32 GenBranchImmInsn(const Insn &insn, const std::vector<uint32> &label2Offset,
                            ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenBranchRegInsn(const Insn &insn) const;
    uint32 GenCompareBranchInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenCondCompareImmInsn(const Insn &insn) const;
    uint32 GenCondCompareRegInsn(const Insn &insn) const;
    uint32 GenConditionalSelectInsn(const Insn &insn) const;
    uint32 GenDataProcess1SrcInsn(const Insn &insn) const;
    uint32 GenDataProcess2SrcInsn(const Insn &insn) const;
    uint32 GenDataProcess3SrcInsn(const Insn &insn) const;
    uint32 GenFloatIntConversionsInsn(const Insn &insn) const;
    uint32 GenFloatCompareInsn(const Insn &insn) const;
    uint32 GenFloatDataProcessing1Insn(const Insn &insn) const;
    uint32 GenFloatDataProcessing2Insn(const Insn &insn) const;
    uint32 GenFloatImmInsn(const Insn &insn) const;
    uint32 GenFloatCondSelectInsn(const Insn &insn) const;
    uint32 GenLoadStoreModeLiteral(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenLoadStoreModeBOi(const Insn &insn) const;
    uint32 GenLoadStoreModeBOrX(const Insn &insn) const;
    uint32 GenLoadStoreRegInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenLoadStoreARInsn(const Insn &insn) const;
    uint32 GenLoadExclusiveInsn(const Insn &insn) const;
    uint32 GenLoadExclusivePairInsn(const Insn &insn) const;
    uint32 GenStoreExclusiveInsn(const Insn &insn) const;
    uint32 GenStoreExclusivePairInsn(const Insn &insn) const;
    uint32 GenLoadPairInsn(const Insn &insn) const;
    uint32 GenStorePairInsn(const Insn &insn) const;
    uint32 GenLoadStoreFloatInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenLoadPairFloatInsn(const Insn &insn) const;
    uint32 GenStorePairFloatInsn(const Insn &insn) const;
    uint32 GenLoadLiteralRegInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenLogicalRegInsn(const Insn &insn) const;
    uint32 GenLogicalImmInsn(const Insn &insn) const;
    uint32 GenMoveWideInsn(const Insn &insn) const;
    uint32 GenPCRelAddrInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenAddPCRelAddrInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenSystemInsn(const Insn &insn) const;
    uint32 GenTestBranchInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenCondBranchInsn(const Insn &insn, ObjFuncEmitInfo &objFuncEmitInfo) const;
    uint32 GenMovReg(const Insn &insn) const;
    uint32 GenMovImm(const Insn &insn) const;
    uint32 EncodeLogicaImm(uint64 imm, uint32 size) const;
    void HandleCallFixup(ObjFuncEmitInfo &objFuncEmitInfo, const Fixup &fixup);
    void HandleAdrFixup(ObjFuncEmitInfo &objFuncEmitInfo, const Fixup &fixup);

    /* emit intrinsic insn */
    void EmitMCCStackMapCall(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitEnv(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitClinit(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitCounter(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitLazyLoad(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitLazyLoadStatic(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitAdrpLdr(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitArrayClassCacheLoad(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitClinitTail(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitGetAndAddInt(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitGetAndSetInt(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitCompareAndSwapInt(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitStringIndexOf(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitStringIndexOf2(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitStringIndexOf3(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitCheckCastNoArray(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitCheckCastIsAssignable(const Insn &insn, std::vector<uint32> &label2Offset,
                                   ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitCheckCastNoSubIsAssignable(const Insn &insn, std::vector<uint32> &label2Offset,
                                        ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitInstanceOfIsAssignable(const Insn &insn, std::vector<uint32> &label2Offset,
                                    ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitInstanceOfNoSubIsAssignable(const Insn &insn, std::vector<uint32> &label2Offset,
                                         ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitMovMovkri16(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);
    void EmitMovMovk64ri16(const Insn &insn, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo);

    void EmitInsn(MOperator mOp, Operand &opnd1, std::vector<uint32> &label2Offset, ObjFuncEmitInfo &objFuncEmitInfo)
    {
        Insn &insn = objFuncEmitInfo.GetCGFunc().GetInsnBuilder()->BuildInsn(mOp, opnd1);
        EncodeInstruction(insn, label2Offset, objFuncEmitInfo);
    }

    void EmitInsn(MOperator mOp, Operand &opnd1, Operand &opnd2, std::vector<uint32> &label2Offset,
                  ObjFuncEmitInfo &objFuncEmitInfo)
    {
        Insn &insn = objFuncEmitInfo.GetCGFunc().GetInsnBuilder()->BuildInsn(mOp, opnd1, opnd2);
        EncodeInstruction(insn, label2Offset, objFuncEmitInfo);
    }

    void EmitInsn(MOperator mOp, Operand &opnd1, Operand &opnd2, Operand &opnd3, std::vector<uint32> &label2Offset,
                  ObjFuncEmitInfo &objFuncEmitInfo)
    {
        Insn &insn = objFuncEmitInfo.GetCGFunc().GetInsnBuilder()->BuildInsn(mOp, opnd1, opnd2, opnd3);
        EncodeInstruction(insn, label2Offset, objFuncEmitInfo);
    }
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OBJ_EMIT_H */
