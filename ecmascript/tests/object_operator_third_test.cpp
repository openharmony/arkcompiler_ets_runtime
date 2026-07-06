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
#include "ecmascript/element_accessor-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_shared_object.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class ObjectOperatorTest : public BaseTestWithScope<false> {
};

static JSFunction *JSObjectTestCreate(JSThread *thread)
{
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

static JSHandle<JSObject> CreateObjectWithElements(JSThread *thread, uint32_t length)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> object =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (uint32_t i = 0; i < length; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(static_cast<int32_t>(i)));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(object), key, key);
    }
    return object;
}

static JSHandle<JSArray> CreateArrayWithElements(JSThread *thread, uint32_t length)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArray> array = factory->NewJSArray();
    array->SetArrayLength(thread, length);
    JSHandle<JSObject> object(array);
    for (uint32_t i = 0; i < length; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(static_cast<int32_t>(i)));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(object), key, key);
    }
    return array;
}

JSTaggedValue TestDefinedSetter([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // 12 : test case
    return JSTaggedValue(12);
}

HWTEST_F_L0(ObjectOperatorTest, WriteDataProperty_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    uint32_t index = 1;
    PropertyDescriptor handleDesc(thread);
    ObjectOperator objectOperator(thread, handleKey);
    PropertyAttributes handleAttr(4);
    handleDesc.SetConfigurable(true); // Desc Set Configurable
    objectOperator.SetAttr(PropertyAttributes(3));
    objectOperator.SetIndex(index);
    // object class is not DictionaryElement and object is Element
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    EXPECT_TRUE(objectOperator.WriteDataProperty(handleObject, handleDesc));
    auto resultDict = NumberDictionary::Cast(handleObject->GetElements(thread).GetTaggedObject());
    int resultEntry = resultDict->FindEntry(thread, JSTaggedValue(index));
    int resultAttrValue = resultDict->GetAttributes(thread, resultEntry).GetPropertyMetaData();

    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), resultAttrValue);
    EXPECT_EQ(objectOperator.GetAttr().GetDictionaryOrder(), 1U);
    EXPECT_TRUE(objectOperator.GetAttr().IsConfigurable());
    EXPECT_EQ(objectOperator.GetIndex(), static_cast<uint32_t>(resultEntry));
    EXPECT_FALSE(objectOperator.IsFastMode());
    EXPECT_TRUE(objectOperator.IsTransition());
}

HWTEST_F_L0(ObjectOperatorTest, WriteDataProperty_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(2));
    JSHandle<PropertyBox> cellHandle1 = factory->NewPropertyBox(handleValue1);
    JSHandle<PropertyBox> cellHandle2 = factory->NewPropertyBox(handleValue2);
    PropertyDescriptor handleDesc(thread);
    handleDesc.SetConfigurable(true);
    PropertyAttributes handleAttr(2);
    handleAttr.SetConfigurable(true);
    // object is JSGlobalObject and not Element
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);
    ObjectOperator objectOperator(thread, handleGlobalObject, handleKey);

    JSMutableHandle<GlobalDictionary> globalDict(thread, handleGlobalObject->GetProperties(thread));
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4);
    globalDict.Update(handleDict.GetTaggedValue());
    JSHandle<GlobalDictionary> handleProperties = GlobalDictionary::PutIfAbsent(
        thread, globalDict, handleKey, JSHandle<JSTaggedValue>(cellHandle1), PropertyAttributes(4));
    handleProperties->SetAttributes(thread, handleAttr.GetDictionaryOrder(), handleAttr);
    handleProperties->SetValue(thread, handleAttr.GetDictionaryOrder(), cellHandle2.GetTaggedValue());
    handleGlobalObject->SetProperties(thread, handleProperties.GetTaggedValue());
    int resultEntry = handleProperties->FindEntry(thread, handleKey.GetTaggedValue());
    objectOperator.SetIndex(resultEntry);

    EXPECT_TRUE(objectOperator.WriteDataProperty(handleGlobalObject, handleDesc));
    auto resultDict = GlobalDictionary::Cast(handleGlobalObject->GetProperties(thread).GetTaggedObject());
    EXPECT_EQ(resultDict->GetAttributes(thread, objectOperator.GetIndex()).GetPropertyMetaData(), 4);
    EXPECT_TRUE(resultDict->GetAttributes(thread, objectOperator.GetIndex()).IsConfigurable());

    EXPECT_EQ(resultDict->GetAttributes(thread, resultEntry).GetBoxType(), PropertyBoxType::MUTABLE);
    EXPECT_EQ(resultDict->GetValue(thread, resultEntry).GetInt(), 1);
}

HWTEST_F_L0(ObjectOperatorTest, WriteDataProperty_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    PropertyDescriptor handleDesc(thread, handleValue);
    handleDesc.SetSetter(handleValue); // Desc is AccessorDescriptor
    handleDesc.SetGetter(handleValue);
    // object is not DictionaryMode and not Element
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleValue);
    ObjectOperator objectOperator1(thread, handleKey);
    objectOperator1.SetAttr(PropertyAttributes(1));

    EXPECT_TRUE(objectOperator1.WriteDataProperty(handleObject, handleDesc));
    auto resultDict = NameDictionary::Cast(handleObject->GetProperties(thread).GetTaggedObject());
    int resultEntry = resultDict->FindEntry(thread, handleKey.GetTaggedValue());
    EXPECT_TRUE(resultDict->GetValue(thread, resultEntry).IsAccessorData());
    EXPECT_EQ(resultDict->GetAttributes(thread, resultEntry).GetValue(), objectOperator1.GetAttr().GetValue());
    // object is DictionaryMode and not Element
    JSObject::DeleteProperty(thread, (handleObject), handleKey);
    JSHandle<JSTaggedValue> handleSetter(factory->NewJSNativePointer(reinterpret_cast<void *>(TestDefinedSetter)));
    JSHandle<AccessorData> handleAccessorData = factory->NewAccessorData();
    handleDesc.SetSetter(handleSetter);
    ObjectOperator objectOperator2(thread, handleKey);
    objectOperator2.SetAttr(PropertyAttributes(handleDesc));
    objectOperator2.SetValue(handleAccessorData.GetTaggedValue());
    EXPECT_TRUE(objectOperator2.WriteDataProperty(handleObject, handleDesc));
    JSHandle<AccessorData> resultAccessorData(thread, objectOperator2.GetValue());
    EXPECT_EQ(resultAccessorData->GetGetter(thread).GetInt(), 2);
    EXPECT_TRUE(resultAccessorData->GetSetter(thread).IsJSNativePointer());
}

HWTEST_F_L0(ObjectOperatorTest, Property_Add_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(3));
    int32_t elementIndex = 2;
    PropertyAttributes handleAttr(elementIndex);
    // object is JSArray and Element
    JSHandle<JSArray> handleArr = factory->NewJSArray();
    handleArr->SetArrayLength(thread, (elementIndex - 1));
    JSHandle<JSTaggedValue> handleArrObj(thread, handleArr.GetTaggedValue());
    ObjectOperator objectOperator1(thread, handleArrObj, elementIndex);
    EXPECT_TRUE(objectOperator1.AddProperty(JSHandle<JSObject>(handleArrObj), handleValue, handleAttr));
    EXPECT_EQ(handleArr->GetArrayLength(), 3U); // (elementIndex - 1) + 2
    // object is DictionaryElement and Element
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    JSObject::DeleteProperty(thread, (handleObject), handleKey); // Delete key2
    ObjectOperator objectOperator2(thread, JSHandle<JSTaggedValue>(handleObject), elementIndex);
    EXPECT_TRUE(objectOperator2.AddProperty(handleObject, handleValue, handleAttr));
    auto resultDict = NumberDictionary::Cast(handleObject->GetElements(thread).GetTaggedObject());
    int resultEntry = resultDict->FindEntry(thread, JSTaggedValue(static_cast<uint32_t>(elementIndex)));
    EXPECT_EQ(resultDict->GetKey(thread, resultEntry).GetInt(), elementIndex);
    EXPECT_EQ(resultDict->GetValue(thread, resultEntry).GetInt(), 3);
}

HWTEST_F_L0(ObjectOperatorTest, Property_Add_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleString(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(3));
    int32_t elementIndex = 4;
    PropertyAttributes handleDefaultAttr(elementIndex);
    PropertyAttributes handleAttr(elementIndex);
    handleDefaultAttr.SetDefaultAttributes();
    // object is not DictionaryMode and DefaultAttr
    JSHandle<JSObject> handleObject1 = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject1), newKey, newKey);
    }
    ObjectOperator objectOperator(thread, JSHandle<JSTaggedValue>(handleObject1), elementIndex);
    EXPECT_TRUE(objectOperator.AddProperty(handleObject1, handleValue, handleDefaultAttr));
    TaggedArray *resultArray = TaggedArray::Cast(handleObject1->GetElements(thread).GetTaggedObject());
    EXPECT_EQ(resultArray->Get(thread, elementIndex).GetInt(), 3);
    EXPECT_EQ(resultArray->GetLength(), 7U);
    // object is not DictionaryMode and not DefaultAttr
    JSHandle<JSObject> handleObject2 = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 4; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject2), newKey, newKey);
    }
    EXPECT_TRUE(objectOperator.AddProperty(handleObject2, handleString, handleAttr));
    auto resultDict = NumberDictionary::Cast(handleObject2->GetElements(thread).GetTaggedObject());
    int resultEntry = resultDict->FindEntry(thread, JSTaggedValue(static_cast<uint32_t>(elementIndex)));
    EXPECT_EQ(resultDict->GetKey(thread, resultEntry).GetInt(), elementIndex);
    EXPECT_TRUE(resultDict->GetValue(thread, resultEntry).IsString());
}

HWTEST_F_L0(ObjectOperatorTest, Property_Add_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    int32_t handleAttrOffset = 4;
    PropertyAttributes handleAttr(handleAttrOffset);
    handleAttr.SetOffset(handleAttrOffset);
    // object is JSGlobalObject and not Element
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj); // no properties
    ObjectOperator objectOperator(thread, handleGlobalObject, handleKey);
    EXPECT_TRUE(objectOperator.AddProperty(handleGlobalObject, handleValue, handleAttr));
    EXPECT_EQ(objectOperator.GetAttr().GetBoxType(), PropertyBoxType::CONSTANT);
    EXPECT_EQ(objectOperator.FastGetValue()->GetInt(), 4);
    EXPECT_EQ(objectOperator.GetIndex(), 0U);
    EXPECT_TRUE(objectOperator.IsFastMode());
    EXPECT_FALSE(objectOperator.IsTransition());
}

HWTEST_F_L0(ObjectOperatorTest, Property_Add_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> handledUndefinedVal(thread, JSTaggedValue::Undefined());
    int32_t handleAttrOffset = 4;
    PropertyAttributes handleAttr(handleAttrOffset);
    handleAttr.SetOffset(handleAttrOffset);
    // object is not DictionaryMode and not Element
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 4; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    EXPECT_EQ(handleObject->GetJSHClass()->GetInlinedProperties(), 4U);
    ObjectOperator objectOperator(thread, handleObject, handleKey);
    EXPECT_TRUE(objectOperator.AddProperty(handleObject, handleValue, handleAttr));
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 4);
    EXPECT_EQ(objectOperator.GetValue().GetInt(), 4);
    EXPECT_EQ(objectOperator.GetIndex(), 0U); // 0 = 4 - 4
    EXPECT_TRUE(objectOperator.IsFastMode());
    EXPECT_TRUE(objectOperator.IsTransition());
    // object is DictionaryMode and not Element
    JSObject::DeleteProperty(thread, (handleObject), handleKey);
    EXPECT_TRUE(objectOperator.AddProperty(handleObject, handledUndefinedVal, handleAttr));
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 4);
    EXPECT_TRUE(objectOperator.GetValue().IsUndefined());
    EXPECT_EQ(objectOperator.GetIndex(), 0U);
    EXPECT_FALSE(objectOperator.IsFastMode());
    EXPECT_FALSE(objectOperator.IsTransition());
}

HWTEST_F_L0(ObjectOperatorTest, Property_DeleteElement1)
{
    uint32_t index = 1; // key value
    uint32_t index2 = 102400;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(102400));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(2));
    PropertyAttributes handleAttr(index);

    // object is not DictionaryMode
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleObject1 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject1), handleKey0, handleKey0);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject1), handleKey1, handleKey1);
    TaggedArray *handleElements = TaggedArray::Cast(handleObject1->GetElements(thread).GetTaggedObject());
    EXPECT_EQ(handleElements->Get(thread, index).GetInt(), 1);

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject1), handleKey, handleKey);
    ObjectOperator objectOperator1(thread, JSHandle<JSTaggedValue>(handleObject1), index2);
    objectOperator1.DeletePropertyInHolder();

    TaggedArray *resultElements = TaggedArray::Cast(handleObject1->GetElements(thread).GetTaggedObject());
    EXPECT_EQ(resultElements->Get(thread, index).GetInt(), 1);
    auto resultDict1 = NumberDictionary::Cast(handleObject1->GetElements(thread).GetTaggedObject());
    EXPECT_TRUE(resultDict1->IsDictionaryMode());
    EXPECT_TRUE(JSObject::GetProperty(thread, handleObject1, handleKey).GetValue()->IsUndefined());
    // object is DictionaryMode
    JSHandle<JSObject> handleObject2 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject2), handleKey0, handleKey0);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject2), handleKey1, handleKey1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject2), handleKey, handleKey);
    
    JSObject::DeleteProperty(thread, (handleObject2), handleKey1);
    ObjectOperator objectOperator2(thread, JSHandle<JSTaggedValue>(handleObject2), index - 1);
    objectOperator2.DeletePropertyInHolder();
    auto resultDict2 = NumberDictionary::Cast(handleObject2->GetElements(thread).GetTaggedObject());
    EXPECT_TRUE(resultDict2->GetKey(thread, index - 1U).IsUndefined());
    EXPECT_TRUE(resultDict2->GetValue(thread, index - 1U).IsUndefined());
    EXPECT_TRUE(JSObject::GetProperty(thread, handleObject2, handleKey0).GetValue()->IsUndefined());
    EXPECT_TRUE(JSObject::GetProperty(thread, handleObject2, handleKey1).GetValue()->IsUndefined());
}

HWTEST_F_L0(ObjectOperatorTest, Property_DeleteElement2)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);
    JSHandle<NumberDictionary> handleDict = NumberDictionary::Create(thread, 4);
    handleGlobalObject->SetElements(thread, handleDict.GetTaggedValue());
    handleGlobalObject->GetClass()->SetIsDictionaryElement(true);
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, globalObj, handleKey, handleValue);
        JSObject::DeleteProperty(thread, handleGlobalObject, handleKey);
    }
    auto resultDict = NumberDictionary::Cast(handleGlobalObject->GetElements(thread).GetTaggedObject());
    EXPECT_EQ(resultDict->Size(), 4);
}

HWTEST_F_L0(ObjectOperatorTest, DeleteElement_V70FeatureGate)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> object = CreateObjectWithElements(thread, 8);
    JSHandle<JSTaggedValue> namedKey(factory->NewFromASCII("named"));
    JSHandle<JSTaggedValue> namedValue(thread, JSTaggedValue(42));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(object), namedKey, namedValue);
    EXPECT_FALSE(object->GetJSHClass()->IsDictionaryElement());
    EXPECT_FALSE(object->GetJSHClass()->IsDictionaryMode());

    constexpr uint32_t deletedIndex = 1;
    ObjectOperator objectOperator(thread, JSHandle<JSTaggedValue>(object), deletedIndex);
    objectOperator.DeletePropertyInHolder();

    JSHandle<JSTaggedValue> deletedKey(thread, JSTaggedValue(static_cast<int32_t>(deletedIndex)));
    EXPECT_TRUE(JSObject::GetProperty(thread, object, deletedKey).GetValue()->IsUndefined());
    EXPECT_EQ(JSObject::GetProperty(thread, object, namedKey).GetValue()->GetInt(), 42);
#if ENABLE_V70_OPTIMIZATION
    EXPECT_FALSE(object->GetJSHClass()->IsDictionaryElement());
    EXPECT_FALSE(object->GetJSHClass()->IsDictionaryMode());
#else
    EXPECT_TRUE(object->GetJSHClass()->IsDictionaryElement());
    EXPECT_TRUE(object->GetJSHClass()->IsDictionaryMode());
#endif
}

HWTEST_F_L0(ObjectOperatorTest, DeleteElementsAtEnd_TrimAndEmpty)
{
    JSHandle<JSObject> handleObject = CreateObjectWithElements(thread, 5);
    uint32_t capacity = ElementAccessor::GetElementsLength(thread, handleObject);
    EXPECT_GE(capacity, 5U);

#if ENABLE_V70_OPTIMIZATION
    // Drop unused trailing slots, then delete the last used index via ObjectOperator.
    if (capacity > 5) {
        JSObject::DeleteElementsAtEnd(thread, handleObject, 5);
    }
    EXPECT_EQ(ElementAccessor::GetElementsLength(thread, handleObject), 5U);
#endif
    {
        ObjectOperator objectOperator(thread, JSHandle<JSTaggedValue>(handleObject), 4);
        objectOperator.DeletePropertyInHolder();
    }
#if ENABLE_V70_OPTIMIZATION
    EXPECT_EQ(ElementAccessor::GetElementsLength(thread, handleObject), 4U);
    EXPECT_FALSE(handleObject->GetJSHClass()->IsDictionaryElement());

    JSHandle<JSTaggedValue> hole(thread, JSTaggedValue::Hole());
    ElementAccessor::Set(thread, handleObject, 2, hole, true, ElementsKind::HOLE);
    JSObject::DeleteElementsAtEnd(thread, handleObject, 3);
    EXPECT_EQ(ElementAccessor::GetElementsLength(thread, handleObject), 2U);

    for (int i = 1; i >= 0; i--) {
        ObjectOperator objectOperator(thread, JSHandle<JSTaggedValue>(handleObject), static_cast<uint32_t>(i));
        objectOperator.DeletePropertyInHolder();
    }
    EXPECT_EQ(ElementAccessor::GetElementsLength(thread, handleObject), 0U);
    EXPECT_EQ(handleObject->GetElements(thread), thread->GetEcmaVM()->GetFactory()->EmptyArray().GetTaggedValue());
    EXPECT_FALSE(handleObject->GetJSHClass()->IsDictionaryElement());
#else
    EXPECT_TRUE(handleObject->GetJSHClass()->IsDictionaryElement());
    EXPECT_TRUE(handleObject->GetJSHClass()->IsDictionaryMode());
#endif
}

HWTEST_F_L0(ObjectOperatorTest, NormalizeElements_KeepsFastProperties)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint32_t length = 128;
    JSHandle<JSArray> array = CreateArrayWithElements(thread, length);
    JSHandle<JSObject> handleObject(array);
    JSHandle<JSTaggedValue> namedKey(factory->NewFromASCII("named"));
    JSHandle<JSTaggedValue> namedValue(thread, JSTaggedValue(42));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), namedKey, namedValue);
    EXPECT_FALSE(handleObject->GetJSHClass()->IsDictionaryMode());
    EXPECT_FALSE(handleObject->GetJSHClass()->IsDictionaryElement());

    JSHandle<JSTaggedValue> hole(thread, JSTaggedValue::Hole());
    for (uint32_t i = 0; i < 125; i++) {
        ElementAccessor::Set(thread, handleObject, i, hole, true, ElementsKind::HOLE);
    }
#if ENABLE_V70_OPTIMIZATION
    // Bypass deletion throttle so the next delete performs the sparsity scan.
    thread->SetElementsDeletionCounter(length / 16);
#endif
    {
        JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(125));
        JSObject::DeleteProperty(thread, handleObject, handleKey);
    }
    EXPECT_TRUE(handleObject->GetJSHClass()->IsDictionaryElement());
#if ENABLE_V70_OPTIMIZATION
    EXPECT_FALSE(handleObject->GetJSHClass()->IsDictionaryMode());
    EXPECT_EQ(thread->GetElementsDeletionCounter(), 0U);
#else
    EXPECT_TRUE(handleObject->GetJSHClass()->IsDictionaryMode());
#endif
    EXPECT_FALSE(handleObject->GetJSHClass()->IsStableElements());
    // GENERIC and DICTIONARY share HOLE_TAGGED; do not use kind equality as a discriminator.
    EXPECT_TRUE(ElementAccessor::IsDictionaryMode(thread, handleObject));
    EXPECT_EQ(array->GetArrayLength(), length);
    EXPECT_EQ(JSObject::GetProperty(thread, handleObject, namedKey).GetValue()->GetInt(), 42);
    JSHandle<JSTaggedValue> survivingKey(thread, JSTaggedValue(127));
    EXPECT_EQ(JSObject::GetProperty(thread, handleObject, survivingKey).GetValue()->GetInt(), 127);
}

#if ENABLE_V70_OPTIMIZATION
HWTEST_F_L0(ObjectOperatorTest, NormalizeElements_PreservesKindAcrossPropertyTransitions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint32_t length = 128;
    JSHandle<JSArray> array = CreateArrayWithElements(thread, length);
    JSHandle<JSObject> object(array);
    JSHandle<JSTaggedValue> existingKey(factory->NewFromASCII("existing"));
    JSHandle<JSTaggedValue> addedKey(factory->NewFromASCII("addedAfterNormalize"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(object), existingKey,
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));

    JSObject::NormalizeElements(thread, object);
    ASSERT_TRUE(object->GetJSHClass()->IsDictionaryElement());
    ASSERT_TRUE(ElementAccessor::IsDictionaryMode(thread, object));
    ASSERT_FALSE(object->GetJSHClass()->IsDictionaryMode());

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(object), addedKey,
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));
    EXPECT_TRUE(object->GetJSHClass()->IsDictionaryElement());
    EXPECT_TRUE(ElementAccessor::IsDictionaryMode(thread, object));
    EXPECT_FALSE(object->GetJSHClass()->IsDictionaryMode());
    EXPECT_EQ(JSObject::GetProperty(thread, object, addedKey).GetValue()->GetInt(), 2);

    EXPECT_TRUE(JSObject::DeleteProperty(thread, object, existingKey));
    EXPECT_TRUE(object->GetJSHClass()->IsDictionaryElement());
    EXPECT_TRUE(ElementAccessor::IsDictionaryMode(thread, object));
    EXPECT_TRUE(object->GetJSHClass()->IsDictionaryMode());

    EXPECT_TRUE(JSObject::PreventExtensions(thread, object));
    EXPECT_TRUE(object->GetJSHClass()->IsDictionaryElement());
    EXPECT_TRUE(ElementAccessor::IsDictionaryMode(thread, object));
    JSHandle<JSTaggedValue> survivingKey(thread, JSTaggedValue(static_cast<int32_t>(length - 1)));
    EXPECT_EQ(JSObject::GetProperty(thread, object, survivingKey).GetValue()->GetInt(),
              static_cast<int32_t>(length - 1));
}

HWTEST_F_L0(ObjectOperatorTest, ShouldNormalize_BelowMinCapacity)
{
    JSHandle<JSArray> smallArray = CreateArrayWithElements(thread, 16);
    JSHandle<JSObject> smallObject(smallArray);
    ASSERT_LT(ElementAccessor::GetElementsLength(thread, smallObject), 64U);
    thread->SetElementsDeletionCounter(7);
    EXPECT_FALSE(JSObject::ShouldNormalizeElementsOnDeletion(thread, smallObject, 0));
    EXPECT_EQ(thread->GetElementsDeletionCounter(), 7U);
    thread->ResetElementsDeletionCounter();
}

HWTEST_F_L0(ObjectOperatorTest, ShouldNormalize_ThrottleAndDenseEarlyExit)
{
    constexpr uint32_t length = 128;
    JSHandle<JSArray> denseArray = CreateArrayWithElements(thread, length);
    JSHandle<JSObject> denseObject(denseArray);
    ASSERT_GE(ElementAccessor::GetElementsLength(thread, denseObject), 64U);
    JSHandle<JSTaggedValue> hole(thread, JSTaggedValue::Hole());
    ElementAccessor::Set(thread, denseObject, 0, hole, true, ElementsKind::HOLE);

    thread->ResetElementsDeletionCounter();
    EXPECT_FALSE(JSObject::ShouldNormalizeElementsOnDeletion(thread, denseObject, 0));
    EXPECT_EQ(thread->GetElementsDeletionCounter(), 1U);

    thread->SetElementsDeletionCounter(length / 16);
    EXPECT_FALSE(JSObject::ShouldNormalizeElementsOnDeletion(thread, denseObject, 0));
    EXPECT_EQ(thread->GetElementsDeletionCounter(), 0U);
    EXPECT_FALSE(denseObject->GetJSHClass()->IsDictionaryElement());
}

HWTEST_F_L0(ObjectOperatorTest, ShouldNormalize_MinimumThreshold)
{
    constexpr uint32_t capacity = 128;
    constexpr uint32_t shortLength = 8;
    JSHandle<JSArray> shortArray = CreateArrayWithElements(thread, capacity);
    JSHandle<JSObject> shortObject(shortArray);
    // Keep capacity >= 64 but shrink logical length so threshold = max(1, length/16) = 1.
    // Hole the discarded tail; do not leave live elements past the new length.
    JSHandle<JSTaggedValue> hole(thread, JSTaggedValue::Hole());
    uint32_t shortCapacity = ElementAccessor::GetElementsLength(thread, shortObject);
    for (uint32_t i = shortLength; i < shortCapacity; i++) {
        ElementAccessor::Set(thread, shortObject, i, hole, true, ElementsKind::HOLE);
    }
    shortArray->SetArrayLength(thread, shortLength);
    ASSERT_GE(shortCapacity, 64U);
    ASSERT_LT(shortArray->GetArrayLength(), 16U);

    thread->ResetElementsDeletionCounter();
    EXPECT_FALSE(JSObject::ShouldNormalizeElementsOnDeletion(thread, shortObject, 0));
    EXPECT_EQ(thread->GetElementsDeletionCounter(), 1U);

    EXPECT_FALSE(JSObject::ShouldNormalizeElementsOnDeletion(thread, shortObject, 0));
    EXPECT_EQ(thread->GetElementsDeletionCounter(), 0U);
    EXPECT_FALSE(shortObject->GetJSHClass()->IsDictionaryElement());
}

HWTEST_F_L0(ObjectOperatorTest, ShouldNormalize_DictionarySpaceBoundary)
{
    constexpr uint32_t capacity = 72;
    constexpr uint32_t liveElements = 4;
    constexpr uint32_t preferFastElementsSizeFactor = 3;
    JSHandle<JSArray> array = CreateArrayWithElements(thread, capacity);
    JSHandle<JSObject> object(array);
    JSHandle<TaggedArray> elements(thread, object->GetElements(thread));
    ASSERT_GE(elements->GetLength(), capacity);
    if (elements->GetLength() > capacity) {
        elements->Trim(thread, capacity);
    }
    ASSERT_EQ(ElementAccessor::GetElementsLength(thread, object), capacity);

    JSHandle<JSTaggedValue> hole(thread, JSTaggedValue::Hole());
    for (uint32_t i = 0; i < capacity - liveElements; i++) {
        ElementAccessor::Set(thread, object, i, hole, true, ElementsKind::HOLE);
    }
    uint32_t dictSize = static_cast<uint32_t>(NumberDictionary::ComputeHashTableSize(liveElements));
    ASSERT_EQ(dictSize, 8U);
    ASSERT_EQ(preferFastElementsSizeFactor * dictSize * NumberDictionary::GetEntrySize(), capacity);

    thread->SetElementsDeletionCounter(capacity / 16);
    EXPECT_TRUE(JSObject::ShouldNormalizeElementsOnDeletion(thread, object, 0));
    EXPECT_EQ(thread->GetElementsDeletionCounter(), 0U);
}

HWTEST_F_L0(ObjectOperatorTest, ShouldNormalize_NonArrayWithLiveTail)
{
    constexpr uint32_t length = 128;
    JSHandle<JSTaggedValue> hole(thread, JSTaggedValue::Hole());

    JSHandle<JSObject> denseObject = CreateObjectWithElements(thread, length);
    uint32_t denseCapacity = ElementAccessor::GetElementsLength(thread, denseObject);
    if (denseCapacity > length) {
        JSObject::DeleteElementsAtEnd(thread, denseObject, length);
        denseCapacity = length;
    }
    ElementAccessor::Set(thread, denseObject, 0, hole, true, ElementsKind::HOLE);
    thread->SetElementsDeletionCounter(denseCapacity / 16);
    EXPECT_FALSE(JSObject::ShouldNormalizeElementsOnDeletion(thread, denseObject, 0));
    EXPECT_EQ(ElementAccessor::GetElementsLength(thread, denseObject), denseCapacity);
    EXPECT_EQ(thread->GetElementsDeletionCounter(), 0U);

    JSHandle<JSObject> sparseObject = CreateObjectWithElements(thread, length);
    uint32_t sparseCapacity = ElementAccessor::GetElementsLength(thread, sparseObject);
    if (sparseCapacity > length) {
        JSObject::DeleteElementsAtEnd(thread, sparseObject, length);
        sparseCapacity = length;
    }
    for (uint32_t i = 0; i < length - 2; i++) {
        ElementAccessor::Set(thread, sparseObject, i, hole, true, ElementsKind::HOLE);
    }
    thread->SetElementsDeletionCounter(sparseCapacity / 16);
    ObjectOperator objectOperator(thread, JSHandle<JSTaggedValue>(sparseObject), length - 2);
    objectOperator.DeletePropertyInHolder();

    EXPECT_TRUE(sparseObject->GetJSHClass()->IsDictionaryElement());
    EXPECT_FALSE(sparseObject->GetJSHClass()->IsDictionaryMode());
    JSHandle<JSTaggedValue> survivingKey(thread, JSTaggedValue(static_cast<int32_t>(length - 1)));
    EXPECT_EQ(JSObject::GetProperty(thread, sparseObject, survivingKey).GetValue()->GetInt(),
              static_cast<int32_t>(length - 1));
}
#endif

HWTEST_F_L0(ObjectOperatorTest, ShouldNormalize_NonArrayTrailingHoles_Trim)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleObject =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    constexpr uint32_t used = 64;
    for (uint32_t i = 0; i < used; i++) {
        JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(static_cast<int32_t>(i)));
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(static_cast<int32_t>(i)));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleValue);
    }
    uint32_t capacityBeforeGrow = ElementAccessor::GetElementsLength(thread, handleObject);
    EXPECT_GE(capacityBeforeGrow, used);
    JSObject::GrowElementsCapacity(thread, handleObject, capacityBeforeGrow);
    uint32_t capacity = ElementAccessor::GetElementsLength(thread, handleObject);
    EXPECT_GT(capacity, used);
    uint32_t deletedIndex = used - 1;
    EXPECT_NE(deletedIndex, capacity - 1);

#if ENABLE_V70_OPTIMIZATION
    thread->SetElementsDeletionCounter(capacity / 16);
#endif
    {
        ObjectOperator objectOperator(thread, JSHandle<JSTaggedValue>(handleObject), deletedIndex);
        objectOperator.DeletePropertyInHolder();
    }
#if ENABLE_V70_OPTIMIZATION
    EXPECT_EQ(ElementAccessor::GetElementsLength(thread, handleObject), deletedIndex);
    EXPECT_FALSE(handleObject->GetJSHClass()->IsDictionaryElement());
#else
    EXPECT_TRUE(handleObject->GetJSHClass()->IsDictionaryElement());
    EXPECT_TRUE(handleObject->GetJSHClass()->IsDictionaryMode());
#endif
}

#if ENABLE_V70_OPTIMIZATION
HWTEST_F_L0(ObjectOperatorTest, NormalizeElements_NotifiesProtoChangeMarker)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSObject> proto = JSObject::ObjectCreate(thread, nullHandle);
    JSHandle<JSObject> child = JSObject::ObjectCreate(thread, proto);
    EXPECT_TRUE(proto->GetJSHClass()->IsPrototype());

    constexpr uint32_t length = 128;
    for (uint32_t i = 0; i < length; i++) {
        JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(static_cast<int32_t>(i)));
        JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(static_cast<int32_t>(i)));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(proto), handleKey, handleValue);
    }
    JSHandle<JSTaggedValue> namedKey(factory->NewFromASCII("named"));
    JSHandle<JSTaggedValue> namedValue(thread, JSTaggedValue(42));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(proto), namedKey, namedValue);

    JSHandle<JSHClass> childClass(thread, child->GetJSHClass());
    JSHandle<JSTaggedValue> marker = JSHClass::EnableProtoChangeMarker(thread, childClass);
    EXPECT_TRUE(marker->IsProtoChangeMarker());
    EXPECT_FALSE(ProtoChangeMarker::Cast(marker->GetTaggedObject())->GetHasChanged());
    EXPECT_TRUE(proto->GetJSHClass()->GetProtoChangeMarker(thread).IsProtoChangeMarker());

    JSObject::NormalizeElements(thread, proto);
    EXPECT_TRUE(proto->GetJSHClass()->IsDictionaryElement());
    EXPECT_FALSE(proto->GetJSHClass()->IsDictionaryMode());
    EXPECT_FALSE(proto->GetJSHClass()->IsStableElements());
    EXPECT_TRUE(ElementAccessor::IsDictionaryMode(thread, proto));
    EXPECT_TRUE(ProtoChangeMarker::Cast(marker->GetTaggedObject())->GetHasChanged());
    EXPECT_EQ(JSObject::GetProperty(thread, proto, namedKey).GetValue()->GetInt(), 42);
}

HWTEST_F_L0(ObjectOperatorTest, NormalizeElements_EmptyAndShared)
{
    JSHandle<JSObject> emptyObject = CreateObjectWithElements(thread, 4);
    JSHandle<JSTaggedValue> hole(thread, JSTaggedValue::Hole());
    for (uint32_t i = 0; i < 4; i++) {
        ElementAccessor::Set(thread, emptyObject, i, hole, true, ElementsKind::HOLE);
    }
    JSObject::NormalizeElements(thread, emptyObject);
    ASSERT_TRUE(emptyObject->GetJSHClass()->IsDictionaryElement());
    ASSERT_TRUE(ElementAccessor::IsDictionaryMode(thread, emptyObject));
    auto dictionary = NumberDictionary::Cast(emptyObject->GetElements(thread).GetTaggedObject());
    EXPECT_EQ(dictionary->EntriesCount(), 0);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> sharedObject = JSHandle<JSObject>::Cast(factory->NewJSSArray());
    EXPECT_FALSE(sharedObject->GetJSHClass()->IsDictionaryElement());
    JSObject::NormalizeElements(thread, sharedObject);
    EXPECT_EXCEPTION();
    EXPECT_FALSE(sharedObject->GetJSHClass()->IsDictionaryElement());
}

HWTEST_F_L0(ObjectOperatorTest, DeleteElement_SharedObjectPreservesLegacyThrow)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> nullHandle = thread->GlobalConstants()->GetHandledNull();
    JSHandle<LayoutInfo> emptyLayout = factory->CreateSLayoutInfo(0);
    JSHandle<JSHClass> hclass = factory->NewSEcmaHClass(
        JSSharedObject::SIZE, 0, JSType::JS_SHARED_OBJECT, nullHandle, JSHandle<JSTaggedValue>(emptyLayout));
    JSHandle<JSObject> sharedObject = factory->NewSharedOldSpaceJSObject(hclass);
    constexpr uint32_t length = 4;
    // NewSTaggedArray only accepts special init values (Hole/Undefined/...).
    JSHandle<TaggedArray> sharedElements = factory->NewSTaggedArray(length, JSTaggedValue::Hole());
    sharedObject->SetElements(thread, sharedElements);
    JSHandle<JSTaggedValue> initValue(thread, JSTaggedValue(7));
    for (uint32_t i = 0; i < length; i++) {
        ElementAccessor::Set(thread, sharedObject, i, initValue, true);
    }

    ObjectOperator objectOperator(thread, JSHandle<JSTaggedValue>(sharedObject), length - 1);
    objectOperator.DeletePropertyInHolder();
    EXPECT_EXCEPTION();
    ASSERT_EQ(ElementAccessor::GetElementsLength(thread, sharedObject), length);
    EXPECT_TRUE(ElementAccessor::Get(thread, sharedObject, length - 1).IsHole());
    EXPECT_FALSE(sharedObject->GetJSHClass()->IsDictionaryElement());
}
#endif

HWTEST_F_L0(ObjectOperatorTest, Property_DeleteProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(33));
    // object is JSGlobalObject
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);
    JSMutableHandle<GlobalDictionary> globalDict(thread, handleGlobalObject->GetProperties(thread));
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4);
    globalDict.Update(handleDict.GetTaggedValue());
    JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(handleKey);
    cellHandle->SetValue(thread, handleValue.GetTaggedValue());
    JSHandle<GlobalDictionary> handleProperties = GlobalDictionary::PutIfAbsent(
        thread, globalDict, handleKey, JSHandle<JSTaggedValue>(cellHandle), PropertyAttributes(12));
    handleGlobalObject->SetProperties(thread, handleProperties.GetTaggedValue());
    ObjectOperator objectOperator1(thread, handleGlobalObject, handleKey);

    objectOperator1.DeletePropertyInHolder();
    auto resultDict = GlobalDictionary::Cast(handleGlobalObject->GetProperties(thread).GetTaggedObject());
    // key not found
    EXPECT_EQ(resultDict->FindEntry(thread, handleKey.GetTaggedValue()), -1);
    EXPECT_EQ(resultDict->GetAttributes(thread, objectOperator1.GetIndex()).GetValue(), 0U);
    // object is not DictionaryMode
    JSHandle<JSObject> handleObject =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey, handleKey1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey0, handleKey0);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey1, handleKey1);
    ObjectOperator objectOperator2(thread, handleObject, handleKey);
    objectOperator2.DeletePropertyInHolder();
    auto resultDict1 = NameDictionary::Cast(handleObject->GetProperties(thread).GetTaggedObject());
    // key not found
    EXPECT_EQ(resultDict1->FindEntry(thread, handleKey.GetTaggedValue()), -1);
}

HWTEST_F_L0(ObjectOperatorTest, Define_SetterAndGettetr)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<AccessorData> handleAccessorData = factory->NewAccessorData();
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(2));
    JSHandle<EcmaString> handleKey(factory->NewFromASCII("value"));
    JSHandle<JSTaggedValue> handleKey1(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII("value1"));
    // object is not DictionaryMode
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleObject =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(i));
        JSHandle<EcmaString> newString =
            factory->ConcatFromString(handleKey, JSTaggedValue::ToString(thread, newValue));
        JSHandle<JSTaggedValue> newKey(thread, newString.GetTaggedValue());
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newValue);
    }
    // object is not Element
    ObjectOperator objectOperator(thread, handleObject, handleKey1);
    objectOperator.SetIndex(1);
    objectOperator.SetValue(handleAccessorData.GetTaggedValue());
    PropertyDescriptor handleDesc(thread, handleValue);
    handleDesc.SetSetter(handleValue);
    handleDesc.SetGetter(handleValue);
    objectOperator.SetAttr(PropertyAttributes(handleDesc));
    objectOperator.DefineSetter(handleValue1);
    objectOperator.DefineGetter(handleValue);

    JSHandle<JSObject> resultObj1(objectOperator.GetReceiver());
    TaggedArray *properties = TaggedArray::Cast(resultObj1->GetProperties(thread).GetTaggedObject());
    JSHandle<AccessorData> resultAccessorData1(thread, properties->Get(thread, objectOperator.GetIndex()));
    EXPECT_EQ(resultAccessorData1->GetGetter(thread).GetInt(), 0);
    EXPECT_EQ(resultAccessorData1->GetSetter(thread).GetInt(), 2);
    // object is DictionaryMode
    JSObject::DeleteProperty(thread, handleObject, handleKey2);
    objectOperator.DefineSetter(handleValue);
    objectOperator.DefineGetter(handleValue1);
    JSHandle<JSObject> resultObj2(objectOperator.GetReceiver());
    auto resultDict = NameDictionary::Cast(resultObj2->GetProperties(thread).GetTaggedObject());
    JSHandle<AccessorData> resultAccessorData2(thread, resultDict->GetValue(thread, objectOperator.GetIndex()));
    EXPECT_EQ(resultAccessorData2->GetGetter(thread).GetInt(), 2);
    EXPECT_EQ(resultAccessorData2->GetSetter(thread).GetInt(), 0);
}
} // namespace panda::test
