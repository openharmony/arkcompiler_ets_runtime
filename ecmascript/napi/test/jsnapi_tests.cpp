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

#include <cstddef>

#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/js_generator_object.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
using BuiltinsFunction = ecmascript::builtins::BuiltinsFunction;
class JSNApiTests : public testing::Test {
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
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        vm_ = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
        thread_ = vm_->GetJSThread();
        vm_->SetEnableForceGC(true);
    }

    void TearDown() override
    {
        vm_->SetEnableForceGC(false);
        JSNApi::DestroyJSVM(vm_);
    }

protected:
    JSThread *thread_ = nullptr;
    EcmaVM *vm_ = nullptr;
};

Local<JSValueRef> FunctionCallback(JsiRuntimeCallInfo* info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

void WeakRefCallback(EcmaVM* vm)
{
    LocalScope scope(vm);
    Local<ObjectRef> object = ObjectRef::New(vm);
    Global<ObjectRef> globalObject(vm, object);
    globalObject.SetWeak();
    Local<ObjectRef> object1 = ObjectRef::New(vm);
    Global<ObjectRef> globalObject1(vm, object1);
    globalObject1.SetWeak();
    vm->CollectGarbage(TriggerGCType::YOUNG_GC);
    vm->CollectGarbage(TriggerGCType::OLD_GC);
    globalObject.FreeGlobalHandleAddr();
}

void ThreadCheck(const EcmaVM *vm)
{
    EXPECT_TRUE(vm->GetJSThread()->GetThreadId() != JSThread::GetCurrentThreadId());
}

HWTEST_F_L0(JSNApiTests, GetGlobalObject)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject());
}

HWTEST_F_L0(JSNApiTests, ThreadIdCheck)
{
    EXPECT_TRUE(vm_->GetJSThread()->GetThreadId() == JSThread::GetCurrentThreadId());
}

HWTEST_F_L0(JSNApiTests, RegisterFunction)
{
    LocalScope scope(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback);
    ASSERT_TRUE(!callback.IsEmpty());
    std::vector<Local<JSValueRef>> arguments;
    arguments.emplace_back(JSValueRef::Undefined(vm_));
    Local<JSValueRef> result =
        callback->Call(vm_, JSValueRef::Undefined(vm_), arguments.data(), arguments.size());
    ASSERT_TRUE(result->IsArray(vm_));
    Local<ArrayRef> array(result);
    ASSERT_EQ(static_cast<uint64_t>(array->Length(vm_)), arguments.size());
}

HWTEST_F_L0(JSNApiTests, GetProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject());

    Local<ObjectRef> key = StringRef::NewFromUtf8(vm_, "Number");
    Local<ObjectRef> property = globalObject->Get(vm_, key);
    ASSERT_TRUE(property->IsFunction());
}

HWTEST_F_L0(JSNApiTests, SetProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject());

    Local<ArrayRef> property = ArrayRef::New(vm_, 3); // 3 : length
    ASSERT_TRUE(property->IsArray(vm_));
    ASSERT_EQ(property->Length(vm_), 3); // 3 : test case of input

    Local<ObjectRef> key = StringRef::NewFromUtf8(vm_, "Test");
    bool result = globalObject->Set(vm_, key, property);
    ASSERT_TRUE(result);

    Local<ObjectRef> propertyGet = globalObject->Get(vm_, key);
    ASSERT_TRUE(propertyGet->IsArray(vm_));
    ASSERT_EQ(Local<ArrayRef>(propertyGet)->Length(vm_), 3); // 3 : test case of input
}

HWTEST_F_L0(JSNApiTests, JsonParser)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject());

    const char *const test{R"({"orientation": "portrait"})"};
    Local<ObjectRef> jsonString = StringRef::NewFromUtf8(vm_, test);

    Local<JSValueRef> result = JSON::Parse(vm_, jsonString);
    ASSERT_TRUE(result->IsObject());

    Local<ObjectRef> keyString = StringRef::NewFromUtf8(vm_, "orientation");
    Local<JSValueRef> property = Local<ObjectRef>(result)->Get(vm_, keyString);
    ASSERT_TRUE(property->IsString());
}

HWTEST_F_L0(JSNApiTests, StrictEqual)
{
    LocalScope scope(vm_);
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    Local<StringRef> target1 = StringRef::NewFromUtf8(vm_, "1");
    Local<NumberRef> target = NumberRef::New(vm_, 1);

    ASSERT_FALSE(origin->IsStrictEquals(vm_, target));
    ASSERT_TRUE(origin->IsStrictEquals(vm_, target1));
}

HWTEST_F_L0(JSNApiTests, InstanceOf)
{
    LocalScope scope(vm_);
    Local<FunctionRef> target = FunctionRef::New(vm_, nullptr);
    Local<ArrayRef> origin = ArrayRef::New(vm_, 1);

    ASSERT_FALSE(origin->InstanceOf(vm_, target));
}

HWTEST_F_L0(JSNApiTests, TypeOf)
{
    LocalScope scope(vm_);
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    Local<StringRef> typeString = origin->Typeof(vm_);
    ASSERT_EQ(typeString->ToString(), "string");

    Local<NumberRef> target = NumberRef::New(vm_, 1);
    typeString = target->Typeof(vm_);
    ASSERT_EQ(typeString->ToString(), "number");
}

HWTEST_F_L0(JSNApiTests, Symbol)
{
    LocalScope scope(vm_);
    Local<StringRef> description = StringRef::NewFromUtf8(vm_, "test");
    Local<SymbolRef> symbol = SymbolRef::New(vm_, description);

    ASSERT_FALSE(description->IsSymbol());
    ASSERT_TRUE(symbol->IsSymbol());
}

HWTEST_F_L0(JSNApiTests, StringUtf8_001)
{
    LocalScope scope(vm_);
    std::string test = "Hello world";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Utf8Length(vm_), 12);          // 12 : length of testString("Hello World")
    char buffer[12];                                      // 12 : length of testString
    EXPECT_EQ(testString->WriteUtf8(buffer, 12), 12); // 12 : length of testString("Hello World")
    std::string res(buffer);
    ASSERT_EQ(res, test);
}

HWTEST_F_L0(JSNApiTests, StringUtf8_002)
{
    LocalScope scope(vm_);
    std::string test = "年";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Utf8Length(vm_), 4);          // 4 : length of testString("年")
    char buffer[4];                                      // 4 : length of testString
    EXPECT_EQ(testString->WriteUtf8(buffer, 4), 4); // 4 : length of testString("年")
    std::string res(buffer);
    ASSERT_EQ(res, test);
}

HWTEST_F_L0(JSNApiTests, StringUtf8_003)
{
    LocalScope scope(vm_);
    std::string str1 = "a";
    std::string str2 = "b";
    std::string test = str1 + '\0' + str2;

    // isWriteBuffer == false, \u0000 ==> C080
    Local<StringRef> testString1 = StringRef::NewFromUtf8(vm_, test.c_str(), test.length());
    EXPECT_EQ(testString1->Utf8Length(vm_), 5);
    char buffer1[4];
    testString1->WriteUtf8(buffer1, 4, false);
    EXPECT_EQ(buffer1[0], 'a');
    EXPECT_EQ(buffer1[1], '\xC0');
    EXPECT_EQ(buffer1[2], '\x80');
    EXPECT_EQ(buffer1[3], 'b');

    // isWriteBuffer == true, \u0000 ==> 0x00U
    Local<StringRef> testString2 = StringRef::NewFromUtf8(vm_, test.c_str(), test.length());
    EXPECT_EQ(testString2->Utf8Length(vm_), 5);
    char buffer2[4];
    testString2->WriteUtf8(buffer2, 4, true);
    EXPECT_EQ(buffer2[0], 'a');
    EXPECT_EQ(buffer2[1], '\0');
    EXPECT_EQ(buffer2[2], 'b');
}

HWTEST_F_L0(JSNApiTests, StringLatin1_001)
{
    LocalScope scope(vm_);
    std::string test = "中";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Length(), 1);
    char buffer[1];
    EXPECT_EQ(testString->WriteLatin1(buffer, 1), 1);

    EXPECT_EQ(buffer[0], '-'); // '-' == 0x2D
}

HWTEST_F_L0(JSNApiTests, StringLatin1_002)
{
    LocalScope scope(vm_);
    std::string test = "En123";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Length(), 5);
    char buffer[5];
    EXPECT_EQ(testString->WriteLatin1(buffer, 5), 5);

    EXPECT_EQ(buffer[0], 'E');
    EXPECT_EQ(buffer[1], 'n');
    EXPECT_EQ(buffer[2], '1');
    EXPECT_EQ(buffer[3], '2');
    EXPECT_EQ(buffer[4], '3');
}

HWTEST_F_L0(JSNApiTests, ToType)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);

    ASSERT_EQ(toString->ToNumber(vm_)->Value(), -123.3); // -123 : test case of input
    ASSERT_EQ(toString->ToBoolean(vm_)->Value(), true);
    ASSERT_EQ(toValue->ToString(vm_)->ToString(), "-123.3");
    ASSERT_TRUE(toValue->ToObject(vm_)->IsObject());
}

HWTEST_F_L0(JSNApiTests, TypeValue)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123");
    Local<JSValueRef> toValue(toString);

    ASSERT_EQ(toString->Int32Value(vm_), -123);        // -123 : test case of input
    ASSERT_EQ(toString->BooleaValue(), true);
    ASSERT_EQ(toString->Uint32Value(vm_), 4294967173U); // 4294967173 : test case of input
    ASSERT_EQ(toString->IntegerValue(vm_), -123);      // -123 : test case of input
}

void* detach()
{
    GTEST_LOG_(INFO) << "detach is running";
    return nullptr;
}

void attach([[maybe_unused]] void* buffer)
{
    GTEST_LOG_(INFO) << "attach is running";
}

HWTEST_F_L0(JSNApiTests, CreateNativeObject)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_, reinterpret_cast<void*>(detach), reinterpret_cast<void*>(attach));
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    Local<JSValueRef> value1 = object->Get(vm_, key);
    ASSERT_TRUE(value->IsStrictEquals(vm_, value1));
    ASSERT_TRUE(object->Has(vm_, key));
    ASSERT_TRUE(object->Delete(vm_, key));
    ASSERT_FALSE(object->Has(vm_, key));
}

HWTEST_F_L0(JSNApiTests, DefineProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    Local<JSValueRef> value1 = object->Get(vm_, key);
    ASSERT_TRUE(value->IsStrictEquals(vm_, value1));
}

HWTEST_F_L0(JSNApiTests, HasProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    ASSERT_TRUE(object->Has(vm_, key));
}

HWTEST_F_L0(JSNApiTests, DeleteProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    ASSERT_TRUE(object->Delete(vm_, key));
    ASSERT_FALSE(object->Has(vm_, key));
}

HWTEST_F_L0(JSNApiTests, GetProtoType)
{
    LocalScope scope(vm_);
    Local<FunctionRef> function = FunctionRef::New(vm_, nullptr);
    Local<JSValueRef> protoType = function->GetPrototype(vm_);
    ASSERT_TRUE(protoType->IsObject());

    Local<FunctionRef> object = ObjectRef::New(vm_);
    protoType = object->GetPrototype(vm_);
    ASSERT_TRUE(protoType->IsObject());

    Local<FunctionRef> native = ObjectRef::New(vm_, reinterpret_cast<void*>(detach), reinterpret_cast<void*>(attach));
    protoType = native->GetPrototype(vm_);
    ASSERT_TRUE(protoType->IsObject());
}

void CheckReject(JsiRuntimeCallInfo* info)
{
    ASSERT_EQ(info->GetArgsNumber(), 1U);
    Local<JSValueRef> reason = info->GetCallArgRef(0);
    ASSERT_TRUE(reason->IsString());
    ASSERT_EQ(Local<StringRef>(reason)->ToString(), "Reject");
}

Local<JSValueRef> RejectCallback(JsiRuntimeCallInfo* info)
{
    LocalScope scope(info->GetVM());
    CheckReject(info);
    return JSValueRef::Undefined(info->GetVM());
}

HWTEST_F_L0(JSNApiTests, PromiseCatch)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> reject = FunctionRef::New(vm_, RejectCallback);
    Local<PromiseRef> catchPromise = promise->Catch(vm_, reject);
    ASSERT_TRUE(promise->IsPromise());
    ASSERT_TRUE(catchPromise->IsPromise());

    Local<StringRef> reason = StringRef::NewFromUtf8(vm_, "Reject");
    ASSERT_TRUE(capability->Reject(vm_, reason));

    vm_->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
}

void CheckResolve(JsiRuntimeCallInfo* info)
{
    ASSERT_EQ(info->GetArgsNumber(), 1U);
    Local<JSValueRef> value = info->GetCallArgRef(0);
    ASSERT_TRUE(value->IsNumber());
    ASSERT_EQ(Local<NumberRef>(value)->Value(), 300.3); // 300.3 : test case of input
}

Local<JSValueRef> ResolvedCallback(JsiRuntimeCallInfo* info)
{
    LocalScope scope(info->GetVM());
    CheckResolve(info);
    return JSValueRef::Undefined(info->GetVM());
}

HWTEST_F_L0(JSNApiTests, PromiseThen)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> resolve = FunctionRef::New(vm_, ResolvedCallback);
    Local<FunctionRef> reject = FunctionRef::New(vm_, RejectCallback);
    Local<PromiseRef> thenPromise = promise->Then(vm_, resolve, reject);
    ASSERT_TRUE(promise->IsPromise());
    ASSERT_TRUE(thenPromise->IsPromise());

    Local<StringRef> value = NumberRef::New(vm_, 300.3); // 300.3 : test case of input
    ASSERT_TRUE(capability->Resolve(vm_, value));
    vm_->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
}

HWTEST_F_L0(JSNApiTests, Constructor)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = JSNApi::GetGlobalObject(vm_);
    Local<StringRef> key = StringRef::NewFromUtf8(vm_, "Number");
    Local<FunctionRef> numberConstructor = object->Get(vm_, key);
    Local<JSValueRef> argv[1];
    argv[0] = NumberRef::New(vm_, 1.3); // 1.3 : test case of input
    Local<JSValueRef> result = numberConstructor->Constructor(vm_, argv, 1);
    ASSERT_TRUE(result->IsObject());
    ASSERT_EQ(result->ToNumber(vm_)->Value(), 1.3); // 1.3 : size of arguments
}

HWTEST_F_L0(JSNApiTests, ArrayBuffer)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());
    ASSERT_EQ(arrayBuffer->ByteLength(vm_), length);
    ASSERT_NE(arrayBuffer->GetBuffer(), nullptr);
    JSNApi::TriggerGC(vm_);
}

HWTEST_F_L0(JSNApiTests, ArrayBufferWithBuffer)
{
    static bool isFree = false;
    struct Data {
        int32_t length;
    };
    const int32_t length = 15;
    Data *data = new Data();
    data->length = length;
    Deleter deleter = [](void *buffer, void *data) -> void {
        delete[] reinterpret_cast<uint8_t *>(buffer);
        Data *currentData = reinterpret_cast<Data *>(data);
        ASSERT_EQ(currentData->length, 15); // 5 : size of arguments
        delete currentData;
        isFree = true;
    };
    {
        LocalScope scope(vm_);
        uint8_t *buffer = new uint8_t[length]();
        Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, buffer, length, deleter, data);
        ASSERT_TRUE(arrayBuffer->IsArrayBuffer());
        ASSERT_EQ(arrayBuffer->ByteLength(vm_), length);
        ASSERT_EQ(arrayBuffer->GetBuffer(), buffer);
    }
    JSNApi::TriggerGC(vm_, JSNApi::TRIGGER_GC_TYPE::FULL_GC);
    ASSERT_TRUE(isFree);
}

HWTEST_F_L0(JSNApiTests, DataView)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    JSNApi::TriggerGC(vm_);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 5 : offset of byte, 7 : length
    Local<DataViewRef> dataView = DataViewRef::New(vm_, arrayBuffer, 5, 7);
    ASSERT_TRUE(dataView->IsDataView());
    ASSERT_EQ(dataView->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
    ASSERT_EQ(dataView->ByteLength(), 7U); // 7 : size of arguments
    ASSERT_EQ(dataView->ByteOffset(), 5U); // 5 : size of arguments

    // 5 : offset of byte, 11 : length
    dataView = DataViewRef::New(vm_, arrayBuffer, 5, 11);
    ASSERT_TRUE(dataView->IsUndefined());
}

HWTEST_F_L0(JSNApiTests, Int8Array)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 5 : offset of byte, 6 : length
    Local<Int8ArrayRef> typedArray = Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsInt8Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 6U); // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5U); // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Uint8Array)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 5 : offset of byte, 6 : length
    Local<Uint8ArrayRef> typedArray = Uint8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsUint8Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 6U);  // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5U);  // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Uint8ClampedArray)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 5 : offset of byte, 6 : length
    Local<Uint8ClampedArrayRef> typedArray = Uint8ClampedArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsUint8ClampedArray());
    ASSERT_EQ(typedArray->ByteLength(vm_), 6U);  // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5U);  // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Int16Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Int16ArrayRef> typedArray = Int16ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsInt16Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 12U);  // 12 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Uint16Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Uint16ArrayRef> typedArray = Uint16ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsUint16Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 12U);  // 12 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Uint32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsUint32Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 24U);  // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Int32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Int32ArrayRef> typedArray = Int32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsInt32Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 24U);  // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Float32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Float32ArrayRef> typedArray = Float32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsFloat32Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 24U);  // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Float64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 57;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 8 : offset of byte, 6 : length
    Local<Float64ArrayRef> typedArray = Float64ArrayRef::New(vm_, arrayBuffer, 8, 6);
    ASSERT_TRUE(typedArray->IsFloat64Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U);  // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);   // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, BigInt64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 57;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 8 : offset of byte, 6 : length
    Local<BigInt64ArrayRef> typedArray = BigInt64ArrayRef::New(vm_, arrayBuffer, 8, 6);
    ASSERT_TRUE(typedArray->IsBigInt64Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U);  // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);   // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, BigUint64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 57;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 8 : offset of byte, 6 : length
    Local<BigUint64ArrayRef> typedArray = BigUint64ArrayRef::New(vm_, arrayBuffer, 8, 6);
    ASSERT_TRUE(typedArray->IsBigUint64Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U);  // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);   // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Error)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, RangeError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::RangeError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, TypeError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::TypeError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, ReferenceError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::ReferenceError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, SyntaxError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::SyntaxError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, OOMError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::OOMError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_001)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    // new with Builtins::Set Prototype
    JSHandle<JSTaggedValue> set = env->GetBuiltinsSetFunction();
    Local<FunctionRef> setLocal = JSNApiHelper::ToLocal<FunctionRef>(set);
    // new with Builtins::Map Prototype
    JSHandle<JSTaggedValue> map = env->GetBuiltinsMapFunction();
    Local<FunctionRef> mapLocal = JSNApiHelper::ToLocal<FunctionRef>(map);
    JSHandle<JSTaggedValue> setPrototype(thread_, JSHandle<JSFunction>::Cast(set)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> mapPrototype(thread_, JSHandle<JSFunction>::Cast(map)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> mapPrototypeProto(thread_, JSTaggedValue::GetPrototype(thread_, mapPrototype));
    bool same = JSTaggedValue::SameValue(setPrototype, mapPrototypeProto);
    // before inherit, map.Prototype.__proto__ should be different from set.Prototype
    ASSERT_FALSE(same);
    // before inherit, map.__proto__ should be different from set
    JSHandle<JSTaggedValue> mapProto(thread_, JSTaggedValue::GetPrototype(thread_, map));
    bool same1 = JSTaggedValue::SameValue(set, mapProto);
    ASSERT_FALSE(same1);

    // Set property to Set Function
    auto factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> defaultString = thread_->GlobalConstants()->GetHandledDefaultString();
    PropertyDescriptor desc = PropertyDescriptor(thread_, defaultString);
    bool success = JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(set), defaultString, desc);
    ASSERT_TRUE(success);
    JSHandle<JSTaggedValue> property1String(thread_, factory->NewFromASCII("property1").GetTaggedValue());
    JSHandle<JSTaggedValue> func = env->GetTypedArrayFunction();
    PropertyDescriptor desc1 = PropertyDescriptor(thread_, func);
    bool success1 = JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(set), property1String, desc1);
    ASSERT_TRUE(success1);

    mapLocal->Inherit(vm_, setLocal);
    JSHandle<JSTaggedValue> sonHandle = JSNApiHelper::ToJSHandle(mapLocal);
    JSHandle<JSTaggedValue> sonPrototype(thread_, JSHandle<JSFunction>::Cast(sonHandle)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> sonPrototypeProto(thread_, JSTaggedValue::GetPrototype(thread_, sonPrototype));
    bool same2 = JSTaggedValue::SameValue(setPrototype, sonPrototypeProto);
    ASSERT_TRUE(same2);
    JSHandle<JSTaggedValue> sonProto(thread_, JSTaggedValue::GetPrototype(thread_, map));
    bool same3 = JSTaggedValue::SameValue(set, sonProto);
    ASSERT_TRUE(same3);

    // son = new Son(), Son() inherit from Parent(), Test whether son.InstanceOf(Parent) is true
    JSHandle<JSObject> sonObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(sonHandle), sonHandle);
    bool isInstance = JSObject::InstanceOf(thread_, JSHandle<JSTaggedValue>::Cast(sonObj), set);
    ASSERT_TRUE(isInstance);

    // Test whether son Function can access to property of parent Function
    OperationResult res = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(sonHandle), defaultString);
    bool same4 = JSTaggedValue::SameValue(defaultString, res.GetValue());
    ASSERT_TRUE(same4);
    OperationResult res1 = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(sonHandle), property1String);
    bool same5 = JSTaggedValue::SameValue(func, res1.GetValue());
    ASSERT_TRUE(same5);

    // new with empty Function Constructor
    Local<FunctionRef> son1 = FunctionRef::New(vm_, FunctionCallback, nullptr);
    son1->Inherit(vm_, mapLocal);
    JSHandle<JSFunction> son1Handle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(son1));
    ASSERT_TRUE(son1Handle->HasFunctionPrototype());
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_002)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    // new with Builtins::weakSet Prototype
    JSHandle<JSTaggedValue> weakSet = env->GetBuiltinsWeakSetFunction();
    Local<FunctionRef> weakSetLocal = JSNApiHelper::ToLocal<FunctionRef>(weakSet);
    // new with Builtins::weakMap Prototype
    JSHandle<JSTaggedValue> weakMap = env->GetBuiltinsWeakMapFunction();
    Local<FunctionRef> weakMapLocal = JSNApiHelper::ToLocal<FunctionRef>(weakMap);

    weakMapLocal->Inherit(vm_, weakSetLocal);

    auto factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> property1String(thread_, factory->NewFromASCII("property1").GetTaggedValue());
    JSHandle<JSTaggedValue> func = env->GetArrayFunction();
    PropertyDescriptor desc1 = PropertyDescriptor(thread_, func);
    bool success1 = JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(weakMap), property1String, desc1);
    ASSERT_TRUE(success1);

    JSHandle<JSTaggedValue> sonHandle = JSNApiHelper::ToJSHandle(weakMapLocal);
    JSHandle<JSObject> sonObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(sonHandle), sonHandle);

    JSHandle<JSTaggedValue> fatherHandle = JSNApiHelper::ToJSHandle(weakSetLocal);
    JSHandle<JSObject> fatherObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(fatherHandle), fatherHandle);

    JSHandle<JSTaggedValue> sonMethod =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>(sonObj), property1String);
    JSHandle<JSTaggedValue> fatherMethod =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>(fatherObj), property1String);
    bool same = JSTaggedValue::SameValue(sonMethod, fatherMethod);
    ASSERT_TRUE(same);
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_003)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    auto factory = vm_->GetFactory();

    JSHandle<Method> invokeSelf =
        factory->NewMethodForNativeFunction(reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf));
    // father type
    JSHandle<JSHClass> protoClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    JSHandle<JSFunction> protoFunc = factory->NewJSFunctionByHClass(invokeSelf, protoClass);
    Local<FunctionRef> protoLocal = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(protoFunc));
    // son type
    JSHandle<JSHClass> noProtoClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSFunction> noProtoFunc = factory->NewJSFunctionByHClass(invokeSelf, noProtoClass);
    Local<FunctionRef> noProtoLocal = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(noProtoFunc));

    JSHandle<JSFunction> sonHandle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(noProtoLocal));
    EXPECT_FALSE(sonHandle->HasFunctionPrototype());

    JSHandle<JSTaggedValue> defaultString = thread_->GlobalConstants()->GetHandledDefaultString();
    PropertyDescriptor desc = PropertyDescriptor(thread_, defaultString);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(protoFunc), defaultString, desc);

    noProtoLocal->Inherit(vm_, protoLocal);
    JSHandle<JSFunction> son1Handle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(noProtoLocal));
    EXPECT_TRUE(son1Handle->HasFunctionPrototype());

    OperationResult res = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(son1Handle), defaultString);
    EXPECT_EQ(JSTaggedValue::SameValue(defaultString, res.GetValue()), true);

    JSHandle<JSTaggedValue> propertyString(thread_, factory->NewFromASCII("property").GetTaggedValue());
    JSHandle<JSTaggedValue> func = env->GetArrayFunction();
    PropertyDescriptor desc1 = PropertyDescriptor(thread_, func);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(protoFunc), propertyString, desc1);
    OperationResult res1 = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(son1Handle), propertyString);
    EXPECT_EQ(JSTaggedValue::SameValue(func, res1.GetValue()), true);
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_004)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    auto factory = vm_->GetFactory();

    JSHandle<JSTaggedValue> weakSet = env->GetBuiltinsWeakSetFunction();
    JSHandle<JSTaggedValue> deleteString(factory->NewFromASCII("delete"));
    JSHandle<JSTaggedValue> addString(factory->NewFromASCII("add"));
    JSHandle<JSTaggedValue> defaultString = thread_->GlobalConstants()->GetHandledDefaultString();
    JSHandle<JSTaggedValue> deleteMethod = JSObject::GetMethod(thread_, weakSet, deleteString);
    JSHandle<JSTaggedValue> addMethod = JSObject::GetMethod(thread_, weakSet, addString);

    JSHandle<Method> invokeSelf =
        factory->NewMethodForNativeFunction(reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf));
    JSHandle<Method> ctor =
        factory->NewMethodForNativeFunction(reinterpret_cast<void *>(BuiltinsFunction::FunctionConstructor));

    JSHandle<JSHClass> protoClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    JSHandle<JSFunction> funcFuncPrototype = factory->NewJSFunctionByHClass(invokeSelf, protoClass);
    // add method in funcPrototype
    PropertyDescriptor desc = PropertyDescriptor(thread_, deleteMethod);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(funcFuncPrototype), deleteString, desc);
    JSHandle<JSTaggedValue> funcFuncPrototypeValue(funcFuncPrototype);

    JSHandle<JSHClass> funcFuncProtoIntanceClass =
       factory->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, funcFuncPrototypeValue);
    // new with NewJSFunctionByHClass::function Class
    JSHandle<JSFunction> protoFunc =
       factory->NewJSFunctionByHClass(ctor, funcFuncProtoIntanceClass);
    EXPECT_TRUE(*protoFunc != nullptr);
    // add method in funcnction
    PropertyDescriptor desc1 = PropertyDescriptor(thread_, addMethod);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(protoFunc), addString, desc1);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(protoFunc), deleteString, desc);
    // father type
    Local<FunctionRef> protoLocal = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(protoFunc));

    JSHandle<JSHClass> noProtoClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSFunction> funcFuncNoProtoPrototype = factory->NewJSFunctionByHClass(invokeSelf, noProtoClass);
    JSHandle<JSTaggedValue> funcFuncNoProtoPrototypeValue(funcFuncNoProtoPrototype);

    JSHandle<JSHClass> funcFuncNoProtoProtoIntanceClass =
       factory->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, funcFuncNoProtoPrototypeValue);
    // new with NewJSFunctionByHClass::function Class
    JSHandle<JSFunction> noProtoFunc = factory->NewJSFunctionByHClass(ctor,
        funcFuncNoProtoProtoIntanceClass);
    EXPECT_TRUE(*noProtoFunc != nullptr);
    // set property that has same key with fater type
    PropertyDescriptor desc2 = PropertyDescriptor(thread_, defaultString);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(noProtoFunc), addString, desc2);
    // son type
    Local<FunctionRef> noProtoLocal = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(noProtoFunc));

    noProtoLocal->Inherit(vm_, protoLocal);

    JSHandle<JSFunction> sonHandle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(noProtoLocal));
    OperationResult res = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(sonHandle), deleteString);
    EXPECT_EQ(JSTaggedValue::SameValue(deleteMethod, res.GetValue()), true);
    // test if the property value changed after inherit
    OperationResult res1 = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(sonHandle), addString);
    EXPECT_EQ(JSTaggedValue::SameValue(defaultString, res1.GetValue()), true);
}

HWTEST_F_L0(JSNApiTests, ClassFunction)
{
    LocalScope scope(vm_);
    Local<FunctionRef> cls = FunctionRef::NewClassFunction(vm_, nullptr, nullptr, nullptr);

    JSHandle<JSTaggedValue> clsObj = JSNApiHelper::ToJSHandle(Local<JSValueRef>(cls));
    ASSERT_TRUE(clsObj->IsClassConstructor());

    JSTaggedValue accessor = JSHandle<JSFunction>(clsObj)->GetPropertyInlinedProps(
                                                           JSFunction::CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX);
    ASSERT_TRUE(accessor.IsInternalAccessor());
}

HWTEST_F_L0(JSNApiTests, WeakRefSecondPassCallback)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object1 = ObjectRef::New(vm_);
    Global<ObjectRef> globalObject1(vm_, object1);
    globalObject1.SetWeak();
    NativeReferenceHelper *temp = nullptr;
    {
        LocalScope scope1(vm_);
        Local<ObjectRef> object2 = ObjectRef::New(vm_);
        Global<ObjectRef> globalObject2(vm_, object2);
        NativeReferenceHelper *ref1 = new NativeReferenceHelper(vm_, globalObject2, WeakRefCallback);
        ref1->SetWeakCallback();
        temp = ref1;
    }
    {
        LocalScope scope1(vm_);
        Local<ObjectRef> object3 = ObjectRef::New(vm_);
        Global<ObjectRef> globalObject3(vm_, object3);
        globalObject3.SetWeak();
    }
    Local<ObjectRef> object4 = ObjectRef::New(vm_);
    Global<ObjectRef> globalObject4(vm_, object4);
    NativeReferenceHelper *ref2 = new NativeReferenceHelper(vm_, globalObject4, WeakRefCallback);
    ref2->SetWeakCallback();
    vm_->CollectGarbage(TriggerGCType::OLD_GC);
    delete temp;
    delete ref2;
}

HWTEST_F_L0(JSNApiTests, TriggerGC_OLD_GC)
{
    vm_->SetEnableForceGC(false);
    auto globalEnv = vm_->GetGlobalEnv();
    auto factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> jsFunc = globalEnv->GetArrayFunction();
    JSHandle<JSObject> objVal1 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
    JSHandle<JSObject> objVal2 =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
    JSObject *newObj2 = *objVal2;
    JSTaggedValue canBeGcValue(newObj2);

    uint32_t arrayLength = 2;
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(arrayLength);
    taggedArray->Set(thread_, 0, objVal1);
    taggedArray->Set(thread_, 1, canBeGcValue);
    EXPECT_EQ(taggedArray->GetIdx(objVal1.GetTaggedValue()), 0U);
    EXPECT_EQ(taggedArray->GetIdx(canBeGcValue), 1U);

    // trigger gc
    JSNApi::TRIGGER_GC_TYPE gcType = JSNApi::TRIGGER_GC_TYPE::OLD_GC;
    JSNApi::TriggerGC(vm_, gcType);
    EXPECT_EQ(taggedArray->GetIdx(objVal1.GetTaggedValue()), 0U);
    EXPECT_EQ(taggedArray->GetIdx(canBeGcValue), TaggedArray::MAX_ARRAY_INDEX);

    vm_->SetEnableForceGC(true);
}

HWTEST_F_L0(JSNApiTests, ExecuteModuleBuffer)
{
    const char *fileName = "__JSNApiTests_ExecuteModuleBuffer.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    bool executeResult =
        JSNApi::ExecuteModuleBuffer(vm_, reinterpret_cast<const uint8_t *>(data), sizeof(data), fileName);
    EXPECT_FALSE(executeResult);
}

HWTEST_F_L0(JSNApiTests, addWorker_DeleteWorker)
{
    JSRuntimeOptions option;
    EcmaVM *workerVm = JSNApi::CreateEcmaVM(option);
    JSNApi::addWorker(vm_, workerVm);
    bool hasDeleted = JSNApi::DeleteWorker(vm_, workerVm);
    EXPECT_TRUE(hasDeleted);

    hasDeleted = JSNApi::DeleteWorker(vm_, nullptr);
    EXPECT_FALSE(hasDeleted);
}

HWTEST_F_L0(JSNApiTests, PrimitiveRef_GetValue)
{
    auto factory = vm_->GetFactory();
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
    EXPECT_EQ(intValue->Value(), 0);

    Local<JSValueRef> jsValue = intValue->GetValue(vm_);
    EXPECT_TRUE(*jsValue == nullptr);

    JSHandle<JSTaggedValue> nullHandle(thread_, JSTaggedValue::Null());
    JSHandle<JSHClass> jsClassHandle =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_PRIMITIVE_REF, nullHandle);
    TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
    JSHandle<JSTaggedValue> jsTaggedValue(thread_, JSTaggedValue(taggedObject));
    Local<PrimitiveRef> jsValueRef = JSNApiHelper::ToLocal<JSPrimitiveRef>(jsTaggedValue);
    EXPECT_TRUE(*(jsValueRef->GetValue(vm_)) != nullptr);
}

HWTEST_F_L0(JSNApiTests, BigIntRef_New_Uint64)
{
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    EXPECT_TRUE(maxBigintUint64->IsBigInt());

    JSHandle<BigInt> maxBigintUint64Val(thread_, JSNApiHelper::ToJSTaggedValue(*maxBigintUint64));
    EXPECT_EQ(maxBigintUint64Val->GetDigit(0), static_cast<uint32_t>(maxUint64 & 0xffffffff));
    EXPECT_EQ(maxBigintUint64Val->GetDigit(1), static_cast<uint32_t>((maxUint64 >> BigInt::DATEBITS) & 0xffffffff));

    uint64_t minUint64 = std::numeric_limits<uint64_t>::min();
    Local<BigIntRef> minBigintUint64 = BigIntRef::New(vm_, minUint64);
    EXPECT_TRUE(minBigintUint64->IsBigInt());

    JSHandle<BigInt> minBigintUint64Val(thread_, JSNApiHelper::ToJSTaggedValue(*minBigintUint64));
    EXPECT_EQ(minBigintUint64Val->GetDigit(0), static_cast<uint32_t>(minUint64 & 0xffffffff));
    EXPECT_EQ(minBigintUint64Val->GetLength(), 1U);
}

HWTEST_F_L0(JSNApiTests, BigIntRef_New_Int64)
{
    int64_t maxInt64 = std::numeric_limits<int64_t>::max();
    Local<BigIntRef> maxBigintInt64 = BigIntRef::New(vm_, maxInt64);
    EXPECT_TRUE(maxBigintInt64->IsBigInt());

    JSHandle<BigInt> maxBigintInt64Val(thread_, JSNApiHelper::ToJSTaggedValue(*maxBigintInt64));
    EXPECT_EQ(maxBigintInt64Val->GetDigit(0), static_cast<uint32_t>(maxInt64 & 0xffffffff));
    EXPECT_EQ(maxBigintInt64Val->GetDigit(1), static_cast<uint32_t>((maxInt64 >> BigInt::DATEBITS) & 0xffffffff));

    int64_t minInt64 = std::numeric_limits<int64_t>::min();
    Local<BigIntRef> minBigintInt64 = BigIntRef::New(vm_, minInt64);
    EXPECT_TRUE(minBigintInt64->IsBigInt());

    JSHandle<BigInt> minBigintInt64Val(thread_, JSNApiHelper::ToJSTaggedValue(*minBigintInt64));
    EXPECT_EQ(minBigintInt64Val->GetSign(), true);
    EXPECT_EQ(minBigintInt64Val->GetDigit(0), static_cast<uint32_t>((-minInt64) & 0xffffffff));
    EXPECT_EQ(minBigintInt64Val->GetDigit(1), static_cast<uint32_t>(((-minInt64) >> BigInt::DATEBITS) & 0xffffffff));
}

HWTEST_F_L0(JSNApiTests, BigIntRef_CreateBigWords_GetWordsArray_GetWordsArraySize)
{
    bool sign = false;
    uint32_t size = 3;
    const uint32_t MULTIPLE = 2;
    const uint64_t words[3] = {
        std::numeric_limits<uint64_t>::min() - 1,
        std::numeric_limits<uint64_t>::min(),
        std::numeric_limits<uint64_t>::max(),
    };
    Local<JSValueRef> bigWords = BigIntRef::CreateBigWords(vm_, sign, size, words);
    EXPECT_TRUE(bigWords->IsBigInt());

    Local<BigIntRef> bigWordsRef(bigWords);
    EXPECT_EQ(bigWordsRef->GetWordsArraySize(), size);

    JSHandle<BigInt> bigintUint64Val(thread_, JSNApiHelper::ToJSTaggedValue(*bigWords));
    EXPECT_EQ(bigintUint64Val->GetSign(), false);
    EXPECT_EQ(bigintUint64Val->GetLength(), size * MULTIPLE);

    bool resultSignBit = true;
    uint64_t *resultWords = new uint64_t[3](); // 3 : length of words array
    bigWordsRef->GetWordsArray(&resultSignBit, size, resultWords);
    EXPECT_EQ(resultSignBit, false);
    EXPECT_EQ(resultWords[0], words[0]);
    EXPECT_EQ(resultWords[1], words[1]);
    EXPECT_EQ(resultWords[2], words[2]);
    delete[] resultWords;
}

HWTEST_F_L0(JSNApiTests, DateRef_New_ToString_GetTime)
{
    double time = 1.1;
    Local<DateRef> data = DateRef::New(vm_, time);
    EXPECT_TRUE(data->IsDate());

    Local<StringRef> tostring = data->ToString(vm_);
    Local<JSValueRef> toValue(tostring);
    EXPECT_TRUE(tostring->IsString());
    double dou = data->GetTime();
    EXPECT_EQ(dou, 1.1);
}

HWTEST_F_L0(JSNApiTests, PromiseRef_Finally)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> reject = FunctionRef::New(vm_, RejectCallback);
    Local<PromiseRef> catchPromise = promise->Finally(vm_, reject);
    ASSERT_TRUE(promise->IsPromise());
    ASSERT_TRUE(catchPromise->IsPromise());
    Local<PromiseRef> catchPromise1 = promise->Then(vm_, reject, reject);
    ASSERT_TRUE(catchPromise1->IsPromise());
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback);
    ASSERT_TRUE(!callback.IsEmpty());
    Local<PromiseRef> catchPromise2 = promise->Then(vm_, callback);
    ASSERT_TRUE(catchPromise2->IsPromise());
}

HWTEST_F_L0(JSNApiTests, JSNApi_SerializeValue)
{
    LocalScope scope(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback);
    ASSERT_TRUE(!callback.IsEmpty());
    std::vector<Local<JSValueRef>> arguments;
    arguments.emplace_back(JSValueRef::Undefined(vm_));
    Local<JSValueRef> result =
        callback->Call(vm_, JSValueRef::Undefined(vm_), arguments.data(), arguments.size());
    ASSERT_TRUE(result->IsArray(vm_));
    Local<ArrayRef> array(result);
    ASSERT_EQ(static_cast<uint64_t>(array->Length(vm_)), arguments.size());
    void* res = nullptr;
    res = JSNApi::SerializeValue(vm_, result, result);
    EXPECT_TRUE(res);
}

HWTEST_F_L0(JSNApiTests, JSNApi_SetHostPromiseRejectionTracker)
{
    void *data = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    JSNApi::SetHostPromiseRejectionTracker(vm_, data, data);
    PromiseRejectCallback res = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPromiseRejectCallback();
    ASSERT_EQ(res, reinterpret_cast<ecmascript::PromiseRejectCallback>(data));
}

HWTEST_F_L0(JSNApiTests, JSNApi_SetNativePtrGetter_SetHostEnqueueJob)
{
    void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    JSNApi::SetNativePtrGetter(vm_, cb);
    NativePtrGetter res = vm_->GetNativePtrGetter();
    ASSERT_EQ(res, reinterpret_cast<ecmascript::NativePtrGetter>(cb));
}

HWTEST_F_L0(JSNApiTests, NumberRef_New)
{
    uint32_t input = 32;
    int64_t input1 = 1;
    Local<NumberRef> res = NumberRef::New(vm_, input);
    Local<NumberRef> res1 = NumberRef::New(vm_, input1);
    ASSERT_TRUE(res->IsNumber());
    ASSERT_TRUE(res1->IsNumber());
}

HWTEST_F_L0(JSNApiTests, ObjectRef_Set)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    void *data1 = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    void *data2 = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    bool res = object->Set(vm_, data1, data2);
    ASSERT_TRUE(res);
}

HWTEST_F_L0(JSNApiTests, ObjectRef_GetOwnEnumerablePropertyNames)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<ArrayRef> res = object->GetOwnEnumerablePropertyNames(vm_);
    ASSERT_TRUE(res->IsArray(vm_));
}

HWTEST_F_L0(JSNApiTests, ObjectRef_SetNativePointerFieldCount_GetNativePointerFieldCount)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    int32_t input = 34;
    object->SetNativePointerFieldCount(input);
    int32_t res = object->GetNativePointerFieldCount();
    ASSERT_EQ(res, input);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void*>(new std::string("test"));
    void *vp2 = static_cast<void*>(new std::string("test"));
    std::string *sp1 = static_cast<std::string*>(vp1);
    object->SetNativePointerField(33, vp1, callBack, vp2);
    void *res1 = object->GetNativePointerField(33);
    std::string *sp2 = static_cast<std::string*>(res1);
    ASSERT_EQ(sp1, sp2);
}

HWTEST_F_L0(JSNApiTests, FunctionRef_GetFunctionPrototype_SetName_GetName)
{
    LocalScope scope(vm_);
    Deleter deleter = nullptr;
    void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    bool callNative = true;
    size_t nativeBindingsize = 15;
    Local<FunctionRef> res = FunctionRef::NewClassFunction(vm_, FunctionCallback,
                                                           deleter, cb, callNative, nativeBindingsize);
    ASSERT_TRUE(res->IsFunction());
    Local<JSValueRef> res1 = res->GetFunctionPrototype(vm_);
    ASSERT_TRUE(!res1->IsArray(vm_));
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    res->SetName(vm_, origin);
    Local<StringRef> s = res->GetName(vm_);
    std::string str = s->ToString();
    ASSERT_EQ("1", str);
}

HWTEST_F_L0(JSNApiTests, JSNApi_SetAssetPath_GetAssetPath)
{
    LocalScope scope(vm_);
    std::string str = "11";
    JSNApi::SetAssetPath(vm_, str);
    std::string res = JSNApi::GetAssetPath(vm_);
    ASSERT_EQ(str, res);
    void *data = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    JSNApi::SetLoop(vm_, data);
    void* res1 = vm_->GetLoop();
    ASSERT_EQ(res1, data);
}

HWTEST_F_L0(JSNApiTests, JSValueRef_ToNativePointer)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);
    ASSERT_EQ(toString->ToNumber(vm_)->Value(), -123.3); // -123 : test case of input
    ASSERT_EQ(toString->ToBoolean(vm_)->Value(), true);
    ASSERT_EQ(toValue->ToString(vm_)->ToString(), "-123.3");
    ASSERT_TRUE(toValue->ToObject(vm_)->IsObject());
    Local<NativePointerRef> res = toValue->ToNativePointer(vm_);
    ASSERT_TRUE(res->IsString());
}

HWTEST_F_L0(JSNApiTests, GeneratorObjectRef_IsGenerator)
{
    ObjectFactory *factory = vm_->GetFactory();
    auto env = vm_->GetGlobalEnv();

    JSHandle<JSTaggedValue> genFunc = env->GetGeneratorFunctionFunction();
    JSHandle<JSGeneratorObject> genObjHandleVal = factory->NewJSGeneratorObject(genFunc);

    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    JSHandle<JSFunction> generatorFunc = JSHandle<JSFunction>::Cast(factory->NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, generatorFunc, FunctionKind::GENERATOR_FUNCTION);

    JSHandle<GeneratorContext> generatorContext = factory->NewGeneratorContext();
    generatorContext->SetMethod(thread_, generatorFunc.GetTaggedValue());

    JSHandle<JSTaggedValue> generatorContextVal = JSHandle<JSTaggedValue>::Cast(generatorContext);
    genObjHandleVal->SetGeneratorContext(thread_,  generatorContextVal.GetTaggedValue());

    JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
    Local<GeneratorObjectRef> genObjectRef = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
    Local<JSValueRef> res = genObjectRef->GetGeneratorFunction(vm_);
    ASSERT_TRUE(res->IsGeneratorFunction());
}

HWTEST_F_L0(JSNApiTests, BigIntToInt64)
{
    LocalScope scope(vm_);
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    EXPECT_TRUE(maxBigintUint64->IsBigInt());
    int64_t num = -11;
    int64_t num1 = num;
    bool lossless = true;
    maxBigintUint64->BigIntToInt64(vm_, &num, &lossless);
    EXPECT_TRUE(num != num1);
}

HWTEST_F_L0(JSNApiTests, BigIntToUint64)
{
    LocalScope scope(vm_);
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    EXPECT_TRUE(maxBigintUint64->IsBigInt());
    uint64_t  num = -11;
    uint64_t  num1 = num;
    bool lossless = true;
    maxBigintUint64->BigIntToUint64(vm_, &num, &lossless);
    EXPECT_TRUE(num != num1);
}

HWTEST_F_L0(JSNApiTests, BooleanRef_New)
{
    LocalScope scope(vm_);
    bool input = true;
    Local<BooleanRef> res = BooleanRef::New(vm_, input);
    EXPECT_TRUE(res->IsBoolean());
    EXPECT_TRUE(res->BooleaValue());
}

HWTEST_F_L0(JSNApiTests, NewFromUnsigned)
{
    LocalScope scope(vm_);
    unsigned int input = 1;
    [[maybe_unused]] Local<IntegerRef> res = IntegerRef::NewFromUnsigned(vm_, input);
    EXPECT_TRUE(res->IntegerValue(vm_) == 1);
    EXPECT_TRUE(res->Value() == 1);
}

HWTEST_F_L0(JSNApiTests, SetBundleName_GetBundleName)
{
    LocalScope scope(vm_);
    std::string str = "11";
    JSNApi::SetBundleName(vm_, str);
    std::string res = JSNApi::GetBundleName(vm_);
    ASSERT_EQ(str, res);
}

HWTEST_F_L0(JSNApiTests, SetModuleName_GetModuleName)
{
    LocalScope scope(vm_);
    std::string str = "11";
    JSNApi::SetModuleName(vm_, str);
    std::string res = JSNApi::GetModuleName(vm_);
    ASSERT_EQ(str, res);
}

HWTEST_F_L0(JSNApiTests, IsBundle)
{
    LocalScope scope(vm_);
    bool res = JSNApi::IsBundle(vm_);
    ASSERT_EQ(res, true);
}

HWTEST_F_L0(JSNApiTests, MapRef_GetSize_GetTotalElements_Get_GetKey_GetValue_New_Set)
{
    LocalScope scope(vm_);
    Local<MapRef> map = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    map->Set(vm_, key, value);
    Local<JSValueRef> res = map->Get(vm_, key);
    ASSERT_EQ(res->ToString(vm_)->ToString(), value->ToString(vm_)->ToString());
    int32_t num = map->GetSize();
    int32_t num1 = map->GetTotalElements();
    ASSERT_EQ(num, 1);
    ASSERT_EQ(num1, 1);
    Local<JSValueRef> res1 = map->GetKey(vm_, 0);
    ASSERT_EQ(res1->ToString(vm_)->ToString(), key->ToString(vm_)->ToString());
    Local<JSValueRef> res2 = map->GetValue(vm_, 0);
    ASSERT_EQ(res2->ToString(vm_)->ToString(), value->ToString(vm_)->ToString());
}

HWTEST_F_L0(JSNApiTests, GetSourceCode)
{
    LocalScope scope(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback);
    bool res = callback->IsNative(vm_);
    EXPECT_TRUE(res);
}

HWTEST_F_L0(JSNApiTests, ObjectRef_Delete)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);
    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    ASSERT_TRUE(object->Delete(vm_, key));
    ASSERT_FALSE(object->Has(vm_, key));
}

HWTEST_F_L0(JSNApiTests, ObjectRef_Set1)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);
    bool res = object->Set(vm_, 12, toValue);
    ASSERT_TRUE(res);
    Local<JSValueRef> res1 = object->Get(vm_, 12);
    ASSERT_EQ(res1->ToString(vm_)->ToString(), toValue->ToString(vm_)->ToString());
}

HWTEST_F_L0(JSNApiTests, NativePointerRef_New)
{
    LocalScope scope(vm_);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void*>(new std::string("test"));
    void *vp2 = static_cast<void*>(new std::string("test"));
    Local<NativePointerRef> res =  NativePointerRef::New(vm_, vp1, callBack, vp2, 0);
    ASSERT_EQ(res->Value(), vp1);
}

HWTEST_F_L0(JSNApiTests, ObjectRef_Has_Delete)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    uint32_t num = 10;
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);
    bool res = object->Set(vm_, num, toValue);
    ASSERT_TRUE(res);
    bool res1 = object->Has(vm_, num);
    ASSERT_TRUE(res1);
    bool res2 = object->Delete(vm_, num);
    ASSERT_TRUE(res2);
    bool res3 = object->Has(vm_, num);
    ASSERT_FALSE(res3);
}

HWTEST_F_L0(JSNApiTests, PromiseRejectInfo_GetData)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> promise(toString);
    Local<StringRef> toString1 = StringRef::NewFromUtf8(vm_, "123.3");
    Local<JSValueRef> reason(toString1);
    void *data = static_cast<void*>(new std::string("test"));
    PromiseRejectInfo  promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
    Local<JSValueRef> promise_res = promisereject.GetPromise();
    Local<JSValueRef> reason_res = promisereject.GetReason();
    ASSERT_EQ(promise_res->ToString(vm_)->ToString(), promise->ToString(vm_)->ToString());
    ASSERT_EQ(reason_res->ToString(vm_)->ToString(), reason->ToString(vm_)->ToString());
    void* dataRes = promisereject.GetData();
    ASSERT_EQ(dataRes, data);
}

HWTEST_F_L0(JSNApiTests, FunctionCallScope)
{
    {
        FunctionCallScope callScope(vm_);
        ASSERT_FALSE(vm_->IsTopLevelCallDepth());
    }
    ASSERT_TRUE(vm_->IsTopLevelCallDepth());
}
}  // namespace panda::test
