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
#include "ecmascript/builtins/builtins_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsSharedSet = ecmascript::builtins::BuiltinsSharedSet;
using JSSharedSet = ecmascript::JSSharedSet;

class BuiltinsSharedSetTest : public BaseTestWithScope<false> {
public:
    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestFunc(EcmaRuntimeCallInfo *argv)
        {
            JSTaggedValue key = GetCallArg(argv, 0).GetTaggedValue();
            if (key.IsUndefined()) {
                return JSTaggedValue::Undefined();
            }
            JSArray *jsArray = JSArray::Cast(GetThis(argv)->GetTaggedObject());
            uint32_t length = jsArray->GetArrayLength() + 1U;
            jsArray->SetArrayLength(argv->GetThread(), length);
            return JSTaggedValue::Undefined();
        }
    };
};

JSSharedSet *CreateBuiltinsSharedSet(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetSBuiltininSetFunction());
    // 4 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 4);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSharedSet::Constructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsECMAObject());
    JSSharedSet *jsSSet = JSSharedSet::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return jsSSet;
}

JSSet *CreateJSSet(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> constructor = thread->GetEcmaVM()->GetGlobalEnv()->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, hashSet);
    return JSSet::Cast(set.GetTaggedValue().GetTaggedObject());
}

enum class AlgorithmType {
    ADD,
    HAS,
};

JSTaggedValue SharedSetAlgorithm(JSThread *thread, JSTaggedValue jsSet, std::vector<JSTaggedValue>& args,
    uint32_t argLen = 8, AlgorithmType type = AlgorithmType::ADD)
{
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argLen);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(jsSet);
    for (size_t i = 0; i < args.size(); i++) {
        ecmaRuntimeCallInfos->SetCallArg(i, args[i]);
    }
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::ADD:
            result = BuiltinsSharedSet::Add(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::HAS:
            result = BuiltinsSharedSet::Has(ecmaRuntimeCallInfos);
            break;
        default:
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}


HWTEST_F_L0(BuiltinsSharedSetTest, CreateSetIteratorTest001)
{
    JSHandle<JSSet> jsSet(thread, CreateJSSet(thread));
    EXPECT_TRUE(*jsSet != nullptr);
    JSHandle<JSTaggedValue> setIteratorValue1 =
        JSSharedSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::KEY);
    EXPECT_EQ(setIteratorValue1->IsJSSetIterator(), false);
}

HWTEST_F_L0(BuiltinsSharedSetTest, NextInternalTest001)
{
    JSTaggedValue setIteratorValue1 =
        JSSharedSetIterator::NextInternal(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_EQ(setIteratorValue1.IsJSSharedSetIterator(), false);
}

HWTEST_F_L0(BuiltinsSharedSetTest, DeleteTest001)
{
    JSHandle<JSSharedSet> set(thread, CreateBuiltinsSharedSet(thread));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(1));
    JSSharedSet::Add(thread, set, value1);
    JSSharedSet::Add(thread, set, value2);
    JSSharedSet::Delete(thread, set, JSHandle<JSTaggedValue>(thread, JSTaggedValue(20)));
}

// new Set("abrupt").toString()
HWTEST_F_L0(BuiltinsSharedSetTest, CreateAndGetSize)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetSBuiltininSetFunction());
    JSHandle<JSSharedSet> set(thread, CreateBuiltinsSharedSet(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = BuiltinsSharedSet::GetSize(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(result.GetRawData(), JSTaggedValue(0).GetRawData());
    }

    JSHandle<TaggedArray> array(factory->NewTaggedArray(5));
    for (int i = 0; i < 5; i++) {
        array->Set(thread, i, JSTaggedValue(i));
    }

    JSHandle<JSArray> values = JSArray::CreateArrayFromList(thread, array);
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, values.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetNewTarget(newTarget.GetTaggedValue());
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
        JSTaggedValue result1 = BuiltinsSharedSet::Constructor(ecmaRuntimeCallInfo1);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSSharedSet> set1(thread, JSSharedSet::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
        EXPECT_EQ(JSSharedSet::GetSize(thread, set1), 5);
    }
}

HWTEST_F_L0(BuiltinsSharedSetTest, AddAndHas)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsSet
    JSHandle<JSSharedSet> set(thread, CreateBuiltinsSharedSet(thread));
    JSHandle<JSTaggedValue> key(thread, factory->NewFromASCII("key").GetTaggedValue());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, key.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsSharedSet::Has(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result1.GetRawData(), JSTaggedValue::False().GetRawData());

    // test Add()
    JSTaggedValue result2 = BuiltinsSharedSet::Add(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result2.IsECMAObject());
    JSSharedSet *jsSSet = JSSharedSet::Cast(reinterpret_cast<TaggedObject *>(result2.GetRawData()));
    JSHandle<JSSharedSet> set2(thread, jsSSet);
    EXPECT_EQ(JSSharedSet::GetSize(thread, set2), 1);

    // test Has()
    JSTaggedValue jsSetTag(jsSSet);
    std::vector<JSTaggedValue> args{key.GetTaggedValue()};
    auto result3 = SharedSetAlgorithm(thread, jsSetTag, args, 6, AlgorithmType::HAS);

    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());

    // test -0.0
    JSHandle<JSTaggedValue> negativeZero(thread, JSTaggedValue(-0.0));
    JSHandle<JSTaggedValue> positiveZero(thread, JSTaggedValue(+0.0));

    args[0] = negativeZero.GetTaggedValue();
    SharedSetAlgorithm(thread, jsSetTag, args, 6, AlgorithmType::ADD);

    args[0] = positiveZero.GetTaggedValue();
    auto result4 = SharedSetAlgorithm(thread, jsSetTag, args, 6, AlgorithmType::HAS);

    EXPECT_EQ(result4.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsSharedSetTest, ForEach)
{
    // generate a set has 5 entry{key1,key2,key3,key4,key5}
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSharedSet> set(thread, CreateBuiltinsSharedSet(thread));
    char keyArray[] = "key0";
    for (uint32_t i = 0U; i < 5U; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));

        std::vector<JSTaggedValue> args{key.GetTaggedValue()};
        auto result1 = SharedSetAlgorithm(thread, set.GetTaggedValue(), args, 6, AlgorithmType::ADD);

        EXPECT_TRUE(result1.IsECMAObject());
        JSHandle<JSSharedSet> set1(thread, JSSharedSet::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
        EXPECT_EQ(JSSharedSet::GetSize(thread, set1), static_cast<int>(i) + 1);
    }
    // test foreach
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func =
        factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(), reinterpret_cast<void *>(TestClass::TestFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = BuiltinsSharedSet::ForEach(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(jsArray->GetArrayLength(), 5U);
}

HWTEST_F_L0(BuiltinsSharedSetTest, DeleteAndRemove)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsSet
    JSHandle<JSSharedSet> set(thread, CreateBuiltinsSharedSet(thread));

    // add 40 keys
    char keyArray[] = "key0";
    for (uint32_t i = 0; i < 40; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        std::vector<JSTaggedValue> args{key.GetTaggedValue()};
        auto result1 = SharedSetAlgorithm(thread, set.GetTaggedValue(), args, 6, AlgorithmType::ADD);

        EXPECT_TRUE(result1.IsECMAObject());
        JSHandle<JSSharedSet> set1(thread, JSSharedSet::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
        EXPECT_EQ(JSSharedSet::GetSize(thread, set1), static_cast<int>(i) + 1);
    }
    // whether jsSet has delete key
    keyArray[3] = '1' + 8;
    JSHandle<JSTaggedValue> deleteKey(factory->NewFromASCII(keyArray));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(set.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, deleteKey.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = BuiltinsSharedSet::Has(ecmaRuntimeCallInfo1);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());

    // delete
    JSTaggedValue result3 = BuiltinsSharedSet::Delete(ecmaRuntimeCallInfo1);

    EXPECT_EQ(result3.GetRawData(), JSTaggedValue::True().GetRawData());

    // check deleteKey is deleted
    JSTaggedValue result4 = BuiltinsSharedSet::Has(ecmaRuntimeCallInfo1);

    EXPECT_EQ(result4.GetRawData(), JSTaggedValue::False().GetRawData());

    JSTaggedValue result5 = BuiltinsSharedSet::GetSize(ecmaRuntimeCallInfo1);

    EXPECT_EQ(result5.GetRawData(), JSTaggedValue(39).GetRawData());

    // clear
    JSTaggedValue result6 = BuiltinsSharedSet::Clear(ecmaRuntimeCallInfo1);
    EXPECT_EQ(result6.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(set->JSSharedSet::GetSize(thread, set), 0);
}

HWTEST_F_L0(BuiltinsSharedSetTest, Species)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> set(thread, CreateBuiltinsSharedSet(thread));
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    EXPECT_TRUE(!speciesSymbol->IsUndefined());

    JSHandle<JSFunction> newTarget(env->GetSBuiltininSetFunction());

    JSTaggedValue value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(newTarget), speciesSymbol).GetValue().GetTaggedValue();
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    EXPECT_EQ(value, newTarget.GetTaggedValue());

    // to string tag
    JSHandle<JSTaggedValue> toStringTagSymbol = env->GetToStringTagSymbol();
    JSHandle<EcmaString> stringTag(JSObject::GetProperty(thread, set, toStringTagSymbol).GetValue());
    EXPECT_TRUE(!stringTag.GetTaggedValue().IsUndefined());

    JSHandle<JSFunction> constructor = JSHandle<JSFunction>::Cast(JSTaggedValue::ToObject(thread, valueHandle));
    EXPECT_EQ(JSTaggedValue::GetPrototype(thread, set), constructor->GetFunctionPrototype());

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("add"));
    JSTaggedValue value1 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value1.IsUndefined());

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("has"));
    JSTaggedValue value2 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value2.IsUndefined());

    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("clear"));
    JSTaggedValue value3 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value3.IsUndefined());

    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("size"));
    JSTaggedValue value4 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value4.IsUndefined());

    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("delete"));
    JSTaggedValue value5 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value5.IsUndefined());

    JSHandle<JSTaggedValue> key6(factory->NewFromASCII("forEach"));
    JSTaggedValue value6 = JSObject::GetProperty(thread, set, key1).GetValue().GetTaggedValue();
    EXPECT_FALSE(value6.IsUndefined());
}

HWTEST_F_L0(BuiltinsSharedSetTest, GetIterator)
{
    JSHandle<JSTaggedValue> set(thread, CreateBuiltinsSharedSet(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    // test Values()
    JSTaggedValue result = BuiltinsSharedSet::Values(ecmaRuntimeCallInfo);
    JSHandle<JSSharedSetIterator> iter(thread, result);
    EXPECT_TRUE(result.IsJSSharedSetIterator());
    EXPECT_EQ(IterationKind::VALUE, IterationKind(iter->GetIterationKind()));

    // test entries()
    JSTaggedValue result2 = BuiltinsSharedSet::Entries(ecmaRuntimeCallInfo);
    JSHandle<JSSharedSetIterator> iter2(thread, result2);
    EXPECT_TRUE(result2.IsJSSharedSetIterator());
    EXPECT_EQ(IterationKind::KEY_AND_VALUE, iter2->GetIterationKind());
}

HWTEST_F_L0(BuiltinsSharedSetTest, GetValue)
{
    JSHandle<JSTaggedValue> set(thread, CreateBuiltinsSharedSet(thread));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(set.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    JSTaggedValue result = BuiltinsSharedSet::Values(ecmaRuntimeCallInfo);
    JSHandle<JSSharedSetIterator> iter(thread, result);
    EXPECT_TRUE(result.IsJSSharedSetIterator());
    EXPECT_EQ(IterationKind::VALUE, IterationKind(iter->GetIterationKind()));

    JSTaggedValue result2 = BuiltinsSharedSet::Entries(ecmaRuntimeCallInfo);
    JSHandle<JSSharedSetIterator> iter2(thread, result2);
    EXPECT_TRUE(result2.IsJSSharedSetIterator());
    EXPECT_EQ(IterationKind::KEY_AND_VALUE, iter2->GetIterationKind());
}

}  // namespace panda::test
