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
#ifndef ECMASCRIPT_JSPANDAFILE_QUICK_FIX_MANAGER_H
#define ECMASCRIPT_JSPANDAFILE_QUICK_FIX_MANAGER_H

#include "ecmascript/jspandafile/quick_fix_loader.h"

namespace panda::ecmascript {
class QuickFixManager {
public:
    using QuickFixQueryCallBack = bool (*)(std::string, std::string &, void **, size_t);

    QuickFixManager() = default;
    ~QuickFixManager();

    void RegisterQuickFixQueryFunc(const QuickFixQueryCallBack callBack);
    void LoadPatchIfNeeded(JSThread *thread, const std::string &baseFileName);
    bool LoadPatch(JSThread *thread, const std::string &patchFileName, const std::string &baseFileName);
    bool LoadPatch(JSThread *thread, const std::string &patchFileName, const void *patchBuffer, size_t patchSize,
                   const std::string &baseFileName);
    bool UnloadPatch(JSThread *thread, const std::string &patchFileName);
    bool IsQuickFixCausedException(JSThread *thread,
                                   const JSHandle<JSTaggedValue> &exceptionInfo,
                                   const std::string &patchFileName);
private:
    // check whether the callback is registered.
    bool HasQueryQuickFixInfoFunc() const
    {
        return callBack_ != nullptr;
    }

    // check whether the patch is loaded.
    inline bool HasLoadedPatch(std::string &patchFileName) const
    {
        return quickFixLoaders_.find(patchFileName) != quickFixLoaders_.end();
    }

    CUnorderedSet<CString> ParseStackInfo(const CString &stackInfo);

    // For multi patch.
    // key: patchFileName, value: QuickFixLoader of this patch.
    CMap<std::string, QuickFixLoader*> quickFixLoaders_;
    QuickFixQueryCallBack callBack_ {nullptr};
};
}  // namespace panda::ecmascript
#endif // ECMASCRIPT_JSPANDAFILE_QUICK_FIX_MANAGER_H