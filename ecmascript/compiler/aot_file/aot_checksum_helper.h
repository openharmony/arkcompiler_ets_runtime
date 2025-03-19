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

#ifndef ECMASCRIPT_COMPILER_AOT_FILE_AOT_CHECKSUM_HELPER_H
#define ECMASCRIPT_COMPILER_AOT_FILE_AOT_CHECKSUM_HELPER_H
#include <unordered_map>
#include "ecmascript/mem/c_string.h"
namespace panda::ecmascript {
class AOTChecksumHelper {
public:
    AOTChecksumHelper() = delete;
    ~AOTChecksumHelper() = delete;
    static bool SerializeChecksumMapToVector(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap,
                                             std::vector<char> &checksumDataVector);
    static bool DeserializeChecksumMapFromChar(const char *checksumData, uint32_t checksumDataSize,
                                               std::unordered_map<CString, uint32_t> &fileNameToChecksumMap);
    static uint32_t CalculateChecksumVectorSize(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap);
    static bool WriteChecksumInfoToVector(const std::unordered_map<CString, uint32_t> &fileNameToChecksumMap,
                                          std::vector<char> &checksumDataVector);
    static uint32_t FastUint32ToDigits(uint32_t number);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_AOT_FILE_AOT_CHECKSUM_HELPER_H