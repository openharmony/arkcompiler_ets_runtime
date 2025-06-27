/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <zlib.h>
#include <unistd.h>
#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "libpandafile/class_data_accessor-inl.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_function_kind.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/js_pandafile_snapshot.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class MockJSPandaFileSnapshot : public JSPandaFileSnapshot {
public:
    static bool WriteDataToFile(JSThread *thread, JSPandaFile *jsPandaFile, const CString &path, const CString &version)
    {
        return JSPandaFileSnapshot::WriteDataToFile(thread, jsPandaFile, path, version);
    }
};
class JSPandaFileSnapshotTest : public testing::Test {
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
        CString path = GetSnapshotPath();
        CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
        if (remove(fileName.c_str()) != 0) {
            GTEST_LOG_(ERROR) << "remove " << fileName << " failed";
        }
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        CString path = GetSnapshotPath();
        CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
        if (remove(fileName.c_str()) != 0) {
            GTEST_LOG_(ERROR) << "remove " << fileName << " failed";
        }
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    static CString GetSnapshotPath()
    {
        char buff[FILENAME_MAX];
        getcwd(buff, FILENAME_MAX);
        CString currentPath(buff);
        if (currentPath.back() != '/') {
            currentPath += "/";
        }
        return currentPath;
    }

    std::shared_ptr<JSPandaFile> NewMockJSPandaFile() const
    {
        Parser parser;
        const char *filename = "/data/storage/el1/bundle/entry/ets/main/modules.abc";
        const char *data = R"(
            .function any func_main_0(any a0, any a1, any a2) {
                ldai 1
                return
            }
        )";
        auto res = parser.Parse(data);
        JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
        std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
        return pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
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

    static void CheckMethodLiteral(MethodLiteral *serializeMethodLiteral, MethodLiteral *deserializeMethodLiteral)
    {
        EXPECT_TRUE(serializeMethodLiteral != nullptr);
        EXPECT_TRUE(deserializeMethodLiteral != nullptr);
        ASSERT_EQ(serializeMethodLiteral->GetCallField(), deserializeMethodLiteral->GetCallField());
        ASSERT_EQ(*serializeMethodLiteral->GetBytecodeArray(), *deserializeMethodLiteral->GetBytecodeArray());
        ASSERT_EQ(serializeMethodLiteral->GetLiteralInfo(), deserializeMethodLiteral->GetLiteralInfo());
        ASSERT_EQ(serializeMethodLiteral->GetExtraLiteralInfo(), deserializeMethodLiteral->GetExtraLiteralInfo());
    }
    static void CheckJSRecordInfo(JSRecordInfo *serializeRecordInfo, JSRecordInfo *deserializeRecordInfo)
    {
        ASSERT_TRUE(serializeRecordInfo != nullptr);
        ASSERT_TRUE(deserializeRecordInfo != nullptr);
        ASSERT_EQ(serializeRecordInfo->mainMethodIndex, deserializeRecordInfo->mainMethodIndex);
        ASSERT_EQ(serializeRecordInfo->isCjs, deserializeRecordInfo->isCjs);
        ASSERT_EQ(serializeRecordInfo->isJson, deserializeRecordInfo->isJson);
        ASSERT_EQ(serializeRecordInfo->isSharedModule, deserializeRecordInfo->isSharedModule);
        ASSERT_EQ(serializeRecordInfo->jsonStringId, deserializeRecordInfo->jsonStringId);
        ASSERT_EQ(serializeRecordInfo->moduleRecordIdx, deserializeRecordInfo->moduleRecordIdx);
        ASSERT_EQ(serializeRecordInfo->hasTopLevelAwait, deserializeRecordInfo->hasTopLevelAwait);
        ASSERT_EQ(serializeRecordInfo->lazyImportIdx, deserializeRecordInfo->lazyImportIdx);
        ASSERT_EQ(serializeRecordInfo->classId, deserializeRecordInfo->classId);
        ASSERT_EQ(serializeRecordInfo->npmPackageName, deserializeRecordInfo->npmPackageName);
    }
    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JSPandaFileSnapshotTest, SerializeAndDeserializeTest)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    // serialize and persist
    ASSERT_TRUE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));

    // deserialize
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    JSPandaFile *deserializePf = pf.get();
    ASSERT_TRUE(JSPandaFileSnapshot::ReadData(thread, deserializePf, path, version));

    // check numMethods
    EXPECT_EQ(serializePf->GetNumMethods(), deserializePf->GetNumMethods());
    // check MethodLiterals
    ASSERT_EQ(serializePf->GetMainMethodIndex(), deserializePf->GetMainMethodIndex());
    MethodLiteral *serializeMethodLiteral = serializePf->FindMethodLiteral(serializePf->GetMainMethodIndex());
    MethodLiteral *deserializeMethodLiteral = deserializePf->FindMethodLiteral(deserializePf->GetMainMethodIndex());
    CheckMethodLiteral(serializeMethodLiteral, deserializeMethodLiteral);
    // check jsRecord
    JSRecordInfo *serializeRecordInfo = serializePf->CheckAndGetRecordInfo("func_main_0");
    JSRecordInfo *deserializeRecordInfo = deserializePf->CheckAndGetRecordInfo("func_main_0");
    CheckJSRecordInfo(serializeRecordInfo, deserializeRecordInfo);
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldNotSerializeWhenFileIsExists)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    // serialize and persist
    ASSERT_TRUE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // return false when file is already exists
    ASSERT_FALSE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldNotDeSerializeWhenFileIsNotExists)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    // return false when file is not exists
    ASSERT_FALSE(FileExist(fileName.c_str()));
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path, version));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldDeSerializeFailedWhenFileIsEmpty)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    std::ofstream ofStream(fileName.c_str());
    ofStream.close();
    // return false when file is empty
    ASSERT_TRUE(FileExist(fileName.c_str()));
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldDeSerializeFailedWhenCheckSumIsNotMatch)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    // serialize and persist
    ASSERT_TRUE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // modify file content
    std::ofstream ofStream(fileName.c_str(), std::ios::app);
    uint32_t mockCheckSum = 123456;
    ofStream << mockCheckSum;
    ofStream.close();
    // deserialize failed when checksum is not match
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldDeSerializeFailedWhenAppVersionCodeIsNotMatch)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    // serialize and persist
    ASSERT_TRUE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // modify app version code
    thread->GetEcmaVM()->SetApplicationVersionCode(1);
    // deserialize failed when app version code is not match
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldDeSerializeFailedWhenVersionCodeIsNotMatch)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    // serialize and persist
    ASSERT_TRUE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // deserialize failed when version code is not match
    CString updatedVersion = "version 205.0.1.125(SP20)";
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path, updatedVersion));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldDeSerializeFailedWhenFileSizeIsNotMatch)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    // serialize and persist
    ASSERT_TRUE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // construct a different JSPandaFile
    Parser parser;
    const char *filename = "/data/storage/el1/bundle/entry/ets/main/modules.abc";
    const char *data = R"(
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            ldai 2
            return
        }
    )";
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    const std::shared_ptr<JSPandaFile> deserializePf = pfManager->NewJSPandaFile(pfPtr.release(), CString(filename));
    // deserialize failed when file size is not match
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldDeSerializeFailedWhenModuleNameIsNotMatch)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    // serialize and persist
    ASSERT_TRUE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // change moduleName from entry to ntest
    uint32_t fileSize = sizeof(uint32_t);
    uint32_t appVersionCodeSize = sizeof(uint32_t);
    uint32_t versionStrLenSize = sizeof(uint32_t);
    uint32_t versionStrLen = version.size();
    uint32_t moduleNameLenSize = sizeof(uint32_t);
    uint32_t moduleNameOffset = appVersionCodeSize + versionStrLenSize + versionStrLen + fileSize + moduleNameLenSize;
    CString moduleName = "ntest";
    std::fstream fStream(fileName.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    fStream.seekp(moduleNameOffset);
    fStream << moduleName;
    fStream.close();
    // adapt checksum
    uint32_t newCheckSum;
    uint32_t checksumSize = sizeof(uint32_t);
    uint32_t contentSize;
    {
        MemMap fileMapMem = FileMap(fileName.c_str(), FILE_RDONLY, PAGE_PROT_READ, 0);
        MemMapScope memMapScope(fileMapMem);
        contentSize = fileMapMem.GetSize() - checksumSize;
        newCheckSum = adler32(0, static_cast<const Bytef*>(fileMapMem.GetOriginAddr()), contentSize);
    }
    fStream.open(fileName.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
    fStream.seekp(contentSize);
    fStream.write(reinterpret_cast<char *>(&newCheckSum), checksumSize);
    fStream.close();
    // deserialize failed when module Name is not match
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}
}