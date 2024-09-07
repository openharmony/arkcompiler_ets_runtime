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

#ifndef MAPLEBE_INCLUDE_CG_CFI_H
#define MAPLEBE_INCLUDE_CG_CFI_H

#include "insn.h"
#include "mempool_allocator.h"
#include "mir_symbol.h"
#include "operand.h"
#include "common_utils.h"

/*
 * Reference:
 * GNU Binutils. AS documentation
 * https://sourceware.org/binutils/docs-2.28/as/index.html
 *
 * CFI blog
 * https://www.imperialviolet.org/2017/01/18/cfi.html
 *
 * System V Application Binary Interface
 * AMD64 Architecture Processor Supplement. Draft Version 0.99.7
 * https://www.uclibc.org/docs/psABI-x86_64.pdf $ 3.7 Figure 3.36
 * (RBP->6, RSP->7)
 *
 * System V Application Binary Interface
 * Inte386 Architecture Processor Supplement. Version 1.0
 * https://www.uclibc.org/docs/psABI-i386.pdf $ 2.5 Table 2.14
 * (EBP->5, ESP->4)
 *
 * DWARF for ARM Architecture (ARM IHI 0040B)
 * infocenter.arm.com/help/topic/com.arm.doc.ihi0040b/IHI0040B_aadwarf.pdf
 * $ 3.1 Table 1
 * (0-15 -> R0-R15)
 */
namespace cfi {
using namespace maple;

enum CfiOpcode : uint8 {
#define CFI_DEFINE(k, sub, n, o0, o1, o2) OP_CFI_##k##sub,
#define ARM_DIRECTIVES_DEFINE(k, sub, n, o0, o1, o2) OP_ARM_DIRECTIVES_##k##sub,
#include "cfi.def"
#undef CFI_DEFINE
#undef ARM_DIRECTIVES_DEFINE
    kOpCfiLast
};

class CfiInsn : public maplebe::Insn {
public:
    CfiInsn(MemPool &memPool, maplebe::MOperator op) : Insn(memPool, op) {}

    CfiInsn(const CfiInsn &other) : maplebe::Insn(other) {}

    CfiInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0) : Insn(memPool, op, opnd0) {}

    CfiInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0, maplebe::Operand &opnd1)
        : Insn(memPool, op, opnd0, opnd1)
    {
    }

    CfiInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0, maplebe::Operand &opnd1,
            maplebe::Operand &opnd2)
        : Insn(memPool, op, opnd0, opnd1, opnd2)
    {
    }

    ~CfiInsn() = default;

    CfiInsn *CloneTree(MapleAllocator &allocator) const override
    {
        auto *insn = allocator.GetMemPool()->New<CfiInsn>(*this);
        insn->DeepClone(*this, allocator);
        return insn;
    }

    bool IsMachineInstruction() const override
    {
        return false;
    }

    void Dump() const override;

    bool CheckMD() const override;

    bool IsCfiInsn() const override
    {
        return true;
    }

    bool IsTargetInsn() const override
    {
        return false;
    }

    bool IsRegDefined(maplebe::regno_t regNO) const override
    {
        CHECK_FATAL(false, "cfi do not def regs");
        return false;
    }

    std::set<uint32> GetDefRegs() const override
    {
        CHECK_FATAL(false, "cfi do not def regs");
        return std::set<uint32>();
    }

private:
    CfiInsn &operator=(const CfiInsn &);
};

class RegOperand : public maplebe::OperandVisitable<RegOperand> {
public:
    RegOperand(uint32 no, uint32 size) : OperandVisitable(kOpdRegister, size), regNO(no) {}

    ~RegOperand() = default;
    using OperandVisitable<RegOperand>::OperandVisitable;

    uint32 GetRegisterNO() const
    {
        return regNO;
    }

    RegOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<RegOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        Operand *opnd = memPool.Clone<RegOperand>(*this);
        return opnd;
    }

    void Dump() const override;

    bool Less(const Operand &right) const override
    {
        (void)right;
        return false;
    }

private:
    uint32 regNO;
};

class ImmOperand : public maplebe::OperandVisitable<ImmOperand> {
public:
    ImmOperand(int64 val, uint32 size) : OperandVisitable(kOpdImmediate, size), val(val) {}

    ~ImmOperand() = default;
    using OperandVisitable<ImmOperand>::OperandVisitable;

    ImmOperand *CloneTree(MapleAllocator &allocator) const override
    {
        // const MIRSymbol is not changed in cg, so we can do shallow copy
        return allocator.GetMemPool()->New<ImmOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        Operand *opnd = memPool.Clone<ImmOperand>(*this);
        return opnd;
    }
    int64 GetValue() const
    {
        return val;
    }

    void Dump() const override;

    bool Less(const Operand &right) const override
    {
        (void)right;
        return false;
    }

private:
    int64 val;
};

class SymbolOperand : public maplebe::OperandVisitable<SymbolOperand> {
public:
    SymbolOperand(maple::MIRSymbol &mirSymbol, uint8 size) : OperandVisitable(kOpdStImmediate, size), symbol(&mirSymbol)
    {
    }
    ~SymbolOperand() = default;
    using OperandVisitable<SymbolOperand>::OperandVisitable;

    SymbolOperand *CloneTree(MapleAllocator &allocator) const override
    {
        // const MIRSymbol is not changed in cg, so we can do shallow copy
        return allocator.GetMemPool()->New<SymbolOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        Operand *opnd = memPool.Clone<SymbolOperand>(*this);
        return opnd;
    }

    bool Less(const Operand &right) const override
    {
        (void)right;
        return false;
    }

    void Dump() const override
    {
        LogInfo::MapleLogger() << "symbol is  : " << symbol->GetName();
    }

private:
    maple::MIRSymbol *symbol;
};

class StrOperand : public maplebe::OperandVisitable<StrOperand> {
public:
    StrOperand(const std::string &str, MemPool &memPool) : OperandVisitable(kOpdString, 0), str(str, &memPool) {}

    ~StrOperand() = default;
    using OperandVisitable<StrOperand>::OperandVisitable;

    StrOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<StrOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        Operand *opnd = memPool.Clone<StrOperand>(*this);
        return opnd;
    }

    bool Less(const Operand &right) const override
    {
        (void)right;
        return false;
    }

    const MapleString &GetStr() const
    {
        return str;
    }

    void Dump() const override;

private:
    const MapleString str;
};

class LabelOperand : public maplebe::OperandVisitable<LabelOperand> {
public:
    LabelOperand(const std::string &parent, LabelIdx labIdx, MemPool &memPool)
        : OperandVisitable(kOpdBBAddress, 0), parentFunc(parent, &memPool), labelIndex(labIdx)
    {
    }

    ~LabelOperand() = default;
    using OperandVisitable<LabelOperand>::OperandVisitable;

    LabelOperand *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<LabelOperand>(*this);
    }

    Operand *Clone(MemPool &memPool) const override
    {
        Operand *opnd = memPool.Clone<LabelOperand>(*this);
        return opnd;
    }

    bool Less(const Operand &right) const override
    {
        (void)right;
        return false;
    }

    void Dump() const override;

    const MapleString &GetParentFunc() const
    {
        return parentFunc;
    }
    LabelIdx GetIabelIdx() const
    {
        return labelIndex;
    };

private:
    const MapleString parentFunc;
    LabelIdx labelIndex;
};

class CFIOpndEmitVisitor
    : public maplebe::OperandVisitorBase,
      public maplebe::OperandVisitors<RegOperand, ImmOperand, SymbolOperand, StrOperand, LabelOperand> {
public:
    explicit CFIOpndEmitVisitor(maplebe::Emitter &asmEmitter) : emitter(asmEmitter) {}
    virtual ~CFIOpndEmitVisitor() = default;

protected:
    maplebe::Emitter &emitter;

private:
    void Visit(RegOperand *v) final;
    void Visit(ImmOperand *v) final;
    void Visit(SymbolOperand *v) final;
    void Visit(StrOperand *v) final;
    void Visit(LabelOperand *v) final;
};
} /* namespace cfi */

#endif /* MAPLEBE_INCLUDE_CG_CFI_H */
