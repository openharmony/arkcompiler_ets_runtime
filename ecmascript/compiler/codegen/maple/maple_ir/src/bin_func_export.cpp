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

#include "mir_function.h"
#include "opcode_info.h"
#include "mir_pragma.h"
#include "mir_builder.h"
#include "bin_mplt.h"
#include <sstream>
#include <vector>

using namespace std;
namespace maple {
void BinaryMplExport::OutputInfoVector(const MIRInfoVector &infoVector, const MapleVector<bool> &infoVectorIsString)
{
    if (!mod.IsWithDbgInfo()) {
        Write(0);
        return;
    }
    WriteNum(infoVector.size());
    for (uint32 i = 0; i < infoVector.size(); i++) {
        OutputStr(infoVector[i].first);
        WriteNum(infoVectorIsString[i] ? 1 : 0);
        if (!infoVectorIsString[i]) {
            WriteNum(infoVector[i].second);
        } else {
            OutputStr(GStrIdx(infoVector[i].second));
        }
    }
}

void BinaryMplExport::OutputFuncIdInfo(MIRFunction *func)
{
    WriteNum(kBinFuncIdInfoStart);
    WriteNum(func->GetPuidxOrigin());  // the funcid
    OutputInfoVector(func->GetInfoVector(), func->InfoIsString());
    if (mod.GetFlavor() == kFlavorLmbc) {
        WriteNum(func->GetFrameSize());
    }
}

void BinaryMplExport::OutputBaseNode(const BaseNode *b)
{
    Write(static_cast<uint8>(b->GetOpCode()));
    Write(static_cast<uint8>(b->GetPrimType()));
}

void BinaryMplExport::OutputLocalSymbol(MIRSymbol *sym)
{
    CHECK_FATAL(sym != nullptr, "null ptr");
    std::unordered_map<const MIRSymbol *, int64>::iterator it = localSymMark.find(sym);
    if (it != localSymMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    WriteNum(kBinSymbol);
    OutputStr(sym->GetNameStrIdx());
    WriteNum(sym->GetSKind());
    WriteNum(sym->GetStorageClass());
    size_t mark = localSymMark.size();
    localSymMark[sym] = static_cast<int64>(mark);
    OutputTypeAttrs(sym->GetAttrs());
    WriteNum(static_cast<int64>(sym->GetIsTmp()));
    if (sym->GetSKind() == kStVar || sym->GetSKind() == kStFunc) {
        OutputSrcPos(sym->GetSrcPosition());
    }
    OutputType(sym->GetTyIdx());
    if (sym->GetSKind() == kStPreg) {
        OutputPreg(sym->GetPreg());
    } else if (sym->GetSKind() == kStConst || sym->GetSKind() == kStVar) {
        OutputConst(sym->GetKonst());
    } else if (sym->GetSKind() == kStFunc) {
        OutputFuncViaSym(sym->GetFunction()->GetPuidx());
    } else {
        CHECK_FATAL(false, "should not used");
    }
}

void BinaryMplExport::OutputPreg(MIRPreg *preg)
{
    if (preg->GetPregNo() < 0) {
        WriteNum(kBinSpecialReg);
        Write(static_cast<uint8>(-preg->GetPregNo()));
        return;
    }
    std::unordered_map<const MIRPreg *, int64>::iterator it = localPregMark.find(preg);
    if (it != localPregMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    WriteNum(kBinPreg);
    Write(static_cast<uint8>(preg->GetPrimType()));
    size_t mark = localPregMark.size();
    localPregMark[preg] = static_cast<int>(mark);
}

void BinaryMplExport::OutputLabel(LabelIdx lidx)
{
    std::unordered_map<LabelIdx, int64>::iterator it = labelMark.find(lidx);
    if (it != labelMark.end()) {
        WriteNum(-(it->second));
        return;
    }

    WriteNum(kBinLabel);
    size_t mark = labelMark.size();
    labelMark[lidx] = static_cast<int64>(mark);
}

void BinaryMplExport::OutputLocalTypeNameTab(const MIRTypeNameTable *typeNameTab)
{
    WriteNum(kBinTypenameStart);
    WriteNum(static_cast<int64>(typeNameTab->Size()));
    for (std::pair<GStrIdx, TyIdx> it : typeNameTab->GetGStrIdxToTyIdxMap()) {
        OutputStr(it.first);
        OutputType(it.second);
    }
}

void BinaryMplExport::OutputFormalsStIdx(MIRFunction *func)
{
    WriteNum(kBinFormalStart);
    WriteNum(func->GetFormalDefVec().size());
    for (FormalDef formalDef : func->GetFormalDefVec()) {
        OutputLocalSymbol(formalDef.formalSym);
    }
}

void BinaryMplExport::OutputAliasMap(MapleMap<GStrIdx, MIRAliasVars> &aliasVarMap)
{
    WriteNum(kBinAliasMapStart);
    WriteInt(static_cast<int32>(aliasVarMap.size()));
    for (std::pair<GStrIdx, MIRAliasVars> it : aliasVarMap) {
        OutputStr(it.first);
        OutputStr(it.second.mplStrIdx);
        OutputType(it.second.tyIdx);
        OutputStr(it.second.sigStrIdx);
    }
}

void BinaryMplExport::OutputFuncViaSym(PUIdx puIdx)
{
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    MIRSymbol *funcSt = GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx());
    OutputSymbol(funcSt);
}

static SrcPosition lastOutputSrcPosition;

void BinaryMplExport::OutputSrcPos(const SrcPosition &pos)
{
    if (!mod.IsWithDbgInfo()) {
        return;
    }
    if (pos.FileNum() == 0 || pos.LineNum() == 0) {  // error case, so output 0
        WriteNum(lastOutputSrcPosition.RawData());
        WriteNum(lastOutputSrcPosition.LineNum());
        return;
    }
    WriteNum(pos.RawData());
    WriteNum(pos.LineNum());
    lastOutputSrcPosition = pos;
}

void BinaryMplExport::OutputReturnValues(const CallReturnVector *retv)
{
    WriteNum(kBinReturnvals);
    WriteNum(static_cast<int64>(retv->size()));
    for (uint32 i = 0; i < retv->size(); i++) {
        RegFieldPair rfp = (*retv)[i].second;
        if (rfp.IsReg()) {
            MIRPreg *preg = curFunc->GetPregTab()->PregFromPregIdx(rfp.GetPregIdx());
            OutputPreg(preg);
        } else {
            WriteNum(0);
            WriteNum((rfp.GetFieldID()));
            OutputLocalSymbol(curFunc->GetLocalOrGlobalSymbol((*retv)[i].first));
        }
    }
}

}  // namespace maple
