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
#include "ecmascript/js_function_kind.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class PandaFileTranslatorTest : public testing::Test {
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

HWTEST_F_L0(PandaFileTranslatorTest, GenerateProgram)
{
    Parser parser;
    auto vm = thread->GetEcmaVM();
    const char *filename = "__PandaFileTranslatorTest1.pa";
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    const char *data = R"(
        .function any func_main_0(any a0, any a1, any a2) {}
        .function void func() {}
    )";
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    MethodLiteral *method1 = new MethodLiteral(methodId[0]);
    MethodLiteral *method2 = new MethodLiteral(methodId[1]);
    pf->SetMethodLiteralToMap(method1);
    pf->SetMethodLiteralToMap(method2);
    pfManager->AddJSPandaFile(pf);

    JSHandle<ecmascript::Program> program1 = pfManager->GenerateProgram(vm, pf.get(), std::string_view("func"));
    JSHandle<JSFunction> mainFunc1(thread, program1->GetMainFunction());
    JSHandle<JSTaggedValue> funcName1 = JSFunction::GetFunctionName(thread, JSHandle<JSFunctionBase>(mainFunc1));
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(funcName1)).ToCString().c_str(), "func");

    pf->UpdateMainMethodIndex(methodId[1].GetOffset());
    JSHandle<ecmascript::Program> program2 = pfManager->GenerateProgram(vm, pf.get(), JSPandaFile::ENTRY_FUNCTION_NAME);
    JSHandle<JSFunction> mainFunc2(thread, program2->GetMainFunction());
    JSHandle<JSTaggedValue> funcName2 = JSFunction::GetFunctionName(thread, JSHandle<JSFunctionBase>(mainFunc2));
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(funcName2)).ToCString().c_str(), "func_main_0");

    pfManager->RemoveJSPandaFile(pf.get());
}

HWTEST_F_L0(PandaFileTranslatorTest, TranslateClasses)
{
    Parser parser;
    const char *filename = "__PandaFileTranslatorTest2.pa";
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
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
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    pfManager->AddJSPandaFile(pf);
    EXPECT_TRUE(pf->FindMethodLiteral(methodId[0].GetOffset()) == nullptr);

    const char *methodName = MethodLiteral::GetMethodName(pf.get(), methodId[0]);
    PandaFileTranslator::TranslateClasses(thread, pf.get(), CString(methodName));
    EXPECT_TRUE(pf->FindMethodLiteral(methodId[0].GetOffset()) != nullptr);
    EXPECT_EQ(pf->FindMethodLiteral(methodId[0].GetOffset())->GetFunctionKind(),
                                    ecmascript::FunctionKind::NONE_FUNCTION);
    pfManager->RemoveJSPandaFile(pf.get());
}
}  // namespace panda::test
