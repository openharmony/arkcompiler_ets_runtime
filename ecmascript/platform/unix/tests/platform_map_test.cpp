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

#include "ecmascript/platform/map.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest.h"

namespace panda::test {
using namespace panda::ecmascript;

class PlatformMapTest : public BaseTestWithOutScope {
};

HWTEST_F_L0(PlatformMapTest, GetPageTagString_HEAP_ThreadIdZero)
{
    // 测试目的：验证GetPageTagString函数在HEAP类型且threadId为0时，返回正确的标签字符串
    const std::string spaceName = "TestSpace";
    const uint32_t threadId = 0;
    std::string result = GetPageTagString(PageTagType::HEAP, spaceName, threadId);
    EXPECT_EQ(result, std::string("ArkTS Heap").append(spaceName));
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_HEAP_ThreadIdNonZero)
{
    // 测试目的：验证GetPageTagString函数在HEAP类型且threadId非0时，返回包含threadId的标签字符串
    const std::string spaceName = "TestSpace";
    const uint32_t threadId = 123;
    std::string result = GetPageTagString(PageTagType::HEAP, spaceName, threadId);
    EXPECT_EQ(result, std::string("ArkTS Heap").append(std::to_string(threadId)).append(spaceName));
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_MACHINE_CODE_ThreadIdZero)
{
    // 测试目的：验证GetPageTagString函数在MACHINE_CODE类型且threadId为0时，返回正确的标签字符串
    const std::string spaceName = "TestSpace";
    const uint32_t threadId = 0;
    std::string result = GetPageTagString(PageTagType::MACHINE_CODE, spaceName, threadId);
    EXPECT_EQ(result, "ArkTS Code");
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_MACHINE_CODE_ThreadIdNonZero)
{
    // 测试目的：验证GetPageTagString函数在MACHINE_CODE类型且threadId非0时，返回包含threadId的标签字符串
    const std::string spaceName = "TestSpace";
    const uint32_t threadId = 456;
    std::string result = GetPageTagString(PageTagType::MACHINE_CODE, spaceName, threadId);
    EXPECT_EQ(result, std::string("ArkTS Code").append(std::to_string(threadId)));
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_MEMPOOL_CACHE)
{
    // 测试目的：验证GetPageTagString函数在MEMPOOL_CACHE类型时，返回正确的标签字符串
    const std::string spaceName = "TestSpace";
    const uint32_t threadId = 789;
    std::string result = GetPageTagString(PageTagType::MEMPOOL_CACHE, spaceName, threadId);
    EXPECT_EQ(result, "ArkTS MemPoolCache");
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_METHOD_LITERAL)
{
    // 测试目的：验证GetPageTagString函数在METHOD_LITERAL类型时，返回正确的标签字符串
    const std::string spaceName = "TestSpace";
    const uint32_t threadId = 999;
    std::string result = GetPageTagString(PageTagType::METHOD_LITERAL, spaceName, threadId);
    EXPECT_EQ(result, "ArkTS MethodLiteral");
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_HEAP_SingleParam)
{
    // 测试目的：验证单参数GetPageTagString函数在HEAP类型时，返回正确的标签字符串
    const char *result = GetPageTagString(PageTagType::HEAP);
    EXPECT_STREQ(result, "ArkTS Heap");
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_MACHINE_CODE_SingleParam)
{
    // 测试目的：验证单参数GetPageTagString函数在MACHINE_CODE类型时，返回正确的标签字符串
    const char *result = GetPageTagString(PageTagType::MACHINE_CODE);
    EXPECT_STREQ(result, "ArkTS Code");
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_MEMPOOL_CACHE_SingleParam)
{
    // 测试目的：验证单参数GetPageTagString函数在MEMPOOL_CACHE类型时，返回正确的标签字符串
    const char *result = GetPageTagString(PageTagType::MEMPOOL_CACHE);
    EXPECT_STREQ(result, "ArkTS MemPoolCache");
}

HWTEST_F_L0(PlatformMapTest, GetPageTagString_METHOD_LITERAL_SingleParam)
{
    // 测试目的：验证单参数GetPageTagString函数在METHOD_LITERAL类型时，返回正确的标签字符串
    const char *result = GetPageTagString(PageTagType::METHOD_LITERAL);
    EXPECT_STREQ(result, "ArkTS MethodLiteral");
}

} // namespace panda::test
