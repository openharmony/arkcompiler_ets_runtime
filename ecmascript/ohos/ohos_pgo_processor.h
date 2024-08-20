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

#ifndef ECMASCRIPT_OHOS_OHOS_PGO_PROCESSOR_H
#define ECMASCRIPT_OHOS_OHOS_PGO_PROCESSOR_H

#include "ecmascript/compiler/aot_compiler_preprocessor.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/ohos/ohos_pkg_args.h"
#include "ecmascript/platform/os.h"

namespace panda::ecmascript::kungfu {
class OhosPgoProcessor {
public:
    static bool MergeAndRemoveRuntimeAp(CompilationOptions &cOptions, const std::shared_ptr<OhosPkgArgs> &mainPkgArgs)
    {
        if (!cOptions.needMerge_) {
            // use baseline ap, do not need to merge and remove runtime ap(s)
            return true;
        }
        if (cOptions.profilerIn_.empty()) {
            LOG_COMPILER(WARN) << "No valid ap files found in : " << mainPkgArgs->GetPgoDir();
            return true;
        }
        std::string mergedAp(mainPkgArgs->GetMergedApPathWithoutCheck());
        bool isSingleAp = cOptions.profilerIn_.find(GetFileDelimiter()) == std::string::npos;
        if (isSingleAp) {
            if (!RenameSingleAp(cOptions.profilerIn_, mergedAp)) {
                return false;
            }
        } else {
            if (!ExportMergedAp(cOptions.profilerIn_, cOptions.hotnessThreshold_, mergedAp)) {
                return false;
            }
            if (!UnlinkRuntimeAp(mainPkgArgs->GetRuntimeApPath())) {
                return false;
            }
        }
        ASSERT(mainPkgArgs != nullptr);
        // all ohos ap(s) merged into the merged ap file.
        cOptions.profilerIn_ = mainPkgArgs->GetMergedApPath();
        SetSecurityLabel(cOptions.profilerIn_);
        ASSERT(!cOptions.profilerIn_.empty());
        return true;
    }

private:
    NO_MOVE_SEMANTIC(OhosPgoProcessor);
    NO_COPY_SEMANTIC(OhosPgoProcessor);
    static bool RenameSingleAp(const std::string &apPath, const std::string &exportAp)
    {
        ASSERT(!exportAp.empty());
        if (apPath == exportAp) {
            // no need to merge self.
            return true;
        }
        std::string apRealPath;
        if (!RealPath(apPath, apRealPath)) {
            return false;
        }
        if (rename(apRealPath.c_str(), exportAp.c_str())) {
            LOG_ECMA(ERROR) << "RenameSingleAp " << apRealPath << " --> " << exportAp << " failure!, errno: " << errno;
            return false;
        }
        return true;
    }

    static bool UnlinkRuntimeAp(const std::string &runtimeAp)
    {
        ASSERT(!runtimeAp.empty());
        std::string runtimeApRealPath;
        if (!RealPath(runtimeAp, runtimeApRealPath)) {
            return false;
        }
        if (!runtimeApRealPath.empty() && (Unlink(runtimeApRealPath.c_str()) == -1)) {
            LOG_COMPILER(ERROR) << "Remove runtime ap failed. file: " << runtimeApRealPath << ", errno: " << errno;
            return false;
        }
        return true;
    }

    static bool ExportMergedAp(const std::string &profilerIn, uint32_t hotnessThreshold, const std::string &exportAp)
    {
        ASSERT(!exportAp.empty());
        return pgo::PGOProfilerManager::MergeApFiles(profilerIn, exportAp, hotnessThreshold,
                                                     pgo::PGOProfilerEncoder::ApGenMode::MERGE);
    }
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_OHOS_OHOS_PGO_PROCESSOR_H