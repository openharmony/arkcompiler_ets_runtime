/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DEBUGGER_HOT_RELOAD_MANAGER_H
#define ECMASCRIPT_DEBUGGER_HOT_RELOAD_MANAGER_H

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/debug_info_extractor.h"

namespace panda::ecmascript::tooling {
class HotReloadManager {
public:
    explicit HotReloadManager(const EcmaVM *vm) : vm_(vm) {}
    ~HotReloadManager() = default;

    NO_COPY_SEMANTIC(HotReloadManager);
    NO_MOVE_SEMANTIC(HotReloadManager);

    void NotifyPatchLoaded(const JSPandaFile *baseFile, const JSPandaFile *patchFile);
    void NotifyPatchUnloaded(const JSPandaFile *patchFile);

    std::vector<DebugInfoExtractor *> GetPatchExtractors(const std::string &url) const;
    const JSPandaFile *GetBaseJSPandaFile(const JSPandaFile *jsPandaFile) const;

private:
    void ExtractPatch(const JSPandaFile *jsPandaFile);

    const EcmaVM *vm_;
    CUnorderedMap<const JSPandaFile *, const JSPandaFile *> baseJSPandaFiles_ {};
    CUnorderedMap<std::string, std::vector<DebugInfoExtractor *>> patchExtractors_ {};
};
}  // namespace panda::ecmascript::tooling
#endif  // ECMASCRIPT_DEBUGGER_HOT_RELOAD_MANAGER_H
