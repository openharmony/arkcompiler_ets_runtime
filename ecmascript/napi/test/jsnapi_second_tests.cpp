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
#include "ecmascript/checkpoint/thread_state_transition.h"
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
#include "ecmascript/js_typed_array.h"
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
#include "gtest/gtest.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;
static const char *TEST_KEY = "TestKey";
static const char *TEST_VALUE = "TestValue";
static const char *TEST_NUM1 = "-3.14";
static const char *TEST_NUM2 = "-123.3";
static const int INT_ONE = 1;
static const int INT_MINUS_1 = -1;
static const char *STR_TEST = "Test";
static const char *STR_NUM = "123";
constexpr char ARK_DEBUGGER_LIB_PATH[] = "LIBRARYPATH";

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

/**
 * @tc.number: ffi_interface_api_054
 * @tc.name: NumberRef_uint32_int64
 * @tc.desc:Define the variable input of type uint32_t and int64_t to determine whether the object is of numerical type
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, NumberRef_uint32_int64)
{
    uint32_t input = 32;
    int64_t input1 = 1;
    Local<NumberRef> res = NumberRef::New(vm_, input);
    Local<NumberRef> res1 = NumberRef::New(vm_, input1);
    ASSERT_TRUE(res->IsNumber());
    ASSERT_TRUE(res1->IsNumber());
}

/**
 * @tc.number: ffi_interface_api_055
 * @tc.name: NumberRef_int32_t_double
 * @tc.desc:Define the variable input of type int32 and double to determine whether the object is of numerical type
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, NumberRef_int32_t_double)
{
    int32_t input = -1;
    double input1 = 1.1;
    Local<NumberRef> res = NumberRef::New(vm_, input);
    Local<NumberRef> res1 = NumberRef::New(vm_, input1);
    ASSERT_TRUE(res->IsNumber());
    ASSERT_TRUE(res1->IsNumber());
}

/**
 * @tc.number: ffi_interface_api_056
 * @tc.name: ObjectRef_Freeze
 * @tc.desc:Execute a Freeze operation and call the JSObject:: SetIntegrity Level method to achieve immutability
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_Freeze)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    thread_ = vm_->GetJSThread();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> set = env->GetBuiltinsSetFunction();
    object->Freeze(vm_);
    ecmascript::ThreadManagedScope managedScope(thread_);
    bool status = JSObject::SetIntegrityLevel(thread_, JSHandle<JSObject>::Cast(set), IntegrityLevel::FROZEN);
    ASSERT_TRUE(status);
}

/**
 * @tc.number: ffi_interface_api_057
 * @tc.name: ObjectRef_Seal
 * @tc.desc:The function is to perform a closed (Seal) operation by calling the JSObject::SetIntegrity Level method
 * to ensure that its properties cannot be modified or deleted, and to set the integrity level of
 * the object to SEALED
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_Seal)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_); // 创建一个新的空对象
    thread_ = vm_->GetJSThread();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> set = env->GetBuiltinsSetFunction();
    object->Seal(vm_); // 尝试将对象封闭
    ecmascript::ThreadManagedScope managedScope(thread_);
    bool status = JSObject::SetIntegrityLevel(thread_, JSHandle<JSObject>::Cast(set), IntegrityLevel::SEALED);
    ASSERT_TRUE(status);
}

/**
 * @tc.number: ffi_interface_api_058
 * @tc.name: ObjectRef_GetAllPropertyNames
 * @tc.desc:Use the GetAllPropertyNames method to obtain all property names of the object and return an ArrayRef object.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_GetAllPropertyNames)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    uint32_t filter = 3;
    Local<ArrayRef> res = object->GetAllPropertyNames(vm_, filter);
    ASSERT_FALSE(res->IsBigInt(vm_));
    ASSERT_TRUE(res->IsArray(vm_));
}

/**
 * @tc.number: ffi_interface_api_059
 * @tc.name: GetIndex
 * @tc.desc:Call the GetIndex() function to obtain the index value and use EXCECT_ EQ()
 * assertion is used to verify whether the result is 0, which is the initial index value.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetIndex)
{
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    jsMap->SetLinkedMap(thread, hashMap);
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(jsMap);
    JSHandle<JSTaggedValue> jsMapIteratorTag4 = JSMapIterator::CreateMapIterator(thread, mapTag, IterationKind::VALUE);
    JSHandle<JSMapIterator> jsMapIterator4(jsMapIteratorTag4);
    Local<MapIteratorRef> mapIterator4 = JSNApiHelper::ToLocal<MapIteratorRef>(jsMapIteratorTag4);
    int32_t res4 = mapIterator4->GetIndex();
    EXPECT_EQ(0, res4);
}

/**
 * @tc.number: ffi_interface_api_060
 * @tc.name: GetKind_entries_values_keys
 * @tc.desc:This test case is mainly used to verify whether the GetKind method of the JSMapIterator object can
 * correctly return the expected type.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetKind_entries_values_keys)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ecmascript::ThreadManagedScope managedScope(thread);
    std::string expectedResult = "entries";
    std::string valuesResult = "values";
    std::string keysResult = "keys";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    jsMap->SetLinkedMap(thread, hashMap);
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(jsMap);
    JSHandle<JSTaggedValue> jsMapIteratorTag =
        JSMapIterator::CreateMapIterator(thread, mapTag, IterationKind::KEY_AND_VALUE);
    JSHandle<JSMapIterator> jsMapIterator(jsMapIteratorTag);
    EXPECT_EQ(JSTaggedValue::SameValue(jsMapIterator->GetIteratedMap(), jsMap->GetLinkedMap()), true);
    Local<MapIteratorRef> mapIterator = JSNApiHelper::ToLocal<MapIteratorRef>(jsMapIteratorTag);
    Local<JSValueRef> res = mapIterator->GetKind(vm_);
    EXPECT_EQ(expectedResult, res->ToString(vm_)->ToString(vm_));
    EXPECT_TRUE(mapIterator->IsMapIterator(vm_));
    JSHandle<JSTaggedValue> jsMapIteratorTag1 = JSMapIterator::CreateMapIterator(thread, mapTag, IterationKind::KEY);
    Local<MapIteratorRef> mapIterator1 = JSNApiHelper::ToLocal<MapIteratorRef>(jsMapIteratorTag1);
    Local<JSValueRef> res1 = mapIterator1->GetKind(vm_);
    EXPECT_EQ(keysResult, res1->ToString(vm_)->ToString(vm_));
    JSHandle<JSTaggedValue> jsMapIteratorTag2 = JSMapIterator::CreateMapIterator(thread, mapTag, IterationKind::VALUE);
    Local<MapIteratorRef> mapIterator2 = JSNApiHelper::ToLocal<MapIteratorRef>(jsMapIteratorTag2);
    Local<JSValueRef> res2 = mapIterator2->GetKind(vm_);
    EXPECT_EQ(valuesResult, res2->ToString(vm_)->ToString(vm_));
}

/**
 * @tc.number: ffi_interface_api_061
 * @tc.name: GetKind_001
 * @tc.desc:This test case is mainly used to verify whether the GetKind method of the JSMapIterator object can
 * correctly return the expected type (in this example, the expected type is "keys").
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetKind_001)
{
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    std::string keysResult = "keys";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    jsMap->SetLinkedMap(thread, hashMap);
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(jsMap);
    JSHandle<JSTaggedValue> jsMapIteratorTag1 = JSMapIterator::CreateMapIterator(thread, mapTag, IterationKind::KEY);
    JSHandle<JSMapIterator> jsMapIterator1(jsMapIteratorTag1);
    EXPECT_EQ(JSTaggedValue::SameValue(jsMapIterator1->GetIteratedMap(), jsMap->GetLinkedMap()), true);
    Local<MapIteratorRef> mapIterator1 = JSNApiHelper::ToLocal<MapIteratorRef>(jsMapIteratorTag1);
    Local<JSValueRef> res1 = mapIterator1->GetKind(vm_);
    EXPECT_EQ(keysResult, res1->ToString(vm_)->ToString(vm_));
}

/**
 * @tc.number: ffi_interface_api_062
 * @tc.name: GetKind_002
 * @tc.desc:This test case is mainly used to verify whether the GetKind method of the JSMapIterator object can
 * correctly return the expected type (in this example, the expected type is "values").
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetKind_002)
{
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    std::string valuesResult = "values";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    jsMap->SetLinkedMap(thread, hashMap);
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(jsMap);
    JSHandle<JSTaggedValue> jsMapIteratorTag2 = JSMapIterator::CreateMapIterator(thread, mapTag, IterationKind::VALUE);
    JSHandle<JSMapIterator> jsMapIterator2(jsMapIteratorTag2);
    EXPECT_EQ(JSTaggedValue::SameValue(jsMapIterator2->GetIteratedMap(), jsMap->GetLinkedMap()), true);
    Local<MapIteratorRef> mapIterator2 = JSNApiHelper::ToLocal<MapIteratorRef>(jsMapIteratorTag2);
    Local<JSValueRef> res2 = mapIterator2->GetKind(vm_);
    EXPECT_EQ(valuesResult, res2->ToString(vm_)->ToString(vm_));
}

/**
 * @tc.number: ffi_interface_api_063
 * @tc.name: GetKind_003
 * @tc.desc:Calling the GetKind method to obtain the iteration type KEY_AND_VALUE Compare with the string
 * variable 'entries' to assert whether the expected result is the same as the actual result
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetKind_003)
{
    ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    std::string expectedResult = "entries";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 从全局环境（GlobalEnv）中获取内置的Map构造函数
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    // 使用构造函数创建一个新的JSMap对象
    JSHandle<JSMap> jsMap =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    // 将创建的LinkedHashMap设置为JSMap的内部数据结构
    jsMap->SetLinkedMap(thread, hashMap);
    // 将jsMap转换为JSTaggedValue的句柄
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(jsMap);
    // 创建一个新的JSMapIterator对象，并将其迭代方式设置为KEY_AND_VALUE
    JSHandle<JSTaggedValue> jsMapIteratorTag =
        JSMapIterator::CreateMapIterator(thread, mapTag, IterationKind::KEY_AND_VALUE);
    JSHandle<JSMapIterator> jsMapIterator(jsMapIteratorTag);
    EXPECT_EQ(JSTaggedValue::SameValue(jsMapIterator->GetIteratedMap(), jsMap->GetLinkedMap()), true);
    // 将jsMapIteratorTag转换为本地代码可以使用的MapIteratorRef类型
    Local<MapIteratorRef> mapIterator = JSNApiHelper::ToLocal<MapIteratorRef>(jsMapIteratorTag);
    Local<JSValueRef> res = mapIterator->GetKind(vm_);
    EXPECT_EQ(expectedResult, res->ToString(vm_)->ToString(vm_));
    EXPECT_TRUE(mapIterator->IsMapIterator(vm_));
}

/*
 * @tc.number: ffi_interface_api_064
 * @tc.name: GetProperty_IsFunction
 * @tc.desc: Verify if the GetProperty function of the JavaScript virtual machine correctly returns a
 * function as the property of the 'Number' key.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetProperty_IsFunction)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject(vm_));

    Local<ObjectRef> key = StringRef::NewFromUtf8(vm_, "Number");
    Local<ObjectRef> property = globalObject->Get(vm_, key);
    ASSERT_TRUE(property->IsFunction(vm_));
}

/**
 * @tc.number: ffi_interface_api_065
 * @tc.name: NewClassFunction
 * @tc.desc:Check if the function created through the NewClassFunction method meets the specifications of the class
 * constructor, and obtain and verify the properties of the function.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, NewClassFunction)
{
    LocalScope scope(vm_);
    Local<FunctionRef> cls = FunctionRef::NewClassFunction(vm_, FunctionCallback, nullptr, nullptr);

    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(Local<JSValueRef>(cls));
    // 断言obj是一个类构造函数
    ASSERT_TRUE(obj->IsClassConstructor());
    // GetPropertyInlinedProps方法获取内联属性的方法,CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX类原型的内联属性索引
    JSTaggedValue res =
        JSHandle<JSFunction>(obj)->GetPropertyInlinedProps(JSFunction::CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX);
    // 断言获取的属性是一个内部访问器
    ASSERT_TRUE(res.IsInternalAccessor());
}

/**
 * @tc.number: ffi_interface_api_066
 * @tc.name: PromiseRef_Finally_IsPromise
 * @tc.desc:FunctionRef:: New Create a reject callback function reject. The function of the code is to test the
 * behavior of the Finally and Then methods of the Promise object
 * in various situations, ensuring that they all return Promise objects and meet the expected behavior.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, PromiseRef_Finally_IsPromise)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> gitpromise = capability->GetPromise(vm_);
    Local<FunctionRef> rejectcallback = FunctionRef::New(vm_, RejectCallback);
    Local<PromiseRef> catchPromise = gitpromise->Finally(vm_, rejectcallback);
    ASSERT_TRUE(gitpromise->IsPromise(vm_));
    ASSERT_TRUE(catchPromise->IsPromise(vm_));
    Local<PromiseRef> catchPromise2 = gitpromise->Then(vm_, rejectcallback, rejectcallback);
    ASSERT_TRUE(catchPromise2->IsPromise(vm_));
    Local<FunctionRef> functioncallback = FunctionRef::New(vm_, FunctionCallback);
    ASSERT_TRUE(!functioncallback.IsEmpty());
    Local<PromiseRef> catchPromise3 = gitpromise->Then(vm_, functioncallback);
    ASSERT_TRUE(catchPromise3->IsPromise(vm_));
}

/**
 * @tc.number: ffi_interface_api_067
 * @tc.name: IsBuffer
 * @tc.desc: Construct a BufferRef function to determine whether it is a Buffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsBuffer)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<BufferRef> buffer = BufferRef::New(vm_, length);
    ASSERT_TRUE(buffer->IsBuffer(vm_));
}

/**
 * @tc.number: ffi_interface_api_068
 * @tc.name: IsDataView
 * @tc.desc: Construct a BufferRef function to determine whether it is a dataView
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsDataView)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<DataViewRef> dataView = DataViewRef::New(vm_, arrayBuffer, 5, 7);
    ASSERT_TRUE(dataView->IsDataView(vm_));
}

/**
 * @tc.number: ffi_interface_api_071
 * @tc.name: IsSharedArrayBuffer
 * @tc.desc: Construct a BufferRef function to determine whether it is a SharedArrayBuffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsSharedArrayBuffer)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<JSValueRef> sharedArrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_FALSE(sharedArrayBuffer->IsSharedArrayBuffer(vm_));
}


/**
 * @tc.number: ffi_interface_api_072
 * @tc.name: IsTrue
 * @tc.desc: Construct a BufferRef function to determine whether it is a IsTrue
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsTrue)
{
    LocalScope scope(vm_);
    Local<JSValueRef> b = JSValueRef::True(vm_);
    ASSERT_TRUE(b->IsTrue());
}


/**
 * @tc.number: ffi_interface_api_073
 * @tc.name: IsFalse
 * @tc.desc: Construct a BufferRef function to determine whether it is a IsFalse
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsFalse)
{
    LocalScope scope(vm_);
    Local<JSValueRef> c = JSValueRef::False(vm_);
    ASSERT_TRUE(c->IsFalse());
}

/**
 * @tc.number: ffi_interface_api_074
 * @tc.name: IsConstructor
 * @tc.desc: Construct a BufferRef function to determine whether it is a Constructor
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsConstructor)
{
    LocalScope scope(vm_);
    Local<FunctionRef> target = FunctionRef::New(vm_, FunctionCallback);
    ASSERT_FALSE(target->IsConstructor(vm_));
}


/**
 * @tc.number: ffi_interface_api_076
 * @tc.name: GetOwnProperty
 * @tc.desc: Construct a BufferRef function to determine whether it is a GetOwnProperty  not Construct function
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetOwnProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, TEST_KEY);
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    Local<JSValueRef> value1 = object->Get(vm_, key);
    ASSERT_TRUE(value->IsStrictEquals(vm_, value1));
}

/**
 * @tc.number: ffi_interface_api_078
 * @tc.name: IsTreeMap
 * @tc.desc: Used to verify whether the given object is a TreeMap.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsTreeMap)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> mapClass = factory->NewEcmaHClass(JSAPITreeMap::SIZE, JSType::JS_API_TREE_MAP, proto);
    JSHandle<JSAPITreeMap> jsTreeMap = JSHandle<JSAPITreeMap>::Cast(factory->NewJSObjectWithInit(mapClass));
    JSHandle<TaggedTreeMap> treeMap(thread, TaggedTreeMap::Create(thread));
    jsTreeMap->SetTreeMap(thread, treeMap);
    JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jsTreeMap);
    Local<JSValueRef> TreeMap = JSNApiHelper::ToLocal<JSAPITreeSet>(argumentTag);
    EXPECT_TRUE(TreeMap->IsTreeMap(vm_));
}

/**
 * @tc.number: ffi_interface_api_079
 * @tc.name: IsTreeSet
 * @tc.desc: Used to verify whether the given object is a TreeSet.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsTreeSet)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> setClass = factory->NewEcmaHClass(JSAPITreeSet::SIZE, JSType::JS_API_TREE_SET, proto);
    JSHandle<JSAPITreeSet> jsTreeSet = JSHandle<JSAPITreeSet>::Cast(factory->NewJSObjectWithInit(setClass));
    JSHandle<TaggedTreeSet> treeSet(thread, TaggedTreeSet::Create(thread));
    jsTreeSet->SetTreeSet(thread, treeSet);
    JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jsTreeSet);
    Local<JSValueRef> TreeSet = JSNApiHelper::ToLocal<JSAPITreeSet>(argumentTag);
    EXPECT_TRUE(TreeSet->IsTreeSet(vm_));
}

/**
 * @tc.number: ffi_interface_api_080
 * @tc.name: IsVector
 * @tc.desc: Used to verify whether the given object is a Vector.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsVector)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> vectorClass = factory->NewEcmaHClass(JSAPIVector::SIZE, JSType::JS_API_VECTOR, proto);
    JSHandle<JSAPIVector> jsVector = JSHandle<JSAPIVector>::Cast(factory->NewJSObjectWithInit(vectorClass));
    jsVector->SetLength(0);
    JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jsVector);
    Local<JSValueRef> Vector = JSNApiHelper::ToLocal<JSAPIVector>(argumentTag);
    EXPECT_TRUE(Vector->IsVector(vm_));
}

/**
 * @tc.number: ffi_interface_api_081
 * @tc.name: IsJSArray
 * @tc.desc: Used to verify whether the given object is a JSArray.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsJSArray)
{
    LocalScope scope(vm_);
    JSHandle<JSTaggedValue> jsArrayTag = JSArray::ArrayCreate(vm_->GetJSThread(), JSTaggedNumber(0));
    Local<JSValueRef> jsArray = JSNApiHelper::ToLocal<JSTypedArray>(jsArrayTag);
    EXPECT_TRUE(jsArray->IsJSArray(vm_));
    Local<JSValueRef> array = JSNApiHelper::ToLocal<ArrayRef>(jsArrayTag);
    EXPECT_TRUE(array->IsArray(vm_));
}

/**
 * @tc.number: ffi_interface_api_082
 * @tc.name: IsMap
 * @tc.desc: Used to verify whether the given object is a map container.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsMap)
{
    LocalScope scope(vm_);
    Local<MapRef> map = MapRef::New(vm_);
    EXPECT_TRUE(map->IsMap(vm_));
}

/**
 * @tc.number: ffi_interface_api_083
 * @tc.name: IsSet
 * @tc.desc: Used to verify whether the given object is a Set container.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetRef_IsSet)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, hashSet);
    JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(set);
    Local<SetRef> Set = JSNApiHelper::ToLocal<SetRef>(setTag);
    EXPECT_TRUE(Set->IsSet(vm_));
}

/**
 * @tc.number: ffi_interface_api_084
 * @tc.name: ObjectRef_NEW
 * @tc.desc: Creating a new Object Ref object is not NULL if successfully created.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_NEW)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    ASSERT_FALSE(object.IsNull());
}

/**
 * @tc.number: ffi_interface_api_085
 * @tc.name: IsFalse
 * @tc.desc: Used to verify whether the given object is false.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNAPI_IsFalse)
{
    LocalScope scope(vm_);
    Local<PrimitiveRef> res = JSValueRef::False(vm_);
    EXPECT_TRUE(res->IsFalse());
}

/**
 * @tc.number: ffi_interface_api_086
 * @tc.name: GetDescription
 * @tc.desc: Used to verify whether the object used to obtain descriptive information was successful.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SymbolRef_GetDescription)
{
    LocalScope scope(vm_);
    Local<StringRef> description = StringRef::NewFromUtf8(vm_, "test");
    Local<SymbolRef> symbol = SymbolRef::New(vm_, description);
    Local<StringRef> desc = symbol->GetDescription(vm_);
    EXPECT_EQ(description->ToString(vm_), desc->ToString(vm_));
    EXPECT_FALSE(symbol.IsNull());
    EXPECT_FALSE(description.IsEmpty());
}

/**
 * @tc.number: ffi_interface_api_087
 * @tc.name: ArrayRefNew_uint32Length_SetValueAt_GetValueAt
 * @tc.desc: Used to verify whether obtaining the length of the array, setting the value of the specified
 * index position of the array, and obtaining the value of the specified index position of the
 * array were successful
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ArrayRefNew_uint32Length_SetValueAt_GetValueAt)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject(vm_));
    Local<ArrayRef> property = ArrayRef::New(vm_, 3); // 3 : length
    ASSERT_TRUE(property->IsArray(vm_));
    ASSERT_EQ(property->Length(vm_), 3); // 3 : test case of input
    uint32_t index = 1;
    Local<JSValueRef> value = ObjectRef::New(vm_);
    bool result = property->SetValueAt(vm_, globalObject, index, value);
    ASSERT_TRUE(result);
    Local<JSValueRef> value1 = property->GetValueAt(vm_, globalObject, index);
    ASSERT_FALSE(value1.IsNull());
}

/**
 * @tc.number: ffi_interface_api_088
 * @tc.name: WeakSetRef_GetSize_GetTotalElements_GetValue
 * @tc.desc: Used to verify whether the size of the weakset for obtaining settings, the total number of
 * elements obtained, and the value of the specified index position obtained were successful
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, WeakSetRef_GetSize_GetTotalElements_GetValue)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakSetFunction();
    JSHandle<JSWeakSet> weakSet =
        JSHandle<JSWeakSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    weakSet->SetLinkedSet(thread, hashSet);
    JSHandle<JSTaggedValue> weakSetTag = JSHandle<JSTaggedValue>::Cast(weakSet);
    Local<JSValueRef> set1 = JSNApiHelper::ToLocal<WeakSetRef>(weakSetTag);
    EXPECT_TRUE(set1->IsWeakSet(vm_));
    Local<WeakSetRef> set = JSNApiHelper::ToLocal<WeakSetRef>(weakSetTag);
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("value"));
    JSWeakSet::Add(thread, weakSet, value);
    int32_t num = set->GetSize(vm_);
    int32_t num1 = set->GetTotalElements(vm_);
    ASSERT_EQ(num, 1);
    ASSERT_EQ(num1, 1);
    Local<JSValueRef> res2 = set->GetValue(vm_, 0);
    ASSERT_EQ(res2->ToString(vm_)->ToString(vm_), "value");
}

/**
 * @tc.number: ffi_interface_api_089
 * @tc.name: GetOwnPropertyNames
 * @tc.desc: An array of self owned property names used to validate the acquisition of objects.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_GetOwnPropertyNames)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<ArrayRef> res = object->GetOwnPropertyNames(vm_);
    EXPECT_TRUE(res->IsArray(vm_));
}

/**
 * @tc.number: ffi_interface_api_090
 * @tc.name: SetBundle
 * @tc.desc: Used to verify whether the initialization resource package was successfully set.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetBundle)
{
    LocalScope scope(vm_);
    bool value = true;
    JSNApi::SetBundle(vm_, value);
    bool res = JSNApi::IsBundle(vm_);
    EXPECT_TRUE(res);
}

/**
 * @tc.number: ffi_interface_api_091
 * @tc.name: SetMockModuleList
 * @tc.desc: Used to verify whether the function of setting the map container module was successful.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_SetMockModuleList)
{
    LocalScope scope(vm_);
    std::map<std::string, std::string> str = { { TEST_NUM1, TEST_NUM2 } };
    JSNApi::SetMockModuleList(vm_, str);
    ASSERT_EQ(std::string(vm_->GetMockModule(TEST_NUM1)), TEST_NUM2);
}

/**
 * @tc.number: ffi_interface_api_092
 * @tc.name: SetSourceMapTranslateCallback
 * @tc.desc: Whether the source mapping translation callback function used to verify the settings.
 * was successfully set
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNAPI_SetSourceMapTranslateCallback)
{
    LocalScope scope(vm_);
    SourceMapTranslateCallback tag { nullptr };
    JSNApi::SetSourceMapTranslateCallback(vm_, tag);
    SourceMapTranslateCallback cur = vm_->GetSourceMapTranslateCallback();
    ASSERT_EQ(nullptr, cur);
}

/**
 * @tc.number: ffi_interface_api_093
 * @tc.name: ExecutePendingJob
 * @tc.desc: Used to verify whether the pending task has been successfully executed.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_ExecutePendingJob)
{
    LocalScope scope(vm_);
    JSNApi::ExecutePendingJob(vm_);
    bool res = EcmaVM::ConstCast(vm_)->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    EXPECT_TRUE(res);
}

/**
 * @tc.number: ffi_interface_api_096
 * @tc.name: IsRegExp
 * @tc.desc: Used to determine whether a given object is a regular expression.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsRegExp)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    jSRegExp->SetByteCodeBuffer(thread, JSTaggedValue::Undefined());
    jSRegExp->SetOriginalSource(thread, JSTaggedValue::Undefined());
    jSRegExp->SetGroupName(thread, JSTaggedValue::Undefined());
    jSRegExp->SetOriginalFlags(thread, JSTaggedValue(0));
    jSRegExp->SetLength(0);
    JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
    Local<JSValueRef> regexp = JSNApiHelper::ToLocal<JSRegExp>(argumentTag);
    EXPECT_TRUE(regexp->IsRegExp(vm_));
}

/**
 * @tc.number: ffi_interface_api_097
 * @tc.name: GetAndClearUncaughtException
 * @tc.desc: Used to verify the success of obtaining and clearing uncaught exceptions
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetAndClearUncaughtException)
{
    LocalScope scope(vm_);
    TryCatch tryCatch(vm_);
    EXPECT_FALSE(tryCatch.HasCaught());
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    EXPECT_TRUE(error->IsError(vm_));
    JSNApi::ThrowException(vm_, error);
    EXPECT_TRUE(vm_->GetJSThread()->HasPendingException());
    JSNApi::GetAndClearUncaughtException(vm_);
    EXPECT_FALSE(vm_->GetJSThread()->HasPendingException());
}

/**
 * @tc.number: ffi_interface_api_098
 * @tc.name: IsJSPrimitiveNumber
 * @tc.desc: Verify if the given JSValueRef object is a primativenumber.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsJSPrimitiveNumber)
{
    LocalScope scope(vm_);
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> jstagvalue;
    JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_NUMBER, jstagvalue);
    JSHandle<JSTaggedValue> jspri = JSHandle<JSTaggedValue>::Cast(jsprimitive);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspri);
    EXPECT_FALSE(object->IsJSPrimitiveNumber(vm_));
}


/**
 * @tc.number: ffi_interface_api_099
 * @tc.name: StringUtf16_NewFromUtf16_Length_WriteUtf16_01
 * @tc.desc:
 * NewFromUtf16：Create StringRef in UTF8 format
 * WriteUtf16：Write the value of StringRef to char16_ T array buffer, testing Chinese
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, StringUtf16_NewFromUtf16_Length_WriteUtf16_01)
{
    LocalScope scope(vm_);
    const char16_t *test = u"年度";
    Local<StringRef> testString = StringRef::NewFromUtf16(vm_, test);
    EXPECT_EQ(testString->Length(vm_), 2);              // 2 : length of testString("年度")
    char16_t buffer[3];                              // 3 : length of testString + 1
    EXPECT_EQ(testString->WriteUtf16(vm_, buffer, 2), 2); // 2 : length of testString("年度")
    GTEST_LOG_(WARNING) << "年度test =" << buffer;
    ASSERT_EQ(buffer[0], u'年');
    ASSERT_EQ(buffer[1], u'度');
}

/**
 * @tc.number: ffi_interface_api_100
 * @tc.name: StringUtf16_NewFromUtf16_Length_WriteUtf16_01
 * @tc.desc: Write the value of StringRef to char16_ T array buffer, testing non Chinese
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, StringUtf16_NewFromUtf16_Length_WriteUtf16_02)
{
    LocalScope scope(vm_);
    const char16_t *test = u"hello world!0?";
    Local<StringRef> testString = StringRef::NewFromUtf16(vm_, test);

    EXPECT_EQ(testString->Length(vm_), 14);
    char16_t buffer[15];                               // 15 : length of testString + 1
    EXPECT_EQ(testString->WriteUtf16(vm_, buffer, 14), 14); // 14 : length of testString("hello world!!!")
    ASSERT_EQ(buffer[0], u'h');
    ASSERT_EQ(buffer[13], u'?');
}

/**
 * @tc.number: ffi_interface_api_101
 * @tc.name: SetRef_IsSet_GetSize_GetTotalElements_GetValue
 * @tc.desc:
 * IsSet：Determine if it is a set container object
 * GetSize：Get set length size
 * GetTotalElements：Get the total number of set elements
 * GetValue：Obtain the element values of the set container according to the following table
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetRef_IsSet_GetSize_GetTotalElements_GetValue)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> jsSet =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    jsSet->SetLinkedSet(thread, hashSet);
    JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(jsSet);
    Local<SetRef> set = JSNApiHelper::ToLocal<SetRef>(setTag);
    EXPECT_TRUE(set->IsSet(vm_));
    JSHandle<JSTaggedValue> fristValue(factory->NewFromASCII("vlue1"));
    JSSet::Add(thread, jsSet, fristValue);
    JSSet::Add(thread, jsSet, fristValue);
    int32_t num = set->GetSize(vm_);
    int32_t num1 = set->GetTotalElements(vm_);
    ASSERT_EQ(num, 1);
    ASSERT_EQ(num1, 1);
    JSHandle<JSTaggedValue> secondValue(factory->NewFromASCII("vlue2"));
    JSSet::Add(thread, jsSet, secondValue);
    num = set->GetSize(vm_);
    num1 = set->GetTotalElements(vm_);
    ASSERT_EQ(num, 2);
    ASSERT_EQ(num1, 2);
    Local<JSValueRef> res1 = set->GetValue(vm_, 0);
    ASSERT_EQ(res1->ToString(vm_)->ToString(vm_), "vlue1");
    Local<JSValueRef> res2 = set->GetValue(vm_, 1);
    ASSERT_EQ(res2->ToString(vm_)->ToString(vm_), "vlue2");
}

static JSSet *CreateJSSet(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, hashSet);
    return JSSet::Cast(set.GetTaggedValue().GetTaggedObject());
}

/**
 * @tc.number: ffi_interface_api_102
 * @tc.name: JSSetIterator_IsSetIterator_GetIndex_GetKind
 * @tc.desc:
 * IsSetIterator：Determine if it is a set iterator
 * GetIndex：Gets the index of the subscript position currently pointed to by the set iterator
 * GetKind：Obtain the index type 'key/value/key_and_value' for the set iterator
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSSetIterator_IsSetIterator_GetIndex_GetKind)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSSet> jsSet(thread, CreateJSSet(thread));
    EXPECT_TRUE(*jsSet != nullptr);
    JSHandle<JSTaggedValue> jsTagSetIterator =
        JSSetIterator::CreateSetIterator(thread, JSHandle<JSTaggedValue>(jsSet), IterationKind::KEY);
    JSHandle<JSSetIterator> jsSetIterator1(jsTagSetIterator);
    EXPECT_EQ(JSTaggedValue::SameValue(jsSetIterator1->GetIteratedSet(), jsSet->GetLinkedSet()), true);
    Local<SetIteratorRef> setIterator = JSNApiHelper::ToLocal<SetIteratorRef>(jsTagSetIterator);
    EXPECT_TRUE(setIterator->IsSetIterator(vm_));
    EXPECT_EQ(setIterator->GetIndex(), 0U);
    Local<JSValueRef> resultKey = StringRef::NewFromUtf8(vm_, "keys");
    EXPECT_EQ(setIterator->GetKind(vm_)->ToString(vm_)->ToString(vm_), resultKey->ToString(vm_)->ToString(vm_));
}

/**
 * @tc.number: ffi_interface_api_103
 * @tc.name: JSValueRef_IsMapIterator
 * @tc.desc: Determine if it is a Map iterator
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsMapIterator)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    jsMap->SetLinkedMap(thread, hashMap);
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(jsMap);
    JSHandle<JSTaggedValue> jsMapIteratorTag =
        JSMapIterator::CreateMapIterator(thread, mapTag, IterationKind::KEY_AND_VALUE);
    JSHandle<JSMapIterator> jsMapIterator(jsMapIteratorTag);
    EXPECT_EQ(JSTaggedValue::SameValue(jsMapIterator->GetIteratedMap(), jsMap->GetLinkedMap()), true);
    Local<JSValueRef> mapIterator = JSNApiHelper::ToLocal<MapIteratorRef>(jsMapIteratorTag);
    EXPECT_TRUE(mapIterator->IsMapIterator(vm_));
}

/**
 * @tc.number: ffi_interface_api_104
 * @tc.name: JSValueRef_IsModuleNamespaceObject
 * @tc.desc: Determine if it is a module space object
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsModuleNamespaceObject)
{
    LocalScope scope(vm_);
    ObjectFactory *objectFactory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> module = objectFactory->NewSourceTextModule();
    JSHandle<LocalExportEntry> localExportEntry1 = objectFactory->NewLocalExportEntry();
    SourceTextModule::AddLocalExportEntry(thread_, module, localExportEntry1, 0, 2);
    JSHandle<LocalExportEntry> localExportEntry2 = objectFactory->NewLocalExportEntry();
    SourceTextModule::AddLocalExportEntry(thread_, module, localExportEntry2, 1, 2);
    JSHandle<TaggedArray> localExportEntries(thread_, module->GetLocalExportEntries());
    CString baseFileName = "a.abc";
    module->SetEcmaModuleFilenameString(baseFileName);
    ModuleManager *moduleManager = thread_->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->AddResolveImportedModule(baseFileName, module.GetTaggedValue());
    JSHandle<ModuleNamespace> np =
        ModuleNamespace::ModuleNamespaceCreate(thread_, JSHandle<JSTaggedValue>::Cast(module), localExportEntries);
    EXPECT_TRUE(ModuleNamespace::PreventExtensions());
    JSHandle<JSTaggedValue> moduleNamespaceTag = JSHandle<JSTaggedValue>::Cast(np);
    Local<JSValueRef> moduleNamespace = JSNApiHelper::ToLocal<ModuleNamespace>(moduleNamespaceTag);
    ASSERT_TRUE(moduleNamespace->IsModuleNamespaceObject(vm_));
}


/**
 * @tc.number: ffi_interface_api_105
 * @tc.name: IsHole
 * @tc.desc: Determine if it is a hole
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsHole)
{
    LocalScope scope(vm_);
    Local<JSValueRef> b = JSValueRef::Hole(vm_);
    ASSERT_TRUE(b->IsHole());
}


/**
 * @tc.number: ffi_interface_api_106
 * @tc.name: GetValueDouble
 * @tc.desc: Get value double from isNumber
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetValueDouble)
{
    LocalScope scope(vm_);
    bool isNumber = true;
    panda::JSValueRef* nativeValue = new JSValueRef();
    ASSERT_FALSE(nativeValue->GetValueDouble(isNumber));
    isNumber = false;
    ASSERT_FALSE(nativeValue->GetValueDouble(isNumber));
}


/**
 * @tc.number: ffi_interface_api_107
 * @tc.name: GetValueInt32
 * @tc.desc: Get value int32 from isNumber
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetValueInt32)
{
    LocalScope scope(vm_);
    bool isNumber = true;
    panda::JSValueRef* nativeValue = new JSValueRef();
    ASSERT_FALSE(nativeValue->GetValueInt32(isNumber));
    isNumber = false;
    ASSERT_FALSE(nativeValue->GetValueInt32(isNumber));
}


/**
 * @tc.number: ffi_interface_api_108
 * @tc.name: GetValueUint32
 * @tc.desc: Get value uint32 from isNumber
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetValueUint32)
{
    LocalScope scope(vm_);
    bool isNumber = true;
    panda::JSValueRef* nativeValue = new JSValueRef();
    ASSERT_FALSE(nativeValue->GetValueUint32(isNumber));
    isNumber = false;
    ASSERT_FALSE(nativeValue->GetValueUint32(isNumber));
}


/**
 * @tc.number: ffi_interface_api_109
 * @tc.name: GetValueInt64
 * @tc.desc: Get value int64 from isNumber
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetValueInt64)
{
    LocalScope scope(vm_);
    bool isNumber = true;
    panda::JSValueRef* nativeValue = new JSValueRef();
    ASSERT_FALSE(nativeValue->GetValueInt64(isNumber));
    isNumber = false;
    ASSERT_FALSE(nativeValue->GetValueInt64(isNumber));
}


/**
 * @tc.number: ffi_interface_api_110
 * @tc.name: GetValueBool
 * @tc.desc: Get value bool from isNumber
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetValueBool)
{
    LocalScope scope(vm_);
    bool isNumber = true;
    panda::JSValueRef* nativeValue = new JSValueRef();
    ASSERT_FALSE(nativeValue->GetValueBool(isNumber));
    isNumber = false;
    ASSERT_FALSE(nativeValue->GetValueBool(isNumber));
}


/**
 * @tc.number: ffi_interface_api_111
 * @tc.name: ToBigInt
 * @tc.desc: String to big int
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ToBigInt)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, TEST_NUM2);
    Local<JSValueRef> toValue(toString);
    EXPECT_FALSE(toValue->ToBigInt(vm_)->IsBigInt(vm_));
}


/**
 * @tc.number: ffi_interface_api_112
 * @tc.name: IsInt && WithinInt32
 * @tc.desc: Determine if it is int && Within Int32
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsInt)
{
    LocalScope scope(vm_);
    bool input = true;
    Local<IntegerRef> res = IntegerRef::New(vm_, input);
    EXPECT_TRUE(res->IsInt());
    EXPECT_TRUE(res->WithinInt32());
}


/**
 * @tc.number: ffi_interface_api_113
 * @tc.name: IsJSFunction
 * @tc.desc: Determine if it is JSFunction
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsJSFunction)
{
    LocalScope scope(vm_);
    bool input = true;
    Local<FunctionRef> res = IntegerRef::New(vm_, input);
    EXPECT_FALSE(res->IsJSFunction(vm_));
}


/**
 * @tc.number: ffi_interface_api_114
 * @tc.name: IsWeakRef
 * @tc.desc: Determine if it is JSFunction
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsWeakRef)
{
    LocalScope scope(vm_);
    Local<JSValueRef> tag = BooleanRef::New(vm_, true);
    EXPECT_FALSE(tag->IsWeakRef(vm_));
}


/**
 * @tc.number: ffi_interface_api_115
 * @tc.name: IsArrayIterator
 * @tc.desc: Determine if it is arrayIterator
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsArrayIterator)
{
    LocalScope scope(vm_);
    Local<JSValueRef> tag = ArrayRef::New(vm_, true);
    EXPECT_FALSE(tag->IsArrayIterator(vm_));
}


/**
 * @tc.number: ffi_interface_api_116
 * @tc.name: IsStringIterator
 * @tc.desc: Determine if it is string Iterator
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsStringIterator)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, TEST_NUM2);
    EXPECT_FALSE(toString->IsStringIterator(vm_));
    std::string str = TEST_NUM2;
    EXPECT_EQ(toString->DebuggerToString(vm_), str);
}


/**
 * @tc.number: ffi_interface_api_116
 * @tc.name: IsJSSharedInt8Array
 * @tc.desc: Determine if it is JSSharedArray
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsJSSharedInt8Array)
{
    LocalScope scope(vm_);
    Local<JSValueRef> tag = ArrayRef::New(vm_, true);
    EXPECT_FALSE(tag->IsJSSharedInt8Array(vm_));
    EXPECT_FALSE(tag->IsJSSharedUint8Array(vm_));
    EXPECT_FALSE(tag->IsJSSharedUint8ClampedArray(vm_));
    EXPECT_FALSE(tag->IsJSSharedInt16Array(vm_));
    EXPECT_FALSE(tag->IsJSSharedUint16Array(vm_));
    EXPECT_FALSE(tag->IsJSSharedInt32Array(vm_));
    EXPECT_FALSE(tag->IsJSSharedFloat32Array(vm_));
    EXPECT_FALSE(tag->IsJSSharedUint32Array(vm_));
}


/**
 * @tc.number: ffi_interface_api_117
 * @tc.name: IsNativeModuleFailureInfoObject
 * @tc.desc: Determine if it is nativeModuleFailureInfoObject
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsNativeModuleFailureInfoObject)
{
    LocalScope scope(vm_);
    Local<JSValueRef> tag = ArrayRef::New(vm_, true);
    EXPECT_FALSE(tag->IsNativeModuleFailureInfoObject(vm_));
    EXPECT_FALSE(tag->IsSendableArrayBuffer(vm_));
    EXPECT_FALSE(tag->IsJSLocale(vm_));
    EXPECT_FALSE(tag->IsJSDateTimeFormat(vm_));
    EXPECT_FALSE(tag->IsJSRelativeTimeFormat(vm_));
    EXPECT_FALSE(tag->IsJSIntl(vm_));
    EXPECT_FALSE(tag->IsJSNumberFormat(vm_));
    EXPECT_FALSE(tag->IsJSCollator(vm_));
    EXPECT_FALSE(tag->IsJSPluralRules(vm_));
    EXPECT_FALSE(tag->IsJSListFormat(vm_));
    EXPECT_FALSE(tag->IsAsyncGeneratorObject(vm_));
}


/**
 * @tc.number: ffi_interface_api_118
 * @tc.name: IsAsyncGeneratorObject
 * @tc.desc: Determine if it is asyncGeneratorObject
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsAsyncGeneratorObject)
{
    LocalScope scope(vm_);
    Local<JSValueRef> tag = ArrayRef::New(vm_, true);
    EXPECT_FALSE(tag->IsAsyncGeneratorObject(vm_));
    EXPECT_FALSE(tag->IsArrayList(vm_));
    EXPECT_FALSE(tag->IsDeque(vm_));
    EXPECT_FALSE(tag->IsHashMap(vm_));
    EXPECT_FALSE(tag->IsHashSet(vm_));
    EXPECT_FALSE(tag->IsLightWeightMap(vm_));
    EXPECT_FALSE(tag->IsLightWeightSet(vm_));
    EXPECT_FALSE(tag->IsLinkedList(vm_));
    EXPECT_FALSE(tag->IsLinkedListIterator(vm_));
    EXPECT_FALSE(tag->IsList(vm_));
    EXPECT_FALSE(tag->IsPlainArray(vm_));
    EXPECT_FALSE(tag->IsQueue(vm_));
    EXPECT_FALSE(tag->IsStack(vm_));
}


/**
 * @tc.number: ffi_interface_api_119
 * @tc.name: IsSendableObject
 * @tc.desc: Determine if it is sendableObject
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsSendableObject)
{
    LocalScope scope(vm_);
    Local<JSValueRef> value = ObjectRef::New(vm_);
    EXPECT_FALSE(value->IsSendableObject(vm_));
    EXPECT_FALSE(value->IsJSShared(vm_));
    EXPECT_FALSE(value->IsSharedArray(vm_));
    EXPECT_FALSE(value->IsSharedTypedArray(vm_));
    EXPECT_FALSE(value->IsSharedSet(vm_));
    EXPECT_FALSE(value->IsSharedMap(vm_));
    EXPECT_FALSE(value->IsSharedMapIterator(vm_));
    EXPECT_TRUE(value->IsHeapObject());
}


/**
 * @tc.number: ffi_interface_api_120
 * @tc.name: GetNativePointerValue
 * @tc.desc: GetNativePointerValue
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetNativePointerValue)
{
    LocalScope scope(vm_);
    Local<JSValueRef> value = ObjectRef::New(vm_);
    bool isNativePointer = true;
    EXPECT_FALSE(value->GetNativePointerValue(vm_, isNativePointer));
}


/**
 * @tc.number: ffi_interface_api_121
 * @tc.name: IsDetachedArraybuffer
 * @tc.desc: IsDetachedArraybuffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsDetachedArraybuffer)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<JSValueRef> sharedArrayBuffer = ArrayBufferRef::New(vm_, length);
    bool isNativePointer = true;
    EXPECT_FALSE(sharedArrayBuffer->IsDetachedArraybuffer(vm_, isNativePointer));
    sharedArrayBuffer->DetachedArraybuffer(vm_, isNativePointer);
    EXPECT_TRUE(isNativePointer);
}


/**
 * @tc.number: ffi_interface_api_122
 * @tc.name: MapRefGet
 * @tc.desc: MapRef Get
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, MapRefGet)
{
    LocalScope scope(vm_);
    Local<MapRef> object = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, TEST_KEY);
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, TEST_VALUE);
    EXPECT_FALSE(object->Has(vm_, key));
    object->Set(vm_, key, value);
    EXPECT_TRUE(object->Has(vm_, key));
    Local<JSValueRef> mapRef = object->Get(vm_, TEST_KEY);
    EXPECT_FALSE(object->Has(vm_, mapRef));
    object->Delete(vm_, key);
    object->GetEntries(vm_);
    object->GetKeys(vm_);
    object->GetValues(vm_);
    EXPECT_FALSE(object->Has(vm_, TEST_KEY));
    object->Clear(vm_);
}


/**
 * @tc.number: ffi_interface_api_123
 * @tc.name: SendableMapRef
 * @tc.desc: SendableMapRef test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SendableMapRef)
{
    LocalScope scope(vm_);
    Local<SendableMapRef> object = SendableMapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, TEST_KEY);
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, TEST_VALUE);
    EXPECT_FALSE(object->Has(vm_, key));
    object->Set(vm_, key, value);
    object->Set(vm_, TEST_KEY, value);
    EXPECT_TRUE(object->Has(vm_, key));
    Local<JSValueRef> sendableMapRef = object->Get(vm_, TEST_KEY);
    sendableMapRef = object->Get(vm_, key);
    EXPECT_FALSE(object->Has(vm_, sendableMapRef));
    Local<SendableMapIteratorRef> sendableMapIteratorRef = object->GetEntries(vm_);
    sendableMapIteratorRef->Next(vm_);
    object->GetKey(vm_, 0);
    object->GetKeys(vm_);
    object->GetValue(vm_, 0);
    object->GetValues(vm_);
    object->GetSize(vm_);
    object->GetTotalElements(vm_);
    EXPECT_TRUE(object->Has(vm_, TEST_KEY));
    object->Delete(vm_, key);
    object->Clear(vm_);
}


/**
 * @tc.number: ffi_interface_api_124
 * @tc.name: MapIteratorRef
 * @tc.desc: MapIteratorRef test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, MapIteratorRef)
{
    LocalScope scope(vm_);
    Local<MapRef> mapRef = MapRef::New(vm_);
    Local<MapIteratorRef> object = MapIteratorRef::New(vm_, mapRef);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, TEST_KEY);
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, TEST_VALUE);
    EXPECT_FALSE(object->Has(vm_, key));
    ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = object->GetEcmaRuntimeCallInfo(vm_);
    object->Next(vm_, ecmaRuntimeCallInfo);
    object->Next(vm_);
    object->Set(vm_, key, value);
    object->Set(vm_, TEST_KEY, value);
    EXPECT_TRUE(object->Has(vm_, key));
    Local<JSValueRef> mapIteratorRef = object->Get(vm_, TEST_KEY);
    mapIteratorRef = object->Get(vm_, key);
    object->Delete(vm_, key);
    EXPECT_FALSE(object->Has(vm_, mapIteratorRef));
}


/**
 * @tc.number: ffi_interface_api_125
 * @tc.name: SetIteratorRef
 * @tc.desc: SetIteratorRef test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetIteratorRef)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, hashSet);
    JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(set);
    Local<SetRef> setRef = JSNApiHelper::ToLocal<SetRef>(setTag);
    Local<MapIteratorRef> object = SetIteratorRef::New(vm_, setRef);
    EXPECT_TRUE(object->IsHeapObject());
}


/**
 * @tc.number: ffi_interface_api_126
 * @tc.name: PromiseRejectInfo
 * @tc.desc: PromiseRejectInfo test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, PromiseRejectInfo)
{
    LocalScope scope(vm_);
    Local<StringRef> toStringPromise = StringRef::NewFromUtf8(vm_, TEST_NUM1);
    Local<JSValueRef> promise(toStringPromise);
    Local<StringRef> toStringReason = StringRef::NewFromUtf8(vm_, TEST_NUM1);
    Local<JSValueRef> reason(toStringReason);
    void *data = static_cast<void *>(new std::string(TEST_VALUE));
    PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
    EXPECT_EQ(promisereject.GetOperation(), PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT);
}

/**
 * @tc.number: ffi_interface_api_127
 * @tc.name: ObjectRef
 * @tc.desc: ObjectRef test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    object = ObjectRef::NewS(vm_);
    std::string str = TEST_VALUE;
    object->CreateNativeModuleFailureInfo(vm_, str);
    bool input = true;
    Local<FunctionRef> res = IntegerRef::New(vm_, input);
    object->CreateAccessorData(vm_, res, res);
    uintptr_t addr = ObjectRef::NewObject(vm_);
    Local<StringRef> key = StringRef::NewFromUtf8(vm_, TEST_VALUE);
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute property(value, true, true, true);
    object->SetConcurrentNativePointerField(vm_, 0);
    EXPECT_FALSE(object->SetPrototype(vm_, object));
    EXPECT_FALSE(object->HasOwnProperty(vm_, key));
    EXPECT_FALSE(object->GetOwnProperty(vm_, key, property));
    EXPECT_NE(addr, 1);
}

/**
 * @tc.number: ffi_interface_api_128
 * @tc.name: SendableArrayBufferRef
 * @tc.desc: SendableArrayBufferRef test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SendableArrayBufferRef)
{
    LocalScope scope(vm_);
    Local<SendableArrayBufferRef> res = SendableArrayBufferRef::New(vm_, 0);
    const int32_t length = 33;
    res->GetBuffer(vm_);
    EXPECT_NE(res->ByteLength(vm_), length);
    res->Detach(vm_);
    EXPECT_NE(res->IsDetach(vm_), length);
}

/**
 * @tc.number: ffi_interface_api_129
 * @tc.name: JSNApiSetPkgAliasList
 * @tc.desc: Used to verify whether the function of setting the map container module was successful.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApiSetPkgAliasList)
{
    LocalScope scope(vm_);
    std::map<std::string, std::string> str = { { TEST_NUM1, TEST_NUM2 } };
    JSNApi::SetPkgAliasList(vm_, str);
    std::string moduleName = TEST_NUM1;
    EXPECT_EQ(std::string(vm_->GetPkgNameWithAlias(TEST_NUM1)), TEST_NUM2);
}

/**
 * @tc.number: ffi_interface_api_130
 * @tc.name: InitForConcurrentFunction
 * @tc.desc: Used to verify whether the function of setting the map container module was successful.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApiInitForConcurrentFunction)
{
    LocalScope scope(vm_);
    void *taskInfo = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    Local<JSValueRef> function = FunctionRef::New(vm_, FunctionCallback);
    EXPECT_FALSE(JSNApi::InitForConcurrentFunction(vm_, function, taskInfo));
}

/**
 * @tc.number: ffi_interface_api_131
 * @tc.name: GetStackBeforeCallNapiSuccess
 * @tc.desc: Used to verify whether the function of setting the map container module was successful.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetStackBeforeCallNapiSuccess)
{
    LocalScope scope(vm_);
    SourceMapCallback callback { nullptr };
    JSNApi::SetSourceMapCallback(vm_, callback);
    vm_->GetJSThread()->SetIsProfiling(false);
    bool getStackBeforeCallNapiSuccess = false;
    JSNApi::GetStackBeforeCallNapiSuccess(vm_, getStackBeforeCallNapiSuccess);
    JSNApi::GetStackAfterCallNapi(vm_);
    EXPECT_FALSE(getStackBeforeCallNapiSuccess);
}

/*
 * @tc.number: ffi_interface_api_132
 * @tc.name: PrintExceptionInfo
 * @tc.desc: Obtain and print abnormal information correctly.
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, PrintExceptionInfo)
{
    LocalScope scope(vm_);
    std::thread t1([&]() {
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        auto *vm2 = JSNApi::CreateJSVM(option);
        auto *thread = vm2->GetJSThread();
        thread->SetException(JSTaggedValue::Exception());
        EXPECT_NE(vm2, nullptr) << "Cannot create Runtime";
        JSNApi::PrintExceptionInfo(vm2);
        JSNApi::DestroyJSVM(vm2);
    });
    {
        ThreadSuspensionScope suspensionScope(thread_);
        t1.join();
    }
}

/**
 * @tc.number: ffi_interface_api_133
 * @tc.name: StartDebugger
 * @tc.desc: StartDebugger test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_StartDebugger)
{
    EcmaVM *vm = nullptr;
    JSNApi::DebugOption *option = new JSNApi::DebugOption();

    EXPECT_FALSE(JSNApi::StartDebugger(vm, *option));

    option->port = INT_MINUS_1;
    EXPECT_FALSE(JSNApi::StartDebugger(vm_, *option));

    option->port = INT_ONE;
    option->libraryPath = nullptr;
    EXPECT_FALSE(JSNApi::StartDebugger(vm_, *option));

    option->libraryPath = ARK_DEBUGGER_LIB_PATH;
    EXPECT_FALSE(JSNApi::StartDebugger(vm_, *option));
}

/**
 * @tc.number: ffi_interface_api_134
 * @tc.name: StartDebuggerForOldProcess
 * @tc.desc: StartDebuggerForOldProcess test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_StartDebuggerForOldProcess)
{
    EcmaVM *vm = nullptr;
    JSNApi::DebugOption *option = new JSNApi::DebugOption();
    DebuggerPostTask debuggerPostTask = {};

    EXPECT_FALSE(JSNApi::StartDebuggerForOldProcess(vm, *option, 1, debuggerPostTask));

    option->libraryPath = ARK_DEBUGGER_LIB_PATH;
    EXPECT_FALSE(JSNApi::StartDebuggerForOldProcess(vm_, *option, 1, debuggerPostTask));
}

/**
 * @tc.number: ffi_interface_api_135
 * @tc.name: StartDebuggerForSocketPair
 * @tc.desc: StartDebuggerForSocketPair test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_StartDebuggerForSocketPair)
{
    int tid = INT_MINUS_1;
    int socketfd = 0;

    EXPECT_FALSE(JSNApi::StartDebuggerForSocketPair(tid, socketfd));

    tid = 0;
    ecmascript::tooling::JsDebuggerManager *jsDebuggerManager = new ecmascript::tooling::JsDebuggerManager(vm_);
    ecmascript::tooling::JsDebuggerManager::AddJsDebuggerManager(tid, jsDebuggerManager);
    EXPECT_FALSE(JSNApi::StartDebuggerForSocketPair(tid, socketfd));
}

/**
 * @tc.number: ffi_interface_api_136
 * @tc.name: NotifyDebugMode
 * @tc.desc: NotifyDebugMode test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_NotifyDebugMode)
{
    int tid = INT_MINUS_1;
    EcmaVM *vm = nullptr;
    JSNApi::DebugOption *option = new JSNApi::DebugOption();
    int32_t instanceId = 0;
    DebuggerPostTask debuggerPostTask = {};
    bool debugApp = false;

    EXPECT_FALSE(JSNApi::NotifyDebugMode(tid, vm, *option));
    EXPECT_TRUE(JSNApi::NotifyDebugMode(tid, vm_, *option));

    debugApp = true;
    EXPECT_FALSE(JSNApi::NotifyDebugMode(tid, vm_, *option, instanceId, debuggerPostTask, debugApp));

    option->libraryPath = ARK_DEBUGGER_LIB_PATH;
    EXPECT_FALSE(JSNApi::NotifyDebugMode(tid, vm_, *option, instanceId, debuggerPostTask, debugApp));
}

/**
 * @tc.number: ffi_interface_api_137
 * @tc.name: StoreDebugInfo
 * @tc.desc: StoreDebugInfo test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_StoreDebugInfo)
{
    int tid = INT_MINUS_1;
    EcmaVM *vm = nullptr;
    JSNApi::DebugOption *option = new JSNApi::DebugOption();
    DebuggerPostTask debuggerPostTask = {};
    bool debugApp = false;

    EXPECT_FALSE(JSNApi::StoreDebugInfo(tid, vm, *option, debuggerPostTask, debugApp));
    EXPECT_FALSE(JSNApi::StoreDebugInfo(tid, vm_, *option, debuggerPostTask, debugApp));

    option->libraryPath = ARK_DEBUGGER_LIB_PATH;
    EXPECT_FALSE(JSNApi::StoreDebugInfo(tid, vm_, *option, debuggerPostTask, debugApp));
}

/**
 * @tc.number: ffi_interface_api_138
 * @tc.name: StopDebugger
 * @tc.desc: StopDebugger test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_StopDebugger)
{
    int tid = INT_MINUS_1;
    EcmaVM *vm = nullptr;

    EXPECT_FALSE(JSNApi::StopDebugger(vm));

    EXPECT_FALSE(JSNApi::StopDebugger(tid));
}

/**
 * @tc.number: ffi_interface_api_138
 * @tc.name: KeyIsNumber
 * @tc.desc: KeyIsNumber test
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_KeyIsNumber)
{
    EXPECT_TRUE(JSNApi::KeyIsNumber(STR_NUM));
    EXPECT_FALSE(JSNApi::KeyIsNumber(STR_TEST));
}
} // namespace panda::test