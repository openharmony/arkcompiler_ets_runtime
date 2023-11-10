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

#ifndef MAPLEIR_VERIFICATION_PHASE_H
#define MAPLEIR_VERIFICATION_PHASE_H
#include "class_hierarchy.h"
#include "verify_pragma_info.h"

namespace maple {
using ClassVerifyPragmas = MapleUnorderedMap<std::string, std::vector<const VerifyPragmaInfo *>>;

class VerifyResult {
public:
    VerifyResult(const MIRModule &module, const KlassHierarchy &klassHierarchy, MemPool &memPool)
        : module(module),
          klassHierarchy(klassHierarchy),
          allocator(&memPool),
          classesCorrectness(allocator.Adapter()),
          classesPragma(allocator.Adapter())
    {
    }

    ~VerifyResult() = default;

    const KlassHierarchy &GetKlassHierarchy() const
    {
        return klassHierarchy;
    }

    const MIRModule &GetMIRModule() const
    {
        return module;
    }

    const MIRFunction *GetCurrentFunction() const
    {
        return module.GetFunctionList().front();
    }

    const std::string &GetCurrentClassName() const
    {
        return GetCurrentFunction()->GetClassType()->GetName();
    }

    const ClassVerifyPragmas &GetDeferredClassesPragma() const
    {
        return classesPragma;
    }

    void AddPragmaVerifyError(const std::string &className, std::string errMsg);
    void AddPragmaAssignableCheck(const std::string &className, std::string fromType, std::string toType);
    void AddPragmaExtendFinalCheck(const std::string &className);
    void AddPragmaOverrideFinalCheck(const std::string &className);

    const MapleUnorderedMap<std::string, bool> &GetResultMap() const
    {
        return classesCorrectness;
    }
    void SetClassCorrectness(const std::string &className, bool result)
    {
        classesCorrectness[className] = result;
    }

    bool HasErrorNotDeferred() const
    {
        for (auto &classResult : classesCorrectness) {
            if (!classResult.second) {
                if (classesPragma.find(classResult.first) == classesPragma.end()) {
                    // Verify result is not OK, but has no deferred check or verify error in runtime
                    return true;
                }
            }
        }
        return false;
    }

private:
    bool HasVerifyError(const std::vector<const VerifyPragmaInfo *> &pragmaInfoPtrVec) const;
    bool HasSamePragmaInfo(const std::vector<const VerifyPragmaInfo *> &pragmaInfoPtrVec,
                           const VerifyPragmaInfo &verifyPragmaInfo) const;

    const MIRModule &module;
    const KlassHierarchy &klassHierarchy;
    MapleAllocator allocator;
    // classesCorrectness<key: className, value: correctness>, correctness is true only if the class is verified OK
    MapleUnorderedMap<std::string, bool> classesCorrectness;
    // classesPragma<key: className, value: PragmaInfo for deferred check>
    ClassVerifyPragmas classesPragma;
};

class VerificationPhaseResult : public AnalysisResult {
public:
    VerificationPhaseResult(MemPool &mp, const VerifyResult &verifyResult)
        : AnalysisResult(&mp), verifyResult(verifyResult)
    {
    }
    ~VerificationPhaseResult() = default;

    const ClassVerifyPragmas &GetDeferredClassesPragma() const
    {
        return verifyResult.GetDeferredClassesPragma();
    }

private:
    const VerifyResult &verifyResult;
};

#ifdef NOT_USED
class DoVerification : public ModulePhase {
public:
    explicit DoVerification(ModulePhaseID id) : ModulePhase(id) {}

    AnalysisResult *Run(MIRModule *module, ModuleResultMgr *mgr) override;
    std::string PhaseName() const override
    {
        return "verification";
    }

    ~DoVerification() = default;

private:
    void VerifyModule(MIRModule &module, VerifyResult &result) const;
    void DeferredCheckFinalClassAndMethod(VerifyResult &result) const;
    bool IsLazyBindingOrDecouple(const KlassHierarchy &klassHierarchy) const;
    bool NeedRuntimeFinalCheck(const KlassHierarchy &klassHierarchy, const std::string &className) const;
    void CheckExtendFinalClass(VerifyResult &result) const;
};
#endif
}  // namespace maple
#endif  // MAPLEIR_VERIFICATION_PHASE_H
