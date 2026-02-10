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

#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/runtime.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;
namespace panda::test {

class RuntimeTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
    }
};

HWTEST_F_L0(RuntimeTest, RegisterTest)
{
    RuntimeOption option;
    std::thread t1([&]() {
        auto vm1 = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm1 != nullptr) << "Cannot create EcmaVM1";
        auto thread1 = vm1->GetJSThread();
        ASSERT_TRUE(thread1 != nullptr);
        EXPECT_EQ(thread1->GetState(), ecmascript::ThreadState::NATIVE);
        JSNApi::DestroyJSVM(vm1);
    });
    
    std::thread t2([&]() {
        auto vm2 = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm2 != nullptr) << "Cannot create EcmaVM2";
        auto thread2 = vm2->GetJSThread();
        ASSERT_TRUE(thread2 != nullptr);
        EXPECT_EQ(thread2->GetState(), ecmascript::ThreadState::NATIVE);
        JSNApi::DestroyJSVM(vm2);
    });

    {
        ecmascript::ThreadSuspensionScope scope(thread);
        t1.join();
        t2.join();
    }
}

HWTEST_F_L0(RuntimeTest, SetInBackground1)
{
    Runtime::GetInstance()->SetInBackground(true);
    ASSERT_TRUE(Runtime::GetInstance()->IsInBackground());

    Runtime::GetInstance()->SetInBackground(false);
    ASSERT_FALSE(Runtime::GetInstance()->IsInBackground());
}

HWTEST_F_L0(RuntimeTest, SetInBackground2)
{
    std::mutex mtx;
    std::condition_variable cv;
    bool paramsChanged = false;

    RuntimeOption option;
    std::thread t1([&]() {
        auto vm1 = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm1 != nullptr) << "Cannot create EcmaVM1";
        auto thread1 = vm1->GetJSThread();
        ASSERT_TRUE(thread1 != nullptr);
        
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return paramsChanged; });

        ASSERT_TRUE(Runtime::GetInstance()->IsInBackground());
        JSNApi::DestroyJSVM(vm1);
    });
    
    std::thread t2([&]() {
        auto vm2 = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm2 != nullptr) << "Cannot create EcmaVM2";
        auto thread2 = vm2->GetJSThread();
        ASSERT_TRUE(thread2 != nullptr);

        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return paramsChanged; });

        ASSERT_TRUE(Runtime::GetInstance()->IsInBackground());
        JSNApi::DestroyJSVM(vm2);
    });

    sleep(1);
    Runtime::GetInstance()->SetInBackground(true);
    {
        std::unique_lock<std::mutex> lock(mtx);
        paramsChanged = true;
        cv.notify_all();
    }
    {
        ecmascript::ThreadSuspensionScope scope(thread);
        t1.join();
        t2.join();
    }
    ASSERT_TRUE(Runtime::GetInstance()->IsInBackground());
}

HWTEST_F_L0(RuntimeTest, EnableProcDumpInSharedOOMBasic)
{
    // Test basic functionality in single-threaded environment
    Runtime::GetInstance()->SetProcDumpInSharedOOM(true);
    ASSERT_TRUE(Runtime::GetInstance()->IsEnableProcDumpInSharedOOM());

    Runtime::GetInstance()->SetProcDumpInSharedOOM(false);
    ASSERT_FALSE(Runtime::GetInstance()->IsEnableProcDumpInSharedOOM());

    // Test toggle
    Runtime::GetInstance()->SetProcDumpInSharedOOM(true);
    ASSERT_TRUE(Runtime::GetInstance()->IsEnableProcDumpInSharedOOM());

    Runtime::GetInstance()->SetProcDumpInSharedOOM(false);
    ASSERT_FALSE(Runtime::GetInstance()->IsEnableProcDumpInSharedOOM());
}

HWTEST_F_L0(RuntimeTest, EnableProcDumpInSharedOOMConcurrent)
{
    // Test thread-safety and memory visibility with release-acquire semantics
    // Release store ensures writes visible before flag, acquire load ensures synchronization
    constexpr int threadNumber = 8;
    // Number of operations per thread, with odd count (1001) ending on even index (1000) resulting in final value true
    constexpr int iterations = 1001;

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
    std::atomic<int> totalOps {0};  // Total operation count to verify all threads completed execution

    auto concurrentWrite = [&mtx, &cv, &ready, &totalOps]() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&]() { return ready; });
        }

        for (size_t i = 0; i < iterations; i++) {
            Runtime::GetInstance()->SetProcDumpInSharedOOM((i & 1) == 0);
            totalOps++;
        }
    };

    std::vector<std::thread> writeThreads;
    for (int _ = 0; _ < threadNumber; _++) {
        writeThreads.emplace_back(concurrentWrite);
    }

    // Start all threads for concurrent execution
    {
        std::unique_lock<std::mutex> lock(mtx);
        ready = true;
    }
    cv.notify_all();

    for (int i = 0; i < threadNumber; i++) {
        writeThreads[i].join();
    }

    // Verify all threads completed expected number of operations
    EXPECT_EQ(totalOps.load(), iterations * threadNumber);

    // Final state check: expect true due to last write (i=1000, even index sets true)
    EXPECT_TRUE(Runtime::GetInstance()->IsEnableProcDumpInSharedOOM());

    Runtime::GetInstance()->SetProcDumpInSharedOOM(false);
    EXPECT_FALSE(Runtime::GetInstance()->IsEnableProcDumpInSharedOOM());
}
}
