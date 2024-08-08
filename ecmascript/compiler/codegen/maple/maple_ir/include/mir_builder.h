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

#ifndef MAPLE_IR_INCLUDE_MIR_BUILDER_H
#define MAPLE_IR_INCLUDE_MIR_BUILDER_H
#include <string>
#include <utility>
#include <vector>
#include <map>
#ifdef _WIN32
#include <pthread.h>
#endif
#include "opcodes.h"
#include "prim_types.h"
#include "mir_type.h"
#include "mir_const.h"
#include "mir_symbol.h"
#include "mir_nodes.h"
#include "mir_module.h"
#include "mir_preg.h"
#include "mir_function.h"
#include "printing.h"
#include "intrinsic_op.h"
#include "opcode_info.h"
#include "global_tables.h"

namespace maple {
using ArgPair = std::pair<std::string, MIRType *>;
using ArgVector = MapleVector<ArgPair>;
class MIRBuilder {
public:
    enum MatchStyle {
        kUpdateFieldID = 0,  // do not match but traverse to update fieldID
        kMatchTopField = 1,  // match top level field only
        kMatchAnyField = 2,  // match any field
        kParentFirst = 4,    // traverse parent first
        kFoundInChild = 8,   // found in child
    };

    explicit MIRBuilder(MIRModule *module)
        : mirModule(module), incompleteTypeRefedSet(mirModule->GetMPAllocator().Adapter())
    {
    }

    virtual ~MIRBuilder() = default;

    virtual void SetCurrentFunction(MIRFunction &fun)
    {
        mirModule->SetCurFunction(&fun);
    }

    virtual MIRFunction *GetCurrentFunction() const
    {
        return mirModule->CurFunction();
    }
    MIRFunction *GetCurrentFunctionNotNull() const
    {
        MIRFunction *func = GetCurrentFunction();
        CHECK_FATAL(func != nullptr, "nullptr check");
        return func;
    }

    MIRModule &GetMirModule()
    {
        return *mirModule;
    }

    const MapleSet<TyIdx> &GetIncompleteTypeRefedSet() const
    {
        return incompleteTypeRefedSet;
    }

    std::vector<std::tuple<uint32, uint32, uint32, uint32>> &GetExtraFieldsTuples()
    {
        return extraFieldsTuples;
    }

    unsigned int GetLineNum() const
    {
        return lineNum;
    }
    void SetLineNum(unsigned int num)
    {
        lineNum = num;
    }

    GStrIdx GetOrCreateStringIndex(const std::string &str) const
    {
        return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(str);
    }

    GStrIdx GetOrCreateStringIndex(GStrIdx strIdx, const std::string &str) const
    {
        std::string firstString(GlobalTables::GetStrTable().GetStringFromStrIdx(strIdx));
        firstString += str;
        return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(firstString);
    }

    GStrIdx GetStringIndex(const std::string &str) const
    {
        return GlobalTables::GetStrTable().GetStrIdxFromName(str);
    }

    MIRFunction *GetOrCreateFunction(const std::string &, TyIdx);
    MIRFunction *GetFunctionFromSymbol(const MIRSymbol &funcst);
    MIRFunction *GetFunctionFromStidx(StIdx stIdx);
    MIRFunction *GetFunctionFromName(const std::string &);
    // For compiler-generated metadata struct
    void AddIntFieldConst(const MIRStructType &sType, MIRAggConst &newConst, uint32 fieldID, int64 constValue);
    void AddAddrofFieldConst(const MIRStructType &sType, MIRAggConst &newConst, uint32 fieldID,
                             const MIRSymbol &fieldSt);
    void AddAddroffuncFieldConst(const MIRStructType &sType, MIRAggConst &newConst, uint32 fieldID,
                                 const MIRSymbol &funcSt);

    bool TraverseToNamedField(MIRStructType &structType, GStrIdx nameIdx, uint32 &fieldID);
    bool TraverseToNamedFieldWithTypeAndMatchStyle(MIRStructType &structType, GStrIdx nameIdx, TyIdx typeIdx,
                                                   uint32 &fieldID, unsigned int matchStyle);
    void TraverseToNamedFieldWithType(MIRStructType &structType, GStrIdx nameIdx, TyIdx typeIdx, uint32 &fieldID,
                                      uint32 &idx);

    FieldID GetStructFieldIDFromNameAndType(MIRType &type, const std::string &name, TyIdx idx, unsigned int matchStyle);
    FieldID GetStructFieldIDFromNameAndType(MIRType &type, const std::string &name, TyIdx idx);
    FieldID GetStructFieldIDFromNameAndTypeParentFirst(MIRType &type, const std::string &name, TyIdx idx);
    FieldID GetStructFieldIDFromNameAndTypeParentFirstFoundInChild(MIRType &type, const std::string &name, TyIdx idx);

    FieldID GetStructFieldIDFromFieldName(MIRType &type, const std::string &name);
    FieldID GetStructFieldIDFromFieldNameParentFirst(MIRType *type, const std::string &name);

    void SetStructFieldIDFromFieldName(MIRStructType &structType, const std::string &name, GStrIdx newStrIdx,
                                       const MIRType &newFieldType);
    // for creating Function.
    MIRSymbol *GetFunctionArgument(MIRFunction &fun, uint32 index) const
    {
        CHECK(index < fun.GetFormalCount(), "index out of range in GetFunctionArgument");
        return fun.GetFormal(index);
    }

    MIRFunction *CreateFunction(const std::string &name, const MIRType &returnType, const ArgVector &arguments,
                                bool isVarg = false, bool createBody = true) const;
    MIRFunction *CreateFunction(StIdx stIdx, bool addToTable = true) const;
    virtual void UpdateFunction(MIRFunction &, const MIRType *, const ArgVector &) {}

    MIRSymbol *GetSymbolFromEnclosingScope(StIdx stIdx) const;
    virtual MIRSymbol *GetOrCreateLocalDecl(const std::string &str, const MIRType &type);
    MIRSymbol *GetLocalDecl(const std::string &str);
    MIRSymbol *CreateLocalDecl(const std::string &str, const MIRType &type);
    MIRSymbol *GetOrCreateGlobalDecl(const std::string &str, const MIRType &type);
    MIRSymbol *GetGlobalDecl(const std::string &str);
    MIRSymbol *GetDecl(const std::string &str);
    MIRSymbol *CreateGlobalDecl(const std::string &str, const MIRType &type, MIRStorageClass sc = kScGlobal);
    MIRSymbol *GetOrCreateDeclInFunc(const std::string &str, const MIRType &type, MIRFunction &func);
    // for creating Expression
    ConstvalNode *CreateConstval(MIRConst *constVal);
    ConstvalNode *CreateIntConst(uint64, PrimType);
    ConstvalNode *CreateFloatConst(float val);
    ConstvalNode *CreateDoubleConst(double val);
    ConstvalNode *CreateFloat128Const(const uint64 *val);
    ConstvalNode *GetConstInt(MemPool &memPool, int val);
    ConstvalNode *GetConstInt(int val)
    {
        return CreateIntConst(val, PTY_i32);
    }

    ConstvalNode *GetConstUInt1(bool val)
    {
        return CreateIntConst(val, PTY_u1);
    }

    ConstvalNode *GetConstUInt8(uint8 val)
    {
        return CreateIntConst(val, PTY_u8);
    }

    ConstvalNode *GetConstUInt16(uint16 val)
    {
        return CreateIntConst(val, PTY_u16);
    }

    ConstvalNode *GetConstUInt32(uint32 val)
    {
        return CreateIntConst(val, PTY_u32);
    }

    ConstvalNode *GetConstUInt64(uint64 val)
    {
        return CreateIntConst(val, PTY_u64);
    }

    ConstvalNode *CreateAddrofConst(BaseNode &);
    ConstvalNode *CreateAddroffuncConst(const BaseNode &);
    ConstvalNode *CreateStrConst(const BaseNode &);
    ConstvalNode *CreateStr16Const(const BaseNode &);
    SizeoftypeNode *CreateExprSizeoftype(const MIRType &type);
    FieldsDistNode *CreateExprFieldsDist(const MIRType &type, FieldID field1, FieldID field2);
    AddrofNode *CreateExprAddrof(FieldID fieldID, const MIRSymbol &symbol, MemPool *memPool = nullptr);
    AddrofNode *CreateExprAddrof(FieldID fieldID, StIdx symbolStIdx, MemPool *memPool = nullptr);
    AddroffuncNode *CreateExprAddroffunc(PUIdx, MemPool *memPool = nullptr);
    AddrofNode *CreateExprDread(const MIRType &type, FieldID fieldID, const MIRSymbol &symbol);
    AddrofNode *CreateExprDread(PrimType ptyp, FieldID fieldID, const MIRSymbol &symbol);
    virtual AddrofNode *CreateExprDread(MIRType &type, MIRSymbol &symbol);
    virtual AddrofNode *CreateExprDread(MIRSymbol &symbol);
    AddrofNode *CreateExprDread(PregIdx pregID, PrimType pty);
    AddrofNode *CreateExprDread(MIRSymbol &symbol, uint16 fieldID);
    DreadoffNode *CreateExprDreadoff(Opcode op, PrimType pty, const MIRSymbol &symbol, int32 offset);
    RegreadNode *CreateExprRegread(PrimType pty, PregIdx regIdx);
    IreadNode *CreateExprIread(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID, BaseNode *addr);
    IreadoffNode *CreateExprIreadoff(PrimType pty, int32 offset, BaseNode *opnd0);
    IreadFPoffNode *CreateExprIreadFPoff(PrimType pty, int32 offset);
    IaddrofNode *CreateExprIaddrof(const MIRType &returnType, const MIRType &ptrType, FieldID fieldID, BaseNode *addr);
    IaddrofNode *CreateExprIaddrof(PrimType returnTypePty, TyIdx ptrTypeIdx, FieldID fieldID, BaseNode *addr);
    BinaryNode *CreateExprBinary(Opcode opcode, const MIRType &type, BaseNode *opnd0, BaseNode *opnd1);
    BinaryNode *CreateExprBinary(Opcode opcode, PrimType pty, BaseNode *opnd0, BaseNode *opnd1)
    {
        MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(pty));
        return CreateExprBinary(opcode, *ty, opnd0, opnd1);
    }
    TernaryNode *CreateExprTernary(Opcode opcode, const MIRType &type, BaseNode *opnd0, BaseNode *opnd1,
                                   BaseNode *opnd2);
    CompareNode *CreateExprCompare(Opcode opcode, const MIRType &type, const MIRType &opndType, BaseNode *opnd0,
                                   BaseNode *opnd1);
    UnaryNode *CreateExprUnary(Opcode opcode, const MIRType &type, BaseNode *opnd);
    GCMallocNode *CreateExprGCMalloc(Opcode opcode, const MIRType &ptype, const MIRType &type);
    JarrayMallocNode *CreateExprJarrayMalloc(Opcode opcode, const MIRType &ptype, const MIRType &type, BaseNode *opnd);
    TypeCvtNode *CreateExprTypeCvt(Opcode o, PrimType toPrimType, PrimType fromPrimType, BaseNode &opnd);
    TypeCvtNode *CreateExprTypeCvt(Opcode o, const MIRType &type, const MIRType &fromtype, BaseNode *opnd);
    ExtractbitsNode *CreateExprExtractbits(Opcode o, const MIRType &type, uint32 bOffset, uint32 bSize, BaseNode *opnd);
    ExtractbitsNode *CreateExprExtractbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize, BaseNode *opnd);
    DepositbitsNode *CreateExprDepositbits(Opcode o, PrimType type, uint32 bOffset, uint32 bSize, BaseNode *leftOpnd,
                                           BaseNode *rightOpnd);
    RetypeNode *CreateExprRetype(const MIRType &type, const MIRType &fromType, BaseNode *opnd);
    RetypeNode *CreateExprRetype(const MIRType &type, PrimType fromType, BaseNode *opnd);
    ArrayNode *CreateExprArray(const MIRType &arrayType);
    ArrayNode *CreateExprArray(const MIRType &arrayType, BaseNode *op);
    ArrayNode *CreateExprArray(const MIRType &arrayType, BaseNode *op1, BaseNode *op2);
    ArrayNode *CreateExprArray(const MIRType &arrayType, std::vector<BaseNode *> ops);
    IntrinsicopNode *CreateExprIntrinsicop(MIRIntrinsicID id, Opcode op, PrimType primType, TyIdx tyIdx,
                                           const MapleVector<BaseNode *> &ops);
    IntrinsicopNode *CreateExprIntrinsicop(MIRIntrinsicID idx, Opcode opcode, const MIRType &type,
                                           const MapleVector<BaseNode *> &ops);
    // for creating Statement.
    NaryStmtNode *CreateStmtReturn(BaseNode *rVal);
    NaryStmtNode *CreateStmtNary(Opcode op, BaseNode *rVal);
    NaryStmtNode *CreateStmtNary(Opcode op, const MapleVector<BaseNode *> &rVals);
    AssertNonnullStmtNode *CreateStmtAssertNonnull(Opcode op, BaseNode *rVal, GStrIdx funcNameIdx);
    CallAssertNonnullStmtNode *CreateStmtCallAssertNonnull(Opcode op, BaseNode *rVal, GStrIdx callFuncNameIdx,
                                                           size_t index, GStrIdx stmtFuncNameIdx);
    CallAssertBoundaryStmtNode *CreateStmtCallAssertBoundary(Opcode op, const MapleVector<BaseNode *> &rVals,
                                                             GStrIdx funcNameIdx, size_t index,
                                                             GStrIdx stmtFuncNameIdx);
    AssertBoundaryStmtNode *CreateStmtAssertBoundary(Opcode op, const MapleVector<BaseNode *> &rVals,
                                                     GStrIdx funcNameIdx);
    UnaryStmtNode *CreateStmtUnary(Opcode op, BaseNode *rVal);
    UnaryStmtNode *CreateStmtThrow(BaseNode *rVal);
    DassignNode *CreateStmtDassign(const MIRSymbol &var, FieldID fieldID, BaseNode *src);
    DassignNode *CreateStmtDassign(StIdx sIdx, FieldID fieldID, BaseNode *src);
    RegassignNode *CreateStmtRegassign(PrimType pty, PregIdx regIdx, BaseNode *src);
    IassignNode *CreateStmtIassign(const MIRType &type, FieldID fieldID, BaseNode *addr, BaseNode *src);
    IassignoffNode *CreateStmtIassignoff(PrimType pty, int32 offset, BaseNode *opnd0, BaseNode *src);
    IassignFPoffNode *CreateStmtIassignFPoff(Opcode op, PrimType pty, int32 offset, BaseNode *src);
    CallNode *CreateStmtCall(PUIdx puIdx, const MapleVector<BaseNode *> &args, Opcode opcode = OP_call);
    CallNode *CreateStmtCall(const std::string &name, const MapleVector<BaseNode *> &args);
    CallNode *CreateStmtVirtualCall(PUIdx puIdx, const MapleVector<BaseNode *> &args)
    {
        return CreateStmtCall(puIdx, args, OP_virtualcall);
    }

    CallNode *CreateStmtSuperclassCall(PUIdx puIdx, const MapleVector<BaseNode *> &args)
    {
        return CreateStmtCall(puIdx, args, OP_superclasscall);
    }

    CallNode *CreateStmtInterfaceCall(PUIdx puIdx, const MapleVector<BaseNode *> &args)
    {
        return CreateStmtCall(puIdx, args, OP_interfacecall);
    }

    IcallNode *CreateStmtIcall(const MapleVector<BaseNode *> &args);
    IcallNode *CreateStmtIcallAssigned(const MapleVector<BaseNode *> &args, const MIRSymbol &ret);
    IcallNode *CreateStmtIcallAssigned(const MapleVector<BaseNode *> &args, PregIdx pregIdx);
    IcallNode *CreateStmtIcallproto(const MapleVector<BaseNode *> &args, const TyIdx &prototypeIdx);
    IcallNode *CreateStmtIcallprotoAssigned(const MapleVector<BaseNode *> &args, const MIRSymbol &ret);
    // For Call, VirtualCall, SuperclassCall, InterfaceCall
    IntrinsiccallNode *CreateStmtIntrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode *> &arguments,
                                               TyIdx tyIdx = TyIdx());
    IntrinsiccallNode *CreateStmtXintrinsicCall(MIRIntrinsicID idx, const MapleVector<BaseNode *> &arguments);
    CallNode *CreateStmtCallAssigned(PUIdx puidx, const MIRSymbol *ret, Opcode op = OP_callassigned);
    CallNode *CreateStmtCallAssigned(PUIdx puidx, const MapleVector<BaseNode *> &args, const MIRSymbol *ret,
                                     Opcode op = OP_callassigned, TyIdx tyIdx = TyIdx());
    CallNode *CreateStmtCallRegassigned(PUIdx, PregIdx, Opcode);
    CallNode *CreateStmtCallRegassigned(PUIdx, PregIdx, Opcode, BaseNode *opnd);
    CallNode *CreateStmtCallRegassigned(PUIdx, const MapleVector<BaseNode *> &, PregIdx, Opcode);
    IntrinsiccallNode *CreateStmtIntrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode *> &arguments,
                                                       PregIdx retPregIdx);
    IntrinsiccallNode *CreateStmtIntrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode *> &arguments,
                                                       const MIRSymbol *ret, TyIdx tyIdx = TyIdx());
    IntrinsiccallNode *CreateStmtXintrinsicCallAssigned(MIRIntrinsicID idx, const MapleVector<BaseNode *> &args,
                                                        const MIRSymbol *ret);
    IfStmtNode *CreateStmtIf(BaseNode *cond);
    IfStmtNode *CreateStmtIfThenElse(BaseNode *cond);
    DoloopNode *CreateStmtDoloop(StIdx, bool, BaseNode *, BaseNode *, BaseNode *);
    SwitchNode *CreateStmtSwitch(BaseNode *opnd, LabelIdx defaultLabel, const CaseVector &switchTable);
    GotoNode *CreateStmtGoto(Opcode o, LabelIdx labIdx);
    JsTryNode *CreateStmtJsTry(Opcode o, LabelIdx cLabIdx, LabelIdx fLabIdx);
    TryNode *CreateStmtTry(const MapleVector<LabelIdx> &cLabIdxs);
    CatchNode *CreateStmtCatch(const MapleVector<TyIdx> &tyIdxVec);
    LabelIdx GetOrCreateMIRLabel(const std::string &name);
    LabelIdx CreateLabIdx(MIRFunction &mirFunc);
    LabelNode *CreateStmtLabel(LabelIdx labIdx);
    StmtNode *CreateStmtComment(const std::string &comment);
    CondGotoNode *CreateStmtCondGoto(BaseNode *cond, Opcode op, LabelIdx labIdx);
    void AddStmtInCurrentFunctionBody(StmtNode &stmt);
    MIRSymbol *GetSymbol(TyIdx, const std::string &, MIRSymKind, MIRStorageClass, uint8, bool) const;
    MIRSymbol *GetSymbol(TyIdx, GStrIdx, MIRSymKind, MIRStorageClass, uint8, bool) const;
    MIRSymbol *GetOrCreateSymbol(TyIdx, const std::string &, MIRSymKind, MIRStorageClass, MIRFunction *, uint8,
                                 bool) const;
    MIRSymbol *GetOrCreateSymbol(TyIdx, GStrIdx, MIRSymKind, MIRStorageClass, MIRFunction *, uint8, bool) const;
    MIRSymbol *CreatePregFormalSymbol(TyIdx, PregIdx, MIRFunction &) const;
    // for creating symbol
    MIRSymbol *CreateSymbol(TyIdx, const std::string &, MIRSymKind, MIRStorageClass, MIRFunction *, uint8) const;
    MIRSymbol *CreateSymbol(TyIdx, GStrIdx, MIRSymKind, MIRStorageClass, MIRFunction *, uint8) const;
    MIRSymbol *CreateConstStringSymbol(const std::string &symbolName, const std::string &content);
    // for creating nodes
    AddrofNode *CreateAddrof(const MIRSymbol &st, PrimType pty = PTY_ptr);
    AddrofNode *CreateDread(const MIRSymbol &st, PrimType pty);
    virtual MemPool *GetCurrentFuncCodeMp();
    virtual MapleAllocator *GetCurrentFuncCodeMpAllocator();
    virtual MemPool *GetCurrentFuncDataMp();

    virtual void GlobalLock() {}
    virtual void GlobalUnlock() {}

    void SetCurrentDebugComment(const std::string &comment)
    {
        currComment = GetMirModule().CurFunction()->AddNewDebugComment(comment);
    }

    void ClearCurrentDebugComment()
    {
        currComment = nullptr;
    }

private:
    MIRSymbol *GetOrCreateGlobalDecl(const std::string &str, TyIdx tyIdx, bool &created) const;
    MIRSymbol *GetOrCreateLocalDecl(const std::string &str, TyIdx tyIdx, MIRSymbolTable &symbolTable,
                                    bool &created) const;
    template <class T, typename... Arguments>
    T *NewNode(Arguments &&... args)
    {
        static_assert(std::is_base_of_v<BaseNode, T>);
        T *stmt =  GetCurrentFuncCodeMp()->New<T>(args...);
        stmt->SetDebugComment(currComment);
        return stmt;
    }

    MIRModule *mirModule;
    MapleSet<TyIdx> incompleteTypeRefedSet;
    // <className strIdx, fieldname strIdx, typename strIdx, attr list strIdx>
    std::vector<std::tuple<uint32, uint32, uint32, uint32>> extraFieldsTuples;
    unsigned int lineNum = 0;
    const MapleString *currComment { nullptr };
};

class MIRBuilderExt : public MIRBuilder {
public:
    explicit MIRBuilderExt(MIRModule *module, pthread_mutex_t *mutex = nullptr);
    virtual ~MIRBuilderExt() = default;

    void SetCurrentFunction(MIRFunction &func) override
    {
        curFunction = &func;
    }

    MIRFunction *GetCurrentFunction() const override
    {
        return curFunction;
    }

    MemPool *GetCurrentFuncCodeMp() override;
    MapleAllocator *GetCurrentFuncCodeMpAllocator() override;
    void GlobalLock() override;
    void GlobalUnlock() override;

private:
    MIRFunction *curFunction = nullptr;
    pthread_mutex_t *mutex = nullptr;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_BUILDER_H
