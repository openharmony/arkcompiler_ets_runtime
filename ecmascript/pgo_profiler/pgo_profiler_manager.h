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

#include <memory>

#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/pgo_profiler/pgo_profiler_loader.h"
#include "ecmascript/pgo_profiler/pgo_profiler_saver.h"

namespace panda::ecmascript {
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

    void Initialize(const std::string &outDir, uint32_t hotnessThreshold)
    {
        saver_ = std::make_unique<PGOProfilerSaver>(outDir, hotnessThreshold);
    }

    void Destroy()
    {
        if (saver_) {
            saver_->Save();
            saver_->Destroy();
            saver_.reset();
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
        return saver_ && saver_->IsInitialized();
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
        if (saver_) {
            saver_->SamplePandaFileInfo(checksum);
        }
    }

    void Merge(PGOProfiler *profiler)
    {
        if (saver_ && profiler->isEnable_) {
            saver_->TerminateSaveTask();
            saver_->Merge(*profiler->recordInfos_);
        }
    }

    void AsynSave()
    {
        if (saver_) {
            saver_->PostSaveTask();
        }
    }

    bool PUBLIC_API TextToBinary(const std::string &inPath, const std::string &outPath, uint32_t hotnessThreshold)
    {
        PGOProfilerSaver saver(outPath, hotnessThreshold);
        if (!saver.InitializeData()) {
            LOG_ECMA(ERROR) << "PGO Profiler saver initialized failed";
            return false;
        }
        bool ret = saver.LoadAPTextFile(inPath);
        if (ret) {
            saver.Save();
        }
        saver.Destroy();
        return ret;
    }

    bool PUBLIC_API BinaryToText(const std::string &inPath, const std::string &outPath, uint32_t hotnessThreshold)
    {
        PGOProfilerLoader loader(inPath, hotnessThreshold);
        if (!loader.LoadFull()) {
            return false;
        }
        bool ret = loader.SaveAPTextFile(outPath);
        loader.Clear();
        return ret;
    }

private:
    bool InitializeData()
    {
        if (!saver_) {
            return false;
        }
        return saver_->InitializeData();
    }

    std::unique_ptr<PGOProfilerSaver> saver_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_PGO_PROFILER_MANAGER_H
