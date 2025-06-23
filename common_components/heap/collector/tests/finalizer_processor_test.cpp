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

#include "common_components/heap/collector/finalizer_processor.h"
#include "common_components/heap/collector/collector_proxy.h"
#include "common_components/heap/heap_manager.h"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class FinalizerProcessorTest : public common::test::BaseTestWithScope {
    protected:
    static void SetUpTestCase()
    {
        BaseRuntime::GetInstance()->Init();
    }

    static void TearDownTestCase() {}

    void SetUp() override
    {
        MutatorManager::Instance().CreateRuntimeMutator(ThreadType::GC_THREAD);
    }

    void TearDown() override
    {
        MutatorManager::Instance().DestroyRuntimeMutator(ThreadType::GC_THREAD);
    }
};

HWTEST_F_L0(FinalizerProcessorTest, RegisterFinalizer_TEST1)
{
    FinalizerProcessor finalizerProcessor;
    HeapAddress addr = common::HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject*>(addr);
    new (obj) BaseObject(); // Construct BaseObject
    finalizerProcessor.RegisterFinalizer(obj);
    bool flag = common::RegionSpace::IsMarkedObject(obj);
    EXPECT_FALSE(flag);
}

HWTEST_F_L0(FinalizerProcessorTest, EnqueueFinalizables_TEST1)
{
    FinalizerProcessor finalizerProcessor;
    HeapAddress addr = common::HeapManager::Allocate(sizeof(BaseObject), AllocType::MOVEABLE_OBJECT, true);
    BaseObject *obj = reinterpret_cast<BaseObject*>(addr);
    new (obj) BaseObject(); // Construct BaseObject
    finalizerProcessor.RegisterFinalizer(obj);
    std::function<bool(BaseObject*)> finalizable = [](BaseObject* obj) {
        return !common::RegionSpace::IsMarkedObject(obj);
    };
    finalizerProcessor.EnqueueFinalizables(finalizable, 1);
    bool flag = finalizable(obj);
    EXPECT_TRUE(flag);
}
} // namespace common::test