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
class CjsModuleTest : public testing::Test {
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
 * @tc.name: SearchFromModuleCache
 * @tc.desc: Call "SearchFromModuleCache" function: return the corresponding module.exports,
 * if Module(store in GlobalEnv) contains the module which named "xxx". Else, return Hole.
 * by checking returned value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(CjsModuleTest, SearchFromModuleCache)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto vm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<CjsModuleCache> cache = CjsModuleCache::Create(thread, CjsModuleCache::DEAULT_DICTIONART_CAPACITY);
    JSHandle<CjsModule> module1 = factory->NewCjsModule();
    JSHandle<CjsModule> module2 = factory->NewCjsModule();
    JSHandle<JSTaggedValue> fileName1(factory->NewFromUtf8("ark/js_runtime/test1.js"));
    JSHandle<JSTaggedValue> fileName2(factory->NewFromUtf8("ark/js_runtime/test2.js"));
    JSHandle<JSTaggedValue> fileName3(factory->NewFromUtf8("ark/js_runtime/test3.js"));
    JSHandle<EcmaString> exports(factory->NewFromUtf8("export string"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));
    CjsModule::InitializeModule(thread, module1, fileName1, dirName);
    CjsModule::InitializeModule(thread, module2, fileName2, dirName);
    JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module1.GetTaggedValue(), exportsName.GetTaggedValue(),
                                 exports.GetTaggedValue());
    SlowRuntimeStub::StObjByName(thread, module2.GetTaggedValue(), exportsName.GetTaggedValue(),
                                 exports.GetTaggedValue());

    cache = CjsModuleCache::PutIfAbsentAndReset(thread, cache, fileName1, JSHandle<JSTaggedValue>(module1));
    cache = CjsModuleCache::PutIfAbsentAndReset(thread, cache, fileName2, JSHandle<JSTaggedValue>(module2));
    JSHandle<JSTaggedValue> moduleObj(env->GetCjsModuleFunction());
    JSHandle<JSTaggedValue> cacheName = globalConst->GetHandledCjsCacheString();
    SlowRuntimeStub::StObjByName(thread, moduleObj.GetTaggedValue(), cacheName.GetTaggedValue(),
                                 cache.GetTaggedValue());
    JSHandle<JSTaggedValue> test1 = CjsModule::SearchFromModuleCache(thread, fileName1);
    EXPECT_TRUE(test1->IsString());
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(exports), JSHandle<EcmaString>(test1)), 0);

    JSHandle<JSTaggedValue> test2 = CjsModule::SearchFromModuleCache(thread, fileName3);
    EXPECT_TRUE(test2->IsHole());
}

/**
 * @tc.name: PutIntoCache
 * @tc.desc: Call "PutIntoCache" function, put module into Module(store in GlobalEnv).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(CjsModuleTest, PutIntoCache)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<CjsModule> module = factory->NewCjsModule();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("./ark/js_runtime"));
    JSHandle<EcmaString> exports(factory->NewFromUtf8("test"));
    CjsModule::InitializeModule(thread, module, fileName, dirName);

    JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), exportsName.GetTaggedValue(),
                                 exports.GetTaggedValue());
    CjsModule::PutIntoCache(thread, module, fileName);
    JSHandle<JSTaggedValue> test = CjsModule::SearchFromModuleCache(thread, fileName);
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, exports, JSHandle<EcmaString>(test)), 0);
}
} // namespace panda::test
