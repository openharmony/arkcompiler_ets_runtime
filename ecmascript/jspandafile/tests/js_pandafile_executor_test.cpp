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

#include "ecmascript/global_env.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class JSPandaFileExecutorTest : public testing::Test {
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

HWTEST_F_L0(JSPandaFileExecutorTest, Execute)
{
    const char *fileName = "__JSPandaFileExecutorTest1.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    MethodLiteral *method = new MethodLiteral(methodId[0]);
    method->Initialize(pf.get());
    pf->SetMethodLiteralToMap(method);
    pfManager->AddJSPandaFile(pf);
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::Execute(thread, pf.get(), JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_TRUE(result);

    pfManager->RemoveJSPandaFile(pf.get());
}

HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteFromFile)
{
    const char *fileName = "__JSPandaFileExecutorTest2.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    MethodLiteral *method = new MethodLiteral(methodId[0]);
    method->Initialize(pf.get());
    pf->SetMethodLiteralToMap(method);
    pfManager->AddJSPandaFile(pf);
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::ExecuteFromAbcFile(thread, CString(fileName), JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_TRUE(result);

    pfManager->RemoveJSPandaFile(pf.get());
    std::shared_ptr<JSPandaFile> foundPf = pfManager->FindJSPandaFile(fileName);
    EXPECT_TRUE(foundPf == nullptr);
}

HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteFromBuffer)
{
    const char *fileName = "__JSPandaFileExecutorTest3.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    MethodLiteral *method = new MethodLiteral(methodId[0]);
    method->Initialize(pf.get());
    pf->SetMethodLiteralToMap(method);
    pfManager->AddJSPandaFile(pf);
    Expected<JSTaggedValue, bool> result = JSPandaFileExecutor::ExecuteFromBuffer(
        thread, (void *)data, sizeof(data), JSPandaFile::ENTRY_MAIN_FUNCTION, CString(fileName));
    EXPECT_TRUE(result);

    pfManager->RemoveJSPandaFile(pf.get());
    std::shared_ptr<JSPandaFile> foundPf = pfManager->FindJSPandaFile(fileName);
    EXPECT_TRUE(foundPf == nullptr);
}

HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteAbcFileWithSingletonPatternFlag)
{
    int result = JSPandaFileExecutor::ExecuteAbcFileWithSingletonPatternFlag(thread, "",
        "entry", JSPandaFile::ENTRY_MAIN_FUNCTION, false);
    EXPECT_EQ(result, JSPandaFileExecutor::ROUTE_URI_ERROR);

    int result1 = JSPandaFileExecutor::ExecuteAbcFileWithSingletonPatternFlag(thread, "com.application.example",
        "entry", JSPandaFile::ENTRY_MAIN_FUNCTION, false);
    EXPECT_EQ(result1, JSPandaFileExecutor::ROUTE_URI_ERROR);
}

HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteInsecureAbcFile_1)
{
    std::string fileName = QUICKFIX_ABC_PATH "multi_file/base/module.abc";
    bool result = JSPandaFileExecutor::ExecuteInsecureAbcFile(thread, fileName.c_str());
    EXPECT_TRUE(result == false);
}

HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteInsecureAbcFile_2)
{
    ecmascript::ThreadManagedScope managedScope(thread);
    std::string fileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    bool result = JSPandaFileExecutor::ExecuteInsecureAbcFile(thread, fileName.c_str());
    EXPECT_TRUE(result == true);
}
}  // namespace panda::test
