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

#include "mir_nodes.h"


#include "mir_function.h"

namespace maple {
MIRModule *theMIRModule = nullptr;
std::atomic<uint32> StmtNode::stmtIDNext(1);  // 0 is reserved
uint32 StmtNode::lastPrintedLineNum = 0;
uint16 StmtNode::lastPrintedColumnNum = 0;
const int32 CondGotoNode::probAll = 10000;

const char *GetIntrinsicName(MIRIntrinsicID intrn)
{
    switch (intrn) {
        default:
#define DEF_MIR_INTRINSIC(STR, NAME, INTRN_CLASS, RETURN_TYPE, ...) \
    case INTRN_##STR:                                               \
        return #STR;
#include "intrinsics.def"
#undef DEF_MIR_INTRINSIC
    }
}

MIRType *IreadNode::GetType() const
{
    MIRPtrType *ptrtype = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx));
    CHECK_FATAL(fieldID == 0, "fieldID must be 0");
    return ptrtype->GetPointedType();
}

void BlockNode::AddStatement(StmtNode *stmt)
{
    DEBUG_ASSERT(stmt != nullptr, "null ptr check");
    stmtNodeList.push_back(stmt);
}

void BlockNode::AppendStatementsFromBlock(BlockNode &blk)
{
    if (blk.GetStmtNodes().empty()) {
        return;
    }
    stmtNodeList.splice(stmtNodeList.end(), blk.GetStmtNodes());
}

/// Insert stmt as the first
void BlockNode::InsertFirst(StmtNode *stmt)
{
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmtNodeList.push_front(stmt);
}

/// Insert stmt as the last
void BlockNode::InsertLast(StmtNode *stmt)
{
    DEBUG_ASSERT(stmt != nullptr, "stmt is null");
    stmtNodeList.push_back(stmt);
}

// remove sstmtNode1 from block
void BlockNode::RemoveStmt(const StmtNode *stmtNode1)
{
    DEBUG_ASSERT(stmtNode1 != nullptr, "delete a null stmtment");
    (void)stmtNodeList.erase(stmtNode1);
}

/// Insert stmtNode2 before stmtNode1 in current block.
void BlockNode::InsertBefore(const StmtNode *stmtNode1, StmtNode *stmtNode2)
{
    stmtNodeList.insert(stmtNode1, stmtNode2);
}

/// Insert stmtNode2 after stmtNode1 in current block.
void BlockNode::InsertAfter(const StmtNode *stmtNode1, StmtNode *stmtNode2)
{
    stmtNodeList.insertAfter(stmtNode1, stmtNode2);
}

#ifdef ARK_LITECG_DEBUG
void BaseNode::DumpBase(int32 indent) const
{
    PrintIndentation(indent);
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
}

void UnaryNode::DumpOpnd(const MIRModule &, int32 indent) const
{
    DumpOpnd(indent);
}

void UnaryNode::DumpOpnd(int32 indent) const
{
    LogInfo::MapleLogger() << " (";
    if (uOpnd != nullptr) {
        uOpnd->Dump(indent);
    }
    LogInfo::MapleLogger() << ")";
}

void UnaryNode::Dump(int32 indent) const
{
    BaseNode::DumpBase(0);
    DumpOpnd(*theMIRModule, indent);
}

void TypeCvtNode::Dump(int32 indent) const
{
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " ";
    LogInfo::MapleLogger() << GetPrimTypeName(GetPrimType()) << " " << GetPrimTypeName(FromType());
    DumpOpnd(*theMIRModule, indent);
}

void RetypeNode::Dump(int32 indent) const
{
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " ";
    LogInfo::MapleLogger() << GetPrimTypeName(GetPrimType()) << " ";
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (ty->GetKind() == kTypeScalar) {
        LogInfo::MapleLogger() << "<";
        ty->Dump(indent + 1);
        LogInfo::MapleLogger() << ">";
    } else {
        ty->Dump(indent + 1);
    }
    DumpOpnd(*theMIRModule, indent);
}

void ExtractbitsNode::Dump(int32 indent) const
{
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
    if (GetOpCode() == OP_extractbits) {
        LogInfo::MapleLogger() << " " << static_cast<int32>(bitsOffset) << " " << static_cast<int32>(bitsSize);
    } else {
        LogInfo::MapleLogger() << " " << static_cast<int32>(bitsSize);
    }
    DumpOpnd(*theMIRModule, indent);
}

void IreadNode::Dump(int32 indent) const
{
    BaseNode::DumpBase(0);
    LogInfo::MapleLogger() << " ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
    LogInfo::MapleLogger() << " " << fieldID;
    DumpOpnd(*theMIRModule, indent);
}

void BinaryNode::Dump(int32 indent) const
{
    BaseNode::DumpBase(0);
    BinaryOpnds::Dump(indent);
}

void BinaryOpnds::Dump(int32 indent) const
{
    LogInfo::MapleLogger() << " (";
    if (bOpnd[0]->IsLeaf() && bOpnd[1]->IsLeaf()) {
        bOpnd[0]->Dump(0);
        LogInfo::MapleLogger() << ", ";
        bOpnd[1]->Dump(0);
    } else {
        LogInfo::MapleLogger() << '\n';
        PrintIndentation(indent + 1);
        bOpnd[0]->Dump(indent + 1);
        LogInfo::MapleLogger() << ",\n";
        PrintIndentation(indent + 1);
        bOpnd[1]->Dump(indent + 1);
    }
    LogInfo::MapleLogger() << ")";
}

void CompareNode::Dump(int32 indent) const
{
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
    LogInfo::MapleLogger() << " " << GetPrimTypeName(opndType);
    BinaryOpnds::Dump(indent);
}

void NaryOpnds::Dump(int32 indent) const
{
    LogInfo::MapleLogger() << " (";
    if (GetNopndSize() == 0) {
        LogInfo::MapleLogger() << ")";
        return;
    }
    if (GetNopndSize() == 1) {
        GetNopndAt(0)->Dump(indent);
    } else {
        bool allisLeaf = true;
        for (size_t i = 0; i < GetNopndSize(); ++i)
            if (!GetNopndAt(i)->IsLeaf()) {
                allisLeaf = false;
                break;
            }
        if (allisLeaf) {
            GetNopndAt(0)->Dump(0);
            for (size_t i = 1; i < GetNopndSize(); ++i) {
                LogInfo::MapleLogger() << ", ";
                GetNopndAt(i)->Dump(0);
            }
        } else {
            LogInfo::MapleLogger() << '\n';
            PrintIndentation(indent + 1);
            GetNopndAt(0)->Dump(indent + 1);
            for (size_t i = 1; i < GetNopndSize(); ++i) {
                LogInfo::MapleLogger() << ",\n";
                PrintIndentation(indent + 1);
                GetNopndAt(i)->Dump(indent + 1);
            }
        }
    }
    LogInfo::MapleLogger() << ")";
}

void DeoptBundleInfo::Dump(int32 indent) const
{
    size_t deoptBundleSize = deoptBundleInfo.size();
    if (deoptBundleSize == 0) {
        return;
    }
    LogInfo::MapleLogger() << " deopt: (";
    bool isFirstItem = true;
    for (const auto &elem : deoptBundleInfo) {
        if (!isFirstItem) {
            LogInfo::MapleLogger() << ", ";
        } else {
            isFirstItem = false;
        }
        LogInfo::MapleLogger() << elem.first << ": ";
        auto valueKind = elem.second.GetMapleValueKind();
        if (valueKind == MapleValue::kPregKind) {
            LogInfo::MapleLogger() << "%" << elem.second.GetPregIdx() << " ";
        } else if (valueKind == MapleValue::kConstKind) {
            if (elem.second.GetConstValue().GetKind() != kConstInt) {
                CHECK_FATAL(false, "not supported currently");
            }
            LogInfo::MapleLogger() << static_cast<const MIRIntConst &>(elem.second.GetConstValue()).GetValue() << " ";
        }
    }
    LogInfo::MapleLogger() << ")";
}

void NaryNode::Dump(int32 indent) const
{
    BaseNode::DumpBase(0);
    NaryOpnds::Dump(indent);
}
#endif

#ifdef ARK_LITECG_DEBUG
void IntrinsicopNode::Dump(int32 indent) const
{
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
    LogInfo::MapleLogger() << " " << GetIntrinsicName(GetIntrinsic());
    NaryOpnds::Dump(indent);
}

void ConstvalNode::Dump(int32) const
{
    if (GetConstVal()->GetType().GetKind() != kTypePointer) {
        BaseNode::DumpBase(0);
        LogInfo::MapleLogger() << " ";
    }
    GetConstVal()->Dump();
}

void AddrofNode::Dump(int32) const
{
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
    DEBUG_ASSERT(theMIRModule->CurFunction() != nullptr, "theMIRModule->CurFunction() should not be nullptr");
    const MIRSymbol *st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(GetStIdx());
    LogInfo::MapleLogger() << (GetStIdx().Islocal() ? " %" : " $");
    DEBUG_ASSERT(st != nullptr, "null ptr check");
    LogInfo::MapleLogger() << st->GetName();
    if (fieldID != 0) {
        LogInfo::MapleLogger() << " " << fieldID;
    }
}

void DreadoffNode::Dump(int32) const
{
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
    const MIRSymbol *st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(stIdx);
    LogInfo::MapleLogger() << (stIdx.Islocal() ? " %" : " $");
    DEBUG_ASSERT(st != nullptr, "null ptr check");
    LogInfo::MapleLogger() << st->GetName();
    LogInfo::MapleLogger() << " " << offset;
}

void RegreadNode::Dump(int32) const
{
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name << " " << GetPrimTypeName(GetPrimType());
    if (regIdx >= 0) {
        LogInfo::MapleLogger() << " %"
                               << theMIRModule->CurFunction()->GetPregTab()->PregFromPregIdx(regIdx)->GetPregNo();
        return;
    }
    LogInfo::MapleLogger() << " %%";
    switch (regIdx) {
        case -kSregSp:
            LogInfo::MapleLogger() << "SP";
            break;
        case -kSregFp:
            LogInfo::MapleLogger() << "FP";
            break;
        default:
            int32 retValIdx = (-regIdx) - kSregRetval0;
            LogInfo::MapleLogger() << "retval" << retValIdx;
            break;
    }
}

void StmtNode::DumpBase(int32 indent) const
{
    srcPosition.DumpLoc(lastPrintedLineNum, lastPrintedColumnNum);
    // dump stmtFreqs
    DEBUG_ASSERT(theMIRModule->CurFunction() != nullptr, "theMIRModule->CurFunction() should not be nullptr");
    PrintIndentation(indent);
    LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOpCode()).name;
}

void StmtNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    LogInfo::MapleLogger() << '\n';
}

void DassignNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    const MIRSymbol *st = theMIRModule->CurFunction()->GetLocalOrGlobalSymbol(stIdx);
    DEBUG_ASSERT(st != nullptr, "null ptr check");
    LogInfo::MapleLogger() << (st->IsLocal() ? " %" : " $");
    LogInfo::MapleLogger() << st->GetName() << " " << fieldID;
    LogInfo::MapleLogger() << " (";
    if (GetRHS() != nullptr) {
        GetRHS()->Dump(indent + 1);
    } else {
        LogInfo::MapleLogger() << "/*empty-rhs*/";
    }
    LogInfo::MapleLogger() << ")\n";
}

void RegassignNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    LogInfo::MapleLogger() << " " << GetPrimTypeName(GetPrimType());
    if (regIdx >= 0) {
        LogInfo::MapleLogger() << " %"
                               << theMIRModule->CurFunction()->GetPregTab()->PregFromPregIdx(regIdx)->GetPregNo();
    } else {
        LogInfo::MapleLogger() << " %%";
        switch (regIdx) {
            case -kSregSp:
                LogInfo::MapleLogger() << "SP";
                break;
            case -kSregFp:
                LogInfo::MapleLogger() << "FP";
                break;
            case -kSregRetval0:
                LogInfo::MapleLogger() << "retval0";
                break;
            // no default
            default:
                break;
        }
    }
    LogInfo::MapleLogger() << " (";
    UnaryStmtNode::Opnd(0)->Dump(indent + 1);
    LogInfo::MapleLogger() << ")\n";
}

void IassignNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    LogInfo::MapleLogger() << " ";
    GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
    LogInfo::MapleLogger() << " " << fieldID;
    LogInfo::MapleLogger() << " (";
    if (addrExpr->IsLeaf() && rhs->IsLeaf()) {
        addrExpr->Dump(0);
        LogInfo::MapleLogger() << ", ";
        rhs->Dump(0);
    } else {
        LogInfo::MapleLogger() << '\n';
        PrintIndentation(indent + 1);
        addrExpr->Dump(indent + 1);
        LogInfo::MapleLogger() << ", \n";
        PrintIndentation(indent + 1);
        rhs->Dump(indent + 1);
    }
    LogInfo::MapleLogger() << ")\n";
}

void GotoNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    if (offset == 0) {
        LogInfo::MapleLogger() << '\n';
    } else {
        LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName(static_cast<LabelIdx>(offset))
                               << '\n';
    }
}

void CondGotoNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    LogInfo::MapleLogger() << " @" << theMIRModule->CurFunction()->GetLabelName(static_cast<LabelIdx>(offset));
    LogInfo::MapleLogger() << " (";
    Opnd(0)->Dump(indent);
    LogInfo::MapleLogger() << ")\n";
}

void SwitchNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    LogInfo::MapleLogger() << " (";
    switchOpnd->Dump(indent);
    if (defaultLabel == 0) {
        LogInfo::MapleLogger() << ") 0 {";
    } else {
        LogInfo::MapleLogger() << ") @" << theMIRModule->CurFunction()->GetLabelName(defaultLabel) << " {";
    }
    for (auto it = switchTable.begin(); it != switchTable.end(); it++) {
        LogInfo::MapleLogger() << '\n';
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << std::hex << "0x" << it->first << std::dec;
        LogInfo::MapleLogger() << ": goto @" << theMIRModule->CurFunction()->GetLabelName(it->second);
    }
    LogInfo::MapleLogger() << " }\n";
}

void RangeGotoNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    LogInfo::MapleLogger() << " (";
    Opnd(0)->Dump(indent);
    LogInfo::MapleLogger() << ") " << tagOffset << " {";
    for (auto it = rangegotoTable.begin(); it != rangegotoTable.end(); it++) {
        LogInfo::MapleLogger() << '\n';
        PrintIndentation(indent + 1);
        LogInfo::MapleLogger() << std::hex << "0x" << it->first << std::dec;
        LogInfo::MapleLogger() << ": goto @" << theMIRModule->CurFunction()->GetLabelName(it->second);
    }
    LogInfo::MapleLogger() << " }\n";
}

void UnaryStmtNode::DumpOpnd(const MIRModule &, int32 indent) const
{
    DumpOpnd(indent);
}

void UnaryStmtNode::DumpOpnd(int32 indent) const
{
    LogInfo::MapleLogger() << " (";
    if (uOpnd != nullptr) {
        uOpnd->Dump(indent);
    }
    LogInfo::MapleLogger() << ")\n";
}

void UnaryStmtNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    DumpOpnd(indent);
}

void IfStmtNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    LogInfo::MapleLogger() << " (";
    Opnd()->Dump(indent);
    LogInfo::MapleLogger() << ")";
    thenPart->Dump(indent);
    if (elsePart) {
        PrintIndentation(indent);
        LogInfo::MapleLogger() << "else {\n";
        for (auto &stmt : elsePart->GetStmtNodes()) {
            stmt.Dump(indent + 1);
        }
        PrintIndentation(indent);
        LogInfo::MapleLogger() << "}\n";
    }
}

void BinaryStmtNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    BinaryOpnds::Dump(indent);
    LogInfo::MapleLogger() << '\n';
}

void NaryStmtNode::Dump(int32 indent) const
{
    StmtNode::DumpBase(indent);
    NaryOpnds::Dump(indent);
    LogInfo::MapleLogger() << '\n';
}

void DumpCallReturns(const MIRModule &mod, CallReturnVector nrets, int32 indent)
{
    const MIRFunction *mirFunc = mod.CurFunction();
    if (nrets.empty()) {
        LogInfo::MapleLogger() << " {}\n";
        return;
    } else if (nrets.size() == 1) {
        StIdx stIdx = nrets.begin()->first;
        RegFieldPair regFieldPair = nrets.begin()->second;
        if (!regFieldPair.IsReg()) {
            const MIRSymbol *st = mirFunc->GetLocalOrGlobalSymbol(stIdx);
            DEBUG_ASSERT(st != nullptr, "st is null");
            FieldID fieldID = regFieldPair.GetFieldID();
            LogInfo::MapleLogger() << " { dassign ";
            LogInfo::MapleLogger() << (stIdx.Islocal() ? "%" : "$");
            LogInfo::MapleLogger() << st->GetName() << " " << fieldID << " }\n";
            return;
        } else {
            PregIdx regIdx = regFieldPair.GetPregIdx();
            const MIRPreg *mirPreg = mirFunc->GetPregItem(static_cast<PregIdx>(regIdx));
            DEBUG_ASSERT(mirPreg != nullptr, "mirPreg is null");
            LogInfo::MapleLogger() << " { regassign";
            LogInfo::MapleLogger() << " " << GetPrimTypeName(mirPreg->GetPrimType());
            LogInfo::MapleLogger() << " %" << mirPreg->GetPregNo() << "}\n";
            return;
        }
    }
    LogInfo::MapleLogger() << " {\n";
    constexpr int32 spaceNum = 2;
    for (auto it = nrets.begin(); it != nrets.end(); it++) {
        PrintIndentation(indent + spaceNum);
        StIdx stIdx = (it)->first;
        RegFieldPair regFieldPair = it->second;
        if (!regFieldPair.IsReg()) {
            FieldID fieldID = regFieldPair.GetFieldID();
            LogInfo::MapleLogger() << "dassign";
            const MIRSymbol *st = mirFunc->GetLocalOrGlobalSymbol(stIdx);
            DEBUG_ASSERT(st != nullptr, "st is null");
            LogInfo::MapleLogger() << (stIdx.Islocal() ? " %" : " $");
            LogInfo::MapleLogger() << st->GetName() << " " << fieldID << '\n';
        } else {
            PregIdx regIdx = regFieldPair.GetPregIdx();
            const MIRPreg *mirPreg = mirFunc->GetPregItem(static_cast<PregIdx>(regIdx));
            DEBUG_ASSERT(mirPreg != nullptr, "mirPreg is null");
            LogInfo::MapleLogger() << "regassign"
                                   << " " << GetPrimTypeName(mirPreg->GetPrimType());
            LogInfo::MapleLogger() << " %" << mirPreg->GetPregNo() << '\n';
        }
    }
    PrintIndentation(indent + 1);
    LogInfo::MapleLogger() << "}\n";
}

void CallNode::Dump(int32 indent, bool newline) const
{
    StmtNode::DumpBase(indent);
    if (tyIdx != 0u) {
        LogInfo::MapleLogger() << " ";
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(indent + 1);
    }
    CHECK(puIdx < GlobalTables::GetFunctionTable().GetFuncTable().size(), "index out of range in CallNode::Dump");
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    DumpCallConvInfo();
    LogInfo::MapleLogger() << " &" << func->GetName();
    NaryOpnds::Dump(indent);
    DeoptBundleInfo::Dump(indent);
    if (kOpcodeInfo.IsCallAssigned(GetOpCode())) {
        DumpCallReturns(*theMIRModule, this->GetReturnVec(), indent);
    } else if (newline) {
        LogInfo::MapleLogger() << '\n';
    }
}
#endif

MIRType *IcallNode::GetCallReturnType()
{
    if (op == OP_icall || op == OP_icallassigned) {
        return GlobalTables::GetTypeTable().GetTypeFromTyIdx(retTyIdx);
    }
    // icallproto or icallprotoassigned
    MIRFuncType *funcType = static_cast<MIRFuncType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(retTyIdx));
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(funcType->GetRetTyIdx());
}

#ifdef ARK_LITECG_DEBUG
void IcallNode::Dump(int32 indent, bool newline) const
{
    StmtNode::DumpBase(indent);
    DumpCallConvInfo();
    if (op == OP_icallproto || op == OP_icallprotoassigned) {
        LogInfo::MapleLogger() << " ";
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(retTyIdx)->Dump(indent + 1);
    }
    NaryOpnds::Dump(indent);
    DeoptBundleInfo::Dump(indent);
    if (kOpcodeInfo.IsCallAssigned(GetOpCode())) {
        DumpCallReturns(*theMIRModule, this->returnValues, indent);
    } else if (newline) {
        LogInfo::MapleLogger() << '\n';
    }
}

void IntrinsiccallNode::Dump(int32 indent, bool newline) const
{
    StmtNode::DumpBase(indent);
    if (tyIdx != 0u) {
        LogInfo::MapleLogger() << " ";
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(indent + 1);
    }
    if (GetOpCode() == OP_intrinsiccall || GetOpCode() == OP_intrinsiccallassigned ||
        GetOpCode() == OP_intrinsiccallwithtype) {
        LogInfo::MapleLogger() << " " << GetIntrinsicName(intrinsic);
    } else {
        LogInfo::MapleLogger() << " " << intrinsic;
    }
    NaryOpnds::Dump(indent);
    if (kOpcodeInfo.IsCallAssigned(GetOpCode())) {
        DumpCallReturns(*theMIRModule, this->GetReturnVec(), indent);
    } else if (newline) {
        LogInfo::MapleLogger() << '\n';
    }
}

void BlockNode::Dump(int32 indent, const MIRSymbolTable *theSymTab, MIRPregTable *thePregTab, bool withInfo,
                     bool isFuncbody, MIRFlavor flavor) const
{
    if (!withInfo) {
        LogInfo::MapleLogger() << " {\n";
    }
    // output puid for debugging purpose
    DEBUG_ASSERT(theMIRModule->CurFunction() != nullptr, "theMIRModule->CurFunction() should not be nullptr");
    if (isFuncbody) {
        theMIRModule->CurFunction()->DumpFuncBody(indent);
        if (theSymTab != nullptr || thePregTab != nullptr) {
            // print the locally declared type names
            if (theMIRModule->CurFunction()->HaveTypeNameTab()) {
                constexpr int32 spaceNum = 2;
                for (auto it : theMIRModule->CurFunction()->GetGStrIdxToTyIdxMap()) {
                    const std::string &name = GlobalTables::GetStrTable().GetStringFromStrIdx(it.first);
                    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it.second);
                    PrintIndentation(indent + 1);
                    LogInfo::MapleLogger() << "type %" << name << " ";
                    if (type->GetKind() != kTypeByName) {
                        type->Dump(indent + spaceNum, true);
                    } else {
                        type->Dump(indent + spaceNum);
                    }
                    LogInfo::MapleLogger() << '\n';
                }
            }
            // print the locally declared variables
            DEBUG_ASSERT(theSymTab != nullptr, "theSymTab should not be nullptr");
            theSymTab->Dump(true, indent + 1, false, flavor); /* first:isLocal, third:printDeleted */
            if (thePregTab != nullptr) {
                thePregTab->DumpPregsWithTypes(indent + 1);
            }
        }
        LogInfo::MapleLogger() << '\n';
    }
    srcPosition.DumpLoc(lastPrintedLineNum, lastPrintedColumnNum);
    for (auto &stmt : GetStmtNodes()) {
        stmt.Dump(indent + 1);
    }
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "}\n";
}

void LabelNode::Dump(int32) const
{
    DEBUG_ASSERT(theMIRModule->CurFunction() != nullptr, "theMIRModule->CurFunction() should not be nullptr");
    if (theMIRModule->CurFunction()->WithLocInfo()) {
        srcPosition.DumpLoc(lastPrintedLineNum, lastPrintedColumnNum);
    }
    LogInfo::MapleLogger() << "@" << theMIRModule->CurFunction()->GetLabelName(labelIdx) << "\n";
}

void CommentNode::Dump(int32 indent) const
{
    srcPosition.DumpLoc(lastPrintedLineNum, lastPrintedColumnNum);
    PrintIndentation(indent);
    LogInfo::MapleLogger() << "#" << comment << '\n';
}

void EmitStr(const MapleString &mplStr)
{
    const char *str = mplStr.c_str();
    size_t len = mplStr.length();
    LogInfo::MapleLogger() << "\"";

    // don't expand special character; convert all \s to \\s in string
    for (size_t i = 0; i < len; ++i) {
        /* Referred to GNU AS: 3.6.1.1 Strings */
        constexpr int kBufSize = 5;
        constexpr int kFirstChar = 0;
        constexpr int kSecondChar = 1;
        constexpr int kThirdChar = 2;
        constexpr int kLastChar = 4;
        char buf[kBufSize];
        if (isprint(*str)) {
            buf[kFirstChar] = *str;
            buf[kSecondChar] = 0;
            if (*str == '\\' || *str == '\"') {
                buf[kFirstChar] = '\\';
                buf[kSecondChar] = *str;
                buf[kThirdChar] = 0;
            }
            LogInfo::MapleLogger() << buf;
        } else if (*str == '\b') {
            LogInfo::MapleLogger() << "\\b";
        } else if (*str == '\n') {
            LogInfo::MapleLogger() << "\\n";
        } else if (*str == '\r') {
            LogInfo::MapleLogger() << "\\r";
        } else if (*str == '\t') {
            LogInfo::MapleLogger() << "\\t";
        } else if (*str == '\0') {
            buf[kFirstChar] = '\\';
            buf[kSecondChar] = '0';
            buf[kThirdChar] = 0;
            LogInfo::MapleLogger() << buf;
        } else {
            /* all others, print as number */
            int ret = snprintf_s(buf, sizeof(buf), kBufSize - 1, "\\%03o", static_cast<unsigned char>(*str) & 0xFF);
            if (ret < 0) {
                FATAL(kLncFatal, "snprintf_s failed");
            }
            buf[kLastChar] = '\0';
            LogInfo::MapleLogger() << buf;
        }
        str++;
    }

    LogInfo::MapleLogger() << "\"\n";
}

std::string SafetyCheckStmtNode::GetFuncName() const
{
    return GlobalTables::GetStrTable().GetStringFromStrIdx(funcNameIdx);
}
#endif
}  // namespace maple
