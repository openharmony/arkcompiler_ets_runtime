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

#include <cstdio>

#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_sampling.h"
#include "ecmascript/dfx/stackinfo/js_stackgetter.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class HeapSamplingTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        instance->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(HeapSamplingTest, StartHeapSampling)
{
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
    uint64_t samplingInterval = 1 << 15; // default interval
    int stackDepth = 128; // default depth
    bool result = heapProfile->StartHeapSampling(samplingInterval, stackDepth);
    ASSERT_TRUE(result);
    result = heapProfile->StartHeapSampling(samplingInterval, stackDepth);
    ASSERT_FALSE(result);
}

HWTEST_F_L0(HeapSamplingTest, StopHeapSampling)
{
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
    uint64_t samplingInterval = 1 << 15; // default interval
    int stackDepth = 128; // default depth
    bool result = heapProfile->StartHeapSampling(samplingInterval, stackDepth);
    ASSERT_TRUE(result);
    heapProfile->StopHeapSampling();
    result = heapProfile->StartHeapSampling(samplingInterval, stackDepth);
    ASSERT_TRUE(result);
}

HWTEST_F_L0(HeapSamplingTest, GetAllocationProfile)
{
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
    const SamplingInfo *result = heapProfile->GetAllocationProfile();
    ASSERT_TRUE(result == nullptr);
    uint64_t samplingInterval = 1 << 15; // default interval
    int stackDepth = 128; // default depth
    heapProfile->StartHeapSampling(samplingInterval, stackDepth);
    result = heapProfile->GetAllocationProfile();
    EXPECT_TRUE(result != nullptr);
    EXPECT_TRUE(result->head_.callFrameInfo_.functionName_ == "(root)");
}

HWTEST_F_L0(HeapSamplingTest, ImplementSampling)
{
    uint64_t samplingInterval = 1 << 15; // default interval
    int stackDepth = 128; // default depth
    std::unique_ptr<HeapSampling> heapSampling = std::make_unique<HeapSampling>(instance,
        const_cast<Heap *>(instance->GetHeap()), samplingInterval, stackDepth);
    size_t size = 1U << 15U; // default size
    Address addr = 0;
    heapSampling->ImplementSampling(addr, size);
    const SamplingInfo *result = heapSampling->GetAllocationProfile();
    EXPECT_TRUE(result != nullptr);
    EXPECT_TRUE(result->samples_[0].size_ == size);
}
}  // namespace panda::test
