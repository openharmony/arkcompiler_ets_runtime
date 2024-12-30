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

#ifndef ECMASCRIPT_PGO_PROFILER_MANAGER_H
#define ECMASCRIPT_PGO_PROFILER_MANAGER_H

#include <atomic>
#include <csignal>
#include <memory>

#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"

namespace panda::ecmascript::pgo {
using ApGenMode = PGOProfilerEncoder::ApGenMode;

class PGOProfilerManager {
public:
    PGOProfilerManager() = default;
    ~PGOProfilerManager() = default;
    NO_COPY_SEMANTIC(PGOProfilerManager);
    NO_MOVE_SEMANTIC(PGOProfilerManager);

    static PGOProfilerManager* PUBLIC_API GetInstance();
    static void SavingSignalHandler(int signo);
    void Initialize(const std::string& outDir, uint32_t hotnessThreshold);
    void SetBundleName(const std::string& bundleName);
    const std::string GetBundleName() const;
    void SetRequestAotCallback(const RequestAotCallback& cb);
    bool RequestAot(const std::string& bundleName, const std::string& moduleName, RequestAotMode triggerMode);
    void Destroy();
    std::shared_ptr<PGOProfiler> BuildProfiler(EcmaVM* vm, bool isEnable);
    bool IsEnable() const;
    void Destroy(std::shared_ptr<PGOProfiler>& profiler);
    void Reset(const std::shared_ptr<PGOProfiler>& profiler, bool isEnable);
    void SamplePandaFileInfo(uint32_t checksum, const CString& abcName);
    void SetModuleName(const std::string& moduleName);
    bool PUBLIC_API GetPandaFileId(const CString& abcName, ApEntityId& entryId) const;
    bool GetPandaFileDesc(ApEntityId abcId, CString& desc) const;
    void SetApGenMode(ApGenMode mode);
    ApGenMode GetApGenMode() const;
    void Merge(PGOProfiler* profiler);
    void RegisterSavingSignal();
    void AsyncSave();
    bool IsDisableAot() const;
    void SetDisableAot(bool state);
    void SetDisablePGO(bool state);
    void ForceDump();
    bool PUBLIC_API TextToBinary(const std::string& inPath,
                                 const std::string& outPath,
                                 uint32_t hotnessThreshold,
                                 ApGenMode mode);
    bool PUBLIC_API BinaryToText(const std::string& inPath, const std::string& outPath, uint32_t hotnessThreshold);
    static bool PUBLIC_API MergeApFiles(const std::string &inFiles, const std::string &outPath,
                                        uint32_t hotnessThreshold, ApGenMode mode);
    static bool PUBLIC_API MergeApFiles(std::unordered_map<CString, uint32_t> &fileNameToChecksumMap,
                                        PGOProfilerDecoder &merger);
    void SetIsApFileCompatible(bool isCompatible);
    bool GetIsApFileCompatible() const;
    size_t GetMaxAotMethodSize() const;
    void SetMaxAotMethodSize(uint32_t value);
    bool IsBigMethod(uint32_t methodSize) const;
    std::shared_ptr<PGOState> GetPGOState() const;
    std::shared_ptr<PGOInfo> GetPGOInfo() const;
    bool ResetOutPathByModuleName(const std::string& moduleName);
    bool ResetOutPath(const std::string& fileName);
    static bool ResetOutPath(const std::string& path, std::string& realPath, const std::string& fileName);
    const std::string& GetOutPath() const;
    void RequestAot();
    void PostResetOutPathTask(const std::string& moduleName);
    bool IsInitialized() const;
    static Mutex& GetPGOInfoMutex();
    ConcurrentGuardValue& GetConcurrentGuardValue();

private:
    bool InitializeData();

    ConcurrentGuardValue v_;
    bool disableAot_ {false};
    bool disablePGO_ {false};
    Mutex requestAotCallbackMutex_;
    RequestAotCallback requestAotCallback_;
    std::atomic_bool enableSignalSaving_ { false };
    Mutex profilersMutex_;
    std::set<std::shared_ptr<PGOProfiler>> profilers_;
    bool isApFileCompatible_ {true};
    uint32_t maxAotMethodSize_ {0};
    std::shared_ptr<PGOState> state_;
    std::shared_ptr<PGOInfo> pgoInfo_;
    std::string outDir_;
    std::string outPath_;
    std::string bundleName_;
    std::string moduleName_;
    uint32_t hotnessThreshold_;
    std::atomic_bool isInitialized_ {false};
    std::atomic_bool hasPostModuleName_ {false};
    Mutex resetOutPathMutex_;
    ApGenMode apGenMode_ {ApGenMode::MERGE};
};

class ResetOutPathTask : public Task {
public:
    ResetOutPathTask(std::string moduleName, int32_t id): Task(id), moduleName_(std::move(moduleName)) {};
    virtual ~ResetOutPathTask() override = default;

    bool Run([[maybe_unused]] uint32_t threadIndex) override
    {
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "ResetOutPathTask::Run");
        PGOProfilerManager::GetInstance()->ResetOutPathByModuleName(moduleName_);
        return true;
    }

    TaskType GetTaskType() const override
    {
        return TaskType::PGO_RESET_OUT_PATH_TASK;
    }

    NO_COPY_SEMANTIC(ResetOutPathTask);
    NO_MOVE_SEMANTIC(ResetOutPathTask);

private:
    std::string moduleName_;
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_MANAGER_H
