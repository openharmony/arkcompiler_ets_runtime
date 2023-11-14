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

#include "class_init.h"
#include <iostream>
#include <fstream>

namespace {
constexpr char kCinfString[] = "__cinf_Ljava_2Flang_2FString_3B";
constexpr char kINFOFilename[] = "INFO_filename";
constexpr char kCoreAll[] = "core-all";
constexpr char kMCCPreClinitCheck[] = "MCC_PreClinitCheck";
constexpr char kMCCPostClinitCheck[] = "MCC_PostClinitCheck";
}  // namespace

// This phase does two things.
// 1. Insert clinit(class initialization) check, a intrinsic call INTRN_MPL_CLINIT_CHECK
//   for place where needed.
//   Insert clinit check for static native methods which are not private.
// 2. Lower JAVA_CLINIT_CHECK to MPL_CLINIT_CHECK.
//   Before insert or tranform the clinit check, we used a optimise based on
//   white list. When the dexname is core-all and the class is in the white list
//   we dont't insert clinit check.Because the class in the white list is intialized
//   in the system bootup.
namespace maple {
bool ClassInit::CanRemoveClinitCheck(const std::string &clinitClassname) const
{
    if (!Options::usePreloadedClass) {
        return false;
    }
    if (clinitClassname.empty()) {
        return false;
    }
    if (clinitClassname == kCinfString) {
        return true;
    }
    uint32 dexNameIdx =
        GetMIRModule().GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kINFOFilename));
    const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
    if (dexName.find(kCoreAll) != std::string::npos) {
        return false;
    }
    return IsSystemPreloadedClass(clinitClassname);
}

void ClassInit::GenClassInitCheckProfile(MIRFunction &func, const MIRSymbol &classInfo, StmtNode *clinit) const
{
    GenPreClassInitCheck(func, classInfo, clinit);
    GenPostClassInitCheck(func, classInfo, clinit);
}

void ClassInit::GenPreClassInitCheck(MIRFunction &func, const MIRSymbol &classInfo, const StmtNode *clinit) const
{
    MIRFunction *preClinit = builder->GetOrCreateFunction(kMCCPreClinitCheck, (TyIdx)(PTY_void));
    BaseNode *classInfoNode = builder->CreateExprAddrof(0, classInfo);
    MapleVector<BaseNode *> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.push_back(classInfoNode);
    CallNode *callPreclinit = builder->CreateStmtCall(preClinit->GetPuidx(), args);
    func.GetBody()->InsertBefore(clinit, callPreclinit);
}

void ClassInit::GenPostClassInitCheck(MIRFunction &func, const MIRSymbol &classInfo, const StmtNode *clinit) const
{
    MIRFunction *postClinit = builder->GetOrCreateFunction(kMCCPostClinitCheck, (TyIdx)(PTY_void));
    BaseNode *classInfoNode = builder->CreateExprAddrof(0, classInfo);
    MapleVector<BaseNode *> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.push_back(classInfoNode);
    CallNode *callPostClinit = builder->CreateStmtCall(postClinit->GetPuidx(), args);
    func.GetBody()->InsertAfter(clinit, callPostClinit);
}

void ClassInit::ProcessFunc(MIRFunction *func)
{
    // No field will be involved in critical native funcs.
    DEBUG_ASSERT(func != nullptr, "null ptr check!");
    if (func->IsEmpty() || func->GetAttr(FUNCATTR_critical_native)) {
        return;
    }
    currFunc = func;
    builder->SetCurrentFunction(*func);
    // Insert clinit check for static methods.
    MIRType *selfClassType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(func->GetClassTyIdx());
    std::string selfClassName;
    if (selfClassType != nullptr) {
        selfClassName = GlobalTables::GetStrTable().GetStringFromStrIdx(selfClassType->GetNameStrIdx());
    } else {
        const std::string &funcName = func->GetName();
        size_t pos = funcName.find(namemangler::kNameSplitterStr);
        constexpr size_t prePos = 2;
        constexpr size_t ligalPos = 2;
        while (pos != std::string::npos &&
               (pos >= ligalPos && funcName[pos - 1] == '_' && funcName[pos - prePos] != '_')) {
            constexpr size_t nextPos = 3;
            pos = funcName.find(namemangler::kNameSplitterStr, pos + nextPos);
        }
        selfClassName = funcName.substr(0, pos);
    }
    // Insert clinit check for static native methods which are not private.
    // We have to do this here because native methods are generated as empty by maplefe,
    // If we simply insert clinit-check (which does not have return value), there will
    // be no return statement for native methods which do hava a return value.
    // clinit check for static java (non-native) methods which are not private is
    // already inserted by maplefe.
    if (func->IsStatic() && !func->IsPrivate() && !func->IsClinit() && func->IsNative()) {
        MIRType *classType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(func->GetClassTyIdx());
        CHECK_FATAL(classType != nullptr, "class type is nullptr");
        const std::string &className = GlobalTables::GetStrTable().GetStringFromStrIdx(classType->GetNameStrIdx());
        if (!CanRemoveClinitCheck(className)) {
            Klass *klass = klassHierarchy->GetKlassFromName(className);
            CHECK_FATAL(klass != nullptr, "klass is nullptr in ClassInit::ProcessFunc");
            if (klass->GetClinit() && func != klass->GetClinit()) {
                MIRSymbol *classInfo = GetClassInfo(className);
                BaseNode *classInfoNode = builder->CreateExprAddrof(0, *classInfo);
                if (trace) {
                    LogInfo::MapleLogger() << "\t- low-cost clinit - insert check in static method " << func->GetName()
                                           << "clasname " << className << "\n";
                }
                MapleVector<BaseNode *> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
                args.push_back(classInfoNode);
                StmtNode *intrinsicCall = builder->CreateStmtIntrinsicCall(INTRN_MPL_CLINIT_CHECK, args);
                func->GetBody()->InsertFirst(intrinsicCall);
                DEBUG_ASSERT(classInfo != nullptr, "null ptr check!");
#ifdef CLINIT_CHECK
                GenClassInitCheckProfile(*func, *classInfo, intrinsicCall);
#endif  // CLINIT_CHECK
            }
        }
    }
    // Lower JAVA_CLINIT_CHECK to MPL_CLINIT_CHECK.
    StmtNode *stmt = func->GetBody()->GetFirst();
    while (stmt != nullptr) {
        if (stmt->GetOpCode() == OP_intrinsiccallwithtype) {
            auto *intrinsicCall = static_cast<IntrinsiccallNode *>(stmt);
            if (intrinsicCall->GetIntrinsic() == INTRN_JAVA_CLINIT_CHECK) {
                // intrinsiccallwithtype <$LTest_3B> JAVA_CLINIT_CHECK ()        -->
                // intrinsiccall MPL_CLINIT_CHECK (addrof ptr $__cinf_LTest_3B)
                CHECK_FATAL(intrinsicCall->GetNopndSize() == 0, "wrong arg vectors");
                CHECK_FATAL(intrinsicCall->GetTyIdx() < GlobalTables::GetTypeTable().GetTypeTable().size(),
                            "index out of range");
                MIRType *classType = GlobalTables::GetTypeTable().GetTypeTable()[intrinsicCall->GetTyIdx()];
                DEBUG_ASSERT(classType != nullptr, "null ptr check!");
                CHECK_FATAL(classType->GetNameStrIdx() != 0u, "symbol name is null for type index %d",
                            static_cast<uint32>(intrinsicCall->GetTyIdx()));
                const std::string &className =
                    GlobalTables::GetStrTable().GetStringFromStrIdx(classType->GetNameStrIdx());
                Klass *klass = klassHierarchy->GetKlassFromName(className);
                bool doClinitCheck = false;
                if (klass == nullptr) {
                    WARN(kLncWarn, "ClassInit::ProcessFunc: Skip INCOMPLETE type %s", className.c_str());
                    doClinitCheck = true;
                } else {
                    doClinitCheck =
                        !CanRemoveClinitCheck(className) && klassHierarchy->NeedClinitCheckRecursively(*klass);
                }
                if (Options::buildApp != 0) {
                    doClinitCheck = true;
                }
                if (doClinitCheck) {
                    MIRSymbol *classInfo = GetClassInfo(className);
                    AddrofNode *classInfoNode = builder->CreateExprAddrof(0, *classInfo);
                    MapleVector<BaseNode *> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
                    args.push_back(classInfoNode);
                    StmtNode *mplIntrinsicCall = builder->CreateStmtIntrinsicCall(INTRN_MPL_CLINIT_CHECK, args);
                    func->GetBody()->ReplaceStmt1WithStmt2(stmt, mplIntrinsicCall);
                    if (trace) {
                        LogInfo::MapleLogger() << "\t- low-cost clinit - lower JAVA_CLINIT_CHECK " << className
                                               << " in " << func->GetName() << "()\n";
                    }
                    DEBUG_ASSERT(classInfo != nullptr, "null ptr check!");
#ifdef CLINIT_CHECK
                    GenClassInitCheckProfile(*func, *classInfo, mplIntrinsicCall);
#endif  // CLINIT_CHECK
                } else {
                    func->GetBody()->RemoveStmt(stmt);
                }
            }
        }
        stmt = stmt->GetNext();
    }
}

MIRSymbol *ClassInit::GetClassInfo(const std::string &classname)
{
    const std::string &classInfoName = CLASSINFO_PREFIX_STR + classname;
    MIRType *classInfoType =
        GlobalTables::GetTypeTable().GetOrCreateClassType(namemangler::kClassMetadataTypeName, GetMIRModule());
    MIRSymbol *classInfo = builder->GetOrCreateGlobalDecl(classInfoName, *classInfoType);
    Klass *klass = klassHierarchy->GetKlassFromName(classname);
    if (klass == nullptr || !klass->GetMIRStructType()->IsLocal()) {
        classInfo->SetStorageClass(kScExtern);
    }
    return classInfo;
}

}  // namespace maple
