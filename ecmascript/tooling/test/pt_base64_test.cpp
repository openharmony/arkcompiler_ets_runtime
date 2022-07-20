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

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/tooling/base/pt_base64.h"

using namespace panda::ecmascript::tooling;

namespace panda::test {
class PtBase64Test : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

HWTEST_F_L0(PtBase64Test, ShortTextTest)
{
    std::string dec = "Hello";
    std::string enc;
    std::string des;
    uint32_t len = PtBase64::Encode(dec, enc);
    EXPECT_EQ(static_cast<int>(len), 8);
    EXPECT_EQ(enc, "SGVsbG8=");

    len = PtBase64::Decode(enc, des);
    EXPECT_EQ(static_cast<int>(len), 5);
    EXPECT_EQ(des, "Hello");
}

HWTEST_F_L0(PtBase64Test, LongTextTest)
{
    std::string enc = "SWYgeW91IGNhbiBzZWUgdGhpcyBtZXNzYWdlLCBpdCBtZWFucyB0aGF0IFB0QmFzZTY0RGVjb2RlIHdvcmtzIHdlbGw=";
    std::string dec = "If you can see this message, it means that PtBase64Decode works well";
    std::string des;

    uint32_t len = PtBase64::Encode(dec, des);
    EXPECT_EQ(static_cast<int>(len), 92);
    EXPECT_EQ(enc, des);

    len = PtBase64::Decode(enc, des);
    EXPECT_EQ(static_cast<int>(len), 68);
    EXPECT_EQ(des, dec);
}

HWTEST_F_L0(PtBase64Test, ErrorTextTest)
{
    std::string src = "SGVsbG8==";
    std::string des;
    PtBase64 ptBase64;
    uint32_t len = ptBase64.Decode(src, des);
    EXPECT_EQ(static_cast<int>(len), 0);
    EXPECT_EQ(des, "");

    len = PtBase64::Encode("", des);
    EXPECT_EQ(static_cast<int>(len), 0);
    EXPECT_EQ(des, "");
}
}