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

#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/pgo_profiler/pgo_trace.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/platform/os.h"

namespace panda::ecmascript::pgo {

bool PGOProfilerEncoder::Save(const std::shared_ptr<PGOInfo> pgoInfo)
{
    return InternalSave(pgoInfo);
}

bool PGOProfilerEncoder::SaveAndRename(const std::shared_ptr<PGOInfo> info, const SaveTask* task)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfilerEncoder::SaveAndRename");
    LOG_PGO(INFO) << "start save and rename ap file to " << path_;
    ClockScope start;
    umask(S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); // ensure the permission os ap file is -rw-------
    static const char* tempSuffix = ".tmp";
    auto tmpOutPath = path_ + "." + std::to_string(getpid()) + tempSuffix;
    std::fstream fileStream(tmpOutPath.c_str(),
                            std::fstream::binary | std::fstream::out | std::fstream::in | std::fstream::trunc);
    if (!fileStream.is_open()) {
        LOG_PGO(ERROR) << "can't open the file path (" << tmpOutPath << ") errno: " << errno;
        return false;
    }
    if (info->GetHeaderPtr() == nullptr) {
        LOG_PGO(FATAL) << "[PGOProfilerEncoder::SaveAndRename] header_ is not initialized";
    }
    info->GetPandaFileInfos().ProcessToBinary(fileStream, info->GetHeaderPtr()->GetPandaInfoSection());
    info->GetRecordDetailInfosPtr()->ProcessToBinary(task, fileStream, info->GetHeaderPtr());
    PGOFileSectionInterface::ProcessSectionToBinary(
        info->GetRecordDetailInfos(), fileStream, info->GetHeaderPtr(), *info->GetAbcFilePool().GetPool());
    info->GetHeaderPtr()->SetFileSize(static_cast<uint32_t>(fileStream.tellp()));
    info->GetHeaderPtr()->SetCompatibleAnVersion(AOTFileVersion::AN_VERSION);
    info->GetHeaderPtr()->ProcessToBinary(fileStream);
    if (info->GetHeaderPtr()->SupportFileConsistency()) {
        AddChecksum(fileStream);
    }
    fileStream.close();
    umask(0); // unset umask to avoid affecting other file permissions in the process
    if (task && task->IsTerminate()) {
        return false;
    }
    if (FileExist(path_.c_str()) && remove(path_.c_str())) {
        LOG_PGO(ERROR) << "remove " << path_ << " failed, errno: " << errno;
        return false;
    }
    if (rename(tmpOutPath.c_str(), path_.c_str())) {
        LOG_PGO(ERROR) << "rename " << tmpOutPath << " to " << path_ << " failed, errno: " << errno;
        return false;
    }
    PGOProfilerManager::GetInstance()->RequestAot();
    if (PGOTrace::GetInstance()->IsEnable()) {
        PGOTrace::GetInstance()->SetSaveTime(start.TotalSpentTime());
        PGOTrace::GetInstance()->Print();
    }
    LOG_PGO(INFO) << "successfully save and rename ap file to " << path_;
    return true;
}

bool PGOProfilerEncoder::InternalSave(const std::shared_ptr<PGOInfo> rtInfo, const SaveTask* task)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PGOProfilerEncoder::InternalSave");
    LOG_PGO(INFO) << (mode_ == MERGE ? "MERGE(1)" : "OVERWRITE(0)") << " pgo info";
    if ((mode_ == MERGE) && FileExist(path_.c_str())) {
        auto info = std::make_shared<PGOInfo>(rtInfo->GetHotnessThreshold());
        PGOProfilerDecoder decoder(path_, rtInfo->GetHotnessThreshold());
        info->MergeWithExistProfile(*rtInfo, decoder, task);
        return SaveAndRename(info, task);
    }
    return SaveAndRename(rtInfo, task);
}

void PGOProfilerEncoder::AddChecksum(std::fstream& fileStream)
{
    static constexpr uint32_t KILO_BYTES = 1024;
    static constexpr uint32_t STEP_IN_KB = 256;
    static constexpr uint32_t STEP_SIZE = STEP_IN_KB * KILO_BYTES;
    uint32_t size = static_cast<uint32_t>(fileStream.seekp(0, std::fstream::end).tellp());
    std::unique_ptr<std::vector<uint8_t>> buffer = std::make_unique<std::vector<uint8_t>>(STEP_SIZE);
    // first, calculate the version field's checksum.
    fileStream.seekg(PGOProfilerHeader::MAGIC_SIZE, std::fstream::beg)
        .read(reinterpret_cast<char*>(buffer->data()), PGOProfilerHeader::VERSION_SIZE);
    uint32_t checksum = adler32(0, reinterpret_cast<const Bytef*>(buffer->data()), PGOProfilerHeader::VERSION_SIZE);
    // second, calculate the checksum for remaining content(exclude checksum field).
    uint32_t remainingSize = size - PGOProfilerHeader::CHECKSUM_END_OFFSET;
    fileStream.seekg(PGOProfilerHeader::CHECKSUM_END_OFFSET);
    while (remainingSize > 0) {
        uint32_t readSize = std::min(STEP_SIZE, remainingSize);
        remainingSize = remainingSize - readSize;
        fileStream.read(reinterpret_cast<char*>(buffer->data()), readSize);
        checksum = adler32(checksum, reinterpret_cast<const Bytef*>(buffer->data()), readSize);
    }
    // third, write the checksum back to the checksum field in the output stream.
    fileStream.seekp(PGOProfilerHeader::MAGIC_SIZE + PGOProfilerHeader::VERSION_SIZE, std::fstream::beg);
    fileStream.write(reinterpret_cast<char*>(&checksum), sizeof(checksum));
}

void PGOProfilerEncoder::TerminateSaveTask()
{
    Taskpool::GetCurrentTaskpool()->TerminateTask(GLOBAL_TASK_ID, TaskType::PGO_SAVE_TASK);
}

void PGOProfilerEncoder::PostSaveTask(const std::string& path,
                                      ApGenMode mode,
                                      const std::shared_ptr<PGOInfo> pgoInfo)
{
    LOG_PGO(INFO) << "dispatch save task, path: " << path;
    auto encoder = std::make_shared<PGOProfilerEncoder>(path, mode);
    Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SaveTask>(encoder, pgoInfo, GLOBAL_TASK_ID));
}

void PGOProfilerEncoder::StartSaveTask(const std::shared_ptr<PGOInfo> info, const SaveTask* task)
{
    if (task == nullptr) {
        return;
    }
    if (task->IsTerminate()) {
        LOG_PGO(ERROR) << "save task is terminated";
        return;
    }
    LockHolder lock(PGOProfilerManager::GetPGOInfoMutex());
    InternalSave(info, task);
}

ApGenMode PGOProfilerEncoder::GetApGenMode() const
{
    return mode_;
}

void PGOProfilerEncoder::SetApGenMode(ApGenMode mode)
{
    mode_ = mode;
}
} // namespace panda::ecmascript::pgo
