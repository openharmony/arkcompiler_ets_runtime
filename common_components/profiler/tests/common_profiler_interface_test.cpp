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

#include "common_components/profiler/common_profiler_interface.h"
#include "common_components/profiler/heap_profiler_listener.cpp"
#include "common_components/tests/test_helper.h"

using namespace common;

namespace common::test {
class MockCommonProfiler : public CommonHeapProfilerInterface {
public:
    bool isSingleInstance = false;
    void DumpHeapSnapshotForCMCOOM() override
    {
        isSingleInstance = true;
    }
};

class CommonProfilerInterfaceTest : public common::test::BaseTestWithScope {
};

HWTEST_F_L0(CommonProfilerInterfaceTest, DumpHeapSnapshotBeforeOOM)
{
    MockCommonProfiler *instance = new MockCommonProfiler();
    ASSERT_NE(instance, nullptr);
    CommonHeapProfilerInterface::SetSingleInstance(nullptr);
    CommonHeapProfilerInterface::DumpHeapSnapshotBeforeOOM();
    CommonHeapProfilerInterface::SetSingleInstance(instance);
    CommonHeapProfilerInterface::DumpHeapSnapshotBeforeOOM();
    ASSERT_TRUE(instance->isSingleInstance);
    delete instance;
}
} // namespace common::test

namespace common::test {
class HeapProfilerListenerTest : public common::test::BaseTestWithScope {
protected:
    void SetUp() override
    {
        listener_ = &HeapProfilerListener::GetInstance();
        moveEventCallbackCalled = false;
        callbackId = 0;
    }

    void TearDown() override
    {
        if (callbackId != 0) {
            listener_->UnRegisterMoveEventCb(callbackId);
        }
    }

    HeapProfilerListener* listener_;
    bool moveEventCallbackCalled;
    uint32_t callbackId;
};

uintptr_t capturedFromObj = 0;
uintptr_t capturedToObj = 0;
size_t capturedSize = 0;

HWTEST_F_L0(HeapProfilerListenerTest, OnMoveEvent)
{
    std::function<void(uintptr_t, uintptr_t, size_t)> emptyCallback;
    callbackId = listener_->RegisterMoveEventCb(emptyCallback);
    listener_->OnMoveEvent(0x1000, 0x2000, 64);
    uintptr_t fromObj = 0x12345678;
    uintptr_t toObj = 0x87654321;
    size_t size = 1024;
    auto callback = [this](uintptr_t from, uintptr_t to, size_t sz) {
        moveEventCallbackCalled = true;
        capturedFromObj = from;
        capturedToObj = to;
        capturedSize = sz;
    };

    callbackId = listener_->RegisterMoveEventCb(callback);
    listener_->OnMoveEvent(fromObj, toObj, size);
    EXPECT_TRUE(moveEventCallbackCalled);
    EXPECT_EQ(capturedFromObj, fromObj);
    EXPECT_EQ(capturedToObj, toObj);
    EXPECT_EQ(capturedSize, size);
}
} //namespace common::test
