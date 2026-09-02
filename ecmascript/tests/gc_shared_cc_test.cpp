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

#include <condition_variable>
#include <mutex>
#include <thread>

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class SharedCCTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        CreateVM(options);
    }

    void CreateVM(JSRuntimeOptions &options)
    {
        options.SetEnableForceGC(false);
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        thread->ManagedCodeBegin();
        scope = new EcmaHandleScope(thread);
    }

    void CollectSharedCC()
    {
        SharedHeap::GetInstance()->CollectGarbage<TriggerGCType::SHARED_CC, GCReason::OTHER>(thread);
    }

    void CollectLocalFullGC()
    {
        const_cast<Heap *>(instance->GetHeap())->CollectGarbage(TriggerGCType::FULL_GC);
    }

    JSHandle<TaggedArray> AllocateSharedArray(size_t arrayLen)
    {
        return instance->GetFactory()->NewSTaggedArray(arrayLen, JSTaggedValue::Undefined());
    }

    void AllocateSharedGarbage()
    {
        constexpr size_t ARRAY_LEN = 16 * 1024;
        constexpr size_t ARRAY_COUNT = 64;
        EcmaHandleScope temporaryScope(thread);
        for (size_t i = 0; i < ARRAY_COUNT; i++) {
            AllocateSharedArray(ARRAY_LEN);
        }
    }
};

class SharedCCVerificationTest : public SharedCCTest {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetArkProperties(options.GetArkProperties() | ArkProperties::ENABLE_HEAP_VERIFY);
        CreateVM(options);
    }
};

class SharedCCStringTableSweepDisabledTest : public SharedCCTest {
public:
    void SetUp() override
    {
        JSRuntimeOptions options;
        options.SetArkProperties(
            options.GetArkProperties() | ArkProperties::DISABLE_STRING_TABLE_CONCURRENT_SWEEP);
        CreateVM(options);
    }
};

HWTEST_F_L0(SharedCCTest, BasicSurvivalTest)
{
    constexpr size_t ARRAY_LEN = 10;
    JSHandle<TaggedArray> aliveArr = AllocateSharedArray(ARRAY_LEN);
    AllocateSharedGarbage();

    CollectSharedCC();

    EXPECT_EQ(aliveArr->GetLength(), ARRAY_LEN);
}

HWTEST_F_L0(SharedCCTest, WeakRefSurvivalTest)
{
    constexpr size_t ARRAY_LEN = 10;
    JSHandle<TaggedArray> sOld1 = AllocateSharedArray(ARRAY_LEN);
    JSHandle<TaggedArray> sOld2 = AllocateSharedArray(ARRAY_LEN);
    AllocateSharedGarbage();

    auto weakRef = sOld2.GetTaggedValue();
    weakRef.CreateWeakRef();
    sOld1->Set(thread, 0, weakRef);
    EXPECT_TRUE(JSTaggedValue(sOld1->Get(thread, 0)).IsWeak());

    CollectSharedCC();

    JSTaggedValue afterGC = sOld1->Get(thread, 0);
    ASSERT_TRUE(afterGC.IsWeak());
    EXPECT_EQ(afterGC.GetWeakReferent(), sOld2.GetTaggedValue().GetRawHeapObject());
}

HWTEST_F_L0(SharedCCTest, LocalToSharedRefTest)
{
    constexpr size_t ARRAY_LEN = 10;
    JSHandle<TaggedArray> localArr =
        instance->GetFactory()->NewTaggedArray(ARRAY_LEN, JSTaggedValue::Undefined(), false);
    {
        EcmaHandleScope temporaryScope(thread);
        JSHandle<TaggedArray> sharedArr = AllocateSharedArray(ARRAY_LEN);
        localArr->Set(thread, 0, sharedArr);
    }
    AllocateSharedGarbage();

    CollectSharedCC();

    JSTaggedValue refAfterGC = localArr->Get(thread, 0);
    ASSERT_TRUE(refAfterGC.IsHeapObject());
    EXPECT_TRUE(refAfterGC.IsInSharedHeap());
}

HWTEST_F_L0(SharedCCTest, MultipleCyclesTest)
{
    constexpr size_t ARRAY_LEN = 10;
    for (int cycle = 0; cycle < 3; cycle++) {
        JSHandle<TaggedArray> alive = AllocateSharedArray(ARRAY_LEN);
        AllocateSharedGarbage();

        CollectSharedCC();

        EXPECT_EQ(alive->GetLength(), ARRAY_LEN);
    }
}

HWTEST_F_L0(SharedCCTest, LocalFullGCAfterSharedCCTest)
{
    constexpr size_t ARRAY_LEN = 10;
    JSHandle<TaggedArray> localArr =
        instance->GetFactory()->NewTaggedArray(ARRAY_LEN, JSTaggedValue::Undefined(), false);
    {
        EcmaHandleScope temporaryScope(thread);
        JSHandle<TaggedArray> sharedArr = AllocateSharedArray(ARRAY_LEN);
        localArr->Set(thread, 0, sharedArr);
    }
    AllocateSharedGarbage();

    CollectSharedCC();
    CollectLocalFullGC();

    JSTaggedValue ref = localArr->Get(thread, 0);
    ASSERT_TRUE(ref.IsHeapObject());
    EXPECT_TRUE(ref.IsInSharedHeap());
}

HWTEST_F_L0(SharedCCTest, LocalFullGCBeforeSharedCCTest)
{
    constexpr size_t ARRAY_LEN = 10;
    JSHandle<TaggedArray> localArr =
        instance->GetFactory()->NewTaggedArray(ARRAY_LEN, JSTaggedValue::Undefined(), false);
    {
        EcmaHandleScope temporaryScope(thread);
        JSHandle<TaggedArray> sharedArr = AllocateSharedArray(ARRAY_LEN);
        localArr->Set(thread, 0, sharedArr);
    }
    AllocateSharedGarbage();

    CollectLocalFullGC();
    CollectSharedCC();

    JSTaggedValue ref = localArr->Get(thread, 0);
    ASSERT_TRUE(ref.IsHeapObject());
    EXPECT_TRUE(ref.IsInSharedHeap());
}

HWTEST_F_L0(SharedCCTest, CyclicRefTest)
{
    constexpr size_t ARRAY_LEN = 10;
    JSHandle<TaggedArray> sOld1 = AllocateSharedArray(ARRAY_LEN);
    AllocateSharedGarbage();
    {
        EcmaHandleScope temporaryScope(thread);
        JSHandle<TaggedArray> sOld2 = AllocateSharedArray(ARRAY_LEN);
        sOld1->Set(thread, 0, sOld2);
        sOld2->Set(thread, 0, sOld1);
    }

    CollectSharedCC();

    JSTaggedValue ref1 = sOld1->Get(thread, 0);
    ASSERT_TRUE(ref1.IsHeapObject());
    JSHandle<TaggedArray> sOld2(thread, ref1);
    JSTaggedValue ref2 = sOld2->Get(thread, 0);
    ASSERT_TRUE(ref2.IsHeapObject());
    EXPECT_TRUE(ref1.IsInSharedHeap());
    EXPECT_TRUE(ref2.IsInSharedHeap());
    EXPECT_EQ(ref2.GetRawHeapObject(), sOld1.GetTaggedValue().GetRawHeapObject());
}

HWTEST_F_L0(SharedCCTest, DeepObjectGraphTest)
{
    constexpr size_t ARRAY_LEN = 10;
    constexpr size_t CHAIN_DEPTH = 8;
    JSHandle<TaggedArray> root = AllocateSharedArray(ARRAY_LEN);
    {
        EcmaHandleScope temporaryScope(thread);
        JSHandle<TaggedArray> current = root;
        for (size_t i = 1; i < CHAIN_DEPTH; i++) {
            JSHandle<TaggedArray> next = AllocateSharedArray(ARRAY_LEN);
            current->Set(thread, 0, next);
            current = next;
        }
    }
    AllocateSharedGarbage();

    CollectSharedCC();

    JSHandle<TaggedArray> current = root;
    for (size_t i = 1; i < CHAIN_DEPTH; i++) {
        JSTaggedValue next = current->Get(thread, 0);
        ASSERT_TRUE(next.IsHeapObject());
        ASSERT_TRUE(next.IsInSharedHeap());
        current = JSHandle<TaggedArray>(thread, next);
    }
    EXPECT_EQ(current->GetLength(), ARRAY_LEN);
}

HWTEST_F_L0(SharedCCVerificationTest, SharedCCWithHeapVerifyTest)
{
    constexpr size_t ARRAY_LEN = 10;
    ASSERT_TRUE(instance->GetJSOptions().EnableHeapVerify());
    JSHandle<TaggedArray> sOld1 = AllocateSharedArray(ARRAY_LEN);
    JSHandle<TaggedArray> sOld2 = AllocateSharedArray(ARRAY_LEN);
    sOld1->Set(thread, 0, sOld2);
    AllocateSharedGarbage();

    CollectSharedCC();

    JSTaggedValue ref = sOld1->Get(thread, 0);
    ASSERT_TRUE(ref.IsHeapObject());
    EXPECT_TRUE(ref.IsInSharedHeap());
    EXPECT_EQ(sOld2->GetLength(), ARRAY_LEN);
}

HWTEST_F_L0(SharedCCTest, SharedNonMovableRefTest)
{
    constexpr size_t ARRAY_LEN = 10;
    JSHandle<TaggedArray> sOld = AllocateSharedArray(ARRAY_LEN);
    AllocateSharedGarbage();

    JSHandle<TaggedArray> sNonmovable = instance->GetFactory()->NewSTaggedArray(
        ARRAY_LEN, JSTaggedValue::Undefined(), MemSpaceType::SHARED_NON_MOVABLE);
    sOld->Set(thread, 0, sNonmovable);

    CollectSharedCC();

    JSTaggedValue ref = sOld->Get(thread, 0);
    ASSERT_TRUE(ref.IsHeapObject());
    EXPECT_TRUE(ref.IsInSharedHeap());
    EXPECT_EQ(ref.GetRawHeapObject(), sNonmovable.GetTaggedValue().GetRawHeapObject());
}

HWTEST_F_L0(SharedCCStringTableSweepDisabledTest, StringTableSweepDisabledTest)
{
    constexpr size_t ARRAY_LEN = 10;
    constexpr char stringData[] = "shared_cc_sweep_disabled";
    ASSERT_FALSE(instance->GetJSOptions().EnableStringTableConcurrentSweep());
    LocalScope localScope(instance);

    Local<StringRef> interned = StringRef::NewFromUtf8(instance, stringData);

    JSHandle<TaggedArray> alive = AllocateSharedArray(ARRAY_LEN);
    AllocateSharedGarbage();

    CollectSharedCC();

    Local<StringRef> afterGC = StringRef::NewFromUtf8(instance, stringData);
    EXPECT_TRUE(interned == afterGC);
    EXPECT_EQ(alive->GetLength(), ARRAY_LEN);
}

namespace {
constexpr size_t WORKER_ARRAY_LEN = 10;
constexpr size_t WORKER_LOCAL_REF_COUNT = 2;
constexpr size_t WORKER_GARBAGE_LEN = 16 * 1024;
constexpr size_t WORKER_GARBAGE_COUNT = 64;

struct WorkerCtx {
    std::mutex mtx;
    std::condition_variable cv;
    bool workerReady {false};
    bool gcDone {false};
    bool workerOk {false};
};

void VerifyWorkerRefs(JSThread *workerThread, const JSHandle<TaggedArray> &localArr)
{
    JSTaggedValue r1 = localArr->Get(workerThread, 0);
    EXPECT_TRUE(r1.IsHeapObject());
    EXPECT_TRUE(r1.IsInSharedHeap());
    EXPECT_EQ(JSHandle<TaggedArray>(workerThread, r1)->GetLength(), WORKER_ARRAY_LEN);
    JSTaggedValue r2 = localArr->Get(workerThread, 1);
    EXPECT_TRUE(r2.IsHeapObject());
    EXPECT_TRUE(r2.IsInSharedHeap());
    EXPECT_EQ(JSHandle<TaggedArray>(workerThread, r2)->GetLength(), WORKER_ARRAY_LEN);
    JSTaggedValue r12 = JSHandle<TaggedArray>(workerThread, r1)->Get(workerThread, 0);
    EXPECT_TRUE(r12.IsHeapObject());
    EXPECT_TRUE(r12.IsInSharedHeap());
}

void WorkerRun(WorkerCtx *ctx)
{
    JSRuntimeOptions options;
    options.SetEnableForceGC(false);
    EcmaVM *workerVm = JSNApi::CreateEcmaVM(options);
    ASSERT_TRUE(workerVm != nullptr) << "Cannot create worker EcmaVM";
    JSThread *workerThread = workerVm->GetJSThread();
    workerThread->ManagedCodeBegin();
    {
        EcmaHandleScope workerScope(workerThread);
        ObjectFactory *factory = workerVm->GetFactory();
        JSHandle<TaggedArray> sharedAlive1 = factory->NewSTaggedArray(WORKER_ARRAY_LEN, JSTaggedValue::Undefined());
        JSHandle<TaggedArray> sharedAlive2 = factory->NewSTaggedArray(WORKER_ARRAY_LEN, JSTaggedValue::Undefined());
        {
            EcmaHandleScope tmpScope(workerThread);
            for (size_t i = 0; i < WORKER_GARBAGE_COUNT; i++) {
                factory->NewSTaggedArray(WORKER_GARBAGE_LEN, JSTaggedValue::Undefined());
            }
        }
        sharedAlive1->Set(workerThread, 0, sharedAlive2);
        JSHandle<TaggedArray> localArr =
            factory->NewTaggedArray(WORKER_LOCAL_REF_COUNT, JSTaggedValue::Undefined(), false);
        localArr->Set(workerThread, 0, sharedAlive1);
        localArr->Set(workerThread, 1, sharedAlive2);
        {
            ThreadSuspensionScope suspensionScope(workerThread);
            std::unique_lock<std::mutex> lock(ctx->mtx);
            ctx->workerReady = true;
            ctx->cv.notify_one();
            ctx->cv.wait(lock, [ctx] { return ctx->gcDone; });
        }
        VerifyWorkerRefs(workerThread, localArr);
    }
    workerThread->ManagedCodeEnd();
    JSNApi::DestroyJSVM(workerVm);
    std::lock_guard<std::mutex> lock(ctx->mtx);
    ctx->workerOk = true;
    ctx->cv.notify_one();
}
} // namespace

HWTEST_F_L0(SharedCCTest, WorkerThreadSharedRSetTest)
{
    WorkerCtx ctx;
    std::thread worker(WorkerRun, &ctx);
    {
        ThreadNativeScope nativeScope(thread);
        std::unique_lock<std::mutex> lock(ctx.mtx);
        ctx.cv.wait(lock, [&ctx] { return ctx.workerReady; });
    }
    CollectSharedCC();
    {
        std::lock_guard<std::mutex> lock(ctx.mtx);
        ctx.gcDone = true;
        ctx.cv.notify_all();
    }
    {
        ThreadNativeScope nativeScope(thread);
        std::unique_lock<std::mutex> lock(ctx.mtx);
        ctx.cv.wait(lock, [&ctx] { return ctx.workerOk; });
    }
    worker.join();
    EXPECT_TRUE(ctx.workerOk);
}

} // namespace panda::test
