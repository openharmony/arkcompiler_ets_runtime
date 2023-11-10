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

#ifndef MAPLEALL_VERIFY_MARK_H
#define MAPLEALL_VERIFY_MARK_H
#include "class_hierarchy.h"
#include "verify_pragma_info.h"

namespace maple {
#ifdef NOT_USED
class DoVerifyMark : public ModulePhase {
public:
    explicit DoVerifyMark(ModulePhaseID id) : ModulePhase(id) {}

    AnalysisResult *Run(MIRModule *module, ModuleResultMgr *mgr) override;

    std::string PhaseName() const override
    {
        return "verifymark";
    }

    ~DoVerifyMark() override = default;

private:
    void AddAnnotations(MIRModule &module, const Klass &klass,
                        const std::vector<const VerifyPragmaInfo *> &pragmaInfoVec);
};
#endif
}  // namespace maple
#endif  // MAPLEALL_VERIFY_MARK_H