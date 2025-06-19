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

#include "common_components/tests/test_helper.h"
#include "common_components/mutator/satb_buffer.h"

using namespace common;

namespace common::test {
class SatbBufferTest : public BaseTestWithScope {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SatbBufferTest, ShouldEnqueue_Test1)
{
    BaseObject* obj = nullptr;
    bool ret = SatbBuffer::Instance().ShouldEnqueue(obj);
    EXPECT_TRUE(ret == false);
}
}  // namespace common::test
