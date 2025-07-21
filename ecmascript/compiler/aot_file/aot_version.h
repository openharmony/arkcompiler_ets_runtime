/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_AOT_VERSION_H
#define ECMASCRIPT_AOT_VERSION_H

#include "ecmascript/base/file_header.h"

namespace panda::ecmascript {
class AOTFileVersion {
public:
    AOTFileVersion() = delete;
    // List of historical versions for *.an / *.ai
    // Release Version           Snapshot Version
    //     3.2                      0.0.0.x
    //     4.0                      4.0.0.x
    static constexpr base::FileHeaderBase::VersionType AN_VERSION = {4, 0, 1, 7};
    static constexpr bool AN_STRICT_MATCH = true;
    static constexpr base::FileHeaderBase::VersionType AI_VERSION = {4, 0, 1, 2};
    static constexpr bool AI_STRICT_MATCH = true;
    constexpr static const int VERSION_MULTIPLE_SIZE = 2;
    static PUBLIC_API std::string GetAOTVersion()
    {
        return base::FileHeaderBase::ConvToStr(AN_VERSION);
    }
    static PUBLIC_API bool CheckAOTVersion(const std::string& oldVersion)
    {
        std::optional<base::FileHeaderBase::VersionType> oldVersionFormat =
            base::FileHeaderBase::strToVersion(oldVersion);
        if (oldVersionFormat) {
            return base::FileHeaderBase::VerifyVersionWithoutFile(AN_VERSION, *oldVersionFormat);
        }
        return true;
    }
};

extern "C" int32_t GetAOTVersion(std::string &aotVersion);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_AOT_VERSION_H
