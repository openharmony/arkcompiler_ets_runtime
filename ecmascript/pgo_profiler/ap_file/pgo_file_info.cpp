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

#include "ecmascript/pgo_profiler/ap_file/pgo_file_info.h"
namespace panda::ecmascript::pgo {
using StringHelper = base::StringHelper;
bool PGOProfilerHeader::strictMatch_ = true;

bool PGOProfilerHeader::BuildFromLegacy(void *buffer, PGOProfilerHeader **header)
{
    if (buffer == nullptr || header == nullptr) {
        LOG_ECMA(ERROR) << "buffer or header is null!";
        return false;
    }
    auto *inHeader = reinterpret_cast<PGOProfilerHeaderLegacy *>(buffer);
    size_t desSize = Size(inHeader->GetSectionNumber());
    if (desSize > LastSize()) {
        LOG_ECMA(ERROR) << "header size error, expected size is less than " << LastSize() << ", but got " << desSize;
        return false;
    }
    Build(header, desSize);
    // copy header base.
    if (memcpy_s(*header, sizeof(FileHeaderBase), inHeader, sizeof(FileHeaderBase)) != EOK) {
        UNREACHABLE();
    }
    // skip elastic header field, and copy section info from incoming buffer.
    auto sectionSize = desSize - sizeof(FileHeaderElastic);
    if (memcpy_s(&((*header)->sectionNumber_), sectionSize, &(inHeader->GetSectionNumber()), sectionSize) != EOK) {
        UNREACHABLE();
    }
    return true;
}

bool PGOProfilerHeader::BuildFromElastic(void *buffer, size_t bufferSize, PGOProfilerHeader **header)
{
    auto *inHeader = reinterpret_cast<PGOProfilerHeader *>(buffer);
    if (!inHeader->Verify(buffer, bufferSize)) {
        return false;
    }
    size_t desSize = inHeader->Size();
    if (desSize > LastSize()) {
        LOG_ECMA(ERROR) << "header size error, expected size is less than " << LastSize() << ", but got " << desSize;
        return false;
    }
    Build(header, desSize);
    if (memcpy_s(*header, desSize, inHeader, desSize) != EOK) {
        UNREACHABLE();
    }
    return true;
}

bool PGOProfilerHeader::ParseFromBinary(void *buffer, size_t bufferSize, PGOProfilerHeader **header)
{
    auto *inHeaderBase = reinterpret_cast<FileHeaderBase *>(buffer);
    if (inHeaderBase->VerifyVersion("ap file", LAST_VERSION, IsStrictMatch())) {
        if (!inHeaderBase->CompatibleVerify(ELASTIC_HEADER_MINI_VERSION)) {
            return BuildFromLegacy(buffer, header);
        }
        return BuildFromElastic(buffer, bufferSize, header);
    }
    return false;
}

bool PGOProfilerHeader::VerifyFileSize(size_t bufferSize) const
{
    if (!SupportFileSize()) {
        return true;
    }
    if (GetFileSize() != bufferSize) {
        LOG_ECMA(ERROR) << "Verify ap file's file size failed. size: " << std::hex << bufferSize << " vs "
                        << GetFileSize();
        return false;
    }
    return true;
}

bool PGOProfilerHeader::VerifyConsistency(void *buffer, size_t bufferSize) const
{
    if (!SupportFileConsistency()) {
        return true;
    }
    uint32_t checksum = adler32(0, reinterpret_cast<const Bytef *>(buffer) + MAGIC_SIZE, VERSION_SIZE);
    checksum = adler32(checksum, reinterpret_cast<const Bytef *>(buffer) + CHECKSUM_END_OFFSET,
                       bufferSize - CHECKSUM_END_OFFSET);
    if (checksum != GetChecksum()) {
        LOG_ECMA(ERROR) << "Verify ap file's consistency failed. checksum: " << std::hex << checksum << " vs "
                        << std::hex << GetChecksum();
        return false;
    }
    return true;
}

void PGOProfilerHeader::ProcessToBinary(std::fstream &fileStream) const
{
    fileStream.seekp(0);
    if (base::FileHeaderBase::CompatibleVerify(ELASTIC_HEADER_MINI_VERSION)) {
        fileStream.write(reinterpret_cast<const char *>(this), Size());
    } else {
        // copy header base.
        fileStream.write(reinterpret_cast<const char *>(this), sizeof(FileHeaderBase));
        // skip elastic header field, and copy section info from incoming buffer.
        auto sectionSize = Size() - sizeof(FileHeaderElastic);
        fileStream.write(reinterpret_cast<const char *>(&sectionNumber_), sectionSize);
    }
}

bool PGOProfilerHeader::ParseFromText(std::ifstream &stream)
{
    std::string header;
    if (std::getline(stream, header)) {
        if (header.empty()) {
            return false;
        }
        auto index = header.find(DumpUtils::BLOCK_START);
        if (index == std::string::npos) {
            return false;
        }
        auto version = header.substr(index + 1);
        if (!InternalSetVersion(version)) {
            return false;
        }
        if (!Verify()) {
            return false;
        }
        if (!base::FileHeaderBase::CompatibleVerify(ELASTIC_HEADER_MINI_VERSION)) {
            auto *pandaInfoSection = GetPandaInfoSection();
            if (pandaInfoSection == nullptr) {
                return false;
            }
            pandaInfoSection->offset_ -= sizeof(PGOProfilerHeader) - sizeof(PGOProfilerHeaderLegacy);
        }
        return true;
    }
    return false;
}

bool PGOProfilerHeader::ProcessToText(std::ofstream &stream) const
{
    if (!Verify()) {
        return false;
    }
    stream << DumpUtils::VERSION_HEADER << InternalGetVersion() << DumpUtils::NEW_LINE;
    stream << "Compatible an file version: " << ConvToStr(GetCompatibleAnVersion()) << DumpUtils::NEW_LINE;
    if (SupportFileConsistency()) {
        stream << "FileSize: " << GetFileSize() << " ,HeaderSize: " << GetHeaderSize() << " ,Checksum: " << std::hex
               << GetChecksum() << DumpUtils::NEW_LINE;
    }
    return true;
}
} // namespace panda::ecmascript::pgo