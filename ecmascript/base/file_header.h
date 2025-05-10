/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "utils/bit_utils.h"

#include <array>
#include <optional>
#include <stddef.h>
#include <stdint.h>

namespace panda::ecmascript::base {
class FileHeaderBase {
public:
    static constexpr size_t MAGIC_SIZE = 8;
    static constexpr size_t VERSION_SIZE = 4;
    static constexpr uint32_t CHECKSUM_END_OFFSET = MAGIC_SIZE + VERSION_SIZE + sizeof(uint32_t);
    static constexpr std::array<uint8_t, MAGIC_SIZE> MAGIC = {'P', 'A', 'N', 'D', 'A', '\0', '\0', '\0'};
    using VersionType = std::array<uint8_t, VERSION_SIZE>;

    static VersionType ToVersion(uint32_t versionNumber)
    {
        return bit_cast<VersionType>(ReverseBytes(versionNumber));
    }

    static uint32_t ToVersionNumber(const VersionType &version)
    {
        return ReverseBytes(bit_cast<uint32_t>(version));
    }

    static bool VerifyVersion(const char *fileDesc, uint32_t currVersion, uint32_t lastVersion, bool strictMatch)
    {
        return VerifyVersion(fileDesc, ToVersion(currVersion), ToVersion(lastVersion), strictMatch);
    }

    template <size_t size>
    static std::string ConvToStr(const std::array<uint8_t, size> &array)
    {
        std::string ret;
        for (size_t i = 0; i < size; ++i) {
            if (i) {
                ret += ".";
            }
            ret += std::to_string(array.at(i));
        }
        return ret;
    }

    static std::optional<VersionType> strToVersion(const std::string& version)
    {
        std::vector<std::string> versionNumber = StringHelper::SplitString(version, ".");
        VersionType formatVersion;
        if (versionNumber.size() != VERSION_SIZE) {
            return {};
        }
        for (uint32_t i = 0; i < VERSION_SIZE; i++) {
            uint32_t result = 0;
            if (!StringHelper::StrToUInt32(versionNumber[i].c_str(), &result)) {
                return {};
            }
            formatVersion.at(i) = static_cast<uint8_t>(result);
        }
        return formatVersion;
    }

    static bool VerifyVersionWithoutFile(const VersionType& currVersion, const VersionType& lastVersion)
    {
        if (currVersion > lastVersion) {
            return true;
        }
        return false;
    }

    bool VerifyVersion(const char *fileDesc, const VersionType &lastVersion, bool strictMatch) const
    {
        if (magic_ != MAGIC) {
            LOG_HOST_TOOL_ERROR << "Magic mismatch, please make sure " << fileDesc
                                << " and the source code are matched";
            LOG_ECMA(ERROR) << "magic error, expected magic is " << ConvToStr(MAGIC) << ", but got "
                            << ConvToStr(magic_);
            return false;
        }
        if (!VerifyVersion(fileDesc, version_, lastVersion, strictMatch)) {
            return false;
        }
        LOG_ECMA(DEBUG) << "Magic:" << ConvToStr(magic_) << ", version:" << InternalGetVersion();
        return true;
    }

    bool CompatibleVerify(const VersionType &expectVersion) const
    {
        return version_ >= expectVersion;
    }

    VersionType GetVersion() const
    {
        return version_;
    }

    void SetVersion(VersionType version)
    {
        version_ = version;
    }

protected:
    explicit FileHeaderBase(const VersionType &lastVersion) : magic_(MAGIC), version_(lastVersion) {}

    static bool VerifyVersion(const char *fileDesc, const VersionType &currVersion, const VersionType &lastVersion,
                              bool strictMatch)
    {
        bool matched = strictMatch ? (currVersion == lastVersion) : (currVersion <= lastVersion);
        if (!matched) {
            LOG_HOST_TOOL_ERROR << fileDesc << " version error, expected version should be "
                                << (strictMatch ? "equal to " : "less or equal than ") << ConvToStr(lastVersion)
                                << ", but got " << ConvToStr(currVersion);
            return false;
        }
        return true;
    }

    std::string InternalGetVersion() const
    {
        return ConvToStr(version_);
    }

    bool InternalSetVersion(const std::string &version)
    {
        std::vector<std::string> versionNumber = StringHelper::SplitString(version, ".");
        if (versionNumber.size() != VERSION_SIZE) {
            LOG_ECMA(ERROR) << "version: " << version << " format error";
            return false;
        }
        for (uint32_t i = 0; i < VERSION_SIZE; i++) {
            uint32_t result = 0;
            if (!StringHelper::StrToUInt32(versionNumber[i].c_str(), &result)) {
                LOG_ECMA(ERROR) << "version: " << version << " format error";
                return false;
            }
            version_.at(i) = static_cast<uint8_t>(result);
        }
        return true;
    }

private:
    std::array<uint8_t, MAGIC_SIZE> magic_;
    VersionType version_;
};

class FileHeaderElastic : public FileHeaderBase {
public:
    static constexpr uint32_t ENDIAN_VALUE = 0x12345678;
    void SetChecksum(uint32_t checksum)
    {
        checksum_ = checksum;
    }

    uint32_t GetChecksum() const
    {
        return checksum_;
    }

    void SetHeaderSize(uint32_t size)
    {
        headerSize_ = size;
    }

    uint32_t GetHeaderSize() const
    {
        return headerSize_;
    }

    void SetFileSize(uint32_t size)
    {
        fileSize_ = size;
    }

    uint32_t GetFileSize() const
    {
        return fileSize_;
    }

    uint32_t GetEndianTag() const
    {
        return endianTag_;
    }

    VersionType GetCompatibleAnVersion() const
    {
        return compatibleAnVersion_;
    }

    void SetCompatibleAnVersion(VersionType version)
    {
        compatibleAnVersion_ = version;
    }

protected:
    explicit FileHeaderElastic(const VersionType &lastVersion) : FileHeaderBase(lastVersion) {}

private:
    uint32_t checksum_ {0};
    uint32_t fileSize_ {0};
    uint32_t headerSize_ {0};
    uint32_t endianTag_ {ENDIAN_VALUE};
    VersionType compatibleAnVersion_;
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_FILE_HEADER_H
