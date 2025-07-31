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

#include "common_interfaces/thread/thread_holder-inl.h"
#include "ecmascript/cross_vm/dynamic_object_accessor.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"
#include "thread/thread_holder.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class DynamicObjectAccessorTest : public BaseTestWithScope<false> {};

static JSHandle<JSObject> JSObjectCreate(JSThread *thread)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    auto jsFunc =  globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSHandle<JSTaggedValue> objFunc(thread, jsFunc);
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> jsObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    return jsObject;
}

HWTEST_F_L0(DynamicObjectAccessorTest, SetGetHasProperty)
{
    if (!g_isEnableCMCGC) {
        return;
    }
    ThreadHolder *threadHolder = thread->GetThreadHolder();
    JSHandle<JSObject> jsobject = JSObjectCreate(thread);
    EXPECT_TRUE(*jsobject != nullptr);

    char array[] = "x";
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII(array));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));

    DynamicObjectAccessor dynamicObjectAccessor;
    dynamicObjectAccessor.SetProperty(threadHolder, jsobject.GetTaggedValue().GetTaggedObject(), array,
                                      value.GetTaggedValue());
    EXPECT_EQ(dynamicObjectAccessor.GetProperty(threadHolder,
        jsobject.GetTaggedValue().GetTaggedObject(), array).GetInt(), 1);
    EXPECT_EQ(dynamicObjectAccessor.HasProperty(threadHolder,
        jsobject.GetTaggedValue().GetTaggedObject(), array), true);
}

HWTEST_F_L0(DynamicObjectAccessorTest, SetGetHasElement)
{
    if (!g_isEnableCMCGC) {
        return;
    }
    ThreadHolder *threadHolder = thread->GetThreadHolder();
    JSHandle<JSObject> jsobject = JSObjectCreate(thread);
    EXPECT_TRUE(*jsobject != nullptr);

    uint32_t index = 0;
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));

    DynamicObjectAccessor dynamicObjectAccessor;
    dynamicObjectAccessor.SetElementByIdx(threadHolder, jsobject.GetTaggedValue().GetTaggedObject(), index,
                                          value.GetTaggedValue());
    EXPECT_EQ(dynamicObjectAccessor.GetElementByIdx(threadHolder,
        jsobject.GetTaggedValue().GetTaggedObject(), index).GetInt(), 1);
    EXPECT_EQ(dynamicObjectAccessor.HasElementByIdx(threadHolder,
        jsobject.GetTaggedValue().GetTaggedObject(), index), true);
}
} // namespace panda::test