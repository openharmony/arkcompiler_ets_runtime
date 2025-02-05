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

#include "ecmascript/debugger/debugger_api.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class DebuggerApiTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(DebuggerApiTest, GetObjectHashCode1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc(thread, globalEnv->GetObjectFunction().GetObject<JSFunction>());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    ECMAObject::SetHash(thread, 87, JSHandle<ECMAObject>::Cast(obj));
    auto hash = tooling::DebuggerApi::GetObjectHashCode(thread->GetEcmaVM(), JSHandle<JSTaggedValue>::Cast(obj));
    EXPECT_TRUE(hash == 87);
}
}