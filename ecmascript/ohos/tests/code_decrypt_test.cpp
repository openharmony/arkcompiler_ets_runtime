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

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "gtest/gtest.h"

#include "ecmascript/ohos/code_decrypt.h"
#include "ecmascript/tests/test_helper.h"

#if defined(CODE_ENCRYPTION_ENABLE)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::ohos;

namespace panda::test {
#if defined(CODE_ENCRYPTION_ENABLE)

class CodeDecryptTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase";
    }
};

// DecryptSetKey
HWTEST_F_L0(CodeDecryptTest, DecryptSetKeyTest)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    int srcAppId = 12345;
    int result = DecryptSetKey(pipefd[0], srcAppId);
    close(pipefd[0]);
    close(pipefd[1]);
}

// DecrypRemoveKey
HWTEST_F_L0(CodeDecryptTest, DecrypRemoveKeyTest)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    int srcAppId = 54321;
    int result = DecrypRemoveKey(pipefd[0], srcAppId);
    close(pipefd[0]);
    close(pipefd[1]);
}

// DecryptAssociateKey
HWTEST_F_L0(CodeDecryptTest, DecryptAssociateKeyTest)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    int dstAppId = 11111;
    int srcAppId = 22222;
    int result = DecryptAssociateKey(pipefd[0], dstAppId, srcAppId);
    close(pipefd[0]);
    close(pipefd[1]);
}

// DecrypRemoveAssociateKey
HWTEST_F_L0(CodeDecryptTest, DecrypRemoveAssociateKeyTest)
{
    int pipefd[2];
    ASSERT_EQ(pipe(pipefd), 0);
    int dstAppId = 33333;
    int srcAppId = 44444;
    int result = DecrypRemoveAssociateKey(pipefd[0], dstAppId, srcAppId);
    close(pipefd[0]);
    close(pipefd[1]);
}

// 使用无效文件描述符进行测试
HWTEST_F_L0(CodeDecryptTest, InvalidFileDescriptorTest)
{
    int srcAppId = 12345;
    int result = DecryptSetKey(-1, srcAppId);
    ASSERT_EQ(result, -1);
    result = DecrypRemoveKey(-1, srcAppId);
    ASSERT_EQ(result, -1);
    result = DecryptAssociateKey(-1, 11111, srcAppId);
    ASSERT_EQ(result, -1);
    result = DecrypRemoveAssociateKey(-1, 11111, srcAppId);
    ASSERT_EQ(result, -1);
}

#else
class CodeDecryptTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase - CODE_ENCRYPTION_ENABLE is undefined";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownTestCase - CODE_ENCRYPTION_ENABLE is undefined";
    }
};

HWTEST_F_L0(CodeDecryptTest, DecryptFunctionsNotDefinedTest)
{
    GTEST_LOG_(INFO) << "CODE_ENCRYPTION_ENABLE is undefined";
    ASSERT_TRUE(true);
}

#endif
}  // namespace panda::test
