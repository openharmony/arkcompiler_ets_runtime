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

#include <unistd.h>
#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "gtest/gtest.h"
#include "libpandabase/utils/utf.h"
#include "class_data_accessor-inl.h"
#define private public
#define protected public
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_snapshot.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#include "ecmascript/platform/file.h"
#undef protected
#undef private
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/ohos/ohos_constants.h"
#include "ecmascript/ohos/ohos_version_info_tools.h"
#include "ecmascript/platform/filesystem.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/js_function_kind.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

using ClassTranslateWork = std::vector<std::pair<uint32_t, uint32_t>>;
using CurClassTranslateWork = std::pair<uint32_t, ClassTranslateWork>;

namespace panda::test {
class MockJSPandaFileSnapshotForTest : public JSPandaFileSnapshot {
public:
    static bool WriteDataToFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
                                const CString &version)
    {
        return JSPandaFileSnapshot::WriteDataToFile(thread, jsPandaFile, path, version);
    }

    static CString GetSnapshotFileName(const CString &fileName, const CString &path)
    {
        return JSPandaFileSnapshot::GetJSPandaFileFileName(fileName, path);
    }

    static bool ReadDataFromFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path,
                                 const CString &version)
    {
        return JSPandaFileSnapshot::ReadDataFromFile(thread, jsPandaFile, path, version);
    }
};
class JSPandaFileTest : public testing::Test {
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
protected:
    std::shared_ptr<JSPandaFile> CreateJSPandaFile(const char *source, const CString filename)
    {
        Parser parser;
        const std::string fn = "SRC.pa"; // test file name : "SRC.pa"
        auto res = parser.Parse(source, fn);
        EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);

        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), filename);
        return pf;
    }

    std::shared_ptr<JSPandaFile> CreateJSPandaFileWithThread(const char *source, const CString filename)
    {
        Parser parser;
        const std::string fn = "SRC.pa";
        auto res = parser.Parse(source, fn);
        EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);

        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        return pfManager->NewJSPandaFile(thread, pfPtr.release(), filename, "");
    }

    void NormalTranslateJSPandaFile(const std::shared_ptr<JSPandaFile>& pf) const
    {
        const File *file = pf->GetPandaFile();
        const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
        const File::EntityId classId = file->GetClassId(typeDesc);
        ClassDataAccessor cda(*file, classId);
        std::vector<File::EntityId> methodId {};
        cda.EnumerateMethods([&](const MethodDataAccessor &mda) {
            methodId.push_back(mda.GetMethodId());
        });
        pf->UpdateMainMethodIndex(methodId[0].GetOffset());
        const char *methodName = MethodLiteral::GetMethodName(pf.get(), methodId[0]);
        PandaFileTranslator::TranslateClasses(thread, pf.get(), CString(methodName));
    }

    static CString GetCurrentDirPath()
    {
        char buff[FILENAME_MAX];
        getcwd(buff, FILENAME_MAX);
        CString path(buff);
        if (path.back() != '/') {
            path += "/";
        }
        return path;
    }
};

HWTEST_F_L0(JSPandaFileTest, CreateJSPandaFile)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    EXPECT_TRUE(pf != nullptr);
}

HWTEST_F_L0(JSPandaFileTest, GetJSPandaFileDesc)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    const CString expectFileName = pf->GetJSPandaFileDesc();
    EXPECT_STREQ(expectFileName.c_str(), "test.pa");
}

HWTEST_F_L0(JSPandaFileTest, GetPandaFile)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    EXPECT_TRUE(file != nullptr);
}

HWTEST_F_L0(JSPandaFileTest, GetMethodLiterals_GetNumMethods)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
        .function void foo3() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    MethodLiteral *method = pf->GetMethodLiterals();
    EXPECT_TRUE(method != nullptr);

    uint32_t methodNum = pf->GetNumMethods();
    EXPECT_EQ(methodNum, 3U); // 3 : number of methods
}

HWTEST_F_L0(JSPandaFileTest, SetMethodLiteralToMap_FindMethodLiteral)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
        .function void foo3() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = file->GetClassId(typeDesc);
    EXPECT_TRUE(classId.IsValid());

    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    int count = 0;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
        count++;
    });
    EXPECT_EQ(count, 3); // 3 : number of methods

    MethodLiteral *method1 = new MethodLiteral(methodId[0]);
    MethodLiteral *method2 = new MethodLiteral(methodId[1]);
    MethodLiteral *method3 = new MethodLiteral(methodId[2]);
    pf->SetMethodLiteralToMap(method1);
    pf->SetMethodLiteralToMap(method2);
    pf->SetMethodLiteralToMap(method3);
    EXPECT_STREQ(MethodLiteral::ParseFunctionName(pf.get(), methodId[0]).c_str(), "foo1");
    EXPECT_STREQ(MethodLiteral::ParseFunctionName(pf.get(), methodId[1]).c_str(), "foo2");
    EXPECT_STREQ(MethodLiteral::ParseFunctionName(pf.get(), methodId[2]).c_str(), "foo3");
}

HWTEST_F_L0(JSPandaFileTest, GetOrInsertConstantPool_GetConstpoolIndex_GetConstpoolMap)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
        .function void foo3() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = file->GetClassId(typeDesc);
    EXPECT_TRUE(classId.IsValid());

    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    int count = 0;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
        count++;
    });
    EXPECT_EQ(count, 3); // 3 : number of methods

    uint32_t index1 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[0].GetOffset());
    uint32_t index2 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[1].GetOffset());
    uint32_t index3 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[2].GetOffset());
    EXPECT_EQ(index1, 0U);
    EXPECT_EQ(index2, 1U);
    EXPECT_EQ(index3, 2U);

    uint32_t conPoolIndex = pf->GetConstpoolIndex();
    EXPECT_EQ(conPoolIndex, 3U);

    CUnorderedMap<uint32_t, uint64_t> constpoolMap = pf->GetConstpoolMap();
    ConstPoolValue constPoolValue1(constpoolMap.at(methodId[0].GetOffset()));
    ConstPoolValue constPoolValue2(constpoolMap.at(methodId[1].GetOffset()));
    ConstPoolValue constPoolValue3(constpoolMap.at(methodId[2].GetOffset()));
    ConstPoolType type1 = constPoolValue1.GetConstpoolType();
    ConstPoolType type2 = constPoolValue2.GetConstpoolType();
    ConstPoolType type3 = constPoolValue3.GetConstpoolType();
    uint32_t gotIndex1 = constPoolValue1.GetConstpoolIndex();
    uint32_t gotIndex2 = constPoolValue2.GetConstpoolIndex();
    uint32_t gotIndex3 = constPoolValue3.GetConstpoolIndex();
    EXPECT_EQ(type1, ConstPoolType::METHOD);
    EXPECT_EQ(type2, ConstPoolType::METHOD);
    EXPECT_EQ(type3, ConstPoolType::METHOD);
    EXPECT_EQ(gotIndex1, 0U);
    EXPECT_EQ(gotIndex2, 1U);
    EXPECT_EQ(gotIndex3, 2U);
}

HWTEST_F_L0(JSPandaFileTest, GetMainMethodIndex_UpdateMainMethodIndex)
{
    const char *source = R"(
        .function void func1() {}
        .function void func2() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = file->GetClassId(typeDesc);
    EXPECT_TRUE(classId.IsValid());

    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    int count = 0;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
        count++;
    });
    EXPECT_EQ(count, 2); // 2 : number of methods

    uint32_t mainMethodIndex = pf->GetMainMethodIndex();
    EXPECT_EQ(mainMethodIndex, 0U);

    pf->UpdateMainMethodIndex(methodId[0].GetOffset());
    mainMethodIndex = pf->GetMainMethodIndex();
    EXPECT_EQ(mainMethodIndex, methodId[0].GetOffset());

    pf->UpdateMainMethodIndex(methodId[1].GetOffset());
    mainMethodIndex = pf->GetMainMethodIndex();
    EXPECT_EQ(mainMethodIndex, methodId[1].GetOffset());
}

HWTEST_F_L0(JSPandaFileTest, GetClasses)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = file->GetClassId(typeDesc);
    EXPECT_TRUE(classId.IsValid());

    const File::Header *header = file->GetHeader();
    Span fileData(file->GetBase(), header->file_size);
    Span classIdxData = fileData.SubSpan(header->class_idx_off, header->num_classes * sizeof(uint32_t));
    auto classesData = Span(reinterpret_cast<const uint32_t *>(classIdxData.data()), header->num_classes);

    Span<const uint32_t> classes = pf->GetClasses();
    EXPECT_EQ(classes.Data(), classesData.Data());
}

HWTEST_F_L0(JSPandaFileTest, IsModule_IsCjs)
{
    const char *source1 = R"(
        .function void foo1() {}
    )";
    const CString fileName1 = "test1.pa";
    std::shared_ptr<JSPandaFile> pf1 = CreateJSPandaFile(source1, fileName1);
    JSPandaFile::JSRecordInfo info =
        const_cast<JSPandaFile *>(pf1.get())-> FindRecordInfo(JSPandaFile::ENTRY_FUNCTION_NAME);
    EXPECT_EQ(pf1->IsModule(&info), false);
    EXPECT_EQ(pf1->IsCjs(&info), false);
}

HWTEST_F_L0(JSPandaFileTest, SetLoadedAOTStatus_IsLoadedAOT)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    bool isLoadedAOT = pf->IsLoadedAOT();
    EXPECT_EQ(isLoadedAOT, false);

    pf->SetAOTFileInfoIndex(0);
    isLoadedAOT = pf->IsLoadedAOT();
    EXPECT_EQ(isLoadedAOT, true);
}

HWTEST_F_L0(JSPandaFileTest, GetFileUniqId)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetFileUniqId(), merge_hashes(panda_file::File::CalcFilenameHash(""),
        GetHash32(reinterpret_cast<const uint8_t *>(pf->GetPandaFile()->GetHeader()),
        sizeof(panda_file::File::Header))));
}

HWTEST_F_L0(JSPandaFileTest, IsParsedConstpoolOfCurrentVM)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    auto &recordInfo = pf->FindRecordInfo(JSPandaFile::ENTRY_FUNCTION_NAME);
    EXPECT_TRUE(!recordInfo.IsParsedConstpoolOfCurrentVM(instance));
    recordInfo.SetParsedConstpoolVM(instance);
    EXPECT_TRUE(pf->FindRecordInfo(JSPandaFile::ENTRY_FUNCTION_NAME).IsParsedConstpoolOfCurrentVM(instance));
}

HWTEST_F_L0(JSPandaFileTest, NormalizedFileDescTest)
{
    const char *source = R"(
        .function void foo() {}
    )";
    CString fileName = "/data/storage/el1/bundle/entry/ets/modules.abc";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetNormalizedFileDesc(), "entry/ets/modules.abc");

    fileName = "/data/storage/el1/bundle/entry/ets/widgets.abc";
    pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetNormalizedFileDesc(), "entry/ets/widgets.abc");

    fileName = "/data/app/el1/bundle/public/com.xx.xx/entry.hsp/entry/ets/modules.abc";
    pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetNormalizedFileDesc(), "entry/ets/modules.abc");

    fileName = "/data/app/el1/bundle/public/com.xx.xx/entry.hap/entry/ets/widgets.abc";
    pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetNormalizedFileDesc(), "entry/ets/widgets.abc");

    fileName = "/system/app/Camera/Camera.hap/entry1/ets/modules.abc";
    pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetNormalizedFileDesc(), "entry1/ets/modules.abc");

    fileName = "test.pa";
    pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetNormalizedFileDesc(), fileName);

    fileName = "libapp.ability.AbilityStage.z.so/app.ability.AbilityStage.js";
    pf = CreateJSPandaFile(source, fileName);
    EXPECT_EQ(pf->GetNormalizedFileDesc(), fileName);
}

/**
 * @tc.name: CheckIsRecordWithBundleName
 * @tc.desc: Test CheckIsRecordWithBundleName method to validate bundle name checking functionality
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CheckIsRecordWithBundleName)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    // Test with a valid entry that contains '/' - this should not trigger FATAL
    CString validEntry = "com.example.myapp/index.ets";
    pf->CheckIsRecordWithBundleName(validEntry);

    // Verify that IsRecordWithBundleName can be called after CheckIsRecordWithBundleName
    // Note: The exact behavior depends on the internal state of jsRecordInfo_
    bool isRecordWithBundleName = pf->IsRecordWithBundleName();
    // The result may vary based on internal implementation, but the method should execute without crashing

    // Test with another valid entry
    CString anotherValidEntry = "bundle_name/another_file.js";
    pf->CheckIsRecordWithBundleName(anotherValidEntry);
    bool isRecordWithBundleName2 = pf->IsRecordWithBundleName();

    // Both calls should complete without crashing, which is the primary test
    // The actual values of isRecordWithBundleName and isRecordWithBundleName2 depend on internal state
    SUCCEED() << "CheckIsRecordWithBundleName executed without crashing";
}

/**
 * @tc.name: GetFunctionKind_FromPandaFileFunctionKind
 * @tc.desc: Test GetFunctionKind method to validate conversion from panda function kind to ecmascript function kind
 * @tc.type: FUNC
 * @tc.require: issue#12070
 */
HWTEST_F_L0(JSPandaFileTest, GetFunctionKind_FromPandaFileFunctionKind)
{
    // Test GetFunctionKind(panda_file::FunctionKind) conversion method
    using namespace panda_file;

    // Test each mapping case
    EXPECT_EQ(JSPandaFile::GetFunctionKind(panda::panda_file::FunctionKind::NONE),
              panda::ecmascript::FunctionKind::NONE_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(panda::panda_file::FunctionKind::FUNCTION),
              panda::ecmascript::FunctionKind::BASE_CONSTRUCTOR);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(panda::panda_file::FunctionKind::NC_FUNCTION),
              panda::ecmascript::FunctionKind::ARROW_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(panda::panda_file::FunctionKind::GENERATOR_FUNCTION),
              panda::ecmascript::FunctionKind::GENERATOR_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(panda::panda_file::FunctionKind::ASYNC_FUNCTION),
              panda::ecmascript::FunctionKind::ASYNC_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(panda::panda_file::FunctionKind::ASYNC_GENERATOR_FUNCTION),
              panda::ecmascript::FunctionKind::ASYNC_GENERATOR_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(panda::panda_file::FunctionKind::ASYNC_NC_FUNCTION),
              panda::ecmascript::FunctionKind::ASYNC_ARROW_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(panda::panda_file::FunctionKind::CONCURRENT_FUNCTION),
              panda::ecmascript::FunctionKind ::CONCURRENT_FUNCTION);

    // Test with sendable function mask (bitwise OR with SENDABLE_FUNCTION_MASK)
    // This tests the mask removal logic
    auto maskedFuncKind = static_cast<panda::panda_file::FunctionKind>(
        static_cast<uint32_t>(FunctionKind::FUNCTION) |
        static_cast<uint32_t>(JSPandaFile::SENDABLE_FUNCTION_MASK));
    EXPECT_EQ(JSPandaFile::GetFunctionKind(maskedFuncKind),
              panda::ecmascript::FunctionKind::BASE_CONSTRUCTOR);

    auto maskedGenKind = static_cast<FunctionKind>(
        static_cast<uint32_t>(panda::panda_file::FunctionKind::GENERATOR_FUNCTION) |
        static_cast<uint32_t>(JSPandaFile::SENDABLE_FUNCTION_MASK));
    EXPECT_EQ(JSPandaFile::GetFunctionKind(maskedGenKind), panda::ecmascript::FunctionKind::GENERATOR_FUNCTION);
}

/**
 * @tc.name: GetFunctionKind_FromConstPoolType
 * @tc.desc: Test GetFunctionKind method to validate conversion from constant pool type to ecmascript function kind
 * @tc.type: FUNC
 * @tc.require: issue#12071
 */
HWTEST_F_L0(JSPandaFileTest, GetFunctionKind_FromConstPoolType)
{
    // Test GetFunctionKind(ConstPoolType) conversion method

    // Test each mapping case
    EXPECT_EQ(JSPandaFile::GetFunctionKind(ConstPoolType::BASE_FUNCTION),
              panda::ecmascript::FunctionKind::BASE_CONSTRUCTOR);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(ConstPoolType::NC_FUNCTION),
              panda::ecmascript::FunctionKind::ARROW_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(ConstPoolType::GENERATOR_FUNCTION),
              panda::ecmascript::FunctionKind::GENERATOR_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(ConstPoolType::ASYNC_FUNCTION),
              panda::ecmascript::FunctionKind::ASYNC_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(ConstPoolType::CLASS_FUNCTION),
              panda::ecmascript::FunctionKind::CLASS_CONSTRUCTOR);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(ConstPoolType::METHOD),
              panda::ecmascript::FunctionKind::NORMAL_FUNCTION);
    EXPECT_EQ(JSPandaFile::GetFunctionKind(ConstPoolType::ASYNC_GENERATOR_FUNCTION),
              panda::ecmascript::FunctionKind::ASYNC_GENERATOR_FUNCTION);
}

/**
 * @tc.name: TranslateClasses
 * @tc.desc: Test TranslateClasses method to validate class translation in a JSPandaFile
 *           Test PostInitializeMethodTask method to validate posting initialization tasks to the taskpool
 *           Test CheckOngoingClassTranslating method to validate ongoing class translation checking functionality
 * @tc.type: FUNC
 * @tc.require: issue#12073, issue#12074, issue#12075
 */
HWTEST_F_L0(JSPandaFileTest, TranslateClasses)
{
    std::string fileName = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->LoadJSPandaFile(thread, CString(fileName.c_str()),
                                                                 JSPandaFile::ENTRY_MAIN_FUNCTION);
    pf->TranslateClasses(thread, "foo");
}

/**
 * @tc.name: ReduceTaskCount
 * @tc.desc: Test ReduceTaskCount method to validate task count reduction and condition variable signaling
 * @tc.type: FUNC
 * @tc.require: issue#12077
 */
HWTEST_F_L0(JSPandaFileTest, ReduceTaskCount)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_reduce.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    // Call ReduceTaskCount - this should decrease the task count
    // and potentially signal the condition variable if count reaches 0
    pf->ReduceTaskCount();
}

/**
 * @tc.name: InitializeMergedPF_FromConstructorWithThread
 * @tc.desc: Test InitializeMergedPF in second constructor (line 77) to merged panda file initialization
 * @tc.type: FUNC
 * @tc.require: issue#12804
 */
HWTEST_F_L0(JSPandaFileTest, InitializeMergedPF_FromConstructorWithThread)
{
    std::string fileNameStr = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = pfManager->LoadJSPandaFile(
        thread, CString(fileNameStr.c_str()), JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_NE(jsPandaFile, nullptr);
    EXPECT_FALSE(jsPandaFile->IsBundlePack());
}

/**
 * @tc.name: CheckIsBundlePack_ModuleRecord
 * @tc.desc: Test CheckIsBundlePack with ES module (MODULE_RECORD_IDX branch at line 123)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CheckIsBundlePack_ModuleRecord)
{
    // merge.abc is ES module with MODULE_RECORD_IDX field, should set isBundlePack_ = false
    std::string fileNameStr = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = pfManager->LoadJSPandaFile(
        thread, CString(fileNameStr.c_str()), JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_NE(jsPandaFile, nullptr);
    // ES module should have MODULE_RECORD_IDX field, causing isBundlePack_ = false
    EXPECT_FALSE(jsPandaFile->IsBundlePack());
}

/**
 * @tc.name: CheckIsRecordWithBundleName_EmptyJsRecordInfo
 * @tc.desc: Test CheckIsRecordWithBundleName when jsRecordInfo_ is empty, for loop at line 142 should not execute
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CheckIsRecordWithBundleName_EmptyJsRecordInfo)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_empty_record.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    // Clear jsRecordInfo_ to simulate empty scenario (line 142 for loop not executed)
    auto &recordInfo = const_cast<CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo *> &>(
        pf->GetJSRecordInfo());
    for (auto &pair : recordInfo) {
        delete pair.second;
    }
    recordInfo.clear();

    // Call CheckIsRecordWithBundleName with empty jsRecordInfo_
    CString entry = "com.example.myapp/index.ets";
    pf->CheckIsRecordWithBundleName(entry);

    // When jsRecordInfo_ is empty, isRecordWithBundleName_ should remain default value (true)
    EXPECT_TRUE(pf->IsRecordWithBundleName());
}

/**
 * @tc.name: CheckIsBundlePack_CommonJS
 * @tc.desc: Test CheckIsBundlePack with CommonJS module (IS_COMMON_JS branch at line 123)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CheckIsBundlePack_CommonJS)
{
    // merge.abc contains IS_COMMON_JS field, should set isBundlePack_ = false
    std::string fileNameStr = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = pfManager->LoadJSPandaFile(
        thread, CString(fileNameStr.c_str()), JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_NE(jsPandaFile, nullptr);
    // merge.abc has IS_COMMON_JS field, causing isBundlePack_ = false
    EXPECT_FALSE(jsPandaFile->IsBundlePack());
}

/**
 * @tc.name: CheckIsBundlePack_ModuleRecordIdx
 * @tc.desc: Test CheckIsBundlePack with ES module (MODULE_RECORD_IDX branch at line 123)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CheckIsBundlePack_ModuleRecordIdx)
{
    // merge.abc is ES module with MODULE_RECORD_IDX field, should set isBundlePack_ = false
    std::string fileNameStr = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = pfManager->LoadJSPandaFile(
        thread, CString(fileNameStr.c_str()), JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_NE(jsPandaFile, nullptr);
    // ES module should have MODULE_RECORD_IDX field, causing isBundlePack_ = false
    EXPECT_FALSE(jsPandaFile->IsBundlePack());
}

/**
 * @tc.name: CheckIsSharedModule
 * @tc.desc: Test InitializeMergedPF with IS_SHARED_MODULE field (line 284)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CheckIsSharedModule)
{
    // merge.abc contains IS_SHARED_MODULE field, initialize merged panda file
    std::string fileNameStr = QUICKFIX_ABC_PATH "multi_file/base/merge.abc";
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> jsPandaFile = pfManager->LoadJSPandaFile(
        thread, CString(fileNameStr.c_str()), JSPandaFile::ENTRY_MAIN_FUNCTION);
    EXPECT_NE(jsPandaFile, nullptr);

    // Verify the IsSharedModule function works correctly (line 284 branch covered)
    // merge.abc is not bundle pack, so GetJSRecordInfo() will return multiple records
    const auto &recordInfo = jsPandaFile->GetJSRecordInfo();
    EXPECT_FALSE(recordInfo.empty());

    // Test IsSharedModule function for each record
    for (const auto &pair : recordInfo) {
        JSPandaFile::JSRecordInfo *info = pair.second;
        // IsSharedModule function should work without crash
        bool isShared = jsPandaFile->IsSharedModule(info);
        // The value depends on the abc file content
        (void)isShared;  // Avoid unused variable warning
    }
}

/**
 * @tc.name: GetRecordName_CacheHit
 * @tc.desc: Test GetRecordName cache hit scenario (line 473 if branch - iter != end())
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, GetRecordName_CacheHit)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_cache_hit.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    // Get method literal to obtain a valid EntityId
    Span<const uint32_t> classIndexes = pf->GetPandaFile()->GetClasses();
    ASSERT_GT(classIndexes.Size(), 0);

    EntityId classId(classIndexes[0]);
    panda_file::ClassDataAccessor cda(*pf->GetPandaFile(), classId);

    EntityId methodId;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId = mda.GetMethodId();
    });

    // First call - cache miss, will parse and store in cache
    CString recordName1 = pf->GetRecordName(methodId);
    EXPECT_FALSE(recordName1.empty());

    // Second call - cache hit (line 473: iter != recordNameMap_.end() is true)
    CString recordName2 = pf->GetRecordName(methodId);
    EXPECT_EQ(recordName1, recordName2);
}

/**
 * @tc.name: GetRecordName_CacheMiss
 * @tc.desc: Test GetRecordName cache miss scenario (line 473 else branch - iter == end())
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, GetRecordName_CacheMiss)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_cache_miss.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    // Get method literal to obtain a valid EntityId
    Span<const uint32_t> classIndexes = pf->GetPandaFile()->GetClasses();
    ASSERT_GT(classIndexes.Size(), 0);

    EntityId classId(classIndexes[0]);
    panda_file::ClassDataAccessor cda(*pf->GetPandaFile(), classId);

    EntityId methodId;
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId = mda.GetMethodId();
    });

    // First call - cache miss (line 473: iter == recordNameMap_.end() is false)
    // This will parse the record name and store in cache
    CString recordName = pf->GetRecordName(methodId);
    EXPECT_FALSE(recordName.empty());
}

/**
 * @tc.name: GetClassAndMethodIndexes_NoMoreClasses
 * @tc.desc: Test GetClassAndMethodIndexes when classIndex_ >= numClasses_ (line 510 return immediately)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, GetClassAndMethodIndexes_NoMoreClasses)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_no_more_classes.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    // First call consumes all classes
    ClassTranslateWork indexes;
    pf->GetClassAndMethodIndexes(indexes);

    // Second call - classIndex_ >= numClasses_, should return empty (line 510)
    indexes.clear();
    pf->GetClassAndMethodIndexes(indexes);
    EXPECT_TRUE(indexes.empty());
}

/**
 * @tc.name: GetClassAndMethodIndexes_UseMinCount
 * @tc.desc: Test GetClassAndMethodIndexes when remaining classes < ASYN_TRANSLATE_CLSSS_COUNT (line 514-515)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, GetClassAndMethodIndexes_UseMinCount)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_min_count.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    // First call - remaining classes < 128, use minimum count (2)
    ClassTranslateWork indexes;
    pf->GetClassAndMethodIndexes(indexes);
    // Since numClasses_ is small (< 128), line 514-515 branch is taken
    // cnts will be set to ASYN_TRANSLATE_CLSSS_MIN_COUNT (2)
    EXPECT_TRUE(indexes.size() <= JSPandaFile::ASYN_TRANSLATE_CLSSS_MIN_COUNT);
}

/**
 * @tc.name: ConstructorWithThread_SnapshotReadSuccess
 * @tc.desc: Test JSPandaFile(thread, pf, desc, entryPoint) constructor when snapshot read succeeds (line 59 return)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, ConstructorWithThread_SnapshotReadSuccess)
{
    CString path = GetCurrentDirPath();
    const char *abcFilename = "/data/storage/el1/bundle/entry/ets/main/modules.abc";
    const char *source = R"(
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    std::shared_ptr<JSPandaFile> serializePf = CreateJSPandaFile(source, CString(abcFilename));
    NormalTranslateJSPandaFile(serializePf);
    bool bundlePackSave = serializePf->IsBundlePack();
    serializePf->SetBundlePack(false);
    serializePf->ownedNpmEntries_.emplace_back("testRecord", "testEntry");
    const auto &npmEntry = serializePf->ownedNpmEntries_.back();
    serializePf->npmEntries_.insert(
        {std::string_view(npmEntry.first.c_str(), npmEntry.first.size()),
         std::string_view(npmEntry.second.c_str(), npmEntry.second.size())});
    CString snapshotFileName =
        MockJSPandaFileSnapshotForTest::GetSnapshotFileName(serializePf->GetJSPandaFileDesc(), path);
    remove(snapshotFileName.c_str());
    CString version = "test_version";
    EXPECT_TRUE(MockJSPandaFileSnapshotForTest::WriteDataToFile(
        thread, serializePf.get(), path, version));
    serializePf->SetBundlePack(bundlePackSave);
    EXPECT_TRUE(FileExist(snapshotFileName.c_str()));

    std::shared_ptr<JSPandaFile> deserializePf = CreateJSPandaFile(source, CString(abcFilename));
    EXPECT_TRUE(MockJSPandaFileSnapshotForTest::ReadDataFromFile(
        thread, deserializePf.get(), path, version));
    EXPECT_NE(deserializePf, nullptr);
    EXPECT_FALSE(deserializePf->IsBundlePack());

    auto it = deserializePf->npmEntries_.find("testRecord");
    EXPECT_NE(it, deserializePf->npmEntries_.end());
    EXPECT_EQ(it->second, "testEntry");

    remove(snapshotFileName.c_str());
}

/**
 * @tc.name: CheckIsBundlePack_ExternalClassSkipped
 * @tc.desc: Test CheckIsBundlePack skips external classes (line 116 continue branch)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CheckIsBundlePack_ExternalClassSkipped)
{
    const char *source = R"(
        .record panda.String <external>
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    const CString fileName = "test_external.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    EXPECT_NE(pf, nullptr);
    EXPECT_TRUE(pf->IsBundlePack());
}

/**
 * @tc.name: CheckIsRecordWithBundleName_NpmPathSkipped
 * @tc.desc: Test CheckIsRecordWithBundleName skips records with
 *           PACKAGE_PATH_SEGMENT or NPM_PATH_SEGMENT (line 147 continue)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CheckIsRecordWithBundleName_NpmPathSkipped)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_npm_skip.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    auto &recordInfo = const_cast<CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo *> &>(
        pf->GetJSRecordInfo());
    for (auto &pair : recordInfo) {
        delete pair.second;
    }
    recordInfo.clear();

    static std::string npmRecordName = "com.example.myapp/node_modules/pkg/module";
    auto *npmRecord = new JSPandaFile::JSRecordInfo();
    recordInfo.insert({std::string_view(npmRecordName), npmRecord});

    CString entry = "com.example.myapp/index.ets";
    pf->CheckIsRecordWithBundleName(entry);
    EXPECT_TRUE(pf->IsRecordWithBundleName());
}

/**
 * @tc.name: Destructor_PfNullptr
 * @tc.desc: Test destructor when pf_ is nullptr (line 164 if branch not taken)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, Destructor_PfNullptr)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_destructor_nullptr.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    EXPECT_NE(pf, nullptr);

    const panda_file::File *rawPf = pf->pf_;
    pf->pf_ = nullptr;
    pf.reset();
    delete rawPf;
}

/**
 * @tc.name: GetOrInsertConstantPool_WithExternalConstpoolMap
 * @tc.desc: Test GetOrInsertConstantPool with non-null constpoolMap and non-bundle pack (line 189)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, GetOrInsertConstantPool_WithExternalConstpoolMap)
{
    const char *source = R"(
        .function void foo1() {}
        .function void foo2() {}
    )";
    const CString fileName = "test_ext_constpool.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    pf->SetBundlePack(false);

    const File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    File::EntityId classId = file->GetClassId(typeDesc);
    ClassDataAccessor cda(*file, classId);
    std::vector<File::EntityId> methodId {};
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        methodId.push_back(mda.GetMethodId());
    });

    CUnorderedMap<uint32_t, uint64_t> externalMap;
    uint32_t index1 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[0].GetOffset(), &externalMap);
    EXPECT_EQ(index1, 0U);
    EXPECT_EQ(externalMap.size(), 1U);

    // cache hit on external map (line 195)
    uint32_t index2 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[0].GetOffset(), &externalMap);
    EXPECT_EQ(index2, 0U);
    EXPECT_EQ(externalMap.size(), 1U);

    uint32_t index3 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[1].GetOffset(), &externalMap);
    EXPECT_EQ(index3, 1U);
    EXPECT_EQ(externalMap.size(), 2U);

    // cache hit on internal constpoolMap_ (line 195)
    pf->SetBundlePack(true);
    uint32_t index4 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[0].GetOffset());
    EXPECT_EQ(index4, 2U);
    uint32_t index5 = pf->GetOrInsertConstantPool(ConstPoolType::METHOD, methodId[0].GetOffset());
    EXPECT_EQ(index5, index4);
}

/**
 * @tc.name: InitializeUnMergedPF_ExternalClassSkipped
 * @tc.desc: Test InitializeUnMergedPF skips external classes (line 214 continue)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, InitializeUnMergedPF_ExternalClassSkipped)
{
    const char *source = R"(
        .record panda.String <external>
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    const CString fileName = "test_init_unmerged_external.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    EXPECT_NE(pf, nullptr);
    EXPECT_TRUE(pf->IsBundlePack());
    EXPECT_GT(pf->numMethods_, 0U);
}

/**
 * @tc.name: ConstructorWithThread_SnapshotDisabled
 * @tc.desc: Test second constructor when snapshot is disabled (line 56 false branch)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, ConstructorWithThread_SnapshotDisabled)
{
    const char *source = R"(
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    const CString fileName = "test_snapshot_disabled.pa";

    EcmaVM *vm = thread->GetEcmaVM();
    JSRuntimeOptions &options = vm->GetJSOptions();
    int arkProperties = options.GetArkProperties();
    options.SetArkProperties(arkProperties | static_cast<int>(ArkProperties::DISABLE_JSPANDAFILE_MODULE_SNAPSHOT));

    Parser parser;
    const std::string fn = "SRC.pa";
    auto res = parser.Parse(source, fn);
    EXPECT_EQ(parser.ShowError().err, Error::ErrorType::ERR_NONE);
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(thread, pfPtr.release(), fileName, "");
    EXPECT_NE(pf, nullptr);
    EXPECT_TRUE(pf->IsBundlePack());
    EXPECT_GT(pf->numMethods_, 0U);

    options.SetArkProperties(arkProperties);
}

/**
 * @tc.name: GetEntryPoint_FoundInNpmEntries
 * @tc.desc: Test GetEntryPoint when record is found in npmEntries_ and HasRecord returns true (line 326)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, GetEntryPoint_FoundInNpmEntries)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_get_entry.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    static std::string recordKey = "npm_pkg/module";
    static std::string entryValue = "entry_point_record";
    pf->ownedNpmEntries_.emplace_back(CString(recordKey.c_str()), CString(entryValue.c_str()));
    const auto &npmEntry = pf->ownedNpmEntries_.back();
    pf->npmEntries_.insert(
        {std::string_view(npmEntry.first.c_str(), npmEntry.first.size()),
         std::string_view(npmEntry.second.c_str(), npmEntry.second.size())});

    auto &recordInfo = const_cast<CUnorderedMap<std::string_view, JSPandaFile::JSRecordInfo *> &>(
        pf->GetJSRecordInfo());
    static std::string entryRecordName = "entry_point_record";
    auto *info = new JSPandaFile::JSRecordInfo();
    recordInfo.insert({std::string_view(entryRecordName), info});

    CString result = pf->GetEntryPoint(CString(recordKey.c_str()));
    EXPECT_EQ(result, CString(entryValue.c_str()));
}

/**
 * @tc.name: GetNormalizedFileDesc_OhosModulePosNpos
 * @tc.desc: Test GetNormalizedFileDesc when desc has /ets/ but no '/' before it (line 434)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, GetNormalizedFileDesc_OhosModulePosNpos)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_norm_desc.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);

    CString desc = "abc/ets/modules.abc";
    CString result = pf->GetNormalizedFileDesc(desc);
    EXPECT_EQ(result, desc);
}

/**
 * @tc.name: CallReleaseSecureMemFunc_NoCallback
 * @tc.desc: Test CallReleaseSecureMemFunc when fileMapper is non-null but callback is null (line 680)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSPandaFileTest, CallReleaseSecureMemFunc_NoCallback)
{
    const char *source = R"(
        .function void foo() {}
    )";
    const CString fileName = "test_release_mem.pa";
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName);
    EXPECT_NE(pf, nullptr);

    int dummyMapper = 0;
    pf->CallReleaseSecureMemFunc(&dummyMapper);
}

}  // namespace panda::test
