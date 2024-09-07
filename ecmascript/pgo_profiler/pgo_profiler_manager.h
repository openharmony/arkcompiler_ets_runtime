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
#include "os/mutex.h"

namespace panda::ecmascript::pgo {
class PGOProfilerManager {
public:
    using ApGenMode = PGOProfilerEncoder::ApGenMode;
    static PGOProfilerManager *PUBLIC_API GetInstance();

    static void SavingSignalHandler(int signo);

    PGOProfilerManager() = default;
    ~PGOProfilerManager() = default;

    NO_COPY_SEMANTIC(PGOProfilerManager);
    NO_MOVE_SEMANTIC(PGOProfilerManager);

    void Initialize(const std::string &outDir, uint32_t hotnessThreshold)
    {
        // For FA jsvm, merge with existed output file
        encoder_ = std::make_unique<PGOProfilerEncoder>(outDir, hotnessThreshold, ApGenMode::MERGE);
    }

    void SetBundleName(const std::string &bundleName)
    {
        if (encoder_) {
            encoder_->SetBundleName(bundleName);
        }
    }

    const std::string GetBundleName()
    {
        if (encoder_) {
            return encoder_->GetBundleName();
        }
        return "";
    }

    void SetRequestAotCallback(const RequestAotCallback &cb)
    {
        os::memory::LockHolder lock(*mutex_);
        if (requestAotCallback_ != nullptr) {
            return;
        }
        requestAotCallback_ = cb;
    }

    bool RequestAot(const std::string &bundleName, const std::string &moduleName, RequestAotMode triggerMode)
    {
        RequestAotCallback cb;
        {
            os::memory::LockHolder lock(*mutex_);
            if (requestAotCallback_ == nullptr) {
                LOG_ECMA(ERROR) << "Trigger aot failed. callback is null.";
                return false;
            }
            cb = requestAotCallback_;
        }
        return (cb(bundleName, moduleName, static_cast<int32_t>(triggerMode)) == 0);
    }

    void Destroy()
    {
        if (encoder_) {
            encoder_->Save();
            encoder_->Destroy();
            encoder_.reset();
        }
    }

    // Factory
    std::shared_ptr<PGOProfiler> Build(EcmaVM *vm, bool isEnable)
    {
        if (isEnable) {
            isEnable = InitializeData();
        }
        auto profiler = std::make_shared<PGOProfiler>(vm, isEnable);
        {
            os::memory::LockHolder lock(*mutex_);
            profilers_.insert(profiler);
        }
        return profiler;
    }

    // Return false if force disabled or never initialized
    bool IsEnable() const
    {
        return !disablePGO_ && encoder_ && encoder_->IsInitialized();
    }

    void Destroy(std::shared_ptr<PGOProfiler> &profiler)
    {
        if (profiler != nullptr) {
            profiler->WaitPGODumpFinish();
            profiler->HandlePGOPreDump();
            Merge(profiler.get());
            {
                os::memory::LockHolder lock(*mutex_);
                profilers_.erase(profiler);
            }
            profiler.reset();
        }
    }

    void Reset(const std::shared_ptr<PGOProfiler>& profiler, bool isEnable)
    {
        if (isEnable) {
            isEnable = InitializeData();
        }
        if (profiler) {
            profiler->Reset(isEnable);
        }
    }

    void SamplePandaFileInfo(uint32_t checksum, const CString &abcName)
    {
        if (encoder_) {
            encoder_->SamplePandaFileInfo(checksum, abcName);
        }
    }

    void SetModuleName(const std::string &moduleName)
    {
        if (encoder_) {
            encoder_->PostResetOutPathTask(moduleName);
        }
    }

    bool GetPandaFileId(const CString &abcName, ApEntityId &entryId) const
    {
        if (encoder_) {
            return encoder_->GetPandaFileId(abcName, entryId);
        }
        return false;
    }

    bool GetPandaFileDesc(ApEntityId abcId, CString &desc) const
    {
        if (encoder_) {
            return encoder_->GetPandaFileDesc(abcId, desc);
        }
        return false;
    }

    void SetApGenMode(ApGenMode mode)
    {
        if (encoder_) {
            encoder_->SetApGenMode(mode);
        }
    }

    void Merge(PGOProfiler *profiler)
    {
        if (encoder_ && profiler->isEnable_) {
            encoder_->TerminateSaveTask();
            encoder_->Merge(*profiler->recordInfos_);
        }
    }

    void RegisterSavingSignal();

    void AsyncSave()
    {
        if (encoder_) {
            encoder_->PostSaveTask();
        }
    }

    bool IsDisableAot() const
    {
        return disableAot_;
    }

    void SetDisableAot(bool state)
    {
        disableAot_ = state;
    }

    // Only set flag to ensure future actions will not trigger PGO path
    // Caller should handle existing threads and PGO data properly
    void SetDisablePGO(bool state)
    {
        disablePGO_ = state;
    }

    void ForceSave()
    {
        os::memory::LockHolder lock(*mutex_);
        for (const auto &profiler : profilers_) {
            profiler->DumpByForce();
        }
        GetInstance()->AsyncSave();
    }

    bool PUBLIC_API TextToBinary(const std::string &inPath, const std::string &outPath, uint32_t hotnessThreshold,
                                 ApGenMode mode)
    {
        PGOProfilerEncoder encoder(outPath, hotnessThreshold, mode);
        PGOProfilerEncoder decoder(outPath, hotnessThreshold, mode);
        if (!encoder.InitializeData()) {
            LOG_ECMA(ERROR) << "PGO Profiler encoder initialized failed";
            return false;
        }
        if (!decoder.InitializeData()) {
            LOG_ECMA(ERROR) << "PGO Profiler decoder initialized failed";
            return false;
        }
        bool ret = decoder.LoadAPTextFile(inPath);
        if (ret) {
            encoder.Merge(decoder);
            ret = encoder.Save();
        }
        encoder.Destroy();
        decoder.Destroy();
        return ret;
    }

    bool PUBLIC_API BinaryToText(const std::string &inPath, const std::string &outPath, uint32_t hotnessThreshold)
    {
        PGOProfilerDecoder decoder(inPath, hotnessThreshold);
        if (!decoder.LoadFull()) {
            return false;
        }
        bool ret = decoder.SaveAPTextFile(outPath);
        decoder.Clear();
        return ret;
    }

    static bool PUBLIC_API MergeApFiles(const std::string &inFiles, const std::string &outPath,
                                        uint32_t hotnessThreshold, ApGenMode mode);
    static bool PUBLIC_API MergeApFiles(uint32_t checksum, PGOProfilerDecoder &merger);

    void SetIsApFileCompatible(bool isCompatible)
    {
        isApFileCompatible_ = isCompatible;
    }

    bool GetIsApFileCompatible() const
    {
        return isApFileCompatible_;
    }

    size_t GetMaxAotMethodSize() const
    {
        return maxAotMethodSize_;
    }

    void SetMaxAotMethodSize(uint32_t value)
    {
        maxAotMethodSize_ = value;
    }

    bool IsBigMethod(uint32_t methodSize) const
    {
        return maxAotMethodSize_ != 0 && methodSize > maxAotMethodSize_;
    }

    bool IsEnableForceIC() const
    {
        return isEnableForceIC_;
    }

    void SetEnableForceIC(bool isEnableForceIC)
    {
        isEnableForceIC_ = isEnableForceIC;
    }

private:
    bool InitializeData()
    {
        if (!encoder_) {
            return false;
        }
        bool initializedResult = encoder_->InitializeData();
        if (initializedResult && !enableSignalSaving_) {
            RegisterSavingSignal();
        }
        return initializedResult;
    }

    bool disableAot_ {false};
    bool disablePGO_ {false};
    std::unique_ptr<PGOProfilerEncoder> encoder_;
    RequestAotCallback requestAotCallback_;
    std::atomic_bool enableSignalSaving_ { false };
    os::memory::Mutex *mutex_ = new os::memory::Mutex();
    std::set<std::shared_ptr<PGOProfiler>> profilers_;
    bool isApFileCompatible_ {true};
    bool isEnableForceIC_ {true};
    uint32_t maxAotMethodSize_ {0};
};
} // namespace panda::ecmascript::pgo
#endif  // ECMASCRIPT_PGO_PROFILER_MANAGER_H
