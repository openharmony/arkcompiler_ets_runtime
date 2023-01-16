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
class CjsManagerTest : public testing::Test {
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
 * @tc.name: InitializeCommonJS
 * @tc.desc: Call "InitializeCommonJS" function, connect relationship between objects.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(CjsManagerTest, InitializeCommonJS)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto *globalConst = thread->GlobalConstants();

    JSHandle<CjsModule> module = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));
    CJSInfo cjsInfo(module, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo);

    JSHandle<JSTaggedValue> exportsKey = globalConst->GetHandledCjsExportsString();
    JSTaggedValue exportsVal =
        SlowRuntimeStub::LdObjByName(thread, module.GetTaggedValue(),
                                     exportsKey.GetTaggedValue(), false, JSTaggedValue::Undefined());

    EXPECT_EQ(JSTaggedValue::SameValue(exportsVal, JSTaggedValue::Hole()), false);
}

/**
 * @tc.name: CollectExecutedExp
 * @tc.desc: Call "CollectExecutedExp" function,check whether the same-name module is replaced.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(CjsManagerTest, CollectExecutedExp)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto *globalConst = thread->GlobalConstants();
    auto vm = thread->GetEcmaVM();

    JSHandle<CjsModule> module1 = factory->NewCjsModule();
    JSHandle<JSTaggedValue> require = thread->GetEcmaVM()->GetGlobalEnv()->GetCjsRequireFunction();
    JSHandle<CjsExports> exports = factory->NewCjsExports();
    JSHandle<JSTaggedValue> fileName(factory->NewFromUtf8("ark/js_runtime/test.js"));
    JSHandle<JSTaggedValue> dirName(factory->NewFromUtf8("ark/js_runtime"));
    CJSInfo cjsInfo1(module1, require, exports, fileName, dirName);
    RequireManager::InitializeCommonJS(thread, cjsInfo1);

    JSHandle<CjsModule> module2 = factory->NewCjsModule();
    JSHandle<EcmaString> exportsStr(factory->NewFromUtf8("test"));
    CjsModule::InitializeModule(thread, module2, fileName, dirName);
    JSHandle<JSTaggedValue> exportsName = globalConst->GetHandledCjsExportsString();
    SlowRuntimeStub::StObjByName(thread, module2.GetTaggedValue(), exportsName.GetTaggedValue(),
                                 exportsStr.GetTaggedValue());

    CJSInfo cjsInfo2(module2, require, exports, fileName, dirName);
    RequireManager::CollectExecutedExp(thread, cjsInfo2);
    JSHandle<JSTaggedValue> test = CjsModule::SearchFromModuleCache(thread, fileName);

    EXPECT_EQ(EcmaStringAccessor::Compare(vm, JSHandle<EcmaString>(test), exportsStr), 0);
}
} // namespace panda::test
