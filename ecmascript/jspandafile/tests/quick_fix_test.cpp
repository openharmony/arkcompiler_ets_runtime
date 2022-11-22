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

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"

#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/quick_fix_manager.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/napi/include/jsnapi.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

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

    JSNApi::SetBundle(instance, false);

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
    std::string baseFileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    std::string patchFileName = QUICKFIX_ABC_PATH "multi_file/patch/merge.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    JSNApi::SetBundle(instance, false);

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

HWTEST_F_L0(QuickFixTest, HotReload_Buffer)
{
    const char *baseFileName = "__base.pa";
    const char *patchFileName = "__patch.pa";
    const char *data = R"(
        .function void foo() {}
    )";

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> basePF = pandasm::AsmEmitter::Emit(res.Value());
    std::unique_ptr<const File> patchPF = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFile *baseFile = pfManager->NewJSPandaFile(basePF.release(), CString(baseFileName));
    JSPandaFile *patchFile = pfManager->NewJSPandaFile(patchPF.release(), CString(patchFileName));
    pfManager->InsertJSPandaFile(baseFile);
    pfManager->InsertJSPandaFile(patchFile);

    bool result = JSNApi::LoadPatch(instance, patchFileName, (void *)data, sizeof(data), baseFileName);
    EXPECT_FALSE(result);

    pfManager->RemoveJSPandaFile((void *)baseFile);
    pfManager->RemoveJSPandaFile((void *)patchFile);
}

bool QuickFixQueryFunc(
    std::string baseFileName, std::string &patchFileName, void ** patchBuffer, size_t patchBufferSize)
{
    if (baseFileName != QUICKFIX_ABC_PATH "multi_file/base/merge.abc") {
        return false;
    }

    patchFileName = "__index.pa";
    const char *data = R"(
        .function void foo() {}
    )";

    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> patchPF = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    JSPandaFile *patchFile = pfManager->NewJSPandaFile(patchPF.release(), patchFileName.c_str());
    pfManager->InsertJSPandaFile(patchFile);
    patchBuffer = (void **) data;
    patchBufferSize = sizeof(data);
    return true;
}

HWTEST_F_L0(QuickFixTest, HotReload_RegisterQuickFixQueryFunc)
{
    std::string baseFileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    std::string patchFileName = "__index.pa";
    JSNApi::RegisterQuickFixQueryFunc(instance, QuickFixQueryFunc);

    QuickFixManager *quickFixManager = instance->GetQuickFixManager();
    quickFixManager->LoadPatchIfNeeded(thread, baseFileName);

    const JSPandaFile *baseFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(baseFileName.c_str());
    const JSPandaFile *patchFile = JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName.c_str());
    EXPECT_TRUE(baseFile != nullptr);
    EXPECT_TRUE(patchFile != nullptr);

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    pfManager->RemoveJSPandaFile((void *)baseFile);
    pfManager->RemoveJSPandaFile((void *)patchFile);
}
}  // namespace panda::test
