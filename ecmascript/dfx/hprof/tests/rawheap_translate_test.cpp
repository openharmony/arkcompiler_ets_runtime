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

    rawheap_translate::Node* CreateNode()
    {
        if (!rawheap_) {
            return nullptr;
        }
        return rawheap_->CreateNode();
    }

    size_t GetNodeCount()
    {
        if (!rawheap_) {
            return 0;
        }
        return rawheap_->GetNodeCount();
    }

    size_t GetEdgeCount()
    {
        if (!rawheap_) {
            return 0;
        }
        return rawheap_->GetEdgeCount();
    }

    void CreateEdge(rawheap_translate::Node *node, uint64_t addr, uint32_t nameOrIndex,
        rawheap_translate::EdgeType type)
    {
        if (!rawheap_) {
            return;
        }
        rawheap_->CreateEdge(node, addr, nameOrIndex, type);
    }

    uint64_t GetHoleValue()
    {
        return rawheap_translate::RawHeapTranslateV1::VALUE_HOLE;
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
            writer.WriteUInt64(i * 8);                  // 8: 8-byte alignment root identifier
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
            writer.WriteUInt64(i * 8);                              // 8: 8-byte alignment object identifier
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

        // write root table header: count + identifier size
        int rootCnt = 10;
        writer.WriteUInt32(rootCnt);
        writer.WriteUInt32(sizeof(uint32_t));  // identifier size: 4 bytes for V2

        // write root addresses
        for (uint32_t i = 1; i <= rootCnt; i++) {
            writer.WriteUInt32(i);
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
    int expectedSize = 15;    // 15: expected 10 root + 1 synthetic root + 4 root groups
    ASSERT_EQ(nodes.size(), expectedSize);

    rawheap_translate::StringId strId = nodes[1]->strId;
    rawheap_translate::StringHashMap* strTable = rawheapHelper.GetStringTable();
    auto key = strTable->GetKeyByStringId(strId);
    auto str = strTable->GetStringByKey(key);
    ASSERT_EQ(str, "LocalHandleRoot[0]");

    strId = nodes[2]->strId;
    key = strTable->GetKeyByStringId(strId);
    str = strTable->GetStringByKey(key);
    ASSERT_EQ(str, "GlobalHandleRoot[0]");

    strId = nodes[3]->strId;
    key = strTable->GetKeyByStringId(strId);
    str = strTable->GetStringByKey(key);
    ASSERT_EQ(str, "VMRoot[0]");

    strId = nodes[4]->strId;
    key = strTable->GetKeyByStringId(strId);
    str = strTable->GetStringByKey(key);
    ASSERT_EQ(str, "FrameRoot[0]");

    for (int i = 1; i < expectedSize - 4; i++) {
        strId = nodes[i + 4]->strId;

        key = strTable->GetKeyByStringId(strId);
        str = strTable->GetStringByKey(key);
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
    // 16: expected 10 root + 1 synthetic root + 4 root groups + 1 metadata
    int expectedSize = 16;
    ASSERT_EQ(nodes.size(), expectedSize);

    rawheap_translate::StringId strId = nodes[1]->strId;
    rawheap_translate::StringHashMap* strTable = rawheapHelper.GetStringTable();
    auto key = strTable->GetKeyByStringId(strId);
    auto str = strTable->GetStringByKey(key);
    ASSERT_EQ(str, "LocalHandleRoot[0]");

    strId = nodes[2]->strId;
    key = strTable->GetKeyByStringId(strId);
    str = strTable->GetStringByKey(key);
    ASSERT_EQ(str, "GlobalHandleRoot[0]");

    strId = nodes[3]->strId;
    key = strTable->GetKeyByStringId(strId);
    str = strTable->GetStringByKey(key);
    ASSERT_EQ(str, "VMRoot[0]");

    strId = nodes[4]->strId;
    key = strTable->GetKeyByStringId(strId);
    str = strTable->GetStringByKey(key);
    ASSERT_EQ(str, "FrameRoot[0]");

    for (int i = 1; i < expectedSize - 5; i++) {
        strId = nodes[i + 5]->strId;

        key = strTable->GetKeyByStringId(strId);
        str = strTable->GetStringByKey(key);
        ASSERT_EQ(str, "test_root_" + std::to_string(i));
    }
}

HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1NullDataCheck)
{
    // Test the null pointer check in Translate() method
    // Create RawHeapTranslateV1 instance
    rawheap_translate::RawHeapTranslateV1 rawheap(metaParser.get());
    RawHeapTranslateV1TestHelper helper(&rawheap);

    // Create several nodes using helper (nodes will have nullptr data by default)
    rawheap_translate::Node* node1 = helper.CreateNode();
    rawheap_translate::Node* node2 = helper.CreateNode();
    rawheap_translate::Node* node3 = helper.CreateNode();
    rawheap_translate::Node* node4 = helper.CreateNode();

    ASSERT_TRUE(node1 != nullptr);
    ASSERT_TRUE(node2 != nullptr);
    ASSERT_TRUE(node3 != nullptr);
    ASSERT_TRUE(node4 != nullptr);

    // Ensure at least one node has nullptr data (default)
    // The new null check in Translate() should skip nodes with nullptr data

    // Call Translate() - with the new null check, this should not crash
    // The function may return false due to missing required data (hclass nodes, etc.)
    // but the important thing is it doesn't dereference null pointer
    bool result = rawheap.Translate();

    // Don't assert on result - focus is on null pointer safety
    // The test passes if Translate() returns (no crash)
    ASSERT_TRUE(result);
}

HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1SkipHolePrimitiveNode)
{
    rawheap_translate::RawHeapTranslateV1 rawheap(metaParser.get());
    RawHeapTranslateV1TestHelper helper(&rawheap);
    rawheap_translate::Node *node = helper.CreateNode();
    ASSERT_TRUE(node != nullptr);

    size_t nodeCount = helper.GetNodeCount();
    size_t edgeCount = helper.GetEdgeCount();
    uint32_t edgeCountOfNode = node->edgeCount;

    helper.CreateEdge(node, helper.GetHoleValue(), 0, rawheap_translate::EdgeType::DEFAULT);
    ASSERT_EQ(helper.GetNodeCount(), nodeCount);
    ASSERT_EQ(helper.GetEdgeCount(), edgeCount);
    ASSERT_EQ(node->edgeCount, edgeCountOfNode);

    helper.CreateEdge(node, 0U, 0, rawheap_translate::EdgeType::DEFAULT);
    ASSERT_EQ(helper.GetNodeCount(), nodeCount);
    ASSERT_EQ(helper.GetEdgeCount(), edgeCount);
    ASSERT_EQ(node->edgeCount, edgeCountOfNode);
}

// Tests for: MetaParser::IsJSWrappedNapiObject (rawheap_translate/metadata_parse.cpp)
HWTEST_F_L0(RawHeapTranslateTest, IsJSWrappedNapiObject)
{
    // ParseTypeEnums assigns meta->type sequentially (0, 1, 2, ...) based on insertion order,
    // NOT based on the JSON value.  The JSON value is used as meta->nodeType.
    // So the first entry gets JSType=0, the second gets JSType=1.
    // Here: JS_OBJECT → JSType 0, JS_WRAPPED_NAPI_OBJECT → JSType 1.
    std::string metadataJson =
        "{\"type_enum\": {\"JS_OBJECT\": 0, \"JS_WRAPPED_NAPI_OBJECT\": 0},"
        "\"type_list\": ["
            "{\"name\": \"JS_OBJECT\","
             "\"offsets\": [], \"end_offset\": 8, \"parents\": []},"
            "{\"name\": \"JS_WRAPPED_NAPI_OBJECT\","
             "\"offsets\": [], \"end_offset\": 8, \"parents\": []}"
        "],"
        "\"type_layout\": {"
            "\"Dictionary_layout\": {"
                "\"name\": \"Dictionary\","
                "\"key_index\": 0, \"value_index\": 1,"
                "\"detail_index\": 2, \"entry_size\": 3, \"header_size\": 4"
            "},"
            "\"Type_range\": {"
                "\"string_first\": \"JS_OBJECT\","
                "\"string_last\": \"JS_OBJECT\","
                "\"js_object_first\": \"JS_OBJECT\","
                "\"js_object_last\": \"JS_WRAPPED_NAPI_OBJECT\""
            "}"
        "},"
        "\"version\": \"1.0.0\"}";

    cJSON *metadataCJson = cJSON_ParseWithLength(metadataJson.c_str(), metadataJson.size());
    ASSERT_TRUE(metadataCJson != nullptr);

    bool result = metaParser->Parse(metadataCJson);
    cJSON_Delete(metadataCJson);
    ASSERT_TRUE(result);

    // Sequential assignment: JS_OBJECT=0, JS_WRAPPED_NAPI_OBJECT=1
    rawheap_translate::JSType wrappedType = static_cast<rawheap_translate::JSType>(1);
    rawheap_translate::JSType jsObjectType = static_cast<rawheap_translate::JSType>(0);

    ASSERT_TRUE(metaParser->IsJSWrappedNapiObject(wrappedType))
        << "IsJSWrappedNapiObject should return true for JS_WRAPPED_NAPI_OBJECT type";
    ASSERT_FALSE(metaParser->IsJSWrappedNapiObject(jsObjectType))
        << "IsJSWrappedNapiObject should return false for JS_OBJECT type";
    ASSERT_FALSE(metaParser->IsJSWrappedNapiObject(static_cast<rawheap_translate::JSType>(99)))
        << "IsJSWrappedNapiObject should return false for unknown type";
}

HWTEST_F_L0(RawHeapTranslateTest, SourceTextModuleMetaDataParse)
{
    std::string metadataJson =
        "{\"type_enum\": {\"SOURCE_TEXT_MODULE\": 100, \"JS_OBJECT\": 0},"
        "\"type_list\": ["
            "{\"name\": \"JS_OBJECT\","
             "\"offsets\": [], \"end_offset\": 8, \"parents\": []},"
            "{\"name\": \"SOURCE_TEXT_MODULE\","
             "\"offsets\": ["
                 "{\"name\": \"Environment\", \"offset\": 0, \"size\": 8},"
                 "{\"name\": \"Namespace\", \"offset\": 8, \"size\": 8},"
                 "{\"name\": \"EcmaModuleFileName\", \"offset\": 136, \"size\": 8},"
                 "{\"name\": \"EcmaModuleRecordName\", \"offset\": 144, \"size\": 8}"
             "], \"end_offset\": 160, \"parents\": []}"
        "],"
        "\"type_layout\": {"
            "\"Dictionary_layout\": {"
                "\"name\": \"Dictionary\","
                "\"key_index\": 0, \"value_index\": 1,"
                "\"detail_index\": 2, \"entry_size\": 3, \"header_size\": 4"
            "},"
            "\"Type_range\": {"
                "\"string_first\": \"JS_OBJECT\","
                "\"string_last\": \"JS_OBJECT\","
                "\"js_object_first\": \"JS_OBJECT\","
                "\"js_object_last\": \"JS_OBJECT\""
            "}"
        "},"
        "\"version\": \"1.0.0\"}";

    cJSON *metadataCJson = cJSON_ParseWithLength(metadataJson.c_str(), metadataJson.size());
    ASSERT_TRUE(metadataCJson != nullptr);

    bool result = metaParser->Parse(metadataCJson);
    cJSON_Delete(metadataCJson);
    ASSERT_TRUE(result);

    rawheap_translate::MetaData *meta = metaParser->GetMetaData("SOURCE_TEXT_MODULE");
    ASSERT_TRUE(meta != nullptr);
    ASSERT_EQ(meta->endOffset, 160);

    bool foundEcmaModuleFileName = false;
    bool foundEcmaModuleRecordName = false;
    for (const auto &field : meta->fields) {
        if (field.name == "EcmaModuleFileName") {
            foundEcmaModuleFileName = true;
            ASSERT_EQ(field.offset, 136);
            ASSERT_EQ(field.size, 8);
        }
        if (field.name == "EcmaModuleRecordName") {
            foundEcmaModuleRecordName = true;
            ASSERT_EQ(field.offset, 144);
            ASSERT_EQ(field.size, 8);
        }
    }
    ASSERT_TRUE(foundEcmaModuleFileName) << "EcmaModuleFileName field not found in SOURCE_TEXT_MODULE metadata";
    ASSERT_TRUE(foundEcmaModuleRecordName) << "EcmaModuleRecordName field not found in SOURCE_TEXT_MODULE metadata";
}

}  // namespace panda::test
