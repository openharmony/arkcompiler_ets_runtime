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
#include "libpandafile/class_data_accessor-inl.h"
#include "libziparchive/zip_archive.h"

#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class JSPandaFileManagerTest : public testing::Test {
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

HWTEST_F_L0(JSPandaFileManagerTest, GetInstance)
{
    JSPandaFileManager *manager = JSPandaFileManager::GetInstance();
    EXPECT_TRUE(manager != nullptr);
}

HWTEST_F_L0(JSPandaFileManagerTest, NewJSPandaFile)
{
    Parser parser;
    std::string fileName = "__JSPandaFileManagerTest.pa";
    const char *source = R"(
        .function void foo() {}
    )";
    auto res = parser.Parse(source, fileName);
    EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);

    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    JSPandaFile *pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName.c_str()));
    EXPECT_TRUE(pf != nullptr);

    auto expectFileName = pf->GetJSPandaFileDesc();
    EXPECT_STREQ(expectFileName.c_str(), "__JSPandaFileManagerTest.pa");
    remove(fileName.c_str());
    JSPandaFileManager::RemoveJSPandaFile(pf);
}

HWTEST_F_L0(JSPandaFileManagerTest, OpenJSPandaFile)
{
    const char *filename = "__JSPandaFileManagerTest.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    JSPandaFile *ojspf = pfManager->OpenJSPandaFile(filename);
    EXPECT_TRUE(ojspf == nullptr);

    Parser parser;
    auto res = parser.Parse(data);
    EXPECT_TRUE(pandasm::AsmEmitter::Emit(filename, res.Value()));

    ojspf = pfManager->OpenJSPandaFile(filename);
    EXPECT_TRUE(ojspf != nullptr);
    EXPECT_STREQ(ojspf->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest.pa");
    JSPandaFileManager::RemoveJSPandaFile(ojspf);

    remove(filename);
    ojspf = pfManager->OpenJSPandaFile(filename);
    EXPECT_TRUE(ojspf == nullptr);
}

HWTEST_F_L0(JSPandaFileManagerTest, Insert_Find_Remove_JSPandaFile)
{
    const char *filename1 = "__JSPandaFileManagerTest1.pa";
    const char *filename2 = "__JSPandaFileManagerTest2.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr1 = pandasm::AsmEmitter::Emit(res.Value());
    std::unique_ptr<const File> pfPtr2 = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFile *pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
    JSPandaFile *pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(filename2));
    pfManager->InsertJSPandaFile(pf1);
    pfManager->InsertJSPandaFile(pf2);
    const JSPandaFile *foundPf1 = pfManager->FindJSPandaFile(filename1);
    const JSPandaFile *foundPf2 = pfManager->FindJSPandaFile(filename2);
    EXPECT_STREQ(foundPf1->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest1.pa");
    EXPECT_STREQ(foundPf2->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest2.pa");

    pfManager->RemoveJSPandaFile((void *)pf1);
    pfManager->RemoveJSPandaFile((void *)pf2);
    const JSPandaFile *afterRemovePf1 = pfManager->FindJSPandaFile(filename1);
    const JSPandaFile *afterRemovePf2 = pfManager->FindJSPandaFile(filename2);
    EXPECT_EQ(afterRemovePf1, nullptr);
    EXPECT_EQ(afterRemovePf2, nullptr);
}

HWTEST_F_L0(JSPandaFileManagerTest, LoadJSPandaFile)
{
    const char *filename1 = "__JSPandaFileManagerTest1.pa";
    const char *filename2 = "__JSPandaFileManagerTest2.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr1 = pandasm::AsmEmitter::Emit(res.Value());
    std::unique_ptr<const File> pfPtr2 = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFile *pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
    JSPandaFile *pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(filename2));
    pfManager->InsertJSPandaFile(pf1);
    pfManager->InsertJSPandaFile(pf2);
    const JSPandaFile *loadedPf1 = pfManager->LoadJSPandaFile(thread, filename1, JSPandaFile::ENTRY_MAIN_FUNCTION);
    const JSPandaFile *loadedPf2 =
        pfManager->LoadJSPandaFile(thread, filename2, JSPandaFile::ENTRY_MAIN_FUNCTION, (void *)data, sizeof(data));
    EXPECT_TRUE(loadedPf1 != nullptr);
    EXPECT_TRUE(loadedPf2 != nullptr);
    EXPECT_STREQ(loadedPf1->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest1.pa");
    EXPECT_STREQ(loadedPf2->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest2.pa");

    pfManager->RemoveJSPandaFile((void *)pf1);
    pfManager->RemoveJSPandaFile((void *)pf2);
    pfManager->RemoveJSPandaFile((void *)loadedPf1);
    pfManager->RemoveJSPandaFile((void *)loadedPf2);
}

HWTEST_F_L0(JSPandaFileManagerTest, GenerateProgram)
{
    Parser parser;
    auto vm = thread->GetEcmaVM();
    const char *filename = "__JSPandaFileManagerTest.pa";
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    const char *data = R"(
        .function void foo() {}
    )";
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFile *pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    const File *file = pf->GetPandaFile();
    File::EntityId class_id = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, class_id);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    MethodLiteral method(pf, methodId[0]);
    pf->SetMethodLiteralToMap(&method);
    pfManager->InsertJSPandaFile(pf);

    JSHandle<ecmascript::Program> program = pfManager->GenerateProgram(vm, pf, JSPandaFile::ENTRY_FUNCTION_NAME);
    JSHandle<JSFunction> mainFunc(thread, program->GetMainFunction());
    JSHandle<JSTaggedValue> funcName = JSFunction::GetFunctionName(thread, JSHandle<JSFunctionBase>(mainFunc));
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(funcName)).ToCString().c_str(), "foo");

    pfManager->RemoveJSPandaFile((void *)pf);
}

HWTEST_F_L0(JSPandaFileManagerTest, GetJSPtExtractor)
{
    const char *filename = "__JSPandaFileManagerTest.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFile *pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    pfManager->InsertJSPandaFile(pf);
    DebugInfoExtractor *extractor = pfManager->GetJSPtExtractor(pf);
    EXPECT_TRUE(extractor != nullptr);

    pfManager->RemoveJSPandaFile((void *)pf);
}

HWTEST_F_L0(JSPandaFileManagerTest, EnumerateJSPandaFiles)
{
    const char *filename1 = "__JSPandaFileManagerTest3.pa";
    const char *filename2 = "__JSPandaFileManagerTest4.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr1 = pandasm::AsmEmitter::Emit(res.Value());
    std::unique_ptr<const File> pfPtr2 = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFile *pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
    JSPandaFile *pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(filename2));
    pfManager->InsertJSPandaFile(pf1);
    pfManager->InsertJSPandaFile(pf2);
    std::vector<CString> descList{};
    int count = 0;
    pfManager->EnumerateJSPandaFiles([&](const JSPandaFile *file) -> bool {
        auto desc = file->GetJSPandaFileDesc();
        descList.push_back(desc);
        count++;
        return true;
    });
    EXPECT_EQ(count, 2); // 2 : test number of files
    // Sort by the hash value of the element, the output is unordered
    EXPECT_STREQ(descList[0].c_str(), "__JSPandaFileManagerTest4.pa");
    EXPECT_STREQ(descList[1].c_str(), "__JSPandaFileManagerTest3.pa");

    pfManager->RemoveJSPandaFile((void *)pf1);
    pfManager->RemoveJSPandaFile((void *)pf2);
}
}  // namespace panda::test
