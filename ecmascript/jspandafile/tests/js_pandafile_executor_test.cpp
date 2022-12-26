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
    JSPandaFile *pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
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
    method->Initialize(pf);
    pf->SetMethodLiteralToMap(method);
    pfManager->InsertJSPandaFile(pf);
    Expected<JSTaggedValue, bool> result = JSPandaFileExecutor::Execute(thread, pf, JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_TRUE(result);
    EXPECT_EQ(result.Value(), JSTaggedValue::Hole());

    pfManager->RemoveJSPandaFile((void *)pf);
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
    JSPandaFile *pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
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
    method->Initialize(pf);
    pf->SetMethodLiteralToMap(method);
    pfManager->InsertJSPandaFile(pf);
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::ExecuteFromFile(thread, CString(fileName), JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_TRUE(result);
    EXPECT_EQ(result.Value(), JSTaggedValue::Hole());

    pfManager->RemoveJSPandaFile((void *)pf);
    const JSPandaFile *foundPf = pfManager->FindJSPandaFile(fileName);
    pfManager->RemoveJSPandaFile((void *)foundPf);
}

HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteFromBuffer)
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
    JSPandaFile *pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
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
    method->Initialize(pf);
    pf->SetMethodLiteralToMap(method);
    pfManager->InsertJSPandaFile(pf);
    Expected<JSTaggedValue, bool> result = JSPandaFileExecutor::ExecuteFromBuffer(
        thread, (void *)data, sizeof(data), JSPandaFile::ENTRY_MAIN_FUNCTION, CString(fileName));
    EXPECT_TRUE(result);
    EXPECT_EQ(result.Value(), JSTaggedValue::Hole());

    pfManager->RemoveJSPandaFile((void *)pf);
    const JSPandaFile *foundPf = pfManager->FindJSPandaFile(fileName);
    pfManager->RemoveJSPandaFile((void *)foundPf);
}

HWTEST_F_L0(JSPandaFileExecutorTest, NormalizePath)
{
    CString res1 = "node_modules/0/moduleTest/index";
    CString moduleRecordName1 = "node_modules///0//moduleTest/index";
    
    CString res2 = "./node_modules/0/moduleTest/index";
    CString moduleRecordName2 = "./node_modules///0//moduleTest/index";

    CString res3 = "../node_modules/0/moduleTest/index";
    CString moduleRecordName3 = "../node_modules/0/moduleTest///index";

    CString res4 = "./moduleTest/index";
    CString moduleRecordName4 = "./node_modules/..//moduleTest////index";

    CString normalName1 = JSPandaFileExecutor::NormalizePath(moduleRecordName1);
    CString normalName2 = JSPandaFileExecutor::NormalizePath(moduleRecordName2);
    CString normalName3 = JSPandaFileExecutor::NormalizePath(moduleRecordName3);
    CString normalName4 = JSPandaFileExecutor::NormalizePath(moduleRecordName4);

    EXPECT_EQ(res1, normalName1);
    EXPECT_EQ(res2, normalName2);
    EXPECT_EQ(res3, normalName3);
    EXPECT_EQ(res4, normalName4);
}
}  // namespace panda::test
