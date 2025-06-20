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

#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/js_array.h"

using namespace panda::ecmascript;

namespace panda::test {
class GlobalHandleLeakDetectTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetArkProperties(options.GetArkProperties() | GLOBAL_OBJECT_LEAK_CHECK | GLOBAL_PRIMITIVE_LEAK_CHECK);
        ecmaVm_ = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(ecmaVm_ != nullptr) << "Cannot create EcmaVM";
        thread_ = ecmaVm_->GetJSThread();
        thread_->ManagedCodeBegin();
        scope_ = new EcmaHandleScope(thread_);
        ecmaVm_->SetEnableForceGC(false);
        heapProfiler_ = reinterpret_cast<HeapProfiler *>(HeapProfilerInterface::GetInstance(ecmaVm_));
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm_, scope_);
    }

    void DumpHeapSnapshot(const std::string &detectResultFilePath)
    {
        DumpSnapShotOption dumpOption;
        dumpOption.dumpFormat = DumpFormat::JSON;
        dumpOption.captureNumericValue = true;
        dumpOption.isSync = false;
        FileDescriptorStream stream(-1);

        JSRuntimeOptions &options = ecmaVm_->GetJSOptions();
        options.SwitchStartGlobalLeakCheck();
        if (options.EnableGlobalLeakCheck() && options.IsStartGlobalLeakCheck()) {
            int fd = open(detectResultFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
            ASSERT_NE(fd, -1) << strerror(errno);
            thread_->SetStackTraceFd(fd);
        }
        heapProfiler_->DumpHeapSnapshot(&stream, dumpOption);
    }

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
    HeapProfiler *heapProfiler_ {nullptr};
};

HWTEST_F_L0(GlobalHandleLeakDetectTest, TestDetectionResult)
{
    std::string detectResultFilePath{"global_handle_leak_detect_result.txt"};
    DumpHeapSnapshot(detectResultFilePath);

    JSHandle<JSTaggedValue> arrayInRoot = JSArray::ArrayCreate(thread_, JSTaggedNumber(1));
    // New a GlobalHandle without Dispose
    thread_->NewGlobalHandle(arrayInRoot.GetTaggedType());

    DumpHeapSnapshot(detectResultFilePath);

    std::ifstream file {detectResultFilePath};
    std::string line;
    bool hasHandleNumber {false};
    bool hasLeakObject {false};
    bool hasLeakFooter {false};
    while (std::getline(file, line)) {
        if (!hasHandleNumber && line.find("Global Handle Number") != std::string::npos) {
            hasHandleNumber = true;
            continue;
        }
        if (!hasLeakObject && line.find("Global maybe leak object address") != std::string::npos) {
            hasLeakObject = true;
            continue;
        }
        if (!hasLeakFooter && line.find("Global leak check success!") != std::string::npos) {
            hasLeakFooter = true;
            continue;
        }
    }
    ASSERT_TRUE(hasHandleNumber);
    ASSERT_TRUE(hasLeakObject);
    ASSERT_TRUE(hasLeakFooter);
}
}  // namespace panda::test