/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env_constants.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class GlobalEnvConstantsTest : public BaseTestWithScope<false> {};

/**
 * @tc.name: IsEmptyLayoutReadOnly
 * @tc.desc: Check whether the empty layout info is empty and readonly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GlobalEnvConstantsTest, IsEmptyLayoutReadOnly)
{
    JSHandle<LayoutInfo> emptyLayoutInfo =
        JSHandle<LayoutInfo>::Cast(thread->GlobalConstants()->GetHandledEmptyLayoutInfo());
    // Check is empty
    EXPECT_EQ(emptyLayoutInfo->NumberOfElements(), 0);
    // Check can't addKey
    EXPECT_EQ(emptyLayoutInfo->GetPropertiesCapacity(), 0);
}

}  // namespace panda::test
