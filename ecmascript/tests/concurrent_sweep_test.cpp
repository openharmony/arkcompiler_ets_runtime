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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"

using namespace panda::ecmascript;

namespace panda::test {
class ConcurrentSweepTest : public BaseTestWithScope<false> {
};

TEST_F(ConcurrentSweepTest, ConcurrentSweep)
{
    const uint8_t *utf8 = reinterpret_cast<const uint8_t *>("test");
    JSHandle<EcmaString> test1(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, utf8, 4, true)); // 4 : utf8 encoding length
    if (instance->IsInitialized()) {
        instance->CollectGarbage(ecmascript::TriggerGCType::OLD_GC);
    }
    JSHandle<EcmaString> test2(thread,
        EcmaStringAccessor::CreateFromUtf8(instance, utf8, 4, true)); // 4 : utf8 encoding length
    ASSERT_EQ(EcmaStringAccessor(test1).GetLength(), 4U);
    ASSERT_NE(test1.GetTaggedValue().GetTaggedObject(), test2.GetTaggedValue().GetTaggedObject());
}
}  // namespace panda::test
