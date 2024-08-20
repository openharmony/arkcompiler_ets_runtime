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

#ifndef MAPLEBE_INCLUDE_CG_ISA_H
#define MAPLEBE_INCLUDE_CG_ISA_H

#include <cstdint>
#include "types_def.h"
#include "operand.h"

namespace maplebe {
// For verify & split insn
#define VERIFY_INSN(INSN) (INSN)->VerifySelf()
#define SPLIT_INSN(INSN, FUNC) \
    (INSN)->SplitSelf((FUNC)->IsAfterRegAlloc(), (FUNC)->GetInsnBuilder(), (FUNC)->GetOpndBuilder())

// circular dependency exists, no other choice
class Insn;
class InsnBuilder;
class OperandBuilder;

enum MopProperty : maple::uint8 {
    kInsnIsAbstract,
    kInsnIsMove,
    kInsnIsLoad,
    kInsnIsLoadPair,
    kInsnIsStore,
    kInsnIsStorePair,
    kInsnIsLoadAddress,
    kInsnIsAtomic,
    kInsnIsCall,
    kInsnIsSpecialCall,
    kInsnIsTailCall,
    kInsnIsConversion,
    kInsnIsCondDef,
    kInsnHasAcqure,
    kInsnHasAcqureRCpc,
    kInsnHasLOAcqure,
    kInsnHasRelease,
    kInsnHasLORelease,
    kInsnCanThrow,
    kInsnIsDMB,
    kInsnIsUnCondBr,
    kInsnIsCondBr,
    kInsnHasLoop,
    kInsnIsBinaryOp,
    kInsnIsPhi,
    kInsnIsUnaryOp,
    kInsnIsShift,
    kInsnInlineAsm,
    kInsnSpecialIntrisic,
    kInsnIsNop,
    kInsnIntrinsic,
    kInsnIsBreakPoint,
};
using regno_t = uint32_t;
#define ISABSTRACT 1ULL
#define ISMOVE (1ULL << kInsnIsMove)
#define ISLOAD (1ULL << kInsnIsLoad)
#define ISLOADPAIR (1ULL << kInsnIsLoadPair)
#define ISSTORE (1ULL << kInsnIsStore)
#define ISSTOREPAIR (1ULL << kInsnIsStorePair)
#define ISLOADADDR (1ULL << kInsnIsLoadAddress)
#define ISATOMIC (1ULL << kInsnIsAtomic)
#define ISCALL (1ULL << kInsnIsCall)
#define ISSPCALL (1ULL << kInsnIsSpecialCall)
#define ISTAILCALL (1ULL << kInsnIsTailCall)
#define ISCONVERSION (1ULL << kInsnIsConversion)
#define ISCONDDEF (1ULL << kInsnIsCondDef)
#define HASACQUIRE (1ULL << kInsnHasAcqure)
#define HASACQUIRERCPC (1ULL << kInsnHasAcqureRCpc)
#define HASLOACQUIRE (1ULL << kInsnHasLOAcqure)
#define HASRELEASE (1ULL << kInsnHasRelease)
#define HASLORELEASE (1ULL << kInsnHasLORelease)
#define CANTHROW (1ULL << kInsnCanThrow)
#define ISDMB (1ULL << kInsnIsDMB)
#define ISUNCONDBRANCH (1ULL << kInsnIsUnCondBr)
#define ISCONDBRANCH (1ULL << kInsnIsCondBr)
#define HASLOOP (1ULL << kInsnHasLoop)
#define ISBASICOP (1ULL << kInsnIsBinaryOp)
#define ISPHI (1ULL << kInsnIsPhi)
#define ISUNARYOP (1ULL << kInsnIsUnaryOp)
#define ISSHIFT (1ULL << kInsnIsShift)
#define INLINEASM (1ULL << kInsnInlineAsm)
#define SPINTRINSIC (1ULL << kInsnSpecialIntrisic)
#define ISNOP (1ULL << kInsnIsNop)
#define ISINTRINSIC (1ULL << kInsnIntrinsic)
#define ISBREAKPOINT (1ULL << kInsnIsBreakPoint)
constexpr maplebe::regno_t kInvalidRegNO = 0;

/*
 * ARM64 has 32 int registes and 32 FP registers.
 * AMD64/X86_64 has 16 int registes, and 16 FP registers.
 * In either case, the corresponding calling conventions use
 * the smaller number of caller-saved registers.
 * 64 bit is not large enough?
 */
using CsrBitset = uint64_t;

template <typename ParaType>
class ConstraintFunction {
public:
    using CfPointer = bool (*)(ParaType);
    bool CheckConstraint(CfPointer ccfunc, ParaType a) const
    {
        return (*ccfunc)(a);
    }
};

/*
 * abstract machine instruction
 * a lower-level maple IR which is aimed to represent general machine instruction for extreme cpus
 * 1. Support conversion between all types and registers
 * 2. Support conversion between memory and registers
 * 3. Support three address basic operations
 *
 */
namespace abstract {
#define DEFINE_MOP(op, ...) op,
enum AbstractMOP_t : maple::uint32 {
#include "abstract_mmir.def"
    kMopLast
};
#undef DEFINE_MOP
}  // namespace abstract

enum EncodeType : uint8 {
    kMovImm,
    kMovReg,
    kAddSubExtendReg,
    kAddSubImm,
    kAddSubShiftImm,
    kAddSubReg,
    kAddSubShiftReg,
    kBitfield,
    kExtract,
    kBranchImm,
    kBranchReg,
    kCompareBranch,
    kCondCompareImm,
    kCondCompareReg,
    kConditionalSelect,
    kDataProcess1Src,
    kDataProcess2Src,
    kDataProcess3Src,
    kFloatIntConversions,
    kFloatCompare,
    kFloatDataProcessing1,
    kFloatDataProcessing2,
    kFloatDataProcessing3,
    kFloatImm,
    kFloatCondSelect,
    kLoadStoreReg,
    kLoadStoreAR,
    kLoadExclusive,
    kLoadExclusivePair,
    kStoreExclusive,
    kStoreExclusivePair,
    kLoadPair,
    kStorePair,
    kLoadStoreFloat,
    kLoadPairFloat,
    kStorePairFloat,
    kLoadLiteralReg,
    kLogicalReg,
    kLogicalImm,
    kMoveWide,
    kPCRelAddr,
    kAddPCRelAddr,
    kBrkInsn,
    kSystemInsn,
    kTestBranch,
    kCondBranch,
    kUnknownEncodeType,
};

struct InsnDesc {
    using ImmValidFunc = std::function<bool(const MapleVector<Operand *>)>;
    using SplitFunc = std::function<void(Insn *, bool, InsnBuilder *, OperandBuilder *)>;

    InsnDesc(MOperator op, std::vector<const OpndDesc *> opndmd, uint64 props, uint64 ltype, const std::string &inName,
             const std::string &inFormat, uint32 anum)
        : opc(op),
          opndMD(opndmd),
          properties(props),
          latencyType(ltype),
          name(inName),
          format(inFormat),
          atomicNum(anum) {};

    // for hard-coded machine description.
    InsnDesc(MOperator op, std::vector<const OpndDesc *> opndmd, uint64 props, uint64 ltype, const std::string &inName,
             const std::string &inFormat, uint32 anum, const ImmValidFunc &vFunc, const SplitFunc &sFunc)
        : opc(op),
          opndMD(opndmd),
          properties(props),
          latencyType(ltype),
          name(inName),
          format(inFormat),
          atomicNum(anum),
          validFunc(vFunc),
          splitFunc(sFunc) {};

    // for hard-coded machine description.
    InsnDesc(MOperator op, std::vector<const OpndDesc *> opndmd, uint64 props, uint64 ltype, const std::string &inName,
             const std::string &inFormat, uint32 anum, const ImmValidFunc &vFunc, const SplitFunc &sFunc,
             EncodeType type, uint32 encode)
        : opc(op),
          opndMD(opndmd),
          properties(props),
          latencyType(ltype),
          name(inName),
          format(inFormat),
          atomicNum(anum),
          validFunc(vFunc),
          splitFunc(sFunc),
          encodeType(type),
          mopEncode(encode) {};

    // for aarch64 assemble
    InsnDesc(MOperator op, std::vector<const OpndDesc *> opndmd, uint64 props, uint64 ltype, const std::string &inName,
             const std::string &inFormat, uint32 anum, const ImmValidFunc &vFunc, EncodeType type, uint32 encode)
        : opc(op),
          opndMD(opndmd),
          properties(props),
          latencyType(ltype),
          name(inName),
          format(inFormat),
          atomicNum(anum),
          validFunc(vFunc),
          encodeType(type),
          mopEncode(encode) {};

    // for aarch64 assemble
    InsnDesc(MOperator op, std::vector<const OpndDesc *> opndmd, uint64 props, uint64 ltype, const std::string &inName,
             const std::string &inFormat, uint32 anum, EncodeType type, uint32 encode)
        : opc(op),
          opndMD(opndmd),
          properties(props),
          latencyType(ltype),
          name(inName),
          format(inFormat),
          atomicNum(anum),
          encodeType(type),
          mopEncode(encode) {};

    MOperator opc;
    std::vector<const OpndDesc *> opndMD;
    uint64 properties;
    uint32 latencyType;
    const std::string name;
    const std::string format;
    uint32 atomicNum;                 /* indicate how many asm instructions it will emit. */
    ImmValidFunc validFunc = nullptr; /* If insn has immOperand, this function needs to be implemented. */
    // If insn needs to be split, this function needs to be implemented.
    SplitFunc splitFunc = nullptr;
    EncodeType encodeType = kUnknownEncodeType;
    uint32 mopEncode = 0x00000000;

    bool IsSame(const InsnDesc &left, std::function<bool(const InsnDesc &left, const InsnDesc &right)> cmp) const;

    bool IsCall() const
    {
        return (properties & ISCALL) != 0;
    }
    // call insn does not obey standard call procedure!
    bool IsSpecialCall() const
    {
        return (properties & ISSPCALL) != 0;
    }
    bool IsTailCall() const
    {
        return properties & ISTAILCALL;
    }
    bool IsPhi() const
    {
        return (properties & ISPHI) != 0;
    }
    bool IsPhysicalInsn() const
    {
        return (properties & ISABSTRACT) == 0;
    }
    bool IsStore() const
    {
        return (properties & ISSTORE) != 0;
    }
    bool IsLoad() const
    {
        return (properties & ISLOAD) != 0;
    }
    bool IsConversion() const
    {
        return (properties & ISCONVERSION) != 0;
    }
    bool IsLoadPair() const
    {
        return (properties & (ISLOADPAIR)) != 0;
    }
    bool IsStorePair() const
    {
        return (properties & (ISSTOREPAIR)) != 0;
    }
    bool IsLoadStorePair() const
    {
        return (properties & (ISLOADPAIR | ISSTOREPAIR)) != 0;
    }
    bool IsMove() const
    {
        return (properties & ISMOVE) != 0;
    }
    bool IsDMB() const
    {
        return (properties & (ISDMB)) != 0;
    }
    bool IsBasicOp() const
    {
        return (properties & ISBASICOP) != 0;
    }
    bool IsCondBranch() const
    {
        return (properties & (ISCONDBRANCH)) != 0;
    }
    bool IsUnCondBranch() const
    {
        return (properties & (ISUNCONDBRANCH)) != 0;
    }
    bool IsLoadAddress() const
    {
        return (properties & (ISLOADADDR)) != 0;
    }
    bool IsAtomic() const
    {
        return (properties & ISATOMIC) != 0;
    }

    bool IsCondDef() const
    {
        return (properties & ISCONDDEF) != 0;
    }

    bool IsVolatile() const
    {
        return ((properties & HASRELEASE) != 0) || ((properties & HASACQUIRE) != 0);
    }

    bool IsMemAccessBar() const
    {
        return (properties & (HASRELEASE | HASACQUIRE | HASACQUIRERCPC | HASLOACQUIRE | HASLORELEASE)) != 0;
    }

    bool IsMemAccess() const
    {
        return (properties & (ISLOAD | ISSTORE | ISLOADPAIR | ISSTOREPAIR)) != 0;
    }

    bool IsBranch() const
    {
        return (properties & (ISCONDBRANCH | ISUNCONDBRANCH)) != 0;
    }

    bool HasLoop() const
    {
        return (properties & HASLOOP) != 0;
    }

    bool CanThrow() const
    {
        return (properties & CANTHROW) != 0;
    }

    bool IsInlineAsm() const
    {
        return properties & INLINEASM;
    }

    bool IsSpecialIntrinsic() const
    {
        return properties & SPINTRINSIC;
    }

    bool IsIntrinsic() const
    {
        return properties & ISINTRINSIC;
    }

    MOperator GetOpc() const
    {
        return opc;
    }

    bool Verify(const MapleVector<Operand *> &opnds) const
    {
        if (!validFunc) {
            return true;
        }
        if (opnds.size() != opndMD.size()) {
            CHECK_FATAL_FALSE("The size of opnds is wrong.");
        }
        return validFunc(opnds);
    }

    void Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder) const
    {
        if (!splitFunc) {
            return;
        }
        splitFunc(insn, isAfterRegAlloc, insnBuilder, opndBuilder);
    }

    const OpndDesc *GetOpndDes(size_t index) const
    {
        return opndMD[index];
    }

    uint32 GetOpndMDLength() const
    {
        return opndMD.size();
    }

    uint32 GetOperandSize() const
    {
        if (properties & (ISLOAD | ISSTORE)) {
            /* use memory operand */
            return GetOpndDes(1)->GetSize();
        }
        /* use dest operand */
        return GetOpndDes(0)->GetSize();
    }

    bool Is64Bit() const
    {
        return GetOperandSize() == k64BitSize;
    }

    uint32 GetLatencyType() const
    {
        return latencyType;
    }

    bool IsUnaryOp() const
    {
        return (properties & ISUNARYOP) != 0;
    }

    bool IsShift() const
    {
        return (properties & ISSHIFT) != 0;
    }

    const std::string &GetName() const
    {
        return name;
    }

    const std::string &GetFormat() const
    {
        return format;
    }

    uint32 GetAtomicNum() const
    {
        return atomicNum;
    }

    bool IsBreakPoint() const
    {
        return (properties & ISBREAKPOINT) != 0;
    }

    EncodeType GetEncodeType() const
    {
        return encodeType;
    }

    uint32 GetMopEncode() const
    {
        return mopEncode;
    }

    static const InsnDesc &GetAbstractId(MOperator opc)
    {
        DEBUG_ASSERT(opc < abstract::kMopLast, "op must be lower than kMopLast");
        return abstractId[opc];
    }

    static const InsnDesc abstractId[abstract::kMopLast];
};

enum RegAddress : uint32 { kRegHigh = 0x4, kRegLow = 0x8 };
constexpr uint32 kMemLow12 = 0x10;
constexpr uint32 kLiteralLow12 = kMemLow12;
constexpr uint32 kPreInc = 0x20;
constexpr uint32 kPostInc = 0x40;
constexpr uint32 kLoadLiteral = 0x80;

enum BitIndex : maple::uint8 {
    k8BitIndex = 0,
    k16BitIndex,
    k32BitIndex,
    k64BitIndex,
    kBitIndexEnd,
};

static inline BitIndex GetBitIndex(uint32 bitSize)
{
    switch (bitSize) {
        case k8BitSize:
            return k8BitIndex;
        case k16BitSize:
            return k16BitIndex;
        case k32BitSize:
            return k32BitIndex;
        case k64BitSize:
            return k64BitIndex;
        default:
            CHECK_FATAL(false, "NIY, Not support size");
    }
}
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ISA_H */
