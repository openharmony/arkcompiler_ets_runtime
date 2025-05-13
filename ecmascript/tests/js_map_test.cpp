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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class JSMapTest : public BaseTestWithScope<false> {
protected:
    JSMap *CreateMap()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
        JSHandle<JSMap> map =
            JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
        map->SetLinkedMap(thread, hashMap);
        return *map;
    }
};

HWTEST_F_L0(JSMapTest, MapCreate)
{
    JSMap *map = CreateMap();
    EXPECT_TRUE(map != nullptr);
}

HWTEST_F_L0(JSMapTest, AddAndHas)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsMap
    JSHandle<JSMap> map(thread, CreateMap());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSMap::Set(thread, map, key, value);
    EXPECT_TRUE(map->Has(thread, key.GetTaggedValue()));
}

HWTEST_F_L0(JSMapTest, DeleteAndGet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsMap
    JSHandle<JSMap> map(thread, CreateMap());

    // add 40 keys
    char keyArray[] = "key0";
    for (uint32_t i = 0; i < 40; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSMap::Set(thread, map, key, value);
        EXPECT_TRUE(map->Has(thread, key.GetTaggedValue()));
    }
    EXPECT_EQ(map->GetSize(), 40);
    // whether jsMap has delete key
    keyArray[3] = '1' + 8;
    JSHandle<JSTaggedValue> deleteKey(factory->NewFromASCII(keyArray));
    EXPECT_EQ(map->GetValue(8), JSTaggedValue(8));
    JSMap::Delete(thread, map, deleteKey);
    EXPECT_FALSE(map->Has(thread, deleteKey.GetTaggedValue()));
    EXPECT_EQ(map->GetSize(), 39);
}

HWTEST_F_L0(JSMapTest, DeleteAndGet2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsMap
    JSHandle<JSMap> map(thread, CreateMap());
    JSHandle<JSTaggedValue> mapvalue(map);
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(10);
    int pointArr[10];
    for (uint32_t i = 0; i < 10; i++) {
        JSHandle<JSObject> obj = factory->NewEmptyJSObject(0);
        auto *nativePointer = pointArr + i;
        JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(nativePointer);
        Barriers::SetObject<true>(thread, *obj, ECMAObject::HASH_OFFSET, pointer.GetTaggedValue().GetRawData());
        taggedArray->Set(thread, i, obj);
    }
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    JSHandle<JSTaggedValue> setfunc =
        JSObject::GetProperty(thread, mapvalue, thread->GlobalConstants()->GetHandledSetString()).GetValue();
    JSHandle<JSTaggedValue> getfunc = thread->GlobalConstants()->GetHandledMapGet();
    JSHandle<JSTaggedValue> deletefunc = thread->GlobalConstants()->GetHandledMapDelete();

    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> key(thread, taggedArray->Get(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        ecmaRuntimeCallInfo->SetFunction(setfunc.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(key.GetTaggedValue(), value.GetTaggedValue());
        auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSFunction::Call(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSTaggedValue hashField =
            JSTaggedValue(Barriers::GetTaggedValue(key->GetTaggedObject(), ECMAObject::HASH_OFFSET));
        EXPECT_TRUE(hashField.IsTaggedArray());
    }
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> key(thread, taggedArray->Get(i));
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo->SetFunction(getfunc.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(key.GetTaggedValue());
        auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        auto res = JSFunction::Call(ecmaRuntimeCallInfo);
        EXPECT_TRUE(res.GetInt() == i);
        TestHelper::TearDownFrame(thread, prev);
        JSTaggedValue hashField =
            JSTaggedValue(Barriers::GetTaggedValue(key->GetTaggedObject(), ECMAObject::HASH_OFFSET));
        EXPECT_TRUE(hashField.IsTaggedArray());
    }
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> key(thread, taggedArray->Get(i));
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo->SetFunction(deletefunc.GetTaggedValue());
        ecmaRuntimeCallInfo->SetThis(map.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(key.GetTaggedValue());
        auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        auto res = JSFunction::Call(ecmaRuntimeCallInfo);
        EXPECT_TRUE(res.IsTrue());
        TestHelper::TearDownFrame(thread, prev);
        JSTaggedValue hashField =
            JSTaggedValue(Barriers::GetTaggedValue(key->GetTaggedObject(), ECMAObject::HASH_OFFSET));
        EXPECT_TRUE(hashField.IsTaggedArray());
    }
}

HWTEST_F_L0(JSMapTest, Iterator)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSMap> map(thread, CreateMap());
    for (int i = 0; i < 5; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i + 10));
        JSMap::Set(thread, map, key, value);
    }

    JSHandle<JSTaggedValue> keyIter(factory->NewJSMapIterator(map, IterationKind::KEY));
    JSHandle<JSTaggedValue> valueIter(factory->NewJSMapIterator(map, IterationKind::VALUE));
    JSHandle<JSTaggedValue> iter(factory->NewJSMapIterator(map, IterationKind::KEY_AND_VALUE));

    JSHandle<JSTaggedValue> indexKey(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> elementKey(thread, JSTaggedValue(1));

    JSHandle<JSTaggedValue> keyResult0 = JSIterator::IteratorStep(thread, keyIter);
    JSHandle<JSTaggedValue> valueResult0 = JSIterator::IteratorStep(thread, valueIter);
    JSHandle<JSTaggedValue> result0 = JSIterator::IteratorStep(thread, iter);

    EXPECT_EQ(0, JSIterator::IteratorValue(thread, keyResult0)->GetInt());
    EXPECT_EQ(10, JSIterator::IteratorValue(thread, valueResult0)->GetInt());
    JSHandle<JSTaggedValue> result0Handle = JSIterator::IteratorValue(thread, result0);
    EXPECT_EQ(0, JSObject::GetProperty(thread, result0Handle, indexKey).GetValue()->GetInt());
    EXPECT_EQ(10, JSObject::GetProperty(thread, result0Handle, elementKey).GetValue()->GetInt());

    JSHandle<JSTaggedValue> keyResult1 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(1, JSIterator::IteratorValue(thread, keyResult1)->GetInt());
    for (int i = 0; i < 3; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSMap::Delete(thread, map, key);
    }
    JSHandle<JSTaggedValue> keyResult2 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(3, JSIterator::IteratorValue(thread, keyResult2)->GetInt());
    JSHandle<JSTaggedValue> keyResult3 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(4, JSIterator::IteratorValue(thread, keyResult3)->GetInt());
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(5));
    JSMap::Set(thread, map, key, key);
    JSHandle<JSTaggedValue> keyResult4 = JSIterator::IteratorStep(thread, keyIter);

    EXPECT_EQ(5, JSIterator::IteratorValue(thread, keyResult4)->GetInt());
    JSHandle<JSTaggedValue> keyResult5 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(JSTaggedValue::False(), keyResult5.GetTaggedValue());
}
}  // namespace panda::test
