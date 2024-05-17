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

#include "builtin_test_util.h"
#include "ecmascript/builtins/builtins_dataview.h"

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using DataViewType = ecmascript::DataViewType;
class BuiltinsDataViewTest : public BaseTestWithScope<false> {
};

enum class AlgorithmType {
    GET_OFFSET,
    GET_BYTELENGTH,
    GET_BUFFER,
    GET_INT8,
    SET_INT8,
    GET_UINT8,
    SET_UINT8,
    GET_UINT16,
    SET_UINT16,
    GET_INT16,
    SET_INT16,
    GET_UINT32,
    SET_UINT32,
    GET_INT32,
    SET_INT32,
    GET_FLOAT32,
    SET_FLOAT32,
    GET_FLOAT64,
    SET_FLOAT64,
    GET_BIGINT64,
    SET_BIGINT64,
};

static JSTaggedValue DataViewAlgorithmGet(AlgorithmType type, EcmaRuntimeCallInfo* ecmaRuntimeCallInfo)
{
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::GET_OFFSET:
            result = BuiltinsDataView::GetOffset(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_BYTELENGTH:
            result = BuiltinsDataView::GetByteLength(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_BUFFER:
            result = BuiltinsDataView::GetBuffer(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_INT8:
            result = BuiltinsDataView::GetInt8(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_UINT16:
            result = BuiltinsDataView::GetUint16(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_INT16:
            result = BuiltinsDataView::GetInt16(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_UINT32:
            result = BuiltinsDataView::GetUint32(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_INT32:
            result = BuiltinsDataView::GetInt32(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_FLOAT32:
            result = BuiltinsDataView::GetFloat32(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_FLOAT64:
            result = BuiltinsDataView::GetFloat64(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_BIGINT64:
            result = BuiltinsDataView::GetBigInt64(ecmaRuntimeCallInfo);
            break;
        default:
            break;
    }
    return result;
}

static JSTaggedValue DataViewAlgorithm(JSThread *thread, std::vector<JSTaggedValue>& args, int32_t maxArgLen,
    AlgorithmType type, JSTaggedValue thisValue = JSTaggedValue::Undefined())
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, maxArgLen, thisValue);
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::SET_INT8:
            result = BuiltinsDataView::SetInt8(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_UINT8:
            result = BuiltinsDataView::GetUint8(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::SET_UINT8:
            result = BuiltinsDataView::SetUint8(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::SET_UINT16:
            result = BuiltinsDataView::SetUint16(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::SET_INT16:
            result = BuiltinsDataView::SetInt16(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::SET_UINT32:
            result = BuiltinsDataView::SetUint32(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::SET_INT32:
            result = BuiltinsDataView::SetInt32(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::SET_FLOAT32:
            result = BuiltinsDataView::SetFloat32(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::SET_FLOAT64:
            result = BuiltinsDataView::SetFloat64(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::SET_BIGINT64:
            result = BuiltinsDataView::SetBigInt64(ecmaRuntimeCallInfo);
            break;
        default:
            result = DataViewAlgorithmGet(type, ecmaRuntimeCallInfo);
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

JSTaggedValue CreateBuiltinsDataviewArrayBuffer(JSThread *thread, int32_t length)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> arrayBuffer(thread, env->GetArrayBufferFunction().GetTaggedValue());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    // 6 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayBuffer), 6);
    ecmaRuntimeCallInfo->SetFunction(arrayBuffer.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(length));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::ArrayBufferConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

JSTaggedValue CreateBuiltinsDataView(JSThread *thread, int32_t length, int32_t byte_offset)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> dataView(thread, env->GetDataViewFunction().GetTaggedValue());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    JSTaggedValue tagged = CreateBuiltinsDataviewArrayBuffer(thread, length);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    // 8 : test case
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*dataView), 8);
    ecmaRuntimeCallInfo->SetFunction(dataView.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(byte_offset));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDataView::DataViewConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

void SetUint8(JSThread *thread, const JSHandle<JSDataView> &view, int32_t offset, JSTaggedValue value)
{
    std::vector<JSTaggedValue> vals{JSTaggedValue(offset), value};
    DataViewAlgorithm(thread, vals, 8, AlgorithmType::SET_UINT8, view.GetTaggedValue());  // 8: data max len
}

// new DataView(new ArrayBuffer(10), 1)
HWTEST_F_L0(BuiltinsDataViewTest, Constructor)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> dataView(thread, env->GetDataViewFunction().GetTaggedValue());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    JSTaggedValue tagged = CreateBuiltinsDataviewArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*dataView), 8);
    ecmaRuntimeCallInfo->SetFunction(dataView.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(1));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDataView::DataViewConstructor(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsECMAObject());
    TestHelper::TearDownFrame(thread, prev);

    // case: Detached Buffer
    arrBuf->SetArrayBufferData(thread, JSTaggedValue::Null());
    ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*dataView), 8);
    ecmaRuntimeCallInfo->SetFunction(dataView.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(1));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    result = BuiltinsDataView::DataViewConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();
}

// new DataView(new ArrayBuffer(10), 1).byteOffset
HWTEST_F_L0(BuiltinsDataViewTest, byteOffset)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 10, 1);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));

    std::vector<JSTaggedValue> vals{};
    auto result = DataViewAlgorithm(thread, vals, 4, AlgorithmType::GET_OFFSET, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());

    // case: Detached Buffer
    JSTaggedValue tagged1 = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged1.GetRawData())));
    arrBuf->SetArrayBufferData(thread, JSTaggedValue::Null());
    view->SetViewedArrayBuffer(thread, arrBuf);

    result = DataViewAlgorithm(thread, vals, 4, AlgorithmType::GET_OFFSET, view.GetTaggedValue());
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();
}

// new DataView(new ArrayBuffer(10), 2).byteLength
HWTEST_F_L0(BuiltinsDataViewTest, byteLength)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 10, 2);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));

    std::vector<JSTaggedValue> vals{};
    auto result = DataViewAlgorithm(thread, vals, 4, AlgorithmType::GET_BYTELENGTH, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(8).GetRawData());

    // case: Detached Buffer
    JSTaggedValue tagged1 = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged1.GetRawData())));
    arrBuf->SetArrayBufferData(thread, JSTaggedValue::Null());
    view->SetViewedArrayBuffer(thread, arrBuf);
    result = DataViewAlgorithm(thread, vals, 4, AlgorithmType::GET_BYTELENGTH, view.GetTaggedValue());
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();
}

// new DataView(new ArrayBuffer(10), 1).buffer
HWTEST_F_L0(BuiltinsDataViewTest, buffer)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 10, 1);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    std::vector<JSTaggedValue> vals{};
    auto result = DataViewAlgorithm(thread, vals, 4, AlgorithmType::GET_BUFFER, view.GetTaggedValue());
    ASSERT_EQ(result.IsArrayBuffer(), true);
}

// new DataView(new ArrayBuffer(8), 0).SetUint16/GetUint16
HWTEST_F_L0(BuiltinsDataViewTest, getUint16)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));

    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue(-1870724872), JSTaggedValue::False()};
    auto result = DataViewAlgorithm(thread, vals, 10, AlgorithmType::SET_UINT16, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    std::vector<JSTaggedValue> vals2{JSTaggedValue(0), JSTaggedValue::True()};
    auto result1 = DataViewAlgorithm(thread, vals2, 8, AlgorithmType::GET_UINT16, view.GetTaggedValue());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(63488).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetInt16/GetInt16
HWTEST_F_L0(BuiltinsDataViewTest, getInt16)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue(-1870724872), JSTaggedValue::False()};
    auto result = DataViewAlgorithm(thread, vals, 10, AlgorithmType::SET_INT16, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    std::vector<JSTaggedValue> vals2{JSTaggedValue(0), JSTaggedValue::True()};
    auto result1 = DataViewAlgorithm(thread, vals2, 8, AlgorithmType::GET_INT16, view.GetTaggedValue());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(-2048).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetUint8/GetUint32
static JSHandle<JSDataView> GetCommonInt32(JSThread *thread)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0); // 8: data len
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    SetUint8(thread, view, 0, JSTaggedValue(127)); // 127:value
    SetUint8(thread, view, 1, JSTaggedValue(255)); // 1: the second value, 255:value
    SetUint8(thread, view, 2, JSTaggedValue(255)); // 2: the third value, 255:value
    SetUint8(thread, view, 3, JSTaggedValue(255)); // 3: the forth value, 255:value
    return view;
}

HWTEST_F_L0(BuiltinsDataViewTest, GetUint32)
{
    auto view = GetCommonInt32(thread);
    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue::False()};
    auto result = DataViewAlgorithm(thread, vals, 8, AlgorithmType::GET_UINT32, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(2147483647).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetUint8/GetInt32
HWTEST_F_L0(BuiltinsDataViewTest, GetInt32)
{
    auto view = GetCommonInt32(thread);
    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue::False()};
    auto result = DataViewAlgorithm(thread, vals, 8, AlgorithmType::GET_INT32, view.GetTaggedValue());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(2147483647).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetUint8/GetInt8
HWTEST_F_L0(BuiltinsDataViewTest, GetInt8)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    SetUint8(thread, view, 0, JSTaggedValue(255));

    std::vector<JSTaggedValue> vals{JSTaggedValue(0)};
    auto result = DataViewAlgorithm(thread, vals, 6, AlgorithmType::GET_INT8, view.GetTaggedValue());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetUint8/GetUint8
HWTEST_F_L0(BuiltinsDataViewTest, GetUint8)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    SetUint8(thread, view, 0, JSTaggedValue(127));

    std::vector<JSTaggedValue> vals{JSTaggedValue(0)};
    auto result = DataViewAlgorithm(thread, vals, 6, AlgorithmType::GET_UINT8, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(127).GetRawData());
}

// new DataView(new ArrayBuffer(8), 4).SetUint8/GetFloat32
HWTEST_F_L0(BuiltinsDataViewTest, GetFloat32)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    SetUint8(thread, view, 4, JSTaggedValue(75));
    SetUint8(thread, view, 5, JSTaggedValue(75));
    SetUint8(thread, view, 6, JSTaggedValue(75));
    SetUint8(thread, view, 7, JSTaggedValue(75));

    std::vector<JSTaggedValue> vals{JSTaggedValue(4), JSTaggedValue::False()};
    auto result = DataViewAlgorithm(thread, vals, 8, AlgorithmType::GET_FLOAT32, view.GetTaggedValue());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(13323083)).GetRawData());
}

// new DataView(new ArrayBuffer(12), 4).SetUint8/GetFloat64
HWTEST_F_L0(BuiltinsDataViewTest, GetFloat64)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 12, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    SetUint8(thread, view, 4, JSTaggedValue(67));
    SetUint8(thread, view, 5, JSTaggedValue(67));
    SetUint8(thread, view, 6, JSTaggedValue(68));
    SetUint8(thread, view, 7, JSTaggedValue(68));
    SetUint8(thread, view, 8, JSTaggedValue(67));
    SetUint8(thread, view, 9, JSTaggedValue(67));
    SetUint8(thread, view, 10, JSTaggedValue(68));
    SetUint8(thread, view, 11, JSTaggedValue(68));

    std::vector<JSTaggedValue> vals{JSTaggedValue(4), JSTaggedValue::False()};
    auto result = DataViewAlgorithm(thread, vals, 8, AlgorithmType::GET_FLOAT64, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(10846169068898440)).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetUint32/GetUint32
HWTEST_F_L0(BuiltinsDataViewTest, SetUint32)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));

    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue(0x907f00f8), JSTaggedValue::True()};
    auto result = DataViewAlgorithm(thread, vals, 10, AlgorithmType::SET_UINT32, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    std::vector<JSTaggedValue> vals2{JSTaggedValue(0), JSTaggedValue::False()};
    auto result1 = DataViewAlgorithm(thread, vals2, 8, AlgorithmType::GET_UINT32, view.GetTaggedValue());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(0xf8007f90)).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetInt32/GetInt32
HWTEST_F_L0(BuiltinsDataViewTest, SetInt32)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));

    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue(-1870724872), JSTaggedValue::True()};
    auto result = DataViewAlgorithm(thread, vals, 10, AlgorithmType::SET_INT32, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    std::vector<JSTaggedValue> vals2{JSTaggedValue(0), JSTaggedValue::False()};
    auto result1 = DataViewAlgorithm(thread, vals2, 8, AlgorithmType::GET_INT32, view.GetTaggedValue());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(-134185072).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetInt8/GetUint8
HWTEST_F_L0(BuiltinsDataViewTest, SetInt8)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));

    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue(-1)};
    auto result = DataViewAlgorithm(thread, vals, 8, AlgorithmType::SET_INT8, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    std::vector<JSTaggedValue> vals2{JSTaggedValue(0)};
    auto result1 = DataViewAlgorithm(thread, vals2, 6, AlgorithmType::GET_UINT8, view.GetTaggedValue());

    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(255).GetRawData());
}

// new DataView(new ArrayBuffer(4), 0).SetFloat32/GetFloat32
HWTEST_F_L0(BuiltinsDataViewTest, SetFloat32)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 4, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue(42), JSTaggedValue::True()};
    auto result = DataViewAlgorithm(thread, vals, 10, AlgorithmType::SET_FLOAT32, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    std::vector<JSTaggedValue> vals2{JSTaggedValue(0), JSTaggedValue::False()};
    auto result1 = DataViewAlgorithm(thread, vals2, 8, AlgorithmType::GET_FLOAT32, view.GetTaggedValue());
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1.4441781973331565e-41)).GetRawData());
}

// new DataView(new ArrayBuffer(8), 0).SetFloat64/GetFloat64
HWTEST_F_L0(BuiltinsDataViewTest, SetFloat64)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 8, 0);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));

    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue(42), JSTaggedValue::True()};
    auto result = DataViewAlgorithm(thread, vals, 10, AlgorithmType::SET_FLOAT64, view.GetTaggedValue());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    std::vector<JSTaggedValue> vals2{JSTaggedValue(0), JSTaggedValue::False()};
    auto result1 = DataViewAlgorithm(thread, vals2, 8, AlgorithmType::GET_FLOAT64, view.GetTaggedValue());

    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(8.759e-320)).GetRawData());
}

static JSHandle<JSDataView> BigInt64Common(JSThread *thread)
{
    JSTaggedValue tagged = CreateBuiltinsDataView(thread, 10, 2);
    JSHandle<JSDataView> view(thread, JSDataView::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    JSTaggedValue tagged1 = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged1.GetRawData())));
    arrBuf->SetArrayBufferData(thread, JSTaggedValue::Null());
    view->SetViewedArrayBuffer(thread, arrBuf);
    return view;
}

HWTEST_F_L0(BuiltinsDataViewTest, GetBigInt64)
{
    // case: Detached Buffer
    auto view = BigInt64Common(thread);
    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue::False()};
    auto result = DataViewAlgorithm(thread, vals, 8, AlgorithmType::GET_BIGINT64, view.GetTaggedValue());
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();
}

HWTEST_F_L0(BuiltinsDataViewTest, SetBigInt64)
{
    auto view = BigInt64Common(thread);
    std::vector<JSTaggedValue> vals{JSTaggedValue(0), JSTaggedValue(10)};
    auto result = DataViewAlgorithm(thread, vals, 8, AlgorithmType::SET_BIGINT64, view.GetTaggedValue());
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();
    // case index < 0 is unreachable
}
}  // namespace panda::test
