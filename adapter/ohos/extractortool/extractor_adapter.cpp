/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/log_wrapper.h"
#include "extractor_adapter.h"

namespace panda::ecmascript {

std::shared_ptr<FileMapper> GetSafeDataAsShared(Extractor& extractor, const std::string& fileName)
{
    std::unique_ptr<FileMapper> uniqueMapper = extractor.GetSafeData(fileName);
    if (uniqueMapper) {
        uniqueMapper->SetAutoReleaseMem(true);
    }
    return std::shared_ptr<FileMapper>(uniqueMapper.release());
}

std::string GetFilePathByOffset(ZipFile& zipFile, uintptr_t offset)
{
    static const std::string dynamicAbc = "ets/modules.abc";
    static constexpr char extNameAbc[] = ".abc";
    const ZipEntryMap& entries = zipFile.GetAllEntries();
    for (const auto& entry : entries) {
        if (entry.first.size() >= sizeof(extNameAbc) - 1 &&
            entry.first.compare(entry.first.size() - (sizeof(extNameAbc) - 1),
                                sizeof(extNameAbc) - 1, extNameAbc) != 0) {
            continue;  // only .abc entries carry a meaningful data offset
        }
        ZipPos dataOffset = 0;
        uint32_t length = 0;
        if (zipFile.GetDataOffsetRelative(entry.second, dataOffset, length) &&
            static_cast<uintptr_t>(dataOffset) == offset) {
            return entry.first;
        }
    }
    LOG_ECMA(WARN) << "Unknown offset, attemp to parse " << dynamicAbc;
    return dynamicAbc;
}
}  // namespace panda::ecmascript
