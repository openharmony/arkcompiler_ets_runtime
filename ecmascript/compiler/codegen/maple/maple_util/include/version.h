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

#ifndef VERSION_H
#define VERSION_H
#include <cstdint>
#include <string>
#include "securec.h"
class Version {
private:
#if MAJOR_VERSION
    static constexpr const uint32_t kMajorVersion = MAJOR_VERSION;
#else   // MAJOR_VERSION
    static constexpr const uint32_t kMajorVersion = 1;
#endif  // MAJOR_VERSION

#ifdef MINOR_VERSION
    static constexpr const uint32_t kMinorVersion = MINOR_VERSION;
#else
    static constexpr const uint32_t kMinorVersion = 0;
#endif  // MINOR_VERSION

// [a-zA-Z0-9_\-]+
#ifdef RELEASE_VERSION
    static constexpr const char *kReleaseVersion = RELEASE_VERSION;
#else
    static constexpr const char *kReleaseVersion = "0";
#endif  // RELEASE_VERSION

// B[0-9]{3}
#ifdef BUILD_VERSION
    static constexpr const uint32_t kBuildVersion = BUILD_VERSION;
#endif  // BUILD_VERSION

#ifdef GIT_REVISION
    static constexpr const char *kGitRevision = GIT_REVISION;
#endif  // GIT_REVISION

#ifdef MINOR_RUNTIME_VERSION
    static constexpr const uint32_t kMinorRuntimeVersion = MINOR_RUNTIME_VERSION;
#else
    static constexpr const uint32_t kMinorRuntimeVersion = 0;
#endif  // kMinorRuntimeVersion

public:
    static std::string GetVersionStr()
    {
        std::ostringstream oss;
        oss << kMajorVersion << "." << kMinorVersion << "." << kReleaseVersion;

#ifdef BUILD_VERSION
        constexpr int BUILD_VERSION_LEN = 5;
        char buffer[BUILD_VERSION_LEN] = {0};
        int ret = sprintf_s(buffer, BUILD_VERSION_LEN, "B%03d", kBuildVersion);
        if (ret >= 0) {
            oss << "." << buffer;
        }
#endif  // BUILD_VERSION

#ifdef GIT_REVISION
        oss << " " << kGitRevision;
#endif  // GIT_REVISION
        return oss.str();
    }

    static inline uint32_t GetMajorVersion()
    {
        return kMajorVersion;
    }

    static inline uint32_t GetMinorVersion()
    {
        return kMinorVersion;
    }

    static inline const char *GetReleaseVersion()
    {
        return kReleaseVersion;
    }

    static inline uint32_t GetRuntimeVersion()
    {
        return kMinorRuntimeVersion;
    }
};
#endif  // VERSION_H
