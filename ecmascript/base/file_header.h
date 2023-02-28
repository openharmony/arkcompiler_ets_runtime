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
protected:
    static constexpr size_t MAGIC_SIZE = 8;
    static constexpr size_t VERSION_SIZE = 4;
    static constexpr std::array<uint8_t, MAGIC_SIZE> MAGIC = {'P', 'A', 'N', 'D', 'A', '\0', '\0', '\0'};

    FileHeader(const std::array<uint8_t, VERSION_SIZE> &lastVersion) : magic_(MAGIC), version_(lastVersion) {}

    bool VerifyInner(const std::array<uint8_t, VERSION_SIZE> &lastVersion) const
    {
        if (magic_ != MAGIC) {
            LOG_HOST_TOOL_ERROR << "Magic mismatch, please make sure apPath file and the code are matched";
            LOG_ECMA(ERROR) << "magic error, expected magic is " << ConvToStr(MAGIC)
                            << ", but got " << ConvToStr(magic_);
            return false;
        }
        if (version_ > lastVersion) {
            LOG_HOST_TOOL_ERROR << "version error, expected version should be less or equal than "
                            << ConvToStr(lastVersion) << ", but got " << GetVersionInner();
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
    template <size_t size>
    std::string ConvToStr(std::array<uint8_t, size> array) const
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
    std::array<uint8_t, VERSION_SIZE> version_;
};

}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_FILE_HEADER_H
