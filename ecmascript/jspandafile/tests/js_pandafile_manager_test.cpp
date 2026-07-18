/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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

#include <memory>
#include <sstream>
#include <string>
#include "zlib.h"

#define private public
#define protected public
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/abc_buffer_cache.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#undef protected
#undef private
#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "class_data_accessor-inl.h"
#include "libziparchive/zip_archive.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"


using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
constexpr size_t PANDAFILE_FILE_HEADER_SIZE = 8;

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

    static void FakeReleaseSecureMemCallback(void* mapper)
    {
        if (mapper != nullptr) {
            *reinterpret_cast<uint8_t *>(mapper) = 10; // 10: random number
        }
    }

    static uint32_t CalcChecksum(const std::shared_ptr<JSPandaFile>& pf)
    {
        constexpr size_t PANDAFILE_CONTENT_OFFSET = PANDAFILE_FILE_HEADER_SIZE + sizeof(uint32_t);
        return adler32(1UL, static_cast<const uint8_t*>(pf->GetBase()) + PANDAFILE_CONTENT_OFFSET,
                       pf->GetFileSize() - PANDAFILE_CONTENT_OFFSET);
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
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName.c_str()));
    EXPECT_NE(pf, nullptr);

    auto expectFileName = pf->GetJSPandaFileDesc();
    EXPECT_STREQ(expectFileName.c_str(), "__JSPandaFileManagerTest.pa");
    remove(fileName.c_str());
    pfManager->RemoveJSPandaFile(pf.get());
}

HWTEST_F_L0(JSPandaFileManagerTest, OpenJSPandaFile)
{
    const char *filename = "__JSPandaFileManagerTest.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> ojspf = pfManager->OpenJSPandaFile(filename);
    EXPECT_TRUE(ojspf == nullptr);

    Parser parser;
    auto res = parser.Parse(data);
    EXPECT_TRUE(pandasm::AsmEmitter::Emit(filename, res.Value()));

    ojspf = pfManager->OpenJSPandaFile(filename);
    EXPECT_TRUE(ojspf != nullptr);
    EXPECT_STREQ(ojspf->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest.pa");
    pfManager->RemoveJSPandaFile(ojspf.get());

    remove(filename);
    ojspf = pfManager->OpenJSPandaFile(filename);
    EXPECT_TRUE(ojspf == nullptr);
}

HWTEST_F_L0(JSPandaFileManagerTest, Add_Find_Remove_JSPandaFile)
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
    std::shared_ptr<JSPandaFile> pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
    std::shared_ptr<JSPandaFile> pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(filename2));
    pfManager->AddJSPandaFile(pf1);
    pfManager->AddJSPandaFile(pf2);
    std::shared_ptr<JSPandaFile> foundPf1 = pfManager->FindJSPandaFile(filename1);
    std::shared_ptr<JSPandaFile> foundPf2 = pfManager->FindJSPandaFile(filename2);
    EXPECT_STREQ(foundPf1->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest1.pa");
    EXPECT_STREQ(foundPf2->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest2.pa");

    pfManager->RemoveJSPandaFile(pf1.get());
    pfManager->RemoveJSPandaFile(pf2.get());
    std::shared_ptr<JSPandaFile> afterRemovePf1 = pfManager->FindJSPandaFile(filename1);
    std::shared_ptr<JSPandaFile> afterRemovePf2 = pfManager->FindJSPandaFile(filename2);
    EXPECT_EQ(afterRemovePf1, nullptr);
    EXPECT_EQ(afterRemovePf2, nullptr);
}

HWTEST_F_L0(JSPandaFileManagerTest, MultiEcmaVM_Add_Find_Remove_JSPandaFile)
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
    std::shared_ptr<JSPandaFile> pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
    std::shared_ptr<JSPandaFile> pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(filename2));
    pfManager->AddJSPandaFile(pf1);
    pfManager->AddJSPandaFile(pf2);

    EcmaVM *vm = instance->GetJSThread()->GetEcmaVM();
    JSHandle<ConstantPool> constpool1 = instance->GetFactory()->NewSConstantPool(1);
    JSHandle<ConstantPool> constpool2 = instance->GetFactory()->NewSConstantPool(2);
    constpool1 = vm->AddOrUpdateConstpool(pf1.get(), constpool1, 0);
    constpool2 = vm->AddOrUpdateConstpool(pf2.get(), constpool2, 0);

    std::thread t1([&]() {
        EcmaVM *instance1;
        EcmaHandleScope *scope1;
        JSThread *thread1;
        TestHelper::CreateEcmaVMWithScope(instance1, thread1, scope1);
        std::shared_ptr<JSPandaFile> loadedPf1 = pfManager->LoadJSPandaFile(
            thread1, filename1, JSPandaFile::ENTRY_MAIN_FUNCTION, false, ExecuteTypes::STATIC);
        EXPECT_TRUE(pf1 == loadedPf1);
        EXPECT_TRUE(instance1->GetJSThread()->GetEcmaVM()->HasCachedConstpool(pf1.get()));
        TestHelper::DestroyEcmaVMWithScope(instance1, scope1); // Remove 'instance1' when ecmaVM destruct.
    });
    {
        ecmascript::ThreadSuspensionScope suspensionScope(thread);
        t1.join();
    }

    std::shared_ptr<JSPandaFile> foundPf1 = pfManager->FindJSPandaFile(filename1);
    EXPECT_TRUE(foundPf1 != nullptr);

    pfManager->RemoveJSPandaFile(pf1.get());
    pfManager->RemoveJSPandaFile(pf2.get());
    std::shared_ptr<JSPandaFile> afterRemovePf1 = pfManager->FindJSPandaFile(filename1);
    std::shared_ptr<JSPandaFile> afterRemovePf2 = pfManager->FindJSPandaFile(filename2);
    EXPECT_EQ(afterRemovePf1, nullptr);
    EXPECT_EQ(afterRemovePf2, nullptr);
    
    // panda file would be managed by gc, readd to panda file manager after check
    pfManager->AddJSPandaFile(pf1);
    pfManager->AddJSPandaFile(pf2);
}

void CreateJSPandaFileAndConstpool(EcmaVM *vm)
{
    const char *filename = "__JSPandaFileManagerTest1.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    pfManager->AddJSPandaFile(pf);

    [[maybe_unused]] EcmaHandleScope handleScope(vm->GetJSThread());
    JSHandle<ConstantPool> constpool = vm->GetFactory()->NewSConstantPool(1);
    constpool = vm->AddOrUpdateConstpool(pf.get(), constpool, 0);
    JSHandle<ConstantPool> newConstpool = vm->GetFactory()->NewConstantPool(1);
    vm->SetUnsharedConstpool(constpool, newConstpool.GetTaggedValue());
}

HWTEST_F_L0(JSPandaFileManagerTest, GC_Add_Find_Remove_JSPandaFile)
{
    const char *filename = "__JSPandaFileManagerTest1.pa";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();

    CreateJSPandaFileAndConstpool(instance);
    // Remove 'instance' and JSPandafile when trigger GC.
    JSThread *thread = instance->GetJSThread();
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_FAILED>(thread);
    SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_FAILED>(thread);
    std::shared_ptr<JSPandaFile> afterRemovePf = pfManager->FindJSPandaFile(filename);
    EXPECT_EQ(afterRemovePf, nullptr);
}

HWTEST_F_L0(JSPandaFileManagerTest, LoadJSPandaFile)
{
    const char *filename1 = "__JSPandaFileManagerTest1.pa";
    const char *filename2 = "__JSPandaFileManagerTest2.pa";
    const char *filename3 = "__JSPandaFileManagerTest3.abc";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr1 = pandasm::AsmEmitter::Emit(res.Value());
    std::unique_ptr<const File> pfPtr2 = pandasm::AsmEmitter::Emit(res.Value());
    std::unique_ptr<const File> pfPtr3 = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
    std::shared_ptr<JSPandaFile> pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(filename2));
    std::shared_ptr<JSPandaFile> pf3 = pfManager->NewJSPandaFile(pfPtr3.release(), CString(filename3));
    pfManager->AddJSPandaFile(pf1);
    pfManager->AddJSPandaFile(pf2);
    pfManager->AddJSPandaFile(pf3);
    std::shared_ptr<JSPandaFile> loadedPf1 =
        pfManager->LoadJSPandaFile(thread, filename1, JSPandaFile::ENTRY_MAIN_FUNCTION);
    std::shared_ptr<JSPandaFile> loadedPf2 =
        pfManager->LoadJSPandaFile(thread, filename2, JSPandaFile::ENTRY_MAIN_FUNCTION, (void *)data, sizeof(data));
    std::shared_ptr<JSPandaFile> loadedPf3 =
        pfManager->LoadJSPandaFile(thread, filename3, JSPandaFile::ENTRY_MAIN_FUNCTION, (void *)data, sizeof(data));
    EXPECT_TRUE(loadedPf1 != nullptr);
    EXPECT_TRUE(loadedPf2 != nullptr);
    EXPECT_TRUE(loadedPf3 != nullptr);
    EXPECT_TRUE(pf1 == loadedPf1);
    EXPECT_TRUE(pf2 == loadedPf2);
    EXPECT_TRUE(pf3 == loadedPf3);
    EXPECT_STREQ(loadedPf1->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest1.pa");
    EXPECT_STREQ(loadedPf2->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest2.pa");
    EXPECT_STREQ(loadedPf3->GetJSPandaFileDesc().c_str(), "__JSPandaFileManagerTest3.abc");

    pfManager->RemoveJSPandaFile(pf1.get());
    pfManager->RemoveJSPandaFile(pf2.get());
    pfManager->RemoveJSPandaFile(pf3.get());
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
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    const File *file = pf->GetPandaFile();
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });
    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    MethodLiteral *method = new MethodLiteral(methodId[0]);
    pf->SetMethodLiteralToMap(method);
    pfManager->AddJSPandaFile(pf);

    JSHandle<ecmascript::Program> program = pfManager->GenerateProgram(vm, pf.get(), JSPandaFile::ENTRY_FUNCTION_NAME);
    JSHandle<JSFunction> mainFunc(thread, program->GetMainFunction(thread));
    JSHandle<JSTaggedValue> funcName = JSFunction::GetFunctionName(thread, JSHandle<JSFunctionBase>(mainFunc));
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(funcName)).ToCString(thread).c_str(), "foo");
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
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    pfManager->AddJSPandaFile(pf);
    DebugInfoExtractor *extractor = pfManager->GetJSPtExtractor(pf.get());
    EXPECT_TRUE(extractor != nullptr);

    pfManager->RemoveJSPandaFile(pf.get());
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
    std::shared_ptr<JSPandaFile> pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
    std::shared_ptr<JSPandaFile> pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(filename2));
    pfManager->AddJSPandaFile(pf1);
    pfManager->AddJSPandaFile(pf2);
    std::vector<CString> descList{};
    int count = 0;
    pfManager->EnumerateJSPandaFiles([&](const std::shared_ptr<JSPandaFile> &file) -> bool {
        auto desc = file->GetJSPandaFileDesc();
        std::cout << "desc:" << desc << std::endl;
        if (desc.length() >= 3 && desc.substr(desc.length() - 3) == ".pa") {
            count++;
            descList.push_back(desc);
        }
        return true;
    });
    EXPECT_EQ(count, 2); // 2 : test number of files
    // Sort by the hash value of the element, the output is unordered
    std::sort(descList.begin(), descList.end());
    EXPECT_STREQ(descList[0].c_str(), "__JSPandaFileManagerTest3.pa");
    EXPECT_STREQ(descList[1].c_str(), "__JSPandaFileManagerTest4.pa");
}

HWTEST_F_L0(JSPandaFileManagerTest, CheckFilePath)
{
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const char *fileName = "__JSPandaFileManagerTest3.abc";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    pfManager->AddJSPandaFile(pf);
    bool result = pfManager->CheckFilePath(thread, fileName);
    EXPECT_TRUE(result);
    pfManager->RemoveJSPandaFile(pf.get());
}

HWTEST_F_L0(JSPandaFileManagerTest, GetJSPandaFileByBufferFiles)
{
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const char *fileName = "__JSPandaFileManagerTest3.abc";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    std::shared_ptr<JSPandaFile> jsPandaFile;
    pfManager->AddJSPandaFile(pf);
    AbcBufferCache *abcBufferCache = thread->GetEcmaVM()->GetAbcBufferCache();
    abcBufferCache->AddAbcBufferToCache(CString(fileName), (void *)data, sizeof(data), AbcBufferType::NORMAL_BUFFER);
    AbcBufferInfo bufferInfo = abcBufferCache->FindJSPandaFileInAbcBufferCache(CString(fileName));
    EXPECT_TRUE(bufferInfo.buffer_ != nullptr);
    abcBufferCache->DeleteAbcBufferFromCache(CString(fileName));
    jsPandaFile = pfManager->LoadJSPandaFile(thread, CString(fileName), "");
    EXPECT_TRUE(jsPandaFile != nullptr);
}

HWTEST_F_L0(JSPandaFileManagerTest, LoadInsecureJSPandaFile_1)
{
    CString fileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    auto jsPandaFile = pfManager->LoadInsecureJSPandaFile(thread, fileName, "");
    EXPECT_TRUE(jsPandaFile != nullptr);
    pfManager->RemoveJSPandaFile(jsPandaFile.get());
}

HWTEST_F_L0(JSPandaFileManagerTest, LoadInsecureJSPandaFile_2)
{
    CString fileName = QUICKFIX_ABC_PATH "multi_file/base/module.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    auto jsPandaFile = pfManager->LoadInsecureJSPandaFile(thread, fileName, "");
    EXPECT_TRUE(jsPandaFile == nullptr);
    pfManager->RemoveJSPandaFile(jsPandaFile.get());
}

/**
 * @tc.name: FindJSPandaFileWithChecksum
 * @tc.desc: Test FindJSPandaFileWithChecksum method to validate finding JSPandaFile by filename and checksum
 * @tc.type: FUNC
 * @tc.require: issue#12131
 */
HWTEST_F_L0(JSPandaFileManagerTest, FindJSPandaFileWithChecksum)
{
    const char *filename = "__JSPandaFileManagerTestWithChecksum.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    pfManager->AddJSPandaFile(pf);

    // Get the checksum of the loaded file
    uint32_t checksum = pf->GetChecksum();

    // Test FindJSPandaFileWithChecksum with correct checksum
    std::shared_ptr<JSPandaFile> foundPf = pfManager->FindJSPandaFileWithChecksum(filename, checksum);
    EXPECT_NE(foundPf, nullptr);
    EXPECT_STREQ(foundPf->GetJSPandaFileDesc().c_str(), filename);

    // Test with wrong checksum - should return nullptr and obsolete the file
    std::shared_ptr<JSPandaFile> wrongChecksumPf = pfManager->FindJSPandaFileWithChecksum(filename, checksum + 1);
    EXPECT_TRUE(wrongChecksumPf == nullptr);

    pfManager->RemoveJSPandaFile(pf.get());
}

/**
 * @tc.name: GetJSPandaFile
 * @tc.desc: Test GetJSPandaFile method to validate finding JSPandaFile by panda file pointer
 * @tc.type: FUNC
 * @tc.require: issue#12132
 */
HWTEST_F_L0(JSPandaFileManagerTest, GetJSPandaFile)
{
    const char *filename = "__JSPandaFileManagerTestGetJSPandaFile.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    pfManager->AddJSPandaFile(pf);

    // Test GetJSPandaFile by panda file pointer
    const panda_file::File *pandaFilePtr = pf->GetPandaFile();
    std::shared_ptr<JSPandaFile> foundPf = pfManager->GetJSPandaFile(pandaFilePtr);
    EXPECT_NE(foundPf, nullptr);
    EXPECT_STREQ(foundPf->GetJSPandaFileDesc().c_str(), filename);
    EXPECT_EQ(foundPf.get(), pf.get());

    // Test with nullptr panda file pointer
    std::shared_ptr<JSPandaFile> nullPf = pfManager->GetJSPandaFile(nullptr);
    EXPECT_TRUE(nullPf == nullptr);

    pfManager->RemoveJSPandaFile(pf.get());
}

/**
 * @tc.name: ObsoleteLoadedJSPandaFile
 * @tc.desc: Test ObsoleteLoadedJSPandaFile method to validate moving loaded JSPandaFile to old files set
 * @tc.type: FUNC
 * @tc.require: issue#12133
 */
HWTEST_F_L0(JSPandaFileManagerTest, ObsoleteLoadedJSPandaFile)
{
    const char *filename = "__JSPandaFileManagerTestObsolete.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    pfManager->AddJSPandaFile(pf);

    // Verify the file exists in loaded files
    std::shared_ptr<JSPandaFile> foundPfBefore = pfManager->FindJSPandaFile(filename);
    EXPECT_NE(foundPfBefore, nullptr);

    // Obsolete the loaded file
    pfManager->ObsoleteLoadedJSPandaFile(filename);

    // Verify the file no longer exists in loaded files
    std::shared_ptr<JSPandaFile> foundPfAfter = pfManager->FindJSPandaFile(filename);
    EXPECT_TRUE(foundPfAfter == nullptr);

    pfManager->RemoveJSPandaFile(pf.get());
}

/**
 * @tc.name: OpenJSPandaFileFromBuffer
 * @tc.desc: Test OpenJSPandaFileFromBuffer method to validate opening JSPandaFile from memory buffer
 * @tc.type: FUNC
 * @tc.require: issue#12136
 */
HWTEST_F_L0(JSPandaFileManagerTest, OpenJSPandaFileFromBuffer)
{
    const char *filename = "__JSPandaFileManagerTestFromBuffer.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());

    // Get buffer and size
    const void *buffer = pfPtr->GetBase();
    size_t size = pfPtr->GetHeader()->file_size;

    // Cast to uint8_t* for the method
    uint8_t *uintBuffer = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(buffer));

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();

    // Test OpenJSPandaFileFromBuffer
    std::shared_ptr<JSPandaFile> pf = pfManager->OpenJSPandaFileFromBuffer(uintBuffer, size, CString(filename));
    EXPECT_NE(pf, nullptr);
    EXPECT_STREQ(pf->GetJSPandaFileDesc().c_str(), filename);

    // Clean up

    pfManager->RemoveJSPandaFile(pf.get());
}

/**
 * @tc.name: GenPandafileCheckReport
 * @tc.desc: Test GenPandafileCheckReport method to validate checksum report generation
 * @tc.type: FUNC
 * @tc.require: issue#12796
 */
HWTEST_F_L0(JSPandaFileManagerTest, GenPandafileCheckReport)
{
    const char *filename1 = "__JSPandaFileManagerTest_GenPandafileCheckReport.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr1 = pandasm::AsmEmitter::Emit(res.Value());

    JSPandaFileManager localManager;
    JSPandaFileManager *pfManager = &localManager;
    std::shared_ptr<JSPandaFile> pf1 = pfManager->NewJSPandaFile(pfPtr1.release(), CString(filename1));
    pf1->checksum_ = CalcChecksum(pf1);
    pfManager->AddJSPandaFile(pf1);
    // Test 1: All files have correct checksum - should return empty string
    std::string report = pfManager->GenPandafileCheckReport();
    EXPECT_TRUE(report.empty());

    // Test 2: Modify checksum of pf1 to simulate damaged file
    uint32_t originalChecksum = pf1->GetChecksum();
    pf1->checksum_ = originalChecksum + 1;

    report = pfManager->GenPandafileCheckReport();
    EXPECT_FALSE(report.empty());
    EXPECT_TRUE(report.find(filename1) != std::string::npos);
    EXPECT_TRUE(report.find("checksum check failed") != std::string::npos);

    // Restore checksum
    pf1->checksum_ = originalChecksum;

    // Test 3: Test with bundle install path
    const char *bundlePath = "/data/storage/el1/bundle/com.example.app/entry/entry/ets/modules.abc";
    std::unique_ptr<const File> pfPtr2 = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf2 = pfManager->NewJSPandaFile(pfPtr2.release(), CString(bundlePath));
    pf2->checksum_ = CalcChecksum(pf2);
    pfManager->AddJSPandaFile(pf2);

    pf2->checksum_ = pf2->GetChecksum() + 1;
    report = pfManager->GenPandafileCheckReport();
    EXPECT_FALSE(report.empty());
    // The report should show the relative path after bundle install path
    EXPECT_TRUE(report.find("/data/storage/el1/bundle/") == std::string::npos);
    EXPECT_TRUE(report.find("com.example.app/entry/entry/ets/modules.abc") != std::string::npos);

    // Clean up
    pfManager->RemoveJSPandaFile(pf1.get());
    pfManager->RemoveJSPandaFile(pf2.get());
}

/**
 * @tc.name: GenPandafileCheckReportWithNullPtr
 * @tc.desc: Test GenPandafileCheckReport with null buffer pointer
 * @tc.type: FUNC
 * @tc.require: issue#12796
 */
HWTEST_F_L0(JSPandaFileManagerTest, GenPandafileCheckReportWithNullPtr)
{
    const char *filename = "__JSPandaFileManagerTestNull.pa";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    
    JSPandaFileManager localManager;
    JSPandaFileManager *pfManager = &localManager;
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    pfManager->AddJSPandaFile(pf);
    auto originPtr = const_cast<panda_file::File*>(pf->pf_)->base_.ptr_;
    const_cast<panda_file::File*>(pf->pf_)->base_.ptr_ = nullptr;

    // Simulate null buffer by getting the base pointer and setting it to null
    // Note: We can't directly modify pf_ pointer, but we can test the report format
    // by checking that the function handles the case correctly
    
    std::string report = pfManager->GenPandafileCheckReport();
    std::cout << "repor is: " << report << std::endl;
    // With valid file, should return empty string
    EXPECT_TRUE(report.find("invalid buffer pointer") != std::string::npos);
    const_cast<panda_file::File*>(pf->pf_)->base_.ptr_ = originPtr;

    // Clean up
    pfManager->RemoveJSPandaFile(pf.get());
}

/**
 * @tc.name: AbcBufferInfoDefaultFields
 * @tc.desc: Test AbcBufferInfo default constructor initializes needUpdate_ and fileMapper_ correctly
 * @tc.type: FUNC
 * @tc.require: issue#13418
 */
HWTEST_F_L0(JSPandaFileManagerTest, AbcBufferInfoDefaultFields)
{
    // Test default constructor: needUpdate_ should be false, fileMapper_ should be nullptr
    AbcBufferInfo defaultInfo;
    EXPECT_EQ(defaultInfo.buffer_, nullptr);
    EXPECT_EQ(defaultInfo.size_, 0U);
    EXPECT_EQ(defaultInfo.bufferType_, AbcBufferType::NORMAL_BUFFER);
    EXPECT_FALSE(defaultInfo.needUpdate_);
    EXPECT_EQ(defaultInfo.fileMapper_, nullptr);

    // Test parameterized constructor: needUpdate_ and fileMapper_ should be set correctly
    uint8_t buffer[4] = {1, 2, 3, 4}; // 4: random size
    void *mockMapper = reinterpret_cast<void *>(buffer);
    AbcBufferInfo paramInfo(buffer, sizeof(buffer), AbcBufferType::SECURE_BUFFER, true, mockMapper);
    EXPECT_EQ(paramInfo.buffer_, buffer);
    EXPECT_EQ(paramInfo.size_, sizeof(buffer));
    EXPECT_EQ(paramInfo.bufferType_, AbcBufferType::SECURE_BUFFER);
    EXPECT_TRUE(paramInfo.needUpdate_);
    EXPECT_EQ(paramInfo.fileMapper_, mockMapper);
}

/**
 * @tc.name: AddAbcBufferToCacheWithNeedUpdateAndFileMapper
 * @tc.desc: Test AddAbcBufferToCache preserves needUpdate and fileMapper in cache
 * @tc.type: FUNC
 * @tc.require: issue#13418
 */
HWTEST_F_L0(JSPandaFileManagerTest, AddAbcBufferToCacheWithNeedUpdateAndFileMapper)
{
    AbcBufferCache *abcBufferCache = thread->GetEcmaVM()->GetAbcBufferCache();
    CString fileName = "__TestAddAbcBufferWithParams.abc";
    const char *data = R"(
        .function void foo() {}
    )";

    // Test: AddAbcBufferToCache with needUpdate=true and fileMapper
    uint8_t mockMapper = 0;
    void *fileMapper = &mockMapper;
    abcBufferCache->AddAbcBufferToCache(fileName, data, sizeof(data),
        AbcBufferType::SECURE_BUFFER, true, fileMapper);

    AbcBufferInfo bufferInfo = abcBufferCache->FindJSPandaFileInAbcBufferCache(fileName);
    EXPECT_TRUE(bufferInfo.buffer_ != nullptr);
    EXPECT_EQ(bufferInfo.bufferType_, AbcBufferType::SECURE_BUFFER);
    EXPECT_TRUE(bufferInfo.needUpdate_);
    EXPECT_EQ(bufferInfo.fileMapper_, fileMapper);

    // Test: AddAbcBufferToCache with default needUpdate=false and fileMapper=nullptr
    CString fileName2 = "__TestAddAbcBufferDefault.abc";
    abcBufferCache->AddAbcBufferToCache(fileName2, data, sizeof(data), AbcBufferType::NORMAL_BUFFER);

    AbcBufferInfo bufferInfo2 = abcBufferCache->FindJSPandaFileInAbcBufferCache(fileName2);
    EXPECT_TRUE(bufferInfo2.buffer_ != nullptr);
    EXPECT_EQ(bufferInfo2.bufferType_, AbcBufferType::NORMAL_BUFFER);
    EXPECT_FALSE(bufferInfo2.needUpdate_);
    EXPECT_EQ(bufferInfo2.fileMapper_, nullptr);

    abcBufferCache->DeleteAbcBufferFromCache(fileName);
    abcBufferCache->DeleteAbcBufferFromCache(fileName2);
}

/**
 * @tc.name: AbcBufferCacheScopeDestructorFindJSPandaFile
 * @tc.desc: Test AbcBufferCacheScope destructor uses FindJSPandaFile instead of held pointer
 * @tc.type: FUNC
 * @tc.require: issue#13418
 */
HWTEST_F_L0(JSPandaFileManagerTest, AbcBufferCacheScopeDestructorFindJSPandaFile)
{
    JSNApi::SetReleaseSecureMemCallback(FakeReleaseSecureMemCallback);

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const char *fileName = "__TestCacheScopeDestructor.abc";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    uint8_t *fileMapper = new uint8_t[1];
    *fileMapper = 1;
    uint8_t *buffer = new uint8_t[1];
    pf->SetFileMapper(fileMapper);
    pfManager->AddJSPandaFile(pf);

    {
        // AbcBufferCacheScope destructor should find jsPandaFile via FindJSPandaFile
        // and correctly check fileMapper match
        AbcBufferCacheScope bufferScope(
            thread, CString(fileName), buffer, 3, false, reinterpret_cast<void *>(fileMapper)); // 3: random number
        // When scope ends, destructor calls FindJSPandaFile(fileName)
        // fileMapper_ == fileMapper and pf->GetFileMapper() == fileMapper, so no release
    }
    // fileMapper should not be released since it matches the pandafile's mapper
    EXPECT_EQ(*fileMapper, 1);

    pfManager->RemoveJSPandaFile(pf.get());
    // Must reset pf before delete[] fileMapper to avoid use-after-free:
    // ~JSPandaFile() calls CallReleaseSecureMemFunc(fileMapper_) which writes to fileMapper
    // If fileMapper is already freed, writing to it corrupts glibc tcache metadata
    pf.reset();
    delete[] buffer;
    delete[] fileMapper;
}

/**
 * @tc.name: AbcBufferCacheScopeDestructorWithNullPandaFile
 * @tc.desc: Test AbcBufferCacheScope destructor safely returns when FindJSPandaFile returns nullptr
 * @tc.type: FUNC
 * @tc.require: issue#13418
 */
HWTEST_F_L0(JSPandaFileManagerTest, AbcBufferCacheScopeDestructorWithNullPandaFile)
{
    JSNApi::SetReleaseSecureMemCallback(FakeReleaseSecureMemCallback);

    const char *fileName = "__TestNullPandaFileScope.abc";
    uint8_t *buffer = new uint8_t[1];
    uint8_t *secondFileMapper = new uint8_t[1];
    *secondFileMapper = 5; // 5: random number

    // Ensure no JSPandaFile is registered for this filename
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> existingPf = pfManager->FindJSPandaFile(fileName);
    EXPECT_TRUE(existingPf == nullptr);

    {
        // AbcBufferCacheScope destructor should handle null jsPandaFile safely
        // FindJSPandaFile returns nullptr -> early return, no crash
        AbcBufferCacheScope bufferScope(thread, CString(fileName), buffer, 3,
            false, reinterpret_cast<void *>(secondFileMapper)); // 3: random number
    }
    // secondFileMapper should not be released since jsPandaFile is nullptr
    EXPECT_EQ(*secondFileMapper, 5); // 5: original value unchanged
    existingPf.reset();
    delete[] buffer;
    delete[] secondFileMapper;
}

/**
 * @tc.name: AbcBufferCacheScopeSecureMemReleaseAfterPandaFileRemoved
 * @tc.desc: Test secure memory release when JSPandaFile is removed (SharedGC scenario)
 * @tc.type: FUNC
 * @tc.require: issue#13418
 */
HWTEST_F_L0(JSPandaFileManagerTest, AbcBufferCacheScopeSecureMemReleaseAfterPandaFileRemoved)
{
    JSNApi::SetReleaseSecureMemCallback(FakeReleaseSecureMemCallback);

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const char *fileName = "__TestSecureMemReleaseRemoved.abc";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    uint8_t *fileMapper = new uint8_t[1];
    *fileMapper = 1;
    uint8_t *secondFileMapper = new uint8_t[1];
    uint8_t *buffer = new uint8_t[1];
    pf->SetFileMapper(fileMapper);
    pfManager->AddJSPandaFile(pf);

    {
        // first: same fileMapper, no release needed
        AbcBufferCacheScope bufferScope(
            thread, CString(fileName), buffer, 3, false, reinterpret_cast<void *>(fileMapper)); // 3: random number
    }
    EXPECT_EQ(*fileMapper, 1);

    {
        // second: different fileMapper, needs release
        // But pandafile still exists -> fileMapper_ != pf->GetFileMapper() -> release secondFileMapper
        AbcBufferCacheScope bufferScope(
            thread, CString(fileName), buffer, 3, false, reinterpret_cast<void *>(secondFileMapper));
    }
    EXPECT_EQ(*secondFileMapper, 10); // 10: released by FakeReleaseSecureMemCallback

    // Simulate SharedGC: remove JSPandaFile from manager
    pfManager->RemoveJSPandaFile(pf.get());
    pf.reset(); // now sole owner gone, ~JSPandaFile() calls CallReleaseSecureMemFunc(fileMapper_) -> *fileMapper = 10

    {
        // After SharedGC removed the pandafile, FindJSPandaFile returns nullptr
        // Destructor should early return safely, no crash from dangling pointer
        uint8_t *thirdFileMapper = new uint8_t[1];
        *thirdFileMapper = 7; // 7: random number
        AbcBufferCacheScope bufferScope(
            thread, CString(fileName), buffer, 3, false, reinterpret_cast<void *>(thirdFileMapper));
        // thirdFileMapper is not released because jsPandaFile is nullptr -> early return
        EXPECT_EQ(*thirdFileMapper, 7); // 7: unchanged, not released
        delete[] thirdFileMapper;
    }

    delete[] buffer;
    delete[] fileMapper;
    delete[] secondFileMapper;
}

/**
 * @tc.name: AbcBufferCacheScopeWithNeedUpdate
 * @tc.desc: Test AbcBufferCacheScope constructor with needUpdate=true stores parameter in cache
 * @tc.type: FUNC
 * @tc.require: issue#13418
 */
HWTEST_F_L0(JSPandaFileManagerTest, AbcBufferCacheScopeWithNeedUpdate)
{
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const char *fileName = "__TestCacheScopeNeedUpdate.abc";
    const char *data = R"(
        .function void foo() {}
    )";
    Parser parser;
    auto res = parser.Parse(data);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), CString(fileName));
    uint8_t *buffer = new uint8_t[1];
    pfManager->AddJSPandaFile(pf);

    AbcBufferCache *abcBufferCache = thread->GetEcmaVM()->GetAbcBufferCache();

    {
        // needUpdate=true should be stored in buffer cache
        AbcBufferCacheScope bufferScope(
            thread, CString(fileName), buffer, 3, true, nullptr); // 3: random number, needUpdate=true

        AbcBufferInfo cachedInfo = abcBufferCache->FindJSPandaFileInAbcBufferCache(CString(fileName));
        EXPECT_TRUE(cachedInfo.needUpdate_);
        EXPECT_EQ(cachedInfo.fileMapper_, nullptr);
        EXPECT_EQ(cachedInfo.bufferType_, AbcBufferType::SECURE_BUFFER);
    }
    // After scope ends, cache entry should be deleted
    AbcBufferInfo afterInfo = abcBufferCache->FindJSPandaFileInAbcBufferCache(CString(fileName));
    EXPECT_EQ(afterInfo.buffer_, nullptr);

    pfManager->RemoveJSPandaFile(pf.get());
    delete[] buffer;
}

/**
 * @tc.name: AbcBufferCacheScopeDestructorFindNewPandaFileAfterSharedGC
 * @tc.desc: Test that after SharedGC, AbcBufferCacheScope destructor finds the NEW (current) JSPandaFile
 *           via FindJSPandaFile, not the old one. This is the core fix of issue#13418:
 *           the old code held a raw pointer to the OLD JSPandaFile, which could have a different
 *           fileMapper than the scope's fileMapper_, causing incorrect release of active memory.
 *           The new code uses FindJSPandaFile which returns the NEW JSPandaFile whose fileMapper
 *           matches scope's fileMapper_ -> no release, avoiding the dangling release bug.
 * @tc.type: FUNC
 * @tc.require: issue#13418
 */
HWTEST_F_L0(JSPandaFileManagerTest, AbcBufferCacheScopeDestructorFindNewPandaFileAfterSharedGC)
{
    JSNApi::SetReleaseSecureMemCallback(FakeReleaseSecureMemCallback);

    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    const char *fileName = "__TestNewPandaFileAfterGC.abc";

    // Step 1: Create OLD JSPandaFile and register it
    const char *oldData = R"(
        .function void oldFoo() {}
    )";
    Parser oldParser;
    auto oldRes = oldParser.Parse(oldData);
    std::unique_ptr<const File> oldPfPtr = pandasm::AsmEmitter::Emit(oldRes.Value());
    std::shared_ptr<JSPandaFile> oldPf = pfManager->NewJSPandaFile(oldPfPtr.release(), CString(fileName));
    uint8_t *oldFileMapper = new uint8_t[1];
    *oldFileMapper = 1;
    uint8_t *buffer = new uint8_t[1];
    oldPf->SetFileMapper(oldFileMapper);
    pfManager->AddJSPandaFile(oldPf);

    // Step 2: Simulate SharedGC — obsolete OLD, remove from loadedJSPandaFiles_
    pfManager->RemoveJSPandaFile(oldPf.get());

    // Step 3: Load NEW JSPandaFile for the same filename
    const char *newData = R"(
        .function void newFoo() {}
    )";
    Parser newParser;
    auto newRes = newParser.Parse(newData);
    std::unique_ptr<const File> newPfPtr = pandasm::AsmEmitter::Emit(newRes.Value());
    std::shared_ptr<JSPandaFile> newPf = pfManager->NewJSPandaFile(newPfPtr.release(), CString(fileName));
    uint8_t *newFileMapper = new uint8_t[1];
    *newFileMapper = 2; // 2: distinct from oldFileMapper=1
    newPf->SetFileMapper(newFileMapper);
    pfManager->AddJSPandaFile(newPf);

    // Step 4: Verify FindJSPandaFile returns NEW (not OLD)
    std::shared_ptr<JSPandaFile> foundPf = pfManager->FindJSPandaFile(fileName);
    EXPECT_EQ(foundPf.get(), newPf.get());

    // Step 5: Core test — scope's fileMapper_ == newFileMapper
    // FindJSPandaFile finds NEW, NEW->GetFileMapper() == newFileMapper == scope's fileMapper_
    // -> no release -> this is the exact scenario the PR fixes (old code would find OLD, mismatch, and release)
    {
        AbcBufferCacheScope bufferScope(
            thread, CString(fileName), buffer, 3, false, reinterpret_cast<void *>(newFileMapper)); // 3: random number
    }
    // newFileMapper should NOT be released since it matches NEW pandafile's mapper
    EXPECT_EQ(*newFileMapper, 2);

    // Step 6: scope's fileMapper_ != NEW's fileMapper -> should release scope's fileMapper
    // otherMapper must be declared outside the block so delete[] happens AFTER bufferScope destructor
    uint8_t *otherMapper = new uint8_t[1];
    *otherMapper = 5; // 5: random number
    {
        AbcBufferCacheScope bufferScope(
            thread, CString(fileName), buffer, 3, false, reinterpret_cast<void *>(otherMapper));
        // FindJSPandaFile finds NEW, NEW->GetFileMapper() != otherMapper -> release otherMapper
        // bufferScope destructor runs at block exit, calls CallReleaseSecureMemFunc(otherMapper)
    }
    // After scope exits: bufferScope destructor has called CallReleaseSecureMemFunc(otherMapper)
    // FakeReleaseSecureMemCallback only writes a byte, doesn't free; must delete[] manually
    // delete[] must happen AFTER destructor to avoid use-after-free on already-freed pointer
    delete[] otherMapper;

    // Cleanup
    pfManager->RemoveJSPandaFile(newPf.get());
    foundPf.reset(); // Must reset before delete[] to avoid use-after-free: foundPf holds ref to newPf
    newPf.reset(); // ~JSPandaFile() -> CallReleaseSecureMemFunc(newFileMapper_) -> *newFileMapper = 10
    oldPf.reset(); // ~JSPandaFile() -> CallReleaseSecureMemFunc(oldFileMapper_) -> *oldFileMapper = 10

    delete[] buffer;
    delete[] newFileMapper;
    delete[] oldFileMapper;
}

}  // namespace panda::test
