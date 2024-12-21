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

#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
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
    explicit HeapProfilerFriendTest(const EcmaVM *vm) : heapProfiler(vm) {}

    HeapSnapshot *MakeHeapSnapshotTest(HeapProfiler::SampleType type, DumpSnapShotOption dumpOption)
    {
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
private:
    HeapProfiler heapProfiler;
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
    // lineString: 24
    // treeString: 32
    // totalSize: 8 * 24 + 7 * 32
    ASSERT_EQ(totalSize, 416);
}

HWTEST_F_L0(HeapSnapShotTest, TestGeneratePrivateStringNode)
{
    HeapProfilerFriendTest tester(ecmaVm_);
    DumpSnapShotOption dumpOption;
    dumpOption.isPrivate = true;
    HeapSnapShotFriendTest heapSnapShotTest(ecmaVm_, tester.GetEcmaStringTableTest(),
                                            dumpOption, false, tester.GetEntryIdMapTest());
    Node *node = heapSnapShotTest.GeneratePrivateStringNodeTest(0);
    // lineString: 24
    ASSERT_EQ(node->GetSelfSize(), 24);
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
    // treeString: 32
    ASSERT_EQ(movedNode->GetSelfSize(), 32);
}
}