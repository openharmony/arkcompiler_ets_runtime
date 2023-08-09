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

#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include <memory>

#include "ecmascript/base/file_header.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
bool PGOProfilerDecoder::Load()
{
    if (isLoaded_) {
        Clear();
    }
    if (!LoadAPBinaryFile()) {
        return false;
    }
    void *addr = fileMapAddr_.GetOriginAddr();

    if (!PGOProfilerHeader::ParseFromBinary(addr, fileMapAddr_.GetSize(), &header_)) {
        UnLoadAPBinaryFile();
        LOG_ECMA(ERROR) << "Parse profiler header failed";
        return false;
    }
    pandaFileInfos_.ParseFromBinary(addr, header_->GetPandaInfoSection());
    if (!recordSimpleInfos_) {
        recordSimpleInfos_ = std::make_unique<PGORecordSimpleInfos>(hotnessThreshold_);
    }
    recordSimpleInfos_->ParseFromBinary(addr, header_);
    UnLoadAPBinaryFile();

    isLoaded_ = true;
    return true;
}

bool PGOProfilerDecoder::Verify(uint32_t checksum)
{
    if (!isLoaded_) {
        return false;
    }
    if (IsMethodMatchEnabled()) {
        LOG_ECMA(INFO) << "Skip verify file checksum, use method checksum instead.";
        isVerifySuccess_ = true;
    } else {
        isVerifySuccess_ = pandaFileInfos_.Checksum(checksum);
    }
    return isVerifySuccess_;
}

bool PGOProfilerDecoder::LoadAndVerify(uint32_t checksum)
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

bool PGOProfilerDecoder::LoadFull()
{
    if (isLoaded_) {
        Clear();
    }
    // profiler dump tools may write data to memory when merge ap files.
    if (!LoadAPBinaryFile(PAGE_PROT_READWRITE)) {
        return false;
    }
    void *addr = fileMapAddr_.GetOriginAddr();

    if (!PGOProfilerHeader::ParseFromBinary(addr, fileMapAddr_.GetSize(), &header_)) {
        UnLoadAPBinaryFile();
        LOG_ECMA(ERROR) << "Parse profiler header failed";
        return false;
    }
    pandaFileInfos_.ParseFromBinary(addr, header_->GetPandaInfoSection());
    if (!recordDetailInfos_) {
        recordDetailInfos_ = std::make_shared<PGORecordDetailInfos>(hotnessThreshold_);
    }
    recordDetailInfos_->ParseFromBinary(addr, header_);

    isLoaded_ = true;
    return true;
}

bool PGOProfilerDecoder::SaveAPTextFile(const std::string &outPath)
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

bool PGOProfilerDecoder::LoadAPBinaryFile(int prot)
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
    fileMapAddr_ = FileMap(realPath.c_str(), FILE_RDONLY, prot);
    if (fileMapAddr_.GetOriginAddr() == nullptr) {
        LOG_ECMA(ERROR) << "File mmap failed";
        return false;
    }
    return true;
}

void PGOProfilerDecoder::UnLoadAPBinaryFile()
{
    if (fileMapAddr_.GetOriginAddr() != nullptr && fileMapAddr_.GetSize() > 0) {
        FileUnMap(fileMapAddr_);
        fileMapAddr_.Reset();
    }
}

void PGOProfilerDecoder::Clear()
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

bool PGOProfilerDecoder::Match(const CString &recordName, PGOMethodId methodId)
{
    if (!isLoaded_) {
        return true;
    }
    if (!isVerifySuccess_) {
        return false;
    }
    return recordSimpleInfos_->Match(recordName, methodId);
}

bool PGOProfilerDecoder::GetHClassLayoutDesc(PGOSampleType classType, PGOHClassLayoutDesc **desc) const
{
    if (!isLoaded_ || !isVerifySuccess_) {
        return false;
    }
    return recordSimpleInfos_->GetHClassLayoutDesc(classType, desc);
}

void PGOProfilerDecoder::GetMismatchResult(uint32_t &totalMethodCount, uint32_t &mismatchMethodCount,
                                           std::set<std::pair<std::string, CString>> &mismatchMethodSet) const
{
    if (!isLoaded_ || !isVerifySuccess_) {
        return;
    }
    return recordSimpleInfos_->GetMismatchResult(totalMethodCount, mismatchMethodCount, mismatchMethodSet);
}

bool PGOProfilerDecoder::InitMergeData()
{
    ASSERT(!isLoaded_);
    if (!recordSimpleInfos_) {
        recordSimpleInfos_ = std::make_unique<PGORecordSimpleInfos>(hotnessThreshold_);
    }
    if (!header_) {
        // For merge scene, we only care about the ap capability which is in the version field.
        PGOProfilerHeader::Build(&header_, sizeof(PGOProfilerHeader));
        memset_s(header_, sizeof(PGOProfilerHeader), 0, sizeof(PGOProfilerHeader));
    }
    isLoaded_ = true;
    isVerifySuccess_ = true;
    return true;
}

void PGOProfilerDecoder::Merge(const PGOProfilerDecoder &decoder)
{
    if (!isLoaded_ || !isVerifySuccess_) {
        return;
    }
    // For merge scene, we chose the highest version from input ap files
    if (!(header_->CompatibleVerify(decoder.header_->GetVersion()))) {
        // For merge scene, we only care about the ap capability which is in the version field.
        memcpy_s(header_, sizeof(base::FileHeaderBase), decoder.header_, sizeof(base::FileHeaderBase));
    }
    pandaFileInfos_.Merge(decoder.GetPandaFileInfos());
    recordSimpleInfos_->Merge(decoder.GetRecordSimpleInfos());
}
} // namespace panda::ecmascript
