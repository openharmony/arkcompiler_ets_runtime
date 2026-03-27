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
#include <vector>
#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "class_data_accessor-inl.h"

#define private public
#define protected public
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/jspandafile/js_pandafile_snapshot.h"
#include "ecmascript/jspandafile/js_pandafile_record_info_snapshot.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#undef protected
#undef private
#include "ecmascript/global_env.h"
#include "ecmascript/js_function_kind.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/ohos/ohos_constants.h"
#include "ecmascript/ohos/ohos_version_info_tools.h"
#include "ecmascript/platform/filesystem.h"
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
        CString stateFilePath = path + ModulesSnapshotHelper::MODULE_SNAPSHOT_STATE_FILE_NAME.data();
        if (FileExist(stateFilePath.c_str())) {
            if (remove(stateFilePath.c_str()) != 0) {
                GTEST_LOG_(ERROR) << "remove '" << stateFilePath << "' failed when setup";
            }
        }
        disabledFeature = ModulesSnapshotHelper::g_featureState_;
        loadedFeature = ModulesSnapshotHelper::g_featureLoaded_;
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        CString path = GetSnapshotPath();
        CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
        if (remove(fileName.c_str()) != 0) {
            GTEST_LOG_(ERROR) << "remove " << fileName << " failed";
        }
        ModulesSnapshotHelper::g_featureState_ = disabledFeature;
        ModulesSnapshotHelper::g_featureLoaded_ = loadedFeature;
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

    std::shared_ptr<JSPandaFile> NewMockJSPandaFileWithSnapshotLoad() const
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
        return JSPandaFileManager::GetInstance()->NewJSPandaFile(
            thread, pandasm::AsmEmitter::Emit(res.Value()).release(), CString(filename), "");
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
    static void CheckNpmEntries(const CUnorderedMap<std::string_view, std::string_view> &serializeNpmEntries,
                                const CUnorderedMap<std::string_view, std::string_view> &deserializeNpmEntries)
    {
        ASSERT_EQ(serializeNpmEntries.size(), deserializeNpmEntries.size());
        for (const auto &[key, value] : serializeNpmEntries) {
            auto it = deserializeNpmEntries.find(key);
            ASSERT_TRUE(it != deserializeNpmEntries.end());
            ASSERT_EQ(it->second, value);
        }
    }

    static bool WriteBufferData(std::vector<uint8_t> &buffer, size_t &offset, size_t bufLen, const void *src, size_t n)
    {
        if (memcpy_s(buffer.data() + offset, bufLen - offset, src, n) != EOK) {
            return false;
        }
        offset += n;
        return true;
    }

    static std::vector<uint8_t> BuildValidRecordInfoSectionBuffer()
    {
        uint32_t numClasses = 1;
        uint32_t numMethods = 1;
        uint8_t isBundlePack = 0;
        uint32_t recordCount = 1;
        const char *recordName = "test";
        uint32_t recordNameLen = std::strlen(recordName);
        uint8_t flags = 1;
        uint32_t jsonStringId = 1;
        uint32_t moduleRecordIdx = 1;
        uint32_t lazyImportIdx = 1;
        uint32_t classId = 1;
        const char *pkgName = "@ohos/testpkg";
        uint32_t pkgNameLen = std::strlen(pkgName);
        uint32_t npmCount = 1;
        const char *key = "key";
        uint32_t keyLen = std::strlen(key);
        const char *value = "value";
        uint32_t valLen = std::strlen(value);
        constexpr size_t baseSectionSize = sizeof(numClasses) + sizeof(numMethods) + sizeof(isBundlePack);
        const size_t jsRecordInfoSize = sizeof(recordCount) + sizeof(recordNameLen) + recordNameLen + sizeof(flags) +
                                        sizeof(jsonStringId) + sizeof(moduleRecordIdx) + sizeof(lazyImportIdx) +
                                        sizeof(classId) + sizeof(pkgNameLen) + pkgNameLen;
        const size_t npmEntriesSize = sizeof(npmCount) + sizeof(keyLen) + keyLen + sizeof(valLen) + valLen;
        const size_t bufLen = baseSectionSize + jsRecordInfoSize + npmEntriesSize;
        std::vector<uint8_t> buffer(bufLen, 0);
        size_t offset = 0;
        WriteBufferData(buffer, offset, bufLen, &numClasses, sizeof(numClasses));
        WriteBufferData(buffer, offset, bufLen, &numMethods, sizeof(numMethods));
        WriteBufferData(buffer, offset, bufLen, &isBundlePack, sizeof(isBundlePack));
        WriteBufferData(buffer, offset, bufLen, &recordCount, sizeof(recordCount));
        WriteBufferData(buffer, offset, bufLen, &recordNameLen, sizeof(recordNameLen));
        WriteBufferData(buffer, offset, bufLen, recordName, recordNameLen);
        WriteBufferData(buffer, offset, bufLen, &flags, sizeof(flags));
        WriteBufferData(buffer, offset, bufLen, &jsonStringId, sizeof(jsonStringId));
        WriteBufferData(buffer, offset, bufLen, &moduleRecordIdx, sizeof(moduleRecordIdx));
        WriteBufferData(buffer, offset, bufLen, &lazyImportIdx, sizeof(lazyImportIdx));
        WriteBufferData(buffer, offset, bufLen, &classId, sizeof(classId));
        WriteBufferData(buffer, offset, bufLen, &pkgNameLen, sizeof(pkgNameLen));
        WriteBufferData(buffer, offset, bufLen, pkgName, pkgNameLen);
        WriteBufferData(buffer, offset, bufLen, &npmCount, sizeof(npmCount));
        WriteBufferData(buffer, offset, bufLen, &keyLen, sizeof(keyLen));
        WriteBufferData(buffer, offset, bufLen, key, keyLen);
        WriteBufferData(buffer, offset, bufLen, &valLen, sizeof(valLen));
        WriteBufferData(buffer, offset, bufLen, value, valLen);
        return buffer;
    }

    bool ReadCorruptRecordInfoSection(size_t mappedSize, const char *sceneName) const
    {
        const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
        pf->ResetAfterSnapshotFail();
        pf->SetBundlePack(false);

        std::vector<uint8_t> buffer = JSPandaFileSnapshotTest::BuildValidRecordInfoSectionBuffer();
        MemMap memMap(buffer.data(), mappedSize);
        FileMemMapReader reader(memMap, []() {}, sceneName);
        return JSPandaFileRecordInfoSnapshot::ReadRecordInfoSection(pf.get(), reader);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    int disabledFeature { 0 };
    int loadedFeature { 0 };
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
    ASSERT_TRUE(JSPandaFileSnapshot::ReadDataFromFile(thread, deserializePf, path, version));

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
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    // return false when file is not exists
    ASSERT_FALSE(FileExist(fileName.c_str()));
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ShouldDeSerializeFailedWhenFileIsEmpty)
{
    // construct JSPandaFile
    CString path = GetSnapshotPath();
    CString fileName = path + "entry" + JSPandaFileSnapshot::JSPANDAFILE_FILE_NAME.data();
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    std::ofstream ofStream(fileName.c_str());
    ofStream.close();
    // return false when file is empty
    ASSERT_TRUE(FileExist(fileName.c_str()));
    ASSERT_FALSE(JSPandaFileSnapshot::ReadData(thread, deserializePf.get(), path));
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
    Chmod(fileName, "rw");
    std::ofstream ofStream(fileName.c_str(), std::ios::app);
    uint32_t mockCheckSum = 123456;
    ofStream << mockCheckSum;
    ofStream.close();
    // deserialize failed when checksum is not match
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(JSPandaFileSnapshot::ReadDataFromFile(thread, deserializePf.get(), path, version));
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
    ASSERT_FALSE(JSPandaFileSnapshot::ReadDataFromFile(thread, deserializePf.get(), path, version));
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
    ASSERT_FALSE(JSPandaFileSnapshot::ReadDataFromFile(thread, deserializePf.get(), path, updatedVersion));
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
    ASSERT_FALSE(JSPandaFileSnapshot::ReadDataFromFile(thread, deserializePf.get(), path, version));
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
    Chmod(fileName.c_str(), "rw");
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
    ASSERT_FALSE(JSPandaFileSnapshot::ReadDataFromFile(thread, deserializePf.get(), path, version));
    // check file is deleted
    ASSERT_FALSE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, WriteAndReadRecordInfoSectionWithMergedPF)
{
    // serialize
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    bool bundlePackSave = serializePf->IsBundlePack();
    serializePf->SetBundlePack(false);
    JSRecordInfo *recordInfo = serializePf->CheckAndGetRecordInfo("func_main_0");
    if (recordInfo != nullptr) {
        recordInfo->npmPackageName = "@ohos/testpkg";
        recordInfo->isCjs = true;
        recordInfo->isJson = false;
        recordInfo->isSharedModule = true;
        recordInfo->hasTopLevelAwait = false;
    }
    serializePf->ownedNpmEntries_.emplace_back("testRecord", "testEntry");
    const auto &serializeNpmEntry = serializePf->ownedNpmEntries_.back();
    serializePf->npmEntries_.insert(
        {std::string_view(serializeNpmEntry.first.c_str(), serializeNpmEntry.first.size()),
         std::string_view(serializeNpmEntry.second.c_str(), serializeNpmEntry.second.size())});
    ASSERT_TRUE(MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path, version));
    serializePf->SetBundlePack(bundlePackSave);

    // deserialize
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    deserializePf->jsRecordInfo_.clear();
    deserializePf->npmEntries_.clear();
    deserializePf->methodLiterals_ = nullptr;
#if ENABLE_LATEST_OPTIMIZATION
    deserializePf->methodLiteralMap_.Clear();
#else
    deserializePf->methodLiteralMap_.clear();
#endif
    ASSERT_TRUE(JSPandaFileSnapshot::ReadDataFromFile(thread, deserializePf.get(), path, version));

    ASSERT_TRUE(deserializePf != nullptr);
    ASSERT_FALSE(deserializePf->IsBundlePack());

    JSRecordInfo *serializeRecordInfo = serializePf->CheckAndGetRecordInfo("func_main_0");
    JSRecordInfo *deserializeRecordInfo = deserializePf->CheckAndGetRecordInfo("func_main_0");
    CheckJSRecordInfo(serializeRecordInfo, deserializeRecordInfo);

    CheckNpmEntries(serializePf->npmEntries_, deserializePf->npmEntries_);
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ReadRecordInfoSection)
{
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    pf->ResetAfterSnapshotFail();
    pf->SetBundlePack(false);

    std::vector<uint8_t> buffer = JSPandaFileSnapshotTest::BuildValidRecordInfoSectionBuffer();
    MemMap memMap(buffer.data(), buffer.size());
    FileMemMapReader reader(memMap, []() {}, "ReadRecordInfoSection");
    ASSERT_TRUE(JSPandaFileRecordInfoSnapshot::ReadRecordInfoSection(pf.get(), reader));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, WriteRecordInfoSection_FailBundlePack)
{
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(pf);
    pf->SetBundlePack(true);

    std::vector<uint8_t> buffer(64, 0);
    MemMap memMap(buffer.data(), buffer.size());
    FileMemMapWriter writer(memMap, "WriteRecordInfoSection_BundlePack");
    ASSERT_FALSE(JSPandaFileRecordInfoSnapshot::WriteRecordInfoSection(thread->GetEcmaVM(), pf.get(), writer));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, WriteRecordInfoSection_EmptyPkgName)
{
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(pf);
    bool bundlePackSave = pf->IsBundlePack();
    pf->SetBundlePack(false);
    JSRecordInfo *recordInfo = pf->CheckAndGetRecordInfo("func_main_0");
    ASSERT_NE(recordInfo, nullptr);
    recordInfo->npmPackageName.clear();
    std::vector<uint8_t> buffer(JSPandaFileRecordInfoSnapshot::CalculateRecordInfoSectionSize(pf.get()), 0);
    MemMap memMap(buffer.data(), buffer.size());
    FileMemMapWriter writer(memMap, "WriteRecordInfoSection_EmptyPkgName");
    ASSERT_TRUE(JSPandaFileRecordInfoSnapshot::WriteRecordInfoSection(thread->GetEcmaVM(), pf.get(), writer));
    pf->SetBundlePack(bundlePackSave);
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ReadRecordInfoSection_EmptyPkgNameAndZeroMethods)
{
    std::vector<uint8_t> buffer = BuildValidRecordInfoSectionBuffer();
    constexpr uint32_t zero = 0;
    constexpr size_t numMethodsOffset = sizeof(uint32_t);
    constexpr size_t recordNameLen = sizeof("test") - 1;
    constexpr size_t pkgNameLenOffset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) +
                                        sizeof(uint32_t) + recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) * 4;
    buffer.resize(pkgNameLenOffset + sizeof(uint32_t) * 2);
    size_t offset = numMethodsOffset;
    WriteBufferData(buffer, offset, buffer.size(), &zero, sizeof(uint32_t));
    offset = pkgNameLenOffset;
    WriteBufferData(buffer, offset, buffer.size(), &zero, sizeof(uint32_t));
    WriteBufferData(buffer, offset, buffer.size(), &zero, sizeof(uint32_t));
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    pf->jsRecordInfo_.clear();
    pf->npmEntries_.clear();
    pf->ownedRecordNames_.clear();
    pf->ownedNpmEntries_.clear();
    pf->methodLiterals_ = nullptr;
    pf->numMethods_ = 0;
#if ENABLE_LATEST_OPTIMIZATION
    pf->methodLiteralMap_.Clear();
#else
    pf->methodLiteralMap_.clear();
#endif
    MemMap memMap(buffer.data(), buffer.size());
    FileMemMapReader reader(memMap, []() {}, "ReadRecordInfoSection_EmptyPkgNameAndZeroMethods");
    ASSERT_TRUE(JSPandaFileRecordInfoSnapshot::ReadRecordInfoSection(pf.get(), reader));
    JSRecordInfo *recordInfo = pf->CheckAndGetRecordInfo("test");
    ASSERT_NE(recordInfo, nullptr);
    ASSERT_EQ(pf->GetMethodLiterals(), nullptr);
    ASSERT_TRUE(pf->npmEntries_.empty());
    ASSERT_TRUE(recordInfo->npmPackageName.empty());
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_NumClasses)
{
    size_t offset = 0;
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_NumClasses"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_NumMethods)
{
    size_t offset = sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_NumMethods"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_IsBundlePack)
{
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_IsBundlePack"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_RecordCount)
{
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_RecordCount"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_RecordNameLen)
{
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_RecordNameLen"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_RecordName)
{
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_RecordName"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_Flags)
{
    size_t recordNameLen = std::strlen("test");
    size_t offset =
        sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + recordNameLen;
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_Flags"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_JsonStringId)
{
    size_t recordNameLen = std::strlen("test");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_JsonStringId"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_ModuleRecordIdx)
{
    size_t recordNameLen = std::strlen("test");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_ModuleRecordIdx"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_LazyImportIdx)
{
    size_t recordNameLen = std::strlen("test");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_LazyImportIdx"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_ClassId)
{
    size_t recordNameLen = std::strlen("test");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_ClassId"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_PkgNameLen)
{
    size_t recordNameLen = std::strlen("test");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_PkgNameLen"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_PkgName)
{
    size_t recordNameLen = std::strlen("test");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    sizeof(uint32_t) + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_PkgName"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_NpmCount)
{
    size_t recordNameLen = std::strlen("test");
    size_t pkgNameLen = std::strlen("@ohos/testpkg");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    sizeof(uint32_t) + sizeof(uint32_t) + pkgNameLen;
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_NpmCount"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_KeyLen)
{
    size_t recordNameLen = std::strlen("test");
    size_t pkgNameLen = std::strlen("@ohos/testpkg");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    sizeof(uint32_t) + sizeof(uint32_t) + pkgNameLen + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_KeyLen"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_Key)
{
    size_t recordNameLen = std::strlen("test");
    size_t pkgNameLen = std::strlen("@ohos/testpkg");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    sizeof(uint32_t) + sizeof(uint32_t) + pkgNameLen + sizeof(uint32_t) + sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_Key"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_ValLen)
{
    size_t recordNameLen = std::strlen("test");
    size_t pkgNameLen = std::strlen("@ohos/testpkg");
    size_t keyLen = std::strlen("key");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    sizeof(uint32_t) + sizeof(uint32_t) + pkgNameLen + sizeof(uint32_t) + sizeof(uint32_t) + keyLen;
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_ValLen"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, RecordInfoSection_CorruptRecordInfo_Value)
{
    size_t recordNameLen = std::strlen("test");
    size_t pkgNameLen = std::strlen("@ohos/testpkg");
    size_t keyLen = std::strlen("key");
    size_t offset = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    recordNameLen + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) +
                    sizeof(uint32_t) + sizeof(uint32_t) + pkgNameLen + sizeof(uint32_t) + sizeof(uint32_t) + keyLen +
                    sizeof(uint32_t);
    ASSERT_FALSE(ReadCorruptRecordInfoSection(offset, "CorruptRecordInfo_Value"));
}

HWTEST_F_L0(JSPandaFileSnapshotTest, UseSnapshotSuccessTest)
{
    CString path = ohos::OhosConstants::PANDAFILE_AND_MODULE_SNAPSHOT_DIR;
    if (!FileExist(path.c_str())) {
        return;
    }
    JSRuntimeOptions &options = thread->GetEcmaVM()->GetJSOptions();
    int arkProperties = options.GetArkProperties();
    options.SetArkProperties(arkProperties & ~static_cast<int>(ArkProperties::DISABLE_JSPANDAFILE_MODULE_SNAPSHOT));
    ModulesSnapshotHelper::g_featureState_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
    ModulesSnapshotHelper::g_featureLoaded_ = static_cast<int>(SnapshotFeatureState::DEFAULT);
    const std::shared_ptr<JSPandaFile> serializePf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(serializePf);
    bool bundlePackSave = serializePf->IsBundlePack();
    serializePf->SetBundlePack(false);
    JSRecordInfo *recordInfo = serializePf->CheckAndGetRecordInfo("func_main_0");
    if (recordInfo != nullptr) {
        recordInfo->npmPackageName = "@ohos/testpkg";
    }
    serializePf->ownedNpmEntries_.emplace_back("testRecord", "testEntry");
    const auto &snapshotNpmEntry = serializePf->ownedNpmEntries_.back();
    serializePf->npmEntries_.insert(
        {std::string_view(snapshotNpmEntry.first.c_str(), snapshotNpmEntry.first.size()),
         std::string_view(snapshotNpmEntry.second.c_str(), snapshotNpmEntry.second.size())});
    CString fileName = JSPandaFileSnapshot::GetJSPandaFileFileName(serializePf->GetJSPandaFileDesc(), path);
    CString stateFilePath = path + ModulesSnapshotHelper::MODULE_SNAPSHOT_STATE_FILE_NAME.data();
    remove(fileName.c_str());
    remove(stateFilePath.c_str());
    if (memset_s(ModulesSnapshotHelper::g_stateFilePathBuffer_, PATH_MAX, 0, PATH_MAX) != EOK) {
        return;
    }
    if (memcpy_s(ModulesSnapshotHelper::g_stateFilePathBuffer_, PATH_MAX, stateFilePath.c_str(),
                 stateFilePath.length()) != EOK) {
        return;
    }
    MockJSPandaFileSnapshot::WriteDataToFile(thread, serializePf.get(), path,
                                             ohos::OhosVersionInfoTools::GetRomVersion());
    serializePf->SetBundlePack(bundlePackSave);
    ASSERT_TRUE(FileExist(fileName.c_str()));
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFileWithSnapshotLoad();
    JSRecordInfo *deserializeRecordInfo = deserializePf->CheckAndGetRecordInfo("func_main_0");
    auto it = deserializePf->npmEntries_.find("testRecord");
    ASSERT_NE(deserializeRecordInfo, nullptr);
    ASSERT_NE(it, deserializePf->npmEntries_.end());
    ASSERT_FALSE(deserializePf->IsBundlePack());
    ASSERT_EQ(deserializeRecordInfo->npmPackageName, "@ohos/testpkg");
    ASSERT_EQ(it->second, "testEntry");
    options.SetArkProperties(arkProperties);
    remove(fileName.c_str());
    remove(stateFilePath.c_str());
}

HWTEST_F_L0(JSPandaFileSnapshotTest, ResetAfterSnapshotFailTest)
{
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(pf);
    JSRecordInfo *recordInfo = pf->CheckAndGetRecordInfo("func_main_0");
    if (recordInfo != nullptr) {
        recordInfo->npmPackageName = "@ohos/testpkg";
    }
    pf->ownedNpmEntries_.emplace_back("testRecord", "testEntry");
    const auto &resetNpmEntry = pf->ownedNpmEntries_.back();
    pf->npmEntries_.insert({std::string_view(resetNpmEntry.first.c_str(), resetNpmEntry.first.size()),
                            std::string_view(resetNpmEntry.second.c_str(), resetNpmEntry.second.size())});
    pf->ownedRecordNames_.emplace_back("staleRecord");
    pf->ownedNpmEntries_.emplace_back("staleKey", "staleValue");

    pf->ResetAfterSnapshotFail();

    ASSERT_TRUE(pf->jsRecordInfo_.empty());
    ASSERT_TRUE(pf->npmEntries_.empty());
    ASSERT_TRUE(pf->ownedRecordNames_.empty());
    ASSERT_TRUE(pf->ownedNpmEntries_.empty());
    ASSERT_EQ(pf->GetMethodLiterals(), nullptr);
    ASSERT_EQ(pf->numMethods_, 0U);
    ASSERT_EQ(pf->numClasses_, 0U);
}
}
