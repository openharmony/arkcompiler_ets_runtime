/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/js_object-inl.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class ObjectOperatorTest : public BaseTestWithScope<false> {
};

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

bool TestBoolSetter([[maybe_unused]] JSThread *thread,
                    [[maybe_unused]] const JSHandle<JSObject> &jsObject,
                    [[maybe_unused]] const JSHandle<JSTaggedValue> &value,
                    [[maybe_unused]] bool success)
{
    return true;
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor1)
{
    JSHandle<JSTaggedValue> handleKey(thread, JSTaggedValue(1));
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSTaggedVale>(), type)
    ObjectOperator objectOperator1(thread, handleKey, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    EXPECT_TRUE(objectOperator1.GetReceiver()->IsJSGlobalObject());
    EXPECT_FALSE(objectOperator1.GetHolder()->IsJSGlobalObject());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleKey, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
    EXPECT_TRUE(objectOperator2.GetReceiver()->IsJSGlobalObject());
    EXPECT_TRUE(objectOperator2.GetHolder()->IsJSGlobalObject());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> handleHolder = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSObject>(), JSHandle<JSTaggedVale>(), type)
    ObjectOperator objectOperator1(thread, handleHolder, handleKey, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleHolder, handleKey, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor3)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> symbolFunc = env->GetSymbolFunction();
    JSHandle<JSTaggedValue> handleHolder(thread, symbolFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReceiver(thread, symbolFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSTaggedVale>(), JSHandle<JSTaggedVale>(), type)
    ObjectOperator objectOperator1(thread, handleHolder, handleKey, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleHolder, handleKey, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor4)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> stringFunc = env->GetStringFunction();
    JSHandle<JSTaggedValue> handleHolder(thread, stringFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReceiver(thread, stringFunc.GetTaggedValue());
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSTaggedVale>(), JSHandle<JSTaggedVale>(), type)
    ObjectOperator objectOperator1(thread, handleHolder, handleReceiver, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleHolder, handleReceiver, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor5)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> boolFunc = env->GetBooleanFunction();
    JSHandle<JSTaggedValue> handleHolder(thread, boolFunc.GetTaggedValue());
    uint32_t index = 1;
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSHandle<JSTaggedVale>(), index, type)
    ObjectOperator objectOperator1(thread, handleHolder, index, type);
    EXPECT_TRUE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleHolder, index, type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor6)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> numFunc = env->GetNumberFunction();
    JSHandle<JSTaggedValue> handleReceiver(thread, numFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> handleName(factory->NewFromASCII("name"));
    OperatorType type = OperatorType::PROTOTYPE_CHAIN;
    // ObjectOperator(thread, JSTaggedVale(), JSTaggedValue(), type)
    ObjectOperator objectOperator1(thread, handleReceiver.GetTaggedValue(), handleName.GetTaggedValue(), type);
    EXPECT_FALSE(objectOperator1.IsOnPrototype());
    type = OperatorType::OWN;
    ObjectOperator objectOperator2(thread, handleReceiver.GetTaggedValue(), handleName.GetTaggedValue(), type);
    EXPECT_FALSE(objectOperator2.IsOnPrototype());
}

HWTEST_F_L0(ObjectOperatorTest, ObjectOperator_Constructor7)
{
    JSHandle<JSTaggedValue> handleReceiver(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleName(thread, JSTaggedValue(2));
    PropertyAttributes handleAttr(4);
    // ObjectOperator(thread, JSTaggedVale(), JSTaggedValue(), PropertyAttributes())
    ObjectOperator objectOperator(thread, handleReceiver.GetTaggedValue(), handleName.GetTaggedValue(), handleAttr);
    EXPECT_EQ(objectOperator.GetReceiver()->GetInt(), 1);
    EXPECT_EQ(objectOperator.GetAttr().GetPropertyMetaData(), 4);
    EXPECT_EQ(objectOperator.GetKey()->GetInt(), 2);
}

HWTEST_F_L0(ObjectOperatorTest, UpdateDateValue_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleKey3(thread, JSTaggedValue(102400));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(5));
    ObjectOperator objectOperator1(thread, handleValue);
    objectOperator1.SetIndex(1);

    // object is not DictionaryMode
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 3; i++) {
        JSHandle<JSTaggedValue> newKey(thread, JSTaggedValue(i));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newKey);
    }
    EXPECT_TRUE(objectOperator1.UpdateDataValue(handleObject, handleValue, false));
    auto *resultElements =TaggedArray::Cast(handleObject->GetElements(thread).GetTaggedObject());
    EXPECT_EQ(resultElements->Get(thread, objectOperator1.GetIndex()).GetInt(), 4);

    // object is DictionaryMode
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), handleKey3, handleKey);
    JSObject::DeleteProperty(thread, handleObject, handleKey3);
    EXPECT_TRUE(objectOperator1.UpdateDataValue(handleObject, handleValue1, false));
    auto *resultDict = NumberDictionary::Cast(handleObject->GetElements(thread).GetTaggedObject());
    EXPECT_EQ(resultDict->GetValue(thread, objectOperator1.GetIndex()).GetInt(), 5);

    // object value is InternalAccessor
    JSHandle<AccessorData> handleAccessorData = factory->NewInternalAccessor(
        reinterpret_cast<void*>(TestBoolSetter), nullptr);
    JSHandle<JSTaggedValue> handleValue2(handleAccessorData);
    ObjectOperator objectOperator2(thread, handleKey);
    objectOperator2.SetValue(handleAccessorData.GetTaggedValue());
    objectOperator2.SetIndex(1);
    EXPECT_TRUE(objectOperator2.UpdateDataValue(handleObject, handleValue, true));
}

HWTEST_F_L0(ObjectOperatorTest, UpdateDataValue_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(100));
    // object is JSGlobalObject
    JSHandle<JSTaggedValue> globalObj = env->GetJSGlobalObject();
    JSHandle<JSObject> handleGlobalObject(globalObj);

    JSMutableHandle<GlobalDictionary> holderDict(thread, handleGlobalObject->GetProperties(thread));
    JSHandle<GlobalDictionary> handleDict = GlobalDictionary::Create(thread, 4); // numberofElements = 4
    holderDict.Update(handleDict.GetTaggedValue());
    JSHandle<PropertyBox> cellHandle = factory->NewPropertyBox(handleKey);
    cellHandle->SetValue(thread, JSTaggedValue(4));
    JSHandle<GlobalDictionary> handleProperties =
        GlobalDictionary::PutIfAbsent(thread, holderDict, handleKey,
                                      JSHandle<JSTaggedValue>(cellHandle), PropertyAttributes(4));
    handleGlobalObject->SetProperties(thread, handleProperties.GetTaggedValue()); // Set Properties
    int keyEntry = handleProperties->FindEntry(thread, handleKey.GetTaggedValue());

    ObjectOperator objectOperator(thread, handleGlobalObject, handleKey);
    objectOperator.SetIndex(keyEntry);
    EXPECT_TRUE(objectOperator.UpdateDataValue(handleGlobalObject, handleValue, false));
    auto *resultDict = GlobalDictionary::Cast(handleGlobalObject->GetProperties(thread).GetTaggedObject());
    PropertyBox *resultCell = resultDict->GetBox(thread, objectOperator.GetIndex());
    EXPECT_EQ(resultCell->GetValue(thread).GetInt(), 100);
}

HWTEST_F_L0(ObjectOperatorTest, UpdateDataValue_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("key"));
    JSHandle<EcmaString> handleKey1 = factory->NewFromASCII("value");
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII("value1"));

    ObjectOperator objectOperator(thread, handleKey);
    objectOperator.SetIndex(1);
    PropertyDescriptor handleDesc(thread);
    PropertyAttributes handleAttr(handleDesc);
    handleAttr.SetIsInlinedProps(true);
    objectOperator.SetAttr(PropertyAttributes(handleDesc));

    // object is not DictionaryMode
    JSHandle<JSObject> handleObject = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(i));
        JSHandle<EcmaString> newString =
            factory->ConcatFromString(handleKey1, JSTaggedValue::ToString(thread, newValue));
        JSHandle<JSTaggedValue> newKey(thread, newString.GetTaggedValue());
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObject), newKey, newValue);
    }
    EXPECT_TRUE(objectOperator.UpdateDataValue(handleObject, handleValue1, false));
    EXPECT_EQ(handleObject->GetPropertyInlinedProps(thread, objectOperator.GetIndex()).GetInt(), 3);

    // object is DictionaryMode
    JSObject::DeleteProperty(thread, handleObject, handleKey2);
    EXPECT_TRUE(objectOperator.UpdateDataValue(handleObject, handleValue, false));
    TaggedArray *resultElements2 = TaggedArray::Cast(handleObject->GetProperties(thread).GetTaggedObject());
    auto *resultDict = NumberDictionary::Cast(resultElements2);
    EXPECT_EQ(resultDict->GetValue(thread, objectOperator.GetIndex()).GetInt(), 4);
}
} // namespace panda::test