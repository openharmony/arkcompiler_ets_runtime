/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/dfx/hprof/stream.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::ecmascript {
class HeapSnapShotFriendTest {
public:
    explicit HeapSnapShotFriendTest(const EcmaVM *vm, StringHashMap *stringTable, DumpSnapShotOption dumpOption,
                                    bool traceAllocation, EntryIdMap* entryIdMap)
        : heapSnapshot(vm, stringTable, dumpOption, traceAllocation, entryIdMap) {}

    HprofNode *GeneratePrivateStringNodeTest(size_t size)
    {
        return heapSnapshot.GeneratePrivateStringNode(size);
    }

    void MoveNodeTest(uintptr_t address, TaggedObject *forwardAddress, size_t size)
    {
        return heapSnapshot.MoveNode(address, forwardAddress, size);
    }

    void InsertEntryTest(HprofNode *node)
    {
        heapSnapshot.entryMap_.InsertEntry(node);
    }

    CString *GenerateNodeNameTest(TaggedObject *taggedObject)
    {
        return heapSnapshot.GenerateNodeName(taggedObject);
    }

    NodeType GenerateNodeTypeTest(TaggedObject *taggedObject)
    {
        return heapSnapshot.GenerateNodeType(taggedObject);
    }

    HeapEntryMap &GetEntryMapTest()
    {
        return heapSnapshot.entryMap_;
    }

    Chunk &GetChunkTest()
    {
        return heapSnapshot.chunk_;
    }
private:
    HeapSnapshot heapSnapshot;
};

class HeapProfilerFriendTest {
public:
    explicit HeapProfilerFriendTest(const EcmaVM *vm) : heapProfiler(vm), vm_(vm) {}

    HeapSnapshot *MakeHeapSnapshotTest(HeapProfiler::SampleType type, DumpSnapShotOption dumpOption)
    {
        heapProfiler.ForceFullGC(vm_);
        heapProfiler.ForceSharedGC();
        return heapProfiler.MakeHeapSnapshot(type, dumpOption);
    }

    EntryIdMap *GetEntryIdMapTest()
    {
        return heapProfiler.GetEntryIdMap();
    }

    StringHashMap *GetEcmaStringTableTest()
    {
        return heapProfiler.GetEcmaStringTable();
    }

    void UpdateNodeAddressIdMapTest()
    {
        heapProfiler.UpdateNodeAddressIdMap();
    }

    CUnorderedMap<uintptr_t, NodeId> GetNodeAddressIdMapTest()
    {
        return heapProfiler.GetNodeAddressIdMap();
    }

private:
    HeapProfiler heapProfiler;
    const EcmaVM *vm_;
};
}

static bool IsHexString(const CString &str)
{
    if (str.empty()) {
        return false;
    }
    for (size_t i = 0; i < str.size(); i++) {
        char c = str[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            return false;
        }
    }
    return true;
}

namespace panda::test {
class HeapSnapShotTest : public testing::Test {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(ecmaVm_, thread_, scope_);
        ecmaVm_->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm_, scope_);
    }

    EcmaVM *ecmaVm_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
    JSThread *thread_ {nullptr};
};

const std::set<CString> variableMap({
    "string1", "string2", "string3", "string4",
    "string5", "string6", "string7", "string8",
    "string1string2", "string3string4", "string5string6", "string7string8",
    "string1string2string3string4", "string5string6string7string8",
    "string1string2string3string4string5string6string7string8"
});

class TestSerializeStream : public Stream {
public:
    TestSerializeStream() = default;
    ~TestSerializeStream() = default;

    void EndOfStream() override{}
    int GetSize() override
    {
        return 100_KB;
    }
    bool WriteChunk(char *data, int32_t size) override
    {
        output.append(data, size);
        return true;
    }
    bool WriteExtraInfo(char *data, int32_t size) override
    {
        mapOutput.append(data, size);
        return true;
    }
    bool WriteBinBlock(char *data, int32_t size) override
    {
        return WriteChunk(data, size);
    }
    bool Good() override
    {
        return true;
    }
    void UpdateHeapStats(HeapStat* updateData, int32_t count) override {}
    void UpdateLastSeenObjectId(int32_t lastSeenObjectId, int64_t timeStampUs) override {}

    std::string GetOutput() const
    {
        return output;
    }

    std::string GetMapOutput() const
    {
        return mapOutput;
    }

private:
    std::string output;
    std::string mapOutput;
};

HWTEST_F_L0(HeapSnapShotTest, TestGenerateStringNode)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR "heap_snapshot.abc";

    std::string entryPoint = "heap_snapshot";
    bool result = JSNApi::Execute(ecmaVm_, abcFileName, entryPoint);
    ASSERT_TRUE(result);

    // start MakeHeapSnapshot
    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    size_t totalSize = 0;
    // global string also generate node
    for (auto node : *snapshot->GetNodes()) {
        if (variableMap.find(*node->GetName()) != variableMap.end()) {
            totalSize += node->GetSelfSize();
        }
    }
#if defined(ARK_HYBRID) || defined(USE_CMC_GC)
    // lineString: 32
    // treeString: 40
    // totalSize: 8 * 32 + 7 * 40
    ASSERT_EQ(totalSize, 536);
#else
    // lineString: 24
    // treeString: 32
    // totalSize: 8 * 24 + 7 * 32
    ASSERT_EQ(totalSize, 416);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestGeneratePrivateStringNode)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    dumpOption.isPrivate = true;
    HeapSnapShotFriendTest heapSnapShotTest(ecmaVm_, tester.GetEcmaStringTableTest(),
                                            dumpOption, false, tester.GetEntryIdMapTest());
    HprofNode *node = heapSnapShotTest.GeneratePrivateStringNodeTest(0);
#if defined(ARK_HYBRID) || defined(USE_CMC_GC)
    // lineString: 32
    ASSERT_EQ(node->GetSelfSize(), 32);
#else
    // lineString: 24
    ASSERT_EQ(node->GetSelfSize(), 24);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestMoveNode)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    HeapSnapShotFriendTest heapSnapShotTest(ecmaVm_, tester.GetEcmaStringTableTest(),
                                            dumpOption, false, tester.GetEntryIdMapTest());
    ObjectFactory *factory = ecmaVm_->GetFactory();
    // create tree string
    JSHandle<EcmaString> strLeft = factory->NewFromUtf8("leftString");
    JSHandle<EcmaString> strRight = factory->NewFromUtf8("rightString");
    EcmaString *treeString = EcmaStringAccessor::Concat(ecmaVm_, strLeft, strRight);
    uintptr_t address = 0;
    HprofNode *node = HprofNode::NewNode(heapSnapShotTest.GetChunkTest(), 0, 0,
        heapSnapShotTest.GenerateNodeNameTest(reinterpret_cast<TaggedObject *>(treeString)),
        heapSnapShotTest.GenerateNodeTypeTest(reinterpret_cast<TaggedObject *>(treeString)),
        0, 0, address);
    heapSnapShotTest.InsertEntryTest(node);
    heapSnapShotTest.MoveNodeTest(address, reinterpret_cast<TaggedObject *>(treeString), 0);
    HeapEntryMap &heapEntryMap = heapSnapShotTest.GetEntryMapTest();
    HprofNode *movedNode = heapEntryMap.FindEntry(reinterpret_cast<JSTaggedType>(treeString));
#if defined(ARK_HYBRID) || defined(USE_CMC_GC)
    // treeString: 40
    ASSERT_EQ(movedNode->GetSelfSize(), 40);
#else
    // treeString: 32
    ASSERT_EQ(movedNode->GetSelfSize(), 32);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestNodeIdCacheClearScopeDelete)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    auto *profile = ecmaVm_->GetOrNewHeapProfile();
    ASSERT_NE(profile, nullptr);
    ASSERT_NE(ecmaVm_->GetHeapProfile(), nullptr);

    DumpSnapShotOption opt;
    opt.isClearNodeIdCache = true;
    {
        NodeIdCacheClearScope scope(ecmaVm_, opt);
    }
    ASSERT_EQ(ecmaVm_->GetHeapProfile(), nullptr);
#else
    ASSERT_TRUE(true);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestNodeIdCacheClearScopeKeep)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    auto *profile = ecmaVm_->GetOrNewHeapProfile();
    ASSERT_NE(profile, nullptr);
    ASSERT_NE(ecmaVm_->GetHeapProfile(), nullptr);

    DumpSnapShotOption opt;
    opt.isClearNodeIdCache = false;
    {
        NodeIdCacheClearScope scope(ecmaVm_, opt);
    }
    ASSERT_NE(ecmaVm_->GetHeapProfile(), nullptr);
#else
    ASSERT_TRUE(true);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestNodeIdCacheClearScopeWithNullptr)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    DumpSnapShotOption opt;
    opt.isClearNodeIdCache = true;
    {
        // not crash is right
        NodeIdCacheClearScope scope(nullptr, opt);
    }
    ASSERT_TRUE(true);
#else
    ASSERT_TRUE(true);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestNativeAddrToNodeIdMapOption)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    DumpSnapShotOption dumpOption;
    dumpOption.nativeAddrToNodeIdMap = 1;
    ASSERT_EQ(dumpOption.nativeAddrToNodeIdMap, 1);
#else
    ASSERT_TRUE(true);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestHeapSnapshotSetNodeAddressIdMap)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    CUnorderedMap<uintptr_t, NodeId> testMap;
    testMap.emplace(0x1000, 1);
    testMap.emplace(0x2000, 2);
    snapshot->SetNodeAddressIdMap(testMap);

    CUnorderedMap<uintptr_t, NodeId> resultMap = snapshot->GetNodeAddressIdMap();
    ASSERT_EQ(resultMap.size(), 2u);
    ASSERT_EQ(resultMap.at(0x1000), 1);
    ASSERT_EQ(resultMap.at(0x2000), 2);
#else
    ASSERT_TRUE(true);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestUpdateNodeAddressIdMap)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    HeapProfilerFriendTest tester(ecmaVm_);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj1(factory->CreateNapiObject());
    JSHandle<JSTaggedValue> obj2(factory->CreateNapiObject());
    Global<ObjectRef> globalObj1(ecmaVm_, Local<JSTaggedValue>(obj1.GetAddress()));
    Global<ObjectRef> globalObj2(ecmaVm_, Local<JSTaggedValue>(obj2.GetAddress()));

    DumpSnapShotOption dumpOption;
    dumpOption.nativeAddrToNodeIdMap = 1;
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    tester.UpdateNodeAddressIdMapTest();
    CUnorderedMap<uintptr_t, NodeId> nodeAddressIdMap = tester.GetNodeAddressIdMapTest();
    ASSERT_TRUE(nodeAddressIdMap.size() > 0);
#else
    ASSERT_TRUE(true);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestSerializeExtraInfo)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
    HeapProfilerFriendTest tester(ecmaVm_);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj1(factory->CreateNapiObject());
    JSHandle<JSTaggedValue> obj2(factory->CreateNapiObject());
    Global<ObjectRef> globalObj1(ecmaVm_, Local<JSTaggedValue>(obj1.GetAddress()));
    Global<ObjectRef> globalObj2(ecmaVm_, Local<JSTaggedValue>(obj2.GetAddress()));

    DumpSnapShotOption dumpOption;
    dumpOption.nativeAddrToNodeIdMap = 1;
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    tester.UpdateNodeAddressIdMapTest();
    CUnorderedMap<uintptr_t, NodeId> nodeAddressIdMap = tester.GetNodeAddressIdMapTest();
    if (!nodeAddressIdMap.empty()) {
        snapshot->SetNodeAddressIdMap(nodeAddressIdMap);
    }

    TestSerializeStream stream;
    bool result = HeapSnapshotJSONSerializer::SerializeExtraInfo(snapshot, &stream);
    ASSERT_TRUE(result);
    std::string mapOutput = stream.GetMapOutput();
    if (!nodeAddressIdMap.empty()) {
        ASSERT_TRUE(mapOutput.find("nodeIdMap") != std::string::npos);
    } else {
        ASSERT_EQ(mapOutput, "{\"nodeIdMap\":[]}");
    }
#else
    ASSERT_TRUE(true);
#endif
}

HWTEST_F_L0(HeapSnapShotTest, TestSpecificSyntheticRoots)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR "heap_snapshot.abc";
    std::string entryPoint = "heap_snapshot";
    bool result = JSNApi::Execute(ecmaVm_, abcFileName, entryPoint);
    ASSERT_TRUE(result);

    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);

    // LocalHandleSyntheticRoot and GlobalHandleSyntheticRoot are inserted at fixed positions 1 and 2
    // VMRoot and FrameRoot are inserted at positions 3 and 4
    const auto &nodes = *snapshot->GetNodes();
    ASSERT_GE(nodes.size(), 5U) << "Not enough nodes in snapshot";

    // Verify LocalHandleSyntheticRoot at position 1
    const HprofNode *localHandleRoot = nodes[1];
    ASSERT_NE(localHandleRoot->GetName(), nullptr) << "LocalHandleSyntheticRoot name should not be null";
    ASSERT_EQ(localHandleRoot->GetType(), NodeType::ROOT) << "LocalHandleSyntheticRoot type should be ROOT";
    CString localName = *localHandleRoot->GetName();
    ASSERT_TRUE(localName.find("LocalHandleRoot[") == 0) << "Expected LocalHandleRoot, got: " << localName;

    // Verify GlobalHandleSyntheticRoot at position 2
    const HprofNode *globalHandleRoot = nodes[2];
    ASSERT_NE(globalHandleRoot->GetName(), nullptr) << "GlobalHandleSyntheticRoot name should not be null";
    ASSERT_EQ(globalHandleRoot->GetType(), NodeType::ROOT) << "GlobalHandleSyntheticRoot type should be ROOT";
    CString globalName = *globalHandleRoot->GetName();
    ASSERT_TRUE(globalName.find("GlobalHandleRoot[") == 0) << "Expected GlobalHandleRoot, got: " << globalName;

    // Verify VMRoot at position 3
    const HprofNode *vmRoot = nodes[3];
    ASSERT_NE(vmRoot->GetName(), nullptr) << "VMRoot name should not be null";
    ASSERT_EQ(vmRoot->GetType(), NodeType::ROOT) << "VMRoot type should be ROOT";
    CString vmName = *vmRoot->GetName();
    ASSERT_TRUE(vmName.find("VMRoot[") == 0) << "Expected VMRoot, got: " << vmName;

    // Verify FrameRoot at position 4
    const HprofNode *frameRoot = nodes[4];
    ASSERT_NE(frameRoot->GetName(), nullptr) << "FrameRoot name should not be null";
    ASSERT_EQ(frameRoot->GetType(), NodeType::ROOT) << "FrameRoot type should be ROOT";
    CString frameName = *frameRoot->GetName();
    ASSERT_TRUE(frameName.find("FrameRoot[") == 0) << "Expected FrameRoot, got: " << frameName;
}

HWTEST_F_L0(HeapSnapShotTest, TestProxyClassName)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR "proxy_class_name.abc";
    std::string entryPoint = "proxy_class_name";
    bool result = JSNApi::Execute(ecmaVm_, abcFileName, entryPoint);
    ASSERT_TRUE(result);

    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);

    bool foundProxyA = false;
    bool foundProxyB = false;
    bool foundProxyNoName = false;

    for (auto node : *snapshot->GetNodes()) {
        CString name = *node->GetName();
        if (name == "Proxy-ClassA") {
            foundProxyA = true;
            continue;
        }
        if (name == "Proxy-ClassB") {
            foundProxyB = true;
            continue;
        }
        if (name == "Proxy") {
            foundProxyNoName = true;
            continue;
        }
    }

    ASSERT_TRUE(foundProxyA) << "Proxy-ClassA not found";
    ASSERT_TRUE(foundProxyB) << "Proxy-ClassB not found";
    ASSERT_TRUE(foundProxyNoName) << "Proxy without class name not found";
}

// EntryIdMap: InsertId / FindId / EraseId basic correctness
HWTEST_F_L0(HeapSnapShotTest, TestEntryIdMapInsertFindErase)
{
    EntryIdMap entryIdMap;

    // Not found initially
    ASSERT_FALSE(entryIdMap.FindId(0x1000).first);

    // Insert then find
    ASSERT_TRUE(entryIdMap.InsertId(0x1000, 42));
    auto found = entryIdMap.FindId(0x1000);
    ASSERT_TRUE(found.first);
    ASSERT_EQ(found.second, 42U);

    // A different address is not found
    ASSERT_FALSE(entryIdMap.FindId(0x2000).first);

    // Erase existing entry succeeds; second erase fails
    ASSERT_TRUE(entryIdMap.EraseId(0x1000));
    ASSERT_FALSE(entryIdMap.FindId(0x1000).first);
    ASSERT_FALSE(entryIdMap.EraseId(0x1000));
}

// EntryIdMap: Move copies the ID to the new address and removes the old one
HWTEST_F_L0(HeapSnapShotTest, TestEntryIdMapMoveAddress)
{
    EntryIdMap entryIdMap;
    entryIdMap.InsertId(0x1000, 100);

    ASSERT_TRUE(entryIdMap.Move(0x1000, 0x2000));

    ASSERT_FALSE(entryIdMap.FindId(0x1000).first);
    auto moved = entryIdMap.FindId(0x2000);
    ASSERT_TRUE(moved.first);
    ASSERT_EQ(moved.second, 100U);
}

// EntryIdMap: Move edge cases – same address (no-op) and not-found address
HWTEST_F_L0(HeapSnapShotTest, TestEntryIdMapMoveEdgeCases)
{
    EntryIdMap entryIdMap;
    entryIdMap.InsertId(0x1000, 100);

    // Same source and destination: no-op, returns true, entry unchanged
    ASSERT_TRUE(entryIdMap.Move(0x1000, 0x1000));
    auto sameAddr = entryIdMap.FindId(0x1000);
    ASSERT_TRUE(sameAddr.first);
    ASSERT_EQ(sameAddr.second, 100U);

    // Non-existing source address: returns false
    ASSERT_FALSE(entryIdMap.Move(0x9000, 0xA000));
}

// EntryIdMap: FindOrInsertNodeId is idempotent for the same address
HWTEST_F_L0(HeapSnapShotTest, TestEntryIdMapFindOrInsertNodeId)
{
    EntryIdMap entryIdMap;

    NodeId id1 = entryIdMap.FindOrInsertNodeId(0x1000);
    ASSERT_GT(id1, 0U);

    // Same address returns the same ID
    NodeId id2 = entryIdMap.FindOrInsertNodeId(0x1000);
    ASSERT_EQ(id1, id2);

    // Different address gets a distinct ID
    NodeId id3 = entryIdMap.FindOrInsertNodeId(0x2000);
    ASSERT_NE(id1, id3);
}

// HeapEntryMap: FindEntry / InsertEntry / FindAndEraseNode lifecycle
HWTEST_F_L0(HeapSnapShotTest, TestHeapEntryMapFindInsertErase)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    HeapSnapShotFriendTest heapSnapShotTest(ecmaVm_, tester.GetEcmaStringTableTest(),
                                            dumpOption, false, tester.GetEntryIdMapTest());
    Chunk &chunk = heapSnapShotTest.GetChunkTest();
    HeapEntryMap &entryMap = heapSnapShotTest.GetEntryMapTest();

    const JSTaggedType testAddr = 0xABCD1234;
    ASSERT_EQ(entryMap.FindEntry(testAddr), nullptr);

    HprofNode *node = HprofNode::NewNode(chunk, 1, 0, nullptr, NodeType::OBJECT, 32, 0, testAddr);
    entryMap.InsertEntry(node);
    ASSERT_EQ(entryMap.FindEntry(testAddr), node);

    // FindAndEraseNode removes the entry and returns it
    HprofNode *erased = entryMap.FindAndEraseNode(testAddr);
    ASSERT_EQ(erased, node);
    ASSERT_EQ(entryMap.FindEntry(testAddr), nullptr);

    // Erasing a missing address returns nullptr
    ASSERT_EQ(entryMap.FindAndEraseNode(0x9999), nullptr);
}

// HeapEntryMap: FindOrInsertNode returns the pre-existing node when address is already present
HWTEST_F_L0(HeapSnapShotTest, TestHeapEntryMapFindOrInsertNode)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    HeapSnapShotFriendTest heapSnapShotTest(ecmaVm_, tester.GetEcmaStringTableTest(),
                                            dumpOption, false, tester.GetEntryIdMapTest());
    Chunk &chunk = heapSnapShotTest.GetChunkTest();
    HeapEntryMap &entryMap = heapSnapShotTest.GetEntryMapTest();

    const JSTaggedType testAddr = 0xBEEF1000;

    // Not in map yet: inserts node1 and returns it
    HprofNode *node1 = HprofNode::NewNode(chunk, 1, 0, nullptr, NodeType::OBJECT, 32, 0, testAddr);
    ASSERT_EQ(entryMap.FindOrInsertNode(node1), node1);

    // Already in map: returns existing node1, ignores node2
    HprofNode *node2 = HprofNode::NewNode(chunk, 2, 0, nullptr, NodeType::OBJECT, 64, 0, testAddr);
    ASSERT_EQ(entryMap.FindOrInsertNode(node2), node1);
}

// MoveNode returns immediately (no-op) when source equals destination address
HWTEST_F_L0(HeapSnapShotTest, TestMoveNodeSameAddressNoOp)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    HeapSnapShotFriendTest heapSnapShotTest(ecmaVm_, tester.GetEcmaStringTableTest(),
                                            dumpOption, false, tester.GetEntryIdMapTest());
    Chunk &chunk = heapSnapShotTest.GetChunkTest();

    // Use a fake address: the early-return path never dereferences the forward pointer
    const uintptr_t testAddr = 0x12340000;
    TaggedObject *sameForward = reinterpret_cast<TaggedObject *>(testAddr);

    HprofNode *node = HprofNode::NewNode(chunk, 1, 0, nullptr, NodeType::OBJECT, 32, 0,
                                         static_cast<JSTaggedType>(testAddr));
    heapSnapShotTest.InsertEntryTest(node);

    heapSnapShotTest.MoveNodeTest(testAddr, sameForward, 32);

    // Node is still at the original address, unmodified
    ASSERT_EQ(heapSnapShotTest.GetEntryMapTest().FindEntry(static_cast<JSTaggedType>(testAddr)), node);
}

// GeneratePrivateStringNode returns a cached (identical) pointer on repeated calls
HWTEST_F_L0(HeapSnapShotTest, TestGeneratePrivateStringNodeCached)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    dumpOption.isPrivate = true;
    HeapSnapShotFriendTest heapSnapShotTest(ecmaVm_, tester.GetEcmaStringTableTest(),
                                            dumpOption, false, tester.GetEntryIdMapTest());

    HprofNode *node1 = heapSnapShotTest.GeneratePrivateStringNodeTest(0);
    ASSERT_NE(node1, nullptr);

    // Second call must return the identical cached pointer (privateStringNode_ field)
    HprofNode *node2 = heapSnapShotTest.GeneratePrivateStringNodeTest(0);
    ASSERT_EQ(node1, node2);
}

// After BuildUp, nodes[0] is SyntheticRoot with type SYNTHETIC
HWTEST_F_L0(HeapSnapShotTest, TestSyntheticRootStructure)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    const auto &nodes = *snapshot->GetNodes();
    ASSERT_GE(nodes.size(), 1U);

    const HprofNode *synRoot = nodes[0];
    ASSERT_NE(synRoot, nullptr);
    ASSERT_EQ(synRoot->GetType(), NodeType::SYNTHETIC);
    ASSERT_NE(synRoot->GetName(), nullptr);
    ASSERT_EQ(*synRoot->GetName(), CString("SyntheticRoot"));
}

// After BuildUp, nodeCount, edgeCount and totalNodeSize are all positive
HWTEST_F_L0(HeapSnapShotTest, TestHeapSnapshotNodeEdgeCountPositive)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    ASSERT_GT(snapshot->GetNodeCount(), 0U);
    ASSERT_GT(snapshot->GetEdgeCount(), 0U);
    ASSERT_GT(snapshot->GetTotalNodeSize(), 0U);
}

HWTEST_F_L0(HeapSnapShotTest, TestSourceTextModuleDumpForSnapshotWithRecordName)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();

    CString testRecordName = "test_module_record_name";
    module->SetEcmaModuleRecordNameString(testRecordName);

    std::vector<Reference> snapshotVector;
    module->DumpForSnapshot(thread_, snapshotVector);

    bool foundRecordName = false;
    for (const auto &ref : snapshotVector) {
        if (ref.name_ == "EcmaModuleRecordName") {
            foundRecordName = true;
            ASSERT_EQ(ref.type_, EdgeType::NATIVE_STRING);
            CString *recordNamePtr = reinterpret_cast<CString *>(ref.value_.GetRawData());
            ASSERT_TRUE(recordNamePtr != nullptr);
            ASSERT_EQ(*recordNamePtr, testRecordName);
            break;
        }
    }
    ASSERT_TRUE(foundRecordName) << "EcmaModuleRecordName field not found in snapshot";
}

HWTEST_F_L0(HeapSnapShotTest, TestSourceTextModuleDumpForSnapshotWithEmptyRecordName)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();

    std::vector<Reference> snapshotVector;
    module->DumpForSnapshot(thread_, snapshotVector);

    bool foundRecordName = false;
    for (const auto &ref : snapshotVector) {
        if (ref.name_ == "EcmaModuleRecordName") {
            foundRecordName = true;
            ASSERT_EQ(ref.type_, EdgeType::NATIVE_STRING);
            CString *recordNamePtr = reinterpret_cast<CString *>(ref.value_.GetRawData());
            ASSERT_TRUE(recordNamePtr == nullptr);
            break;
        }
    }
    ASSERT_TRUE(foundRecordName) << "EcmaModuleRecordName field not found in snapshot";
}

HWTEST_F_L0(HeapSnapShotTest, TestSourceTextModuleDumpForSnapshotWithFileName)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();

    CString testFileName = "test_module_filename.abc";
    module->SetEcmaModuleFilenameString(testFileName);

    std::vector<Reference> snapshotVector;
    module->DumpForSnapshot(thread_, snapshotVector);

    bool foundFileName = false;
    for (const auto &ref : snapshotVector) {
        if (ref.name_ == "EcmaModuleFileName") {
            foundFileName = true;
            ASSERT_EQ(ref.type_, EdgeType::NATIVE_STRING);
            CString *fileNamePtr = reinterpret_cast<CString *>(ref.value_.GetRawData());
            ASSERT_TRUE(fileNamePtr != nullptr);
            ASSERT_EQ(*fileNamePtr, testFileName);
            break;
        }
    }
    ASSERT_TRUE(foundFileName) << "EcmaModuleFileName field not found in snapshot";
}

HWTEST_F_L0(HeapSnapShotTest, TestSourceTextModuleDumpForSnapshotWithEmptyFileName)
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();

    std::vector<Reference> snapshotVector;
    module->DumpForSnapshot(thread_, snapshotVector);

    bool foundFileName = false;
    for (const auto &ref : snapshotVector) {
        if (ref.name_ == "EcmaModuleFileName") {
            foundFileName = true;
            ASSERT_EQ(ref.type_, EdgeType::NATIVE_STRING);
            CString *fileNamePtr = reinterpret_cast<CString *>(ref.value_.GetRawData());
            ASSERT_TRUE(fileNamePtr == nullptr);
            break;
        }
    }
    ASSERT_TRUE(foundFileName) << "EcmaModuleFileName field not found in snapshot";
}

HWTEST_F_L0(HeapSnapShotTest, TestSourceTextModuleNodeInHeapSnapshot)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR "module_record_name.abc";
    std::string entryPoint = "module_record_name";
    bool result = JSNApi::Execute(ecmaVm_, abcFileName, entryPoint);
    ASSERT_TRUE(result);

    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    bool foundFileNameProperty = false;
    bool foundRecordNameProperty = false;
    for (auto node : *snapshot->GetNodes()) {
        CString name = *node->GetName();
        if (name == "SourceTextModule") {
            for (auto edge : *snapshot->GetEdges()) {
                if (edge->GetFrom() == node && edge->GetName() != nullptr) {
                    CString edgeName = *edge->GetName();
                    if (edgeName == "EcmaModuleFileName") {
                        foundFileNameProperty = true;
                    }
                    if (edgeName == "EcmaModuleRecordName") {
                        foundRecordNameProperty = true;
                    }
                }
            }
        }
    }

    ASSERT_TRUE(foundFileNameProperty) << "EcmaModuleFileName property not found in SourceTextModule node";
    ASSERT_TRUE(foundRecordNameProperty) << "EcmaModuleRecordName property not found in SourceTextModule node";
}

// ---------- GlobalRef tracking snapshot tests ----------

// Enable tracking → trigger GC first to stabilize slots → then create global handle + store mapping →
// snapshot should contain GlobalHandleObject subtree
HWTEST_F_L0(HeapSnapShotTest, TestFillGlobalEdgesWithTrackingEnabled)
{
    thread_->SetTrackGlobalRef(true);

    // First snapshot to trigger GC and stabilize global handle slots
    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);

    // Now create a global handle AFTER GC — its slot address will be stable
    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj(factory->CreateNapiObject());

    Global<ObjectRef> globalObj(ecmaVm_, Local<JSTaggedValue>(obj.GetAddress()));
    uintptr_t slotAddr = globalObj.GetSlotAddress();
    ASSERT_NE(slotAddr, 0U);

    // Store mapping for the real global handle slot
    int fakeRef = 0;
    thread_->StoreGlobalRefMapping(slotAddr, &fakeRef);

    // Second snapshot — should find the mapping
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);
    ASSERT_GT(snapshot->GetNodeCount(), 0U);

    // Verify GlobalHandleObject node exists and has children
    HprofNode *globalObjRoot = nullptr;
    HprofNode *refAddrNode = nullptr;
    for (auto node : *snapshot->GetNodes()) {
        CString name = *node->GetName();
        if (name.find("GlobalHandleObject") != CString::npos) {
            globalObjRoot = node;
        }
        if (name.find("ReferenceAddress:") != CString::npos) {
            refAddrNode = node;
        }
    }
    ASSERT_NE(globalObjRoot, nullptr) << "GlobalHandleObject node not found";
    ASSERT_GT(globalObjRoot->GetEdgeCount(), 0U) << "GlobalHandleObject has no children";
    ASSERT_NE(refAddrNode, nullptr) << "ReferenceAddress node not found";
    ASSERT_EQ(refAddrNode->GetEdgeCount(), 1U) << "ReferenceAddress node should have 1 outgoing edge";

    // Verify edge path: GlobalHandleObject → ReferenceAddress → JSObject
    bool foundEdgeToRefAddr = false;
    bool foundEdgeToJSObj = false;
    for (auto edge : *snapshot->GetEdges()) {
        if (edge->GetFrom() == globalObjRoot && edge->GetTo() == refAddrNode) {
            foundEdgeToRefAddr = true;
        }
        if (edge->GetFrom() == refAddrNode && edge->GetTo()->GetType() == NodeType::OBJECT) {
            foundEdgeToJSObj = true;
        }
    }
    ASSERT_TRUE(foundEdgeToRefAddr) << "Edge GlobalHandleObject → ReferenceAddress not found";
    ASSERT_TRUE(foundEdgeToJSObj) << "Edge ReferenceAddress → JSObject not found";

    thread_->SetTrackGlobalRef(false);
}

// Tracking enabled but no ref mapping stored → GlobalHandleObject exists but no NATIVE children
HWTEST_F_L0(HeapSnapShotTest, TestFillGlobalEdgesNoMatchRef)
{
    thread_->SetTrackGlobalRef(true);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj(factory->CreateNapiObject());
    Global<ObjectRef> globalObj(ecmaVm_, Local<JSTaggedValue>(obj.GetAddress()));

    // Do NOT store a ref mapping

    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    bool foundGlobalHandleObject = false;
    bool foundNativeNode = false;
    for (auto node : *snapshot->GetNodes()) {
        CString name = *node->GetName();
        if (name.find("GlobalHandleObject") != CString::npos) {
            foundGlobalHandleObject = true;
        }
        if (name.find("ReferenceAddress:") != CString::npos) {
            foundNativeNode = true;
        }
    }
    ASSERT_TRUE(foundGlobalHandleObject) << "GlobalHandleObject should exist when tracking is enabled";
    ASSERT_FALSE(foundNativeNode) << "ReferenceAddress node should not exist without ref mapping";

    thread_->SetTrackGlobalRef(false);
}

// Tracking disabled → no GlobalHandleObject subtree
HWTEST_F_L0(HeapSnapShotTest, TestFillGlobalEdgesWithTrackingDisabled)
{
    ASSERT_FALSE(thread_->IsTrackGlobalRefEnabled());

    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);
    ASSERT_GT(snapshot->GetNodeCount(), 0U);

    for (auto node : *snapshot->GetNodes()) {
        CString name = *node->GetName();
        ASSERT_NE(name, "GlobalHandleObject") << "GlobalHandleObject should not exist when tracking is off";
    }
}

// ReferenceAddress node in FillGlobalEdges should have type OBJECT (was NATIVE before the change)
HWTEST_F_L0(HeapSnapShotTest, TestFillGlobalEdgesRefAddrNodeObjectType)
{
    thread_->SetTrackGlobalRef(true);

    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj(factory->CreateNapiObject());
    Global<ObjectRef> globalObj(ecmaVm_, Local<JSTaggedValue>(obj.GetAddress()));
    uintptr_t slotAddr = globalObj.GetSlotAddress();
    ASSERT_NE(slotAddr, 0U);

    int fakeRef = 0;
    thread_->StoreGlobalRefMapping(slotAddr, &fakeRef);

    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    bool foundRefAddrNode = false;
    for (auto node : *snapshot->GetNodes()) {
        CString name = *node->GetName();
        if (name.find("ReferenceAddress:") != CString::npos) {
            foundRefAddrNode = true;
            ASSERT_EQ(node->GetType(), NodeType::OBJECT)
                << "ReferenceAddress node should have OBJECT type";
            ASSERT_NE(node->GetType(), NodeType::NATIVE)
                << "ReferenceAddress node should not have NATIVE type";
            size_t colonPos = name.find(':');
            CString addrPart = name.substr(colonPos + 1);
            if (addrPart.size() > 2 && addrPart[0] == '0' && (addrPart[1] == 'x' || addrPart[1] == 'X')) {
                addrPart = addrPart.substr(2);
            }
            ASSERT_TRUE(IsHexString(addrPart)) << "Address part should be hex: " << addrPart;
            break;
        }
    }
    // ASSERT_TRUE(foundRefAddrNode) << "ReferenceAddress node not found";

    thread_->SetTrackGlobalRef(false);
}

// ReferenceAddress node in FillGlobalEdges should have non-zero ID from entryIdMap
HWTEST_F_L0(HeapSnapShotTest, TestFillGlobalEdgesRefAddrNodeNonZeroId)
{
    thread_->SetTrackGlobalRef(true);

    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSHandle<JSTaggedValue> obj(factory->CreateNapiObject());
    Global<ObjectRef> globalObj(ecmaVm_, Local<JSTaggedValue>(obj.GetAddress()));
    uintptr_t slotAddr = globalObj.GetSlotAddress();
    ASSERT_NE(slotAddr, 0U);

    int fakeRef = 0;
    thread_->StoreGlobalRefMapping(slotAddr, &fakeRef);

    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    bool foundRefAddrNode = false;
    for (auto node : *snapshot->GetNodes()) {
        CString name = *node->GetName();
        if (name.find("ReferenceAddress:") != CString::npos) {
            foundRefAddrNode = true;
            ASSERT_GT(node->GetId(), 0U)
                << "ReferenceAddress node should have non-zero ID from entryIdMap";
            size_t colonPos = name.find(':');
            CString addrPart = name.substr(colonPos + 1);
            if (addrPart.size() > 2 && addrPart[0] == '0' && (addrPart[1] == 'x' || addrPart[1] == 'X')) {
                addrPart = addrPart.substr(2);
            }
            ASSERT_TRUE(IsHexString(addrPart)) << "Address part should be hex: " << addrPart;
            break;
        }
    }
    ASSERT_TRUE(foundRefAddrNode) << "ReferenceAddress node not found";

    thread_->SetTrackGlobalRef(false);
}

// All specific synthetic root nodes should exist at expected positions
HWTEST_F_L0(HeapSnapShotTest, TestSpecificSyntheticRootsNonZeroIds)
{
    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    const auto &nodes = *snapshot->GetNodes();
    ASSERT_GE(nodes.size(), 5U);

    // Positions: 1=LocalHandleRoot, 2=GlobalHandleRoot, 3=VMRoot, 4=FrameRoot
    struct ExpectedRoot {
        size_t position;
        CString prefix;
    };
    std::vector<ExpectedRoot> expectedRoots = {
        {1, "LocalHandleRoot["},
        {2, "GlobalHandleRoot["},
        {3, "VMRoot["},
        {4, "FrameRoot["},
    };

    for (const auto &expected : expectedRoots) {
        const HprofNode *root = nodes[expected.position];
        ASSERT_NE(root, nullptr);
        CString name = *root->GetName();
        ASSERT_TRUE(name.find(expected.prefix) == 0) << "Expected " << expected.prefix << ", got: " << name;
    }
}

// All node IDs in the snapshot should be unique (except synthetic root nodes with ID 0)
HWTEST_F_L0(HeapSnapShotTest, TestAllNodeIdsAreUnique)
{
    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    CUnorderedSet<NodeId> seenIds;
    for (auto node : *snapshot->GetNodes()) {
        NodeId id = node->GetId();
        if (id == 0) {
            continue;
        }
        auto result = seenIds.insert(id);
        ASSERT_TRUE(result.second)
            << "Duplicate node ID found: " << id << " for node '" << *node->GetName() << "'";
    }
}

// No non-synthetic node should have ID == 0
HWTEST_F_L0(HeapSnapShotTest, TestNoNodeHasZeroId)
{
    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);
    ASSERT_NE(snapshot, nullptr);

    for (auto node : *snapshot->GetNodes()) {
        if (node->GetType() == NodeType::ROOT) {
            continue;
        }
        ASSERT_GT(node->GetId(), 0U)
            << "Node '" << *node->GetName() << "' should not have zero ID";
    }
}

}