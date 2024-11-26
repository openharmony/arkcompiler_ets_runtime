/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
constexpr int32_t INT_VALUE_0 = 0;
constexpr int32_t INT_VALUE_1 = 1;
constexpr int32_t INT_VALUE_2 = 2;

namespace panda::test {
class EcmaGlobalStorageTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(EcmaGlobalStorageTest, XRefGlobalNodes)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<TaggedArray> weakRefArray = vm->GetFactory()->NewTaggedArray(INT_VALUE_2, JSTaggedValue::Hole());
    uintptr_t xRefArrayAddress;
    vm->SetEnableForceGC(false);
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        JSHandle<JSTaggedValue> xRefArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        JSHandle<JSTaggedValue> normalArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_2));
        xRefArrayAddress = thread->NewXRefGlobalHandle(xRefArray.GetTaggedType());
        weakRefArray->Set(thread, INT_VALUE_0, xRefArray.GetTaggedValue().CreateAndGetWeakRef());
        weakRefArray->Set(thread, INT_VALUE_1, normalArray.GetTaggedValue().CreateAndGetWeakRef());
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(!weakRefArray->Get(INT_VALUE_0).IsUndefined());
    EXPECT_TRUE(weakRefArray->Get(INT_VALUE_1).IsUndefined());

    thread->DisposeXRefGlobalHandle(xRefArrayAddress);
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    vm->SetEnableForceGC(true);
    EXPECT_TRUE(weakRefArray->Get(INT_VALUE_0).IsUndefined());
}
};