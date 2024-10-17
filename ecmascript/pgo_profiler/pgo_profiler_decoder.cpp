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

#include "ecmascript/platform/file.h"
#include "ecmascript/pgo_profiler/pgo_profiler_info.h"

namespace panda::ecmascript::pgo {
bool PGOProfilerDecoder::Load(const std::shared_ptr<PGOAbcFilePool> &externalAbcFilePool)
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
    LoadAbcIdPool(externalAbcFilePool, *recordSimpleInfos_, addr);
    recordSimpleInfos_->ParseFromBinary(addr, header_, abcFilePool_);
    UnLoadAPBinaryFile();

    isLoaded_ = true;
    return true;
}

bool PGOProfilerDecoder::Verify(uint32_t checksum)
{
    if (!isLoaded_) {
        return false;
    }
    // Notice: lx maybe can support method checksum;
    return pandaFileInfos_.Checksum(checksum);
}

bool PGOProfilerDecoder::LoadAndVerify(uint32_t checksum, const std::shared_ptr<PGOAbcFilePool> &externalAbcFilePool)
{
    // The file does not exist. Enter full compiler mode.
    if (inPath_.empty()) {
        LOG_ECMA(INFO) << "When the file is empty. Enter full compiler mode.";
        Clear();
        return true;
    }
    if (Load(externalAbcFilePool) && Verify(checksum)) {
        return true;
    }
    return false;
}

bool PGOProfilerDecoder::LoadFull(const std::shared_ptr<PGOAbcFilePool> &externalAbcFilePool)
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

    LoadAbcIdPool(externalAbcFilePool, *recordDetailInfos_, addr);
    if (!recordDetailInfos_->ParseFromBinary(addr, header_)) {
        return false;
    }
    recordDetailInfos_->ResetAbcIdRemap();
    isLoaded_ = true;
    return true;
}

void PGOProfilerDecoder::LoadAbcIdPool(const std::shared_ptr<PGOAbcFilePool> &externalAbcFilePool,
                                       PGOContext &context, void *addr)
{
    if (externalAbcFilePool != nullptr) {
        abcFilePool_ = externalAbcFilePool;
        externalAbcFilePool_ = true;
    } else {
        abcFilePool_ = std::make_unique<PGOAbcFilePool>();
        externalAbcFilePool_ = false;
    }

    if (header_->SupportProfileTypeWithAbcId()) {
        auto abcFilePoolTemp = std::make_shared<PGOAbcFilePool>();
        PGOFileSectionInterface::ParseSectionFromBinary(context, addr, header_, *abcFilePoolTemp->GetPool());
        // step1: [abc pool merge] merge abcFilePool from ap file to memory.
        abcFilePool_->Merge(context, *abcFilePoolTemp);
    }
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
    if (header_ == nullptr) {
        LOG_ECMA(FATAL) << "PGOProfilerDecoder::SaveAPTextFile:header_ is nullptr";
    }
    if (!header_->ProcessToText(fileStream)) {
        return false;
    }
    pandaFileInfos_.ProcessToText(fileStream);
    recordDetailInfos_->ProcessToText(fileStream);
    abcFilePool_->GetPool()->ProcessToText(fileStream);
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
        if (abcFilePool_ && !externalAbcFilePool_) {
            abcFilePool_->Clear();
        }
        if (recordDetailInfos_) {
            recordDetailInfos_->Clear();
        }
        if (recordSimpleInfos_) {
            recordSimpleInfos_->Clear();
        }
        isLoaded_ = false;
    }
}

bool PGOProfilerDecoder::Match(const JSPandaFile *jsPandaFile, const CString &recordName, PGOMethodId methodId)
{
    if (!isLoaded_) {
        return true;
    }
    if (!isVerifySuccess_) {
        return false;
    }
    return recordSimpleInfos_->Match(GetNormalizedFileDesc(jsPandaFile), recordName, EntityId(methodId));
}

bool PGOProfilerDecoder::GetHClassTreeDesc(PGOSampleType profileType, PGOHClassTreeDesc **desc) const
{
    if (!isLoaded_ || !isVerifySuccess_) {
        return false;
    }
    return recordSimpleInfos_->GetHClassTreeDesc(profileType, desc);
}

void PGOProfilerDecoder::GetMismatchResult(const JSPandaFile *jsPandaFile, uint32_t &totalMethodCount,
                                           uint32_t &mismatchMethodCount,
                                           std::set<std::pair<std::string, CString>> &mismatchMethodSet) const
{
    if (!isLoaded_ || !isVerifySuccess_) {
        return;
    }
    return recordSimpleInfos_->GetMismatchResult(GetNormalizedFileDesc(jsPandaFile), totalMethodCount,
                                                 mismatchMethodCount, mismatchMethodSet);
}

CString PGOProfilerDecoder::GetNormalizedFileDesc(const JSPandaFile *jsPandaFile) const
{
    ASSERT(jsPandaFile != nullptr);
    if (header_->SupportProfileTypeWithAbcId()) {
        return jsPandaFile->GetNormalizedFileDesc();
    }
    return "";
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
        ASSERT(header_ != nullptr);
        memset_s(header_, sizeof(PGOProfilerHeader), 0, sizeof(PGOProfilerHeader));
    }
    if (!abcFilePool_) {
        abcFilePool_ = std::make_shared<PGOAbcFilePool>();
        externalAbcFilePool_ = false;
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
} // namespace panda::ecmascript::pgo
