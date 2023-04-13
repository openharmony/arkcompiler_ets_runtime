/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_DFX_PGO_PROFILER_MANAGER_H
#define ECMASCRIPT_DFX_PGO_PROFILER_MANAGER_H

#include <algorithm>
#include <cstring>
#include <memory>

#include "ecmascript/base/file_header.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/chunk_containers.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/taskpool/task.h"

namespace panda::ecmascript {
enum class SampleMode : uint8_t {
    HOTNESS_MODE,
    CALL_MODE,
};

/*
 * Support statistics of JS Function call heat. Save the method ID whose calls are less than MIN_COUNT.
 *
 * The saving format is as follows:
 * "recordName1:[methodId/count/mode/name,methodId/count/mode/name......]"
 * "recordName2:[methodId/count/mode/name,methodId/count/mode/name,methodId/count/mode/name......]"
 *                                     "......"
 * */
class MethodProfilerInfo {
public:
    static constexpr size_t ALIGN_SIZE = 4;

    MethodProfilerInfo(EntityId id, uint32_t count, SampleMode mode, uint16_t length)
        : id_(id), count_(count), mode_(mode), methodLength_(length) {}

    static int32_t Size(uint32_t length)
    {
        return sizeof(MethodProfilerInfo) + AlignUp(length, ALIGN_SIZE);
    }

    int32_t Size()
    {
        return sizeof(MethodProfilerInfo) + AlignUp(methodLength_, ALIGN_SIZE);
    }

    void IncreaseCount()
    {
        count_++;
    }

    void ClearCount()
    {
        count_ = 0;
    }

    void Merge(const MethodProfilerInfo *info)
    {
        count_ += info->GetCount();
        methodLength_ = info->GetMethodLength();
        SetSampleMode(info->GetSampleMode());
        SetMethodName(info->GetMethodName(), info->GetMethodLength());
    }

    EntityId GetMethodId() const
    {
        return id_;
    }

    uint32_t GetCount() const
    {
        return count_;
    }

    uint16_t GetMethodLength() const
    {
        return methodLength_;
    }

    void SetMethodName(const char *methodName, size_t len)
    {
        if (memcpy_s(&methodName_, methodLength_, methodName, len) != EOK) {
            LOG_ECMA(ERROR) << "SetMethodName memcpy_s failed" << methodName << ", len = " << len;
        }
        *(&methodName_ + len) = '\0';
    }

    const char *GetMethodName() const
    {
        return &methodName_;
    }

    void SetSampleMode(SampleMode mode)
    {
        if (mode_ == SampleMode::HOTNESS_MODE) {
            return;
        }
        mode_ = mode;
    }

    SampleMode GetSampleMode() const
    {
        return mode_;
    }

    NO_COPY_SEMANTIC(MethodProfilerInfo);
    NO_MOVE_SEMANTIC(MethodProfilerInfo);

private:
    EntityId id_;
    uint32_t count_ {0};
    SampleMode mode_ {SampleMode::CALL_MODE};
    uint16_t methodLength_ {0};
    char methodName_ {'\0'};
};

class PGOProfilerHeader : public base::FileHeader {
public:
    static constexpr VersionType LAST_VERSION = {0, 0, 0, 1};

    PGOProfilerHeader() : base::FileHeader(LAST_VERSION) {}

    bool Verify()
    {
        return VerifyInner(LAST_VERSION);
    }
};

class PandaFileProfilerInfo {
public:
    PandaFileProfilerInfo() = default;
    PandaFileProfilerInfo(uint32_t checksum) : checksum_(checksum) {}

    uint32_t GetChecksum()
    {
        return checksum_;
    }

private:
    uint32_t checksum_;
};

class PGOProfiler {
public:
    NO_COPY_SEMANTIC(PGOProfiler);
    NO_MOVE_SEMANTIC(PGOProfiler);

    void Sample(JSTaggedType value, SampleMode mode = SampleMode::CALL_MODE);
private:
    PGOProfiler(EcmaVM *vm, bool isEnable)
        : isEnable_(isEnable), chunk_(vm->GetNativeAreaAllocator()), profilerMap_(&chunk_) {};
    virtual ~PGOProfiler();
    void Reset(bool isEnable)
    {
        isEnable_ = isEnable;
        profilerMap_.clear();
        methodCount_ = 0;
    }

    static constexpr uint32_t MERGED_EVERY_COUNT = 10;
    bool isEnable_ {false};
    Chunk chunk_;
    ChunkUnorderedMap<CString, ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *> profilerMap_;
    uint32_t methodCount_ {0};
    friend class PGOProfilerManager;
};

class PGOProfilerManager {
public:
    static PGOProfilerManager *GetInstance()
    {
        static PGOProfilerManager instance;
        return &instance;
    }

    PGOProfilerManager() = default;
    ~PGOProfilerManager() = default;

    NO_COPY_SEMANTIC(PGOProfilerManager);
    NO_MOVE_SEMANTIC(PGOProfilerManager);

    void Initialize(uint32_t hotnessThreshold, const std::string &outDir);
    bool InitializeData();

    void Destroy();

    // Factory
    PGOProfiler *Build(EcmaVM *vm, bool isEnable)
    {
        if (isEnable) {
            isEnable = InitializeData();
        }
        return new PGOProfiler(vm, isEnable);
    }

    void Reset(PGOProfiler *profiler, bool isEnable)
    {
        if (profiler) {
            profiler->Reset(isEnable);
        }
        if (isEnable) {
            InitializeData();
        }
    }

    void Destroy(PGOProfiler *profiler)
    {
        if (profiler != nullptr) {
            Merge(profiler);
            delete profiler;
        }
    }

    void SamplePandaFileInfo(uint32_t checksum);
    void Merge(PGOProfiler *profile);
    void TerminateSaveTask();
    void PostSaveTask();

private:
    class SaveTask : public Task {
    public:
        SaveTask(int32_t id) : Task(id) {};
        virtual ~SaveTask() = default;

        bool Run([[maybe_unused]] uint32_t threadIndex) override
        {
            PGOProfilerManager::GetInstance()->StartSaveTask(this);
            return true;
        }

        TaskType GetTaskType() override
        {
            return TaskType::PGO_SAVE_TASK;
        }

        NO_COPY_SEMANTIC(SaveTask);
        NO_MOVE_SEMANTIC(SaveTask);
    };

    void StartSaveTask(SaveTask *task);
    void SaveProfiler(SaveTask *task = nullptr);
    void ProcessProfileHeader(std::ofstream &fileStream);
    void ProcessPandaFileInfo(std::ofstream &fileStream);
    void ProcessProfile(std::ofstream &fileStream, SaveTask *task);

    bool isInitialized_ {false};
    uint32_t hotnessThreshold_ {2};
    std::string outDir_;
    std::string realOutPath_;
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<Chunk> chunk_;
    PGOProfilerHeader header_;
    ChunkUnorderedMap<CString, ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *> *globalProfilerMap_;
    ChunkVector<PandaFileProfilerInfo *> *pandaFileProfilerInfos_;
    os::memory::Mutex mutex_;
};

} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_PGO_PROFILER_MANAGER_H
