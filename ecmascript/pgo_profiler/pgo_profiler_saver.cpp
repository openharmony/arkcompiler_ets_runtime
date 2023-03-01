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

#include "ecmascript/pgo_profiler/pgo_profiler_saver.h"

#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
static const std::string PROFILE_FILE_NAME = "/modules.ap";
void PGOProfilerSaver::Destroy()
{
    if (!isInitialized_) {
        return;
    }
    PGOProfilerHeader::Destroy(&header_);
    pandaFileInfos_.reset();
    globalRecordInfos_->Clear();
    globalRecordInfos_.reset();
    isInitialized_ = false;
}

bool PGOProfilerSaver::InitializeData()
{
    if (!isInitialized_) {
        if (!RealPath(outDir_, realOutPath_, false)) {
            return false;
        }
        realOutPath_ += PROFILE_FILE_NAME;
        LOG_ECMA(INFO) << "Save profiler to file:" << realOutPath_;
        PGOProfilerHeader::Build(&header_, PGOProfilerHeader::LastSize());
        pandaFileInfos_ = std::make_unique<PGOPandaFileInfos>();
        globalRecordInfos_ = std::make_unique<PGORecordDetailInfos>(hotnessThreshold_);
        isInitialized_ = true;
    }
    return true;
}

void PGOProfilerSaver::SamplePandaFileInfo(uint32_t checksum)
{
    if (!isInitialized_) {
        return;
    }
    pandaFileInfos_->Sample(checksum);
}

void PGOProfilerSaver::Merge(const PGORecordDetailInfos &recordInfos)
{
    if (!isInitialized_) {
        return;
    }
    os::memory::LockHolder lock(mutex_);
    globalRecordInfos_->Merge(recordInfos);
}

void PGOProfilerSaver::Save()
{
    if (!isInitialized_) {
        return;
    }
    os::memory::LockHolder lock(mutex_);
    SaveProfiler();
}

void PGOProfilerSaver::SaveProfiler(const SaveTask *task)
{
    std::ofstream fileStream(realOutPath_.c_str());
    if (!fileStream.is_open()) {
        LOG_ECMA(ERROR) << "The file path(" << realOutPath_ << ") open failure!";
        return;
    }
    pandaFileInfos_->ProcessToBinary(fileStream, header_->GetPandaInfoSection());
    globalRecordInfos_->ProcessToBinary(task, fileStream, header_->GetRecordInfoSection());
    header_->ProcessToBinary(fileStream);
    fileStream.close();
}

void PGOProfilerSaver::TerminateSaveTask()
{
    if (!isInitialized_) {
        return;
    }
    Taskpool::GetCurrentTaskpool()->TerminateTask(GLOBAL_TASK_ID, TaskType::PGO_SAVE_TASK);
}

void PGOProfilerSaver::PostSaveTask()
{
    if (!isInitialized_) {
        return;
    }
    Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SaveTask>(this, GLOBAL_TASK_ID));
}

void PGOProfilerSaver::StartSaveTask(const SaveTask *task)
{
    if (task == nullptr) {
        return;
    }
    if (task->IsTerminate()) {
        LOG_ECMA(ERROR) << "StartSaveTask: task is already terminate";
        return;
    }
    os::memory::LockHolder lock(mutex_);
    SaveProfiler(task);
}

bool PGOProfilerSaver::LoadAPTextFile(const std::string &inPath)
{
    if (!isInitialized_) {
        return false;
    }
    std::string realPath;
    if (!RealPath(inPath, realPath)) {
        return false;
    }

    std::ifstream fileStream(realPath.c_str());
    if (!fileStream.is_open()) {
        LOG_ECMA(ERROR) << "The file path(" << realOutPath_ << ") open failure!";
        return false;
    }

    if (!header_->ParseFromText(fileStream)) {
        LOG_ECMA(ERROR) << "header format error";
        return false;
    }
    if (!pandaFileInfos_->ParseFromText(fileStream)) {
        LOG_ECMA(ERROR) << "panda file info format error";
        return false;
    }
    if (!globalRecordInfos_->ParseFromText(fileStream)) {
        LOG_ECMA(ERROR) << "record info format error";
        return false;
    }

    return true;
}
} // namespace panda::ecmascript
