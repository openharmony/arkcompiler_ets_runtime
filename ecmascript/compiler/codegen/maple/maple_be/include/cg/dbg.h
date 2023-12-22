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

#ifndef MAPLEBE_INCLUDE_CG_DBG_H
#define MAPLEBE_INCLUDE_CG_DBG_H

#include "insn.h"
#include "mempool_allocator.h"
#include "mir_symbol.h"
#include "debug_info.h"

namespace mpldbg {
using namespace maple;

/* https://sourceware.org/binutils/docs-2.28/as/Loc.html */
enum LocOpt { kBB, kProEnd, kEpiBeg, kIsStmt, kIsa, kDisc };

enum DbgOpcode : uint8 {
#define DBG_DEFINE(k, sub, n, o0, o1, o2) OP_DBG_##k##sub,
#define ARM_DIRECTIVES_DEFINE(k, sub, n, o0, o1, o2) OP_ARM_DIRECTIVES_##k##sub,
#include "dbg.def"
#undef DBG_DEFINE
#undef ARM_DIRECTIVES_DEFINE
    kOpDbgLast
};

class DbgInsn : public maplebe::Insn {
public:
    DbgInsn(MemPool &memPool, maplebe::MOperator op) : Insn(memPool, op) {}

    DbgInsn(const DbgInsn &other) : maplebe::Insn(other) {}

    DbgInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0) : Insn(memPool, op, opnd0) {}

    DbgInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0, maplebe::Operand &opnd1)
        : Insn(memPool, op, opnd0, opnd1)
    {
    }

    DbgInsn(MemPool &memPool, maplebe::MOperator op, maplebe::Operand &opnd0, maplebe::Operand &opnd1,
            maplebe::Operand &opnd2)
        : Insn(memPool, op, opnd0, opnd1, opnd2)
    {
    }

    ~DbgInsn() = default;

    DbgInsn *CloneTree(MapleAllocator &allocator) const override
    {
        auto *insn = allocator.GetMemPool()->New<DbgInsn>(*this);
        insn->DeepClone(*this, allocator);
        return insn;
    }

    bool IsMachineInstruction() const override
    {
        return false;
    }

    void Dump() const override;

#if DEBUG
    void Check() const override;
#endif

    bool IsTargetInsn() const override
    {
        return false;
    }

    bool IsDbgInsn() const override
    {
        return true;
    }

    bool IsDbgLine() const override
    {
        return mOp == OP_DBG_loc;
    }

    bool IsRegDefined(maplebe::regno_t regNO) const override
    {
        CHECK_FATAL(false, "dbg insn do not def regs");
        return false;
    }

    std::set<uint32> GetDefRegs() const override
    {
        CHECK_FATAL(false, "dbg insn do not def regs");
        return std::set<uint32>();
    }

    uint32 GetBothDefUseOpnd() const override
    {
        return maplebe::kInsnMaxOpnd;
    }

    uint32 GetLoc() const;

private:
    DbgInsn &operator=(const DbgInsn &);
};

class ImmOperand : public maplebe::OperandVisitable<ImmOperand> {
public:
    explicit ImmOperand(int64 val) : OperandVisitable(kOpdImmediate, k32BitSize), val(val) {}

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

    void Dump() const override;

    bool Less(const Operand &right) const override
    {
        (void)right;
        return false;
    }

    int64 GetVal() const
    {
        return val;
    }

private:
    int64 val;
};

class DBGOpndEmitVisitor : public maplebe::OperandVisitorBase, public maplebe::OperandVisitor<ImmOperand> {
public:
    explicit DBGOpndEmitVisitor(maplebe::Emitter &asmEmitter) : emitter(asmEmitter) {}
    virtual ~DBGOpndEmitVisitor() = default;

protected:
    maplebe::Emitter &emitter;

private:
    void Visit(ImmOperand *v) final;
};

} /* namespace mpldbg */

#endif /* MAPLEBE_INCLUDE_CG_DBG_H */
