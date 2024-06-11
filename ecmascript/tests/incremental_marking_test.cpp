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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/verification.h"

using namespace panda::ecmascript;

namespace panda::test {
class IncrementalMarkingTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
        instance->SetEnableForceGC(false);
    }

    JSHandle<TaggedArray> CreateTaggedArray(uint32_t length, JSTaggedValue initVal, MemSpaceType spaceType)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        return factory->NewTaggedArray(length, initVal, spaceType);
    }
};

HWTEST_F_L0(IncrementalMarkingTest, PerformanceWithIncrementalMarking)
{
    uint32_t length = 1_KB;
    JSHandle<TaggedArray> rootArray =
        CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    for (uint32_t i = 0; i < length; i++) {
        auto array = CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->EnableNotifyIdle();
    heap->SetIdleTask(IdleTaskType::INCREMENTAL_MARK);  // incremental mark
    for (uint32_t i = 0; i < length; i++) {
        auto array = CreateTaggedArray(length, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
}
}  // namespace panda::test
