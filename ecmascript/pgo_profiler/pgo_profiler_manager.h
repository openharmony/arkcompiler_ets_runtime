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

namespace panda::ecmascript {
class PGOProfilerManager {
public:
    using ApGenMode = PGOProfilerEncoder::ApGenMode;
    static PGOProfilerManager *GetInstance()
    {
        static PGOProfilerManager instance;
        return &instance;
    }

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

    void Destroy()
    {
        if (encoder_) {
            encoder_->Save();
            encoder_->Destroy();
            encoder_.reset();
        }
    }

    // Factory
    PGOProfiler *Build(EcmaVM *vm, bool isEnable)
    {
        if (isEnable) {
            isEnable = InitializeData();
        }
        return new PGOProfiler(vm, isEnable);
    }

    bool IsEnable() const
    {
        return encoder_ && encoder_->IsInitialized();
    }

    void Destroy(PGOProfiler *profiler)
    {
        if (profiler != nullptr) {
            Merge(profiler);
            delete profiler;
        }
    }

    void Reset(PGOProfiler *profiler, bool isEnable)
    {
        if (isEnable) {
            isEnable = InitializeData();
        }
        if (profiler) {
            profiler->Reset(isEnable);
        }
    }

    void SamplePandaFileInfo(uint32_t checksum)
    {
        if (encoder_) {
            encoder_->SamplePandaFileInfo(checksum);
        }
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

    void AsynSave()
    {
        if (encoder_) {
            encoder_->PostSaveTask();
        }
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

    static bool MergeApFiles(const std::string &inFiles, const std::string &outPath, uint32_t hotnessThreshold,
                             ApGenMode mode);
    static bool MergeApFiles(uint32_t checksum, PGOProfilerDecoder &merger);

private:
    bool InitializeData()
    {
        if (!encoder_) {
            return false;
        }
        if (!enableSignalSaving_) {
            RegisterSavingSignal();
        }
        return encoder_->InitializeData();
    }

    std::unique_ptr<PGOProfilerEncoder> encoder_;
    std::atomic_bool enableSignalSaving_ { false };
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PGO_PROFILER_MANAGER_H
