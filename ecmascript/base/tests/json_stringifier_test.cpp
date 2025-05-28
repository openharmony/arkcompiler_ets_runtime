/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/base/json_helper.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_set.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JsonStringifierTest : public BaseTestWithScope<false> {
public:
    using TransformType = base::JsonHelper::TransformType;
};

static JSTaggedValue CreateBaseJSObject(JSThread *thread, const CString keyCStr)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> objectFunc(globalEnv->GetObjectFunction());

    JSHandle<JSObject> jsObject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    EXPECT_TRUE(*jsObject != nullptr);

    JSHandle<JSTaggedValue> handleKey1(factory->NewFromASCII(&keyCStr[0]));
    JSHandle<JSTaggedValue> handleValue1(thread, JSTaggedValue(1)); // 1 : test case
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), handleKey1, handleValue1);

    CString str2 = "x";
    JSHandle<JSTaggedValue> handleKey2(factory->NewFromASCII(str2));
    JSHandle<JSTaggedValue> handleValue2(thread, JSTaggedValue(3.6)); // 3.6 : test case
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), handleKey2, handleValue2);

    CString str3 = "y";
    JSHandle<JSTaggedValue> handleKey3(factory->NewFromASCII(str3));
    JSHandle<JSTaggedValue> handleValue3(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), handleKey3, handleValue3);

    return jsObject.GetTaggedValue();
}

static JSHandle<JSSharedMap> CreateSharedMap(JSThread *thread)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetSharedMapPrototype();
    auto emptySLayout = thread->GlobalConstants()->GetHandledEmptySLayoutInfo();
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSHClass> mapClass = factory->NewSEcmaHClass(JSSharedMap::SIZE, 0,
                                                          JSType::JS_SHARED_MAP, proto,
                                                          emptySLayout);
    JSHandle<JSObject> obj = factory->NewSharedOldSpaceJSObjectWithInit(mapClass);
    JSHandle<JSSharedMap> jsMap = JSHandle<JSSharedMap>::Cast(obj);
    JSHandle<LinkedHashMap> linkedMap(
        LinkedHashMap::Create(thread, LinkedHashMap::MIN_CAPACITY, MemSpaceKind::SHARED));
    jsMap->SetLinkedMap(thread, linkedMap);
    jsMap->SetModRecord(0);
    return jsMap;
}

static JSHandle<JSSharedSet> CreateJSSharedSet(JSThread *thread)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> proto = globalEnv->GetSFunctionPrototype();
    auto emptySLayout = thread->GlobalConstants()->GetHandledEmptySLayoutInfo();
    JSHandle<JSHClass> setClass = factory->NewSEcmaHClass(JSSharedSet::SIZE, 0,
        JSType::JS_SHARED_SET, proto, emptySLayout);
    JSHandle<JSSharedSet> jsSet = JSHandle<JSSharedSet>::Cast(factory->NewSharedOldSpaceJSObjectWithInit(setClass));
    JSHandle<LinkedHashSet> linkedSet(
        LinkedHashSet::Create(thread, LinkedHashSet::MIN_CAPACITY, MemSpaceKind::SHARED));
    jsSet->SetLinkedSet(thread, linkedSet);
    jsSet->SetModRecord(0);
    return jsSet;
}

static JSHandle<JSMap> CreateJSMap(JSThread *thread)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> constructor = globalEnv->GetBuiltinsMapFunction();
    JSHandle<JSMap> map =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> linkedMap = LinkedHashMap::Create(thread);
    map->SetLinkedMap(thread, linkedMap);
    return JSHandle<JSMap>(thread, *map);
}

static JSHandle<JSSet> CreateJSSet(JSThread *thread)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetFunctionPrototype();
    auto vm = thread->GetEcmaVM();
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSHClass> hclass = factory->NewEcmaHClass(JSSet::SIZE, JSType::JS_SET, proto);
    JSHandle<JSObject> jsSetObject =  factory->NewJSObjectWithInit(hclass);
    JSHandle<JSSet> jsSet = JSHandle<JSSet>::Cast(jsSetObject);
    JSHandle<LinkedHashSet> linkedSet(LinkedHashSet::Create(thread));
    jsSet->SetLinkedSet(thread, linkedSet);
    return jsSet;
}

static JSAPIHashMap *CreateHashMap(JSThread *thread)
{
    return EcmaContainerCommon::CreateHashMap(thread);
}

static JSAPIHashSet *CreateHashSet(JSThread *thread)
{
    return EcmaContainerCommon::CreateHashSet(thread);
}

static JSTaggedValue TestForStringfy1([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // false: test case
    return JSTaggedValue(JSTaggedValue::False());
}

static JSTaggedValue TestForStringfy2([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    // 10.12: test case
    return JSTaggedValue(10.12);
}

/**
 * @tc.name: Stringify_001
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is JSFunction,the third parameter
 *           is Undefined.if the second parameter is JSFunction,the return value is the parameter stringification
 *           after through "call" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> handleObj = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));
    JSHandle<JSFunction> handleFunc1 = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForStringfy1));
    JSHandle<JSFunction> handleFunc2 = factory->NewJSFunction(env, reinterpret_cast<void *>(TestForStringfy2));
    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer1(thread, handleFunc1.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer2(thread, handleFunc2.GetTaggedValue());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JsonStringifier stringifier1(thread);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleValue, handleReplacer1, handleGap);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("false", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());

    JsonStringifier stringifier2(thread);
    JSHandle<JSTaggedValue> resultString2 = stringifier2.Stringify(handleValue, handleReplacer2, handleGap);
    EXPECT_TRUE(resultString2->IsString());
    JSHandle<EcmaString> handleEcmaStr2(resultString2);
    EXPECT_STREQ("10.12", EcmaStringAccessor(handleEcmaStr2).ToCString().c_str());
}

/**
 * @tc.name: Stringify_002
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is Undefined,the third parameter
 *           is Number.This situation will stringize parameters through "SerializeJSONObject" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_002)
{
    JsonStringifier stringifier(thread);

    JSHandle<JSTaggedValue> handleObj = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));
    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue(static_cast<int32_t>(10)));

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{\n          \"z\": 1,\n          \"x\": 3.6,\n          \"y\": \"abc\"\n}",
                                                     EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

/**
 * @tc.name: Stringify_003
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is Undefined,the third parameter
 *           is String,This situation will stringize parameters through "SerializeJSONObject" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSHandle<JSTaggedValue> handleObj = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));
    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, handleStr.GetTaggedValue());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> resultStr =
        factory->NewFromASCII("{\ntttt\"z\": 1,\ntttt\"x\": 3.6,\ntttt\"y\": \"abc\"\n}");
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, resultStr, JSHandle<EcmaString>(resultString)), 0);
}

/**
 * @tc.name: Stringify_004
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is JSArray,the third parameter
 *           is String.This situation will stringize parameters through "SerializeJSONObject" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSHandle<JSTaggedValue> handleObj1 = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));

    JSArray *handleArr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> handleObj2(thread, handleArr);
    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleValue0(factory->NewFromASCII("z"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj2), handleKey0, handleValue0);

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> handleValue1(factory->NewFromASCII("x"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj2), handleKey1, handleValue1);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));

    JSHandle<JSTaggedValue> handleValue(thread, handleObj1.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, handleObj2.GetTaggedValue());
    JSHandle<JSTaggedValue> handleGap(thread, handleStr.GetTaggedValue());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{\ntttt\"z\": 1,\ntttt\"x\": 3.6\n}", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

/**
 * @tc.name: Stringify_005
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the ECMAObject,the second parameter is Undefined,the third parameter
 *           is Undefined.This situation will stringize the first parameter through "SerializeJSONObject" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_005)
{
    JsonStringifier stringifier(thread);
    JSHandle<JSTaggedValue> handleObj = JSHandle<JSTaggedValue>(thread, CreateBaseJSObject(thread, "z"));

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{\"z\":1,\"x\":3.6,\"y\":\"abc\"}", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

/**
 * @tc.name: Stringify_006
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the JSArray,the second parameter is Undefined,the third parameter
 *           is String,This situation will stringize parameters through "SerializeJSArray" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_006)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSArray *handleArr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> handleObj(thread, handleArr);

    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> handleValue0(factory->NewFromASCII("json"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj), handleKey0, handleValue0);

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    PropertyDescriptor handleDesc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(100)), true, true, true);
    JSArray::DefineOwnProperty(thread, handleObj, handleKey1, handleDesc);

    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue2(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj), handleKey2, handleValue2);

    JSHandle<JSTaggedValue> handleMsg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> handleStr(JSTaggedValue::ToString(thread, handleMsg));

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, handleStr.GetTaggedValue());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("[\ntttt\"json\",\ntttt100,\ntttt\"abc\"\n]", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

/**
 * @tc.name: Stringify_007
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the JSObject,the second parameter is Undefined,the third parameter
 *           is Undefined.This situation will stringize the first parameter through "SerializeJSArray" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_007)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSArray *handleArr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> handleObj(thread, handleArr);

    JSHandle<JSTaggedValue> handleKey0(thread, JSTaggedValue(0));
    PropertyDescriptor handleDesc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, handleObj, handleKey0, handleDesc0);

    JSHandle<JSTaggedValue> handleKey1(thread, JSTaggedValue(1));
    PropertyDescriptor handleDesc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3.6)), true, true, true);
    JSArray::DefineOwnProperty(thread, handleObj, handleKey1, handleDesc1);

    JSHandle<JSTaggedValue> handleKey2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> handleValue2(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(handleObj), handleKey2, handleValue2);

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("[1,3.6,\"abc\"]", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

/**
 * @tc.name: Stringify_008
 * @tc.desc: Check whether the result returned through "Stringify" function is within expectations
 *           the first parameter of the JSObject,the second parameter is Undefined,the third parameter
 *           is Undefined.This situation will stringize the first parameter through "SerializePrimitiveRef"
 *           function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JsonStringifierTest, Stringify_008)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<JSTaggedValue> handleStr(factory->NewFromASCII("\"\\\b\f\n\r\t"));
    JSHandle<JSPrimitiveRef> handlePrimitiveRef = factory->NewJSString(handleStr, undefined);
    JSHandle<JSObject> handleObj(thread, handlePrimitiveRef.GetTaggedValue());

    JSHandle<JSTaggedValue> handleValue(thread, handleObj.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("\"\\\"\\\\\\b\\f\\n\\r\\t\"", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

static void* Detach(void *param1, void *param2, void *hint, void *detachData)
{
    GTEST_LOG_(INFO) << "detach is running";
    if (param1 == nullptr && param2 == nullptr) {
        GTEST_LOG_(INFO) << "detach: two params is nullptr";
    }
    if (hint == nullptr && detachData) {
        GTEST_LOG_(INFO) << "detach: hint is nullptr";
    }
    return nullptr;
}

static void* Attach([[maybe_unused]] void *enginePointer, [[maybe_unused]] void *buffer, [[maybe_unused]] void *hint,
                    [[maybe_unused]] void *attachData)
{
    GTEST_LOG_(INFO) << "attach is running";
    return nullptr;
}

static panda::JSNApi::NativeBindingInfo* CreateNativeBindingInfo(void* attach, void* detach)
{
    GTEST_LOG_(INFO) << "CreateNativeBindingInfo";
    auto info = panda::JSNApi::NativeBindingInfo::CreateNewInstance();
    info->attachFunc = attach;
    info->detachFunc = detach;
    return info;
}

HWTEST_F_L0(JsonStringifierTest, Stringify_009)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread);

    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> objectFunc(globalEnv->GetObjectFunction());
    JSHandle<JSObject> jsObject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    EXPECT_TRUE(*jsObject != nullptr);

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("key1"));
    auto info = CreateNativeBindingInfo(reinterpret_cast<void*>(Attach), reinterpret_cast<void*>(Detach));
    JSHandle<JSTaggedValue> value1(factory->NewJSNativePointer(reinterpret_cast<void*>(info)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), key1, value1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsObject), key2, value2);

    JSHandle<JSTaggedValue> handleValue(thread, jsObject.GetTaggedValue());
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleValue, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{\"key1\":{},\"key2\":\"abc\"}", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_010)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSSharedMap> sharedMap = CreateSharedMap(thread);
    JSHandle<JSTaggedValue> handleMap = JSHandle<JSTaggedValue>(sharedMap);
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleMap, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{}", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());

    JsonStringifier stringifier1(thread, TransformType::SENDABLE);
    JSHandle<JSSharedMap> sharedMap1 = CreateSharedMap(thread);
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSSharedMap::Set(thread, sharedMap1, key1, value1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSSharedMap::Set(thread, sharedMap1, key2, value2);

    JSHandle<JSTaggedValue> handleMap1 = JSHandle<JSTaggedValue>(sharedMap1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleMap1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleMap1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("{\"key1\":\"abc\",\"key2\":\"val\"}", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_011)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSMap> jsMap = CreateJSMap(thread);
    JSHandle<JSTaggedValue> handleMap = JSHandle<JSTaggedValue>(jsMap);
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleMap, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{}", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());

    JsonStringifier stringifier1(thread, TransformType::SENDABLE);
    JSHandle<JSMap> jsMap1 = CreateJSMap(thread);
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSMap::Set(thread, jsMap1, key1, value1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSMap::Set(thread, jsMap1, key2, value2);

    JSHandle<JSTaggedValue> handleMap1 = JSHandle<JSTaggedValue>(jsMap1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleMap1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleMap1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("{\"key1\":\"abc\",\"key2\":\"val\"}", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_012)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSSharedSet> sharedSet = CreateJSSharedSet(thread);
    JSHandle<JSTaggedValue> handleSet = JSHandle<JSTaggedValue>(sharedSet);
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleSet, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("[]", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());

    JsonStringifier stringifier1(thread, TransformType::SENDABLE);
    JSHandle<JSSharedSet> sharedSet1 = CreateJSSharedSet(thread);
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSSharedSet::Add(thread, sharedSet1, value1);

    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSSharedSet::Add(thread, sharedSet1, value2);

    JSHandle<JSTaggedValue> handleSet1 = JSHandle<JSTaggedValue>(sharedSet1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleSet1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleSet1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("[\"abc\",\"val\"]", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_013)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSSet> jsSet = CreateJSSet(thread);
    JSHandle<JSTaggedValue> handleSet = JSHandle<JSTaggedValue>(jsSet);
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleSet, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("[]", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());

    JsonStringifier stringifier1(thread, TransformType::SENDABLE);
    JSHandle<JSSet> jsSet1 = CreateJSSet(thread);
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSSet::Add(thread, jsSet1, value1);

    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSSet::Add(thread, jsSet1, value2);

    JSHandle<JSTaggedValue> handleSet1 = JSHandle<JSTaggedValue>(jsSet1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleSet1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleSet1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("[\"abc\",\"val\"]", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_014)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSAPIHashMap> hashMap(thread, CreateHashMap(thread));
    JSHandle<JSTaggedValue> handleMap = JSHandle<JSTaggedValue>(hashMap);
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleMap, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("{}", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());

    JsonStringifier stringifier1(thread, TransformType::SENDABLE);
    JSHandle<JSAPIHashMap> hashMap1(thread, CreateHashMap(thread));
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSAPIHashMap::Set(thread, hashMap1, key1, value1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSAPIHashMap::Set(thread, hashMap1, key2, value2);

    JSHandle<JSTaggedValue> handleMap1 = JSHandle<JSTaggedValue>(hashMap1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleMap1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleMap1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STRNE("{}", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_015)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSAPIHashSet> hashSet(thread, CreateHashSet(thread));
    JSHandle<JSTaggedValue> handleSet = JSHandle<JSTaggedValue>(hashSet);
    JSHandle<JSTaggedValue> handleReplacer(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> resultString = stringifier.Stringify(handleSet, handleReplacer, handleGap);
    EXPECT_TRUE(resultString->IsString());
    JSHandle<EcmaString> handleEcmaStr(resultString);
    EXPECT_STREQ("[]", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());

    JsonStringifier stringifier1(thread, TransformType::SENDABLE);
    JSHandle<JSAPIHashSet> hashSet1(thread, CreateHashSet(thread));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSAPIHashSet::Add(thread, hashSet1, value1);

    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSAPIHashSet::Add(thread, hashSet1, value2);

    JSHandle<JSTaggedValue> handleSet1 = JSHandle<JSTaggedValue>(hashSet1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleSet1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleSet1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STRNE("[]", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_016)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSSharedMap> sharedMap1 = CreateSharedMap(thread);
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSSharedMap::Set(thread, sharedMap1, key1, value1);
    JSSharedMap::Clear(thread, sharedMap1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSSharedMap::Set(thread, sharedMap1, key2, value2);
    JSSharedMap::Set(thread, sharedMap1, key1, value1);

    JSHandle<JSTaggedValue> handleMap1 = JSHandle<JSTaggedValue>(sharedMap1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleMap1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier.Stringify(handleMap1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("{\"key2\":\"val\",\"key1\":\"abc\"}", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_017)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSSharedSet> sharedSet1 = CreateJSSharedSet(thread);
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSSharedSet::Add(thread, sharedSet1, value1);
    JSSharedSet::Clear(thread, sharedSet1);

    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSSharedSet::Add(thread, sharedSet1, value2);
    JSSharedSet::Add(thread, sharedSet1, value1);

    JSHandle<JSTaggedValue> handleSet1 = JSHandle<JSTaggedValue>(sharedSet1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleSet1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier.Stringify(handleSet1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("[\"val\",\"abc\"]", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_018)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSSharedMap> sharedMap1 = CreateSharedMap(thread);
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSSharedMap::Set(thread, sharedMap1, key1, value1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    JSSharedMap::Set(thread, sharedMap1, key2, undefined);
    JSSharedMap::Set(thread, sharedMap1, undefined, value2);
    
    JSHandle<JSTaggedValue> handleMap1 = JSHandle<JSTaggedValue>(sharedMap1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleMap1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier.Stringify(handleMap1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("{\"key1\":\"abc\",\"undefined\":\"val\"}", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_019)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier(thread, TransformType::SENDABLE);
    JSHandle<JSSharedSet> sharedSet1 = CreateJSSharedSet(thread);
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSSharedSet::Add(thread, sharedSet1, value1);

    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("val"));
    JSSharedSet::Add(thread, sharedSet1, value2);

    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    JSSharedSet::Add(thread, sharedSet1, undefined);

    JSHandle<JSTaggedValue> handleSet1 = JSHandle<JSTaggedValue>(sharedSet1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleSet1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier.Stringify(handleSet1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("[\"abc\",\"val\",null]", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_020)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JsonStringifier stringifier1(thread, TransformType::SENDABLE);
    JSHandle<JSAPIHashMap> hashMap1(thread, CreateHashMap(thread));
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("abc"));
    JSAPIHashMap::Set(thread, hashMap1, key1, value1);

    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("key2"));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue::Undefined());
    JSAPIHashMap::Set(thread, hashMap1, key2, value2);
    
    JSHandle<JSTaggedValue> handleMap1 = JSHandle<JSTaggedValue>(hashMap1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleMap1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleMap1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("{\"key1\":\"abc\"}", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

HWTEST_F_L0(JsonStringifierTest, Stringify_021)
{
    JsonStringifier stringifier1(thread, TransformType::SENDABLE);
    JSHandle<JSAPIHashSet> hashSet1(thread, CreateHashSet(thread));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue::Undefined());
    JSAPIHashSet::Add(thread, hashSet1, value1);
    
    JSHandle<JSTaggedValue> handleSet1 = JSHandle<JSTaggedValue>(hashSet1);
    JSHandle<JSTaggedValue> handleReplacer1(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> handleGap1(thread, JSTaggedValue::Undefined());
    EXPECT_TRUE(*handleSet1 != nullptr);
    JSHandle<JSTaggedValue> resultString1 = stringifier1.Stringify(handleSet1, handleReplacer1, handleGap1);
    EXPECT_TRUE(resultString1->IsString());
    JSHandle<EcmaString> handleEcmaStr1(resultString1);
    EXPECT_STREQ("[null]", EcmaStringAccessor(handleEcmaStr1).ToCString().c_str());
}

#if ENABLE_NEXT_OPTIMIZATION
HWTEST_F_L0(JsonStringifierTest, AppendSpecialDouble_01)
{
    struct TestCase {
        double value;
        const char* expected;
    };
    TestCase testCases[] = {
        { 0.0, "0" },
        { NAN, "NaN" },
        { INFINITY, "Infinity" },
        { -INFINITY, "-Infinity" },
    };
    CString str;
    for (const auto& testCase : testCases) {
        bool appended = AppendSpecialDouble(str, testCase.value);
        ASSERT_TRUE(appended);
        EXPECT_STREQ(testCase.expected, str.c_str());
        str.clear();
    }
}

HWTEST_F_L0(JsonStringifierTest, AppendDoubleToString_01)
{
    struct TestCase {
        double value;
        const char* expected;
    };
    TestCase testCases[] = {
        { 123000.0, "123000" },
        { -456000.0, "-456000" },
        { 1234.5678, "1234.5678" },
        { -8765.4321, "-8765.4321" },
        { 0.00123, "0.00123" },
        { -0.0456, "-0.0456" },
        { 1.2345e24, "1.2345e+24" },
        { -6.789e22, "-6.789e+22" },
        { 0.00000000123, "1.23e-9" },
        { -0.0000000456, "-4.56e-8" }
    };
    CString str;
    for (const auto& testCase : testCases) {
        AppendDoubleToString(str, testCase.value);
        EXPECT_STREQ(testCase.expected, str.c_str());
        str.clear();
    }
}

HWTEST_F_L0(JsonStringifierTest, ConvertToCStringAndAppend_01)
{
    struct TestCase {
        JSTaggedValue value;
        const char* expected;
    };
    TestCase testCases[] = {
        { JSTaggedValue(NAN), "NaN" },
        { JSTaggedValue(INFINITY), "Infinity" },
        { JSTaggedValue(42), "42" },
        { JSTaggedValue(-3.14), "-3.14" }
    };
    CString str;
    for (const auto& testCase : testCases) {
        ConvertNumberToCStringAndAppend(str, testCase.value);
        EXPECT_STREQ(testCase.expected, str.c_str());
        str.clear();
    }
}
#endif
}  // namespace panda::test
