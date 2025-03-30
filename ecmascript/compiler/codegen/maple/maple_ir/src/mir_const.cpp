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

#include "mir_const.h"
#include "mir_function.h"
#include "printing.h"
#include <iomanip>

#if MIR_FEATURE_FULL

namespace maple {
#ifdef ARK_LITECG_DEBUG
void MIRIntConst::Dump(const MIRSymbolTable *) const
{
    LogInfo::MapleLogger() << value;
}
#endif

bool MIRIntConst::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &intConst = static_cast<const MIRIntConst &>(rhs);
    return ((&intConst.GetType() == &GetType()) && (intConst.value == value));
}

#ifdef ARK_LITECG_DEBUG
uint8 MIRIntConst::GetActualBitWidth() const
{
    if (value == 0) {
        return 1;
    }

    int64 val = GetExtValue();
    uint64 tmp = val < 0 ? -(val + 1) : val;

    uint8 width = 0;
    while (tmp != 0) {
        ++width;
        tmp = tmp >> 1u;
    }

    return width;
}

void MIRAddrofConst::Dump(const MIRSymbolTable *localSymTab) const
{
    LogInfo::MapleLogger() << "addrof " << GetPrimTypeName(PTY_ptr);
    const MIRSymbol *sym = stIdx.IsGlobal() ? GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx())
                                            : localSymTab->GetSymbolFromStIdx(stIdx.Idx());
    DEBUG_ASSERT(stIdx.IsGlobal() || sym->GetStorageClass() == kScPstatic || sym->GetStorageClass() == kScFstatic,
                 "MIRAddrofConst can only point to a global symbol");
    CHECK_FATAL(sym != nullptr, "null ptr");
    LogInfo::MapleLogger() << (stIdx.IsGlobal() ? " $" : " %") << sym->GetName();
    if (fldID > 0) {
        LogInfo::MapleLogger() << " " << fldID;
    }
    if (offset != 0) {
        LogInfo::MapleLogger() << " (" << offset << ")";
    }
}
#endif

bool MIRAddrofConst::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &rhsA = static_cast<const MIRAddrofConst &>(rhs);
    if (&GetType() != &rhs.GetType()) {
        return false;
    }
    return (stIdx == rhsA.stIdx) && (fldID == rhsA.fldID);
}

#ifdef ARK_LITECG_DEBUG
void MIRAddroffuncConst::Dump(const MIRSymbolTable *) const
{
    LogInfo::MapleLogger() << "addroffunc " << GetPrimTypeName(PTY_ptr);
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    CHECK_FATAL(func != nullptr, "null ptr");
    CHECK_NULL_FATAL(GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx()));
    LogInfo::MapleLogger() << " &"
                           << GlobalTables::GetGsymTable().GetSymbolFromStidx(func->GetStIdx().Idx())->GetName();
}
#endif

bool MIRAddroffuncConst::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &rhsAf = static_cast<const MIRAddroffuncConst &>(rhs);
    return (&GetType() == &rhs.GetType()) && (puIdx == rhsAf.puIdx);
}

#ifdef ARK_LITECG_DEBUG
void MIRLblConst::Dump(const MIRSymbolTable *) const
{
    LogInfo::MapleLogger() << "addroflabel " << GetPrimTypeName(PTY_ptr);
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
    LogInfo::MapleLogger() << " @" << func->GetLabelName(value);
}
#endif

bool MIRLblConst::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &lblConst = static_cast<const MIRLblConst &>(rhs);
    return (lblConst.value == value);
}

bool MIRFloatConst::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &floatConst = static_cast<const MIRFloatConst &>(rhs);
    if (std::isnan(floatConst.value.floatValue)) {
        return std::isnan(value.floatValue);
    }
    if (std::isnan(value.floatValue)) {
        return std::isnan(floatConst.value.floatValue);
    }
    if (floatConst.value.floatValue == 0.0 && value.floatValue == 0.0) {
        return floatConst.IsNeg() == IsNeg();
    }
    // Use bitwise comparison instead of approximate comparison for FP to avoid treating 0.0 and FLT_MIN as equal
    return (floatConst.value.intValue == value.intValue);
}

bool MIRDoubleConst::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &floatConst = static_cast<const MIRDoubleConst &>(rhs);
    if (std::isnan(floatConst.value.dValue)) {
        return std::isnan(value.dValue);
    }
    if (std::isnan(value.dValue)) {
        return std::isnan(floatConst.value.dValue);
    }
    if (floatConst.value.dValue == 0.0 && value.dValue == 0.0) {
        return floatConst.IsNeg() == IsNeg();
    }
    // Use bitwise comparison instead of approximate comparison for FP to avoid treating 0.0 and DBL_MIN as equal
    return (floatConst.value.intValue == value.intValue);
}

bool MIRAggConst::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &aggregateConst = static_cast<const MIRAggConst &>(rhs);
    if (aggregateConst.constVec.size() != constVec.size()) {
        return false;
    }
    for (size_t i = 0; i < constVec.size(); ++i) {
        if (!(*aggregateConst.constVec[i] == *constVec[i])) {
            return false;
        }
    }
    return true;
}

#ifdef ARK_LITECG_DEBUG
void MIRFloatConst::Dump(const MIRSymbolTable *) const
{
    LogInfo::MapleLogger() << std::setprecision(std::numeric_limits<float>::max_digits10) << value.floatValue << "f";
}

void MIRDoubleConst::Dump(const MIRSymbolTable *) const
{
    LogInfo::MapleLogger() << std::setprecision(std::numeric_limits<double>::max_digits10) << value.dValue;
}

void MIRAggConst::Dump(const MIRSymbolTable *localSymTab) const
{
    LogInfo::MapleLogger() << "[";
    size_t size = constVec.size();
    for (size_t i = 0; i < size; ++i) {
        if (fieldIdVec[i] != 0) {
            LogInfo::MapleLogger() << fieldIdVec[i] << "= ";
        }
        constVec[i]->Dump(localSymTab);
        if (i != size - 1) {
            LogInfo::MapleLogger() << ", ";
        }
    }
    LogInfo::MapleLogger() << "]";
}
#endif

MIRStrConst::MIRStrConst(const std::string &str, MIRType &type)
    : MIRConst(type, kConstStrConst), value(GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str))
{
}

#ifdef ARK_LITECG_DEBUG
void MIRStrConst::Dump(const MIRSymbolTable *) const
{
    LogInfo::MapleLogger() << "conststr " << GetPrimTypeName(GetType().GetPrimType());
    const std::string &dumpStr = GlobalTables::GetUStrTable().GetStringFromStrIdx(value);
    PrintString(dumpStr);
}
#endif

bool MIRStrConst::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &rhsCs = static_cast<const MIRStrConst &>(rhs);
    return (&rhs.GetType() == &GetType()) && (value == rhsCs.value);
}

MIRStr16Const::MIRStr16Const(const std::u16string &str, MIRType &type)
    : MIRConst(type, kConstStr16Const), value(GlobalTables::GetU16StrTable().GetOrCreateStrIdxFromName(str))
{
}

#ifdef ARK_LITECG_DEBUG
void MIRStr16Const::Dump(const MIRSymbolTable *) const
{
    LogInfo::MapleLogger() << "conststr16 " << GetPrimTypeName(GetType().GetPrimType());
    std::u16string str16 = GlobalTables::GetU16StrTable().GetStringFromStrIdx(value);
    // UTF-16 string are dumped as UTF-8 string in mpl to keep the printable chars in ascii form
    std::string str;
    (void)namemangler::UTF16ToUTF8(str, str16);
    PrintString(str);
}
#endif

bool MIRStr16Const::operator==(const MIRConst &rhs) const
{
    if (&rhs == this) {
        return true;
    }
    if (GetKind() != rhs.GetKind()) {
        return false;
    }
    const auto &rhsCs = static_cast<const MIRStr16Const &>(rhs);
    return (&GetType() == &rhs.GetType()) && (value == rhsCs.value);
}

bool IsDivSafe(const MIRIntConst &dividend, const MIRIntConst &divisor, PrimType pType)
{
    if (IsUnsignedInteger(pType)) {
        return divisor.GetValue() != 0;
    }

    return divisor.GetValue() != 0 && (!dividend.GetValue().IsMinValue() || !divisor.GetValue().AreAllBitsOne());
}

}  // namespace maple
#endif  // MIR_FEATURE_FULL
