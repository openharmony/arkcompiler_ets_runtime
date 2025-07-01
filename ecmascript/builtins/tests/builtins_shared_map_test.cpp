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
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/builtins/builtins_async_generator.h"
#include "ecmascript/builtins/builtins_shared_map.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsSharedMap = ecmascript::builtins::BuiltinsSharedMap;

class BuiltinsSharedMapTest : public BaseTestWithScope<false> {
public:
    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestFunc(EcmaRuntimeCallInfo *argv)
        {
            int num = GetCallArg(argv, 0)->GetInt();
            JSArray *jsArray = JSArray::Cast(GetThis(argv)->GetTaggedObject());
            int length = jsArray->GetArrayLength() + num;
            jsArray->SetArrayLength(argv->GetThread(), length);
            return JSTaggedValue::Undefined();
        }
    };
};

JSSharedMap *CreateSBuiltinsMap(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetSBuiltininMapFunction());
    // 4 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSharedMap::Constructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsECMAObject());
    JSSharedMap *jsSMap = JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return jsSMap;
}


HWTEST_F_L0(BuiltinsSharedMapTest, CreateAndGetSize)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetSBuiltininMapFunction());
    JSHandle<JSSharedMap> map(thread, CreateSBuiltinsMap(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = BuiltinsSharedMap::GetSize(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(result.GetRawData(), JSTaggedValue(0).GetRawData());
    }
    JSHandle<TaggedArray> array(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++) {
        JSHandle<TaggedArray> internalArray(factory->NewTaggedArray(2));
        internalArray->Set(thread, 0, JSTaggedValue(i));
        internalArray->Set(thread, 1, JSTaggedValue(i));
        auto arr = JSArray::CreateArrayFromList(thread, internalArray);
        array->Set(thread, i, arr);
    }
    JSHandle<JSArray> values = JSArray::CreateArrayFromList(thread, array);
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, values.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetNewTarget(newTarget.GetTaggedValue());
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        JSTaggedValue result1 = BuiltinsSharedMap::Constructor(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSSharedMap> map1(thread, JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
        EXPECT_EQ(JSSharedMap::GetSize(thread, map1), 5);
    }
}


HWTEST_F_L0(BuiltinsSharedMapTest, GetKeyTest001)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSSharedMap> map(thread, CreateSBuiltinsMap(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(map.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = BuiltinsSharedMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSSharedMap());
        JSHandle<JSSharedMap> jsSMap(thread, JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData())));
        EXPECT_EQ(JSSharedMap::GetSize(thread, jsSMap), i + 1);
    }
    JSTaggedValue result = JSSharedMap::GetKey(thread, map, 0);
    EXPECT_NE(result.GetRawData(), JSTaggedValue::Exception().GetRawData());
}

HWTEST_F_L0(BuiltinsSharedMapTest, GetValueTest001)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSSharedMap> map(thread, CreateSBuiltinsMap(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(map.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = BuiltinsSharedMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSSharedMap());
        JSHandle<JSSharedMap> jsSMap(thread, JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData())));
        EXPECT_EQ(JSSharedMap::GetSize(thread, jsSMap), i + 1);
    }
    JSTaggedValue result = JSSharedMap::GetValue(thread, map, 0);
    EXPECT_NE(result.GetRawData(), JSTaggedValue::Exception().GetRawData());
}

HWTEST_F_L0(BuiltinsSharedMapTest, DeleteTest001)
{
    JSHandle<JSSharedMap> map(thread, CreateSBuiltinsMap(thread));
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(10));
    bool result = JSSharedMap::Delete(thread, map, key);
    ASSERT_EQ(result, false);
}

enum class AlgorithmType {
    SET,
    FOR_EACH,
    HAS,
};

JSTaggedValue MapAlgorithm(JSThread *thread, JSTaggedValue thisArg, std::vector<JSTaggedValue>& args,
    int32_t argLen, AlgorithmType type)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argLen);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisArg);
    for (size_t i = 0; i < args.size(); i++) {
        ecmaRuntimeCallInfo->SetCallArg(i, args[i]);
    }
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::SET:
            result = BuiltinsSharedMap::Set(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::FOR_EACH:
            result = BuiltinsSharedMap::ForEach(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::HAS:
            result = BuiltinsSharedMap::Has(ecmaRuntimeCallInfo);
            break;
        default:
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

HWTEST_F_L0(BuiltinsSharedMapTest, SetAndHas)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsSMap
    JSHandle<JSSharedMap> map(thread, CreateSBuiltinsMap(thread));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    // JSSharedMap *jsSMap;
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result1 = BuiltinsSharedMap::Has(ecmaRuntimeCallInfo);

        EXPECT_EQ(result1.GetRawData(), JSTaggedValue::False().GetRawData());

        // test Set()
        JSTaggedValue result2 = BuiltinsSharedMap::Set(ecmaRuntimeCallInfo);

        EXPECT_TRUE(result2.IsECMAObject());
        JSHandle<JSSharedMap> jsSMap(thread, JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result2.GetRawData())));
        EXPECT_EQ(JSSharedMap::GetSize(thread, jsSMap), 1);

        std::vector<JSTaggedValue> args{key.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(1))}; // 1:value
        auto result3 = MapAlgorithm(thread, jsSMap.GetTaggedValue(), args, 8, AlgorithmType::HAS); // 8: arg len
        EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());
    }
}

HWTEST_F_L0(BuiltinsSharedMapTest, SetAndGet)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSSharedMap> map(thread, CreateSBuiltinsMap(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(map.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = BuiltinsSharedMap::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsJSSharedMap());
        JSHandle<JSSharedMap> jsSMap(thread, JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData())));
        EXPECT_EQ(JSSharedMap::GetSize(thread, jsSMap), i + 1);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(map.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = BuiltinsSharedMap::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }
}

HWTEST_F_L0(BuiltinsSharedMapTest, ForEach)
{
    // generate a map has 5 entries{key1:0,key2:1,key3:2,key4:3,key5:4}
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSSharedMap> map(thread, CreateSBuiltinsMap(thread));
    char keyArray[] = "key0";
    for (uint32_t i = 0; i < 5; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        std::vector<JSTaggedValue> args{key.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(i))};
        auto result1 = MapAlgorithm(thread, map.GetTaggedValue(), args, 8, AlgorithmType::SET);

        EXPECT_TRUE(result1.IsECMAObject());
        JSHandle<JSSharedMap> jsSMap(thread, JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
        EXPECT_EQ(JSSharedMap::GetSize(thread, jsSMap), static_cast<int>(i) + 1);
    }
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFunc));

    std::vector<JSTaggedValue> args{func.GetTaggedValue(), jsArray.GetTaggedValue()};
    auto result2 = MapAlgorithm(thread, map.GetTaggedValue(), args, 8, AlgorithmType::FOR_EACH);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(jsArray->GetArrayLength(), 10U);
}

HWTEST_F_L0(BuiltinsSharedMapTest, DeleteAndRemove)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsSMap
    JSHandle<JSSharedMap> map(thread, CreateSBuiltinsMap(thread));

    // add 40 keys
    char keyArray[] = "key0";
    for (uint32_t i = 0; i < 40; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(thread, factory->NewFromASCII(keyArray).GetTaggedValue());
        std::vector<JSTaggedValue> args{key.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(i))};
        auto result1 = MapAlgorithm(thread, map.GetTaggedValue(), args, 8, AlgorithmType::SET);

        EXPECT_TRUE(result1.IsECMAObject());
        JSHandle<JSSharedMap> jsSMap(thread, JSSharedMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
        EXPECT_EQ(JSSharedMap::GetSize(thread, jsSMap), static_cast<int>(i) + 1);
    }
    // whether jsSMap has delete key
    keyArray[3] = '1' + 8;
    JSHandle<JSTaggedValue> deleteKey(factory->NewFromASCII(keyArray));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, deleteKey.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = BuiltinsSharedMap::Has(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());

    // delete
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result3 = BuiltinsSharedMap::Delete(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());

    // check deleteKey is deleted
    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result4 = BuiltinsSharedMap::Has(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev2);
    EXPECT_EQ(result4.GetRawData(), JSTaggedValue::False().GetRawData());
    [[maybe_unused]] auto prev3 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result5 = BuiltinsSharedMap::GetSize(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev3);
    EXPECT_EQ(result5.GetRawData(), JSTaggedValue(39).GetRawData());

    // clear
    [[maybe_unused]] auto prev4 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result6 = BuiltinsSharedMap::Clear(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev4);
    EXPECT_EQ(result6.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(JSSharedMap::GetSize(thread, map), 0);
}
HWTEST_F_L0(BuiltinsSharedMapTest, Species)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> map(thread, CreateSBuiltinsMap(thread));

    // test species
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    EXPECT_TRUE(!speciesSymbol.GetTaggedValue().IsUndefined());

    JSHandle<JSFunction> newTarget(env->GetSBuiltininMapFunction());

    JSTaggedValue value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(newTarget), speciesSymbol).GetValue().GetTaggedValue();
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    EXPECT_EQ(value, newTarget.GetTaggedValue());

    JSHandle<JSFunction> constructor = JSHandle<JSFunction>::Cast(JSTaggedValue::ToObject(thread, valueHandle));
    EXPECT_EQ(JSTaggedValue::GetPrototype(thread, map), constructor->GetFunctionPrototype(thread));

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("set"));
    JSTaggedValue value1 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value1.IsUndefined());

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("has"));
    JSTaggedValue value2 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value2.IsUndefined());

    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("clear"));
    JSTaggedValue value3 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value3.IsUndefined());

    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("size"));
    JSTaggedValue value4 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value4.IsUndefined());

    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("delete"));
    JSTaggedValue value5 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value5.IsUndefined());

    JSHandle<JSTaggedValue> key6(factory->NewFromASCII("forEach"));
    JSTaggedValue value6 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value6.IsUndefined());

    JSHandle<JSTaggedValue> key7(factory->NewFromASCII("get"));
    JSTaggedValue value7 = JSObject::GetProperty(thread, map, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value7.IsUndefined());
}

HWTEST_F_L0(BuiltinsSharedMapTest, GetIterator)
{
    JSHandle<JSTaggedValue> map(thread, CreateSBuiltinsMap(thread));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    // test Values()
    JSTaggedValue result = BuiltinsSharedMap::Values(ecmaRuntimeCallInfo);
    JSHandle<JSSharedMapIterator> iter(thread, result);
    EXPECT_TRUE(result.IsJSSharedMapIterator());
    EXPECT_EQ(IterationKind::VALUE, iter->GetIterationKind());

    // test Keys()
    JSTaggedValue result1 = BuiltinsSharedMap::Keys(ecmaRuntimeCallInfo);
    JSHandle<JSSharedMapIterator> iter1(thread, result1);
    EXPECT_TRUE(result1.IsJSSharedMapIterator());
    EXPECT_EQ(IterationKind::KEY, iter1->GetIterationKind());

    // test entries()
    JSTaggedValue result2 = BuiltinsSharedMap::Entries(ecmaRuntimeCallInfo);
    JSHandle<JSSharedMapIterator> iter2(thread, result2);
    EXPECT_TRUE(result2.IsJSSharedMapIterator());
    EXPECT_EQ(IterationKind::KEY_AND_VALUE, iter2->GetIterationKind());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedMapTest, NextInternalTest001)
{
    JSTaggedValue result2 =
        JSSharedMapIterator::NextInternal(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_EQ(result2.IsJSSharedSetIterator(), false);
}
}
