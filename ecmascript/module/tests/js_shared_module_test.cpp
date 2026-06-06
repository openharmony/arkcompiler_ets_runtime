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

#include "ecmascript/module/js_shared_module.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {

class ScopedDisableForceGC final {
   public:
    explicit ScopedDisableForceGC(EcmaVM *vm) : vm_(vm), oldEnableForceGC_(vm->GetJSOptions().EnableForceGC())
    {
        vm_->SetEnableForceGC(false);
    }

    ~ScopedDisableForceGC() { vm_->SetEnableForceGC(oldEnableForceGC_); }

   private:
    EcmaVM *vm_{nullptr};
    bool oldEnableForceGC_{true};
};

class JSSharedModuleTest : public testing::Test {
   public:
    static void SetUpTestCase() { GTEST_LOG_(INFO) << "SetUpTestCase"; }

    static void TearDownTestCase() { GTEST_LOG_(INFO) << "TearDownCase"; }

    void SetUp() override { TestHelper::CreateEcmaVMWithScope(instance, thread, scope); }

    void TearDown() override { TestHelper::DestroyEcmaVMWithScope(instance, scope); }

    EcmaVM *instance{nullptr};
    ecmascript::EcmaHandleScope *scope{nullptr};
    JSThread *thread{nullptr};
};

/**
 * @tc.name: SendableClassModule_CloneModuleEnvironment_ResolvedBindingRecord
 * @tc.desc: Test CloneModuleEnvironment with RESOLVEDBINDING_RECORD type (switch branch line 96)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, SendableClassModule_CloneModuleEnvironment_ResolvedBindingRecord)
{
    ObjectFactory *factory = instance->GetFactory();

    // Create a shared module
    JSHandle<SourceTextModule> sharedModule = factory->NewSSourceTextModule();
    sharedModule->SetSharedType(SharedTypes::SHARED_MODULE);
    sharedModule->SetStatus(ModuleStatus::EVALUATED);
    sharedModule->SetTypes(ModuleTypes::ECMA_MODULE);
    CString moduleName = "sharedTestModule";
    sharedModule->SetEcmaModuleRecordNameString(moduleName);

    // Create a binding name
    JSHandle<JSTaggedValue> bindingName(factory->NewFromASCII("sharedExport"));

    // Create a ResolvedBinding with shared module
    JSHandle<ResolvedBinding> resolvedBinding = factory->NewResolvedBindingRecord(sharedModule, bindingName);

    // Create module environment with the ResolvedBinding
    JSHandle<TaggedArray> moduleEnvironment = factory->NewTaggedArray(1);
    moduleEnvironment->Set(thread, 0, resolvedBinding.GetTaggedValue());

    // Call CloneModuleEnvironment which internally calls CloneRecordNameBinding
    JSHandle<TaggedArray> clonedEnv =
        SendableClassModule::CloneModuleEnvironment(thread, JSHandle<JSTaggedValue>::Cast(moduleEnvironment));

    // Verify the cloned environment is valid
    EXPECT_TRUE(clonedEnv->GetLength() == 1);
    // The result should be a ResolvedBinding (SResolvedBindingRecord) for shared module
    JSTaggedValue clonedBinding = clonedEnv->Get(thread, 0);
    EXPECT_TRUE(clonedBinding.IsResolvedBinding());
}

/**
 * @tc.name: SendableClassModule_CloneRecordNameBinding_NonSharedModule
 * @tc.desc: Test CloneRecordNameBinding when resolvedModule is not a shared module (branch line 75-78)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, SendableClassModule_CloneRecordNameBinding_NonSharedModule)
{
    ObjectFactory *factory = instance->GetFactory();
    ScopedDisableForceGC disableForceGC(instance);
    // Create a non-shared module (UNSENDABLE_MODULE)
    JSHandle<SourceTextModule> nonSharedModule = factory->NewSourceTextModule();
    nonSharedModule->SetSharedType(SharedTypes::UNSENDABLE_MODULE);
    nonSharedModule->SetStatus(ModuleStatus::EVALUATED);
    nonSharedModule->SetTypes(ModuleTypes::ECMA_MODULE);
    CString moduleName = "nonSharedTestModule";
    CString moduleFileName = "nonSharedTestModule.js";
    nonSharedModule->SetEcmaModuleRecordNameString(moduleName);
    nonSharedModule->SetEcmaModuleFilenameString(moduleFileName);

    // Create a binding name
    JSHandle<JSTaggedValue> bindingName(factory->NewFromASCII("nonSharedExport"));

    // Create a ResolvedBinding with non-shared module
    JSHandle<ResolvedBinding> resolvedBinding = factory->NewResolvedBindingRecord(nonSharedModule, bindingName);

    // Create module environment with the ResolvedBinding
    JSHandle<TaggedArray> moduleEnvironment = factory->NewTaggedArray(1);
    moduleEnvironment->Set(thread, 0, resolvedBinding.GetTaggedValue());

    // Call CloneModuleEnvironment which internally calls CloneRecordNameBinding
    JSHandle<TaggedArray> clonedEnv =
        SendableClassModule::CloneModuleEnvironment(thread, JSHandle<JSTaggedValue>::Cast(moduleEnvironment));

    // Verify the cloned environment is valid
    EXPECT_TRUE(clonedEnv->GetLength() == 1);
    // The result should be a ResolvedRecordBinding for non-shared module
    JSTaggedValue clonedBinding = clonedEnv->Get(thread, 0);
    EXPECT_TRUE(clonedBinding.IsResolvedRecordBinding());
}

/**
 * @tc.name: JSSharedModule_GenerateSharedExports_EmptyExports
 * @tc.desc: Test GenerateSharedExports when exports array is empty (branch line 153)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, JSSharedModule_GenerateSharedExports_EmptyExports)
{
    ObjectFactory *factory = instance->GetFactory();

    // Create a shared module
    JSHandle<SourceTextModule> sharedModule = factory->NewSSourceTextModule();
    sharedModule->SetSharedType(SharedTypes::SHARED_MODULE);
    sharedModule->SetStatus(ModuleStatus::EVALUATED);
    sharedModule->SetTypes(ModuleTypes::ECMA_MODULE);
    CString moduleName = "emptyExportsModule";
    sharedModule->SetEcmaModuleRecordNameString(moduleName);

    // Create an empty exports array to trigger the branch at line 153
    JSHandle<TaggedArray> emptyExports = factory->NewTaggedArray(0);

    // Call SModuleNamespaceCreate which internally calls GenerateSharedExports
    // This will trigger the if (newLength == 0) branch at line 153
    JSHandle<ModuleNamespace> namespaceObj =
        JSSharedModule::SModuleNamespaceCreate(thread, JSHandle<JSTaggedValue>::Cast(sharedModule), emptyExports);

    // Verify the namespace was created properly
    EXPECT_TRUE(JSHandle<JSTaggedValue>::Cast(namespaceObj)->IsModuleNamespace());

    // Verify exports is set (should be an empty sorted array)
    JSTaggedValue exports = namespaceObj->GetExports(thread);
    EXPECT_FALSE(exports.IsUndefined());
}

/**
 * @tc.name: GenerateSendableFuncModule_NonSourceTextModule
 * @tc.desc: Test GenerateSendableFuncModule with non-SourceTextModule input (line 27-29)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, GenerateSendableFuncModule_NonSourceTextModule)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<JSTaggedValue> recordName = JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("test_record"));
    JSHandle<JSTaggedValue> result = SendableClassModule::GenerateSendableFuncModule(thread, recordName);
    EXPECT_TRUE(result->IsString());
}

/**
 * @tc.name: GenerateSendableFuncModule_ModuleInSharedHeap
 * @tc.desc: Test GenerateSendableFuncModule when module is already in shared heap (line 33-35)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, GenerateSendableFuncModule_ModuleInSharedHeap)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<SourceTextModule> sharedModule = factory->NewSSourceTextModule();
    sharedModule->SetSharedType(SharedTypes::SHARED_MODULE);
    JSHandle<JSTaggedValue> result =
        SendableClassModule::GenerateSendableFuncModule(thread, JSHandle<JSTaggedValue>::Cast(sharedModule));
    EXPECT_EQ(result.GetTaggedValue(), sharedModule.GetTaggedValue());
}

/**
 * @tc.name: CloneModuleEnvironment_UndefinedEnvironment
 * @tc.desc: Test CloneModuleEnvironment with undefined environment (line 99-101)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, CloneModuleEnvironment_UndefinedEnvironment)
{
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<TaggedArray> result = SendableClassModule::CloneModuleEnvironment(thread, undefined);
    EXPECT_TRUE(result.GetTaggedValue().IsUndefined());
}

/**
 * @tc.name: CloneModuleEnvironment_ResolvedIndexBinding
 * @tc.desc: Test CloneModuleEnvironment with RESOLVEDINDEXBINDING_RECORD (line 116-119)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, CloneModuleEnvironment_ResolvedIndexBinding)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<SourceTextModule> sharedModule = factory->NewSSourceTextModule();
    sharedModule->SetSharedType(SharedTypes::SHARED_MODULE);
    JSHandle<ResolvedIndexBinding> binding = factory->NewResolvedIndexBindingRecord(sharedModule, 0);
    JSHandle<TaggedArray> env = factory->NewTaggedArray(1);
    env->Set(thread, 0, binding.GetTaggedValue());
    JSHandle<TaggedArray> result =
        SendableClassModule::CloneModuleEnvironment(thread, JSHandle<JSTaggedValue>::Cast(env));
    EXPECT_EQ(result->GetLength(), 1U);
    EXPECT_TRUE(result->Get(thread, 0).IsResolvedIndexBinding());
}

/**
 * @tc.name: CloneModuleEnvironment_ResolvedRecordIndexBinding
 * @tc.desc: Test CloneModuleEnvironment with RESOLVEDRECORDINDEXBINDING_RECORD (line 121-122)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, CloneModuleEnvironment_ResolvedRecordIndexBinding)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<EcmaString> record = factory->NewFromASCII("recIdxModule");
    JSHandle<EcmaString> fileName = factory->NewFromASCII("recIdxModule.js");
    JSHandle<JSTaggedValue> binding = JSHandle<JSTaggedValue>::Cast(
        factory->NewSResolvedRecordIndexBindingRecord(record, fileName, 0));
    JSHandle<TaggedArray> env = factory->NewTaggedArray(1);
    env->Set(thread, 0, binding.GetTaggedValue());
    JSHandle<TaggedArray> result =
        SendableClassModule::CloneModuleEnvironment(thread, JSHandle<JSTaggedValue>::Cast(env));
    EXPECT_EQ(result->GetLength(), 1U);
}

/**
 * @tc.name: CloneModuleEnvironment_NonSharedIndexBinding
 * @tc.desc: Test CloneModuleEnvironment indirectly covers CloneRecordIndexBinding non-shared branch
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, CloneModuleEnvironment_NonSharedIndexBinding)
{
    ScopedDisableForceGC disableForceGC(instance);
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<SourceTextModule> nonSharedModule = factory->NewSourceTextModule();
    nonSharedModule->SetEcmaModuleRecordNameString("nonSharedIdxMod");
    nonSharedModule->SetEcmaModuleFilenameString("nonSharedIdxMod.js");
    JSHandle<ResolvedIndexBinding> binding = factory->NewResolvedIndexBindingRecord(nonSharedModule, 3);
    JSHandle<TaggedArray> env = factory->NewTaggedArray(1);
    env->Set(thread, 0, binding.GetTaggedValue());
    JSHandle<TaggedArray> result =
        SendableClassModule::CloneModuleEnvironment(thread, JSHandle<JSTaggedValue>::Cast(env));
    EXPECT_EQ(result->GetLength(), 1U);
    JSTaggedValue clonedBinding = result->Get(thread, 0);
    EXPECT_TRUE(clonedBinding.IsRecord());
}

/**
 * @tc.name: CloneModuleEnvironment_ResolvedRecordBinding
 * @tc.desc: Test CloneModuleEnvironment with RESOLVEDRECORDBINDING_RECORD (line 123-124)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, CloneModuleEnvironment_ResolvedRecordBinding)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<EcmaString> record = factory->NewFromASCII("recBindModule");
    JSHandle<JSTaggedValue> bindingName(factory->NewFromASCII("bindName"));
    JSHandle<JSTaggedValue> binding = JSHandle<JSTaggedValue>::Cast(
        factory->NewSResolvedRecordBindingRecord(record, bindingName));
    JSHandle<TaggedArray> env = factory->NewTaggedArray(1);
    env->Set(thread, 0, binding.GetTaggedValue());
    JSHandle<TaggedArray> result =
        SendableClassModule::CloneModuleEnvironment(thread, JSHandle<JSTaggedValue>::Cast(env));
    EXPECT_EQ(result->GetLength(), 1U);
}

/**
 * @tc.name: CloneEnvForSModule_NonSharedModule
 * @tc.desc: Test CloneEnvForSModule with non-shared module returns original env (line 137-139)
 * @tc.type: FUNC
 */
HWTEST_F_L0(JSSharedModuleTest, CloneEnvForSModule_NonSharedModule)
{
    ObjectFactory *factory = instance->GetFactory();
    JSHandle<SourceTextModule> nonSharedModule = factory->NewSourceTextModule();
    nonSharedModule->SetSharedType(SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> envRec = factory->NewTaggedArray(2);
    JSHandle<TaggedArray> result = JSSharedModule::CloneEnvForSModule(thread, nonSharedModule, envRec);
    EXPECT_EQ(result.GetTaggedValue(), envRec.GetTaggedValue());
}

}  // namespace panda::test
