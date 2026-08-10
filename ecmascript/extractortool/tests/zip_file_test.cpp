/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstdio>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include <zlib.h>
#include "gtest/gtest.h"
#include "ecmascript/extractortool/src/zip_file.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/extractortool/src/zip_file_reader.h"

using namespace panda::ecmascript;
namespace panda::ecmascript {
class ZipFileFriend {
public:
    explicit ZipFileFriend(const std::string &path) : zipFile_(path)
    {
    }

    ~ZipFileFriend()
    {
        Close();
    }

    void SetContentLocation(ZipPos start, size_t length)
    {
        zipFile_.SetContentLocation(start, length);
    }

    bool CheckEndDir(EndDir &endDir) const
    {
        return zipFile_.CheckEndDir(endDir);
    }

    bool ParseEndDirectory()
    {
        return zipFile_.ParseEndDirectory();
    }

    bool ParseOneEntry(uint8_t *&entryPtr)
    {
        return zipFile_.ParseOneEntry(entryPtr);
    }

    void AddEntryToTree(const std::string &fileName)
    {
        return zipFile_.AddEntryToTree(fileName);
    }

    bool ParseAllEntries()
    {
        return zipFile_.ParseAllEntries();
    }

    bool Open()
    {
        return zipFile_.Open();
    }

    void Close()
    {
        zipFile_.Close();
    }

    bool IsDirExist(const std::string &dir) const
    {
        return zipFile_.IsDirExist(dir);
    }

    void GetAllFileList(const std::string &srcPath, std::vector<std::string> &assetList)
    {
        zipFile_.GetAllFileList(srcPath, assetList);
    }

    void GetChildNames(const std::string &srcPath, std::set<std::string> &fileSet)
    {
        zipFile_.GetChildNames(srcPath, fileSet);
    }

    bool GetEntry(const std::string &entryName, ZipEntry &resultEntry) const
    {
        return zipFile_.GetEntry(entryName, resultEntry);
    }

    bool CheckCoherencyLocalHeader(const ZipEntry &zipEntry, uint16_t &extraSize) const
    {
        return zipFile_.CheckCoherencyLocalHeader(zipEntry, extraSize);
    }

    std::unique_ptr<FileMapper> CreateFileMapper(const std::string &fileName, FileMapperType type) const
    {
        return zipFile_.CreateFileMapper(fileName, type);
    }

    bool ExtractToBufByName(const std::string &fileName, std::unique_ptr<uint8_t[]> &dataPtr, size_t &len) const
    {
        return zipFile_.ExtractToBufByName(fileName, dataPtr, len);
    }

    void SetIsOpen(bool isOpen)
    {
        zipFile_.isOpen_ = isOpen;
    }

    ZipPos GetFileStartPos() const
    {
        return zipFile_.fileStartPos_;
    }

    void SetFileLength(ZipPos length)
    {
        zipFile_.fileLength_ = length;
    }

    ZipPos GetFileLength() const
    {
        return zipFile_.fileLength_;
    }

    const std::string &GetPathName() const
    {
        return zipFile_.pathName_;
    }

    void SetPathName(const std::string &newPathName)
    {
        zipFile_.pathName_ = newPathName;
    }

    std::shared_ptr<ZipFileReader> GetZipFileReader() const
    {
        return zipFile_.zipFileReader_;
    }

    void SetZipFileReader(const std::shared_ptr<ZipFileReader> &newZipFileReader)
    {
        zipFile_.zipFileReader_ = newZipFileReader;
    }

private:
    ZipFile zipFile_;
};
}

namespace panda::test {
class ZipFileTest : public testing::Test {
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
        instance->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance{nullptr};
    EcmaHandleScope *scope{nullptr};
    JSThread *thread{nullptr};
};

int CreateTestFile()
{
    const std::string fileName = "TestFile.zip";
    std::ofstream file(fileName);
    if (!file.is_open()) {
        return 1;
    }
    file << "This is a test file." << std::endl;
    file.close();
    if (!file) {
        return 1;
    }
    return 0;
}

int DeleteTestFile()
{
    const char *fileName = "TestFile.zip";
    if (std::remove(fileName) != 0) {
        return 1;
    }
    return 0;
}

HWTEST_F_L0(ZipFileTest, SetContentLocationTest)
{
    std::string pathName = "path/to/zip.hap";
    ZipFileFriend zipFileFriend(pathName);
    zipFileFriend.SetIsOpen(true);
    zipFileFriend.SetContentLocation(1, 1);
    EXPECT_EQ(zipFileFriend.GetFileStartPos(), 0);
    EXPECT_EQ(zipFileFriend.GetFileLength(), 0);

    zipFileFriend.SetIsOpen(false);
    zipFileFriend.SetContentLocation(1, 1);
    EXPECT_EQ(zipFileFriend.GetFileStartPos(), 1);
    EXPECT_EQ(zipFileFriend.GetFileLength(), 1);
}

HWTEST_F_L0(ZipFileTest, CheckEndDirTest)
{
    std::string pathName = "path/to/zip.hap";
    ZipFileFriend zipFileFriend(pathName);
    EndDir endDir;
    EXPECT_FALSE(zipFileFriend.CheckEndDir(endDir));

    zipFileFriend.SetFileLength(100);
    EndDir endDir2;
    endDir2.signature = 0x06054b50;
    endDir2.numDisk = 0;
    endDir2.startDiskOfCentralDir = 0;
    endDir2.totalEntriesInThisDisk = 1;
    endDir2.totalEntries = 1;
    endDir2.sizeOfCentralDir = 78;
    endDir2.offset = 0;
    endDir2.commentLen = 0;

    EXPECT_TRUE(zipFileFriend.CheckEndDir(endDir2));
}

HWTEST_F_L0(ZipFileTest, ParseEndDirectoryTest)
{
    std::string pathName = "path/to/zip.hap";
    ZipFileFriend zipFileFriend(pathName);
    zipFileFriend.SetFileLength(22);
    EXPECT_FALSE(zipFileFriend.ParseEndDirectory());
}

HWTEST_F_L0(ZipFileTest, OpenTest)
{
    std::string pathName = "test_files/long_path_name.txt";
    ZipFileFriend zipFileFriend(pathName);
    zipFileFriend.SetIsOpen(true);
    EXPECT_TRUE(zipFileFriend.Open());

    zipFileFriend.SetIsOpen(false);
    std::string longPathName(4097, 'a');
    zipFileFriend.SetPathName(longPathName);
    EXPECT_FALSE(zipFileFriend.Open());
}

HWTEST_F_L0(ZipFileTest, CloseTest)
{
    std::string pathName = "zipFileTest.zip";
    ZipFileFriend zipFileFriend(pathName);
    zipFileFriend.Close();
    EXPECT_EQ(zipFileFriend.GetPathName(), "zipFileTest.zip");

    EXPECT_EQ(CreateTestFile(), 0);
    std::shared_ptr<ZipFileReader> zipFileReader = ZipFileReader::CreateZipFileReader(pathName);
    zipFileFriend.SetZipFileReader(zipFileReader);
    EXPECT_EQ(DeleteTestFile(), 0);
    zipFileFriend.Close();
}

HWTEST_F_L0(ZipFileTest, IsDirExistTest)
{
    std::string pathName = "zipFileTest.zip";
    ZipFileFriend zipFileFriend(pathName);
    EXPECT_FALSE(zipFileFriend.IsDirExist(""));
    std::string dir = ".";
    EXPECT_FALSE(zipFileFriend.IsDirExist(dir));
    dir = "path/to/nonexistent";
    EXPECT_FALSE(zipFileFriend.IsDirExist(dir));
    dir = "/";
    EXPECT_TRUE(zipFileFriend.IsDirExist(dir));
}

HWTEST_F_L0(ZipFileTest, GetAllFileListTest)
{
    std::string pathName = "zipFileTest.zip";
    ZipFileFriend zipFileFriend(pathName);
    std::vector<std::string> assetList;
    zipFileFriend.GetAllFileList("", assetList);
    EXPECT_TRUE(assetList.empty());

    assetList.clear();
    zipFileFriend.GetAllFileList(".", assetList);
    EXPECT_TRUE(assetList.empty());

    assetList.clear();
    zipFileFriend.GetAllFileList("./", assetList);
    EXPECT_TRUE(assetList.empty());

    assetList.clear();
    zipFileFriend.GetAllFileList("path/to/nonexistent", assetList);
    EXPECT_TRUE(assetList.empty());
}

HWTEST_F_L0(ZipFileTest, GetChildNamesTest)
{
    std::string pathName = "zipFileTest.zip";
    ZipFileFriend zipFileFriend(pathName);
    std::set<std::string> fileSet;

    zipFileFriend.GetChildNames("", fileSet);
    EXPECT_TRUE(fileSet.empty());

    fileSet.clear();
    zipFileFriend.GetChildNames("/", fileSet);
    EXPECT_TRUE(fileSet.empty());

    fileSet.clear();
    zipFileFriend.GetChildNames(".", fileSet);
    EXPECT_TRUE(fileSet.empty());

    fileSet.clear();
    zipFileFriend.GetChildNames(".", fileSet);
    EXPECT_TRUE(fileSet.empty());

    fileSet.clear();
    zipFileFriend.GetChildNames("path/to/nonexistent", fileSet);
    EXPECT_TRUE(fileSet.empty());
}

HWTEST_F_L0(ZipFileTest, GetEntryTest)
{
    std::string pathName = "zipFileTest.zip";
    ZipFileFriend zipFileFriend(pathName);
    ZipEntry resultEntry;
    std::string nonExistingEntryName = "nonExistingEntry";
    EXPECT_FALSE(zipFileFriend.GetEntry(nonExistingEntryName, resultEntry));
}

HWTEST_F_L0(ZipFileTest, CheckCoherencyLocalHeaderTest)
{
    std::string pathName = "zipFileTest.zip";
    ZipFileFriend zipFileFriend(pathName);
    ZipEntry zipEntry;
    uint16_t extraSize;
    size_t fileStartPos_;
    fileStartPos_ = 0;
    zipEntry.compressionMethod = 8;
    zipEntry.localHeaderOffset = 0;
    zipEntry.fileName = "testFile.txt";
    zipEntry.compressionMethod = 9;
    EXPECT_FALSE(zipFileFriend.CheckCoherencyLocalHeader(zipEntry, extraSize));
    zipEntry.fileName = std::string(4096, 'a');
    EXPECT_FALSE(zipFileFriend.CheckCoherencyLocalHeader(zipEntry, extraSize));
    zipEntry.fileName = "differentName.txt";
    EXPECT_FALSE(zipFileFriend.CheckCoherencyLocalHeader(zipEntry, extraSize));
}

// Helper: build a minimal ZIP file where the DEFLATED entry's uncompressedSize is understated.
// The ZIP contains one entry with compressionMethod=8 (DEFLATED), but the declared
// uncompressedSize is much smaller than the actual decompressed data, triggering a
// heap buffer overflow in UnzipWithInflatedFromMMap if not properly checked.
static bool CreateMalformedZipForOverflowTest(const std::string &zipPath, const std::string &entryName,
    const std::vector<uint8_t> &rawData, uint32_t fakeUncompSize)
{
    // 1. Compress rawData with raw deflate
    uLongf compSize = compressBound(rawData.size());
    std::vector<uint8_t> compData(compSize);
    z_stream strm = {};
    if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL,
        Z_DEFAULT_STRATEGY) != Z_OK) {
        return false;
    }
    strm.next_in = const_cast<Bytef *>(rawData.data());
    strm.avail_in = rawData.size();
    strm.next_out = compData.data();
    strm.avail_out = compSize;
    if (deflate(&strm, Z_FINISH) != Z_STREAM_END) {
        deflateEnd(&strm);
        return false;
    }
    compSize = strm.total_out;
    deflateEnd(&strm);

    // 2. Compute CRC of actual uncompressed data
    uLong crcVal = crc32(0L, rawData.data(), rawData.size());

    // 3. Build Local Header (with lying uncompressedSize)
    uint16_t nameSize = static_cast<uint16_t>(entryName.size());
    LocalHeader lh = {};
    lh.signature = 0x04034b50;
    lh.versionNeeded = 20;
    lh.flags = 0;
    lh.compressionMethod = 8; // Z_DEFLATED
    lh.crc = static_cast<uint32_t>(crcVal);
    lh.compressedSize = static_cast<uint32_t>(compSize);
    lh.uncompressedSize = fakeUncompSize;
    lh.nameSize = nameSize;
    lh.extraSize = 0;

    // 4. Build Central Directory Entry (must match local header for CheckCoherencyLocalHeader)
    CentralDirEntry cd = {};
    cd.signature = 0x02014b50;
    cd.versionMade = 20;
    cd.versionNeeded = 20;
    cd.flags = 0;
    cd.compressionMethod = 8;
    cd.crc = static_cast<uint32_t>(crcVal);
    cd.compressedSize = static_cast<uint32_t>(compSize);
    cd.uncompressedSize = fakeUncompSize;
    cd.nameSize = nameSize;
    cd.extraSize = 0;
    cd.commentSize = 0;
    cd.diskNumStart = 0;
    cd.internalAttr = 0;
    cd.externalAttr = 0;
    cd.localHeaderOffset = 0;

    // 5. Build End of Central Directory
    uint32_t cdOffset = sizeof(LocalHeader) + nameSize + static_cast<uint32_t>(compSize);
    EndDir eocd = {};
    eocd.signature = 0x06054b50;
    eocd.numDisk = 0;
    eocd.startDiskOfCentralDir = 0;
    eocd.totalEntriesInThisDisk = 1;
    eocd.totalEntries = 1;
    eocd.sizeOfCentralDir = sizeof(CentralDirEntry) + nameSize;
    eocd.offset = cdOffset;
    eocd.commentLen = 0;

    // 6. Write ZIP file
    std::ofstream out(zipPath, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char *>(&lh), sizeof(lh));
    out.write(entryName.data(), nameSize);
    out.write(reinterpret_cast<const char *>(compData.data()), compSize);
    out.write(reinterpret_cast<const char *>(&cd), sizeof(cd));
    out.write(entryName.data(), nameSize);
    out.write(reinterpret_cast<const char *>(&eocd), sizeof(eocd));
    out.close();
    return out.good();
}

// Test that UnzipWithInflatedFromMMap rejects a DEFLATED entry whose actual
// decompressed size exceeds the declared uncompressedSize (zip bomb / heap overflow).
HWTEST_F_L0(ZipFileTest, UnzipWithInflatedFromMMap_BufferOverflowDetected)
{
    // Prepare test data: 4096 bytes of repetitive content
    std::vector<uint8_t> rawData(4096, 'A');
    const std::string entryName = "ets/sourceMaps.map";
    const uint32_t fakeUncompSize = 16; // Lie: claim only 16 bytes, actual is 4096

    std::string zipPath = "malformed_overflow_test.zip";
    ASSERT_TRUE(CreateMalformedZipForOverflowTest(zipPath, entryName, rawData, fakeUncompSize));

    ZipFileFriend zipFileFriend(zipPath);
    ASSERT_TRUE(zipFileFriend.Open());

    std::unique_ptr<uint8_t[]> dataPtr;
    size_t len = 0;
    // ExtractToBufByName should fail because the actual inflated data (4096 bytes)
    // exceeds the declared uncompressedSize (16 bytes), triggering the overflow check
    bool result = zipFileFriend.ExtractToBufByName(entryName, dataPtr, len);
    EXPECT_FALSE(result);

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Test that UnzipWithInflatedFromMMap succeeds when uncompressedSize matches actual data
HWTEST_F_L0(ZipFileTest, UnzipWithInflatedFromMMap_ValidEntrySucceeds)
{
    // Prepare test data with correct uncompressedSize
    std::vector<uint8_t> rawData(256, 'B');
    const std::string entryName = "ets/sourceMaps.map";
    const uint32_t realUncompSize = static_cast<uint32_t>(rawData.size());

    std::string zipPath = "valid_inflate_test.zip";
    ASSERT_TRUE(CreateMalformedZipForOverflowTest(zipPath, entryName, rawData, realUncompSize));

    ZipFileFriend zipFileFriend(zipPath);
    ASSERT_TRUE(zipFileFriend.Open());

    std::unique_ptr<uint8_t[]> dataPtr;
    size_t len = 0;
    bool result = zipFileFriend.ExtractToBufByName(entryName, dataPtr, len);
    EXPECT_TRUE(result);
    // Verify the decompressed data matches
    ASSERT_EQ(len, realUncompSize);
    EXPECT_EQ(memcmp(dataPtr.get(), rawData.data(), realUncompSize), 0);

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}
}