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

#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_locale.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/base/string_helper.h"
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
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_stack.h"
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
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/js_module_deregister.h"
#include "ecmascript/module/js_module_record.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_module_namespace.h"
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
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
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
    Deleter deleter = nullptr;
    void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    bool callNative = true;
    size_t nativeBindingSize = 15;
    Local<FunctionRef> obj(FunctionRef::NewClassFunction(vm_, FunCallback, deleter, cb, callNative, nativeBindingSize));
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        (void)obj->IsFunction();
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
        JSNApiHelper::ToLocal<SetRef>(setTag)->IsSet();
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
        genObject->IsGeneratorFunction();
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
        obj->IsGeneratorFunction();
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
    Deleter deleter = [](void *buffer, void *data) -> void {
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
        (void)arrayBuffer->IsArrayBuffer();
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
    Deleter deleter = [](void *buffer, void *data) -> void {
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
        (void)bufferRef->IsBuffer();
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
        (void)typedArray->IsUint32Array();
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
        (void)typedArray->IsFloat32Array();
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
        (void)floatArray->IsFloat64Array();
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
        JSNApiHelper::ToLocal<JSValueRef>(arryListTag)->IsArrayList();
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
    Deleter deleter = [](void *buffer, void *data) -> void {
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
        (void)arrayBuffer->IsDetach();
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
        (void)obj->GetBuffer();
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
        JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsArgumentsObject();
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
        JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsAsyncFunction();
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
        JSNApiHelper::ToLocal<ObjectRef>(argumentTag)->IsJSLocale();
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
        (void)JSNApiHelper::ToLocal<JSValueRef>(deQue)->IsDeque();
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
        JSNApiHelper::ToLocal<StringRef>(setTag)->IsStringIterator();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsStringIterator);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<StringRef>(setTag)->IsStringIterator();
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUint8Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
    Local<Uint8ArrayRef> object = Uint8ArrayRef::New(vm_, buffer, 4, 5);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsUint8Array();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsUint8Array);
    GTEST_LOG_(INFO) << std::boolalpha << object->IsUint8Array();
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt8Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
    Local<ObjectRef> object = Int8ArrayRef::New(vm_, buffer, 4, 5);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsInt8Array();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt8Array);
    GTEST_LOG_(INFO) << std::boolalpha << object->IsInt8Array();
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBigInt64Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
    Local<ObjectRef> object = BigInt64ArrayRef::New(vm_, buffer, 4, 5);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsBigInt64Array();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigInt64Array);
    GTEST_LOG_(INFO) << std::boolalpha << object->IsBigInt64Array();
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsBigUint64Array)
{
    LocalScope scope(vm_);
    CalculateForTime();
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, 5);
    Local<ObjectRef> object = BigUint64ArrayRef::New(vm_, buffer, 4, 5);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        object->IsBigUint64Array();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsBigUint64Array);
    GTEST_LOG_(INFO) << std::boolalpha << object->IsBigUint64Array();
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
        jsValueRef->IsJSPrimitiveRef();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPrimitiveRef);
    GTEST_LOG_(INFO) << std::boolalpha << jsValueRef->IsJSPrimitiveRef();
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
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSDateTimeFormat();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSDateTimeFormat);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSDateTimeFormat();
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
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSRelativeTimeFormat();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSRelativeTimeFormat);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSRelativeTimeFormat();
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
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSIntl();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSIntl);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSIntl();
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
        JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSNumberFormat();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSNumberFormat);
    GTEST_LOG_(INFO) << std::boolalpha << JSNApiHelper::ToLocal<JSValueRef>(jsTaggedValue)->IsJSNumberFormat();
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
        ASSERT_TRUE(tag->IsHashMap());
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
        ASSERT_TRUE(tag->IsHashSet());
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
        ASSERT_TRUE(tag->IsLightWeightMap());
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
        ASSERT_TRUE(tag->IsLightWeightSet());
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

HWTEST_F_L0(JSNApiSplTest, HasWritable_true)
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

HWTEST_F_L0(JSNApiSplTest, HasConfigurable_false)
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

HWTEST_F_L0(JSNApiSplTest, HasEnumerable_true)
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
}
