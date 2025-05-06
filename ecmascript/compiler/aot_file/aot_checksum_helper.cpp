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
#include "ecmascript/base/dtoa_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/compiler/aot_file/aot_checksum_helper.h"
#include "ecmascript/log_wrapper.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {

bool AOTChecksumHelper::SerializeChecksumMapToVector(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap,
                                                     std::vector<char> &checksumDataVector)
{
    // save fileName to checksum relationship as like
    // pandafileNormalizeDes:checksum
    // /xxx/yyy/zzz.abc:123456
    if (fileNameToChecksumMap.empty()) {
        LOG_COMPILER(ERROR) << "abc file checksum map cant't be empty!";
        return false;
    }
    uint32_t checksumVectorSize = CalculateChecksumVectorSize(fileNameToChecksumMap);
    checksumDataVector.resize(checksumVectorSize);
    if (!WriteChecksumInfoToVector(fileNameToChecksumMap, checksumDataVector)) {
        checksumDataVector.resize(0);
        LOG_COMPILER(ERROR) << "Serialize checksumMap to .an failed!";
        return false;
    }
    return true;
}

uint32_t AOTChecksumHelper::CalculateChecksumVectorSize(
    const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
{
    uint32_t size = 0;
    for (const auto &pair : fileNameToChecksumMap) {
        // 2 for ':' and '\0'
        size += pair.first.size() + FastUint32ToDigits(pair.second) + 2;
    }
    return size;
}

uint32_t AOTChecksumHelper::FastUint32ToDigits(uint32_t number)
{
    return (number >= base::DtoaHelper::TEN9POW)   ? 10  // 10 digits
           : (number >= base::DtoaHelper::TEN8POW) ? 9   // 9 digits
           : (number >= base::DtoaHelper::TEN7POW) ? 8   // 8 digits
           : (number >= base::DtoaHelper::TEN6POW) ? 7   // 7 digits
           : (number >= base::DtoaHelper::TEN5POW) ? 6   // 6 digits
           : (number >= base::DtoaHelper::TEN4POW) ? 5   // 5 digits
           : (number >= base::DtoaHelper::TEN3POW) ? 4   // 4 digits
           : (number >= base::DtoaHelper::TEN2POW) ? 3   // 3 digits
           : (number >= base::DtoaHelper::TEN)     ? 2   // 2 digits
                                                   : 1;      // 1 digit
}

bool AOTChecksumHelper::WriteChecksumInfoToVector(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap,
                                                  std::vector<char> &checksumDataVector)
{
    char *basePtr = checksumDataVector.data();
    char *endPtr = basePtr + checksumDataVector.size();
    char *writePtr = basePtr;
    for (const auto &pair : fileNameToChecksumMap) {
        size_t remainSize = endPtr - writePtr;
        int written = snprintf_s(writePtr, remainSize, remainSize - 1, "%s:%u", pair.first.c_str(), pair.second);
        if (written < 0 || static_cast<size_t>(written) >= remainSize) {
            LOG_COMPILER(ERROR) << "wirte checksum info to AOT .an file failed!";
            return false;
        }
        // 1 for '\0'
        writePtr += written + 1;
    }
    if (writePtr != endPtr) {
        LOG_COMPILER(ERROR) << "Checksum vector not fully filled: "
                            << "expected size=" << checksumDataVector.size()
                            << ", actual used=" << (writePtr - basePtr);
        return false;
    }
    return true;
}

bool AOTChecksumHelper::DeserializeChecksumMapFromChar(const char *checksumData, uint32_t checksumDataSize,
                                                       std::unordered_map<CString, uint32_t> &fileNameToChecksumMap)
{
    if (checksumData == nullptr || checksumDataSize == 0) {
        LOG_COMPILER(ERROR) << "Invalid checksum data";
        return false;
    }

    const char *curPtr = checksumData;
    const char *endPtr = checksumData + checksumDataSize;

    while (curPtr < endPtr) {
        const char *entryEnd = static_cast<const char *>(memchr(curPtr, '\0', endPtr - curPtr));
        if (entryEnd == nullptr || entryEnd >= endPtr) {
            LOG_COMPILER(ERROR) << "Corrupted checksum data: missing string terminator";
            return false;
        }

        const char *separator = static_cast<const char *>(memchr(curPtr, ':', entryEnd - curPtr));
        if (separator == nullptr || separator >= entryEnd) {
            LOG_COMPILER(ERROR) << "Corrupted checksum data: missing separator";
            return false;
        }

        if (separator == curPtr || separator + 1 == entryEnd) {
            LOG_COMPILER(ERROR) << "Invalid entry format: empty filename or checksum";
            return false;
        }

        CString filename(curPtr, separator - curPtr);

        uint32_t checksum;
        if (!base::StringHelper::StrToUInt32(separator + 1, &checksum)) {
            LOG_COMPILER(ERROR) << "Invalid checksum value";
            return false;
        }

        fileNameToChecksumMap.emplace(std::move(filename), checksum);

        curPtr = entryEnd + 1;
    }

    if (fileNameToChecksumMap.empty()) {
        LOG_COMPILER(ERROR) << "No valid entries found in checksum data";
        return false;
    }
    return true;
}

}  // namespace panda::ecmascript
