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

bool PGOProfilerManager::MergeApFiles(std::unordered_map<CString, uint32_t> &fileNameToChecksumMap,
                                      PGOProfilerDecoder &merger)
{
    uint32_t hotnessThreshold = merger.GetHotnessThreshold();
    std::string inFiles(merger.GetInPath());
    arg_list_t pgoFileNamesVector = base::StringHelper::SplitString(inFiles, GetFileDelimiter());
    if (pgoFileNamesVector.empty()) {
        return true;
    }
    merger.InitMergeData();
    bool hasMerged = false;
    std::string firstApFileName;
    for (const auto &fileName : pgoFileNamesVector) {
        PGOProfilerDecoder decoder(fileName, hotnessThreshold);
        if (!decoder.LoadAndVerify(fileNameToChecksumMap, merger.GetAbcFilePool())) {
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
    // In the scenario of on-device application compilation, the input fileNameToChecksumMap only contains ABC
    // information of the application, while AP holds all ABC information collected during the
    // collection phase. Considering external HSP dependencies, the input fileNameToChecksumMap
    // needs to be merged with the information from AP, that we can write full ABC information to the an file.
    merger.MergeFileNameToChecksumMap(fileNameToChecksumMap);
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
