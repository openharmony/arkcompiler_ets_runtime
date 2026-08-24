/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/serializer/serialize_data.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::test;

namespace panda::test {

class FlatBufferSerializeTest : public testing::Test {
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

    // Helper: serialize a value through the flat-buffer overload, then deserialize back.
    Local<JSValueRef> RoundTrip(Local<JSValueRef> value, size_t &outSize)
    {
        Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
        Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));

        uint8_t *buf = JSNApi::SerializeValue(ecmaVm, value, transfer, cloneList,
                                              false,   /* defaultTransfer */
                                              true,    /* defaultCloneShared */
                                              false,   /* needSerializeStack */
                                              outSize);
        EXPECT_NE(buf, nullptr);
        EXPECT_GT(outSize, static_cast<size_t>(0));

        Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
        free(buf);
        return result;
    }
};

// ===================== Primitive Types =====================

HWTEST_F_L0(FlatBufferSerializeTest, NumberDouble)
{
    size_t outSize = 0;
    double expected = 42.5;
    Local<JSValueRef> value(NumberRef::New(ecmaVm, expected));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsNumber());
    EXPECT_EQ(Local<NumberRef>(result)->Value(), expected);
}

HWTEST_F_L0(FlatBufferSerializeTest, Integer32)
{
    size_t outSize = 0;
    int32_t expected = -12345;
    Local<JSValueRef> value(NumberRef::New(ecmaVm, expected));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsNumber());
    EXPECT_EQ(result->Int32Value(ecmaVm), expected);
}

HWTEST_F_L0(FlatBufferSerializeTest, String)
{
    size_t outSize = 0;
    const char *expected = "hello serialize";
    Local<JSValueRef> value(StringRef::NewFromUtf8(ecmaVm, expected));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsString(ecmaVm));
    std::string actual = Local<StringRef>(result)->ToString(ecmaVm);
    EXPECT_EQ(actual, std::string(expected));
}

HWTEST_F_L0(FlatBufferSerializeTest, BooleanTrue)
{
    size_t outSize = 0;
    Local<JSValueRef> value(BooleanRef::New(ecmaVm, true));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsBoolean());
    EXPECT_TRUE(result->IsTrue());
}

HWTEST_F_L0(FlatBufferSerializeTest, BooleanFalse)
{
    size_t outSize = 0;
    Local<JSValueRef> value(BooleanRef::New(ecmaVm, false));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsBoolean());
    EXPECT_TRUE(result->IsFalse());
}

HWTEST_F_L0(FlatBufferSerializeTest, Undefined)
{
    size_t outSize = 0;
    Local<JSValueRef> value(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsUndefined());
}

HWTEST_F_L0(FlatBufferSerializeTest, Null)
{
    size_t outSize = 0;
    Local<JSValueRef> value(JSValueRef::Null(ecmaVm));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsNull());
}

// ===================== Complex Types =====================

HWTEST_F_L0(FlatBufferSerializeTest, Object)
{
    size_t outSize = 0;

    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "x"), NumberRef::New(ecmaVm, 10));
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "y"), StringRef::NewFromUtf8(ecmaVm, "abc"));
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "flag"), BooleanRef::New(ecmaVm, true));

    Local<JSValueRef> result = RoundTrip(obj, outSize);
    EXPECT_TRUE(result->IsObject(ecmaVm));

    auto resObj = Local<ObjectRef>(result);
    Local<JSValueRef> xVal = resObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "x"));
    Local<JSValueRef> yVal = resObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "y"));
    Local<JSValueRef> flagVal = resObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "flag"));

    EXPECT_TRUE(xVal->IsNumber());
    EXPECT_EQ(xVal->Int32Value(ecmaVm), 10);
    EXPECT_TRUE(yVal->IsString(ecmaVm));
    EXPECT_TRUE(flagVal->IsTrue());
}

HWTEST_F_L0(FlatBufferSerializeTest, Array)
{
    size_t outSize = 0;

    uint32_t len = 4;
    Local<ArrayRef> arr = ArrayRef::New(ecmaVm, len);
    ArrayRef::SetValueAt(ecmaVm, arr, 0, NumberRef::New(ecmaVm, 1.0));
    ArrayRef::SetValueAt(ecmaVm, arr, 1, NumberRef::New(ecmaVm, 2.0));
    ArrayRef::SetValueAt(ecmaVm, arr, 2, StringRef::NewFromUtf8(ecmaVm, "three"));
    ArrayRef::SetValueAt(ecmaVm, arr, 3, BooleanRef::New(ecmaVm, false));

    Local<JSValueRef> result = RoundTrip(arr, outSize);
    EXPECT_TRUE(result->IsArray(ecmaVm));

    auto resArr = Local<ArrayRef>(result);
    EXPECT_EQ(resArr->Length(ecmaVm), len);

    EXPECT_TRUE(ArrayRef::GetValueAt(ecmaVm, resArr, 0)->IsNumber());
    EXPECT_EQ(ArrayRef::GetValueAt(ecmaVm, resArr, 0)->Int32Value(ecmaVm), 1);
    EXPECT_TRUE(ArrayRef::GetValueAt(ecmaVm, resArr, 1)->IsNumber());
    EXPECT_EQ(ArrayRef::GetValueAt(ecmaVm, resArr, 1)->Int32Value(ecmaVm), 2);
    EXPECT_TRUE(ArrayRef::GetValueAt(ecmaVm, resArr, 2)->IsString(ecmaVm));
    EXPECT_TRUE(ArrayRef::GetValueAt(ecmaVm, resArr, 3)->IsFalse());
}

HWTEST_F_L0(FlatBufferSerializeTest, NestedObject)
{
    size_t outSize = 0;

    Local<ObjectRef> inner = ObjectRef::New(ecmaVm);
    inner->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "val"), NumberRef::New(ecmaVm, 99));

    Local<ObjectRef> outer = ObjectRef::New(ecmaVm);
    outer->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "child"), inner);
    outer->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "name"), StringRef::NewFromUtf8(ecmaVm, "parent"));

    Local<JSValueRef> result = RoundTrip(outer, outSize);
    EXPECT_TRUE(result->IsObject(ecmaVm));

    auto resOuter = Local<ObjectRef>(result);
    Local<JSValueRef> nameVal = resOuter->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "name"));
    EXPECT_TRUE(nameVal->IsString(ecmaVm));

    Local<JSValueRef> childVal = resOuter->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "child"));
    EXPECT_TRUE(childVal->IsObject(ecmaVm));

    auto resChild = Local<ObjectRef>(childVal);
    Local<JSValueRef> childValField = resChild->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "val"));
    EXPECT_TRUE(childValField->IsNumber());
    EXPECT_EQ(childValField->Int32Value(ecmaVm), 99);
}

HWTEST_F_L0(FlatBufferSerializeTest, MixedStructure)
{
    size_t outSize = 0;

    Local<ArrayRef> arr = ArrayRef::New(ecmaVm, 2);
    ArrayRef::SetValueAt(ecmaVm, arr, 0, NumberRef::New(ecmaVm, 10));
    ArrayRef::SetValueAt(ecmaVm, arr, 1, NumberRef::New(ecmaVm, 20));

    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "items"), arr);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "count"), NumberRef::New(ecmaVm, 2));

    Local<JSValueRef> result = RoundTrip(obj, outSize);
    EXPECT_TRUE(result->IsObject(ecmaVm));

    auto resObj = Local<ObjectRef>(result);
    Local<JSValueRef> countVal = resObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "count"));
    EXPECT_EQ(countVal->Int32Value(ecmaVm), 2);

    Local<JSValueRef> itemsVal = resObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "items"));
    EXPECT_TRUE(itemsVal->IsArray(ecmaVm));

    auto resItems = Local<ArrayRef>(itemsVal);
    EXPECT_EQ(resItems->Length(ecmaVm), static_cast<uint32_t>(2));
    EXPECT_EQ(ArrayRef::GetValueAt(ecmaVm, resItems, 0)->Int32Value(ecmaVm), 10);
    EXPECT_EQ(ArrayRef::GetValueAt(ecmaVm, resItems, 1)->Int32Value(ecmaVm), 20);
}

// ===================== Edge Cases =====================

HWTEST_F_L0(FlatBufferSerializeTest, MultipleDeserializes)
{
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    const char *expected = "multi-deserialize";

    size_t outSize = 0;
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm,
                                          StringRef::NewFromUtf8(ecmaVm, expected),
                                          transfer, cloneList,
                                          false, true, false, outSize);
    ASSERT_NE(buf, nullptr);
    ASSERT_GT(outSize, static_cast<size_t>(0));

    for (int i = 0; i < 3; i++) {
        Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
        EXPECT_TRUE(result->IsString(ecmaVm));
        std::string actual = Local<StringRef>(result)->ToString(ecmaVm);
        EXPECT_EQ(actual, std::string(expected));
    }

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, EmptyObject)
{
    size_t outSize = 0;
    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    Local<JSValueRef> result = RoundTrip(obj, outSize);
    EXPECT_TRUE(result->IsObject(ecmaVm));
}

HWTEST_F_L0(FlatBufferSerializeTest, EmptyArray)
{
    size_t outSize = 0;
    Local<ArrayRef> arr = ArrayRef::New(ecmaVm, 0);
    Local<JSValueRef> result = RoundTrip(arr, outSize);
    EXPECT_TRUE(result->IsArray(ecmaVm));
    EXPECT_EQ(Local<ArrayRef>(result)->Length(ecmaVm), static_cast<uint32_t>(0));
}

HWTEST_F_L0(FlatBufferSerializeTest, NaN)
{
    size_t outSize = 0;
    Local<JSValueRef> value(NumberRef::New(ecmaVm, std::numeric_limits<double>::quiet_NaN()));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsNumber());
    EXPECT_TRUE(std::isnan(Local<NumberRef>(result)->Value()));
}

HWTEST_F_L0(FlatBufferSerializeTest, Infinity)
{
    size_t outSize = 0;
    Local<JSValueRef> value(NumberRef::New(ecmaVm, std::numeric_limits<double>::infinity()));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsNumber());
    double val = Local<NumberRef>(result)->Value();
    EXPECT_TRUE(std::isinf(val));
    EXPECT_GT(val, 0.0);
}

HWTEST_F_L0(FlatBufferSerializeTest, NegativeInfinity)
{
    size_t outSize = 0;
    Local<JSValueRef> value(NumberRef::New(ecmaVm, -std::numeric_limits<double>::infinity()));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsNumber());
    double val = Local<NumberRef>(result)->Value();
    EXPECT_TRUE(std::isinf(val));
    EXPECT_LT(val, 0.0);
}

HWTEST_F_L0(FlatBufferSerializeTest, Zero)
{
    size_t outSize = 0;

    Local<JSValueRef> posZero(NumberRef::New(ecmaVm, 0.0));
    Local<JSValueRef> negZero(NumberRef::New(ecmaVm, -0.0));

    Local<JSValueRef> posResult = RoundTrip(posZero, outSize);
    Local<JSValueRef> negResult = RoundTrip(negZero, outSize);

    EXPECT_TRUE(posResult->IsNumber());
    EXPECT_TRUE(negResult->IsNumber());
    EXPECT_EQ(Local<NumberRef>(posResult)->Value(), 0.0);
    EXPECT_EQ(Local<NumberRef>(negResult)->Value(), 0.0);
}

HWTEST_F_L0(FlatBufferSerializeTest, LongString)
{
    size_t outSize = 0;
    std::string expected(1024, 'A');
    Local<JSValueRef> value(StringRef::NewFromUtf8(ecmaVm, expected.c_str()));
    Local<JSValueRef> result = RoundTrip(value, outSize);

    EXPECT_TRUE(result->IsString(ecmaVm));
    std::string actual = Local<StringRef>(result)->ToString(ecmaVm);
    EXPECT_EQ(actual, expected);
    EXPECT_EQ(actual.size(), static_cast<size_t>(1024));
}

HWTEST_F_L0(FlatBufferSerializeTest, ManyProperties)
{
    size_t outSize = 0;

    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    const int propCount = 50;
    for (int i = 0; i < propCount; i++) {
        std::string key = "prop_" + std::to_string(i);
        obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, key.c_str()), NumberRef::New(ecmaVm, i));
    }

    Local<JSValueRef> result = RoundTrip(obj, outSize);
    EXPECT_TRUE(result->IsObject(ecmaVm));

    auto resObj = Local<ObjectRef>(result);
    for (int i = 0; i < propCount; i++) {
        std::string key = "prop_" + std::to_string(i);
        Local<JSValueRef> val = resObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, key.c_str()));
        EXPECT_TRUE(val->IsNumber());
        EXPECT_EQ(val->Int32Value(ecmaVm), i);
    }
}

// ===================== Pack/Unpack unit tests =====================

HWTEST_F_L0(FlatBufferSerializeTest, PackProducesNonNullBuffer)
{
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));

    size_t outSize = 0;
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm,
                                          NumberRef::New(ecmaVm, 123.4),
                                          transfer, cloneList,
                                          false, true, false, outSize);
    EXPECT_NE(buf, nullptr);
    EXPECT_GT(outSize, static_cast<size_t>(0));
    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, UnpackIndependentFromSourceBuffer)
{
    // Verify Unpack creates a deep copy: freeing the flat buffer right after
    // Unpack must not affect the resulting SerializeData or the deserialized value.
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    const char *expected = "independent-buffer";

    size_t outSize = 0;
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm,
                                          StringRef::NewFromUtf8(ecmaVm, expected),
                                          transfer, cloneList,
                                          false, true, false, outSize);
    ASSERT_NE(buf, nullptr);
    ASSERT_GT(outSize, static_cast<size_t>(0));

    // Copy the flat buffer into a freshly malloc'd region and free the original,
    // simulating the buffer being read back from disk in another address space.
    uint8_t *ownedCopy = static_cast<uint8_t *>(malloc(outSize));
    ASSERT_NE(ownedCopy, nullptr);
    ASSERT_EQ(memcpy_s(ownedCopy, outSize, buf, outSize), EOK);
    free(buf);

    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, ownedCopy, nullptr);
    free(ownedCopy);

    EXPECT_TRUE(result->IsString(ecmaVm));
    std::string actual = Local<StringRef>(result)->ToString(ecmaVm);
    EXPECT_EQ(actual, std::string(expected));
}

HWTEST_F_L0(FlatBufferSerializeTest, DeserializeNullBufferReturnsUndefined)
{
    uint8_t *nullBuf = nullptr;
    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, nullBuf, nullptr);
    EXPECT_TRUE(result->IsUndefined());
}

HWTEST_F_L0(FlatBufferSerializeTest, PackNullSerializeDataReturnsNull)
{
    std::unique_ptr<SerializeData> nullData;
    size_t outSize = 1; // intentionally non-zero, must be reset to 0
    uint8_t *buf = SerializeData::Pack(nullData, outSize);
    EXPECT_EQ(buf, nullptr);
    EXPECT_EQ(outSize, static_cast<size_t>(0));
}

HWTEST_F_L0(FlatBufferSerializeTest, UnpackNullBufferReturnsNull)
{
    auto data = SerializeData::Unpack(thread, nullptr);
    EXPECT_EQ(data, nullptr);
}

// ===================== P0: Core Missing Features =====================

HWTEST_F_L0(FlatBufferSerializeTest, DefaultTransferTrue_ArrayBufferTransferSemantics)
{
    // 创建包含 ArrayBuffer 的对象
    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    Local<ArrayBufferRef> ab = ArrayBufferRef::New(ecmaVm, 16);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "buf"), ab);

    size_t outSize = 0;
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, obj, transfer, cloneList,
                                          true,   /* defaultTransfer=true */
                                          true,   /* defaultCloneShared */
                                          false,  /* needSerializeStack */
                                          outSize);
    ASSERT_NE(buf, nullptr);
    EXPECT_GT(outSize, static_cast<size_t>(0));

    // 验证：默认转移语义下原始 ArrayBuffer 应被 detach
    EXPECT_EQ(ab->ByteLength(ecmaVm), 0);

    // 反序列化验证：重建的 ArrayBuffer 内容完整
    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    ASSERT_TRUE(result->IsObject(ecmaVm));
    Local<ObjectRef> resultObj = result->ToObject(ecmaVm);
    Local<JSValueRef> resultBuf = resultObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "buf"));
    ASSERT_TRUE(resultBuf->IsArrayBuffer(ecmaVm));
    EXPECT_EQ(Local<ArrayBufferRef>(resultBuf)->ByteLength(ecmaVm), 16);

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, ExplicitTransferAndCloneList)
{
    // 创建对象含两个 ArrayBuffer
    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    Local<ArrayBufferRef> abTransfer = ArrayBufferRef::New(ecmaVm, 8);
    Local<ArrayBufferRef> abKeep = ArrayBufferRef::New(ecmaVm, 16);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "buf1"), abTransfer);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "buf2"), abKeep);

    // transfer 数组仅包含 abTransfer
    Local<ArrayRef> transferArr = ArrayRef::New(ecmaVm, 1);
    ArrayRef::SetValueAt(ecmaVm, transferArr, 0, abTransfer);

    // cloneList 为空数组
    Local<ArrayRef> cloneListArr = ArrayRef::New(ecmaVm, 0);

    size_t outSize = 0;
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, obj, transferArr, cloneListArr,
                                          false,  /* defaultTransfer */
                                          false,  /* defaultCloneShared */
                                          false,  /* needSerializeStack */
                                          outSize);
    ASSERT_NE(buf, nullptr);
    EXPECT_GT(outSize, static_cast<size_t>(0));

    // 验证：transfer 中指定的 abTransfer 被 detach
    EXPECT_EQ(abTransfer->ByteLength(ecmaVm), 0);
    // 验证：不在 transfer 中的 abKeep 未被 detach
    EXPECT_EQ(abKeep->ByteLength(ecmaVm), 16);

    // 反序列化验证
    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    ASSERT_TRUE(result->IsObject(ecmaVm));
    Local<ObjectRef> resultObj = result->ToObject(ecmaVm);
    Local<JSValueRef> resultBuf1 = resultObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "buf1"));
    ASSERT_TRUE(resultBuf1->IsArrayBuffer(ecmaVm));
    EXPECT_EQ(Local<ArrayBufferRef>(resultBuf1)->ByteLength(ecmaVm), 8);

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, TransferCloneListMutualExclusion_ReturnsNull)
{
    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    Local<ArrayBufferRef> ab = ArrayBufferRef::New(ecmaVm, 8);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "buf"), ab);

    // transfer 和 cloneList 都包含同一个 ArrayBuffer
    Local<ArrayRef> transferArr = ArrayRef::New(ecmaVm, 1);
    ArrayRef::SetValueAt(ecmaVm, transferArr, 0, ab);
    Local<ArrayRef> cloneListArr = ArrayRef::New(ecmaVm, 1);
    ArrayRef::SetValueAt(ecmaVm, cloneListArr, 0, ab);

    size_t outSize = 0;
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, obj, transferArr, cloneListArr,
                                          false, false, false, outSize);
    // 互斥冲突：返回 nullptr，outSize 为 0
    EXPECT_EQ(buf, nullptr);
    EXPECT_EQ(outSize, static_cast<size_t>(0));

    // ArrayBuffer 未被 detach（序列化失败不应有副作用）
    EXPECT_EQ(ab->ByteLength(ecmaVm), 8);
}

HWTEST_F_L0(FlatBufferSerializeTest, TruncatedBuffer_DeserializeReturnsUndefinedOrNoCrash)
{
    // 先正常序列化获取合法缓冲区
    Local<StringRef> value = StringRef::NewFromUtf8(ecmaVm, "hello");
    size_t outSize = 0;
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *fullBuf = JSNApi::SerializeValue(ecmaVm, value, transfer, cloneList,
                                              false, true, false, outSize);
    ASSERT_NE(fullBuf, nullptr);
    EXPECT_GT(outSize, static_cast<size_t>(4));  // 确保足够长以便截断

    // 创建截断缓冲区：只复制前 4 字节
    size_t truncatedSize = 4;
    uint8_t *truncatedBuf = static_cast<uint8_t *>(malloc(truncatedSize));
    ASSERT_EQ(memcpy_s(truncatedBuf, truncatedSize, fullBuf, truncatedSize), EOK);
    free(fullBuf);

    // 反序列化截断缓冲区：应返回 Undefined 且不崩溃
    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, truncatedBuf, nullptr);
    // 预期行为：返回 Undefined（无法从截断数据重建有效值）
    EXPECT_TRUE(result->IsUndefined());

    free(truncatedBuf);
}

// ===================== P1: Important Features/Error Paths =====================

HWTEST_F_L0(FlatBufferSerializeTest, UndefinedOriginalValue_AmbiguityRoundTrip)
{
    Local<JSValueRef> undefValue(JSValueRef::Undefined(ecmaVm));

    size_t outSize = 0;
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, undefValue, transfer, cloneList,
                                          false, true, false, outSize);
    // undefined 可正常序列化
    ASSERT_NE(buf, nullptr);
    EXPECT_GT(outSize, static_cast<size_t>(0));

    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    // 反序列化返回 Undefined（与错误场景返回相同，存在歧义）
    EXPECT_TRUE(result->IsUndefined());

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, EmptyTransferArray_NoArrayBufferTransferred)
{
    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    Local<ArrayBufferRef> ab = ArrayBufferRef::New(ecmaVm, 8);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "buf"), ab);

    // transfer 为空数组
    Local<ArrayRef> transferArr = ArrayRef::New(ecmaVm, 0);
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));

    size_t outSize = 0;
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, obj, transferArr, cloneList,
                                          false, true, false, outSize);
    ASSERT_NE(buf, nullptr);

    // 空 transfer 数组：ArrayBuffer 未被转移（按值克隆）
    EXPECT_EQ(ab->ByteLength(ecmaVm), 8);

    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    ASSERT_TRUE(result->IsObject(ecmaVm));
    Local<ObjectRef> resultObj = result->ToObject(ecmaVm);
    Local<JSValueRef> resultBuf = resultObj->Get(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "buf"));
    ASSERT_TRUE(resultBuf->IsArrayBuffer(ecmaVm));
    EXPECT_EQ(Local<ArrayBufferRef>(resultBuf)->ByteLength(ecmaVm), 8);

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, EmptyCloneListArray_NoEffect)
{
    Local<StringRef> value = StringRef::NewFromUtf8(ecmaVm, "test");
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<ArrayRef> cloneListArr = ArrayRef::New(ecmaVm, 0);

    size_t outSize = 0;
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, value, transfer, cloneListArr,
                                          false, true, false, outSize);
    ASSERT_NE(buf, nullptr);

    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    ASSERT_TRUE(result->IsString(ecmaVm));
    EXPECT_EQ(Local<StringRef>(result)->ToString(ecmaVm), "test");

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, DefaultTransferTrue_MultipleArrayBuffer_AllDetached)
{
    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    Local<ArrayBufferRef> ab1 = ArrayBufferRef::New(ecmaVm, 8);
    Local<ArrayBufferRef> ab2 = ArrayBufferRef::New(ecmaVm, 16);
    Local<ArrayBufferRef> ab3 = ArrayBufferRef::New(ecmaVm, 32);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "a"), ab1);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "b"), ab2);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "c"), ab3);

    size_t outSize = 0;
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, obj, transfer, cloneList,
                                          true,   /* defaultTransfer=true */
                                          true, false, outSize);
    ASSERT_NE(buf, nullptr);

    // 默认转移语义下所有 ArrayBuffer 都应被 detach
    EXPECT_EQ(ab1->ByteLength(ecmaVm), 0);
    EXPECT_EQ(ab2->ByteLength(ecmaVm), 0);
    EXPECT_EQ(ab3->ByteLength(ecmaVm), 0);

    // 反序列化验证所有 ArrayBuffer 重建完整
    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    ASSERT_TRUE(result->IsObject(ecmaVm));
    Local<ObjectRef> resultObj = result->ToObject(ecmaVm);
    Local<JSValueRef> ra = resultObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "a"));
    Local<JSValueRef> rb = resultObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "b"));
    Local<JSValueRef> rc = resultObj->Get(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "c"));
    ASSERT_TRUE(ra->IsArrayBuffer(ecmaVm));
    ASSERT_TRUE(rb->IsArrayBuffer(ecmaVm));
    ASSERT_TRUE(rc->IsArrayBuffer(ecmaVm));
    EXPECT_EQ(Local<ArrayBufferRef>(ra)->ByteLength(ecmaVm), 8);
    EXPECT_EQ(Local<ArrayBufferRef>(rb)->ByteLength(ecmaVm), 16);
    EXPECT_EQ(Local<ArrayBufferRef>(rc)->ByteLength(ecmaVm), 32);

    free(buf);
}

// ===================== P2: Robustness/Boundary/Compatibility =====================

HWTEST_F_L0(FlatBufferSerializeTest, DeserializeAfterBufferFreed_NoUseAfterFree)
{
    Local<StringRef> value = StringRef::NewFromUtf8(ecmaVm, "independent");
    size_t outSize = 0;
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, value, transfer, cloneList,
                                          false, true, false, outSize);
    ASSERT_NE(buf, nullptr);

    // 复制缓冲区内容
    uint8_t *copy = static_cast<uint8_t *>(malloc(outSize));
    ASSERT_EQ(memcpy_s(copy, outSize, buf, outSize), EOK);
    free(buf);  // 释放原始缓冲区

    // 从副本反序列化
    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, copy, nullptr);
    ASSERT_TRUE(result->IsString(ecmaVm));
    EXPECT_EQ(Local<StringRef>(result)->ToString(ecmaVm), "independent");

    free(copy);
}

HWTEST_F_L0(FlatBufferSerializeTest, TransferArrayBuffer_OriginalDetachedAndReconstructed)
{
    Local<ArrayBufferRef> ab = ArrayBufferRef::New(ecmaVm, 8);
    // 写入测试数据
    uint8_t *data = static_cast<uint8_t*>(ab->GetBuffer(ecmaVm));
    for (int i = 0; i < 8; i++) {
        data[i] = static_cast<uint8_t>(0xA0 + i);
    }

    Local<ArrayRef> transferArr = ArrayRef::New(ecmaVm, 1);
    ArrayRef::SetValueAt(ecmaVm, transferArr, 0, ab);

    size_t outSize = 0;
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, ab, transferArr, cloneList,
                                          false, true, false, outSize);
    ASSERT_NE(buf, nullptr);

    // ArrayBuffer 被转移（detach）
    EXPECT_EQ(ab->ByteLength(ecmaVm), 0);

    // 反序列化验证：重建的 ArrayBuffer 包含原始数据
    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    ASSERT_TRUE(result->IsArrayBuffer(ecmaVm));
    Local<ArrayBufferRef> resultAb = Local<ArrayBufferRef>(result);
    EXPECT_EQ(resultAb->ByteLength(ecmaVm), 8);
    uint8_t *resultData = static_cast<uint8_t*>(resultAb->GetBuffer(ecmaVm));
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(resultData[i], static_cast<uint8_t>(0xA0 + i));
    }

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, DefaultCloneSharedFalse_WithoutCloneList)
{
    Local<StringRef> value = StringRef::NewFromUtf8(ecmaVm, "test_no_clone_shared");
    size_t outSize = 0;
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, value, transfer, cloneList,
                                          false,
                                          false,  /* defaultCloneShared=false */
                                          false, outSize);
    // 无共享对象时 defaultCloneShared=false 应仍能正常序列化
    ASSERT_NE(buf, nullptr);
    EXPECT_GT(outSize, static_cast<size_t>(0));

    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    ASSERT_TRUE(result->IsString(ecmaVm));
    EXPECT_EQ(Local<StringRef>(result)->ToString(ecmaVm), "test_no_clone_shared");

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, ZeroSizeArrayBuffer_SerializeAndDeserialize)
{
    Local<ArrayBufferRef> ab = ArrayBufferRef::New(ecmaVm, 0);  // 零长度
    size_t outSize = 0;
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *buf = JSNApi::SerializeValue(ecmaVm, ab, transfer, cloneList,
                                          false, true, false, outSize);
    ASSERT_NE(buf, nullptr);

    Local<JSValueRef> result = JSNApi::DeserializeValue(ecmaVm, buf, nullptr);
    ASSERT_TRUE(result->IsArrayBuffer(ecmaVm));
    EXPECT_EQ(Local<ArrayBufferRef>(result)->ByteLength(ecmaVm), 0);

    free(buf);
}

HWTEST_F_L0(FlatBufferSerializeTest, MultipleRoundTrips_DataConsistency)
{
    Local<ObjectRef> obj = ObjectRef::New(ecmaVm);
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "x"),
             NumberRef::New(ecmaVm, 42));
    obj->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "y"),
             StringRef::NewFromUtf8(ecmaVm, "hello"));

    // 第一次 round-trip
    size_t size1 = 0;
    Local<JSValueRef> transfer(JSValueRef::Undefined(ecmaVm));
    Local<JSValueRef> cloneList(JSValueRef::Undefined(ecmaVm));
    uint8_t *buf1 = JSNApi::SerializeValue(ecmaVm, obj, transfer, cloneList,
                                           false, true, false, size1);
    ASSERT_NE(buf1, nullptr);
    Local<JSValueRef> result1 = JSNApi::DeserializeValue(ecmaVm, buf1, nullptr);
    free(buf1);

    // 第二次 round-trip（对第一次反序列化结果再序列化）
    size_t size2 = 0;
    uint8_t *buf2 = JSNApi::SerializeValue(ecmaVm, result1, transfer, cloneList,
                                           false, true, false, size2);
    ASSERT_NE(buf2, nullptr);
    Local<JSValueRef> result2 = JSNApi::DeserializeValue(ecmaVm, buf2, nullptr);
    free(buf2);

    // 验证两次 round-trip 结果一致
    ASSERT_TRUE(result2->IsObject(ecmaVm));
    Local<ObjectRef> resultObj2 = result2->ToObject(ecmaVm);
    Local<JSValueRef> x2 = resultObj2->Get(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "x"));
    Local<JSValueRef> y2 = resultObj2->Get(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "y"));
    EXPECT_TRUE(x2->IsNumber());
    EXPECT_EQ(x2->Int32Value(ecmaVm), 42);
    EXPECT_TRUE(y2->IsString(ecmaVm));
}

}  // namespace panda::test
