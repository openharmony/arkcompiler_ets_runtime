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

#include "verification.h"
#include "mir_nodes.h"
#include "class_hierarchy.h"
#include "utils.h"

namespace maple {
#ifdef NOT_USED
AnalysisResult *DoVerification::Run(MIRModule *module, ModuleResultMgr *mgr)
{
    LogInfo::MapleLogger() << "========== Starting Verify Module =====================" << '\n';
    CHECK_FATAL(module != nullptr, "Module should be not null");
    LogInfo::MapleLogger() << "Current module = " << module->GetFileName() << std::endl;

    auto chaAnalysis = mgr->GetAnalysisResult(MoPhase_CHA, module);
    if (chaAnalysis == nullptr) {
        LogInfo::MapleLogger() << "Can't find class hierarchy result" << '\n';
        return nullptr;
    }
    KlassHierarchy *klassHierarchy = static_cast<KlassHierarchy *>(chaAnalysis);

    MemPool *memPool = NewMemPool();
    auto *verifyResult = memPool->New<VerifyResult>(*module, *klassHierarchy, *memPool);
    VerifyModule(*module, *verifyResult);
    if (verifyResult->HasErrorNotDeferred()) {
        LogInfo::MapleLogger() << "Warning: Verify MIR failed!  " << '\n';
    }

    CheckExtendFinalClass(*verifyResult);
    if (IsLazyBindingOrDecouple(*klassHierarchy)) {
        DeferredCheckFinalClassAndMethod(*verifyResult);
    }

    auto *result = memPool->New<VerificationPhaseResult>(*memPool, *verifyResult);
    mgr->AddResult(GetPhaseID(), *module, *result);
    LogInfo::MapleLogger() << "========== Finished Verify Module =====================" << '\n';
    return result;
}

void DoVerification::CheckExtendFinalClass(VerifyResult &result) const
{
    const KlassHierarchy &klassHierarchy = result.GetKlassHierarchy();
    std::stack<const Klass *> classesToMark;
    for (auto klass : klassHierarchy.GetTopoSortedKlasses()) {
        if (klass == nullptr || !klass->HasSubKlass()) {
            continue;
        }
        const MIRStructType *mirStructType = klass->GetMIRStructType();
        if ((mirStructType == nullptr) || (!mirStructType->IsMIRClassType()) ||
            (!static_cast<const MIRClassType *>(mirStructType)->IsFinal())) {
            continue;
        }
        classesToMark.push(klass);
        while (!classesToMark.empty()) {
            const auto &subKlasses = classesToMark.top()->GetSubKlasses();
            classesToMark.pop();
            for (auto subKlass : subKlasses) {
                if (subKlass == nullptr) {
                    continue;
                }
                std::string errMsg =
                    "Class " + subKlass->GetKlassName() + " cannot inherit from final class " + klass->GetKlassName();
                result.AddPragmaVerifyError(subKlass->GetKlassName(), std::move(errMsg));
                if (subKlass->HasSubKlass()) {
                    classesToMark.push(subKlass);
                }
            }
        }
    }
}

bool DoVerification::IsLazyBindingOrDecouple(const KlassHierarchy &klassHierarchy) const
{
    if (Options::buildApp != 0) {
        return true;
    }
    const Klass *objectKlass = klassHierarchy.GetKlassFromLiteral(namemangler::kJavaLangObjectStr);
    bool isLibCore = ((objectKlass != nullptr) && (objectKlass->GetMIRStructType()->IsLocal()));
    return !isLibCore;
}

bool DoVerification::NeedRuntimeFinalCheck(const KlassHierarchy &klassHierarchy, const std::string &className) const
{
    const Klass *klass = klassHierarchy.GetKlassFromName(className);
    if (klass == nullptr || klass->GetMIRStructType() == nullptr || !klass->HasSuperKlass()) {
        return false;
    }
    return klass->GetMIRStructType()->IsLocal();
}

void DoVerification::DeferredCheckFinalClassAndMethod(VerifyResult &result) const
{
    const auto &functionList = result.GetMIRModule().GetFunctionList();
    std::set<std::string> classesAdded;
    for (size_t index = 0; index < functionList.size(); ++index) {
        result.SetCurrentFunction(index);
        const auto &className = result.GetCurrentClassName();
        if (classesAdded.find(className) != classesAdded.end()) {
            continue;
        }

        if (!NeedRuntimeFinalCheck(result.GetKlassHierarchy(), className)) {
            continue;
        }
        // do not extend final parent class
        result.AddPragmaExtendFinalCheck(className);
        // do not override final method
        result.AddPragmaOverrideFinalCheck(className);
        LogInfo::MapleLogger() << "Check Final class and method for class " << className << '\n';
        (void)classesAdded.insert(className);
    }
}

void DoVerification::VerifyModule(MIRModule &module, VerifyResult &result) const
{
    const auto &resultMap = result.GetResultMap();

    // theModule is needed by verify() functions in mir_nodes.cpp
    theMIRModule = &module;
    const auto &functionList = result.GetMIRModule().GetFunctionList();
    for (size_t index = 0; index < functionList.size(); ++index) {
        result.SetCurrentFunction(index);
        MIRFunction *currentFunction = functionList[index];
        const BlockNode *block = currentFunction->GetBody();

        // For verify() functions compatibility
        theMIRModule->SetCurFunction(currentFunction);

        if (block == nullptr) {
            continue;
        }
        bool blockResult = block->Verify(result);

        const auto &className = result.GetCurrentClassName();
        auto iter = resultMap.find(className);
        if (iter == resultMap.end()) {
            result.SetClassCorrectness(className, blockResult);
        } else {
            result.SetClassCorrectness(className, iter->second && blockResult);
        }
    }
}
#endif

bool VerifyResult::HasVerifyError(const std::vector<const VerifyPragmaInfo *> &pragmaInfoPtrVec) const
{
    if (pragmaInfoPtrVec.empty()) {
        return false;
    }
    return (utils::ToRef(pragmaInfoPtrVec.front()).IsVerifyError());
}

bool VerifyResult::HasSamePragmaInfo(const std::vector<const VerifyPragmaInfo *> &pragmaInfoPtrVec,
                                     const VerifyPragmaInfo &verifyPragmaInfo) const
{
    for (auto &pragmaInfoPtr : pragmaInfoPtrVec) {
        const VerifyPragmaInfo &pragmaInfoRef = utils::ToRef(pragmaInfoPtr);
        if (!verifyPragmaInfo.IsEqualTo(pragmaInfoRef)) {
            continue;
        }
        if (!verifyPragmaInfo.IsAssignableCheck()) {
            return true;
        }
        if (static_cast<const AssignableCheckPragma &>(verifyPragmaInfo)
                .IsEqualTo(static_cast<const AssignableCheckPragma &>(pragmaInfoRef))) {
            return true;
        }
    }
    return false;
}

void VerifyResult::AddPragmaVerifyError(const std::string &className, std::string errMsg)
{
    classesCorrectness[className] = false;
    auto classIter = classesPragma.find(className);
    if (classIter != classesPragma.end() && HasVerifyError(classIter->second)) {
        return;
    }
    const VerifyPragmaInfo *verifyError = allocator.GetMemPool()->New<ThrowVerifyErrorPragma>(std::move(errMsg));
    if (classIter == classesPragma.end()) {
        classesPragma[className].push_back(verifyError);
        return;
    }
    std::vector<const VerifyPragmaInfo *> &pragmaInfoPtrVec = classIter->second;
    pragmaInfoPtrVec.clear();
    pragmaInfoPtrVec.push_back(verifyError);
}

void VerifyResult::AddPragmaAssignableCheck(const std::string &className, std::string fromType, std::string toType)
{
    classesCorrectness[className] = false;
    auto classIter = classesPragma.find(className);
    if (classIter != classesPragma.end() && HasVerifyError(classIter->second)) {
        return;
    }
    const VerifyPragmaInfo *assignableCheck =
        allocator.GetMemPool()->New<AssignableCheckPragma>(std::move(fromType), std::move(toType));
    if (classIter == classesPragma.end()) {
        classesPragma[className].push_back(assignableCheck);
        return;
    }
    std::vector<const VerifyPragmaInfo *> &pragmaInfoPtrVec = classIter->second;
    if (HasSamePragmaInfo(pragmaInfoPtrVec, *assignableCheck)) {
        return;
    }
    pragmaInfoPtrVec.push_back(assignableCheck);
}

void VerifyResult::AddPragmaExtendFinalCheck(const std::string &className)
{
    classesCorrectness[className] = false;
    auto classIter = classesPragma.find(className);
    if (classIter != classesPragma.end() && HasVerifyError(classIter->second)) {
        return;
    }

    const VerifyPragmaInfo *extendFinalCheck = allocator.GetMemPool()->New<ExtendFinalCheckPragma>();
    if (classIter == classesPragma.end()) {
        classesPragma[className].push_back(extendFinalCheck);
        return;
    }
    std::vector<const VerifyPragmaInfo *> &pragmaInfoPtrVec = classIter->second;
    if (HasSamePragmaInfo(pragmaInfoPtrVec, *extendFinalCheck)) {
        return;
    }
    pragmaInfoPtrVec.push_back(extendFinalCheck);
}

void VerifyResult::AddPragmaOverrideFinalCheck(const std::string &className)
{
    classesCorrectness[className] = false;
    auto classIter = classesPragma.find(className);
    if (classIter != classesPragma.end() && HasVerifyError(classIter->second)) {
        return;
    }

    const VerifyPragmaInfo *overrideFinalCheck = allocator.GetMemPool()->New<OverrideFinalCheckPragma>();
    if (classIter == classesPragma.end()) {
        classesPragma[className].push_back(overrideFinalCheck);
        return;
    }
    std::vector<const VerifyPragmaInfo *> &pragmaInfoPtrVec = classIter->second;
    if (HasSamePragmaInfo(pragmaInfoPtrVec, *overrideFinalCheck)) {
        return;
    }
    pragmaInfoPtrVec.push_back(overrideFinalCheck);
}
}  // namespace maple
