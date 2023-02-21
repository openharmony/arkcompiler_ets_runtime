/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::test {
class CjsModuleCacheTest : public testing::Test {
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
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: PutIfAbsent
 * @tc.desc: Call "PutIfAbsent" function, check whether the module is loaded successfully into the CjsModuleCache.
 * by checking returned value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(CjsModuleCacheTest, PutIfAbsent)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto vm = thread->GetEcmaVM();
    JSHandle<CjsModuleCache> cache = CjsModuleCache::Create(thread, CjsModuleCache::DEAULT_DICTIONART_CAPACITY);

    JSHandle<CjsModule> module = factory->NewCjsModule();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));
    CjsModule::InitializeModule(thread, module, fileName, dirName);
    JSHandle<CjsModuleCache> newCacheTested = CjsModuleCache::PutIfAbsentAndReset(thread, cache,
                                              JSHandle<JSTaggedValue>::Cast(fileName),
                                              JSHandle<JSTaggedValue>(module));
    JSHandle<CjsModule> moduleTested = JSHandle<CjsModule>(thread,
                                           newCacheTested->GetModule(fileName.GetTaggedValue()));

    EXPECT_TRUE(newCacheTested->ContainsModule(fileName.GetTaggedValue()));
    JSHandle<EcmaString> fileNameTested(thread, moduleTested->GetFilename());
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, fileNameTested, JSHandle<EcmaString>(fileName)), 0);
}

/**
 * @tc.name: ResetModule
 * @tc.desc: Call "ResetModule" function,check whether the same-name module is replaced.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(CjsModuleCacheTest, ResetModule)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto vm = thread->GetEcmaVM();
    JSHandle<CjsModuleCache> cache = CjsModuleCache::Create(thread, CjsModuleCache::DEAULT_DICTIONART_CAPACITY);
    JSHandle<CjsModule> module = factory->NewCjsModule();
    JSHandle<CjsModule> moduleWithNewExp = factory->NewCjsModule();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("./ark/js_runtime"));
    JSHandle<EcmaString> test(factory->NewFromUtf8("test"));
    CjsModule::InitializeModule(thread, module, fileName, dirName);
    CjsModule::InitializeModule(thread, moduleWithNewExp, fileName, dirName);
    moduleWithNewExp->SetExports(thread, test);
    JSHandle<CjsModuleCache> newCache = CjsModuleCache::PutIfAbsentAndReset(thread, cache,
                                        JSHandle<JSTaggedValue>::Cast(fileName),
                                        JSHandle<JSTaggedValue>(module));
    JSHandle<CjsModuleCache> newCacheTested = CjsModuleCache::ResetModule(thread, newCache, fileName,
                                                  JSHandle<JSTaggedValue>::Cast(moduleWithNewExp));
    JSHandle<CjsModule> moduleTested = JSHandle<CjsModule>(thread,
                                           newCacheTested->GetModule(fileName.GetTaggedValue()));

    JSHandle<EcmaString> exportsTested(thread, moduleTested->GetExports());
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, exportsTested, test), 0);
}
} // namespace panda::test
