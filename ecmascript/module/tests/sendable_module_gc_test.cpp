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

#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/module/js_shared_module.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/runtime.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {

class SendableModuleGcTest : public testing::Test {
public:
    static void SetUpTestCase() { GTEST_LOG_(INFO) << "SetUpTestCase"; }

    static void TearDownTestCase() { GTEST_LOG_(INFO) << "TearDownCase"; }

    void SetUp() override { TestHelper::CreateEcmaVMWithScope(instance, thread, scope); }

    void TearDown() override { TestHelper::DestroyEcmaVMWithScope(instance, scope); }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: SendableModuleGcTest_NativeFieldHelpers
 * @tc.desc: Test Extract/ReleaseSharedModuleNativeFields helpers: extract nulls fields
 *   (idempotent), release frees raw pointers per build config, null-safe.
 * @tc.type: FUNC
 */
HWTEST_F_L0(SendableModuleGcTest, NativeFieldHelpers)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<SourceTextModule> sModule = factory->NewSSourceTextModule();
    sModule->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
#if ENABLE_MODULE_MEMORY_OPTIMIZATION
    SharedModuleManager *sharedModuleManager = SharedModuleManager::GetInstance();
    size_t storageSize = sharedModuleManager->GetModuleFilenameStorageSizeForTest();
#endif
    sModule->SetEcmaSharedModuleFilenameString("sendable_module_gc_helper.abc");
    bool *lazyArray = new bool[2] {false, false};
    sModule->SetLazyImportArray(lazyArray);
    CString recordName = "sendable_module_gc_helper_record";
    sModule->SetEcmaModuleRecordNameString(recordName);

    // First extract returns the raw pointers and nulls the fields.
    auto fields = sModule->ExtractSharedModuleNativeFields();
    EXPECT_EQ(fields.lazyImportArray, lazyArray);
    EXPECT_NE(fields.filename, nullptr);
    EXPECT_NE(fields.recordName, nullptr);
    EXPECT_EQ(sModule->GetEcmaModuleFilename(), 0U);
    EXPECT_EQ(sModule->GetLazyImportStatusArray(), nullptr);
    EXPECT_EQ(sModule->GetEcmaModuleRecordNameString(), "");

    // Second extract returns all nulls.
    auto fieldsAgain = sModule->ExtractSharedModuleNativeFields();
    EXPECT_EQ(fieldsAgain.lazyImportArray, nullptr);
    EXPECT_EQ(fieldsAgain.filename, nullptr);
    EXPECT_EQ(fieldsAgain.recordName, nullptr);

    // Release frees the extracted pointers.
    SourceTextModule::ReleaseSharedModuleNativeFields(fields);
#if ENABLE_MODULE_MEMORY_OPTIMIZATION
    EXPECT_EQ(sharedModuleManager->GetModuleFilenameStorageSizeForTest(), storageSize);
#endif

    // Release is null-safe.
    SourceTextModule::ReleaseSharedModuleNativeFields(SourceTextModule::NativeFields {});
}

/**
 * @tc.name: SendableModuleGcTest.SendableFuncModuleGcCallbackOnCollect
 * @tc.desc: AC-1.1: an unreachable sendable func module is detected during SharedGC;
 *   its native fields are extracted before sweep and released after GC, exactly once.
 * @tc.type: FUNC
 */
HWTEST_F_L0(SendableModuleGcTest, SendableFuncModuleGcCallbackOnCollect)
{
    SharedModuleManager *sharedModuleManager = SharedModuleManager::GetInstance();
    size_t storageSize = sharedModuleManager->GetModuleFilenameStorageSizeForTest();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    {
        EcmaHandleScope innerScope(thread);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<SourceTextModule> sModule = factory->NewSSourceTextModule();
        sModule->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
        sModule->SetEcmaSharedModuleFilenameString("sendable_module_gc_dead.abc");
        sModule->SetLazyImportArray(new bool[2] {false, false});
        sModule->SetEcmaModuleRecordNameString("sendable_module_gc_dead_record");
        sHeap->PushToSendableModuleList(sModule.GetTaggedValue());
        EXPECT_EQ(sHeap->GetSendableModuleListSize(), 1U);
    } // innerScope closes: module unreachable
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(sHeap->GetSendableModuleListSize(), 0U);
    // Filename storage restored.
    EXPECT_EQ(sharedModuleManager->GetModuleFilenameStorageSizeForTest(), storageSize);
}

/**
 * @tc.name: SendableModuleGcTest.SendableFuncModuleGcCallbackAliveFuncGuard
 * @tc.desc: AC-1.2: a module still referenced by a live handle (as a live jsFunction would)
 *   survives SharedGC: no cleanup happens while it is reachable; once the reference is
 *   dropped, the next SharedGC reaps it.
 * @tc.type: FUNC
 */
HWTEST_F_L0(SendableModuleGcTest, SendableFuncModuleGcCallbackAliveFuncGuard)
{
    SharedModuleManager *sharedModuleManager = SharedModuleManager::GetInstance();
    size_t storageSize = sharedModuleManager->GetModuleFilenameStorageSizeForTest();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    {
        // Live handle keeps the module alive.
        EcmaHandleScope innerScope(thread);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<SourceTextModule> sModule = factory->NewSSourceTextModule();
        sModule->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
        sModule->SetEcmaSharedModuleFilenameString("sendable_module_gc_alive.abc");
        sModule->SetLazyImportArray(new bool[2] {false, false});
        sHeap->PushToSendableModuleList(sModule.GetTaggedValue());

        sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
        EXPECT_EQ(sHeap->GetSendableModuleListSize(), 1U);
        EXPECT_EQ(sModule->GetEcmaModuleFilenameString(), "sendable_module_gc_alive.abc");
    } // innerScope closes
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(sHeap->GetSendableModuleListSize(), 0U);
    EXPECT_EQ(sharedModuleManager->GetModuleFilenameStorageSizeForTest(), storageSize);
}
/**
 * @tc.name: SendableModuleGcTest.SendableFuncModuleGcCallbackAfterExplicitDestroy
 * @tc.desc: AC-1.3: a module already cleaned by the explicit path (NativeObjDestroy, which runs
 *   DestroySharedModuleCNativeFields) must not be double-released when GC later reaps it:
 *   extraction finds null fields, nothing is queued. The explicit path only owns
 *   lazyImportArray/filename: recordName survives it and is released by the GC path instead.
 * @tc.type: FUNC
 */
HWTEST_F_L0(SendableModuleGcTest, SendableFuncModuleGcCallbackAfterExplicitDestroy)
{
    SharedModuleManager *sharedModuleManager = SharedModuleManager::GetInstance();
    size_t storageSize = sharedModuleManager->GetModuleFilenameStorageSizeForTest();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    CString recordName = "sendable_module_gc_explicit";
    {
        EcmaHandleScope innerScope(thread);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<SourceTextModule> sModule = factory->NewSSourceTextModule();
        sModule->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
        sModule->SetEcmaSharedModuleFilenameString("sendable_module_gc_explicit.abc");
        sModule->SetLazyImportArray(new bool[2] {false, false});
        sModule->SetEcmaModuleRecordNameString(recordName);
        thread->GetModuleManager()->AddSendableModuleToCache(recordName, sModule.GetTaggedValue());
        sHeap->PushToSendableModuleList(sModule.GetTaggedValue());
        // Explicit path: NativeObjDestroy runs DestroySharedModuleCNativeFields.
        thread->GetModuleManager()->NativeObjDestroy();
        EXPECT_EQ(sModule->GetEcmaModuleFilename(), 0U);
        EXPECT_EQ(sModule->GetLazyImportStatusArray(), nullptr);
        // recordName survives the explicit path.
        EXPECT_EQ(sModule->GetEcmaModuleRecordNameString(), recordName);
        // Drop the strong cache: module becomes unreachable.
        thread->GetModuleManager()->ClearSendableModulesForTest();
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(sHeap->GetSendableModuleListSize(), 0U);
    EXPECT_EQ(sharedModuleManager->GetModuleFilenameStorageSizeForTest(), storageSize);
}

/**
 * @tc.name: SendableModuleGcTest.SendableFuncModuleCacheResidentAcrossGC
 * @tc.desc: AC-2.1: while the VM is alive, resolvedSendableModules_ keeps the module strongly
 *   rooted across repeated compacting SharedGCs; the cache keeps returning that module (handle
 *   slots are updated on forwarding, so both reads happen post-GC).
 * @tc.type: FUNC
 */
HWTEST_F_L0(SendableModuleGcTest, SendableFuncModuleCacheResidentAcrossGC)
{
    ObjectFactory *factory = instance->GetFactory();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    ModuleManager *moduleManager = thread->GetModuleManager();
    JSHandle<SourceTextModule> localModule = factory->NewSourceTextModule();
    CString recordName = "sendable_module_gc_resident_src";
    localModule->SetEcmaModuleRecordNameString(recordName);

    JSHandle<JSTaggedValue> sendable = moduleManager->GenerateSendableFuncModule(
        JSHandle<JSTaggedValue>::Cast(localModule));
    ASSERT_TRUE(sendable->IsSourceTextModule());
    for (int i = 0; i < 2; i++) {
        sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
        // Cache hit after compacting GC.
        JSHandle<JSTaggedValue> again = moduleManager->GenerateSendableFuncModule(
            JSHandle<JSTaggedValue>::Cast(localModule));
        EXPECT_TRUE(again->IsSourceTextModule());
        EXPECT_EQ(again.GetTaggedValue(), sendable.GetTaggedValue());
    }
    EXPECT_TRUE(moduleManager->TryGetSendableModule(recordName)->IsSourceTextModule());
}

/**
 * @tc.name: SendableModuleGcTest.SendableFuncModuleAutoRegistered
 * @tc.desc: GenerateSendableFuncModule registers the newly created module in the weak list:
 *   after the module cache is cleared (VM teardown simulation) and references drop, SharedGC
 *   reaps the module and releases its native fields.
 * @tc.type: FUNC
 */
HWTEST_F_L0(SendableModuleGcTest, SendableFuncModuleAutoRegistered)
{
    SharedModuleManager *sharedModuleManager = SharedModuleManager::GetInstance();
    size_t storageSize = sharedModuleManager->GetModuleFilenameStorageSizeForTest();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    {
        EcmaHandleScope innerScope(thread);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<SourceTextModule> localModule = factory->NewSourceTextModule();
        CString recordName = "sendable_module_gc_auto_src";
        localModule->SetEcmaModuleRecordNameString(recordName);
        ModuleManager *moduleManager = thread->GetModuleManager();
        JSHandle<JSTaggedValue> sendable = moduleManager->GenerateSendableFuncModule(
            JSHandle<JSTaggedValue>::Cast(localModule));
        ASSERT_TRUE(sendable->IsSourceTextModule());
        SourceTextModule *sModule = SourceTextModule::Cast(sendable->GetTaggedObject());
        sModule->SetEcmaSharedModuleFilenameString("sendable_module_gc_auto.abc");
        EXPECT_EQ(sHeap->GetSendableModuleListSize(), 1U);
        // Simulate VM teardown.
        moduleManager->ClearSendableModulesForTest();
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(sHeap->GetSendableModuleListSize(), 0U);
    EXPECT_EQ(sharedModuleManager->GetModuleFilenameStorageSizeForTest(), storageSize);
}
/**
 * @tc.name: SendableModuleGcTest.SendableFuncModuleGcCallbackOncePerModule
 * @tc.desc: AC-1.4 (idempotence half): even if a module is (erroneously) registered twice in
 *   the weak list, its native fields are released exactly once: the first dead-entry
 *   extraction nulls the fields, the second finds nulls and queues nothing.
 * @tc.type: FUNC
 */
HWTEST_F_L0(SendableModuleGcTest, SendableFuncModuleGcCallbackOncePerModule)
{
    SharedModuleManager *sharedModuleManager = SharedModuleManager::GetInstance();
    size_t storageSize = sharedModuleManager->GetModuleFilenameStorageSizeForTest();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    {
        EcmaHandleScope innerScope(thread);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<SourceTextModule> sModule = factory->NewSSourceTextModule();
        sModule->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
        sModule->SetEcmaSharedModuleFilenameString("sendable_module_gc_twice.abc");
        sModule->SetLazyImportArray(new bool[2] {false, false});
        // Simulate an abnormal double registration.
        sHeap->PushToSendableModuleList(sModule.GetTaggedValue());
        sHeap->PushToSendableModuleList(sModule.GetTaggedValue());
        EXPECT_EQ(sHeap->GetSendableModuleListSize(), 2U);
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    // Both entries removed, storage restored exactly once.
    EXPECT_EQ(sHeap->GetSendableModuleListSize(), 0U);
    EXPECT_EQ(sharedModuleManager->GetModuleFilenameStorageSizeForTest(), storageSize);
}

/**
 * @tc.name: SendableModuleGcTest.TwoModulesTwoCallbacks
 * @tc.desc: AC-1.4 (per-module granularity): two distinct modules are reaped in the same GC
 *   and each one's native fields are released (storage count drops by exactly two).
 * @tc.type: FUNC
 */
HWTEST_F_L0(SendableModuleGcTest, TwoModulesTwoCallbacks)
{
    SharedModuleManager *sharedModuleManager = SharedModuleManager::GetInstance();
    size_t storageSize = sharedModuleManager->GetModuleFilenameStorageSizeForTest();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    {
        EcmaHandleScope innerScope(thread);
        ObjectFactory *factory = instance->GetFactory();
        JSHandle<SourceTextModule> moduleA = factory->NewSSourceTextModule();
        moduleA->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
        moduleA->SetEcmaSharedModuleFilenameString("sendable_module_gc_a.abc");
        moduleA->SetLazyImportArray(new bool[2] {false, false});
        JSHandle<SourceTextModule> moduleB = factory->NewSSourceTextModule();
        moduleB->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
        moduleB->SetEcmaSharedModuleFilenameString("sendable_module_gc_b.abc");
        moduleB->SetLazyImportArray(new bool[2] {false, false});
        sHeap->PushToSendableModuleList(moduleA.GetTaggedValue());
        sHeap->PushToSendableModuleList(moduleB.GetTaggedValue());
        EXPECT_EQ(sHeap->GetSendableModuleListSize(), 2U);
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    EXPECT_EQ(sHeap->GetSendableModuleListSize(), 0U);
    EXPECT_EQ(sharedModuleManager->GetModuleFilenameStorageSizeForTest(), storageSize);
}
}  // namespace panda::test
