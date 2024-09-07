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

#include "mir_symbol.h"
#include "mir_function.h"
#include "reflection_analysis.h"
#include "printing.h"
#include "string_utils.h"

namespace maple {
using namespace namemangler;

uint32 MIRSymbol::lastPrintedLineNum = 0;
uint16 MIRSymbol::lastPrintedColumnNum = 0;

void MIRSymbol::SetNameStrIdx(const std::string &name)
{
    nameStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
}

#ifdef ARK_LITECG_DEBUG
bool MIRSymbol::HasAddrOfValues() const
{
    return StringUtils::StartsWith(GetName(), VTAB_PREFIX_STR) || StringUtils::StartsWith(GetName(), ITAB_PREFIX_STR) ||
           StringUtils::StartsWith(GetName(), kVtabOffsetTabStr) ||
           StringUtils::StartsWith(GetName(), kDecoupleStaticKeyStr) || IsClassInitBridge() || IsReflectionInfo() ||
           IsReflectionHashTabBucket() || IsReflectionStrTab() || IsITabConflictInfo() || IsRegJNITab() ||
           IsRegJNIFuncTab() || IsLiteral();
}

bool MIRSymbol::IsLiteral() const
{
    return StringUtils::StartsWith(GetName(), kConstString);
}

bool MIRSymbol::IsLiteralPtr() const
{
    return StringUtils::StartsWith(GetName(), kConstStringPtr);
}
#endif

MIRType *MIRSymbol::GetType() const
{
    return GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
}

#ifdef ARK_LITECG_DEBUG
bool MIRSymbol::PointsToConstString() const
{
    MIRType *origType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    if (origType->GetKind() == kTypePointer) {
        return static_cast<MIRPtrType *>(origType)->PointsToConstString();
    }
    return false;
}

bool MIRSymbol::IsConstString() const
{
    return typeAttrs.GetAttr(ATTR_static) && typeAttrs.GetAttr(ATTR_final) && PointsToConstString();
}

bool MIRSymbol::IsReflectionStrTab() const
{
    return StringUtils::StartsWith(GetName(), kReflectionStrtabPrefixStr) ||
           StringUtils::StartsWith(GetName(), kReflectionStartHotStrtabPrefixStr) ||
           StringUtils::StartsWith(GetName(), kReflectionBothHotStrTabPrefixStr) ||
           StringUtils::StartsWith(GetName(), kReflectionRunHotStrtabPrefixStr) ||
           StringUtils::StartsWith(GetName(), kReflectionNoEmitStrtabPrefixStr);
}

bool MIRSymbol::IsRegJNITab() const
{
    return false;
}

bool MIRSymbol::IsRegJNIFuncTab() const
{
    return StringUtils::StartsWith(GetName(), kRegJNIFuncTabPrefixStr);
}

bool MIRSymbol::IsMuidTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidPrefixStr);
}

bool MIRSymbol::IsMuidRoTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidRoPrefixStr);
}

bool MIRSymbol::IsCodeLayoutInfo() const
{
    return StringUtils::StartsWith(GetName(), kFunctionLayoutStr);
}

std::string MIRSymbol::GetMuidTabName() const
{
    if (!IsMuidTab()) {
        return "";
    }
    size_t idx = GetName().find(kFileNameSplitterStr);
    return (idx != std::string::npos) ? GetName().substr(0, idx) : "";
}

bool MIRSymbol::IsMuidFuncDefTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidFuncDefTabPrefixStr);
}

bool MIRSymbol::IsMuidFuncDefOrigTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidFuncDefOrigTabPrefixStr);
}

bool MIRSymbol::IsMuidFuncInfTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidFuncInfTabPrefixStr);
}

bool MIRSymbol::IsMuidFuncUndefTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidFuncUndefTabPrefixStr);
}

bool MIRSymbol::IsMuidDataDefTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidDataDefTabPrefixStr);
}

bool MIRSymbol::IsMuidDataDefOrigTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidDataDefOrigTabPrefixStr);
}

bool MIRSymbol::IsMuidDataUndefTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidDataUndefTabPrefixStr);
}

bool MIRSymbol::IsMuidRangeTab() const
{
    return StringUtils::StartsWith(GetName(), kMuidRangeTabPrefixStr);
}

bool MIRSymbol::IsArrayClassCache() const
{
    return StringUtils::StartsWith(GetName(), kArrayClassCacheTable);
}

bool MIRSymbol::IsArrayClassCacheName() const
{
    return StringUtils::StartsWith(GetName(), kArrayClassCacheNameTable);
}

bool MIRSymbol::IsClassInitBridge() const
{
    return StringUtils::StartsWith(GetName(), CLASS_INIT_BRIDGE_PREFIX_STR);
}

bool MIRSymbol::IsReflectionHashTabBucket() const
{
    return StringUtils::StartsWith(GetName(), kMuidClassMetadataBucketPrefixStr);
}

bool MIRSymbol::IsReflectionInfo() const
{
    return IsReflectionClassInfo() || IsReflectionClassInfoRO() || IsReflectionFieldsInfo() ||
           IsReflectionFieldsInfoCompact() || IsReflectionMethodsInfo() || IsReflectionPrimitiveClassInfo() ||
           IsReflectionSuperclassInfo() || IsReflectionMethodsInfoCompact();
}

bool MIRSymbol::IsReflectionFieldsInfo() const
{
    return StringUtils::StartsWith(GetName(), kFieldsInfoPrefixStr);
}

bool MIRSymbol::IsReflectionFieldsInfoCompact() const
{
    return StringUtils::StartsWith(GetName(), kFieldsInfoCompactPrefixStr);
}

bool MIRSymbol::IsReflectionSuperclassInfo() const
{
    return StringUtils::StartsWith(GetName(), SUPERCLASSINFO_PREFIX_STR);
}

bool MIRSymbol::IsReflectionFieldOffsetData() const
{
    return StringUtils::StartsWith(GetName(), kFieldOffsetDataPrefixStr);
}

bool MIRSymbol::IsReflectionMethodAddrData() const
{
    return (GetName().find(kMethodAddrDataPrefixStr) == 0);
}

bool MIRSymbol::IsReflectionMethodSignature() const
{
    return (GetName().find(kMethodSignaturePrefixStr) == 0);
}

bool MIRSymbol::IsReflectionClassInfo() const
{
    return StringUtils::StartsWith(GetName(), CLASSINFO_PREFIX_STR);
}

bool MIRSymbol::IsReflectionClassInfoPtr() const
{
    return StringUtils::StartsWith(GetName(), kClassINfoPtrPrefixStr);
}

bool MIRSymbol::IsReflectionClassInfoRO() const
{
    return StringUtils::StartsWith(GetName(), CLASSINFO_RO_PREFIX_STR);
}

bool MIRSymbol::IsITabConflictInfo() const
{
    return StringUtils::StartsWith(GetName(), ITAB_CONFLICT_PREFIX_STR);
}

bool MIRSymbol::IsReflectionPrimitiveClassInfo() const
{
    return StringUtils::StartsWith(GetName(), PRIMITIVECLASSINFO_PREFIX_STR);
}

bool MIRSymbol::IsReflectionMethodsInfo() const
{
    return StringUtils::StartsWith(GetName(), kMethodsInfoPrefixStr);
}

bool MIRSymbol::IsReflectionMethodsInfoCompact() const
{
    return StringUtils::StartsWith(GetName(), kMethodsInfoCompactPrefixStr);
}

bool MIRSymbol::IsGctibSym() const
{
    return StringUtils::StartsWith(GetName(), GCTIB_PREFIX_STR);
}

void MIRSymbol::Dump(bool isLocal, int32 indent, bool suppressInit, const MIRSymbolTable *localsymtab) const
{
    if (sKind == kStVar || sKind == kStFunc) {
        srcPosition.DumpLoc(lastPrintedLineNum, lastPrintedColumnNum);
    }
    // exclude unused symbols, formal symbols and extern functions
    if (GetStorageClass() == kScUnused || GetStorageClass() == kScFormal ||
        (GetStorageClass() == kScExtern && sKind == kStFunc)) {
        return;
    }
    if (GetIsImported() && !GetAppearsInCode()) {
        return;
    }
    if (GetTyIdx() >= GlobalTables::GetTypeTable().GetTypeTable().size()) {
        FATAL(kLncFatal, "valid maple_ir with illegal type");
    }
    if (GetStorageClass() == kScText && GetFunction() != nullptr) {
        // without body
        GetFunction()->Dump(true);
        return;
    }
    const char *ids = isLocal ? "%" : "$";
    PrintIndentation(indent);
    if (isTmp) {
        LogInfo::MapleLogger() << "tempvar ";
    } else {
        LogInfo::MapleLogger() << "var ";
    }
    LogInfo::MapleLogger() << ids << GetName() << " ";
    if (GetStorageClass() == kScFstatic) {
        LogInfo::MapleLogger() << "fstatic ";
    } else if (GetStorageClass() == kScPstatic) {
        LogInfo::MapleLogger() << "pstatic ";
    } else if (GetStorageClass() == kScExtern) {
        LogInfo::MapleLogger() << "extern ";
    }
    if (GetTyIdx() != 0u) {
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(GetTyIdx())->Dump(indent + 1);
    }
    if (sectionAttr != UStrIdx(0)) {
        LogInfo::MapleLogger() << " section (";
        PrintString(GlobalTables::GetUStrTable().GetStringFromStrIdx(sectionAttr));
        LogInfo::MapleLogger() << " )";
    }
    if (asmAttr != UStrIdx(0)) {
        LogInfo::MapleLogger() << " asmattr (";
        PrintString(GlobalTables::GetUStrTable().GetStringFromStrIdx(asmAttr));
        LogInfo::MapleLogger() << " )";
    }
    typeAttrs.DumpAttributes();
    if (GetStorageClass() == kScTypeInfoName || GetStorageClass() == kScTypeInfo ||
        GetStorageClass() == kScTypeCxxAbi) {
        LogInfo::MapleLogger() << '\n';
        return;
    }
    if (IsConst() && !suppressInit && !(IsLiteral() && GetStorageClass() == kScExtern)) {
        LogInfo::MapleLogger() << " = ";
        GetKonst()->Dump(localsymtab);
    }
    LogInfo::MapleLogger() << '\n';
}

void MIRSymbol::DumpAsLiteralVar() const
{
    if (IsLiteral()) {
        LogInfo::MapleLogger() << GetName();
    }
}

void MIRSymbolTable::Dump(bool isLocal, int32 indent, bool printDeleted, MIRFlavor flavor) const
{
    size_t size = symbolTable.size();
    for (size_t i = 0; i < size; ++i) {
        MIRSymbol *symbol = symbolTable[i];
        if (symbol == nullptr) {
            continue;
        }
        if (!printDeleted && symbol->IsDeleted()) {
            continue;
        }
        if (flavor == kFlavorLmbc && symbol->LMBCAllocateOffSpecialReg()) {
            continue;
        }
        symbol->Dump(isLocal, indent, false /* suppressinit */, this);
    }
}
#endif

void MIRLabelTable::AddToStringLabelMap(LabelIdx labelIdx)
{
    CHECK_FATAL(labelIdx < labelTable.size(), "index out of range in MIRLabelTable::AddToStringLabelMap");
    if (labelTable[labelIdx] == 0u) {
        // generate a label name based on lab_idx
        std::ostringstream labelNameStream;
        labelNameStream << "@" << labelIdx;
        std::string labelName = labelNameStream.str();
        labelTable[labelIdx] = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(labelName);
    }
    GStrIdx strIdx = labelTable[labelIdx];
    strIdxToLabIdxMap[strIdx] = labelIdx;
}
}  // namespace maple
