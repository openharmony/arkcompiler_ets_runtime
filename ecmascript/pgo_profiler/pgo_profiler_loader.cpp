/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/pgo_profiler/pgo_profiler_loader.h"

#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
bool PGOProfilerLoader::Load()
{
    if (isLoaded_) {
        Clear();
    }
    if (!LoadAPBinaryFile()) {
        return false;
    }
    void *addr = fileMapAddr_.GetOriginAddr();

    if (!PGOProfilerHeader::ParseFromBinary(addr, &header_)) {
        UnLoadAPBinaryFile();
        LOG_ECMA(ERROR) << "Parse profiler header failed";
        return false;
    }
    pandaFileInfos_.ParseFromBinary(addr, header_->GetPandaInfoSection());
    if (!recordSimpleInfos_) {
        recordSimpleInfos_ = std::make_unique<PGORecordSimpleInfos>(hotnessThreshold_);
    }
    recordSimpleInfos_->ParseFromBinary(addr, header_->GetRecordInfoSection());
    UnLoadAPBinaryFile();

    isLoaded_ = true;
    return true;
}

bool PGOProfilerLoader::Verify(uint32_t checksum)
{
    if (!isLoaded_) {
        return false;
    }
    isVerifySuccess_ = pandaFileInfos_.CheckSum(checksum);
    return isVerifySuccess_;
}

bool PGOProfilerLoader::LoadAndVerify(uint32_t checksum)
{
    // The file does not exist. Enter full compiler mode.
    if (inPath_.empty()) {
        LOG_ECMA(INFO) << "When the file is empty. Enter full compiler mode.";
        Clear();
        return true;
    }
    if (Load() && Verify(checksum)) {
        return true;
    }
    return false;
}

bool PGOProfilerLoader::LoadFull()
{
    if (isLoaded_) {
        Clear();
    }
    if (!LoadAPBinaryFile()) {
        return false;
    }
    void *addr = fileMapAddr_.GetOriginAddr();

    if (!PGOProfilerHeader::ParseFromBinary(addr, &header_)) {
        UnLoadAPBinaryFile();
        LOG_ECMA(ERROR) << "Parse profiler header failed";
        return false;
    }
    pandaFileInfos_.ParseFromBinary(addr, header_->GetPandaInfoSection());
    if (!recordDetailInfos_) {
        recordDetailInfos_ = std::make_unique<PGORecordDetailInfos>(hotnessThreshold_);
    }
    recordDetailInfos_->ParseFromBinary(addr, header_->GetRecordInfoSection());

    isLoaded_ = true;
    return true;
}

bool PGOProfilerLoader::SaveAPTextFile(const std::string &outPath)
{
    if (!isLoaded_) {
        return false;
    }
    std::string realOutPath;
    if (!RealPath(outPath, realOutPath, false)) {
        return false;
    }
    std::ofstream fileStream(realOutPath.c_str());
    if (!fileStream.is_open()) {
        LOG_ECMA(ERROR) << "The file path(" << realOutPath << ") open failure!";
        return false;
    }

    if (!header_->ProcessToText(fileStream)) {
        return false;
    }
    pandaFileInfos_.ProcessToText(fileStream);
    recordDetailInfos_->ProcessToText(fileStream);
    return true;
}

bool PGOProfilerLoader::LoadAPBinaryFile()
{
    std::string realPath;
    if (!RealPath(inPath_, realPath)) {
        return false;
    }

    static const std::string endString = ".ap";
    if (realPath.compare(realPath.length() - endString.length(), endString.length(), endString)) {
        LOG_ECMA(ERROR) << "The file path( " << realPath << ") does not end with .ap";
        return false;
    }
    LOG_ECMA(INFO) << "Load profiler from file:" << realPath;
    fileMapAddr_ = FileMap(realPath.c_str(), FILE_RDONLY, PAGE_PROT_READ);
    if (fileMapAddr_.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "File mmap failed";
        return false;
    }
    return true;
}

void PGOProfilerLoader::UnLoadAPBinaryFile()
{
    if (fileMapAddr_.GetOriginAddr() != nullptr && fileMapAddr_.GetSize() > 0) {
        FileUnMap(fileMapAddr_);
        fileMapAddr_.Reset();
    }
}

void PGOProfilerLoader::Clear()
{
    if (isLoaded_) {
        UnLoadAPBinaryFile();
        isVerifySuccess_ = true;
        hotnessThreshold_ = 0;
        PGOProfilerHeader::Destroy(&header_);
        pandaFileInfos_.Clear();
        if (recordDetailInfos_) {
            recordDetailInfos_->Clear();
        }
        if (recordSimpleInfos_) {
            recordSimpleInfos_->Clear();
        }
        isLoaded_ = false;
    }
}

bool PGOProfilerLoader::Match(const CString &recordName, EntityId methodId)
{
    if (!isLoaded_) {
        return true;
    }
    if (!isVerifySuccess_) {
        return false;
    }
    return recordSimpleInfos_->Match(recordName, methodId);
}
} // namespace panda::ecmascript
