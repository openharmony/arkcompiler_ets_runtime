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

#ifndef MAPLE_IR_INCLUDE_MIR_NODES_H
#define MAPLE_IR_INCLUDE_MIR_NODES_H
#include <sstream>
#include <utility>
#include <atomic>
#include "opcodes.h"
#include "opcode_info.h"
#include "mir_type.h"
#include "cmpl.h"
#include "mir_module.h"
#include "mir_const.h"
#include "maple_string.h"
#include "src_position.h"
#include "ptr_list_ref.h"

namespace maple {
constexpr size_t kFirstOpnd = 0;
constexpr size_t kSecondOpnd = 1;
constexpr size_t kThirdOpnd = 2;
constexpr size_t kFourthOpnd = 3;
constexpr size_t kOpndNum = 3;

extern MIRModule *theMIRModule;
extern void EmitStr(const MapleString &mplStr);

class MIRPregTable;  // circular dependency exists, no other choice
class TypeTable;     // circular dependency exists, no other choice
class VerifyResult;  // circular dependency exists, no other choice

struct RegFieldPair {
public:
    RegFieldPair() = default;

    RegFieldPair(FieldID fidx, PregIdx pidx) : fieldID(fidx), pregIdx(pidx) {}

    bool IsReg() const
    {
        return pregIdx > 0;
    }

    FieldID GetFieldID() const
    {
        return fieldID;
    }

    PregIdx GetPregIdx() const
    {
        return pregIdx;
    }

    void SetFieldID(FieldID fld)
    {
        fieldID = fld;
    }

    void SetPregIdx(PregIdx idx)
    {
        pregIdx = idx;
    }

private:
    FieldID fieldID = 0;
    PregIdx pregIdx = 0;
};

using CallReturnPair = std::pair<StIdx, RegFieldPair>;
using CallReturnVector = MapleVector<CallReturnPair>;
// Made public so that other modules (such as maplebe) can print intrinsic names
// in debug information or comments in assembly files.
const char *GetIntrinsicName(MIRIntrinsicID intrn);
class BaseNode : public BaseNodeT {
public:
    explicit BaseNode(Opcode o)
    {
        op = o;
        ptyp = kPtyInvalid;
        typeFlag = 0;
        numOpnds = 0;
    }

    BaseNode(Opcode o, uint8 numOpr)
    {
        op = o;
        ptyp = kPtyInvalid;
        typeFlag = 0;
        numOpnds = numOpr;
    }

    BaseNode(const Opcode o, const PrimType typ, uint8 numOpr)
    {
        op = o;
        ptyp = typ;
        typeFlag = 0;
        numOpnds = numOpr;
    }

    virtual ~BaseNode() = default;

    virtual BaseNode *CloneTree(MapleAllocator &allocator) const
    {
        return allocator.GetMemPool()->New<BaseNode>(*this);
    }

    virtual void DumpBase(int32 indent) const;

    virtual void Dump(int32 indent) const
    {
        DumpBase(indent);
    }

    void Dump() const
    {
        Dump(0);
        LogInfo::MapleLogger() << '\n';
    }

    virtual uint8 SizeOfInstr() const
    {
        return kOpcodeInfo.GetTableItemAt(GetOpCode()).instrucSize;
    }

    const char *GetOpName() const;
    bool MayThrowException();
    virtual size_t NumOpnds() const
    {
        return numOpnds;
    }

    virtual BaseNode *Opnd(size_t) const
    {
        DEBUG_ASSERT(0, "override needed");
        return nullptr;
    }

    virtual void SetOpnd(BaseNode *, size_t)
    {
        DEBUG_ASSERT(0, "This should not happen");
    }

    virtual bool IsLeaf() const
    {
        return true;
    }

    virtual CallReturnVector *GetCallReturnVector()
    {
        return nullptr;
    }

    virtual MIRType *GetCallReturnType()
    {
        return nullptr;
    }

    virtual bool IsUnaryNode() const
    {
        return false;
    }

    virtual bool IsBinaryNode() const
    {
        return false;
    }

    virtual bool IsTernaryNode() const
    {
        return false;
    }

    virtual bool IsNaryNode() const
    {
        return false;
    }

    bool IsCondBr() const
    {
        return kOpcodeInfo.IsCondBr(GetOpCode());
    }

    bool IsConstval() const
    {
        return op == OP_constval;
    }

    virtual bool Verify() const
    {
        return true;
    }

    virtual bool Verify(VerifyResult &) const
    {
        return Verify();
    }

    virtual bool IsSSANode() const
    {
        return false;
    }

    virtual bool IsSameContent(const BaseNode *node) const
    {
        return false;
    }
};

class UnaryNode : public BaseNode {
public:
    explicit UnaryNode(Opcode o) : BaseNode(o, 1) {}

    UnaryNode(Opcode o, PrimType typ) : BaseNode(o, typ, 1) {}

    UnaryNode(Opcode o, PrimType typ, BaseNode *expr) : BaseNode(o, typ, 1), uOpnd(expr) {}

    virtual ~UnaryNode() override = default;

    void DumpOpnd(const MIRModule &mod, int32 indent) const;
    void DumpOpnd(int32 indent) const;
    void Dump(int32 indent) const override;
    bool Verify() const override;

    bool Verify(VerifyResult &) const override
    {
        return Verify();
    }

    UnaryNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<UnaryNode>(*this);
        node->SetOpnd(uOpnd->CloneTree(allocator), 0);
        return node;
    }

    BaseNode *Opnd(size_t) const override
    {
        return uOpnd;
    }

    size_t NumOpnds() const override
    {
        return 1;
    }

    void SetOpnd(BaseNode *node, size_t) override
    {
        uOpnd = node;
    }

    bool IsLeaf() const override
    {
        return false;
    }

    bool IsUnaryNode() const override
    {
        return true;
    }

    bool IsSameContent(const BaseNode *node) const override;

private:
    BaseNode *uOpnd = nullptr;
};

class TypeCvtNode : public UnaryNode {
public:
    explicit TypeCvtNode(Opcode o) : UnaryNode(o) {}

    TypeCvtNode(Opcode o, PrimType typ) : UnaryNode(o, typ) {}

    TypeCvtNode(Opcode o, PrimType typ, PrimType fromtyp, BaseNode *expr)
        : UnaryNode(o, typ, expr), fromPrimType(fromtyp)
    {
    }

    virtual ~TypeCvtNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    bool Verify(VerifyResult &) const override
    {
        return Verify();
    }

    TypeCvtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<TypeCvtNode>(*this);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    PrimType FromType() const
    {
        return fromPrimType;
    }

    void SetFromType(PrimType from)
    {
        fromPrimType = from;
    }

    bool IsSameContent(const BaseNode *node) const override;

private:
    PrimType fromPrimType = kPtyInvalid;
};

// used for retype
class RetypeNode : public TypeCvtNode {
public:
    RetypeNode() : TypeCvtNode(OP_retype) {}

    explicit RetypeNode(PrimType typ) : TypeCvtNode(OP_retype, typ) {}

    RetypeNode(PrimType typ, PrimType fromtyp, TyIdx idx, BaseNode *expr)
        : TypeCvtNode(OP_retype, typ, fromtyp, expr), tyIdx(idx)
    {
    }

    virtual ~RetypeNode() = default;
    void Dump(int32 indent) const override;
    bool Verify(VerifyResult &verifyResult) const override;

    RetypeNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<RetypeNode>(*this);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    const TyIdx &GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(const TyIdx tyIdxVal)
    {
        tyIdx = tyIdxVal;
    }

private:
    bool VerifyPrimTypesAndOpnd() const;
    bool CheckFromJarray(const MIRType &from, const MIRType &to, VerifyResult &verifyResult) const;
    bool VerifyCompleteMIRType(const MIRType &from, const MIRType &to, bool isJavaRefType,
                               VerifyResult &verifyResult) const;
    bool VerifyJarrayDimention(const MIRJarrayType &from, const MIRJarrayType &to, VerifyResult &verifyResult) const;
    bool IsJavaAssignable(const MIRType &from, const MIRType &to, VerifyResult &verifyResult) const;

    bool BothPointerOrJarray(const MIRType &from, const MIRType &to) const
    {
        if (from.GetKind() != to.GetKind()) {
            return false;
        }
        return from.IsMIRPtrType() || from.IsMIRJarrayType();
    }

    bool IsInterfaceOrClass(const MIRType &mirType) const
    {
        return mirType.IsMIRClassType() || mirType.IsMIRInterfaceType();
    }

    bool IsJavaRefType(const MIRType &mirType) const
    {
        return mirType.IsMIRJarrayType() || mirType.IsMIRClassType() || mirType.IsMIRInterfaceType();
    }

    TyIdx tyIdx = TyIdx(0);
};

// used for extractbits, sext, zext
class ExtractbitsNode : public UnaryNode {
public:
    explicit ExtractbitsNode(Opcode o) : UnaryNode(o) {}

    ExtractbitsNode(Opcode o, PrimType typ) : UnaryNode(o, typ) {}

    ExtractbitsNode(Opcode o, PrimType typ, uint8 offset, uint8 size)
        : UnaryNode(o, typ), bitsOffset(offset), bitsSize(size)
    {
    }

    ExtractbitsNode(Opcode o, PrimType typ, uint8 offset, uint8 size, BaseNode *expr)
        : UnaryNode(o, typ, expr), bitsOffset(offset), bitsSize(size)
    {
    }

    virtual ~ExtractbitsNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    ExtractbitsNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<ExtractbitsNode>(*this);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    uint8 GetBitsOffset() const
    {
        return bitsOffset;
    }

    void SetBitsOffset(uint8 offset)
    {
        bitsOffset = offset;
    }

    uint8 GetBitsSize() const
    {
        return bitsSize;
    }

    void SetBitsSize(uint8 size)
    {
        bitsSize = size;
    }

private:
    uint8 bitsOffset = 0;
    uint8 bitsSize = 0;
};

class GCMallocNode : public BaseNode {
public:
    explicit GCMallocNode(Opcode o) : BaseNode(o) {}

    GCMallocNode(Opcode o, PrimType typ, TyIdx tIdx) : BaseNode(o, typ, 0), tyIdx(tIdx) {}

    virtual ~GCMallocNode() = default;

    void Dump(int32 indent) const override;

    GCMallocNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<GCMallocNode>(*this);
        return node;
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

    void SetOrigPType(PrimType type)
    {
        origPrimType = type;
    }

private:
    TyIdx tyIdx = TyIdx(0);
    PrimType origPrimType = kPtyInvalid;
};

class JarrayMallocNode : public UnaryNode {
public:
    explicit JarrayMallocNode(Opcode o) : UnaryNode(o) {}

    JarrayMallocNode(Opcode o, PrimType typ) : UnaryNode(o, typ) {}

    JarrayMallocNode(Opcode o, PrimType typ, TyIdx typeIdx) : UnaryNode(o, typ), tyIdx(typeIdx) {}

    JarrayMallocNode(Opcode o, PrimType typ, TyIdx typeIdx, BaseNode *opnd) : UnaryNode(o, typ, opnd), tyIdx(typeIdx) {}

    virtual ~JarrayMallocNode() = default;

    void Dump(int32 indent) const override;

    JarrayMallocNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<JarrayMallocNode>(*this);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

private:
    TyIdx tyIdx = TyIdx(0);
};

// iaddrof also use this node
class IreadNode : public UnaryNode {
public:
    explicit IreadNode(Opcode o) : UnaryNode(o) {}

    IreadNode(Opcode o, PrimType typ) : UnaryNode(o, typ) {}

    IreadNode(Opcode o, PrimType typ, TyIdx typeIdx, FieldID fid) : UnaryNode(o, typ), tyIdx(typeIdx), fieldID(fid) {}

    IreadNode(Opcode o, PrimType typ, TyIdx typeIdx, FieldID fid, BaseNode *expr)
        : UnaryNode(o, typ, expr), tyIdx(typeIdx), fieldID(fid)
    {
    }

    virtual ~IreadNode() = default;
    void Dump(int32 indent) const override;
    bool Verify() const override;

    IreadNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IreadNode>(*this);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    const TyIdx &GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(const TyIdx tyIdxVal)
    {
        tyIdx = tyIdxVal;
    }

    FieldID GetFieldID() const
    {
        return fieldID;
    }

    void SetFieldID(FieldID fieldIDVal)
    {
        fieldID = fieldIDVal;
    }

    bool IsSameContent(const BaseNode *node) const override;

    // the base of an address expr is either a leaf or an iread
    BaseNode &GetAddrExprBase() const
    {
        BaseNode *base = Opnd(0);
        while (base->NumOpnds() != 0 && base->GetOpCode() != OP_iread) {
            base = base->Opnd(0);
        }
        return *base;
    }

    bool IsVolatile() const;

    MIRType *GetType() const;

protected:
    TyIdx tyIdx = TyIdx(0);
    FieldID fieldID = 0;
};

// IaddrofNode has the same member fields and member methods as IreadNode
using IaddrofNode = IreadNode;

class IreadoffNode : public UnaryNode {
public:
    IreadoffNode() : UnaryNode(OP_ireadoff) {}

    IreadoffNode(PrimType ptyp, int32 ofst) : UnaryNode(OP_ireadoff, ptyp), offset(ofst) {}

    IreadoffNode(PrimType ptyp, BaseNode *opnd, int32 ofst) : UnaryNode(OP_ireadoff, ptyp, opnd), offset(ofst) {}

    virtual ~IreadoffNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    IreadoffNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IreadoffNode>(*this);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    int32 GetOffset() const
    {
        return offset;
    }

    void SetOffset(int32 offsetValue)
    {
        offset = offsetValue;
    }

    bool IsSameContent(const BaseNode *node) const override;

private:
    int32 offset = 0;
};

class IreadFPoffNode : public BaseNode {
public:
    IreadFPoffNode() : BaseNode(OP_ireadfpoff) {}

    IreadFPoffNode(PrimType ptyp, int32 ofst) : BaseNode(OP_ireadfpoff, ptyp, 0), offset(ofst) {}

    virtual ~IreadFPoffNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    IreadFPoffNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IreadFPoffNode>(*this);
        return node;
    }

    int32 GetOffset() const
    {
        return offset;
    }

    void SetOffset(int32 offsetValue)
    {
        offset = offsetValue;
    }

    bool IsSameContent(const BaseNode *node) const override;

private:
    int32 offset = 0;
};

class IreadPCoffNode : public IreadFPoffNode {
public:
    IreadPCoffNode(Opcode o, PrimType typ, uint8 numopns)
    {
        op = o;
        ptyp = typ;
        numOpnds = numopns;
    }
    virtual ~IreadPCoffNode() {}
};

typedef IreadPCoffNode AddroffPCNode;

class BinaryOpnds {
public:
    virtual ~BinaryOpnds() = default;

    virtual void Dump(int32 indent) const;

    BaseNode *GetBOpnd(size_t i) const
    {
        CHECK_FATAL(i < kOperandNumBinary, "Invalid operand idx in BinaryOpnds");
        return bOpnd[i];
    }

    void SetBOpnd(BaseNode *node, size_t i)
    {
        CHECK_FATAL(i < kOperandNumBinary, "Invalid operand idx in BinaryOpnds");
        bOpnd[i] = node;
    }

    virtual bool IsSameContent(const BaseNode *node) const;

private:
    BaseNode *bOpnd[kOperandNumBinary];
};

class BinaryNode : public BaseNode, public BinaryOpnds {
public:
    explicit BinaryNode(Opcode o) : BaseNode(o, kOperandNumBinary) {}

    BinaryNode(Opcode o, PrimType typ) : BaseNode(o, typ, kOperandNumBinary) {}

    BinaryNode(Opcode o, PrimType typ, BaseNode *l, BaseNode *r) : BaseNode(o, typ, kOperandNumBinary)
    {
        SetBOpnd(l, 0);
        SetBOpnd(r, 1);
    }

    virtual ~BinaryNode() = default;

    using BaseNode::Dump;
    void Dump(int32 indent) const override;
    bool Verify() const override;

    BinaryNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<BinaryNode>(*this);
        node->SetBOpnd(GetBOpnd(0)->CloneTree(allocator), 0);
        node->SetBOpnd(GetBOpnd(1)->CloneTree(allocator), 1);
        return node;
    }

    bool IsCommutative() const
    {
        switch (GetOpCode()) {
            case OP_add:
            case OP_mul:
            case OP_band:
            case OP_bior:
            case OP_bxor:
            case OP_land:
            case OP_lior:
                return true;
            default:
                return false;
        }
    }

    BaseNode *Opnd(size_t i) const override
    {
        DEBUG_ASSERT(i < kOperandNumBinary, "invalid operand idx in BinaryNode");
        DEBUG_ASSERT(i >= 0, "invalid operand idx in BinaryNode");
        return GetBOpnd(i);
    }

    size_t NumOpnds() const override
    {
        return kOperandNumBinary;
    }

    void SetOpnd(BaseNode *node, size_t i = 0) override
    {
        SetBOpnd(node, i);
    }

    bool IsLeaf() const override
    {
        return false;
    }

    bool IsBinaryNode() const override
    {
        return true;
    }
    bool IsSameContent(const BaseNode *node) const override;
};

class CompareNode : public BinaryNode {
public:
    explicit CompareNode(Opcode o) : BinaryNode(o) {}

    CompareNode(Opcode o, PrimType typ) : BinaryNode(o, typ) {}

    CompareNode(Opcode o, PrimType typ, PrimType otype, BaseNode *l, BaseNode *r)
        : BinaryNode(o, typ, l, r), opndType(otype)
    {
    }

    virtual ~CompareNode() = default;

    using BinaryNode::Dump;
    void Dump(int32 indent) const override;
    bool Verify() const override;

    CompareNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<CompareNode>(*this);
        node->SetBOpnd(GetBOpnd(0)->CloneTree(allocator), 0);
        node->SetBOpnd(GetBOpnd(1)->CloneTree(allocator), 1);
        return node;
    }

    PrimType GetOpndType() const
    {
        return opndType;
    }

    void SetOpndType(PrimType type)
    {
        opndType = type;
    }

private:
    PrimType opndType = kPtyInvalid;  // type of operands.
};

class DepositbitsNode : public BinaryNode {
public:
    DepositbitsNode() : BinaryNode(OP_depositbits) {}

    DepositbitsNode(Opcode o, PrimType typ) : BinaryNode(o, typ) {}

    DepositbitsNode(Opcode o, PrimType typ, uint8 offset, uint8 size, BaseNode *l, BaseNode *r)
        : BinaryNode(o, typ, l, r), bitsOffset(offset), bitsSize(size)
    {
    }

    virtual ~DepositbitsNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    DepositbitsNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<DepositbitsNode>(*this);
        node->SetBOpnd(GetBOpnd(0)->CloneTree(allocator), 0);
        node->SetBOpnd(GetBOpnd(1)->CloneTree(allocator), 1);
        return node;
    }

    uint8 GetBitsOffset() const
    {
        return bitsOffset;
    }

    void SetBitsOffset(uint8 offset)
    {
        bitsOffset = offset;
    }

    uint8 GetBitsSize() const
    {
        return bitsSize;
    }

    void SetBitsSize(uint8 size)
    {
        bitsSize = size;
    }

private:
    uint8 bitsOffset = 0;
    uint8 bitsSize = 0;
};

// used for resolveinterfacefunc, resolvevirtualfunc
// bOpnd[0] stores base vtab/itab address
// bOpnd[1] stores offset
class ResolveFuncNode : public BinaryNode {
public:
    explicit ResolveFuncNode(Opcode o) : BinaryNode(o) {}

    ResolveFuncNode(Opcode o, PrimType typ) : BinaryNode(o, typ) {}

    ResolveFuncNode(Opcode o, PrimType typ, PUIdx idx) : BinaryNode(o, typ), puIdx(idx) {}

    ResolveFuncNode(Opcode o, PrimType typ, PUIdx pIdx, BaseNode *opnd0, BaseNode *opnd1)
        : BinaryNode(o, typ, opnd0, opnd1), puIdx(pIdx)
    {
    }

    virtual ~ResolveFuncNode() = default;

    void Dump(int32 indent) const override;

    ResolveFuncNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<ResolveFuncNode>(*this);
        node->SetBOpnd(GetBOpnd(0)->CloneTree(allocator), 0);
        node->SetBOpnd(GetBOpnd(1)->CloneTree(allocator), 1);
        return node;
    }

    BaseNode *GetTabBaseAddr() const
    {
        return GetBOpnd(0);
    }

    BaseNode *GetOffset() const
    {
        return GetBOpnd(1);
    }

    PUIdx GetPuIdx() const
    {
        return puIdx;
    }

    void SetPUIdx(PUIdx idx)
    {
        puIdx = idx;
    }

private:
    PUIdx puIdx = 0;
};

class TernaryNode : public BaseNode {
public:
    explicit TernaryNode(Opcode o) : BaseNode(o, kOperandNumTernary) {}

    TernaryNode(Opcode o, PrimType typ) : BaseNode(o, typ, kOperandNumTernary) {}

    TernaryNode(Opcode o, PrimType typ, BaseNode *e0, BaseNode *e1, BaseNode *e2) : BaseNode(o, typ, kOperandNumTernary)
    {
        topnd[kFirstOpnd] = e0;
        topnd[kSecondOpnd] = e1;
        topnd[kThirdOpnd] = e2;
    }

    virtual ~TernaryNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    TernaryNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<TernaryNode>(*this);
        node->topnd[kFirstOpnd] = topnd[kFirstOpnd]->CloneTree(allocator);
        node->topnd[kSecondOpnd] = topnd[kSecondOpnd]->CloneTree(allocator);
        node->topnd[kThirdOpnd] = topnd[kThirdOpnd]->CloneTree(allocator);
        return node;
    }

    BaseNode *Opnd(size_t i) const override
    {
        CHECK_FATAL(i < kOperandNumTernary, "array index out of range");
        return topnd[i];
    }

    size_t NumOpnds() const override
    {
        return kOperandNumTernary;
    }

    void SetOpnd(BaseNode *node, size_t i = 0) override
    {
        CHECK_FATAL(i < kOperandNumTernary, "array index out of range");
        topnd[i] = node;
    }

    bool IsLeaf() const override
    {
        return false;
    }

    bool IsTernaryNode() const override
    {
        return true;
    }

private:
    BaseNode *topnd[kOperandNumTernary] = {nullptr, nullptr, nullptr};
};

class NaryOpnds {
public:
    explicit NaryOpnds(MapleAllocator &mpallocter) : nOpnd(mpallocter.Adapter()) {}

    virtual ~NaryOpnds() = default;

    virtual void Dump(int32 indent) const;
    bool VerifyOpnds() const;

    const MapleVector<BaseNode *> &GetNopnd() const
    {
        return nOpnd;
    }

    MapleVector<BaseNode *> &GetNopnd()
    {
        return nOpnd;
    }

    size_t GetNopndSize() const
    {
        return nOpnd.size();
    }

    BaseNode *GetNopndAt(size_t i) const
    {
        CHECK_FATAL(i < nOpnd.size(), "array index out of range");
        return nOpnd[i];
    }

    void SetNOpndAt(size_t i, BaseNode *opnd)
    {
        CHECK_FATAL(i < nOpnd.size(), "array index out of range");
        nOpnd[i] = opnd;
    }

    void SetNOpnd(const MapleVector<BaseNode *> &val)
    {
        nOpnd = val;
    }

private:
    MapleVector<BaseNode *> nOpnd;
};

class MapleValue {
public:
    MapleValue(PregIdx preg) : pregIdx(preg), kind(kPregKind) {}
    MapleValue(MIRSymbol *sym) : symbol(sym), kind(kSymbolKind) {}
    MapleValue(MIRConst *value) : constVal(value), kind(kConstKind) {}
    MapleValue(const MapleValue &val) = default;
    ~MapleValue() = default;

    enum MapleValueKind {
        kPregKind,
        kSymbolKind,
        kConstKind,
    };

    MapleValueKind GetMapleValueKind() const
    {
        return kind;
    }

    const MIRSymbol &GetSymbol() const
    {
        DEBUG_ASSERT(symbol != nullptr, "value is not be initialized with symbol");
        return *symbol;
    }

    PregIdx GetPregIdx() const
    {
        DEBUG_ASSERT(kind == kPregKind, "value is not be initialized with preg");
        return pregIdx;
    }

    const MIRConst &GetConstValue() const
    {
        DEBUG_ASSERT(kind == kConstKind, "value is not be initialized with preg");
        return *constVal;
    }

private:
    PregIdx pregIdx = 0;
    MIRSymbol *symbol = nullptr;
    MIRConst *constVal = nullptr;
    MapleValueKind kind;
};

class DeoptBundleInfo {
public:
    explicit DeoptBundleInfo(MapleAllocator &mpallocter) : deoptBundleInfo(mpallocter.Adapter()) {}

    virtual ~DeoptBundleInfo() = default;

    virtual void Dump(int32 indent) const;

    const MapleUnorderedMap<int32, MapleValue> &GetDeoptBundleInfo() const
    {
        return deoptBundleInfo;
    }

    MapleUnorderedMap<int32, MapleValue> &GetDeoptBundleInfo()
    {
        return deoptBundleInfo;
    }

    void SetDeoptBundleInfo(const MapleUnorderedMap<int32, MapleValue> &vregMap)
    {
        deoptBundleInfo = vregMap;
    }

    void AddDeoptBundleInfo(int32 deoptVreg, MapleValue value)
    {
        deoptBundleInfo.insert(std::pair<int32, MapleValue>(deoptVreg, value));
    }

private:
    MapleUnorderedMap<int32, MapleValue> deoptBundleInfo;
};

class NaryNode : public BaseNode, public NaryOpnds {
public:
    NaryNode(MapleAllocator &allocator, Opcode o) : BaseNode(o), NaryOpnds(allocator) {}

    NaryNode(const MIRModule &mod, Opcode o) : NaryNode(mod.GetCurFuncCodeMPAllocator(), o) {}

    NaryNode(MapleAllocator &allocator, Opcode o, PrimType typ) : BaseNode(o, typ, 0), NaryOpnds(allocator) {}

    NaryNode(const MIRModule &mod, Opcode o, PrimType typ) : NaryNode(mod.GetCurFuncCodeMPAllocator(), o, typ) {}

    NaryNode(MapleAllocator &allocator, const NaryNode &node)
        : BaseNode(node.GetOpCode(), node.GetPrimType(), node.numOpnds), NaryOpnds(allocator)
    {
    }

    NaryNode(const MIRModule &mod, const NaryNode &node) : NaryNode(mod.GetCurFuncCodeMPAllocator(), node) {}

    NaryNode(NaryNode &node) = delete;
    NaryNode &operator=(const NaryNode &node) = delete;
    virtual ~NaryNode() = default;

    void Dump(int32 indent) const override;

    NaryNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<NaryNode>(allocator, *this);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        return node;
    }

    BaseNode *Opnd(size_t i) const override
    {
        return GetNopndAt(i);
    }

    size_t NumOpnds() const override
    {
        DEBUG_ASSERT(numOpnds == GetNopndSize(), "NaryNode has wrong numOpnds field");
        return GetNopndSize();
    }

    void SetOpnd(BaseNode *node, size_t i = 0) override
    {
        DEBUG_ASSERT(i < GetNopnd().size(), "array index out of range");
        SetNOpndAt(i, node);
    }

    bool IsLeaf() const override
    {
        return false;
    }

    bool Verify() const override
    {
        return true;
    }

    bool IsNaryNode() const override
    {
        return true;
    }
};

class IntrinsicopNode : public NaryNode {
public:
    IntrinsicopNode(MapleAllocator &allocator, Opcode o, TyIdx typeIdx = TyIdx())
        : NaryNode(allocator, o), intrinsic(INTRN_UNDEFINED), tyIdx(typeIdx)
    {
    }

    IntrinsicopNode(const MIRModule &mod, Opcode o, TyIdx typeIdx = TyIdx())
        : IntrinsicopNode(mod.GetCurFuncCodeMPAllocator(), o, typeIdx)
    {
    }

    IntrinsicopNode(MapleAllocator &allocator, Opcode o, PrimType typ, TyIdx typeIdx = TyIdx())
        : NaryNode(allocator, o, typ), intrinsic(INTRN_UNDEFINED), tyIdx(typeIdx)
    {
    }

    IntrinsicopNode(const MIRModule &mod, Opcode o, PrimType typ, TyIdx typeIdx = TyIdx())
        : IntrinsicopNode(mod.GetCurFuncCodeMPAllocator(), o, typ, typeIdx)
    {
    }

    IntrinsicopNode(MapleAllocator &allocator, const IntrinsicopNode &node)
        : NaryNode(allocator, node), intrinsic(node.GetIntrinsic()), tyIdx(node.GetTyIdx())
    {
    }

    IntrinsicopNode(const MIRModule &mod, const IntrinsicopNode &node)
        : IntrinsicopNode(mod.GetCurFuncCodeMPAllocator(), node)
    {
    }

    IntrinsicopNode(IntrinsicopNode &node) = delete;
    IntrinsicopNode &operator=(const IntrinsicopNode &node) = delete;
    virtual ~IntrinsicopNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;
    bool Verify(VerifyResult &verifyResult) const override;

    IntrinsicopNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IntrinsicopNode>(allocator, *this);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        node->SetNumOpnds(GetNopndSize());
        return node;
    }

    MIRIntrinsicID GetIntrinsic() const
    {
        return intrinsic;
    }

    void SetIntrinsic(MIRIntrinsicID intrinsicID)
    {
        intrinsic = intrinsicID;
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

    // IntrinDesc query
    const IntrinDesc &GetIntrinDesc() const
    {
        return IntrinDesc::intrinTable[intrinsic];
    }

    bool VerifyJArrayLength(VerifyResult &verifyResult) const;

private:
    MIRIntrinsicID intrinsic;
    TyIdx tyIdx;
};

class ConstvalNode : public BaseNode {
public:
    ConstvalNode() : BaseNode(OP_constval) {}

    explicit ConstvalNode(PrimType typ) : BaseNode(OP_constval, typ, 0) {}

    explicit ConstvalNode(MIRConst *constv) : BaseNode(OP_constval), constVal(constv) {}

    ConstvalNode(PrimType typ, MIRConst *constv) : BaseNode(OP_constval, typ, 0), constVal(constv) {}
    virtual ~ConstvalNode() = default;
    void Dump(int32 indent) const override;

    ConstvalNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<ConstvalNode>(*this);
    }

    const MIRConst *GetConstVal() const
    {
        return constVal;
    }

    MIRConst *GetConstVal()
    {
        return constVal;
    }

    void SetConstVal(MIRConst *val)
    {
        constVal = val;
    }

    bool IsSameContent(const BaseNode *node) const override;

private:
    MIRConst *constVal = nullptr;
};

class ConststrNode : public BaseNode {
public:
    ConststrNode() : BaseNode(OP_conststr) {}

    explicit ConststrNode(UStrIdx i) : BaseNode(OP_conststr), strIdx(i) {}

    ConststrNode(PrimType typ, UStrIdx i) : BaseNode(OP_conststr, typ, 0), strIdx(i) {}

    virtual ~ConststrNode() = default;

    void Dump(int32 indent) const override;
    bool IsSameContent(const BaseNode *node) const override;

    ConststrNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<ConststrNode>(*this);
    }

    UStrIdx GetStrIdx() const
    {
        return strIdx;
    }

    void SetStrIdx(UStrIdx idx)
    {
        strIdx = idx;
    }

private:
    UStrIdx strIdx = UStrIdx(0);
};

class Conststr16Node : public BaseNode {
public:
    Conststr16Node() : BaseNode(OP_conststr16) {}

    explicit Conststr16Node(U16StrIdx i) : BaseNode(OP_conststr16), strIdx(i) {}

    Conststr16Node(PrimType typ, U16StrIdx i) : BaseNode(OP_conststr16, typ, 0), strIdx(i) {}

    virtual ~Conststr16Node() = default;

    void Dump(int32 indent) const override;
    bool IsSameContent(const BaseNode *node) const override;

    Conststr16Node *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<Conststr16Node>(*this);
    }

    U16StrIdx GetStrIdx() const
    {
        return strIdx;
    }

    void SetStrIdx(U16StrIdx idx)
    {
        strIdx = idx;
    }

private:
    U16StrIdx strIdx = U16StrIdx(0);
};

class SizeoftypeNode : public BaseNode {
public:
    SizeoftypeNode() : BaseNode(OP_sizeoftype) {}

    explicit SizeoftypeNode(TyIdx t) : BaseNode(OP_sizeoftype), tyIdx(t) {}

    SizeoftypeNode(PrimType type, TyIdx t) : BaseNode(OP_sizeoftype, type, 0), tyIdx(t) {}

    virtual ~SizeoftypeNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    SizeoftypeNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<SizeoftypeNode>(*this);
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

private:
    TyIdx tyIdx = TyIdx(0);
};

class FieldsDistNode : public BaseNode {
public:
    FieldsDistNode() : BaseNode(OP_fieldsdist) {}

    FieldsDistNode(TyIdx t, FieldID f1, FieldID f2) : BaseNode(OP_fieldsdist), tyIdx(t), fieldID1(f1), fieldID2(f2) {}

    FieldsDistNode(PrimType typ, TyIdx t, FieldID f1, FieldID f2)
        : BaseNode(OP_fieldsdist, typ, 0), tyIdx(t), fieldID1(f1), fieldID2(f2)
    {
    }

    virtual ~FieldsDistNode() = default;

    void Dump(int32 indent) const override;

    FieldsDistNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<FieldsDistNode>(*this);
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

    FieldID GetFieldID1() const
    {
        return fieldID1;
    }

    void SetFiledID1(FieldID id)
    {
        fieldID1 = id;
    }

    FieldID GetFieldID2() const
    {
        return fieldID2;
    }

    void SetFiledID2(FieldID id)
    {
        fieldID2 = id;
    }

private:
    TyIdx tyIdx = TyIdx(0);
    FieldID fieldID1 = 0;
    FieldID fieldID2 = 0;
};

class ArrayNode : public NaryNode {
public:
    ArrayNode(MapleAllocator &allocator) : NaryNode(allocator, OP_array) {}

    explicit ArrayNode(const MIRModule &mod) : ArrayNode(mod.GetCurFuncCodeMPAllocator()) {}

    ArrayNode(MapleAllocator &allocator, PrimType typ, TyIdx idx) : NaryNode(allocator, OP_array, typ), tyIdx(idx) {}

    ArrayNode(const MIRModule &mod, PrimType typ, TyIdx idx) : ArrayNode(mod.GetCurFuncCodeMPAllocator(), typ, idx) {}

    ArrayNode(MapleAllocator &allocator, PrimType typ, TyIdx idx, bool bcheck)
        : NaryNode(allocator, OP_array, typ), tyIdx(idx), boundsCheck(bcheck)
    {
    }

    ArrayNode(const MIRModule &mod, PrimType typ, TyIdx idx, bool bcheck)
        : ArrayNode(mod.GetCurFuncCodeMPAllocator(), typ, idx, bcheck)
    {
    }

    ArrayNode(MapleAllocator &allocator, const ArrayNode &node)
        : NaryNode(allocator, node), tyIdx(node.tyIdx), boundsCheck(node.boundsCheck)
    {
    }

    ArrayNode(const MIRModule &mod, const ArrayNode &node) : ArrayNode(mod.GetCurFuncCodeMPAllocator(), node) {}

    ArrayNode(ArrayNode &node) = delete;
    ArrayNode &operator=(const ArrayNode &node) = delete;
    virtual ~ArrayNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;
    bool IsSameBase(ArrayNode *);

    size_t NumOpnds() const override
    {
        DEBUG_ASSERT(numOpnds == GetNopndSize(), "ArrayNode has wrong numOpnds field");
        return GetNopndSize();
    }

    ArrayNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<ArrayNode>(allocator, *this);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        node->boundsCheck = boundsCheck;
        node->SetNumOpnds(GetNopndSize());
        return node;
    }

    const MIRType *GetArrayType(const TypeTable &tt) const;
    MIRType *GetArrayType(const TypeTable &tt);

    BaseNode *GetIndex(size_t i)
    {
        return Opnd(i + 1);
    }

    const BaseNode *GetDim(const MIRModule &mod, TypeTable &tt, int i) const;
    BaseNode *GetDim(const MIRModule &mod, TypeTable &tt, int i);

    BaseNode *GetBase()
    {
        return Opnd(0);
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

    bool GetBoundsCheck() const
    {
        return boundsCheck;
    }

    void SetBoundsCheck(bool check)
    {
        boundsCheck = check;
    }

private:
    TyIdx tyIdx;
    bool boundsCheck = true;
};

class AddrofNode : public BaseNode {
public:
    explicit AddrofNode(Opcode o) : BaseNode(o), stIdx() {}

    AddrofNode(Opcode o, PrimType typ) : AddrofNode(o, typ, StIdx(), 0) {}

    AddrofNode(Opcode o, PrimType typ, StIdx sIdx, FieldID fid) : BaseNode(o, typ, 0), stIdx(sIdx), fieldID(fid) {}

    virtual ~AddrofNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;
    bool CheckNode(const MIRModule &mod) const;

    AddrofNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<AddrofNode>(*this);
    }

    StIdx GetStIdx() const
    {
        return stIdx;
    }

    void SetStIdx(StIdx idx)
    {
        stIdx = idx;
    }

    void SetStFullIdx(uint32 idx)
    {
        stIdx.SetFullIdx(idx);
    }

    FieldID GetFieldID() const
    {
        return fieldID;
    }

    void SetFieldID(FieldID fieldIDVal)
    {
        fieldID = fieldIDVal;
    }

    bool IsVolatile(const MIRModule &mod) const;

    bool IsSameContent(const BaseNode *node) const override;

private:
    StIdx stIdx;
    FieldID fieldID = 0;
};

// DreadNode has the same member fields and member methods as AddrofNode
using DreadNode = AddrofNode;

class DreadoffNode : public BaseNode {
public:
    explicit DreadoffNode(Opcode o) : BaseNode(o), stIdx() {}

    DreadoffNode(Opcode o, PrimType typ) : BaseNode(o, typ, 0), stIdx() {}

    virtual ~DreadoffNode() = default;

    void Dump(int32 indent) const override;

    DreadoffNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<DreadoffNode>(*this);
    }

    bool IsVolatile(const MIRModule &mod) const;

    bool IsSameContent(const BaseNode *node) const override;

public:
    StIdx stIdx;
    int32 offset = 0;
};

// AddrofoffNode has the same member fields and member methods as DreadoffNode
using AddrofoffNode = DreadoffNode;

class RegreadNode : public BaseNode {
public:
    RegreadNode() : BaseNode(OP_regread) {}

    explicit RegreadNode(PregIdx pIdx) : BaseNode(OP_regread), regIdx(pIdx) {}

    RegreadNode(PrimType primType, PregIdx pIdx) : RegreadNode(pIdx)
    {
        ptyp = primType;
    }

    virtual ~RegreadNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    RegreadNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<RegreadNode>(*this);
    }

    PregIdx GetRegIdx() const
    {
        return regIdx;
    }
    void SetRegIdx(PregIdx reg)
    {
        regIdx = reg;
    }

    bool IsSameContent(const BaseNode *node) const override;

private:
    PregIdx regIdx = 0;  // 32bit, negative if special register
};

class AddroffuncNode : public BaseNode {
public:
    AddroffuncNode() : BaseNode(OP_addroffunc) {}

    AddroffuncNode(PrimType typ, PUIdx pIdx) : BaseNode(OP_addroffunc, typ, 0), puIdx(pIdx) {}

    virtual ~AddroffuncNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    AddroffuncNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<AddroffuncNode>(*this);
    }

    PUIdx GetPUIdx() const
    {
        return puIdx;
    }

    void SetPUIdx(PUIdx puIdxValue)
    {
        puIdx = puIdxValue;
    }

    bool IsSameContent(const BaseNode *node) const override;

private:
    PUIdx puIdx = 0;  // 32bit now
};

class AddroflabelNode : public BaseNode {
public:
    AddroflabelNode() : BaseNode(OP_addroflabel) {}

    explicit AddroflabelNode(uint32 ofst) : BaseNode(OP_addroflabel), offset(ofst) {}

    virtual ~AddroflabelNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    AddroflabelNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<AddroflabelNode>(*this);
    }

    uint32 GetOffset() const
    {
        return offset;
    }

    void SetOffset(uint32 offsetValue)
    {
        offset = offsetValue;
    }

    bool IsSameContent(const BaseNode *node) const override;

private:
    LabelIdx offset = 0;
};

// for cleanuptry, jscatch, finally, retsub, endtry, membaracquire, membarrelease,
// membarstoreload, membarstorestore
class StmtNode : public BaseNode, public PtrListNodeBase<StmtNode> {
public:
    static std::atomic<uint32> stmtIDNext;  // for assigning stmtID, initialized to 1; 0 is reserved
    static uint32 lastPrintedLineNum;       // used during printing ascii output
    static uint16 lastPrintedColumnNum;

    explicit StmtNode(Opcode o) : BaseNode(o), PtrListNodeBase(), stmtID(stmtIDNext), stmtOriginalID(stmtIDNext)
    {
        ++stmtIDNext;
    }

    StmtNode(Opcode o, uint8 numOpr)
        : BaseNode(o, numOpr), PtrListNodeBase(), stmtID(stmtIDNext), stmtOriginalID(stmtIDNext)
    {
        ++stmtIDNext;
    }

    StmtNode(Opcode o, PrimType typ, uint8 numOpr)
        : BaseNode(o, typ, numOpr), PtrListNodeBase(), stmtID(stmtIDNext), stmtOriginalID(stmtIDNext)
    {
        ++stmtIDNext;
    }

    // used for NaryStmtNode when clone
    StmtNode(Opcode o, PrimType typ, uint8 numOpr, const SrcPosition &srcPosition, uint32 stmtOriginalID,
             StmtAttrs attrs)
        : BaseNode(o, typ, numOpr),
          PtrListNodeBase(),
          srcPosition(srcPosition),
          stmtID(stmtIDNext),
          stmtOriginalID(stmtOriginalID),
          stmtAttrs(attrs)
    {
        ++stmtIDNext;
    }

    virtual ~StmtNode() = default;

    using BaseNode::Dump;
    void DumpBase(int32 indent) const override;
    void Dump(int32 indent) const override;
    void InsertAfterThis(StmtNode &pos);
    void InsertBeforeThis(StmtNode &pos);

    virtual StmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *s = allocator.GetMemPool()->New<StmtNode>(*this);
        s->SetStmtID(stmtIDNext++);
        s->SetMeStmtID(meStmtID);
        return s;
    }

    virtual bool Verify() const override
    {
        return true;
    }

    virtual bool Verify(VerifyResult &) const override
    {
        return Verify();
    }

    const SrcPosition &GetSrcPos() const
    {
        return srcPosition;
    }

    SrcPosition &GetSrcPos()
    {
        return srcPosition;
    }

    void SetSrcPos(SrcPosition pos)
    {
        srcPosition = pos;
    }

    uint32 GetStmtID() const
    {
        return stmtID;
    }

    void SetStmtID(uint32 id)
    {
        stmtID = id;
    }

    uint32 GetOriginalID() const
    {
        return stmtOriginalID;
    }

    void SetOriginalID(uint32 id)
    {
        stmtOriginalID = id;
    }

    uint32 GetMeStmtID() const
    {
        return meStmtID;
    }

    void SetMeStmtID(uint32 id)
    {
        meStmtID = id;
    }

    StmtNode *GetRealNext() const;

    virtual BaseNode *GetRHS() const
    {
        return nullptr;
    }

    bool GetIsLive() const
    {
        return isLive;
    }

    void SetIsLive(bool live) const
    {
        isLive = live;
    }

    bool IsInSafeRegion() const
    {
        return stmtAttrs.GetAttr(STMTATTR_insaferegion);
    }

    void SetInSafeRegion()
    {
        stmtAttrs.SetAttr(STMTATTR_insaferegion);
    }

    void CopySafeRegionAttr(const StmtAttrs &stmtAttr)
    {
        this->stmtAttrs.AppendAttr(stmtAttr.GetTargetAttrFlag(STMTATTR_insaferegion));
    }

    void SetMayTailcall(bool flag = true)
    {
        stmtAttrs.SetAttr(STMTATTR_mayTailcall, flag);
    }

    bool GetMayTailCall() const
    {
        return stmtAttrs.GetAttr(STMTATTR_mayTailcall);
    }

    const StmtAttrs &GetStmtAttrs() const
    {
        return stmtAttrs;
    }

    void SetAttr(StmtAttrKind x)
    {
        stmtAttrs.SetAttr(x);
    }

    bool GetAttr(StmtAttrKind x) const
    {
        return stmtAttrs.GetAttr(x);
    }

    void SetStmtAttrs(StmtAttrs stmtAttrs_)
    {
        stmtAttrs = stmtAttrs_;
    }

protected:
    SrcPosition srcPosition;

private:
    uint32 stmtID;                // a unique ID assigned to it
    uint32 stmtOriginalID;        // first define id, no change when clone, need copy when emit from MeStmt
    uint32 meStmtID = 0;          // Need copy when emit from MeStmt, attention:this just for two stmt(if && call)
    mutable bool isLive = false;  // only used for dse to save compile time
                                  // mutable to keep const-ness at most situation
    StmtAttrs stmtAttrs;
};

class IassignNode : public StmtNode {
public:
    IassignNode() : IassignNode(TyIdx(0), 0, nullptr, nullptr) {}

    IassignNode(TyIdx tyIdx, FieldID fieldID, BaseNode *addrOpnd, BaseNode *rhsOpnd)
        : StmtNode(OP_iassign), tyIdx(tyIdx), fieldID(fieldID), addrExpr(addrOpnd), rhs(rhsOpnd)
    {
        BaseNodeT::SetNumOpnds(kOperandNumBinary);
    }

    virtual ~IassignNode() = default;

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

    FieldID GetFieldID() const
    {
        return fieldID;
    }

    void SetFieldID(FieldID fid)
    {
        fieldID = fid;
    }

    BaseNode *Opnd(size_t i) const override
    {
        if (i == 0) {
            return addrExpr;
        }
        return rhs;
    }

    size_t NumOpnds() const override
    {
        return kOperandNumBinary;
    }

    void SetOpnd(BaseNode *node, size_t i) override
    {
        if (i == 0) {
            addrExpr = node;
        } else {
            rhs = node;
        }
    }

    void Dump(int32 indent) const override;
    bool Verify() const override;

    IassignNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *bn = allocator.GetMemPool()->New<IassignNode>(*this);
        bn->SetStmtID(stmtIDNext++);
        bn->SetOpnd(addrExpr->CloneTree(allocator), 0);
        bn->SetRHS(rhs->CloneTree(allocator));
        return bn;
    }

    // the base of an address expr is either a leaf or an iread
    BaseNode &GetAddrExprBase() const
    {
        BaseNode *base = addrExpr;
        while (base->NumOpnds() != 0 && base->GetOpCode() != OP_iread) {
            base = base->Opnd(0);
        }
        return *base;
    }

    void SetAddrExpr(BaseNode *exp)
    {
        addrExpr = exp;
    }

    BaseNode *GetRHS() const override
    {
        return rhs;
    }

    void SetRHS(BaseNode *node)
    {
        rhs = node;
    }

    bool AssigningVolatile() const;

private:
    TyIdx tyIdx;
    FieldID fieldID;

public:
    BaseNode *addrExpr;
    BaseNode *rhs;
};

// goto and gosub
class GotoNode : public StmtNode {
public:
    explicit GotoNode(Opcode o) : StmtNode(o) {}

    GotoNode(Opcode o, uint32 ofst) : StmtNode(o), offset(ofst) {}

    virtual ~GotoNode() = default;

    void Dump(int32 indent) const override;

    GotoNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *g = allocator.GetMemPool()->New<GotoNode>(*this);
        g->SetStmtID(stmtIDNext++);
        return g;
    }

    uint32 GetOffset() const
    {
        return offset;
    }

    void SetOffset(uint32 o)
    {
        offset = o;
    }

private:
    uint32 offset = 0;
};

// jstry
class JsTryNode : public StmtNode {
public:
    JsTryNode() : StmtNode(OP_jstry) {}

    JsTryNode(uint16 catchofst, uint16 finallyofset)
        : StmtNode(OP_jstry), catchOffset(catchofst), finallyOffset(finallyofset)
    {
    }

    virtual ~JsTryNode() = default;

    void Dump(int32 indent) const override;

    JsTryNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *t = allocator.GetMemPool()->New<JsTryNode>(*this);
        t->SetStmtID(stmtIDNext++);
        return t;
    }

    uint16 GetCatchOffset() const
    {
        return catchOffset;
    }

    void SetCatchOffset(uint32 offset)
    {
        catchOffset = offset;
    }

    uint16 GetFinallyOffset() const
    {
        return finallyOffset;
    }

    void SetFinallyOffset(uint32 offset)
    {
        finallyOffset = offset;
    }

private:
    uint16 catchOffset = 0;
    uint16 finallyOffset = 0;
};

// try, cpptry
class TryNode : public StmtNode {
public:
    explicit TryNode(MapleAllocator &allocator) : StmtNode(OP_try), offsets(allocator.Adapter()) {}

    explicit TryNode(const MapleVector<LabelIdx> &offsets) : StmtNode(OP_try), offsets(offsets) {}

    explicit TryNode(const MIRModule &mod) : TryNode(mod.GetCurFuncCodeMPAllocator()) {}

    TryNode(TryNode &node) = delete;
    TryNode &operator=(const TryNode &node) = delete;
    virtual ~TryNode() = default;

    using StmtNode::Dump;
    void Dump(int32 indent) const override;

    MapleVector<LabelIdx> &GetOffsets()
    {
        return offsets;
    }

    LabelIdx GetOffset(size_t i) const
    {
        DEBUG_ASSERT(i < offsets.size(), "array index out of range");
        return offsets.at(i);
    }

    void SetOffset(LabelIdx offsetValue, size_t i)
    {
        DEBUG_ASSERT(i < offsets.size(), "array index out of range");
        offsets[i] = offsetValue;
    }

    void AddOffset(LabelIdx offsetValue)
    {
        offsets.push_back(offsetValue);
    }

    void ResizeOffsets(size_t offsetSize)
    {
        offsets.resize(offsetSize);
    }

    void SetOffsets(const MapleVector<LabelIdx> &offsetsValue)
    {
        offsets = offsetsValue;
    }

    size_t GetOffsetsCount() const
    {
        return offsets.size();
    }

    MapleVector<LabelIdx>::iterator GetOffsetsBegin()
    {
        return offsets.begin();
    }

    MapleVector<LabelIdx>::iterator GetOffsetsEnd()
    {
        return offsets.end();
    }

    void OffsetsInsert(MapleVector<LabelIdx>::iterator a, MapleVector<LabelIdx>::iterator b,
                       MapleVector<LabelIdx>::iterator c)
    {
        (void)offsets.insert(a, b, c);
    }

    TryNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<TryNode>(allocator);
        node->SetStmtID(stmtIDNext++);
        for (size_t i = 0; i < offsets.size(); ++i) {
            node->AddOffset(offsets[i]);
        }
        return node;
    }

private:
    MapleVector<LabelIdx> offsets;
};

// catch
class CatchNode : public StmtNode {
public:
    explicit CatchNode(MapleAllocator &allocator) : StmtNode(OP_catch), exceptionTyIdxVec(allocator.Adapter()) {}

    explicit CatchNode(const MapleVector<TyIdx> &tyIdxVec) : StmtNode(OP_catch), exceptionTyIdxVec(tyIdxVec) {}

    explicit CatchNode(const MIRModule &mod) : CatchNode(mod.GetCurFuncCodeMPAllocator()) {}

    CatchNode(CatchNode &node) = delete;
    CatchNode &operator=(const CatchNode &node) = delete;
    virtual ~CatchNode() = default;

    using StmtNode::Dump;
    void Dump(int32 indent) const override;

    TyIdx GetExceptionTyIdxVecElement(size_t i) const
    {
        CHECK_FATAL(i < exceptionTyIdxVec.size(), "array index out of range");
        return exceptionTyIdxVec[i];
    }

    const MapleVector<TyIdx> &GetExceptionTyIdxVec() const
    {
        return exceptionTyIdxVec;
    }

    size_t Size() const
    {
        return exceptionTyIdxVec.size();
    }

    void SetExceptionTyIdxVecElement(TyIdx idx, size_t i)
    {
        CHECK_FATAL(i < exceptionTyIdxVec.size(), "array index out of range");
        exceptionTyIdxVec[i] = idx;
    }

    void SetExceptionTyIdxVec(MapleVector<TyIdx> vec)
    {
        exceptionTyIdxVec = vec;
    }

    void PushBack(TyIdx idx)
    {
        exceptionTyIdxVec.push_back(idx);
    }

    CatchNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<CatchNode>(allocator);
        node->SetStmtID(stmtIDNext++);
        for (size_t i = 0; i < Size(); ++i) {
            node->PushBack(GetExceptionTyIdxVecElement(i));
        }
        return node;
    }

private:
    MapleVector<TyIdx> exceptionTyIdxVec;
};

// cppcatch
class CppCatchNode : public StmtNode {
public:
    explicit CppCatchNode(const TyIdx &idx) : StmtNode(OP_cppcatch), exceptionTyIdx(idx) {}
    explicit CppCatchNode() : CppCatchNode(TyIdx(0)) {}

    explicit CppCatchNode(const CppCatchNode &node) = delete;
    CppCatchNode &operator=(const CppCatchNode &node) = delete;
    ~CppCatchNode() = default;

    void Dump(int32 indent) const override;

    CppCatchNode *CloneTree(MapleAllocator &allocator) const override
    {
        CppCatchNode *node = allocator.GetMemPool()->New<CppCatchNode>();
        node->SetStmtID(stmtIDNext++);
        node->exceptionTyIdx = exceptionTyIdx;
        return node;
    }

    CppCatchNode *CloneTree(const MIRModule &mod) const
    {
        return CppCatchNode::CloneTree(*mod.CurFuncCodeMemPoolAllocator());
    }

public:
    TyIdx exceptionTyIdx;
};

using CasePair = std::pair<int64, LabelIdx>;
using CaseVector = MapleVector<CasePair>;
class SwitchNode : public StmtNode {
public:
    explicit SwitchNode(MapleAllocator &allocator) : StmtNode(OP_switch, 1), switchTable(allocator.Adapter()) {}

    explicit SwitchNode(const MIRModule &mod) : SwitchNode(mod.GetCurFuncCodeMPAllocator()) {}

    SwitchNode(MapleAllocator &allocator, LabelIdx label) : SwitchNode(allocator, label, nullptr) {}

    SwitchNode(MapleAllocator &allocator, LabelIdx label, BaseNode *opnd)
        : StmtNode(OP_switch, 1), switchOpnd(opnd), defaultLabel(label), switchTable(allocator.Adapter())
    {
    }

    SwitchNode(const MIRModule &mod, LabelIdx label) : SwitchNode(mod.GetCurFuncCodeMPAllocator(), label) {}

    SwitchNode(MapleAllocator &allocator, const SwitchNode &node)
        : StmtNode(node.GetOpCode(), node.GetPrimType(), node.numOpnds),
          defaultLabel(node.GetDefaultLabel()),
          switchTable(allocator.Adapter())
    {
    }

    SwitchNode(const MIRModule &mod, const SwitchNode &node) : SwitchNode(mod.GetCurFuncCodeMPAllocator(), node) {}

    SwitchNode(SwitchNode &node) = delete;
    SwitchNode &operator=(const SwitchNode &node) = delete;
    virtual ~SwitchNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    SwitchNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<SwitchNode>(allocator, *this);
        node->SetSwitchOpnd(switchOpnd->CloneTree(allocator));
        for (size_t i = 0; i < switchTable.size(); ++i) {
            node->InsertCasePair(switchTable[i]);
        }
        return node;
    }

    BaseNode *Opnd(size_t) const override
    {
        return switchOpnd;
    }

    void SetOpnd(BaseNode *node, size_t) override
    {
        switchOpnd = node;
    }

    BaseNode *GetSwitchOpnd() const
    {
        return switchOpnd;
    }

    void SetSwitchOpnd(BaseNode *node)
    {
        switchOpnd = node;
    }

    LabelIdx GetDefaultLabel() const
    {
        return defaultLabel;
    }

    void SetDefaultLabel(LabelIdx idx)
    {
        defaultLabel = idx;
    }

    const CaseVector &GetSwitchTable() const
    {
        return switchTable;
    }

    CaseVector &GetSwitchTable()
    {
        return switchTable;
    }

    CasePair GetCasePair(size_t idx) const
    {
        DEBUG_ASSERT(idx < switchTable.size(), "out of range in SwitchNode::GetCasePair");
        return switchTable.at(idx);
    }

    void SetSwitchTable(CaseVector vec)
    {
        switchTable = vec;
    }

    void InsertCasePair(CasePair pair)
    {
        switchTable.push_back(pair);
    }

    void UpdateCaseLabelAt(size_t i, LabelIdx idx)
    {
        switchTable[i] = std::make_pair(switchTable[i].first, idx);
    }

    void SortCasePair(bool func(const CasePair &, const CasePair &))
    {
        std::sort(switchTable.begin(), switchTable.end(), func);
    }

private:
    BaseNode *switchOpnd = nullptr;
    LabelIdx defaultLabel = 0;
    CaseVector switchTable;
};

using MCasePair = std::pair<BaseNode *, LabelIdx>;
using MCaseVector = MapleVector<MCasePair>;
class MultiwayNode : public StmtNode {
public:
    explicit MultiwayNode(MapleAllocator &allocator) : StmtNode(OP_multiway, 1), multiWayTable(allocator.Adapter()) {}

    explicit MultiwayNode(const MIRModule &mod) : MultiwayNode(mod.GetCurFuncCodeMPAllocator()) {}

    MultiwayNode(MapleAllocator &allocator, LabelIdx label)
        : StmtNode(OP_multiway, 1), defaultLabel(label), multiWayTable(allocator.Adapter())
    {
    }

    MultiwayNode(const MIRModule &mod, LabelIdx label) : MultiwayNode(mod.GetCurFuncCodeMPAllocator(), label) {}

    MultiwayNode(MapleAllocator &allocator, const MultiwayNode &node)
        : StmtNode(node.GetOpCode(), node.GetPrimType(), node.numOpnds, node.GetSrcPos(), node.GetOriginalID(),
                   node.GetStmtAttrs()),
          defaultLabel(node.defaultLabel),
          multiWayTable(allocator.Adapter())
    {
    }

    MultiwayNode(const MIRModule &mod, const MultiwayNode &node) : MultiwayNode(mod.GetCurFuncCodeMPAllocator(), node)
    {
    }

    MultiwayNode(MultiwayNode &node) = delete;
    MultiwayNode &operator=(const MultiwayNode &node) = delete;
    virtual ~MultiwayNode() = default;

    void Dump(int32 indent) const override;

    MultiwayNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *nd = allocator.GetMemPool()->New<MultiwayNode>(allocator, *this);
        nd->multiWayOpnd = static_cast<BaseNode *>(multiWayOpnd->CloneTree(allocator));
        for (size_t i = 0; i < multiWayTable.size(); ++i) {
            BaseNode *node = multiWayTable[i].first->CloneTree(allocator);
            MCasePair pair(static_cast<BaseNode *>(node), multiWayTable[i].second);
            nd->multiWayTable.push_back(pair);
        }
        return nd;
    }

    BaseNode *Opnd(size_t i) const override
    {
        return *(&multiWayOpnd + static_cast<uint32>(i));
    }

    const BaseNode *GetMultiWayOpnd() const
    {
        return multiWayOpnd;
    }

    void SetMultiWayOpnd(BaseNode *multiwayOpndPara)
    {
        multiWayOpnd = multiwayOpndPara;
    }

    void SetDefaultlabel(LabelIdx defaultLabelPara)
    {
        defaultLabel = defaultLabelPara;
    }

    void AppendElemToMultiWayTable(const MCasePair &mCasrPair)
    {
        multiWayTable.push_back(mCasrPair);
    }

    const MCaseVector &GetMultiWayTable() const
    {
        return multiWayTable;
    }

private:
    BaseNode *multiWayOpnd = nullptr;
    LabelIdx defaultLabel = 0;
    MCaseVector multiWayTable;
};

// eval, throw, free, decref, incref, decrefreset, assertnonnull, igoto
class UnaryStmtNode : public StmtNode {
public:
    explicit UnaryStmtNode(Opcode o) : StmtNode(o, 1) {}

    UnaryStmtNode(Opcode o, PrimType typ) : StmtNode(o, typ, 1) {}

    UnaryStmtNode(Opcode o, PrimType typ, BaseNode *opnd) : StmtNode(o, typ, 1), uOpnd(opnd) {}

    virtual ~UnaryStmtNode() = default;

    using StmtNode::Dump;
    void Dump(int32 indent) const override;
    void DumpOpnd(const MIRModule &mod, int32 indent) const;
    void DumpOpnd(int32 indent) const;

    bool Verify() const override
    {
        return uOpnd->Verify();
    }

    bool Verify(VerifyResult &verifyResult) const override
    {
        if (GetOpCode() == OP_throw && !VerifyThrowable(verifyResult)) {
            return false;
        }
        return uOpnd->Verify(verifyResult);
    }

    UnaryStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<UnaryStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(uOpnd->CloneTree(allocator), 0);
        return node;
    }

    bool IsLeaf() const override
    {
        return false;
    }

    BaseNode *GetRHS() const override
    {
        return Opnd(0);
    }

    virtual void SetRHS(BaseNode *rhs)
    {
        this->SetOpnd(rhs, 0);
    }

    BaseNode *Opnd(size_t i = 0) const override
    {
        (void)i;
        return uOpnd;
    }

    void SetOpnd(BaseNode *node, size_t) override
    {
        uOpnd = node;
    }

private:
    bool VerifyThrowable(VerifyResult &verifyResult) const;

    BaseNode *uOpnd = nullptr;
};

// dassign, maydassign
class DassignNode : public UnaryStmtNode {
public:
    DassignNode() : UnaryStmtNode(OP_dassign), stIdx() {}

    explicit DassignNode(PrimType typ) : UnaryStmtNode(OP_dassign, typ), stIdx() {}

    DassignNode(PrimType typ, BaseNode *opnd) : UnaryStmtNode(OP_dassign, typ, opnd), stIdx() {}

    DassignNode(PrimType typ, BaseNode *opnd, StIdx idx, FieldID fieldID)
        : UnaryStmtNode(OP_dassign, typ, opnd), stIdx(idx), fieldID(fieldID)
    {
    }

    DassignNode(BaseNode *opnd, StIdx idx, FieldID fieldID) : DassignNode(kPtyInvalid, opnd, idx, fieldID) {}

    virtual ~DassignNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    DassignNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<DassignNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    size_t NumOpnds() const override
    {
        return 1;
    }

    bool IsIdentityDassign() const
    {
        BaseNode *rhs = GetRHS();
        if (rhs->GetOpCode() != OP_dread) {
            return false;
        }
        auto *dread = static_cast<AddrofNode *>(rhs);
        return (stIdx == dread->GetStIdx());
    }

    BaseNode *GetRHS() const override
    {
        return UnaryStmtNode::GetRHS();
    }

    void SetRHS(BaseNode *rhs) override
    {
        UnaryStmtNode::SetOpnd(rhs, 0);
    }

    StIdx GetStIdx() const
    {
        return stIdx;
    }
    void SetStIdx(StIdx s)
    {
        stIdx = s;
    }

    const FieldID &GetFieldID() const
    {
        return fieldID;
    }

    void SetFieldID(FieldID f)
    {
        fieldID = f;
    }

    bool AssigningVolatile(const MIRModule &mod) const;

private:
    StIdx stIdx;
    FieldID fieldID = 0;
};

class DassignoffNode : public UnaryStmtNode {
public:
    DassignoffNode() : UnaryStmtNode(OP_dassignoff), stIdx() {}

    explicit DassignoffNode(PrimType typ) : UnaryStmtNode(OP_dassignoff, typ), stIdx() {}

    DassignoffNode(PrimType typ, BaseNode *opnd) : UnaryStmtNode(OP_dassignoff, typ, opnd), stIdx() {}

    DassignoffNode(const StIdx &lhsStIdx, int32 dOffset, PrimType rhsType, BaseNode *rhsNode)
        : DassignoffNode(rhsType, rhsNode)
    {
        stIdx = lhsStIdx;
        offset = dOffset;
    }
    virtual ~DassignoffNode() = default;

    void Dump(int32 indent) const override;

    DassignoffNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<DassignoffNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    size_t NumOpnds() const override
    {
        return 1;
    }

    BaseNode *GetRHS() const override
    {
        return UnaryStmtNode::GetRHS();
    }

    void SetRHS(BaseNode *rhs) override
    {
        UnaryStmtNode::SetOpnd(rhs, 0);
    }

public:
    StIdx stIdx;
    int32 offset = 0;
};

class RegassignNode : public UnaryStmtNode {
public:
    RegassignNode() : UnaryStmtNode(OP_regassign) {}

    RegassignNode(PrimType primType, PregIdx idx, BaseNode *opnd)
        : UnaryStmtNode(OP_regassign, primType, opnd), regIdx(idx)
    {
    }

    virtual ~RegassignNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    RegassignNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<RegassignNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    BaseNode *GetRHS() const override
    {
        return UnaryStmtNode::GetRHS();
    }

    void SetRHS(BaseNode *rhs) override
    {
        UnaryStmtNode::SetOpnd(rhs, 0);
    }

    PregIdx GetRegIdx() const
    {
        return regIdx;
    }
    void SetRegIdx(PregIdx idx)
    {
        regIdx = idx;
    }

private:
    PregIdx regIdx = 0;  // 32bit, negative if special register
};

// brtrue and brfalse
class CondGotoNode : public UnaryStmtNode {
public:
    static const int32 probAll;
    explicit CondGotoNode(Opcode o) : CondGotoNode(o, 0, nullptr) {}

    CondGotoNode(Opcode o, uint32 offset, BaseNode *opnd) : UnaryStmtNode(o, kPtyInvalid, opnd), offset(offset)
    {
        BaseNodeT::SetNumOpnds(kOperandNumUnary);
    }

    virtual ~CondGotoNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    uint32 GetOffset() const
    {
        return offset;
    }

    void SetOffset(uint32 offsetValue)
    {
        offset = offsetValue;
    }

    bool IsBranchProbValid() const
    {
        return branchProb > 0 && branchProb < probAll;
    }

    int32 GetBranchProb() const
    {
        return branchProb;
    }

    void SetBranchProb(int32 prob)
    {
        branchProb = prob;
    }

    void ReverseBranchProb()
    {
        if (IsBranchProbValid()) {
            branchProb = probAll - branchProb;
        }
    }

    void InvalidateBranchProb()
    {
        if (IsBranchProbValid()) {
            branchProb = -1;
        }
    }

    CondGotoNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<CondGotoNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

private:
    uint32 offset;
    int32 branchProb = -1;  // branch probability, a negative number indicates that the probability is invalid
};

using SmallCasePair = std::pair<uint16, uint32>;
using SmallCaseVector = MapleVector<SmallCasePair>;
class RangeGotoNode : public UnaryStmtNode {
public:
    explicit RangeGotoNode(MapleAllocator &allocator) : UnaryStmtNode(OP_rangegoto), rangegotoTable(allocator.Adapter())
    {
    }

    explicit RangeGotoNode(const MIRModule &mod) : RangeGotoNode(mod.GetCurFuncCodeMPAllocator()) {}

    RangeGotoNode(MapleAllocator &allocator, const RangeGotoNode &node)
        : UnaryStmtNode(node), tagOffset(node.tagOffset), rangegotoTable(allocator.Adapter())
    {
    }

    RangeGotoNode(const MIRModule &mod, const RangeGotoNode &node)
        : RangeGotoNode(mod.GetCurFuncCodeMPAllocator(), node)
    {
    }

    RangeGotoNode(RangeGotoNode &node) = delete;
    RangeGotoNode &operator=(const RangeGotoNode &node) = delete;
    virtual ~RangeGotoNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;
    RangeGotoNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<RangeGotoNode>(allocator, *this);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        for (size_t i = 0; i < rangegotoTable.size(); ++i) {
            node->rangegotoTable.push_back(rangegotoTable[i]);
        }
        return node;
    }

    const SmallCaseVector &GetRangeGotoTable() const
    {
        return rangegotoTable;
    }

    const SmallCasePair &GetRangeGotoTableItem(size_t i) const
    {
        return rangegotoTable.at(i);
    }

    void SetRangeGotoTable(SmallCaseVector rt)
    {
        rangegotoTable = rt;
    }

    void AddRangeGoto(uint32 tag, LabelIdx idx)
    {
        rangegotoTable.push_back(SmallCasePair(tag, idx));
    }

    int32 GetTagOffset() const
    {
        return tagOffset;
    }

    void SetTagOffset(int32 offset)
    {
        tagOffset = offset;
    }

private:
    int32 tagOffset = 0;
    // add each tag to tagOffset field to get the actual tag values
    SmallCaseVector rangegotoTable;
};

class BlockNode : public StmtNode {
public:
    using StmtNodes = PtrListRef<StmtNode>;

    BlockNode() : StmtNode(OP_block) {}

    ~BlockNode()
    {
        stmtNodeList.clear();
    }

    void AddStatement(StmtNode *stmt);
    void AppendStatementsFromBlock(BlockNode &blk);
    void InsertFirst(StmtNode *stmt);  // Insert stmt as the first
    void InsertLast(StmtNode *stmt);   // Insert stmt as the last
    void ReplaceStmtWithBlock(StmtNode &stmtNode, BlockNode &blk);
    void ReplaceStmt1WithStmt2(const StmtNode *stmtNode1, StmtNode *stmtNode2);
    void RemoveStmt(const StmtNode *stmtNode1);
    void InsertBefore(const StmtNode *stmtNode1, StmtNode *stmtNode2);  // Insert ss2 before ss1 in current block.
    void InsertAfter(const StmtNode *stmtNode1, StmtNode *stmtNode2);   // Insert ss2 after ss1 in current block.
    // insert all the stmts in inblock to the current block after stmt1
    void InsertBlockAfter(BlockNode &inblock, const StmtNode *stmt1);
    void Dump(int32 indent, const MIRSymbolTable *theSymTab, MIRPregTable *thePregTab, bool withInfo, bool isFuncbody,
              MIRFlavor flavor) const;
    bool Verify() const override;
    bool Verify(VerifyResult &verifyResult) const override;

    void Dump(int32 indent) const override
    {
        Dump(indent, nullptr, nullptr, false, false, kFlavorUnknown);
    }

    BlockNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *blk = allocator.GetMemPool()->New<BlockNode>();
        blk->SetStmtID(stmtIDNext++);
        for (auto &stmt : stmtNodeList) {
            StmtNode *newStmt = static_cast<StmtNode *>(stmt.CloneTree(allocator));
            DEBUG_ASSERT(newStmt != nullptr, "null ptr check");
            newStmt->SetPrev(nullptr);
            newStmt->SetNext(nullptr);
            blk->AddStatement(newStmt);
        }
        return blk;
    }

    BlockNode *CloneTreeWithSrcPosition(const MIRModule &mod)
    {
        MapleAllocator &allocator = mod.GetCurFuncCodeMPAllocator();
        auto *blk = allocator.GetMemPool()->New<BlockNode>();
        blk->SetStmtID(stmtIDNext++);
        for (auto &stmt : stmtNodeList) {
            StmtNode *newStmt = static_cast<StmtNode *>(stmt.CloneTree(allocator));
            DEBUG_ASSERT(newStmt != nullptr, "null ptr check");
            newStmt->SetSrcPos(stmt.GetSrcPos());
            newStmt->SetPrev(nullptr);
            newStmt->SetNext(nullptr);
            blk->AddStatement(newStmt);
        }
        return blk;
    }

    BlockNode *CloneTreeWithFreqs(MapleAllocator &allocator, std::unordered_map<uint32_t, uint64_t> &toFreqs,
                                  std::unordered_map<uint32_t, uint64_t> &fromFreqs, uint64_t numer, uint64_t denom,
                                  uint32_t updateOp);

    bool IsEmpty() const
    {
        return stmtNodeList.empty();
    }

    void ResetBlock()
    {
        stmtNodeList.clear();
    }

    StmtNode *GetFirst()
    {
        return &(stmtNodeList.front());
    }

    const StmtNode *GetFirst() const
    {
        return &(stmtNodeList.front());
    }

    void SetFirst(StmtNode *node)
    {
        stmtNodeList.update_front(node);
    }

    StmtNode *GetLast()
    {
        return &(stmtNodeList.back());
    }

    const StmtNode *GetLast() const
    {
        return &(stmtNodeList.back());
    }

    void SetLast(StmtNode *node)
    {
        stmtNodeList.update_back(node);
    }

    StmtNodes &GetStmtNodes()
    {
        return stmtNodeList;
    }

    const StmtNodes &GetStmtNodes() const
    {
        return stmtNodeList;
    }

private:
    StmtNodes stmtNodeList;
};

class IfStmtNode : public UnaryStmtNode {
public:
    IfStmtNode() : UnaryStmtNode(OP_if)
    {
        numOpnds = kOperandNumTernary;
    }

    virtual ~IfStmtNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    IfStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IfStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd()->CloneTree(allocator), 0);
        node->thenPart = thenPart->CloneTree(allocator);
        if (elsePart != nullptr) {
            node->elsePart = elsePart->CloneTree(allocator);
        }
        node->SetMeStmtID(GetMeStmtID());
        return node;
    }

    IfStmtNode *CloneTreeWithFreqs(MapleAllocator &allocator, std::unordered_map<uint32_t, uint64_t> &toFreqs,
                                   std::unordered_map<uint32_t, uint64_t> &fromFreqs, uint64_t numer, uint64_t denom,
                                   uint32_t updateOp)
    {
        auto *node = allocator.GetMemPool()->New<IfStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd()->CloneTree(allocator), 0);
        if (fromFreqs.count(GetStmtID()) > 0) {
            uint64_t oldFreq = fromFreqs[GetStmtID()];
            uint64_t newFreq = numer == 0 ? 0 : (denom > 0 ? (oldFreq * numer / denom) : oldFreq);
            toFreqs[node->GetStmtID()] = (newFreq > 0 || numer == 0) ? newFreq : 1;
            if (updateOp & kUpdateOrigFreq) {
                uint64_t left = ((oldFreq - newFreq) > 0 || oldFreq == 0) ? (oldFreq - newFreq) : 1;
                fromFreqs[GetStmtID()] = left;
            }
        }
        node->thenPart = thenPart->CloneTreeWithFreqs(allocator, toFreqs, fromFreqs, numer, denom, updateOp);
        if (elsePart != nullptr) {
            node->elsePart = elsePart->CloneTreeWithFreqs(allocator, toFreqs, fromFreqs, numer, denom, updateOp);
        }
        node->SetMeStmtID(GetMeStmtID());
        return node;
    }

    BaseNode *Opnd(size_t i = kFirstOpnd) const override
    {
        if (i == kFirstOpnd) {
            return UnaryStmtNode::Opnd(kFirstOpnd);
        } else if (i == kSecondOpnd) {
            return thenPart;
        } else if (i == kThirdOpnd) {
            DEBUG_ASSERT(elsePart != nullptr, "IfStmtNode has wrong numOpnds field, the elsePart is nullptr");
            DEBUG_ASSERT(numOpnds == kOperandNumTernary,
                         "IfStmtNode has wrong numOpnds field, the elsePart is nullptr");
            return elsePart;
        }
        DEBUG_ASSERT(false, "IfStmtNode has wrong numOpnds field: %u", NumOpnds());
        return nullptr;
    }

    BlockNode *GetThenPart() const
    {
        return thenPart;
    }

    void SetThenPart(BlockNode *node)
    {
        thenPart = node;
    }

    BlockNode *GetElsePart() const
    {
        return elsePart;
    }

    void SetElsePart(BlockNode *node)
    {
        elsePart = node;
    }

    size_t NumOpnds() const override
    {
        if (elsePart == nullptr) {
            return kOperandNumBinary;
        }
        return kOperandNumTernary;
    }

private:
    BlockNode *thenPart = nullptr;
    BlockNode *elsePart = nullptr;
};

// for both while and dowhile
class WhileStmtNode : public UnaryStmtNode {
public:
    explicit WhileStmtNode(Opcode o) : UnaryStmtNode(o)
    {
        BaseNodeT::SetNumOpnds(kOperandNumBinary);
    }

    virtual ~WhileStmtNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    WhileStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<WhileStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        node->body = body->CloneTree(allocator);
        return node;
    }

    WhileStmtNode *CloneTreeWithFreqs(MapleAllocator &allocator, std::unordered_map<uint32_t, uint64_t> &toFreqs,
                                      std::unordered_map<uint32_t, uint64_t> &fromFreqs, uint64_t numer, uint64_t denom,
                                      uint32_t updateOp)
    {
        auto *node = allocator.GetMemPool()->New<WhileStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        if (fromFreqs.count(GetStmtID()) > 0) {
            int64_t oldFreq = static_cast<int64_t>(fromFreqs[GetStmtID()]);
            int64_t newFreq = static_cast<int64_t>(numer == 0 ? 0 : (denom > 0 ? (oldFreq * numer / denom) : oldFreq));
            toFreqs[node->GetStmtID()] = (newFreq > 0 || numer == 0) ? static_cast<uint64_t>(newFreq) : 1;
            if (updateOp & kUpdateOrigFreq) {
                int64_t left = (oldFreq - newFreq) > 0 ? (oldFreq - newFreq) : 1;
                fromFreqs[GetStmtID()] = left;
            }
        }
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        node->body = body->CloneTreeWithFreqs(allocator, toFreqs, fromFreqs, numer, denom, updateOp);
        return node;
    }

    void SetBody(BlockNode *node)
    {
        body = node;
    }

    BlockNode *GetBody() const
    {
        return body;
    }

    BaseNode *Opnd(size_t i = 0) const override
    {
        if (i == 0) {
            return UnaryStmtNode::Opnd();
        } else if (i == 1) {
            return body;
        }
        DEBUG_ASSERT(false, "WhileStmtNode has wrong numOpnds field: %u", NumOpnds());
        return nullptr;
    }

private:
    BlockNode *body = nullptr;
};

class DoloopNode : public StmtNode {
public:
    DoloopNode() : DoloopNode(StIdx(), false, nullptr, nullptr, nullptr, nullptr) {}

    DoloopNode(StIdx doVarStIdx, bool isPReg, BaseNode *startExp, BaseNode *contExp, BaseNode *incrExp,
               BlockNode *doBody)
        : StmtNode(OP_doloop, kOperandNumDoloop),
          doVarStIdx(doVarStIdx),
          isPreg(isPReg),
          startExpr(startExp),
          condExpr(contExp),
          incrExpr(incrExp),
          doBody(doBody)
    {
    }

    virtual ~DoloopNode() = default;

    void DumpDoVar(const MIRModule &mod) const;
    void Dump(int32 indent) const override;
    bool Verify() const override;

    DoloopNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<DoloopNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetStartExpr(startExpr->CloneTree(allocator));
        node->SetContExpr(GetCondExpr()->CloneTree(allocator));
        node->SetIncrExpr(GetIncrExpr()->CloneTree(allocator));
        node->SetDoBody(GetDoBody()->CloneTree(allocator));
        return node;
    }

    DoloopNode *CloneTreeWithFreqs(MapleAllocator &allocator, std::unordered_map<uint32_t, uint64_t> &toFreqs,
                                   std::unordered_map<uint32_t, uint64_t> &fromFreqs, uint64_t numer, uint64_t denom,
                                   uint32_t updateOp)
    {
        auto *node = allocator.GetMemPool()->New<DoloopNode>(*this);
        node->SetStmtID(stmtIDNext++);
        if (fromFreqs.count(GetStmtID()) > 0) {
            uint64_t oldFreq = fromFreqs[GetStmtID()];
            uint64_t newFreq = oldFreq;
            if (updateOp & kUpdateFreqbyScale) {  // used in inline/clone
                newFreq = numer == 0 ? 0 : (denom > 0 ? (oldFreq * numer / denom) : oldFreq);
            } else if (updateOp & kUpdateUnrolledFreq) {  // used in unrolled part
                uint64_t bodyFreq = fromFreqs[GetDoBody()->GetStmtID()];
                newFreq = denom > 0 ? (bodyFreq * numer / denom + (oldFreq - bodyFreq)) : oldFreq;
            } else if (updateOp & kUpdateUnrollRemainderFreq) {  // used in unrolled remainder
                uint64_t bodyFreq = fromFreqs[GetDoBody()->GetStmtID()];
                newFreq = denom > 0 ? (((bodyFreq * numer) % denom) + (oldFreq - bodyFreq)) : oldFreq;
            }
            toFreqs[node->GetStmtID()] = static_cast<uint64_t>(newFreq);
            DEBUG_ASSERT(oldFreq >= newFreq, "sanity check");
            if (updateOp & kUpdateOrigFreq) {
                uint64_t left = oldFreq - newFreq;
                fromFreqs[GetStmtID()] = left;
            }
        }
        node->SetStartExpr(startExpr->CloneTree(allocator));
        node->SetContExpr(GetCondExpr()->CloneTree(allocator));
        node->SetIncrExpr(GetIncrExpr()->CloneTree(allocator));
        node->SetDoBody(GetDoBody()->CloneTreeWithFreqs(allocator, toFreqs, fromFreqs, numer, denom, updateOp));
        return node;
    }

    void SetDoVarStIdx(StIdx idx)
    {
        doVarStIdx = idx;
    }

    PregIdx GetDoVarPregIdx() const
    {
        return static_cast<PregIdx>(doVarStIdx.FullIdx());
    }

    const StIdx &GetDoVarStIdx() const
    {
        return doVarStIdx;
    }

    void SetDoVarStFullIdx(uint32 idx)
    {
        doVarStIdx.SetFullIdx(idx);
    }

    void SetIsPreg(bool isPregVal)
    {
        isPreg = isPregVal;
    }

    bool IsPreg() const
    {
        return isPreg;
    }

    void SetStartExpr(BaseNode *node)
    {
        startExpr = node;
    }

    BaseNode *GetStartExpr() const
    {
        return startExpr;
    }

    void SetContExpr(BaseNode *node)
    {
        condExpr = node;
    }

    BaseNode *GetCondExpr() const
    {
        return condExpr;
    }

    void SetIncrExpr(BaseNode *node)
    {
        incrExpr = node;
    }

    BaseNode *GetIncrExpr() const
    {
        return incrExpr;
    }

    void SetDoBody(BlockNode *node)
    {
        doBody = node;
    }

    BlockNode *GetDoBody() const
    {
        return doBody;
    }

    BaseNode *Opnd(size_t i) const override
    {
        if (i == kFirstOpnd) {
            return startExpr;
        }
        if (i == kSecondOpnd) {
            return condExpr;
        }
        if (i == kThirdOpnd) {
            return incrExpr;
        }
        return *(&doBody + i - kOpndNum);
    }

    size_t NumOpnds() const override
    {
        return kOperandNumDoloop;
    }

    void SetOpnd(BaseNode *node, size_t i) override
    {
        if (i == kFirstOpnd) {
            startExpr = node;
        }
        if (i == kSecondOpnd) {
            SetContExpr(node);
        }
        if (i == kThirdOpnd) {
            incrExpr = node;
        } else {
            *(&doBody + i - kOpndNum) = static_cast<BlockNode *>(node);
        }
    }

private:
    static constexpr int kOperandNumDoloop = 4;
    StIdx doVarStIdx;  // must be local; cast to PregIdx for preg
    bool isPreg;
    BaseNode *startExpr;
    BaseNode *condExpr;
    BaseNode *incrExpr;
    BlockNode *doBody;
};

class ForeachelemNode : public StmtNode {
public:
    ForeachelemNode() : StmtNode(OP_foreachelem)
    {
        BaseNodeT::SetNumOpnds(kOperandNumUnary);
    }

    virtual ~ForeachelemNode() = default;

    const StIdx &GetElemStIdx() const
    {
        return elemStIdx;
    }

    void SetElemStIdx(StIdx elemStIdxValue)
    {
        elemStIdx = elemStIdxValue;
    }

    const StIdx &GetArrayStIdx() const
    {
        return arrayStIdx;
    }

    void SetArrayStIdx(StIdx arrayStIdxValue)
    {
        arrayStIdx = arrayStIdxValue;
    }

    BlockNode *GetLoopBody() const
    {
        return loopBody;
    }

    void SetLoopBody(BlockNode *loopBodyValue)
    {
        loopBody = loopBodyValue;
    }

    BaseNode *Opnd(size_t) const override
    {
        return loopBody;
    }

    size_t NumOpnds() const override
    {
        return numOpnds;
    }

    void Dump(int32 indent) const override;

    ForeachelemNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<ForeachelemNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetLoopBody(loopBody->CloneTree(allocator));
        return node;
    }

private:
    StIdx elemStIdx;   // must be local symbol
    StIdx arrayStIdx;  // symbol table entry of the array/collection variable
    BlockNode *loopBody = nullptr;
};

// used by assertge, assertlt
class BinaryStmtNode : public StmtNode, public BinaryOpnds {
public:
    explicit BinaryStmtNode(Opcode o) : StmtNode(o, kOperandNumBinary) {}

    virtual ~BinaryStmtNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;
    BinaryStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<BinaryStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetBOpnd(GetBOpnd(0)->CloneTree(allocator), 0);
        node->SetBOpnd(GetBOpnd(1)->CloneTree(allocator), 1);
        return node;
    }

    BaseNode *Opnd(size_t i) const override
    {
        DEBUG_ASSERT(i < kOperandNumBinary, "Invalid operand idx in BinaryStmtNode");
        DEBUG_ASSERT(i >= 0, "Invalid operand idx in BinaryStmtNode");
        return GetBOpnd(i);
    }

    size_t NumOpnds() const override
    {
        return kOperandNumBinary;
    }

    void SetOpnd(BaseNode *node, size_t i) override
    {
        SetBOpnd(node, i);
    }

    bool IsLeaf() const override
    {
        return false;
    }
};

class IassignoffNode : public BinaryStmtNode {
public:
    IassignoffNode() : BinaryStmtNode(OP_iassignoff) {}

    explicit IassignoffNode(int32 ofst) : BinaryStmtNode(OP_iassignoff), offset(ofst) {}

    IassignoffNode(PrimType primType, int32 offset, BaseNode *addrOpnd, BaseNode *srcOpnd) : IassignoffNode(offset)
    {
        BaseNodeT::SetPrimType(primType);
        SetBOpnd(addrOpnd, 0);
        SetBOpnd(srcOpnd, 1);
    }

    virtual ~IassignoffNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    IassignoffNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IassignoffNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetBOpnd(GetBOpnd(0)->CloneTree(allocator), 0);
        node->SetBOpnd(GetBOpnd(1)->CloneTree(allocator), 1);
        return node;
    }

    int32 GetOffset() const
    {
        return offset;
    }

    void SetOffset(int32 newOffset)
    {
        offset = newOffset;
    }

private:
    int32 offset = 0;
};

// for iassignfpoff, iassignspoff, iassignpcoff
class IassignFPoffNode : public UnaryStmtNode {
public:
    IassignFPoffNode(Opcode o) : UnaryStmtNode(o) {}

    explicit IassignFPoffNode(Opcode o, int32 ofst) : UnaryStmtNode(o), offset(ofst) {}

    IassignFPoffNode(Opcode o, PrimType primType, int32 offset, BaseNode *src) : IassignFPoffNode(o, offset)
    {
        BaseNodeT::SetPrimType(primType);
        UnaryStmtNode::SetOpnd(src, 0);
    }

    virtual ~IassignFPoffNode() = default;

    void Dump(int32 indent) const override;
    bool Verify() const override;

    IassignFPoffNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IassignFPoffNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd(0)->CloneTree(allocator), 0);
        return node;
    }

    void SetOffset(int32 ofst)
    {
        offset = ofst;
    }

    int32 GetOffset() const
    {
        return offset;
    }

private:
    int32 offset = 0;
};

typedef IassignFPoffNode IassignPCoffNode;

class BlkassignoffNode : public BinaryStmtNode {
public:
    BlkassignoffNode() : BinaryStmtNode(OP_blkassignoff)
    {
        ptyp = PTY_agg;
        ptyp = PTY_agg;
        alignLog2 = 0;
        offset = 0;
    }
    explicit BlkassignoffNode(int32 ofst, int32 bsize) : BinaryStmtNode(OP_blkassignoff), offset(ofst), blockSize(bsize)
    {
        ptyp = PTY_agg;
        alignLog2 = 0;
    }
    explicit BlkassignoffNode(int32 ofst, int32 bsize, BaseNode *dest, BaseNode *src)
        : BinaryStmtNode(OP_blkassignoff), offset(ofst), blockSize(bsize)
    {
        ptyp = PTY_agg;
        alignLog2 = 0;
        SetBOpnd(dest, 0);
        SetBOpnd(src, 1);
    }
    ~BlkassignoffNode() = default;

    void Dump(int32 indent) const override;

    BlkassignoffNode *CloneTree(MapleAllocator &allocator) const override
    {
        BlkassignoffNode *node = allocator.GetMemPool()->New<BlkassignoffNode>(offset, blockSize);
        node->SetStmtID(stmtIDNext++);
        node->SetBOpnd(GetBOpnd(0)->CloneTree(allocator), 0);
        node->SetBOpnd(GetBOpnd(1)->CloneTree(allocator), 1);
        return node;
    }

    uint32 GetAlign() const
    {
        uint32 res = 1;
        for (uint32 i = 0; i < alignLog2; i++) {
            res <<= 1;
        }
        return res;
    }

    void SetAlign(uint32 x)
    {
        if (x == 0) {
            alignLog2 = 0;
            return;
        }
        DEBUG_ASSERT((~(x - 1) & x) == x, "SetAlign called with non power of 2");
        uint32 res = 0;
        while (x != 1) {
            x >>= 1;
            ++res;
        }
        alignLog2 = res;
    }

    uint32 alignLog2 : 4;
    int32 offset : 28;
    int32 blockSize = 0;
};

// used by return, syncenter, syncexit
class NaryStmtNode : public StmtNode, public NaryOpnds {
public:
    NaryStmtNode(MapleAllocator &allocator, Opcode o) : StmtNode(o), NaryOpnds(allocator) {}

    NaryStmtNode(const MIRModule &mod, Opcode o) : NaryStmtNode(mod.GetCurFuncCodeMPAllocator(), o) {}

    NaryStmtNode(MapleAllocator &allocator, const NaryStmtNode &node)
        // do not use stmt copy constructor
        : StmtNode(node.GetOpCode(), node.GetPrimType(), node.numOpnds, node.GetSrcPos(), node.GetOriginalID(),
                   node.GetStmtAttrs()),
          NaryOpnds(allocator)
    {
    }

    NaryStmtNode(const MIRModule &mod, const NaryStmtNode &node) : NaryStmtNode(mod.GetCurFuncCodeMPAllocator(), node)
    {
    }

    explicit NaryStmtNode(const NaryStmtNode &node) = delete;
    NaryStmtNode &operator=(const NaryStmtNode &node) = delete;
    virtual ~NaryStmtNode() = default;

    void Dump(int32 indent) const override;
    void DumpCallConvInfo() const
    {
        if (GetAttr(STMTATTR_ccall)) {
            LogInfo::MapleLogger() << " ccall";
        } else if (GetAttr(STMTATTR_webkitjscall)) {
            LogInfo::MapleLogger() << " webkitjscc";
        } else if (GetAttr(STMTATTR_ghcall)) {
            LogInfo::MapleLogger() << " ghccc";
        } else {
            // default is ccall
            LogInfo::MapleLogger() << " ccall";
        }
    }
    bool Verify() const override;

    NaryStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<NaryStmtNode>(allocator, *this);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        node->SetNumOpnds(GetNopndSize());
        return node;
    }

    BaseNode *Opnd(size_t i) const override
    {
        return GetNopndAt(i);
    }

    void SetOpnd(BaseNode *node, size_t i) override
    {
        DEBUG_ASSERT(i < GetNopnd().size(), "array index out of range");
        SetNOpndAt(i, node);
    }

    size_t NumOpnds() const override
    {
        DEBUG_ASSERT(numOpnds == GetNopndSize(), "NaryStmtNode has wrong numOpnds field");
        return GetNopndSize();
    }

    void SetOpnds(const MapleVector<BaseNode *> &arguments)
    {
        SetNOpnd(arguments);
        SetNumOpnds(arguments.size());
    }

    void PushOpnd(BaseNode *node)
    {
        if (node != nullptr) {
            GetNopnd().push_back(node);
        }
        SetNumOpnds(GetNopndSize());
    }

    void InsertOpnd(BaseNode *node, size_t idx)
    {
        if (node == nullptr || idx > GetNopndSize()) {
            return;
        }
        auto begin = GetNopnd().begin();
        for (size_t i = 0; i < idx; ++i) {
            ++begin;
        }
        (void)GetNopnd().insert(begin, node);
        SetNumOpnds(GetNopndSize());
    }
};

class SafetyCheckStmtNode {
public:
    explicit SafetyCheckStmtNode(GStrIdx funcNameIdx) : funcNameIdx(funcNameIdx) {}
    explicit SafetyCheckStmtNode(const SafetyCheckStmtNode &stmtNode) : funcNameIdx(stmtNode.GetFuncNameIdx()) {}

    virtual ~SafetyCheckStmtNode() = default;

    std::string GetFuncName() const;

    GStrIdx GetFuncNameIdx() const
    {
        return funcNameIdx;
    }

    void Dump() const
    {
        LogInfo::MapleLogger() << " <&" << GetFuncName() << ">";
    }

private:
    GStrIdx funcNameIdx;
};

// used by callassertnonnull, callassertle
class SafetyCallCheckStmtNode {
public:
    SafetyCallCheckStmtNode(GStrIdx callFuncNameIdx, size_t paramIndex, GStrIdx stmtFuncNameIdx)
        : callFuncNameIdx(callFuncNameIdx), paramIndex(paramIndex), stmtFuncNameIdx(stmtFuncNameIdx)
    {
    }
    explicit SafetyCallCheckStmtNode(const SafetyCallCheckStmtNode &stmtNode)
        : callFuncNameIdx(stmtNode.GetFuncNameIdx()),
          paramIndex(stmtNode.GetParamIndex()),
          stmtFuncNameIdx(stmtNode.GetStmtFuncNameIdx())
    {
    }

    virtual ~SafetyCallCheckStmtNode() = default;

    std::string GetFuncName() const;
    GStrIdx GetFuncNameIdx() const
    {
        return callFuncNameIdx;
    }
    std::string GetStmtFuncName() const;
    size_t GetParamIndex() const
    {
        return paramIndex;
    }

    GStrIdx GetStmtFuncNameIdx() const
    {
        return stmtFuncNameIdx;
    }

    void Dump() const
    {
        LogInfo::MapleLogger() << " <&" << GetFuncName() << ", " << paramIndex << ", &" << GetStmtFuncName() << ">";
    }

private:
    GStrIdx callFuncNameIdx;
    size_t paramIndex;
    GStrIdx stmtFuncNameIdx;
};

// used by callassertnonnull
class CallAssertNonnullStmtNode : public UnaryStmtNode, public SafetyCallCheckStmtNode {
public:
    CallAssertNonnullStmtNode(Opcode o, GStrIdx callFuncNameIdx, size_t paramIndex, GStrIdx stmtFuncNameIdx)
        : UnaryStmtNode(o), SafetyCallCheckStmtNode(callFuncNameIdx, paramIndex, stmtFuncNameIdx)
    {
    }
    virtual ~CallAssertNonnullStmtNode() {}

    void Dump(int32 indent) const override;

    CallAssertNonnullStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<CallAssertNonnullStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd()->CloneTree(allocator), 0);
        return node;
    }
};

// used by assertnonnull
class AssertNonnullStmtNode : public UnaryStmtNode, public SafetyCheckStmtNode {
public:
    AssertNonnullStmtNode(Opcode o, GStrIdx funcNameIdx) : UnaryStmtNode(o), SafetyCheckStmtNode(funcNameIdx) {}
    virtual ~AssertNonnullStmtNode() {}

    void Dump(int32 indent) const override;

    AssertNonnullStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<AssertNonnullStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        node->SetOpnd(Opnd()->CloneTree(allocator), 0);
        return node;
    }
};

// used by assertle
class AssertBoundaryStmtNode : public NaryStmtNode, public SafetyCheckStmtNode {
public:
    AssertBoundaryStmtNode(MapleAllocator &allocator, Opcode o, GStrIdx funcNameIdx)
        : NaryStmtNode(allocator, o), SafetyCheckStmtNode(funcNameIdx)
    {
    }
    virtual ~AssertBoundaryStmtNode() {}

    AssertBoundaryStmtNode(MapleAllocator &allocator, const AssertBoundaryStmtNode &stmtNode)
        : NaryStmtNode(allocator, stmtNode), SafetyCheckStmtNode(stmtNode)
    {
    }

    AssertBoundaryStmtNode(const MIRModule &mod, Opcode o, GStrIdx funcNameIdx)
        : AssertBoundaryStmtNode(mod.GetCurFuncCodeMPAllocator(), o, funcNameIdx)
    {
    }

    void Dump(int32 indent) const override;

    AssertBoundaryStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<AssertBoundaryStmtNode>(allocator, *this);
        node->SetStmtID(stmtIDNext++);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        node->SetNumOpnds(GetNopndSize());
        return node;
    }
};

// used by callassertle
class CallAssertBoundaryStmtNode : public NaryStmtNode, public SafetyCallCheckStmtNode {
public:
    CallAssertBoundaryStmtNode(MapleAllocator &allocator, Opcode o, GStrIdx funcNameIdx, size_t paramIndex,
                               GStrIdx stmtFuncNameIdx)
        : NaryStmtNode(allocator, o), SafetyCallCheckStmtNode(funcNameIdx, paramIndex, stmtFuncNameIdx)
    {
    }
    virtual ~CallAssertBoundaryStmtNode() {}

    CallAssertBoundaryStmtNode(MapleAllocator &allocator, const CallAssertBoundaryStmtNode &stmtNode)
        : NaryStmtNode(allocator, stmtNode), SafetyCallCheckStmtNode(stmtNode)
    {
    }

    CallAssertBoundaryStmtNode(const MIRModule &mod, Opcode o, GStrIdx funcNameIdx, size_t paramIndex,
                               GStrIdx stmtFuncNameIdx)
        : CallAssertBoundaryStmtNode(mod.GetCurFuncCodeMPAllocator(), o, funcNameIdx, paramIndex, stmtFuncNameIdx)
    {
    }

    void Dump(int32 indent) const override;

    CallAssertBoundaryStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<CallAssertBoundaryStmtNode>(allocator, *this);
        node->SetStmtID(stmtIDNext++);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        node->SetNumOpnds(GetNopndSize());
        return node;
    }
};

// used by call, virtualcall, virtualicall, superclasscall, interfacecall,
// interfaceicall, customcall
// polymorphiccall
// callassigned, virtualcallassigned, virtualicallassigned,
// superclasscallassigned, interfacecallassigned, interfaceicallassigned,
// customcallassigned
// polymorphiccallassigned
class CallNode : public NaryStmtNode, public DeoptBundleInfo {
public:
    CallNode(MapleAllocator &allocator, Opcode o)
        : NaryStmtNode(allocator, o), DeoptBundleInfo(allocator), returnValues(allocator.Adapter())
    {
    }

    CallNode(MapleAllocator &allocator, Opcode o, PUIdx idx) : CallNode(allocator, o, idx, TyIdx()) {}

    CallNode(MapleAllocator &allocator, Opcode o, PUIdx idx, TyIdx tdx)
        : NaryStmtNode(allocator, o),
          DeoptBundleInfo(allocator),
          puIdx(idx),
          tyIdx(tdx),
          returnValues(allocator.Adapter())
    {
    }

    CallNode(const MIRModule &mod, Opcode o) : CallNode(mod.GetCurFuncCodeMPAllocator(), o) {}

    CallNode(const MIRModule &mod, Opcode o, PUIdx idx, TyIdx tdx)
        : CallNode(mod.GetCurFuncCodeMPAllocator(), o, idx, tdx)
    {
    }

    CallNode(MapleAllocator &allocator, const CallNode &node)
        : NaryStmtNode(allocator, node),
          DeoptBundleInfo(allocator),
          puIdx(node.GetPUIdx()),
          tyIdx(node.tyIdx),
          returnValues(allocator.Adapter())
    {
    }

    CallNode(const MIRModule &mod, const CallNode &node) : CallNode(mod.GetCurFuncCodeMPAllocator(), node) {}

    CallNode(CallNode &node) = delete;
    CallNode &operator=(const CallNode &node) = delete;
    virtual ~CallNode() = default;
    virtual void Dump(int32 indent, bool newline) const;
    bool Verify() const override;
    MIRType *GetCallReturnType() override;
    const MIRSymbol *GetCallReturnSymbol(const MIRModule &mod) const;

    CallNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<CallNode>(allocator, *this);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        for (size_t i = 0; i < returnValues.size(); ++i) {
            node->GetReturnVec().push_back(returnValues[i]);
        }
        node->SetNumOpnds(GetNopndSize());
        for (const auto &elem : GetDeoptBundleInfo()) {
            node->AddDeoptBundleInfo(elem.first, elem.second);
        }
        return node;
    }

    PUIdx GetPUIdx() const
    {
        return puIdx;
    }

    void SetPUIdx(const PUIdx idx)
    {
        puIdx = idx;
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

    CallReturnVector &GetReturnVec()
    {
        return returnValues;
    }

    CallReturnPair GetReturnPair(size_t idx) const
    {
        DEBUG_ASSERT(idx < returnValues.size(), "out of range in CallNode::GetReturnPair");
        return returnValues.at(idx);
    }

    void SetReturnPair(CallReturnPair retVal, size_t idx)
    {
        DEBUG_ASSERT(idx < returnValues.size(), "out of range in CallNode::GetReturnPair");
        returnValues.at(idx) = retVal;
    }

    const CallReturnVector &GetReturnVec() const
    {
        return returnValues;
    }

    CallReturnPair GetNthReturnVec(size_t i) const
    {
        DEBUG_ASSERT(i < returnValues.size(), "array index out of range");
        return returnValues[i];
    }

    void SetReturnVec(const CallReturnVector &vec)
    {
        returnValues = vec;
    }

    size_t NumOpnds() const override
    {
        DEBUG_ASSERT(numOpnds == GetNopndSize(), "CallNode has wrong numOpnds field");
        return GetNopndSize();
    }

    void Dump(int32 indent) const override
    {
        Dump(indent, true);
    }

    CallReturnVector *GetCallReturnVector() override
    {
        return &returnValues;
    }

    void SetCallReturnVector(const CallReturnVector &value)
    {
        returnValues = value;
    }

private:
    PUIdx puIdx = 0;
    TyIdx tyIdx = TyIdx(0);
    CallReturnVector returnValues;
};

// icall, icallassigned, icallproto and icallprotoassigned
class IcallNode : public NaryStmtNode, public DeoptBundleInfo {
public:
    IcallNode(MapleAllocator &allocator, Opcode o)
        : NaryStmtNode(allocator, o), DeoptBundleInfo(allocator), retTyIdx(0), returnValues(allocator.Adapter())
    {
        BaseNodeT::SetNumOpnds(kOperandNumUnary);
    }

    IcallNode(MapleAllocator &allocator, Opcode o, TyIdx idx)
        : NaryStmtNode(allocator, o), DeoptBundleInfo(allocator), retTyIdx(idx), returnValues(allocator.Adapter())
    {
        BaseNodeT::SetNumOpnds(kOperandNumUnary);
    }

    IcallNode(const MIRModule &mod, Opcode o) : IcallNode(mod.GetCurFuncCodeMPAllocator(), o) {}

    IcallNode(const MIRModule &mod, Opcode o, TyIdx idx) : IcallNode(mod.GetCurFuncCodeMPAllocator(), o, idx) {}

    IcallNode(MapleAllocator &allocator, const IcallNode &node)
        : NaryStmtNode(allocator, node),
          DeoptBundleInfo(allocator),
          retTyIdx(node.retTyIdx),
          returnValues(allocator.Adapter())
    {
    }

    IcallNode(const MIRModule &mod, const IcallNode &node) : IcallNode(mod.GetCurFuncCodeMPAllocator(), node) {}

    IcallNode(IcallNode &node) = delete;
    IcallNode &operator=(const IcallNode &node) = delete;
    virtual ~IcallNode() = default;

    virtual void Dump(int32 indent, bool newline) const;
    bool Verify() const override;
    MIRType *GetCallReturnType() override;
    const MIRSymbol *GetCallReturnSymbol(const MIRModule &mod) const;
    IcallNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IcallNode>(allocator, *this);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        for (size_t i = 0; i < returnValues.size(); ++i) {
            node->returnValues.push_back(returnValues[i]);
        }
        node->SetNumOpnds(GetNopndSize());
        for (const auto &elem : GetDeoptBundleInfo()) {
            node->AddDeoptBundleInfo(elem.first, elem.second);
        }
        return node;
    }

    TyIdx GetRetTyIdx() const
    {
        return retTyIdx;
    }

    void SetRetTyIdx(TyIdx idx)
    {
        retTyIdx = idx;
    }

    const CallReturnVector &GetReturnVec() const
    {
        return returnValues;
    }

    CallReturnVector &GetReturnVec()
    {
        return returnValues;
    }

    void SetReturnVec(const CallReturnVector &vec)
    {
        returnValues = vec;
    }

    size_t NumOpnds() const override
    {
        DEBUG_ASSERT(numOpnds == GetNopndSize(), "IcallNode has wrong numOpnds field");
        return GetNopndSize();
    }

    void Dump(int32 indent) const override
    {
        Dump(indent, true);
    }

    CallReturnVector *GetCallReturnVector() override
    {
        return &returnValues;
    }

private:
    TyIdx retTyIdx;  // for icall: return type for callee; for icallproto: the prototye
    // the 0th operand is the function pointer
    CallReturnVector returnValues;
};

// used by intrinsiccall and xintrinsiccall
class IntrinsiccallNode : public NaryStmtNode, public DeoptBundleInfo {
public:
    IntrinsiccallNode(MapleAllocator &allocator, Opcode o)
        : NaryStmtNode(allocator, o),
          DeoptBundleInfo(allocator),
          intrinsic(INTRN_UNDEFINED),
          tyIdx(0),
          returnValues(allocator.Adapter())
    {
    }

    IntrinsiccallNode(MapleAllocator &allocator, Opcode o, MIRIntrinsicID id)
        : NaryStmtNode(allocator, o),
          DeoptBundleInfo(allocator),
          intrinsic(id),
          tyIdx(0),
          returnValues(allocator.Adapter())
    {
    }

    IntrinsiccallNode(const MIRModule &mod, Opcode o) : IntrinsiccallNode(mod.GetCurFuncCodeMPAllocator(), o) {}

    IntrinsiccallNode(const MIRModule &mod, Opcode o, MIRIntrinsicID id)
        : IntrinsiccallNode(mod.GetCurFuncCodeMPAllocator(), o, id)
    {
    }

    IntrinsiccallNode(MapleAllocator &allocator, const IntrinsiccallNode &node)
        : NaryStmtNode(allocator, node),
          DeoptBundleInfo(allocator),
          intrinsic(node.GetIntrinsic()),
          tyIdx(node.tyIdx),
          returnValues(allocator.Adapter())
    {
    }

    IntrinsiccallNode(const MIRModule &mod, const IntrinsiccallNode &node)
        : IntrinsiccallNode(mod.GetCurFuncCodeMPAllocator(), node)
    {
    }

    IntrinsiccallNode(IntrinsiccallNode &node) = delete;
    IntrinsiccallNode &operator=(const IntrinsiccallNode &node) = delete;
    virtual ~IntrinsiccallNode() = default;

    virtual void Dump(int32 indent, bool newline) const;
    bool Verify() const override;
    MIRType *GetCallReturnType() override;

    IntrinsiccallNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IntrinsiccallNode>(allocator, *this);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        for (size_t i = 0; i < returnValues.size(); ++i) {
            node->GetReturnVec().push_back(returnValues[i]);
        }
        node->SetNumOpnds(GetNopndSize());
        return node;
    }

    MIRIntrinsicID GetIntrinsic() const
    {
        return intrinsic;
    }

    void SetIntrinsic(MIRIntrinsicID id)
    {
        intrinsic = id;
    }

    TyIdx GetTyIdx() const
    {
        return tyIdx;
    }

    void SetTyIdx(TyIdx idx)
    {
        tyIdx = idx;
    }

    CallReturnVector &GetReturnVec()
    {
        return returnValues;
    }

    const CallReturnVector &GetReturnVec() const
    {
        return returnValues;
    }

    void SetReturnVec(const CallReturnVector &vec)
    {
        returnValues = vec;
    }

    size_t NumOpnds() const override
    {
        DEBUG_ASSERT(numOpnds == GetNopndSize(), "IntrinsiccallNode has wrong numOpnds field");
        return GetNopndSize();
    }

    void Dump(int32 indent) const override
    {
        Dump(indent, true);
    }

    CallReturnVector *GetCallReturnVector() override
    {
        return &returnValues;
    }

    CallReturnPair &GetCallReturnPair(uint32 i)
    {
        DEBUG_ASSERT(i < returnValues.size(), "array index out of range");
        return returnValues.at(i);
    }

private:
    MIRIntrinsicID intrinsic;
    TyIdx tyIdx;
    CallReturnVector returnValues;
};

// used by callinstant, virtualcallinstant, superclasscallinstant and
// interfacecallinstant, callinstantassigned, virtualcallinstantassigned,
// superclasscallinstantassigned and interfacecallinstantassigned
class CallinstantNode : public CallNode {
public:
    CallinstantNode(MapleAllocator &allocator, Opcode o, TyIdx tIdx) : CallNode(allocator, o), instVecTyIdx(tIdx) {}

    CallinstantNode(const MIRModule &mod, Opcode o, TyIdx tIdx)
        : CallinstantNode(mod.GetCurFuncCodeMPAllocator(), o, tIdx)
    {
    }

    CallinstantNode(MapleAllocator &allocator, const CallinstantNode &node)
        : CallNode(allocator, node), instVecTyIdx(node.instVecTyIdx)
    {
    }

    CallinstantNode(const MIRModule &mod, const CallinstantNode &node)
        : CallinstantNode(mod.GetCurFuncCodeMPAllocator(), node)
    {
    }

    CallinstantNode(CallinstantNode &node) = delete;
    CallinstantNode &operator=(const CallinstantNode &node) = delete;
    virtual ~CallinstantNode() = default;

    void Dump(int32 indent, bool newline) const override;
    void Dump(int32 indent) const override
    {
        Dump(indent, true);
    }

    CallinstantNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<CallinstantNode>(allocator, *this);
        for (size_t i = 0; i < GetNopndSize(); ++i) {
            node->GetNopnd().push_back(GetNopndAt(i)->CloneTree(allocator));
        }
        for (size_t i = 0; i < GetReturnVec().size(); ++i) {
            node->GetReturnVec().push_back(GetNthReturnVec(i));
        }
        node->SetNumOpnds(GetNopndSize());
        return node;
    }

    CallReturnVector *GetCallReturnVector() override
    {
        return &GetReturnVec();
    }

private:
    TyIdx instVecTyIdx;
};

class LabelNode : public StmtNode {
public:
    LabelNode() : StmtNode(OP_label) {}

    explicit LabelNode(LabelIdx idx) : StmtNode(OP_label), labelIdx(idx) {}

    virtual ~LabelNode() = default;

    void Dump(int32 indent) const override;

    LabelNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *l = allocator.GetMemPool()->New<LabelNode>(*this);
        l->SetStmtID(stmtIDNext++);
        return l;
    }

    LabelIdx GetLabelIdx() const
    {
        return labelIdx;
    }

    void SetLabelIdx(LabelIdx idx)
    {
        labelIdx = idx;
    }

private:
    LabelIdx labelIdx = 0;
};

class CommentNode : public StmtNode {
public:
    explicit CommentNode(const MapleAllocator &allocator) : StmtNode(OP_comment), comment(allocator.GetMemPool()) {}

    explicit CommentNode(const MIRModule &mod) : CommentNode(mod.GetCurFuncCodeMPAllocator()) {}

    CommentNode(const MapleAllocator &allocator, const std::string &cmt)
        : StmtNode(OP_comment), comment(cmt, allocator.GetMemPool())
    {
    }

    CommentNode(const MIRModule &mod, const std::string &cmt) : CommentNode(mod.GetCurFuncCodeMPAllocator(), cmt) {}

    CommentNode(const MapleAllocator &allocator, const CommentNode &node)
        : StmtNode(node.GetOpCode(), node.GetPrimType()), comment(node.comment, allocator.GetMemPool())
    {
    }

    CommentNode(const MIRModule &mod, const CommentNode &node) : CommentNode(mod.GetCurFuncCodeMPAllocator(), node) {}

    CommentNode(CommentNode &node) = delete;
    CommentNode &operator=(const CommentNode &node) = delete;
    virtual ~CommentNode() = default;

    void Dump(int32 indent) const override;

    CommentNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *c = allocator.GetMemPool()->New<CommentNode>(allocator, *this);
        return c;
    }

    const MapleString &GetComment() const
    {
        return comment;
    }

    void SetComment(MapleString com)
    {
        comment = com;
    }

    void SetComment(const std::string &str)
    {
        comment = str;
    }

    void SetComment(const char *str)
    {
        comment = str;
    }

    void Append(const std::string &str)
    {
        comment += str;
    }

private:
    MapleString comment;
};

enum AsmQualifierKind : unsigned {  // they are alreadgy Maple IR keywords
    kASMvolatile,
    kASMinline,
    kASMgoto,
};

class AsmNode : public NaryStmtNode {
public:
    explicit AsmNode(MapleAllocator *alloc)
        : NaryStmtNode(*alloc, OP_asm),
          asmString(alloc->GetMemPool()),
          inputConstraints(alloc->Adapter()),
          asmOutputs(alloc->Adapter()),
          outputConstraints(alloc->Adapter()),
          clobberList(alloc->Adapter()),
          gotoLabels(alloc->Adapter()),
          qualifiers(0)
    {
    }

    AsmNode(MapleAllocator &allocator, const AsmNode &node)
        : NaryStmtNode(allocator, node),
          asmString(node.asmString, allocator.GetMemPool()),
          inputConstraints(allocator.Adapter()),
          asmOutputs(allocator.Adapter()),
          outputConstraints(allocator.Adapter()),
          clobberList(allocator.Adapter()),
          gotoLabels(allocator.Adapter()),
          qualifiers(node.qualifiers)
    {
    }

    virtual ~AsmNode() = default;

    AsmNode *CloneTree(MapleAllocator &allocator) const override;

    void SetQualifier(AsmQualifierKind x)
    {
        qualifiers |= (1U << static_cast<uint32>(x));
    }

    bool GetQualifier(AsmQualifierKind x) const
    {
        return (qualifiers & (1U << static_cast<uint32>(x))) != 0;
    }

    CallReturnVector *GetCallReturnVector() override
    {
        return &asmOutputs;
    }

    void SetHasWriteInputs()
    {
        hasWriteInputs = true;
    }

    bool HasWriteInputs() const
    {
        return hasWriteInputs;
    }

    void DumpOutputs(int32 indent, std::string &uStr) const;
    void DumpInputOperands(int32 indent, std::string &uStr) const;
    void Dump(int32 indent) const override;

    MapleString asmString;
    MapleVector<UStrIdx> inputConstraints;  // length is numOpnds
    CallReturnVector asmOutputs;
    MapleVector<UStrIdx> outputConstraints;  // length is returnValues.size()
    MapleVector<UStrIdx> clobberList;
    MapleVector<LabelIdx> gotoLabels;
    uint32 qualifiers;

private:
    bool hasWriteInputs = false;
};

void DumpCallReturns(const MIRModule &mod, CallReturnVector nrets, int32 indent);
bool HasIreadExpr(const BaseNode *expr);
size_t MaxDepth(const BaseNode *expr);
}  // namespace maple

#define LOAD_SAFE_CAST_FOR_MIR_NODE
#include "ir_safe_cast_traits.def"

#endif  // MAPLE_IR_INCLUDE_MIR_NODES_H
