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
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/patch/quick_fix_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/module_resolver.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
using Program = panda::ecmascript::Program;
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope, false, false, false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope, false);
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

    auto res = JSNApi::LoadPatch(instance, patchFileName, baseFileName);
    EXPECT_TRUE(res == PatchErrorCode::SUCCESS);

    Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(instance);
    result = JSNApi::IsQuickFixCausedException(instance, exception, patchFileName);
    EXPECT_FALSE(result);

    res = JSNApi::UnloadPatch(instance, patchFileName);
    EXPECT_TRUE(res == PatchErrorCode::SUCCESS);
}

HWTEST_F_L0(QuickFixTest, HotReload_MultiFile)
{
    std::string baseFileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    std::string patchFileName = QUICKFIX_ABC_PATH "multi_file/patch/merge.abc";

    JSNApi::EnableUserUncaughtErrorHandler(instance);

    JSNApi::SetBundle(instance, false);

    bool result = JSNApi::Execute(instance, baseFileName, "main");
    EXPECT_TRUE(result);

    auto res = JSNApi::LoadPatch(instance, patchFileName, baseFileName);
    EXPECT_TRUE(res == PatchErrorCode::SUCCESS);

    Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(instance);
    result = JSNApi::IsQuickFixCausedException(instance, exception, patchFileName);
    EXPECT_FALSE(result);

    res = JSNApi::UnloadPatch(instance, patchFileName);
    EXPECT_TRUE(res == PatchErrorCode::SUCCESS);
}

HWTEST_F_L0(QuickFixTest, HotReload_MultiHap)
{
    std::string baseFileName1 = QUICKFIX_ABC_PATH "single_file/base/index.abc";
    std::string patchFileName1 = QUICKFIX_ABC_PATH "single_file/patch/index.abc";

    std::string baseFileName2 = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    std::string patchFileName2 = QUICKFIX_ABC_PATH "multi_file/patch/merge.abc";

    JSNApi::SetBundle(instance, false);

    bool result = JSNApi::Execute(instance, baseFileName1, "index");
    EXPECT_TRUE(result);

    result = JSNApi::Execute(instance, baseFileName2, "main");
    EXPECT_TRUE(result);

    auto res = JSNApi::LoadPatch(instance, patchFileName1, baseFileName1);
    EXPECT_TRUE(res == PatchErrorCode::SUCCESS);

    res = JSNApi::LoadPatch(instance, patchFileName2, baseFileName2);
    EXPECT_TRUE(res == PatchErrorCode::SUCCESS);

    res = JSNApi::UnloadPatch(instance, patchFileName1);
    EXPECT_TRUE(res == PatchErrorCode::SUCCESS);

    res = JSNApi::UnloadPatch(instance, patchFileName2);
    EXPECT_TRUE(res == PatchErrorCode::SUCCESS);
}

HWTEST_F_L0(QuickFixTest, HotReload_Buffer)
{
    const char *baseFileName = "__base.pa";
    const char *baseData = R"(
        .function void foo() {}
    )";
    const char *patchFileName = "__patch.pa";
    const char *patchData = R"(
        .function void foo() {}
    )";

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(patchData);
    std::unique_ptr<const File> basePF = pandasm::AsmEmitter::Emit(res.Value());
    std::unique_ptr<const File> patchPF = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> baseFile = pfManager->NewJSPandaFile(basePF.release(), CString(baseFileName));
    std::shared_ptr<JSPandaFile> patchFile = pfManager->NewJSPandaFile(patchPF.release(), CString(patchFileName));
    pfManager->AddJSPandaFile(baseFile);
    pfManager->AddJSPandaFile(patchFile);

    auto result = JSNApi::LoadPatch(instance, patchFileName, (uint8_t *)patchData, sizeof(patchData),
                                    baseFileName, (uint8_t *)baseData, sizeof(baseData));
    EXPECT_FALSE(result == PatchErrorCode::SUCCESS);

    pfManager->RemoveJSPandaFile(baseFile.get());
    pfManager->RemoveJSPandaFile(patchFile.get());
}

HWTEST_F_L0(QuickFixTest, HotReload_Instantiate)
{
    ThreadManagedScope managedScope(thread);

    CString baseFileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    std::shared_ptr<JSPandaFile> baseFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFileName, "");
    EXPECT_TRUE(baseFile != nullptr);

    CString patchFileName = QUICKFIX_ABC_PATH "multi_file/patch/merge.abc";
    std::shared_ptr<JSPandaFile> patchFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, patchFileName, "");
    EXPECT_TRUE(patchFile != nullptr);

    CString replacedRecordName = "main";
    thread->SetStageOfHotReload(StageOfHotReload::BEGIN_EXECUTE_PATCHMAIN);

    JSHandle<JSTaggedValue> module = ModuleResolver::HostResolveImportedModuleForHotReload(thread,
        patchFileName, replacedRecordName);
    EXPECT_FALSE(module->IsHole());

    JSHandle<Program> program =
        JSPandaFileManager::GetInstance()->GenerateProgram(instance, patchFile.get(), replacedRecordName);
    EXPECT_FALSE(program.IsEmpty());

    SourceTextModule::Instantiate(thread, module);
    EXPECT_TRUE(JSHandle<SourceTextModule>::Cast(module)->GetStatus() == ModuleStatus::INSTANTIATED);

    thread->SetStageOfHotReload(StageOfHotReload::LOAD_END_EXECUTE_PATCHMAIN);
    JSHandle<SourceTextModule>::Cast(module)->SetStatus(ModuleStatus::UNINSTANTIATED);
    SourceTextModule::Instantiate(thread, module);
    EXPECT_TRUE(JSHandle<SourceTextModule>::Cast(module)->GetStatus() == ModuleStatus::INSTANTIATED);
}

bool QuickFixQueryFunc(
    std::string baseFileName, std::string &patchFileName, uint8_t ** patchBuffer, size_t &patchBufferSize)
{
    if (baseFileName != QUICKFIX_ABC_PATH "multi_file/base/merge.abc") {
        return false;
    }

    patchFileName = "__index.pa";
    const char data[] = R"(
        .function void foo() {}
    )";

    Parser parser;
    auto res = parser.Parse(data);
    uint8_t *bufferData = reinterpret_cast<uint8_t *>((const_cast<char *>(data)));
    *patchBuffer = bufferData;
    patchBufferSize = sizeof(data);
    return true;
}

HWTEST_F_L0(QuickFixTest, HotReload_RegisterQuickFixQueryFunc)
{
    std::string baseFileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    std::string patchFileName = "__index.pa";
    JSNApi::RegisterQuickFixQueryFunc(instance, QuickFixQueryFunc);

    std::shared_ptr<JSPandaFile> baseFile =
        JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, baseFileName.c_str(), "");
    EXPECT_TRUE(baseFile != nullptr);
    std::shared_ptr<JSPandaFile> patchFile =
        JSPandaFileManager::GetInstance()->FindJSPandaFile(patchFileName.c_str());
    EXPECT_TRUE(patchFile == nullptr);

    QuickFixManager *quickFixManager = instance->GetQuickFixManager();
    quickFixManager->LoadPatchIfNeeded(thread, baseFile.get());

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    pfManager->RemoveJSPandaFile(baseFile.get());
}
}  // namespace panda::test
