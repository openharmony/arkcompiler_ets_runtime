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

#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include <fstream>

#include "ecmascript/platform/file.h"
namespace panda::ecmascript::pgo {
namespace {
    constexpr int32_t PGO_SAVING_SIGNAL = 50;
} // namespace

PGOProfilerManager* PGOProfilerManager::GetInstance()
{
    static PGOProfilerManager* instance = new PGOProfilerManager();
    return instance;
}

bool PGOProfilerManager::MergeApFiles(const std::string &inFiles, const std::string &outPath, uint32_t hotnessThreshold,
                                      ApGenMode mode)
{
    arg_list_t apFileNames = base::StringHelper::SplitString(inFiles, GetFileDelimiter());
    PGOProfilerEncoder merger(outPath, hotnessThreshold, mode);
    if (!merger.InitializeData()) {
        LOG_ECMA(ERROR) << "PGO Profiler encoder initialized failed. outPath: " << outPath
                        << " ,hotnessThreshold: " << hotnessThreshold;
        return false;
    }
    bool hasMerged = false;
    std::string firstApFileName;
    for (const auto &fileName : apFileNames) {
        if (!base::StringHelper::EndsWith(fileName, ".ap")) {
            LOG_ECMA(ERROR) << "The file path (" << fileName << ") does not end with .ap";
            continue;
        }
        PGOProfilerDecoder decoder(fileName, hotnessThreshold);
        if (!decoder.LoadFull(merger.GetAbcFilePool())) {
            LOG_ECMA(ERROR) << "Fail to load file path (" << fileName << "), skip it.";
            continue;
        }
        if (!hasMerged) {
            firstApFileName = fileName;
        } else {
            if (!merger.VerifyPandaFileMatched(decoder.GetPandaFileInfos(), firstApFileName, fileName)) {
                continue;
            }
        }
        if (!decoder.IsCompatibleWithAOTFile()) {
            LOG_ECMA(ERROR) << "The ap file (" << fileName << ") is not compatible with AOT version. skip it";
            continue;
        }
        merger.Merge(decoder.GetRecordDetailInfos());
        merger.Merge(decoder.GetPandaFileInfos());
        hasMerged = true;
    }
    if (!hasMerged) {
        LOG_ECMA(ERROR)
            << "No ap file pass verify, no ap file compatible an version, no ap file processed. Input files: "
            << inFiles;
        GetInstance()->SetIsApFileCompatible(false);
        return false;
    }
    GetInstance()->SetIsApFileCompatible(true);
    merger.Save();
    return true;
}

bool PGOProfilerManager::MergeApFiles(uint32_t checksum, PGOProfilerDecoder &merger)
{
    uint32_t hotnessThreshold = merger.GetHotnessThreshold();
    std::string inFiles(merger.GetInPath());
    arg_list_t pandaFileNames = base::StringHelper::SplitString(inFiles, GetFileDelimiter());
    if (pandaFileNames.empty()) {
        return true;
    }
    merger.InitMergeData();
    bool hasMerged = false;
    std::string firstApFileName;
    for (const auto &fileName : pandaFileNames) {
        PGOProfilerDecoder decoder(fileName, hotnessThreshold);
        if (!decoder.LoadAndVerify(checksum, merger.GetAbcFilePool())) {
            LOG_ECMA(ERROR) << "Load and verify file (" << fileName << ") failed, skip it.";
            continue;
        }
        if (!hasMerged) {
            firstApFileName = fileName;
        } else {
            if (!merger.GetPandaFileInfos().VerifyChecksum(decoder.GetPandaFileInfos(), firstApFileName, fileName)) {
                continue;
            }
        }
        if (!decoder.IsCompatibleWithAOTFile()) {
            LOG_ECMA(ERROR) << "The ap file (" << fileName << ") is not compatible with AOT version. skip it";
            continue;
        }
        merger.Merge(decoder);
        hasMerged = true;
    }
    if (!hasMerged) {
        LOG_ECMA(ERROR)
            << "No ap file pass verify, no ap file compatible an version, no ap file processed. Input files: "
            << inFiles;
        GetInstance()->SetIsApFileCompatible(false);
        return false;
    }
    GetInstance()->SetIsApFileCompatible(true);
    return true;
}

void PGOProfilerManager::RegisterSavingSignal()
{
    LOG_ECMA(INFO) << "Register Pgo Saving Signal";
    if (encoder_ == nullptr) {
        LOG_ECMA(ERROR) << "Can not register pgo saving signal, because encoder is null.";
        return;
    }
    if (!encoder_->IsInitialized()) {
        LOG_ECMA(DEBUG) << "Can not register pgo saving signal, because encoder is initialized.";
        return;
    }
    signal(PGO_SAVING_SIGNAL, SavingSignalHandler);
    enableSignalSaving_ = true;
}

void PGOProfilerManager::SavingSignalHandler(int signo)
{
    if (signo != PGO_SAVING_SIGNAL) {
        return;
    }

    PGOProfilerManager::GetInstance()->ForceSave();
}
} // namespace panda::ecmascript::pgo
