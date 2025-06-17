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

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class TypedArrayHelperTest : public BaseTestWithScope<false> {
};

EcmaRuntimeCallInfo *CreateTypedArrayCallInfo(JSThread *thread, const JSHandle<TaggedArray> &array, DataViewType type)
{
    auto vm = thread->GetEcmaVM();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> arrayFunc;
    switch (type) {
        case DataViewType::BIGINT64: {
            arrayFunc = JSHandle<JSFunction>(env->GetBigInt64ArrayFunction());
            break;
        }
        case DataViewType::BIGUINT64: {
            arrayFunc = JSHandle<JSFunction>(env->GetBigUint64ArrayFunction());
            break;
        }
        case DataViewType::FLOAT32: {
            arrayFunc = JSHandle<JSFunction>(env->GetFloat32ArrayFunction());
            break;
        }
        case DataViewType::FLOAT64: {
            arrayFunc = JSHandle<JSFunction>(env->GetFloat64ArrayFunction());
            break;
        }
        case DataViewType::INT16: {
            arrayFunc = JSHandle<JSFunction>(env->GetInt16ArrayFunction());
            break;
        }
        case DataViewType::INT32: {
            arrayFunc = JSHandle<JSFunction>(env->GetInt32ArrayFunction());
            break;
        }
        case DataViewType::INT8: {
            arrayFunc = JSHandle<JSFunction>(env->GetInt8ArrayFunction());
            break;
        }
        case DataViewType::UINT16: {
            arrayFunc = JSHandle<JSFunction>(env->GetUint16ArrayFunction());
            break;
        }
        case DataViewType::UINT32: {
            arrayFunc = JSHandle<JSFunction>(env->GetUint32ArrayFunction());
            break;
        }
        case DataViewType::UINT8: {
            arrayFunc = JSHandle<JSFunction>(env->GetUint8ArrayFunction());
            break;
        }
        case DataViewType::UINT8_CLAMPED: {
            arrayFunc = JSHandle<JSFunction>(env->GetUint8ClampedArrayFunction());
            break;
        }
        default: {
            JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
            arrayFunc = JSHandle<JSFunction>(undefined);
            break;
        }
    }
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    EcmaRuntimeCallInfo* ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6); // 6 : arguments length
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue(*arrayFunc));
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo->SetCallArg(0, jsarray.GetTaggedValue());
    return ecmaRuntimeCallInfo;
}

void CreateArrayList(JSThread *thread, JSHandle<JSTaggedValue> &arrayVal)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(0));
    EcmaRuntimeCallInfo *argv = CreateTypedArrayCallInfo(thread, array, DataViewType::UINT32);
    JSHandle<JSTaggedValue> constructorName = thread->GlobalConstants()->GetHandledUint32ArrayString();
    auto prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> uint32Array(
        thread, TypedArrayHelper::TypedArrayConstructor(argv, constructorName, DataViewType::UINT32));
    TestHelper::TearDownFrame(thread, prev);
    arrayVal = uint32Array;
}

HWTEST_F_L0(TypedArrayHelperTest, TypedArrayConstructor)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(-129)); // Int8 min minus 1
    array->Set(thread, 1, JSTaggedValue(-128)); // Int8 min
    array->Set(thread, 2, JSTaggedValue(128)); // Int8 max plus 1
    EcmaRuntimeCallInfo* argv = CreateTypedArrayCallInfo(thread, array, DataViewType::INT8);
    JSHandle<JSTaggedValue> constructorName = thread->GlobalConstants()->GetHandledInt8ArrayString();
    auto prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> int8Array(thread, TypedArrayHelper::TypedArrayConstructor(argv, constructorName,
                                                                                      DataViewType::INT8));
    TestHelper::TearDownFrame(thread, prev);
    OperationResult result0 = JSTypedArray::GetProperty(thread, int8Array, 0U);
    OperationResult result1 = JSTypedArray::GetProperty(thread, int8Array, 1U);
    OperationResult result2 = JSTypedArray::GetProperty(thread, int8Array, 2U);
    EXPECT_EQ(result0.GetRawValue().GetTaggedValue().GetInt(), 127);  // 127 : -129 to int8 127
    EXPECT_EQ(result1.GetRawValue().GetTaggedValue().GetInt(), -128); // -128 : -128 to int8 -128
    EXPECT_EQ(result2.GetRawValue().GetTaggedValue().GetInt(), -128); // -128 : 128 to int8 -128
}

HWTEST_F_L0(TypedArrayHelperTest, AllocateTypedArray_001)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto factory = ecmaVm->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(0));
    EcmaRuntimeCallInfo* argv = CreateTypedArrayCallInfo(thread, array, DataViewType::UINT8);
    JSHandle<JSTaggedValue> constructorName = thread->GlobalConstants()->GetHandledUint8ArrayString();
    auto prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> newTarget = BuiltinsBase::GetNewTarget(argv);
    JSHandle<JSObject> arrayObj =
        TypedArrayHelper::AllocateTypedArray(thread, constructorName, newTarget, DataViewType::UINT8);
    TestHelper::TearDownFrame(thread, prev);
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*arrayObj);
    EXPECT_EQ(jsTypedArray->GetContentType(), ContentType::Number);
    EXPECT_EQ(jsTypedArray->GetTypedArrayName(thread).GetRawData(), constructorName.GetTaggedValue().GetRawData());
    EXPECT_EQ(jsTypedArray->GetByteLength(), 0U);
    EXPECT_EQ(jsTypedArray->GetByteOffset(), 0U);
    EXPECT_EQ(jsTypedArray->GetArrayLength(), 0U);
}

HWTEST_F_L0(TypedArrayHelperTest, AllocateTypedArray_002)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto factory = ecmaVm->GetFactory();
    int32_t length = 256;
    JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    EcmaRuntimeCallInfo* argv = CreateTypedArrayCallInfo(thread, array, DataViewType::UINT8);
    JSHandle<JSTaggedValue> constructorName = thread->GlobalConstants()->GetHandledUint8ArrayString();
    auto prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> newTarget = BuiltinsBase::GetNewTarget(argv);
    JSHandle<JSObject> arrayObj =
        TypedArrayHelper::AllocateTypedArray(thread, constructorName, newTarget, length, DataViewType::UINT8);
    TestHelper::TearDownFrame(thread, prev);
    JSTypedArray *jsTypedArray = JSTypedArray::Cast(*arrayObj);
    EXPECT_EQ(jsTypedArray->GetContentType(), ContentType::Number);
    EXPECT_EQ(jsTypedArray->GetTypedArrayName(thread).GetRawData(), constructorName.GetTaggedValue().GetRawData());
    EXPECT_EQ(jsTypedArray->GetByteLength(), 256U);
    EXPECT_EQ(jsTypedArray->GetByteOffset(), 0U);
    EXPECT_EQ(jsTypedArray->GetArrayLength(), 256U);
}

HWTEST_F_L0(TypedArrayHelperTest, TypedArraySpeciesCreate)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    uint32_t lenVal = 3;
    JSHandle<TaggedArray> array(factory->NewTaggedArray(lenVal));
    EcmaRuntimeCallInfo* argv = CreateTypedArrayCallInfo(thread, array, DataViewType::INT16);
    JSHandle<JSTaggedValue> constructorName = thread->GlobalConstants()->GetHandledInt16ArrayString();
    auto prev = TestHelper::SetupFrame(thread, argv);
    JSHandle<JSTaggedValue> int16Array(thread, TypedArrayHelper::TypedArrayConstructor(argv, constructorName,
                                                                                      DataViewType::INT16));
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSTypedArray> arrayObj(int16Array);
    uint32_t len = arrayObj->GetArrayLength();
    EXPECT_EQ(len, 3U);

    uint32_t newLenVal = 6;
    JSTaggedType args[1] = {JSTaggedValue(newLenVal).GetRawData()};
    JSHandle<JSObject> newArrObj = TypedArrayHelper::TypedArraySpeciesCreate(thread, arrayObj, 1, args); // 1 : one arg
    uint32_t newLen = JSHandle<JSTypedArray>::Cast(newArrObj)->GetArrayLength();
    EXPECT_EQ(newLen, 6U);
}

HWTEST_F_L0(TypedArrayHelperTest, TypedArrayCreate)
{
    auto vm = thread->GetEcmaVM();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetUint16ArrayFunction();
    JSHandle<JSTaggedValue> constructorName = thread->GlobalConstants()->GetHandledUint16ArrayString();
    EXPECT_TRUE(constructor->IsConstructor());

    uint32_t lenVal = 256;
    JSTaggedType args[1] = {JSTaggedValue(lenVal).GetRawData()};
    JSHandle<JSObject> newArrObj = TypedArrayHelper::TypedArrayCreate(thread, constructor, 1, args); // 1 : one arg
    uint32_t len = JSHandle<JSTypedArray>::Cast(newArrObj)->GetArrayLength();
    JSHandle<JSTaggedValue> type(thread, JSHandle<JSTypedArray>::Cast(newArrObj)->GetTypedArrayName(thread));
    EXPECT_EQ(len, 256U);
    EXPECT_EQ(type.GetTaggedValue().GetRawData(), constructorName.GetTaggedValue().GetRawData());
}

HWTEST_F_L0(TypedArrayHelperTest, ValidateTypedArray)
{
    JSHandle<JSTaggedValue> uint32Array;
    CreateArrayList(thread, uint32Array);
    JSHandle<JSTaggedValue> buffer(thread, TypedArrayHelper::ValidateTypedArray(thread, uint32Array));
    JSTaggedValue result = JSHandle<JSTypedArray>::Cast(uint32Array)->GetViewedArrayBufferOrByteArray(thread);
    EXPECT_EQ(buffer.GetTaggedValue().GetRawData(), result.GetRawData());
}

HWTEST_F_L0(TypedArrayHelperTest, GetType_001)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(0));
    EcmaRuntimeCallInfo* argv1 = CreateTypedArrayCallInfo(thread, array, DataViewType::INT8);
    EcmaRuntimeCallInfo* argv2 = CreateTypedArrayCallInfo(thread, array, DataViewType::BIGUINT64);
    EcmaRuntimeCallInfo* argv3 = CreateTypedArrayCallInfo(thread, array, DataViewType::UINT8_CLAMPED);
    JSHandle<JSTaggedValue> constructorName1 = thread->GlobalConstants()->GetHandledInt8ArrayString();
    JSHandle<JSTaggedValue> constructorName2 = thread->GlobalConstants()->GetHandledBigUint64ArrayString();
    JSHandle<JSTaggedValue> constructorName3 = thread->GlobalConstants()->GetHandledUint8ClampedArrayString();
    auto prev1 = TestHelper::SetupFrame(thread, argv1);
    JSHandle<JSTaggedValue> int8ArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv1, constructorName1,
                                                                                      DataViewType::INT8));
    TestHelper::TearDownFrame(thread, prev1);
    auto prev2 = TestHelper::SetupFrame(thread, argv2);
    JSHandle<JSTaggedValue> bigUint64ArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv2, constructorName2,
                                                                                      DataViewType::BIGUINT64));
    TestHelper::TearDownFrame(thread, prev2);
    auto prev3 = TestHelper::SetupFrame(thread, argv3);
    JSHandle<JSTaggedValue> uint8ClampedArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv3,
        constructorName3, DataViewType::UINT8_CLAMPED));
    TestHelper::TearDownFrame(thread, prev3);
    JSHandle<JSTypedArray> int8Array(int8ArrayVal);
    JSHandle<JSTypedArray> bigUint64Array(bigUint64ArrayVal);
    JSHandle<JSTypedArray> uint8ClampedArray(uint8ClampedArrayVal);
    DataViewType type1 = TypedArrayHelper::GetType(int8Array);
    DataViewType type2 = TypedArrayHelper::GetType(JSHandle<JSTypedArray>::Cast(bigUint64Array));
    DataViewType type3 = TypedArrayHelper::GetType(JSHandle<JSTypedArray>::Cast(uint8ClampedArray));
    EXPECT_EQ(type1, DataViewType::INT8);
    EXPECT_EQ(type2, DataViewType::BIGUINT64);
    EXPECT_EQ(type3, DataViewType::UINT8_CLAMPED);
}

HWTEST_F_L0(TypedArrayHelperTest, GetType_002)
{
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_INT8_ARRAY), DataViewType::INT8);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_UINT8_ARRAY), DataViewType::UINT8);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_UINT8_CLAMPED_ARRAY), DataViewType::UINT8_CLAMPED);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_INT16_ARRAY), DataViewType::INT16);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_UINT16_ARRAY), DataViewType::UINT16);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_INT32_ARRAY), DataViewType::INT32);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_UINT32_ARRAY), DataViewType::UINT32);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_FLOAT32_ARRAY), DataViewType::FLOAT32);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_FLOAT64_ARRAY), DataViewType::FLOAT64);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_BIGINT64_ARRAY), DataViewType::BIGINT64);
    EXPECT_EQ(TypedArrayHelper::GetType(JSType::JS_OBJECT), DataViewType::BIGUINT64);
}

HWTEST_F_L0(TypedArrayHelperTest, GetElementSize_001)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(0));
    EcmaRuntimeCallInfo* argv1 = CreateTypedArrayCallInfo(thread, array, DataViewType::INT8);
    EcmaRuntimeCallInfo* argv2 = CreateTypedArrayCallInfo(thread, array, DataViewType::INT16);
    EcmaRuntimeCallInfo* argv3 = CreateTypedArrayCallInfo(thread, array, DataViewType::INT32);
    EcmaRuntimeCallInfo* argv4 = CreateTypedArrayCallInfo(thread, array, DataViewType::BIGUINT64);
    JSHandle<JSTaggedValue> constructorName1 = thread->GlobalConstants()->GetHandledInt8ArrayString();
    JSHandle<JSTaggedValue> constructorName2 = thread->GlobalConstants()->GetHandledInt16ArrayString();
    JSHandle<JSTaggedValue> constructorName3 = thread->GlobalConstants()->GetHandledInt32ArrayString();
    JSHandle<JSTaggedValue> constructorName4 = thread->GlobalConstants()->GetHandledBigUint64ArrayString();
    auto prev1 = TestHelper::SetupFrame(thread, argv1);
    JSHandle<JSTaggedValue> int8ArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv1, constructorName1,
                                                                                      DataViewType::INT8));
    TestHelper::TearDownFrame(thread, prev1);
    auto prev2 = TestHelper::SetupFrame(thread, argv2);
    JSHandle<JSTaggedValue> int16ArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv2, constructorName2,
                                                                                      DataViewType::INT16));
    TestHelper::TearDownFrame(thread, prev2);
    auto prev3 = TestHelper::SetupFrame(thread, argv3);
    JSHandle<JSTaggedValue> int32ArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv3, constructorName3,
                                                                                      DataViewType::INT32));
    TestHelper::TearDownFrame(thread, prev3);
    auto prev4 = TestHelper::SetupFrame(thread, argv4);
    JSHandle<JSTaggedValue> bigUint64ArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv3, constructorName4,
                                                                                      DataViewType::BIGUINT64));
    TestHelper::TearDownFrame(thread, prev4);
    JSHandle<JSTypedArray> int8Array(int8ArrayVal);
    JSHandle<JSTypedArray> int16Array(int16ArrayVal);
    JSHandle<JSTypedArray> int32Array(int32ArrayVal);
    JSHandle<JSTypedArray> bigUint64Array(bigUint64ArrayVal);
    uint32_t elementSize1 = TypedArrayHelper::GetElementSize(int8Array);
    uint32_t elementSize2 = TypedArrayHelper::GetElementSize(JSHandle<JSTypedArray>::Cast(int16Array));
    uint32_t elementSize3 = TypedArrayHelper::GetElementSize(JSHandle<JSTypedArray>::Cast(int32Array));
    uint32_t elementSize4 = TypedArrayHelper::GetElementSize(JSHandle<JSTypedArray>::Cast(bigUint64Array));
    EXPECT_EQ(elementSize1, 1U);
    EXPECT_EQ(elementSize2, 2U);
    EXPECT_EQ(elementSize3, 4U);
    EXPECT_EQ(elementSize4, 8U);
}

HWTEST_F_L0(TypedArrayHelperTest, GetElementSize_002)
{
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_INT8_ARRAY), 1U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_UINT8_ARRAY), 1U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_UINT8_CLAMPED_ARRAY), 1U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_INT16_ARRAY), 2U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_UINT16_ARRAY), 2U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_INT32_ARRAY), 4U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_UINT32_ARRAY), 4U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_FLOAT32_ARRAY), 4U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_FLOAT64_ARRAY), 8U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_BIGINT64_ARRAY), 8U);
    EXPECT_EQ(TypedArrayHelper::GetElementSize(JSType::JS_OBJECT), 8U);
}

HWTEST_F_L0(TypedArrayHelperTest, GetConstructor)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(0));
    EcmaRuntimeCallInfo* argv1 = CreateTypedArrayCallInfo(thread, array, DataViewType::INT8);
    EcmaRuntimeCallInfo* argv2 = CreateTypedArrayCallInfo(thread, array, DataViewType::BIGUINT64);
    EcmaRuntimeCallInfo* argv3 = CreateTypedArrayCallInfo(thread, array, DataViewType::UINT8_CLAMPED);
    JSHandle<JSTaggedValue> constructorName1 = thread->GlobalConstants()->GetHandledInt8ArrayString();
    JSHandle<JSTaggedValue> constructorName2 = thread->GlobalConstants()->GetHandledBigUint64ArrayString();
    JSHandle<JSTaggedValue> constructorName3 = thread->GlobalConstants()->GetHandledUint8ClampedArrayString();
    auto prev1 = TestHelper::SetupFrame(thread, argv1);
    JSHandle<JSTaggedValue> int8ArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv1, constructorName1,
                                                                                      DataViewType::INT8));
    TestHelper::TearDownFrame(thread, prev1);
    auto prev2 = TestHelper::SetupFrame(thread, argv2);
    JSHandle<JSTaggedValue> bigUint64ArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv2, constructorName2,
                                                                                      DataViewType::BIGUINT64));
    TestHelper::TearDownFrame(thread, prev2);
    auto prev3 = TestHelper::SetupFrame(thread, argv3);
    JSHandle<JSTaggedValue> uint8ClampedArrayVal(thread, TypedArrayHelper::TypedArrayConstructor(argv3,
        constructorName3, DataViewType::UINT8_CLAMPED));
    TestHelper::TearDownFrame(thread, prev3);
    JSHandle<JSTaggedValue> constructor1 = TypedArrayHelper::GetConstructor(thread, int8ArrayVal);
    JSHandle<JSTaggedValue> constructor2 = TypedArrayHelper::GetConstructor(thread, bigUint64ArrayVal);
    JSHandle<JSTaggedValue> constructor3 = TypedArrayHelper::GetConstructor(thread, uint8ClampedArrayVal);
    EXPECT_EQ(constructor1.GetTaggedValue(), env->GetInt8ArrayFunction().GetTaggedValue());
    EXPECT_EQ(constructor2.GetTaggedValue(), env->GetBigUint64ArrayFunction().GetTaggedValue());
    EXPECT_EQ(constructor3.GetTaggedValue(), env->GetUint8ClampedArrayFunction().GetTaggedValue());
}

HWTEST_F_L0(TypedArrayHelperTest, GetConstructorFromType)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::INT8).GetTaggedValue(),
        env->GetInt8ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::UINT8).GetTaggedValue(),
        env->GetUint8ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::UINT8_CLAMPED).GetTaggedValue(),
        env->GetUint8ClampedArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::INT16).GetTaggedValue(),
        env->GetInt16ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::UINT16).GetTaggedValue(),
        env->GetUint16ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::INT32).GetTaggedValue(),
        env->GetInt32ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::UINT32).GetTaggedValue(),
        env->GetUint32ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::FLOAT32).GetTaggedValue(),
        env->GetFloat32ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::FLOAT64).GetTaggedValue(),
        env->GetFloat64ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::BIGINT64).GetTaggedValue(),
        env->GetBigInt64ArrayFunction().GetTaggedValue());
    EXPECT_EQ(TypedArrayHelper::GetConstructorFromType(thread, DataViewType::BIGUINT64).GetTaggedValue(),
        env->GetBigUint64ArrayFunction().GetTaggedValue());
}

HWTEST_F_L0(TypedArrayHelperTest, GetSizeFromType)
{
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::INT8), 1U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::UINT8), 1U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::UINT8_CLAMPED), 1U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::INT16), 2U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::UINT16), 2U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::INT32), 4U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::UINT32), 4U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::FLOAT32), 4U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::FLOAT64), 8U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::BIGINT64), 8U);
    EXPECT_EQ(TypedArrayHelper::GetSizeFromType(DataViewType::BIGUINT64), 8U);
}

HWTEST_F_L0(TypedArrayHelperTest, SortCompare)
{
    JSHandle<JSTaggedValue> uint32Array;
    CreateArrayList(thread, uint32Array);
    JSHandle<JSTaggedValue> buffer(thread, TypedArrayHelper::ValidateTypedArray(thread, uint32Array));
    EXPECT_FALSE(builtins::BuiltinsArrayBuffer::IsDetachedBuffer(thread, buffer.GetTaggedValue()));

    JSHandle<JSTaggedValue> callbackfnHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> nan(thread, JSTaggedValue(base::NAN_VALUE));
    JSHandle<JSTaggedValue> eZero(thread, JSTaggedValue(-0.0));
    JSHandle<JSTaggedValue> pZero(thread, JSTaggedValue(+0.0));
    JSHandle<JSTaggedValue> larger(thread, JSTaggedValue(321));
    JSHandle<JSTaggedValue> smaller(thread, JSTaggedValue(123));

    // if first value and second value are both NaN, return +0.
    int32_t result = TypedArrayHelper::SortCompare(thread, callbackfnHandle, buffer, nan, nan);
    EXPECT_EQ(result, +0);

    // if first value is NaN, return 1.
    result = TypedArrayHelper::SortCompare(thread, callbackfnHandle, buffer, nan, smaller);
    EXPECT_EQ(result, 1);

    // if second value is NaN, return -1.
    result = TypedArrayHelper::SortCompare(thread, callbackfnHandle, buffer, smaller, nan);
    EXPECT_EQ(result, -1);

    // if first value is smaller than second value, return -1.
    result = TypedArrayHelper::SortCompare(thread, callbackfnHandle, buffer, smaller, larger);
    EXPECT_EQ(result, -1);

    // if first value is larger than second value, return 1.
    result = TypedArrayHelper::SortCompare(thread, callbackfnHandle, buffer, larger, smaller);
    EXPECT_EQ(result, 1);

    // "-0.0" sort compare with "+0.0", retrun -1
    result = TypedArrayHelper::SortCompare(thread, callbackfnHandle, buffer, eZero, pZero);
    EXPECT_EQ(result, -1);

    // "+0.0" sort compare with "-0.0", retrun 1
    result = TypedArrayHelper::SortCompare(thread, callbackfnHandle, buffer, pZero, eZero);
    EXPECT_EQ(result, 1);
}

HWTEST_F_L0(TypedArrayHelperTest, FastCreateTypedArray) {
    uint8_t first = static_cast<uint8_t>(DataViewType::BIGINT64);
    uint8_t last = static_cast<uint8_t>(DataViewType::UINT8_CLAMPED);
    for (uint8_t type = first; type <= last; ++type) {
        DataViewType arrayType = static_cast<DataViewType>(type);
        JSHandle<JSTaggedValue> constructorName = TypedArrayHelper::GetConstructorNameFromType(thread, arrayType);
        JSHandle<JSObject> arrayObj =
            TypedArrayHelper::FastCreateTypedArray(thread, constructorName, 0, arrayType);
        JSTypedArray *jsTypedArray = JSTypedArray::Cast(*arrayObj);
        if (arrayType == DataViewType::BIGINT64 ||
            arrayType == DataViewType::BIGUINT64) {
            EXPECT_EQ(jsTypedArray->GetContentType(), ContentType::BigInt);
        } else {
            EXPECT_EQ(jsTypedArray->GetContentType(), ContentType::Number);
        }
        EXPECT_EQ(jsTypedArray->GetTypedArrayName(thread).GetRawData(), constructorName.GetTaggedValue().GetRawData());
        EXPECT_EQ(jsTypedArray->GetByteLength(), 0U);
        EXPECT_EQ(jsTypedArray->GetByteOffset(), 0U);
        EXPECT_EQ(jsTypedArray->GetArrayLength(), 0U);
    }
}
}  // namespace panda::test
