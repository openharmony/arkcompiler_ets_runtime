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

using namespace panda::ecmascript;

namespace panda::test {
class LocalHandleLeakDetectTest : public testing::Test {
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
        ecmaVm_ = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(ecmaVm_ != nullptr) << "Cannot create EcmaVM";
        thread_ = ecmaVm_->GetJSThread();
        thread_->ManagedCodeBegin();
        ecmaVm_->SetEnableForceGC(false);
        heapProfiler_ = reinterpret_cast<HeapProfiler *>(HeapProfilerInterface::GetInstance(ecmaVm_));
    }

    void TearDown() override
    {
        ecmaVm_->GetJSThread()->ManagedCodeEnd();
        JSNApi::DestroyJSVM(ecmaVm_);
    }

    void DumpHeapSnapshot(const std::string &detectResultFilePath)
    {
        DumpSnapShotOption dumpOption;
        dumpOption.dumpFormat = DumpFormat::JSON;
        FileDescriptorStream stream(-1);    // No snapshot file name is specified

        heapProfiler_->SwitchStartLocalHandleLeakDetect();
        if (heapProfiler_->IsStartLocalHandleLeakDetect()) {
            int fd = open(detectResultFilePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
            ASSERT_NE(fd, -1) << strerror(errno);
            heapProfiler_->SetLeakStackTraceFd(fd);
        }

        bool result = heapProfiler_->DumpHeapSnapshot(&stream, dumpOption, nullptr, nullptr);
        ASSERT_TRUE(result) << "DumpHeapSnapshot failed!";
    }

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};
    HeapProfiler *heapProfiler_ {nullptr};
};

HWTEST_F_L0(LocalHandleLeakDetectTest, TestDetectionResult)
{
    std::string detectResultFilePath{"local_handle_leak_detect_result.txt"};
    DumpHeapSnapshot(detectResultFilePath);
    ObjectFactory *factory = ecmaVm_->GetFactory();
    // Create a handle without a HandleScope
    factory->NewTaggedArray(10);
    DumpHeapSnapshot(detectResultFilePath);

    std::ifstream file {detectResultFilePath};
    std::string line;
    bool hasLeakHeader {false};
    bool hasNodeId {false};
    bool hasLeakFooter {false};
    while (std::getline(file, line)) {
        if (!hasLeakHeader && line.find("Local Handle Leak Detection Result") != std::string::npos) {
            hasLeakHeader = true;
            continue;
        }
        if (!hasNodeId && line.find("NodeId") != std::string::npos) {
            hasNodeId = true;
            continue;
        }
        if (!hasLeakFooter && line.find("End of Local Handle Leak Detection Result") != std::string::npos) {
            hasLeakFooter = true;
            continue;
        }
    }
    ASSERT_TRUE(hasLeakHeader);
    ASSERT_TRUE(hasNodeId);
    ASSERT_TRUE(hasLeakFooter);
}
}  // namespace panda::test