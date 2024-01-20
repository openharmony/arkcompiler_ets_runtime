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

#include "ecmascript/serializer/value_serializer.h"
#include "ecmascript/serializer/base_deserializer.h"

using namespace panda::ecmascript;
using namespace testing::ext;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using DeserializeFunc = void (*)(SerializeData* data);
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<uint64_t, std::nano>;

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

    void JSSharedArrayBufferTest(SerializeData *data,
                           const JSHandle<JSArrayBuffer> &originArrayBuffer, int32_t byteLength, const char *msg)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
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
        EXPECT_TRUE(buffer == resBuffer) << "Not Same pointer!";
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

    void SharedObjectTest1(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize SharedObject fail";
        EXPECT_TRUE(res->IsJSSharedObject()) << "[NotJSSharedObject] Deserialize SharedObject fail";
        JSHandle<JSObject> sObj = JSHandle<JSObject>::Cast(res);

        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> key1(factory->NewFromASCII("number1"));
        JSHandle<JSTaggedValue> key2(factory->NewFromASCII("boolean2"));
        JSHandle<JSTaggedValue> key3(factory->NewFromASCII("string3"));
        JSHandle<JSTaggedValue> key4(factory->NewFromASCII("funcA"));
        JSHandle<JSTaggedValue> val1 = JSObject::GetProperty(thread, sObj, key1).GetRawValue();
        JSHandle<JSTaggedValue> val2 = JSObject::GetProperty(thread, sObj, key2).GetRawValue();
        JSHandle<JSTaggedValue> val3 = JSObject::GetProperty(thread, sObj, key3).GetRawValue();
        JSHandle<JSTaggedValue> val4 = JSObject::GetProperty(thread, sObj, key4).GetRawValue();
        EXPECT_TRUE(val4->IsJSSharedFunction());
        EXPECT_EQ(val1->GetInt(), 1024);    // 1024 is the expected value
        EXPECT_TRUE(val2->ToBoolean());
        JSHandle<EcmaString> str3 = JSHandle<EcmaString>(val3);
        JSHandle<EcmaString> strTest3 = factory->NewFromStdString("hello world!");
        EXPECT_TRUE(JSTaggedValue::StringCompare(*str3, *strTest3));
        Destroy();
    }

    void SharedObjectTest2(SerializeData *data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize SharedObject fail";
        EXPECT_TRUE(res->IsJSSharedObject()) << "[NotJSSharedObject] Deserialize SharedObject fail";
        JSHandle<JSObject> sObj = JSHandle<JSObject>::Cast(res);

        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> key1(factory->NewFromASCII("funcA"));
        JSHandle<JSTaggedValue> key2(factory->NewFromASCII("funcB"));
        JSHandle<JSTaggedValue> val1 = JSObject::GetProperty(thread, sObj, key1).GetRawValue();
        JSHandle<JSTaggedValue> val2 = JSObject::GetProperty(thread, sObj, key2).GetRawValue();
        EXPECT_TRUE(val1->IsJSSharedFunction());
        EXPECT_TRUE(val2->IsJSSharedFunction());
        EXPECT_TRUE(val1->IsCallable());
        JSHandle<JSFunction> func1(val1);
        EXPECT_FALSE(func1->GetProtoOrHClass().IsHole());
        EXPECT_TRUE(func1->GetLexicalEnv().IsTaggedArray());
        EXPECT_TRUE(func1->GetHomeObject().IsJSSharedObject());
        Destroy();
    }

    void SharedObjectTest3(SerializeData* data)
    {
        Init();
        BaseDeserializer deserializer(thread, data);
        JSHandle<JSTaggedValue> res = deserializer.ReadValue();
        EXPECT_FALSE(res.IsEmpty());
        EXPECT_TRUE(res->IsJSSharedObject()) << "[NotJSSharedObject] Deserialize SharedObject fail";
        
        JSHandle<JSObject> sObj = JSHandle<JSObject>::Cast(res);
        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, sObj);
        uint32_t length = array->GetLength();
        EXPECT_EQ(length, 10U);
        for (uint32_t i = 0; i < length; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            JSHandle<JSTaggedValue> value =
                JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(sObj), key).GetValue();
            EXPECT_TRUE(value->GetTaggedObject()->GetClass()->IsJSObject());
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

    void ConcurrentFunctionTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.Deserialize();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ConcurrentFunction fail";
        EXPECT_TRUE(res->IsJSFunction()) << "[NotJSFunction] Deserialize ConcurrentFunction fail";
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

        JSHandle<JSTaggedValue> key3(factory->NewFromASCII("key"));
        OperationResult result3 = JSObject::GetProperty(thread, res, key3);
        JSHandle<JSTaggedValue> value3 = result3.GetRawValue();
        EXPECT_TRUE(value3->IsJSFunction());

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
    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->SerializeJSTaggedValue(JSTaggedValue::True());
    serializer->SerializeJSTaggedValue(JSTaggedValue::False());
    serializer->SerializeJSTaggedValue(JSTaggedValue::Undefined());
    serializer->SerializeJSTaggedValue(JSTaggedValue::Null());
    serializer->SerializeJSTaggedValue(JSTaggedValue::Hole());
    std::unique_ptr<SerializeData> data = serializer->Release();

    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSSpecialValueTest, jsDeserializerTest, data.release());
    t1.join();
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
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::ERROR, msg));

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, errorTag, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSErrorTest1, jsDeserializerTest, data.release());
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSError2)
{
#ifdef NDEBUG
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSHandle<EcmaString> key1(factory->NewFromASCII("error1"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("error2"));
    JSHandle<EcmaString> msg(factory->NewFromASCII("this is error"));
    JSHandle<JSTaggedValue> errorTag =
        JSHandle<JSTaggedValue>::Cast(factory->NewJSError(base::ErrorType::ERROR, msg));


    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key1), errorTag);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), JSHandle<JSTaggedValue>(key2), errorTag);

    ValueSerializer *serializer = new ValueSerializer(thread);
    serializer->WriteValue(thread, JSHandle<JSTaggedValue>(obj),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                           JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSErrorTest2, jsDeserializerTest, data.release());
    t1.join();
    delete serializer;
#endif
};

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
    t1.join();
    delete serializer;
};

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
    t1.join();
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
    t1.join();
    delete serializer;
    // test if detached
    EXPECT_FALSE(arrBuf->IsDetach());
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
                   jsDeserializerTest, data.release(), jsArrayBuffer, 12, changeStr.c_str());
    t1.join();
    EXPECT_TRUE(strcmp((char *)buffer, "world hello") == 0) << "Serialize JSArrayBuffer fail";
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

    ValueSerializer *serializer = new ValueSerializer(thread);
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(int8Array),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    EXPECT_TRUE(success) << "Serialize type array fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TypedArrayTest1, jsDeserializerTest, data.release());
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
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeSharedObject1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetSObjectFunction();
    JSHandle<JSSharedObject> sObj =
        JSHandle<JSSharedObject>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("number1"));
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("boolean2"));
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII("string3"));
    JSHandle<JSTaggedValue> key4(factory->NewFromASCII("funcA"));
    JSHandle<JSTaggedValue> key5(factory->NewFromASCII("funcB"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1024));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue::True());
    JSHandle<JSTaggedValue> value3(factory->NewFromStdString("hello world!"));
    
    // test func
    JSHandle<JSFunction> func1 = thread->GetEcmaVM()->GetFactory()->NewSFunction(env, nullptr,
        FunctionKind::NORMAL_FUNCTION);
    EXPECT_TRUE(*func1 != nullptr);
    JSHandle<JSTaggedValue> value4(thread, func1.GetTaggedValue());
    EXPECT_TRUE(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(sObj), key1, value1));
    EXPECT_TRUE(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(sObj), key2, value2));
    EXPECT_TRUE(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(sObj), key3, value3));
    EXPECT_TRUE(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(sObj), key4, value4));
    ValueSerializer *serializer = new ValueSerializer(thread);

    // set value to array
    JSHandle<JSArray> array = factory->NewJSArray();
    array->SetArrayLength(thread, 1);
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, JSHandle<JSTaggedValue>(sObj));
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(sObj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(array));
    EXPECT_TRUE(success) << "Serialize sObj fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SharedObjectTest1, jsDeserializerTest, data.release());
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeSharedObject2)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetSObjectFunction();
    JSHandle<JSSharedObject> sObj =
        JSHandle<JSSharedObject>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<EcmaString> key1(factory->NewFromASCII("str1"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("str2"));
    for (int i = 0; i < 10; i++) {
        JSHandle<JSSharedObject> sObj1 =
            JSHandle<JSSharedObject>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
        JSHandle<EcmaString> key3(factory->NewFromASCII("str3"));
        for (int j = 0; j < 10; j++) {
            key3 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(ecmaVm, key3, key1));
            JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(sObj1), JSHandle<JSTaggedValue>(key3),
                                  JSHandle<JSTaggedValue>(factory->NewEmptyJSObject()));
        }
        key2 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(ecmaVm, key2, key1));
        EXPECT_TRUE(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(sObj), JSHandle<JSTaggedValue>(key2),
                              JSHandle<JSTaggedValue>(sObj1)));
    }

    ValueSerializer *serializer = new ValueSerializer(thread);
    // set value to array
    JSHandle<JSArray> array = factory->NewJSArray();
    array->SetArrayLength(thread, 1);
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, JSHandle<JSTaggedValue>(sObj));
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(sObj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(array));
    EXPECT_TRUE(success) << "Serialize sObj fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SharedObjectTest3, jsDeserializerTest, data.release());
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeSharedObject3)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetSObjectFunction();
    JSHandle<JSSharedObject> sObj =
        JSHandle<JSSharedObject>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<EcmaString> key1(factory->NewFromASCII("str1"));
    JSHandle<EcmaString> key2(factory->NewFromASCII("str2"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    for (int i = 0; i < 512; i++) {
        key2 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Concat(ecmaVm, key2, key1));
        EXPECT_TRUE(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(sObj), JSHandle<JSTaggedValue>(key2),
                                          value1));
    }

    ValueSerializer *serializer = new ValueSerializer(thread);
    // set value to array
    JSHandle<JSArray> array = factory->NewJSArray();
    array->SetArrayLength(thread, 1);
    JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>(array), 0, JSHandle<JSTaggedValue>(sObj));
    bool success = serializer->WriteValue(thread, JSHandle<JSTaggedValue>(sObj),
                                          JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()),
                                          JSHandle<JSTaggedValue>(array));
    EXPECT_TRUE(success) << "Serialize sObj fail";
    std::unique_ptr<SerializeData> data = serializer->Release();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::SharedObjectTest4, jsDeserializerTest, data.release());
    t1.join();
    delete serializer;
};
}  // namespace panda::test
