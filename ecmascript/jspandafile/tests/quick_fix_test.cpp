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

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda::ecmascript;

namespace panda::test {
class QuickFixTest : public testing::Test {
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

HWTEST_F_L0(QuickFixTest, HotReload_SingleFile)
{
    std::string baseFileName = QUICKFIX_ABC_PATH "single_file/base/index.abc";
    std::string patchFileName = QUICKFIX_ABC_PATH "single_file/patch/index.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool result = JSNApi::Execute(instance, baseFileName, "index");
    EXPECT_TRUE(result);

    result = JSNApi::LoadPatch(instance, patchFileName, baseFileName);
    EXPECT_TRUE(result);

    Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(instance);
    result = JSNApi::IsQuickFixCausedException(instance, exception, patchFileName);
    EXPECT_FALSE(result);

    result = JSNApi::UnloadPatch(instance, patchFileName);
    EXPECT_TRUE(result);
}

HWTEST_F_L0(QuickFixTest, HotReload_MultiFile)
{
    std::string baseFileName = QUICKFIX_ABC_PATH "multi_file/base/index.abc";
    std::string patchFileName = QUICKFIX_ABC_PATH "multi_file/patch/index.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    bool result = JSNApi::Execute(instance, baseFileName, "index");
    EXPECT_TRUE(result);

    result = JSNApi::LoadPatch(instance, patchFileName, baseFileName);
    EXPECT_TRUE(result);

    Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(instance);
    result = JSNApi::IsQuickFixCausedException(instance, exception, patchFileName);
    EXPECT_FALSE(result);

    result = JSNApi::UnloadPatch(instance, patchFileName);
    EXPECT_TRUE(result);
}
}  // namespace panda::test
