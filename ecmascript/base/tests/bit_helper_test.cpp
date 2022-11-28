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

#include "ecmascript/base/bit_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class BitHelperTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(BitHelperTest, CountLeadingZeros_CountTrailingZeros)
{
    uint8_t uint8MaxValue = std::numeric_limits<uint8_t>::max();
    uint8_t uint8MinValue = std::numeric_limits<uint8_t>::min();
    EXPECT_EQ(CountLeadingZeros<uint8_t>(uint8MaxValue), 0U);
    EXPECT_EQ(CountLeadingZeros<uint8_t>(uint8MinValue), 8U);
    EXPECT_EQ(CountTrailingZeros<uint8_t>(uint8MaxValue), 0U);
    EXPECT_EQ(CountTrailingZeros<uint8_t>(uint8MinValue), 8U);

    uint16_t uint16MaxValue = std::numeric_limits<uint16_t>::max();
    uint16_t uint16MinValue = std::numeric_limits<uint16_t>::min();
    EXPECT_EQ(CountLeadingZeros<uint16_t>(uint16MaxValue), 0U);
    EXPECT_EQ(CountLeadingZeros<uint16_t>(uint16MinValue), 16U);
    EXPECT_EQ(CountTrailingZeros<uint16_t>(uint16MaxValue), 0U);
    EXPECT_EQ(CountTrailingZeros<uint16_t>(uint16MinValue), 16U);

    uint32_t uint32MaxValue = std::numeric_limits<uint32_t>::max();
    uint32_t uint32MinValue = std::numeric_limits<uint32_t>::min();
    EXPECT_EQ(CountLeadingZeros<uint32_t>(uint32MaxValue), 0U);
    EXPECT_EQ(CountLeadingZeros<uint32_t>(uint32MinValue), 32U);
    EXPECT_EQ(CountTrailingZeros<uint32_t>(uint32MaxValue), 0U);
    EXPECT_EQ(CountTrailingZeros<uint32_t>(uint32MinValue), 32U);
    
    uint64_t uint64MaxValue = std::numeric_limits<uint64_t>::max();
    uint64_t uint64MinValue = std::numeric_limits<uint64_t>::min();
    EXPECT_EQ(CountLeadingZeros<uint64_t>(uint64MaxValue), 0U);
    EXPECT_EQ(CountLeadingZeros<uint64_t>(uint64MinValue), 64U);
    EXPECT_EQ(CountTrailingZeros<uint64_t>(uint64MaxValue), 0U);
    EXPECT_EQ(CountTrailingZeros<uint64_t>(uint64MinValue), 64U);
}

HWTEST_F_L0(BitHelperTest, CountLeadingZeros32_CountLeadingOnes32)
{
    uint32_t uint32MaxValue = std::numeric_limits<uint32_t>::max();
    uint32_t uint32CommonValue1 = std::numeric_limits<uint32_t>::max() >> 1;
    uint32_t uint32CommonValue2 = std::numeric_limits<uint32_t>::max() >> 31; // 31 : right shift digit
    uint32_t uint32MinValue = std::numeric_limits<uint32_t>::min();
    EXPECT_EQ(CountLeadingZeros32(uint32MaxValue), 0U);
    EXPECT_EQ(CountLeadingZeros32(uint32CommonValue1), 1U);
    EXPECT_EQ(CountLeadingZeros32(uint32CommonValue2), 31U);
    EXPECT_EQ(CountLeadingZeros32(uint32MinValue), 32U);
    EXPECT_EQ(CountLeadingOnes32(uint32MaxValue), 32U);
    EXPECT_EQ(CountLeadingOnes32(uint32CommonValue1), 0U);
    EXPECT_EQ(CountLeadingOnes32(uint32CommonValue2), 0U);
    EXPECT_EQ(CountLeadingOnes32(uint32MinValue), 0U);
}

HWTEST_F_L0(BitHelperTest, CountLeadingZeros64_CountLeadingOnes64)
{
    uint64_t uint64MaxValue = std::numeric_limits<uint64_t>::max();
    uint64_t uint64CommonValue1 = std::numeric_limits<uint64_t>::max() >> 1;
    uint64_t uint64CommonValue2 = std::numeric_limits<uint64_t>::max() >> 63; // 63 : right shift digit
    uint64_t uint64MinValue = std::numeric_limits<uint64_t>::min();
    EXPECT_EQ(CountLeadingZeros64(uint64MaxValue), 0U);
    EXPECT_EQ(CountLeadingZeros64(uint64CommonValue1), 1U);
    EXPECT_EQ(CountLeadingZeros64(uint64CommonValue2), 63U);
    EXPECT_EQ(CountLeadingZeros64(uint64MinValue), 64U);
    EXPECT_EQ(CountLeadingOnes64(uint64MaxValue), 64U);
    EXPECT_EQ(CountLeadingOnes64(uint64CommonValue1), 0U);
    EXPECT_EQ(CountLeadingOnes64(uint64CommonValue2), 0U);
    EXPECT_EQ(CountLeadingOnes64(uint64MinValue), 0U);
}
}  // namespace panda::test
