/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef ARK_COMMON_RUNTIME_H
#define ARK_COMMON_RUNTIME_H

// Arkcommon's Runtime
#include <mutex>
#include <unordered_map>

#include "securec.h"

#include "common_components/common_runtime/src/common/runtime.h"
#include "common_components/common_runtime/src/base/globals.h"
#include "common_components/log/log.h"

namespace panda {

enum class StackGrowConfig {
    UNDEF = 0,
    STACK_GROW_OFF = 1,
    STACK_GROW_ON = 2,
};

class ArkCommonRuntime : private Runtime {
public:
    static void CreateAndInit(const RuntimeParam& runtimeParam);
    static void FiniAndDelete();
    static HeapParam GetHeapParam() { return Runtime::Current().GetRuntimeParam().heapParam; }
    void SetGCThreshold(uint64_t threshold) override
    {
        if (threshold == 0) {
            LOG_COMMON(ERROR) << "The threshold must be greater than 0.\n";
        } else {
            param_.gcParam.gcThreshold = threshold * KB;
            LOG_COMMON(INFO) << "gcThreshold is set to " << threshold << " KB.";
        }
    }
    static GCParam GetGCParam() { return Runtime::Current().GetRuntimeParam().gcParam; }
    RuntimeParam GetRuntimeParam() const override { return param_; }
    void SetCommandLinesArgs(int argc, const char* argv[])
    {
        commandLineArgs_ = new const char* [argc + 1]();
        size_t cstrLen = 0;
        for (int i = 0; i < argc; ++i) {
            cstrLen = strlen(argv[i]) + 1;
            commandLineArgs_[i] = new const char[cstrLen]();
            LOGE_IF(memcpy_s(const_cast<char*>(commandLineArgs_[i]), cstrLen, argv[i], cstrLen) != EOK) <<
                "memcpy_s fail";
        }
    }

    const char** GetCommandLineArgs() { return commandLineArgs_; }
    static StackGrowConfig stackGrowConfig;

protected:
    explicit ArkCommonRuntime(const RuntimeParam& runtimeParam);
    ~ArkCommonRuntime() override
    {
        if (commandLineArgs_) {
            for (int i = 0; commandLineArgs_[i]; ++i) {
                delete[] commandLineArgs_[i];
            }
            delete[] commandLineArgs_;
        }
    }

private:
    void Init();
    void Fini();

    RuntimeParam param_;
    const char** commandLineArgs_ = nullptr;
    // ConcurrencyModelMap subModelMap
    std::mutex mtx_;
};
} // namespace panda
#endif // ARK_COMMON_RUNTIME_H
