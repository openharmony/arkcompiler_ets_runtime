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

#include "verify_mark.h"
#include "verification.h"
#include "verify_annotation.h"
#include "class_hierarchy.h"
#include "utils.h"

namespace maple {
#ifdef NOT_USED
AnalysisResult *DoVerifyMark::Run(MIRModule *module, ModuleResultMgr *mgr)
{
    LogInfo::MapleLogger() << "Start marking verification result" << '\n';

    CHECK_FATAL(module != nullptr, "Module should not be null");

    auto chaAnalysis = mgr->GetAnalysisResult(MoPhase_CHA, module);
    if (chaAnalysis == nullptr) {
        LogInfo::MapleLogger() << "Can't find class hierarchy result" << '\n';
        return nullptr;
    }
    auto verifyAnalysis = mgr->GetAnalysisResult(MoPhase_VERIFICATION, module);
    if (verifyAnalysis == nullptr) {
        LogInfo::MapleLogger() << "Can't find verification result" << '\n';
        return nullptr;
    }

    auto klassHierarchy = static_cast<KlassHierarchy *>(chaAnalysis);
    const auto *verifyResult = static_cast<VerificationPhaseResult *>(verifyAnalysis);
    const auto &verifyResultMap = verifyResult->GetDeferredClassesPragma();
    for (auto &classResult : verifyResultMap) {
        if (IsSystemPreloadedClass(classResult.first)) {
            continue;
        }
        Klass &klass = utils::ToRef(klassHierarchy->GetKlassFromName(classResult.first));
        klass.SetFlag(kClassRuntimeVerify);
        LogInfo::MapleLogger() << "class " << klass.GetKlassName() << " is Set as NOT VERIFIED\n";
        AddAnnotations(*module, klass, classResult.second);
    }

    LogInfo::MapleLogger() << "Finished marking verification result" << '\n';
    return nullptr;
}

void DoVerifyMark::AddAnnotations(MIRModule &module, const Klass &klass,
                                  const std::vector<const VerifyPragmaInfo *> &pragmaInfoVec)
{
    MIRStructType *mirStructType = klass.GetMIRStructType();
    if (mirStructType == nullptr || pragmaInfoVec.empty()) {
        return;
    }
    const auto &className = klass.GetKlassName();
    std::vector<const AssignableCheckPragma *> assignableCheckPragmaVec;
    for (auto *pragmaItem : pragmaInfoVec) {
        switch (pragmaItem->GetPragmaType()) {
            case kThrowVerifyError: {
                auto *pragma = static_cast<const ThrowVerifyErrorPragma *>(pragmaItem);
                AddVerfAnnoThrowVerifyError(module, *pragma, *mirStructType);
                LogInfo::MapleLogger() << "\tInserted throw verify error annotation for class " << className
                                       << std::endl;
                LogInfo::MapleLogger() << "\tError: " << pragma->GetMessage() << std::endl;
                break;
            }
            case kAssignableCheck: {
                auto *pragma = static_cast<const AssignableCheckPragma *>(pragmaItem);
                assignableCheckPragmaVec.push_back(pragma);
                LogInfo::MapleLogger() << "\tInserted deferred assignable check from " << pragma->GetFromType()
                                       << " to " << pragma->GetToType() << " in class " << className << '\n';
                break;
            }
            case kExtendFinalCheck: {
                AddVerfAnnoExtendFinalCheck(module, *mirStructType);
                break;
            }
            case kOverrideFinalCheck: {
                AddVerfAnnoOverrideFinalCheck(module, *mirStructType);
                break;
            }
            default:
                CHECK_FATAL(false, "\nError: Unknown Verify PragmaInfoType");
        }
    }
    AddVerfAnnoAssignableCheck(module, assignableCheckPragmaVec, *mirStructType);
}
#endif
}  // namespace maple
