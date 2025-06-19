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
#include "ecmascript/compiler/aot_file/binary_buffer_parser.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class BinaryBufferParserTest : public testing::Test {
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

HWTEST_F_L0(BinaryBufferParserTest, BasicParse)
{
    // This test verifies the basic functionality of BinaryBufferParser:
    // 1. ParseBuffer copies the first 4 bytes from the buffer to dst.
    // 2. ParseBuffer with offset copies bytes 5 and 6 to dst2.
    // 3. ParseBuffer with src pointer copies bytes 7 and 8 to dst3.
    // 4. GetAddr returns the original buffer pointer.
    uint8_t buf[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    BinaryBufferParser parser(buf, 8);

    uint8_t dst[4] = {0};
    parser.ParseBuffer(dst, 4);
    ASSERT_EQ(dst[0], 1);
    ASSERT_EQ(dst[1], 2);
    ASSERT_EQ(dst[2], 3);
    ASSERT_EQ(dst[3], 4);

    // Use ParseBuffer with offset
    uint8_t dst2[2] = {0};
    parser.ParseBuffer(dst2, 2, 4);
    ASSERT_EQ(dst2[0], 5);
    ASSERT_EQ(dst2[1], 6);

    // Use ParseBuffer with src pointer
    uint8_t dst3[2] = {0};
    parser.ParseBuffer(dst3, 2, buf+6);
    ASSERT_EQ(dst3[0], 7);
    ASSERT_EQ(dst3[1], 8);

    // GetAddr should return the original buffer pointer
    ASSERT_EQ(parser.GetAddr(), buf);
}

} // namespace panda::test
