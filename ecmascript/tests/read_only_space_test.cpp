/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"
#ifdef USE_CMC_GC
#include "../runtime_core/common_interfaces/base_runtime.h"
#include "common_components/common_runtime/src/heap_manager.h"
#endif
#include <csetjmp>
#include <csignal>
#include <sys/syscall.h>
using namespace panda::ecmascript;

namespace panda::test {
class ReadOnlySpaceTest : public BaseTestWithScope<false> {
public:
    void SetUp() override
    {
        InitializeLogger();
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        factory = thread->GetEcmaVM()->GetFactory();
        const_cast<panda::ecmascript::Heap *>(thread->GetEcmaVM()->GetHeap())->SetMarkType(MarkType::MARK_FULL);
    }

    void InitializeLogger()
    {
        panda::ecmascript::JSRuntimeOptions runtimeOptions;
        runtimeOptions.SetLogLevel("error");
        Log::Initialize(runtimeOptions.GetLogOptions());
    }

    ObjectFactory *factory {nullptr};
};

static sigjmp_buf g_env;
static bool g_segmentfault_flag = false;
class ReadOnlyTestManager {
public:
    // static constexpr int RO_SEGMENTFAULT = 1;
    static void ProcessReadOnlySegmentFault(int sig)
    {
        g_segmentfault_flag = true;
        siglongjmp(g_env, sig);
    }

    static int RegisterSignal()
    {
        struct sigaction act;
        act.sa_handler = ProcessReadOnlySegmentFault;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGQUIT);
        act.sa_flags = SA_RESETHAND;
        return sigaction(SIGSEGV, &act, nullptr);
    }
};

static pid_t ForkBySyscall(void)
{
#ifdef SYS_fork
    return syscall(SYS_fork);
#else
    return syscall(SYS_clone, SIGCHLD, 0);
#endif
}

HWTEST_F_L0(ReadOnlySpaceTest, ReadOnlyTest)
{
    auto *heap = const_cast<panda::ecmascript::Heap *>(thread->GetEcmaVM()->GetHeap());
#ifndef USE_CMC_GC
    heap->GetReadOnlySpace()->SetReadOnly();
#else
    auto heapManager = BaseRuntime::GetInstance()->GetHeapManager();
    heapManager.SetReadOnlyToROSpace();
#endif
    if (ReadOnlyTestManager::RegisterSignal() == -1) {
        perror("sigaction error");
        exit(1);
    }
#ifndef USE_CMC_GC
    SharedHeap::GetInstance()->WaitGCFinished(thread);
#endif
    auto ret = sigsetjmp(g_env, 1);
    if (ret != SIGSEGV) {
        heap->AllocateReadOnlyOrHugeObject(
            JSHClass::Cast(thread->GlobalConstants()->GetBigIntClass().GetTaggedObject()));
    } else {
        // catch signal SIGSEGV caused by modify read only memory
        EXPECT_TRUE(g_segmentfault_flag);
    }
#ifdef USE_CMC_GC
    EXPECT_TRUE(g_segmentfault_flag);
#endif
}

HWTEST_F_L0(ReadOnlySpaceTest, AllocateTest)
{
    auto *heap = const_cast<panda::ecmascript::Heap *>(thread->GetEcmaVM()->GetHeap());
    auto *object = heap->AllocateReadOnlyOrHugeObject(
        JSHClass::Cast(thread->GlobalConstants()->GetBigIntClass().GetTaggedObject()));
#ifndef USE_CMC_GC
    auto *region = Region::ObjectAddressToRange(object);
    EXPECT_TRUE(region->InReadOnlySpace());
#else
    auto heapManager = BaseRuntime::GetInstance()->GetHeapManager();
    EXPECT_TRUE(heapManager.IsInROSpace(object));
#endif
}

HWTEST_F_L0(ReadOnlySpaceTest, CompactHeapBeforeForkTest)
{
#ifndef USE_CMC_GC
    auto *heap = const_cast<panda::ecmascript::Heap *>(thread->GetEcmaVM()->GetHeap());
    std::string rawStr = "test string";
    JSHandle<EcmaString> string = factory->NewFromStdString(rawStr);
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    auto *regionBefore = Region::ObjectAddressToRange(string.GetObject<TaggedObject>());
    auto *objRegionBefore = Region::ObjectAddressToRange(obj.GetObject<TaggedObject>());
    EXPECT_TRUE(regionBefore->InSharedHeap());
    EXPECT_FALSE(objRegionBefore->InReadOnlySpace());
    heap->CompactHeapBeforeFork();
    auto *regionAfter = Region::ObjectAddressToRange(string.GetObject<TaggedObject>());
    auto *objRegionAfter = Region::ObjectAddressToRange(obj.GetObject<TaggedObject>());
    EXPECT_TRUE(regionAfter->InSharedHeap());
    EXPECT_FALSE(objRegionAfter->InReadOnlySpace());
#else
    EXPECT_TRUE(true);
#endif
}

HWTEST_F_L0(ReadOnlySpaceTest, GCTest)
{
    auto *heap = const_cast<panda::ecmascript::Heap *>(thread->GetEcmaVM()->GetHeap());
    auto *object = heap->AllocateReadOnlyOrHugeObject(
        JSHClass::Cast(thread->GlobalConstants()->GetBigIntClass().GetTaggedObject()));
#ifndef USE_CMC_GC
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    auto *region = Region::ObjectAddressToRange(object);
    EXPECT_TRUE(region->InReadOnlySpace());
#else
    auto baseRuntime = BaseRuntime::GetInstance();
    auto heapManager = baseRuntime->GetHeapManager();
    baseRuntime->RequestGC(GcType::FULL);
    EXPECT_TRUE(heapManager.IsInROSpace(object));
#endif
}

HWTEST_F_L0(ReadOnlySpaceTest, ForkTest)
{
    auto vm = thread->GetEcmaVM();
    auto *heap = const_cast<panda::ecmascript::Heap *>(thread->GetEcmaVM()->GetHeap());
#ifndef USE_CMC_GC
    std::string rawStr = "fork string";
    JSHandle<EcmaString> string = factory->NewFromStdString(rawStr);
#else
    auto *object = heap->AllocateReadOnlyOrHugeObject(
            JSHClass::Cast(thread->GlobalConstants()->GetBigIntClass().GetTaggedObject()));
    auto baseRuntime = BaseRuntime::GetInstance();
    auto heapManager = baseRuntime->GetHeapManager();
#endif
    JSNApi::PreFork(vm);
    if (ForkBySyscall() == 0) {
        panda::RuntimeOption postOption;
        JSNApi::PostFork(vm, postOption);
        // test gc in child process
#ifndef USE_CMC_GC
        heap->CollectGarbage(TriggerGCType::OLD_GC);
        auto *region = Region::ObjectAddressToRange(string.GetObject<TaggedObject>());
        EXPECT_TRUE(region->InSharedHeap());
#else
        baseRuntime->RequestGC(GcType::FULL);
        EXPECT_TRUE(heapManager.IsInROSpace(object));
    } else {
        int status;
        pid_t ter_pid = wait(&status);
        if (ter_pid == -1)
        {
            exit(1);
        }
#endif
    }
}
}  // namespace panda::test
