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
#include "common_components/mutator/mutator.h"
#include "common_components/base_runtime/base_runtime.cpp"

using namespace common;

namespace common::test {
class MutatorTest : public BaseTestWithScope {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F_L0(MutatorTest, GetThreadLocalData_Test1)
{
    ThreadLocalData *ret = GetThreadLocalData();
    ASSERT_TRUE(ret != nullptr);
}

HWTEST_F_L0(MutatorTest, TransitionGCPhase_Test1)
{
    MutatorBase *mutatorBase = new MutatorBase();
    mutatorBase->Init();
    bool ret = mutatorBase->TransitionGCPhase(false);
    EXPECT_TRUE(ret == true);

    MutatorBase::SuspensionType  flag = MutatorBase::SuspensionType::SUSPENSION_FOR_GC_PHASE;
    mutatorBase->SetSuspensionFlag(flag);
    BaseRuntime::GetInstance()->Init();
    ret = mutatorBase->TransitionGCPhase(false);
    EXPECT_TRUE(ret == true);

    ret = mutatorBase->TransitionGCPhase(true);
    EXPECT_TRUE(ret == true);
    ret = mutatorBase->TransitionGCPhase(false);
    EXPECT_TRUE(ret == true);
    delete mutatorBase;
}
}  // namespace common::test
