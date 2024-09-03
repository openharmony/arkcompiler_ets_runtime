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

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/debugger/dropframe_manager.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "assembler/assembly-parser.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::ecmascript::tooling {
class DropframeManagerFriendTest {
public:
    std::vector<std::tuple<JSHandle<JSTaggedValue>, uint16_t,
        JSHandle<JSTaggedValue>>> GetLexModifyRecordOfTopFrameTest ()
    {
        return manager.GetLexModifyRecordOfTopFrame();
    }

    void NewLexModifyRecordLevelTest()
    {
        manager.NewLexModifyRecordLevel();
    }

    uint32_t GetLexModifyRecordLevelTest()
    {
        return manager.GetLexModifyRecordLevel();
    }

    void RemoveLexModifyRecordOfTopFrameTest(JSThread *thread)
    {
        manager.RemoveLexModifyRecordOfTopFrame(thread);
    }

    void PopMethodInfoTest()
    {
        manager.PopMethodInfo();
    }

    void PushMethodInfoTest(std::tuple<JSPandaFile*, panda_file::File::EntityId> methodInfo)
    {
        manager.PushMethodInfo(methodInfo);
    }

    bool CheckExitMethodInfoTest(std::tuple<JSPandaFile*, panda_file::File::EntityId> methodInfo)
    {
        return manager.CheckExitMethodInfo(methodInfo);
    }

    void MergeLexModifyRecordOfTopFrameTest(JSThread *thread)
    {
        manager.MergeLexModifyRecordOfTopFrame(thread);
    }

private:
    DropframeManager manager;
};
}

namespace panda::test {
class DropframeManagerTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(DropframeManagerTest, GetLexModifyRecordOfTopFrameTest)
{
    DropframeManagerFriendTest manager;
    manager.GetLexModifyRecordOfTopFrameTest();
    uint32_t result = manager.GetLexModifyRecordLevelTest();
    EXPECT_FALSE(result);

    manager.NewLexModifyRecordLevelTest();
    manager.GetLexModifyRecordOfTopFrameTest();
    result = manager.GetLexModifyRecordLevelTest();
    EXPECT_TRUE(result);
}

HWTEST_F_L0(DropframeManagerTest, RemoveLexModifyRecordOfTopFrameTest)
{
    DropframeManagerFriendTest manager;
    JSThread *thread = ecmaVm->GetJSThread();
    uint32_t result = manager.GetLexModifyRecordLevelTest();
    manager.RemoveLexModifyRecordOfTopFrameTest(thread);
    EXPECT_FALSE(result);

    manager.NewLexModifyRecordLevelTest();
    result = manager.GetLexModifyRecordLevelTest();
    EXPECT_TRUE(result);

    manager.RemoveLexModifyRecordOfTopFrameTest(thread);
    result = manager.GetLexModifyRecordLevelTest();
    EXPECT_FALSE(result);
}

HWTEST_F_L0(DropframeManagerTest, PopMethodInfoTest)
{
    DropframeManagerFriendTest manager;
    Parser parser;
    const char *filename = "__PandaFileTranslatorTest2.pa";
    const char *data = R"(
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    const panda_file::File *testpf = pf.get()->GetPandaFile();
    CString descriptor = "example_descriptor";
    JSPandaFile* jspandaFilePtr = new JSPandaFile(testpf, descriptor);

    panda_file::File::EntityId entityId(42);
    std::tuple<JSPandaFile*, panda_file::File::EntityId> methodInfo(jspandaFilePtr, entityId);

    manager.PopMethodInfoTest();
    manager.PushMethodInfoTest(methodInfo);
    bool result = manager.CheckExitMethodInfoTest(methodInfo);
    EXPECT_EQ(result, true);

    manager.PopMethodInfoTest();
    result = manager.CheckExitMethodInfoTest(methodInfo);
    EXPECT_EQ(result, false);
}

HWTEST_F_L0(DropframeManagerTest, MergeLexModifyRecordOfTopFrameEmptyTest)
{
    DropframeManagerFriendTest manager;
    JSThread *thread = ecmaVm->GetJSThread();
    manager.MergeLexModifyRecordOfTopFrameTest(thread);
    uint32_t result = manager.GetLexModifyRecordLevelTest();
    EXPECT_FALSE(result);
}
}  // namespace panda::test
