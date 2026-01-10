/*
 * Copyright (c) 2022-2026 Huawei Device Co., Ltd.
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
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#define private public
#define protected public
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#undef protected
#undef private
#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "class_data_accessor-inl.h"

#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class JSPandaFileExecutorTest  : public BaseTestWithScope<false> {
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

/**
 * @tc.name: ExecuteFromAbsolutePathAbcFile
 * @tc.desc: Test ExecuteFromAbsolutePathAbcFile method to validate execution from absolute path ABC file
 * @tc.type: FUNC
 * @tc.require: issue#12144
 */
HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteFromAbsolutePathAbcFile)
{
    // Test ExecuteFromAbsolutePathAbcFile with absolute path
    // The method will call ExecuteFromFile internally
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
        JSPandaFileExecutor::ExecuteFromAbsolutePathAbcFile(thread, CString(fileName),
            JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_TRUE(result);
    pfManager->RemoveJSPandaFile(pf.get());
}

/**
 * @tc.name: ExecuteModuleBuffer
 * @tc.desc: Test ExecuteModuleBuffer method to validate module execution from buffer
 * @tc.type: FUNC
 * @tc.require: issue#12145
 */
HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteModuleBuffer)
{
    std::string fileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->LoadJSPandaFile(thread, CString(fileName.c_str()),
                                                                 JSPandaFile::ENTRY_MAIN_FUNCTION);

    // Get the buffer data
    const void *buffer = pf->GetBase();
    size_t size = pf->GetFileSize();

    // Test ExecuteModuleBuffer with buffer data. False with recordinfo == null, because notmodule info will be failed
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::ExecuteModuleBuffer(thread, buffer, size, CString(fileName.c_str()), false);
    EXPECT_FALSE(result);
    // ExecuteModuleBuffer will create a invalid JSPandaFile and add it to JSPandaFileManager
    // So remove it from JSPandaFileManager after test
    std::shared_ptr<JSPandaFile> emptyPf = nullptr;
    pfManager->EnumerateJSPandaFiles([&emptyPf](const std::shared_ptr<JSPandaFile> &file) -> bool {
        emptyPf = file;
        return false;
    });
    EXPECT_NE(emptyPf, nullptr);
    pfManager->RemoveJSPandaFile(emptyPf.get());
}

/**
 * @tc.name: CommonExecuteBuffer
 * @tc.desc: Test CommonExecuteBuffer method to validate common buffer execution functionality
 * @tc.type: FUNC
 * @tc.require: issue#12146
 */
HWTEST_F_L0(JSPandaFileExecutorTest, CommonExecuteBuffer)
{
    std::string fileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->LoadJSPandaFile(thread, CString(fileName.c_str()),
                                                                 JSPandaFile::ENTRY_MAIN_FUNCTION);

    // Get the buffer data
    const void *buffer = pf->GetBase();
    size_t size = pf->GetFileSize();
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::CommonExecuteBuffer(thread, CString(fileName), "main", pf.get());
    EXPECT_TRUE(result);

    // Test the other overload with buffer parameters
    Expected<JSTaggedValue, bool> result2 =
        JSPandaFileExecutor::CommonExecuteBuffer(thread, CString(fileName), "main", buffer, size);
    EXPECT_TRUE(result2);
}

/**
 * @tc.name: BindPreloadedPandaFilesToAOT
 * @tc.desc: Test BindPreloadedPandaFilesToAOT method to validate binding preloaded panda files to AOT
 * @tc.type: FUNC
 * @tc.require: issue#12147
 */
HWTEST_F_L0(JSPandaFileExecutorTest, BindPreloadedPandaFilesToAOT)
{
    // Test BindPreloadedPandaFilesToAOT with a module name
    std::string moduleName = "test_module";
    JSPandaFileExecutor::BindPreloadedPandaFilesToAOT(instance, moduleName);
}

/**
 * @tc.name: ExecuteFromBufferSecure
 * @tc.desc: Test ExecuteFromBufferSecure method to validate execution from secure buffer
 * @tc.type: FUNC
 * @tc.require: issue#12148
 */
HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteFromBufferSecure)
{
    const char *fileName = "ExecuteFromBufferSecure.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());

    // Get the buffer data as uint8_t*
    uint8_t *buffer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(pfPtr->GetBase()));
    size_t size = pfPtr->GetHeader()->file_size;

    // Test ExecuteFromBufferSecure with secure buffer
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::ExecuteFromBufferSecure(thread, buffer, size,
            JSPandaFile::ENTRY_MAIN_FUNCTION, CString(fileName), false, nullptr);
    EXPECT_TRUE(result);

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    // ExecuteFromBufferSecure will create a invalid JSPandaFile and add it to JSPandaFileManager
    // So remove it from JSPandaFileManager after test
    std::shared_ptr<JSPandaFile> emptyPf = nullptr;
    pfManager->EnumerateJSPandaFiles([&emptyPf](const std::shared_ptr<JSPandaFile> &file) -> bool {
        emptyPf = file;
        return false;
    });
    EXPECT_NE(emptyPf, nullptr);
    pfManager->RemoveJSPandaFile(emptyPf.get());
}

/**
 * @tc.name: ExecuteModuleBufferSecure
 * @tc.desc: Test ExecuteModuleBufferSecure method to validate module execution from secure buffer
 * @tc.type: FUNC
 * @tc.require: issue#12149
 */
HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteModuleBufferSecure)
{
    std::string fileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->LoadJSPandaFile(thread, CString(fileName.c_str()),
                                                                 JSPandaFile::ENTRY_MAIN_FUNCTION);

    // Get the buffer data
    uint8_t *buffer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(pf->GetBase()));
    size_t size = pf->GetFileSize();

    // Test ExecuteModuleBufferSecure with secure buffer
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::ExecuteModuleBufferSecure(thread, buffer, size, CString(fileName), false);
    EXPECT_FALSE(result);

    
    // ExecuteModuleBufferSecure will create a invalid JSPandaFile and add it to JSPandaFileManager
    // So remove it from JSPandaFileManager after test
    std::shared_ptr<JSPandaFile> emptyPf = nullptr;
    pfManager->EnumerateJSPandaFiles([&emptyPf](const std::shared_ptr<JSPandaFile> &file) -> bool {
        emptyPf = file;
        return false;
    });
    EXPECT_NE(emptyPf, nullptr);
    pfManager->RemoveJSPandaFile(emptyPf.get());
}

/**
 * @tc.name: ExecuteSecureWithOhmUrl
 * @tc.desc: Test ExecuteSecureWithOhmUrl method to validate execution with Ohm URL from secure buffer
 * @tc.type: FUNC
 * @tc.require: issue#12150
 */
HWTEST_F_L0(JSPandaFileExecutorTest, ExecuteSecureWithOhmUrl)
{
    std::string fileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->LoadJSPandaFile(thread,
                                                                 CString(fileName.c_str()),
                                                                 JSPandaFile::ENTRY_MAIN_FUNCTION);

    // Get the buffer data as uint8_t*
    uint8_t *buffer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(pf->GetBase()));
    size_t size = pf->GetFileSize();

    // Test ExecuteSecureWithOhmUrl with secure buffer and Ohm URL
    Expected<JSTaggedValue, bool> result =
        JSPandaFileExecutor::ExecuteSecureWithOhmUrl(thread, buffer, size, CString(fileName),
            CString("main"));
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsExecuteModuleInAbcFile
 * @tc.desc: Test IsExecuteModuleInAbcFile method to validate checking if module exists in ABC file
 * @tc.type: FUNC
 * @tc.require: issue#12149
 */
HWTEST_F_L0(JSPandaFileExecutorTest, IsExecuteModuleInAbcFile)
{
    const char *fileName = "IsExecuteModuleInAbcFile.abc";
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
    pfManager->AddJSPandaFile(pf);

    // Test IsExecuteModuleInAbcFile to check if a module exists in ABC file
    bool result = JSPandaFileExecutor::IsExecuteModuleInAbcFile(thread, CString("test_bundle"),
        CString(fileName), CString("func_main_0"));
    EXPECT_FALSE(result);  // Expecting false since the test file may not contain the specified function

    // Clean up
    pfManager->RemoveJSPandaFile(pf.get());
}

/**
 * @tc.name: IsExecuteModuleInAbcFileSecure
 * @tc.desc: Test method to validate checking if module exists in ABC file (secure version)
 * @tc.type: FUNC
 * @tc.require: issue#12152
 */
HWTEST_F_L0(JSPandaFileExecutorTest, IsExecuteModuleInAbcFileSecure)
{
    uint8_t *buffer = nullptr;
    size_t size = 0;

    // Test IsExecuteModuleInAbcFileSecure to check if a module exists in ABC file (secure version)
    bool result = JSPandaFileExecutor::IsExecuteModuleInAbcFileSecure(thread, buffer, size,
        CString("IsExecuteModuleInAbcFileSecure"), CString("func_main_0"));
    EXPECT_FALSE(result);  // Expecting false since the test file does not contain the specified function
}

}  // namespace panda::test
