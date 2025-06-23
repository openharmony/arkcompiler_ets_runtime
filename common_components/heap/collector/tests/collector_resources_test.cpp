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

#include "common_components/heap/collector/collector_resources.h"
#include "common_components/mutator/mutator_manager.h"
#include "common_components/tests/test_helper.h"
#include "common_interfaces/base_runtime.h"

using namespace common;

namespace common::test {
class CollectorResourcesTest : public BaseTestWithScope {
protected:
    static void SetUpTestCase()
    {
        BaseRuntime::GetInstance()->Init();
    }

    static void TearDownTestCase()
    {
        BaseRuntime::GetInstance()->Fini();
    }

    void SetUp() override
    {
        MutatorManager::Instance().CreateRuntimeMutator(ThreadType::GC_THREAD);
    }

    void TearDown() override
    {
        MutatorManager::Instance().DestroyRuntimeMutator(ThreadType::GC_THREAD);
    }
};

HWTEST_F_L0(CollectorResourcesTest, RequestHeapDumpTest) {
    Heap::GetHeap().GetCollectorResources().RequestHeapDump(
        GCTask::GCTaskType::GC_TASK_INVALID);
    EXPECT_TRUE(Heap::GetHeap().IsGCEnabled());
}
} // namespace common::test