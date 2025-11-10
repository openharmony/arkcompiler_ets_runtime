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
#ifndef ECMASCRIPT_PATCH_QUICK_FIX_MANAGER_H
#define ECMASCRIPT_PATCH_QUICK_FIX_MANAGER_H

#include "ecmascript/napi/include/jsnapi_internals.h"
#include "ecmascript/patch/patch_loader.h"
#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {
class QuickFixManager {
public:
    QuickFixManager() = default;
    ~QuickFixManager();

    void RegisterQuickFixQueryFunc(const std::function<bool(std::string baseFileName,
                        std::string &patchFileName,
                        uint8_t **patchBuffer,
                        size_t &patchSize)> callBack);
    void LoadPatchIfNeeded(JSThread *thread, const JSPandaFile *baseFile);
    PatchErrorCode LoadPatch(JSThread *thread, const std::string &patchFileName, const std::string &baseFileName);
    PatchErrorCode LoadPatch(JSThread *thread,
                             const std::string &patchFileName, uint8_t *patchBuffer, size_t patchSize,
                             const std::string &baseFileName, uint8_t *baseBuffer, size_t baseSize);
    PatchErrorCode UnloadPatch(JSThread *thread, const std::string &patchFileName);
    bool IsQuickFixCausedException(JSThread *thread,
                                   const JSHandle<JSTaggedValue> &exceptionInfo,
                                   const std::string &patchFileName);
    JSTaggedValue CheckAndGetPatch(JSThread *thread, const JSPandaFile *baseFile, EntityId baseMethodId);
    CString GetBaseFileName(const CString &fileName);
    CString GetBaseFileName(const JSHandle<SourceTextModule> &module);

    inline CString GetBaseFileNameForHotReload(JSThread *thread, const CString &fileName)
    {
        if (thread->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN) {
            return GetBaseFileName(fileName);
        }
        return fileName;
    }

    inline void UpdateHotReloadModule(JSThread *thread, JSMutableHandle<SourceTextModule> &module)
    {
        if (thread->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN) {
            UpdateHotReloadModuleAfterLoad(thread, module);
        }
    }

    inline void UpdateHotReloadModuleAndNamespace(JSThread *thread,
                                                  JSMutableHandle<SourceTextModule> &module,
                                                  JSMutableHandle<ModuleNamespace> &moduleNamespace)
    {
        if (thread->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN) {
            UpdateHotReloadModuleAndNamespaceAfterLoad(thread, module, moduleNamespace);
        }
    }
private:
    // check whether the callback is registered.
    bool HasQueryQuickFixInfoFunc() const
    {
        return callBack_ != nullptr;
    }

    inline void UpdateHotReloadModuleAfterLoad(JSThread *thread, JSMutableHandle<SourceTextModule> &module)
    {
        ASSERT(thread->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN);
        const JSHandle<JSTaggedValue> moduleOfHotReload =
            thread->GetEcmaVM()->FindPatchModule(module->GetEcmaModuleRecordNameString());
        if (!moduleOfHotReload->IsHole()) {
            module.Update(moduleOfHotReload);
        }
    }

    inline void UpdateHotReloadModuleAndNamespaceAfterLoad(JSThread *thread,
                                                           JSMutableHandle<SourceTextModule> &module,
                                                           JSMutableHandle<ModuleNamespace> &moduleNamespace)
    {
        ASSERT(thread->GetStageOfHotReload() == StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN);
        const JSHandle<JSTaggedValue> moduleOfHotReload =
            thread->GetEcmaVM()->FindPatchModule(module->GetEcmaModuleRecordNameString());
        if (!moduleOfHotReload->IsHole()) {
            module.Update(moduleOfHotReload);
            moduleNamespace.Update(SourceTextModule::GetModuleNamespace(thread, module));
        }
    }

    CUnorderedSet<CString> ParseStackInfo(const CString &stackInfo);

    // key: baseFileName
    CMap<CString, PatchInfo> methodInfos_ {};
    std::function<bool(std::string baseFileName,
                        std::string &patchFileName,
                        uint8_t **patchBuffer,
                        size_t &patchSize)> callBack_;
    CMap<uint32_t, CString> baseClassInfo_ {};
    // key: patch file name, value: base file name
    CMap<CString, CString> patchAndBaseFileNameMap_;
    std::set<CString> checkedFiles_ {};
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_PATCH_QUICK_FIX_MANAGER_H