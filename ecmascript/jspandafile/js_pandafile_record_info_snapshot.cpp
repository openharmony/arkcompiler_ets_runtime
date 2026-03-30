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

#include "ecmascript/jspandafile/js_pandafile_record_info_snapshot.h"

#include <memory>

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
size_t JSPandaFileRecordInfoSnapshot::CalculateRecordInfoSectionSize(const JSPandaFile *jsPandaFile)
{
    // Skip if bundlePack (unmerged pf doesn't need this optimization)
    if (jsPandaFile->IsBundlePack()) {
        return 0;
    }

    const auto &jsRecordInfo = jsPandaFile->jsRecordInfo_;
    const auto &npmEntries = jsPandaFile->npmEntries_;

    constexpr size_t baseSectionSize =
        sizeof(jsPandaFile->numClasses_) + sizeof(jsPandaFile->numMethods_) + sizeof(uint8_t);
    constexpr size_t recordCountSize = sizeof(uint32_t);
    constexpr size_t npmEntriesCountSize = sizeof(uint32_t);
    constexpr size_t flagsSize = sizeof(uint8_t);

    size_t jsRecordInfoEntriesSize = recordCountSize;
    for (const auto &jsRecord : jsRecordInfo) {
        const auto &recordName = jsRecord.first;
        const auto *recordInfo = jsRecord.second;
        const uint32_t recordNameLen = recordName.size();
        const uint32_t npmPackageNameLen = recordInfo->npmPackageName.size();
        jsRecordInfoEntriesSize += sizeof(recordNameLen) + recordNameLen + flagsSize +
                                   sizeof(recordInfo->jsonStringId) + sizeof(recordInfo->moduleRecordIdx) +
                                   sizeof(recordInfo->lazyImportIdx) + sizeof(recordInfo->classId) +
                                   sizeof(npmPackageNameLen) + npmPackageNameLen;
    }

    size_t npmEntriesDataSize = npmEntriesCountSize;
    for (const auto &npmEntry : npmEntries) {
        const uint32_t keyLen = npmEntry.first.size();
        const uint32_t valueLen = npmEntry.second.size();
        npmEntriesDataSize += sizeof(keyLen) + keyLen + sizeof(valueLen) + valueLen;
    }

    return baseSectionSize + jsRecordInfoEntriesSize + npmEntriesDataSize;
}

bool JSPandaFileRecordInfoSnapshot::WriteRecordInfoSection(const EcmaVM *vm, const JSPandaFile *jsPandaFile,
                                                           FileMemMapWriter &writer)
{
    // Skip if bundlePack (unmerged pf doesn't need this optimization)
    if (jsPandaFile->IsBundlePack()) {
        return false;
    }

    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
                      "JSPandaFileRecordInfoSnapshot::WriteRecordInfoSection", "");

    // Write numClasses_
    uint32_t numClasses = jsPandaFile->numClasses_;
    writer.WriteSingleData(&numClasses, sizeof(numClasses), "numClasses");

    // Write numMethods_
    uint32_t numMethods = jsPandaFile->numMethods_;
    writer.WriteSingleData(&numMethods, sizeof(numMethods), "numMethods");

    // Write isBundlePack_
    uint8_t isBundlePack = jsPandaFile->isBundlePack_ ? 1 : 0;
    writer.WriteSingleData(&isBundlePack, sizeof(isBundlePack), "isBundlePack");

    // Write jsRecordInfo count
    const auto &jsRecordInfo = jsPandaFile->GetJSRecordInfo();
    uint32_t recordCount = jsRecordInfo.size();
    writer.WriteSingleData(&recordCount, sizeof(recordCount), "recordCount");

    // Write each JSRecordInfo
    for (const auto &[recordName, info] : jsRecordInfo) {
        // Write recordName (owned copy to avoid string_view dangling)
        uint32_t recordNameLen = recordName.size();
        writer.WriteSingleData(&recordNameLen, sizeof(recordNameLen), "recordNameLen");
        writer.WriteSingleData(recordName.data(), recordNameLen, "recordName");

        // Pack flags into single byte
        uint8_t flags = (info->isCjs ? 0x01 : 0) | (info->isJson ? 0x02 : 0) | (info->isSharedModule ? 0x04 : 0) |
                        (info->hasTopLevelAwait ? 0x08 : 0);
        writer.WriteSingleData(&flags, sizeof(flags), "flags");

        // Write integer fields
        writer.WriteSingleData(&info->jsonStringId, sizeof(info->jsonStringId), "jsonStringId");
        writer.WriteSingleData(&info->moduleRecordIdx, sizeof(info->moduleRecordIdx), "moduleRecordIdx");
        writer.WriteSingleData(&info->lazyImportIdx, sizeof(info->lazyImportIdx), "lazyImportIdx");
        writer.WriteSingleData(&info->classId, sizeof(info->classId), "classId");

        // Write npmPackageName (owned string)
        uint32_t pkgNameLen = info->npmPackageName.size();
        writer.WriteSingleData(&pkgNameLen, sizeof(pkgNameLen), "pkgNameLen");
        if (pkgNameLen > 0) {
            writer.WriteSingleData(info->npmPackageName.c_str(), pkgNameLen, "pkgName");
        }
    }

    // Write npmEntries count and entries
    // Access npmEntries_ directly since we're a friend class
    const auto &npmEntries = jsPandaFile->npmEntries_;
    uint32_t npmCount = npmEntries.size();
    writer.WriteSingleData(&npmCount, sizeof(npmCount), "npmCount");

    for (const auto &[key, value] : npmEntries) {
        uint32_t keyLen = key.size();
        writer.WriteSingleData(&keyLen, sizeof(keyLen), "keyLen");
        writer.WriteSingleData(key.data(), keyLen, "key");
        uint32_t valLen = value.size();
        writer.WriteSingleData(&valLen, sizeof(valLen), "valLen");
        writer.WriteSingleData(value.data(), valLen, "value");
    }

    return true;
}

bool JSPandaFileRecordInfoSnapshot::ReadRecordInfoSection(JSPandaFile *jsPandaFile, FileMemMapReader &reader)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "JSPandaFileRecordInfoSnapshot::ReadRecordInfoSection",
                      "");

    // Read numClasses_
    uint32_t numClasses = 0;
    if (!reader.ReadSingleData(&numClasses, sizeof(numClasses), "numClasses")) {
        return false;
    }
    jsPandaFile->numClasses_ = numClasses;

    // Read numMethods_
    uint32_t numMethods = 0;
    if (!reader.ReadSingleData(&numMethods, sizeof(numMethods), "numMethods")) {
        return false;
    }
    jsPandaFile->numMethods_ = numMethods;

    // Read isBundlePack_
    uint8_t isBundlePack = 0;
    if (!reader.ReadSingleData(&isBundlePack, sizeof(isBundlePack), "isBundlePack")) {
        return false;
    }
    jsPandaFile->isBundlePack_ = (isBundlePack != 0);

    // Read jsRecordInfo count
    uint32_t recordCount = 0;
    if (!reader.ReadSingleData(&recordCount, sizeof(recordCount), "recordCount")) {
        return false;
    }

    // Reserve space for jsRecordInfo
    jsPandaFile->jsRecordInfo_.reserve(recordCount);

    // Reserve storage for owned strings to avoid string_view dangling
    // The string_view keys in jsRecordInfo_ and npmEntries_ will point to these owned strings
    jsPandaFile->ownedRecordNames_.reserve(recordCount);

    // Read each JSRecordInfo
    for (uint32_t i = 0; i < recordCount; i++) {
        // Read recordName
        uint32_t recordNameLen = 0;
        if (!reader.ReadSingleData(&recordNameLen, sizeof(recordNameLen), "recordNameLen")) {
            return false;
        }
        CString recordName;
        if (!reader.ReadString(recordName, recordNameLen, "recordName")) {
            return false;
        }

        // Store recordName in persistent storage FIRST, then create string_view from it
        jsPandaFile->ownedRecordNames_.push_back(std::move(recordName));
        const CString &storedRecordName = jsPandaFile->ownedRecordNames_.back();

        // Create JSRecordInfo
        auto info = std::make_unique<JSPandaFile::JSRecordInfo>();

        // Read flags
        uint8_t flags = 0;
        if (!reader.ReadSingleData(&flags, sizeof(flags), "flags")) {
            return false;
        }
        info->isCjs = (flags & 0x01) != 0;
        info->isJson = (flags & 0x02) != 0;
        info->isSharedModule = (flags & 0x04) != 0;
        info->hasTopLevelAwait = (flags & 0x08) != 0;

        // Read integer fields
        if (!reader.ReadSingleData(&info->jsonStringId, sizeof(info->jsonStringId), "jsonStringId")) {
            return false;
        }
        if (!reader.ReadSingleData(&info->moduleRecordIdx, sizeof(info->moduleRecordIdx), "moduleRecordIdx")) {
            return false;
        }
        if (!reader.ReadSingleData(&info->lazyImportIdx, sizeof(info->lazyImportIdx), "lazyImportIdx")) {
            return false;
        }
        if (!reader.ReadSingleData(&info->classId, sizeof(info->classId), "classId")) {
            return false;
        }

        // Read npmPackageName
        uint32_t pkgNameLen = 0;
        if (!reader.ReadSingleData(&pkgNameLen, sizeof(pkgNameLen), "pkgNameLen")) {
            return false;
        }
        if (pkgNameLen > 0) {
            if (!reader.ReadString(info->npmPackageName, pkgNameLen, "pkgName")) {
                return false;
            }
        }

        // Create string_view key pointing to the persistent owned string
        // This is SAFE because storedRecordName lives as long as jsPandaFile
        JSPandaFile::JSRecordInfo *rawInfo = info.release();
        jsPandaFile->jsRecordInfo_.emplace(std::string_view(storedRecordName.c_str(), storedRecordName.size()),
                                           rawInfo);
    }

    // Read npmEntries count
    uint32_t npmCount = 0;
    if (!reader.ReadSingleData(&npmCount, sizeof(npmCount), "npmCount")) {
        return false;
    }

    // Reserve storage for owned npm entries
    jsPandaFile->ownedNpmEntries_.reserve(npmCount);

    // Read npmEntries
    for (uint32_t i = 0; i < npmCount; i++) {
        uint32_t keyLen = 0;
        if (!reader.ReadSingleData(&keyLen, sizeof(keyLen), "keyLen")) {
            return false;
        }
        CString key;
        if (!reader.ReadString(key, keyLen, "key")) {
            return false;
        }

        uint32_t valLen = 0;
        if (!reader.ReadSingleData(&valLen, sizeof(valLen), "valLen")) {
            return false;
        }
        CString value;
        if (!reader.ReadString(value, valLen, "value")) {
            return false;
        }

        // Store in persistent storage FIRST
        jsPandaFile->ownedNpmEntries_.push_back(std::make_pair(std::move(key), std::move(value)));
        const auto &storedEntry = jsPandaFile->ownedNpmEntries_.back();

        // Create string_view keys pointing to persistent owned strings
        jsPandaFile->npmEntries_.emplace(std::string_view(storedEntry.first.c_str(), storedEntry.first.size()),
                                         std::string_view(storedEntry.second.c_str(), storedEntry.second.size()));
    }

    // Allocate methodLiterals_ buffer
    if (numMethods > 0) {
        jsPandaFile->methodLiterals_ = static_cast<MethodLiteral *>(JSPandaFileManager::AllocateBuffer(
            sizeof(MethodLiteral) * numMethods, jsPandaFile->isBundlePack_, jsPandaFile->mode_));

#if ENABLE_LATEST_OPTIMIZATION
        jsPandaFile->methodLiteralMap_.Reserve(numMethods);
#else
        jsPandaFile->methodLiteralMap_.reserve(numMethods);
#endif
    }

    LOG_ECMA(INFO) << "JSPandaFileRecordInfoSnapshot::ReadRecordInfoSection success";
    return true;
}
}  // namespace panda::ecmascript
