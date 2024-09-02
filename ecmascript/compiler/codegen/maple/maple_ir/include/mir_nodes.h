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

#ifdef ARK_LITECG_DEBUG
extern void EmitStr(const MapleString &mplStr);
#endif

class MIRPregTable;  // circular dependency exists, no other choice
class TypeTable;     // circular dependency exists, no other choice

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

#ifdef ARK_LITECG_DEBUG
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
#endif

    virtual uint8 SizeOfInstr() const
    {
        return kOpcodeInfo.GetTableItemAt(GetOpCode()).instrucSize;
    }

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

    virtual bool IsSSANode() const
    {
        return false;
    }

    void SetDebugComment(const MapleString* str)
    {
        debugComment = str;
    }

    const MapleString* GetDebugComment() const
    {
        return debugComment;
    }

private:
    const MapleString* debugComment {nullptr};
};

class UnaryNode : public BaseNode {
public:
    explicit UnaryNode(Opcode o) : BaseNode(o, 1) {}

    UnaryNode(Opcode o, PrimType typ) : BaseNode(o, typ, 1) {}

    UnaryNode(Opcode o, PrimType typ, BaseNode *expr) : BaseNode(o, typ, 1), uOpnd(expr) {}

    virtual ~UnaryNode() override = default;

#ifdef ARK_LITECG_DEBUG
    void DumpOpnd(const MIRModule &mod, int32 indent) const;
    void DumpOpnd(int32 indent) const;
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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
        DEBUG_ASSERT(GetPrimTypeSize(fromtyp) == GetPrimTypeSize(typ), "retype bit widith doesn' match");
    }

    virtual ~RetypeNode() = default;

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

    // the base of an address expr is either a leaf or an iread
    BaseNode &GetAddrExprBase() const
    {
        BaseNode *base = Opnd(0);
        while (base->NumOpnds() != 0 && base->GetOpCode() != OP_iread) {
            base = base->Opnd(0);
        }
        return *base;
    }

    MIRType *GetType() const;

protected:
    TyIdx tyIdx = TyIdx(0);
    FieldID fieldID = 0;
};

// IaddrofNode has the same member fields and member methods as IreadNode
using IaddrofNode = IreadNode;

class BinaryOpnds {
public:
    virtual ~BinaryOpnds() = default;

#ifdef ARK_LITECG_DEBUG
    virtual void Dump(int32 indent) const;
#endif

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

#ifdef ARK_LITECG_DEBUG
    using BaseNode::Dump;
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    using BinaryNode::Dump;
    void Dump(int32 indent) const override;
#endif

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

class NaryOpnds {
public:
    explicit NaryOpnds(MapleAllocator &mpallocter) : nOpnd(mpallocter.Adapter()) {}

    virtual ~NaryOpnds() = default;

#ifdef ARK_LITECG_DEBUG
    virtual void Dump(int32 indent) const;
#endif

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

#ifdef ARK_LITECG_DEBUG
    virtual void Dump(int32 indent) const;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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
        DEBUG_ASSERT(constVal != nullptr, "constVal shoule not be nullptr");
        return constVal;
    }

    void SetConstVal(MIRConst *val)
    {
        constVal = val;
    }

private:
    MIRConst *constVal = nullptr;
};

class AddrofNode : public BaseNode {
public:
    explicit AddrofNode(Opcode o) : BaseNode(o), stIdx() {}

    AddrofNode(Opcode o, PrimType typ) : AddrofNode(o, typ, StIdx(), 0) {}

    AddrofNode(Opcode o, PrimType typ, StIdx sIdx, FieldID fid) : BaseNode(o, typ, 0), stIdx(sIdx), fieldID(fid) {}

    virtual ~AddrofNode() = default;

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

    DreadoffNode *CloneTree(MapleAllocator &allocator) const override
    {
        return allocator.GetMemPool()->New<DreadoffNode>(*this);
    }

public:
    StIdx stIdx;
    int32 offset = 0;
};

class RegreadNode : public BaseNode {
public:
    RegreadNode() : BaseNode(OP_regread) {}

    explicit RegreadNode(PregIdx pIdx) : BaseNode(OP_regread), regIdx(pIdx) {}

    RegreadNode(PrimType primType, PregIdx pIdx) : RegreadNode(pIdx)
    {
        ptyp = primType;
    }

    virtual ~RegreadNode() = default;

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

private:
    PregIdx regIdx = 0;  // 32bit, negative if special register
};

// for finally
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

#ifdef ARK_LITECG_DEBUG
    using BaseNode::Dump;
    void DumpBase(int32 indent) const override;
    void Dump(int32 indent) const override;
#endif

    virtual StmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *s = allocator.GetMemPool()->New<StmtNode>(*this);
        s->SetStmtID(stmtIDNext++);
        s->SetMeStmtID(meStmtID);
        return s;
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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

private:
    TyIdx tyIdx;
    FieldID fieldID;

public:
    BaseNode *addrExpr;
    BaseNode *rhs;
};

// goto
class GotoNode : public StmtNode {
public:
    explicit GotoNode(Opcode o) : StmtNode(o) {}

    GotoNode(Opcode o, uint32 ofst) : StmtNode(o), offset(ofst) {}

    virtual ~GotoNode() = default;

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

// eval, throw, free
class UnaryStmtNode : public StmtNode {
public:
    explicit UnaryStmtNode(Opcode o) : StmtNode(o, 1) {}

    UnaryStmtNode(Opcode o, PrimType typ) : StmtNode(o, typ, 1) {}

    UnaryStmtNode(Opcode o, PrimType typ, BaseNode *opnd) : StmtNode(o, typ, 1), uOpnd(opnd) {}

    virtual ~UnaryStmtNode() = default;

#ifdef ARK_LITECG_DEBUG
    using StmtNode::Dump;
    void Dump(int32 indent) const override;
    void DumpOpnd(const MIRModule &mod, int32 indent) const;
    void DumpOpnd(int32 indent) const;
#endif

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
    BaseNode *uOpnd = nullptr;
};

// dassign
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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

private:
    StIdx stIdx;
    FieldID fieldID = 0;
};

class RegassignNode : public UnaryStmtNode {
public:
    RegassignNode() : UnaryStmtNode(OP_regassign) {}

    RegassignNode(PrimType primType, PregIdx idx, BaseNode *opnd)
        : UnaryStmtNode(OP_regassign, primType, opnd), regIdx(idx)
    {
    }

    virtual ~RegassignNode() = default;

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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
    void RemoveStmt(const StmtNode *stmtNode1);
    void InsertBefore(const StmtNode *stmtNode1, StmtNode *stmtNode2);  // Insert ss2 before ss1 in current block.
    void InsertAfter(const StmtNode *stmtNode1, StmtNode *stmtNode2);   // Insert ss2 after ss1 in current block

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent, const MIRSymbolTable *theSymTab, MIRPregTable *thePregTab, bool withInfo, bool isFuncbody,
              MIRFlavor flavor) const;

    void Dump(int32 indent) const override
    {
        Dump(indent, nullptr, nullptr, false, false, kFlavorUnknown);
    }
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

    IfStmtNode *CloneTree(MapleAllocator &allocator) const override
    {
        auto *node = allocator.GetMemPool()->New<IfStmtNode>(*this);
        node->SetStmtID(stmtIDNext++);
        CHECK_NULL_FATAL(Opnd());
        node->SetOpnd(Opnd()->CloneTree(allocator), 0);
        node->thenPart = thenPart->CloneTree(allocator);
        if (elsePart != nullptr) {
            node->elsePart = elsePart->CloneTree(allocator);
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

class BinaryStmtNode : public StmtNode, public BinaryOpnds {
public:
    explicit BinaryStmtNode(Opcode o) : StmtNode(o, kOperandNumBinary) {}

    virtual ~BinaryStmtNode() = default;

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

// used by return
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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
    void DumpCallConvInfo() const
    {
        if (GetAttr(STMTATTR_ccall)) {
            LogInfo::MapleLogger() << " ccall";
        } else if (GetAttr(STMTATTR_webkitjscall)) {
            LogInfo::MapleLogger() << " webkitjscc";
        } else {
            // default is ccall
            LogInfo::MapleLogger() << " ccall";
        }
    }
#endif

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

#ifdef ARK_LITECG_DEBUG
    std::string GetFuncName() const;
#endif

    GStrIdx GetFuncNameIdx() const
    {
        return funcNameIdx;
    }

#ifdef ARK_LITECG_DEBUG
    void Dump() const
    {
        LogInfo::MapleLogger() << " <&" << GetFuncName() << ">";
    }
#endif

private:
    GStrIdx funcNameIdx;
};

// used by call,
// callassigned,
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

#ifdef ARK_LITECG_DEBUG
    virtual void Dump(int32 indent, bool newline) const;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override
    {
        Dump(indent, true);
    }
#endif

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

#ifdef ARK_LITECG_DEBUG
    virtual void Dump(int32 indent, bool newline) const;
#endif

    MIRType *GetCallReturnType() override;
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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override
    {
        Dump(indent, true);
    }
#endif

    CallReturnVector *GetCallReturnVector() override
    {
        return &returnValues;
    }

private:
    TyIdx retTyIdx;  // for icall: return type for callee; for icallproto: the prototye
    // the 0th operand is the function pointer
    CallReturnVector returnValues;
};

// used by intrinsiccall
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

#ifdef ARK_LITECG_DEBUG
    virtual void Dump(int32 indent, bool newline) const;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override
    {
        Dump(indent, true);
    }
#endif

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

class LabelNode : public StmtNode {
public:
    LabelNode() : StmtNode(OP_label) {}

    explicit LabelNode(LabelIdx idx) : StmtNode(OP_label), labelIdx(idx) {}

    virtual ~LabelNode() = default;

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
    void Dump(int32 indent) const override;
#endif

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

#ifdef ARK_LITECG_DEBUG
void DumpCallReturns(const MIRModule &mod, CallReturnVector nrets, int32 indent);
#endif
}  // namespace maple

#define LOAD_SAFE_CAST_FOR_MIR_NODE
#include "ir_safe_cast_traits.def"

#endif  // MAPLE_IR_INCLUDE_MIR_NODES_H
