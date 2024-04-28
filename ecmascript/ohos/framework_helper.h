/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_OHOS_FRAMEWORK_HELPER_H
#define ECMASCRIPT_OHOS_FRAMEWORK_HELPER_H

#include <string>
#include <set>

#include "ecmascript/platform/file.h"
#include "macros.h"

namespace panda::ecmascript {
class FrameworkHelper {
public:
    constexpr static const char *const STATEMGMT_ABC_FILE_NAME = "/stateMgmt.abc";
    constexpr static const char *const JSENUMSTYLE_ABC_FILE_NAME = "/jsEnumStyle.abc";
    constexpr static const char *const JSUICONTEXT_ABC_FILE_NAME = "/jsUIContext.abc";
    constexpr static const char *const ARKCOMPONENT_ABC_FILE_NAME = "/arkComponent.abc";
    constexpr static const char *const PRELOAD_PATH_PREFIX = "/system/";
    constexpr static const char *const PRELOAD_AN_FOLDER = "/ark-cache/";

    FrameworkHelper(JSThread *thread)
        : thread_(thread),
          vm_(thread->GetEcmaVM())
    {
        FilePathInit();
    }

    ~FrameworkHelper() = default;

    EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

    void SetCompilerFrameworkAotPath(std::string frameworkAotPath)
    {
        CompilerFrameworkAotPath_ = std::move(frameworkAotPath);
    }

    static void GetRealRecordName(CString &recordName)
    {
        if (recordName == "_GLOBAL::func_main_0") {
            recordName = "_GLOBAL";
        }
    }

    const std::set<std::string> &GetFrameworkAbcFiles() const
    {
        return frameworkAbcFiles_;
    }

    bool IsFrameworkAbcFile(const std::string& fileName) const
    {
        return frameworkAbcFiles_.find(fileName) != frameworkAbcFiles_.end();
    }

private:
    void FilePathInit()
    {
        if (vm_->GetJSOptions().WasSetCompilerFrameworkAbcPath()) {
            CompilerFrameworkAotPath_ = vm_->GetJSOptions().GetCompilerFrameworkAbcPath();
            frameworkAbcFiles_.insert(CompilerFrameworkAotPath_ + STATEMGMT_ABC_FILE_NAME);
            frameworkAbcFiles_.insert(CompilerFrameworkAotPath_ + JSENUMSTYLE_ABC_FILE_NAME);
            frameworkAbcFiles_.insert(CompilerFrameworkAotPath_ + JSUICONTEXT_ABC_FILE_NAME);
            frameworkAbcFiles_.insert(CompilerFrameworkAotPath_ + ARKCOMPONENT_ABC_FILE_NAME);
        }
    }

    [[maybe_unused]] JSThread *thread_ {nullptr};
    EcmaVM *vm_ {nullptr};

    std::set<std::string> frameworkAbcFiles_ {};
    std::string CompilerFrameworkAotPath_ {""};
};
}  // namespace panda::ecmascript::kungfu
#endif