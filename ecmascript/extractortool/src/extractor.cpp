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

#include "extractor.h"

#include <fstream>

#include "constants.h"
#include "file_path_utils.h"
#include "ecmascript/platform/file.h"

namespace panda {
namespace ecmascript {
namespace {
constexpr char EXT_NAME_ABC[] = ".abc";
}
Extractor::Extractor(const std::string &source) : zipFile_(source)
{
    hapPath_ = source;
}

Extractor::~Extractor()
{}

bool Extractor::Init()
{
    if (!zipFile_.Open()) {
        return false;
    }
    initial_ = true;
    return true;
}

bool Extractor::GetFileBuffer(const std::string& srcPath, std::ostringstream& dest)
{
    if (!initial_) {
        return false;
    }

    if (srcPath.empty()) {
        return false;
    }

    std::string relativePath = GetRelativePath(srcPath);
    if (!ExtractByName(relativePath, dest)) {
        return false;
    }

    return true;
}

bool Extractor::GetFileList(const std::string& srcPath, std::vector<std::string>& assetList)
{
    if (!initial_) {
        return false;
    }

    if (srcPath.empty()) {
        return false;
    }
    zipFile_.GetAllFileList(srcPath, assetList);
    if (assetList.empty()) {
    }

    return true;
}

bool Extractor::HasEntry(const std::string &fileName) const
{
    if (!initial_) {
        return false;
    }

    return zipFile_.HasEntry(fileName);
}

bool Extractor::IsDirExist(const std::string &dir) const
{
    if (!initial_) {
        return false;
    }
    if (dir.empty()) {
        return false;
    }
    return zipFile_.IsDirExist(dir);
}

bool Extractor::ExtractByName(const std::string &fileName, std::ostream &dest) const
{
    if (!initial_) {
        return false;
    }
    if (!zipFile_.ExtractFile(fileName, dest)) {
        return false;
    }
    return true;
}

bool Extractor::ExtractFile(const std::string &fileName, const std::string &targetPath) const
{
    if (!panda::ecmascript::FileExist(targetPath.c_str())) {
        return false;
    }
    std::ofstream fileStream;
    fileStream.open(targetPath, std::ios_base::out | std::ios_base::binary);
    if (!fileStream.is_open()) {
        return false;
    }
    if ((!ExtractByName(fileName, fileStream)) || (!fileStream.good())) {
        fileStream.clear();
        fileStream.close();
        if (remove(targetPath.c_str()) != 0) {
        }
        return false;
    }
    fileStream.clear();
    fileStream.close();
    return true;
}

bool Extractor::IsSameHap(const std::string& hapPath) const
{
    return !hapPath_.empty() && !hapPath.empty() && hapPath_ == hapPath;
}

std::unique_ptr<FileMapper> Extractor::GetData(const std::string &fileName, bool) const
{
    std::string relativePath = GetRelativePath(fileName);
    return zipFile_.CreateFileMapper(relativePath, FileMapperType::NORMAL_MEM);
}

std::shared_ptr<FileMapper> Extractor::GetSafeData(const std::string &fileName)
{
    std::string relativePath = GetRelativePath(fileName);
    if (!StringEndWith(relativePath, EXT_NAME_ABC, sizeof(EXT_NAME_ABC) - 1)) {
        return nullptr;
    }

    return zipFile_.CreateFileMapper(relativePath, FileMapperType::SAFE_ABC);
}

std::unique_ptr<FileMapper> Extractor::GetMmapData(const std::string &fileName)
{
    std::string relativePath = GetRelativePath(fileName);
    return zipFile_.CreateFileMapper(relativePath, FileMapperType::SHARED_MMAP);
}

bool Extractor::IsStageModel()
{
    if (isStageModel_.has_value()) {
        return isStageModel_.value();
    }
    isStageModel_ = !zipFile_.HasEntry("config.json");
    return isStageModel_.value();
}

bool Extractor::ExtractToBufByName(const std::string &fileName, std::unique_ptr<uint8_t[]> &dataPtr,
    size_t &len)
{
    std::string relativePath = GetRelativePath(fileName);
    return zipFile_.ExtractToBufByName(relativePath, dataPtr, len);
}

bool Extractor::GetFileInfo(const std::string &fileName, FileInfo &fileInfo) const
{
    std::string relativePath = GetRelativePath(fileName);
    ZipEntry zipEntry;
    if (!zipFile_.GetEntry(relativePath, zipEntry)) {
        return false;
    }

    ZipPos offset = 0;
    uint32_t length = 0;
    if (!zipFile_.GetDataOffsetRelative(relativePath, offset, length)) {
        return false;
    }

    fileInfo.fileName = fileName;
    fileInfo.offset = static_cast<uint32_t>(offset);
    fileInfo.length = static_cast<uint32_t>(length);
    fileInfo.lastModTime = zipEntry.modifiedTime;
    fileInfo.lastModDate = zipEntry.modifiedDate;
    return true;
}

bool Extractor::GetFileList(const std::string &srcPath, std::set<std::string> &fileSet)
{
    if (!initial_) {
        return false;
    }

    if (srcPath.empty()) {
        return false;
    }

    zipFile_.GetChildNames(srcPath, fileSet);
    if (fileSet.empty()) {
    }

    return true;
}

bool Extractor::IsHapCompress(const std::string &fileName) const
{
    std::string relativePath = GetRelativePath(fileName);
    ZipEntry zipEntry;
    if (!zipFile_.GetEntry(relativePath, zipEntry)) {
        return false;
    }
    return zipEntry.compressionMethod > 0;
}

std::mutex ExtractorUtil::mapMutex_;
std::unordered_map<std::string, std::shared_ptr<Extractor>> ExtractorUtil::extractorMap_;
std::string ExtractorUtil::GetLoadFilePath(const std::string &hapPath)
{
    std::string loadPath;
    if (StringStartWith(hapPath, Constants::ABS_CODE_PATH, std::string(Constants::ABS_CODE_PATH).length())) {
        loadPath = GetLoadPath(hapPath);
    } else {
        loadPath = hapPath;
    }
    return loadPath;
}

std::shared_ptr<Extractor> ExtractorUtil::GetExtractor(const std::string &hapPath, bool &newCreate, bool cache)
{
    newCreate = false;
    if (hapPath.empty()) {
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> mapMutex(mapMutex_);
        auto mapIter = extractorMap_.find(hapPath);
        if (mapIter != extractorMap_.end()) {
            return mapIter->second;
        }
    }

    std::shared_ptr<Extractor> extractor = std::make_shared<Extractor>(hapPath);
    if (!extractor->Init()) {
        return nullptr;
    }
    if (cache) {
        std::lock_guard<std::mutex> mapMutex(mapMutex_);
        extractorMap_.emplace(hapPath, extractor);
    }
    newCreate = true;
    return extractor;
}

void ExtractorUtil::DeleteExtractor(const std::string &hapPath)
{
    if (hapPath.empty()) {
        return;
    }

    std::lock_guard<std::mutex> mapMutex(mapMutex_);
    auto mapIter = extractorMap_.find(hapPath);
    if (mapIter != extractorMap_.end()) {
        extractorMap_.erase(mapIter);
    }
}
}  // namespace AbilityBase
}  // namespace OHOS
