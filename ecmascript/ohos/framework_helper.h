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
#include <vector>

#include "ecmascript/platform/file.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class FrameworkHelper {
public:

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
        const std::vector<std::string> arkuiFileNames = {"/stateMgmt.abc"};
        if (vm_->GetJSOptions().WasSetCompilerFrameworkAbcPath()) {
            CompilerFrameworkAotPath_ = vm_->GetJSOptions().GetCompilerFrameworkAbcPath();
        }
        if (vm_->GetJSOptions().IsEnableFrameworkAOT()) {
            for (const auto &name : arkuiFileNames) {
                std::string fileName = CompilerFrameworkAotPath_ + name;
                if (FileExist(fileName.c_str())) {
                    frameworkAbcFiles_.insert(fileName);
                }
            }
        }
    }

    [[maybe_unused]] JSThread *thread_ {nullptr};
    EcmaVM *vm_ {nullptr};

    std::set<std::string> frameworkAbcFiles_ {};
    std::string CompilerFrameworkAotPath_ {"/etc/abc/framework"};
};
}  // namespace panda::ecmascript::kungfu
#endif