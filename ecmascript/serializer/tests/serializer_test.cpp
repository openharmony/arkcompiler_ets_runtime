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

#include "libpandabase/utils/utf.h"
#include "libpandafile/class_data_accessor-inl.h"

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

#include "ecmascript/serializer/value_serializer.h"
#include "ecmascript/serializer/base_deserializer.h"
#include "ecmascript/serializer/module_deserializer.h"
#include "ecmascript/serializer/module_serializer.h"

using namespace panda::ecmascript;
using namespace testing::ext;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using DeserializeFunc = void (*)(SerializeData* data);
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<uint64_t, std::nano>;

constexpr int32_t INITIALIZE_SIZE = 100;

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
        thread->ManagedCodeBegin();
    }
    void Destroy()
    {
        thread->ManagedCodeEnd();
        delete scope;
        scope = nullptr;
        ecmaVm->SetEnableForceGC(false);
        thread->ClearException();
        JSNApi::DestroyJSVM(ecmaVm);
    }

    void JSSpecialValueTest(SerializeData* data)
    {
        Init();
        JSHandle<JSTaggedValue> jsTrue(thread, JSTaggedValue::True());
        JSHandle<JSTaggedValue> jsFalse(thread, JSTaggedValue::False());
        JSHandle<JSTaggedValue> jsUndefined(thread, JSTaggedValue::Undefined());
        JSHandle<JSTaggedValue> jsNull(thread, JSTaggedValue::Null());
        JSHandle<JSTaggedValue> jsHole(thread, JSTaggedValue::Hole());

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> retTrue = deserializer.ReadValue();
        EXPECT_TRUE(JSTaggedValue::SameValue(jsTrue, retTrue)) << "Not same value for JS_TRUE";
        JSHandle<JSTaggedValue> retFalse = deserializer.ReadValue();
        EXPECT_TRUE(JSTaggedValue::SameValue(jsFalse, retFalse)) << "Not same value for JS_FALSE";
        JSHandle<JSTaggedValue> retUndefined = deserializer.ReadValue();
        JSHandle<JSTaggedValue> retNull = deserializer.ReadValue();
        JSHandle<JSTaggedValue> retHole = deserializer.ReadValue();

        EXPECT_TRUE(JSTaggedValue::SameValue(jsUndefined, retUndefined)) << "Not same value for JS_UNDEFINED";
        EXPECT_TRUE(JSTaggedValue::SameValue(jsNull, retNull)) << "Not same value for JS_NULL";
        EXPECT_TRUE(JSTaggedValue::SameValue(jsHole, retHole)) << "Not same value for JS_HOLE";
        Destroy();
    }

    void LineStringTest(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_FALSE(res.IsEmpty());
        EXPECT_TRUE(res->IsLineString());

        Destroy();
    }

    void TreeStringTest(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_FALSE(res.IsEmpty());
        EXPECT_TRUE(res->IsLineString());

        Destroy();
    }

    void SlicedStringTest(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_FALSE(res.IsEmpty());
        EXPECT_TRUE(res->IsSlicedString());

        Destroy();
    }

    void JSPlainObjectTest1(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);
        EXPECT_FALSE(retObj.IsEmpty());

        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, retObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 4U); // 4 : test case
        double sum = 0.0;
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj), key).GetValue()->GetNumber();
            sum += a;
        }
        EXPECT_EQ(sum, 10); // 10 : test case

        Destroy();
    }

    void JSPlainObjectTest2(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);
        EXPECT_FALSE(retObj.IsEmpty());

        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, retObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 10U);
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            JSHandle<JSTaggedValue> value =
                JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj), key).GetValue();
            EXPECT_TRUE(value->GetTaggedObject()->GetClass()->IsJSObject());
        }

        Destroy();
    }

    void JSPlainObjectTest3(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);
        EXPECT_FALSE(retObj.IsEmpty());
        EXPECT_TRUE(retObj->GetClass()->IsDictionaryMode());

        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, retObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 1030U);
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            JSHandle<JSTaggedValue> value =
                JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj), key).GetValue();
            EXPECT_TRUE(value->IsInt());
        }

        Destroy();
    }

    void JSPlainObjectTest4(SerializeData* data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("str1"));

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);
        EXPECT_FALSE(retObj.IsEmpty());

        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj), key).GetValue();
        EXPECT_TRUE(value->IsTaggedArray());
        TaggedArray *array = reinterpret_cast<TaggedArray *>(value->GetTaggedObject());
        size_t length = array->GetLength();
        EXPECT_EQ(length, 102400U); // 102400: array length
        for (uint32_t i = 0; i < length; i++) {
            EXPECT_TRUE(array->Get(i).IsHole());
        }

        Destroy();
    }

    void PrimitiveTest(SerializeData* data)
    {
        Init();

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_FALSE(objValue.IsEmpty());
        EXPECT_TRUE(objValue->IsJSObject());

        Destroy();
    }

    void JSErrorTest1(SerializeData* data)
    {
        Init();

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_FALSE(objValue.IsEmpty());
        EXPECT_TRUE(objValue->IsJSError());

        Destroy();
    }

    void JSErrorTest2(SerializeData* data)
    {
        Init();

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);
        EXPECT_FALSE(retObj.IsEmpty());

        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, retObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 2U);
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            JSHandle<JSTaggedValue> value =
                JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj), key).GetValue();
            EXPECT_TRUE(value->IsJSError());
        }

        Destroy();
    }

    void JSErrorTest3(SerializeData *data)
    {
        Init();

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);
        EXPECT_FALSE(retObj.IsEmpty());

        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, retObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 7U); // 7 : test case
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            JSHandle<JSTaggedValue> value =
                JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj), key).GetValue();
            EXPECT_TRUE(value->IsJSError());
        }

        Destroy();
    }

    void BigIntTest(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);
        EXPECT_FALSE(retObj.IsEmpty());

        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, retObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 2U);
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            JSHandle<JSTaggedValue> value =
                JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj), key).GetValue();
            EXPECT_TRUE(value->GetTaggedObject()->GetClass()->IsBigInt());
        }

        Destroy();
    }

    void NativeBindingObjectTest1(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);
        EXPECT_TRUE(objValue->IsUndefined());
        Destroy();
    }

    void NativeBindingObjectTest2(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> objValue = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);
        EXPECT_TRUE(objValue->IsJSObject());

        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);
        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, retObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 2U);
        JSHandle<JSTaggedValue> key(thread, array->Get(0));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj), key).GetValue();
        EXPECT_TRUE(value->IsUndefined());

        Destroy();
    }

    void JSSetTest(SerializeData* data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(7)); // 7 : test case
        JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(9)); // 9 : test case
        JSHandle<JSTaggedValue> value3(factory->NewFromASCII("x"));
        JSHandle<JSTaggedValue> value4(factory->NewFromASCII("y"));

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> setValue = deserializer.ReadValue();
        EXPECT_TRUE(!setValue.IsEmpty());
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

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
        EXPECT_TRUE(retSet->Has(thread, value1.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(thread, value2.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(thread, value3.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(thread, value4.GetTaggedValue()));
        Destroy();
    }

    void JSArrayTest(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> arrayValue = deserializer.ReadValue();
        EXPECT_TRUE(!arrayValue.IsEmpty());
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

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

    void EcmaStringTest1(SerializeData* data)
    {
        Init();
        const char *rawStr = "ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "ssssss";
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromASCII(rawStr);

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ecmaString fail";
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        auto ecmaStringCode = EcmaStringAccessor(ecmaString).GetHashcode();
        auto resEcmaStringCode = EcmaStringAccessor(resEcmaString).GetHashcode();
        EXPECT_TRUE(ecmaStringCode == resEcmaStringCode) << "Not same HashCode";
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void EcmaStringTest2(SerializeData* data)
    {
        Init();
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromStdString("你好，世界");
        JSHandle<EcmaString> ecmaString1 = thread->GetEcmaVM()->GetFactory()->NewFromStdString("你好，世界");
        auto ecmaStringCode1 = EcmaStringAccessor(ecmaString).GetHashcode();
        auto ecmaString1Code = EcmaStringAccessor(ecmaString1).GetHashcode();
        EXPECT_TRUE(ecmaStringCode1 == ecmaString1Code) << "Not same HashCode";
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*ecmaString, *ecmaString1)) << "Not same EcmaString";

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ecmaString fail";
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        auto ecmaStringCode2 = EcmaStringAccessor(ecmaString).GetHashcode();
        auto resEcmaStringCode = EcmaStringAccessor(resEcmaString).GetHashcode();
        EXPECT_TRUE(ecmaStringCode2 == resEcmaStringCode) << "Not same HashCode";
        EXPECT_TRUE(EcmaStringAccessor::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void Int32Test(SerializeData* data)
    {
        Init();
        int32_t a = 64;
        int32_t min = -2147483648;
        int32_t b = -63;
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> resA = deserializer.ReadValue();
        JSHandle<JSTaggedValue> resMin = deserializer.ReadValue();
        JSHandle<JSTaggedValue> resB = deserializer.ReadValue();
        EXPECT_TRUE(!resA.IsEmpty() && !resMin.IsEmpty() && !resB.IsEmpty()) << "[Empty] Deserialize Int32 fail";
        EXPECT_TRUE(resA->IsInt() && resMin->IsInt() && resB->IsInt()) << "[NotInt] Deserialize Int32 fail";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resA) == a) << "Not Same Value";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resMin) == min) << "Not Same Value";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resB) == b) << "Not Same Value";
        Destroy();
    }

    void DoubleTest(SerializeData* data)
    {
        Init();
        double a = 3.1415926535;
        double b = -3.1415926535;
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> resA = deserializer.ReadValue();
        JSHandle<JSTaggedValue> resB = deserializer.ReadValue();
        EXPECT_TRUE(!resA.IsEmpty() && !resB.IsEmpty()) << "[Empty] Deserialize double fail";
        EXPECT_TRUE(resA->IsDouble() && resB->IsDouble()) << "[NotInt] Deserialize double fail";
        EXPECT_TRUE(resA->GetDouble() == a) << "Not Same Value";
        EXPECT_TRUE(resB->GetDouble() == b) << "Not Same Value";
        Destroy();
    }

    void JSDateTest(SerializeData* data)
    {
        Init();
        double tm = 28 * 60 * 60 * 1000;  // 28 * 60 * 60 * 1000 : test case
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSDate fail";
        EXPECT_TRUE(res->IsDate()) << "[NotJSDate] Deserialize JSDate fail";
        JSHandle<JSDate> resDate = JSHandle<JSDate>(res);
        EXPECT_TRUE(resDate->GetTimeValue() == JSTaggedValue(tm)) << "Not Same Time Value";
        Destroy();
    }

    void JSMapTest(SerializeData* data, const JSHandle<JSMap> &originMap)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
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

    void JSSharedArrayBufferTest(SerializeData *data, int32_t byteLength, const char *msg)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSArrayBuffer fail";
        EXPECT_TRUE(res->IsSharedArrayBuffer()) << "[NotJSArrayBuffer] Deserialize JSArrayBuffer fail";
        JSHandle<JSArrayBuffer> resJSArrayBuffer = JSHandle<JSArrayBuffer>::Cast(res);
        int32_t resByteLength = static_cast<int32_t>(resJSArrayBuffer->GetArrayBufferByteLength());
        EXPECT_TRUE(resByteLength == byteLength) << "Not Same ByteLength";
        JSHandle<JSTaggedValue> resBufferData(thread, resJSArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        ASSERT_NE(resBuffer, nullptr);

        if (msg != nullptr) {
            if (memcpy_s(resBuffer, byteLength, msg, byteLength) != EOK) {
                EXPECT_TRUE(false) << " memcpy error!";
            }
        }
        Destroy();
    }

    void SerializeMultiSharedRegionTest(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty());
        EXPECT_TRUE(res->IsJSObject());
        JSTaggedValue elements = JSHandle<JSObject>(res)->GetElements();
        EXPECT_TRUE(elements.IsTaggedArray());
        EXPECT_EQ(JSHandle<TaggedArray>(thread, elements)->GetLength(), 10 * 1024); // 10 * 1024: array length
        JSTaggedValue value = JSHandle<TaggedArray>(thread, elements)->Get(0);
        EXPECT_TRUE(value.IsTaggedArray());
        uint32_t length = JSHandle<TaggedArray>(thread, value)->GetLength();
        EXPECT_EQ(length, 11 * 1024); // 11 * 1024: array length
        Destroy();
    }

    void SerializeMultiSharedRegionTest1(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty());
        EXPECT_TRUE(res->IsJSObject());
        JSTaggedValue elements = JSHandle<JSObject>(res)->GetElements();
        EXPECT_TRUE(elements.IsTaggedArray());
        EXPECT_EQ(JSHandle<TaggedArray>(thread, elements)->GetLength(), 3 * 1024); // 3 * 1024: array length
        for (int i = 0; i < 5; i++) { // 5: array elements
            JSTaggedValue value = JSHandle<TaggedArray>(thread, elements)->Get(i);
            EXPECT_TRUE(value.IsTaggedArray());
            uint32_t length = JSHandle<TaggedArray>(thread, value)->GetLength();
            EXPECT_EQ(length, 3 * 1024); // 3 * 1024: array length
        }
        Destroy();
    }

    void SerializeMultiSharedRegionTest2(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty());
        EXPECT_TRUE(res->IsJSObject());
        JSTaggedValue properties = JSHandle<JSObject>(res)->GetProperties();
        EXPECT_TRUE(properties.IsTaggedArray());
        EXPECT_EQ(JSHandle<TaggedArray>(thread, properties)->GetLength(), 3 * 1024); // 3 * 1024: array length
        for (int i = 0; i < 5; i++) { // 5: array elements
            JSTaggedValue value = JSHandle<TaggedArray>(thread, properties)->Get(i);
            EXPECT_TRUE(value.IsTaggedArray());
            uint32_t length = JSHandle<TaggedArray>(thread, value)->GetLength();
            EXPECT_EQ(length, 3 * 1024); // 3 * 1024: array length
        }

        JSTaggedValue elements = JSHandle<JSObject>(res)->GetElements();
        EXPECT_TRUE(elements.IsTaggedArray());
        EXPECT_EQ(JSHandle<TaggedArray>(thread, elements)->GetLength(), 3 * 1024); // 3 * 1024: array length
        for (int i = 0; i < 5; i++) { // 5: array elements
            JSTaggedValue value = JSHandle<TaggedArray>(thread, elements)->Get(i);
            EXPECT_TRUE(value.IsTaggedArray());
            uint32_t length = JSHandle<TaggedArray>(thread, value)->GetLength();
            EXPECT_EQ(length, 3 * 1024); // 3 * 1024: array length
        }
        Destroy();
    }

    void JSSharedSetBasicTest1(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSSharedSet failed";
        EXPECT_TRUE(res->IsJSSharedSet()) << "[NotJSSharedSet] Deserialize JSSharedSet failed";
        JSHandle<JSSharedSet> jsSet = JSHandle<JSSharedSet>::Cast(res);
        auto size = JSSharedSet::GetSize(thread, jsSet);
        EXPECT_TRUE(size == INITIALIZE_SIZE);
        JSSharedSet::Clear(thread, jsSet);
        Destroy();
    }

    void JSSharedSetBasicTest2(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSSharedSet failed";
        EXPECT_TRUE(res->IsJSSharedSet()) << "[NotJSSharedSet] Deserialize JSSharedSet failed";
        JSHandle<JSSharedSet> jsSet = JSHandle<JSSharedSet>::Cast(res);

        auto size = JSSharedSet::GetSize(thread, jsSet);
        EXPECT_TRUE(size == INITIALIZE_SIZE);
        for (int32_t i = 0; i < size; i++) {
            EXPECT_TRUE(JSSharedSet::Has(thread, jsSet, JSTaggedValue(i)));
        }
        JSSharedSet::Add(thread, jsSet, JSHandle<JSTaggedValue>(thread, JSTaggedValue(INITIALIZE_SIZE)));
        bool result = JSSharedSet::Delete(thread, jsSet, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)));
        EXPECT_TRUE(result) << "Delete failed";
        Destroy();
    }

    void JSSharedSetMultiThreadTest1(SerializeData *data)
    {
        EXPECT_TRUE(data != nullptr);
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSSharedSet fail";
        EXPECT_TRUE(res->IsJSSharedSet()) << "[NotJSSharedSet] Deserialize JSSharedSet fail";
        JSHandle<JSSharedSet> jsSet = JSHandle<JSSharedSet>::Cast(res);
        EXPECT_TRUE(JSSharedSet::GetSize(thread, jsSet) == INITIALIZE_SIZE);
        for (int i = 0; i < INITIALIZE_SIZE; i++) {
            EXPECT_TRUE(JSSharedSet::Has(thread, jsSet, JSTaggedValue(i)));
        }
        Destroy();
    }

    void JSSharedMapBasicTest1(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSSharedMap failed";
        EXPECT_TRUE(res->IsJSSharedMap()) << "[NotJSSharedMap] Deserialize JSSharedMap failed";
        JSHandle<JSSharedMap> jsMap = JSHandle<JSSharedMap>::Cast(res);
        auto size = JSSharedMap::GetSize(thread, jsMap);
        EXPECT_TRUE(size == INITIALIZE_SIZE);
        JSSharedMap::Clear(thread, jsMap);
        Destroy();
    }

    void JSSharedMapBasicTest2(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSSharedMap failed";
        EXPECT_TRUE(res->IsJSSharedMap()) << "[NotJSSharedMap] Deserialize JSSharedMap failed";
        JSHandle<JSSharedMap> jsMap = JSHandle<JSSharedMap>::Cast(res);

        auto size = JSSharedMap::GetSize(thread, jsMap);
        EXPECT_TRUE(size == INITIALIZE_SIZE);
        for (int32_t i = 0; i < size; i++) {
            EXPECT_TRUE(JSSharedMap::Has(thread, jsMap, JSTaggedValue(i)));
        }
        JSSharedMap::Set(thread, jsMap, JSHandle<JSTaggedValue>(thread, JSTaggedValue(INITIALIZE_SIZE)),
            JSHandle<JSTaggedValue>(thread, JSTaggedValue(INITIALIZE_SIZE)));
        bool result = JSSharedMap::Delete(thread, jsMap, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)));
        EXPECT_TRUE(result) << "Delete failed";
        Destroy();
    }

    void JSRegexpTest(SerializeData *data)
    {
        Init();
        JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromASCII("key2");
        JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromASCII("i");
        char buffer[] = "1234567";  // use char buffer to simulate byteCodeBuffer
        uint32_t bufferSize = 7;

        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
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

    void TypedArrayTest1(SerializeData *data)
    {
        Init();
        JSHandle<JSTaggedValue> originTypedArrayName(thread, thread->GlobalConstants()->GetInt8ArrayString());
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
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

    void TypedArrayTest2(SerializeData *data)
    {
        Init();
        JSHandle<JSTaggedValue> originTypedArrayName(thread, thread->GlobalConstants()->GetInt8ArrayString());
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
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

    void SharedObjectTest4(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_FALSE(res.IsEmpty());
        EXPECT_TRUE(res->IsJSSharedObject()) << "[NotJSSharedObject] Deserialize SharedObject fail";

        JSHandle<JSObject> sObj = JSHandle<JSObject>::Cast(res);
        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, sObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 512U);
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            JSHandle<JSTaggedValue> value =
                JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(sObj), key).GetValue();
            EXPECT_TRUE(value->IsInt());
        }

        Destroy();
    }

    void SerializeSharedFunctionTest(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize SharedFunction fail";
        EXPECT_TRUE(res->IsJSSharedFunction()) << "[NotJSSharedFunction] Deserialize SharedFunction fail";
        JSHandle<JSSharedFunction> sFunc = JSHandle<JSSharedFunction>::Cast(res);

        EXPECT_TRUE(sFunc->IsCallable());
        EXPECT_FALSE(sFunc->GetProtoOrHClass().IsHole());
        EXPECT_TRUE(sFunc->GetLexicalEnv().IsTaggedArray());
        EXPECT_TRUE(sFunc->GetHomeObject().IsJSSharedObject());
        JSHandle<JSSharedObject> sObj(thread, sFunc->GetHomeObject());
        Destroy();
    }

    void SerializeSharedFunctionTest1(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize SharedFunction fail";
        EXPECT_TRUE(res->IsJSSharedFunction()) << "[NotJSSharedFunction] Deserialize SharedFunction fail";
        Destroy();
    }

    void ObjectWithConcurrentFunctionTest(SerializeData* data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ObjectWithConcurrentFunction fail";

        JSHandle<JSTaggedValue> key1(factory->NewFromASCII("abc"));
        OperationResult result1 = JSObject::GetProperty(thread, res, key1);
        JSHandle<JSTaggedValue> value1 = result1.GetRawValue();
        EXPECT_TRUE(value1->IsString());

        JSHandle<JSTaggedValue> key2(factory->NewFromASCII("2"));
        OperationResult result2 = JSObject::GetProperty(thread, res, key2);
        JSHandle<JSTaggedValue> value2 = result2.GetRawValue();
        EXPECT_TRUE(value2->IsJSFunction());
        EXPECT_TRUE(JSHandle<JSFunction>::Cast(value2)->GetWorkNodePointer() == reinterpret_cast<uintptr_t>(nullptr));
        JSHandle<JSTaggedValue> key3(factory->NewFromASCII("key"));
        OperationResult result3 = JSObject::GetProperty(thread, res, key3);
        JSHandle<JSTaggedValue> value3 = result3.GetRawValue();
        EXPECT_TRUE(value3->IsJSFunction());
        EXPECT_TRUE(JSHandle<JSFunction>::Cast(value3)->GetWorkNodePointer() == reinterpret_cast<uintptr_t>(nullptr));

        Destroy();
    }

    void TransferJSArrayBufferTest1(SerializeData *data, uintptr_t bufferAddrCheck)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

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

    void TransferJSArrayBufferTest2(SerializeData *data, uintptr_t bufferAddrCheck)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);
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

    void TransferJSArrayBufferTest3(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TransferJSArrayBuffer3 fail";
        EXPECT_TRUE(res->IsArrayBuffer()) << "[NotJSArrayBuffer] Deserialize TransferJSArrayBuffer3 fail";

        JSHandle<JSArrayBuffer> arrBuf = JSHandle<JSArrayBuffer>::Cast(res);
        EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 0);
        JSHandle<JSTaggedValue> nativePtr(thread, arrBuf->GetArrayBufferData());
        EXPECT_TRUE(nativePtr->IsUndefined()) << "[NotJSNativePointer] Deserialize TransferJSArrayBuffer3 fail";
        Destroy();
    }

    void TransferJSArrayBufferTest5(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TransferJSArrayBuffer5 fail";
        EXPECT_TRUE(res->IsArrayBuffer()) << "[NotJSArrayBuffer] Deserialize TransferJSArrayBuffer5 fail";

        JSHandle<JSArrayBuffer> arrBuf = JSHandle<JSArrayBuffer>::Cast(res);
        EXPECT_EQ(arrBuf->GetArrayBufferByteLength(), 5); // 5: bufferLength
        JSHandle<JSTaggedValue> nativePtr(thread, arrBuf->GetArrayBufferData());
        EXPECT_TRUE(reinterpret_cast<JSNativePointer *>(nativePtr->GetTaggedObject())->GetDeleter());
        Destroy();
    }

    void SerializeCloneListTest1(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize CloneListTest1 fail";
        EXPECT_TRUE(res->IsJSShared());
        JSType resType = res->GetTaggedObject()->GetClass()->GetObjectType();
        EXPECT_EQ(resType, JSType::JS_SHARED_OBJECT);

        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("str2str1"));
        JSHandle<JSTaggedValue> shareObj =
            JSObject::GetProperty(thread, JSHandle<JSObject>(res), key).GetValue();
        EXPECT_TRUE(shareObj->IsJSShared());
        Destroy();
    }

    void SerializeCloneListTest2(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize CloneListTest2 fail";
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("shareObj"));
        JSHandle<JSTaggedValue> shareObj =
            JSObject::GetProperty(thread, JSHandle<JSObject>(res), key).GetValue();
        EXPECT_TRUE(shareObj->IsJSShared());
        Destroy();
    }

    void SerializeCloneListTest4(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize CloneListTest4 fail";
        Destroy();
    }

    void SourceTextModuleTest(SerializeData* data)
    {
        Init();
        ModuleDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        ecmaVm->CollectGarbage(TriggerGCType::YOUNG_GC);
        ecmaVm->CollectGarbage(TriggerGCType::OLD_GC);

        EXPECT_FALSE(res.IsEmpty());
        EXPECT_TRUE(res->IsSourceTextModule());

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

    void Reset()
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
        thread = nullptr;
        ecmaVm = nullptr;
        scope = nullptr;
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    JSThread *thread {nullptr};
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
};

HWTEST_F_L0(JSSerializerTest, SerializeJSSpecialValue)
{
    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->SerializeJSTaggedValue(JSTaggedValue::True());
    serializer->SerializeJSTaggedValue(JSTaggedValue::False());
    serializer->SerializeJSTaggedValue(JSTaggedValue::Undefined());
    serializer->SerializeJSTaggedValue(JSTaggedValue::Null());
    serializer->SerializeJSTaggedValue(JSTaggedValue::Hole());
    std::unique_ptr<SerializeData> data = serializer->Release();

    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSSpecialValueTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeLineString)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> str(factory->NewFromASCII("123"));

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(str),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::LineStringTest, jsDeserializerTest, data.release());
    {
        ecmascript::ThreadSuspensionScope suspensionScope(thread);
        t1.join();
    }
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeTreeString)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> str1(factory->NewFromASCII("123456789"));
    JSHandle<EcmaString> str2(factory->NewFromASCII("abcdefghi"));

    JSHandle<EcmaString> str3 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(ecmaVm, str1, str2));
    EXPECT_TRUE(str3.GetTaggedValue().IsTreeString());

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(str3),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TreeStringTest, jsDeserializerTest, data.release());
    {
        ecmascript::ThreadSuspensionScope suspensionScope(thread);
        t1.join();
    }
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeSlicedString)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> str1(factory->NewFromASCII("123456789abcedfghijk"));

    JSHandle<EcmaString> str2 =
        JSHandle<EcmaString>(thread, EcmaStringAccessor::GetSubString(ecmaVm, str1, 2, 13)); // 2: start, 3: len
    EXPECT_TRUE(str2.GetTaggedValue().IsSlicedString());

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(str2),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SlicedStringTest, jsDeserializerTest, data.release());
    {
        ecmascript::ThreadSuspensionScope suspensionScope(thread);
        t1.join();
    }
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject1)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("2"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("3"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("y"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value4(thread, JSTaggedValue(4));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key3, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key4, value4);

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSPlainObjectTest1, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject2)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> key1(factory->NewFromASCII("str1"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("str2"));
    for (int i = 0; i < 10; i++) {
        JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
        JSHandle<EcmaString> key3(factory->NewFromASCII("str3"));
        for (int j = 0; j < 10; j++) {
            key3 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(ecmaVm, key3, key1));
            JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), JSHandle<JSTaggedValue>(key3),
                                  JSHandle<JSTaggedValue>(factory->NewEmptyJSObject()));
        }
        key2 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(ecmaVm, key2, key1));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key2),
                              JSHandle<JSTaggedValue>(obj1));
    }

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSPlainObjectTest2, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

// test dictionary mode
HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject3)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> key1(factory->NewFromASCII("str1"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("str2"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    for (int i = 0; i < 1030; i++) {
        key2 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(ecmaVm, key2, key1));
        JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key2), value1);
    }

    EXPECT_TRUE(obj->GetClass()->IsDictionaryMode());

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSPlainObjectTest3, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

// test huge object serialize
HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject4)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> key1(factory->NewFromASCII("str1"));
    // new huge tagged array
    JSHandle<TaggedArray> taggedArray =
        factory->NewTaggedArray(1024 * 100, JSTaggedValue::Hole(), MemSpaceType::OLD_SPACE);

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key1),
                          JSHandle<JSTaggedValue>(taggedArray));

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSPlainObjectTest4, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject5)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("2"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("3"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("y"));
    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("func"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value4(thread, JSTaggedValue(4));
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSFunction> function = factory->NewJSFunction(env, nullptr, FunctionKind::NORMAL_FUNCTION);
    EXPECT_TRUE(function->IsJSFunction());
    JSHandle<JSTaggedValue> value5 = JSHandle<JSTaggedValue>::Cast(function);

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key3, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key4, value4);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key5, value5);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_FALSE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    BaseDeserializer deserializer(thread, data.release());
    JSHandle<JSTaggedValue> res = deserializer.ReadValue();
    EXPECT_TRUE(res.IsEmpty());
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSError1)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<EcmaString> msg(factory->NewFromASCII("this is error"));
    JSHandle<JSTaggedValue> errorTag =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::ERROR, msg, StackCheck::NO));

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, errorTag, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSErrorTest1, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSError2)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> key1(factory->NewFromASCII("error1"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("error2"));
    JSHandle<EcmaString> msg(factory->NewFromASCII("this is error"));
    JSHandle<JSTaggedValue> errorTag =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::ERROR, msg, StackCheck::NO));


    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key1), errorTag);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key2), errorTag);

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSErrorTest2, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSError3)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> key1(factory->NewFromASCII("error1"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("error2"));
    JSHandle<EcmaString> key3(factory->NewFromASCII("error3"));
    JSHandle<EcmaString> key4(factory->NewFromASCII("error4"));
    JSHandle<EcmaString> key5(factory->NewFromASCII("error5"));
    JSHandle<EcmaString> key6(factory->NewFromASCII("error6"));
    JSHandle<EcmaString> key7(factory->NewFromASCII("error7"));
    JSHandle<EcmaString> msg(factory->NewFromASCII("this is error"));
    JSHandle<JSTaggedValue> error1 =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::RANGE_ERROR, msg, StackCheck::NO));
    JSHandle<JSTaggedValue> error2 =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::REFERENCE_ERROR, msg, StackCheck::NO));
    JSHandle<JSTaggedValue> error3 =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::TYPE_ERROR, msg, StackCheck::NO));
    JSHandle<JSTaggedValue> error4 =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::URI_ERROR, msg, StackCheck::NO));
    JSHandle<JSTaggedValue> error5 =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::SYNTAX_ERROR, msg, StackCheck::NO));
    JSHandle<JSTaggedValue> error6 =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::OOM_ERROR, msg, StackCheck::NO));
    JSHandle<JSTaggedValue> error7 =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::TERMINATION_ERROR, msg, StackCheck::NO));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key1), error1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key2), error2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key3), error3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key4), error4);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key5), error5);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key6), error6);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key7), error7);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSErrorTest3, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
}

HWTEST_F_L0(JSSerializerTest, SerializeBigInt)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> key1(factory->NewFromASCII("pss"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("nativeHeap"));
    CString value1 = "365769";
    CString value2 = "139900";
    JSHandle<BigInt> bigInt1 = BigIntHelper::SetBigInt(thread, value1);
    JSHandle<BigInt> bigInt2 = BigIntHelper::SetBigInt(thread, value1);

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key1),
                          JSHandle<JSTaggedValue>(bigInt1));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key2),
                          JSHandle<JSTaggedValue>(bigInt2));

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize bigInt fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::BigIntTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializePrimitive)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> keyInt(factory->NewFromASCII("int"));
    JSHandle<EcmaString> keyDouble(factory->NewFromASCII("double"));
    JSHandle<EcmaString> keyBoolean(factory->NewFromASCII("boolean"));
    JSHandle<EcmaString> keyString(factory->NewFromASCII("string"));

    int32_t intValue = 42;
    double doubleValue = 3.14159;
    bool booleanValue = true;
    JSHandle<EcmaString> stringValue(factory->NewFromASCII("Hello World"));

    JSHandle<JSPrimitiveRef> intPrimitive = factory->NewJSPrimitiveRef(
        PrimitiveType::PRIMITIVE_NUMBER, JSHandle<JSTaggedValue>(thread, JSTaggedValue(intValue)));
    JSHandle<JSPrimitiveRef> doublePrimitive = factory->NewJSPrimitiveRef(
        PrimitiveType::PRIMITIVE_NUMBER, JSHandle<JSTaggedValue>(thread, JSTaggedValue(doubleValue)));
    JSHandle<JSPrimitiveRef> booleanPrimitive = factory->NewJSPrimitiveRef(
        PrimitiveType::PRIMITIVE_BOOLEAN, JSHandle<JSTaggedValue>(thread, JSTaggedValue(booleanValue)));
    JSHandle<JSPrimitiveRef> stringPrimitive = factory->NewJSPrimitiveRef(
        PrimitiveType::PRIMITIVE_STRING, JSHandle<JSTaggedValue>(stringValue));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(keyInt),
                        JSHandle<JSTaggedValue>(intPrimitive));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(keyDouble),
                        JSHandle<JSTaggedValue>(doublePrimitive));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(keyBoolean),
                        JSHandle<JSTaggedValue>(booleanPrimitive));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(keyString),
                        JSHandle<JSTaggedValue>(stringPrimitive));

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize primitive types failed";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::PrimitiveTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
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

HWTEST_F_L0(JSSerializerTest, SerializeNativeBindingObject1)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1 = env->GetNativeBindingSymbol();
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("x"));
    auto info = CreateNativeBindingInfo(reinterpret_cast<void*>(Attach), reinterpret_cast<void*>(Detach));
    JSHandle<JSTaggedValue> value1(factory->NewJSNativePointer(reinterpret_cast<void*>(info)));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(1));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    obj1->GetClass()->SetIsNativeBindingObject(true);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj1),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::NativeBindingObjectTest1, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
}

HWTEST_F_L0(JSSerializerTest, SerializeNativeBindingObject2)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1 = env->GetNativeBindingSymbol();
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("xx"));
    auto info = CreateNativeBindingInfo(reinterpret_cast<void*>(Attach), reinterpret_cast<void*>(Detach));
    JSHandle<JSTaggedValue> value1(factory->NewJSNativePointer(reinterpret_cast<void*>(info)));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(2));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    obj1->GetClass()->SetIsNativeBindingObject(true);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key2, JSHandle<JSTaggedValue>(obj1));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key3, value3);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj2),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::NativeBindingObjectTest2, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
}

HWTEST_F_L0(JSSerializerTest, SerializeNativeBindingObject3)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1 = env->GetNativeBindingSymbol();
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("x"));
    auto info = CreateNativeBindingInfo(reinterpret_cast<void*>(Attach), nullptr);
    JSHandle<JSTaggedValue> value1(factory->NewJSNativePointer(reinterpret_cast<void*>(info)));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(1));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    obj1->GetClass()->SetIsNativeBindingObject(true);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj1),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_FALSE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    BaseDeserializer deserializer(thread, data.release());
    JSHandle<JSTaggedValue> res = deserializer.ReadValue();
    EXPECT_TRUE(res.IsEmpty());
    delete serializer;
}

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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(set),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSSet fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSSetTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(jsDate),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSDate fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSDateTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(map),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSMap fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSMapTest, jsDeserializerTest, data.release(), map);
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(jsRegexp),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSRegExp fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSRegexpTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(array),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSArray fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSArrayTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeEcmaString1)
{
    const char *rawStr = "ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "ssssss";
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromASCII(rawStr);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(ecmaString),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest1, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

// Test EcmaString contains Chinese Text
HWTEST_F_L0(JSSerializerTest, SerializeEcmaString2)
{
    std::string rawStr = "你好，世界";
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromStdString(rawStr);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(ecmaString),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest2, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeInt32_t)
{
    int32_t a = 64, min = -2147483648, b = -63;
    JSTaggedValue aTag(a), minTag(min), bTag(b);

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->SerializeJSTaggedValue(aTag);
    serializer->SerializeJSTaggedValue(minTag);
    serializer->SerializeJSTaggedValue(bTag);
    std::unique_ptr<SerializeData> data = serializer->Release();

    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::Int32Test, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeDouble)
{
    double a = 3.1415926535, b = -3.1415926535;
    JSTaggedValue aTag(a), bTag(b);

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->SerializeJSTaggedValue(aTag);
    serializer->SerializeJSTaggedValue(bTag);
    std::unique_ptr<SerializeData> data = serializer->Release();

    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::DoubleTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
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
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key4, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key5, JSHandle<JSTaggedValue>(concurrentFunction2));

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize concurrent function fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;

    std::thread t1(&JSDeserializerTest::ObjectWithConcurrentFunctionTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

// not support most function except concurrent function
HWTEST_F_L0(JSSerializerTest, SerializeFunction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> function = factory->NewJSFunction(env, nullptr, FunctionKind::NORMAL_FUNCTION);
    EXPECT_TRUE(function->IsJSFunction());

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(function),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_FALSE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    BaseDeserializer deserializer(thread, data.release());
    JSHandle<JSTaggedValue> res = deserializer.ReadValue();
    EXPECT_TRUE(res.IsEmpty());
    delete serializer;
}

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
    JSHandle<JSTaggedValue> arrBufTag = JSHandle<JSTaggedValue>(arrBuf);

    JSHandle<JSArray> array = factory->NewJSArray();

    // set value to array
    array->SetArrayLength(thread, 1);
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, arrBufTag);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, arrBufTag, JSHandle<JSTaggedValue>(array),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize transfer JSArrayBuffer fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest1,
                   jsDeserializerTest,
                   data.release(),
                   reinterpret_cast<uintptr_t>(buffer));
    ecmascript::ThreadSuspensionScope scope(thread);
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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, arrBufTag,
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize not transfer JSArrayBuffer fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest2,
                   jsDeserializerTest,
                   data.release(),
                   reinterpret_cast<uintptr_t>(buffer));
    ecmascript::ThreadSuspensionScope scope(thread);
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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, arrBufTag,
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize empty JSArrayBuffer fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest3, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
    // test if detached
    EXPECT_FALSE(arrBuf->IsDetach());
};

// Test serialize JSArrayBuffer with external native buffer that not transfer
HWTEST_F_L0(JSSerializerTest, TransferJSArrayBuffer4)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    // create a JSArrayBuffer
    size_t length = 5;
    uint8_t value = 100;
    void *buffer = reinterpret_cast<void *>(malloc(length));
    if (memset_s(buffer, length, value, length) != EOK) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    JSHandle<JSArrayBuffer> arrBuf = factory->NewJSArrayBuffer(buffer, length, nullptr, nullptr);
    JSHandle<JSTaggedValue> arrBufTag = JSHandle<JSTaggedValue>::Cast(arrBuf);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool res = serializer->WriteValue(thread, arrBufTag,
                                      JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                      JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_FALSE(res) << "serialize JSArrayBuffer with external native shall not clone it";
    free(buffer);
};

void ArrayBufferDeleter([[maybe_unused]] void *env, void *buf, [[maybe_unused]] void *data)
{
    free(buf);
}

// Test serialize JSArrayBuffer with external native buffer that transfer
HWTEST_F_L0(JSSerializerTest, TransferJSArrayBuffer5)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    // create a JSArrayBuffer
    size_t length = 5;
    uint8_t value = 100;
    void *buffer = reinterpret_cast<void *>(malloc(length));
    if (memset_s(buffer, length, value, length) != EOK) {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    JSHandle<JSArrayBuffer> arrBuf = factory->NewJSArrayBuffer(buffer, length, ArrayBufferDeleter, nullptr);
    JSHandle<JSTaggedValue> arrBufTag = JSHandle<JSTaggedValue>::Cast(arrBuf);

    ValueSerializer *serializer = new ValueSerializer(thread, true);
    bool res = serializer->WriteValue(thread, arrBufTag,
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(res) << "serialize JSArrayBuffer with external pointer fail";
    EXPECT_TRUE(arrBuf->IsDetach());
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TransferJSArrayBufferTest5, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, TransferJSArrayBuffer6)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> transfer(factory->NewFromASCII("transfer"));
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(transfer),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_FALSE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    BaseDeserializer deserializer(thread, data.release());
    JSHandle<JSTaggedValue> res = deserializer.ReadValue();
    EXPECT_TRUE(res.IsEmpty());
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, TransferJSArrayBuffer7)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> transfer(factory->NewFromASCII("transfer"));
    JSHandle<JSArray> array = factory->NewJSArray();
    // set value to array
    array->SetArrayLength(thread, 1);
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, JSHandle<JSTaggedValue>(transfer));
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(array),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_FALSE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    BaseDeserializer deserializer(thread, data.release());
    JSHandle<JSTaggedValue> res = deserializer.ReadValue();
    EXPECT_TRUE(res.IsEmpty());
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSArrayBufferShared2)
{
    std::string msg = "hello world";
    int msgBufferLen = static_cast<int>(msg.length()) + 1;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> jsArrayBuffer = factory->NewJSSharedArrayBuffer(msgBufferLen);
    JSHandle<JSTaggedValue> BufferData(thread, jsArrayBuffer->GetArrayBufferData());
    JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(BufferData);
    void *buffer = resNp->GetExternalPointer();
    if (memcpy_s(buffer, msgBufferLen, msg.c_str(), msgBufferLen) != EOK) {
        EXPECT_TRUE(false) << " memcpy error";
    }

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(jsArrayBuffer),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSSharedArrayBuffer fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::string changeStr = "world hello";
    std::thread t1(&JSDeserializerTest::JSSharedArrayBufferTest,
                   jsDeserializerTest, data.release(), 12, changeStr.c_str());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    EXPECT_TRUE(strcmp((char *)buffer, "world hello") == 0) << "Serialize JSArrayBuffer fail";
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
    void *buffer = resNp->GetExternalPointer();
    if (memcpy_s(buffer, msgBufferLen, msg.c_str(), msgBufferLen) != EOK) {
        EXPECT_TRUE(false) << " memcpy error";
    }

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(jsArrayBuffer),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSSharedArrayBuffer fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::string changeStr = "world hello";
    std::thread t1(&JSDeserializerTest::JSSharedArrayBufferTest,
                   jsDeserializerTest, data.get(), 12, changeStr.c_str());
    {
        ecmascript::ThreadSuspensionScope scope(thread);
        t1.join();
        EXPECT_TRUE(strcmp((char *)buffer, "world hello") == 0) << "Serialize JSArrayBuffer fail";
        changeStr = "world hella";
        JSDeserializerTest jsDeserializerTest1;
        std::thread t2(&JSDeserializerTest::JSSharedArrayBufferTest,
                    jsDeserializerTest1, data.get(), 12, changeStr.c_str());
        t2.join();
        EXPECT_TRUE(strcmp((char *)buffer, "world hella") == 0) << "Serialize JSArrayBuffer fail";
        changeStr = "world hellb";
        JSDeserializerTest jsDeserializerTest2;
        std::thread t3(&JSDeserializerTest::JSSharedArrayBufferTest,
                    jsDeserializerTest2, data.get(), 12, changeStr.c_str());
        t3.join();
        EXPECT_TRUE(strcmp((char *)buffer, "world hellb") == 0) << "Serialize JSArrayBuffer fail";
    }
    delete serializer;
    data.reset();
    EXPECT_TRUE(JSHandle<JSTaggedValue>(jsArrayBuffer)->IsSharedArrayBuffer());
};

HWTEST_F_L0(JSSerializerTest, SerializeJSNativePointer)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSNativePointer> np = factory->NewJSNativePointer(nullptr);
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(np),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    // Don't support serialize JSNativePointer directly
    EXPECT_TRUE(!success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    EXPECT_TRUE(data->IsIncompleteData());
    delete serializer;
}

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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(int8Array),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize type array fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TypedArrayTest1, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(int8Array),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize type array fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TypedArrayTest2, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

JSHandle<JSObject> CreateEmptySObject(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> nullHandle = globalConst->GetHandledNull();
    JSHandle<LayoutInfo> emptyLayout = factory->CreateSLayoutInfo(0);
    JSHandle<JSHClass> hclass = factory->NewSEcmaHClass(JSSharedObject::SIZE, 0, JSType::JS_SHARED_OBJECT, nullHandle,
                                                        JSHandle<JSTaggedValue>(emptyLayout));
    return factory->NewSharedOldSpaceJSObject(hclass);
}

JSHandle<JSObject> CreateSObject(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> nullHandle = globalConst->GetHandledNull();

    uint32_t index = 0;
    PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
    attributes.SetIsInlinedProps(true);
    attributes.SetRepresentation(Representation::TAGGED);
    uint32_t length = 4;
    JSHandle<LayoutInfo> layout = factory->CreateSLayoutInfo(length);

    JSHandle<EcmaString> key1(factory->NewFromASCII("str1"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("str2"));

    while (index < length) {
        attributes.SetOffset(index);
        attributes.SetIsAccessor(false);
        key2 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(thread->GetEcmaVM(), key2, key1));
        auto stringTable = thread->GetEcmaVM()->GetEcmaStringTable();
        stringTable->GetOrInternString(thread->GetEcmaVM(), *key2);
        layout->AddKey(thread, index++, key2.GetTaggedValue(), attributes);
    }

    JSHandle<JSHClass> hclass = factory->NewSEcmaHClass(JSSharedObject::SIZE, length, JSType::JS_SHARED_OBJECT,
                                                        nullHandle, JSHandle<JSTaggedValue>(layout));
    JSHandle<JSObject> object = factory->NewSharedOldSpaceJSObject(hclass);
    uint32_t fieldIndex = 0;
    while (fieldIndex < length) {
        JSHandle<JSObject> emptyObject = CreateEmptySObject(thread);
        object->SetPropertyInlinedProps(thread, fieldIndex++, emptyObject.GetTaggedValue());
    }
    return object;
}

JSHandle<JSSharedSet> CreateSSet(JSThread *thread)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> proto = globalEnv->GetSharedSetPrototype();
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

JSHandle<JSSharedMap> CreateSMap(JSThread *thread)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> proto = globalEnv->GetSharedMapPrototype();
    auto emptySLayout = thread->GlobalConstants()->GetHandledEmptySLayoutInfo();
    JSHandle<JSHClass> mapClass = factory->NewSEcmaHClass(JSSharedMap::SIZE, 0,
        JSType::JS_SHARED_MAP, proto, emptySLayout);
    JSHandle<JSSharedMap> jsMap = JSHandle<JSSharedMap>::Cast(factory->NewSharedOldSpaceJSObjectWithInit(mapClass));
    JSHandle<LinkedHashMap> linkedMap(
        LinkedHashMap::Create(thread, LinkedHashSet::MIN_CAPACITY, MemSpaceKind::SHARED));
    jsMap->SetLinkedMap(thread, linkedMap);
    jsMap->SetModRecord(0);
    return jsMap;
}

HWTEST_F_L0(JSSerializerTest, SerializeCloneListTest1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> shareObj = CreateSObject(thread);
    EXPECT_TRUE(shareObj->IsJSShared());

    JSHandle<EcmaString> key(factory->NewFromASCII("str2str1"));
    JSHandle<JSTaggedValue> shareObj1 =
        JSObject::GetProperty(thread, JSHandle<JSObject>(shareObj), JSHandle<JSTaggedValue>(key)).GetValue();
    EXPECT_TRUE(shareObj1->IsJSShared());

    JSHandle<JSArray> array = factory->NewJSArray();
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, JSHandle<JSTaggedValue>(shareObj));

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(shareObj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(array));
    EXPECT_TRUE(success) << "SerializeCloneListTest1: Serialize shared obj fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SerializeCloneListTest1, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeCloneListTest2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> rootObj = factory->NewEmptyJSObject();
    JSHandle<JSObject> shareObj = CreateSObject(thread);
    JSHandle<JSObject> noShareObj = CreateSObject(thread);

    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("shareObj"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("noShareObj"));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(rootObj), key1, JSHandle<JSTaggedValue>(shareObj));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(rootObj), key2, JSHandle<JSTaggedValue>(noShareObj));

    JSHandle<JSArray> array = factory->NewJSArray();
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, JSHandle<JSTaggedValue>(shareObj));

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(rootObj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(array));
    EXPECT_TRUE(success) << "SerializeCloneListTest2: Serialize shared obj fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SerializeCloneListTest2, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeCloneListTest3)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> cloneList(factory->NewFromASCII("cloneList"));
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(cloneList));
    EXPECT_FALSE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    BaseDeserializer deserializer(thread, data.release());
    JSHandle<JSTaggedValue> res = deserializer.ReadValue();
    EXPECT_TRUE(res.IsEmpty());
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeCloneListTest4)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();

    JSHandle<JSArrayBuffer> arrBuf = factory->NewJSArrayBuffer(0);
    JSHandle<JSTaggedValue> arrBufTag = JSHandle<JSTaggedValue>::Cast(arrBuf);
    JSHandle<JSArray> array = factory->NewJSArray();
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, arrBufTag);

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(array));
    EXPECT_TRUE(success) << "SerializeCloneListTest4: Serialize shared obj fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SerializeCloneListTest4, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeCloneListTest5)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSArray> array = factory->NewJSArray();
    // set value to array
    array->SetArrayLength(thread, 1);
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, JSHandle<JSTaggedValue>(obj1));
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(array));
    EXPECT_FALSE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    BaseDeserializer deserializer(thread, data.release());
    JSHandle<JSTaggedValue> res = deserializer.ReadValue();
    EXPECT_TRUE(res.IsEmpty());
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSSharedSetBasic1)
{
    JSHandle<JSSharedSet> jsSet = CreateSSet(thread);
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(jsSet),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSSharedSet failed";
    std::unique_ptr<SerializeData> data = serializer->Release();
    {
        for (int i = 0; i < INITIALIZE_SIZE; i++) {
            JSSharedSet::Add(thread, jsSet, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
        }
        JSDeserializerTest jsDeserializerTest;
        // The Deserializer thread will clear the shared set
        std::thread t1(&JSDeserializerTest::JSSharedSetBasicTest1,
                       jsDeserializerTest, data.get());
        ecmascript::ThreadSuspensionScope scope(thread);
        t1.join();
        EXPECT_TRUE(JSSharedSet::GetSize(thread, jsSet) == 0);
    }
    {
        for (int i = 0; i < INITIALIZE_SIZE; i++) {
            JSSharedSet::Add(thread, jsSet, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
        }
        EXPECT_TRUE(!JSSharedSet::Has(thread, jsSet, JSTaggedValue(INITIALIZE_SIZE)));
        JSDeserializerTest jsDeserializerTest;
        // The Deserializer thread will add and delete a element
        std::thread t1(&JSDeserializerTest::JSSharedSetBasicTest2,
                       jsDeserializerTest, data.get());
        ecmascript::ThreadSuspensionScope scope(thread);
        t1.join();
        EXPECT_TRUE(!JSSharedSet::Has(thread, jsSet, JSTaggedValue(0)));
        EXPECT_TRUE(JSSharedSet::Has(thread, jsSet, JSTaggedValue(INITIALIZE_SIZE)));
    }
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeMultiThreadJSSharedSet)
{
    Reset();
    JSHandle<JSSharedSet> jsSet = CreateSSet(thread);
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(jsSet),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSSharedSet fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    for (int i = 0; i < INITIALIZE_SIZE; i++) {
        JSSharedSet::Add(thread, jsSet, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
    }
    constexpr uint32_t maxNumDeserializers = 10;
    JSDeserializerTest jsDeserializerTests[maxNumDeserializers];
    std::thread threads[maxNumDeserializers];
    for (int32_t i = 0; i < maxNumDeserializers; i++) {
        threads[i] = std::thread(&JSDeserializerTest::JSSharedSetMultiThreadTest1,
            jsDeserializerTests[i], data.get());
    }
    ecmascript::ThreadSuspensionScope scope(thread);
    for (int i = 0; i < maxNumDeserializers; i++) {
        threads[i].join();
    }
    EXPECT_TRUE(jsSet->GetModRecord() == 0);
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSSharedMapBasic)
{
    JSHandle<JSSharedMap> jsMap = CreateSMap(thread);
    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(jsMap),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize JSSharedMap failed";
    std::unique_ptr<SerializeData> data = serializer->Release();
    {
        for (int i = 0; i < INITIALIZE_SIZE; i++) {
            JSSharedMap::Set(thread, jsMap, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)),
                JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
        }
        JSDeserializerTest jsDeserializerTest;
        // The Deserializer thread will clear the shared map
        std::thread t1(&JSDeserializerTest::JSSharedMapBasicTest1,
                       jsDeserializerTest, data.get());
        ecmascript::ThreadSuspensionScope scope(thread);
        t1.join();
        EXPECT_TRUE(JSSharedMap::GetSize(thread, jsMap) == 0);
    }
    {
        for (int i = 0; i < INITIALIZE_SIZE; i++) {
            JSSharedMap::Set(thread, jsMap, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)),
                JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
        }
        EXPECT_TRUE(!JSSharedMap::Has(thread, jsMap, JSTaggedValue(INITIALIZE_SIZE)));
        JSDeserializerTest jsDeserializerTest;
        // The Deserializer thread will add and delete a element
        std::thread t1(&JSDeserializerTest::JSSharedMapBasicTest2,
                       jsDeserializerTest, data.get());
        ecmascript::ThreadSuspensionScope scope(thread);
        t1.join();
        EXPECT_TRUE(!JSSharedMap::Has(thread, jsMap, JSTaggedValue(0)));
        EXPECT_TRUE(JSSharedMap::Has(thread, jsMap, JSTaggedValue(INITIALIZE_SIZE)));
    }
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeMultiSharedRegion)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<TaggedArray> array = factory->NewSTaggedArray(10 * 1024, JSTaggedValue::Hole()); // 10 * 1024: array length
    for (int i = 0; i < 5; i++) {
        JSHandle<TaggedArray> element = factory->NewSTaggedArray((11 + i) * 1024, JSTaggedValue::Hole());
        array->Set(thread, i, element);
    }
    JSHandle<JSObject> sobj = CreateEmptySObject(thread);
    sobj->SetElements(thread, array);
    ValueSerializer *serializer = new ValueSerializer(thread, false, true);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(sobj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SerializeMultiSharedRegionTest, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeMultiSharedRegion1)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(3 * 1024, JSTaggedValue::Hole()); // 3 * 1024: array length
    for (int i = 0; i < 5; i++) {
        JSHandle<TaggedArray> element = factory->NewTaggedArray(3 * 1024, JSTaggedValue::Hole());
        array->Set(thread, i, element);
    }
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    obj->SetElements(thread, array);
    ValueSerializer *serializer = new ValueSerializer(thread, false, true);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SerializeMultiSharedRegionTest1, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeMultiSharedRegion2)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<TaggedArray> array = factory->NewTaggedArray(3 * 1024, // 10 * 1024: array length
        JSTaggedValue::Hole());
    for (int i = 0; i < 5; i++) {
        JSHandle<TaggedArray> element = factory->NewTaggedArray(3 * 1024, JSTaggedValue::Hole());
        array->Set(thread, i, element);
    }
    JSHandle<TaggedArray> array1 = factory->NewTaggedArray(3 * 1024, // 10 * 1024: array length
        JSTaggedValue::Hole(), true);
    for (int i = 0; i < 5; i++) {
        JSHandle<TaggedArray> element = factory->NewTaggedArray(3 * 1024, JSTaggedValue::Hole(), true);
        array1->Set(thread, i, element);
    }
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    obj->SetProperties(thread, array);
    obj->SetElements(thread, array1);
    ValueSerializer *serializer = new ValueSerializer(thread, false, true);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success);
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SerializeMultiSharedRegionTest2, jsDeserializerTest, data.release());
    ecmascript::ThreadSuspensionScope scope(thread);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeSourceTextModule)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory *objectFactory = vm->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    CString baseFileName = "modules.abc";
    CString recordName = "a";
    CString recordName1 = "@ohos:hilog";
    module->SetEcmaModuleFilenameString(baseFileName);
    module->SetEcmaModuleRecordNameString(recordName);
    module->SetTypes(ModuleTypes::ECMA_MODULE);
    module->SetStatus(ModuleStatus::INSTANTIATED);
    JSHandle<JSTaggedValue> val = JSHandle<JSTaggedValue>::Cast(objectFactory->NewFromUtf8("val"));
    JSHandle<SourceTextModule> module1 = objectFactory->NewSourceTextModule();
    JSHandle<TaggedArray> requestedModules = objectFactory->NewTaggedArray(2);
    requestedModules->Set(thread, 0, module1);
    requestedModules->Set(thread, 1, module1);
    module->SetRequestedModules(thread, requestedModules.GetTaggedValue());
    JSHandle<JSTaggedValue> importName = val;
    JSHandle<JSTaggedValue> localName = val;
    JSHandle<ImportEntry> importEntry1 =
        objectFactory->NewImportEntry(0, importName, localName, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module, importEntry1, 0, 2);
    JSHandle<JSTaggedValue> starString = thread->GlobalConstants()->GetHandledStarString();
    JSHandle<ImportEntry> importEntry2 =
        objectFactory->NewImportEntry(1, starString, localName, SharedTypes::UNSENDABLE_MODULE);
    SourceTextModule::AddImportEntry(thread, module, importEntry2, 1, 2);

    module1->SetEcmaModuleFilenameString(baseFileName);
    module1->SetEcmaModuleRecordNameString(recordName1);
    JSHandle<LocalExportEntry> localExportEntry =
        objectFactory->NewLocalExportEntry(val, val, 0, SharedTypes::UNSENDABLE_MODULE);
    JSHandle<TaggedArray> localExportEntries = objectFactory->NewTaggedArray(1);
    localExportEntries->Set(thread, 0, localExportEntry);
    module1->SetLocalExportEntries(thread, localExportEntries);
    module1->SetTypes(ModuleTypes::NATIVE_MODULE);
    module1->SetStatus(ModuleStatus::INSTANTIATED);

    ValueSerializer *serializer = new ModuleSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(module),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SourceTextModuleTest, jsDeserializerTest, data.release());
    {
        ThreadSuspensionScope suspensionScope(thread);
        t1.join();
    }
    delete serializer;
};
}  // namespace panda::test
