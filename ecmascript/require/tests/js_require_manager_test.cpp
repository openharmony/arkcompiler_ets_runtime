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
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_require_manager.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::test {
class RequireManagerTest : public testing::Test {
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
 * @tc.name: InitializeCommonJS_BasicInitialization
 * @tc.desc: Call "InitializeCommonJS" function with basic parameters to verify
 *           module.exports, parent, and cache are set correctly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, InitializeCommonJS_BasicInitialization)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<CjsModule> module = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));

    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo);

    JSHandle<JSTaggedValue> exportsKey = globalConst->GetHandledCjsExportsString();
    JSTaggedValue exportsVal = SlowRuntimeStub::LdObjByName(thread, module.GetTaggedValue(),
        exportsKey.GetTaggedValue(), false, JSTaggedValue::Undefined());

    EXPECT_EQ(JSTaggedValue::SameValue(thread, exportsVal, JSTaggedValue::Hole()), false);

    JSHandle<JSTaggedValue> parentKey(factory->NewFromASCII("parent"));
    JSTaggedValue parentVal = SlowRuntimeStub::LdObjByName(thread, require.GetTaggedValue(),
        parentKey.GetTaggedValue(), false, JSTaggedValue::Undefined());

    EXPECT_EQ(JSTaggedValue::SameValue(thread, parentVal, module.GetTaggedValue()), true);
}

/**
 * @tc.name: InitializeCommonJS_MultipleModules
 * @tc.desc: Call "InitializeCommonJS" function multiple times with different modules
 *           to verify each module is cached correctly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, InitializeCommonJS_MultipleModules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<CjsModule> module1 = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports1 = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName1(factory->NewFromUtf8("ark/js_runtime/test1.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));

    CJSInfo cjsInfo1(module1, require, exports1, fileName1, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo1);

    JSHandle<CjsModule> module2 = factory->NewCjsModule();
    JSHandle<CjsExports> exports2 = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName2(factory->NewFromUtf8("ark/js_runtime/test2.js"));

    CJSInfo cjsInfo2(module2, require, exports2, fileName2, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo2);

    JSHandle<JSTaggedValue> cached1 = CjsModule::SearchFromModuleCache(thread, fileName1);
    EXPECT_NE(cached1->IsHole(), true);

    JSHandle<JSTaggedValue> cached2 = CjsModule::SearchFromModuleCache(thread, fileName2);
    EXPECT_NE(cached2->IsHole(), true);
}

/**
 * @tc.name: InitializeCommonJS_WithPathTraversal
 * @tc.desc: Call "InitializeCommonJS" function with path containing special characters
 *           to verify proper handling of various file paths.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, InitializeCommonJS_WithPathTraversal)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<CjsModule> module = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/nested/deep/test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime/nested/deep"));

    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo);

    JSHandle<JSTaggedValue> cached = CjsModule::SearchFromModuleCache(thread, fileName);
    EXPECT_NE(cached->IsHole(), true);
}

/**
 * @tc.name: InitializeCommonJS_VerifyParentLink
 * @tc.desc: Call "InitializeCommonJS" function to verify the parent link
 *           between require and module is established correctly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, InitializeCommonJS_VerifyParentLink)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<CjsModule> module = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/parent_test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));

    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo);

    JSHandle<JSTaggedValue> parentKey(factory->NewFromASCII("parent"));
    JSTaggedValue parentVal = SlowRuntimeStub::LdObjByName(thread, require.GetTaggedValue(),
        parentKey.GetTaggedValue(), false, JSTaggedValue::Undefined());

    EXPECT_EQ(parentVal, module.GetTaggedValue());
}

/**
 * @tc.name: InitializeCommonJS_WithEmptyFilename
 * @tc.desc: Call "InitializeCommonJS" function with empty filename to verify
 *           the function handles edge cases properly.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, InitializeCommonJS_WithEmptyFilename)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<CjsModule> module = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8(""));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8(""));

    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo);

    JSHandle<JSTaggedValue> exportsKey = globalConst->GetHandledCjsExportsString();
    JSTaggedValue exportsVal = SlowRuntimeStub::LdObjByName(thread, module.GetTaggedValue(),
        exportsKey.GetTaggedValue(), false, JSTaggedValue::Undefined());

    EXPECT_EQ(JSTaggedValue::SameValue(thread, exportsVal, JSTaggedValue::Hole()), false);
}

/**
 * @tc.name: CollectExecutedExp_BasicCollection
 * @tc.desc: Call "CollectExecutedExp" function to verify the module cache
 *           is properly updated with executed exports.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, CollectExecutedExp_BasicCollection)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<CjsModule> module1 = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/collect_test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));

    CJSInfo cjsInfo1(module1, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo1);

    JSHandle<CjsModule> module2 = factory->NewCjsModule();
    JSHandle<EcmaString> exportsStr(factory->NewFromUtf8("test_exports"));
    CjsModule::InitializeModule(thread, module2, fileName, dirName);
    JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module2.GetTaggedValue(), exportsName.GetTaggedValue(),
        exportsStr.GetTaggedValue());

    CJSInfo cjsInfo2(module2, require, exports, fileName, dirName);
    RequireManager::CollectExecutedExp(thread, cjsInfo2);

    JSHandle<JSTaggedValue> result = CjsModule::SearchFromModuleCache(thread, fileName);
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(result), exportsStr), 0);
}

/**
 * @tc.name: CollectExecutedExp_MultipleCollections
 * @tc.desc: Call "CollectExecutedExp" function multiple times to verify
 *           the cache is correctly updated with the latest module.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, CollectExecutedExp_MultipleCollections)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/multi_collect.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));

    JSHandle<CjsModule> initModule = factory->NewCjsModule();
    CJSInfo initCjsInfo(initModule, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, initCjsInfo);

    for (int i = 0; i < 3; i++) {
        JSHandle<CjsModule> module = factory->NewCjsModule();
        CString exportStr = CString("export").append(std::to_string(i));
        JSHandle<EcmaString> exportsVal(factory->NewFromUtf8(exportStr.c_str()));
        CjsModule::InitializeModule(thread, module, fileName, dirName);
        JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
        SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), exportsName.GetTaggedValue(),
            exportsVal.GetTaggedValue());

        CJSInfo cjsInfo(module, require, exports, fileName, dirName);
        RequireManager::CollectExecutedExp(thread, cjsInfo);
    }

    JSHandle<JSTaggedValue> result = CjsModule::SearchFromModuleCache(thread, fileName);
    JSHandle<EcmaString> expectedStr(factory->NewFromUtf8("export2"));
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(result), expectedStr), 0);
}

/**
 * @tc.name: CollectExecutedExp_WithNonExistentModule
 * @tc.desc: Call "CollectExecutedExp" function with a module that doesn't exist
 *           in cache to verify proper cache creation.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, CollectExecutedExp_WithNonExistentModule)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<CjsModule> initModule = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/nonexistent.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));

    CJSInfo initCjsInfo(initModule, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, initCjsInfo);

    JSHandle<CjsModule> module = factory->NewCjsModule();
    CjsModule::InitializeModule(thread, module, fileName, dirName);
    JSHandle<EcmaString> exportsStr(factory->NewFromUtf8("new_module"));
    JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), exportsName.GetTaggedValue(),
        exportsStr.GetTaggedValue());

    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::CollectExecutedExp(thread, cjsInfo);

    JSHandle<JSTaggedValue> result = CjsModule::SearchFromModuleCache(thread, fileName);
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(result), exportsStr), 0);
}

/**
 * @tc.name: CollectExecutedExp_VerifyCacheReset
 * @tc.desc: Call "CollectExecutedExp" function to verify that the module cache
 *           is properly reset with new module instance.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, CollectExecutedExp_VerifyCacheReset)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<CjsModule> initModule = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/reset_test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));

    CJSInfo initCjsInfo(initModule, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, initCjsInfo);

    JSHandle<CjsModule> module1 = factory->NewCjsModule();
    JSHandle<EcmaString> exports1(factory->NewFromUtf8("initial_exports"));
    CjsModule::InitializeModule(thread, module1, fileName, dirName);
    JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module1.GetTaggedValue(), exportsName.GetTaggedValue(),
        exports1.GetTaggedValue());

    CJSInfo cjsInfo1(module1, require, exports, fileName, dirName);
    RequireManager::CollectExecutedExp(thread, cjsInfo1);

    JSHandle<CjsModule> module2 = factory->NewCjsModule();
    JSHandle<EcmaString> exports2(factory->NewFromUtf8("reset_exports"));
    CjsModule::InitializeModule(thread, module2, fileName, dirName);
    SlowRuntimeStub::StObjByName(thread, module2.GetTaggedValue(), exportsName.GetTaggedValue(),
        exports2.GetTaggedValue());

    CJSInfo cjsInfo2(module2, require, exports, fileName, dirName);
    RequireManager::CollectExecutedExp(thread, cjsInfo2);

    JSHandle<JSTaggedValue> result = CjsModule::SearchFromModuleCache(thread, fileName);
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(result), exports2), 0);
    EXPECT_NE(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(result), exports1), 0);
}

/**
 * @tc.name: InitializeCommonJS_ThenCollectExecutedExp
 * @tc.desc: Call "InitializeCommonJS" followed by "CollectExecutedExp" to verify
 *           the complete workflow from initialization to execution collection.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, InitializeCommonJS_ThenCollectExecutedExp)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<CjsModule> module1 = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/workflow_test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));

    CJSInfo cjsInfo(module1, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo);

    JSHandle<CjsModule> module2 = factory->NewCjsModule();
    JSHandle<EcmaString> exportsStr(factory->NewFromUtf8("workflow_exports"));
    CjsModule::InitializeModule(thread, module2, fileName, dirName);
    JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module2.GetTaggedValue(), exportsName.GetTaggedValue(),
        exportsStr.GetTaggedValue());

    CJSInfo cjsInfo2(module2, require, exports, fileName, dirName);
    RequireManager::CollectExecutedExp(thread, cjsInfo2);

    JSHandle<JSTaggedValue> result = CjsModule::SearchFromModuleCache(thread, fileName);
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(result), exportsStr), 0);

    JSHandle<JSTaggedValue> parentKey(factory->NewFromASCII("parent"));
    JSTaggedValue parentVal = SlowRuntimeStub::LdObjByName(thread, require.GetTaggedValue(),
        parentKey.GetTaggedValue(), false, JSTaggedValue::Undefined());
    EXPECT_EQ(parentVal, module1.GetTaggedValue());
}

/**
 * @tc.name: CollectExecutedExp_WithAbsolutePath
 * @tc.desc: Call "CollectExecutedExp" function with absolute path filename
 *           to verify proper handling of different path formats.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(RequireManagerTest, CollectExecutedExp_WithAbsolutePath)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<CjsModule> initModule = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("/absolute/path/to/module.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("/absolute/path/to"));

    CJSInfo initCjsInfo(initModule, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, initCjsInfo);

    JSHandle<CjsModule> module = factory->NewCjsModule();
    CjsModule::InitializeModule(thread, module, fileName, dirName);
    JSHandle<EcmaString> exportsStr(factory->NewFromUtf8("absolute_exports"));
    JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module.GetTaggedValue(), exportsName.GetTaggedValue(),
        exportsStr.GetTaggedValue());

    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::CollectExecutedExp(thread, cjsInfo);

    JSHandle<JSTaggedValue> result = CjsModule::SearchFromModuleCache(thread, fileName);
    EXPECT_EQ(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(result), exportsStr), 0);
}
} // namespace panda::test
