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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/containers_hashmap.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/containers/tests/containers_test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersHashMapTest : public testing::Test {
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

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> map = GetCallArg(argv, 2); // 2 means the secode arg
            if (!map->IsUndefined()) {
                if (value->IsNumber()) {
                    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                    JSAPIHashMap::Set(thread, JSHandle<JSAPIHashMap>::Cast(map), key, newValue);
                }
            }
            JSHandle<JSAPIHashMap> jsHashMap(GetThis(argv));
            JSAPIHashMap::Set(thread, jsHashMap, key, value);
            return JSTaggedValue::Undefined();
        }
    };
protected:
    JSTaggedValue InitializeHashMapConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::HashMap)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIHashMap> CreateJSAPIHashMap()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeHashMapConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersHashMap::HashMapConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIHashMap> map(thread, result);
        return map;
    }
};

// new HashMap()
HWTEST_F_L0(ContainersHashMapTest, HashMapConstructor)
{
    InitializeHashMapConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeHashMapConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersHashMap::HashMapConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIHashMap());
    JSHandle<JSAPIHashMap> mapHandle(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(mapHandle));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
    int size = mapHandle->GetSize();
    ASSERT_EQ(size, 0);

    // test HashMapConstructor exception
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersHashMap, HashMapConstructor, objCallInfo);
}

// hashmap.set(key, value), hashmap.get(key)
HWTEST_F_L0(ContainersHashMapTest, SetAndGet)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }
}

// hashmap.hasKey(key), hashmap.hasValue(value)
HWTEST_F_L0(ContainersHashMapTest, HasKeyAndHasValue)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        // test hasKey
        {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tMap.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersHashMap::HasKey(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }
        // test hasValue
        {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tMap.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersHashMap::HasValue(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        // test hasKey
        {
            std::string iKey = myKey + std::to_string(i);
            key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tMap.GetTaggedValue());
            callInfo->SetCallArg(0, key.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersHashMap::HasKey(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }
        // test hasValue
        {
            std::string iValue = myValue + std::to_string(i);
            value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(tMap.GetTaggedValue());
            callInfo->SetCallArg(0, value.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result = ContainersHashMap::HasValue(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue::True());
        }
    }
}

// hashmap.keys(), hashmap.values(), hashmap.entries()
HWTEST_F_L0(ContainersHashMapTest, KeysAndValuesAndEntries)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result2 = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result2.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result2.GetTaggedObject())->GetSize(), i + 1);
    }

    // test keys
    auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(tMap.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo1);
    JSHandle<JSTaggedValue> iterKeys(thread, ContainersHashMap::Keys(callInfo1));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iterKeys->IsJSAPIHashMapIterator());

    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterKeys.GetTaggedValue());

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPIHashMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev1);
        JSHandle<JSTaggedValue> iterKey = JSIterator::IteratorValue(thread, result);
        JSTaggedValue keyFlag = tMap->HasKey(thread, iterKey.GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::True(), keyFlag);
    }

    // test values
    callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(tMap.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, callInfo1);
    JSHandle<JSTaggedValue> iterValues(thread, ContainersHashMap::Values(callInfo1));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iterValues->IsJSAPIHashMapIterator());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPIHashMapIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev2);
        JSHandle<JSTaggedValue> iterValue = JSIterator::IteratorValue(thread, result);
        JSTaggedValue valueFlag = JSAPIHashMap::HasValue(thread, tMap, iterValue);
        EXPECT_EQ(JSTaggedValue::True(), valueFlag);
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev3 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result1 = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev3);
        EXPECT_TRUE(result1.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result1.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }
    EXPECT_EQ(tMap->GetSize(), NODE_NUMBERS * 2);
    // test keys after add
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        JSTaggedValue keyFlag = tMap->HasKey(thread, key.GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::True(), keyFlag);
    }
    // test values after add
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        JSTaggedValue valueFlag = JSAPIHashMap::HasValue(thread, tMap, value);
        EXPECT_EQ(JSTaggedValue::True(), valueFlag);
    }

    // test entries
    {
        auto callInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo2->SetFunction(JSTaggedValue::Undefined());
        callInfo2->SetThis(tMap.GetTaggedValue());
        [[maybe_unused]] auto prev6 = TestHelper::SetupFrame(thread, callInfo2);
        JSHandle<JSTaggedValue> iter(thread, ContainersHashMap::Entries(callInfo2));
        TestHelper::TearDownFrame(thread, prev6);
        EXPECT_TRUE(iter->IsJSAPIHashMapIterator());

        JSHandle<JSTaggedValue> first(thread, JSTaggedValue(0));
        JSHandle<JSTaggedValue> second(thread, JSTaggedValue(1));
        JSMutableHandle<JSTaggedValue> result3(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> entries(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
            result3.Update(JSAPIHashMapIterator::Next(callInfo));
            TestHelper::TearDownFrame(thread, prev1);
            entries.Update(JSIterator::IteratorValue(thread, result3).GetTaggedValue());
            JSHandle<JSTaggedValue> iterKey = JSObject::GetProperty(thread, entries, first).GetValue();
            JSTaggedValue keyFlag = tMap->HasKey(thread, iterKey.GetTaggedValue());
            EXPECT_EQ(JSTaggedValue::True(), keyFlag);
            JSHandle<JSTaggedValue> iterValue = JSObject::GetProperty(thread, entries, second).GetValue();
            JSTaggedValue valueFlag = JSAPIHashMap::HasValue(thread, tMap, iterValue);
            EXPECT_EQ(JSTaggedValue::True(), valueFlag);
        }

        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            std::string iKey = myKey + std::to_string(i);
            std::string iValue = myValue + std::to_string(i);
            key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
            value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            JSTaggedValue keyFlag = tMap->HasKey(thread, key.GetTaggedValue());
            EXPECT_EQ(JSTaggedValue::True(), keyFlag);
            JSTaggedValue valueFlag = JSAPIHashMap::HasValue(thread, tMap, value);
            EXPECT_EQ(JSTaggedValue::True(), valueFlag);
        }
    }
}

// hashmap.remove(key)
HWTEST_F_L0(ContainersHashMapTest, Remove)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS / 2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rValue = ContainersHashMap::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rValue, JSTaggedValue(NODE_NUMBERS / 2));
        EXPECT_EQ(tMap->GetSize(), NODE_NUMBERS - 1);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i == (NODE_NUMBERS / 2)) {
            EXPECT_EQ(result, JSTaggedValue::Undefined());
        } else {
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i);
    }

    {
        std::string iKey = myKey + std::to_string(NODE_NUMBERS / 2);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        std::string iValue = myValue + std::to_string(NODE_NUMBERS / 2);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rValue = ContainersHashMap::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(rValue, value.GetTaggedValue()));
        EXPECT_EQ(tMap->GetSize(), NODE_NUMBERS * 2 - 2);
    }
}

// hashmap.setAll(map)
HWTEST_F_L0(ContainersHashMapTest, SetAll)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> sMap = CreateJSAPIHashMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(sMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    JSHandle<JSAPIHashMap> dMap = CreateJSAPIHashMap();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dMap.GetTaggedValue());
        callInfo->SetCallArg(0, sMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashMap::SetAll(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(dMap->GetSize(), NODE_NUMBERS);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(sMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dMap.GetTaggedValue());
        callInfo->SetCallArg(0, sMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashMap::SetAll(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(dMap->GetSize(), NODE_NUMBERS * 2);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(result, value.GetTaggedValue()));
    }
    EXPECT_EQ(dMap->GetSize(), 2 * NODE_NUMBERS);
}

// hashmap.clear()
HWTEST_F_L0(ContainersHashMapTest, Clear)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashMap::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(tMap->GetSize(), (uint32_t)0);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Undefined());
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashMap::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(tMap->GetSize(), (uint32_t)0);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Undefined());
    }
}

// hashmap.replace(key, value)
HWTEST_F_L0(ContainersHashMapTest, Replace)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS / 2));
        callInfo->SetCallArg(1, JSTaggedValue(NODE_NUMBERS));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Replace(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(tMap->GetSize(), NODE_NUMBERS);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i == (NODE_NUMBERS / 2)) {
            EXPECT_EQ(result, JSTaggedValue(NODE_NUMBERS));
        } else {
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }

    {
        std::string iKey = myKey + std::to_string(NODE_NUMBERS / 2);
        std::string iValue = myValue + std::to_string(NODE_NUMBERS);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Replace(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(tMap->GetSize(), NODE_NUMBERS * 2);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        std::string iValue;
        if (i == (NODE_NUMBERS / 2)) {
            iValue = myValue + std::to_string(NODE_NUMBERS);
        } else {
            iValue = myValue + std::to_string(i);
        }
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(result, value.GetTaggedValue()));
    }
}

// hashmap.ForEach(callbackfn, this)
HWTEST_F_L0(ContainersHashMapTest, ForEach)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> tMap = CreateJSAPIHashMap();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), i + 1);
    }
    // test foreach function with TestForEachFunc;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIHashMap> dMap = CreateJSAPIHashMap();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, dMap.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashMap::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    EXPECT_EQ(dMap->GetSize(), NODE_NUMBERS);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i * 2));
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }

    // test add string
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSAPIHashMap());
        EXPECT_EQ(JSAPIHashMap::Cast(result.GetTaggedObject())->GetSize(), NODE_NUMBERS + i + 1);
    }
    EXPECT_EQ(tMap->GetSize(), NODE_NUMBERS * 2);
    EXPECT_EQ(dMap->GetSize(), NODE_NUMBERS);

    // test foreach function with TestForEachFunc;
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, dMap.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashMap::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    EXPECT_EQ(dMap->GetSize(), NODE_NUMBERS * 2);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i * 4)); // 4 means 4 times
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dMap.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i * 2));
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        std::string iValue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(dMap.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, value.GetTaggedValue());
    }
}

HWTEST_F_L0(ContainersHashMapTest, ProxyOfGetLength)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashMap> treeMap = CreateJSAPIHashMap();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    JSHandle<JSProxy> proxy = CreateJSProxyHandle(thread);
    proxy->SetTarget(thread, treeMap.GetTaggedValue());
    callInfo->SetThis(proxy.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersHashMap::GetLength(callInfo);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(retult, JSTaggedValue(i + 1));
    }
}

HWTEST_F_L0(ContainersHashMapTest, ExceptionReturn)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, SetAll);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, Keys);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, Values);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, Entries);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, ForEach);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, Set);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, Get);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, Remove);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, HasKey);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, HasValue);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, Replace);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, Clear);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashMap, GetLength);

    JSHandle<JSAPIHashMap> treeMap = CreateJSAPIHashMap();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(treeMap.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersHashMap, SetAll, callInfo);
    }
}
} // namespace panda::test
