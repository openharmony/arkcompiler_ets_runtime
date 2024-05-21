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

#include "builtin_test_util.h"
#include "ecmascript/builtins/builtins_atomics.h"

#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/builtins/builtins_sharedarraybuffer.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/base/atomic_helper.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using TypedArray = ecmascript::builtins::BuiltinsTypedArray;
class BuiltinsAtomicsTest : public BaseTestWithScope<false> {
};

JSTypedArray *CreateTypedArray(JSThread *thread, const JSHandle<TaggedArray> &array, DataViewType type)
{
    auto vm = thread->GetEcmaVM();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    JSHandle<JSFunction> arrayFunc;
    JSTaggedValue result = JSTaggedValue::Hole();
    switch (type) {
        case DataViewType::BIGINT64: {
            arrayFunc = JSHandle<JSFunction>(env->GetBigInt64ArrayFunction());
            auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6);
            ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*arrayFunc));
            ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
            ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
            result = TypedArray::BigInt64ArrayConstructor(ecmaRuntimeCallInfo1);
            TestHelper::TearDownFrame(thread, prev);
            break;
        }
        case DataViewType::BIGUINT64: {
            arrayFunc = JSHandle<JSFunction>(env->GetBigUint64ArrayFunction());
            auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6);
            ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*arrayFunc));
            ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
            ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
            result = TypedArray::BigUint64ArrayConstructor(ecmaRuntimeCallInfo1);
            TestHelper::TearDownFrame(thread, prev);
            break;
        }
        case DataViewType::INT16: {
            arrayFunc = JSHandle<JSFunction>(env->GetInt16ArrayFunction());
            auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6);
            ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*arrayFunc));
            ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
            ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
            result = TypedArray::Int16ArrayConstructor(ecmaRuntimeCallInfo1);
            TestHelper::TearDownFrame(thread, prev);
            break;
        }
        case DataViewType::INT32: {
            arrayFunc = JSHandle<JSFunction>(env->GetInt32ArrayFunction());
            auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6);
            ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*arrayFunc));
            ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
            ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
            result = TypedArray::Int32ArrayConstructor(ecmaRuntimeCallInfo1);
            TestHelper::TearDownFrame(thread, prev);
            break;
        }
        case DataViewType::INT8: {
            arrayFunc = JSHandle<JSFunction>(env->GetInt8ArrayFunction());
            auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6);
            ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*arrayFunc));
            ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
            ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
            result = TypedArray::Int8ArrayConstructor(ecmaRuntimeCallInfo1);
            TestHelper::TearDownFrame(thread, prev);
            break;
        }
        case DataViewType::UINT16: {
            arrayFunc = JSHandle<JSFunction>(env->GetUint16ArrayFunction());
            auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6);
            ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*arrayFunc));
            ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
            ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
            result = TypedArray::Uint16ArrayConstructor(ecmaRuntimeCallInfo1);
            TestHelper::TearDownFrame(thread, prev);
            break;
        }
        case DataViewType::UINT32: {
            arrayFunc = JSHandle<JSFunction>(env->GetUint32ArrayFunction());
            auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6);
            ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*arrayFunc));
            ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
            ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
            result = TypedArray::Uint32ArrayConstructor(ecmaRuntimeCallInfo1);
            TestHelper::TearDownFrame(thread, prev);
            break;
        }
        case DataViewType::UINT8: {
            arrayFunc = JSHandle<JSFunction>(env->GetUint8ArrayFunction());
            auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*arrayFunc), 6);
            ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*arrayFunc));
            ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
            ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
            result = TypedArray::Uint8ArrayConstructor(ecmaRuntimeCallInfo1);
            TestHelper::TearDownFrame(thread, prev);
            break;
        }
        default: {
            JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
            arrayFunc = JSHandle<JSFunction>(undefined);
            break;
        }
    }
    EXPECT_TRUE(result.IsECMAObject());
    JSTypedArray *arr = JSTypedArray::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return arr;
}

JSTypedArray *CreateInt32TypedArray(JSThread *thread, const JSHandle<JSArrayBuffer> &arrBuf)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> int32_array(env->GetInt32ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    //  6 : test case
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*int32_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*int32_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, arrBuf.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = TypedArray::Int32ArrayConstructor(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsECMAObject());
    JSTypedArray *int32arr = JSTypedArray::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return int32arr;
}

enum class AlgorithmType {
    ALGORITHM_AND,
    ALGORITHM_ADD,
    ALGORITHM_SUB,
    ALGORITHM_OR,
    ALGORITHM_XOR,
    ALGORITHM_LOAD,
    ALGORITHM_STORE,
    ALGORITHM_WAIT,
    ALGORITHM_NOTIFY,
    ALGORITHM_EXCHANGE,
    ALGORITHM_COMP_EXCHANGE,
};

static JSTaggedValue AtomicsAlgorithm(JSThread *thread, JSHandle<JSTaggedValue>& obj, std::vector<int32_t>& vals,
    uint32_t argLen = 8, AlgorithmType type = AlgorithmType::ALGORITHM_LOAD)
{
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argLen);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetCallArg(0, obj.GetTaggedValue());
    for (size_t i = 0; i < vals.size(); i++) {
        ecmaRuntimeCallInfos->SetCallArg(i+1, JSTaggedValue(vals[i]));
    }
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::ALGORITHM_AND:
            result = BuiltinsAtomics::And(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_ADD:
            result = BuiltinsAtomics::Add(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_SUB:
            result = BuiltinsAtomics::Sub(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_LOAD:
            result = BuiltinsAtomics::Load(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_STORE:
            result = BuiltinsAtomics::Store(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_COMP_EXCHANGE:
            result = BuiltinsAtomics::CompareExchange(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_EXCHANGE:
            result = BuiltinsAtomics::Exchange(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_OR:
            result = BuiltinsAtomics::Or(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_XOR:
            result = BuiltinsAtomics::Xor(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_WAIT:
            result = BuiltinsAtomics::Wait(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_NOTIFY:
            result = BuiltinsAtomics::Notify(ecmaRuntimeCallInfos);
            break;
        default:
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

static JSHandle<TaggedArray> CreateArrayList(JSThread *thread, std::vector<int32_t> vals, size_t maxLen)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(maxLen));
    for (size_t i = 0; i < vals.size(); i++) {
        array->Set(thread, i, JSTaggedValue(vals[i]));
    }
    return  array;
}

HWTEST_F_L0(BuiltinsAtomicsTest, Add_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{7, 8, 9};
    auto array = CreateArrayList(thread, arrVals, 3);

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, DataViewType::UINT8));
    std::vector<int32_t> vals{0, 5};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_ADD);
    ASSERT_EQ(result.GetInt(), 7);
}

static JSTaggedValue AddCommon(JSThread *thread, DataViewType type, JSHandle<JSTaggedValue>& obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(10)); // 10： array len

    obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, type));
    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_ADD);
    return result;
}

HWTEST_F_L0(BuiltinsAtomicsTest, Add_2)
{
    ASSERT_NE(thread, nullptr);
    JSHandle<JSTaggedValue> obj;
    auto result = AddCommon(thread, DataViewType::INT8, obj);
    ASSERT_EQ(result.GetInt(), 0);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Add_3)
{
    ASSERT_NE(thread, nullptr);
    JSHandle<JSTaggedValue> obj;
    auto result = AddCommon(thread, DataViewType::UINT16, obj);
    ASSERT_EQ(result.GetInt(), 0);
    std::vector<int32_t> vals{0, 2};
    result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_ADD);
    ASSERT_EQ(result.GetInt(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, SubAndAdd_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, DataViewType::INT16));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsAtomics::Sub(ecmaRuntimeCallInfo);
    JSTaggedValue addResult = BuiltinsAtomics::Add(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(addResult.GetInt(), 3);
}

HWTEST_F_L0(BuiltinsAtomicsTest, And_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{7, 0, 0};
    auto array = CreateArrayList(thread, arrVals, 10);

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, DataViewType::INT32));
    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_AND);
    ASSERT_EQ(result.GetInt(), 7);
}

HWTEST_F_L0(BuiltinsAtomicsTest, And_2)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{7, 0, 0};
    auto array = CreateArrayList(thread, arrVals, 10);

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array,
                                                                                   DataViewType::UINT32));
    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_AND);
    ASSERT_EQ(result.GetInt(), 7);

    std::vector<int32_t> storeVals{0};
    result = AtomicsAlgorithm(thread, obj, storeVals, 8, AlgorithmType::ALGORITHM_LOAD);
    ASSERT_EQ(result.GetInt(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, CompareExchange_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, DataViewType::UINT8));
    std::vector<int32_t> vals{0, 5, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_COMP_EXCHANGE);
    ASSERT_EQ(result.GetInt(), 5);
}

HWTEST_F_L0(BuiltinsAtomicsTest, CompareExchange_2)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, DataViewType::INT8));
    std::vector<int32_t> vals{0, 5, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_COMP_EXCHANGE);
    ASSERT_EQ(result.GetInt(), 5);

    std::vector<int32_t> loadVals{0};
    result = AtomicsAlgorithm(thread, obj, loadVals, 8, AlgorithmType::ALGORITHM_LOAD);
    ASSERT_EQ(result.GetInt(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, TypedArrayCover)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{2, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());
    // UINT16
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array,
                                                                                   DataViewType::UINT16));
    std::vector<int32_t> vals{0, 2, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_COMP_EXCHANGE);
    ASSERT_EQ(result.GetInt(), 2);
    // INT16
    obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, DataViewType::INT16));
    result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_COMP_EXCHANGE);
    ASSERT_EQ(result.GetInt(), 2);
    // UINT32
    obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, DataViewType::UINT32));
    result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_COMP_EXCHANGE);
    ASSERT_EQ(result.GetInt(), 2);
    // INT32
    obj = JSHandle<JSTaggedValue>(thread, CreateTypedArray(thread, array, DataViewType::INT32));
    result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_COMP_EXCHANGE);
    ASSERT_EQ(result.GetInt(), 2);

    // Detached Buffer
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 0);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    obj = JSHandle<JSTaggedValue>(thread, CreateInt32TypedArray(thread, arrBuf));
    arrBuf->SetArrayBufferData(thread, JSTaggedValue::Null());
    result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_COMP_EXCHANGE);
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();
}

HWTEST_F_L0(BuiltinsAtomicsTest, Exchange_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{3, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{0, 6};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_EXCHANGE);
    ASSERT_EQ(result.GetInt(), 3);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Exchange_2)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{3, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{0, 6};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_EXCHANGE);
    ASSERT_EQ(result.GetInt(), 3);

    std::vector<int32_t> loadVals{0};
    result = AtomicsAlgorithm(thread, obj, loadVals, 8, AlgorithmType::ALGORITHM_LOAD);
    ASSERT_EQ(result.GetInt(), 6);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Or_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_OR);
    ASSERT_EQ(result.GetInt(), 5);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Or_2)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_OR);
    ASSERT_EQ(result.GetInt(), 5);

    std::vector<int32_t> loadVals{0};
    result = AtomicsAlgorithm(thread, obj, loadVals, 8, AlgorithmType::ALGORITHM_LOAD);
    ASSERT_EQ(result.GetInt(), 7);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Sub_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 0, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_SUB);
    ASSERT_EQ(result.GetInt(), 5);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Sub_2)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{0, 5, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{1, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_SUB);
    ASSERT_EQ(result.GetInt(), 5);

    std::vector<int32_t> loadVals{1};
    result = AtomicsAlgorithm(thread, obj, loadVals, 8, AlgorithmType::ALGORITHM_LOAD);
    ASSERT_EQ(result.GetInt(), 3);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Xor_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 7, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{1, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_XOR);
    ASSERT_EQ(result.GetInt(), 7);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Xor_2)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 7, 0};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{1, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_XOR);
    ASSERT_EQ(result.GetInt(), 7);

    std::vector<int32_t> loadVals{1};
    result = AtomicsAlgorithm(thread, obj, loadVals, 8, AlgorithmType::ALGORITHM_LOAD);
    ASSERT_EQ(result.GetInt(), 5);
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_FALSE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, IsLockFree_5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(8)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsAtomics::IsLockFree(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.ToBoolean());
}

HWTEST_F_L0(BuiltinsAtomicsTest, Store_1)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 6, 7};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_STORE);
    ASSERT_EQ(result.GetDouble(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Store_2)
{
    ASSERT_NE(thread, nullptr);
    std::vector<int32_t> arrVals{5, 6, 7};
    auto array = CreateArrayList(thread, arrVals, arrVals.size());

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));
    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_STORE);
    ASSERT_EQ(result.GetDouble(), 2);

    std::vector<int32_t> addVals{0};
    result = AtomicsAlgorithm(thread, obj, addVals, 8, AlgorithmType::ALGORITHM_ADD);
    ASSERT_EQ(result.GetInt(), 2);
}

HWTEST_F_L0(BuiltinsAtomicsTest, Wait)
{
    ASSERT_NE(thread, nullptr);
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 4);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateInt32TypedArray(thread, arrBuf));

    // Not Equal
    std::vector<int32_t> vals{0, 2, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_WAIT); // 12 : arg max len
    ASSERT_EQ(result, thread->GlobalConstants()->GetNotEqualString());

    // timeout
    vals[1] = 0;
    vals[2] = 100;
    result = AtomicsAlgorithm(thread, obj, vals, 12, AlgorithmType::ALGORITHM_WAIT); // 12 : arg max len
    ASSERT_EQ(result, thread->GlobalConstants()->GetTimeoutString());
}

HWTEST_F_L0(BuiltinsAtomicsTest, Notify)
{
    ASSERT_NE(thread, nullptr);
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 4);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateInt32TypedArray(thread, arrBuf));

    std::vector<int32_t> vals{0, 2};
    auto result = AtomicsAlgorithm(thread, obj, vals, 10, AlgorithmType::ALGORITHM_NOTIFY);
    ASSERT_EQ(result, JSTaggedValue(0));
}
}
