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

#ifndef ECMASCRIPT_PGO_PROFILER_ENCODER_H
#define ECMASCRIPT_PGO_PROFILER_ENCODER_H

#include <atomic>
#include <memory>
#include <utility>

#include "ecmascript/pgo_profiler/pgo_profiler_info.h"
#include "macros.h"

namespace panda::ecmascript::pgo {
class PGOProfilerDecoder;
class PGOProfilerEncoder {
public:
    enum ApGenMode { OVERWRITE, MERGE };

    PGOProfilerEncoder(const std::string &outDir, uint32_t hotnessThreshold, ApGenMode mode)
        : outDir_(outDir), hotnessThreshold_(hotnessThreshold), mode_(mode)
    {
        pandaFileInfos_ = std::make_unique<PGOPandaFileInfos>();
        abcFilePool_ = std::make_shared<PGOAbcFilePool>();
    }

    ~PGOProfilerEncoder()
    {
        Destroy();
    }

    NO_COPY_SEMANTIC(PGOProfilerEncoder);
    NO_MOVE_SEMANTIC(PGOProfilerEncoder);

    static void AddChecksum(std::fstream& fileStream);

    bool PUBLIC_API InitializeData();

    void PUBLIC_API Destroy();

    void SetBundleName(const std::string &bundleName)
    {
        bundleName_ = bundleName;
    }

    const std::string GetBundleName()
    {
        return bundleName_;
    }

    bool IsInitialized() const
    {
        return isProfilingInitialized_;
    }

    void SamplePandaFileInfo(uint32_t checksum, const CString &abcName);
    bool PUBLIC_API GetPandaFileId(const CString &abcName, ApEntityId &entryId);
    bool GetPandaFileDesc(ApEntityId abcId, CString &desc);
    void Merge(const PGORecordDetailInfos &recordInfos);
    void Merge(const PGOPandaFileInfos &pandaFileInfos);
    void Merge(const PGOProfilerEncoder &encoder);
    bool VerifyPandaFileMatched(const PGOPandaFileInfos &pandaFileInfos, const std::string &base,
                                const std::string &incoming) const;
    std::shared_ptr<PGOAbcFilePool> GetAbcFilePool() const
    {
        return abcFilePool_;
    }
    void TerminateSaveTask();
    void PostSaveTask();
    void SetApGenMode(ApGenMode mode)
    {
        mode_ = mode;
    }

    bool PUBLIC_API Save();

    bool PUBLIC_API LoadAPTextFile(const std::string &inPath);

    void PostResetOutPathTask(const std::string &moduleName);

    bool ResetOutPathByModuleName(const std::string &moduleName);

protected:
    PGOProfilerHeader *header_ {nullptr};

private:
    void StartSaveTask(const SaveTask *task);
    bool InternalSave(const SaveTask *task = nullptr);
    bool SaveAndRename(const SaveTask *task = nullptr);
    void MergeWithExistProfile(PGOProfilerEncoder &runtimeEncoder, PGOProfilerDecoder &decoder,
                               const SaveTask *task = nullptr);
    void RequestAot();
    bool ResetOutPath(const std::string& profileFileName);

    bool isProfilingInitialized_ {false};
    std::string outDir_;
    uint32_t hotnessThreshold_ {2};
    std::string realOutPath_;
    std::unique_ptr<PGOPandaFileInfos> pandaFileInfos_;
    std::shared_ptr<PGOAbcFilePool> abcFilePool_;
    std::shared_ptr<PGORecordDetailInfos> globalRecordInfos_;
    // rwLock_ is used to protect the pandaFileInfos_ and abcFilePool_
    RWLock rwLock_;
    // mutex_ is used to protect the others
    Mutex mutex_;
    std::atomic_bool hasPostModuleName_ {false};
    std::string moduleName_;
    std::string bundleName_;
    ApGenMode mode_ {OVERWRITE};
    friend SaveTask;
};

class SaveTask : public Task {
public:
    explicit SaveTask(PGOProfilerEncoder *encoder, int32_t id) : Task(id), encoder_(encoder) {};
    virtual ~SaveTask() override = default;

    bool Run([[maybe_unused]] uint32_t threadIndex) override
    {
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SaveTask::Run");
        encoder_->StartSaveTask(this);
        return true;
    }

    TaskType GetTaskType() const override
    {
        return TaskType::PGO_SAVE_TASK;
    }

    NO_COPY_SEMANTIC(SaveTask);
    NO_MOVE_SEMANTIC(SaveTask);
private:
    PGOProfilerEncoder *encoder_;
};

class ResetOutPathTask : public Task {
public:
    ResetOutPathTask(PGOProfilerEncoder *encoder, std::string moduleName, int32_t id)
        : Task(id), encoder_(encoder), moduleName_(std::move(moduleName)) {};
    virtual ~ResetOutPathTask() override = default;

    bool Run([[maybe_unused]] uint32_t threadIndex) override
    {
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "ResetOutPathTask::Run");
        encoder_->ResetOutPathByModuleName(moduleName_);
        return true;
    }

    TaskType GetTaskType() const override
    {
        return TaskType::PGO_RESET_OUT_PATH_TASK;
    }

    NO_COPY_SEMANTIC(ResetOutPathTask);
    NO_MOVE_SEMANTIC(ResetOutPathTask);

private:
    PGOProfilerEncoder *encoder_;
    std::string moduleName_;
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_ENCODER_H
