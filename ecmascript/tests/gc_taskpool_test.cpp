/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/taskpool/taskpool.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCTest : public BaseTestWithScope<false> {
public:
    class DummyTask : public Task {
    public:
        DummyTask(int32_t id, uint64_t executionMilliseconds)
            : Task(id), executionMilliseconds_(executionMilliseconds) {}
        ~DummyTask() override = default;
        bool Run([[maybe_unused]] uint32_t threadIndex) override
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(executionMilliseconds_));
            return true;
        }

        NO_COPY_SEMANTIC(DummyTask);
        NO_MOVE_SEMANTIC(DummyTask);
    private:
        uint64_t executionMilliseconds_;
    };

    class DummyDelayedTask : public Task {
    public:
        DummyDelayedTask(int32_t id, uint64_t executionMilliseconds, SteadyTimePoint deadline)
            : Task(id), executionMilliseconds_(executionMilliseconds), deadline_(deadline) {}
        ~DummyDelayedTask() override = default;
        bool Run([[maybe_unused]] uint32_t threadIndex) override
        {
            SteadyTimePoint current = std::chrono::steady_clock::now();
            EXPECT_TRUE((std::chrono::duration_cast<std::chrono::duration<double>>(current - deadline_)).count() > 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(executionMilliseconds_));
            return true;
        }

        NO_COPY_SEMANTIC(DummyDelayedTask);
        NO_MOVE_SEMANTIC(DummyDelayedTask);
    private:
        uint64_t executionMilliseconds_;
        SteadyTimePoint deadline_;
    };
};

HWTEST_F_L0(GCTest, TaskpoolTest1)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    uint64_t executionMilliseconds = 100;
    for (uint32_t i = 0; i < 10; i++) {
        Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<DummyTask>(heap->GetJSThread()->GetThreadId(), executionMilliseconds));
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

HWTEST_F_L0(GCTest, TaskpoolTest2)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    uint64_t executionMilliseconds = 100;
    uint64_t delayMilliseconds = 300;
    for (uint32_t i = 0; i < 10; i++) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMilliseconds);
        Taskpool::GetCurrentTaskpool()->PostDelayedTask(
            std::make_unique<DummyDelayedTask>(heap->GetJSThread()->GetThreadId(), executionMilliseconds, deadline),
            delayMilliseconds);
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

HWTEST_F_L0(GCTest, TaskpoolTest3)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    uint64_t executionMilliseconds = 500;
    uint64_t delayMilliseconds = 300;
    for (uint32_t i = 0; i < 10; i++) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMilliseconds);
        Taskpool::GetCurrentTaskpool()->PostDelayedTask(
            std::make_unique<DummyDelayedTask>(heap->GetJSThread()->GetThreadId(), executionMilliseconds, deadline),
            delayMilliseconds);
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

HWTEST_F_L0(GCTest, TaskpoolTest4)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    uint64_t executionMilliseconds = 100;
    uint64_t delayMilliseconds = 300;
    for (uint32_t i = 0; i < 10; i++) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMilliseconds);
        Taskpool::GetCurrentTaskpool()->PostDelayedTask(
            std::make_unique<DummyDelayedTask>(heap->GetJSThread()->GetThreadId(), executionMilliseconds, deadline),
            delayMilliseconds);
    }
    for (uint32_t i = 0; i < 10; i++) {
        Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<DummyTask>(heap->GetJSThread()->GetThreadId(), executionMilliseconds));
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
}

HWTEST_F_L0(GCTest, TaskpoolTest5)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    uint64_t hugeJobExecutionMilliseconds = 3000;
    uint64_t executionMilliseconds = 100;
    uint64_t delayMilliseconds = 300;
    Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<DummyTask>(heap->GetJSThread()->GetThreadId(), hugeJobExecutionMilliseconds));
    for (uint32_t i = 0; i < 10; i++) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(delayMilliseconds);
        Taskpool::GetCurrentTaskpool()->PostDelayedTask(
            std::make_unique<DummyDelayedTask>(heap->GetJSThread()->GetThreadId(), executionMilliseconds, deadline),
            delayMilliseconds);
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
}
} // namespace panda::test