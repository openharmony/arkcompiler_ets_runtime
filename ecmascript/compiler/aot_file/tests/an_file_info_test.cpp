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

#include "gtest/gtest.h"
#include <sstream>

#include "ecmascript/compiler/aot_file/an_file_info.h"
#include "ecmascript/tests/test_helper.h"


using namespace panda::ecmascript;

namespace panda::test {
class AnFileInfoTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }
    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F_L0(AnFileInfoTest, DestroyAndDump)
{
    // This test constructs a simple AnFileInfo object and verifies:
    // 1. The Dump method can be called without crashing.
    // 2. The Destroy method can be called without crashing.
    AnFileInfo info;
    info.Dump();
    info.Destroy();
    SUCCEED();
}

}  // namespace panda::test