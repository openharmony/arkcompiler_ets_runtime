/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/dfx/hprof/rawheap_translate/metadata_parse.h"
#include "ecmascript/dfx/hprof/rawheap_translate/rawheap_translate.h"
#include "ecmascript/dfx/hprof/rawheap_translate/utils.h"

using namespace panda::ecmascript;

namespace panda::test {
class RawHeapTranslateTest : public testing::Test {
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
        metaParser = std::make_unique<rawheap_translate::MetaParser>();
    }

    void TearDown() override
    {
    }

    std::unique_ptr<rawheap_translate::MetaParser> metaParser {nullptr};
};

class RawHeapTranslateV1TestHelper {
public:
    explicit RawHeapTranslateV1TestHelper(rawheap_translate::RawHeapTranslateV1 *rawheap) : rawheap_(rawheap) {}
    ~RawHeapTranslateV1TestHelper() = default;

    bool ReadRootTable(rawheap_translate::FileReader &file)
    {
        if (!rawheap_) {
            return false;
        }
        return rawheap_->ReadRootTable(file);
    }

    bool ReadStringTable(rawheap_translate::FileReader &file)
    {
        if (!rawheap_) {
            return false;
        }
        return rawheap_->ReadStringTable(file);
    }

    rawheap_translate::StringHashMap* GetStringTable()
    {
        if (!rawheap_) {
            return nullptr;
        }
        return rawheap_->GetStringTable();
    }

    void AddSectionRecord(uint32_t record)
    {
        if (!rawheap_) {
            return;
        }
        rawheap_->sections_.push_back(record);
    }

    std::vector<rawheap_translate::Node *>* GetNodes()
    {
        if (!rawheap_) {
            return nullptr;
        }
        return rawheap_->GetNodes();
    }

    void WriteTestData(BinaryWriter &writer)
    {
        // write version
        char version[sizeof(uint64_t)] = "1.0.0";
        writer.WriteBinBlock(version, sizeof(uint64_t));

        uint32_t rootOffset = writer.GetCurrentFileSize();
        AddSectionRecord(rootOffset);

        // write root table
        int rootCnt = 10;                           // root count
        writer.WriteUInt32(rootCnt);
        writer.WriteUInt32(sizeof(uint64_t));       // size of object identifier

        for (uint64_t i = 1; i <= rootCnt; i++) {
            writer.WriteUInt64(i);                  // root identifier
        }
        AddSectionRecord(writer.GetCurrentFileSize() - rootOffset);

        uint32_t strOffset = writer.GetCurrentFileSize();
        AddSectionRecord(strOffset);

        // write string table, there is empty
        writer.WriteUInt32(rootCnt);     // string count
        writer.WriteUInt32(0);      // unused int32

        for (int i = 1; i <= rootCnt; i++) {
            std::string str = "test_root_" + std::to_string(i);
            writer.WriteUInt32(str.size());                     // size of current string
            writer.WriteUInt32(1);                              // object count
            writer.WriteUInt64(i);                              // object identifier
            writer.WriteBinBlock(const_cast<char *>(str.c_str()), str.size() + 1);      // string
        }

        AddSectionRecord(writer.GetCurrentFileSize() - strOffset);
        writer.EndOfWriteBinBlock();
    }

private:
    rawheap_translate::RawHeapTranslateV1 *rawheap_ {nullptr};
};

class RawHeapTranslateV2TestHelper {
public:
    explicit RawHeapTranslateV2TestHelper(rawheap_translate::RawHeapTranslateV2 *rawheap) : rawheap_(rawheap) {}
    ~RawHeapTranslateV2TestHelper() = default;

    bool ReadRootTable(rawheap_translate::FileReader &file)
    {
        if (!rawheap_) {
            return false;
        }
        return rawheap_->ReadRootTable(file);
    }

    bool ReadStringTable(rawheap_translate::FileReader &file)
    {
        if (!rawheap_) {
            return false;
        }
        return rawheap_->ReadStringTable(file);
    }

    bool ReadObjectTable(rawheap_translate::FileReader &file)
    {
        if (!rawheap_) {
            return false;
        }
        return rawheap_->ReadObjectTable(file);
    }

    rawheap_translate::StringHashMap* GetStringTable()
    {
        if (!rawheap_) {
            return nullptr;
        }
        return rawheap_->GetStringTable();
    }

    void AddSectionRecord(uint32_t record)
    {
        if (!rawheap_) {
            return;
        }
        rawheap_->sections_.push_back(record);
    }

    std::vector<rawheap_translate::Node *>* GetNodes()
    {
        if (!rawheap_) {
            return nullptr;
        }
        return rawheap_->GetNodes();
    }

    void WriteTestData(BinaryWriter &writer)
    {
        // write version
        char version[sizeof(uint64_t)] = "2.0.0";
        writer.WriteBinBlock(version, sizeof(uint64_t));

        uint32_t rootOffset = writer.GetCurrentFileSize();
        AddSectionRecord(rootOffset);

        // write root table
        int rootCnt = 10;                           // root count
        writer.WriteUInt32(rootCnt);
        writer.WriteUInt32(sizeof(uint32_t));       // size of object identifier

        for (uint32_t i = 1; i <= rootCnt; i++) {
            writer.WriteUInt32(i);                  // root identifier
        }
        AddSectionRecord(writer.GetCurrentFileSize() - rootOffset);

        uint32_t strOffset = writer.GetCurrentFileSize();
        AddSectionRecord(strOffset);

        // write string table, there is empty
        writer.WriteUInt32(rootCnt);    // string count
        writer.WriteUInt32(0);          // unused int32

        for (int i = 1; i <= rootCnt; i++) {
            std::string str = "test_root_" + std::to_string(i);
            writer.WriteUInt32(str.size());                     // size of current string
            writer.WriteUInt32(1);                              // object count
            writer.WriteUInt32(i);                              // object identifier
            writer.WriteBinBlock(const_cast<char *>(str.c_str()), str.size() + 1);      // string
        }
        AddSectionRecord(writer.GetCurrentFileSize() - strOffset);

        uint32_t objectOffset = writer.GetCurrentFileSize();
        writer.WriteUInt32(rootCnt);
        writer.WriteUInt32(24);     // 24: sizeof addr table
        for (int i = 1; i <= rootCnt; i++) {
            writer.WriteUInt32(i);
            writer.WriteUInt32(0);
            writer.WriteUInt64(i);
            writer.WriteUInt32(0);
            writer.WriteUInt32(0);
        }
        writer.WriteUInt64(0);

        AddSectionRecord(objectOffset);
        AddSectionRecord(writer.GetCurrentFileSize() - objectOffset);
        writer.EndOfWriteBinBlock();
    }

private:
    rawheap_translate::RawHeapTranslateV2 *rawheap_ {nullptr};
};

HWTEST_F_L0(RawHeapTranslateTest, MetaDataParse)
{
    /*
    metadataJson a json is similar to the following example
    {
        "type_enum": {"INVALID": 8},
        "type_list": [
            {
                "name": "JS_OBJECT",
                "offsets": [
                    {
                        "name": "Properties",
                        "offset": 0,
                        "size": 8
                    },
                    {
                        "name": "Elements",
                        "offset": 8,
                        "size": 8
                    }
                ],
                "end_offset": 16,
                "parents": [
                    "ECMA_OBJECT"
                ]
            },
            {
                "name": "ECMA_OBJECT",
                "offsets": [],
                "end_offset": 8,
                "parents": [
                    "TAGGED_OBJECT"
                ]
            },
            {
                "name": "TAGGED_OBJECT",
                "offsets": [],
                "end_offset": 8,
                "parents": []
            }
        ],
        "type_layout": {
            "Dictionary_layout": {
                "name": "Dictionary",
                "key_index": 0,
                "value_index": 1,
                "detail_index": 2,
                "entry_size": 3,
                "header_size": 4
            },
            "Type_range": {
                "string_first": "LINE_STRING",
                "string_last": "TREE_STRING",
                "js_object_first": "JS_OBJECT",
                "js_object_last": "JS_GLOBAL_OBJECT"
            }
        },
        "version": "1.0.0"
    }
    */
    std::string metadataJson =
        "{\"type_enum\": {\"INVALID\": 8}, \"type_list\": [{\"name\": \"JS_OBJECT\","
        "\"offsets\": [{\"name\": \"Properties\", \"offset\": 0, \"size\": 8}, "
        "{\"name\": \"Elements\", \"offset\": 8, \"size\": 8}], \"end_offset\": 16, "
        "\"parents\": [\"ECMA_OBJECT\"]}, {\"name\": \"ECMA_OBJECT\", \"offsets\": [], "
        "\"end_offset\": 8, \"parents\": [\"TAGGED_OBJECT\"]}, {\"name\": \"TAGGED_OBJECT\", "
        "\"offsets\": [], \"end_offset\": 8, \"parents\": []}], \"type_layout\": {\"Dictionary_layout\": {"
        "\"name\": \"Dictionary\", \"key_index\": 0, \"value_index\": 1, \"detail_index\": 2, "
        "\"entry_size\": 3, \"header_size\": 4}, \"Type_range\": {\"string_first\": \"LINE_STRING\", "
        "\"string_last\": \"TREE_STRING\", \"js_object_first\": \"JS_OBJECT\", \"js_object_last\": "
        "\"JS_GLOBAL_OBJECT\"}}, \"version\": \"1.0.0\"}";

    cJSON *metadataCJson = cJSON_ParseWithLength(metadataJson.c_str(), metadataJson.size());
    ASSERT_TRUE(metadataCJson != nullptr);

    bool result = metaParser->Parse(metadataCJson);
    cJSON_Delete(metadataCJson);
    ASSERT_TRUE(result);

    rawheap_translate::MetaData *meta = metaParser->GetMetaData("JS_OBJECT");
    ASSERT_TRUE(meta != nullptr);
    ASSERT_EQ(meta->endOffset, 32);  // 32: 16 + 8 + 8 = 32, all parent endOffset count total
}

HWTEST_F_L0(RawHeapTranslateTest, BytesToNumber)
{
    char bytes[4] = {0x78, 0x56, 0x34, 0x12};  // write by little endian

    uint32_t u32 = rawheap_translate::ByteToU32(bytes);
    if (rawheap_translate::IsLittleEndian()) {
        ASSERT_TRUE(u32 == 0x12345678);
    } else {
        ASSERT_TRUE(u32 == 0x78563412);
    }
}

HWTEST_F_L0(RawHeapTranslateTest, CheckVersion)
{
    rawheap_translate::Version version00(0, 0, 0);
    rawheap_translate::Version version01(0, 1, 0);
    rawheap_translate::Version version10(1, 0, 0);
    rawheap_translate::Version version11(1, 1, 0);
    rawheap_translate::Version version20(2, 0, 0);
    rawheap_translate::Version version21(2, 1, 0);
    rawheap_translate::Version version30(3, 0, 0);
    rawheap_translate::Version version31(3, 1, 0);

    ASSERT_TRUE(version00 < version01);
    ASSERT_TRUE(version01 < version20);
    ASSERT_TRUE(version20 < version21);
    ASSERT_TRUE(version21 < version30);
    ASSERT_TRUE(version30 < version31);

    rawheap_translate::RawHeap *rawheap00 = rawheap_translate::RawHeap::ParseRawheap(version00, nullptr);
    rawheap_translate::RawHeap *rawheap01 = rawheap_translate::RawHeap::ParseRawheap(version01, nullptr);
    rawheap_translate::RawHeap *rawheap10 = rawheap_translate::RawHeap::ParseRawheap(version10, nullptr);
    rawheap_translate::RawHeap *rawheap11 = rawheap_translate::RawHeap::ParseRawheap(version11, nullptr);
    rawheap_translate::RawHeap *rawheap20 = rawheap_translate::RawHeap::ParseRawheap(version20, nullptr);
    rawheap_translate::RawHeap *rawheap21 = rawheap_translate::RawHeap::ParseRawheap(version21, nullptr);
    rawheap_translate::RawHeap *rawheap30 = rawheap_translate::RawHeap::ParseRawheap(version30, nullptr);
    rawheap_translate::RawHeap *rawheap31 = rawheap_translate::RawHeap::ParseRawheap(version31, nullptr);

    ASSERT_TRUE(rawheap00 != nullptr);
    ASSERT_TRUE(rawheap01 != nullptr);
    ASSERT_TRUE(rawheap10 != nullptr);
    ASSERT_TRUE(rawheap11 != nullptr);
    ASSERT_TRUE(rawheap20 != nullptr);
    ASSERT_TRUE(rawheap21 == nullptr);
    ASSERT_TRUE(rawheap30 == nullptr);
    ASSERT_TRUE(rawheap31 == nullptr);
}

HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1)
{
    const std::string rawheap_v1_for_test_filename = "rawheap_v1_for_test.rawheap";
    int fd = open(rawheap_v1_for_test_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper rawheapHelper(&rawheap);

    rawheapHelper.WriteTestData(writer);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(rawheap_v1_for_test_filename));
    ASSERT_TRUE(rawheapHelper.ReadRootTable(file));
    ASSERT_TRUE(rawheapHelper.ReadStringTable(file));

    std::vector<rawheap_translate::Node *> nodes = *rawheapHelper.GetNodes();
    int expectedSize = 11;      // 11: expected 10 root + 1 synthetic root
    ASSERT_EQ(nodes.size(), expectedSize);

    for (int i = 1; i < expectedSize; i++) {
        rawheap_translate::StringId strId = nodes[i]->strId;
        rawheap_translate::StringHashMap* strTable = rawheapHelper.GetStringTable();

        auto key = strTable->GetKeyByStringId(strId);
        auto str = strTable->GetStringByKey(key);
        ASSERT_EQ(str, "test_root_" + std::to_string(i));
    }
}

HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV2)
{
    const std::string rawheap_v2_for_test_filename = "rawheap_v2_for_test.rawheap";
    int fd = open(rawheap_v2_for_test_filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV2 rawheap(nullptr);
    RawHeapTranslateV2TestHelper rawheapHelper(&rawheap);

    rawheapHelper.WriteTestData(writer);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(rawheap_v2_for_test_filename));
    ASSERT_TRUE(rawheapHelper.ReadObjectTable(file));
    ASSERT_TRUE(rawheapHelper.ReadRootTable(file));
    ASSERT_TRUE(rawheapHelper.ReadStringTable(file));

    std::vector<rawheap_translate::Node *> nodes = *rawheapHelper.GetNodes();
    int expectedSize = 11;      // 11: expected 10 root + 1 synthetic root
    ASSERT_EQ(nodes.size(), expectedSize);

    for (int i = 1; i < expectedSize; i++) {
        rawheap_translate::StringId strId = nodes[i]->strId;
        rawheap_translate::StringHashMap* strTable = rawheapHelper.GetStringTable();

        auto key = strTable->GetKeyByStringId(strId);
        auto str = strTable->GetStringByKey(key);
        ASSERT_EQ(str, "test_root_" + std::to_string(i));
    }
}
}  // namespace panda::test
