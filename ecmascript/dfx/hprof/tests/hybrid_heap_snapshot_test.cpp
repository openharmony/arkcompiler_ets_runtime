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

#include "ecmascript/dfx/hprof/hybrid/hybrid_heap_profiler.h"
#include "ecmascript/dfx/hprof/hybrid/hybrid_heap_snapshot.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/stream.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/tests/test_helper.h"

#include <unordered_map>

using namespace panda::ecmascript;

namespace panda::test {

// ============================================================================
// MockSTSVMInterface — implements all Hybrid Heapdump virtual methods
// ============================================================================

class MockSTSVMInterface : public arkplatform::STSVMInterface {
public:
    std::vector<arkplatform::NodeInfo> roots_;
    std::map<uint64_t, arkplatform::NodeInfo> nodeInfoMap_;
    std::map<uint64_t, std::vector<arkplatform::EdgeInfo>> edgeMap_;
    std::unordered_map<uint64_t, uint64_t> jsToEts_;
    std::unordered_map<uint64_t, uint64_t> etsToJs_;
    bool isAttached_ = true;
    bool xgcTriggered_ = false;
    bool etsGCed_ = false;
    bool etsSuspended_ = false;
    bool etsWasSuspended_ = false;

    void MarkFromObject(void *obj) override
    {
        (void)obj;
    }

    void OnVMAttach() override {}

    void OnVMDetach() override {}

    bool StartXGCBarrier(const NoWorkPred &func) override
    {
        (void)func;
        return true;
    }

    bool WaitForConcurrentMark(const NoWorkPred &func) override
    {
        (void)func;
        return true;
    }

    void RemarkStartBarrier() override {}

    bool WaitForRemark(const NoWorkPred &func) override
    {
        (void)func;
        return true;
    }

    void FinishXGCBarrier() override {}

    bool TriggerXGC() override
    {
        return true;
    }

    void NotifyWaiters() override {}

    bool GetStaticFrameInfo(const void *frame, arkplatform::HybridFrameInfo &frameInfo) override
    {
        (void)frame;
        (void)frameInfo;
        return false;
    }

    bool AttachCurrentThread() override
    {
        return true;
    }

    bool DetachCurrentThread() override
    {
        return true;
    }

    bool TriggerXGCAndWait() override
    {
        xgcTriggered_ = true;
        return true;
    }

    void EtsForceFullGC() override
    {
        etsGCed_ = true;
    }

    void SuspendEtsThreads() override
    {
        etsSuspended_ = true;
        etsWasSuspended_ = true;
    }

    void ResumeEtsThreads() override
    {
        etsSuspended_ = false;
    }

    std::vector<arkplatform::NodeInfo> GetEtsVMRoots() override
    {
        return roots_;
    }

    void GetEtsNodeEdges(uint64_t etsAddr, std::vector<arkplatform::EdgeInfo> &edges) override
    {
        auto it = edgeMap_.find(etsAddr);
        if (it != edgeMap_.end()) {
            edges = it->second;
        }
    }

    arkplatform::NodeInfo GetEtsNodeInfo(uint64_t etsAddr) override
    {
        auto it = nodeInfoMap_.find(etsAddr);
        if (it != nodeInfoMap_.end()) {
            return it->second;
        }
        return {"unknown", arkplatform::StaticNodeType::DEFAULT, 0, 0, etsAddr};
    }

    std::vector<arkplatform::NodeInfo> GetAllEtsObjects() override
    {
        std::vector<arkplatform::NodeInfo> all;
        for (auto &pair : nodeInfoMap_) {
            all.push_back(pair.second);
        }
        return all;
    }

    void IterateEtsObjects(const std::function<void(uint64_t)> &callback) override
    {
        for (auto &pair : nodeInfoMap_) {
            callback(pair.first);
        }
    }

    void GetXRefMaps(uintptr_t ecmaVM, std::unordered_map<uint64_t, uint64_t> &jsToEts,
                     std::unordered_map<uint64_t, uint64_t> &etsToJs) override
    {
        (void)ecmaVM;
        jsToEts = jsToEts_;
        etsToJs = etsToJs_;
    }

    bool IsCurrentThreadAttached() override
    {
        return isAttached_;
    }

    bool UnionStackIsEmpty(bool *isEmpty) override
    {
        return isEmpty;
    }

    bool ForEachFrameInUnionStack(const std::function<void(const void *frame, bool isStaticFrame)> &callback) override
    {
        (void)callback;
        return true;
    }

    void AddStaticNode(uint64_t addr, const std::string &name,
                       arkplatform::StaticNodeType type, size_t size, size_t nativeSize = 0)
    {
        nodeInfoMap_[addr] = {name, type, size, nativeSize, addr};
    }

    void AddStaticEdge(uint64_t from, uint64_t to, const std::string &name,
                       arkplatform::StaticEdgeType type)
    {
        edgeMap_[from].push_back({type, from, to, name, 0});
    }

    void AddStaticEdge(uint64_t from, uint64_t to, uint32_t index,
                       arkplatform::StaticEdgeType type)
    {
        edgeMap_[from].push_back({type, from, to, "", index});
    }

    void SetupCycleEdges()
    {
        for (auto &[addr, info] : nodeInfoMap_) {
            for (auto &[otherAddr, otherInfo] : nodeInfoMap_) {
                if (addr != otherAddr) {
                    AddStaticEdge(addr, otherAddr, "ref", arkplatform::StaticEdgeType::PROPERTY);
                }
            }
        }
    }
};

// ============================================================================
// Friend helper — in panda::ecmascript to match friend declaration
// ============================================================================

}  // close panda::test temporarily

namespace panda::ecmascript {
class HybridHeapProfilerTestHelper {
public:
    static void SetSTSInterface(HybridHeapProfiler &profiler, arkplatform::STSVMInterface *sts)
    {
        profiler.stsInterface_ = sts;
    }

    static bool BuildAndSerializeSnapshot(HybridHeapProfiler &profiler, EcmaVM *vm,
                                          Stream *stream, const DumpSnapShotOption &dumpOption)
    {
        return profiler.BuildAndSerializeSnapshot(vm, stream, dumpOption);
    }

    static void ForceFullGC(HybridHeapProfiler &profiler, EcmaVM *vm,
                            const DumpSnapShotOption &dumpOption)
    {
        profiler.ForceFullGC(vm, dumpOption);
    }

    static void JSForceFullGC(const EcmaVM *vm)
    {
        HybridHeapProfiler::JSForceFullGC(vm);
    }

    static void WaitForJSGCFinish(const EcmaVM *vm, const DumpSnapShotOption &dumpOption)
    {
        HybridHeapProfiler::WaitForJSGCFinish(vm, dumpOption);
    }

    static EntryIdMap &GetEntryIdMapRef(HybridHeapProfiler &profiler)
    {
        return profiler.entryIdMap_;
    }

    static void TestJSSuspendAllScopeDisabled(EcmaVM *vm)
    {
        HybridHeapProfiler::JSSuspendAllScope scope(vm, false);
    }

    static void TestJSSuspendAllScopeEnabled(EcmaVM *vm)
    {
        HybridHeapProfiler::JSSuspendAllScope scope(vm, true);
    }

    static void TestEtsSuspendAllScopeEnabled(arkplatform::STSVMInterface *sts)
    {
        HybridHeapProfiler::EtsSuspendAllScope scope(sts, true);
    }

    static void TestEtsSuspendAllScopeDisabled(arkplatform::STSVMInterface *sts)
    {
        HybridHeapProfiler::EtsSuspendAllScope scope(sts, false);
    }

    static bool SetAppFreezeFilter(HybridHeapProfiler &profiler)
    {
        return profiler.SetAppFreezeFilter();
    }

    static void FillIdMap(HybridHeapProfiler &profiler, EcmaVM *vm,
                          const DumpSnapShotOption &dumpOption)
    {
        profiler.FillIdMap(vm, dumpOption);
    }
};
}  // namespace panda::ecmascript

namespace panda::test {

// ============================================================================
// Test Stream
// ============================================================================

class TestHybridStream : public Stream {
public:
    TestHybridStream() = default;
    ~TestHybridStream() override = default;

    void EndOfStream() override {}

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
        (void)data;
        (void)size;
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

    void UpdateHeapStats(HeapStat* updateData, int32_t count) override
    {
        (void)updateData;
        (void)count;
    }

    void UpdateLastSeenObjectId(int32_t lastSeenObjectId, int64_t timeStampUs) override
    {
        (void)lastSeenObjectId;
        (void)timeStampUs;
    }

    std::string GetOutput() const
    {
        return output;
    }

    void Clear()
    {
        output.clear();
    }

private:
    std::string output;
};

// ============================================================================
// Helper to set up mock STS with a small cycle of static nodes for testing
// ============================================================================

static void SetupMockCycle(MockSTSVMInterface &mock, const std::vector<uint64_t> &addrs,
                           bool addRoots = true)
{
    constexpr size_t mockObjectSize = 32;
    for (auto addr : addrs) {
        mock.AddStaticNode(addr, "Obj" + std::to_string(addr), arkplatform::StaticNodeType::OBJECT, mockObjectSize);
    }
    if (addRoots && !addrs.empty()) {
        mock.roots_.push_back(mock.nodeInfoMap_[addrs[0]]);
    }
    mock.SetupCycleEdges();
}

static HprofNode *FindNodeByName(const HybridHeapSnapshot &snapshot, const CString &name)
{
    for (auto node : *snapshot.GetNodes()) {
        if (node->GetName() != nullptr && *node->GetName() == name) {
            return node;
        }
    }
    return nullptr;
}

static Edge *FindEdgeByName(const HybridHeapSnapshot &snapshot, const CString &name)
{
    for (auto edge : *snapshot.GetEdges()) {
        if (edge->GetType() == EdgeType::ELEMENT) {
            continue;
        }
        if (edge->GetName() != nullptr && *edge->GetName() == name) {
            return edge;
        }
    }
    return nullptr;
}

static size_t CountEdgesByType(const HybridHeapSnapshot &snapshot, EdgeType type)
{
    size_t count = 0;
    for (auto edge : *snapshot.GetEdges()) {
        if (edge->GetType() == type) {
            count++;
        }
    }
    return count;
}

static void AssertOutputContains(const std::string &output, const std::string &field)
{
    ASSERT_NE(output.find(field), std::string::npos) << "serialized snapshot should contain " << field;
}

// ============================================================================
// Fixtures
// ============================================================================

class PureFunctionTest : public testing::Test {};

class HybridHeapSnapshotTest : public testing::Test {
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

// ============================================================================
// Pure function tests (no VM)
// ============================================================================

HWTEST_F_L0(PureFunctionTest, MapStaticNodeAndEdgeTypes)
{
    ASSERT_EQ(HybridHeapSnapshot::MapStaticNodeType(arkplatform::StaticNodeType::ARRAY),
              NodeType::ARRAY);
    ASSERT_EQ(HybridHeapSnapshot::MapStaticNodeType(arkplatform::StaticNodeType::STRING),
              NodeType::STRING);
    ASSERT_EQ(HybridHeapSnapshot::MapStaticNodeType(arkplatform::StaticNodeType::OBJECT),
              NodeType::OBJECT);
    ASSERT_EQ(HybridHeapSnapshot::MapStaticNodeType(arkplatform::StaticNodeType::CLASS),
              NodeType::CLASS);
    ASSERT_EQ(HybridHeapSnapshot::MapStaticNodeType(static_cast<arkplatform::StaticNodeType>(99)),
              NodeType::DEFAULT);

    ASSERT_EQ(HybridHeapSnapshot::MapStaticEdgeType(arkplatform::StaticEdgeType::ELEMENT),
              EdgeType::ELEMENT);
    ASSERT_EQ(HybridHeapSnapshot::MapStaticEdgeType(arkplatform::StaticEdgeType::PROPERTY),
              EdgeType::PROPERTY);
    ASSERT_EQ(HybridHeapSnapshot::MapStaticEdgeType(arkplatform::StaticEdgeType::WEAK),
              EdgeType::WEAK);
    ASSERT_EQ(HybridHeapSnapshot::MapStaticEdgeType(static_cast<arkplatform::StaticEdgeType>(99)),
              EdgeType::DEFAULT);
}

HWTEST_F_L0(PureFunctionTest, NewEnumAndStructValues)
{
    ASSERT_EQ(static_cast<uint8_t>(LanguageEnv::DYNAMIC), 0);
    ASSERT_EQ(static_cast<uint8_t>(LanguageEnv::STATIC), 1);
    ASSERT_EQ(static_cast<uint8_t>(LanguageEnv::HYBRID), 2);

    DumpSnapShotOption opt;
    ASSERT_FALSE(opt.dumpDynamicHeap);
    ASSERT_FALSE(opt.dumpStaticHeap);
    ASSERT_EQ(opt.languageEnv, LanguageEnv::DYNAMIC);

    ASSERT_NE(NodeType::CLASS, NodeType::OBJECT);
    ASSERT_NE(EdgeType::XREF, EdgeType::PROPERTY);
}

// ============================================================================
// Profiler basic tests
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, ProfilerBasicBehavior)
{
    HybridHeapProfiler profiler(ecmaVm_);
    ASSERT_FALSE(profiler.HasSTSInterface());
    ASSERT_EQ(profiler.GetSTSInterface(), nullptr);
    ASSERT_NE(profiler.GetEntryIdMap(), nullptr);

    auto *instance = HybridHeapProfiler::GetInstance();
    // In hybrid mode, GetOrNewHybridHeapProfiler() creates and returns a profiler instance.
    ASSERT_NE(instance, nullptr);

    TestHybridStream stream;
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;
    ASSERT_FALSE(profiler.Dump(ecmaVm_, &stream, dumpOption));
    ASSERT_TRUE(HybridHeapProfilerTestHelper::SetAppFreezeFilter(profiler));
}

HWTEST_F_L0(HybridHeapSnapshotTest, EmptyModeSnapshot)
{
    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, nullptr, &entryIdMap, &stringTable,
                                false, false, false, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_FALSE(snapshot.BuildHybridSnapshot());
    ASSERT_EQ(snapshot.GetNodeCount(), 1U);
    const auto &nodes = *snapshot.GetNodes();
    ASSERT_EQ(nodes[0]->GetType(), NodeType::SYNTHETIC);
    ASSERT_NE(nodes[0]->GetName(), nullptr);
    ASSERT_EQ(*nodes[0]->GetName(), CString("Synthetic"));
}

// ============================================================================
// GC / Suspend / WaitFor tests
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, ForceFullGC_RespectsDumpOptions)
{
    HybridHeapProfiler profiler(ecmaVm_);
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    HybridHeapProfilerTestHelper::SetSTSInterface(profiler, mockSts.get());

    DumpSnapShotOption dumpOption;
    dumpOption.isFullGC = false;
    HybridHeapProfilerTestHelper::ForceFullGC(profiler, ecmaVm_, dumpOption);
    ASSERT_FALSE(mockSts->xgcTriggered_);
    ASSERT_FALSE(mockSts->etsGCed_);

    dumpOption.isFullGC = true;
    dumpOption.dumpDynamicHeap = false;
    dumpOption.dumpStaticHeap = true;
    HybridHeapProfilerTestHelper::ForceFullGC(profiler, ecmaVm_, dumpOption);
    ASSERT_TRUE(mockSts->etsGCed_);
    ASSERT_FALSE(mockSts->xgcTriggered_);

    mockSts->xgcTriggered_ = false;
    mockSts->etsGCed_ = false;
    dumpOption.dumpDynamicHeap = true;
    dumpOption.dumpStaticHeap = true;
    HybridHeapProfilerTestHelper::ForceFullGC(profiler, ecmaVm_, dumpOption);
    ASSERT_TRUE(mockSts->xgcTriggered_);
    ASSERT_TRUE(mockSts->etsGCed_);
}

HWTEST_F_L0(HybridHeapSnapshotTest, SuspendScopes_RespectEnableFlag)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();

    HybridHeapProfilerTestHelper::TestEtsSuspendAllScopeDisabled(mockSts.get());
    ASSERT_FALSE(mockSts->etsWasSuspended_);
    ASSERT_FALSE(mockSts->etsSuspended_);

    HybridHeapProfilerTestHelper::TestEtsSuspendAllScopeEnabled(mockSts.get());
    ASSERT_TRUE(mockSts->etsWasSuspended_);
    ASSERT_FALSE(mockSts->etsSuspended_);

    HybridHeapProfilerTestHelper::TestJSSuspendAllScopeDisabled(ecmaVm_);
    ASSERT_TRUE(ecmaVm_->IsInitialized());
}

HWTEST_F_L0(HybridHeapSnapshotTest, FillIdMap_StaticAndNoHeapModes)
{
    HybridHeapProfiler profiler(ecmaVm_);
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    SetupMockCycle(*mockSts, {0x1000, 0x2000, 0x3000});
    HybridHeapProfilerTestHelper::SetSTSInterface(profiler, mockSts.get());

    // First build a snapshot to populate entryIdMap with static node IDs
    TestHybridStream stream;
    DumpSnapShotOption dumpOption;
    dumpOption.dumpDynamicHeap = false;
    dumpOption.dumpStaticHeap = true;
    dumpOption.dumpFormat = DumpFormat::JSON;
    ASSERT_TRUE(HybridHeapProfilerTestHelper::BuildAndSerializeSnapshot(
        profiler, ecmaVm_, &stream, dumpOption));

    // FillIdMap with static-only should iterate via IterateEtsObjects
    HybridHeapProfilerTestHelper::FillIdMap(profiler, ecmaVm_, dumpOption);
    auto [found, id] = profiler.GetEntryIdMap()->FindId(0x1000);
    ASSERT_TRUE(found);
    ASSERT_GT(id, 0U);
    ASSERT_EQ(profiler.GetEntryIdMap()->GetIdCount(), mockSts->nodeInfoMap_.size());

    dumpOption.dumpStaticHeap = false;
    dumpOption.dumpDynamicHeap = false;
    HybridHeapProfilerTestHelper::FillIdMap(profiler, ecmaVm_, dumpOption);
    ASSERT_EQ(profiler.GetEntryIdMap()->GetIdCount(), 0U);
}

HWTEST_F_L0(HybridHeapSnapshotTest, DumpAsync_FillIdMapBothHeaps)
{
    HybridHeapProfiler profiler(ecmaVm_);
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "AsyncStaticObj", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "AsyncChild", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->SetupCycleEdges();
    HybridHeapProfilerTestHelper::SetSTSInterface(profiler, mockSts.get());

    TestHybridStream stream;
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;
    dumpOption.isFullGC = false;
    dumpOption.isSync = false;
    dumpOption.isBeforeFill = true;
    // Dump sets dumpDynamicHeap=true (vm!=null) and dumpStaticHeap=true (mock IsAttached=true),
    // then calls FillIdMap with both heaps BEFORE the fork. FillIdMap covers dynamic + static.
    ASSERT_TRUE(profiler.Dump(ecmaVm_, &stream, dumpOption));

    // Verify FillIdMap populated the entryIdMap
    auto [found, id] = profiler.GetEntryIdMap()->FindId(0x1000);
    ASSERT_TRUE(found) << "FillIdMap should have populated static node 0x1000";
    ASSERT_GT(id, 0U);
    ASSERT_TRUE(profiler.GetEntryIdMap()->FindId(0x2000).first);

    // Wait for forked child to finish (it builds snapshot and calls _exit(0))
    usleep(200000);  // 200ms
}

// ============================================================================
// BuildAndSerialize tests
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, BuildAndSerialize_DynamicOnly)
{
    HybridHeapProfiler profiler(ecmaVm_);
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    HybridHeapProfilerTestHelper::SetSTSInterface(profiler, mockSts.get());
    TestHybridStream stream;
    DumpSnapShotOption dumpOption;
    dumpOption.dumpDynamicHeap = true;
    dumpOption.dumpStaticHeap = false;
    dumpOption.dumpFormat = DumpFormat::JSON;
    ASSERT_TRUE(HybridHeapProfilerTestHelper::BuildAndSerializeSnapshot(
        profiler, ecmaVm_, &stream, dumpOption));
    ASSERT_GT(profiler.GetEntryIdMap()->GetIdCount(), 0U);
    AssertOutputContains(stream.GetOutput(), "\"nodes\"");
}

HWTEST_F_L0(HybridHeapSnapshotTest, BuildAndSerialize_StaticOnly)
{
    HybridHeapProfiler profiler(ecmaVm_);
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "StaticRoot", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "Child1", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->AddStaticNode(0x3000, "Child2", arkplatform::StaticNodeType::OBJECT, 16);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->SetupCycleEdges();
    HybridHeapProfilerTestHelper::SetSTSInterface(profiler, mockSts.get());
    TestHybridStream stream;
    DumpSnapShotOption dumpOption;
    dumpOption.dumpDynamicHeap = false;
    dumpOption.dumpStaticHeap = true;
    dumpOption.dumpFormat = DumpFormat::JSON;
    ASSERT_TRUE(HybridHeapProfilerTestHelper::BuildAndSerializeSnapshot(
        profiler, ecmaVm_, &stream, dumpOption));
    std::string output = stream.GetOutput();
    AssertOutputContains(output, "\"StaticRoot\"");
    AssertOutputContains(output, "\"Child1\"");
    AssertOutputContains(output, "\"Child2\"");
    ASSERT_TRUE(profiler.GetEntryIdMap()->FindId(0x1000).first);
    ASSERT_TRUE(profiler.GetEntryIdMap()->FindId(0x2000).first);
    ASSERT_TRUE(profiler.GetEntryIdMap()->FindId(0x3000).first);
}

HWTEST_F_L0(HybridHeapSnapshotTest, BuildAndSerialize_WithSync)
{
    HybridHeapProfiler profiler(ecmaVm_);
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "SyncObj", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "H1", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->AddStaticNode(0x3000, "H2", arkplatform::StaticNodeType::OBJECT, 16);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->SetupCycleEdges();
    HybridHeapProfilerTestHelper::SetSTSInterface(profiler, mockSts.get());
    TestHybridStream stream;
    DumpSnapShotOption dumpOption;
    dumpOption.dumpDynamicHeap = false;
    dumpOption.dumpStaticHeap = true;
    dumpOption.dumpFormat = DumpFormat::JSON;
    dumpOption.isSync = true;
    ASSERT_TRUE(HybridHeapProfilerTestHelper::BuildAndSerializeSnapshot(
        profiler, ecmaVm_, &stream, dumpOption));
    auto &idMap = HybridHeapProfilerTestHelper::GetEntryIdMapRef(profiler);
    auto [found, id] = idMap.FindId(0x1000);
    ASSERT_TRUE(found);
    ASSERT_GT(id, 0U);
    AssertOutputContains(stream.GetOutput(), "\"SyncObj\"");
}

// ============================================================================
// Dump flow correctness tests (verify snapshot CONTENT)
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, DumpDynamic_VerifyJSObjects)
{
    // The VM initialization already creates many built-in JS objects.
    // Dump the dynamic heap and verify it contains real JS objects.
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, false, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    bool hasObject = false;
    bool hasString = false;
    for (auto node : *snapshot.GetNodes()) {
        auto type = node->GetType();
        if (type == NodeType::OBJECT) {
            hasObject = true;
        }
        if (type == NodeType::STRING) {
            hasString = true;
        }
    }
    ASSERT_TRUE(hasObject) << "Snapshot should contain OBJECT nodes from built-in objects";
    ASSERT_TRUE(hasString) << "Snapshot should contain STRING nodes from built-in strings";
    ASSERT_GT(snapshot.GetNodeCount(), 5U);
    ASSERT_GT(snapshot.GetTotalNodeSize(), 0U);
}

HWTEST_F_L0(HybridHeapSnapshotTest, DumpFullFlow_VerifyOutput)
{
    HybridHeapProfiler profiler(ecmaVm_);
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "StaticRoot", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "StaticChild", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->AddStaticEdge(0x1000, 0x2000, "field", arkplatform::StaticEdgeType::PROPERTY);
    mockSts->AddStaticEdge(0x2000, 0x1000, "back", arkplatform::StaticEdgeType::PROPERTY);
    HybridHeapProfilerTestHelper::SetSTSInterface(profiler, mockSts.get());

    TestHybridStream stream;
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::JSON;
    dumpOption.isFullGC = false;
    dumpOption.isSync = true;
    ASSERT_TRUE(profiler.Dump(ecmaVm_, &stream, dumpOption));

    std::string output = stream.GetOutput();
    ASSERT_FALSE(output.empty());
    AssertOutputContains(output, "\"nodes\"");
    AssertOutputContains(output, "\"edges\"");
    AssertOutputContains(output, "\"StaticRoot\"");
    AssertOutputContains(output, "\"StaticChild\"");
    AssertOutputContains(output, "\"field\"");
}

HWTEST_F_L0(HybridHeapSnapshotTest, DumpHybrid_VerifyBothHeaps)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x5000, "ETSObj", arkplatform::StaticNodeType::CLASS, 128);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x5000]);

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    bool foundStatic = false;
    size_t dynamicCount = 0;
    for (auto node : *snapshot.GetNodes()) {
        if (node->GetName() != nullptr && *node->GetName() == "ETSObj") {
            foundStatic = true;
            ASSERT_EQ(node->GetType(), NodeType::CLASS);
            ASSERT_EQ(node->GetSelfSize(), 128U);
            ASSERT_FALSE(node->IsDynamic());
        }
        if (node->IsDynamic()) {
            dynamicCount++;
        }
    }
    ASSERT_TRUE(foundStatic) << "Snapshot should contain static ETSObj node";
    ASSERT_GT(dynamicCount, 0) << "Snapshot should contain dynamic JS nodes";
}

HWTEST_F_L0(HybridHeapSnapshotTest, DumpWithXRef_VerifyXRefEdges)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "ETSRoot", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "ETSChild", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->AddStaticEdge(0x1000, 0x2000, "ref", arkplatform::StaticEdgeType::PROPERTY);
    mockSts->AddStaticEdge(0x2000, 0x1000, "back", arkplatform::StaticEdgeType::PROPERTY);

    // Use a real JS object address (the global object) for the XRef mapping.
    // Only populate jsToEts_ (dynamic→static). etsToJs_ is left empty because
    // DumpXRefStaticToDynamic calls GenerateNode on the mapped JS address,
    // which crashes on fake addresses.
    JSTaggedValue globalObj = ecmaVm_->GetGlobalEnv()->GetGlobalObject();
    ASSERT_TRUE(globalObj.IsHeapObject());
    uint64_t jsAddr = static_cast<uint64_t>(globalObj.GetRawData());
    mockSts->jsToEts_[jsAddr] = 0x1000;

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    ASSERT_GT(CountEdgesByType(snapshot, EdgeType::XREF), 0U)
        << "Snapshot should contain XREF edges when XRef maps are configured";
}

HWTEST_F_L0(HybridHeapSnapshotTest, DumpWithXRef_StaticToDynamic)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "ETSObj", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);

    // Use the global object for the XRef mapping (static→dynamic)
    JSTaggedValue globalObj = ecmaVm_->GetGlobalEnv()->GetGlobalObject();
    ASSERT_TRUE(globalObj.IsHeapObject());
    uint64_t jsAddr = static_cast<uint64_t>(globalObj.GetRawData());
    mockSts->etsToJs_[0x1000] = jsAddr;

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    ASSERT_GT(CountEdgesByType(snapshot, EdgeType::XREF), 0U)
        << "Static to Dynamic XRef edges should be created when etsToJs is configured";
}

HWTEST_F_L0(HybridHeapSnapshotTest, DumpWithXRef_BidirectionalMapping)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "ETSRoot", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);

    // Use global object for bidirectional XRef mapping
    // jsToEts: global object references static ETS object
    // etsToJs: static ETS object references the same global object
    JSTaggedValue globalObj = ecmaVm_->GetGlobalEnv()->GetGlobalObject();

    ASSERT_TRUE(globalObj.IsHeapObject());
    uint64_t jsAddr = static_cast<uint64_t>(globalObj.GetRawData());
    mockSts->jsToEts_[jsAddr] = 0x1000;
    mockSts->etsToJs_[0x1000] = jsAddr;

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    ASSERT_GE(CountEdgesByType(snapshot, EdgeType::XREF), 2U)
        << "Bidirectional XRef edges should be created when both maps are configured";
}

// ============================================================================
// Static-only snapshot tests
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, StaticOnly_EmptyAndRootedNodes)
{
    {
        auto mockSts = std::make_unique<MockSTSVMInterface>();
        EntryIdMap entryIdMap;
        StringHashMap stringTable(ecmaVm_);
        HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                    false, false, true, ecmaVm_->GetNativeAreaAllocator());
        ASSERT_FALSE(snapshot.BuildHybridSnapshot());
        ASSERT_EQ(snapshot.GetNodeCount(), 1U);
    }

    {
        auto mockSts = std::make_unique<MockSTSVMInterface>();
        SetupMockCycle(*mockSts, {0x1000, 0x2000, 0x3000});
        EntryIdMap entryIdMap;
        StringHashMap stringTable(ecmaVm_);
        HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                    false, false, true, ecmaVm_->GetNativeAreaAllocator());
        ASSERT_TRUE(snapshot.BuildHybridSnapshot());
        ASSERT_GT(snapshot.GetNodeCount(), 1U);
        ASSERT_NE(FindNodeByName(snapshot, "Obj4096"), nullptr);
        ASSERT_NE(FindNodeByName(snapshot, "Obj8192"), nullptr);
        ASSERT_NE(FindNodeByName(snapshot, "Obj12288"), nullptr);
        ASSERT_EQ(CountEdgesByType(snapshot, EdgeType::PROPERTY), 6U);
    }
}

HWTEST_F_L0(HybridHeapSnapshotTest, StaticOnly_NodeProperties)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "StaticRoot1", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "StaticRoot2", arkplatform::StaticNodeType::STRING, 32);
    mockSts->AddStaticNode(0x3000, "StaticRoot3", arkplatform::StaticNodeType::OBJECT, 16);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x2000]);
    mockSts->SetupCycleEdges();

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, false, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    HprofNode *obj1 = FindNodeByName(snapshot, "StaticRoot1");
    ASSERT_NE(obj1, nullptr);
    ASSERT_EQ(obj1->GetType(), NodeType::OBJECT);
    ASSERT_EQ(obj1->GetSelfSize(), 64U);

    HprofNode *obj2 = FindNodeByName(snapshot, "StaticRoot2");
    ASSERT_NE(obj2, nullptr);
    ASSERT_EQ(obj2->GetType(), NodeType::STRING);
    ASSERT_EQ(obj2->GetSelfSize(), 32U);
}

HWTEST_F_L0(HybridHeapSnapshotTest, StaticOnly_NodeDetails)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x3000, "MyArray", arkplatform::StaticNodeType::ARRAY, 128, 16);
    mockSts->AddStaticNode(0x4000, "Elem1", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->AddStaticNode(0x5000, "Elem2", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x3000]);
    mockSts->AddStaticEdge(0x3000, 0x4000, 0U, arkplatform::StaticEdgeType::ELEMENT);
    mockSts->AddStaticEdge(0x3000, 0x5000, 1U, arkplatform::StaticEdgeType::ELEMENT);
    mockSts->AddStaticEdge(0x4000, 0x5000, "ref", arkplatform::StaticEdgeType::PROPERTY);
    mockSts->AddStaticEdge(0x5000, 0x4000, "ref", arkplatform::StaticEdgeType::PROPERTY);
    mockSts->AddStaticEdge(0x4000, 0x3000, "back", arkplatform::StaticEdgeType::PROPERTY);

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, false, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    HprofNode *array = FindNodeByName(snapshot, "MyArray");
    ASSERT_NE(array, nullptr);
    ASSERT_EQ(array->GetType(), NodeType::ARRAY);
    ASSERT_EQ(array->GetSelfSize(), 128U);
    ASSERT_EQ(array->GetNativeSize(), 16U);
    ASSERT_EQ(array->GetAddress(), 0x3000U);
    ASSERT_FALSE(array->IsDynamic());
}

HWTEST_F_L0(HybridHeapSnapshotTest, StaticOnly_PropertyEdges)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "Parent", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "Child", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->AddStaticNode(0x6000, "Child2", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->AddStaticEdge(0x1000, 0x2000, "field1", arkplatform::StaticEdgeType::PROPERTY);
    mockSts->AddStaticEdge(0x1000, 0x6000, "field2", arkplatform::StaticEdgeType::PROPERTY);
    mockSts->SetupCycleEdges();

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, false, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    Edge *field1 = FindEdgeByName(snapshot, "field1");
    ASSERT_NE(field1, nullptr);
    ASSERT_EQ(field1->GetType(), EdgeType::PROPERTY);
    ASSERT_NE(field1->GetFrom(), nullptr);
    ASSERT_NE(field1->GetTo(), nullptr);
    ASSERT_EQ(field1->GetFrom()->GetAddress(), 0x1000U);
    ASSERT_EQ(field1->GetTo()->GetAddress(), 0x2000U);
}

HWTEST_F_L0(HybridHeapSnapshotTest, StaticOnly_ElementEdges)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "ArrayObj", arkplatform::StaticNodeType::ARRAY, 128);
    mockSts->AddStaticNode(0x2000, "Element0", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->AddStaticNode(0x3000, "Element1", arkplatform::StaticNodeType::STRING, 16);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->AddStaticEdge(0x1000, 0x2000, 0U, arkplatform::StaticEdgeType::ELEMENT);
    mockSts->AddStaticEdge(0x1000, 0x3000, 1U, arkplatform::StaticEdgeType::ELEMENT);
    mockSts->SetupCycleEdges();

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, false, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    ASSERT_EQ(CountEdgesByType(snapshot, EdgeType::ELEMENT), 2U);
}

// ============================================================================
// Dynamic-only snapshot tests
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, DynamicOnly_BuildsSubRoots)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, false, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());
    ASSERT_GT(snapshot.GetNodeCount(), 1U);

    const auto &nodes = *snapshot.GetNodes();
    ASSERT_GE(nodes.size(), 5U);

    ASSERT_EQ(nodes[0]->GetType(), NodeType::SYNTHETIC);
    ASSERT_NE(nodes[1]->GetName(), nullptr);
    ASSERT_NE(nodes[1]->GetName()->find("LocalHandleRoot"), CString::npos);
    ASSERT_NE(nodes[2]->GetName(), nullptr);
    ASSERT_NE(nodes[2]->GetName()->find("GlobalHandleRoot"), CString::npos);
    ASSERT_NE(nodes[3]->GetName(), nullptr);
    ASSERT_NE(nodes[3]->GetName()->find("VMRoot"), CString::npos);
    ASSERT_NE(nodes[4]->GetName(), nullptr);
    ASSERT_NE(nodes[4]->GetName()->find("FrameRoot"), CString::npos);
}

// ============================================================================
// Hybrid mode tests
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, Hybrid_FullBuild)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "StaticObj", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());
    ASSERT_GT(snapshot.GetNodeCount(), 5U);
    HprofNode *staticObj = FindNodeByName(snapshot, "StaticObj");
    ASSERT_NE(staticObj, nullptr);
    ASSERT_FALSE(staticObj->IsDynamic());
    ASSERT_EQ(staticObj->GetAddress(), 0x1000U);
}

HWTEST_F_L0(HybridHeapSnapshotTest, Hybrid_StaticNodesPresent)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x5000, "HybridStaticObj", arkplatform::StaticNodeType::CLASS, 256);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x5000]);

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    HprofNode *staticObj = FindNodeByName(snapshot, "HybridStaticObj");
    ASSERT_NE(staticObj, nullptr);
    ASSERT_EQ(staticObj->GetType(), NodeType::CLASS);
    ASSERT_EQ(staticObj->GetSelfSize(), 256U);
    ASSERT_FALSE(staticObj->IsDynamic());
}

HWTEST_F_L0(HybridHeapSnapshotTest, Hybrid_NodeAndEdgeCounts)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "Obj1", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "Obj2", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x2000]);
    mockSts->AddStaticEdge(0x1000, 0x2000, "ref", arkplatform::StaticEdgeType::PROPERTY);

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());
    ASSERT_GT(snapshot.GetNodeCount(), 0U);
    ASSERT_GT(snapshot.GetEdgeCount(), 0U);
    ASSERT_GT(snapshot.GetTotalNodeSize(), 0U);
    Edge *ref = FindEdgeByName(snapshot, "ref");
    ASSERT_NE(ref, nullptr);
    ASSERT_EQ(ref->GetType(), EdgeType::PROPERTY);
    ASSERT_EQ(ref->GetFrom()->GetAddress(), 0x1000U);
    ASSERT_EQ(ref->GetTo()->GetAddress(), 0x2000U);
}

HWTEST_F_L0(HybridHeapSnapshotTest, Hybrid_SyntheticRootEdges)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    const auto &nodes = *snapshot.GetNodes();
    ASSERT_GE(nodes.size(), 1U);
    HprofNode *syntheticRoot = nodes[0];
    ASSERT_EQ(syntheticRoot->GetType(), NodeType::SYNTHETIC);
    ASSERT_GT(syntheticRoot->GetEdgeCount(), 0U);
}

HWTEST_F_L0(HybridHeapSnapshotTest, Hybrid_NoXRefWhenMapsEmpty)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "Obj", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    SetupMockCycle(*mockSts, {0x7000, 0x8000}, false);

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());
    for (auto edge : *snapshot.GetEdges()) {
        ASSERT_NE(edge->GetType(), EdgeType::XREF);
    }
}

// ============================================================================
// EntryIdMap tests
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, EntryIdMap_StaticMappingPersistsAcrossSnapshots)
{
    EntryIdMap entryIdMap;
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    SetupMockCycle(*mockSts, {0x1000, 0x2000, 0x3000});

    {
        StringHashMap stringTable(ecmaVm_);
        HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                    false, false, true, ecmaVm_->GetNativeAreaAllocator());
        ASSERT_TRUE(snapshot.BuildHybridSnapshot());
    }

    auto [found1, id1] = entryIdMap.FindId(0x1000);
    ASSERT_TRUE(found1);
    ASSERT_GT(id1, 0U);

    {
        StringHashMap stringTable(ecmaVm_);
        HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                    false, false, true, ecmaVm_->GetNativeAreaAllocator());
        ASSERT_TRUE(snapshot.BuildHybridSnapshot());
    }
    auto [found2, id2] = entryIdMap.FindId(0x1000);
    ASSERT_TRUE(found2);
    ASSERT_EQ(id1, id2);
}

HWTEST_F_L0(HybridHeapSnapshotTest, EntryIdMap_UniqueNewID)
{
    EntryIdMap entryIdMap;
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "First", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "Second", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->AddStaticNode(0x3000, "Helper", arkplatform::StaticNodeType::OBJECT, 16);
    mockSts->SetupCycleEdges();

    {
        mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
        StringHashMap stringTable(ecmaVm_);
        HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                    false, false, true, ecmaVm_->GetNativeAreaAllocator());
        ASSERT_TRUE(snapshot.BuildHybridSnapshot());
    }
    auto [found1, id1] = entryIdMap.FindId(0x1000);
    ASSERT_TRUE(found1);

    {
        mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x2000]);
        StringHashMap stringTable(ecmaVm_);
        HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                    false, false, true, ecmaVm_->GetNativeAreaAllocator());
        ASSERT_TRUE(snapshot.BuildHybridSnapshot());
    }
    auto [found2, id2] = entryIdMap.FindId(0x2000);
    ASSERT_TRUE(found2);
    ASSERT_NE(id1, id2);
}

HWTEST_F_L0(HybridHeapSnapshotTest, EntryIdMap_ObjectMovement)
{
    EntryIdMap entryIdMap;
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    SetupMockCycle(*mockSts, {0x1000, 0x2000, 0x3000});

    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, false, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    auto [found1, id1] = entryIdMap.FindId(0x1000);
    ASSERT_TRUE(found1) << "Object should be found at original address";
    ASSERT_GT(id1, 0U);

    // Simulate object movement (0x1000 -> 0x4000) due to GC
    bool moved = entryIdMap.Move(0x1000, 0x4000);
    ASSERT_TRUE(moved) << "Move should succeed for existing entry";

    auto [found2, id2] = entryIdMap.FindId(0x4000);
    ASSERT_TRUE(found2) << "Object should be found at new address after Move";
    ASSERT_EQ(id1, id2) << "ID should persist after address change";

    [[maybe_unused]] auto [found3, id3] = entryIdMap.FindId(0x1000);
    ASSERT_FALSE(found3) << "Old address should not exist after Move";
}

HWTEST_F_L0(HybridHeapSnapshotTest, EntryIdMap_EraseId)
{
    EntryIdMap entryIdMap;
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    SetupMockCycle(*mockSts, {0x1000, 0x2000, 0x3000});

    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, false, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    [[maybe_unused]] auto [found1, id1] = entryIdMap.FindId(0x1000);
    ASSERT_TRUE(found1) << "Object should exist before erase";

    // Erase the ID mapping
    bool erased = entryIdMap.EraseId(0x1000);
    ASSERT_TRUE(erased) << "EraseId should succeed for existing entry";

    [[maybe_unused]] auto [found2, id2] = entryIdMap.FindId(0x1000);
    ASSERT_FALSE(found2) << "Object should not exist after erase";

    // Erasing non-existent entry should fail gracefully
    bool erasedAgain = entryIdMap.EraseId(0x1000);
    ASSERT_FALSE(erasedAgain) << "EraseId should return false for non-existent entry";
}

HWTEST_F_L0(HybridHeapSnapshotTest, EntryIdMap_RemoveUnmarkedObjects)
{
    EntryIdMap entryIdMap;
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    mockSts->AddStaticNode(0x1000, "LiveObj", arkplatform::StaticNodeType::OBJECT, 64);
    mockSts->AddStaticNode(0x2000, "DeadObj", arkplatform::StaticNodeType::OBJECT, 32);
    mockSts->AddStaticNode(0x3000, "Helper", arkplatform::StaticNodeType::OBJECT, 16);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x1000]);
    mockSts->roots_.push_back(mockSts->nodeInfoMap_[0x2000]);
    mockSts->SetupCycleEdges();

    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, false, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    // Both objects should be present
    auto [found1, id1] = entryIdMap.FindId(0x1000);
    auto [found2, id2] = entryIdMap.FindId(0x2000);
    ASSERT_TRUE(found1);
    ASSERT_TRUE(found2);
    ASSERT_NE(id1, id2);

    // Note: RemoveUnmarkedObjects requires a HeapMarker which is not easily accessible
    // in unit tests. The method is tested indirectly during full GC cycles.
    // This test verifies that the EntryIdMap correctly stores both objects.
    ASSERT_EQ(entryIdMap.GetIdCount(), 3U);
}

// ============================================================================
// Serialization tests
// ============================================================================

HWTEST_F_L0(HybridHeapSnapshotTest, Serialization_DynamicOnlyContainsSnapshotFields)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, true, false, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    TestHybridStream stream;
    ASSERT_TRUE(HeapSnapshotJSONSerializer::Serialize(&snapshot, &stream));
    std::string output = stream.GetOutput();
    ASSERT_FALSE(output.empty());
    AssertOutputContains(output, "\"snapshot\"");
    AssertOutputContains(output, "\"nodes\"");
    AssertOutputContains(output, "\"edges\"");
}

HWTEST_F_L0(HybridHeapSnapshotTest, Serialization_ValidateJSONStructure)
{
    auto mockSts = std::make_unique<MockSTSVMInterface>();
    SetupMockCycle(*mockSts, {0x1000, 0x2000, 0x3000});

    EntryIdMap entryIdMap;
    StringHashMap stringTable(ecmaVm_);
    HybridHeapSnapshot snapshot(ecmaVm_, mockSts.get(), &entryIdMap, &stringTable,
                                false, false, true, ecmaVm_->GetNativeAreaAllocator());
    ASSERT_TRUE(snapshot.BuildHybridSnapshot());

    TestHybridStream stream;
    ASSERT_TRUE(HeapSnapshotJSONSerializer::Serialize(&snapshot, &stream));

    std::string output = stream.GetOutput();
    ASSERT_FALSE(output.empty()) << "Serialized output should not be empty";

    // Verify JSON structure completeness
    AssertOutputContains(output, "\"snapshot\"");
    AssertOutputContains(output, "\"meta\"");
    AssertOutputContains(output, "\"node_fields\"");
    AssertOutputContains(output, "\"node_types\"");
    AssertOutputContains(output, "\"edge_fields\"");
    AssertOutputContains(output, "\"edge_types\"");
    AssertOutputContains(output, "\"nodes\"");
    AssertOutputContains(output, "\"edges\"");

    // Verify test object data is present (SetupMockCycle uses "Obj" prefix)
    AssertOutputContains(output, "\"Obj");
}

}  // namespace panda::test
