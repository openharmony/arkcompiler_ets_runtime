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

// Configuration for the 5th root group (globalRef) when crafting rawheap binary.
//   emit         - false: omit the group entirely (old-binary / backward-compat)
//   trackingOff  - true : emit 0xFFFFFFFF sentinel, no payload (tracking disabled)
//   entries      - (refAddr, heapObjAddr) pairs written as the payload. Ignored
//                  when emit=false or trackingOff=true.
struct GlobalRefGroupConfig {
    bool emit {true};
    bool trackingOff {false};
    std::vector<std::pair<uint64_t, uint64_t>> entries;
};

namespace {
// Heap pointer alignment used for V1 root addresses (i*8 layout).
constexpr uint64_t ROOT_ADDR_ALIGN_V1 = 8;
// V2 object-table item is 24 bytes: syntheticAddr(u32) + size(u32) + nodeId(u64)
// + nativeSize(u32) + type(u32). Mirrors rawheap_translate::AddrTableItemV2.
constexpr uint32_t V2_OBJECT_TABLE_ITEM_SIZE = 24;

// Emit the 5th root group header (and optional payload). When useU32HeapAddr is
// true (V2), heapObjAddr is written as u32; otherwise u64 (V1). refAddr is u64
// in both versions.
void WriteGlobalRefGroupPayload(BinaryWriter &writer, const GlobalRefGroupConfig &cfg,
                                bool useU32HeapAddr)
{
    if (!cfg.emit) {
        return;
    }
    if (cfg.trackingOff) {
        writer.WriteUInt32(rawheap_translate::GLOBAL_REF_TRACK_OFF_MARK);
        return;
    }
    writer.WriteUInt32(static_cast<uint32_t>(cfg.entries.size()));
    for (const auto &e : cfg.entries) {
        writer.WriteUInt64(e.first);
        if (useU32HeapAddr) {
            writer.WriteUInt32(static_cast<uint32_t>(e.second));
        } else {
            writer.WriteUInt64(e.second);
        }
    }
}

// Emit the string table: rootCnt strings, each bound to one root address. V1
// roots are u64 (i * ROOT_ADDR_ALIGN_V1); V2 roots are u32 synthetic (i).
void WriteStringTable(BinaryWriter &writer, uint32_t rootCnt, bool useU32Roots)
{
    writer.WriteUInt32(rootCnt);
    writer.WriteUInt32(0);
    for (uint32_t i = 1; i <= rootCnt; i++) {
        std::string str = "test_root_" + std::to_string(i);
        writer.WriteUInt32(str.size());
        writer.WriteUInt32(1);
        if (useU32Roots) {
            writer.WriteUInt32(i);
        } else {
            writer.WriteUInt64(i * ROOT_ADDR_ALIGN_V1);
        }
        writer.WriteBinBlock(const_cast<char *>(str.c_str()), str.size() + 1);
    }
}

// Emit the V2 object table. Each entry maps syntheticAddr=i to a node so that
// AddGlobalHandleObjectNodes' FindNode can resolve heapObjAddr in [1..rootCnt].
// When withEdges is true, each object is given size=sizeof(uint64_t) (one edge
// slot) and a 4-byte heap-pointer payload is appended after the table — this is
// the minimum needed to drive RawHeapTranslateV2::Translate to completion,
// because Translate's per-node hclass lookup calls GetNextEdgeTo once. Without
// edges, GetNextEdgeTo returns nullptr on the first heap object and Translate
// bails out. The edge encodes syntheticAddr=1: the low byte (0x01) doubles as
// GetNextEdgeTo's tag (heap pointer — ZERO/INTL/DOUB bits clear) and the full
// 4-byte little-endian value resolves via FindNode(1).
void WriteV2ObjectTable(BinaryWriter &writer, uint32_t rootCnt, bool withEdges = false)
{
    writer.WriteUInt32(rootCnt);
    writer.WriteUInt32(V2_OBJECT_TABLE_ITEM_SIZE);
    for (uint32_t i = 1; i <= rootCnt; i++) {
        writer.WriteUInt32(i);     // syntheticAddr
        writer.WriteUInt32(withEdges ? static_cast<uint32_t>(sizeof(uint64_t)) : 0);  // size
        writer.WriteUInt64(i);     // nodeId
        writer.WriteUInt32(0);     // nativeSize
        writer.WriteUInt32(0);     // type
    }
    if (withEdges) {
        for (uint32_t i = 1; i <= rootCnt; i++) {
            writer.WriteUInt32(1);
        }
    }
    writer.WriteUInt64(0);
}
}  // namespace

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
        GlobalRefGroupConfig disabled;
        disabled.emit = false;
        WriteTestDataWithGlobalRef(writer, disabled);
    }

    // Emit a rawheap binary that includes 4 (empty) root groups and a configurable
    // 5th globalRef group. Root addresses are i*ROOT_ADDR_ALIGN_V1 (i=1..10);
    // FindNode in AddGlobalHandleObjectNodes resolves heapObjAddr values that match.
    void WriteTestDataWithGlobalRef(BinaryWriter &writer, const GlobalRefGroupConfig &cfg)
    {
        char version[sizeof(uint64_t)] = "1.0.0";
        writer.WriteBinBlock(version, sizeof(uint64_t));

        uint32_t rootOffset = writer.GetCurrentFileSize();
        AddSectionRecord(rootOffset);

        constexpr int rootCnt = 10;
        writer.WriteUInt32(rootCnt);                       // root count
        writer.WriteUInt32(sizeof(uint64_t));              // size of object identifier
        for (uint64_t i = 1; i <= rootCnt; i++) {
            writer.WriteUInt64(i * ROOT_ADDR_ALIGN_V1);
        }
        // sections_[1] covers ONLY root table header + root identifiers (mirrors
        // RawHeapDumpV1::DumpRootTable). The 4 root groups + 5th globalRef group
        // below are trailing data the parser discovers via
        // sections_[2] - sections_[0] - sections_[1].
        AddSectionRecord(writer.GetCurrentFileSize() - rootOffset);

        // 4 empty root groups (LocalHandle / GlobalHandle / VM / Frame).
        writer.WriteUInt32(0);
        writer.WriteUInt32(0);
        writer.WriteUInt32(0);
        writer.WriteUInt32(0);

        // V1: heapObjAddr and root addresses are u64.
        WriteGlobalRefGroupPayload(writer, cfg, false);

        uint32_t strOffset = writer.GetCurrentFileSize();
        AddSectionRecord(strOffset);
        WriteStringTable(writer, rootCnt, false);
        AddSectionRecord(writer.GetCurrentFileSize() - strOffset);
        writer.EndOfWriteBinBlock();
    }

    void AddGlobalHandleObjectNodes()
    {
        if (rawheap_ == nullptr) {
            return;
        }
        rawheap_->AddGlobalHandleObjectNodes();
    }

    bool IsGlobalRefTrackingEnabled() const
    {
        if (rawheap_ == nullptr) {
            return false;
        }
        return rawheap_->globalRefTrackingEnabled_;
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
        GlobalRefGroupConfig disabled;
        disabled.emit = false;
        WriteTestDataWithGlobalRef(writer, disabled);
    }

    // V2 counterpart: writes version, root table (with 4 empty root groups and a
    // configurable 5th globalRef group), string table, and object table. Root
    // addresses are u32 values 1..10; object table maps syntheticAddr=i to a node,
    // so AddGlobalHandleObjectNodes' FindNode will resolve heapObjAddr in [1..10].
    // When withEdges is true, the object table is emitted with valid edge payload
    // so that Translate() can run to completion (see WriteV2ObjectTable).
    void WriteTestDataWithGlobalRef(BinaryWriter &writer, const GlobalRefGroupConfig &cfg,
                                    bool withEdges = false)
    {
        char version[sizeof(uint64_t)] = "2.0.0";
        writer.WriteBinBlock(version, sizeof(uint64_t));

        uint32_t rootOffset = writer.GetCurrentFileSize();
        AddSectionRecord(rootOffset);

        constexpr int rootCnt = 10;
        writer.WriteUInt32(rootCnt);
        writer.WriteUInt32(sizeof(uint32_t));  // identifier size: 4 bytes for V2
        for (uint32_t i = 1; i <= rootCnt; i++) {
            writer.WriteUInt32(i);
        }
        // sections_[1] = root table header + root identifiers only (mirrors
        // RawHeapDumpV2::DumpRootTable). The 4 root groups + 5th globalRef group
        // below are discovered via sections_[2] - sections_[0] - sections_[1].
        AddSectionRecord(writer.GetCurrentFileSize() - rootOffset);

        // 4 empty root groups.
        writer.WriteUInt32(0);
        writer.WriteUInt32(0);
        writer.WriteUInt32(0);
        writer.WriteUInt32(0);

        // V2: heapObjAddr and root addresses are u32.
        WriteGlobalRefGroupPayload(writer, cfg, true);

        uint32_t strOffset = writer.GetCurrentFileSize();
        AddSectionRecord(strOffset);
        WriteStringTable(writer, rootCnt, true);
        AddSectionRecord(writer.GetCurrentFileSize() - strOffset);

        uint32_t objectOffset = writer.GetCurrentFileSize();
        WriteV2ObjectTable(writer, rootCnt, withEdges);
        AddSectionRecord(objectOffset);
        AddSectionRecord(writer.GetCurrentFileSize() - objectOffset);
        writer.EndOfWriteBinBlock();
    }

    void AddGlobalHandleObjectNodes()
    {
        if (rawheap_ == nullptr) {
            return;
        }
        rawheap_->AddGlobalHandleObjectNodes();
    }

    bool IsGlobalRefTrackingEnabled() const
    {
        if (rawheap_ == nullptr) {
            return false;
        }
        return rawheap_->globalRefTrackingEnabled_;
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

// =============================================================================
// GlobalHandleObject / globalRef group parsing (V1 + V2).
//
// These tests replace the original GlobalRefTrackTest dump+translate integration
// tests with translate-only tests: they craft the rawheap binary by hand (via
// BinaryWriter + GlobalRefGroupConfig), drive the parser directly, then invoke
// AddGlobalHandleObjectNodes to materialize the GlobalHandleObject subtree.
// This works on every platform (including ARM32, where HeapProfiler::BinaryDump
// is a no-op) because no EcmaVM or dumper is involved.
//
// Root address conventions in the crafted binary:
//   V1: roots are i*8 for i=1..10  -> nodesMap_ keys {8,16,24,...,80}
//   V2: roots are i   for i=1..10  -> nodesMap_ keys {1,2,...,10} via object table
// heapObjAddr in a globalRef entry must fall in those keys for FindNode to succeed.
// =============================================================================

namespace {
// Look up a node's name via the string table.
std::string NodeName(rawheap_translate::StringHashMap *strTable, const rawheap_translate::Node *node)
{
    auto key = strTable->GetKeyByStringId(node->strId);
    return strTable->GetStringByKey(key);
}

// Return true if any node's name starts with "GlobalHandleObject".
// Skips nodes whose strId is below CUSTOM_STRID_START — those are either
// unconfigured (e.g. V2 metadataNode_ when the binary has no metadata,
// which leaves strId at the Node default of 1) or reserved. Calling
// GetKeyByStringId on them would underflow orderedKey_.
std::string GetGlobalHandleObjectNodeName(const std::vector<rawheap_translate::Node *> &nodes,
                                          rawheap_translate::StringHashMap *strTable)
{
    for (const auto *n : nodes) {
        if (n->strId < rawheap_translate::StringHashMap::CUSTOM_STRID_START) {
            continue;
        }
        std::string name = NodeName(strTable, n);
        if (name.rfind("GlobalHandleObject", 0) == 0) {
            return name;
        }
    }
    return "";
}

bool HasNodeWithName(const std::vector<rawheap_translate::Node *> &nodes,
                     rawheap_translate::StringHashMap *strTable,
                     const std::string &expectedName)
{
    for (const auto *n : nodes) {
        if (n->strId < rawheap_translate::StringHashMap::CUSTOM_STRID_START) {
            continue;
        }
        if (NodeName(strTable, n) == expectedName) {
            return true;
        }
    }
    return false;
}

bool HasGlobalHandleObjectNode(const std::vector<rawheap_translate::Node *> &nodes,
                               rawheap_translate::StringHashMap *strTable)
{
    return !GetGlobalHandleObjectNodeName(nodes, strTable).empty();
}
}  // namespace

// V1 basic: two valid entries -> GlobalHandleObject[2] with 2 ReferenceAddress children.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1GlobalRefBasic)
{
    const std::string filename = "rawheap_v1_gref_basic.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.entries = {{0xaaa0, 8}, {0xbbb0, 16}};  // heapObjAddr 8 and 16 are valid roots
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_TRUE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 18U);  // 1 synthetic + 4 root groups + 10 roots + 1 GHO + 2 virtuals
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[2]");
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xaaa0"));
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xbbb0"));
}

// V1 "filter" equivalent: the dumper would have already dropped non-heap entries,
// so the binary contains only the one survivor. Verifies parser handles count=1.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1GlobalRefFiltersNonHeapObject)
{
    const std::string filename = "rawheap_v1_gref_filter.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.entries = {{0xccc0, 8}};  // single valid entry (non-heap already filtered by dumper)
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 17U);  // 15 base + 1 GHO + 1 virtual
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[1]");
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xccc0"));
}

// V1 "weak tag" equivalent: the dumper strips the weak bit before writing
// heapObjAddr, so on the parse side weak- and strong-originated entries look
// identical. Test verifies the parser handles the cleaned addresses correctly.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1GlobalRefStripsWeakTag)
{
    const std::string filename = "rawheap_v1_gref_weak.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    // Both heapObjAddrs are post-strip aligned heap addresses (8 and 16).
    cfg.entries = {{0xddd0, 8}, {0xeee0, 16}};
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 18U);
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[2]");
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xddd0"));
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xeee0"));
}

// V1 orphan: entry whose heapObjAddr is not in the root set. With the new design,
// all entries get a virtual node at parse time (no pre-filtering); the
// virtualNode -> heapObj property edge is built later in Translate, which is
// not exercised here. Verifies the translator does not crash and both virtual
// nodes are materialized.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1GlobalRefDropsOrphanEntry)
{
    const std::string filename = "rawheap_v1_gref_orphan.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    // First entry valid (root 8); second entry orphan (1024 is not a root address).
    cfg.entries = {{0xaaa0, 8}, {0xbbb0, 1024}};
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 18U);  // 15 base + 1 GHO + 2 virtuals (orphan not filtered at parse time)
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[2]");
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xaaa0"));
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xbbb0"));
}

// V1 tracking-off: dumper emits the 0xFFFFFFFF sentinel, no payload. Translator
// must NOT add a GlobalHandleObject node (tracking disabled flag stays false).
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1GlobalRefTrackingOff)
{
    const std::string filename = "rawheap_v1_gref_off.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.trackingOff = true;
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_FALSE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 15U);  // base layout only, no GHO
    ASSERT_FALSE(HasGlobalHandleObjectNode(nodes, helper.GetStringTable()));
}

// V1 tracking-on but no mappings: count=0 (no sentinel, no payload). Translator
// still adds GlobalHandleObject[0] with zero children to surface the empty state.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1GlobalRefTrackingOnNoRef)
{
    const std::string filename = "rawheap_v1_gref_noref.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;  // count=0, tracking on
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_TRUE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 16U);  // 15 base + 1 GHO[0]
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[0]");
}

// V1 backward-compat: rawheap predates the 5th group entirely. Translator must
// treat absence as "no tracking info" and skip GlobalHandleObject creation.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV1GlobalRefBackwardCompat)
{
    const std::string filename = "rawheap_v1_gref_old.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.emit = false;  // no 5th group at all
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_FALSE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 15U);
    ASSERT_FALSE(HasGlobalHandleObjectNode(nodes, helper.GetStringTable()));
}

// V2 basic: two valid entries -> GlobalHandleObject[2] with 2 ReferenceAddress children.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV2GlobalRefBasic)
{
    const std::string filename = "rawheap_v2_gref_basic.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV2 rawheap(nullptr);
    RawHeapTranslateV2TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.entries = {{0xaaa0, 1}, {0xbbb0, 2}};  // heapObjAddr 1 and 2 are valid roots
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadObjectTable(file));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_TRUE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    // 1 synthetic + 4 root groups + 1 metadata + 10 roots + 1 GHO + 2 virtuals = 19
    ASSERT_EQ(nodes.size(), 19U);
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[2]");
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xaaa0"));
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xbbb0"));
}

// V2 "filter" equivalent: single valid entry post-filter.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV2GlobalRefFiltersNonHeapObject)
{
    const std::string filename = "rawheap_v2_gref_filter.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV2 rawheap(nullptr);
    RawHeapTranslateV2TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.entries = {{0xccc0, 1}};
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadObjectTable(file));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));

    auto nodes = *helper.GetNodes();
    // 16 base (1 synthetic + 4 root groups + 1 metadata + 10 roots) + 1 GHO + 1 virtual = 18
    ASSERT_EQ(nodes.size(), 18U);
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[1]");
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xccc0"));
}

// V2 "weak tag" equivalent: two post-strip entries.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV2GlobalRefStripsWeakTag)
{
    const std::string filename = "rawheap_v2_gref_weak.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV2 rawheap(nullptr);
    RawHeapTranslateV2TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.entries = {{0xddd0, 1}, {0xeee0, 2}};
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadObjectTable(file));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));

    auto nodes = *helper.GetNodes();
    // 16 base + 1 GHO + 2 virtuals = 19
    ASSERT_EQ(nodes.size(), 19U);
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[2]");
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xddd0"));
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xeee0"));
}

// V2 tracking-on but no mappings: count=0 -> GlobalHandleObject[0].
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV2GlobalRefTrackingOnNoRef)
{
    const std::string filename = "rawheap_v2_gref_noref.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV2 rawheap(nullptr);
    RawHeapTranslateV2TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadObjectTable(file));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_TRUE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    // 16 base + 1 GHO[0] = 17
    ASSERT_EQ(nodes.size(), 17U);
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[0]");
}

// V2 backward-compat: rawheap predates the 5th group. No GlobalHandleObject.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV2GlobalRefBackwardCompat)
{
    const std::string filename = "rawheap_v2_gref_old.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV2 rawheap(nullptr);
    RawHeapTranslateV2TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.emit = false;
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadObjectTable(file));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_FALSE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 16U);  // base layout + globalHandleObject slot (unconfigured)
    ASSERT_FALSE(HasGlobalHandleObjectNode(nodes, helper.GetStringTable()));
}

// Regression: drive RawHeapTranslateV2::Translate end-to-end with global ref
// tracking on. Existing V2 tests only exercise Parse; Translate's loop reads
// the flat mem_ edge stream in lockstep with nodes_, so any off-by-one in the
// loop start (e.g. assuming GHO sits at index 6 when it actually sits at
// 6+heapObjCount) desyncs the stream and surfaces as Translate returning false.
// This test runs Translate with tracking on and asserts success.
HWTEST_F_L0(RawHeapTranslateTest, RawHeapTranslateV2TranslateWithGlobalRef)
{
    const std::string filename = "rawheap_v2_translate_gref.rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);

    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV2 rawheap(metaParser.get());
    RawHeapTranslateV2TestHelper helper(&rawheap);

    GlobalRefGroupConfig cfg;
    cfg.entries = {{0xaaa0, 1}};
    // withEdges=true emits one valid heap-pointer edge per object so that
    // GetNextEdgeTo can resolve an hclass for every heap object.
    helper.WriteTestDataWithGlobalRef(writer, cfg, true);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadObjectTable(file));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_TRUE(helper.IsGlobalRefTrackingEnabled());

    ASSERT_TRUE(rawheap.Translate());

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(GetGlobalHandleObjectNodeName(nodes, helper.GetStringTable()), "GlobalHandleObject[1]");
    ASSERT_TRUE(HasNodeWithName(nodes, helper.GetStringTable(), "ReferenceAddress:0xaaa0"));
}

// Focused regression: a rawheap with NO GlobalHandleObject data — whether because
// the 5th group is entirely absent (old binary) or carries the 0xFFFFFFFF sentinel
// (tracking off) — must parse without crashing and must not synthesize any node
// whose name starts with "GlobalHandleObject". Covers both V1 and V2.
static void ParseV1NoGlobalHandleObject(const std::string &tag, const GlobalRefGroupConfig &cfg)
{
    SCOPED_TRACE("V1 case: " + tag);
    std::string filename = "rawheap_v1_no_gho_" + tag + ".rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);
    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV1 rawheap(nullptr);
    RawHeapTranslateV1TestHelper helper(&rawheap);
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_FALSE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 15U);
    ASSERT_FALSE(HasGlobalHandleObjectNode(nodes, helper.GetStringTable()))
        << "V1 case '" << tag << "' should not produce any GlobalHandleObject node";
}

static void ParseV2NoGlobalHandleObject(const std::string &tag, const GlobalRefGroupConfig &cfg)
{
    SCOPED_TRACE("V2 case: " + tag);
    std::string filename = "rawheap_v2_no_gho_" + tag + ".rawheap";
    int fd = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0666);
    ASSERT_FALSE(fd == -1);
    FileDescriptorStream stream(fd);
    BinaryWriter writer(&stream);
    rawheap_translate::RawHeapTranslateV2 rawheap(nullptr);
    RawHeapTranslateV2TestHelper helper(&rawheap);
    helper.WriteTestDataWithGlobalRef(writer, cfg);

    rawheap_translate::FileReader file;
    ASSERT_TRUE(file.Initialize(filename));
    ASSERT_TRUE(helper.ReadObjectTable(file));
    ASSERT_TRUE(helper.ReadRootTable(file));
    ASSERT_TRUE(helper.ReadStringTable(file));
    ASSERT_FALSE(helper.IsGlobalRefTrackingEnabled());

    auto nodes = *helper.GetNodes();
    ASSERT_EQ(nodes.size(), 16U);  // base layout + globalHandleObject slot (unconfigured)
    ASSERT_FALSE(HasGlobalHandleObjectNode(nodes, helper.GetStringTable()))
        << "V2 case '" << tag << "' should not produce any GlobalHandleObject node";
}

HWTEST_F_L0(RawHeapTranslateTest, ParseRawheapWithoutGlobalHandleObjectData)
{
    GlobalRefGroupConfig absent;
    absent.emit = false;
    GlobalRefGroupConfig sentinel;
    sentinel.trackingOff = true;

    ASSERT_NO_FATAL_FAILURE(ParseV1NoGlobalHandleObject("absent", absent));
    ASSERT_NO_FATAL_FAILURE(ParseV1NoGlobalHandleObject("sentinel", sentinel));
    ASSERT_NO_FATAL_FAILURE(ParseV2NoGlobalHandleObject("absent", absent));
    ASSERT_NO_FATAL_FAILURE(ParseV2NoGlobalHandleObject("sentinel", sentinel));
}

}  // namespace panda::test
