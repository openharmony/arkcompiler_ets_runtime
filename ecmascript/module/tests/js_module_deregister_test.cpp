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

#include "ecmascript/js_tagged_value_wrapper-inl.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_manager_map.h"
#include "ecmascript/module/module_tools.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tagged_dictionary.h"

#define private public
#include "ecmascript/module/js_module_manager.h"
#undef private

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/napi/include/jsnapi_expo.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class ModuleDeregisterTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    JSHandle<SourceTextModule> CreateEvaluatedDynamicModule(const CString &recordName)
    {
        JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
        module->SetEcmaModuleRecordNameString(recordName);
        module->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
        module->SetStatus(ModuleStatus::EVALUATED);
        module->SetRegisterCounts(0);
        return module;
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: ModuleDeregister_TryToRemoveSO_WithCallback
 * @tc.desc: Test TryToRemoveSO with valid unload callback (not entering null callback branch)
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_TryToRemoveSO_WithCallback)
{
    ObjectFactory *factory = instance->GetFactory();
    
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    CString recordName = "@ohos:hilog";
    module->SetEcmaModuleRecordNameString(recordName);
    
    bool callbackCalled = false;
    JSNApi::SetUnloadNativeModuleCallback(instance, [&callbackCalled](const std::string &) -> bool {
        callbackCalled = true;
        return true;
    });
    
    bool result = ModuleDeregister::TryToRemoveSO(thread, module);
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(callbackCalled);
}

HWTEST_F_L0(ModuleDeregisterTest, ModuleManager_RemoveModule_NormalModule)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString recordName = "test_module";
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    bool isLoaded = moduleManager->IsModuleLoaded(recordName);
    EXPECT_TRUE(isLoaded);
    // Remove module from cache
    ModuleDeregister::RemoveModule(thread, module);
    isLoaded = moduleManager->IsModuleLoaded(recordName);
    EXPECT_FALSE(isLoaded);
    EXPECT_EQ(moduleManager->pendingRemovalModules_.Size(), 1U);
    JSHandle<JSTaggedValue> restoredModule = moduleManager->TryGetPendingRemovalModule(recordName);
    EXPECT_EQ(restoredModule.GetTaggedValue(), module.GetTaggedValue());
    EXPECT_TRUE(moduleManager->IsModuleLoaded(recordName));
    EXPECT_EQ(moduleManager->pendingRemovalModules_.Size(), 0U);
    EXPECT_TRUE(moduleManager->TryGetPendingRemovalModule(recordName)->IsUndefined());
}

HWTEST_F_L0(ModuleDeregisterTest, ModuleManager_RemoveModule_BothModulesExist)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *objectFactory = thread->GetEcmaVM()->GetFactory();
    CString recordName = "test_module_both";
    // Create and add a normal module to the cache
    JSHandle<SourceTextModule> normalModule = objectFactory->NewSourceTextModule();
    normalModule->SetEcmaModuleRecordNameString(recordName);
    normalModule->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    moduleManager->AddResolveImportedModule(recordName, normalModule.GetTaggedValue());
    // Create and add a sendable module to the cache
    JSHandle<SourceTextModule> sendableModule = objectFactory->NewSSourceTextModule();
    sendableModule->SetSharedType(SharedTypes::SENDABLE_FUNCTION_MODULE);
    sendableModule->SetEcmaSharedModuleFilenameString("test_module_both.abc");
    moduleManager->AddSendableModuleToCache(recordName, sendableModule.GetTaggedValue());

    // Verify both modules are in cache
    bool isLoaded = moduleManager->IsModuleLoaded(recordName);
    EXPECT_TRUE(isLoaded);
    JSHandle<JSTaggedValue> cached = moduleManager->TryGetSendableModule(recordName);
    EXPECT_TRUE(cached->IsSourceTextModule());
    
    // Remove module from cache
    ModuleDeregister::RemoveModule(thread, normalModule);
    // Verify module is removed from cache
    isLoaded = moduleManager->IsModuleLoaded(recordName);
    EXPECT_FALSE(isLoaded);
    cached = moduleManager->TryGetSendableModule(recordName);
    EXPECT_TRUE(cached->IsUndefined());
    EXPECT_EQ(sendableModule->GetEcmaModuleFilename(), 0U);
    EXPECT_EQ(moduleManager->pendingRemovalModules_.Size(), 1U);
    EXPECT_EQ(moduleManager->TryGetPendingRemovalModule(recordName).GetTaggedValue(), normalModule.GetTaggedValue());
}

HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_RestoreModuleFromPending_DynamicCircularDependencies)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *factory = instance->GetFactory();
    CString recordNameA = "pending_dynamic_a";
    CString recordNameB = "pending_dynamic_b";
    CString recordNameStable = "pending_stable";
    JSHandle<SourceTextModule> moduleA = factory->NewSourceTextModule();
    JSHandle<SourceTextModule> moduleB = factory->NewSourceTextModule();
    JSHandle<SourceTextModule> stableModule = factory->NewSourceTextModule();
    moduleA->SetEcmaModuleRecordNameString(recordNameA);
    moduleB->SetEcmaModuleRecordNameString(recordNameB);
    stableModule->SetEcmaModuleRecordNameString(recordNameStable);
    moduleA->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    moduleB->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    stableModule->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
    moduleA->SetStatus(ModuleStatus::EVALUATED);
    moduleB->SetStatus(ModuleStatus::EVALUATED);
    moduleA->SetRegisterCounts(0);
    moduleB->SetRegisterCounts(0);
    stableModule->SetRegisterCounts(0);
    JSHandle<TaggedArray> requestedByA = factory->NewTaggedArray(2);
    requestedByA->Set(thread, 0, moduleB);
    requestedByA->Set(thread, 1, stableModule);
    moduleA->SetRequestedModules(thread, requestedByA.GetTaggedValue());
    JSHandle<TaggedArray> requestedByB = factory->NewTaggedArray(1);
    requestedByB->Set(thread, 0, moduleA);
    moduleB->SetRequestedModules(thread, requestedByB.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameA, moduleA.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameB, moduleB.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameStable, stableModule.GetTaggedValue());
    ModuleDeregister::RemoveModule(thread, moduleA);
    ModuleDeregister::RemoveModule(thread, moduleB);

    JSHandle<JSTaggedValue> restoredModuleA = moduleManager->TryGetPendingRemovalModule(recordNameA);
    ASSERT_EQ(restoredModuleA.GetTaggedValue(), moduleA.GetTaggedValue());
    ModuleDeregister::RestoreModuleFromPending(thread, restoredModuleA, ExecuteTypes::DYNAMIC);

    EXPECT_EQ(moduleA->GetRegisterCounts(), 1);
    EXPECT_EQ(moduleB->GetRegisterCounts(), 1);
    EXPECT_EQ(stableModule->GetRegisterCounts(), 0);
    EXPECT_EQ(moduleA->GetLoadingTypes(), LoadingTypes::DYNAMITC_MODULE);
    EXPECT_EQ(moduleB->GetLoadingTypes(), LoadingTypes::DYNAMITC_MODULE);
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameA));
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameB));
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameStable));
    EXPECT_EQ(moduleManager->pendingRemovalModules_.Size(), 0U);
}

HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_RestoreModuleFromPending_DiamondDependencyOnce)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *factory = instance->GetFactory();
    CString recordNameA = "pending_diamond_a";
    CString recordNameB = "pending_diamond_b";
    CString recordNameC = "pending_diamond_c";
    CString recordNameD = "pending_diamond_d";
    JSHandle<SourceTextModule> moduleA = CreateEvaluatedDynamicModule(recordNameA);
    JSHandle<SourceTextModule> moduleB = CreateEvaluatedDynamicModule(recordNameB);
    JSHandle<SourceTextModule> moduleC = CreateEvaluatedDynamicModule(recordNameC);
    JSHandle<SourceTextModule> moduleD = CreateEvaluatedDynamicModule(recordNameD);
    JSHandle<TaggedArray> requestedByA = factory->NewTaggedArray(2);
    requestedByA->Set(thread, 0, moduleB);
    requestedByA->Set(thread, 1, moduleC);
    moduleA->SetRequestedModules(thread, requestedByA.GetTaggedValue());
    JSHandle<TaggedArray> requestedByB = factory->NewTaggedArray(1);
    requestedByB->Set(thread, 0, moduleD);
    moduleB->SetRequestedModules(thread, requestedByB.GetTaggedValue());
    JSHandle<TaggedArray> requestedByC = factory->NewTaggedArray(1);
    requestedByC->Set(thread, 0, moduleD);
    moduleC->SetRequestedModules(thread, requestedByC.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameA, moduleA.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameB, moduleB.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameC, moduleC.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameD, moduleD.GetTaggedValue());
    ModuleDeregister::RemoveModule(thread, moduleA);
    ModuleDeregister::RemoveModule(thread, moduleB);
    ModuleDeregister::RemoveModule(thread, moduleC);
    ModuleDeregister::RemoveModule(thread, moduleD);

    JSHandle<JSTaggedValue> restoredModuleA = moduleManager->TryGetPendingRemovalModule(recordNameA);
    ASSERT_EQ(restoredModuleA.GetTaggedValue(), moduleA.GetTaggedValue());
    ModuleDeregister::RestoreModuleFromPending(thread, restoredModuleA, ExecuteTypes::DYNAMIC);

    EXPECT_EQ(moduleA->GetRegisterCounts(), 1);
    EXPECT_EQ(moduleB->GetRegisterCounts(), 1);
    EXPECT_EQ(moduleC->GetRegisterCounts(), 1);
    EXPECT_EQ(moduleD->GetRegisterCounts(), 1);
    EXPECT_EQ(moduleA->GetLoadingTypes(), LoadingTypes::DYNAMITC_MODULE);
    EXPECT_EQ(moduleB->GetLoadingTypes(), LoadingTypes::DYNAMITC_MODULE);
    EXPECT_EQ(moduleC->GetLoadingTypes(), LoadingTypes::DYNAMITC_MODULE);
    EXPECT_EQ(moduleD->GetLoadingTypes(), LoadingTypes::DYNAMITC_MODULE);
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameA));
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameB));
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameC));
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameD));
    EXPECT_EQ(moduleManager->pendingRemovalModules_.Size(), 0U);
}

HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_RestoreModuleFromPending_PropagatesPendingException)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *factory = instance->GetFactory();
    CString recordName = "pending_exception_parent";
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    JSHandle<SourceTextModule> requiredModule = factory->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString(recordName);
    requiredModule->SetEcmaModuleRecordNameString("pending_exception_dependency");
    module->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    requiredModule->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    module->SetRegisterCounts(0);
    requiredModule->SetRegisterCounts(0);
    JSHandle<TaggedArray> requestedModules = factory->NewTaggedArray(1);
    requestedModules->Set(thread, 0, requiredModule);
    module->SetRequestedModules(thread, requestedModules.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    ModuleDeregister::RemoveModule(thread, module);
    JSHandle<JSTaggedValue> restoredModule = moduleManager->TryGetPendingRemovalModule(recordName);
    ASSERT_EQ(restoredModule.GetTaggedValue(), module.GetTaggedValue());
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::ERROR, "pending exception", StackCheck::NO);
    thread->SetException(error.GetTaggedValue());

    ModuleDeregister::RestoreModuleFromPending(thread, restoredModule, ExecuteTypes::DYNAMIC);

    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(module->GetRegisterCounts(), 1);
    EXPECT_EQ(requiredModule->GetRegisterCounts(), 0);
    thread->ClearException();
}

HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_DecreaseRegisterCounts_SkipsPendingDependency)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *factory = instance->GetFactory();
    CString parentRecordName = "dynamic_parent";
    CString childRecordName = "dynamic_child";
    JSHandle<SourceTextModule> parentModule = factory->NewSourceTextModule();
    JSHandle<SourceTextModule> childModule = factory->NewSourceTextModule();
    parentModule->SetEcmaModuleRecordNameString(parentRecordName);
    childModule->SetEcmaModuleRecordNameString(childRecordName);
    parentModule->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    childModule->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    parentModule->SetStatus(ModuleStatus::EVALUATED);
    childModule->SetStatus(ModuleStatus::EVALUATED);
    parentModule->SetRegisterCounts(1);
    childModule->SetRegisterCounts(0);
    JSHandle<TaggedArray> requestedModules = factory->NewTaggedArray(1);
    requestedModules->Set(thread, 0, childModule);
    parentModule->SetRequestedModules(thread, requestedModules.GetTaggedValue());
    moduleManager->AddResolveImportedModule(parentRecordName, parentModule.GetTaggedValue());
    moduleManager->AddResolveImportedModule(childRecordName, childModule.GetTaggedValue());
    ModuleDeregister::RemoveModule(thread, childModule);
    std::set<CString> decreaseModules = {parentRecordName};

    ModuleDeregister::DecreaseRegisterCounts(thread, parentModule, decreaseModules);

    EXPECT_EQ(parentModule->GetRegisterCounts(), 0);
    EXPECT_EQ(childModule->GetRegisterCounts(), 0);
    EXPECT_FALSE(moduleManager->IsLocalModuleLoaded(parentRecordName));
    EXPECT_FALSE(moduleManager->IsLocalModuleLoaded(childRecordName));
    EXPECT_EQ(moduleManager->pendingRemovalModules_.Size(), 2U);
}

HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_FreeModuleRecord_ReleasesPendingModuleNameBuffer)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    NativeAreaAllocator *allocator = instance->GetNativeAreaAllocator();
    CString recordName = "pending_finalizer_module";
    JSHandle<SourceTextModule> module = instance->GetFactory()->NewSourceTextModule();
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    moduleManager->AddResolveImportedModule(recordName, module.GetTaggedValue());
    ModuleDeregister::RemoveModule(thread, module);
    size_t nativeMemoryUsageBefore = allocator->GetNativeMemoryUsage();
    size_t bufferSize = recordName.size() + 1;
    void *moduleNameBuffer = allocator->AllocateBuffer(bufferSize);
    ASSERT_NE(moduleNameBuffer, nullptr);
    ASSERT_EQ(memcpy_s(moduleNameBuffer, bufferSize, recordName.c_str(), bufferSize), EOK);
    EXPECT_GT(allocator->GetNativeMemoryUsage(), nativeMemoryUsageBefore);

    ModuleDeregister::FreeModuleRecord(nullptr, moduleNameBuffer, thread);

    EXPECT_EQ(allocator->GetNativeMemoryUsage(), nativeMemoryUsageBefore);
}

HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_ResetRegisterCounts_StabilizesSharedDynamicDependency)
{
    ModuleManager *moduleManager = thread->GetModuleManager();
    ObjectFactory *factory = instance->GetFactory();
    CString recordNameA = "reload_dynamic_a";
    CString recordNameB = "reload_dynamic_b";
    CString recordNameC = "reload_shared_dynamic_c";
    JSHandle<SourceTextModule> moduleA = CreateEvaluatedDynamicModule(recordNameA);
    JSHandle<SourceTextModule> moduleB = CreateEvaluatedDynamicModule(recordNameB);
    JSHandle<SourceTextModule> moduleC = CreateEvaluatedDynamicModule(recordNameC);
    JSHandle<TaggedArray> requestedByA = factory->NewTaggedArray(1);
    requestedByA->Set(thread, 0, moduleC);
    moduleA->SetRequestedModules(thread, requestedByA.GetTaggedValue());
    JSHandle<TaggedArray> requestedByB = factory->NewTaggedArray(1);
    requestedByB->Set(thread, 0, moduleC);
    moduleB->SetRequestedModules(thread, requestedByB.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameA, moduleA.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameB, moduleB.GetTaggedValue());
    moduleManager->AddResolveImportedModule(recordNameC, moduleC.GetTaggedValue());
    ModuleDeregister::RemoveModule(thread, moduleA);
    ModuleDeregister::RemoveModule(thread, moduleB);
    ModuleDeregister::RemoveModule(thread, moduleC);

    JSHandle<JSTaggedValue> restoredA = moduleManager->TryGetPendingRemovalModule(recordNameA);
    ModuleDeregister::RestoreModuleFromPending(thread, restoredA, ExecuteTypes::DYNAMIC);
    EXPECT_EQ(moduleC->GetRegisterCounts(), 1);

    JSHandle<JSTaggedValue> restoredB = moduleManager->TryGetPendingRemovalModule(recordNameB);
    ModuleDeregister::RestoreModuleFromPending(thread, restoredB, ExecuteTypes::DYNAMIC);

    EXPECT_EQ(moduleA->GetRegisterCounts(), 1);
    EXPECT_EQ(moduleB->GetRegisterCounts(), 1);
    EXPECT_EQ(moduleC->GetRegisterCounts(), 1);
    EXPECT_EQ(moduleC->GetLoadingTypes(), LoadingTypes::STABLE_MODULE);
    EXPECT_EQ(moduleManager->pendingRemovalModules_.Size(), 0U);

    std::set<CString> decreaseA = {recordNameA};
    ModuleDeregister::DecreaseRegisterCounts(thread, moduleA, decreaseA);
    EXPECT_EQ(moduleA->GetRegisterCounts(), 0);
    EXPECT_EQ(moduleC->GetRegisterCounts(), 1);
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameC));

    std::set<CString> decreaseB = {recordNameB};
    ModuleDeregister::DecreaseRegisterCounts(thread, moduleB, decreaseB);
    EXPECT_EQ(moduleB->GetRegisterCounts(), 0);
    EXPECT_EQ(moduleC->GetRegisterCounts(), 1);
    EXPECT_TRUE(moduleManager->IsLocalModuleLoaded(recordNameC));
    EXPECT_EQ(moduleManager->pendingRemovalModules_.Size(), 2U);
}
}  // namespace panda::test
