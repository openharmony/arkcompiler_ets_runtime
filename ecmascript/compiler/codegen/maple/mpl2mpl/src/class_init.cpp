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
constexpr char kINFOFilename[] = "INFO_filename";
constexpr char kCoreAll[] = "core-all";
constexpr char kMCCPreClinitCheck[] = "MCC_PreClinitCheck";
constexpr char kMCCPostClinitCheck[] = "MCC_PostClinitCheck";
}  // namespace

// This phase does two things.
// 1. Insert clinit(class initialization) check, a intrinsic call INTRN_MPL_CLINIT_CHECK
//   for place where needed.
//   Insert clinit check for static native methods which are not private.
namespace maple {
bool ClassInit::CanRemoveClinitCheck(const std::string &clinitClassname) const
{
    if (!Options::usePreloadedClass) {
        return false;
    }
    if (clinitClassname.empty()) {
        return false;
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
