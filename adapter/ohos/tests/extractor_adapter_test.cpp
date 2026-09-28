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

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

// ===========================================================================
// Fake ability_base types (inlined — no external stub headers needed)
// ===========================================================================
namespace OHOS {
namespace AbilityBase {

enum class FileMapperType {
    NORMAL_MEM,
    SHARED_MMAP,
    SAFE_ABC
};

class FileMapper {
public:
    FileMapper() = default;
    FileMapper(FileMapper &) = delete;
    void operator=(FileMapper &) = delete;
    ~FileMapper()
    {
        if (autoReleaseMem_) {
            released_ = true;
        }
    }

    explicit FileMapper(std::string fileName, size_t dataLen)
        : fileName_(std::move(fileName)), dataLen_(dataLen) {}

    bool IsCompressed() { return isCompressed_; }
    uint8_t *GetDataPtr() { return dataPtr_; }
    size_t GetDataLen() { return dataLen_; }
    std::string GetFileName() { return fileName_; }
    int32_t GetOffset() { return static_cast<int32_t>(offset_); }
    void SetAutoReleaseMem(bool autoRelease) { autoReleaseMem_ = autoRelease; }

    static bool WasReleased() { return released_; }
    static void ResetReleased() { released_ = false; }

private:
    std::string fileName_;
    bool isCompressed_ = false;
    uint8_t *dataPtr_ = nullptr;
    size_t dataLen_ = 0;
    size_t offset_ = 0;
    bool autoReleaseMem_ = false;
    static inline bool released_ = false;
};

using ZipPos = uint64_t;

struct ZipEntry {
    ZipEntry() = default;
    explicit ZipEntry(uint32_t offset) : localHeaderOffset(offset) {}
    ~ZipEntry() = default;

    uint16_t compressionMethod = 0;
    uint32_t uncompressedSize = 0;
    uint32_t compressedSize = 0;
    uint32_t localHeaderOffset = 0;
    uint32_t crc = 0;
    uint16_t flags = 0;
    uint16_t modifiedTime = 0;
    uint16_t modifiedDate = 0;
    std::string fileName;
};

using ZipEntryMap = std::unordered_map<std::string, ZipEntry>;

struct FakeOffsetResult {
    bool success = false;
    ZipPos offset = 0;
    uint32_t length = 0;
};

class ZipFile {
public:
    explicit ZipFile(const std::string &pathName) : pathName_(pathName) {}
    ~ZipFile() = default;

    const ZipEntryMap &GetAllEntries() const { return entries_; }

    bool GetDataOffsetRelative(const ZipEntry &zipEntry, ZipPos &offset, uint32_t &length) const
    {
        auto it = offsetResults_.find(zipEntry.fileName);
        if (it == offsetResults_.end()) {
            return false;
        }
        offset = it->second.offset;
        length = it->second.length;
        return it->second.success;
    }

    void AddEntry(const std::string &fileName, const FakeOffsetResult &result)
    {
        ZipEntry entry;
        entry.fileName = fileName;
        entries_[fileName] = entry;
        offsetResults_[fileName] = result;
    }

    void ClearEntries()
    {
        entries_.clear();
        offsetResults_.clear();
    }

private:
    std::string pathName_;
    ZipEntryMap entries_;
    std::unordered_map<std::string, FakeOffsetResult> offsetResults_;
};

class Extractor {
public:
    explicit Extractor(const std::string &source) : hapPath_(source) {}
    virtual ~Extractor() = default;

    std::unique_ptr<FileMapper> GetSafeData(const std::string &fileName)
    {
        lastRequestedFileName_ = fileName;
        return std::move(safeDataResult_);
    }

    void SetSafeDataResult(std::unique_ptr<FileMapper> mapper)
    {
        safeDataResult_ = std::move(mapper);
    }

    const std::string &GetLastRequestedFileName() const
    {
        return lastRequestedFileName_;
    }

private:
    std::string hapPath_;
    std::unique_ptr<FileMapper> safeDataResult_;
    std::string lastRequestedFileName_;
};

}  // namespace AbilityBase
}  // namespace OHOS

// ===========================================================================
// Adapter functions under test (same logic as extractor_adapter.cpp)
// ===========================================================================
namespace panda::ecmascript {
using Extractor = OHOS::AbilityBase::Extractor;
using FileMapper = OHOS::AbilityBase::FileMapper;
using ZipFile = OHOS::AbilityBase::ZipFile;
using ZipEntryMap = OHOS::AbilityBase::ZipEntryMap;
using ZipPos = OHOS::AbilityBase::ZipPos;

std::shared_ptr<FileMapper> GetSafeDataAsShared(Extractor& extractor, const std::string& fileName)
{
    std::unique_ptr<FileMapper> uniqueMapper = extractor.GetSafeData(fileName);
    if (uniqueMapper) {
        uniqueMapper->SetAutoReleaseMem(true);
    }
    return std::shared_ptr<FileMapper>(uniqueMapper.release());
}

std::string GetFilePathByOffset(ZipFile& zipFile, uintptr_t offset)
{
    static const std::string dynamicAbc = "ets/modules.abc";
    static constexpr char extNameAbc[] = ".abc";
    const ZipEntryMap& entries = zipFile.GetAllEntries();
    for (const auto& entry : entries) {
        if (entry.first.size() >= sizeof(extNameAbc) - 1 &&
            entry.first.compare(entry.first.size() - (sizeof(extNameAbc) - 1),
                                sizeof(extNameAbc) - 1, extNameAbc) != 0) {
            continue;
        }
        ZipPos dataOffset = 0;
        uint32_t length = 0;
        if (zipFile.GetDataOffsetRelative(entry.second, dataOffset, length) &&
            static_cast<uintptr_t>(dataOffset) == offset) {
            return entry.first;
        }
    }
    return dynamicAbc;
}
}  // namespace panda::ecmascript

// ===========================================================================
// Tests
// ===========================================================================
using namespace panda::ecmascript;
using OHOS::AbilityBase::Extractor;
using OHOS::AbilityBase::FakeOffsetResult;
using OHOS::AbilityBase::FileMapper;
using OHOS::AbilityBase::ZipFile;

namespace panda::test {
namespace {
constexpr const char *DYNAMIC_ABC = "ets/modules.abc";

constexpr ZipPos TEST_OFFSET_FIRST = 100;
constexpr ZipPos TEST_OFFSET_SECOND = 200;
constexpr ZipPos TEST_OFFSET_EXACT_SUFFIX = 50;
constexpr uint32_t TEST_LENGTH_FIRST = 50;
constexpr uint32_t TEST_LENGTH_SECOND = 60;
constexpr uint32_t TEST_LENGTH_EXACT_SUFFIX = 10;
constexpr size_t TEST_DATA_LEN_DEFAULT = 100;
constexpr size_t TEST_DATA_LEN_SMALL = 16;
constexpr size_t TEST_DATA_LEN_MEDIUM = 32;

std::unique_ptr<FileMapper> MakeFileMapper(const std::string &fileName, size_t dataLen)
{
    return std::make_unique<FileMapper>(fileName, dataLen);
}
}  // namespace

class ExtractorAdapterTest : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}

    void SetUp() override {}
    void TearDown() override {}
};

// --- GetSafeDataAsShared tests ---

TEST_F(ExtractorAdapterTest, GetSafeDataAsShared_ReturnsSharedWithValidMapper)
{
    Extractor extractor("dummy.hap");
    extractor.SetSafeDataResult(MakeFileMapper("test.abc", TEST_DATA_LEN_DEFAULT));

    auto shared = GetSafeDataAsShared(extractor, "test.abc");
    ASSERT_NE(shared, nullptr);
    EXPECT_EQ(shared.use_count(), 1);
    EXPECT_EQ(shared->GetFileName(), "test.abc");
    EXPECT_EQ(shared->GetDataLen(), TEST_DATA_LEN_DEFAULT);
}

TEST_F(ExtractorAdapterTest, GetSafeDataAsShared_ReturnsNullWhenGetSafeDataReturnsNull)
{
    Extractor extractor("dummy.hap");
    extractor.SetSafeDataResult(nullptr);

    auto shared = GetSafeDataAsShared(extractor, "missing.abc");
    EXPECT_EQ(shared, nullptr);
}

TEST_F(ExtractorAdapterTest, GetSafeDataAsShared_ForwardsFileNameToExtractor)
{
    Extractor extractor("dummy.hap");
    extractor.SetSafeDataResult(MakeFileMapper("forwarded.abc", TEST_DATA_LEN_SMALL));

    const std::string requested = "forwarded.abc";
    auto shared = GetSafeDataAsShared(extractor, requested);
    ASSERT_NE(shared, nullptr);
    EXPECT_EQ(extractor.GetLastRequestedFileName(), requested);
}

TEST_F(ExtractorAdapterTest, GetSafeDataAsShared_TransfersOwnership)
{
    Extractor extractor("dummy.hap");
    extractor.SetSafeDataResult(MakeFileMapper("owner.abc", TEST_DATA_LEN_MEDIUM));

    auto shared = GetSafeDataAsShared(extractor, "owner.abc");
    ASSERT_NE(shared, nullptr);
    auto second = GetSafeDataAsShared(extractor, "owner.abc");
    EXPECT_EQ(second, nullptr);
    EXPECT_EQ(shared->GetFileName(), "owner.abc");
}

TEST_F(ExtractorAdapterTest, GetSafeDataAsShared_SetsAutoReleaseMem)
{
    FileMapper::ResetReleased();
    {
        Extractor extractor("dummy.hap");
        extractor.SetSafeDataResult(MakeFileMapper("autorelease.abc", TEST_DATA_LEN_DEFAULT));

        auto shared = GetSafeDataAsShared(extractor, "autorelease.abc");
        ASSERT_NE(shared, nullptr);
        EXPECT_FALSE(FileMapper::WasReleased());
    }
    EXPECT_TRUE(FileMapper::WasReleased());
    FileMapper::ResetReleased();
}

// --- GetFilePathByOffset tests ---

TEST_F(ExtractorAdapterTest, GetFilePathByOffset_ReturnsPathWhenOffsetMatches)
{
    ZipFile zipFile("dummy.hap");
    FakeOffsetResult result;
    result.success = true;
    result.offset = TEST_OFFSET_FIRST;
    result.length = TEST_LENGTH_FIRST;
    zipFile.AddEntry("a.abc", result);

    EXPECT_EQ(GetFilePathByOffset(zipFile, TEST_OFFSET_FIRST), "a.abc");
}

TEST_F(ExtractorAdapterTest, GetFilePathByOffset_ReturnsDefaultWhenEntriesEmpty)
{
    ZipFile zipFile("dummy.hap");
    EXPECT_EQ(GetFilePathByOffset(zipFile, 0), std::string(DYNAMIC_ABC));
}

TEST_F(ExtractorAdapterTest, GetFilePathByOffset_ReturnsDefaultWhenNoAbcEntries)
{
    ZipFile zipFile("dummy.hap");
    FakeOffsetResult result;
    result.success = true;
    result.offset = TEST_OFFSET_FIRST;
    result.length = TEST_LENGTH_FIRST;
    zipFile.AddEntry("a.txt", result);

    EXPECT_EQ(GetFilePathByOffset(zipFile, TEST_OFFSET_FIRST), std::string(DYNAMIC_ABC));
}

TEST_F(ExtractorAdapterTest, GetFilePathByOffset_ReturnsDefaultWhenOffsetMismatch)
{
    ZipFile zipFile("dummy.hap");
    FakeOffsetResult result;
    result.success = true;
    result.offset = TEST_OFFSET_FIRST;
    result.length = TEST_LENGTH_FIRST;
    zipFile.AddEntry("a.abc", result);

    EXPECT_EQ(GetFilePathByOffset(zipFile, TEST_OFFSET_SECOND), std::string(DYNAMIC_ABC));
}

TEST_F(ExtractorAdapterTest, GetFilePathByOffset_ReturnsDefaultWhenGetDataOffsetRelativeFails)
{
    ZipFile zipFile("dummy.hap");
    FakeOffsetResult result;
    result.success = false;
    result.offset = 0;
    result.length = 0;
    zipFile.AddEntry("a.abc", result);

    EXPECT_EQ(GetFilePathByOffset(zipFile, 0), std::string(DYNAMIC_ABC));
}

TEST_F(ExtractorAdapterTest, GetFilePathByOffset_ReturnsSecondEntryWhenFirstMismatch)
{
    ZipFile zipFile("dummy.hap");
    FakeOffsetResult first;
    first.success = true;
    first.offset = TEST_OFFSET_FIRST;
    first.length = TEST_LENGTH_FIRST;
    zipFile.AddEntry("a.abc", first);
    FakeOffsetResult second;
    second.success = true;
    second.offset = TEST_OFFSET_SECOND;
    second.length = TEST_LENGTH_SECOND;
    zipFile.AddEntry("b.abc", second);

    EXPECT_EQ(GetFilePathByOffset(zipFile, TEST_OFFSET_SECOND), "b.abc");
}

TEST_F(ExtractorAdapterTest, GetFilePathByOffset_MatchesEntryWithExactAbcSuffix)
{
    ZipFile zipFile("dummy.hap");
    FakeOffsetResult result;
    result.success = true;
    result.offset = TEST_OFFSET_EXACT_SUFFIX;
    result.length = TEST_LENGTH_EXACT_SUFFIX;
    zipFile.AddEntry(".abc", result);

    EXPECT_EQ(GetFilePathByOffset(zipFile, TEST_OFFSET_EXACT_SUFFIX), ".abc");
}
}  // namespace panda::test