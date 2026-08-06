/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <fstream>
#include <vector>

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "class_data_accessor-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function_kind.h"
#include "ecmascript/jspandafile/method_literal.h"
#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/platform/filesystem.h"

#define private public
#define protected public
#include "ecmascript/jspandafile/constpool_snapshot.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/snapshot/common/modules_snapshot_helper.h"
#include "ecmascript/runtime.h"
#undef protected
#undef private

#include "ecmascript/ecma_string.h"
#include "ecmascript/js_array.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/serializer/file_deserializer.h"
#include "ecmascript/serializer/file_serializer.h"

#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::panda_file;
using namespace panda::pandasm;

namespace panda::test {
class MockConstPoolSnapshot : public ConstPoolSnapshot {
public:
    static bool WriteDataToFile(const std::unique_ptr<SerializeData> &data, const CString &filePath,
                                ModulesSnapshotHelper::SnapshotVersionInfo::UniquePtr header)
    {
        return ModulesSnapshotHelper::WriteDataToFile(data, filePath, header, "MockConstPoolSnapshot");
    }

    static bool ReadDataFromFile(std::unique_ptr<SerializeData> &data, const CString &path,
                                 ModulesSnapshotHelper::SnapshotVersionInfo::UniquePtr header)
    {
        return ModulesSnapshotHelper::ReadDataFromFile(data, path, header, "MockConstPoolSnapshot");
    }

    static std::unique_ptr<SerializeData> GetSerializeData(JSThread *thread, JSPandaFile *pandafile)
    {
        return ConstPoolSnapshot::GetSerializeData(thread, pandafile);
    }

    static JSHandle<JSTaggedValue> GetSerializeArray(JSThread *thread, JSPandaFile *pandafile)
    {
        return ConstPoolSnapshot::GetSerializeArray(thread, pandafile);
    }

    static bool SerializeAndSaving(JSThread *thread, JSPandaFile *pandafile, const CString &path,
                                   const CString &version)
    {
        CString filePath = path + GetSnapshotFileName(pandafile);
        std::unique_ptr<SerializeData> serializeData = GetSerializeData(thread, pandafile);
        if (serializeData == nullptr) {
            return false;
        }
        auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(
            thread->GetEcmaVM()->GetApplicationVersionCode(), version, pandafile->GetJSPandaFileDesc());
        return WriteDataToFile(serializeData, filePath, std::move(header));
    }

    static bool MockSerializeAndSavingHasIncompleteData(JSThread *thread, JSPandaFile *pandafile, const CString &path,
                                                        const CString &version)
    {
        CString filePath = path + GetSnapshotFileName(pandafile);
        std::unique_ptr<SerializeData> serializeData = GetSerializeData(thread, pandafile);
        if (serializeData == nullptr) {
            return false;
        }
        serializeData->SetIncompleteData(true);
        auto header = ModulesSnapshotHelper::SnapshotVersionInfo::New(
            thread->GetEcmaVM()->GetApplicationVersionCode(), version, pandafile->GetJSPandaFileDesc());
        return WriteDataToFile(serializeData, filePath, std::move(header));
    }
};

class ConstPoolSnapshotTest : public testing::Test {
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
        CString fileName = path + GetSnapshotFileName();
        if (FileExist(fileName.c_str()) && remove(fileName.c_str()) != 0) {
            GTEST_LOG_(ERROR) << "remove " << fileName << " failed";
        }
        CString stateFilePath = path + ModulesSnapshotHelper::MODULE_SNAPSHOT_STATE_FILE_NAME.data();
        if (FileExist(stateFilePath.c_str()) && remove(stateFilePath.c_str()) != 0) {
            GTEST_LOG_(ERROR) << "remove '" << stateFilePath << "' failed when setup";
        }
        disabledFeature = ModulesSnapshotHelper::g_featureState_;
        loadedFeature = ModulesSnapshotHelper::g_featureLoaded_;
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        CString path = GetSnapshotPath();
        // Remove constpool entries from Runtime before VM destruction
        // to avoid use-after-free of dangling JSPandaFile pointers
        {
            Runtime *runtime = Runtime::GetInstance();
            LockHolder lock(runtime->constpoolLock_);
            runtime->globalSharedConstpools_.clear();
        }
        for (auto& file : requiredFiles) {
            CString fileName = path + file;
            if (FileExist(fileName.c_str()) && remove(fileName.c_str()) != 0) {
                GTEST_LOG_(ERROR) << "remove " << fileName << " failed";
            }
        }
        DeleteFilesWithSuffix(path.c_str(), ModulesSnapshotHelper::BACKUP_FILE_SUFFIX.data());
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

    CString GetSnapshotFileName()
    {
        auto result = "entry" + CString(ConstPoolSnapshot::CONSTPOOL_SNAPSHOT_FILE_SUFFIX);
        requiredFiles.insert(result);
        return result;
    }

    CString GetSnapshotFileName(JSPandaFile* pandafile)
    {
        auto result = ConstPoolSnapshot::GetSnapshotFileName(pandafile);
        requiredFiles.insert(result);
        return result;
    }

    ModulesSnapshotHelper::SnapshotVersionInfo::UniquePtr NewHeader(
        const CString &version, const CString &description) const
    {
        return ModulesSnapshotHelper::SnapshotVersionInfo::New(
            thread->GetEcmaVM()->GetApplicationVersionCode(), version, description);
    }

    ModulesSnapshotHelper::SnapshotVersionInfo::UniquePtr NewHeader(
        const CString &version, JSPandaFile *pandafile) const
    {
        return NewHeader(version, pandafile->GetJSPandaFileDesc());
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

    void NormalTranslateJSPandaFile(const std::shared_ptr<JSPandaFile> &pf) const
    {
        const File *file = pf->GetPandaFile();
        const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
        const File::EntityId classId = file->GetClassId(typeDesc);
        ClassDataAccessor cda(*file, classId);
        std::vector<File::EntityId> methodId{};
        cda.EnumerateMethods([&](const MethodDataAccessor &mda) { methodId.push_back(mda.GetMethodId()); });
        pf->UpdateMainMethodIndex(methodId[0].GetOffset());
        const char *methodName = MethodLiteral::GetMethodName(pf.get(), methodId[0]);
        PandaFileTranslator::TranslateClasses(thread, pf.get(), CString(methodName));
        // Register shared constpools so ForEachSharedConstpool can find them
        thread->GetEcmaVM()->CreateAllConstpool(pf.get());
    }

    // Round-trips a single ConstantPool through the same FileSerializer/FileDeserializer pipeline
    // that ConstPoolSnapshot::GetSerializeData / DeserializeData rely on, so that the constpool
    // cache (compressed area) serialization filtering can be inspected in isolation.
    // The returned handle is rooted in the test scope and its cache slots can be inspected directly.
    JSHandle<ConstantPool> SerializeAndDeserializeConstPool(JSHandle<ConstantPool> constpool)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        // ConstPoolSnapshot wraps the shared constpools in a TaggedArray root before serializing.
        JSHandle<JSTaggedValue> root(factory->NewTaggedArray(1));
        TaggedArray::Cast(root.GetTaggedValue())->Set(thread, 0, constpool.GetTaggedValue());

        std::unique_ptr<SerializeData> data;
        {
            EcmaHandleScope scope(thread);
            JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
            FileSerializer serializer(thread);
            EXPECT_TRUE(serializer.WriteValue(thread, root, undefined, undefined));
            data = serializer.Release();
        }
        EXPECT_NE(data, nullptr);
        if (data == nullptr) {
            return JSHandle<ConstantPool>(thread, JSTaggedValue::Hole());
        }
        // Deserialize with the same FileDeserializer used by ConstPoolSnapshot::DeserializeData.
        FileDeserializer deserializer(thread, data.get());
        JSHandle<JSTaggedValue> result = deserializer.ReadValue();
        EXPECT_TRUE(result->IsTaggedArray());
        JSTaggedValue constpoolVal = TaggedArray::Cast(result.GetTaggedValue())->Get(thread, 0);
        return JSHandle<ConstantPool>(thread, constpoolVal);
    }

    EcmaVM *instance{nullptr};
    EcmaHandleScope *scope{nullptr};
    int disabledFeature{0};
    int loadedFeature{0};
    JSThread *thread{nullptr};
    std::unordered_set<CString> requiredFiles{};
};

HWTEST_F_L0(ConstPoolSnapshotTest, SerializeAndReadDataFromFile)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(pf);
    // serialize
    std::unique_ptr<SerializeData> serializeData = MockConstPoolSnapshot::GetSerializeData(thread, pf.get());
    ASSERT_NE(serializeData, nullptr);
    CString filePath = path + GetSnapshotFileName(pf.get());
    auto header = NewHeader(version, pf.get());
    ASSERT_TRUE(MockConstPoolSnapshot::WriteDataToFile(serializeData, filePath, std::move(header)));
    ASSERT_TRUE(FileExist(filePath.c_str()));
    // read back
    auto readHeader = NewHeader(version, pf.get());
    std::unique_ptr<SerializeData> readData = std::make_unique<SerializeData>(thread);
    ASSERT_TRUE(MockConstPoolSnapshot::ReadDataFromFile(readData, filePath, std::move(readHeader)));
    ASSERT_NE(readData, nullptr);
    ASSERT_NE(readData->Data(), nullptr);
    ASSERT_EQ(readData->Size(), serializeData->Size());
}

HWTEST_F_L0(ConstPoolSnapshotTest, ShouldNotSerializeWhenFileIsExists)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    CString fileName = path + GetSnapshotFileName(pf.get());
    NormalTranslateJSPandaFile(pf);
    // serialize and persist
    std::unique_ptr<SerializeData> serializeData = MockConstPoolSnapshot::GetSerializeData(thread, pf.get());
    ASSERT_NE(serializeData, nullptr);
    auto header = NewHeader(version, pf.get());
    ASSERT_TRUE(MockConstPoolSnapshot::WriteDataToFile(serializeData, fileName, std::move(header)));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // SerializeDataAndPostSavingJob returns early when file already exists
    EcmaVM *vm = thread->GetEcmaVM();
    ConstPoolSnapshot::SerializeDataAndPostSavingJob(vm, pf.get(), path, version);
    // File should still exist (not modified)
    ASSERT_TRUE(FileExist(fileName.c_str()));
}

HWTEST_F_L0(ConstPoolSnapshotTest, ShouldNotDeserializeWhenFileIsNotExists)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    CString fileName = path + GetSnapshotFileName(pf.get());
    // return false when file is not exists
    ASSERT_FALSE(FileExist(fileName.c_str()));
    ASSERT_FALSE(ConstPoolSnapshot::DeserializeData(thread->GetEcmaVM(), pf.get(), path, version));
}

HWTEST_F_L0(ConstPoolSnapshotTest, ShouldDeserializeFailedWhenFileIsEmpty)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    CString fileName = path + GetSnapshotFileName(pf.get());
    std::ofstream ofStream(fileName.c_str());
    ofStream.close();
    // return false when file is empty
    ASSERT_TRUE(FileExist(fileName.c_str()));
    ASSERT_FALSE(ConstPoolSnapshot::DeserializeData(thread->GetEcmaVM(), pf.get(), path, version));
}

HWTEST_F_L0(ConstPoolSnapshotTest, ShouldDeserializeFailedWhenCheckSumIsNotMatch)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    CString fileName = path + GetSnapshotFileName(pf.get());
    NormalTranslateJSPandaFile(pf);
    // serialize and persist
    std::unique_ptr<SerializeData> serializeData = MockConstPoolSnapshot::GetSerializeData(thread, pf.get());
    ASSERT_NE(serializeData, nullptr);
    CString filePath = path + GetSnapshotFileName(pf.get());
    auto header = NewHeader(version, pf.get());
    ASSERT_TRUE(MockConstPoolSnapshot::WriteDataToFile(serializeData, filePath, std::move(header)));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // modify file content
    Chmod(fileName, "rw");
    std::ofstream ofStream(fileName.c_str(), std::ios::app);
    uint32_t mockCheckSum = 123456;
    ofStream << mockCheckSum;
    ofStream.close();
    // deserialize failed when checksum is not match
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(ConstPoolSnapshot::DeserializeData(thread->GetEcmaVM(), deserializePf.get(), path, version));
}

HWTEST_F_L0(ConstPoolSnapshotTest, ShouldDeserializeFailedWhenAppVersionCodeIsNotMatch)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    CString fileName = path + GetSnapshotFileName(pf.get());
    NormalTranslateJSPandaFile(pf);
    // serialize and persist
    std::unique_ptr<SerializeData> serializeData = MockConstPoolSnapshot::GetSerializeData(thread, pf.get());
    ASSERT_NE(serializeData, nullptr);
    auto header = NewHeader(version, pf.get());
    ASSERT_TRUE(MockConstPoolSnapshot::WriteDataToFile(serializeData, fileName, std::move(header)));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // modify app version code
    thread->GetEcmaVM()->SetApplicationVersionCode(1);
    // deserialize failed when app version code is not match
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(ConstPoolSnapshot::DeserializeData(thread->GetEcmaVM(), deserializePf.get(), path, version));
}

HWTEST_F_L0(ConstPoolSnapshotTest, ShouldDeserializeFailedWhenVersionCodeIsNotMatch)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    CString fileName = path + GetSnapshotFileName(pf.get());
    NormalTranslateJSPandaFile(pf);
    // serialize and persist
    std::unique_ptr<SerializeData> serializeData = MockConstPoolSnapshot::GetSerializeData(thread, pf.get());
    ASSERT_NE(serializeData, nullptr);
    auto header = NewHeader(version, pf.get());
    ASSERT_TRUE(MockConstPoolSnapshot::WriteDataToFile(serializeData, fileName, std::move(header)));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // deserialize failed when version code is not match
    CString updatedVersion = "version 205.0.1.125(SP20)";
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(ConstPoolSnapshot::DeserializeData(thread->GetEcmaVM(), deserializePf.get(), path, updatedVersion));
}

HWTEST_F_L0(ConstPoolSnapshotTest, ShouldDeserializeFailedWhenHasIncompleteData)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    CString fileName = path + GetSnapshotFileName(pf.get());
    NormalTranslateJSPandaFile(pf);
    // serialize and persist with incomplete data
    ASSERT_TRUE(MockConstPoolSnapshot::MockSerializeAndSavingHasIncompleteData(thread, pf.get(), path, version));
    ASSERT_TRUE(FileExist(fileName.c_str()));
    // deserialize failed when has incomplete data
    const std::shared_ptr<JSPandaFile> deserializePf = NewMockJSPandaFile();
    ASSERT_FALSE(ConstPoolSnapshot::DeserializeData(thread->GetEcmaVM(), deserializePf.get(), path, version));
}

HWTEST_F_L0(ConstPoolSnapshotTest, WriteDataToFileMmapFailed)
{
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(pf);
    std::unique_ptr<SerializeData> serializeData = MockConstPoolSnapshot::GetSerializeData(thread, pf.get());
    ASSERT_NE(serializeData, nullptr);
    // Use an invalid/non-existent directory path to cause mmap failure
    CString invalidPath =
        CString("/non_existent_dir/deep/nested/path/") + GetSnapshotFileName(pf.get());
    CString version = "version 205.0.1.120(SP20)";
    auto header = NewHeader(version, pf.get());
    ASSERT_FALSE(MockConstPoolSnapshot::WriteDataToFile(serializeData, invalidPath, std::move(header)));
}

HWTEST_F_L0(ConstPoolSnapshotTest, ReadDataFromFileWithInvalidPath)
{
    CString invalidPath = "/non_existent_path/";
    CString version = "version 205.0.1.120(SP20)";
    // Read from non-existent file - should fail
    std::unique_ptr<SerializeData> data = std::make_unique<SerializeData>(thread);
    auto header = NewHeader(version, "");
    ASSERT_FALSE(MockConstPoolSnapshot::ReadDataFromFile(data, invalidPath, std::move(header)));
}

HWTEST_F_L0(ConstPoolSnapshotTest, GetSerializeDataTest)
{
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(pf);
    // Test GetSerializeData returns valid data
    std::unique_ptr<SerializeData> serializeData = MockConstPoolSnapshot::GetSerializeData(thread, pf.get());
    ASSERT_NE(serializeData, nullptr);
    // Verify serialize data has buffer
    ASSERT_NE(serializeData->Data(), nullptr);
    ASSERT_GT(serializeData->Size(), static_cast<size_t>(0));
}

HWTEST_F_L0(ConstPoolSnapshotTest, GetSerializeArrayTest)
{
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(pf);
    // Get serialize array - should contain shared constpools for this pandafile
    auto result = MockConstPoolSnapshot::GetSerializeArray(thread, pf.get());
    ASSERT_TRUE(result->IsTaggedArray());
    ASSERT_GT(JSHandle<TaggedArray>::Cast(result)->GetLength(), static_cast<uint32_t>(0));
}

HWTEST_F_L0(ConstPoolSnapshotTest, ReadDataFromFileTest)
{
    CString path = GetSnapshotPath();
    CString version = "version 205.0.1.120(SP20)";
    const std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile();
    NormalTranslateJSPandaFile(pf);
    // First serialize
    std::unique_ptr<SerializeData> serializeData = MockConstPoolSnapshot::GetSerializeData(thread, pf.get());
    ASSERT_NE(serializeData, nullptr);
    CString fileName = path + GetSnapshotFileName(pf.get());
    auto writeHeader = NewHeader(version, pf.get());
    ASSERT_TRUE(MockConstPoolSnapshot::WriteDataToFile(serializeData, fileName, std::move(writeHeader)));
    // Read data from file
    auto readHeader = NewHeader(version, pf.get());
    std::unique_ptr<SerializeData> data = std::make_unique<SerializeData>(thread);
    ASSERT_TRUE(MockConstPoolSnapshot::ReadDataFromFile(data, fileName, std::move(readHeader)));
    ASSERT_NE(data, nullptr);
}

// ConstPool cache field serialization: a STRING entry CAN be serialized and survives the
// serialize/deserialize round-trip (the cache serializer keeps only LINE_STRING entries).
HWTEST_F_L0(ConstPoolSnapshotTest, CacheStringCanBeSerialized)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const uint32_t numOfCache = ConstantPool::AlignUpNumOfCacheForCompressedPointer(8);
    JSHandle<ConstantPool> constpool = factory->NewConstantPool(numOfCache);
    constpool->SetSharedConstpoolId(JSTaggedValue(0));

    static constexpr uint32_t STRING_SLOT = 0;
    JSHandle<EcmaString> originStr = factory->NewFromASCII("ConstPoolCacheString");
    // The constpool cache serializer keeps only LINE_STRING entries.
    ASSERT_TRUE(originStr.GetTaggedValue().IsLineString());
    constpool->SetObjectToCache(thread, STRING_SLOT, originStr.GetTaggedValue());

    JSHandle<ConstantPool> deserialized = SerializeAndDeserializeConstPool(constpool);
    ASSERT_TRUE(deserialized.GetTaggedValue().IsConstantPool());
    JSTaggedValue cached = deserialized->GetObjectFromCache(thread, STRING_SLOT);
    // String cache entries CAN survive the serialize/deserialize round-trip.
    ASSERT_TRUE(cached.IsString());
    JSHandle<EcmaString> desStr(thread, cached);
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread->GetEcmaVM(), desStr, originStr));
}

// ConstPool cache field serialization: a METHOD entry (AOTLiteralInfo) CANNOT be serialized
// and becomes a Hole after the round-trip.
HWTEST_F_L0(ConstPoolSnapshotTest, CacheMethodCannotBeSerialized)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const uint32_t numOfCache = ConstantPool::AlignUpNumOfCacheForCompressedPointer(8);
    JSHandle<ConstantPool> constpool = factory->NewConstantPool(numOfCache);
    constpool->SetSharedConstpoolId(JSTaggedValue(0));

    static constexpr uint32_t METHOD_SLOT = 1;
    // In the constpool cache a method is stored as an AOTLiteralInfo.
    JSHandle<AOTLiteralInfo> methodInfo = factory->NewAOTLiteralInfo(1);
    ASSERT_TRUE(methodInfo.GetTaggedValue().IsAOTLiteralInfo());
    constpool->SetObjectToCache(thread, METHOD_SLOT, methodInfo.GetTaggedValue());

    JSHandle<ConstantPool> deserialized = SerializeAndDeserializeConstPool(constpool);
    ASSERT_TRUE(deserialized.GetTaggedValue().IsConstantPool());
    JSTaggedValue cached = deserialized->GetObjectFromCache(thread, METHOD_SLOT);
    // Non-string cache entries (method / AOTLiteralInfo) CANNOT be serialized and become Hole.
    ASSERT_TRUE(cached.IsHole());
}

// ConstPool cache field serialization: a LITERAL entry (JSArray/JSObject) CANNOT be serialized
// and becomes a Hole after the round-trip.
HWTEST_F_L0(ConstPoolSnapshotTest, CacheLiteralCannotBeSerialized)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const uint32_t numOfCache = ConstantPool::AlignUpNumOfCacheForCompressedPointer(8);
    JSHandle<ConstantPool> constpool = factory->NewConstantPool(numOfCache);
    constpool->SetSharedConstpoolId(JSTaggedValue(0));

    static constexpr uint32_t LITERAL_SLOT = 2;
    // An array/object literal cached in the constpool is a JSArray/JSObject.
    JSHandle<JSArray> literal = factory->NewJSArray();
    ASSERT_TRUE(literal.GetTaggedValue().IsJSArray());
    constpool->SetObjectToCache(thread, LITERAL_SLOT, literal.GetTaggedValue());

    JSHandle<ConstantPool> deserialized = SerializeAndDeserializeConstPool(constpool);
    ASSERT_TRUE(deserialized.GetTaggedValue().IsConstantPool());
    JSTaggedValue cached = deserialized->GetObjectFromCache(thread, LITERAL_SLOT);
    // Non-string cache entries (literal / JSArray) CANNOT be serialized and become Hole.
    ASSERT_TRUE(cached.IsHole());
}
} // namespace panda::test
