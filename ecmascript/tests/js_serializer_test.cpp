/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include <thread>

#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
#include "libpandabase/utils/utf.h"
#include "libpandafile/class_data_accessor-inl.h"

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_serializer.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace testing::ext;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class JSDeserializerTest {
public:
    JSDeserializerTest() : ecmaVm(nullptr), scope(nullptr), thread(nullptr) {}
    void Init()
    {
        JSRuntimeOptions options;
        options.SetEnableForceGC(true);
        ecmaVm = JSNApi::CreateEcmaVM(options);
        ecmaVm->SetEnableForceGC(true);
        EXPECT_TRUE(ecmaVm != nullptr) << "Cannot create Runtime";
        thread = ecmaVm->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }
    void Destroy()
    {
        delete scope;
        scope = nullptr;
        ecmaVm->SetEnableForceGC(false);
        thread->ClearException();
        JSNApi::DestroyJSVM(ecmaVm);
    }

    void JSSpecialValueTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<JSTaggedValue> jsTrue(thread, JSTaggedValue::True());
        JSHandle<JSTaggedValue> jsFalse(thread, JSTaggedValue::False());
        JSHandle<JSTaggedValue> jsUndefined(thread, JSTaggedValue::Undefined());
        JSHandle<JSTaggedValue> jsNull(thread, JSTaggedValue::Null());
        JSHandle<JSTaggedValue> jsHole(thread, JSTaggedValue::Hole());

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> retTrue = deserializer.Deserialize();
        EXPECT_TRUE(JSTaggedValue::SameValue(jsTrue, retTrue)) << "Not same value for JS_TRUE";
        JSHandle<JSTaggedValue> retFalse = deserializer.Deserialize();
        EXPECT_TRUE(JSTaggedValue::SameValue(jsFalse, retFalse)) << "Not same value for JS_FALSE";
        JSHandle<JSTaggedValue> retUndefined = deserializer.Deserialize();
        JSHandle<JSTaggedValue> retNull = deserializer.Deserialize();
        JSHandle<JSTaggedValue> retHole = deserializer.Deserialize();

        EXPECT_TRUE(JSTaggedValue::SameValue(jsUndefined, retUndefined)) << "Not same value for JS_UNDEFINED";
        EXPECT_TRUE(JSTaggedValue::SameValue(jsNull, retNull)) << "Not same value for JS_NULL";
        EXPECT_TRUE(JSTaggedValue::SameValue(jsHole, retHole)) << "Not same value for JS_HOLE";
        Destroy();
    }

    void JSPlainObjectTest1(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue1 = deserializer.Deserialize();
        JSHandle<JSTaggedValue> objValue2 = deserializer.Deserialize();

        JSHandle<JSObject> retObj1 = JSHandle<JSObject>::Cast(objValue1);
        JSHandle<JSObject> retObj2 = JSHandle<JSObject>::Cast(objValue2);
        EXPECT_FALSE(retObj1.IsEmpty());
        EXPECT_FALSE(retObj2.IsEmpty());

        JSHandle<TaggedArray> array1 = JSObject::GetOwnPropertyKeys(thread, retObj1);
        uint32_t length1 = array1->GetLength();
        EXPECT_EQ(length1, 4U); // 4 : test case
        double sum1 = 0.0;
        for (uint32_t i = 0; i < length1; i++) {
            JSHandle<JSTaggedValue> key(thread, array1->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj1), key).GetValue()->GetNumber();
            sum1 += a;
        }
        EXPECT_EQ(sum1, 10); // 10 : test case

        JSHandle<TaggedArray> array2 = JSObject::GetOwnPropertyKeys(thread, retObj2);
        uint32_t length2 = array2->GetLength();
        EXPECT_EQ(length2, 4U); // 4 : test case
        double sum2 = 0.0;
        for (uint32_t i = 0; i < length2; i++) {
            JSHandle<JSTaggedValue> key(thread, array2->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj2), key).GetValue()->GetNumber();
            sum2 += a;
        }
        EXPECT_EQ(sum2, 26); // 26 : test case
        Destroy();
    }

    void JSPlainObjectTest2(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue1 = deserializer.Deserialize();

        JSHandle<JSTaggedValue> key1(factory->NewFromASCII("detectResultItems"));
        JSHandle<JSTaggedValue> key2(factory->NewFromASCII("adviceId"));
        OperationResult result = JSObject::GetProperty(thread, objValue1, key1);
        JSHandle<JSTaggedValue> value1 = result.GetRawValue();
        EXPECT_TRUE(value1->IsJSArray());
        JSHandle<JSTaggedValue> value2 = JSArray::FastGetPropertyByValue(thread, value1, 0);
        JSHandle<JSObject> obj = JSHandle<JSObject>::Cast(value2);
        bool res = JSObject::HasProperty(thread, obj, key2);
        EXPECT_TRUE(res);
        OperationResult result1 = JSObject::GetProperty(thread, value2, key2);
        JSHandle<JSTaggedValue> value3 = result1.GetRawValue();
        JSHandle<JSTaggedValue> value4 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
        EXPECT_EQ(value3, value4);
        Destroy();
    }

    void NativeBindingObjectTest1(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        [[maybe_unused]] JSHandle<JSTaggedValue> objValue1 = deserializer.Deserialize();
        JSHandle<JSTaggedValue> objValue2 = deserializer.Deserialize();
        [[maybe_unused]] JSHandle<JSTaggedValue> objValue3 = deserializer.Deserialize();

        JSHandle<JSObject> retObj2 = JSHandle<JSObject>::Cast(objValue2);
        EXPECT_FALSE(retObj2.IsEmpty());

        JSHandle<TaggedArray> array2 = JSObject::GetOwnPropertyKeys(thread, retObj2);
        uint32_t length2 = array2->GetLength();
        EXPECT_EQ(length2, 6U); // 6 : test case
        Destroy();
    }

    void NativeBindingObjectTest2(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue1 = deserializer.Deserialize();
        [[maybe_unused]] JSHandle<JSTaggedValue> objValue2 = deserializer.Deserialize();

        JSHandle<JSObject> retObj1 = JSHandle<JSObject>::Cast(objValue1);
        EXPECT_FALSE(retObj1.IsEmpty());
        JSHandle<TaggedArray> array1 = JSObject::GetOwnPropertyKeys(thread, retObj1);
        uint32_t length1 = array1->GetLength();
        EXPECT_EQ(length1, 4U); // 4 : test case
        JSHandle<JSTaggedValue> key1(factory->NewFromASCII("abc"));
        JSHandle<JSTaggedValue> key2(factory->NewFromASCII("8"));
        JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(6)); // 6 : test case
        JSHandle<JSTaggedValue> value2 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
        OperationResult res1 = JSObject::GetProperty(thread, objValue1, key1);
        JSHandle<JSTaggedValue> resValue1 = res1.GetRawValue();
        EXPECT_EQ(resValue1, value1);
        OperationResult res2 = JSObject::GetProperty(thread, objValue1, key2);
        JSHandle<JSTaggedValue> resValue2 = res2.GetRawValue();
        EXPECT_EQ(resValue2, value2);
        Destroy();
    }

    void NativeBindingObjectTest3(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSDeserializer deserializer(thread, data.first, data.second);
        [[maybe_unused]] JSHandle<JSTaggedValue> objValue1 = deserializer.Deserialize();
        JSHandle<JSTaggedValue> objValue2 = deserializer.Deserialize();

        JSHandle<JSTaggedValue> key1(factory->NewFromASCII("root1"));
        JSHandle<JSTaggedValue> key2(factory->NewFromASCII("root2"));
        JSHandle<JSTaggedValue> key3(factory->NewFromASCII("root3"));
        JSHandle<JSTaggedValue> key4(factory->NewFromASCII("name"));
        JSHandle<JSTaggedValue> key5(factory->NewFromASCII("version"));
        JSHandle<JSTaggedValue> key6(factory->NewFromASCII("product"));
        JSHandle<JSTaggedValue> key7(factory->NewFromASCII("os"));
        JSHandle<JSTaggedValue> key8(factory->NewFromASCII("system"));
        JSHandle<JSTaggedValue> value1(factory->NewFromASCII(""));
        OperationResult res1 = JSObject::GetProperty(thread, objValue2, key1);
        JSHandle<JSTaggedValue> resValue1 = res1.GetRawValue();
        EXPECT_EQ(resValue1, value1);
        OperationResult res2 = JSObject::GetProperty(thread, objValue2, key2);
        JSHandle<JSTaggedValue> resValue2 = res2.GetRawValue();
        EXPECT_EQ(resValue2, value1);
        OperationResult res3 = JSObject::GetProperty(thread, objValue2, key3);
        JSHandle<JSTaggedValue> resValue3 = res3.GetRawValue();
        EXPECT_EQ(resValue3, value1);
        OperationResult res4 = JSObject::GetProperty(thread, objValue2, key4);
        JSHandle<JSTaggedValue> resValue4 = res4.GetRawValue();
        EXPECT_EQ(resValue4, value1);
        OperationResult res5 = JSObject::GetProperty(thread, objValue2, key5);
        JSHandle<JSTaggedValue> resValue5 = res5.GetRawValue();
        EXPECT_EQ(resValue5, value1);
        OperationResult res6 = JSObject::GetProperty(thread, objValue2, key6);
        JSHandle<JSTaggedValue> resValue6 = res6.GetRawValue();
        EXPECT_EQ(resValue6, value1);
        OperationResult res7 = JSObject::GetProperty(thread, objValue2, key7);
        JSHandle<JSTaggedValue> resValue7 = res7.GetRawValue();
        EXPECT_EQ(resValue7, value1);
        OperationResult res8 = JSObject::GetProperty(thread, objValue2, key8);
        JSHandle<JSTaggedValue> resValue8 = res8.GetRawValue();
        EXPECT_EQ(resValue8, value1);
        Destroy();
    }

    void DescriptionTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> key1(factory->NewFromASCII("x"));
        JSHandle<JSTaggedValue> key2(thread->GetEcmaVM()->GetFactory()->NewFromASCII("y"));
        JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
        JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2)); // 2 : test case

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue = deserializer.Deserialize();
        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);

        PropertyDescriptor desc3(thread);
        PropertyDescriptor desc4(thread);

        JSHandle<TaggedArray> array1 = JSObject::GetOwnPropertyKeys(thread, retObj);
        JSHandle<JSTaggedValue> retKey1(thread, array1->Get(0));
        JSHandle<JSTaggedValue> retKey2(thread, array1->Get(1));
        JSObject::GetOwnProperty(thread, retObj, retKey1, desc3);
        JSObject::GetOwnProperty(thread, retObj, retKey2, desc4);
        EXPECT_EQ(key1.GetTaggedValue().GetRawData(), retKey1.GetTaggedValue().GetRawData());

        EXPECT_EQ(desc3.GetValue().GetTaggedValue().GetRawData(), value1.GetTaggedValue().GetRawData());
        EXPECT_TRUE(desc3.IsWritable());
        EXPECT_FALSE(desc3.IsEnumerable());
        EXPECT_TRUE(desc3.IsConfigurable());

        EXPECT_EQ(desc4.GetValue().GetTaggedValue().GetRawData(), value2.GetTaggedValue().GetRawData());
        EXPECT_FALSE(desc4.IsWritable());
        EXPECT_TRUE(desc4.IsEnumerable());
        EXPECT_FALSE(desc4.IsConfigurable());
        Destroy();
    }

    void JSSetTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(7)); // 7 : test case
        JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(9)); // 9 : test case
        JSHandle<JSTaggedValue> value3(factory->NewFromASCII("x"));
        JSHandle<JSTaggedValue> value4(factory->NewFromASCII("y"));

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> setValue = deserializer.Deserialize();
        EXPECT_TRUE(!setValue.IsEmpty());
        JSHandle<JSSet> retSet = JSHandle<JSSet>::Cast(setValue);
        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>::Cast(retSet));
        uint32_t propertyLength = array->GetLength();
        EXPECT_EQ(propertyLength, 2U); // 2 : test case
        int sum = 0;
        for (uint32_t i = 0; i < propertyLength; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retSet), key).GetValue()->GetNumber();
            sum += a;
        }
        EXPECT_EQ(sum, 16); // 16 : test case

        EXPECT_EQ(retSet->GetSize(), 4);  // 4 : test case
        EXPECT_TRUE(retSet->Has(value1.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(value2.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(value3.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(value4.GetTaggedValue()));
        Destroy();
    }

    void JSArrayTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> arrayValue = deserializer.Deserialize();
        EXPECT_TRUE(!arrayValue.IsEmpty());

        JSHandle<JSArray> retArray = JSHandle<JSArray>::Cast(arrayValue);

        JSHandle<TaggedArray> keyArray = JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(retArray));
        uint32_t propertyLength = keyArray->GetLength();
        EXPECT_EQ(propertyLength, 23U);  // 23 : test case
        int sum = 0;
        for (uint32_t i = 0; i < propertyLength; i++) {
            JSHandle<JSTaggedValue> key(thread, keyArray->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retArray), key).GetValue()->GetNumber();
            sum += a;
        }
        EXPECT_EQ(sum, 226);  // 226 : test case

        // test get value from array
        for (int i = 0; i < 20; i++) {  // 20 : test case
            JSHandle<JSTaggedValue> value = JSArray::FastGetPropertyByValue(thread, arrayValue, i);
            EXPECT_EQ(i, value.GetTaggedValue().GetInt());
        }
        Destroy();
    }

    void ObjectsPropertyReferenceTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue1 = deserializer.Deserialize();
        JSHandle<JSTaggedValue> objValue2 = deserializer.Deserialize();
        EXPECT_TRUE(!objValue1.IsEmpty()) << "[Empty] Deserialize obj1 fail";
        EXPECT_TRUE(!objValue2.IsEmpty()) << "[Empty] Deserialize obj2 fail";
        Destroy();
    }

    void EcmaStringTest1(std::pair<uint8_t *, size_t> data)
    {
        Init();
        const char *rawStr = "this is a test ecmaString";
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromASCII(rawStr);

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ecmaString fail";
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        auto ecmaStringCode = EcmaStringAccessor(ecmaString).GetHashcode();
        auto resEcmaStringCode = EcmaStringAccessor(resEcmaString).GetHashcode();
        EXPECT_TRUE(ecmaStringCode == resEcmaStringCode) << "Not same HashCode";
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void EcmaStringTest2(std::pair<uint8_t *, size_t> data)
    {
        Init();
        const char *rawStr = "ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "ssssss";
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromASCII(rawStr);

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ecmaString fail";
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        auto ecmaStringCode = EcmaStringAccessor(ecmaString).GetHashcode();
        auto resEcmaStringCode = EcmaStringAccessor(resEcmaString).GetHashcode();
        EXPECT_TRUE(ecmaStringCode == resEcmaStringCode) << "Not same HashCode";
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void EcmaStringTest3(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->GetEmptyString();

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        auto ecmaStringCode = EcmaStringAccessor(ecmaString).GetHashcode();
        auto resEcmaStringCode = EcmaStringAccessor(resEcmaString).GetHashcode();
        EXPECT_TRUE(ecmaStringCode == resEcmaStringCode) << "Not same HashCode";
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void EcmaStringTest4(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromStdString("你好，世界");
        JSHandle<EcmaString> ecmaString1 = thread->GetEcmaVM()->GetFactory()->NewFromStdString("你好，世界");
        auto ecmaStringCode1 = EcmaStringAccessor(ecmaString).GetHashcode();
        auto ecmaString1Code = EcmaStringAccessor(ecmaString1).GetHashcode();
        EXPECT_TRUE(ecmaStringCode1 == ecmaString1Code) << "Not same HashCode";
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*ecmaString, *ecmaString1)) << "Not same EcmaString";

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ecmaString fail";
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        auto ecmaStringCode2 = EcmaStringAccessor(ecmaString).GetHashcode();
        auto resEcmaStringCode = EcmaStringAccessor(resEcmaString).GetHashcode();
        EXPECT_TRUE(ecmaStringCode2 == resEcmaStringCode) << "Not same HashCode";
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void Int32Test(std::pair<uint8_t *, size_t> data)
    {
        Init();
        int32_t a = 64;
        int32_t min = -2147483648;
        int32_t b = -63;
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> resA = deserializer.Deserialize();
        JSHandle<JSTaggedValue> resMin = deserializer.Deserialize();
        JSHandle<JSTaggedValue> resB = deserializer.Deserialize();
        EXPECT_TRUE(!resA.IsEmpty() && !resMin.IsEmpty() && !resB.IsEmpty()) << "[Empty] Deserialize Int32 fail";
        EXPECT_TRUE(resA->IsInt() && resMin->IsInt() && resB->IsInt()) << "[NotInt] Deserialize Int32 fail";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resA) == a) << "Not Same Value";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resMin) == min) << "Not Same Value";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resB) == b) << "Not Same Value";
        Destroy();
    }

    void DoubleTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        double a = 3.1415926535;
        double b = -3.1415926535;
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> resA = deserializer.Deserialize();
        JSHandle<JSTaggedValue> resB = deserializer.Deserialize();
        EXPECT_TRUE(!resA.IsEmpty() && !resB.IsEmpty()) << "[Empty] Deserialize double fail";
        EXPECT_TRUE(resA->IsDouble() && resB->IsDouble()) << "[NotInt] Deserialize double fail";
        EXPECT_TRUE(resA->GetDouble() == a) << "Not Same Value";
        EXPECT_TRUE(resB->GetDouble() == b) << "Not Same Value";
        Destroy();
    }

    void JSDateTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        double tm = 28 * 60 * 60 * 1000;  // 28 * 60 * 60 * 1000 : test case
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSDate fail";
        EXPECT_TRUE(res->IsDate()) << "[NotJSDate] Deserialize JSDate fail";
        JSHandle<JSDate> resDate = JSHandle<JSDate>(res);
        EXPECT_TRUE(resDate->GetTimeValue() == JSTaggedValue(tm)) << "Not Same Time Value";
        Destroy();
    }

    void JSMapTest(std::pair<uint8_t *, size_t> data, const JSHandle<JSMap> &originMap)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSMap fail";
        EXPECT_TRUE(res->IsJSMap()) << "[NotJSMap] Deserialize JSMap fail";
        JSHandle<JSMap> resMap = JSHandle<JSMap>::Cast(res);
        EXPECT_TRUE(originMap->GetSize() == resMap->GetSize()) << "the map size Not equal";
        uint32_t resSize = static_cast<uint32_t>(resMap->GetSize());
        for (uint32_t i = 0; i < resSize; i++) {
            JSHandle<JSTaggedValue> resKey(thread, resMap->GetKey(i));
            JSHandle<JSTaggedValue> resValue(thread, resMap->GetValue(i));
            JSHandle<JSTaggedValue> key(thread, originMap->GetKey(i));
            JSHandle<JSTaggedValue> value(thread, originMap->GetValue(i));

            JSHandle<EcmaString> resKeyStr = JSHandle<EcmaString>::Cast(resKey);
            JSHandle<EcmaString> keyStr = JSHandle<EcmaString>::Cast(key);
            EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*resKeyStr, *keyStr)) << "Not same map key";
            EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resValue) == JSTaggedValue::ToInt32(thread, value))
                << "Not same map value";
        }
        Destroy();
    }

    void JSArrayBufferTest(std::pair<uint8_t *, size_t> data,
                           const JSHandle<JSArrayBuffer> &originArrayBuffer, int32_t byteLength, const char *msg)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSArrayBuffer fail";
        EXPECT_TRUE(res->IsArrayBuffer()) << "[NotJSArrayBuffer] Deserialize JSArrayBuffer fail";
        JSHandle<JSArrayBuffer> resJSArrayBuffer = JSHandle<JSArrayBuffer>::Cast(res);
        int32_t resByteLength = static_cast<int32_t>(resJSArrayBuffer->GetArrayBufferByteLength());
        EXPECT_TRUE(resByteLength == byteLength) << "Not Same ByteLength"; // 10 : test case

        JSHandle<JSTaggedValue> bufferData(thread, originArrayBuffer->GetArrayBufferData());
        auto np = JSHandle<JSNativePointer>::Cast(bufferData);
        void *buffer = np->GetExternalPointer();
        ASSERT_NE(buffer, nullptr);
        JSHandle<JSTaggedValue> resBufferData(thread, resJSArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        ASSERT_NE(resBuffer, nullptr);

        for (int32_t i = 0; i < resByteLength; i++) {
            EXPECT_TRUE(static_cast<char *>(resBuffer)[i] == static_cast<char *>(buffer)[i]) << "Not Same Buffer";
        }

        if (msg != nullptr) {
            if (memcpy_s(resBuffer, byteLength, msg, byteLength) != EOK) {
                EXPECT_TRUE(false) << " memcpy error!";
            }
        }
        Destroy();
    }

    void JSSharedArrayBufferTest(std::pair<uint8_t *, size_t> data,
                           const JSHandle<JSArrayBuffer> &originArrayBuffer, int32_t byteLength, const char *msg)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSArrayBuffer fail";
        EXPECT_TRUE(res->IsSharedArrayBuffer()) << "[NotJSArrayBuffer] Deserialize JSArrayBuffer fail";
        JSHandle<JSArrayBuffer> resJSArrayBuffer = JSHandle<JSArrayBuffer>::Cast(res);
        int32_t resByteLength = static_cast<int32_t>(resJSArrayBuffer->GetArrayBufferByteLength());
        EXPECT_TRUE(resByteLength == byteLength) << "Not Same ByteLength";
        JSHandle<JSTaggedValue> bufferData(thread, originArrayBuffer->GetArrayBufferData());
        auto np = JSHandle<JSNativePointer>::Cast(bufferData);
        void *buffer = np->GetExternalPointer();
        ASSERT_NE(buffer, nullptr);
        JSHandle<JSTaggedValue> resBufferData(thread, resJSArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        ASSERT_NE(resBuffer, nullptr);
        EXPECT_TRUE((uint64_t)buffer == (uint64_t)resBuffer) << "Not Same pointer!";
        for (int32_t i = 0; i < resByteLength; i++) {
            EXPECT_TRUE(static_cast<char *>(resBuffer)[i] == static_cast<char *>(buffer)[i]) << "Not Same Buffer";
        }

        if (msg != nullptr) {
            if (memcpy_s(resBuffer, byteLength, msg, byteLength) != EOK) {
                EXPECT_TRUE(false) << " memcpy error!";
            }
        }
        Destroy();
    }

    void JSSharedArrayBufferTest1(std::pair<uint8_t *, size_t> data, int32_t byteLength, const char *msg)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSArrayBuffer fail";
        EXPECT_TRUE(res->IsSharedArrayBuffer()) << "[NotJSArrayBuffer] Deserialize JSArrayBuffer fail";
        JSHandle<JSArrayBuffer> resJSArrayBuffer = JSHandle<JSArrayBuffer>::Cast(res);
        JSHandle<JSTaggedValue> resBufferData(thread, resJSArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        if (msg != nullptr) {
            if (memcpy_s(resBuffer, byteLength, msg, byteLength) != EOK) {
                EXPECT_TRUE(false) << " memcpy error!";
            }
        }
        Destroy();
    }

    void JSRegexpTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromASCII("key2");
        JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromASCII("i");
        char buffer[] = "1234567";  // use char buffer to simulate byteCodeBuffer
        uint32_t bufferSize = 7;

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSRegExp fail";
        EXPECT_TRUE(res->IsJSRegExp()) << "[NotJSRegexp] Deserialize JSRegExp fail";
        JSHandle<JSRegExp> resJSRegexp(res);

        uint32_t resBufferSize = resJSRegexp->GetLength();
        EXPECT_TRUE(resBufferSize == bufferSize) << "Not Same Length";
        JSHandle<JSTaggedValue> originalSource(thread, resJSRegexp->GetOriginalSource());
        EXPECT_TRUE(originalSource->IsString());
        JSHandle<JSTaggedValue> originalFlags(thread, resJSRegexp->GetOriginalFlags());
        EXPECT_TRUE(originalFlags->IsString());
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*JSHandle<EcmaString>(originalSource), *pattern));
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*JSHandle<EcmaString>(originalFlags), *flags));

        JSHandle<JSTaggedValue> resBufferData(thread, resJSRegexp->GetByteCodeBuffer());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        ASSERT_NE(resBuffer, nullptr);

        for (uint32_t i = 0; i < resBufferSize; i++) {
            EXPECT_TRUE(static_cast<char *>(resBuffer)[i] == buffer[i]) << "Not Same ByteCode";
        }
        Destroy();
    }

    void TypedArrayTest1(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<JSTaggedValue> originTypedArrayName(thread, thread->GlobalConstants()->GetInt8ArrayString());
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TypedArray fail";
        EXPECT_TRUE(res->IsJSInt8Array()) << "[NotJSInt8Array] Deserialize TypedArray fail";
        JSHandle<JSTypedArray> resJSInt8Array = JSHandle<JSTypedArray>::Cast(res);

        JSHandle<JSTaggedValue> typedArrayName(thread, resJSInt8Array->GetTypedArrayName());
        uint32_t byteLength = resJSInt8Array->GetByteLength();
        uint32_t byteOffset = resJSInt8Array->GetByteOffset();
        uint32_t arrayLength = resJSInt8Array->GetArrayLength();
        ContentType contentType = resJSInt8Array->GetContentType();
        JSHandle<JSTaggedValue> viewedArrayBuffer(thread, resJSInt8Array->GetViewedArrayBufferOrByteArray());

        EXPECT_TRUE(typedArrayName->IsString());
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*JSHandle<EcmaString>(typedArrayName),
                                                        *JSHandle<EcmaString>(originTypedArrayName)));
        EXPECT_EQ(byteLength, 10) << "Not Same ByteLength"; // 10: bufferLength
        EXPECT_EQ(byteOffset, 0) << "Not Same ByteOffset";
        EXPECT_EQ(arrayLength, 10) << "Not Same ArrayLength"; // 10: arrayLength
        EXPECT_TRUE(contentType == ContentType::Number) << "Not Same ContentType";

        // check arrayBuffer
        EXPECT_TRUE(viewedArrayBuffer->IsArrayBuffer());
        JSHandle<JSArrayBuffer> resJSArrayBuffer(viewedArrayBuffer);
        uint32_t resTaggedLength = resJSArrayBuffer->GetArrayBufferByteLength();
        EXPECT_EQ(resTaggedLength, 10) << "Not same viewedBuffer length"; // 10: bufferLength
        JSHandle<JSTaggedValue> resBufferData(thread, resJSArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        for (uint32_t i = 0; i < resTaggedLength; i++) {
            EXPECT_EQ(static_cast<uint8_t *>(resBuffer)[i], i) << "Not same viewedBuffer";
        }
        Destroy();
    }

    void TypedArrayTest2(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<JSTaggedValue> originTypedArrayName(thread, thread->GlobalConstants()->GetInt8ArrayString());
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TypedArray fail";
        EXPECT_TRUE(res->IsJSInt8Array()) << "[NotJSInt8Array] Deserialize TypedArray fail";
        JSHandle<JSTypedArray> resJSInt8Array = JSHandle<JSTypedArray>::Cast(res);

        JSHandle<JSTaggedValue> typedArrayName(thread, resJSInt8Array->GetTypedArrayName());
        uint32_t byteLength = resJSInt8Array->GetByteLength();
        uint32_t byteOffset = resJSInt8Array->GetByteOffset();
        uint32_t arrayLength = resJSInt8Array->GetArrayLength();
        ContentType contentType = resJSInt8Array->GetContentType();
        JSHandle<JSTaggedValue> byteArray(thread, resJSInt8Array->GetViewedArrayBufferOrByteArray());

        EXPECT_TRUE(typedArrayName->IsString());
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*JSHandle<EcmaString>(typedArrayName),
                                                        *JSHandle<EcmaString>(originTypedArrayName)));
        EXPECT_EQ(byteLength, 10) << "Not Same ByteLength"; // 10: bufferLength
        EXPECT_EQ(byteOffset, 0) << "Not Same ByteOffset";
        EXPECT_EQ(arrayLength, 10) << "Not Same ArrayLength"; // 10: arrayLength
        EXPECT_TRUE(contentType == ContentType::Number) << "Not Same ContentType";

        // check byteArray
        EXPECT_TRUE(byteArray->IsByteArray());
        JSHandle<ByteArray> resByteArray(byteArray);
        uint32_t resTaggedLength = resByteArray->GetArrayLength();
        EXPECT_EQ(resTaggedLength, 10) << "Not same viewedBuffer length"; // 10: bufferLength
        uint32_t resElementSize = resByteArray->GetByteLength();
        EXPECT_EQ(resElementSize, 1) << "Not same byteArray size";
        for (uint32_t i = 0; i < resTaggedLength; i++) {
            JSHandle<JSTaggedValue> taggedVal(thread, resByteArray->Get(thread, i, DataViewType::UINT8));
            int32_t byteArrayVal = JSTaggedValue::ToInt32(thread, taggedVal);
            EXPECT_EQ(byteArrayVal, 255) << "Not same byteArray value"; // 255: value in byteArray
        }
        Destroy();
    }

    void TaggedArrayTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TaggedArray fail";
        EXPECT_TRUE(res.GetTaggedValue().IsTaggedArray()) << "[NotTaggedArray] Deserialize TaggedArray fail";

        // check taggedArray
        JSHandle<TaggedArray> taggedArray = JSHandle<TaggedArray>::Cast(res);
        EXPECT_EQ(taggedArray->GetLength(), 4U); // 4 : test case
        EcmaString *str11 = reinterpret_cast<EcmaString *>(taggedArray->Get(0).GetTaggedObject());
        EcmaString *str22 = reinterpret_cast<EcmaString *>(taggedArray->Get(1).GetTaggedObject());
        EXPECT_EQ(std::strcmp(EcmaStringAccessor(str11).ToCString().c_str(), "str11"), 0);
        EXPECT_EQ(std::strcmp(EcmaStringAccessor(str22).ToCString().c_str(), "str22"), 0);
        EXPECT_TRUE(taggedArray->Get(2).IsUndefined()); // 2: the second index
        Destroy();
    }

    void MethodTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize Method fail";
        EXPECT_TRUE(res.GetTaggedValue().IsMethod()) << "[NotMethod] Deserialize Method fail";

        JSHandle<Method> method = JSHandle<Method>::Cast(res);
        EXPECT_TRUE(method->IsNativeWithCallField());
        Destroy();
    }

    void MethodTest1(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize Method fail";
        EXPECT_TRUE(res.GetTaggedValue().IsMethod()) << "[NotMethod] Deserialize Method fail";

        JSHandle<Method> method = JSHandle<Method>::Cast(res);
        EXPECT_FALSE(method->IsNativeWithCallField());

        JSHandle<ConstantPool> constpool(thread, method->GetConstantPool());
        EXPECT_EQ(constpool->GetLength(), 4U);
        EXPECT_EQ(constpool->GetCacheLength(), 2U);
        const JSPandaFile *jsPandaFile = constpool->GetJSPandaFile();
        EXPECT_TRUE(jsPandaFile != nullptr);
        const CString &desc = jsPandaFile->GetJSPandaFileDesc();
        EXPECT_EQ(desc, "test.pa");
        Destroy();
    }

    void AOTMethodTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize Method fail";
        EXPECT_TRUE(res.GetTaggedValue().IsMethod()) << "[NotMethod] Deserialize Method fail";

        JSHandle<Method> method = JSHandle<Method>::Cast(res);
        EXPECT_FALSE(method->IsNativeWithCallField());
        EXPECT_TRUE(method->IsAotWithCallField());
        EXPECT_TRUE(method->IsFastCall());
        uintptr_t codeEntry = method->GetCodeEntryOrLiteral();
        EXPECT_EQ(codeEntry, 0x1234);
        Destroy();
    }

    void ConcurrentFunctionTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ConcurrentFunction fail";
        EXPECT_TRUE(res->IsJSFunction()) << "[NotJSFunction] Deserialize ConcurrentFunction fail";
        Destroy();
    }

    void BigIntTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserialzier(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res1 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res2 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res3 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res4 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res5 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res6 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res7 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res8 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res9 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res10 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res11 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res12 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res13 = deserialzier.Deserialize();
        JSHandle<JSTaggedValue> res14 = deserialzier.Deserialize();
        EXPECT_TRUE(!res1.IsEmpty() && !res14.IsEmpty()) << "[Empty] Deserialize BigInt fail";
        EXPECT_TRUE(res3->IsBigInt() && res12->IsBigInt()) << "[NotBigInt] Deserialize BigInt fail";

        CString str1 = "100000000000000000000000000000000000000000000000000000"; // Binary: 2 ^ 53
        CString str2 = "111111111111111111111111111111111111111111111111111111"; // Binary: 2 ^ 54 - 1
        CString str3 = "-9007199254740991012345";
        CString str4 = "1234567890987654321"; // Decimal
        JSHandle<BigInt> bigInt1 = BigIntHelper::SetBigInt(thread, str1);
        JSHandle<BigInt> bigInt2 = BigIntHelper::SetBigInt(thread, str2);
        JSHandle<BigInt> bigInt3 = BigIntHelper::SetBigInt(thread, str3);
        JSHandle<BigInt> bigInt4 = BigInt::Uint32ToBigInt(thread, 0); // 0 : test case
        JSHandle<BigInt> bigInt5 = BigIntHelper::SetBigInt(thread, str4, BigInt::DECIMAL);
        JSHandle<BigInt> bigInt6 = BigInt::Uint64ToBigInt(thread, ULLONG_MAX);
        JSHandle<BigInt> bigInt7 = BigInt::Uint64ToBigInt(thread, UINT_MAX);
        JSHandle<BigInt> bigInt8 = BigInt::Uint32ToBigInt(thread, std::numeric_limits<uint32_t>::max());
        JSHandle<BigInt> bigInt9 = BigInt::Uint32ToBigInt(thread, std::numeric_limits<uint32_t>::min());
        JSHandle<BigInt> bigInt10 = BigInt::Int64ToBigInt(thread, LLONG_MAX);
        JSHandle<BigInt> bigInt11 = BigInt::Int64ToBigInt(thread, LLONG_MIN);
        JSHandle<BigInt> bigInt12 = BigInt::Int64ToBigInt(thread, INT_MAX);
        JSHandle<BigInt> bigInt13 = BigInt::Int64ToBigInt(thread, INT_MIN);
        JSHandle<BigInt> bigInt14 = BigInt::Int32ToBigInt(thread, -1); // -1 : test case
        EXPECT_TRUE(BigInt::Equal(res1.GetTaggedValue(), bigInt1.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res2.GetTaggedValue(), bigInt2.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res3.GetTaggedValue(), bigInt3.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res4.GetTaggedValue(), bigInt4.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res5.GetTaggedValue(), bigInt5.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res6.GetTaggedValue(), bigInt6.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res7.GetTaggedValue(), bigInt7.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res8.GetTaggedValue(), bigInt8.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res9.GetTaggedValue(), bigInt9.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res10.GetTaggedValue(), bigInt10.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res11.GetTaggedValue(), bigInt11.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res12.GetTaggedValue(), bigInt12.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res13.GetTaggedValue(), bigInt13.GetTaggedValue())) << "Not Same BigInt";
        EXPECT_TRUE(BigInt::Equal(res14.GetTaggedValue(), bigInt14.GetTaggedValue())) << "Not Same BigInt";
        Destroy();
    }

    void ObjectWithConcurrentFunctionTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ObjectWithConcurrentFunction fail";
        EXPECT_TRUE(res->IsObject()) <<
            "[NotObjectWithConcurrentFunction] Deserialize ObjectWithConcurrentFunction fail";

        JSHandle<JSTaggedValue> key1(factory->NewFromASCII("2"));
        OperationResult result1 = JSObject::GetProperty(thread, res, key1);
        JSHandle<JSTaggedValue> value1 = result1.GetRawValue();
        EXPECT_TRUE(value1->IsJSFunction());
        JSHandle<JSTaggedValue> key2(factory->NewFromASCII("abc"));
        OperationResult result2 = JSObject::GetProperty(thread, res, key2);
        JSHandle<JSTaggedValue> value2 = result2.GetRawValue();
        EXPECT_TRUE(value2->IsString());
        JSHandle<JSTaggedValue> key3(factory->NewFromASCII("4"));
        OperationResult result3 = JSObject::GetProperty(thread, res, key3);
        JSHandle<JSTaggedValue> value3 = result3.GetRawValue();
        EXPECT_TRUE(value3->IsJSFunction());
        JSHandle<JSTaggedValue> key4(factory->NewFromASCII("key"));
        OperationResult result4 = JSObject::GetProperty(thread, res, key4);
        JSHandle<JSTaggedValue> value4 = result4.GetRawValue();
        EXPECT_TRUE(value4->IsString());
        Destroy();
    }

    void TransferJSArrayBufferTest1(std::pair<uint8_t *, size_t> data, uintptr_t bufferAddrCheck)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TransferJSArrayBuffer1 fail";
        EXPECT_TRUE(res->IsArrayBuffer()) << "[NotJSArrayBuffer] Deserialize TransferJSArrayBuffer1 fail";

        JSHandle<JSArrayBuffer> arrBuf = JSHandle<JSArrayBuffer>::Cast(res);
        EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 5); // 5: bufferLength
        JSHandle<JSTaggedValue> nativePtr(thread, arrBuf->GetArrayBufferData());
        EXPECT_TRUE(nativePtr->IsJSNativePointer()) << "[NotJSNativePointer] Deserialize TransferJSArrayBuffer1 fail";
        JSHandle<JSNativePointer> np = JSHandle<JSNativePointer>::Cast(nativePtr);
        uintptr_t bufferAddr = reinterpret_cast<uintptr_t>(np->GetExternalPointer());
        // The deserialized C buffer pointer shall be same to the original one
        EXPECT_EQ(static_cast<uint64_t>(bufferAddr), static_cast<uint64_t>(bufferAddrCheck));
        Destroy();
    }

    void TransferJSArrayBufferTest2(std::pair<uint8_t *, size_t> data, uintptr_t bufferAddrCheck)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TransferJSArrayBuffer2 fail";
        EXPECT_TRUE(res->IsArrayBuffer()) << "[NotJSArrayBuffer] Deserialize TransferJSArrayBuffer2 fail";

        JSHandle<JSArrayBuffer> arrBuf = JSHandle<JSArrayBuffer>::Cast(res);
        EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 5); // 5: bufferLength
        JSHandle<JSTaggedValue> nativePtr(thread, arrBuf->GetArrayBufferData());
        EXPECT_TRUE(nativePtr->IsJSNativePointer()) << "[NotJSNativePointer] Deserialize TransferJSArrayBuffer2 fail";
        JSHandle<JSNativePointer> np = JSHandle<JSNativePointer>::Cast(nativePtr);
        uintptr_t bufferAddr = reinterpret_cast<uintptr_t>(np->GetExternalPointer());
        // The deserialized C buffer pointer shall be different to the original one
        EXPECT_NE(static_cast<uint64_t>(bufferAddr), static_cast<uint64_t>(bufferAddrCheck));
        Destroy();
    }

    void TransferJSArrayBufferTest3(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TransferJSArrayBuffer3 fail";
        EXPECT_TRUE(res->IsArrayBuffer()) << "[NotJSArrayBuffer] Deserialize TransferJSArrayBuffer3 fail";

        JSHandle<JSArrayBuffer> arrBuf = JSHandle<JSArrayBuffer>::Cast(res);
        EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 0);
        JSHandle<JSTaggedValue> nativePtr(thread, arrBuf->GetArrayBufferData());
        EXPECT_TRUE(nativePtr->IsUndefined()) << "[NotJSNativePointer] Deserialize TransferJSArrayBuffer3 fail";
        Destroy();
    }

    void TransferJSArrayBufferTest4(std::pair<uint8_t *, size_t> data, uintptr_t bufferAddrCheck)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TransferJSArrayBuffer4 fail";
        EXPECT_TRUE(res->IsArrayBuffer()) << "[NotJSArrayBuffer] Deserialize TransferJSArrayBuffer4 fail";

        JSHandle<JSArrayBuffer> arrBuf = JSHandle<JSArrayBuffer>::Cast(res);
        EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 5); // 5: bufferLength
        JSHandle<JSTaggedValue> nativePtr(thread, arrBuf->GetArrayBufferData());
        EXPECT_TRUE(nativePtr->IsJSNativePointer()) << "[NotJSNativePointer] Deserialize TransferJSArrayBuffer4 fail";
        JSHandle<JSNativePointer> np = JSHandle<JSNativePointer>::Cast(nativePtr);
        uintptr_t bufferAddr = reinterpret_cast<uintptr_t>(np->GetExternalPointer());
        // The deserialized C buffer pointer shall be same to the original one
        EXPECT_EQ(static_cast<uint64_t>(bufferAddr), static_cast<uint64_t>(bufferAddrCheck));
        Destroy();
    }

    void TransferJSArrayBufferTest5(std::pair<uint8_t *, size_t> data, uintptr_t bufferAddrCheck)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TransferJSArrayBuffer5 fail";
        EXPECT_TRUE(res->IsJSInt8Array()) << "[NotJSArrayBuffer] Deserialize TransferJSArrayBuffer5 fail";

        JSHandle<JSTypedArray> resJSInt8Array = JSHandle<JSTypedArray>::Cast(res);

        JSHandle<JSTaggedValue> typedArrayName(thread, resJSInt8Array->GetTypedArrayName());
        uint32_t byteLength = resJSInt8Array->GetByteLength();
        uint32_t byteOffset = resJSInt8Array->GetByteOffset();
        uint32_t arrayLength = resJSInt8Array->GetArrayLength();
        ContentType contentType = resJSInt8Array->GetContentType();
        JSHandle<JSTaggedValue> viewedArrayBuffer(thread, resJSInt8Array->GetViewedArrayBufferOrByteArray());
        EXPECT_EQ(byteLength, 10) << "Not Same ByteLength"; // 10: bufferLength
        EXPECT_EQ(byteOffset, 0) << "Not Same ByteOffset";
        EXPECT_EQ(arrayLength, 10) << "Not Same ArrayLength"; // 10: arrayLength
        EXPECT_TRUE(contentType == ContentType::Number) << "Not Same ContentType";

        // check arrayBuffer
        EXPECT_TRUE(viewedArrayBuffer->IsArrayBuffer());
        JSHandle<JSArrayBuffer> resJSArrayBuffer(viewedArrayBuffer);
        uint32_t resTaggedLength = resJSArrayBuffer->GetArrayBufferByteLength();
        EXPECT_EQ(resTaggedLength, 10) << "Not same viewedBuffer length"; // 10: bufferLength
        JSHandle<JSTaggedValue> resBufferData(thread, resJSArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        for (uint32_t i = 0; i < resTaggedLength; i++) {
            EXPECT_EQ(static_cast<uint8_t *>(resBuffer)[i], i) << "Not same viewedBuffer";
        }
        JSHandle<JSTaggedValue> nativePtr(thread, resJSArrayBuffer->GetArrayBufferData());
        EXPECT_TRUE(nativePtr->IsJSNativePointer()) << "[NotJSNativePointer] Deserialize TransferJSArrayBuffer5 fail";
        JSHandle<JSNativePointer> np = JSHandle<JSNativePointer>::Cast(nativePtr);
        uintptr_t bufferAddr = reinterpret_cast<uintptr_t>(np->GetExternalPointer());
        // The deserialized C buffer pointer shall be same to the original one
        EXPECT_EQ(static_cast<uint64_t>(bufferAddr), static_cast<uint64_t>(bufferAddrCheck));
        Destroy();
    }

private:
    EcmaVM *ecmaVm = nullptr;
    EcmaHandleScope *scope = nullptr;
    JSThread *thread = nullptr;
};

class JSSerializerTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

    JSThread *thread {nullptr};
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
};

HWTEST_F_L0(JSSerializerTest, SerializeJSSpecialValue)
{
    JSSerializer *serializer = new JSSerializer(thread);
    JSHandle<JSTaggedValue> jsTrue(thread, JSTaggedValue::True());
    JSHandle<JSTaggedValue> jsFalse(thread, JSTaggedValue::False());
    JSHandle<JSTaggedValue> jsUndefined(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> jsNull(thread, JSTaggedValue::Null());
    JSHandle<JSTaggedValue> jsHole(thread, JSTaggedValue::Hole());
    serializer->SerializeJSTaggedValue(jsTrue);
    serializer->SerializeJSTaggedValue(jsFalse);
    serializer->SerializeJSTaggedValue(jsUndefined);
    serializer->SerializeJSTaggedValue(jsNull);
    serializer->SerializeJSTaggedValue(jsHole);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSSpecialValueTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject1)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("2"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("3"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("y"));
    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> key6(factory->NewFromASCII("b"));
    JSHandle<JSTaggedValue> key7(factory->NewFromASCII("5"));
    JSHandle<JSTaggedValue> key8(factory->NewFromASCII("6"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value4(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> value5(thread, JSTaggedValue(5));
    JSHandle<JSTaggedValue> value6(thread, JSTaggedValue(6));
    JSHandle<JSTaggedValue> value7(thread, JSTaggedValue(7));
    JSHandle<JSTaggedValue> value8(thread, JSTaggedValue(8));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key3, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key4, value4);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key5, value5);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key6, value6);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key7, value7);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key8, value8);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj2));

    EXPECT_TRUE(success1);
    EXPECT_TRUE(success2);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSPlainObjectTest1, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject2)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("diagnosisItem"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("detectStatus"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("detectResultItems"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("faultId"));
    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("faultContent"));
    JSHandle<JSTaggedValue> key6(factory->NewFromASCII("faultContentParams"));
    JSHandle<JSTaggedValue> key7(factory->NewFromASCII("adviceId"));
    JSHandle<JSTaggedValue> key8(factory->NewFromASCII("adviceContent"));
    JSHandle<JSTaggedValue> key9(factory->NewFromASCII("adviceContentParams"));
    JSHandle<JSTaggedValue> value1 = JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("VoiceDetect1"));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value3 = JSHandle<JSTaggedValue>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> value4 = JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("80000001"));
    JSHandle<JSTaggedValue> value5 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
    JSHandle<JSTaggedValue> value6 = JSHandle<JSTaggedValue>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> value7 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
    JSHandle<JSTaggedValue> value8 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
    JSHandle<JSTaggedValue> value9 = JSHandle<JSTaggedValue>::Cast(factory->NewJSArray());

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key4, value4);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key5, value5);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key6, value6);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key7, value7);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key8, value8);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key9, value9);
    JSArray::FastSetPropertyByValue(thread, value3, 0, JSHandle<JSTaggedValue>(obj2));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key3, value3);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));

    EXPECT_TRUE(success1);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSPlainObjectTest2, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

static void* detach(void *param1, void *param2, void *hint)
{
    GTEST_LOG_(INFO) << "detach is running";
    if (param1 == nullptr && param2 == nullptr) {
        GTEST_LOG_(INFO) << "detach: two params is nullptr";
    }
    if (hint == nullptr) {
        GTEST_LOG_(INFO) << "detach: hint is nullptr";
    }
    return nullptr;
}

static void* attach([[maybe_unused]] void *enginePointer, [[maybe_unused]] void *buffer, [[maybe_unused]] void *hint)
{
    GTEST_LOG_(INFO) << "attach is running";
    return nullptr;
}

HWTEST_F_L0(JSSerializerTest, SerializeNativeBindingObject1)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj3 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1 = env->GetDetachSymbol();
    JSHandle<JSTaggedValue> key2 = env->GetAttachSymbol();
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("y"));
    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> key6(factory->NewFromASCII("b"));
    JSHandle<JSTaggedValue> key7(factory->NewFromASCII("5"));
    JSHandle<JSTaggedValue> key8(factory->NewFromASCII("6"));
    JSHandle<JSTaggedValue> value1(factory->NewJSNativePointer(reinterpret_cast<void*>(detach)));
    JSHandle<JSTaggedValue> value2(factory->NewJSNativePointer(reinterpret_cast<void*>(attach)));
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value4(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value5(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value6(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> value7(thread, JSTaggedValue(5));
    JSHandle<JSTaggedValue> value8(thread, JSTaggedValue(6));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key3, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key4, value4);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key5, value5);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key6, value6);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key7, value7);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key8, value8);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj3), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj3), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj3), key3, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj3), key4, value4);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj2));
    bool success3 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj3));

    EXPECT_TRUE(success1);
    EXPECT_TRUE(success2);
    EXPECT_TRUE(success3);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::NativeBindingObjectTest1, jsDeserializerTest, data);
    t1.join();
    delete serializer;
}

// not support native object without detach and attach
HWTEST_F_L0(JSSerializerTest, SerializeNativeBindingObject2)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("abc"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("xyz"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("6"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("8"));
    JSHandle<JSTaggedValue> key5 = env->GetAttachSymbol();
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(6));
    JSHandle<JSTaggedValue> value2 = JSHandle<JSTaggedValue>::Cast(factory->NewFromASCII("VALUE"));
    JSHandle<JSTaggedValue> value3 = JSHandle<JSTaggedValue>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> value4 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
    JSHandle<JSTaggedValue> value5(factory->NewJSNativePointer(reinterpret_cast<void*>(attach)));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key3, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key4, value4);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key5, value5);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj2));

    EXPECT_TRUE(success1);
    EXPECT_FALSE(success2);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::NativeBindingObjectTest2, jsDeserializerTest, data);
    t1.join();
    delete serializer;
}

HWTEST_F_L0(JSSerializerTest, SerializeNativeBindingObject3)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1 = env->GetDetachSymbol();
    JSHandle<JSTaggedValue> key2 = env->GetAttachSymbol();
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("root1"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("root2"));
    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("root3"));
    JSHandle<JSTaggedValue> key6(factory->NewFromASCII("name"));
    JSHandle<JSTaggedValue> key7(factory->NewFromASCII("version"));
    JSHandle<JSTaggedValue> key8(factory->NewFromASCII("product"));
    JSHandle<JSTaggedValue> key9(factory->NewFromASCII("os"));
    JSHandle<JSTaggedValue> key10(factory->NewFromASCII("system"));
    JSHandle<JSTaggedValue> value1(factory->NewJSNativePointer(reinterpret_cast<void*>(detach)));
    JSHandle<JSTaggedValue> value2(factory->NewJSNativePointer(reinterpret_cast<void*>(attach)));
    JSHandle<JSTaggedValue> value3(factory->NewFromASCII(""));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key3, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key4, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key5, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key6, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key7, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key8, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key9, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key10, value3);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj2));
    EXPECT_TRUE(success1);
    EXPECT_TRUE(success2);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::NativeBindingObjectTest3, jsDeserializerTest, data);
    t1.join();
    delete serializer;
}

HWTEST_F_L0(JSSerializerTest, TestSerializeDescription)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> key2(thread->GetEcmaVM()->GetFactory()->NewFromASCII("y"));

    PropertyDescriptor desc1(thread);
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    desc1.SetValue(value1);
    desc1.SetWritable(true);
    desc1.SetEnumerable(false);
    desc1.SetConfigurable(true);
    JSObject::DefineOwnProperty(thread, obj, key1, desc1);

    PropertyDescriptor desc2(thread);
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    desc2.SetValue(value2);
    desc2.SetWritable(false);
    desc2.SetEnumerable(true);
    desc2.SetConfigurable(false);
    JSObject::DefineOwnProperty(thread, obj, key2, desc2);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::DescriptionTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, TestSerializeJSSet)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> linkedSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, linkedSet);
    // set property to set
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(7));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(9));
    JSHandle<JSTaggedValue> value3(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value4(factory->NewFromASCII("y"));

    JSSet::Add(thread, set, value1);
    JSSet::Add(thread, set, value2);
    JSSet::Add(thread, set, value3);
    JSSet::Add(thread, set, value4);

    // set property to object
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("5"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("6"));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(set), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(set), key2, value2);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(set));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSSetTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, TestSerializeJSArray)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSArray> array = factory->NewJSArray();

    // set property to object
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("abasd"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("qweqwedasd"));

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(7));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(9));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(array), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(array), key2, value2);

    // set value to array
    array->SetArrayLength(thread, 20);
    for (int i = 0; i < 20; i++) {
        JSHandle<JSTaggedValue> data(thread, JSTaggedValue(i));
        JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>::Cast(array), i, data);
    }

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(array));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSArrayTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

// Test the situation that Objects' properties stores values that reference with each other
HWTEST_F_L0(JSSerializerTest, TestObjectsPropertyReference)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();
    [[maybe_unused]] JSHandle<JSObject> obj3 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("abc"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("def"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("dgsdgf"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("qwjhrf"));

    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(10));
    JSHandle<JSTaggedValue> value4(thread, JSTaggedValue(5));

    // set property to obj1
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj1), key1, JSHandle<JSTaggedValue>::Cast(obj2));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj1), key3, value3);

    // set property to obj2
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj2), key2, JSHandle<JSTaggedValue>::Cast(obj1));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj2), key4, value4);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj2));
    EXPECT_TRUE(success1) << "Serialize obj1 fail";
    EXPECT_TRUE(success2) << "Serialize obj2 fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::ObjectsPropertyReferenceTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeEcmaString1)
{
    const char *rawStr = "this is a test ecmaString";
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromASCII(rawStr);
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(ecmaString));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest1, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeEcmaString2)
{
    const char *rawStr = "ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "ssssss";
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromASCII(rawStr);
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(ecmaString));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest2, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeEcmaString3)
{
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->GetEmptyString();
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(ecmaString));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest3, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

// Test EcmaString contains Chinese Text
HWTEST_F_L0(JSSerializerTest, SerializeEcmaString4)
{
    std::string rawStr = "你好，世界";
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromStdString(rawStr);
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(ecmaString));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest4, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeInt32_t)
{
    JSSerializer *serializer = new JSSerializer(thread);
    int32_t a = 64, min = -2147483648, b = -63;
    JSTaggedValue aTag(a), minTag(min), bTag(b);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, aTag));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, minTag));
    bool success3 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, bTag));
    EXPECT_TRUE(success1 && success2 && success3) << "Serialize Int32 fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::Int32Test, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeDouble)
{
    JSSerializer *serializer = new JSSerializer(thread);
    double a = 3.1415926535, b = -3.1415926535;
    JSTaggedValue aTag(a), bTag(b);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, aTag));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, bTag));
    EXPECT_TRUE(success1 && success2) << "Serialize Double fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::DoubleTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

JSDate *JSDateCreate(EcmaVM *ecmaVM)
{
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateFunction = globalEnv->GetDateFunction();
    JSHandle<JSDate> dateObject =
        JSHandle<JSDate>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dateFunction), dateFunction));
    return *dateObject;
}

HWTEST_F_L0(JSSerializerTest, SerializeDate)
{
    double tm = 28 * 60 * 60 * 1000;
    JSHandle<JSDate> jsDate(thread, JSDateCreate(ecmaVm));
    jsDate->SetTimeValue(thread, JSTaggedValue(tm));
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsDate));
    EXPECT_TRUE(success) << "Serialize JSDate fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSDateTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

JSMap *CreateMap(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> map =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> linkedMap = LinkedHashMap::Create(thread);
    map->SetLinkedMap(thread, linkedMap);
    return *map;
}

HWTEST_F_L0(JSSerializerTest, SerializeJSMap)
{
    JSHandle<JSMap> map(thread, CreateMap(thread));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("3"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(12345));
    JSMap::Set(thread, map, key1, value1);
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("key1"));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(34567));
    JSMap::Set(thread, map, key2, value2);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(map));
    EXPECT_TRUE(success) << "Serialize JSMap fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSMapTest, jsDeserializerTest, data, map);
    t1.join();
    delete serializer;
};

JSArrayBuffer *CreateJSArrayBuffer(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetArrayBufferFunction();
    JSHandle<JSArrayBuffer> jsArrayBuffer =
        JSHandle<JSArrayBuffer>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    return *jsArrayBuffer;
}

HWTEST_F_L0(JSSerializerTest, SerializeJSArrayBuffer)
{
    JSHandle<JSArrayBuffer> jsArrayBuffer(thread, CreateJSArrayBuffer(thread));
    int32_t byteLength = 10;
    thread->GetEcmaVM()->GetFactory()->NewJSArrayBufferData(jsArrayBuffer, byteLength);
    jsArrayBuffer->SetArrayBufferByteLength(byteLength);
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(jsArrayBuffer);
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(7));
    BuiltinsArrayBuffer::SetValueInBuffer(thread, obj.GetTaggedValue(), 1, DataViewType::UINT8,
                                          number, true);
    number = JSHandle<JSTaggedValue>(thread, JSTaggedValue(17));
    BuiltinsArrayBuffer::SetValueInBuffer(thread, obj.GetTaggedValue(), 3, DataViewType::UINT8, // 3:index
                                          number, true);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsArrayBuffer));
    EXPECT_TRUE(success) << "Serialize JSArrayBuffer fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSArrayBufferTest, jsDeserializerTest, data, jsArrayBuffer, 10, nullptr);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSArrayBufferShared)
{
    std::string msg = "hello world";
    uint32_t msgBufferLen = msg.length() + 1U;
    char* msgBuffer = new char[msgBufferLen] { 0 };
    if (memcpy_s(msgBuffer, msgBufferLen, msg.c_str(), msgBufferLen) != EOK) {
        delete[] msgBuffer;
        EXPECT_TRUE(false) << " memcpy error";
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> jsArrayBuffer = factory->NewJSArrayBuffer(msgBuffer, msgBufferLen, nullptr, nullptr);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsArrayBuffer));
    EXPECT_TRUE(success) << "Serialize JSArrayBuffer fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSArrayBufferTest, jsDeserializerTest, data, jsArrayBuffer, 12, nullptr);
    t1.join();
    delete serializer;
    delete[] msgBuffer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSArrayBufferShared2)
{
    std::string msg = "hello world";
    int msgBufferLen = static_cast<int>(msg.length()) + 1;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> jsArrayBuffer = factory->NewJSSharedArrayBuffer(msgBufferLen);
    JSHandle<JSTaggedValue> BufferData(thread, jsArrayBuffer->GetArrayBufferData());
    JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(BufferData);
    void *Buffer = resNp->GetExternalPointer();
    if (memcpy_s(Buffer, msgBufferLen, msg.c_str(), msgBufferLen) != EOK) {
        EXPECT_TRUE(false) << " memcpy error";
    }
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsArrayBuffer));
    EXPECT_TRUE(success) << "Serialize JSArrayBuffer fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::string changeStr = "world hello";
    std::thread t1(&JSDeserializerTest::JSSharedArrayBufferTest,
                   jsDeserializerTest, data, jsArrayBuffer, 12, changeStr.c_str());
    t1.join();
    EXPECT_TRUE(strcmp((char *)Buffer, "world hello") == 0) << "Serialize JSArrayBuffer fail";
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSArrayBufferShared3)
{
    std::string msg = "hello world";
    int msgBufferLen = static_cast<int>(msg.length()) + 1;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> jsArrayBuffer = factory->NewJSSharedArrayBuffer(msgBufferLen);
    JSHandle<JSTaggedValue> BufferData(thread, jsArrayBuffer->GetArrayBufferData());
    JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(BufferData);
    void *Buffer = resNp->GetExternalPointer();
    if (memcpy_s(Buffer, msgBufferLen, msg.c_str(), msgBufferLen) != EOK) {
        EXPECT_TRUE(false) << " memcpy error";
    }
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsArrayBuffer));
    EXPECT_TRUE(success) << "Serialize JSArrayBuffer fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::string changeStr = "aaaaaaaaaa";
    std::thread t1(&JSDeserializerTest::JSSharedArrayBufferTest1, jsDeserializerTest, data, 5, changeStr.c_str());
    t1.join();
    EXPECT_TRUE(strcmp((char *)Buffer, "aaaaa world") == 0) << "Serialize JSArrayBuffer fail";
    JSSerializer *serializer2 = new JSSerializer(thread);
    bool success2 = serializer2->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsArrayBuffer));
    EXPECT_TRUE(success2) << "Serialize JSArrayBuffer fail";
    std::pair<uint8_t *, size_t> data2 = serializer2->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest2;
    std::string changeStr2 = "bbbbbbbbbbb";
    std::thread t2(&JSDeserializerTest::JSSharedArrayBufferTest1, jsDeserializerTest2, data2, 6, changeStr2.c_str());
    t2.join();
    EXPECT_TRUE(strcmp((char *)Buffer, "bbbbbbworld") == 0) << "Serialize JSArrayBuffer fail";
    delete serializer;
    delete serializer2;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSRegExp)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetRegExpFunction();
    JSHandle<JSRegExp> jsRegexp =
        JSHandle<JSRegExp>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromASCII("key2");
    JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromASCII("i");
    char buffer[] = "1234567";  // use char to simulate bytecode
    uint32_t bufferSize = 7;
    factory->NewJSRegExpByteCodeData(jsRegexp, static_cast<void *>(buffer), bufferSize);
    jsRegexp->SetOriginalSource(thread, JSHandle<JSTaggedValue>(pattern));
    jsRegexp->SetOriginalFlags(thread, JSHandle<JSTaggedValue>(flags));

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsRegexp));
    EXPECT_TRUE(success) << "Serialize JSRegExp fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSRegexpTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

JSArrayBuffer *CreateTestJSArrayBuffer(JSThread *thread)
{
    JSHandle<JSArrayBuffer> jsArrayBuffer(thread, CreateJSArrayBuffer(thread));
    int32_t byteLength = 10;
    thread->GetEcmaVM()->GetFactory()->NewJSArrayBufferData(jsArrayBuffer, byteLength);
    jsArrayBuffer->SetArrayBufferByteLength(byteLength);
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(jsArrayBuffer);
    JSMutableHandle<JSTaggedValue> number(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < 10; i++) { // 10: arrayLength
        number.Update(JSTaggedValue(i));
        BuiltinsArrayBuffer::SetValueInBuffer(thread, obj.GetTaggedValue(), i, DataViewType::UINT8,
            number, true);
    }
    return *jsArrayBuffer;
}

HWTEST_F_L0(JSSerializerTest, SerializeJSTypedArray1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetInt8ArrayFunction();
    JSHandle<JSTypedArray> int8Array =
        JSHandle<JSTypedArray>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    JSHandle<JSTaggedValue> viewedArrayBuffer(thread, CreateTestJSArrayBuffer(thread));
    int8Array->SetViewedArrayBufferOrByteArray(thread, viewedArrayBuffer);
    int byteLength = 10;
    int byteOffset = 0;
    int arrayLength = (byteLength - byteOffset) / (sizeof(int8_t));
    int8Array->SetByteLength(byteLength);
    int8Array->SetByteOffset(byteOffset);
    int8Array->SetTypedArrayName(thread, thread->GlobalConstants()->GetInt8ArrayString());
    int8Array->SetArrayLength(arrayLength);
    int8Array->SetContentType(ContentType::Number);
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(int8Array));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TypedArrayTest1, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSTypedArray2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetInt8ArrayFunction();
    JSHandle<JSTypedArray> int8Array =
        JSHandle<JSTypedArray>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    uint8_t value = 255; // 255 : test case
    JSTaggedType val = JSTaggedValue(value).GetRawData();
    int byteArrayLength = 10; // 10: arrayLength
    JSHandle<ByteArray> byteArray = factory->NewByteArray(byteArrayLength, sizeof(value));
    for (int i = 0; i < byteArrayLength; i++) {
        byteArray->Set(thread, i, DataViewType::UINT8, val);
    }
    int8Array->SetViewedArrayBufferOrByteArray(thread, byteArray);
    int byteLength = 10;
    int byteOffset = 0;
    int arrayLength = (byteLength - byteOffset) / (sizeof(int8_t));
    int8Array->SetByteLength(byteLength);
    int8Array->SetByteOffset(byteOffset);
    int8Array->SetTypedArrayName(thread, thread->GlobalConstants()->GetInt8ArrayString());
    int8Array->SetArrayLength(arrayLength);
    int8Array->SetContentType(ContentType::Number);
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(int8Array));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TypedArrayTest2, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeTaggedArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> taggedArray(factory->NewTaggedArray(4));
    JSHandle<EcmaString> str1 = factory->NewFromASCII("str11");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("str22");
    // set value to the taggedarray
    taggedArray->Set(thread, 0, str1.GetTaggedValue());
    taggedArray->Set(thread, 1, str2.GetTaggedValue());
    taggedArray->Set(thread, 2, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(taggedArray));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TaggedArrayTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

std::shared_ptr<JSPandaFile> CreateJSPandaFile(const char *source, const CString filename, uint32_t *methodOffset)
{
    pandasm::Parser parser;
    const std::string fn = "SRC.pa"; // test file name : "SRC.pa"
    auto res = parser.Parse(source, fn);

    std::unique_ptr<const panda_file::File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), filename);
    const panda_file::File *file = pf->GetPandaFile();
    const uint8_t *typeDesc = utf::CStringAsMutf8("L_GLOBAL;");
    panda_file::File::EntityId classId = file->GetClassId(typeDesc);
    EXPECT_TRUE(classId.IsValid());

    panda_file::ClassDataAccessor cda(*file, classId);
    cda.EnumerateMethods([&](panda_file::MethodDataAccessor &mda) {
        *methodOffset = mda.GetMethodId().GetOffset();
    });
    return pf;
}

static void TestFunc()
{
    return;
}

HWTEST_F_L0(JSSerializerTest, SerializeMethod1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<Method> method = factory->NewMethodForNativeFunction(reinterpret_cast<void*>(TestFunc));
    EXPECT_TRUE(method.GetTaggedValue().IsMethod());

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(method));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::MethodTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeMethod2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const char *source = R"(
        .function void foo() {
            lda.str "helloworld"
            returnundefined
        }
    )";
    const CString fileName = "test.pa";
    uint32_t methodOffset = 0;
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName, &methodOffset);
    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(methodOffset));
    JSHandle<Method> method = factory->NewMethod(methodLiteral);
    EXPECT_TRUE(method.GetTaggedValue().IsMethod());
    JSPandaFileManager::GetInstance()->AddJSPandaFileVm(thread->GetEcmaVM(), pf);
    EXPECT_TRUE(pf != nullptr);

    JSHandle<ConstantPool> constPool = factory->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    EXPECT_TRUE(constPool.GetTaggedValue().IsConstantPool());
    method->SetConstantPool(thread, constPool.GetTaggedValue());

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(method));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::MethodTest1, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeAOTMethod)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const char *source = R"(
        .function void foo() {
            lda.str "hello aot"
            returnundefined
        }
    )";
    const CString fileName = "test_aot_method.pa";
    uint32_t methodOffset = 0;
    std::shared_ptr<JSPandaFile> pf = CreateJSPandaFile(source, fileName, &methodOffset);
    MethodLiteral *methodLiteral = new MethodLiteral(EntityId(methodOffset));
    methodLiteral->SetAotCodeBit(true);
    methodLiteral->SetIsFastCall(true);
    pf->SetMethodLiteralToMap(methodLiteral);

    JSHandle<Method> method = factory->NewMethod(methodLiteral);
    EXPECT_TRUE(method.GetTaggedValue().IsMethod());
    JSPandaFileManager::GetInstance()->AddJSPandaFileVm(thread->GetEcmaVM(), pf);
    EXPECT_TRUE(pf != nullptr);

    JSHandle<ConstantPool> constPool = factory->NewConstantPool(4);
    constPool->SetJSPandaFile(pf.get());
    EXPECT_TRUE(constPool.GetTaggedValue().IsConstantPool());
    method->SetConstantPool(thread, constPool.GetTaggedValue());

    uintptr_t codeEntry = 0x1234;
    method->SetCodeEntryAndMarkAOT(codeEntry);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(method));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::AOTMethodTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

// support concurrent function
HWTEST_F_L0(JSSerializerTest, SerializeConcurrentFunction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> concurrentFunction = factory->NewJSFunction(env, nullptr, FunctionKind::CONCURRENT_FUNCTION);
    EXPECT_TRUE(concurrentFunction->IsJSFunction());
    EXPECT_TRUE(concurrentFunction->GetFunctionKind() == ecmascript::FunctionKind::CONCURRENT_FUNCTION);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(concurrentFunction));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::ConcurrentFunctionTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeBigInt)
{
    CString str1 = "100000000000000000000000000000000000000000000000000000"; // Binary: 2 ^ 53
    CString str2 = "111111111111111111111111111111111111111111111111111111"; // Binary: 2 ^ 54 - 1
    CString str3 = "-9007199254740991012345";
    CString str4 = "1234567890987654321"; // Decimal
    JSHandle<BigInt> bigInt1 = BigIntHelper::SetBigInt(thread, str1);
    JSHandle<BigInt> bigInt2 = BigIntHelper::SetBigInt(thread, str2);
    JSHandle<BigInt> bigInt3 = BigIntHelper::SetBigInt(thread, str3);
    JSHandle<BigInt> bigInt4 = BigInt::Uint32ToBigInt(thread, 0); // 0 : test case
    JSHandle<BigInt> bigInt5 = BigIntHelper::SetBigInt(thread, str4, BigInt::DECIMAL);
    JSHandle<BigInt> bigInt6 = BigInt::Uint64ToBigInt(thread, ULLONG_MAX);
    JSHandle<BigInt> bigInt7 = BigInt::Uint64ToBigInt(thread, UINT_MAX);
    JSHandle<BigInt> bigInt8 = BigInt::Uint32ToBigInt(thread, std::numeric_limits<uint32_t>::max());
    JSHandle<BigInt> bigInt9 = BigInt::Uint32ToBigInt(thread, std::numeric_limits<uint32_t>::min());
    JSHandle<BigInt> bigInt10 = BigInt::Int64ToBigInt(thread, LLONG_MAX);
    JSHandle<BigInt> bigInt11 = BigInt::Int64ToBigInt(thread, LLONG_MIN);
    JSHandle<BigInt> bigInt12 = BigInt::Int64ToBigInt(thread, INT_MAX);
    JSHandle<BigInt> bigInt13 = BigInt::Int64ToBigInt(thread, INT_MIN);
    JSHandle<BigInt> bigInt14 = BigInt::Int32ToBigInt(thread, -1); // -1 : test case

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt1));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt2));
    bool success3 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt3));
    bool success4 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt4));
    bool success5 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt5));
    bool success6 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt6));
    bool success7 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt7));
    bool success8 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt8));
    bool success9 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt9));
    bool success10 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt10));
    bool success11 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt11));
    bool success12 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt12));
    bool success13 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt13));
    bool success14 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(bigInt14));
    EXPECT_TRUE(success1);
    EXPECT_TRUE(success2);
    EXPECT_TRUE(success3);
    EXPECT_TRUE(success4);
    EXPECT_TRUE(success5);
    EXPECT_TRUE(success6);
    EXPECT_TRUE(success7);
    EXPECT_TRUE(success8);
    EXPECT_TRUE(success9);
    EXPECT_TRUE(success10);
    EXPECT_TRUE(success11);
    EXPECT_TRUE(success12);
    EXPECT_TRUE(success13);
    EXPECT_TRUE(success14);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::BigIntTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
}

HWTEST_F_L0(JSSerializerTest, SerializeObjectWithConcurrentFunction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> concurrentFunction1 = factory->NewJSFunction(env, nullptr, FunctionKind::CONCURRENT_FUNCTION);
    EXPECT_TRUE(concurrentFunction1->IsJSFunction());
    EXPECT_TRUE(concurrentFunction1->GetFunctionKind() == ecmascript::FunctionKind::CONCURRENT_FUNCTION);
    JSHandle<JSFunction> concurrentFunction2 = factory->NewJSFunction(env, nullptr, FunctionKind::CONCURRENT_FUNCTION);
    EXPECT_TRUE(concurrentFunction2->IsJSFunction());
    EXPECT_TRUE(concurrentFunction2->GetFunctionKind() == ecmascript::FunctionKind::CONCURRENT_FUNCTION);
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("1"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("2"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("abc"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("4"));
    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("key"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(12345));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("def"));
    JSHandle<JSTaggedValue> value3(factory->NewFromASCII("value"));
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key2, JSHandle<JSTaggedValue>(concurrentFunction1));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key3, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key4, JSHandle<JSTaggedValue>(concurrentFunction2));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key5, value3);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(obj));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::ObjectWithConcurrentFunctionTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

// not support function
HWTEST_F_L0(JSSerializerTest, SerializeFunction)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = env->GetFunctionFunction();
    EXPECT_TRUE(function->IsJSFunction());

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(function));
    EXPECT_FALSE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializer deserializer(thread, data.first, data.second);
    JSHandle<JSTaggedValue> ret = deserializer.Deserialize();
    EXPECT_TRUE(ret.IsEmpty());
    delete serializer;
}

// not support symbol
HWTEST_F_L0(JSSerializerTest, SerializeSymbolWithProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSymbol> jsSymbol = factory->NewJSSymbol();
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("2"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(8));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsSymbol), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsSymbol), key2, value2);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(jsSymbol));
    EXPECT_FALSE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializer deserializer(thread, data.first, data.second);
    JSHandle<JSTaggedValue> ret = deserializer.Deserialize();
    EXPECT_TRUE(ret.IsEmpty());
    delete serializer;
};

// Test transfer JSArrayBuffer
HWTEST_F_L0(JSSerializerTest, TransferJSArrayBuffer1)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    // create a JSArrayBuffer
    size_t length = 5;
    uint8_t value = 100;
    void *buffer = ecmaVm->GetNativeAreaAllocator()->AllocateBuffer(length);
    if (memset_s(buffer, length, value, length) != EOK) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    JSHandle<JSArrayBuffer> arrBuf = factory->NewJSArrayBuffer(buffer,
        length, NativeAreaAllocator::FreeBufferFunc, ecmaVm->GetNativeAreaAllocator());
    JSHandle<JSTaggedValue> arrBufTag = JSHandle<JSTaggedValue>::Cast(arrBuf);

    CUnorderedSet<uintptr_t> transferDataSet;
    uintptr_t addr = static_cast<uintptr_t>(arrBufTag.GetTaggedType());
    transferDataSet.insert(addr);
    JSSerializer *serializer = new JSSerializer(thread);
    serializer->InitTransferSet(transferDataSet);
    bool success = serializer->SerializeJSTaggedValue(arrBufTag);
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest1,
                   jsDeserializerTest,
                   data,
                   reinterpret_cast<uintptr_t>(buffer));
    t1.join();
    delete serializer;
    // test if detached
    EXPECT_TRUE(arrBuf->IsDetach());
};

// Test serialize JSArrayBuffer that not transfer
HWTEST_F_L0(JSSerializerTest, TransferJSArrayBuffer2)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    // create a JSArrayBuffer
    size_t length = 5;
    uint8_t value = 100;
    void *buffer = ecmaVm->GetNativeAreaAllocator()->AllocateBuffer(length);
    if (memset_s(buffer, length, value, length) != EOK) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    JSHandle<JSArrayBuffer> arrBuf = factory->NewJSArrayBuffer(buffer,
        length, NativeAreaAllocator::FreeBufferFunc, ecmaVm->GetNativeAreaAllocator());
    JSHandle<JSTaggedValue> arrBufTag = JSHandle<JSTaggedValue>::Cast(arrBuf);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(arrBufTag);
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest2, jsDeserializerTest, data,
        reinterpret_cast<uintptr_t>(buffer));
    t1.join();
    delete serializer;
    // test if detached
    EXPECT_FALSE(arrBuf->IsDetach());
};

// Test serialize an empty JSArrayBuffer
HWTEST_F_L0(JSSerializerTest, TransferJSArrayBuffer3)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    // create a JSArrayBuffer
    JSHandle<JSArrayBuffer> arrBuf = factory->NewJSArrayBuffer(0);
    JSHandle<JSTaggedValue> arrBufTag = JSHandle<JSTaggedValue>::Cast(arrBuf);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(arrBufTag);
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest3, jsDeserializerTest, data);
    t1.join();
    delete serializer;
    // test if detached
    EXPECT_FALSE(arrBuf->IsDetach());
};

// Test default transfer JSArrayBuffer
HWTEST_F_L0(JSSerializerTest, TransferJSArrayBuffer4)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    // create a JSArrayBuffer
    size_t length = 5;
    uint8_t value = 100;
    void *buffer = ecmaVm->GetNativeAreaAllocator()->AllocateBuffer(length);
    if (memset_s(buffer, length, value, length) != EOK) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    JSHandle<JSArrayBuffer> arrBuf = factory->NewJSArrayBuffer(buffer,
        length, NativeAreaAllocator::FreeBufferFunc, ecmaVm->GetNativeAreaAllocator());
    JSHandle<JSTaggedValue> arrBufTag = JSHandle<JSTaggedValue>::Cast(arrBuf);

    JSSerializer *serializer = new JSSerializer(thread);
    serializer->SetDefaultTransfer();
    bool success = serializer->SerializeJSTaggedValue(arrBufTag);
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest4, jsDeserializerTest, data,
        reinterpret_cast<uintptr_t>(buffer));
    t1.join();
    delete serializer;
    // test if detached
    EXPECT_TRUE(arrBuf->IsDetach());
}

// Test JSArrayBuffer transfer mixed with other properties in a typedArray
HWTEST_F_L0(JSSerializerTest, TransferJSArrayBufferInTypedArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetInt8ArrayFunction();
    JSHandle<JSTypedArray> int8Array =
        JSHandle<JSTypedArray>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    JSHandle<JSArrayBuffer> arrBuffer(thread, CreateTestJSArrayBuffer(thread));
    JSHandle<JSNativePointer> np =
        JSHandle<JSNativePointer>::Cast(JSHandle<JSTaggedValue>(thread, arrBuffer->GetArrayBufferData()));
    // c buffer ptr
    uintptr_t buffer = reinterpret_cast<uintptr_t>(np->GetExternalPointer());
    int8Array->SetViewedArrayBufferOrByteArray(thread, JSHandle<JSTaggedValue>(arrBuffer));
    int byteLength = 10;
    int byteOffset = 0;
    int arrayLength = (byteLength - byteOffset) / (sizeof(int8_t));
    int8Array->SetByteLength(byteLength);
    int8Array->SetByteOffset(byteOffset);
    int8Array->SetTypedArrayName(thread, thread->GlobalConstants()->GetInt8ArrayString());
    int8Array->SetArrayLength(arrayLength);
    int8Array->SetContentType(ContentType::Number);
    CUnorderedSet<uintptr_t> transferDataSet;
    uintptr_t addr = static_cast<uintptr_t>(arrBuffer.GetTaggedType());
    transferDataSet.insert(addr);
    JSSerializer *serializer = new JSSerializer(thread);
    serializer->InitTransferSet(transferDataSet);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(int8Array));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest5, jsDeserializerTest, data, buffer);
    t1.join();
    delete serializer;
    // test if detached
    EXPECT_TRUE(arrBuffer->IsDetach());
};
}  // namespace panda::test
