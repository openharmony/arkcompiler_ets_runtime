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

#include "ecmascript/dfx/pgo_profiler/pgo_profiler_manager.h"

#include <ios>
#include <sstream>
#include <string>

#include "ecmascript/js_function.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/platform/file.h"

namespace panda::ecmascript {
static const std::string PROFILE_FILE_NAME = "/modules.ap";

PGOProfiler::~PGOProfiler()
{
    isEnable_ = false;
    profilerMap_.clear();
}

void PGOProfiler::Sample(JSTaggedType value, SampleMode mode)
{
    if (!isEnable_) {
        return;
    }
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue jsValue(value);
    if (jsValue.IsJSFunction() && JSFunction::Cast(jsValue)->GetMethod().IsMethod()) {
        auto jsMethod = Method::Cast(JSFunction::Cast(jsValue)->GetMethod());
        JSTaggedValue recordNameValue = JSFunction::Cast(jsValue)->GetRecordName();
        if (recordNameValue.IsHole()) {
            return;
        }
        CString recordName = ConvertToString(recordNameValue);
        auto iter = profilerMap_.find(recordName);
        if (iter != profilerMap_.end()) {
            auto methodCountMap = iter->second;
            auto result = methodCountMap->find(jsMethod->GetMethodId());
            if (result != methodCountMap->end()) {
                auto info = result->second;
                info->IncreaseCount();
                info->SetSampleMode(mode);
            } else {
                size_t len = strlen(jsMethod->GetMethodName());
                void *infoAddr = chunk_.Allocate(MethodProfilerInfo::Size(len));
                auto info = new (infoAddr) MethodProfilerInfo(jsMethod->GetMethodId(), 1, mode, len);
                info->SetMethodName(jsMethod->GetMethodName(), len);
                methodCountMap->emplace(jsMethod->GetMethodId(), info);
                methodCount_++;
            }
        } else {
            ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *methodsCountMap =
                chunk_.New<ChunkUnorderedMap<EntityId, MethodProfilerInfo *>>(&chunk_);
            size_t len = strlen(jsMethod->GetMethodName());
            void *infoAddr = chunk_.Allocate(MethodProfilerInfo::Size(len));
            auto info = new (infoAddr) MethodProfilerInfo(jsMethod->GetMethodId(), 1, mode, len);
            info->SetMethodName(jsMethod->GetMethodName(), len);
            methodsCountMap->emplace(jsMethod->GetMethodId(), info);
            profilerMap_.emplace(recordName, methodsCountMap);
            methodCount_++;
        }
        // Merged every 10 methods
        if (methodCount_ >= MERGED_EVERY_COUNT) {
            LOG_ECMA(INFO) << "Sample: post task to save profiler";
            PGOProfilerManager::GetInstance()->TerminateSaveTask();
            PGOProfilerManager::GetInstance()->Merge(this);
            PGOProfilerManager::GetInstance()->PostSaveTask();
            methodCount_ = 0;
        }
    }
}

void PGOProfilerManager::Initialize(uint32_t hotnessThreshold, const std::string &outDir)
{
    hotnessThreshold_ = hotnessThreshold;
    outDir_ = outDir;
}

void PGOProfilerManager::Destroy()
{
    if (!isInitialized_) {
        return;
    }
    // SaveTask is already finished
    SaveProfiler();
    globalProfilerMap_->clear();
    chunk_.reset();
    nativeAreaAllocator_.reset();
    isInitialized_ = false;
}

bool PGOProfilerManager::InitializeData()
{
    os::memory::LockHolder lock(mutex_);
    if (!isInitialized_) {
        if (!RealPath(outDir_, realOutPath_, false)) {
            return false;
        }
        realOutPath_ += PROFILE_FILE_NAME;
        LOG_ECMA(INFO) << "Save profiler to file:" << realOutPath_;

        nativeAreaAllocator_ = std::make_unique<NativeAreaAllocator>();
        chunk_ = std::make_unique<Chunk>(nativeAreaAllocator_.get());
        globalProfilerMap_ = chunk_->
            New<ChunkUnorderedMap<CString, ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *>>(chunk_.get());
        pandaFileProfilerInfos_ = chunk_->New<ChunkVector<PandaFileProfilerInfo *>>(chunk_.get());
        isInitialized_ = true;
    }
    return true;
}

void PGOProfilerManager::SamplePandaFileInfo(uint32_t checksum)
{
    if (!isInitialized_) {
        return;
    }
    for (auto info : *pandaFileProfilerInfos_) {
        if (info->GetChecksum() == checksum) {
            return;
        }
    }
    auto info = chunk_->New<PandaFileProfilerInfo>(checksum);
    pandaFileProfilerInfos_->emplace_back(info);
}

void PGOProfilerManager::Merge(PGOProfiler *profiler)
{
    if (!isInitialized_) {
        return;
    }

    os::memory::LockHolder lock(mutex_);
    for (auto iter = profiler->profilerMap_.begin(); iter != profiler->profilerMap_.end(); iter++) {
        auto recordName = iter->first;
        auto methodCountMap = iter->second;
        auto globalMethodCountIter = globalProfilerMap_->find(recordName);
        ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *globalMethodCountMap = nullptr;
        if (globalMethodCountIter == globalProfilerMap_->end()) {
            globalMethodCountMap = chunk_->New<ChunkUnorderedMap<EntityId, MethodProfilerInfo *>>(chunk_.get());
            globalProfilerMap_->emplace(recordName, globalMethodCountMap);
        } else {
            globalMethodCountMap = globalMethodCountIter->second;
        }
        for (auto countIter = methodCountMap->begin(); countIter != methodCountMap->end(); countIter++) {
            auto methodId = countIter->first;
            auto &localInfo = countIter->second;
            auto result = globalMethodCountMap->find(methodId);
            if (result != globalMethodCountMap->end()) {
                auto &info = result->second;
                info->Merge(localInfo);
            } else {
                size_t len = strlen(localInfo->GetMethodName());
                void *infoAddr = chunk_->Allocate(MethodProfilerInfo::Size(len));
                auto info = new (infoAddr) MethodProfilerInfo(methodId, localInfo->GetCount(),
                    localInfo->GetSampleMode(), len);
                info->SetMethodName(localInfo->GetMethodName(), len);
                globalMethodCountMap->emplace(methodId, info);
            }
            localInfo->ClearCount();
        }
    }
}

void PGOProfilerManager::SaveProfiler(SaveTask *task)
{
    std::ofstream fileStream(realOutPath_.c_str());
    if (!fileStream.is_open()) {
        LOG_ECMA(ERROR) << "The file path(" << realOutPath_ << ") open failure!";
        return;
    }
    ProcessProfileHeader(fileStream);
    ProcessPandaFileInfo(fileStream);
    ProcessProfile(fileStream, task);
    fileStream.close();
}

void PGOProfilerManager::ProcessProfileHeader(std::ofstream &fileStream)
{
    fileStream.write(reinterpret_cast<char *>(&header_), sizeof(PGOProfilerHeader));
}

void PGOProfilerManager::ProcessPandaFileInfo(std::ofstream &fileStream)
{
    uint32_t size = pandaFileProfilerInfos_->size();
    fileStream.write(reinterpret_cast<char *>(&size), sizeof(uint32_t));
    for (auto info : *pandaFileProfilerInfos_) {
        fileStream.write(reinterpret_cast<char *>(info), sizeof(PandaFileProfilerInfo));
    }
}

void PGOProfilerManager::ProcessProfile(std::ofstream &fileStream, SaveTask *task)
{
    uint32_t recordCount = 0;
    std::stringstream stream;

    for (auto iter = globalProfilerMap_->begin(); iter != globalProfilerMap_->end(); iter++) {
        uint32_t methodCount = 0;
        std::stringstream methodStream;
        auto methodCountMap = iter->second;
        for (auto countIter = methodCountMap->begin(); countIter != methodCountMap->end(); countIter++) {
            LOG_ECMA(DEBUG) << "Method:" << countIter->first << "/" << countIter->second->GetCount()
                            << "/" << std::to_string(static_cast<int>(countIter->second->GetSampleMode()))
                            << "/" << countIter->second->GetMethodName() << "/" << countIter->second->GetMethodLength();
            if (task && task->IsTerminate()) {
                LOG_ECMA(INFO) << "ProcessProfile: task is already terminate";
                return;
            }
            if (countIter->second->GetCount() < hotnessThreshold_ &&
                countIter->second->GetSampleMode() == SampleMode::CALL_MODE) {
                continue;
            }
            methodStream.write(reinterpret_cast<char *>(countIter->second), countIter->second->Size());
            methodCount++;
        }
        if (methodCount > 0) {
            stream << iter->first;
            stream << '\0';
            stream.write(reinterpret_cast<char *>(&methodCount), sizeof(uint32_t));
            stream << methodStream.rdbuf();
            recordCount++;
        }
    }
    fileStream.write(reinterpret_cast<char *>(&recordCount), sizeof(uint32_t));
    fileStream << stream.rdbuf();
}

void PGOProfilerManager::TerminateSaveTask()
{
    if (!isInitialized_) {
        return;
    }
    Taskpool::GetCurrentTaskpool()->TerminateTask(GLOBAL_TASK_ID, TaskType::PGO_SAVE_TASK);
}

void PGOProfilerManager::PostSaveTask()
{
    if (!isInitialized_) {
        return;
    }
    Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<SaveTask>(GLOBAL_TASK_ID));
}

void PGOProfilerManager::StartSaveTask(SaveTask *task)
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
} // namespace panda::ecmascript
