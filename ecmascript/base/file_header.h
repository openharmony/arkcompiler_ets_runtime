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

#ifndef ECMASCRIPT_BASE_FILE_HEADER_H
#define ECMASCRIPT_BASE_FILE_HEADER_H

#include "ecmascript/base/string_helper.h"
#include "ecmascript/log_wrapper.h"
#include <array>
#include <stddef.h>
#include <stdint.h>

namespace panda::ecmascript::base {
class FileHeader {
public:
    static constexpr size_t MAGIC_SIZE = 8;
    static constexpr size_t VERSION_SIZE = 4;
    static constexpr std::array<uint8_t, MAGIC_SIZE> MAGIC = {'P', 'A', 'N', 'D', 'A', '\0', '\0', '\0'};
    using VersionType = std::array<uint8_t, VERSION_SIZE>;
    
    static const VersionType ToVersion(uint32_t versionNumber)
    {
        VersionUnion helper = {.versionNumber = ReverseBytes(versionNumber)};
        return helper.version;
    }

    static uint32_t ToVersionNumber(const VersionType &version)
    {
        VersionUnion helper = {.version = version};
        return ReverseBytes(helper.versionNumber);
    }

    static bool VerifyVersion(const char *fileDesc, uint32_t currVersion, uint32_t lastVersion, bool silent)
    {
        return VerifyVersion(fileDesc, ToVersion(currVersion), ToVersion(lastVersion), silent);
    }

protected:

    FileHeader(const VersionType &lastVersion) : magic_(MAGIC), version_(lastVersion) {}

    static bool VerifyVersion(const char *fileDesc, const VersionType &currVersion, const VersionType &lastVersion,
                              bool silent = false)
    {
        if (currVersion > lastVersion) {
            if (!silent) {
                LOG_HOST_TOOL_ERROR << fileDesc << " version error, expected version should be less or equal than "
                                    << ConvToStr(lastVersion) << ", but got " << ConvToStr(currVersion);
            }
            return false;
        }
        return true;
    }

    bool VerifyInner(const char* fileDesc, const VersionType &lastVersion) const
    {
        if (magic_ != MAGIC) {
            LOG_HOST_TOOL_ERROR << "Magic mismatch, please make sure " << fileDesc <<
                " and the source code are matched";
            LOG_ECMA(ERROR) << "magic error, expected magic is " << ConvToStr(MAGIC)
                            << ", but got " << ConvToStr(magic_);
            return false;
        }
        if (!VerifyVersion(fileDesc, version_, lastVersion)) {
            return false;
        }
        LOG_ECMA(DEBUG) << "Magic:" << ConvToStr(magic_) << ", version:" << GetVersionInner();
        return true;
    }

    std::string GetVersionInner() const
    {
        return ConvToStr(version_);
    }

    bool SetVersionInner(std::string version)
    {
        std::vector<std::string> versionNumber = StringHelper::SplitString(version, ".");
        if (versionNumber.size() != VERSION_SIZE) {
            LOG_ECMA(ERROR) << "version: " << version << " format error";
            return false;
        }
        for (uint32_t i = 0; i < VERSION_SIZE; i++) {
            uint32_t result;
            if (!StringHelper::StrToUInt32(versionNumber[i].c_str(), &result)) {
                LOG_ECMA(ERROR) << "version: " << version << " format error";
                return false;
            }
            version_[i] = static_cast<uint8_t>(result);
        }
        return true;
    }

private:
    union VersionUnion {
        VersionType version;
        uint32_t versionNumber;
        static_assert(sizeof(VersionType) == sizeof(uint32_t));
    };

    template <size_t size>
    static std::string ConvToStr(const std::array<uint8_t, size> &array)
    {
        std::string ret = "";
        for (size_t i = 0; i < size; ++i) {
            if (i) {
                ret += ".";
            }
            ret += std::to_string(array[i]);
        }
        return ret;
    }

    std::array<uint8_t, MAGIC_SIZE> magic_;
    VersionType version_;
};

}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_FILE_HEADER_H
