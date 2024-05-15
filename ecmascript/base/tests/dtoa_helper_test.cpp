/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/base/dtoa_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

#define TEST_DTOA(d, str, buffer, n, k, e1, e2)         \
        DtoaHelper::Dtoa(d, buffer, &(n), &(k));        \
        EXPECT_STREQ(str, buffer);                      \
        EXPECT_EQ(n, e1);                               \
        EXPECT_EQ(k, e2)

namespace panda::test {
class DtoaHelperTest : public BaseTestWithScope<false> {
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

HWTEST_F_L0(DtoaHelperTest, DoubleToAscii)
{
    char buffer1[128] = {};
    int n1; //decimal_point
    int k1; //length
    TEST_DTOA(1.2345, "12345", buffer1, n1, k1, 1, 5);

    char buffer2[128] = {};
    int n2;
    int k2;
    TEST_DTOA(1.2345678, "12345678", buffer2, n2, k2, 1, 8);

    char buffer3[128] = {};
    int n3;
    int k3;
    TEST_DTOA(1e30, "1", buffer3, n3, k3, 31, 1);

    char buffer4[128] = {};
    int n4;
    int k4;
    TEST_DTOA(79.39773355813419, "7939773355813419", buffer4, n4, k4, 2, 16);

    char buffer5[128] = {};
    int n5;
    int k5;
    TEST_DTOA(0.0000001, "1", buffer5, n5, k5, -6, 1);

    char buffer6[128] = {};
    int n6;
    int k6;
    TEST_DTOA(1.234567890123456e30, "1234567890123456", buffer6, n6, k6, 31, 16);

    char buffer7[128] = {};
    int n7;
    int k7;
    TEST_DTOA(2.225073858507201e-308, "2225073858507201", buffer7, n7, k7, -307, 16); // Max subnormal positive double

    char buffer8[128] = {};
    int n8;
    int k8;
    TEST_DTOA(2.2250738585072014e-308, "22250738585072014", buffer8, n8, k8, -307, 17); // Min normal positive double

    char buffer9[128] = {};
    int n9;
    int k9;
    TEST_DTOA(1.7976931348623157e308, "17976931348623157", buffer9, n9, k9, 309, 17); // Max double

    char buffera[128] = {};
    int na;
    int ka;
    TEST_DTOA(5e-301, "5", buffera, na, ka, -300, 1);
}
}  // namespace panda::test
