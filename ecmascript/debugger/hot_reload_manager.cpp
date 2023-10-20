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

#include "ecmascript/debugger/hot_reload_manager.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript::tooling {

void HotReloadManager::NotifyPatchLoaded(const JSPandaFile *baseFile, const JSPandaFile *patchFile)
{
    if (vm_->GetJsDebuggerManager()->GetDebuggerHandler() == nullptr) {
        return;
    }
    LOG_DEBUGGER(DEBUG) << "HotReloadManager::NotifyPatchLoaded";
    baseJSPandaFiles_[patchFile] = baseFile;

    ExtractPatch(patchFile);
    vm_->GetJsDebuggerManager()->ClearSingleStepper();
}

void HotReloadManager::NotifyPatchUnloaded(const JSPandaFile *patchFile)
{
    if (vm_->GetJsDebuggerManager()->GetDebuggerHandler() == nullptr) {
        return;
    }
    LOG_DEBUGGER(DEBUG) << "HotReloadManager::NotifyPatchUnloaded";
    baseJSPandaFiles_.erase(patchFile);
    patchExtractors_.clear();
    vm_->GetJsDebuggerManager()->ClearSingleStepper();
}

const JSPandaFile *HotReloadManager::GetBaseJSPandaFile(const JSPandaFile *jsPandaFile) const
{
    auto iter = baseJSPandaFiles_.find(jsPandaFile);
    if (iter == baseJSPandaFiles_.end()) {
        // if the file is non-patch, the base file is itself
        return jsPandaFile;
    }
    return iter->second;
}

std::vector<DebugInfoExtractor *> HotReloadManager::GetPatchExtractors(const std::string &url) const
{
    std::vector<DebugInfoExtractor *> extractors;
    auto iter = patchExtractors_.find(url);
    if (iter == patchExtractors_.end()) {
        return extractors;
    }
    return iter->second;
}

void HotReloadManager::ExtractPatch(const JSPandaFile *jsPandaFile)
{
    auto *patchExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    if (patchExtractor == nullptr) {
        LOG_DEBUGGER(ERROR) << "ExtractPatch: patch extractor is nullptr";
        return;
    }

    const auto &recordInfos = jsPandaFile->GetJSRecordInfo();
    for (const auto &[recordName, _] : recordInfos) {
        auto mainMethodIndex = panda_file::File::EntityId(jsPandaFile->GetMainMethodIndex(recordName));
        auto *notificationMgr = vm_->GetJsDebuggerManager()->GetNotificationManager();
        const std::string &url = patchExtractor->GetSourceFile(mainMethodIndex);
        if (url.empty()) {
            continue;
        }
        notificationMgr->LoadModuleEvent(jsPandaFile->GetJSPandaFileDesc(), recordName);
        patchExtractors_[url].emplace_back(patchExtractor);
    }
}
}  // namespace panda::ecmascript::tooling
