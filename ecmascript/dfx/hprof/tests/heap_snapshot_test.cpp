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
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::ecmascript {
class HeapSnapShotFriendTest {
public:
    explicit HeapSnapShotFriendTest(const EcmaVM *vm, StringHashMap *stringTable, DumpSnapShotOption dumpOption,
                                    bool traceAllocation, EntryIdMap* entryIdMap)
        : heapSnapshot(vm, stringTable, dumpOption, traceAllocation, entryIdMap) {}

    Node *GeneratePrivateStringNodeTest(size_t size)
    {
        return heapSnapshot.GeneratePrivateStringNode(size);
    }

    void MoveNodeTest(uintptr_t address, TaggedObject *forwardAddress, size_t size)
    {
        return heapSnapshot.MoveNode(address, forwardAddress, size);
    }

    void InsertEntryTest(Node *node)
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
    Node *node = heapSnapShotTest.GeneratePrivateStringNodeTest(0);
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
    Node *node = Node::NewNode(heapSnapShotTest.GetChunkTest(), 0, 0,
        heapSnapShotTest.GenerateNodeNameTest(reinterpret_cast<TaggedObject *>(treeString)),
        heapSnapShotTest.GenerateNodeTypeTest(reinterpret_cast<TaggedObject *>(treeString)),
        0, 0, address);
    heapSnapShotTest.InsertEntryTest(node);
    heapSnapShotTest.MoveNodeTest(address, reinterpret_cast<TaggedObject *>(treeString), 0);
    HeapEntryMap &heapEntryMap = heapSnapShotTest.GetEntryMapTest();
    Node *movedNode = heapEntryMap.FindEntry(reinterpret_cast<JSTaggedType>(treeString));
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

HWTEST_F_L0(HeapSnapShotTest, TestHandleSyntheticRoots)
{
    const std::string abcFileName = HPROF_TEST_ABC_FILES_DIR "heap_snapshot.abc";
    std::string entryPoint = "heap_snapshot";
    bool result = JSNApi::Execute(ecmaVm_, abcFileName, entryPoint);
    ASSERT_TRUE(result);

    DumpSnapShotOption dumpOption;
    HeapProfilerFriendTest tester(ecmaVm_);
    HeapSnapshot *snapshot = tester.MakeHeapSnapshotTest(HeapProfiler::SampleType::ONE_SHOT, dumpOption);

    // LocalHandleSyntheticRoot and GlobalHandleSyntheticRoot are inserted at fixed positions 1 and 2
    const auto &nodes = *snapshot->GetNodes();
    ASSERT_GE(nodes.size(), 3U) << "Not enough nodes in snapshot";

    // Verify LocalHandleSyntheticRoot at position 1
    const Node *localHandleRoot = nodes[1];
    ASSERT_NE(localHandleRoot->GetName(), nullptr) << "LocalHandleSyntheticRoot name should not be null";
    ASSERT_EQ(localHandleRoot->GetType(), NodeType::HANDLE) << "LocalHandleSyntheticRoot type should be HANDLE";
    CString localName = *localHandleRoot->GetName();
    ASSERT_TRUE(localName.find("LocalHandleRoot[") == 0) << "Expected LocalHandleRoot, got: " << localName;

    // Verify GlobalHandleSyntheticRoot at position 2
    const Node *globalHandleRoot = nodes[2];
    ASSERT_NE(globalHandleRoot->GetName(), nullptr) << "GlobalHandleSyntheticRoot name should not be null";
    ASSERT_EQ(globalHandleRoot->GetType(), NodeType::HANDLE) << "GlobalHandleSyntheticRoot type should be HANDLE";
    CString globalName = *globalHandleRoot->GetName();
    ASSERT_TRUE(globalName.find("GlobalHandleRoot[") == 0) << "Expected GlobalHandleRoot, got: " << globalName;
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
    tester.GetEntryIdMapTest()->UpdateEntryIdMap(snapshot);
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
}