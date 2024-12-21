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
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
constexpr int32_t INT_VALUE_0 = 0;
constexpr int32_t INT_VALUE_1 = 1;
constexpr int32_t INT_VALUE_2 = 2;

namespace panda::test {
class EcmaGlobalStorageTest : public BaseTestWithScope<false> {
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
};

HWTEST_F_L0(EcmaGlobalStorageTest, XRefGlobalNodes)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<TaggedArray> weakRefArray = vm->GetFactory()->NewTaggedArray(INT_VALUE_2, JSTaggedValue::Hole());
    uintptr_t xRefArrayAddress;
    vm->SetEnableForceGC(false);
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        JSHandle<JSTaggedValue> xRefArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        JSHandle<JSTaggedValue> normalArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_2));
        xRefArrayAddress = thread->NewXRefGlobalHandle(xRefArray.GetTaggedType());
        weakRefArray->Set(thread, INT_VALUE_0, xRefArray.GetTaggedValue().CreateAndGetWeakRef());
        weakRefArray->Set(thread, INT_VALUE_1, normalArray.GetTaggedValue().CreateAndGetWeakRef());
    }
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(!weakRefArray->Get(INT_VALUE_0).IsUndefined());
    EXPECT_TRUE(weakRefArray->Get(INT_VALUE_1).IsUndefined());

    thread->DisposeXRefGlobalHandle(xRefArrayAddress);
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    vm->SetEnableForceGC(true);
    EXPECT_TRUE(weakRefArray->Get(INT_VALUE_0).IsUndefined());
}

HWTEST_F_L0(EcmaGlobalStorageTest, SetNodeKind)
{
    EcmaVM *vm = thread->GetEcmaVM();
    JSHandle<TaggedArray> weakRefArray = vm->GetFactory()->NewTaggedArray(INT_VALUE_1, JSTaggedValue::Hole());
    vm->SetEnableForceGC(false);
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        JSHandle<JSTaggedValue> xRefArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
        thread->NewXRefGlobalHandle(xRefArray.GetTaggedType());
        weakRefArray->Set(thread, INT_VALUE_0, xRefArray.GetTaggedValue().CreateAndGetWeakRef());
    }
    thread->SetNodeKind(NodeKind::UNIFIED_NODE);
    vm->CollectGarbage(TriggerGCType::FULL_GC);
    thread->SetNodeKind(NodeKind::NORMAL_NODE);
    vm->SetEnableForceGC(true);
    EXPECT_TRUE(weakRefArray->Get(INT_VALUE_0).IsUndefined());
}

HWTEST_F_L0(EcmaGlobalStorageTest, EcmaGlobalStorage)
{
    EcmaVM *vm = thread->GetEcmaVM();
    auto chunk = vm->GetChunk();
    EcmaGlobalStorage<Node> *globalStorage =
        chunk->New<EcmaGlobalStorage<Node>>(nullptr, vm->GetNativeAreaAllocator());
    uintptr_t xRefArrayAddress;
    uintptr_t normalArrayAddress;
    JSHandle<JSTaggedValue> xRefArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_1));
    JSHandle<JSTaggedValue> normalArray = JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_2));

    uint64_t allGlobalNodeCountBefore = INT_VALUE_0;
    uint64_t normalGlobalNodeCountBefore = INT_VALUE_0;
    ASSERT(globalStorage->GetNodeKind() == NodeKind::NORMAL_NODE);
    CalculateGlobalNodeCount(allGlobalNodeCountBefore, normalGlobalNodeCountBefore, globalStorage);
    EXPECT_TRUE(normalGlobalNodeCountBefore == allGlobalNodeCountBefore);

    uint64_t allGlobalNodeCountAfter = INT_VALUE_0;
    uint64_t normalGlobalNodeCountAfter = INT_VALUE_0;
    xRefArrayAddress = globalStorage->NewGlobalHandle<NodeKind::UNIFIED_NODE>(xRefArray.GetTaggedType());
    CalculateGlobalNodeCount(allGlobalNodeCountAfter, normalGlobalNodeCountAfter, globalStorage);
    EXPECT_TRUE(allGlobalNodeCountAfter == allGlobalNodeCountBefore + INT_VALUE_1);
    EXPECT_TRUE(normalGlobalNodeCountAfter == normalGlobalNodeCountBefore);

    allGlobalNodeCountAfter = INT_VALUE_0;
    normalGlobalNodeCountAfter = INT_VALUE_0;
    normalArrayAddress = globalStorage->NewGlobalHandle<NodeKind::NORMAL_NODE>(normalArray.GetTaggedType());
    CalculateGlobalNodeCount(allGlobalNodeCountAfter, normalGlobalNodeCountAfter, globalStorage);
    EXPECT_TRUE(allGlobalNodeCountAfter == allGlobalNodeCountBefore + INT_VALUE_2);
    EXPECT_TRUE(normalGlobalNodeCountAfter == normalGlobalNodeCountBefore + INT_VALUE_1);

    allGlobalNodeCountAfter = INT_VALUE_0;
    normalGlobalNodeCountAfter = INT_VALUE_0;
    globalStorage->DisposeGlobalHandle<NodeKind::UNIFIED_NODE>(xRefArrayAddress);
    CalculateGlobalNodeCount(allGlobalNodeCountAfter, normalGlobalNodeCountAfter, globalStorage);
    EXPECT_TRUE(allGlobalNodeCountAfter == allGlobalNodeCountBefore + INT_VALUE_1);
    EXPECT_TRUE(normalGlobalNodeCountAfter == normalGlobalNodeCountBefore + INT_VALUE_1);

    allGlobalNodeCountAfter = INT_VALUE_0;
    normalGlobalNodeCountAfter = INT_VALUE_0;
    globalStorage->DisposeGlobalHandle<NodeKind::NORMAL_NODE>(normalArrayAddress);
    CalculateGlobalNodeCount(allGlobalNodeCountAfter, normalGlobalNodeCountAfter, globalStorage);
    EXPECT_TRUE(allGlobalNodeCountAfter == allGlobalNodeCountBefore);
    EXPECT_TRUE(normalGlobalNodeCountAfter == normalGlobalNodeCountBefore);

    chunk->Delete(globalStorage);
}
};