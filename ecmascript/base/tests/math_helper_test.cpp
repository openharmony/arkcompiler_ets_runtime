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

#include "ecmascript/base/math_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class MathHelperTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(MathHelperTest, GetIntLog2_001)
{
    const uint32_t commonInput = static_cast<uint32_t>(0b111111111) << 13; // 13 : left shift digit
    const uint32_t maxInput = std::numeric_limits<uint32_t>::max();
    EXPECT_EQ(MathHelper::GetIntLog2(commonInput), 13U);
    EXPECT_EQ(MathHelper::GetIntLog2(maxInput), 0U);
}

HWTEST_F_L0(MathHelperTest, GetIntLog2_002)
{
    const uint64_t commonInput = static_cast<uint64_t>(0b111111111) << 53; // 53 : left shift digit
    const uint64_t maxInput = std::numeric_limits<uint64_t>::max();
    EXPECT_EQ(MathHelper::GetIntLog2(commonInput), 53U);
    EXPECT_EQ(MathHelper::GetIntLog2(maxInput), 0U);
}

HWTEST_F_L0(MathHelperTest, Asinh)
{
    EXPECT_EQ(MathHelper::Asinh(1), 0.88137358701954302523260932497979);
    EXPECT_EQ(MathHelper::Asinh(+0), +0.0);
    EXPECT_EQ(MathHelper::Asinh(-0), -0.0);
    EXPECT_EQ(MathHelper::Asinh(-1), -0.88137358701954302523260932497979);

    double nanResult = MathHelper::Asinh(std::numeric_limits<double>::signaling_NaN());
    EXPECT_TRUE(std::isnan(nanResult));
}

HWTEST_F_L0(MathHelperTest, Atanh)
{
    EXPECT_EQ(MathHelper::Atanh(0), 0);
    EXPECT_EQ(MathHelper::Atanh(0.5), std::atanh(0.5));

    double infResult = MathHelper::Atanh(-1); // limit value
    EXPECT_TRUE(std::isinf(infResult));

    double nanResult = MathHelper::Atanh(2); // out of input range
    EXPECT_TRUE(std::isnan(nanResult));
}
}  // namespace panda::test
