/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <ctime>
#include <sys/time.h>

#include "gtest/gtest.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_locale.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/containers/containers_hashset.h"
#include "ecmascript/containers/containers_lightweightmap.h"
#include "ecmascript/containers/containers_lightweightset.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/debugger/hot_reload_manager.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/module/js_module_record.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/tagged_list.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

static constexpr int NUM_COUNT = 10000;
static constexpr int TIME_UNIT = 1000000;
time_t g_timeFor = 0;
struct timeval g_beginTime;
struct timeval g_endTime;
time_t g_time1 = 0;
time_t g_time2 = 0;
time_t g_time = 0;

#define TEST_TIME(NAME)                                                                  \
    {                                                                                    \
        g_time1 = (g_beginTime.tv_sec * TIME_UNIT) + (g_beginTime.tv_usec);              \
        g_time2 = (g_endTime.tv_sec * TIME_UNIT) + (g_endTime.tv_usec);                  \
        g_time = g_time2 - g_time1;                                                      \
        GTEST_LOG_(INFO) << "name =" << #NAME << " = Time =" << int(g_time - g_timeFor); \
    }

namespace panda::test {
using BuiltinsFunction = ecmascript::builtins::BuiltinsFunction;

Local<JSValueRef> FunCallback(JsiRuntimeCallInfo *info)
{
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

class JSNApiSplTest : public testing::Test {
public:
    static void SetUpTestCase(void)
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase(void)
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        RuntimeOption option;
        option.SetLogLevel(LOG_LEVEL::ERROR);
        vm_ = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
        thread_ = vm_->GetJSThread();
        vm_->SetEnableForceGC(true);
        GTEST_LOG_(INFO) << "SetUp";
    }

    void TearDown() override
    {
        vm_->SetEnableForceGC(false);
        JSNApi::DestroyJSVM(vm_);
        GTEST_LOG_(INFO) << "TearDown";
    }

protected:
    JSThread *thread_ = nullptr;
    EcmaVM *vm_ = nullptr;
};

#ifndef UNUSED
#define UNUSED(X) (void)(X)
#endif

template <typename T> void FreeGlobalCallBack(void *ptr)
{
    T *i = static_cast<T *>(ptr);
    UNUSED(i);
}
template <typename T> void NativeFinalizeCallback(void *ptr)
{
    T *i = static_cast<T *>(ptr);
    delete i;
}

void CalculateForTime()
{
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
    }
    gettimeofday(&g_endTime, nullptr);
    time_t start = (g_beginTime.tv_sec * TIME_UNIT) + (g_beginTime.tv_usec);
    time_t end = (g_endTime.tv_sec * TIME_UNIT) + (g_endTime.tv_usec);
    g_timeFor = end - start;
    GTEST_LOG_(INFO) << "timefor = " << g_timeFor;
}

static JSTaggedValue JSLocaleCreateWithOptionTest(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetLocaleFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> languageKey = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionKey = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> scriptKey = thread->GlobalConstants()->GetHandledScriptString();
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("US"));
    JSHandle<JSTaggedValue> scriptValue(factory->NewFromASCII("Latn"));
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-Latn-US"));
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, languageKey, languageValue);
    JSObject::SetProperty(thread, optionsObj, regionKey, regionValue);
    JSObject::SetProperty(thread, optionsObj, scriptKey, scriptValue);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = ecmascript::builtins::BuiltinsLocale::LocaleConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)JSValueRef::False(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject_NewFromUtf8)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> obj(StringRef::NewFromUtf8(vm_, "-123.3"));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject_NewFromUtf16)
{
    LocalScope scope(vm_);
    CalculateForTime();
    char16_t data = 0Xdf06;
    Local<StringRef> obj = StringRef::NewFromUtf16(vm_, &data);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject_NumberRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    double num = 1.236;
    Local<NumberRef> numberObj(NumberRef::New(vm_, num));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)numberObj->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject_Int32_Max)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = std::numeric_limits<int32_t>::max();
    Local<NumberRef> obj = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject_Int32_Min)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = std::numeric_limits<int32_t>::min();
    Local<NumberRef> obj = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject_Int64_Max)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int64_t num = std::numeric_limits<int64_t>::max();
    Local<NumberRef> obj = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject_BigIntRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint64_t num = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> bigIntObj = BigIntRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)bigIntObj->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject_SymbolRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<SymbolRef> symbolObj(SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "-123.3")));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)symbolObj->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = 10;
    Local<NumberRef> obj(NumberRef::New(vm_, num));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->IsInt();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> obj(StringRef::NewFromUtf8(vm_, "-123.3"));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->IsInt();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt_Int64_Max)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int64_t num = std::numeric_limits<int64_t>::max();
    Local<NumberRef> obj = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->IsInt();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt_Double)
{
    LocalScope scope(vm_);
    CalculateForTime();
    double num = 1.235;
    Local<NumberRef> obj = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->IsInt();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsFunction_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    NativePointerCallback deleter = nullptr;
    void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    bool callNative = true;
    size_t nativeBindingSize = 15;
    Local<FunctionRef> obj(FunctionRef::NewClassFunction(vm_, FunCallback, deleter, cb, callNative, nativeBindingSize));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->IsFunction(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFunction);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsFunction_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    double num = 1.235;
    Local<NumberRef> obj = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->IsInt();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFunction);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsSet)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, hashSet);
    JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(set);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<SetRef>(setTag)->IsSet(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsSet);
}

HWTEST_F_L0(JSNApiSplTest, BooleanRef_New_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = -10000;
    Local<BooleanRef> obj;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        obj = BooleanRef::New(vm_, num);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BooleanRef::New);
}

HWTEST_F_L0(JSNApiSplTest, BooleanRef_New_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<BooleanRef> obj;
    bool flag = false;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        obj = BooleanRef::New(vm_, flag);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BooleanRef::New);
}

HWTEST_F_L0(JSNApiSplTest, BooleanRef_Value_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = -10000;
    Local<BooleanRef> obj(BooleanRef::New(vm_, num));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BooleanRef::Value);
}

HWTEST_F_L0(JSNApiSplTest, BooleanRef_Value_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<BooleanRef> obj = BooleanRef::New(vm_, 0);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BooleanRef::Value);
}

HWTEST_F_L0(JSNApiSplTest, IsGeneratorFunction_True)
{
    ObjectFactory *factory = vm_->GetFactory();
    auto env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> genFunc = env->GetGeneratorFunctionFunction();
    JSHandle<JSGeneratorObject> genObjHandleVal = factory->NewJSGeneratorObject(genFunc);
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    JSHandle<JSFunction> generatorFunc = JSHandle<JSFunction>::Cast(factory->NewJSObject(hclass));
    JSFunction::InitializeJSFunction(vm_->GetJSThread(), generatorFunc, FunctionKind::GENERATOR_FUNCTION);
    JSHandle<GeneratorContext> generatorContext = factory->NewGeneratorContext();
    generatorContext->SetMethod(vm_->GetJSThread(), generatorFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> generatorContextVal = JSHandle<JSTaggedValue>::Cast(generatorContext);
    genObjHandleVal->SetGeneratorContext(vm_->GetJSThread(), generatorContextVal.GetTaggedValue());
    JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
    Local<GeneratorObjectRef> genObjectRef = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
    Local<JSValueRef> genObject = genObjectRef->GetGeneratorFunction(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        genObject->IsGeneratorFunction(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsGeneratorFunction);
}

HWTEST_F_L0(JSNApiSplTest, IsGeneratorFunction_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<BooleanRef> obj = BooleanRef::New(vm_, 0);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        obj->IsGeneratorFunction(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsGeneratorFunction);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsArrayBuffer)
{
    static bool isFree = false;
    struct Data {
        int32_t length;
    };
    const int32_t length = 15;
    Data *data = new Data();
    data->length = length;
    NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
        delete[] reinterpret_cast<uint8_t *>(buffer);
        Data *currentData = reinterpret_cast<Data *>(data);
        delete currentData;
        isFree = true;
    };
    LocalScope scope(vm_);
    CalculateForTime();
    uint8_t *buffer = new uint8_t[length]();
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, buffer, length, deleter, data);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)arrayBuffer->IsArrayBuffer(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArrayBuffer);
}

HWTEST_F_L0(JSNApiSplTest, BufferRef_New1)
{
    static bool isFree = false;
    struct Data {
        int32_t length;
    };
    const int32_t length = 15;
    NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
        delete[] reinterpret_cast<uint8_t *>(buffer);
        Data *currentData = reinterpret_cast<Data *>(data);
        delete currentData;
        isFree = true;
    };
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        isFree = false;
        uint8_t *buffer = new uint8_t[length]();
        Data *data = new Data();
        data->length = length;
        (void)BufferRef::New(vm_, buffer, length, deleter, data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BufferRef::New);
}

HWTEST_F_L0(JSNApiSplTest, BufferRef_New2)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)BufferRef::New(vm_, length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BufferRef::New);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBuffer)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<BufferRef> bufferRef = BufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)bufferRef->IsBuffer(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBuffer);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUint32Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)typedArray->IsUint32Array(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBuIsUint32Arrayffer);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsFloat32Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Float32ArrayRef> typedArray = Float32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)typedArray->IsFloat32Array(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFloat32Array);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsFloat64Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Float64ArrayRef> floatArray = Float64ArrayRef::New(vm_, arrayBuffer, 4, 6);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)floatArray->IsFloat64Array(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFloat64Array);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_TypeOf_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)origin->Typeof(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::TypeOf);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_TypeOf_NumberRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<NumberRef> origin = NumberRef::New(vm_, 1);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)origin->Typeof(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::TypeOf);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_InstanceOf)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> origin = ObjectRef::New(vm_);
    JSHandle<GlobalEnv> globalEnv = vm_->GetGlobalEnv();
    JSHandle<JSFunction> constructor(globalEnv->GetObjectFunction());
    JSHandle<JSTaggedValue> arryListTag = JSHandle<JSTaggedValue>::Cast(constructor);
    Local<JSValueRef> value = JSNApiHelper::ToLocal<JSValueRef>(arryListTag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)origin->InstanceOf(vm_, value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::InstanceOf);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsArrayList)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetFunctionPrototype();
    JSHandle<JSHClass> arrayListClass = factory->NewEcmaHClass(JSAPIArrayList::SIZE, JSType::JS_API_ARRAY_LIST, proto);
    JSHandle<JSAPIArrayList> jsArrayList = JSHandle<JSAPIArrayList>::Cast(factory->NewJSObjectWithInit(arrayListClass));
    jsArrayList->SetLength(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> arryListTag = JSHandle<JSTaggedValue>::Cast(jsArrayList);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<JSValueRef>(arryListTag)->IsArrayList(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArrayList);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRef_Catch)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> reject = FunctionRef::New(vm_, FunCallback);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)promise->Catch(vm_, reject);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRef::Catch);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRef_Finally)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> reject = FunctionRef::New(vm_, FunCallback);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)promise->Finally(vm_, reject);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRef::Finally);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRef_Then)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunCallback);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)promise->Then(vm_, callback);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRef::Then);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRef_Then1)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunCallback);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)promise->Then(vm_, callback, callback);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRef::Then);
}

HWTEST_F_L0(JSNApiSplTest, PromiseCapabilityRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)PromiseCapabilityRef::New(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseCapabilityRef::New);
}

HWTEST_F_L0(JSNApiSplTest, PromiseCapabilityRef_GetPromise)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)capability->GetPromise(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseCapabilityRef::GetPromise);
}

HWTEST_F_L0(JSNApiSplTest, PromiseCapabilityRef_Resolve)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> resolve = FunctionRef::New(vm_, FunCallback);
    Local<FunctionRef> reject = FunctionRef::New(vm_, FunCallback);
    promise->Then(vm_, resolve, reject);
    Local<StringRef> value = NumberRef::New(vm_, 300.3);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)capability->Resolve(vm_, value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseCapabilityRef::Resolve);
}

HWTEST_F_L0(JSNApiSplTest, PromiseCapabilityRef_Reject)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> resolve = FunctionRef::New(vm_, FunCallback);
    Local<FunctionRef> reject = FunctionRef::New(vm_, FunCallback);
    promise->Then(vm_, resolve, reject);
    Local<StringRef> value = NumberRef::New(vm_, 300.3);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)capability->Reject(vm_, value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseCapabilityRef::Reject);
}

HWTEST_F_L0(JSNApiSplTest, ArrayBufferRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)ArrayBufferRef::New(vm_, length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayBufferRef::New);
}

HWTEST_F_L0(JSNApiSplTest, ArrayBufferRef_New1)
{
    static bool isFree = false;
    struct Data {
        int32_t length;
    };
    const int32_t length = 15;
    NativePointerCallback deleter = [](void *env, void *buffer, void *data) -> void {
        delete[] reinterpret_cast<uint8_t *>(buffer);
        Data *currentData = reinterpret_cast<Data *>(data);
        delete currentData;
        isFree = true;
    };
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        isFree = false;
        uint8_t *buffer = new uint8_t[length]();
        Data *data = new Data();
        data->length = length;
        (void)ArrayBufferRef::New(vm_, buffer, length, deleter, data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayBufferRef::New);
}

HWTEST_F_L0(JSNApiSplTest, ArrayBufferRef_ByteLength)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)arrayBuffer->ByteLength(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayBufferRef::ByteLength);
}

HWTEST_F_L0(JSNApiSplTest, ArrayBufferRef_Detach)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)arrayBuffer->Detach(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayBufferRef::Detach);
}

HWTEST_F_L0(JSNApiSplTest, ArrayBufferRef_IsDetach)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    arrayBuffer->Detach(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)arrayBuffer->IsDetach(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayBufferRef::IsDetach);
}

HWTEST_F_L0(JSNApiSplTest, BufferRef_ByteLength)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    Local<BufferRef> obj = BufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        obj->ByteLength(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayBufferRef::ByteLength);
}

HWTEST_F_L0(JSNApiSplTest, BufferRef_BufferRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30;
    Local<BufferRef> obj = BufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->GetBuffer(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BufferRef::BufferRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsArgumentsObject)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSArguments> obj = factory->NewJSArguments();
    JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(obj);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsArgumentsObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArgumentsObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsAsyncFunction)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSAsyncFuncObject> asyncFuncObj = factory->NewJSAsyncFuncObject();
    JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(asyncFuncObj);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsAsyncFunction(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsAsyncFunction);
}


HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSLocale)
{
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));
    JSHandle<JSTaggedValue> argumentTag = JSHandle<JSTaggedValue>::Cast(jsLocale);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsJSLocale(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSLocale);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsDeque)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> proto = thread->GetEcmaVM()->GetGlobalEnv()->GetFunctionPrototype();
    JSHandle<JSHClass> queueClass = factory->NewEcmaHClass(JSAPIDeque::SIZE, JSType::JS_API_DEQUE, proto);
    JSHandle<JSAPIQueue> jsQueue = JSHandle<JSAPIQueue>::Cast(factory->NewJSObjectWithInit(queueClass));
    JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIDeque::DEFAULT_CAPACITY_LENGTH);
    jsQueue->SetLength(thread, JSTaggedValue(0));
    jsQueue->SetFront(0);
    jsQueue->SetTail(0);
    jsQueue->SetElements(thread, newElements);
    JSHandle<JSTaggedValue> deQue = JSHandle<JSTaggedValue>::Cast(jsQueue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)JSNApiHelper::ToLocal<JSValueRef>(deQue)->IsDeque(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsDeque);
}

HWTEST_F_L0(JSNApiSplTest, DataView_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)DataViewRef::New(vm_, arrayBuffer, 5, 7);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(DataView::New);
}

HWTEST_F_L0(JSNApiSplTest, DataView_ByteLength)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<DataViewRef> dataView = DataViewRef::New(vm_, arrayBuffer, 5, 7);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)dataView->ByteLength();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(DataView::ByteLength);
}

HWTEST_F_L0(JSNApiSplTest, DataView_ByteOffset)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<DataViewRef> dataView = DataViewRef::New(vm_, arrayBuffer, 5, 7);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)dataView->ByteOffset();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(DataView::ByteOffset);
}

HWTEST_F_L0(JSNApiSplTest, DataView_GetArrayBuffer)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<DataViewRef> dataView = DataViewRef::New(vm_, arrayBuffer, 5, 7);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)dataView->GetArrayBuffer(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(DataView::GetArrayBuffer);
}

HWTEST_F_L0(JSNApiSplTest, TypedArrayRef_ByteLength)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Int8ArrayRef> obj = Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->ByteLength(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(TypedArrayRef::ByteLength);
}

HWTEST_F_L0(JSNApiSplTest, TypedArrayRef_ByteOffset)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Int8ArrayRef> obj = Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->ByteOffset(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(TypedArrayRef::ByteOffset);
}

HWTEST_F_L0(JSNApiSplTest, TypedArrayRef_ArrayLength)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Int8ArrayRef> obj = Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->ArrayLength(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(TypedArrayRef::ArrayLength);
}

HWTEST_F_L0(JSNApiSplTest, TypedArrayRef_GetArrayBuffer)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Int8ArrayRef> obj = Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->GetArrayBuffer(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(TypedArrayRef::GetArrayBuffer);
}

HWTEST_F_L0(JSNApiSplTest, Int8ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Int8ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, Uint8ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Uint8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Uint8ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, Uint8ClampedArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Uint8ClampedArrayRef::New(vm_, arrayBuffer, 5, 6);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Uint8ClampedArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, Int16ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Int16ArrayRef::New(vm_, arrayBuffer, 5, 6);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Int16ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, Uint16ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Uint16ArrayRef::New(vm_, arrayBuffer, 5, 6);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Uint16ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, Int32ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Int32ArrayRef::New(vm_, arrayBuffer, 5, 6);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Int32ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, Uint32ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Uint32ArrayRef::New(vm_, arrayBuffer, 5, 6);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Uint32ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, BufferRef_BufferToStringCallback)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSHandle<JSArrayBuffer> arrayBuffer = factory->NewJSArrayBuffer(10);
    JSHandle<JSTaggedValue> arryListTag = JSHandle<JSTaggedValue>::Cast(arrayBuffer);
    EcmaRuntimeCallInfo *objCallInfo =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, arryListTag, undefined, 1);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        BufferRef::BufferToStringCallback(objCallInfo);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BufferRef::BufferToStringCallback);
}

JSHandle<JSAPIHashMap> ConstructobjectHashMap(const EcmaVM *vm_)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
    JSHandle<JSTaggedValue> value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    objCallInfo->SetFunction(JSTaggedValue::Undefined());
    objCallInfo->SetThis(value.GetTaggedValue());
    objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::HashMap)));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSTaggedValue> constructor(thread, result);
    JSHandle<JSAPIHashMap> map(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    return map;
}

JSHandle<JSAPIHashSet> ConstructobjectHashSet(const EcmaVM *vm_)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
    JSHandle<JSTaggedValue> value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
    auto objCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    objCallInfo1->SetFunction(JSTaggedValue::Undefined());
    objCallInfo1->SetThis(value.GetTaggedValue());
    objCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::HashSet)));
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, objCallInfo1);
    JSTaggedValue result1 = containers::ContainersPrivate::Load(objCallInfo1);
    JSHandle<JSFunction> newTarget(thread, result1);
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ecmascript::containers::ContainersHashSet::HashSetConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSAPIHashSet> setHandle(thread, result);
    return setHandle;
}

JSHandle<JSAPILightWeightMap> ConstructobjectLightWeightMap(const EcmaVM *vm_)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
    JSHandle<JSTaggedValue> value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
    auto objCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    objCallInfo1->SetFunction(JSTaggedValue::Undefined());
    objCallInfo1->SetThis(value.GetTaggedValue());
    objCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::LightWeightMap)));
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, objCallInfo1);
    JSTaggedValue result1 = ecmascript::containers::ContainersPrivate::Load(objCallInfo1);
    JSHandle<JSFunction> newTarget(thread, result1);
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ecmascript::containers::ContainersLightWeightMap::LightWeightMapConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSAPILightWeightMap> mapHandle(thread, result);
    return mapHandle;
}

JSHandle<JSAPILightWeightSet> ConstructobjectLightWeightSet(const EcmaVM *vm_)
{
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
    JSHandle<JSTaggedValue> value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
    auto objCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    objCallInfo1->SetFunction(JSTaggedValue::Undefined());
    objCallInfo1->SetThis(value.GetTaggedValue());
    objCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::LightWeightSet)));
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, objCallInfo1);
    JSTaggedValue result1 = ecmascript::containers::ContainersPrivate::Load(objCallInfo1);
    JSHandle<JSFunction> newTarget(thread, result1);
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ecmascript::containers::ContainersLightWeightSet::LightWeightSetConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSAPILightWeightSet> mapHandle(thread, result);
    return mapHandle;
}

HWTEST_F_L0(JSNApiSplTest, IsStringIterator)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<EcmaString> recordName = vm_->GetFactory()->NewFromUtf8("646458");
    JSHandle<JSStringIterator> jsStringIter = JSStringIterator::CreateStringIterator(vm_->GetJSThread(), recordName);
    JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(jsStringIter);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<StringRef>(setTag)->IsStringIterator(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsStringIterator);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<StringRef>(setTag)->IsStringIterator(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUint8Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
    Local<Uint8ArrayRef> object = Uint8ArrayRef::New(vm_, buffer, 4, 5);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsUint8Array(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsUint8Array);
    GTEST_LOG_(INFO) << std::boolalpha << object->IsUint8Array(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt8Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
    Local<ObjectRef> object = Int8ArrayRef::New(vm_, buffer, 4, 5);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsInt8Array(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt8Array);
    GTEST_LOG_(INFO) << std::boolalpha << object->IsInt8Array(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBigInt64Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
    Local<ObjectRef> object = BigInt64ArrayRef::New(vm_, buffer, 4, 5);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsBigInt64Array(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt64Array);
    GTEST_LOG_(INFO) << std::boolalpha << object->IsBigInt64Array(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBigUint64Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
    Local<ObjectRef> object = BigUint64ArrayRef::New(vm_, buffer, 4, 5);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsBigUint64Array(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigUint64Array);
    GTEST_LOG_(INFO) << std::boolalpha << object->IsBigUint64Array(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPrimitiveRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    auto factory = vm_->GetFactory();
    int num = 0;
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    EXPECT_EQ(intValue->Value(), num);
    Local<JSValueRef> jsValue = intValue->GetValue(vm_);
    EXPECT_TRUE(*jsValue == nullptr);
    JSHandle<JSTaggedValue> nullHandle(thread_, JSTaggedValue::Null());
    JSHandle<JSHClass> jsClassHandle = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_PRIMITIVE_REF, nullHandle);
    TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
    JSHandle<JSTaggedValue> jsTaggedValue(thread_, JSTaggedValue(taggedObject));
    Local<PrimitiveRef> jsValueRef = JSNApiHelper::ToLocal<JSPrimitiveRef>(jsTaggedValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        jsValueRef->IsJSPrimitiveRef(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveRef);
    GTEST_LOG_(INFO) << std::boolalpha << jsValueRef->IsJSPrimitiveRef(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSDateTimeFormat)
{
    LocalScope scope(vm_);
    CalculateForTime();
    auto factory = vm_->GetFactory();
    int num = 0;
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    EXPECT_EQ(intValue->Value(), num);
    Local<JSValueRef> jsValue = intValue->GetValue(vm_);
    EXPECT_TRUE(*jsValue == nullptr);
    JSHandle<JSTaggedValue> nullHandle(thread_, JSTaggedValue::Null());
    JSHandle<JSHClass> jsClassHandle = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_DATE_TIME_FORMAT, nullHandle);
    TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
    JSHandle<JSTaggedValue> jsTaggedValue(thread_, JSTaggedValue(taggedObject));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSDateTimeFormat(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSDateTimeFormat);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSDateTimeFormat(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSRelativeTimeFormat)
{
    LocalScope scope(vm_);
    CalculateForTime();
    auto factory = vm_->GetFactory();
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
    EXPECT_EQ(intValue->Value(), 0);
    Local<JSValueRef> jsValue = intValue->GetValue(vm_);
    EXPECT_TRUE(*jsValue == nullptr);
    JSHandle<JSTaggedValue> nullHandle(thread_, JSTaggedValue::Null());
    JSHandle<JSHClass> jsClassHandle =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_RELATIVE_TIME_FORMAT, nullHandle);
    TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
    JSHandle<JSTaggedValue> jsTaggedValue(thread_, JSTaggedValue(taggedObject));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSRelativeTimeFormat(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSRelativeTimeFormat);
    GTEST_LOG_(INFO) << std::boolalpha <<
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSRelativeTimeFormat(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSIntl)
{
    LocalScope scope(vm_);
    CalculateForTime();
    auto factory = vm_->GetFactory();
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
    EXPECT_EQ(intValue->Value(), 0);
    Local<JSValueRef> jsValue = intValue->GetValue(vm_);
    EXPECT_TRUE(*jsValue == nullptr);
    JSHandle<JSTaggedValue> nullHandle(thread_, JSTaggedValue::Null());
    JSHandle<JSHClass> jsClassHandle = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_INTL, nullHandle);
    TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
    JSHandle<JSTaggedValue> jsTaggedValue(thread_, JSTaggedValue(taggedObject));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSIntl(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSIntl);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSIntl(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSNumberFormat)
{
    LocalScope scope(vm_);
    CalculateForTime();
    auto factory = vm_->GetFactory();
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
    EXPECT_EQ(intValue->Value(), 0);
    Local<JSValueRef> jsValue = intValue->GetValue(vm_);
    EXPECT_TRUE(*jsValue == nullptr);
    JSHandle<JSTaggedValue> nullHandle(thread_, JSTaggedValue::Null());
    JSHandle<JSHClass> jsClassHandle = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_NUMBER_FORMAT, nullHandle);
    TaggedObject *taggedObject = factory->NewObject(jsClassHandle);
    JSHandle<JSTaggedValue> jsTaggedValue(thread_, JSTaggedValue(taggedObject));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSNumberFormat(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSNumberFormat);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSNumberFormat(vm_);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsHashMap)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<JSAPIHashMap> map = ConstructobjectHashMap(vm_);
    JSHandle<JSTaggedValue> jsHashMap = JSHandle<JSTaggedValue>::Cast(map);
    Local<JSValueRef> tag = JSNApiHelper::ToLocal<JSValueRef>(jsHashMap);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(tag->IsHashMap(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsHashMap);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsHashSet)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<JSAPIHashSet> setHandle = ConstructobjectHashSet(vm_);
    JSHandle<JSTaggedValue> jsHashMap = JSHandle<JSTaggedValue>::Cast(setHandle);
    Local<JSValueRef> tag = JSNApiHelper::ToLocal<JSValueRef>(jsHashMap);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(tag->IsHashSet(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsHashSet);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsLightWeightMap)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<JSAPILightWeightMap> mapHandle = ConstructobjectLightWeightMap(vm_);
    JSHandle<JSTaggedValue> jsHashMap = JSHandle<JSTaggedValue>::Cast(mapHandle);
    Local<JSValueRef> tag = JSNApiHelper::ToLocal<JSValueRef>(jsHashMap);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(tag->IsLightWeightMap(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsLightWeightMap);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsLightWeightSet)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<JSAPILightWeightSet> mapHandle = ConstructobjectLightWeightSet(vm_);
    JSHandle<JSTaggedValue> jsHashMap = JSHandle<JSTaggedValue>::Cast(mapHandle);
    Local<JSValueRef> tag = JSNApiHelper::ToLocal<JSValueRef>(jsHashMap);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(tag->IsLightWeightSet(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsLightWeightSet);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_Default)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.Default();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::Default);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_PropertyAttribute)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        PropertyAttribute *p = new PropertyAttribute();
        delete p;
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::PropertyAttribute);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_PropertyAttribute1)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        PropertyAttribute *p = new PropertyAttribute(value, true, true, true);
        delete p;
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::PropertyAttribute);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_IsWritable)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, true);
    ASSERT_EQ(true, object.IsWritable());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = object.IsWritable();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::IsWritable);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_SetWritable)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, false, true, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.SetWritable(true);
        bool b = object.IsWritable();
        bool b1 = object.HasWritable();
        UNUSED(b);
        UNUSED(b1);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::SetWritable);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_IsEnumerable)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, true);
    ASSERT_EQ(true, object.IsEnumerable());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = object.IsEnumerable();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::IsEnumerable);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_SetEnumerable)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, false, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.SetEnumerable(true);
        bool b = object.IsEnumerable();
        bool b1 = object.HasEnumerable();
        UNUSED(b);
        UNUSED(b1);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::SetEnumerable);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_IsConfigurable)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, true);
    ASSERT_EQ(true, object.IsConfigurable());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = object.IsConfigurable();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::IsConfigurable);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_SetConfigurable)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, false);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.SetConfigurable(true);
        bool b = object.IsConfigurable();
        bool b1 = object.HasConfigurable();
        UNUSED(b);
        UNUSED(b1);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::SetConfigurable);
}

HWTEST_F_L0(JSNApiSplTest, HasWritable_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = object.HasWritable();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::HasWritable);
}

HWTEST_F_L0(JSNApiSplTest, HasConfigurable_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<PropertyAttribute> obj;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = obj->HasConfigurable();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::HasConfigurable);
}

HWTEST_F_L0(JSNApiSplTest, HasEnumerable_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = object.HasEnumerable();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::HasEnumerable);
}

HWTEST_F_L0(JSNApiSplTest, GetValue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.GetValue(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::GetValue);
}

HWTEST_F_L0(JSNApiSplTest, SetValue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.SetValue(value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::SetValue);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_HasValue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object(value, true, true, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)object.HasValue();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::HasValue);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_SetGetter)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.SetGetter(value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::SetGetter);
}

HWTEST_F_L0(JSNApiSplTest, PropertyAttribute_GetGetter)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = BooleanRef::New(vm_, true);
    PropertyAttribute object;
    object.SetGetter(value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)object.GetGetter(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::GetGetter);
}

HWTEST_F_L0(JSNApiSplTest, HasGetter)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object;
    object.SetGetter(value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.HasGetter();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::HasGetter);
}

HWTEST_F_L0(JSNApiSplTest, SetSetter)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.SetSetter(value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::SetSetter);
}

HWTEST_F_L0(JSNApiSplTest, GetSetter)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object;
    object.SetSetter(value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.GetSetter(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::GetSetter);
}

HWTEST_F_L0(JSNApiSplTest, HasSetter)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute object;
    object.SetGetter(value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.HasSetter();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PropertyAttribute::HasSetter);
}

HWTEST_F_L0(JSNApiSplTest, Float32ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = 4;
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, num);
    int32_t byteOffset = 4;
    int32_t length = 20;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Float32ArrayRef::New(vm_, buffer, byteOffset, length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Float32ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, Float64ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = 4;
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, num);
    int32_t byteOffset = 4;
    int32_t length = 20;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Float64ArrayRef::New(vm_, buffer, byteOffset, length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Float64ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, BigInt64ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = 4;
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, num);
    int32_t byteOffset = 4;
    int32_t length = 20;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        BigInt64ArrayRef::New(vm_, buffer, byteOffset, length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigInt64ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, BigUint64ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = 4;
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, num);
    int32_t byteOffset = 4;
    int32_t length = 20;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        BigUint64ArrayRef::New(vm_, buffer, byteOffset, length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigUint64ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, GetOriginalSource)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    jSRegExp->SetByteCodeBuffer(thread, JSTaggedValue::Undefined());
    jSRegExp->SetOriginalSource(thread, JSTaggedValue::Undefined());
    jSRegExp->SetGroupName(thread, JSTaggedValue::Undefined());
    jSRegExp->SetOriginalFlags(thread, JSTaggedValue(0));
    jSRegExp->SetLength(0);
    JSHandle<JSTaggedValue> jsRegTag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsRegTag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetOriginalSource(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(RegExpRef::GetOriginalSource);
}

HWTEST_F_L0(JSNApiSplTest, GetOriginalFlags)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    jSRegExp->SetByteCodeBuffer(thread, JSTaggedValue::Undefined());
    jSRegExp->SetOriginalSource(thread, JSTaggedValue::Undefined());
    jSRegExp->SetGroupName(thread, JSTaggedValue::Undefined());
    jSRegExp->SetOriginalFlags(thread, JSTaggedValue(0));
    jSRegExp->SetLength(0);
    JSHandle<JSTaggedValue> jsRegTag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsRegTag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetOriginalFlags(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(RegExpRef::GetOriginalFlags);
}

HWTEST_F_L0(JSNApiSplTest, IsGlobal)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSGlobalObject> globalObject = JSHandle<JSGlobalObject>::Cast(proto);
    JSHandle<JSTaggedValue> jsRegTag = JSHandle<JSTaggedValue>::Cast(globalObject);
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsRegTag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsGlobal(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(RegExpRef::IsGlobal);
}

HWTEST_F_L0(JSNApiSplTest, IsIgnoreCase)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    JSHandle<JSTaggedValue> jsRegTag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsRegTag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsIgnoreCase(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(RegExpRef::IsIgnoreCase);
}

HWTEST_F_L0(JSNApiSplTest, IsMultiline)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    JSHandle<JSTaggedValue> jsRegTag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsRegTag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsMultiline(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(RegExpRef::IsMultiline);
}

HWTEST_F_L0(JSNApiSplTest, IsDotAll)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    JSHandle<JSTaggedValue> jsregtag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsregtag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsDotAll(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(RegExpRef::IsDotAll);
}

HWTEST_F_L0(JSNApiSplTest, IsUtf16)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    JSHandle<JSTaggedValue> jsregtag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsregtag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsUtf16(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(RegExpRef::IsUtf16);
}

HWTEST_F_L0(JSNApiSplTest, IsStick)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = vm_->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> jSRegExpClass = factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    JSHandle<JSTaggedValue> jsregtag = JSHandle<JSTaggedValue>::Cast(jSRegExp);
    Local<RegExpRef> object = JSNApiHelper::ToLocal<RegExpRef>(jsregtag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsStick(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(RegExpRef::IsStick);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_BooleaValue_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = JSValueRef::True(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->BooleaValue(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_BooleaValue_true);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_BooleaValue_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = JSValueRef::False(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->BooleaValue(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_BooleaValue_false);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IntegerValue_Int64)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int64_t num = 0xffffffffffff; // 0xffffffffffff = A number exceeding 32 bits
    Local<JSValueRef> targetInt = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int64_t i64 = targetInt->IntegerValue(vm_);
        UNUSED(i64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IntegerValue_Int64);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IntegerValue_Double)
{
    LocalScope scope(vm_);
    CalculateForTime();
    double num = 123.456; // 123.456 = random number
    Local<JSValueRef> targetInt = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int64_t i64 = targetInt->IntegerValue(vm_);
        UNUSED(i64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IntegerValue_Double);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IntegerValue_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 789; // 789 = random number
    Local<JSValueRef> targetInt = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int64_t i64 = targetInt->IntegerValue(vm_);
        UNUSED(i64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IntegerValue_Int);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_Uint32Value)
{
    LocalScope scope(vm_);
    CalculateForTime();
    unsigned int num = 456; // 456 = random number
    Local<JSValueRef> targetUInt = IntegerRef::NewFromUnsigned(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t ui = targetUInt->Uint32Value(vm_);
        UNUSED(ui);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_Uint32Value);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToNativePointer_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *vp1 = static_cast<void *>(new std::string("test1"));
    Local<JSValueRef> tag = NativePointerRef::New(vm_, vp1);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<NativePointerRef> npr = tag->ToNativePointer(vm_);
        UNUSED(npr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_ToNativePointer_String);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToNativePointer_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *vp1 = static_cast<void *>(new int(123)); // 123 = random number
    Local<JSValueRef> tag = NativePointerRef::New(vm_, vp1);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<NativePointerRef> npr = tag->ToNativePointer(vm_);
        UNUSED(npr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_ToNativePointer_Int);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToNativePointer_Double)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *vp1 = static_cast<void *>(new double(123.456)); // 123.456 = random number
    Local<JSValueRef> tag = NativePointerRef::New(vm_, vp1);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<NativePointerRef> npr = tag->ToNativePointer(vm_);
        UNUSED(npr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_ToNativePointer_Double);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToNativePointer_Char)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *vp1 = static_cast<void *>(new char('a'));
    Local<JSValueRef> tag = NativePointerRef::New(vm_, vp1);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<NativePointerRef> npr = tag->ToNativePointer(vm_);
        UNUSED(npr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_ToNativePointer_Char);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToNativePointer_Long)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *vp1 = static_cast<void *>(new long(123456)); // 123456 = random number
    Local<JSValueRef> tag = NativePointerRef::New(vm_, vp1);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<NativePointerRef> npr = tag->ToNativePointer(vm_);
        UNUSED(npr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_ToNativePointer_Long);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUndefined_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> tag = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsUndefined();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsUndefined_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUndefined_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = JSValueRef::Undefined(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsUndefined();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsUndefined_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsNull_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> tag = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsNull();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsNull_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsNull_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = JSValueRef::Null(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsNull();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsNull_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_WithinInt32_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = StringRef::NewFromUtf8(vm_, "abcd");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->WithinInt32();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_WithinInt32_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_WithinInt32_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 456; // 456 = random number
    Local<JSValueRef> tag = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->WithinInt32();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_WithinInt32_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBoolean_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> tag = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsBoolean();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsBoolean_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBoolean_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = BooleanRef::New(vm_, false);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsBoolean();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsBoolean_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsString_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsString(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsString_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsString_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = StringRef::NewFromUtf8(vm_, "abc");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsString(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsString_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsProxy_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsProxy(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsProxy_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsProxy_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = ProxyRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsProxy(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsProxy_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsPromise_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsPromise(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsPromise_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsPromise_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = PromiseCapabilityRef::New(vm_)->GetPromise(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsPromise(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsPromise_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsDataView_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsDataView(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsDataView_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsDataView_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 0; // 0 = random number
    Local<JSValueRef> tag = DataViewRef::New(vm_, ArrayBufferRef::New(vm_, num), num, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsDataView(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsDataView_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsWeakRef_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsWeakRef(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsWeakRef_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsWeakMap_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsWeakMap(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsWeakMap_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsWeakMap_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = WeakMapRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsWeakMap(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsWeakMap_True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsWeakSet_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = JSValueRef::Null(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsWeakSet(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsWeakSet_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsWeakSet_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> tag = WeakSetRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsWeakSet(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsWeakSet_True);
}

HWTEST_F_L0(JSNApiSplTest, Global_Global)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> param;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Global<JSValueRef> global(param);
        UNUSED(global);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_Global);
}

HWTEST_F_L0(JSNApiSplTest, Global_OperatorEqual)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> param;
    Global<JSValueRef> global;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global = param;
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_OperatorEqual);
}

HWTEST_F_L0(JSNApiSplTest, Global_GlobalMove)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> param;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Global<JSValueRef> global(std::move(param));
        UNUSED(global);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_GlobalMove);
}

HWTEST_F_L0(JSNApiSplTest, Global_OperatorEqualMove)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> param;
    Global<JSValueRef> global;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global = std::move(param);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_OperatorEqualMove);
}

HWTEST_F_L0(JSNApiSplTest, Global_Global_VM_Local)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<BooleanRef> current = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Global<JSValueRef> global(vm_, current);
        UNUSED(global);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_Global_VM_Local);
}

HWTEST_F_L0(JSNApiSplTest, Global_Global_VM_Global)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<BooleanRef> current(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Global<JSValueRef> global(vm_, current);
        UNUSED(global);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_Global_VM_Global);
}

HWTEST_F_L0(JSNApiSplTest, Global_ToLocal)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<BooleanRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<JSValueRef> local = global.ToLocal();
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_ToLocal);
}

HWTEST_F_L0(JSNApiSplTest, Global_ToLocal_VM)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<BooleanRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<JSValueRef> local = global.ToLocal(vm_);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_ToLocal_VM);
}

HWTEST_F_L0(JSNApiSplTest, Global_Empty)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<BooleanRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.Empty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_Empty);
}

HWTEST_F_L0(JSNApiSplTest, Global_FreeGlobalHandleAddr)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<BooleanRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.FreeGlobalHandleAddr();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_FreeGlobalHandleAddr);
}

HWTEST_F_L0(JSNApiSplTest, Global_OperatorStar)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = (*global)->BooleaValue(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_OperatorStar);
}

HWTEST_F_L0(JSNApiSplTest, Global_OperatorPointTo)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = global->BooleaValue(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_OperatorPointTo);
}

HWTEST_F_L0(JSNApiSplTest, Global_IsEmpty_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = global.IsEmpty();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_IsEmpty_True);
}

HWTEST_F_L0(JSNApiSplTest, Global_IsEmpty_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = global.IsEmpty();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_IsEmpty_False);
}

HWTEST_F_L0(JSNApiSplTest, Global_SetWeak)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.SetWeak();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_SetWeak);
}

HWTEST_F_L0(JSNApiSplTest, Global_SetWeakCallback_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    void *ref = new int(123); // 123 = random number
    WeakRefClearCallBack freeGlobalCallBack = FreeGlobalCallBack<int>;
    WeakRefClearCallBack nativeFinalizeCallback = NativeFinalizeCallback<int>;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.SetWeakCallback(ref, freeGlobalCallBack, nativeFinalizeCallback);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_SetWeakCallback_Int);
}

HWTEST_F_L0(JSNApiSplTest, Global_SetWeakCallback_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    void *ref = new std::string("abc");
    WeakRefClearCallBack freeGlobalCallBack = FreeGlobalCallBack<std::string>;
    WeakRefClearCallBack nativeFinalizeCallback = NativeFinalizeCallback<std::string>;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.SetWeakCallback(ref, freeGlobalCallBack, nativeFinalizeCallback);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_SetWeakCallback_String);
}

HWTEST_F_L0(JSNApiSplTest, Global_SetWeakCallback_Double)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    void *ref = new double(123.456); // 123.456 = random number
    WeakRefClearCallBack freeGlobalCallBack = FreeGlobalCallBack<double>;
    WeakRefClearCallBack nativeFinalizeCallback = NativeFinalizeCallback<double>;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.SetWeakCallback(ref, freeGlobalCallBack, nativeFinalizeCallback);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_SetWeakCallback_Double);
}

HWTEST_F_L0(JSNApiSplTest, Global_SetWeakCallback_Char)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    void *ref = new char('a');
    WeakRefClearCallBack freeGlobalCallBack = FreeGlobalCallBack<char>;
    WeakRefClearCallBack nativeFinalizeCallback = NativeFinalizeCallback<char>;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.SetWeakCallback(ref, freeGlobalCallBack, nativeFinalizeCallback);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_SetWeakCallback_Char);
}

HWTEST_F_L0(JSNApiSplTest, Global_SetWeakCallback_Long)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    void *ref = new long(123456); // 123456 = random number
    WeakRefClearCallBack freeGlobalCallBack = FreeGlobalCallBack<long>;
    WeakRefClearCallBack nativeFinalizeCallback = NativeFinalizeCallback<long>;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.SetWeakCallback(ref, freeGlobalCallBack, nativeFinalizeCallback);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_SetWeakCallback_Long);
}

HWTEST_F_L0(JSNApiSplTest, Global_ClearWeak)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        global.ClearWeak();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_ClearWeak);
}

HWTEST_F_L0(JSNApiSplTest, Global_IsWeak_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = global.IsWeak();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_IsWeak_False);
}

HWTEST_F_L0(JSNApiSplTest, Global_IsWeak_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Global<JSValueRef> global(vm_, BooleanRef::New(vm_, true));
    global.SetWeak();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = global.IsWeak();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Global_IsWeak_True);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_Cast)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    JSValueRef *jsValue = (*local);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        StringRef *str = StringRef::Cast(jsValue);
        UNUSED(str);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_Cast);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_NewFromUtf8)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_NewFromUtf8);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_NewFromUtf8_0)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb", 0);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_NewFromUtf8_0);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_NewFromUtf8_3)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb", 3);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_NewFromUtf8_3);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_NewFromUtf16)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const char16_t *utf16 = u"您好，华为！";
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<StringRef> local = StringRef::NewFromUtf16(vm_, utf16);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_NewFromUtf16);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_NewFromUtf16_0)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const char16_t *utf16 = u"您好，华为！";
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<StringRef> local = StringRef::NewFromUtf16(vm_, utf16, 0);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_NewFromUtf16_0);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_NewFromUtf16_3)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const char16_t *utf16 = u"您好，华为！";
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<StringRef> local = StringRef::NewFromUtf16(vm_, utf16, 3);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_NewFromUtf16_3);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_ToString)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        std::string str = local->ToString(vm_);
        UNUSED(str);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_ToString);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_Length)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->Length(vm_);
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_Length);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_Utf8Length)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->Utf8Length(vm_);
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_Utf8Length);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_WriteUtf8)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    char cs[16] = {0};
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->WriteUtf8(vm_, cs, 6);
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_WriteUtf8);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_WriteUtf8_all)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    char cs[16] = {0}; // 16 = The size of the character array
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->WriteUtf8(vm_, cs, local->Length(vm_));
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_WriteUtf8_all);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_WriteUtf8_0)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    char cs[16] = {0}; // 16 = The size of the character array
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->WriteUtf8(vm_, cs, 0);
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_WriteUtf8_0);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_WriteUtf8_true)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    char cs[16] = {0}; // 16 =The size of the character array
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->WriteUtf8(vm_, cs, 6, true);
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_WriteUtf8_true);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_WriteUtf8_all_true)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    char cs[16] = {0}; // 16 = The size of the character array
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->WriteUtf8(vm_, cs, local->Length(vm_), true);
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_WriteUtf8_all_true);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_WriteUtf16)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf16(vm_, u"您好，华为！");
    char16_t cs[16] = {0}; // 16 = The size of the character array
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->WriteUtf16(vm_, cs, 3);
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_WriteUtf16);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_WriteLatin1)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> local = StringRef::NewFromUtf8(vm_, "abcdefbb");
    char cs[16] = {0}; // 16 = The size of the character array
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        uint32_t length = local->WriteLatin1(vm_, cs, 8);
        UNUSED(length);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_WriteLatin1);
}

HWTEST_F_L0(JSNApiSplTest, StringRef_GetNapiWrapperString)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<StringRef> local = StringRef::GetNapiWrapperString(vm_);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(StringRef_GetNapiWrapperString);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_IsMixedDebugEnabled)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = JSNApi::IsMixedDebugEnabled(vm_);
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_IsMixedDebugEnabled);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_NotifyNativeCalling_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *par = new int(0);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::NotifyNativeCalling(vm_, par);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_NotifyNativeCalling_Int);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_NotifyNativeCalling_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *par = new std::string("abc");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::NotifyNativeCalling(vm_, par);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_NotifyNativeCalling_String);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_NotifyNativeCalling_Char)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *par = new char('a');
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::NotifyNativeCalling(vm_, par);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_NotifyNativeCalling_Char);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_NotifyNativeCalling_Long)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *par = new long(123456); // 123456 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::NotifyNativeCalling(vm_, par);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_NotifyNativeCalling_Long);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_NotifyNativeCalling_Nullptr)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *par = nullptr;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::NotifyNativeCalling(vm_, par);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_NotifyNativeCalling_Nullptr);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SerializeValue_Bool)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = BooleanRef::New(vm_, true);
    Local<JSValueRef> transfer = JSValueRef::Undefined(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer, JSValueRef::Undefined(vm_));
        UNUSED(ptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SerializeValue_Bool);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SerializeValue_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> value = IntegerRef::New(vm_, num);
    Local<JSValueRef> transfer = JSValueRef::Undefined(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer, JSValueRef::Undefined(vm_));
        UNUSED(ptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SerializeValue_Int);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SerializeValue_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "abcdefbb");
    Local<JSValueRef> transfer = JSValueRef::Undefined(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer, JSValueRef::Undefined(vm_));
        UNUSED(ptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SerializeValue_String);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SerializeValue_String2)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "abcdefbb");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, JSValueRef::Undefined(vm_), JSValueRef::Undefined(vm_), true);
        UNUSED(ptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SerializeValue_String2);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SerializeValue_String3)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "abcdefbb");
    Local<JSValueRef> transfer = JSValueRef::Undefined(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer, JSValueRef::Undefined(vm_));
        UNUSED(ptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SerializeValue_String3);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SerializeValue_String_Bool)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "abcdefbb");
    Local<JSValueRef> transfer = JSValueRef::Undefined(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer, JSValueRef::Undefined(vm_));
        UNUSED(ptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SerializeValue_String_Bool);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    EcmaVM *vm2 = JSNApi::CreateJSVM(RuntimeOption());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, StringRef::NewFromUtf8(vm_, "abcdefbb"), JSValueRef::Undefined(vm_),
            JSValueRef::Undefined(vm_));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm2, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    JSNApi::DestroyJSVM(vm2);
    TEST_TIME(JSNApi_DeserializeValue_String);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_Bool)
{
    LocalScope scope(vm_);
    CalculateForTime();
    EcmaVM *vm2 = JSNApi::CreateJSVM(RuntimeOption());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, BooleanRef::New(vm_, true), JSValueRef::Undefined(vm_),
            JSValueRef::Undefined(vm_));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm2, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    JSNApi::DestroyJSVM(vm2);
    TEST_TIME(JSNApi_DeserializeValue_Bool);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    EcmaVM *vm2 = JSNApi::CreateJSVM(RuntimeOption());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, IntegerRef::New(vm_, 123), JSValueRef::Undefined(vm_),
            JSValueRef::Undefined(vm_));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm2, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    JSNApi::DestroyJSVM(vm2);
    TEST_TIME(JSNApi_DeserializeValue_Int);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_Undefined)
{
    LocalScope scope(vm_);
    CalculateForTime();
    EcmaVM *vm2 = JSNApi::CreateJSVM(RuntimeOption());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, JSValueRef::Undefined(vm_), JSValueRef::Undefined(vm_),
            JSValueRef::Undefined(vm_));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm2, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    JSNApi::DestroyJSVM(vm2);
    TEST_TIME(JSNApi_DeserializeValue_Undefined);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_Null)
{
    LocalScope scope(vm_);
    CalculateForTime();
    EcmaVM *vm2 = JSNApi::CreateJSVM(RuntimeOption());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder =
            JSNApi::SerializeValue(vm_, JSValueRef::Null(vm_), JSValueRef::Undefined(vm_), JSValueRef::Undefined(vm_));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm2, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    JSNApi::DestroyJSVM(vm2);
    TEST_TIME(JSNApi_DeserializeValue_Null);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeleteSerializationData_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *data = JSNApi::SerializeValue(vm_, StringRef::NewFromUtf8(vm_, "abcdefbb"),
            StringRef::NewFromUtf8(vm_, "abcdefbb"), JSValueRef::Undefined(vm_));
        JSNApi::DeleteSerializationData(data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeleteSerializationData_String);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeleteSerializationData_Bool)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *data = JSNApi::SerializeValue(vm_, BooleanRef::New(vm_, true), JSValueRef::Undefined(vm_),
            JSValueRef::Undefined(vm_));
        JSNApi::DeleteSerializationData(data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeleteSerializationData_Bool);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeleteSerializationData_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *data = JSNApi::SerializeValue(vm_, BooleanRef::New(vm_, true), JSValueRef::Undefined(vm_),
            JSValueRef::Undefined(vm_));
        JSNApi::DeleteSerializationData(data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeleteSerializationData_Int);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeleteSerializationData_Undefined)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *data = JSNApi::SerializeValue(vm_, JSValueRef::Undefined(vm_), JSValueRef::Undefined(vm_),
            JSValueRef::Undefined(vm_));
        JSNApi::DeleteSerializationData(data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeleteSerializationData_Undefined);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeleteSerializationData_Null)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *data =
            JSNApi::SerializeValue(vm_, JSValueRef::Null(vm_), JSValueRef::Undefined(vm_), JSValueRef::Undefined(vm_));
        JSNApi::DeleteSerializationData(data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeleteSerializationData_Null);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SetHostPromiseRejectionTracker)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *data = reinterpret_cast<void *>(builtins::BuiltinsFunction::FunctionPrototypeInvokeSelf);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetHostPromiseRejectionTracker(vm_, data, data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SetHostPromiseRejectionTracker);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SetHostResolveBufferTracker)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::function<bool(std::string dirPath, bool isHybrid, uint8_t * *buff, size_t * buffSize, std::string &errorMsg)>
        cb = [](const std::string &inputPath, bool isHybrid,
                uint8_t **buff, size_t *buffSize, std::string &errorMsg) -> bool {
        if (inputPath.empty() || buff == nullptr || buffSize == nullptr) {
            return false;
        }
        return true;
    };

    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetHostResolveBufferTracker(vm_, cb);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SetHostResolveBufferTracker);
}

void *NativePtrGetterCallback(void *info)
{
    return info;
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SetNativePtrGetter)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *cb = reinterpret_cast<void *>(NativePtrGetterCallback);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetNativePtrGetter(vm_, cb);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SetNativePtrGetter);
}

Local<JSValueRef> HostEnqueueJobCallback([[maybe_unused]] JsiRuntimeCallInfo *callBackFunc)
{
    Local<JSValueRef> local;
    return local;
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SetHostEnqueueJob)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> cb = FunctionRef::New(vm_, HostEnqueueJobCallback);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetHostEnqueueJob(vm_, cb);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SetHostEnqueueJob);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsMapIterator_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> builtinsMapFunc = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc));
    JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(thread_));
    jsMap->SetLinkedMap(thread_, linkedHashMap);
    JSHandle<JSTaggedValue> mapIteratorVal =
        JSMapIterator::CreateMapIterator(thread_, JSHandle<JSTaggedValue>::Cast(jsMap), IterationKind::KEY);
    Local<MapIteratorRef> object = JSNApiHelper::ToLocal<MapIteratorRef>(mapIteratorVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsMapIterator(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMapIterator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsMapIterator_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsMapIterator(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMapIterator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUint8ClampedArray_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t length = 4;     // 4 = length
    int32_t byteOffset = 0; // 0 = length
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, length);
    Local<Uint8ClampedArrayRef> object = Uint8ClampedArrayRef::New(vm_, buffer, byteOffset, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsUint8ClampedArray(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsUint8ClampedArray);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUint8ClampedArray_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsUint8ClampedArray(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsUint8ClampedArray);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt16Array_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30; // 30 = length
    int32_t byteOffset = 4;    // 4 = offset
    int32_t len = 6;           // 6 = length
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Int16ArrayRef> object = Int16ArrayRef::New(vm_, arrayBuffer, byteOffset, len);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsInt16Array(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt16Array);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt16Array_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsInt16Array(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt16Array);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPrimitiveNumber_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> jsTagValue;
    JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_NUMBER, jsTagValue);
    JSHandle<JSTaggedValue> jspri = JSHandle<JSTaggedValue>::Cast(jsprimitive);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspri);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsJSPrimitiveNumber(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveNumber);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPrimitiveNumber_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSPrimitiveNumber(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveNumber);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPrimitiveInt_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> jsTagValue;
    JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BIGINT, jsTagValue);
    JSHandle<JSTaggedValue> jspri = JSHandle<JSTaggedValue>::Cast(jsprimitive);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspri);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsJSPrimitiveInt(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveInt);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPrimitiveBoolean_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> jsTagValue;
    JSHandle<JSPrimitiveRef> jsprimitive = factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BOOLEAN, jsTagValue);
    JSHandle<JSTaggedValue> jspri = JSHandle<JSTaggedValue>::Cast(jsprimitive);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(jspri);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsJSPrimitiveBoolean(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveBoolean);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPrimitiveBoolean_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSPrimitiveBoolean(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveBoolean);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSCollator_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> ctor = env->GetCollatorFunction();
    JSHandle<JSCollator> collator =
        JSHandle<JSCollator>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSTaggedValue> localeStr = thread_->GlobalConstants()->GetHandledEnUsString();
    JSHandle<JSTaggedValue> undefinedHandle(thread_, JSTaggedValue::Undefined());
    JSHandle<JSCollator> initCollator = JSCollator::InitializeCollator(thread_, collator, localeStr, undefinedHandle);

    JSHandle<JSTaggedValue> collatorTagHandleVal = JSHandle<JSTaggedValue>::Cast(initCollator);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(collatorTagHandleVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsJSCollator(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSCollator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSCollator_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSCollator(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSCollator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPluralRules_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> optionHandle(thread_, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> ctor = env->GetPluralRulesFunction();
    JSHandle<JSPluralRules> pluralRules =
        JSHandle<JSPluralRules>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSTaggedValue> localeStr(factory->NewFromASCII("en-GB"));
    JSHandle<JSPluralRules> initPluralRules =
        JSPluralRules::InitializePluralRules(thread_, pluralRules, localeStr, optionHandle);
    JSHandle<JSTaggedValue> tagPlureRules = JSHandle<JSTaggedValue>::Cast(initPluralRules);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(tagPlureRules);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsJSPluralRules(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPluralRules);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPluralRules_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSPluralRules(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPluralRules);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSListFormat_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSListFormat(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSListFormat);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsAsyncGeneratorFunction_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    MethodLiteral *methodLiteral = nullptr;
    JSHandle<Method> method = factory->NewSMethod(methodLiteral);
    JSHandle<JSFunction> asyncGeneratorFunction = factory->NewJSAsyncGeneratorFunction(method);
    JSHandle<JSTaggedValue> asyncgefu = JSHandle<JSTaggedValue>::Cast(asyncGeneratorFunction);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(asyncgefu);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsAsyncGeneratorFunction(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsAsyncGeneratorFunction);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsAsyncGeneratorFunction_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsAsyncGeneratorFunction(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsAsyncGeneratorFunction);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsLinkedList_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
    JSHandle<JSTaggedValue> value =
        JSObject::GetProperty(thread_, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), 6);
    objCallInfo->SetFunction(JSTaggedValue::Undefined());
    objCallInfo->SetThis(value.GetTaggedValue());
    objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::LinkedList)));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, objCallInfo);
    JSHandle<JSTaggedValue> contianer =
        JSHandle<JSTaggedValue>(thread_, containers::ContainersPrivate::Load(objCallInfo));
    JSHandle<JSAPILinkedList> linkedList =
        JSHandle<JSAPILinkedList>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(contianer), contianer));
    JSTaggedValue doubleList = TaggedDoubleList::Create(thread_);
    linkedList->SetDoubleList(thread_, doubleList);
    JSHandle<JSTaggedValue> linkedListTag = JSHandle<JSTaggedValue>::Cast(linkedList);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(linkedListTag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsLinkedList(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsLinkedList);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsLinkedList_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsLinkedList(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsLinkedList);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsLinkedListIterator_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
    JSHandle<JSTaggedValue> tagvalue =
        JSObject::GetProperty(thread_, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
    uint32_t argvLength = 6; // 6 = argv length
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), argvLength);
    objCallInfo->SetFunction(JSTaggedValue::Undefined());
    objCallInfo->SetThis(tagvalue.GetTaggedValue());
    objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::LinkedList)));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, objCallInfo);
    JSHandle<JSTaggedValue> contianer =
        JSHandle<JSTaggedValue>(thread_, containers::ContainersPrivate::Load(objCallInfo));
    JSHandle<JSAPILinkedList> linkedList =
        JSHandle<JSAPILinkedList>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(contianer), contianer));
    JSTaggedValue doubleList = TaggedDoubleList::Create(thread_);
    linkedList->SetDoubleList(thread_, doubleList);
    uint32_t elementsNum = 256; // 256 = Number of cycles
    for (uint32_t i = 0; i < elementsNum; i++) {
        JSHandle<JSTaggedValue> taggedvalue(thread_, JSTaggedValue(i));
        JSAPILinkedList::Add(thread_, linkedList, taggedvalue);
    }
    JSHandle<JSTaggedValue> taggedValueHandle(thread_, linkedList.GetTaggedValue());
    JSHandle<JSAPILinkedListIterator> linkedListIterator(thread_,
        JSAPILinkedListIterator::CreateLinkedListIterator(thread_, taggedValueHandle).GetTaggedValue());
    JSHandle<JSTaggedValue> linkedListIteratortag = JSHandle<JSTaggedValue>::Cast(linkedListIterator);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(linkedListIteratortag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsLinkedListIterator(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsLinkedListIterator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsLinkedListIterator_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsLinkedListIterator(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsLinkedListIterator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsList_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
    JSHandle<JSTaggedValue> value =
        JSObject::GetProperty(thread_, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
    uint32_t argvLength = 6; // 6 = argv length
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), argvLength);
    objCallInfo->SetFunction(JSTaggedValue::Undefined());
    objCallInfo->SetThis(value.GetTaggedValue());
    objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::List)));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, objCallInfo);
    JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);
    TestHelper::TearDownFrame(thread_, prev);
    JSHandle<JSTaggedValue> constructor(thread_, result);
    JSHandle<JSAPIList> list(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSTaggedValue singleList = TaggedSingleList::Create(thread_);
    list->SetSingleList(thread_, singleList);
    JSHandle<JSTaggedValue> Listtag = JSHandle<JSTaggedValue>::Cast(list);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(Listtag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsList(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsList);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsList_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsList(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsList);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsPlainArray_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
    JSHandle<JSTaggedValue> value =
        JSObject::GetProperty(thread_, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
    uint32_t argvLength = 6; // 6 = argv length
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread_, JSTaggedValue::Undefined(), argvLength);
    objCallInfo->SetFunction(JSTaggedValue::Undefined());
    objCallInfo->SetThis(value.GetTaggedValue());
    objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::PlainArray)));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread_, objCallInfo);
    JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);
    TestHelper::TearDownFrame(thread_, prev);
    JSHandle<JSTaggedValue> constructor(thread_, result);
    JSHandle<JSAPIPlainArray> plainArray(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(8));
    JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(8));
    plainArray->SetKeys(thread_, keyArray);
    plainArray->SetValues(thread_, valueArray);
    JSHandle<JSTaggedValue> plainarraytag = JSHandle<JSTaggedValue>::Cast(plainArray);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(plainarraytag);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsPlainArray(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsPlainArray);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsPlainArray_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = random number
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsPlainArray(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsPlainArray);
}

HWTEST_F_L0(JSNApiSplTest, DateRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    double time = 3.14; // 3.14 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        DateRef::New(vm_, time);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(DateRef::New);
}

HWTEST_F_L0(JSNApiSplTest, DateRef_ToString)
{
    LocalScope scope(vm_);
    CalculateForTime();
    double time = 3.14; // 3.14 = random number
    Local<DateRef> object = DateRef::New(vm_, time);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->ToString(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(DateRef::ToString);
}

HWTEST_F_L0(JSNApiSplTest, DateRef_GetTime)
{
    LocalScope scope(vm_);
    CalculateForTime();
    double time = 3.14; // 3.14 = random number
    Local<DateRef> object = DateRef::New(vm_, time);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetTime(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(DateRef::GetTime);
}

HWTEST_F_L0(JSNApiSplTest, ProxyRef_GetHandler)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> hclass(thread_, env->GetObjectFunction().GetObject<JSFunction>());
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(1));
    JSObject::SetProperty(thread_, targetHandle, key, value);
    JSHandle<JSTaggedValue> handlerHandle(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
    JSHandle<JSTaggedValue> proxyTagValue = JSHandle<JSTaggedValue>::Cast(proxyHandle);
    Local<ProxyRef> object = JSNApiHelper::ToLocal<ProxyRef>(proxyTagValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetHandler(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ProxyRef::GetHandler);
}

HWTEST_F_L0(JSNApiSplTest, ProxyRef_GetTarget)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> hclass(thread_, env->GetObjectFunction().GetObject<JSFunction>());
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(1));
    JSObject::SetProperty(thread_, targetHandle, key, value);
    JSHandle<JSTaggedValue> handlerHandle(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
    JSHandle<JSTaggedValue> proxyTagValue = JSHandle<JSTaggedValue>::Cast(proxyHandle);
    Local<ProxyRef> object = JSNApiHelper::ToLocal<ProxyRef>(proxyTagValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetTarget(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ProxyRef::GetTarget);
}

HWTEST_F_L0(JSNApiSplTest, ProxyRef_IsRevoked)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> hclass(thread_, env->GetObjectFunction().GetObject<JSFunction>());
    JSHandle<JSTaggedValue> targetHandle(factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(1));
    JSObject::SetProperty(thread_, targetHandle, key, value);
    JSHandle<JSTaggedValue> handlerHandle(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
    JSHandle<JSTaggedValue> proxyTagValue = JSHandle<JSTaggedValue>::Cast(proxyHandle);
    Local<ProxyRef> object = JSNApiHelper::ToLocal<ProxyRef>(proxyTagValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsRevoked();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ProxyRef::IsRevoked);
}

HWTEST_F_L0(JSNApiSplTest, MapRef_GetSize)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<MapRef> object = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetSize(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapRef::GetSize);
}

HWTEST_F_L0(JSNApiSplTest, MapRef_GetTotalElements)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<MapRef> object = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetTotalElements(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapRef::GetTotalElements);
}

HWTEST_F_L0(JSNApiSplTest, MapRef_Get)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<MapRef> object = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->Get(vm_, key);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapRef::Get);
}

HWTEST_F_L0(JSNApiSplTest, MapRef_GetKey)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<MapRef> object = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetKey(vm_, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapRef::GetKey);
}

HWTEST_F_L0(JSNApiSplTest, MapRef_GetValue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<MapRef> object = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetValue(vm_, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapRef::GetValue);
}

HWTEST_F_L0(JSNApiSplTest, MapRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        MapRef::New(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapRef::New);
}

HWTEST_F_L0(JSNApiSplTest, MapRef_Set)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<MapRef> object = MapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->Set(vm_, key, value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapRef::Set);
}

HWTEST_F_L0(JSNApiSplTest, WeakMapRef_GetSize)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<WeakMapRef> object = WeakMapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetSize(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(WeakMapRef::GetSize);
}

HWTEST_F_L0(JSNApiSplTest, WeakMapRef_GetTotalElements)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<WeakMapRef> object = WeakMapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetTotalElements(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(WeakMapRef::GetTotalElements);
}

HWTEST_F_L0(JSNApiSplTest, WeakMapRef_GetKey)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<WeakMapRef> object = WeakMapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetKey(vm_, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(WeakMapRef::GetKey);
}

HWTEST_F_L0(JSNApiSplTest, WeakMapRef_GetValue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<WeakMapRef> object = WeakMapRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetValue(vm_, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(WeakMapRef::GetValue);
}

HWTEST_F_L0(JSNApiSplTest, SetRef_GetSize)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<SetRef> object = SetRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetSize(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(SetRef::GetSize);
}

HWTEST_F_L0(JSNApiSplTest, SetRef_GetTotalElements)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<SetRef> object = SetRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetTotalElements(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(SetRef::GetTotalElements);
}

HWTEST_F_L0(JSNApiSplTest, SetRef_GetValue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<SetRef> object = SetRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetValue(vm_, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(SetRef::GetValue);
}

HWTEST_F_L0(JSNApiSplTest, WeakSetRef_GetSize)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<WeakSetRef> object = WeakSetRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetSize(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(WeakSetRef::GetSize);
}

HWTEST_F_L0(JSNApiSplTest, WeakSetRef_GetTotalElements)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<WeakSetRef> object = WeakSetRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetTotalElements(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(WeakSetRef::GetTotalElements);
}

HWTEST_F_L0(JSNApiSplTest, WeakSetRef_GetValue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<WeakSetRef> object = WeakSetRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = StringRef::NewFromUtf8(vm_, "TestValue");
    object->Set(vm_, key, value);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetValue(vm_, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(WeakSetRef::GetValue);
}

HWTEST_F_L0(JSNApiSplTest, IsWeakSetRef_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> weakSet = WeakSetRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(weakSet->IsWeakSet(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsWeakSet);
}

HWTEST_F_L0(JSNApiSplTest, IsWeakSetRef_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsWeakSet(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsWeakSet);
}

HWTEST_F_L0(JSNApiSplTest, MapIteratorRef_GetIndex)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> builtinsMapFunc = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc));
    JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(thread_));
    jsMap->SetLinkedMap(thread_, linkedHashMap);
    JSHandle<JSTaggedValue> mapValue(jsMap);
    JSHandle<JSTaggedValue> mapIteratorVal = JSMapIterator::CreateMapIterator(thread_, mapValue, IterationKind::KEY);
    JSHandle<JSMapIterator> mapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);
    mapIterator->SetNextIndex(1);
    Local<MapIteratorRef> object = JSNApiHelper::ToLocal<MapIteratorRef>(mapIteratorVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetIndex();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapIteratorRef::GetIndex);
}

HWTEST_F_L0(JSNApiSplTest, MapIteratorRef_GetKind)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> builtinsMapFunc = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc));
    JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(thread_));
    jsMap->SetLinkedMap(thread_, linkedHashMap);
    JSHandle<JSTaggedValue> mapValue(jsMap);
    JSHandle<JSTaggedValue> mapIteratorVal = JSMapIterator::CreateMapIterator(thread_, mapValue, IterationKind::KEY);
    JSHandle<JSMapIterator> mapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);
    mapIterator->SetIterationKind(IterationKind::VALUE);
    mapIterator->SetIterationKind(IterationKind::KEY_AND_VALUE);
    Local<MapIteratorRef> object = JSNApiHelper::ToLocal<MapIteratorRef>(mapIteratorVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetKind(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(MapIteratorRef::GetKind);
}

HWTEST_F_L0(JSNApiSplTest, SetIteratorRef_GetIndex)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread_);
    set->SetLinkedSet(thread_, hashSet);
    JSHandle<JSTaggedValue> setIteratorValue =
        JSSetIterator::CreateSetIterator(thread_, JSHandle<JSTaggedValue>(set), IterationKind::KEY);
    JSHandle<JSSetIterator> setIterator = JSHandle<JSSetIterator>::Cast(setIteratorValue);
    setIterator->SetNextIndex(1);
    Local<SetIteratorRef> object = JSNApiHelper::ToLocal<SetIteratorRef>(setIteratorValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetIndex();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(SetIteratorRef::GetIndex);
}

HWTEST_F_L0(JSNApiSplTest, SetIteratorRef_GetKind)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread_);
    set->SetLinkedSet(thread_, hashSet);
    JSHandle<JSTaggedValue> setIteratorValue =
        JSSetIterator::CreateSetIterator(thread_, JSHandle<JSTaggedValue>(set), IterationKind::KEY);
    JSHandle<JSSetIterator> setIterator = JSHandle<JSSetIterator>::Cast(setIteratorValue);
    setIterator->SetIterationKind(IterationKind::VALUE);
    setIterator->SetIterationKind(IterationKind::KEY_AND_VALUE);
    Local<SetIteratorRef> object = JSNApiHelper::ToLocal<SetIteratorRef>(setIteratorValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetKind(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(SetIteratorRef::GetKind);
}

HWTEST_F_L0(JSNApiSplTest, GeneratorFunctionRef_IsGenerator)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
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
    Local<GeneratorFunctionRef> object = genObjectRef->GetGeneratorFunction(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsGenerator(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(GeneratorFunctionRef::IsGenerator);
}

HWTEST_F_L0(JSNApiSplTest, GeneratorObjectRef_GetGeneratorState)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> genFunc = env->GetGeneratorFunctionFunction();
    JSHandle<JSGeneratorObject> genObjHandleVal = factory->NewJSGeneratorObject(genFunc);
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    JSHandle<JSFunction> generatorFunc = JSHandle<JSFunction>::Cast(factory->NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, generatorFunc, FunctionKind::GENERATOR_FUNCTION);
    JSHandle<GeneratorContext> generatorContext = factory->NewGeneratorContext();
    generatorContext->SetMethod(thread_, generatorFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> generatorContextVal = JSHandle<JSTaggedValue>::Cast(generatorContext);
    genObjHandleVal->SetGeneratorContext(thread_, generatorContextVal.GetTaggedValue());
    genObjHandleVal->SetGeneratorState(JSGeneratorState::COMPLETED);
    JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
    Local<GeneratorObjectRef> object = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetGeneratorState(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(GeneratorObjectRef::GetGeneratorState);
}

HWTEST_F_L0(JSNApiSplTest, GeneratorObjectRef_GetGeneratorFunction)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> genFunc = env->GetGeneratorFunctionFunction();
    JSHandle<JSGeneratorObject> genObjHandleVal = factory->NewJSGeneratorObject(genFunc);
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    JSHandle<JSFunction> generatorFunc = JSHandle<JSFunction>::Cast(factory->NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, generatorFunc, FunctionKind::GENERATOR_FUNCTION);
    JSHandle<GeneratorContext> generatorContext = factory->NewGeneratorContext();
    generatorContext->SetMethod(thread_, generatorFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> generatorContextVal = JSHandle<JSTaggedValue>::Cast(generatorContext);
    genObjHandleVal->SetGeneratorContext(thread_, generatorContextVal.GetTaggedValue());
    genObjHandleVal->SetGeneratorState(JSGeneratorState::COMPLETED);
    JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
    Local<GeneratorObjectRef> object = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetGeneratorFunction(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(GeneratorObjectRef::GetGeneratorFunction);
}

HWTEST_F_L0(JSNApiSplTest, GeneratorObjectRef_GetGeneratorReceiver)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> genFunc = env->GetGeneratorFunctionFunction();
    JSHandle<JSGeneratorObject> genObjHandleVal = factory->NewJSGeneratorObject(genFunc);
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    JSHandle<JSFunction> generatorFunc = JSHandle<JSFunction>::Cast(factory->NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, generatorFunc, FunctionKind::GENERATOR_FUNCTION);
    JSHandle<GeneratorContext> generatorContext = factory->NewGeneratorContext();
    generatorContext->SetMethod(thread_, generatorFunc.GetTaggedValue());
    JSHandle<JSTaggedValue> generatorContextVal = JSHandle<JSTaggedValue>::Cast(generatorContext);
    genObjHandleVal->SetGeneratorContext(thread_, generatorContextVal.GetTaggedValue());
    genObjHandleVal->SetGeneratorState(JSGeneratorState::COMPLETED);
    JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(genObjHandleVal);
    Local<GeneratorObjectRef> object = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetGeneratorReceiver(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(GeneratorObjectRef::GetGeneratorReceiver);
}

HWTEST_F_L0(JSNApiSplTest, CollatorRef_GetCompareFunction)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> ctor = env->GetCollatorFunction();
    JSHandle<JSCollator> collator =
        JSHandle<JSCollator>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSTaggedValue> localeStr = thread_->GlobalConstants()->GetHandledEnUsString();
    JSHandle<JSTaggedValue> undefinedHandle(thread_, JSTaggedValue::Undefined());
    JSHandle<JSCollator> initCollator = JSCollator::InitializeCollator(thread_, collator, localeStr, undefinedHandle);
    JSHandle<JSTaggedValue> collatorTagHandleVal = JSHandle<JSTaggedValue>::Cast(initCollator);
    Local<CollatorRef> object = JSNApiHelper::ToLocal<CollatorRef>(collatorTagHandleVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetCompareFunction(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(GeneratorObjectRef::GetCompareFunction);
}

HWTEST_F_L0(JSNApiSplTest, DataTimeFormatRef_GetFormatFunction)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
    JSHandle<JSLocale> locales =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
    icu::Locale icuLocale("zh", "Hans", "Cn", "calendar=chinese");
    factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> options = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    options = JSDateTimeFormat::ToDateTimeOptions(thread_, JSHandle<JSTaggedValue>::Cast(options), RequiredOption::ANY,
        DefaultsOption::ALL);
    JSHandle<JSTaggedValue> dtfCtor = env->GetDateTimeFormatFunction();
    JSHandle<JSDateTimeFormat> dtf =
        JSHandle<JSDateTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dtfCtor), dtfCtor));
    dtf = JSDateTimeFormat::InitializeDateTimeFormat(thread_, dtf, JSHandle<JSTaggedValue>::Cast(locales),
        JSHandle<JSTaggedValue>::Cast(options));
    JSHandle<JSTaggedValue> dtfTagHandleVal = JSHandle<JSTaggedValue>::Cast(dtf);
    Local<DataTimeFormatRef> object = JSNApiHelper::ToLocal<DataTimeFormatRef>(dtfTagHandleVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetFormatFunction(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(DataTimeFormatRef::GetFormatFunction);
}

HWTEST_F_L0(JSNApiSplTest, NumberFormatRef_GetFormatFunction)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> ctor = env->GetNumberFormatFunction();
    JSHandle<JSNumberFormat> numberFormat =
        JSHandle<JSNumberFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    EXPECT_TRUE(*numberFormat != nullptr);
    JSHandle<JSTaggedValue> locales(factory->NewFromASCII("zh-Hans-CN"));
    JSHandle<JSTaggedValue> undefinedOptions(thread_, JSTaggedValue::Undefined());
    JSNumberFormat::InitializeNumberFormat(thread_, numberFormat, locales, undefinedOptions);
    JSHandle<JSTaggedValue> numberformatTagHandleVal = JSHandle<JSTaggedValue>::Cast(numberFormat);
    Local<NumberFormatRef> object = JSNApiHelper::ToLocal<NumberFormatRef>(numberformatTagHandleVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetFormatFunction(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NumberFormatRef::GetFormatFunction);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRejectInfo_PromiseRejectInfo)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> toStringPromise = StringRef::NewFromUtf8(vm_, "-3.14");
    Local<JSValueRef> promise(toStringPromise);
    Local<StringRef> toStringReason = StringRef::NewFromUtf8(vm_, "123.3");
    Local<JSValueRef> reason(toStringReason);
    void *data = static_cast<void *>(new std::string("promisereject"));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRejectInfo::PromiseRejectInfo);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRejectInfo_GetPromise)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> toStringPromise = StringRef::NewFromUtf8(vm_, "-3.14");
    Local<JSValueRef> promise(toStringPromise);
    Local<StringRef> toStringReason = StringRef::NewFromUtf8(vm_, "3.14");
    Local<JSValueRef> reason(toStringReason);
    void *data = static_cast<void *>(new std::string("promisereject"));
    PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        promisereject.GetPromise();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRejectInfo::GetPromise);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRejectInfo_GetReason)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> toStringPromise = StringRef::NewFromUtf8(vm_, "-3.14");
    Local<JSValueRef> promise(toStringPromise);
    Local<StringRef> toStringReason = StringRef::NewFromUtf8(vm_, "3.14");
    Local<JSValueRef> reason(toStringReason);
    void *data = static_cast<void *>(new std::string("promisereject"));
    PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        promisereject.GetReason();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRejectInfo::GetReason);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRejectInfo_GetOperation)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> toStringPromise = StringRef::NewFromUtf8(vm_, "-3.14");
    Local<JSValueRef> promise(toStringPromise);
    Local<StringRef> toStringReason = StringRef::NewFromUtf8(vm_, "3.14");
    Local<JSValueRef> reason(toStringReason);
    void *data = static_cast<void *>(new std::string("promisereject"));
    PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        promisereject.GetOperation();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRejectInfo::GetOperation);
}

HWTEST_F_L0(JSNApiSplTest, PromiseRejectInfo_GetData)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> toStringPromise = StringRef::NewFromUtf8(vm_, "-3.14");
    Local<JSValueRef> promise(toStringPromise);
    Local<StringRef> toStringReason = StringRef::NewFromUtf8(vm_, "3.14");
    Local<JSValueRef> reason(toStringReason);
    void *data = static_cast<void *>(new std::string("promisereject"));
    PromiseRejectInfo promisereject(promise, reason, PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT, data);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        promisereject.GetData();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PromiseRejectInfo::GetData);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_Int32Value)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->Int32Value(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Int32Value);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToNumber)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        toString->ToNumber(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToNumber);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsHole)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> res = IntegerRef::New(vm_, 123);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsHole();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsHole);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsTrue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsTrue();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsTrue);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsSymbol)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsSymbol(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsSymbol);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsObject)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsTypedArray)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsTypedArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsTypedArray);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsNativePointer)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsNativePointer(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsNativePointer);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsRegExp)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsRegExp(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsRegExp);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsArrayIterator)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = random number
    Local<JSValueRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsArrayIterator(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArrayIterator);
}

HWTEST_F_L0(JSNApiSplTest, EscapeLocalScope_gz)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        EscapeLocalScope test(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(EscapeLocalScope::EscapeLocalScope);
}

HWTEST_F_L0(JSNApiSplTest, EscapeLocalScope_Escape)
{
    LocalScope scope(vm_);
    CalculateForTime();
    EscapeLocalScope test(vm_);
    int num = 3; // 3 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        test.Escape(ArrayRef::New(vm_, num));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(EscapeLocalScope::Escape);
}

HWTEST_F_L0(JSNApiSplTest, JSExecutionScope_gz)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSExecutionScope res(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSExecutionScope::JSExecutionScope);
}

HWTEST_F_L0(JSNApiSplTest, LocalScope_gz)
{
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        LocalScope scope(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(LocalScope::LocalScope);
}

HWTEST_F_L0(JSNApiSplTest, PrimitiveRef_GetValue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 0; // 0 = random number
    Local<PrimitiveRef> intValue = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->GetValue(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(PrimitiveRef::GetValue);
}

HWTEST_F_L0(JSNApiSplTest, IntegerRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        IntegerRef::New(vm_, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(IntegerRef::New);
}

HWTEST_F_L0(JSNApiSplTest, IntegerRef_NewFromUnsigned)
{
    LocalScope scope(vm_);
    CalculateForTime();
    unsigned int res = 123; // 123 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        IntegerRef::NewFromUnsigned(vm_, res);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(IntegerRef::NewFromUnsigned);
}

HWTEST_F_L0(JSNApiSplTest, IntegerRef_Value)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 0; // 0 = random number
    Local<IntegerRef> res = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(IntegerRef::Value);
}

HWTEST_F_L0(JSNApiSplTest, NumberRef_New01)
{
    LocalScope scope(vm_);
    CalculateForTime();
    double res = 64; // 64 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NumberRef::New(vm_, res);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NumberRef::New01);
}

HWTEST_F_L0(JSNApiSplTest, NumberRef_New02)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t res = 64; // 64 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NumberRef::New(vm_, res);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NumberRef::New02);
}

HWTEST_F_L0(JSNApiSplTest, NumberRef_New03)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t res = 64; // 64 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NumberRef::New(vm_, res);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NumberRef::New03);
}

HWTEST_F_L0(JSNApiSplTest, NumberRef_New04)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int64_t res = 64; // 64 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NumberRef::New(vm_, res);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NumberRef::New04);
}

HWTEST_F_L0(JSNApiSplTest, NumberRef_Value)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = 0; // 0 = random number
    Local<NumberRef> res = NumberRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NumberRef::Value);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_Cast)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> value = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ObjectRef::Cast(*value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::Cast);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ObjectRef::New(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::New);
}

void *Detach1()
{
    GTEST_LOG_(INFO) << "detach is running";
    return nullptr;
}

void Attach1([[maybe_unused]] void *buffer)
{
    GTEST_LOG_(INFO) << "attach is running";
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_Set02)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->Set(vm_, key, value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::Set02);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_Set03)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->Set(vm_, key, value);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::Set03);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_SetAccessorProperty)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<FunctionRef> target1 = FunctionRef::New(vm_, nullptr);
    Local<FunctionRef> target2 = FunctionRef::New(vm_, nullptr);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->SetAccessorProperty(vm_, key, target1, target2);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::SetAccessorProperty);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_Get01)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->Get(vm_, key);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::Get01);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_Get02)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> object = ObjectRef::New(vm_);
    int32_t key = 123; // 123 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->Get(vm_, key);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::Get02);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_GetOwnProperty)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetOwnProperty(vm_, key, attribute);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::GetOwnProperty);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_GetOwnPropertyNames)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetOwnPropertyNames(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::GetOwnPropertyNames);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_GetAllPropertyNames)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    uint32_t filter = 123; // 123 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetAllPropertyNames(vm_, filter);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::GetAllPropertyNames);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_GetOwnEnumerablePropertyNames)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetOwnEnumerablePropertyNames(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::GetOwnEnumerablePropertyNames);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_GetPrototype)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetPrototype(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::GetPrototype);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_SetPrototype)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<ObjectRef> prototype = object->GetPrototype(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->SetPrototype(vm_, prototype);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::SetPrototype);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_Freeze)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->Freeze(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::Freeze);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_Seal)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->Seal(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::Seal);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_SetNativePointerFieldCount)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    int32_t input = 34; // 34 = random number
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->SetNativePointerFieldCount(vm_, input);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::SetNativePointerFieldCount);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_GetNativePointerFieldCount)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    int32_t input = 34; // 34 = random number
    object->SetNativePointerFieldCount(vm_, input);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetNativePointerFieldCount(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::GetNativePointerFieldCount);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_SetNativePointerField)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void *>(new std::string("test"));
    void *vp2 = static_cast<void *>(new std::string("test"));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->SetNativePointerField(vm_, 33, vp1, callBack, vp2);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::SetNativePointerField);
}

HWTEST_F_L0(JSNApiSplTest, ObjectRef_GetNativePointerField)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    NativePointerCallback callBack = nullptr;
    void *vp1 = static_cast<void *>(new std::string("test"));
    void *vp2 = static_cast<void *>(new std::string("test"));
    object->SetNativePointerField(vm_, 33, vp1, callBack, vp2);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->GetNativePointerField(vm_, 33);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ObjectRef::GetNativePointerField);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_GetGlobalObject)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::GetGlobalObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::GetGlobalObject);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_ExecutePendingJob)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::ExecutePendingJob(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::ExecutePendingJob);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_TriggerGC)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::TriggerGC(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::TriggerGC);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_ThrowException)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::ThrowException(vm_, error);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::ThrowException);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_GetAndClearUncaughtException)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::GetAndClearUncaughtException(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::GetAndClearUncaughtException);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_GetUncaughtException)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::GetUncaughtException(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::GetUncaughtException);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_HasPendingException)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::HasPendingException(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::HasPendingException);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_EnableUserUncaughtErrorHandler)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::EnableUserUncaughtErrorHandler(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::EnableUserUncaughtErrorHandler);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_StartDebugger)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSNApi::DebugOption res;
    res.libraryPath = "mytests";
    res.isDebugMode = true;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::StartDebugger(vm_, res);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::StartDebugger);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_StopDebugger)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::StopDebugger(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi::StopDebugger);
}

HWTEST_F_L0(JSNApiSplTest, JsiRuntimeCallInfo_GetFunctionRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JsiRuntimeCallInfo object;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.GetFunctionRef();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JsiRuntimeCallInfo::GetFunctionRef);
}

HWTEST_F_L0(JSNApiSplTest, JsiRuntimeCallInfo_GetNewTargetRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JsiRuntimeCallInfo object;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.GetNewTargetRef();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JsiRuntimeCallInfo::GetNewTargetRef);
}

HWTEST_F_L0(JSNApiSplTest, JsiRuntimeCallInfo_GetThisRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JsiRuntimeCallInfo object;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.GetThisRef();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JsiRuntimeCallInfo::GetThisRef);
}

HWTEST_F_L0(JSNApiSplTest, JsiRuntimeCallInfo_GetCallArgRef)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JsiRuntimeCallInfo object;
    uint32_t idx = 123;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.GetCallArgRef(idx);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JsiRuntimeCallInfo::GetCallArgRef);
}

HWTEST_F_L0(JSNApiSplTest, FunctionCallScope_Gz)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        FunctionCallScope test(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionCallScope::FunctionCallScope);
}

HWTEST_F_L0(JSNApiSplTest, IsSetIterator_Ture)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> proto = thread_->GetEcmaVM()->GetGlobalEnv()->GetFunctionPrototype();
    JSHandle<JSHClass> setClass = factory->NewEcmaHClass(JSSet::SIZE, JSType::JS_SET, proto);
    JSHandle<JSSet> jsSet = JSHandle<JSSet>::Cast(factory->NewJSObjectWithInit(setClass));
    JSHandle<LinkedHashSet> linkedSet(LinkedHashSet::Create(thread_));
    jsSet->SetLinkedSet(thread_, linkedSet);
    JSHandle<JSSetIterator> jsSetIter = factory->NewJSSetIterator(jsSet, IterationKind::KEY);
    JSHandle<JSTaggedValue> setiter = JSHandle<JSTaggedValue>::Cast(jsSetIter);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(JSNApiHelper::ToLocal<JSValueRef>(setiter)->IsSetIterator(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsSetIterator);
}

HWTEST_F_L0(JSNApiSplTest, IsSetIterator_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsSetIterator(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsSetIterator);
}

HWTEST_F_L0(JSNApiSplTest, IsUint16Array_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = 30;       // 30 = ArrayBuff length
    int32_t byteOffset = 4; // 4 = Offset
    int32_t length = 6;     // 6 = length
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, num);
    Local<Uint16ArrayRef> object = Uint16ArrayRef::New(vm_, buffer, byteOffset, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsUint16Array(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsUint16Array);
}

HWTEST_F_L0(JSNApiSplTest, IsUint16Array_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsUint16Array(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsUint16Array);
}

HWTEST_F_L0(JSNApiSplTest, IsInt32Array_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t num = 30;       // 30 = ArrayBuff length
    int32_t byteOffset = 4; // 4 = Offset
    int32_t length = 6;     // 6 = length
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, num);
    Local<Int32ArrayRef> object = Int32ArrayRef::New(vm_, buffer, byteOffset, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsInt32Array(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt32Array);
}

HWTEST_F_L0(JSNApiSplTest, IsInt32Array_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsInt32Array(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt32Array);
}

HWTEST_F_L0(JSNApiSplTest, IsJSPrimitiveString_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSPrimitiveString(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveString);
}

HWTEST_F_L0(JSNApiSplTest, IsGeneratorObject_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
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
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(genObjectRef->IsGeneratorObject(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsGeneratorObject);
}

HWTEST_F_L0(JSNApiSplTest, IsGeneratorObject_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsGeneratorObject(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsGeneratorObject);
}

HWTEST_F_L0(JSNApiSplTest, IsJSPrimitiveSymbol_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSPrimitiveSymbol(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveSymbol);
}

HWTEST_F_L0(JSNApiSplTest, IsAsyncGeneratorObject_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    auto factory = vm_->GetFactory();
    auto env = vm_->GetGlobalEnv();
    JSHandle<JSAsyncGeneratorObject> asyncGenObj =
        factory->NewJSAsyncGeneratorObject(env->GetAsyncGeneratorFunctionFunction());
    JSHandle<JSTaggedValue> genObjTagHandleVal = JSHandle<JSTaggedValue>::Cast(asyncGenObj);
    Local<GeneratorObjectRef> genObjectRef = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(genObjectRef->IsAsyncGeneratorObject(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsAsyncGeneratorObject);
}

HWTEST_F_L0(JSNApiSplTest, IsAsyncGeneratorObject_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsAsyncGeneratorObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsAsyncGeneratorObject);
}

HWTEST_F_L0(JSNApiSplTest, IsModuleNamespaceObject_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<ModuleNamespace> moduleNamespace = factory->NewModuleNamespace();
    JSHandle<JSTaggedValue> modname = JSHandle<JSTaggedValue>::Cast(moduleNamespace);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ObjectRef>(modname)->IsModuleNamespaceObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsModuleNamespaceObject);
}

HWTEST_F_L0(JSNApiSplTest, IsModuleNamespaceObject_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsModuleNamespaceObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsModuleNamespaceObject);
}

HWTEST_F_L0(JSNApiSplTest, IsSharedArrayBuffer_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    auto *factory = vm_->GetFactory();
    int32_t num = 40; // 40 = ArrayBuffer length
    JSHandle<JSArrayBuffer> jsArrayBuffer = factory->NewJSSharedArrayBuffer(num);
    JSHandle<JSTaggedValue> SAbuffer = JSHandle<JSTaggedValue>::Cast(jsArrayBuffer);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ArrayRef>(SAbuffer)->IsSharedArrayBuffer(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsSharedArrayBuffer);
}

HWTEST_F_L0(JSNApiSplTest, IsSharedArrayBuffer_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsSharedArrayBuffer(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsSharedArrayBuffer);
}

HWTEST_F_L0(JSNApiSplTest, IsStrictEquals_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<ObjectRef> object2 = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsStrictEquals(vm_, object2);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsStrictEquals);
}

HWTEST_F_L0(JSNApiSplTest, IsStrictEquals_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> target1 = StringRef::NewFromUtf8(vm_, "1");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsStrictEquals(vm_, target1);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsStrictEquals);
}

HWTEST_F_L0(JSNApiSplTest, IsQueue_Frue)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> proto = thread->GetEcmaVM()->GetGlobalEnv()->GetFunctionPrototype();
    JSHandle<JSHClass> queueClass = factory->NewEcmaHClass(JSAPIQueue::SIZE, JSType::JS_API_QUEUE, proto);
    JSHandle<JSAPIQueue> jsQueue = JSHandle<JSAPIQueue>::Cast(factory->NewJSObjectWithInit(queueClass));
    JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIQueue::DEFAULT_CAPACITY_LENGTH);
    jsQueue->SetLength(thread, JSTaggedValue(0));
    jsQueue->SetFront(0);
    jsQueue->SetTail(0);
    jsQueue->SetElements(thread, newElements);
    JSHandle<JSTaggedValue> Que = JSHandle<JSTaggedValue>::Cast(jsQueue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ArrayRef>(Que)->IsDeque(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsQueue);
}

HWTEST_F_L0(JSNApiSplTest, IsQueue_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsQueue(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsQueue);
}

HWTEST_F_L0(JSNApiSplTest, IsStack_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> proto = thread->GetEcmaVM()->GetGlobalEnv()->GetFunctionPrototype();
    JSHandle<JSHClass> stackClass = factory->NewEcmaHClass(JSAPIStack::SIZE, JSType::JS_API_STACK, proto);
    JSHandle<JSAPIStack> jsStack = JSHandle<JSAPIStack>::Cast(factory->NewJSObjectWithInit(stackClass));
    jsStack->SetTop(0);
    JSHandle<JSTaggedValue> stcak = JSHandle<JSTaggedValue>::Cast(jsStack);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ArrayRef>(stcak)->IsStack(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsStack);
}

HWTEST_F_L0(JSNApiSplTest, IsStack_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsStack(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsStack);
}

HWTEST_F_L0(JSNApiSplTest, IsTreeMap_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSThread *thread = vm_->GetJSThread();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> mapClass = factory->NewEcmaHClass(JSAPITreeMap::SIZE, JSType::JS_API_TREE_MAP, proto);
    JSHandle<JSAPITreeMap> jsTreeMap = JSHandle<JSAPITreeMap>::Cast(factory->NewJSObjectWithInit(mapClass));
    JSHandle<TaggedTreeMap> treeMap(thread, TaggedTreeMap::Create(thread));
    jsTreeMap->SetTreeMap(thread, treeMap);
    JSHandle<JSTaggedValue> treapp = JSHandle<JSTaggedValue>::Cast(jsTreeMap);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ArrayRef>(treapp)->IsTreeMap(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsTreeMap);
}

HWTEST_F_L0(JSNApiSplTest, IsTreeMap_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsTreeMap(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsTreeMap);
}

HWTEST_F_L0(JSNApiSplTest, IsTreeSet_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSThread *thread = vm_->GetJSThread();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> setClass = factory->NewEcmaHClass(JSAPITreeSet::SIZE, JSType::JS_API_TREE_SET, proto);
    JSHandle<JSAPITreeSet> jsTreeSet = JSHandle<JSAPITreeSet>::Cast(factory->NewJSObjectWithInit(setClass));
    JSHandle<TaggedTreeSet> treeSet(thread, TaggedTreeSet::Create(thread));
    jsTreeSet->SetTreeSet(thread, treeSet);
    JSHandle<JSTaggedValue> tresett = JSHandle<JSTaggedValue>::Cast(jsTreeSet);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ArrayRef>(tresett)->IsTreeSet(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsTreeSet);
}

HWTEST_F_L0(JSNApiSplTest, IsTreeSet_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsTreeSet(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsTreeSet);
}

HWTEST_F_L0(JSNApiSplTest, IsVector_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    ObjectFactory *factory = vm_->GetFactory();
    JSThread *thread = vm_->GetJSThread();
    JSHandle<JSTaggedValue> proto = thread->GetEcmaVM()->GetGlobalEnv()->GetFunctionPrototype();
    JSHandle<JSHClass> vectorClass = factory->NewEcmaHClass(JSAPIVector::SIZE, JSType::JS_API_VECTOR, proto);
    JSHandle<JSAPIVector> jsVector = JSHandle<JSAPIVector>::Cast(factory->NewJSObjectWithInit(vectorClass));
    jsVector->SetLength(0);
    JSHandle<JSTaggedValue> vectt = JSHandle<JSTaggedValue>::Cast(jsVector);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApiHelper::ToLocal<ArrayRef>(vectt)->IsVector(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsVector);
}

HWTEST_F_L0(JSNApiSplTest, Parse_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const char16_t *utf16 = u"您好，华为！";
    Local<StringRef> str = StringRef::NewFromUtf8(vm_, "abc");
    Local<StringRef> str2 = StringRef::NewFromUtf16(vm_, utf16);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSON::Parse(vm_, str);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Parse);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSON::Parse(vm_, str2);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Parse);
}

HWTEST_F_L0(JSNApiSplTest, Stringify)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<JSValueRef> object = ObjectRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSON::Stringify(vm_, object);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Stringify);
}

HWTEST_F_L0(JSNApiSplTest, Error)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Exception::Error(vm_, StringRef::NewFromUtf8(vm_, "test error"));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Error);
}

HWTEST_F_L0(JSNApiSplTest, RangeError)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Exception::RangeError(vm_, StringRef::NewFromUtf8(vm_, "test range error"));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::RangeError);
}

HWTEST_F_L0(JSNApiSplTest, ReferenceError)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Exception::ReferenceError(vm_, StringRef::NewFromUtf8(vm_, "test reference error"));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ReferenceError);
}
HWTEST_F_L0(JSNApiSplTest, SyntaxError)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Exception::SyntaxError(vm_, StringRef::NewFromUtf8(vm_, "test syntax error"));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::SyntaxError);
}

HWTEST_F_L0(JSNApiSplTest, TypeError)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Exception::TypeError(vm_, StringRef::NewFromUtf8(vm_, "test type error"));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::TypeError);
}

HWTEST_F_L0(JSNApiSplTest, AggregateError)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Exception::AggregateError(vm_, StringRef::NewFromUtf8(vm_, "test aggregate error"));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::AggregateError);
}

HWTEST_F_L0(JSNApiSplTest, EvalError)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Exception::EvalError(vm_, StringRef::NewFromUtf8(vm_, "test eval error"));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::EvalError);
}

HWTEST_F_L0(JSNApiSplTest, OOMError)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)Exception::OOMError(vm_, StringRef::NewFromUtf8(vm_, "test out of memory error"));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::OOMError);
}

HWTEST_F_L0(JSNApiSplTest, CreateEcmaVM)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSRuntimeOptions option;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        EcmaVM *workerVm = JSNApi::CreateEcmaVM(option);
        JSNApi::DestroyJSVM(workerVm);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::CreateEcmaVM);
}

HWTEST_F_L0(JSNApiSplTest, PostFork)
{
    LocalScope scope(vm_);
    CalculateForTime();
    RuntimeOption option;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < 10; i++) {
        EcmaVM *vm2 = JSNApi::CreateJSVM(option);
        JSNApi::PreFork(vm2);
        JSNApi::PostFork(vm2, option);
        JSNApi::DestroyJSVM(vm2);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::PostFork);
}

HWTEST_F_L0(JSNApiSplTest, AddWorker)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSRuntimeOptions option;
    gettimeofday(&g_beginTime, nullptr);
    EcmaVM *workerVm = JSNApi::CreateEcmaVM(option);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::AddWorker(vm_, workerVm);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::addWorker);
    JSNApi::DestroyJSVM(workerVm);
}

HWTEST_F_L0(JSNApiSplTest, DeleteWorker)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSRuntimeOptions option;
    EcmaVM *workerVm = JSNApi::CreateEcmaVM(option);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::AddWorker(vm_, workerVm);
        JSNApi::DeleteWorker(vm_, workerVm);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::DeleteWorker);
    JSNApi::DestroyJSVM(workerVm);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBundle)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::IsBundle(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBundle);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_SetBundle)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetBundle(vm_, false);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::SetBundle);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SetAssetPath)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string str = "/data/storage/el1/bundle/moduleName/ets/modules.abc";
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetAssetPath(vm_, str);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::SetAssetPath);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_GetAssetPath)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string str = "/data/storage/el1/bundle/moduleName/ets/modules.abc";
    gettimeofday(&g_beginTime, nullptr);
    JSNApi::SetAssetPath(vm_, str);
    for (int i = 0; i < NUM_COUNT; i++) {
        std::string res = JSNApi::GetAssetPath(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::GetAssetPath);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_SetBundleName)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string str = "11";
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetBundleName(vm_, str);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::SetBundleName);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_GetBundleName)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string str = "11";
    JSNApi::SetBundleName(vm_, str);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        std::string res = JSNApi::GetBundleName(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::GetBundleName);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_GetModuleName)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string str = "11";
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetModuleName(vm_, str);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::SetModuleName);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_SetModuleName)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string str = "11";
    JSNApi::SetModuleName(vm_, str);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        std::string res = JSNApi::GetModuleName(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::GetModuleName);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_SetLoop)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *data = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::SetLoop(vm_, data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::SetLoop);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_InitForConcurrentFunction)
{
    LocalScope scope(vm_);
    CalculateForTime();
    NativePointerCallback deleter = nullptr;
    void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    bool callNative = true;
    size_t nativeBindingsize = 15;
    Local<FunctionRef> res =
        FunctionRef::NewClassFunction(vm_, FunCallback, deleter, cb, callNative, nativeBindingsize);
    ASSERT_TRUE(res->IsFunction(vm_));
    Local<JSValueRef> res1 = res->GetFunctionPrototype(vm_);
    void *taskInfo = nullptr;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::InitForConcurrentFunction(vm_, res1, taskInfo);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::InitForConcurrentFunction);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_Rethrow)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        TryCatch tryCatch(vm_);
        tryCatch.Rethrow();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Rethrow);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_InitForConcurrentThread)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *data = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    ConcurrentCallback concurrentCallback_ { nullptr };
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::InitForConcurrentThread(vm_, concurrentCallback_, data);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::InitForConcurrentThread);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_JsiRuntimeCallInfo)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JsiRuntimeCallInfo();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::JsiRuntimeCallInfo);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_GetThread)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JsiRuntimeCallInfo object;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.GetThread();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::GetThread);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_GetArgsNumber)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JsiRuntimeCallInfo object;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object.GetArgsNumber();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::GetArgsNumber);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_HasCaught_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    JSNApi::ThrowException(vm_, error);
    TryCatch tryCatch(vm_);
    ASSERT_TRUE(tryCatch.HasCaught());
    vm_->GetJSThread()->ClearException();
    JSNApi::ThrowException(vm_, error);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        tryCatch.HasCaught();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::HasCaught);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_HasCaught_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    JSNApi::ThrowException(vm_, error);
    TryCatch tryCatch(vm_);
    tryCatch.GetAndClearException();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        tryCatch.HasCaught();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::HasCaught);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_Undefined)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    bool inputBool = true;
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->Undefined(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Undefined::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->Undefined(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Undefined::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->Undefined(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Undefined::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->Undefined(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Undefined::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->Undefined(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Undefined::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSValueRef::Undefined(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Undefined);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_Null)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    bool inputBool = true;
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->Null(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Null::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->Null(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Null::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->Null(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Null::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->Null(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Null::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->Null(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Null::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSValueRef::Null(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::Null);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    bool inputBool = true;
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->True(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::True::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->True(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::True::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->True(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::True::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->True(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::True::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->True(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::True::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSValueRef::True(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::True);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToBoolean)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    bool inputBool = true;
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->ToBoolean(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToBoolean::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->ToBoolean(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToBoolean::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->ToBoolean(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToBoolean::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->ToBoolean(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToBoolean::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->ToBoolean(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToBoolean::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToObject)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    bool inputBool = true;
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->ToObject(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToObject::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_ToString)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    bool inputBool = true;
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->ToString(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToString::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->ToString(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToString::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->ToString(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToString::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->ToString(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToString::BooleanRef);
    Local<JSValueRef> toTarget(stringUtf8);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        toTarget->ToString(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::ToString::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsFalse)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    bool inputBool = false;
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsFalse();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFalse::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsFalse();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFalse::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsFalse();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFalse::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->IsFalse();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFalse::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsFalse();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsFalse::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsNumber)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    bool inputBool = true;
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsNumber();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsNumber::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsNumber();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsNumber::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsNumber();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsNumber::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->IsNumber();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsNumber::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsNumber();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsNumber::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBigInt)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    int64_t maxInt64 = std::numeric_limits<int64_t>::max();
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 2147483646);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<NumberRef> resUnit64 = NumberRef::New(vm_, maxInt64);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<BigIntRef> maxBigintInt64 = BigIntRef::New(vm_, maxInt64);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsBigInt(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsBigInt(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt::NumberRef32);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit64->IsBigInt(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt::NumberRef64);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsBigInt(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt::BigIntRefU64);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintInt64->IsBigInt(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt::BigIntRefI64);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBigInt2)
{
    LocalScope scope(vm_);
    CalculateForTime();
    bool inputBool = true;
    std::string testUtf8 = "Hello world";
    Local<BooleanRef> resBool = BooleanRef::New(vm_, inputBool);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resBool->IsBigInt(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsBigInt(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsArray)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArray::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArray::NumberRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsArray2)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 3; // 3 = ArrayBuffer Length
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<ArrayRef> arrayObject = ArrayRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArray::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArray::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        arrayObject->IsArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsArray::ArrayRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSArray)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    int length = 3;            // 3 = ArrayBuffer Length
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<ArrayRef> arrayObject = ArrayRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsJSArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSArray::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsJSArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSArray::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsJSArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSArray::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsJSArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSArray::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        arrayObject->IsJSArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSArray::ArrayRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSArray2)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSArray *arr = JSArray::ArrayCreate(thread_, JSTaggedNumber(0)).GetObject<JSArray>();
    JSHandle<JSTaggedValue> obj(thread_, arr);
    Local<JSValueRef> JSArrayObject = JSNApiHelper::ToLocal<JSValueRef>(obj);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSArrayObject->IsJSArray(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSArray::JSArrayObject);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsConstructor)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsConstructor(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsConstructor::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsConstructor(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsConstructor::NumberRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsConstructor2)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    EcmaVM *ecmaVM = thread_->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> func = thread_->GetEcmaVM()->GetFactory()->NewJSFunction(env, static_cast<void *>(nullptr),
        FunctionKind::BASE_CONSTRUCTOR);
    JSHandle<JSObject> nullHandle(thread_, JSTaggedValue::Null());
    JSHandle<JSObject> obj = JSObject::ObjectCreate(thread_, nullHandle);
    JSHandle<JSTaggedValue> objValue(obj);
    [[maybe_unused]] bool makeConstructor = JSFunction::MakeConstructor(thread_, func, objValue);
    JSHandle<JSTaggedValue> funcHandle(func);
    Local<JSValueRef> funConstructor = JSNApiHelper::ToLocal<JSValueRef>(funcHandle);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsConstructor(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsConstructor::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsConstructor(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsConstructor::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        funConstructor->IsConstructor(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsConstructor::funConstructor);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsDate)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    double timeRef = 1.1; // 1.1 = random number
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<DateRef> dateRef = DateRef::New(vm_, timeRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsDate(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsDate::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsDate(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsDate::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsDate(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsDate::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsDate(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsDate::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        dateRef->IsDate(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsDate::DateRef);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsError)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsError(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsError::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsError(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsError::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsError(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsError::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsError(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsError::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        error->IsError(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsError::Exception::Error);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsMap)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 0;               // 0 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<MapRef> mapRef = MapRef::New(vm_);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsMap(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMap::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsMap(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMap::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsMap(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMap::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsMap(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMap::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        mapRef->IsMap(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMap::MapRef);
}

HWTEST_F_L0(JSNApiSplTest, Local_operator01)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 13;              // 13 = random number
    int length = 3;            // 3 = ArrayBufer length
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<ArrayRef> arrayObject = ArrayRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<JSValueRef> integerVaule(intValue);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<JSValueRef> NumberVaule(resUnit32);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<JSValueRef> bigIntVaule(maxBigintUint64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<JSValueRef> stringVaule(stringUtf8);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<JSValueRef> arrayVaule(arrayObject);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator::ArrayRef);
}

HWTEST_F_L0(JSNApiSplTest, Local_operator02)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 13;              // 13 = random number
    int length = 3;            // 3 = ArrayBufer length
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<ArrayRef> arrayObject = ArrayRef::New(vm_, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue->IsNull();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator2::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32->IsNull();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator2::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->IsNull();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator2::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8->IsNull();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator2::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        arrayObject->IsNull();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::Operator2::ArrayRef);
}

HWTEST_F_L0(JSNApiSplTest, Local_IsEmpty)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    int num = 13;              // 13 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<JSValueRef> JSvalue;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue.IsEmpty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::IsEmpty::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        resUnit32.IsEmpty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::IsEmpty::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64.IsEmpty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::IsEmpty::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        stringUtf8.IsEmpty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::IsEmpty::StringRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSvalue.IsEmpty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::IsEmpty);
}

HWTEST_F_L0(JSNApiSplTest, Local_IsNull)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 13; // 13 = random number
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<JSValueRef> JSvalue;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        intValue.IsNull();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::IsNull::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSvalue.IsNull();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(Local::IsNull);
}

HWTEST_F_L0(JSNApiSplTest, CopyableGlobal_ToLocal)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 13; // 13 = random number
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    CopyableGlobal<IntegerRef> copyGlobal(vm_, intValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobal.ToLocal();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::ToLocal);
}

HWTEST_F_L0(JSNApiSplTest, CopyableGlobal_Empty)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 13; // 13 = random number
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    CopyableGlobal<IntegerRef> copyGlobal(vm_, intValue);
    EXPECT_FALSE(copyGlobal.IsEmpty());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobal.Empty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::Empty);
    EXPECT_TRUE(copyGlobal.IsEmpty());
}

HWTEST_F_L0(JSNApiSplTest, CopyableGlobal_operator)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 13; // 13 = random number
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    CopyableGlobal<IntegerRef> copyGlobal(vm_, intValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (*copyGlobal)->IsInt();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::Operator);
    EXPECT_TRUE((*copyGlobal)->IsInt());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobal->IsInt();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::Operator2);
    EXPECT_TRUE(copyGlobal->IsInt());
}

HWTEST_F_L0(JSNApiSplTest, CopyableGlobal_IsEmpty)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string testUtf8 = "Hello world";
    int num = 13; // 13 = random number
    Local<IntegerRef> intValue = IntegerRef::New(vm_, num);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    CopyableGlobal<StringRef> copyGlbString(vm_, stringUtf8);
    CopyableGlobal<IntegerRef> copyGlobal(vm_, intValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlbString.IsEmpty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::IsEmpty::StringRef::first);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobal.IsEmpty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::IsEmpty::IntegerRef);
    copyGlbString.Empty();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlbString.IsEmpty();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::IsEmpty::StringRef::Second);
}

HWTEST_F_L0(JSNApiSplTest, CopyableGlobal_SetWeak_IsWeak_ClearWeak)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string testUtf8 = "Hello world";
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    CopyableGlobal<JSValueRef> copyGlobalback(vm_, stringUtf8);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalback.IsWeak();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::IsWeak::First);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalback.SetWeak();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::SetWeak);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalback.IsWeak();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::IsWeak::Second);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalback.ClearWeak();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::ClearWeak);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalback.IsWeak();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::IsWeak::three);
}

HWTEST_F_L0(JSNApiSplTest, CopyableGlobal_SetWeakCallback)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    double doubleSize = 1.1; // 1.1 = random number
    long longSize = 123456;  // 123456 = random number
    std::string testUtf8 = "Hello world";
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    CopyableGlobal<JSValueRef> copyGlobalStr(vm_, stringUtf8);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalStr.SetWeakCallback(&inputUnit32, FreeGlobalCallBack<uint32_t>,
                NativeFinalizeCallback<uint32_t>);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::SetWeakCallback::Uint32);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalStr.SetWeakCallback(&maxUint64, FreeGlobalCallBack<uint64_t>,
                NativeFinalizeCallback<uint64_t>);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::SetWeakCallback::Uint64);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalStr.SetWeakCallback(&doubleSize, FreeGlobalCallBack<double>,
                NativeFinalizeCallback<double>);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::SetWeakCallback::Double);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalStr.SetWeakCallback(&longSize, FreeGlobalCallBack<long>,
                NativeFinalizeCallback<long>);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::SetWeakCallback::Long);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobalStr.SetWeakCallback(&testUtf8, FreeGlobalCallBack<std::string>,
                NativeFinalizeCallback<std::string>);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::SetWeakCallback::String);
}

HWTEST_F_L0(JSNApiSplTest, BigIntRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    uint64_t minUint64 = std::numeric_limits<uint64_t>::min();
    int64_t maxInt64 = std::numeric_limits<int64_t>::max();
    int64_t minInt64 = std::numeric_limits<int64_t>::min();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        BigIntRef::New(vm_, maxUint64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::New::Uint64::Max);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        BigIntRef::New(vm_, minUint64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::New::Uint64::Min);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        BigIntRef::New(vm_, maxInt64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::New::Int64::Max);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        BigIntRef::New(vm_, minInt64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::New::Int64::Min);
}

HWTEST_F_L0(JSNApiSplTest, BigIntRef_CreateBigWords)
{
    LocalScope scope(vm_);
    CalculateForTime();
    bool sign = false;
    uint32_t size = 3; // 3 = random number
    const uint64_t words[3] = {
        std::numeric_limits<uint64_t>::min() - 1,
        std::numeric_limits<uint64_t>::min(),
        std::numeric_limits<uint64_t>::max(),
    };
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        BigIntRef::CreateBigWords(vm_, sign, size, words);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::CreateBigWords);
}

HWTEST_F_L0(JSNApiSplTest, BigIntRef_GetWordsArraySize)
{
    LocalScope scope(vm_);
    CalculateForTime();
    bool sign = false;
    uint32_t size = 3; // 3 = random number
    const uint64_t words[3] = {
        std::numeric_limits<uint64_t>::min() - 1,
        std::numeric_limits<uint64_t>::min(),
        std::numeric_limits<uint64_t>::max(),
    };
    Local<JSValueRef> bigWords = BigIntRef::CreateBigWords(vm_, sign, size, words);
    Local<BigIntRef> bigWordsRef(bigWords);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bigWordsRef->GetWordsArraySize(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::GetWordsArraySize);
}

HWTEST_F_L0(JSNApiSplTest, BigIntRef_GetWordsArray)
{
    LocalScope scope(vm_);
    CalculateForTime();
    bool sign = false;
    uint32_t size = 3; // 3 = random number
    const uint64_t words[3] = {
        std::numeric_limits<uint64_t>::min() - 1,
        std::numeric_limits<uint64_t>::min(),
        std::numeric_limits<uint64_t>::max(),
    };
    Local<JSValueRef> bigWords = BigIntRef::CreateBigWords(vm_, sign, size, words);
    Local<BigIntRef> bigWordsRef(bigWords);
    bool resultSignBit = true;
    uint64_t *resultWords = new uint64_t[3]();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bigWordsRef->GetWordsArray(vm_, &resultSignBit, size, resultWords);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::GetWordsArray::Uint64);
}

HWTEST_F_L0(JSNApiSplTest, BigIntRef_BigIntToInt64)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    int64_t toNum;
    bool lossless = true;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->BigIntToInt64(vm_, &toNum, &lossless);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::BigIntToInt64);
}

HWTEST_F_L0(JSNApiSplTest, BigIntRef_BigIntToUint64)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    uint64_t toNum;
    bool lossless = true;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        maxBigintUint64->BigIntToUint64(vm_, &toNum, &lossless);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(BigIntRef::BigIntToUint64);
}

HWTEST_F_L0(JSNApiSplTest, SymbolRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string testUtf8 = "Hello world";
    Local<StringRef> description = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        SymbolRef::New(vm_, description);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(SymbolRef::New::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, SymbolRef_GetDescription)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string testUtf8 = "Hello world";
    Local<StringRef> description = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    Local<SymbolRef> symbol = SymbolRef::New(vm_, description);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        symbol->GetDescription(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(SymbolRef::GetDescription);
}

HWTEST_F_L0(JSNApiSplTest, NativePointerRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *vps = static_cast<void *>(new std::string("test"));
    void *vpd = new double(123.456);
    void *vpc = new char('a');
    void *vpl = new long(123456);
    void *vpi = new int(123);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vps);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::String);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vpd);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Double);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vpc);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Char);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vpl);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Long);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vpi);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Int);
}

HWTEST_F_L0(JSNApiSplTest, NativePointerRef_New_Fun)
{
    LocalScope scope(vm_);
    CalculateForTime();
    NativePointerCallback callBack = nullptr;
    void *vps = static_cast<void *>(new std::string("test"));
    void *vps1 = static_cast<void *>(new std::string("test"));
    void *vpd = new double(123.456);
    void *vpd1 = new double(123.456);
    void *vpc = new char('a');
    void *vpc1 = new char('e');
    void *vpl = new long(123456);
    void *vpl1 = new long(123456);
    void *vpi = new int(123);
    void *vpi1 = new int(123);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vps, callBack, vps1, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Fun::String);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vpd, callBack, vpd1, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Fun::Double);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vpc, callBack, vpc1, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Fun::Char);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vpl, callBack, vpl1, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Fun::Long);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        NativePointerRef::New(vm_, vpi, callBack, vpi1, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::New::Fun::Int);
}

HWTEST_F_L0(JSNApiSplTest, NativePointerRef_Value)
{
    LocalScope scope(vm_);
    CalculateForTime();
    void *vps = static_cast<void *>(new std::string("test"));
    void *vps1 = static_cast<void *>(new std::string("test"));
    void *vpd = new double(123.456);
    void *vpd1 = new double(123.456);
    void *vpc = new char('a');
    void *vpc1 = new char('c');
    void *vpl = new long(123456);
    void *vpl1 = new long(123456);
    void *vpi = new int(123);
    void *vpi1 = new int(123);
    Local<NativePointerRef> res_vps = NativePointerRef::New(vm_, vps, NativeAreaAllocator::FreeBufferFunc, vps1, 0);
    Local<NativePointerRef> res_vpd = NativePointerRef::New(vm_, vpd, NativeAreaAllocator::FreeBufferFunc, vpd1, 0);
    Local<NativePointerRef> res_vpc = NativePointerRef::New(vm_, vpc, NativeAreaAllocator::FreeBufferFunc, vpc1, 0);
    Local<NativePointerRef> res_vpl = NativePointerRef::New(vm_, vpl, NativeAreaAllocator::FreeBufferFunc, vpl1, 0);
    Local<NativePointerRef> res_vpi = NativePointerRef::New(vm_, vpi, NativeAreaAllocator::FreeBufferFunc, vpi1, 0);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res_vps->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::Value::String);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res_vpd->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::Value::Double);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res_vpc->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::Value::Char);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res_vpl->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::Value::Long);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res_vpi->Value();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(NativePointerRef::Value::Int);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        FunctionRef::New(vm_, FunCallback);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::New);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_NewClassFunction)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        FunctionRef::NewClassFunction(vm_, FunCallback, nullptr, nullptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::NewClassFunction);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_Call)
{
    LocalScope scope(vm_);
    CalculateForTime();
    uint32_t inputUnit32 = 32; // 32 = random number
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
    Local<JSValueRef> targetBool = BooleanRef::New(vm_, false);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    std::vector<Local<JSValueRef>> argumentsInt;
    std::vector<Local<JSValueRef>> argumentsBool;
    std::vector<Local<JSValueRef>> argumentsNum;
    std::vector<Local<JSValueRef>> argumentsBig;
    argumentsInt.emplace_back(intValue);
    argumentsBool.emplace_back(targetBool);
    argumentsNum.emplace_back(resUnit32);
    argumentsBig.emplace_back(maxBigintUint64);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunCallback);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        callback->Call(vm_, JSValueRef::Undefined(vm_), argumentsInt.data(), argumentsInt.size());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::Call::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        callback->Call(vm_, JSValueRef::Undefined(vm_), argumentsBool.data(), argumentsBool.size());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::Call::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        callback->Call(vm_, JSValueRef::Undefined(vm_), argumentsNum.data(), argumentsNum.size());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::Call::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        callback->Call(vm_, JSValueRef::Undefined(vm_), argumentsBig.data(), argumentsBig.size());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::Call::BigIntRef);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_Call2)
{
    LocalScope scope(vm_);
    CalculateForTime();
    std::string testUtf8 = "Hello world";
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    std::vector<Local<JSValueRef>> argumentsStr;
    argumentsStr.emplace_back(stringUtf8);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunCallback);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        callback->Call(vm_, JSValueRef::Undefined(vm_), argumentsStr.data(), argumentsStr.size());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::Call::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_Constructor)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> cls = FunctionRef::NewClassFunction(vm_, FunCallback, nullptr, nullptr);
    Local<JSValueRef> argv[1];          // 1 = Array Size
    int num = 13;                      // 13 = random number
    argv[0] = NumberRef::New(vm_, num); // 0 = The first element
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        cls->Constructor(vm_, argv, 1);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::Constructor);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_GetFunctionPrototype)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> set = env->GetBuiltinsSetFunction();
    Local<FunctionRef> setLocal = JSNApiHelper::ToLocal<FunctionRef>(set);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        [[maybe_unused]] Local<JSValueRef> funcPrototype = setLocal->GetFunctionPrototype(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::GetFunctionPrototype);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_Inherit)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> set = env->GetBuiltinsSetFunction();
    Local<FunctionRef> setLocal = JSNApiHelper::ToLocal<FunctionRef>(set);
    JSHandle<JSTaggedValue> map = env->GetBuiltinsMapFunction();
    Local<FunctionRef> mapLocal = JSNApiHelper::ToLocal<FunctionRef>(map);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        mapLocal->Inherit(vm_, setLocal);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::Inherit);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_SetName)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> res = FunctionRef::NewClassFunction(vm_, FunCallback, nullptr, nullptr);
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "origin test");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->SetName(vm_, origin);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::SetName);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_GetName)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> res = FunctionRef::NewClassFunction(vm_, FunCallback, nullptr, nullptr);
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "origin test");
    res->SetName(vm_, origin);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->GetName(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::GetName);
}

HWTEST_F_L0(JSNApiSplTest, FunctionRef_IsNative)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<FunctionRef> res = FunctionRef::NewClassFunction(vm_, FunCallback, nullptr, nullptr);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        res->IsNative(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(FunctionRef::IsNative);
}

HWTEST_F_L0(JSNApiSplTest, ArrayRef_New)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        Local<ArrayRef> arrayObj = ArrayRef::New(vm_, 3);
        EXPECT_TRUE(arrayObj->IsArray(vm_));
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayRef::New);
}

HWTEST_F_L0(JSNApiSplTest, ArrayRef_Length)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayRef> arrayObj = ArrayRef::New(vm_, 3);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        arrayObj->Length(vm_);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayRef::Length);
}

HWTEST_F_L0(JSNApiSplTest, ArrayRef_SetValueAt)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayRef> arrayObj = ArrayRef::New(vm_, 1);
    uint32_t inputUnit32 = 32;
    uint64_t maxUint64 = std::numeric_limits<uint64_t>::max();
    std::string testUtf8 = "Hello world";
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 0);
    Local<JSValueRef> targetBool = BooleanRef::New(vm_, false);
    Local<NumberRef> resUnit32 = NumberRef::New(vm_, inputUnit32);
    Local<BigIntRef> maxBigintUint64 = BigIntRef::New(vm_, maxUint64);
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ArrayRef::SetValueAt(vm_, arrayObj, 0, intValue);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayRef::SetValueAt::IntegerRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ArrayRef::SetValueAt(vm_, arrayObj, 0, targetBool);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayRef::SetValueAt::BooleanRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ArrayRef::SetValueAt(vm_, arrayObj, 0, resUnit32);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayRef::SetValueAt::NumberRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ArrayRef::SetValueAt(vm_, arrayObj, 0, maxBigintUint64);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayRef::SetValueAt::BigIntRef);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ArrayRef::SetValueAt(vm_, arrayObj, 0, stringUtf8);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayRef::SetValueAt::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, ArrayRef_GetValueAt)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayRef> arrayObj = ArrayRef::New(vm_, 1);
    std::string testUtf8 = "Hello world";
    Local<StringRef> stringUtf8 = StringRef::NewFromUtf8(vm_, testUtf8.c_str());
    ArrayRef::SetValueAt(vm_, arrayObj, 0, stringUtf8);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ArrayRef::GetValueAt(vm_, arrayObj, 0);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(ArrayRef::GetValueAt::StringRef);
}

HWTEST_F_L0(JSNApiSplTest, CopyableGlobal_GetEcmaVM)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<IntegerRef> intValue = IntegerRef::New(vm_, 13);
    CopyableGlobal<IntegerRef> copyGlobal(vm_, intValue);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        copyGlobal.GetEcmaVM();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(CopyableGlobal::GetEcmaVM);
}
}
