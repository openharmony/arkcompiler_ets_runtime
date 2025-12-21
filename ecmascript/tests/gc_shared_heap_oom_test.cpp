/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <csetjmp>
#include <csignal>

#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCSharedHeapOOMTest : public BaseTestWithScope<false> {
};

static sigjmp_buf g_env;
static bool g_abortFlag = false;
static bool g_cbFlag = false;

static void SharedHeapOOMHandler(int sig)
{
    g_abortFlag = true;
    siglongjmp(g_env, sig);
}

static int RegisterSignal(struct sigaction* oldAct)
{
    struct sigaction act;
    act.sa_handler = SharedHeapOOMHandler;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGABRT);
    act.sa_flags = 0;
    
    return sigaction(SIGABRT, &act, oldAct);
}

static void RestoreSignal(const struct sigaction* oldAct)
{
    sigaction(SIGABRT, oldAct, nullptr);
}

static void ResetAbortFlag()
{
    g_abortFlag = false;
}

static void ResetCbFlag()
{
    g_cbFlag = false;
}

HWTEST_F_L0(GCSharedHeapOOMTest, TestSharedHeapOOM)
{
    struct sigaction oldAct;
    ASSERT_TRUE(RegisterSignal(&oldAct) != -1);

    {
        int ret = sigsetjmp(g_env, 1);
        if (ret == 0) {
            ResetAbortFlag();
            SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
            SharedHeap::GetInstance()->ShouldForceThrowOOMError();
            SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        }
        ASSERT_TRUE(g_abortFlag);
    }

    RestoreSignal(&oldAct);
    ResetAbortFlag();
    
    ASSERT_TRUE(RegisterSignal(&oldAct) != -1);
    {
        int ret = sigsetjmp(g_env, 1);
        if (ret == 0) {
            ResetAbortFlag();
            SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        }
        ASSERT_TRUE(g_abortFlag);
    }
    
    RestoreSignal(&oldAct);
    ResetAbortFlag();
}

HWTEST_F_L0(GCSharedHeapOOMTest, TestSharedHeapOOMWithCallback)
{
    struct sigaction oldAct;
    ASSERT_TRUE(RegisterSignal(&oldAct) != -1);

    thread->GetEcmaVM()->RegisterUncatchableErrorHandler([&]([[maybe_unused]] TryCatch &tryCatch) {
        g_cbFlag = true;
    });
    {
        int ret = sigsetjmp(g_env, 1);
        if (ret == 0) {
            ResetAbortFlag();
            ResetCbFlag();
            SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
            SharedHeap::GetInstance()->ShouldForceThrowOOMError();
            SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        }
        ASSERT_TRUE(g_abortFlag);
        ASSERT_TRUE(g_cbFlag);
    }

    RestoreSignal(&oldAct);
    ResetAbortFlag();
    ResetCbFlag();
    
    ASSERT_TRUE(RegisterSignal(&oldAct) != -1);
    {
        int ret = sigsetjmp(g_env, 1);
        if (ret == 0) {
            ResetAbortFlag();
            ResetCbFlag();
            SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::OTHER>(thread);
        }
        ASSERT_TRUE(g_abortFlag);
        ASSERT_TRUE(g_cbFlag);
    }
    
    RestoreSignal(&oldAct);
    ResetAbortFlag();
    ResetCbFlag();
}
} // namespace panda::test