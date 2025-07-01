/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_map.h"

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsMap = ecmascript::builtins::BuiltinsMap;
using JSMap = ecmascript::JSMap;

class BuiltinsMapTest : public BaseTestWithScope<false> {
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

JSMap *CreateBuiltinsMap(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsMapFunction());
    // 4 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsMap::MapConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsECMAObject());
    JSMap *jsMap = JSMap::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return jsMap;
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
            result = BuiltinsMap::Set(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::FOR_EACH:
            result = BuiltinsMap::ForEach(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::HAS:
            result = BuiltinsMap::Has(ecmaRuntimeCallInfo);
            break;
        default:
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

// new Map("abrupt").toString()
HWTEST_F_L0(BuiltinsMapTest, CreateAndGetSize)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetBuiltinsMapFunction());
    JSHandle<JSMap> map(thread, CreateBuiltinsMap(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = BuiltinsMap::GetSize(ecmaRuntimeCallInfo);
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
        JSTaggedValue result1 = BuiltinsMap::MapConstructor(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(JSMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()))->GetSize(thread), 5);
    }
}

HWTEST_F_L0(BuiltinsMapTest, SetAndHas)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsMap
    JSHandle<JSMap> map(thread, CreateBuiltinsMap(thread));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    JSMap *jsMap;
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result1 = BuiltinsMap::Has(ecmaRuntimeCallInfo);

        EXPECT_EQ(result1.GetRawData(), JSTaggedValue::False().GetRawData());

        // test Set()
        JSTaggedValue result2 = BuiltinsMap::Set(ecmaRuntimeCallInfo);

        EXPECT_TRUE(result2.IsECMAObject());
        jsMap = JSMap::Cast(reinterpret_cast<TaggedObject *>(result2.GetRawData()));
        EXPECT_EQ(jsMap->GetSize(thread), 1);
    }

    // test Has()
    std::vector<JSTaggedValue> args{key.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(1))}; // 1:value
    auto result3 = MapAlgorithm(thread, JSTaggedValue(jsMap), args, 8, AlgorithmType::HAS); // 8: arg len
    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsMapTest, ForEach)
{
    // generate a map has 5 entries{key1:0,key2:1,key3:2,key4:3,key5:4}
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSMap> map(thread, CreateBuiltinsMap(thread));
    char keyArray[] = "key0";
    for (uint32_t i = 0; i < 5; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        std::vector<JSTaggedValue> args{key.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(i))};
        auto result1 = MapAlgorithm(thread, map.GetTaggedValue(), args, 8, AlgorithmType::SET);

        EXPECT_TRUE(result1.IsECMAObject());
        JSMap *jsMap = JSMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()));
        EXPECT_EQ(jsMap->GetSize(thread), static_cast<int>(i) + 1);
    }
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFunc));

    std::vector<JSTaggedValue> args{func.GetTaggedValue(), jsArray.GetTaggedValue()};
    auto result2 = MapAlgorithm(thread, map.GetTaggedValue(), args, 8, AlgorithmType::FOR_EACH);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(jsArray->GetArrayLength(), 10U);
}

HWTEST_F_L0(BuiltinsMapTest, DeleteAndRemove)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsMap
    JSHandle<JSMap> map(thread, CreateBuiltinsMap(thread));

    // add 40 keys
    char keyArray[] = "key0";
    for (uint32_t i = 0; i < 40; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(thread, factory->NewFromASCII(keyArray).GetTaggedValue());
        std::vector<JSTaggedValue> args{key.GetTaggedValue(), JSTaggedValue(static_cast<int32_t>(i))};
        auto result1 = MapAlgorithm(thread, map.GetTaggedValue(), args, 8, AlgorithmType::SET);

        EXPECT_TRUE(result1.IsECMAObject());
        JSMap *jsMap = JSMap::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData()));
        EXPECT_EQ(jsMap->GetSize(thread), static_cast<int>(i) + 1);
    }
    // whether jsMap has delete key
    keyArray[3] = '1' + 8;
    JSHandle<JSTaggedValue> deleteKey(factory->NewFromASCII(keyArray));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(map.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, deleteKey.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = BuiltinsMap::Has(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());

    // delete
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result3 = BuiltinsMap::Delete(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());

    // check deleteKey is deleted
    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result4 = BuiltinsMap::Has(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev2);
    EXPECT_EQ(result4.GetRawData(), JSTaggedValue::False().GetRawData());
    [[maybe_unused]] auto prev3 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result5 = BuiltinsMap::GetSize(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev3);
    EXPECT_EQ(result5.GetRawData(), JSTaggedValue(39).GetRawData());

    // clear
    [[maybe_unused]] auto prev4 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result6 = BuiltinsMap::Clear(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev4);
    EXPECT_EQ(result6.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(map->GetSize(thread), 0);
}

HWTEST_F_L0(BuiltinsMapTest, Species)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> map(thread, CreateBuiltinsMap(thread));

    // test species
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    EXPECT_TRUE(!speciesSymbol.GetTaggedValue().IsUndefined());

    JSHandle<JSFunction> newTarget(env->GetBuiltinsMapFunction());

    JSTaggedValue value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(newTarget), speciesSymbol).GetValue().GetTaggedValue();
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    EXPECT_EQ(value, newTarget.GetTaggedValue());

    // to string tag
    JSHandle<JSTaggedValue> toStringTagSymbol = env->GetToStringTagSymbol();
    JSHandle<EcmaString> stringTag(JSObject::GetProperty(thread, map, toStringTagSymbol).GetValue());
    JSHandle<EcmaString> str = factory->NewFromASCII("Map");
    EXPECT_TRUE(!stringTag.GetTaggedValue().IsUndefined());
    EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(thread, *str, *stringTag));

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

HWTEST_F_L0(BuiltinsMapTest, GetIterator)
{
    JSHandle<JSTaggedValue> map(thread, CreateBuiltinsMap(thread));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    // test Values()
    JSTaggedValue result = BuiltinsMap::Values(ecmaRuntimeCallInfo);
    JSHandle<JSMapIterator> iter(thread, result);
    EXPECT_TRUE(iter->IsJSMapIterator());
    EXPECT_EQ(IterationKind::VALUE, iter->GetIterationKind());
    EXPECT_EQ(JSMap::Cast(map.GetTaggedValue().GetTaggedObject())->GetLinkedMap(thread), iter->GetIteratedMap(thread));

    // test Keys()
    JSTaggedValue result1 = BuiltinsMap::Keys(ecmaRuntimeCallInfo);
    JSHandle<JSMapIterator> iter1(thread, result1);
    EXPECT_TRUE(iter1->IsJSMapIterator());
    EXPECT_EQ(IterationKind::KEY, iter1->GetIterationKind());

    // test entries()
    JSTaggedValue result2 = BuiltinsMap::Entries(ecmaRuntimeCallInfo);
    JSHandle<JSMapIterator> iter2(thread, result2);
    EXPECT_TRUE(iter2->IsJSMapIterator());
    EXPECT_EQ(IterationKind::KEY_AND_VALUE, iter2->GetIterationKind());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsMapTest, Exception)
{
    JSHandle<JSTaggedValue> map(thread, CreateBuiltinsMap(thread));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    auto result = JSSharedMapIterator::CreateMapIterator(thread, map, IterationKind::KEY);
    EXPECT_TRUE(result->IsUndefined());
    auto result1 = JSSharedMapIterator::NextInternal(thread, map);
    EXPECT_EQ(result1, JSTaggedValue::Exception());
    TestHelper::TearDownFrame(thread, prev);
}

}  // namespace panda::test
