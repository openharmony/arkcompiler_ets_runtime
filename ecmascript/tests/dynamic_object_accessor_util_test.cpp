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

#include "common_interfaces/objects/dynamic_object_accessor_util.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class DynamicObjectAccessorUtilTest : public BaseTestWithScope<false> {};

static JSHandle<JSObject> JSObjectCreate(JSThread *thread)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    auto jsFunc =  globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSHandle<JSTaggedValue> objFunc(thread, jsFunc);
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> jsObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    return jsObject;
}

HWTEST_F_L0(DynamicObjectAccessorUtilTest, SetGetProperty01)
{
    JSHandle<JSObject> jsobject = JSObjectCreate(thread);
    EXPECT_TRUE(*jsobject != nullptr);

    char key[] = "name";
    int value = 123;
    JSTaggedValue taggedValue(value);

    auto setResult = common::DynamicObjectAccessorUtil::SetProperty(jsobject.GetTaggedValue().GetTaggedObject(), key,
                                                                    taggedValue.GetRawData());
    EXPECT_EQ(setResult, true);
    auto taggedType = common::DynamicObjectAccessorUtil::GetProperty(jsobject.GetTaggedValue().GetTaggedObject(), key);
    EXPECT_EQ(JSTaggedValue(*taggedType).GetInt(), value);
}
} // namespace panda::test