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
#include "ecmascript/debugger/js_debugger.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "assembler/assembly-parser.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::tooling;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::ecmascript::tooling {
class JsDebuggerFriendTest {
public:
    explicit JsDebuggerFriendTest(const EcmaVM* vm) : jsDebugger_(vm) {}

    bool HandleStepTest(JSHandle<Method> method, uint32_t bcOffset)
    {
        return jsDebugger_.HandleStep(method, bcOffset);
    }

    bool HandleNativeOutTest()
    {
        return jsDebugger_.HandleNativeOut();
    }

    bool HandleBreakpointTest(JSHandle<Method> method, uint32_t bcOffset)
    {
        return jsDebugger_.HandleBreakpoint(method, bcOffset);
    }

    bool IsBreakpointCondSatisfiedTest(std::optional<JSBreakpoint> breakpoint) const
    {
        return jsDebugger_.IsBreakpointCondSatisfied(breakpoint);
    }

private:
    JSDebugger jsDebugger_;
};
}

namespace panda::test {
class JsDebuggerTest : public testing::Test {
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

HWTEST_F_L0(JsDebuggerTest, SetSmartBreakpointTest)
{
    JSDebugger debugger(ecmaVm);
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
    std::shared_ptr<JSPandaFile> jspandaFilePtr = std::make_shared<JSPandaFile>(testpf, descriptor);
    pfManager->AddJSPandaFile(jspandaFilePtr);
    JSPandaFile* jspandaFilePtrTest = new JSPandaFile(testpf, descriptor);
    panda_file::File::EntityId entityId(42);

    JSPtLocation location(jspandaFilePtrTest, entityId, 60, "sourceFile.js");
    bool result = debugger.RemoveBreakpoint(location);
    EXPECT_EQ(result, false);
    result = debugger.SetSmartBreakpoint(location);
    EXPECT_EQ(result, false);
}

HWTEST_F_L0(JsDebuggerTest, JsDebuggerHooksNullTest)
{
    JSHandle<Method> methodHandle;
    uint32_t bcOffsetTest = 42;
    JSHandle<JSTaggedValue> envHandle;
    JSDebugger debugger(ecmaVm);
    debugger.MethodEntry(methodHandle, envHandle);

    bool result = debugger.HandleDebuggerStmt(methodHandle, bcOffsetTest);
    EXPECT_EQ(result, false);

    JsDebuggerFriendTest debuggerFriend(ecmaVm);
    result = debuggerFriend.HandleNativeOutTest();
    EXPECT_EQ(result, false);

    result = debuggerFriend.HandleBreakpointTest(methodHandle, bcOffsetTest);
    EXPECT_EQ(result, false);

    result = debuggerFriend.HandleStepTest(methodHandle, bcOffsetTest);
    EXPECT_EQ(result, false);
}

HWTEST_F_L0(JsDebuggerTest, JsDebuggerBreakpointNullTest)
{
    JsDebuggerFriendTest debuggerFriend(ecmaVm);
    std::optional<ecmascript::tooling::JSBreakpoint> breakpoint;
    bool result = debuggerFriend.IsBreakpointCondSatisfiedTest(breakpoint);
    EXPECT_EQ(result, false);
}
}  // namespace panda::test