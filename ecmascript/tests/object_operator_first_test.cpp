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

#include "ecmascript/object_operator.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class ObjectOperatorTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(ObjectOperatorTest, SetAttr)
{
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(2));

    ObjectOperator objectOperator1(thread, handleValue1);
    ObjectOperator objectOperator2(thread, handleValue2);
    uint32_t handleAttr1 = 2;

    objectOperator1.SetAttr(handleAttr1);
    EXPECT_EQ(objectOperator1.GetAttr().GetPropertyMetaData(), 2);
    EXPECT_FALSE(objectOperator1.IsPrimitiveAttr());

    PropertyAttributes handleAttr2(JSTaggedValue(0));
    objectOperator2.SetAttr(handleAttr2);
    EXPECT_TRUE(objectOperator2.IsPrimitiveAttr());
}

HWTEST_F_L0(ObjectOperatorTest, SetFastMode)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    EXPECT_FALSE(objectOperator.IsFastMode());
    objectOperator.SetFastMode(false);
    EXPECT_FALSE(objectOperator.IsFastMode());

    objectOperator.SetFastMode(true);
    EXPECT_TRUE(objectOperator.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, SetIsOnPrototype)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    EXPECT_TRUE(objectOperator.IsOnPrototype());
    objectOperator.SetIsOnPrototype(true);
    EXPECT_TRUE(objectOperator.IsOnPrototype());

    objectOperator.SetIsOnPrototype(false);
    EXPECT_FALSE(objectOperator.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, SetHasReceiver)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    EXPECT_FALSE(objectOperator.HasReceiver());
    objectOperator.SetHasReceiver(true);
    EXPECT_TRUE(objectOperator.HasReceiver());

    objectOperator.SetHasReceiver(false);
    EXPECT_FALSE(objectOperator.HasReceiver());
}

HWTEST_F_L0(ObjectOperatorTest, SetIsTransition)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    EXPECT_FALSE(objectOperator.IsTransition());
    objectOperator.SetIsTransition(true);
    EXPECT_TRUE(objectOperator.IsTransition());

    objectOperator.SetIsTransition(false);
    EXPECT_FALSE(objectOperator.IsTransition());
}

HWTEST_F_L0(ObjectOperatorTest, SetIndex)
{
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleValue);

    uint32_t index = 1;
    objectOperator.SetIndex(index);
    EXPECT_EQ(objectOperator.GetIndex(), 1U);
    EXPECT_TRUE(objectOperator.IsFound());

    objectOperator.SetIndex(ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_FALSE(objectOperator.IsFound());
}

HWTEST_F_L0(ObjectOperatorTest, SetValue)
{
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(3));
    ObjectOperator objectOperator(thread, handleKey);

    EXPECT_TRUE(objectOperator.GetValue().IsUndefined());
    objectOperator.SetValue(handleValue1.GetTaggedValue());
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 2);

    objectOperator.SetValue(handleValue2.GetTaggedValue());
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 3);
}

HWTEST_F_L0(ObjectOperatorTest, SetAsDefaultAttr)
{
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleKey);
    objectOperator.SetAsDefaultAttr();
    EXPECT_EQ(objectOperator.GetIndex(), ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_TRUE(objectOperator.GetValue().IsUndefined());
    EXPECT_FALSE(objectOperator.IsFastMode());
    EXPECT_FALSE(objectOperator.IsTransition());
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 7);
}

HWTEST_F_L0(ObjectOperatorTest, GetHolder)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSObject> handleHolder = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleHolder, handleKey);

    EXPECT_TRUE(objectOperator.HasHolder());
    EXPECT_TRUE(objectOperator.GetHolder().GetTaggedValue().IsECMAObject());
}

HWTEST_F_L0(ObjectOperatorTest, GetReceiver)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> handleNameString = factory->NewFromASCII("name");
    JSHandle<JSTaggedValue> handleReceiver(thread, JSTaggedValue(1));

    ObjectOperator objectOperator(thread, handleReceiver.GetTaggedValue(), handleNameString.GetTaggedValue());
    EXPECT_EQ(objectOperator.GetReceiver()->GetInt(), 1);
}

HWTEST_F_L0(ObjectOperatorTest, GetElementIndex)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(2.0));
    JSHandle<JSTaggedValue> handleKey3(factory->NewFromASCII("2"));

    ObjectOperator objectOperator1(thread, handleKey1);
    ObjectOperator objectOperator2(thread, handleKey2);
    ObjectOperator objectOperator3(thread, handleKey3);

    EXPECT_EQ(objectOperator1.GetElementIndex(), 1U);
    EXPECT_EQ(objectOperator2.GetElementIndex(), 2U);
    EXPECT_EQ(objectOperator3.GetElementIndex(), 2U);
}

HWTEST_F_L0(ObjectOperatorTest, GetKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII("name"));

    ObjectOperator objectOperator1(thread, handleKey1);
    ObjectOperator objectOperator2(thread, handleKey2);

    EXPECT_TRUE(objectOperator1.GetKey()->IsUndefined());
    EXPECT_TRUE(objectOperator1.IsElement());

    EXPECT_FALSE(objectOperator2.GetKey()->IsUndefined());
    EXPECT_FALSE(objectOperator2.IsElement());
}

HWTEST_F_L0(ObjectOperatorTest, GetThread)
{
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    ObjectOperator objectOperator(thread, handleKey);
    EXPECT_TRUE(objectOperator.GetThread() != nullptr);
}

JSTaggedValue TestDefinedGetter([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // 12 : test case
    return JSTaggedValue(12);
}

JSTaggedValue TestDefinedSetter([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // 12 : test case
    return JSTaggedValue(12);
}

static JSFunction *JSObjectTestCreate(JSThread *thread)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(ObjectOperatorTest, ToPropertyDescriptor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("property"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(1));
    JSHandle<AccessorData> handleAccessor = factory->NewAccessorData();
    JSHandle<AccessorData> handleInternalAccessor =
        factory->NewInternalAccessor(nullptr, reinterpret_cast<void *>(TestDefinedGetter));
    JSHandle<JSTaggedValue> handleHolder(factory->NewEmptyJSObject());

    PropertyDescriptor handleDesc(thread);
    handleDesc.SetGetter(handleValue);
    handleDesc.SetSetter(handleValue);
    PropertyDescriptor handleDesc1(thread);
    handleDesc1.SetWritable(true);
    PropertyDescriptor handleDesc2(thread);
    handleDesc2.SetConfigurable(true);
    handleDesc2.SetEnumerable(true);
    PropertyDescriptor handleDesc3(thread);
    PropertyAttributes handleAttr(handleDesc);

    ObjectOperator objectOperator(thread, handleHolder, handleKey);
    objectOperator.SetIndex(1);
    // object is not IsAccessorDescriptor
    objectOperator.ToPropertyDescriptor(handleDesc1);
    EXPECT_TRUE(!handleDesc1.IsWritable());
    EXPECT_TRUE(handleDesc1.GetValue()->IsUndefined());
    // object is IsAccessorDescriptor
    objectOperator.SetAttr(handleAttr);
    objectOperator.SetValue(handleAccessor.GetTaggedValue());
    objectOperator.ToPropertyDescriptor(handleDesc2);
    EXPECT_FALSE(handleDesc2.IsEnumerable());
    EXPECT_FALSE(handleDesc2.IsConfigurable());
    EXPECT_TRUE(handleDesc2.GetGetter()->IsUndefined());
    EXPECT_TRUE(handleDesc2.GetSetter()->IsUndefined());

    objectOperator.SetValue(handleInternalAccessor.GetTaggedValue());
    objectOperator.ToPropertyDescriptor(handleDesc3);
    EXPECT_EQ(handleDesc3.GetValue()->GetInt(), 12);
}

HWTEST_F_L0(ObjectOperatorTest, handleKey)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> handleString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleObject(factory->NewEmptyJSObject());
    JSHandle<JSTaggedValue> handleSymbol(factory->NewJSSymbol());
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(-1.11));
    JSHandle<JSTaggedValue> handleKey3(thread, handleString.GetTaggedValue());
    JSHandle<JSTaggedValue> handleKey4(thread, handleSymbol.GetTaggedValue());
    JSHandle<JSTaggedValue> handleKey5(thread, handleObject.GetTaggedValue());
    JSHandle<JSTaggedValue> handleKey6(thread, JSTaggedValue(1.11));

    ObjectOperator objectOperator1(thread, handleKey1);
    ObjectOperator objectOperator2(thread, handleKey2);
    ObjectOperator objectOperator3(thread, handleKey3);
    ObjectOperator objectOperator4(thread, handleKey4);
    ObjectOperator objectOperator5(thread, handleKey5);
    ObjectOperator objectOperator6(thread, handleKey6);

    JSHandle<EcmaString> handleEcmaStrTo1(objectOperator1.GetKey());
    EXPECT_STREQ("-1", EcmaStringAccessor(handleEcmaStrTo1).ToCString().c_str());

    JSHandle<EcmaString> handleEcmaStrTo2(objectOperator2.GetKey());
    EXPECT_STREQ("-1.11", EcmaStringAccessor(handleEcmaStrTo2).ToCString().c_str());

    EcmaString *str1 = EcmaString::Cast(objectOperator3.GetKey()->GetTaggedObject());
    EXPECT_TRUE(EcmaStringAccessor(str1).IsInternString());

    EXPECT_TRUE(objectOperator4.GetKey()->IsSymbol());

    EcmaString *str2 = EcmaString::Cast(objectOperator5.GetKey()->GetTaggedObject());
    EXPECT_TRUE(EcmaStringAccessor(str2).IsInternString());

    JSHandle<EcmaString> handleEcmaStrTo3(objectOperator6.GetKey());
    EXPECT_STREQ("1.11", EcmaStringAccessor(handleEcmaStrTo3).ToCString().c_str());
}

HWTEST_F_L0(ObjectOperatorTest, FastGetValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<EcmaString> handleNameString = factory->NewFromASCII("name");
    JSHandle<AccessorData> handleAccessorData = factory->NewAccessorData();
    JSHandle<JSTaggedValue> handleReceiver(thread, JSTaggedValue(123));
    JSHandle<JSTaggedValue> handleValue1(factory->NewPropertyBox(handleReceiver));
    JSHandle<JSTaggedValue> handleValue2(handleAccessorData);
    JSHandle<JSTaggedValue> handleValue3(thread, JSTaggedValue(1));

    ObjectOperator objectOperator1(thread, handleValue3);
    objectOperator1.SetIndex(1);
    objectOperator1.SetValue(handleValue1.GetTaggedValue());
    EXPECT_EQ(objectOperator1.FastGetValue()->GetInt(), 123);

    // op for fast path
    ObjectOperator objectOperator2(thread, handleReceiver.GetTaggedValue(), handleNameString.GetTaggedValue());
    objectOperator2.SetIndex(1);
    objectOperator2.SetValue(handleValue2.GetTaggedValue());
    PropertyDescriptor handleDesc(thread);
    handleDesc.SetGetter(handleValue2);
    handleDesc.SetSetter(handleValue2);
    objectOperator2.SetAttr(PropertyAttributes(handleDesc));
    EXPECT_TRUE(objectOperator2.FastGetValue()->IsUndefined());

    JSHandle<JSFunction> handleGetter = factory->NewJSFunction(env, reinterpret_cast<void *>(TestDefinedGetter));
    handleAccessorData->SetGetter(thread, handleGetter.GetTaggedValue());
    EXPECT_EQ(objectOperator2.FastGetValue()->GetInt(), 12);
}

HWTEST_F_L0(ObjectOperatorTest, ReLookupPropertyInReceiver_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleName(factory->NewFromASCII("123"));
    JSHandle<JSTaggedValue> handleHolder(factory->NewFromASCII("12"));
    JSHandle<JSTaggedValue> handleReceiver(factory->NewJSString(handleName, undefined));
    // Receiver is string
    ObjectOperator objectOperator1(thread, handleHolder, handleReceiver, handleKey);
    objectOperator1.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator1.GetIndex(), 2U);
    EXPECT_TRUE(objectOperator1.GetValue().IsString());
    EXPECT_EQ(objectOperator1.GetAttr().GetPropertyMetaData(), 2);
    EXPECT_TRUE(objectOperator1.IsFastMode());
    // Receiver is not DictionaryMode
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    ObjectOperator objectOperator2(thread, handleHolder, JSHandle<JSTaggedValue>(handleObject), handleKey);
    objectOperator2.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator2.GetIndex(), 2U);
    EXPECT_EQ(objectOperator2.GetValue().GetInt(), 2);
    EXPECT_EQ(objectOperator2.GetAttr().GetPropertyMetaData(), 7);
    EXPECT_TRUE(objectOperator2.IsFastMode());
    // Receiver is DictionaryMode
    JSObject::DeleteProperty(thread, (handleObject), handleKey);
    ObjectOperator objectOperator3(thread, handleHolder, JSHandle<JSTaggedValue>(handleObject), handleKey);
    objectOperator3.ReLookupPropertyInReceiver(); // no key find
    EXPECT_EQ(objectOperator3.GetIndex(), ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_TRUE(objectOperator3.GetValue().IsUndefined());
    EXPECT_EQ(objectOperator3.GetAttr().GetPropertyMetaData(), 0);
    EXPECT_FALSE(objectOperator3.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, ReLookupPropertyInReceiver_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleHolder(factory->NewFromASCII("12"));
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleReceiver(globalObj);
    PropertyAttributes handleAttr(4);
    // Receiver is JSGlobalObject(no properties)
    ObjectOperator objectOperator1(thread, handleHolder, JSHandle<JSTaggedValue>(handleReceiver), handleKey);
    objectOperator1.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator1.GetIndex(), ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_TRUE(objectOperator1.GetValue().IsUndefined());
    EXPECT_EQ(objectOperator1.GetAttr().GetPropertyMetaData(), 0);
    EXPECT_FALSE(objectOperator1.IsFastMode());
    // Receiver is JSGlobalObject(properties)
    JSMutableHandle<GlobalDictionary> receiverDict(thread, handleReceiver->GetProperties());
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4); // numberofElements = 4
    receiverDict.Update(handleDict.GetTaggedValue());
    JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(handleKey);
    cellHandle->SetValue(thread, JSTaggedValue(4));
    JSHandle<GlobalDictionary> handleProperties =
        GlobalDictionary::PutIfAbsent(thread, receiverDict, handleKey, JSHandle<JSTaggedValue>(cellHandle), handleAttr);
    handleReceiver->SetProperties(thread, handleProperties.GetTaggedValue());
    int keyEntry = handleProperties->FindEntry(handleKey.GetTaggedValue());
    ObjectOperator objectOperator2(thread, handleHolder, JSHandle<JSTaggedValue>(handleReceiver), handleKey);
    objectOperator2.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator2.GetIndex(), static_cast<uint32_t>(keyEntry));
    EXPECT_TRUE(objectOperator2.GetValue().IsPropertyBox());
    EXPECT_EQ(objectOperator2.GetAttr().GetPropertyMetaData(), handleAttr.GetPropertyMetaData());
    EXPECT_TRUE(objectOperator2.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, ReLookupPropertyInReceiver_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleHolder(factory->NewFromASCII("12"));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleKey1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey1, handleKey1);
    // Receiver is not DictionaryMode
    ObjectOperator objectOperator1(thread, handleHolder, JSHandle<JSTaggedValue>(handleObject), handleKey);
    objectOperator1.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator1.GetIndex(), 0U);
    EXPECT_EQ(objectOperator1.GetValue().GetInt(), 1);
    EXPECT_EQ(objectOperator1.GetAttr().GetPropertyMetaData(), 7); // default attribute
    EXPECT_TRUE(objectOperator1.IsFastMode());
    // Receiver is DictionaryMode
    JSObject::DeleteProperty(thread, (handleObject), handleKey);
    ObjectOperator objectOperator2(thread, handleHolder, JSHandle<JSTaggedValue>(handleObject), handleKey);
    objectOperator2.ReLookupPropertyInReceiver();
    EXPECT_EQ(objectOperator2.GetIndex(), ObjectOperator::NOT_FOUND_INDEX);
    EXPECT_TRUE(objectOperator2.GetValue().IsUndefined());
    EXPECT_EQ(objectOperator2.GetAttr().GetPropertyMetaData(), 0); // default attribute
    EXPECT_FALSE(objectOperator2.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, LookupProperty_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleValue);
    JSHandle<JSObject> handleObject1 = JSObject::ObjectCreate(thread, handleObject);

    ObjectOperator objectOperator(thread, handleObject1, handleKey);
    // lookup for key is element
    objectOperator.LookupProperty();
    EXPECT_TRUE(objectOperator.IsOnPrototype());
    EXPECT_EQ(objectOperator.GetIndex(), 1U);
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 7);
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 2);
    EXPECT_TRUE(objectOperator.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, LookupProperty_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey1, handleValue1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey2, handleValue2);
    JSHandle<JSObject> handleObject1 = JSObject::ObjectCreate(thread, handleObject);
    ObjectOperator objectOperator(thread, handleObject1, handleKey2);
    // lookup for key is not element
    objectOperator.LookupProperty();
    EXPECT_TRUE(objectOperator.IsOnPrototype());
    EXPECT_EQ(objectOperator.GetIndex(), 0U);
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 7);
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 2);
    EXPECT_TRUE(objectOperator.IsFastMode());
}

HWTEST_F_L0(ObjectOperatorTest, GlobalLookupProperty)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleGlobalObject), handleKey, handleValue);
    JSHandle<JSObject> handleObject = JSObject::ObjectCreate(thread, handleGlobalObject);
    JSHandle<JSTaggedValue> handleGlobalObj(JSHandle<JSGlobalObject>::Cast(handleObject));

    ObjectOperator objectOperator(thread, handleGlobalObj, handleKey);
    objectOperator.GlobalLookupProperty();
    EXPECT_TRUE(objectOperator.IsOnPrototype());
    EXPECT_EQ(objectOperator.GetIndex(), 1U);
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 7);
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 2);
    EXPECT_TRUE(objectOperator.IsFastMode());
}
} // namespace panda::test
