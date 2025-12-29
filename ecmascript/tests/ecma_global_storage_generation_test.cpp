/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace testing::ext;
constexpr int32_t INT_VALUE_0 = 0;
constexpr int32_t INT_VALUE_1 = 1;
constexpr int32_t INT_VALUE_2 = 2;

namespace panda::test {
class EcmaGlobalStorageGenerationTest  : public testing::Test {
public:
    void CalculateGlobalNodeCount(uint64_t &allGlobalNodeCount, uint64_t &normalGlobalNodeCount,
                                  EcmaGlobalStorage<Node> *globalStorage)
    {
        globalStorage->SetNodeKind(NodeKind::UNIFIED_NODE);
        ASSERT(globalStorage->GetNodeKind() == NodeKind::UNIFIED_NODE);
        globalStorage->IterateUsageGlobal([&normalGlobalNodeCount] ([[maybe_unused]] Node *node) {
            normalGlobalNodeCount++;
        });
        globalStorage->SetNodeKind(NodeKind::NORMAL_NODE);
        ASSERT(globalStorage->GetNodeKind() == NodeKind::NORMAL_NODE);
        globalStorage->IterateUsageGlobal([&allGlobalNodeCount] ([[maybe_unused]] Node *node) {
            allGlobalNodeCount++;
        });
    }

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
        JSRuntimeOptions options;
        options.SetArkProperties(options.GetArkProperties() | GLOBAL_OBJECT_LEAK_CHECK | GLOBAL_PRIMITIVE_LEAK_CHECK);
        ecmaVm_ = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(ecmaVm_ != nullptr) << "Cannot create EcmaVM";
        thread_ = ecmaVm_->GetJSThread();
        thread_->ManagedCodeBegin();
        scope_ = new EcmaHandleScope(thread_);
        ecmaVm_->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm_, scope_);
    }

    EcmaVM *ecmaVm_ {nullptr};
    JSThread *thread_ {nullptr};
    EcmaHandleScope *scope_ {nullptr};
};


HWTEST_F(EcmaGlobalStorageGenerationTest, EcmaGlobalStorageYoungGC, TestSize.Level0)
{
    auto chunk = ecmaVm_->GetChunk();
    EcmaGlobalStorage<Node> *globalStorage =
        chunk->New<EcmaGlobalStorage<Node>>(nullptr, ecmaVm_->GetNativeAreaAllocator());

    [[maybe_unused]] JSHandle<JSTaggedValue> xRefArray = JSArray::ArrayCreate(thread_, JSTaggedNumber(INT_VALUE_1));
    [[maybe_unused]] JSHandle<JSTaggedValue> normalArray = JSArray::ArrayCreate(thread_, JSTaggedNumber(INT_VALUE_2));

    uint64_t allGlobalNodeCountBefore = INT_VALUE_0;
    uint64_t normalGlobalNodeCountBefore = INT_VALUE_0;
    ASSERT(globalStorage->GetNodeKind() == NodeKind::NORMAL_NODE);

    auto *heap = const_cast<Heap *>(ecmaVm_->GetHeap());
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    CalculateGlobalNodeCount(allGlobalNodeCountBefore, normalGlobalNodeCountBefore, globalStorage);
    EXPECT_TRUE(normalGlobalNodeCountBefore == allGlobalNodeCountBefore);
    chunk->Delete(globalStorage);
}


HWTEST_F(EcmaGlobalStorageGenerationTest, EcmaGlobalStorageFullGC, TestSize.Level0)
{
    auto chunk = ecmaVm_->GetChunk();
    EcmaGlobalStorage<Node> *globalStorage =
        chunk->New<EcmaGlobalStorage<Node>>(nullptr, ecmaVm_->GetNativeAreaAllocator());

    [[maybe_unused]] JSHandle<JSTaggedValue> xRefArray = JSArray::ArrayCreate(thread_, JSTaggedNumber(INT_VALUE_1));
    [[maybe_unused]] JSHandle<JSTaggedValue> normalArray = JSArray::ArrayCreate(thread_, JSTaggedNumber(INT_VALUE_2));

    uint64_t allGlobalNodeCountBefore = INT_VALUE_0;
    uint64_t normalGlobalNodeCountBefore = INT_VALUE_0;
    ASSERT(globalStorage->GetNodeKind() == NodeKind::NORMAL_NODE);

    auto *heap = const_cast<Heap *>(ecmaVm_->GetHeap());
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    CalculateGlobalNodeCount(allGlobalNodeCountBefore, normalGlobalNodeCountBefore, globalStorage);
    EXPECT_TRUE(normalGlobalNodeCountBefore == allGlobalNodeCountBefore);
    chunk->Delete(globalStorage);
}
};