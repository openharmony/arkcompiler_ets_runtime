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
#include <memory>

#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
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
    MethodProfilerInfo(uint32_t count, const std::string &methodName, SampleMode mode)
        : count_(count), methodName_(methodName), mode_(mode) {}

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
        SetSampleMode(info->GetSampleMode());
    }

    uint32_t GetCount() const
    {
        return count_;
    }

    const std::string &GetMethodName() const
    {
        return methodName_;
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
    uint32_t count_ {0};
    std::string methodName_;
    SampleMode mode_ {SampleMode::CALL_MODE};
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
    void InitializeData();

    void Destroy();

    // Factory
    PGOProfiler *Build(EcmaVM *vm, bool isEnable)
    {
        if (isEnable) {
            InitializeData();
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

    void Merge(PGOProfiler *profile);
    void TerminateSaveTask();
    void PostSaveTask();

private:
    class SaveTask : public Task {
    public:
        explicit SaveTask(int32_t id) : Task(id) {};
        virtual ~SaveTask() override = default;

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
    void ProcessProfile(std::ofstream &fileStream, SaveTask *task);

    bool isEnable_ {false};
    uint32_t hotnessThreshold_ {2};
    std::string outDir_;
    std::unique_ptr<NativeAreaAllocator> nativeAreaAllocator_;
    std::unique_ptr<Chunk> chunk_;
    ChunkUnorderedMap<CString, ChunkUnorderedMap<EntityId, MethodProfilerInfo *> *> *globalProfilerMap_;
    os::memory::Mutex mutex_;
};

} // namespace panda::ecmascript
#endif  // ECMASCRIPT_DFX_PGO_PROFILER_MANAGER_H
