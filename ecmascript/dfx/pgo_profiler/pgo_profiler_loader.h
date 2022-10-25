/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_PGO_PROFILE_LOADER_H
#define ECMASCRIPT_DFX_PGO_PROFILE_LOADER_H

#include <unordered_map>
#include <unordered_set>

#include "ecmascript/ecma_macros.h"
#include "ecmascript/jspandafile/method_literal.h"

namespace panda::ecmascript {
class PGOProfilerLoader {
public:
    PGOProfilerLoader() = default;
    virtual ~PGOProfilerLoader() = default;

    NO_COPY_SEMANTIC(PGOProfilerLoader);
    NO_MOVE_SEMANTIC(PGOProfilerLoader);

    bool PUBLIC_API Match(const CString &recordName, EntityId methodId);
    void PUBLIC_API LoadProfiler(const std::string &inPath, uint32_t hotnessThreshold);
private:
    static constexpr int METHOD_INFO_COUNT = 2;

    void ParseProfiler(const std::string &profilerString);
    void ParseHotMethodInfo(const std::string &methodInfo, std::unordered_set<EntityId> &methodIds);

    bool isLoaded_ {false};
    uint32_t hotnessThreshold_ = {0};
    std::unordered_map<CString, std::unordered_set<EntityId>> hotnessMethods_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_PGO_PROFILE_LOADER_H
