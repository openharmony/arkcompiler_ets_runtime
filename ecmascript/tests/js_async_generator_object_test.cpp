/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAsyncGeneratorObjectTest : public BaseTestWithScope<false> {
};

/**
 * @tc.name: GeneratorValidate
 * @tc.desc: Get the current status of the AsyncGenerator.
 * @tc.type: FUNC
 * @tc.require:issueI5JIGT
 */
HWTEST_F_L0(JSAsyncGeneratorObjectTest, AsyncGeneratorValidate_001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSAsyncGeneratorObject> asyncGenObj =
        factory->NewJSAsyncGeneratorObject(env->GetAsyncGeneratorFunctionFunction());
    JSAsyncGeneratorState state = asyncGenObj->GetAsyncGeneratorState();
    EXPECT_EQ(state, JSAsyncGeneratorState::UNDEFINED);
}
} // namespace panda::test