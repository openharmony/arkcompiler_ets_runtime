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
class BitHelperTest : public BaseTestWithScope<false> {
protected:
    template <class To, class From>
    inline To MemoryCast(const From &src) noexcept
    {
        static_assert(sizeof(To) == sizeof(From), "size of the types must be equal");
        To dst;
        if (memcpy_s(&dst, sizeof(To), &src, sizeof(From)) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
        return dst;
    }
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

HWTEST_F_L0(BitHelperTest, bit_cast)
{
    int8_t int8Value = -128;
    uint8_t uint8Value = 255;
    char char8Value = 127;
    EXPECT_EQ(bit_cast<uint8_t>(int8Value), MemoryCast<uint8_t>(int8Value));
    EXPECT_EQ(bit_cast<char>(int8Value), MemoryCast<char>(int8Value));
    EXPECT_EQ(bit_cast<int8_t>(uint8Value), MemoryCast<int8_t>(uint8Value));
    EXPECT_EQ(bit_cast<char>(uint8Value), MemoryCast<char>(uint8Value));
    EXPECT_EQ(bit_cast<int8_t>(char8Value), MemoryCast<int8_t>(char8Value));
    EXPECT_EQ(bit_cast<uint8_t>(char8Value), MemoryCast<uint8_t>(char8Value));

    int16_t int16Value = -32768;
    uint16_t uint16Value = 65535;
    char16_t char16Value = 32767;
    EXPECT_EQ(bit_cast<uint16_t>(int16Value), MemoryCast<uint16_t>(int16Value));
    EXPECT_EQ(bit_cast<char16_t>(int16Value), MemoryCast<char16_t>(int16Value));
    EXPECT_EQ(bit_cast<int16_t>(uint16Value), MemoryCast<int16_t>(uint16Value));
    EXPECT_EQ(bit_cast<char16_t>(uint16Value), MemoryCast<char16_t>(uint16Value));
    EXPECT_EQ(bit_cast<int16_t>(char16Value), MemoryCast<int16_t>(char16Value));
    EXPECT_EQ(bit_cast<uint16_t>(char16Value), MemoryCast<uint16_t>(char16Value));

    int32_t int32Value = -2147483648;
    uint32_t uint32Value = 2147483648;
    float floatValue = -13.569243f;
    char32_t char32Value = 2147483648;
    EXPECT_EQ(bit_cast<uint32_t>(int32Value), MemoryCast<uint32_t>(int32Value));
    EXPECT_EQ(bit_cast<float>(int32Value), MemoryCast<float>(int32Value));
    EXPECT_EQ(bit_cast<char32_t>(int32Value), MemoryCast<char32_t>(int32Value));
    EXPECT_EQ(bit_cast<int32_t>(uint32Value), MemoryCast<int32_t>(uint32Value));
    EXPECT_EQ(bit_cast<float>(uint32Value), MemoryCast<float>(uint32Value));
    EXPECT_EQ(bit_cast<char32_t>(uint32Value), MemoryCast<char32_t>(uint32Value));
    EXPECT_EQ(bit_cast<int32_t>(floatValue), MemoryCast<int32_t>(floatValue));
    EXPECT_EQ(bit_cast<uint32_t>(floatValue), MemoryCast<uint32_t>(floatValue));
    EXPECT_EQ(bit_cast<char32_t>(floatValue), MemoryCast<char32_t>(floatValue));
    EXPECT_EQ(bit_cast<int32_t>(char32Value), MemoryCast<int32_t>(char32Value));
    EXPECT_EQ(bit_cast<uint32_t>(char32Value), MemoryCast<uint32_t>(char32Value));
    EXPECT_EQ(bit_cast<float>(char32Value), MemoryCast<float>(char32Value));

    int64_t int64Value = -9223372036854775807LL;
    uint64_t uint64Value = 9223372036854775808ULL;
    double doubleValue = -257.93458301390463;
    EXPECT_EQ(bit_cast<uint64_t>(int64Value), MemoryCast<uint64_t>(int64Value));
    EXPECT_EQ(bit_cast<double>(int64Value), MemoryCast<double>(int64Value));
    EXPECT_EQ(bit_cast<int64_t>(uint64Value), MemoryCast<int64_t>(uint64Value));
    EXPECT_EQ(bit_cast<double>(uint64Value), MemoryCast<double>(uint64Value));
    EXPECT_EQ(bit_cast<int64_t>(doubleValue), MemoryCast<int64_t>(doubleValue));
    EXPECT_EQ(bit_cast<uint64_t>(doubleValue), MemoryCast<uint64_t>(doubleValue));
}
}  // namespace panda::test
