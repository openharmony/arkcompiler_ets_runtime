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

#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_module_manager.h"
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

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: ModuleDeregister_ReviseLoadedModuleCount_DynamicModuleNotInList
 * @tc.desc: Test ReviseLoadedModuleCount when dynamic module is not in deregister list
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_ReviseLoadedModuleCount_DynamicModuleNotInList)
{
    ObjectFactory *factory = instance->GetFactory();
    
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    module->SetRegisterCounts(1);
    
    CString moduleName = "test_dynamic_module";
    thread->GetModuleManager()->AddResolveImportedModule(moduleName, module.GetTaggedValue());
    
    ModuleDeregister::ReviseLoadedModuleCount(thread, moduleName);
    
    EXPECT_EQ(module->GetRegisterCounts(), 2);
}

/**
 * @tc.name: ModuleDeregister_ReviseLoadedModuleCount_ModuleNotLoaded
 * @tc.desc: Test ReviseLoadedModuleCount when module is not loaded
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_ReviseLoadedModuleCount_ModuleNotLoaded)
{
    CString moduleName = "non_existent_module";
    
    ModuleDeregister::ReviseLoadedModuleCount(thread, moduleName);
}

/**
 * @tc.name: ModuleDeregister_ReviseLoadedModuleCount_ModuleInDeregisterList
 * @tc.desc: Test ReviseLoadedModuleCount when module is already in deregister list
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_ReviseLoadedModuleCount_ModuleInDeregisterList)
{
    ObjectFactory *factory = instance->GetFactory();
    
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    module->SetRegisterCounts(1);
    
    CString moduleName = "test_module_in_list";
    thread->GetModuleManager()->AddResolveImportedModule(moduleName, module.GetTaggedValue());
    thread->GetEcmaVM()->PushToDeregisterModuleList(moduleName);
    
    ModuleDeregister::ReviseLoadedModuleCount(thread, moduleName);
    
    EXPECT_EQ(module->GetRegisterCounts(), 1);
}

/**
 * @tc.name: ModuleDeregister_ReviseLoadedModuleCount_StableModule
 * @tc.desc: Test ReviseLoadedModuleCount with stable module
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_ReviseLoadedModuleCount_StableModule)
{
    ObjectFactory *factory = instance->GetFactory();
    
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetLoadingTypes(LoadingTypes::STABLE_MODULE);
    module->SetRegisterCounts(1);
    
    CString moduleName = "test_stable_module";
    thread->GetModuleManager()->AddResolveImportedModule(moduleName, module.GetTaggedValue());
    
    ModuleDeregister::ReviseLoadedModuleCount(thread, moduleName);
    
    EXPECT_EQ(module->GetRegisterCounts(), 1);
}

/**
 * @tc.name: ModuleDeregister_IncreaseRegisterCounts_MaxCount
 * @tc.desc: Test IncreaseRegisterCounts when register count reaches UINT16_MAX
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_IncreaseRegisterCounts_MaxCount)
{
    ObjectFactory *factory = instance->GetFactory();
    
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    module->SetRegisterCounts(UINT16_MAX);
    
    std::set<CString> increaseModule = {"test_module"};
    
    ModuleDeregister::IncreaseRegisterCounts(thread, module, increaseModule);
    
    EXPECT_EQ(module->GetLoadingTypes(), LoadingTypes::STABLE_MODULE);
}

/**
 * @tc.name: ModuleDeregister_IncreaseRegisterCounts_NormalIncrement
 * @tc.desc: Test IncreaseRegisterCounts with normal increment
 * @tc.type: FUNC
 */
HWTEST_F_L0(ModuleDeregisterTest, ModuleDeregister_IncreaseRegisterCounts_NormalIncrement)
{
    ObjectFactory *factory = instance->GetFactory();
    
    JSHandle<SourceTextModule> module = factory->NewSourceTextModule();
    module->SetLoadingTypes(LoadingTypes::DYNAMITC_MODULE);
    module->SetRegisterCounts(10);
    
    std::set<CString> increaseModule = {"test_module"};
    
    ModuleDeregister::IncreaseRegisterCounts(thread, module, increaseModule);
    
    EXPECT_EQ(module->GetRegisterCounts(), 11);
}

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
}  // namespace panda::test
