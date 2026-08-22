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
#include <cstring>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include <zlib.h>
#include "securec.h"
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

    bool ParseOneEntry(uint8_t *&entryPtr, const uint8_t *dataEnd)
    {
        return zipFile_.ParseOneEntry(entryPtr, dataEnd);
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

    bool ExtractFile(const std::string &fileName, std::ostream &dest) const
    {
        return zipFile_.ExtractFile(fileName, dest);
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

// ZIP format constants and builders for test fixtures.
namespace {
constexpr uint16_t ZIP_VERSION = 20;  // version 2.0
constexpr uint16_t ZIP_COMPRESSION_STORED = 0;
constexpr uint32_t LOCAL_HEADER_SIGNATURE = 0x04034b50;
constexpr uint32_t CENTRAL_DIR_SIGNATURE = 0x02014b50;
constexpr uint32_t EOCD_SIGNATURE = 0x06054b50;
constexpr size_t EOCD_MIN_SIZE = 22U;
constexpr size_t EOCD_CD_OFFSET_FIELD = 16U;
constexpr size_t CENTRAL_DIR_NAME_SIZE_OFFSET = 28U;
constexpr size_t CENTRAL_DIR_EXTRA_SIZE_OFFSET = 30U;
constexpr size_t CENTRAL_DIR_COMMENT_SIZE_OFFSET = 32U;
constexpr uint16_t TWO_ENTRIES = 2;

LocalHeader BuildLocalHeader(uint16_t nameSize, uint32_t crc, uint32_t dataSize)
{
    LocalHeader lh = {};
    lh.signature = LOCAL_HEADER_SIGNATURE;
    lh.versionNeeded = ZIP_VERSION;
    lh.compressionMethod = ZIP_COMPRESSION_STORED;
    lh.crc = crc;
    lh.compressedSize = dataSize;
    lh.uncompressedSize = dataSize;
    lh.nameSize = nameSize;
    return lh;
}

CentralDirEntry BuildCentralDirEntry(uint16_t nameSize, uint32_t crc, uint32_t dataSize,
                                     uint32_t localHeaderOffset)
{
    CentralDirEntry cd = {};
    cd.signature = CENTRAL_DIR_SIGNATURE;
    cd.versionMade = ZIP_VERSION;
    cd.versionNeeded = ZIP_VERSION;
    cd.compressionMethod = ZIP_COMPRESSION_STORED;
    cd.crc = crc;
    cd.compressedSize = dataSize;
    cd.uncompressedSize = dataSize;
    cd.nameSize = nameSize;
    cd.localHeaderOffset = localHeaderOffset;
    return cd;
}

EndDir BuildEndDir(uint16_t totalEntries, uint32_t cdSize, uint32_t cdOffset)
{
    EndDir eocd = {};
    eocd.signature = EOCD_SIGNATURE;
    eocd.totalEntriesInThisDisk = totalEntries;
    eocd.totalEntries = totalEntries;
    eocd.sizeOfCentralDir = cdSize;
    eocd.offset = cdOffset;
    return eocd;
}

// Returns the byte offset of the End-of-Central-Directory record in buf, or 0 if not found.
size_t FindEocdPos(const std::vector<uint8_t> &buf)
{
    size_t fileSize = buf.size();
    if (fileSize < EOCD_MIN_SIZE) {
        return 0;
    }
    size_t maxCommentLen = fileSize - EOCD_MIN_SIZE;
    for (size_t offset = 0; offset <= maxCommentLen; ++offset) {
        size_t pos = fileSize - EOCD_MIN_SIZE - offset;
        if (*reinterpret_cast<const uint32_t *>(buf.data() + pos) == EOCD_SIGNATURE) {
            return pos;
        }
    }
    return 0;
}
}  // namespace

// Build a minimal valid ZIP with a single STORED entry, then return the raw bytes
// so callers can mutate specific fields before writing to disk.
struct RawZipParts {
    LocalHeader lh;
    std::string entryName;
    std::vector<uint8_t> entryData;
    CentralDirEntry cd;
    EndDir eocd;
};

static RawZipParts BuildValidStoredZipParts(const std::string &entryName, const std::vector<uint8_t> &data)
{
    uint16_t nameSize = static_cast<uint16_t>(entryName.size());
    uint32_t crc = static_cast<uint32_t>(crc32(0L, data.data(), data.size()));
    uint32_t dataSize = static_cast<uint32_t>(data.size());

    LocalHeader lh = BuildLocalHeader(nameSize, crc, dataSize);
    CentralDirEntry cd = BuildCentralDirEntry(nameSize, crc, dataSize, 0);

    uint32_t cdOffset = sizeof(LocalHeader) + nameSize + dataSize;
    uint32_t cdSize = sizeof(CentralDirEntry) + nameSize;
    EndDir eocd = BuildEndDir(1, cdSize, cdOffset);

    return {lh, entryName, data, cd, eocd};
}

static bool WriteZipParts(const std::string &zipPath, const RawZipParts &p)
{
    std::ofstream out(zipPath, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char *>(&p.lh), sizeof(p.lh));
    out.write(p.entryName.data(), p.entryName.size());
    out.write(reinterpret_cast<const char *>(p.entryData.data()), p.entryData.size());
    out.write(reinterpret_cast<const char *>(&p.cd), sizeof(p.cd));
    out.write(p.entryName.data(), p.entryName.size());
    out.write(reinterpret_cast<const char *>(&p.eocd), sizeof(p.eocd));
    out.close();
    return out.good();
}

// Build a ZIP with two stored entries, returning raw bytes for mutation.
struct RawZipParts2 {
    LocalHeader lh1;
    std::string name1;
    std::vector<uint8_t> data1;
    LocalHeader lh2;
    std::string name2;
    std::vector<uint8_t> data2;
    CentralDirEntry cd1;
    CentralDirEntry cd2;
    EndDir eocd;
};

static RawZipParts2 BuildValidTwoEntryZipParts(const std::string &n1, const std::vector<uint8_t> &d1,
    const std::string &n2, const std::vector<uint8_t> &d2)
{
    uint16_t ns1 = static_cast<uint16_t>(n1.size());
    uint16_t ns2 = static_cast<uint16_t>(n2.size());
    uint32_t crc1 = static_cast<uint32_t>(crc32(0L, d1.data(), d1.size()));
    uint32_t crc2 = static_cast<uint32_t>(crc32(0L, d2.data(), d2.size()));
    uint32_t sz1 = static_cast<uint32_t>(d1.size());
    uint32_t sz2 = static_cast<uint32_t>(d2.size());

    LocalHeader lh1 = BuildLocalHeader(ns1, crc1, sz1);
    uint32_t lh1End = sizeof(LocalHeader) + ns1 + sz1;
    LocalHeader lh2 = BuildLocalHeader(ns2, crc2, sz2);
    CentralDirEntry cd1 = BuildCentralDirEntry(ns1, crc1, sz1, 0);
    CentralDirEntry cd2 = BuildCentralDirEntry(ns2, crc2, sz2, lh1End);

    uint32_t cdOffset = lh1End + sizeof(LocalHeader) + ns2 + sz2;
    uint32_t cdSize = sizeof(CentralDirEntry) * TWO_ENTRIES + ns1 + ns2;
    EndDir eocd = BuildEndDir(TWO_ENTRIES, cdSize, cdOffset);

    return {lh1, n1, d1, lh2, n2, d2, cd1, cd2, eocd};
}

static bool WriteZipParts2(const std::string &zipPath, const RawZipParts2 &p)
{
    std::ofstream out(zipPath, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char *>(&p.lh1), sizeof(p.lh1));
    out.write(p.name1.data(), p.name1.size());
    out.write(reinterpret_cast<const char *>(p.data1.data()), p.data1.size());
    out.write(reinterpret_cast<const char *>(&p.lh2), sizeof(p.lh2));
    out.write(p.name2.data(), p.name2.size());
    out.write(reinterpret_cast<const char *>(p.data2.data()), p.data2.size());
    out.write(reinterpret_cast<const char *>(&p.cd1), sizeof(p.cd1));
    out.write(p.name1.data(), p.name1.size());
    out.write(reinterpret_cast<const char *>(&p.cd2), sizeof(p.cd2));
    out.write(p.name2.data(), p.name2.size());
    out.write(reinterpret_cast<const char *>(&p.eocd), sizeof(p.eocd));
    out.close();
    return out.good();
}

// Helper: post-hoc mutate a field in a ZIP file on disk.
// Finds EOCD, reads centralDirOffset, then writes newValue at (centralDirOffset + fieldOffset).
static bool MutateZipField(const std::string &zipPath, size_t fieldOffset, const void *newValue, size_t valueSize)
{
    std::ifstream in(zipPath, std::ios::binary | std::ios::ate);
    if (!in) {
        return false;
    }
    auto fileSize = in.tellg();
    in.seekg(0);
    std::vector<uint8_t> buf(fileSize);
    if (!in.read(reinterpret_cast<char *>(buf.data()), fileSize)) {
        return false;
    }
    in.close();

    size_t eocdPos = FindEocdPos(buf);
    if (eocdPos == 0) {
        return false;
    }
    uint32_t cdOffset = *reinterpret_cast<uint32_t *>(buf.data() + eocdPos + EOCD_CD_OFFSET_FIELD);
    size_t writePos = cdOffset + fieldOffset;
    size_t fileSizeSz = static_cast<size_t>(fileSize);
    if (writePos + valueSize > fileSizeSz) {
        return false;
    }
    if (memcpy_s(buf.data() + writePos, fileSizeSz - writePos, newValue, valueSize) != EOK) {
        return false;
    }

    std::ofstream out(zipPath, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char *>(buf.data()), fileSize);
    out.close();
    return out.good();
}

HWTEST_F_L0(ZipFileTest, ParseAllEntries_InflatedTotalEntries_OOBReadRejected)
{
    std::vector<uint8_t> data(64, 'X');
    auto parts = BuildValidStoredZipParts("test.txt", data);
    // Inflate totalEntries: claim 100 entries when only 1 exists
    parts.eocd.totalEntries = 100;
    parts.eocd.totalEntriesInThisDisk = 100;
    // sizeOfCentralDir still matches 1 entry, so the buffer is too small for 100

    std::string zipPath = "oob_inflated_total_entries.zip";
    ASSERT_TRUE(WriteZipParts(zipPath, parts));

    ZipFileFriend zipFileFriend(zipPath);
    // Open should fail because ParseAllEntries detects entryPtr exceeds centralData
    EXPECT_FALSE(zipFileFriend.Open());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Vector 2: Inflated nameSize in CentralDirEntry causes memcpy_s to read past centralData.
HWTEST_F_L0(ZipFileTest, ParseAllEntries_InflatedNameSize_OOBReadRejected)
{
    std::vector<uint8_t> data(64, 'X');
    auto parts = BuildValidStoredZipParts("test.txt", data);

    std::string zipPath = "oob_inflated_namesize.zip";
    ASSERT_TRUE(WriteZipParts(zipPath, parts));

    // nameSize field is at offset 28 in CentralDirEntry (after: sig=4, verMade=2, verNeeded=2,
    // flags=2, compMethod=2, modTime=2, modDate=2, crc=4, compSize=4, uncompSize=4 = 28)
    uint16_t fakeNameSize = 0xFFFF;
    ASSERT_TRUE(MutateZipField(zipPath, CENTRAL_DIR_NAME_SIZE_OFFSET, &fakeNameSize, sizeof(fakeNameSize)));

    ZipFileFriend zipFileFriend(zipPath);
    EXPECT_FALSE(zipFileFriend.Open());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Vector 3: Inflated extraSize in CentralDirEntry causes entryPtr advance past centralData.
HWTEST_F_L0(ZipFileTest, ParseAllEntries_InflatedExtraSize_OOBReadRejected)
{
    std::vector<uint8_t> data(64, 'X');
    auto parts = BuildValidStoredZipParts("test.txt", data);

    std::string zipPath = "oob_inflated_extrasize.zip";
    ASSERT_TRUE(WriteZipParts(zipPath, parts));

    // extraSize is at offset 30 in CentralDirEntry (nameSize=28+2=30)
    uint16_t fakeExtraSize = 0xFFFF;
    ASSERT_TRUE(MutateZipField(zipPath, CENTRAL_DIR_EXTRA_SIZE_OFFSET, &fakeExtraSize, sizeof(fakeExtraSize)));

    ZipFileFriend zipFileFriend(zipPath);
    EXPECT_FALSE(zipFileFriend.Open());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Vector 4: Inflated commentSize in CentralDirEntry causes entryPtr advance past centralData.
HWTEST_F_L0(ZipFileTest, ParseAllEntries_InflatedCommentSize_OOBReadRejected)
{
    std::vector<uint8_t> data(64, 'X');
    auto parts = BuildValidStoredZipParts("test.txt", data);

    std::string zipPath = "oob_inflated_commentsize.zip";
    ASSERT_TRUE(WriteZipParts(zipPath, parts));

    // commentSize is at offset 32 in CentralDirEntry (extraSize=30+2=32)
    uint16_t fakeCommentSize = 0xFFFF;
    ASSERT_TRUE(MutateZipField(zipPath, CENTRAL_DIR_COMMENT_SIZE_OFFSET, &fakeCommentSize, sizeof(fakeCommentSize)));

    ZipFileFriend zipFileFriend(zipPath);
    EXPECT_FALSE(zipFileFriend.Open());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Vector 5: Truncated central directory — sizeOfCentralDir is smaller than sizeof(CentralDirEntry),
// so even reading the fixed header would be OOB.
HWTEST_F_L0(ZipFileTest, ParseAllEntries_TruncatedCentralDir_OOBReadRejected)
{
    std::vector<uint8_t> data(64, 'X');
    auto parts = BuildValidStoredZipParts("test.txt", data);
    // Shrink sizeOfCentralDir so the central directory buffer is too small for even one header
    parts.eocd.sizeOfCentralDir = 10; // much less than sizeof(CentralDirEntry)=46

    std::string zipPath = "oob_truncated_centraldir.zip";
    ASSERT_TRUE(WriteZipParts(zipPath, parts));

    ZipFileFriend zipFileFriend(zipPath);
    EXPECT_FALSE(zipFileFriend.Open());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Vector 6: Two-entry ZIP where the second entry's nameSize is inflated,
// causing OOB read when parsing the second entry.
HWTEST_F_L0(ZipFileTest, ParseAllEntries_SecondEntryInflatedNameSize_OOBReadRejected)
{
    std::vector<uint8_t> d1(32, 'A');
    std::vector<uint8_t> d2(32, 'B');
    auto parts = BuildValidTwoEntryZipParts("first.txt", d1, "second.txt", d2);

    std::string zipPath = "oob_second_entry_namesize.zip";
    ASSERT_TRUE(WriteZipParts2(zipPath, parts));

    // The second CentralDirEntry starts at: cdOffset + sizeof(CentralDirEntry) + name1.size()
    // We need to mutate the nameSize of the second CD entry.
    // Second CD entry nameSize offset = cdOffset + sizeof(CentralDirEntry) + name1.size() + 28
    // But it's easier to use the raw buffer approach:
    std::ifstream in(zipPath, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(in.is_open());
    auto fileSize = in.tellg();
    in.seekg(0);
    std::vector<uint8_t> buf(fileSize);
    ASSERT_TRUE(in.read(reinterpret_cast<char *>(buf.data()), fileSize));
    in.close();

    // Find EOCD, then locate the second CentralDirEntry's nameSize field.
    size_t eocdPos = FindEocdPos(buf);
    ASSERT_GT(eocdPos, 0U);

    uint32_t cdOffset = *reinterpret_cast<uint32_t *>(buf.data() + eocdPos + EOCD_CD_OFFSET_FIELD);
    // Second CentralDirEntry starts after first CD + first entry name
    uint16_t firstNameSize = *reinterpret_cast<uint16_t *>(buf.data() + cdOffset + CENTRAL_DIR_NAME_SIZE_OFFSET);
    size_t secondCdStart = cdOffset + sizeof(CentralDirEntry) + firstNameSize;
    // Mutate nameSize within the second CentralDirEntry
    *reinterpret_cast<uint16_t *>(buf.data() + secondCdStart + CENTRAL_DIR_NAME_SIZE_OFFSET) = 0xFFFF;

    std::ofstream out(zipPath, std::ios::binary);
    ASSERT_TRUE(out.is_open());
    out.write(reinterpret_cast<const char *>(buf.data()), fileSize);
    out.close();

    ZipFileFriend zipFileFriend(zipPath);
    EXPECT_FALSE(zipFileFriend.Open());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Vector 7: sizeOfCentralDir in EOCD is too small (lies about central directory size),
// causing ReadBuffer to allocate a buffer smaller than totalEntries requires.
HWTEST_F_L0(ZipFileTest, ParseAllEntries_UndersizedCentralDir_OOBReadRejected)
{
    std::vector<uint8_t> data(64, 'X');
    auto parts = BuildValidStoredZipParts("test.txt", data);
    // Claim a smaller central directory size so the buffer is too small
    // but large enough for CheckEndDir to pass
    // Original sizeOfCentralDir = sizeof(CentralDirEntry) + nameSize = 46 + 8 = 54
    // Set it to just 30 bytes — less than sizeof(CentralDirEntry)=46
    parts.eocd.sizeOfCentralDir = 30; // too small for a CentralDirEntry

    std::string zipPath = "oob_undersized_centraldir.zip";
    ASSERT_TRUE(WriteZipParts(zipPath, parts));

    ZipFileFriend zipFileFriend(zipPath);
    EXPECT_FALSE(zipFileFriend.Open());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Positive test: a well-formed ZIP with 2 entries should open successfully.
HWTEST_F_L0(ZipFileTest, ParseAllEntries_ValidTwoEntryZip_Succeeds)
{
    std::vector<uint8_t> d1(32, 'A');
    std::vector<uint8_t> d2(32, 'B');
    auto parts = BuildValidTwoEntryZipParts("first.txt", d1, "second.txt", d2);

    std::string zipPath = "valid_two_entry.zip";
    ASSERT_TRUE(WriteZipParts2(zipPath, parts));

    ZipFileFriend zipFileFriend(zipPath);
    EXPECT_TRUE(zipFileFriend.Open());

    // Verify both entries are accessible
    ZipEntry entry;
    EXPECT_TRUE(zipFileFriend.GetEntry("first.txt", entry));
    EXPECT_TRUE(zipFileFriend.GetEntry("second.txt", entry));

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// Positive test: a well-formed single-entry ZIP should open successfully.
HWTEST_F_L0(ZipFileTest, ParseAllEntries_ValidSingleEntryZip_Succeeds)
{
    std::vector<uint8_t> data(64, 'X');
    auto parts = BuildValidStoredZipParts("hello.txt", data);

    std::string zipPath = "valid_single_entry.zip";
    ASSERT_TRUE(WriteZipParts(zipPath, parts));

    ZipFileFriend zipFileFriend(zipPath);
    EXPECT_TRUE(zipFileFriend.Open());

    ZipEntry entry;
    EXPECT_TRUE(zipFileFriend.GetEntry("hello.txt", entry));
    EXPECT_EQ(entry.compressionMethod, 0);
    EXPECT_EQ(entry.compressedSize, data.size());
    EXPECT_EQ(entry.uncompressedSize, data.size());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
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

// Fix 1: Entry name >= MAX_FILE_NAME should be rejected (not truncated)
HWTEST_F_L0(ZipFileTest, ParseOneEntry_LongNameRejected)
{
    // Build a ZIP with an entry whose name is exactly MAX_FILE_NAME (4096) bytes
    std::string longName(4096, 'A');
    std::vector<uint8_t> data(16, 'Z');
    auto parts = BuildValidStoredZipParts(longName, data);

    std::string zipPath = "long_name_rejected.zip";
    ASSERT_TRUE(WriteZipParts(zipPath, parts));

    ZipFileFriend zipFileFriend(zipPath);
    // Open should fail because ParseOneEntry rejects nameSize >= MAX_FILE_NAME
    EXPECT_FALSE(zipFileFriend.Open());

    zipFileFriend.Close();
    std::remove(zipPath.c_str());
}

// ParseOneEntry internal bounds check: entryPtr + sizeof(CentralDirEntry) > dataEnd
HWTEST_F_L0(ZipFileTest, ParseOneEntry_FixedHeaderExceedsBuffer_Rejected)
{
    std::string pathName = "parseone_bounds_test.zip";
    ZipFileFriend zipFileFriend(pathName);

    // Create a buffer too small for even a CentralDirEntry header
    std::vector<uint8_t> smallBuf(10, 0);
    uint8_t *ptr = smallBuf.data();
    const uint8_t *dataEnd = ptr + smallBuf.size();
    EXPECT_FALSE(zipFileFriend.ParseOneEntry(ptr, dataEnd));
}

// ParseOneEntry internal bounds check: variable-length fields exceed buffer
HWTEST_F_L0(ZipFileTest, ParseOneEntry_VariableFieldsExceedBuffer_Rejected)
{
    std::string pathName = "parseone_varlen_test.zip";
    ZipFileFriend zipFileFriend(pathName);

    // Create a buffer large enough for CentralDirEntry but with inflated nameSize
    std::vector<uint8_t> buf(sizeof(CentralDirEntry) + 4, 0); // 4 extra bytes only
    auto *cd = reinterpret_cast<CentralDirEntry *>(buf.data());
    cd->signature = 0x02014b50;
    cd->nameSize = 100;  // claims 100 bytes of name, but only 4 bytes available
    cd->extraSize = 0;
    cd->commentSize = 0;

    uint8_t *ptr = buf.data();
    const uint8_t *dataEnd = ptr + buf.size();
    EXPECT_FALSE(zipFileFriend.ParseOneEntry(ptr, dataEnd));
}

// ParseOneEntry internal bounds check: nullptr dataEnd
HWTEST_F_L0(ZipFileTest, ParseOneEntry_NullDataEnd_Rejected)
{
    std::string pathName = "parseone_null_end_test.zip";
    ZipFileFriend zipFileFriend(pathName);

    uint8_t dummy = 0;
    uint8_t *ptr = &dummy;
    EXPECT_FALSE(zipFileFriend.ParseOneEntry(ptr, nullptr));
}
}