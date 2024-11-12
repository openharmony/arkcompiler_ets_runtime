/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstddef>
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/circuit_builder_helper.h"
#include "ecmascript/deoptimizer/deoptimizer.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/include/jsnapi_internals.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/pgo_profiler/pgo_profiler.h"
#include "ecmascript/pgo_profiler/pgo_profiler_decoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_encoder.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/weak_vector.h"
#include "ecmascript/regexp/regexp_parser.h"
#include "gtest/gtest.h"
#include "jsnapi_expo.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;

static constexpr char TEST_CHAR_STRING_FLAGS[] = "gimsuy";
static constexpr char TEST_CHAR_STRING_STATE[] = "closed";

namespace panda::test {
using BuiltinsFunction = ecmascript::builtins::BuiltinsFunction;
using PGOProfilerManager = panda::ecmascript::pgo::PGOProfilerManager;
using FunctionForRef = Local<JSValueRef> (*)(JsiRuntimeCallInfo *);
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
        thread_->ManagedCodeBegin();
    }

    void TearDown() override
    {
        thread_->ManagedCodeEnd();
        vm_->SetEnableForceGC(false);
        JSNApi::DestroyJSVM(vm_);
    }

    template <typename T> void TestNumberRef(T val, TaggedType expected)
    {
        LocalScope scope(vm_);
        Local<NumberRef> obj = NumberRef::New(vm_, val);
        ASSERT_TRUE(obj->IsNumber());
        JSTaggedType res = JSNApiHelper::ToJSTaggedValue(*obj).GetRawData();
        ASSERT_EQ(res, expected);
        if constexpr (std::is_floating_point_v<T>) {
            if (std::isnan(val)) {
                ASSERT_TRUE(std::isnan(obj->Value()));
            } else {
                ASSERT_EQ(obj->Value(), val);
            }
        } else if constexpr (sizeof(T) >= sizeof(int32_t)) {
            ASSERT_EQ(obj->IntegerValue(vm_), val);
        } else if constexpr (std::is_signed_v<T>) {
            ASSERT_EQ(obj->Int32Value(vm_), val);
        } else {
            ASSERT_EQ(obj->Uint32Value(vm_), val);
        }
    }

    TaggedType ConvertDouble(double val)
    {
        return base::bit_cast<JSTaggedType>(val) + JSTaggedValue::DOUBLE_ENCODE_OFFSET;
    }

protected:
    JSThread *thread_ = nullptr;
    EcmaVM *vm_ = nullptr;
};

Local<JSValueRef> FunctionCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

void WeakRefCallback(EcmaVM *vm)
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
    ASSERT_TRUE(globalObject->IsObject(vm_));
}

HWTEST_F_L0(JSNApiTests, ThreadIdCheck)
{
    EXPECT_TRUE(vm_->GetJSThread()->GetThreadId() == JSThread::GetCurrentThreadId());
}

/**
 * @tc.number: ffi_interface_api_001
 * @tc.name: RegisterFunction
 * @tc.desc:Through the FunctionRef:: New method, we can obtain a reference to the function, register and execute it,
 * confirm that the return value is an array, and the length of the array is the same as the length of
 * the passed in parameter list.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, RegisterFunction)
{
    LocalScope scope(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback);
    ASSERT_TRUE(!callback.IsEmpty());
    std::vector<Local<JSValueRef>> arguments;
    arguments.emplace_back(JSValueRef::Undefined(vm_));
    Local<JSValueRef> result = callback->Call(vm_, JSValueRef::Undefined(vm_),
                                              arguments.data(), arguments.size());
    ASSERT_TRUE(result->IsArray(vm_));
    Local<ArrayRef> array(result);
    ASSERT_EQ(static_cast<uint64_t>(array->Length(vm_)), arguments.size());
}

HWTEST_F_L0(JSNApiTests, GetProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject(vm_));

    Local<ObjectRef> key = StringRef::NewFromUtf8(vm_, "Number");
    Local<ObjectRef> property = globalObject->Get(vm_, key);
    ASSERT_TRUE(property->IsFunction(vm_));
}

HWTEST_F_L0(JSNApiTests, SetProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject(vm_));

    Local<ArrayRef> property = ArrayRef::New(vm_, 3); // 3 : length
    ASSERT_TRUE(property->IsArray(vm_));
    ASSERT_EQ(property->Length(vm_), 3U); // 3 : test case of input

    Local<ObjectRef> key = StringRef::NewFromUtf8(vm_, "Test");
    bool result = globalObject->Set(vm_, key, property);
    ASSERT_TRUE(result);

    Local<ObjectRef> propertyGet = globalObject->Get(vm_, key);
    ASSERT_TRUE(propertyGet->IsArray(vm_));
    ASSERT_EQ(Local<ArrayRef>(propertyGet)->Length(vm_), 3U); // 3 : test case of input
}

/**
 * @tc.number: ffi_interface_api_002
 * @tc.name: JsonParser
 * @tc.desc:Construct a BufferRef function to determine whether it is a Get
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JsonParser)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject(vm_));

    const char * const test { R"({"orientation": "portrait"})" };
    Local<ObjectRef> jsonString = StringRef::NewFromUtf8(vm_, test);

    Local<JSValueRef> result = JSON::Parse(vm_, jsonString);
    ASSERT_TRUE(result->IsObject(vm_));

    Local<ObjectRef> keyString = StringRef::NewFromUtf8(vm_, "orientation");
    Local<JSValueRef> property = Local<ObjectRef>(result)->Get(vm_, keyString);
    ASSERT_TRUE(property->IsString(vm_));
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

/**
 * @tc.number: ffi_interface_api_003
 * @tc.name: InstanceOf
 * @tc.desc:Verifying whether the InstanceOf method can correctly determine whether an object is an
 * instance of another object.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
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
    ASSERT_EQ(typeString->ToString(vm_), "string");

    Local<NumberRef> target = NumberRef::New(vm_, 1);
    typeString = target->Typeof(vm_);
    ASSERT_EQ(typeString->ToString(vm_), "number");
}

/**
 * @tc.number: ffi_interface_api_004
 * @tc.name: Symbol
 * @tc.desc: Determine if it is a symbol type
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Symbol)
{
    LocalScope scope(vm_);
    Local<StringRef> description = StringRef::NewFromUtf8(vm_, "test");
    Local<SymbolRef> symbol = SymbolRef::New(vm_, description);

    ASSERT_FALSE(description->IsSymbol(vm_));
    ASSERT_TRUE(symbol->IsSymbol(vm_));
}

/**
 * @tc.number: ffi_interface_api_005
 * @tc.name: StringUtf8_001
 * @tc.desc:
 * Utf8Length：Read the non Chinese value length of StringRef according to utf8 type
 * WriteUtf8：Write the non Chinese value of StringRef to the char array buffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, StringUtf8_001)
{
    LocalScope scope(vm_);
    std::string test = "Hello world";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Utf8Length(vm_), 12);       // 12 : length of testString("Hello World")
    char buffer[12];                                  // 12 : length of testString
    EXPECT_EQ(testString->WriteUtf8(vm_, buffer, 12), 12); // 12 : length of testString("Hello World")
    std::string res(buffer);
    ASSERT_EQ(res, test);
}

/**
 * @tc.number: ffi_interface_api_006
 * @tc.name: StringUtf8_002
 * @tc.desc:
 * Utf8Length：Read the non Chinese value length of StringRef according to utf8 type
 * WriteUtf8：Write the non Chinese value of StringRef to the char array buffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, StringUtf8_002)
{
    LocalScope scope(vm_);
    std::string test = "年";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Utf8Length(vm_), 4);      // 4 : length of testString("年")
    char buffer[4];                                 // 4 : length of testString
    EXPECT_EQ(testString->WriteUtf8(vm_, buffer, 4), 4); // 4 : length of testString("年")
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
    EXPECT_EQ(testString1->Utf8Length(vm_, false), 5);
    char buffer1[4];
    testString1->WriteUtf8(vm_, buffer1, 4, false);
    EXPECT_EQ(buffer1[0], 'a');
    EXPECT_EQ(buffer1[1], '\xC0');
    EXPECT_EQ(buffer1[2], '\x80');
    EXPECT_EQ(buffer1[3], 'b');

    // isWriteBuffer == true, \u0000 ==> 0x00U
    Local<StringRef> testString2 = StringRef::NewFromUtf8(vm_, test.c_str(), test.length());
    EXPECT_EQ(testString2->Utf8Length(vm_, true), 4);
    char buffer2[4];
    testString2->WriteUtf8(vm_, buffer2, 4, true);
    EXPECT_EQ(buffer2[0], 'a');
    EXPECT_EQ(buffer2[1], '\0');
    EXPECT_EQ(buffer2[2], 'b');
}

HWTEST_F_L0(JSNApiTests, StringEncodeIntoUint8_001) {
    LocalScope scope(vm_);
    std::string test = "";

    Local<StringRef> testString1 =
        StringRef::NewFromUtf8(vm_, test.c_str(), test.length());
    Local<TypedArrayRef> typedArray = testString1->EncodeIntoUint8Array(vm_);
    EXPECT_TRUE(typedArray->IsUndefined());
}

HWTEST_F_L0(JSNApiTests, StringEncodeIntoUint8_002) {
    LocalScope scope(vm_);
    std::string test = "abc123";
    char excepted[7] = {0x61, 0x62, 0x63, 0x31, 0x32, 0x33, 0};

    Local<StringRef> testString1 =
        StringRef::NewFromUtf8(vm_, test.c_str(), test.length());
    Local<TypedArrayRef> typedArray = testString1->EncodeIntoUint8Array(vm_);

    char *res = reinterpret_cast<char *>(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_));

    ASSERT_STREQ(res, excepted);
}

/**
 * @tc.number: ffi_interface_api_007
 * @tc.name: StringLatin1_001
 * @tc.desc:
 * WriteLatin1：Write the Chinese value of StringRef to the char array buffer
 * Length：Obtain the length of the Chinese value of StringRef
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, StringLatin1_001)
{
    LocalScope scope(vm_);
    std::string test = "中";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Length(vm_), 1U);
    char buffer[1];
    EXPECT_EQ(testString->WriteLatin1(vm_, buffer, 1), 1);

    EXPECT_EQ(buffer[0], '-'); // '-' == 0x2D
}

/**
 * @tc.number: ffi_interface_api_008
 * @tc.name: StringLatin1_002
 * @tc.desc:
 * WriteLatin1：Write the non Chinese value of StringRef to the char array buffer
 * Length：Obtain the length of the non Chinese value of StringRef
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, StringLatin1_002)
{
    LocalScope scope(vm_);
    std::string test = "En123";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Length(vm_), 5U);
    char buffer[5];
    EXPECT_EQ(testString->WriteLatin1(vm_, buffer, 5), 5);

    EXPECT_EQ(buffer[0], 'E');
    EXPECT_EQ(buffer[1], 'n');
    EXPECT_EQ(buffer[2], '1');
    EXPECT_EQ(buffer[3], '2');
    EXPECT_EQ(buffer[4], '3');
}

/**
 * @tc.number: ffi_interface_api_009
 * @tc.name: ToType
 * @tc.desc:
 * ToString：Obtain the length of the non Chinese value of StringRef
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ToType)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);

    ASSERT_EQ(toString->ToNumber(vm_)->Value(), -123.3); // -123 : test case of input
    ASSERT_EQ(toString->ToBoolean(vm_)->Value(), true);
    ASSERT_EQ(toValue->ToString(vm_)->ToString(vm_), "-123.3");
    ASSERT_TRUE(toValue->ToObject(vm_)->IsObject(vm_));
}

HWTEST_F_L0(JSNApiTests, TypeValue)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123");
    Local<JSValueRef> toValue(toString);

    ASSERT_EQ(toString->Int32Value(vm_), -123); // -123 : test case of input
    ASSERT_EQ(toString->BooleaValue(vm_), true);
    ASSERT_EQ(toString->Uint32Value(vm_), 4294967173U); // 4294967173 : test case of input
    ASSERT_EQ(toString->IntegerValue(vm_), -123);       // -123 : test case of input
}

void *Detach()
{
    GTEST_LOG_(INFO) << "detach is running";
    return nullptr;
}

void Attach([[maybe_unused]] int *buffer)
{
    GTEST_LOG_(INFO) << "attach is running";
}

static panda::JSNApi::NativeBindingInfo *CreateNativeBindingInfo(void *attach, void *detach)
{
    GTEST_LOG_(INFO) << "CreateNativeBindingInfo";
    panda::JSNApi::NativeBindingInfo *info = panda::JSNApi::NativeBindingInfo::CreateNewInstance();
    info->attachData = attach;
    info->detachData = detach;
    return info;
}

HWTEST_F_L0(JSNApiTests, CreateNativeObject)
{
    LocalScope scope(vm_);
    auto info = CreateNativeBindingInfo(reinterpret_cast<void *>(Attach), reinterpret_cast<void *>(Detach));
    size_t nativeBindingSize = 7 * sizeof(void *); // 7 : params num
    Local<NativePointerRef> nativeInfo = NativePointerRef::New(
        vm_, reinterpret_cast<void *>(info),
        []([[maybe_unused]] void *env, void *data, [[maybe_unused]] void *info) {
            auto externalInfo = reinterpret_cast<panda::JSNApi::NativeBindingInfo *>(data);
            delete externalInfo;
        },
        nullptr, nativeBindingSize);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    bool result = object->ConvertToNativeBindingObject(vm_, nativeInfo);
    ASSERT_TRUE(result);
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

/**
 * @tc.number: ffi_interface_api_010
 * @tc.name: DefineProperty
 * @tc.desc: Set Key values and corresponding attribute values
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
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

/**
 * @tc.number: ffi_interface_api_011
 * @tc.name: GetPrototype
 * @tc.desc:Verify that the GetPrototype method correctly returns the prototype of the function or object,
 * and verify that the returned prototype is of an object type.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetPrototype)
{
    LocalScope scope(vm_);
    Local<FunctionRef> function = FunctionRef::New(vm_, nullptr);
    Local<JSValueRef> protoType = function->GetPrototype(vm_);
    ASSERT_TRUE(protoType->IsObject(vm_));

    Local<FunctionRef> object = ObjectRef::New(vm_);
    protoType = object->GetPrototype(vm_);
    ASSERT_TRUE(protoType->IsObject(vm_));

    auto info = CreateNativeBindingInfo(reinterpret_cast<void *>(Attach), reinterpret_cast<void *>(Detach));
    size_t nativeBindingSize = 7 * sizeof(void *); // 7 : params num
    Local<NativePointerRef> nativeInfo = NativePointerRef::New(
        vm_, reinterpret_cast<void *>(info),
        []([[maybe_unused]] void *env, void *data, [[maybe_unused]] void *info) {
            auto externalInfo = reinterpret_cast<panda::JSNApi::NativeBindingInfo *>(data);
            delete externalInfo;
        },
        nullptr, nativeBindingSize);
    bool result = object->ConvertToNativeBindingObject(vm_, nativeInfo);
    ASSERT_TRUE(result);
    protoType = object->GetPrototype(vm_);
    ASSERT_TRUE(protoType->IsObject(vm_));
}

/*
 * @tc.number: ffi_interface_api_012
 * @tc.name: CheckReject
 * @tc.desc: The function of CheckReject is similar to that of CheckResolve,
 * but it is used to check whether a function call provides the correct cause of the error,
 * which is achieved through ASSERT_ EQ (Local<StringRef>(reason) ->ToString(vm_),
 * check if the value of this string is equal to "Reject".
 * @tc.type: FUNC
 * @tc.require:  parameter info
 */
void CheckReject(JsiRuntimeCallInfo *info)
{
    ASSERT_EQ(info->GetArgsNumber(), 1U);
    Local<JSValueRef> reason = info->GetCallArgRef(0);
    ASSERT_TRUE(reason->IsString(info->GetVM()));
    ASSERT_EQ(Local<StringRef>(reason)->ToString(info->GetVM()), "Reject");
}

Local<JSValueRef> RejectCallback(JsiRuntimeCallInfo *info)
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
    ASSERT_TRUE(promise->IsPromise(vm_));
    ASSERT_TRUE(catchPromise->IsPromise(vm_));

    Local<StringRef> reason = StringRef::NewFromUtf8(vm_, "Reject");
    ASSERT_TRUE(capability->Reject(vm_, reason));

    vm_->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
}

HWTEST_F_L0(JSNApiTests, PromiseCatchUintPtr)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> reject = FunctionRef::New(vm_, RejectCallback);
    Local<PromiseRef> catchPromise = promise->Catch(vm_, reject);
    ASSERT_TRUE(promise->IsPromise(vm_));
    ASSERT_TRUE(catchPromise->IsPromise(vm_));

    Local<StringRef> reason = StringRef::NewFromUtf8(vm_, "Reject");
    ASSERT_TRUE(capability->Reject(vm_, reinterpret_cast<uintptr_t>(*reason)));

    vm_->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
}

/*
 * @tc.number: ffi_interface_api_013
 * @tc.name: CheckResolve_New_Reject
 * @tc.desc: Verify whether a specific function call provided the correct parameters (a number 300.3),
 * where ASSERT_ TRUE (value ->IsNumber()) Check if this parameter is a number.
 * New:Used to verify whether the creation of a new PromiseCapabilityRef object was successful.
 * Reject:Used to verify whether the reason for rejecting the Promise object was successfully obtained.
 * @tc.type: FUNC
 * @tc.require:  parameter  info
 */
void CheckResolve(JsiRuntimeCallInfo *info)
{
    ASSERT_EQ(info->GetArgsNumber(), 1U);
    Local<JSValueRef> value = info->GetCallArgRef(0);
    ASSERT_TRUE(value->IsNumber());
    ASSERT_EQ(Local<NumberRef>(value)->Value(), 300.3); // 300.3 : test case of input
}

Local<JSValueRef> ResolvedCallback(JsiRuntimeCallInfo *info)
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
    ASSERT_TRUE(promise->IsPromise(vm_));
    ASSERT_TRUE(thenPromise->IsPromise(vm_));

    Local<StringRef> value = NumberRef::New(vm_, 300.3); // 300.3 : test case of input
    ASSERT_TRUE(capability->Resolve(vm_, value));
    vm_->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
}

HWTEST_F_L0(JSNApiTests, PromiseThenUintPtr)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> resolve = FunctionRef::New(vm_, ResolvedCallback);
    Local<FunctionRef> reject = FunctionRef::New(vm_, RejectCallback);
    Local<PromiseRef> thenPromise = promise->Then(vm_, resolve, reject);
    ASSERT_TRUE(promise->IsPromise(vm_));
    ASSERT_TRUE(thenPromise->IsPromise(vm_));

    Local<StringRef> value = NumberRef::New(vm_, 300.3); // 300.3 : test case of input
    ASSERT_TRUE(capability->Resolve(vm_, reinterpret_cast<uintptr_t>(*value)));
    vm_->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
}

/**
 * @tc.number: ffi_interface_api_014
 * @tc.name: Constructor_IsObject
 * @tc.desc: Used to verify whether the creation of a new PromiseCapabilityRef object was successful.
 *           Used to verify whether obtaining a PromiseRef object was successful.
             IsObject：Determine if it is an object
 * @tc.type: FUNC
 * @tc.require:  parameter isobject
 */
HWTEST_F_L0(JSNApiTests, Constructor_IsObject)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = JSNApi::GetGlobalObject(vm_);
    Local<StringRef> key = StringRef::NewFromUtf8(vm_, "Number");
    Local<FunctionRef> numberConstructor = object->Get(vm_, key);
    Local<JSValueRef> argv[1];
    argv[0] = NumberRef::New(vm_, 1.3); // 1.3 : test case of input
    Local<JSValueRef> result = numberConstructor->Constructor(vm_, argv, 1);
    ASSERT_TRUE(result->IsObject(vm_));
    ASSERT_EQ(result->ToNumber(vm_)->Value(), 1.3); // 1.3 : size of arguments
}

/**
 * @tc.number: ffi_interface_api_015
 * @tc.name: Constructor_IsBuffer
 * @tc.desc: Construct a BufferRef function to determine whether it is a Buffer.
 * 			 The constructor used to verify the success of the FunctionRef class.
 * @tc.type: FUNC
 * @tc.require:  parameter  parameter
 */
HWTEST_F_L0(JSNApiTests, ArrayBuffer)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));
    ASSERT_EQ(arrayBuffer->ByteLength(vm_), length);
    ASSERT_NE(arrayBuffer->GetBuffer(vm_), nullptr);
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
    NativePointerCallback deleter = []([[maybe_unused]] void *env, void *buffer, void *data) -> void {
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
        ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));
        ASSERT_EQ(arrayBuffer->ByteLength(vm_), length);
        ASSERT_EQ(arrayBuffer->GetBuffer(vm_), buffer);
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
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 5 : offset of byte, 7 : length
    Local<DataViewRef> dataView = DataViewRef::New(vm_, arrayBuffer, 5, 7);
    ASSERT_TRUE(dataView->IsDataView(vm_));
    ASSERT_EQ(dataView->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
    ASSERT_EQ(dataView->ByteLength(), 7U); // 7 : size of arguments
    ASSERT_EQ(dataView->ByteOffset(), 5U); // 5 : size of arguments

    // 5 : offset of byte, 11 : length
    dataView = DataViewRef::New(vm_, arrayBuffer, 5, 11);
    ASSERT_TRUE(dataView->IsUndefined());
}

/**
 * @tc.number: ffi_interface_api_016
 * @tc.name: Int8Array_IsUndefined
 * @tc.desc:Using the functions of Int8Array and verifying if its attribute values are correct.
 *          Used to determine whether a given object represents an undefined value.
            Determine if it is an int8 array.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Int8Array)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 5 : offset of byte, 6 : length
    Local<Int8ArrayRef> typedArray = Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsInt8Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 6U);  // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5U);  // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

/**
 * @tc.number: ffi_interface_api_017
 * @tc.name: Uint8Array_ ByteLength_ByteOffset_ArrayLength_GetArrayBuffer
 * @tc.desc:Using the functions of Uint8Array and verifying if its attribute values are correct.
 * 		    Used to verify whether the length, offset, array length, and associated
 * ArrayBufferRef object of the bytes obtained from the array were successful.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Uint8Array)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 5 : offset of byte, 6 : length
    Local<Uint8ArrayRef> typedArray = Uint8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsUint8Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 6U);  // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5U);  // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

/**
 * @tc.number: ffi_interface_api_018
 * @tc.name: Uint8ClampedArray
 * @tc.desc:Using the functions of Uint8ClampedArray and verifying if its attribute values are correct.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Uint8ClampedArray)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 5 : offset of byte, 6 : length
    Local<Uint8ClampedArrayRef> typedArray = Uint8ClampedArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsUint8ClampedArray(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 6U);  // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5U);  // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

/**
 * @tc.number: ffi_interface_api_019
 * @tc.name: Int16Array
 * @tc.desc:Using the functions of Int16Array and verifying if its attribute values are correct.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Int16Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 4 : offset of byte, 6 : length
    Local<Int16ArrayRef> typedArray = Int16ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsInt16Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 12U); // 12 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);  // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

/**
 * @tc.number: ffi_interface_api_020
 * @tc.name: Uint16Array
 * @tc.desc:Using the functions of Uint16Array and verifying if its attribute values are correct.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Uint16Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 4 : offset of byte, 6 : length
    Local<Uint16ArrayRef> typedArray = Uint16ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsUint16Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 12U); // 12 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);  // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

/*
 * @tc.number: ffi_interface_api_021
 * @tc.name: Uint32Array
 * @tc.desc: Verify that the Uint32Array method correctly created a Uint32Array with the specified length and offset,
 * and verify that its attribute values match expectations.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Uint32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 4 : offset of byte, 6 : length
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsUint32Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 24U); // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);  // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

/**
 * @tc.number: ffi_interface_api_022
 * @tc.name: Int32Array
 * @tc.desc:Using the functions of Int32Array and verifying if its attribute values are correct.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Int32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 4 : offset of byte, 6 : length
    Local<Int32ArrayRef> typedArray = Int32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsInt32Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 24U); // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);  // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

HWTEST_F_L0(JSNApiTests, Float32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 4 : offset of byte, 6 : length
    Local<Float32ArrayRef> typedArray = Float32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsFloat32Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 24U); // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4U);  // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

HWTEST_F_L0(JSNApiTests, Float64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 57;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 8 : offset of byte, 6 : length
    Local<Float64ArrayRef> typedArray = Float64ArrayRef::New(vm_, arrayBuffer, 8, 6);
    ASSERT_TRUE(typedArray->IsFloat64Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U); // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);  // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

HWTEST_F_L0(JSNApiTests, BigInt64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 57;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 8 : offset of byte, 6 : length
    Local<BigInt64ArrayRef> typedArray = BigInt64ArrayRef::New(vm_, arrayBuffer, 8, 6);
    ASSERT_TRUE(typedArray->IsBigInt64Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U); // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);  // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

/**
 * @tc.number: ffi_interface_api_023
 * @tc.name: IsBigInt64Array
 * @tc.desc: Used to determine whether a given object is a BigInt64Array.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, BigUint64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 57;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));

    // 8 : offset of byte, 6 : length
    Local<BigUint64ArrayRef> typedArray = BigUint64ArrayRef::New(vm_, arrayBuffer, 8, 6);
    ASSERT_TRUE(typedArray->IsBigUint64Array(vm_));
    ASSERT_EQ(typedArray->ByteLength(vm_), 48U); // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8U);  // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6U); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

/**
 * @tc.number: ffi_interface_api_024
 * @tc.name: Error_ThrowException_HasPendingException
 * @tc.desc:
 * Error：Build error message
 * ThrowException：Throw an exception, error is the exception information
 * HasPendingException：Determine if there are any uncaught exceptions
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Error_ThrowException_HasPendingException)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    ASSERT_TRUE(error->IsError(vm_));

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, RangeError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::RangeError(vm_, message);
    ASSERT_TRUE(error->IsError(vm_));

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

/**
 * @tc.number: ffi_interface_api_025
 * @tc.name: TypeError
 * @tc.desc:Tested the ability to create and throw a type error exception, and verified whether the exception
 * was correctly recognized and handled.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, TypeError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::TypeError(vm_, message);
    ASSERT_TRUE(error->IsError(vm_));

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, ReferenceError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::ReferenceError(vm_, message);
    ASSERT_TRUE(error->IsError(vm_));

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, SyntaxError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::SyntaxError(vm_, message);
    ASSERT_TRUE(error->IsError(vm_));

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

/**
 * @tc.number: ffi_interface_api_026
 * @tc.name: OOMError
 * @tc.desc:Create and throw a memory overflow error exception function, and verify
 * whether the exception is correctly recognized and handled.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, OOMError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::OOMError(vm_, message);
    ASSERT_TRUE(error->IsError(vm_));

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_001)
{
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
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
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
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
    JSHandle<JSObject> sonObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(sonHandle), sonHandle);

    JSHandle<JSTaggedValue> fatherHandle = JSNApiHelper::ToJSHandle(weakSetLocal);
    JSHandle<JSObject> fatherObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(fatherHandle), fatherHandle);

    JSHandle<JSTaggedValue> sonMethod = JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>(sonObj), property1String);
    JSHandle<JSTaggedValue> fatherMethod =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>(fatherObj), property1String);
    bool same = JSTaggedValue::SameValue(sonMethod, fatherMethod);
    ASSERT_TRUE(same);
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_003)
{
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
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
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
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
    JSHandle<JSFunction> protoFunc = factory->NewJSFunctionByHClass(ctor, funcFuncProtoIntanceClass);
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
    JSHandle<JSFunction> noProtoFunc = factory->NewJSFunctionByHClass(ctor, funcFuncNoProtoProtoIntanceClass);
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
    Local<FunctionRef> cls = FunctionRef::NewClassFunction(vm_, FunctionCallback, nullptr, nullptr);

    JSHandle<JSTaggedValue> clsObj = JSNApiHelper::ToJSHandle(Local<JSValueRef>(cls));
    ASSERT_TRUE(clsObj->IsClassConstructor());

    JSTaggedValue accessor =
        JSHandle<JSFunction>(clsObj)->GetPropertyInlinedProps(JSFunction::CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX);
    ASSERT_TRUE(accessor.IsInternalAccessor());

    accessor = JSHandle<JSFunction>(clsObj)->GetPropertyInlinedProps(JSFunction::LENGTH_INLINE_PROPERTY_INDEX);
    ASSERT_TRUE(!accessor.IsUndefinedOrNull());
}

HWTEST_F_L0(JSNApiTests, WeakRefSecondPassCallback)
{
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
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

/**
 * @tc.number: ffi_interface_api_027
 * @tc.name: TriggerGC_OLD_GC
 * @tc.desc: GC trigger, gcType is the trigger type
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, TriggerGC_OLD_GC)
{
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
    vm_->SetEnableForceGC(false);
    auto globalEnv = vm_->GetGlobalEnv();
    auto factory = vm_->GetFactory();

    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    ASSERT_EQ("1", origin->ToString(vm_));

    JSHandle<JSTaggedValue> jsFunc = globalEnv->GetArrayFunction();
    JSHandle<JSObject> objVal1 = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
    JSHandle<JSObject> objVal2 = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
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
    gcType = JSNApi::TRIGGER_GC_TYPE::SHARED_GC;
    JSNApi::TriggerGC(vm_, gcType);
    gcType = JSNApi::TRIGGER_GC_TYPE::SHARED_FULL_GC;
    JSNApi::TriggerGC(vm_, gcType);

    EXPECT_EQ(taggedArray->GetIdx(objVal1.GetTaggedValue()), 0U);
    EXPECT_EQ(taggedArray->GetIdx(canBeGcValue), TaggedArray::MAX_ARRAY_INDEX);
    ASSERT_EQ("1", origin->ToString(vm_));

    vm_->SetEnableForceGC(true);
}

HWTEST_F_L0(JSNApiTests, Hint_GC)
{
    ecmascript::ThreadManagedScope managedScope(thread_);
    vm_->SetEnableForceGC(false);
    [[maybe_unused]] auto heap = const_cast<Heap *>(thread_->GetEcmaVM()->GetHeap());

#ifdef NDEBUG
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread_);
        for (int i = 0; i < 2049; i++) {
            [[maybe_unused]] JSHandle<TaggedArray> array = thread_->GetEcmaVM()->GetFactory()->NewTaggedArray(
                1024, JSTaggedValue::Hole(), MemSpaceType::SEMI_SPACE);
        }
    }
    size_t beforeSize = heap->GetHeapObjectSize();
#endif

    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    ASSERT_EQ("1", origin->ToString(vm_));

    // trigger gc
    JSNApi::MemoryReduceDegree degree = JSNApi::MemoryReduceDegree::LOW;
    JSNApi::HintGC(vm_, degree, GCReason::HINT_GC);
    degree = JSNApi::MemoryReduceDegree::MIDDLE;
    JSNApi::HintGC(vm_, degree, GCReason::HINT_GC);
    degree = JSNApi::MemoryReduceDegree::HIGH;
    JSNApi::HintGC(vm_, degree, GCReason::HINT_GC);

    ASSERT_EQ("1", origin->ToString(vm_));
#ifdef NDEBUG
    size_t afterSize = heap->GetHeapObjectSize();
    EXPECT_TRUE(afterSize < beforeSize);
#endif
    vm_->SetEnableForceGC(true);
}

/* @tc.number: ffi_interface_api_028
 * @tc.name: addWorker_DeleteWorker
 * @tc.desc:
 * addWorker：Using a WorkerVm as a parameter to modify the workInfo of the current vm
 * DeleteWorker：Delete WorkerVm
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, addWorker_DeleteWorker)
{
    std::thread t1([&]() {
        JSRuntimeOptions option;
        EcmaVM *workerVm = JSNApi::CreateEcmaVM(option);
        JSNApi::AddWorker(vm_, workerVm);
        bool hasDeleted = JSNApi::DeleteWorker(vm_, workerVm);
        JSNApi::DestroyJSVM(workerVm);
        EXPECT_TRUE(hasDeleted);
    });
    {
        ThreadSuspensionScope suspensionScope(thread_);
        t1.join();
    }

    bool hasDeleted = JSNApi::DeleteWorker(vm_, nullptr);
    EXPECT_FALSE(hasDeleted);
}

/**
 * @tc.number: ffi_interface_api_029
 * @tc.name: PrimitiveRef_GetValue
 * @tc.desc:Create an IntegerRef object with an initial value of 0
 * and test whether the GetValue method can correctly return the associated JSValueRef object.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, PrimitiveRef_GetValue)
{
    auto factory = vm_->GetFactory();
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
    EXPECT_EQ(intValue->Value(), 0);

    Local<JSValueRef> jsValue = intValue->GetValue(vm_);
    EXPECT_TRUE(*jsValue == nullptr);

    JSHandle<JSTaggedValue> nullHandle(thread_, JSTaggedValue::Null());
    JSHandle<JSHClass> jsClassHandle = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_PRIMITIVE_REF, nullHandle);
    JSHandle<JSTaggedValue> jsTaggedValue(factory->NewJSObjectWithInit(jsClassHandle));
    Local<PrimitiveRef> jsValueRef = JSNApiHelper::ToLocal<JSPrimitiveRef>(jsTaggedValue);
    EXPECT_TRUE(*(jsValueRef->GetValue(vm_)) != nullptr);
}

HWTEST_F_L0(JSNApiTests, BigIntRef_New_Uint64)
{
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    EXPECT_TRUE(maxBigintUint64->IsBigInt(vm_));

    JSHandle<BigInt> maxBigintUint64Val(thread_, JSNApiHelper::ToJSTaggedValue(*maxBigintUint64));
    EXPECT_EQ(maxBigintUint64Val->GetDigit(0), static_cast<uint32_t>(maxUint64 & 0xffffffff));
    EXPECT_EQ(maxBigintUint64Val->GetDigit(1), static_cast<uint32_t>((maxUint64 >> BigInt::DATEBITS) & 0xffffffff));

    uint64_t minUint64 = std::numeric_limits<uint64_t>::min();
    Local<BigIntRef> minBigintUint64 = BigIntRef::New(vm_, minUint64);
    EXPECT_TRUE(minBigintUint64->IsBigInt(vm_));

    JSHandle<BigInt> minBigintUint64Val(thread_, JSNApiHelper::ToJSTaggedValue(*minBigintUint64));
    EXPECT_EQ(minBigintUint64Val->GetDigit(0), static_cast<uint32_t>(minUint64 & 0xffffffff));
    EXPECT_EQ(minBigintUint64Val->GetLength(), 1U);
}

HWTEST_F_L0(JSNApiTests, BigIntRef_New_Int64)
{
    int64_t maxInt64 = std::numeric_limits<int64_t>::max();
    Local<BigIntRef> maxBigintInt64 = BigIntRef::New(vm_, maxInt64);
    EXPECT_TRUE(maxBigintInt64->IsBigInt(vm_));

    JSHandle<BigInt> maxBigintInt64Val(thread_, JSNApiHelper::ToJSTaggedValue(*maxBigintInt64));
    EXPECT_EQ(maxBigintInt64Val->GetDigit(0), static_cast<uint32_t>(maxInt64 & 0xffffffff));
    EXPECT_EQ(maxBigintInt64Val->GetDigit(1), static_cast<uint32_t>((maxInt64 >> BigInt::DATEBITS) & 0xffffffff));

    int64_t minInt64 = std::numeric_limits<int64_t>::min();
    Local<BigIntRef> minBigintInt64 = BigIntRef::New(vm_, minInt64);
    EXPECT_TRUE(minBigintInt64->IsBigInt(vm_));

    JSHandle<BigInt> minBigintInt64Val(thread_, JSNApiHelper::ToJSTaggedValue(*minBigintInt64));
    EXPECT_EQ(minBigintInt64Val->GetSign(), true);
    EXPECT_EQ(minBigintInt64Val->GetDigit(0), static_cast<uint32_t>((-minInt64) & 0xffffffff));
    EXPECT_EQ(minBigintInt64Val->GetDigit(1), static_cast<uint32_t>(((-minInt64) >> BigInt::DATEBITS) & 0xffffffff));
}

/**
 * @tc.number: ffi_interface_api_030
 * @tc.name: BigIntRef_CreateBigWords_GetWordsArray_GetWordsArraySize
 * @tc.desc:
 * IsBigInt：Determine if it is bigint
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
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
    EXPECT_TRUE(bigWords->IsBigInt(vm_));

    Local<BigIntRef> bigWordsRef(bigWords);
    EXPECT_EQ(bigWordsRef->GetWordsArraySize(vm_), size);

    JSHandle<BigInt> bigintUint64Val(thread_, JSNApiHelper::ToJSTaggedValue(*bigWords));
    EXPECT_EQ(bigintUint64Val->GetSign(), false);
    EXPECT_EQ(bigintUint64Val->GetLength(), size * MULTIPLE);

    bool resultSignBit = true;
    uint64_t *resultWords = new uint64_t[3](); // 3 : length of words array
    bigWordsRef->GetWordsArray(vm_, &resultSignBit, size, resultWords);
    EXPECT_EQ(resultSignBit, false);
    EXPECT_EQ(resultWords[0], words[0]);
    EXPECT_EQ(resultWords[1], words[1]);
    EXPECT_EQ(resultWords[2], words[2]);
    delete[] resultWords;
}

/**
 * @tc.number: ffi_interface_api_031
 * @tc.name: DateRef_New_ToString_GetTime_BigIntRef_CreateBigWords_GetWordsArray
 * @tc.desc:The purpose of testing is to verify whether the DateRef method correctly converts time to Date type
 * and converts Date type to string type, while also verifying whether its operation to obtain time is correct.
 * 			Used to verify the success of creating a BigIntRef object and obtaining a
 * word array of large integer objects.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, DateRef_New_ToString_GetTime)
{
    double time = 1.1;
    Local<DateRef> data = DateRef::New(vm_, time);
    EXPECT_TRUE(data->IsDate(vm_));

    Local<StringRef> tostring = data->ToString(vm_);
    Local<JSValueRef> toValue(tostring);
    EXPECT_TRUE(tostring->IsString(vm_));
    double dou = data->GetTime(vm_);
    EXPECT_EQ(dou, 1.1);
}

HWTEST_F_L0(JSNApiTests, PromiseRef_Finally)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> reject = FunctionRef::New(vm_, RejectCallback);
    Local<PromiseRef> catchPromise = promise->Finally(vm_, reject);
    ASSERT_TRUE(promise->IsPromise(vm_));
    ASSERT_TRUE(catchPromise->IsPromise(vm_));
    Local<PromiseRef> catchPromise1 = promise->Then(vm_, reject, reject);
    ASSERT_TRUE(catchPromise1->IsPromise(vm_));
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback);
    ASSERT_TRUE(!callback.IsEmpty());
    Local<PromiseRef> catchPromise2 = promise->Then(vm_, callback);
    ASSERT_TRUE(catchPromise2->IsPromise(vm_));
}

/*
 * @tc.number: ffi_interface_api_032
 * @tc.name: JSNApi_SerializeValue
 * @tc.desc: The main function of Undefined is to initialize some variables for subsequent testing,
 * testing the correctness and reliability of the JSNApi:: SerializeValue function,
 * and ensuring that it can serialize values correctly.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_SerializeValue)
{
    LocalScope scope(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback);
    ASSERT_TRUE(!callback.IsEmpty());
    std::vector<Local<JSValueRef>> arguments;
    arguments.emplace_back(JSValueRef::Undefined(vm_));
    Local<JSValueRef> result = callback->Call(vm_, JSValueRef::Undefined(vm_), arguments.data(), arguments.size());
    ASSERT_TRUE(result->IsArray(vm_));
    Local<ArrayRef> array(result);
    ASSERT_EQ(static_cast<uint64_t>(array->Length(vm_)), arguments.size());
    void *res = nullptr;
    res = JSNApi::SerializeValue(vm_, result, JSValueRef::Undefined(vm_), JSValueRef::Undefined(vm_), true);
    EXPECT_TRUE(res);
}

/*
 * @tc.number: ffi_interface_api_033
 * @tc.name: JSNApi_SetHostPromiseRejectionTracker_Call
 * @tc.desc: Can the host Promise reject callback function of the JavaScript virtual machine be set correctly.
 * @         Using the functions of Uint8Array and verifying if its attribute values are correct.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_SetHostPromiseRejectionTracker)
{
    void *data = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    // 设置 JavaScript 虚拟机的宿主 Promise 拒绝回调函数为 data 所指向的函数。
    JSNApi::SetHostPromiseRejectionTracker(vm_, data, data);
    // 首先获取GetJS虚拟机中的当前线程->GetCurrentEcmaContext获取当前上下文->从上下文中获取Promise拒绝回调函数
    PromiseRejectCallback res = vm_->GetJSThread()->GetCurrentEcmaContext()->GetPromiseRejectCallback();
    // 检查回调函数的地址是否等于我们之前设置的 data 的地址
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

/**
 * @tc.number: ffi_interface_api_034
 * @tc.name: ObjectRef_GetOwnEnumerablePropertyNames
 * @tc.desc:Use the GetOwnEnumerablePropertyNames method to obtain all enumerable property names of the object
 * and return an ArrayRef object.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_GetOwnEnumerablePropertyNames)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<ArrayRef> res = object->GetOwnEnumerablePropertyNames(vm_);
    ASSERT_TRUE(res->IsArray(vm_));
}

/**
 * @tc.number: ffi_interface_api_035
 * @tc.name: ObjectRef_SetNativePointerFieldCount_GetNativePointerFieldCount
 * @tc.desc:
 * SetNativePointerFieldCount：Set the count value of the local pointer field to count
 * GetNativePointerField：Get native pointer object
 * SetNativePointerField：Set native pointer properties, including pointers, callback methods,
 * data, and number of bindings
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_SetNativePointerFieldCount_GetNativePointerFieldCount)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    int32_t input = 34;
    object->SetNativePointerFieldCount(vm_, input);
    int32_t res = object->GetNativePointerFieldCount(vm_);
    ASSERT_EQ(res, input);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void *>(new std::string("test"));
    void *vp2 = static_cast<void *>(new std::string("test"));
    std::string *sp1 = static_cast<std::string *>(vp1);
    object->SetNativePointerField(vm_, 33, vp1, callBack, vp2);
    void *res1 = object->GetNativePointerField(vm_, 33);
    std::string *sp2 = static_cast<std::string *>(res1);
    ASSERT_EQ(sp1, sp2);
}

/**
 * @tc.number: ffi_interface_api_036
 * @tc.name: FunctionRef_GetFunctionPrototype_SetName_GetName
 * @tc.desc:Mainly used to verify the correctness of methods such as creating, obtaining prototypes,
 * setting names, and obtaining FunctionRef objects.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, FunctionRef_GetFunctionPrototype_SetName_GetName)
{
    LocalScope scope(vm_);
    NativePointerCallback deleter = nullptr;
    void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    bool callNative = true;
    size_t nativeBindingsize = 15;
    Local<FunctionRef> res =
        FunctionRef::NewClassFunction(vm_, FunctionCallback, deleter, cb, callNative, nativeBindingsize);
    ASSERT_TRUE(res->IsFunction(vm_));
    Local<JSValueRef> res1 = res->GetFunctionPrototype(vm_);
    ASSERT_TRUE(res->IsFunction(vm_));
    ASSERT_TRUE(!res1->IsArray(vm_));
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    res->SetName(vm_, origin);
    Local<StringRef> s = res->GetName(vm_);
    std::string str = s->ToString(vm_);
    ASSERT_EQ("1", str);
}

HWTEST_F_L0(JSNApiTests, JSNApi_SetAssetPath_GetAssetPath)
{
    LocalScope scope(vm_);
    std::string str = "/data/storage/el1/bundle/moduleName/ets/modules.abc";
    JSNApi::SetAssetPath(vm_, str);
    std::string res = JSNApi::GetAssetPath(vm_);
    ASSERT_EQ(str, res);
    void *data = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    JSNApi::SetLoop(vm_, data);
    void *res1 = vm_->GetLoop();
    ASSERT_EQ(res1, data);
}

/**
 * @tc.number: ffi_interface_api_037
 * @tc.name: SetAssetPath
 * @tc.desc:The resource file path used to verify the success of the setup program.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */

HWTEST_F_L0(JSNApiTests, JSValueRef_ToNativePointer)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);
    ASSERT_EQ(toString->ToNumber(vm_)->Value(), -123.3); // -123 : test case of input
    ASSERT_EQ(toString->ToBoolean(vm_)->Value(), true);
    ASSERT_EQ(toValue->ToString(vm_)->ToString(vm_), "-123.3");
    ASSERT_TRUE(toValue->ToObject(vm_)->IsObject(vm_));
    Local<NativePointerRef> res = toValue->ToNativePointer(vm_);
    ASSERT_TRUE(res->IsString(vm_));
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
    genObjHandleVal->SetGeneratorContext(thread_, generatorContextVal.GetTaggedValue());

    JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
    Local<GeneratorObjectRef> genObjectRef = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
    Local<JSValueRef> res = genObjectRef->GetGeneratorFunction(vm_);
    ASSERT_TRUE(res->IsGeneratorFunction(vm_));
}

/**
 * @tc.number: ffi_interface_api_038
 * @tc.name: BigIntToInt64
 * @tc.desc:Is the method of converting BigInt objects to 64 bit signed integers correct, and is it able to
 * handle lossless conversions correctly.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, BigIntToInt64)
{
    LocalScope scope(vm_);
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    EXPECT_TRUE(maxBigintUint64->IsBigInt(vm_));
    int64_t num = -11;
    int64_t num1 = num;
    bool lossless = true;
    maxBigintUint64->BigIntToInt64(vm_, &num, &lossless);
    EXPECT_TRUE(num != num1);
}

/**
 * @tc.number: ffi_interface_api_039
 * @tc.name: BigIntToUint64
 * @tc.desc:Is the method for converting BigInt objects to 64 bit unsigned integers correct and can lossless
 * conversions be handled correctly.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, BigIntToUint64)
{
    LocalScope scope(vm_);
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    EXPECT_TRUE(maxBigintUint64->IsBigInt(vm_));
    uint64_t num = -11;
    uint64_t num1 = num;
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
    EXPECT_TRUE(res->BooleaValue(vm_));
}

/**
 * @tc.number: ffi_interface_api_040
 * @tc.name: NewFromUnsigned
 * @tc.desc:Verify that the NewFromUnsigned method of IntegerRef can correctly create an IntegerRef object
 * representing unsigned integers, and that the value of the object is correct.
 * Value () method to obtain the value of this object, and then assert that this value is equal to
 * the input unsigned integer 1.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
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

/**
 * @tc.number: ffi_interface_api_041
 * @tc.name: IsBundle
 * @tc.desc: Determine if it is a type of Bundle
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsBundle)
{
    LocalScope scope(vm_);
    bool res = JSNApi::IsBundle(vm_);
    ASSERT_EQ(res, true);
}

/**
 * @tc.number: ffi_interface_api_042
 * @tc.name: ObjectRef_Delete
 * @tc.desc:MapRef_GetSize_GetTotalElements_Get_GetKey_GetValue_New_Set
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, MapRef_GetSize_GetTotalElements_Get_GetKey_GetValue_New_Set)
{
    LocalScope scope(vm_);
    Local<MapRef> map = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    map->Set(vm_, key, value);
    Local<JSValueRef> res = map->Get(vm_, key);
    ASSERT_EQ(res->ToString(vm_)->ToString(vm_), value->ToString(vm_)->ToString(vm_));
    int32_t num = map->GetSize(vm_);
    int32_t num1 = map->GetTotalElements(vm_);
    ASSERT_EQ(num, 1);
    ASSERT_EQ(num1, 1);
    Local<JSValueRef> res1 = map->GetKey(vm_, 0);
    ASSERT_EQ(res1->ToString(vm_)->ToString(vm_), key->ToString(vm_)->ToString(vm_));
    Local<JSValueRef> res2 = map->GetValue(vm_, 0);
    ASSERT_EQ(res2->ToString(vm_)->ToString(vm_), value->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, GetSourceCode)
{
    LocalScope scope(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback);
    bool res = callback->IsNative(vm_);
    EXPECT_TRUE(res);
}

/**
 * @tc.number: ffi_interface_api_043
 * @tc.name: ObjectRef_Delete_GetSourceCode
 * @tc.desc:Verify that the Delete method of the Object Ref object correctly deletes a property and that
 * the object no longer contains the property.
 * Using the functions of getsourcecode and verifying if its attribute values are correct.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
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


/**
 * @tc.number: ffi_interface_api_044
 * @tc.name: Has
 * @tc.desc: Used to verify whether a given check object has the specified properties.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_Set1)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);
    bool res = object->Set(vm_, 12, toValue);
    ASSERT_TRUE(res);
    Local<JSValueRef> res1 = object->Get(vm_, 12);
    ASSERT_EQ(res1->ToString(vm_)->ToString(vm_), toValue->ToString(vm_)->ToString(vm_));
}

HWTEST_F_L0(JSNApiTests, NativePointerRef_New)
{
    LocalScope scope(vm_);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void *>(new std::string("test"));
    void *vp2 = static_cast<void *>(new std::string("test"));
    Local<NativePointerRef> res = NativePointerRef::New(vm_, vp1, callBack, vp2, 0);
    ASSERT_EQ(res->Value(), vp1);
}


/**
 * @tc.number: ffi_interface_api_045
 * @tc.name: PromiseRejectInfo_GetData
 * @tc.desc:Construct a BufferRef function to determine whether it is a ObjectRef_Has_Delete
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
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

/**
 * @tc.number: ffi_interface_api_046
 * @tc.name: PromiseRejectInfo_GetData
 * @tc.desc:Mainly tested whether the GetData method of the PromiseRejectInfo object can correctly return
 * the incoming data, and whether the GetPromise and GetReason methods can correctly return Promise and the
 * reason for rejection.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, PromiseRejectInfo_GetData)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> promise(toString);
    Local<StringRef> toString1 = StringRef::NewFromUtf8(vm_, "123.3");
    Local<JSValueRef> reason(toString1);
    void *data = static_cast<void *>(new std::string("test"));
    // 创建一个PromiseRejectInfo对象,并传入被拒绝的Promise,拒绝的原因,拒绝事件类型以及自定义数据
    PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
    // 从上一步创建的PromiseRejectInfo对象中获取被拒绝的Promise,并在下面断言被拒绝的Promise与原始Promise相同
    Local<JSValueRef> promise_res = promisereject.GetPromise();
    // 获取拒绝的原因,并在下面断言拒绝原因与原始拒绝原因相同
    Local<JSValueRef> reason_res = promisereject.GetReason();
    ASSERT_EQ(promise_res->ToString(vm_)->ToString(vm_), promise->ToString(vm_)->ToString(vm_));
    ASSERT_EQ(reason_res->ToString(vm_)->ToString(vm_), reason->ToString(vm_)->ToString(vm_));
    // 获取自定义数据,并在下面断言自定义数据与传入的数据相同
    void *dataRes = promisereject.GetData();
    ASSERT_EQ(dataRes, data);
}

/**
 * @tc.number: ffi_interface_api_047
 * @tc.name: FunctionCallScope
 * @tc.desc:Create and use the function call scope function, and verify whether the depth of function calls is
 * correct when entering and exiting the scope.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, FunctionCallScope)
{
    {
        FunctionCallScope callScope(vm_);
        ASSERT_FALSE(vm_->IsTopLevelCallDepth());
    }
    ASSERT_TRUE(vm_->IsTopLevelCallDepth());
}

HWTEST_F_L0(JSNApiTests, AotTrigger)
{
    std::string bundle;
    std::string module;
    int32_t trigger = -1;
    JSNApi::SetRequestAotCallback(vm_,
        [&](const std::string &bundleName, const std::string &moduleName, int32_t triggerMode) -> bool {
            bundle = bundleName;
            module = moduleName;
            trigger = triggerMode;
            return 100;
        });
    ASSERT_FALSE(ecmascript::pgo::PGOProfilerManager::GetInstance()->RequestAot("com.test.test", "requestAot",
        RequestAotMode::RE_COMPILE_ON_IDLE));
    ASSERT_EQ(bundle, "com.test.test");
    ASSERT_EQ(module, "requestAot");
    ASSERT_EQ(trigger, 0);
}

HWTEST_F_L0(JSNApiTests, JSNApiInternalsTest)
{
#define CHECK_VALUE(VAL) ASSERT_EQ(JSValueRefInternals::VAL, JSTaggedValue::VAL)
    CHECK_VALUE(BIT_PER_BYTE);
    CHECK_VALUE(TAG_BITS_SIZE);
    CHECK_VALUE(TAG_BITS_SHIFT);
    CHECK_VALUE(TAG_MARK);
    CHECK_VALUE(TAG_INT);
    CHECK_VALUE(TAG_INT32_INC_MAX);
    CHECK_VALUE(TAG_INT32_DEC_MIN);
    CHECK_VALUE(TAG_OBJECT);
    CHECK_VALUE(TAG_WEAK);
    CHECK_VALUE(TAG_NULL);
    CHECK_VALUE(TAG_SPECIAL);
    CHECK_VALUE(TAG_BOOLEAN);
    CHECK_VALUE(TAG_EXCEPTION);
    CHECK_VALUE(TAG_OPTIMIZED_OUT);
    CHECK_VALUE(TAG_SPECIAL_MASK);
    CHECK_VALUE(TAG_BOOLEAN_MASK);
    CHECK_VALUE(TAG_HEAPOBJECT_MASK);
    CHECK_VALUE(TAG_WEAK_MASK);
    CHECK_VALUE(VALUE_HOLE);
    CHECK_VALUE(VALUE_NULL);
    CHECK_VALUE(VALUE_FALSE);
    CHECK_VALUE(VALUE_TRUE);
    CHECK_VALUE(VALUE_UNDEFINED);
    CHECK_VALUE(VALUE_EXCEPTION);
    CHECK_VALUE(VALUE_ZERO);
    CHECK_VALUE(VALUE_OPTIMIZED_OUT);
    CHECK_VALUE(INT_SIGN_BIT_OFFSET);
    CHECK_VALUE(DOUBLE_ENCODE_OFFSET_BIT);
    CHECK_VALUE(DOUBLE_ENCODE_OFFSET);
    CHECK_VALUE(VALUE_POSITIVE_ZERO);
    CHECK_VALUE(VALUE_NEGATIVE_ZERO);
#undef CHECK_VALUE
}

HWTEST_F_L0(JSNApiTests, JSNApiInternalsTestNumberRef)
{
    // double
    TestNumberRef(0., JSTaggedValue::DOUBLE_ENCODE_OFFSET);
    TestNumberRef(NAN, base::bit_cast<TaggedType>(ecmascript::base::NAN_VALUE) + JSTaggedValue::DOUBLE_ENCODE_OFFSET);

    // int32_t
    TestNumberRef(static_cast<int32_t>(0), JSTaggedValue::TAG_INT);
    TestNumberRef(INT32_MIN, static_cast<JSTaggedType>(INT32_MIN) | JSTaggedValue::TAG_INT);
    TestNumberRef(INT32_MAX, static_cast<JSTaggedType>(INT32_MAX) | JSTaggedValue::TAG_INT);

    // uint32_t
    TestNumberRef(static_cast<uint32_t>(0), JSTaggedValue::TAG_INT);
    TestNumberRef(static_cast<uint32_t>(INT32_MAX), static_cast<uint32_t>(INT32_MAX) | JSTaggedValue::TAG_INT);
    auto val = static_cast<uint32_t>(INT32_MAX + 1UL);
    TestNumberRef(val, ConvertDouble(static_cast<double>(val)));
    TestNumberRef(UINT32_MAX, ConvertDouble(static_cast<double>(UINT32_MAX)));

    // int64_t
    TestNumberRef(static_cast<int64_t>(INT32_MIN), static_cast<JSTaggedType>(INT32_MIN) | JSTaggedValue::TAG_INT);
    TestNumberRef(static_cast<int64_t>(INT32_MAX), static_cast<JSTaggedType>(INT32_MAX) | JSTaggedValue::TAG_INT);
    TestNumberRef(INT64_MIN, ConvertDouble(static_cast<double>(INT64_MIN)));
    TestNumberRef(INT64_MAX, ConvertDouble(static_cast<double>(INT64_MAX)));
}

HWTEST_F_L0(JSNApiTests, JSNApiInternalsTestBooleanRef)
{
    LocalScope scope(vm_);
    bool input = true;
    Local<BooleanRef> res = BooleanRef::New(vm_, input);
    EXPECT_TRUE(res->IsBoolean());
    EXPECT_TRUE(res->BooleaValue(vm_));
    ASSERT_EQ(JSNApiHelper::ToJSTaggedValue(*res).GetRawData(), JSTaggedValue::VALUE_TRUE);

    input = false;
    res = BooleanRef::New(vm_, input);
    EXPECT_TRUE(res->IsBoolean());
    EXPECT_FALSE(res->BooleaValue(vm_));
    ASSERT_EQ(JSNApiHelper::ToJSTaggedValue(*res).GetRawData(), JSTaggedValue::VALUE_FALSE);
}

HWTEST_F_L0(JSNApiTests, JSNApiInternalsTestNullUndefined)
{
    LocalScope scope(vm_);
    Local<JSValueRef> null = JSValueRef::Null(vm_);
    ASSERT_TRUE(null->IsNull());
    ASSERT_EQ(JSNApiHelper::ToJSTaggedValue(*null).GetRawData(), JSTaggedValue::VALUE_NULL);

    Local<JSValueRef> undefined = JSValueRef::Undefined(vm_);
    ASSERT_TRUE(undefined->IsUndefined());
    ASSERT_EQ(JSNApiHelper::ToJSTaggedValue(*undefined).GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
}

/**
 * @tc.number: ffi_interface_api_048
 * @tc.name: FunctionRef_New_GetFunctionPrototype
 * @tc.desc:The Inheritance Characteristics of Function References and the Function of Obtaining Function Headers
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, FunctionRef_New_GetFunctionPrototype)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> set = env->GetBuiltinsSetFunction();
    Local<FunctionRef> setLocal = JSNApiHelper::ToLocal<FunctionRef>(set);
    JSHandle<JSTaggedValue> map = env->GetBuiltinsMapFunction();
    Local<FunctionRef> mapLocal = JSNApiHelper::ToLocal<FunctionRef>(map);
    JSHandle<JSTaggedValue> setPrototype(thread_, JSHandle<JSFunction>::Cast(set)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> mapPrototype(thread_, JSHandle<JSFunction>::Cast(map)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> mapPrototypeProto(thread_, JSTaggedValue::GetPrototype(thread_, mapPrototype));
    bool same = JSTaggedValue::SameValue(setPrototype, mapPrototypeProto);
    ASSERT_FALSE(same);
    mapLocal->Inherit(vm_, setLocal);
    JSHandle<JSTaggedValue> sonHandle = JSNApiHelper::ToJSHandle(mapLocal);
    JSHandle<JSTaggedValue> sonPrototype(thread_, JSHandle<JSFunction>::Cast(sonHandle)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> sonPrototypeProto(thread_, JSTaggedValue::GetPrototype(thread_, sonPrototype));
    bool same2 = JSTaggedValue::SameValue(setPrototype, sonPrototypeProto);
    ASSERT_TRUE(same2);
    Local<FunctionRef> son1 = FunctionRef::New(vm_, FunctionCallback, nullptr);
    son1->Inherit(vm_, mapLocal);
    JSHandle<JSFunction> son1Handle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(son1));
    ASSERT_TRUE(son1Handle->HasFunctionPrototype());
}

/*
 * @tc.number: ffi_interface_api_049
 * @tc.name: PrintExceptionInfo
 * @tc.desc: Obtain and print abnormal information correctly.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, PrintExceptionInfo)
{
    LocalScope scope(vm_);
    std::thread t1([&](){
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        auto *vm = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm != nullptr) << "Cannot create Runtime";
        JSNApi::PrintExceptionInfo(vm);
        JSNApi::DestroyJSVM(vm);
    });
    {
        ThreadSuspensionScope suspensionScope(thread_);
        t1.join();
    }
}

/*
 * @tc.number: ffi_interface_api_050
 * @tc.name: IsNull
 * @tc.desc: Verify that localnull correctly represents a null value, ensuring that the JavaScript virtual machine
 * can handle null values correctly.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsNull)
{
    LocalScope scope(vm_);
    Local<JSValueRef> localNull = JSValueRef::Null(vm_);
    ASSERT_TRUE(localNull->IsNull());
}

/*
 * @tc.number: ffi_interface_api_051
 * @tc.name: IsNativePointer
 * @tc.desc: Verify that a NativePointerRef object created with a local pointer is correctly
 * recognized as a local pointer.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsNativePointer)
{
    LocalScope scope(vm_);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void *>(new std::string("test"));
    void *vp2 = static_cast<void *>(new std::string("test"));
    Local<NativePointerRef> res = NativePointerRef::New(vm_, vp1, callBack, vp2, 0);
    ASSERT_TRUE(res->IsNativePointer(vm_));
}

/*
 * @tc.number: ffi_interface_api_052
 * @tc.name: ToType_ToBoolean_ToString_ToObject
 * @tc.desc: Verify whether the ToType method of the JavaScript virtual machine can correctly convert string types to
 * the corresponding JavaScript data types.
 * Among them, there is the result of checking the string "-1.3" when it is converted to a Boolean value.
 * Check if the string wrapped in JSValueRef yields a result of "-1.3" when converted to a string.
 * Check if the string wrapped in JSValueRef actually becomes an object when converted to an object.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ToType_ToBoolean_ToString_ToObject)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-1.3");
    Local<JSValueRef> toValue(toString);

    ASSERT_EQ(toString->ToNumber(vm_)->Value(), -1.3);
    ASSERT_EQ(toString->ToBoolean(vm_)->Value(), true);
    ASSERT_EQ(toValue->ToString(vm_)->ToString(vm_), "-1.3");
    ASSERT_TRUE(toValue->ToObject(vm_)->IsObject(vm_));
}

/**
 * @tc.number: ffi_interface_api_053
 * @tc.name: IsTypedArray
 * @tc.desc:Verify that the TypedArray method correctly created a Uint32Array containing the specified
 * offset and length, and verify that its property values match expectations.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsTypedArray)
{
    LocalScope scope(vm_);
    std::string test = "abc";
    char buffer[4];
    memset_s(buffer, sizeof(buffer), 0, sizeof(buffer));
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());
    EXPECT_EQ(testString->WriteUtf8(vm_, buffer, 4), 4);
    // testString 是一个字符串，而不是类型化数组
    ASSERT_FALSE(testString->IsTypedArray(vm_));
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    // 是否是类型化数组
    ASSERT_TRUE(typedArray->IsTypedArray(vm_));
    ASSERT_FALSE(typedArray->IsUndefined());
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(vm_), arrayBuffer->GetBuffer(vm_));
}

HWTEST_F_L0(JSNApiTests, GetOriginalSource)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> regExpFunc = globalEnv->GetRegExpFunction();
    JSHandle<JSRegExp> jSRegExp =
        JSHandle<JSRegExp>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(regExpFunc), regExpFunc));
    jSRegExp->SetOriginalSource(thread, JSTaggedValue::Undefined());
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(JSHandle<JSTaggedValue>::Cast(jSRegExp));
    ASSERT_EQ(object->GetOriginalSource(vm_)->ToString(vm_), "");
}

HWTEST_F_L0(JSNApiTests, GetOriginalFlags)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> regExpFunc = globalEnv->GetRegExpFunction();
    JSHandle<JSRegExp> jSRegExp =
        JSHandle<JSRegExp>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(regExpFunc), regExpFunc));
    jSRegExp->SetOriginalFlags(thread, JSTaggedValue(RegExpParser::FLAG_GLOBAL | RegExpParser::FLAG_IGNORECASE |
                                                     RegExpParser::FLAG_MULTILINE | RegExpParser::FLAG_DOTALL |
                                                     RegExpParser::FLAG_UTF16 | RegExpParser::FLAG_STICKY));
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(JSHandle<JSTaggedValue>::Cast(jSRegExp));
    ASSERT_EQ(object->GetOriginalFlags(vm_), TEST_CHAR_STRING_FLAGS);
}

HWTEST_F_L0(JSNApiTests, GetGeneratorState)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> genFunc = env->GetGeneratorFunctionFunction();
    JSHandle<JSGeneratorObject> genObjHandleVal = factory->NewJSGeneratorObject(genFunc);
    genObjHandleVal->SetGeneratorState(JSGeneratorState::COMPLETED);
    JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
    Local<GeneratorObjectRef> object = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);

    ASSERT_EQ(object->GetGeneratorState(vm_)->ToString(vm_)->ToString(vm_), TEST_CHAR_STRING_STATE);
}
}  // namespace panda::test
