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
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;

namespace panda::test {

class HeapTest :  public BaseTestWithScope<false> {
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

class HeapTestHelper {
public:
    explicit HeapTestHelper(Heap *heap) : heap_(heap) {}

    ~HeapTestHelper() = default;

    void SetIdleGCState(bool flag)
    {
        ASSERT_NE(heap_, nullptr);
        heap_->enableIdleGC_ = flag;
    }

    Heap *GetHeap()
    {
        return heap_;
    }

private:
    Heap *heap_{nullptr};
};

}