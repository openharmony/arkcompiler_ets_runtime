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

#include "bin_mpl_export.h"
#include "bin_mpl_import.h"
#include "mir_function.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "mir_builder.h"
using namespace std;

namespace maple {
constexpr uint32 kOffset4bit = 4;
void BinaryMplImport::ImportInfoVector(MIRInfoVector &infoVector, MapleVector<bool> &infoVectorIsString)
{
    int64 size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        GStrIdx gStrIdx = ImportStr();
        bool isstring = (ReadNum() != 0);
        infoVectorIsString.push_back(isstring);
        if (isstring) {
            GStrIdx fieldval = ImportStr();
            infoVector.emplace_back(MIRInfoPair(gStrIdx, fieldval.GetIdx()));
        } else {
            auto fieldval = static_cast<uint32>(ReadNum());
            infoVector.emplace_back(MIRInfoPair(gStrIdx, fieldval));
        }
    }
}

void BinaryMplImport::ImportFuncIdInfo(MIRFunction *func)
{
    int64 tag = ReadNum();
    CHECK_FATAL(tag == kBinFuncIdInfoStart, "kBinFuncIdInfoStart expected");
    func->SetPuidxOrigin(static_cast<PUIdx>(ReadNum()));
    ImportInfoVector(func->GetInfoVector(), func->InfoIsString());
    if (mod.GetFlavor() == kFlavorLmbc) {
        func->SetFrameSize(static_cast<uint32>(ReadNum()));
    }
}

void BinaryMplImport::ImportBaseNode(Opcode &o, PrimType &typ)
{
    o = static_cast<Opcode>(Read());
    typ = static_cast<PrimType>(Read());
}

MIRSymbol *BinaryMplImport::ImportLocalSymbol(MIRFunction *func)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return nullptr;
    }
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < localSymTab.size(), "index out of bounds");
        return localSymTab.at(static_cast<size_t>(-tag));
    }
    CHECK_FATAL(tag == kBinSymbol, "expecting kBinSymbol in ImportLocalSymbol()");
    MIRSymbol *sym = func->GetSymTab()->CreateSymbol(kScopeLocal);
    localSymTab.push_back(sym);
    sym->SetNameStrIdx(ImportStr());
    (void)func->GetSymTab()->AddToStringSymbolMap(*sym);
    sym->SetSKind(static_cast<MIRSymKind>(ReadNum()));
    sym->SetStorageClass(static_cast<MIRStorageClass>(ReadNum()));
    sym->SetAttrs(ImportTypeAttrs());
    sym->SetIsTmp(ReadNum() != 0);
    if (sym->GetSKind() == kStVar || sym->GetSKind() == kStFunc) {
        ImportSrcPos(sym->GetSrcPosition());
    }
    sym->SetTyIdx(ImportJType());
    if (sym->GetSKind() == kStPreg) {
        PregIdx pregidx = ImportPreg(func);
        MIRPreg *preg = func->GetPregTab()->PregFromPregIdx(pregidx);
        sym->SetPreg(preg);
    } else if (sym->GetSKind() == kStConst || sym->GetSKind() == kStVar) {
        sym->SetKonst(ImportConst(func));
    } else if (sym->GetSKind() == kStFunc) {
        PUIdx puIdx = ImportFuncViaSym(func);
        sym->SetFunction(GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx));
    }
    return sym;
}

PregIdx BinaryMplImport::ImportPreg(MIRFunction *func)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return 0;
    }
    if (tag == kBinSpecialReg) {
        return -Read();
    }
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < localPregTab.size(), "index out of bounds");
        return localPregTab.at(static_cast<size_t>(-tag));
    }
    CHECK_FATAL(tag == kBinPreg, "expecting kBinPreg in ImportPreg()");

    PrimType primType = static_cast<PrimType>(Read());
    PregIdx pidx = func->GetPregTab()->CreatePreg(primType);
    localPregTab.push_back(pidx);
    return pidx;
}

LabelIdx BinaryMplImport::ImportLabel(MIRFunction *func)
{
    int64 tag = ReadNum();
    if (tag == 0) {
        return 0;
    }
    if (tag < 0) {
        CHECK_FATAL(static_cast<size_t>(-tag) < localLabelTab.size(), "index out of bounds");
        return localLabelTab.at(static_cast<size_t>(-tag));
    }
    CHECK_FATAL(tag == kBinLabel, "kBinLabel expected in ImportLabel()");

    LabelIdx lidx = func->GetLabelTab()->CreateLabel();
    localLabelTab.push_back(lidx);
    return lidx;
}

void BinaryMplImport::ImportLocalTypeNameTable(MIRTypeNameTable *typeNameTab)
{
    int64 tag = ReadNum();
    CHECK_FATAL(tag == kBinTypenameStart, "kBinTypenameStart expected in ImportLocalTypeNameTable()");
    int64 size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        GStrIdx strIdx = ImportStr();
        TyIdx tyIdx = ImportJType();
        typeNameTab->SetGStrIdxToTyIdx(strIdx, tyIdx);
    }
}

void BinaryMplImport::ImportFormalsStIdx(MIRFunction *func)
{
    auto tag = ReadNum();
    CHECK_FATAL(tag == kBinFormalStart, "kBinFormalStart expected in ImportFormalsStIdx()");
    auto size = ReadNum();
    for (int64 i = 0; i < size; ++i) {
        func->GetFormalDefVec()[static_cast<uint64>(i)].formalSym = ImportLocalSymbol(func);
    }
}

void BinaryMplImport::ImportAliasMap(MIRFunction *func)
{
    int64 tag = ReadNum();
    CHECK_FATAL(tag == kBinAliasMapStart, "kBinAliasMapStart expected in ImportAliasMap()");
    int32 size = ReadInt();
    for (int32 i = 0; i < size; ++i) {
        MIRAliasVars aliasvars;
        GStrIdx strIdx = ImportStr();
        aliasvars.mplStrIdx = ImportStr();
        aliasvars.tyIdx = ImportJType();
        (void)ImportStr();  // not assigning to mimic parser
        func->GetAliasVarMap()[strIdx] = aliasvars;
    }
}

PUIdx BinaryMplImport::ImportFuncViaSym(MIRFunction *func)
{
    MIRSymbol *sym = InSymbol(func);
    MIRFunction *f = sym->GetFunction();
    return f->GetPuidx();
}

BaseNode *BinaryMplImport::ImportExpression(MIRFunction *func)
{
    Opcode op;
    PrimType typ;
    ImportBaseNode(op, typ);
    switch (op) {
        // leaf
        case OP_constval: {
            MIRConst *constv = ImportConst(func);
            ConstvalNode *constNode = mod.CurFuncCodeMemPool()->New<ConstvalNode>(constv);
            constNode->SetPrimType(typ);
            return constNode;
        }
        case OP_conststr: {
            UStrIdx strIdx = ImportUsrStr();
            ConststrNode *constNode = mod.CurFuncCodeMemPool()->New<ConststrNode>(typ, strIdx);
            constNode->SetPrimType(typ);
            return constNode;
        }
        case OP_addroflabel: {
            AddroflabelNode *alabNode = mod.CurFuncCodeMemPool()->New<AddroflabelNode>();
            alabNode->SetOffset(ImportLabel(func));
            alabNode->SetPrimType(typ);
            (void)func->GetLabelTab()->addrTakenLabels.insert(alabNode->GetOffset());
            return alabNode;
        }
        case OP_addroffunc: {
            PUIdx puIdx = ImportFuncViaSym(func);
            MIRFunction *f = GlobalTables::GetFunctionTable().GetFuncTable()[puIdx];
            CHECK_FATAL(f != nullptr, "null ptr");
            f->GetFuncSymbol()->SetAppearsInCode(true);
            AddroffuncNode *addrNode = mod.CurFuncCodeMemPool()->New<AddroffuncNode>(typ, puIdx);
            return addrNode;
        }
        case OP_sizeoftype: {
            TyIdx tidx = ImportJType();
            SizeoftypeNode *sot = mod.CurFuncCodeMemPool()->New<SizeoftypeNode>(tidx);
            return sot;
        }
        case OP_addrof:
        case OP_addrofoff:
        case OP_dread:
        case OP_dreadoff: {
            int32 num = static_cast<int32>(ReadNum());
            StIdx stIdx;
            stIdx.SetScope(static_cast<uint32>(ReadNum()));
            MIRSymbol *sym = nullptr;
            if (stIdx.Islocal()) {
                sym = ImportLocalSymbol(func);
                CHECK_FATAL(sym != nullptr, "null ptr check");
            } else {
                sym = InSymbol(nullptr);
                CHECK_FATAL(sym != nullptr, "null ptr check");
                if (op == OP_addrof) {
                    sym->SetHasPotentialAssignment();
                }
            }
            stIdx.SetIdx(sym->GetStIdx().Idx());
            if (op == OP_addrof || op == OP_dread) {
                AddrofNode *drNode = mod.CurFuncCodeMemPool()->New<AddrofNode>(op);
                drNode->SetPrimType(typ);
                drNode->SetStIdx(stIdx);
                drNode->SetFieldID(num);
                return drNode;
            } else {
                DreadoffNode *dreadoff = mod.CurFuncCodeMemPool()->New<DreadoffNode>(op);
                dreadoff->SetPrimType(typ);
                dreadoff->stIdx = stIdx;
                dreadoff->offset = num;
                return dreadoff;
            }
        }
        case OP_regread: {
            RegreadNode *regreadNode = mod.CurFuncCodeMemPool()->New<RegreadNode>();
            regreadNode->SetRegIdx(ImportPreg(func));
            regreadNode->SetPrimType(typ);
            return regreadNode;
        }
        case OP_gcmalloc:
        case OP_gcpermalloc:
        case OP_stackmalloc: {
            TyIdx tyIdx = ImportJType();
            GCMallocNode *gcNode = mod.CurFuncCodeMemPool()->New<GCMallocNode>(op, typ, tyIdx);
            return gcNode;
        }
        // unary
        case OP_abs:
        case OP_bnot:
        case OP_lnot:
        case OP_neg:
        case OP_recip:
        case OP_sqrt:
        case OP_alloca:
        case OP_malloc: {
            UnaryNode *unNode = mod.CurFuncCodeMemPool()->New<UnaryNode>(op, typ);
            unNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            return unNode;
        }
        case OP_ceil:
        case OP_cvt:
        case OP_floor:
        case OP_trunc: {
            TypeCvtNode *typecvtNode = mod.CurFuncCodeMemPool()->New<TypeCvtNode>(op, typ);
            typecvtNode->SetFromType(static_cast<PrimType>(Read()));
            typecvtNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            return typecvtNode;
        }
        case OP_retype: {
            RetypeNode *retypeNode = mod.CurFuncCodeMemPool()->New<RetypeNode>(typ);
            retypeNode->SetTyIdx(ImportJType());
            retypeNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            return retypeNode;
        }
        case OP_iread:
        case OP_iaddrof: {
            IreadNode *irNode = mod.CurFuncCodeMemPool()->New<IreadNode>(op, typ);
            irNode->SetTyIdx(ImportJType());
            irNode->SetFieldID(static_cast<FieldID>(ReadNum()));
            irNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            return irNode;
        }
        case OP_ireadoff: {
            int32 ofst = static_cast<int32>(ReadNum());
            IreadoffNode *irNode = mod.CurFuncCodeMemPool()->New<IreadoffNode>(typ, ofst);
            irNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            return irNode;
        }
        case OP_ireadfpoff: {
            int32 ofst = static_cast<int32>(ReadNum());
            IreadFPoffNode *irNode = mod.CurFuncCodeMemPool()->New<IreadFPoffNode>(typ, ofst);
            return irNode;
        }
        case OP_sext:
        case OP_zext:
        case OP_extractbits: {
            ExtractbitsNode *extNode = mod.CurFuncCodeMemPool()->New<ExtractbitsNode>(op, typ);
            extNode->SetBitsOffset(Read());
            extNode->SetBitsSize(Read());
            extNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            return extNode;
        }
        case OP_depositbits: {
            DepositbitsNode *dbNode = mod.CurFuncCodeMemPool()->New<DepositbitsNode>(op, typ);
            dbNode->SetBitsOffset(static_cast<uint8>(ReadNum()));
            dbNode->SetBitsSize(static_cast<uint8>(ReadNum()));
            dbNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            dbNode->SetOpnd(ImportExpression(func), kSecondOpnd);
            return dbNode;
        }
        case OP_gcmallocjarray:
        case OP_gcpermallocjarray: {
            JarrayMallocNode *gcNode = mod.CurFuncCodeMemPool()->New<JarrayMallocNode>(op, typ);
            gcNode->SetTyIdx(ImportJType());
            gcNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            return gcNode;
        }
        // binary
        case OP_sub:
        case OP_mul:
        case OP_div:
        case OP_rem:
        case OP_ashr:
        case OP_lshr:
        case OP_shl:
        case OP_max:
        case OP_min:
        case OP_band:
        case OP_bior:
        case OP_bxor:
        case OP_cand:
        case OP_cior:
        case OP_land:
        case OP_lior:
        case OP_add: {
            BinaryNode *binNode = mod.CurFuncCodeMemPool()->New<BinaryNode>(op, typ);
            binNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            binNode->SetOpnd(ImportExpression(func), kSecondOpnd);
            return binNode;
        }
        case OP_eq:
        case OP_ne:
        case OP_lt:
        case OP_gt:
        case OP_le:
        case OP_ge:
        case OP_cmpg:
        case OP_cmpl:
        case OP_cmp: {
            CompareNode *cmpNode = mod.CurFuncCodeMemPool()->New<CompareNode>(op, typ);
            cmpNode->SetOpndType(static_cast<PrimType>(Read()));
            cmpNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            cmpNode->SetOpnd(ImportExpression(func), kSecondOpnd);
            return cmpNode;
        }
        case OP_resolveinterfacefunc:
        case OP_resolvevirtualfunc: {
            ResolveFuncNode *rsNode = mod.CurFuncCodeMemPool()->New<ResolveFuncNode>(op, typ);
            rsNode->SetPUIdx(ImportFuncViaSym(func));
            rsNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            rsNode->SetOpnd(ImportExpression(func), kSecondOpnd);
            return rsNode;
        }
        // ternary
        case OP_select: {
            TernaryNode *tNode = mod.CurFuncCodeMemPool()->New<TernaryNode>(op, typ);
            tNode->SetOpnd(ImportExpression(func), kFirstOpnd);
            tNode->SetOpnd(ImportExpression(func), kSecondOpnd);
            tNode->SetOpnd(ImportExpression(func), kThirdOpnd);
            return tNode;
        }
        // nary
        case OP_array: {
            TyIdx tidx = ImportJType();
            bool boundsCheck = static_cast<bool>(Read());
            ArrayNode *arrNode =
                mod.CurFuncCodeMemPool()->New<ArrayNode>(func->GetCodeMPAllocator(), typ, tidx, boundsCheck);
            auto n = static_cast<uint32>(ReadNum());
            for (uint32 i = 0; i < n; ++i) {
                arrNode->GetNopnd().push_back(ImportExpression(func));
            }
            arrNode->SetNumOpnds(static_cast<uint8>(arrNode->GetNopnd().size()));
            return arrNode;
        }
        case OP_intrinsicop: {
            IntrinsicopNode *intrnNode =
                mod.CurFuncCodeMemPool()->New<IntrinsicopNode>(func->GetCodeMPAllocator(), op, typ);
            intrnNode->SetIntrinsic(static_cast<MIRIntrinsicID>(ReadNum()));
            auto n = static_cast<uint32>(ReadNum());
            for (uint32 i = 0; i < n; ++i) {
                intrnNode->GetNopnd().push_back(ImportExpression(func));
            }
            intrnNode->SetNumOpnds(static_cast<uint8>(intrnNode->GetNopnd().size()));
            return intrnNode;
        }
        case OP_intrinsicopwithtype: {
            IntrinsicopNode *intrnNode =
                mod.CurFuncCodeMemPool()->New<IntrinsicopNode>(func->GetCodeMPAllocator(), OP_intrinsicopwithtype, typ);
            intrnNode->SetIntrinsic((MIRIntrinsicID)ReadNum());
            intrnNode->SetTyIdx(ImportJType());
            auto n = static_cast<uint32>(ReadNum());
            for (uint32 i = 0; i < n; ++i) {
                intrnNode->GetNopnd().push_back(ImportExpression(func));
            }
            intrnNode->SetNumOpnds(static_cast<uint8>(intrnNode->GetNopnd().size()));
            return intrnNode;
        }
        default:
            CHECK_FATAL(false, "Unhandled op %d", op);
            break;
    }
}

void BinaryMplImport::ImportSrcPos(SrcPosition &pos)
{
    if (!mod.IsWithDbgInfo()) {
        return;
    }
    pos.SetRawData(static_cast<uint32>(ReadNum()));
    pos.SetLineNum(static_cast<uint32>(ReadNum()));
}

void BinaryMplImport::ImportReturnValues(MIRFunction *func, CallReturnVector *retv)
{
    int64 tag = ReadNum();
    CHECK_FATAL(tag == kBinReturnvals, "expecting return values");
    auto size = static_cast<uint32>(ReadNum());
    for (uint32 i = 0; i < size; ++i) {
        RegFieldPair rfp;
        rfp.SetPregIdx(ImportPreg(func));
        if (rfp.IsReg()) {
            retv->push_back(std::make_pair(StIdx(), rfp));
            continue;
        }
        rfp.SetFieldID(static_cast<FieldID>(ReadNum()));
        MIRSymbol *lsym = ImportLocalSymbol(func);
        CHECK_FATAL(lsym != nullptr, "null ptr check");
        retv->push_back(std::make_pair(lsym->GetStIdx(), rfp));
        if (lsym->GetName().find("L_STR") == 0) {
            MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lsym->GetTyIdx());
            CHECK_FATAL(ty->GetKind() == kTypePointer, "Pointer type expected for L_STR prefix");
            MIRPtrType tempType(static_cast<MIRPtrType *>(ty)->GetPointedTyIdx(), PTY_ptr);
            TyIdx newTyidx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&tempType);
            lsym->SetTyIdx(newTyidx);
        }
    }
}

BlockNode *BinaryMplImport::ImportBlockNode(MIRFunction *func)
{
    int64 tag = ReadNum();
    DEBUG_ASSERT(tag == kBinNodeBlock, "expecting a BlockNode");

    BlockNode *block = func->GetCodeMemPool()->New<BlockNode>();
    Opcode op;
    uint8 numOpr;
    ImportSrcPos(block->GetSrcPos());
    int32 size = ReadInt();
    for (int32 k = 0; k < size; ++k) {
        SrcPosition thesrcPosition;
        ImportSrcPos(thesrcPosition);
        op = static_cast<Opcode>(ReadNum());
        StmtNode *stmt = nullptr;
        switch (op) {
            case OP_dassign:
            case OP_dassignoff: {
                PrimType primType = PTY_void;
                if (op == OP_dassignoff) {
                    primType = static_cast<PrimType>(ReadNum());
                }
                int32 num = static_cast<int32>(ReadNum());
                StIdx stIdx;
                stIdx.SetScope(static_cast<uint32>(ReadNum()));
                MIRSymbol *sym = nullptr;
                if (stIdx.Islocal()) {
                    sym = ImportLocalSymbol(func);
                    CHECK_FATAL(sym != nullptr, "null ptr check");
                } else {
                    sym = InSymbol(nullptr);
                    CHECK_FATAL(sym != nullptr, "null ptr check");
                    sym->SetHasPotentialAssignment();
                }
                stIdx.SetIdx(sym->GetStIdx().Idx());
                if (op == OP_dassign) {
                    DassignNode *s = func->GetCodeMemPool()->New<DassignNode>();
                    s->SetStIdx(stIdx);
                    s->SetFieldID(num);
                    s->SetOpnd(ImportExpression(func), kFirstOpnd);
                    stmt = s;
                } else {
                    DassignoffNode *s = func->GetCodeMemPool()->New<DassignoffNode>();
                    s->SetPrimType(primType);
                    s->stIdx = stIdx;
                    s->offset = num;
                    s->SetOpnd(ImportExpression(func), kFirstOpnd);
                    stmt = s;
                }
                break;
            }
            case OP_regassign: {
                RegassignNode *s = func->GetCodeMemPool()->New<RegassignNode>();
                s->SetPrimType(static_cast<PrimType>(Read()));
                s->SetRegIdx(ImportPreg(func));
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                stmt = s;
                break;
            }
            case OP_iassign: {
                IassignNode *s = func->GetCodeMemPool()->New<IassignNode>();
                s->SetTyIdx(ImportJType());
                s->SetFieldID(static_cast<FieldID>(ReadNum()));
                s->SetAddrExpr(ImportExpression(func));
                s->SetRHS(ImportExpression(func));
                stmt = s;
                break;
            }
            case OP_iassignoff: {
                IassignoffNode *s = func->GetCodeMemPool()->New<IassignoffNode>();
                s->SetPrimType((PrimType)Read());
                s->SetOffset(static_cast<int32>(ReadNum()));
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                s->SetOpnd(ImportExpression(func), kSecondOpnd);
                stmt = s;
                break;
            }
            case OP_iassignspoff:
            case OP_iassignfpoff: {
                IassignFPoffNode *s = func->GetCodeMemPool()->New<IassignFPoffNode>(op);
                s->SetPrimType(static_cast<PrimType>(Read()));
                s->SetOffset(static_cast<int32>(ReadNum()));
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                stmt = s;
                break;
            }
            case OP_blkassignoff: {
                BlkassignoffNode *s = func->GetCodeMemPool()->New<BlkassignoffNode>();
                int32 offsetAlign = static_cast<int32>(ReadNum());
                s->offset = offsetAlign >> kOffset4bit;
                s->alignLog2 = offsetAlign & 0xf;
                s->blockSize = static_cast<int32>(ReadNum());
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                s->SetOpnd(ImportExpression(func), kSecondOpnd);
                stmt = s;
                break;
            }
            case OP_call:
            case OP_virtualcall:
            case OP_virtualicall:
            case OP_superclasscall:
            case OP_interfacecall:
            case OP_interfaceicall:
            case OP_customcall: {
                CallNode *s = func->GetCodeMemPool()->New<CallNode>(mod, op);
                s->SetPUIdx(ImportFuncViaSym(func));
                MIRFunction *f = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(s->GetPUIdx());
                CHECK_FATAL(f != nullptr, "null ptr");
                f->GetFuncSymbol()->SetAppearsInCode(true);
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_callassigned:
            case OP_virtualcallassigned:
            case OP_virtualicallassigned:
            case OP_superclasscallassigned:
            case OP_interfacecallassigned:
            case OP_interfaceicallassigned:
            case OP_customcallassigned: {
                CallNode *s = func->GetCodeMemPool()->New<CallNode>(mod, op);
                s->SetPUIdx(ImportFuncViaSym(func));
                MIRFunction *f = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(s->GetPUIdx());
                CHECK_FATAL(f != nullptr, "null ptr");
                f->GetFuncSymbol()->SetAppearsInCode(true);
                ImportReturnValues(func, &s->GetReturnVec());
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                const auto &calleeName =
                    GlobalTables::GetFunctionTable().GetFunctionFromPuidx(s->GetPUIdx())->GetName();
                if (calleeName == "setjmp") {
                    func->SetHasSetjmp();
                }
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_polymorphiccall: {
                CallNode *s = func->GetCodeMemPool()->New<CallNode>(mod, op);
                s->SetPUIdx(ImportFuncViaSym(func));
                MIRFunction *f = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(s->GetPUIdx());
                CHECK_FATAL(f != nullptr, "null ptr");
                f->GetFuncSymbol()->SetAppearsInCode(true);
                s->SetTyIdx(ImportJType());
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_polymorphiccallassigned: {
                CallNode *s = func->GetCodeMemPool()->New<CallNode>(mod, op);
                s->SetPUIdx(ImportFuncViaSym(func));
                MIRFunction *f = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(s->GetPUIdx());
                CHECK_FATAL(f != nullptr, "null ptr");
                f->GetFuncSymbol()->SetAppearsInCode(true);
                s->SetTyIdx(ImportJType());
                ImportReturnValues(func, &s->GetReturnVec());
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_icallproto:
            case OP_icall: {
                IcallNode *s = func->GetCodeMemPool()->New<IcallNode>(mod, op);
                s->SetRetTyIdx(ImportJType());
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_icallprotoassigned:
            case OP_icallassigned: {
                IcallNode *s = func->GetCodeMemPool()->New<IcallNode>(mod, op);
                s->SetRetTyIdx(ImportJType());
                ImportReturnValues(func, &s->GetReturnVec());
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_intrinsiccall:
            case OP_xintrinsiccall: {
                IntrinsiccallNode *s = func->GetCodeMemPool()->New<IntrinsiccallNode>(mod, op);
                s->SetIntrinsic(static_cast<MIRIntrinsicID>(ReadNum()));
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_intrinsiccallassigned:
            case OP_xintrinsiccallassigned: {
                IntrinsiccallNode *s = func->GetCodeMemPool()->New<IntrinsiccallNode>(mod, op);
                s->SetIntrinsic((MIRIntrinsicID)ReadNum());
                ImportReturnValues(func, &s->GetReturnVec());
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                if (s->GetReturnVec().size() == 1 && s->GetReturnVec()[0].first.Idx() != 0) {
                    MIRSymbol *retsymbol = func->GetSymTab()->GetSymbolFromStIdx(s->GetReturnVec()[0].first.Idx());
                    MIRType *rettype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(retsymbol->GetTyIdx());
                    CHECK_FATAL(rettype != nullptr, "rettype is null in MIRParser::ParseStmtIntrinsiccallAssigned");
                    s->SetPrimType(rettype->GetPrimType());
                }
                stmt = s;
                break;
            }
            case OP_intrinsiccallwithtype: {
                IntrinsiccallNode *s = func->GetCodeMemPool()->New<IntrinsiccallNode>(mod, op);
                s->SetIntrinsic((MIRIntrinsicID)ReadNum());
                s->SetTyIdx(ImportJType());
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_intrinsiccallwithtypeassigned: {
                IntrinsiccallNode *s = func->GetCodeMemPool()->New<IntrinsiccallNode>(mod, op);
                s->SetIntrinsic((MIRIntrinsicID)ReadNum());
                s->SetTyIdx(ImportJType());
                ImportReturnValues(func, &s->GetReturnVec());
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                if (s->GetReturnVec().size() == 1 && s->GetReturnVec()[0].first.Idx() != 0) {
                    MIRSymbol *retsymbol = func->GetSymTab()->GetSymbolFromStIdx(s->GetReturnVec()[0].first.Idx());
                    MIRType *rettype = GlobalTables::GetTypeTable().GetTypeFromTyIdx(retsymbol->GetTyIdx());
                    CHECK_FATAL(rettype != nullptr, "rettype is null in MIRParser::ParseStmtIntrinsiccallAssigned");
                    s->SetPrimType(rettype->GetPrimType());
                }
                stmt = s;
                break;
            }
            case OP_syncenter:
            case OP_syncexit:
            case OP_return: {
                NaryStmtNode *s = func->GetCodeMemPool()->New<NaryStmtNode>(mod, op);
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            case OP_jscatch:
            case OP_cppcatch:
            case OP_finally:
            case OP_endtry:
            case OP_cleanuptry:
            case OP_retsub:
            case OP_membaracquire:
            case OP_membarrelease:
            case OP_membarstorestore:
            case OP_membarstoreload: {
                stmt = mod.CurFuncCodeMemPool()->New<StmtNode>(op);
                break;
            }
            case OP_eval:
            case OP_throw:
            case OP_free:
            case OP_decref:
            case OP_incref:
            case OP_decrefreset:
                CASE_OP_ASSERT_NONNULL
            case OP_igoto: {
                UnaryStmtNode *s = mod.CurFuncCodeMemPool()->New<UnaryStmtNode>(op);
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                stmt = s;
                break;
            }
            case OP_label: {
                LabelNode *s = mod.CurFuncCodeMemPool()->New<LabelNode>();
                s->SetLabelIdx(ImportLabel(func));
                stmt = s;
                break;
            }
            case OP_goto:
            case OP_gosub: {
                GotoNode *s = mod.CurFuncCodeMemPool()->New<GotoNode>(op);
                s->SetOffset(ImportLabel(func));
                stmt = s;
                break;
            }
            case OP_brfalse:
            case OP_brtrue: {
                CondGotoNode *s = mod.CurFuncCodeMemPool()->New<CondGotoNode>(op);
                s->SetOffset(ImportLabel(func));
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                stmt = s;
                break;
            }
            case OP_switch: {
                SwitchNode *s = mod.CurFuncCodeMemPool()->New<SwitchNode>(mod);
                s->SetDefaultLabel(ImportLabel(func));
                auto tagSize = static_cast<uint32>(ReadNum());
                for (uint32 i = 0; i < tagSize; ++i) {
                    int64 casetag = ReadNum();
                    LabelIdx lidx = ImportLabel(func);
                    CasePair cpair = std::make_pair(casetag, lidx);
                    s->GetSwitchTable().push_back(cpair);
                }
                s->SetSwitchOpnd(ImportExpression(func));
                stmt = s;
                break;
            }
            case OP_rangegoto: {
                RangeGotoNode *s = mod.CurFuncCodeMemPool()->New<RangeGotoNode>(mod);
                s->SetTagOffset(static_cast<int32>(ReadNum()));
                uint32 tagSize = static_cast<uint32>(ReadNum());
                for (uint32 i = 0; i < tagSize; ++i) {
                    uint16 casetag = static_cast<uint16>(ReadNum());
                    LabelIdx lidx = ImportLabel(func);
                    s->AddRangeGoto(casetag, lidx);
                }
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                stmt = s;
                break;
            }
            case OP_jstry: {
                JsTryNode *s = mod.CurFuncCodeMemPool()->New<JsTryNode>();
                s->SetCatchOffset(ImportLabel(func));
                s->SetFinallyOffset(ImportLabel(func));
                stmt = s;
                break;
            }
            case OP_cpptry:
            case OP_try: {
                TryNode *s = mod.CurFuncCodeMemPool()->New<TryNode>(mod);
                auto numLabels = static_cast<uint32>(ReadNum());
                for (uint32 i = 0; i < numLabels; ++i) {
                    s->GetOffsets().push_back(ImportLabel(func));
                }
                stmt = s;
                break;
            }
            case OP_catch: {
                CatchNode *s = mod.CurFuncCodeMemPool()->New<CatchNode>(mod);
                auto numTys = static_cast<uint32>(ReadNum());
                for (uint32 i = 0; i < numTys; ++i) {
                    s->PushBack(ImportJType());
                }
                stmt = s;
                break;
            }
            case OP_comment: {
                CommentNode *s = mod.CurFuncCodeMemPool()->New<CommentNode>(mod);
                string str;
                ReadAsciiStr(str);
                s->SetComment(str);
                stmt = s;
                break;
            }
            case OP_dowhile:
            case OP_while: {
                WhileStmtNode *s = mod.CurFuncCodeMemPool()->New<WhileStmtNode>(op);
                s->SetBody(ImportBlockNode(func));
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                stmt = s;
                break;
            }
            case OP_if: {
                IfStmtNode *s = mod.CurFuncCodeMemPool()->New<IfStmtNode>();
                bool hasElsePart = (static_cast<size_t>(ReadNum()) != kFirstOpnd);
                s->SetThenPart(ImportBlockNode(func));
                if (hasElsePart) {
                    s->SetElsePart(ImportBlockNode(func));
                    s->SetNumOpnds(kOperandNumTernary);
                }
                s->SetOpnd(ImportExpression(func), kFirstOpnd);
                stmt = s;
                break;
            }
            case OP_block: {
                stmt = ImportBlockNode(func);
                break;
            }
            case OP_asm: {
                AsmNode *s = mod.CurFuncCodeMemPool()->New<AsmNode>(&mod.GetCurFuncCodeMPAllocator());
                mod.CurFunction()->SetHasAsm();
                s->qualifiers = static_cast<uint32>(ReadNum());
                string str;
                ReadAsciiStr(str);
                s->asmString = str;
                // the outputs
                auto count = static_cast<size_t>(ReadNum());
                UStrIdx strIdx;
                for (size_t i = 0; i < count; ++i) {
                    strIdx = ImportUsrStr();
                    s->outputConstraints.push_back(strIdx);
                }
                ImportReturnValues(func, &s->asmOutputs);
                // the clobber list
                count = static_cast<size_t>(ReadNum());
                for (size_t i = 0; i < count; ++i) {
                    strIdx = ImportUsrStr();
                    s->clobberList.push_back(strIdx);
                }
                // the labels
                count = static_cast<size_t>(ReadNum());
                for (size_t i = 0; i < count; ++i) {
                    LabelIdx lidx = ImportLabel(func);
                    s->gotoLabels.push_back(lidx);
                }
                // the inputs
                numOpr = static_cast<uint8>(ReadNum());
                s->SetNumOpnds(numOpr);
                for (int32 i = 0; i < numOpr; ++i) {
                    strIdx = ImportUsrStr();
                    s->inputConstraints.push_back(strIdx);
                    const std::string &inStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(strIdx);
                    if (inStr[0] == '+') {
                        s->SetHasWriteInputs();
                    }
                }
                for (int32 i = 0; i < numOpr; ++i) {
                    s->GetNopnd().push_back(ImportExpression(func));
                }
                stmt = s;
                break;
            }
            default:
                CHECK_FATAL(false, "Unhandled opcode tag %d", tag);
                break;
        }
        stmt->SetSrcPos(thesrcPosition);
        block->AddStatement(stmt);
    }
    if (func != nullptr) {
        func->SetBody(block);
    }
    return block;
}

void BinaryMplImport::ReadFunctionBodyField()
{
    (void)ReadInt();  /// skip total size
    int32 size = ReadInt();
    for (int64 i = 0; i < size; ++i) {
        PUIdx puIdx = ImportFunction();
        MIRFunction *fn = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
        CHECK_FATAL(fn != nullptr, "null ptr");
        mod.SetCurFunction(fn);
        fn->GetFuncSymbol()->SetAppearsInCode(true);
        localSymTab.clear();
        localSymTab.push_back(nullptr);
        localPregTab.clear();
        localPregTab.push_back(0);
        localLabelTab.clear();
        localLabelTab.push_back(0);

        fn->AllocSymTab();
        fn->AllocPregTab();
        fn->AllocTypeNameTab();
        fn->AllocLabelTab();

        ImportFuncIdInfo(fn);
        ImportLocalTypeNameTable(fn->GetTypeNameTab());
        ImportFormalsStIdx(fn);
        if (mod.GetFlavor() < kMmpl) {
            ImportAliasMap(fn);
        }
        (void)ImportBlockNode(fn);
        mod.AddFunction(fn);
    }
    return;
}
}  // namespace maple
