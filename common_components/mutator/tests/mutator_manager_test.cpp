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

#include "common_components/tests/test_helper.h"
#include "common_components/mutator/mutator_manager.h"
#include "common_components/base_runtime/base_runtime.cpp"
#include "common_components/heap/w_collector/w_collector.h"

using namespace common;

namespace common::test {
class MutatorManagerTest : public BaseTestWithScope {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F_L0(MutatorManagerTest, IsRuntimeThread_Test1)
{
    ThreadLocalData* localData = ThreadLocal::GetThreadLocalData();
    localData->threadType = ThreadType::HOT_UPDATE_THREAD;
    bool ret = IsRuntimeThread();
    EXPECT_TRUE(ret == true);

    localData->threadType = ThreadType::ARK_PROCESSOR;
    ret = IsRuntimeThread();
    EXPECT_TRUE(ret == false);
}

HWTEST_F_L0(MutatorManagerTest, IsGcThread_Test1)
{
    ThreadLocalData* localData = ThreadLocal::GetThreadLocalData();
    localData->threadType = ThreadType::GC_THREAD;
    bool ret = IsGcThread();
    EXPECT_TRUE(ret == true);

    localData->threadType = ThreadType::ARK_PROCESSOR;
    ret = IsGcThread();
    EXPECT_TRUE(ret == false);
}

HWTEST_F_L0(MutatorManagerTest, BindMutator_Test1)
{
    ThreadLocalData* localData = ThreadLocal::GetThreadLocalData();
    localData->buffer = nullptr;
    MutatorManager *managerPtr = new MutatorManager();
    Mutator mutator;
    managerPtr->BindMutator(mutator);
    EXPECT_TRUE(localData->buffer != nullptr);

    managerPtr->BindMutator(mutator);
    EXPECT_TRUE(localData->mutator != nullptr);
    delete managerPtr;
}

HWTEST_F_L0(MutatorManagerTest, CreateMutator_Test1)
{
    ThreadLocalData* localData = ThreadLocal::GetThreadLocalData();
    localData->mutator = new Mutator();
    MutatorManager *managerPtr = new MutatorManager();
    BaseRuntime::GetInstance()->Init();
    Mutator* ptr = managerPtr->CreateMutator();
    EXPECT_TRUE(ptr != nullptr);

    delete managerPtr;
    delete localData->mutator;
}

HWTEST_F_L0(MutatorManagerTest, TransitMutatorToExit_Test1)
{
    ThreadLocalData* localData = ThreadLocal::GetThreadLocalData();
    localData->mutator = new Mutator();
    MutatorManager *managerPtr = new MutatorManager();
    localData->mutator->SetInSaferegion(MutatorBase::SaferegionState::SAFE_REGION_FALSE);
    managerPtr->TransitMutatorToExit();
    delete managerPtr;
    delete localData->mutator;
}

HWTEST_F_L0(MutatorManagerTest, CreateRuntimeMutato_Test1)
{
    ThreadType threadType = ThreadType::FP_THREAD;
    Mutator* ptr = MutatorManager::Instance().CreateRuntimeMutator(threadType);
    EXPECT_TRUE(ptr != nullptr);

    threadType = ThreadType::GC_THREAD;
    ptr = MutatorManager::Instance().CreateRuntimeMutator(threadType);
    EXPECT_TRUE(ptr != nullptr);
}

HWTEST_F_L0(MutatorManagerTest, DumpMutators_Test1)
{
    MutatorManager *managerPtr = new MutatorManager();
    uint32_t timeoutTimes = 1;
    managerPtr->DumpMutators(timeoutTimes);
    delete managerPtr;
}
}  // namespace common::test
