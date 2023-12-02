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
        object->GetOriginalFlags();
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
        bool b = tag->BooleaValue();
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
        bool b = tag->BooleaValue();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_BooleaValue_false);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IntegerValue_Int64)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int64_t num = 0xffffffffffff; // 0xffffffffffff = 超过32位的一个数字
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
    double num = 123.456; // 123.456 = 随机构造的一个double
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
    int num = 789; // 123.456 = 随机构造的一个int
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
    unsigned int num = 456; // 123.456 = 随机构造的一个unsigned int
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
    void *vp1 = static_cast<void *>(new int(123)); // 123 = 随机构造的一个int
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
    void *vp1 = static_cast<void *>(new double(123.456)); // 123.456 = 随机构造的一个double
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
    void *vp1 = static_cast<void *>(new long(123456)); // 123456 = 随机构造的一个long
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
    int num = 123; // 123 = 随机构造的一个int
    Local<JSValueRef> tag = IntegerRef::New(vm_,  num);
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
    int num = 123; // 123 = 随机构造的一个int
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
    int num = 456; // 456 = 随机构造的一个int
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
    int num = 123; // 123 = 随机构造的一个int
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
        bool b = tag->IsString();
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
        bool b = tag->IsString();
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
        bool b = tag->IsProxy();
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
        bool b = tag->IsProxy();
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
        bool b = tag->IsPromise();
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
        bool b = tag->IsPromise();
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
        bool b = tag->IsDataView();
        UNUSED(b);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef_IsDataView_False);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsDataView_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 0; // 0 = 随机构造的一个int
    Local<JSValueRef> tag = DataViewRef::New(vm_, ArrayBufferRef::New(vm_, num), num, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        bool b = tag->IsDataView();
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
        bool b = tag->IsWeakRef();
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
        bool b = tag->IsWeakMap();
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
        bool b = tag->IsWeakMap();
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
        bool b = tag->IsWeakSet();
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
        bool b = tag->IsWeakSet();
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
        bool b = (*global)->BooleaValue();
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
        bool b = global->BooleaValue();
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
    void *ref = new int(123); // 123 = 随机构造的一个int
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
    void *ref = new double(123.456); // 123.456 = 随机构造的一个double
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
    void *ref = new long(123456); // 123456 = 随机构造的一个long
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
        std::string str = local->ToString();
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
        uint32_t length = local->Length();
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
        int length = local->WriteUtf8(cs, 6);
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
    char cs[16] = {0}; // 16 = 字符数组的大小
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int length = local->WriteUtf8(cs, local->Length());
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
    char cs[16] = {0}; // 16 = 字符数组的大小
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int length = local->WriteUtf8(cs, 0);
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
    char cs[16] = {0}; // 16 = 字符数组的大小
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int length = local->WriteUtf8(cs, 6, true);
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
    char cs[16] = {0}; // 16 = 字符数组的大小
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int length = local->WriteUtf8(cs, local->Length(), true);
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
    char16_t cs[16] = {0}; // 16 = 字符数组的大小
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int length = local->WriteUtf16(cs, 3);
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
    char cs[16] = {0}; // 16 = 字符数组的大小
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        int length = local->WriteLatin1(cs, 8);
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
    void *par = new long(123456); // 123456 = 随机构造的long
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
    Local<JSValueRef> transfer = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer);
        UNUSED(ptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SerializeValue_Bool);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_SerializeValue_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 123; // 123 = 随机构造的int
    Local<JSValueRef> value = IntegerRef::New(vm_, num);
    Local<JSValueRef> transfer = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer);
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
    Local<JSValueRef> transfer = StringRef::NewFromUtf8(vm_, "abcdefbb");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer);
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
        void *ptr = JSNApi::SerializeValue(vm_, value, value);
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
    Local<JSValueRef> transfer = StringRef::NewFromUtf8(vm_, "abcdefbbghijkl");
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer);
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
    Local<JSValueRef> transfer = BooleanRef::New(vm_, true);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *ptr = JSNApi::SerializeValue(vm_, value, transfer);
        UNUSED(ptr);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_SerializeValue_String_Bool);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, StringRef::NewFromUtf8(vm_, "abcdefbb"),
            StringRef::NewFromUtf8(vm_, "abcdefbb"));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm_, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeserializeValue_String);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_Bool)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, BooleanRef::New(vm_, true), BooleanRef::New(vm_, true));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm_, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeserializeValue_Bool);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_Int)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, IntegerRef::New(vm_, 123), IntegerRef::New(vm_, 123));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm_, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeserializeValue_Int);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_Undefined)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, JSValueRef::Undefined(vm_), JSValueRef::Undefined(vm_));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm_, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeserializeValue_Undefined);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeserializeValue_Null)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *recoder = JSNApi::SerializeValue(vm_, JSValueRef::Null(vm_), JSValueRef::Null(vm_));
        void *hint = nullptr;
        Local<JSValueRef> local = JSNApi::DeserializeValue(vm_, recoder, hint);
        UNUSED(local);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DeserializeValue_Null);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DeleteSerializationData_String)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        void *data = JSNApi::SerializeValue(vm_, StringRef::NewFromUtf8(vm_, "abcdefbb"),
            StringRef::NewFromUtf8(vm_, "abcdefbb"));
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
        void *data = JSNApi::SerializeValue(vm_, BooleanRef::New(vm_, true), BooleanRef::New(vm_, true));
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
        void *data = JSNApi::SerializeValue(vm_, BooleanRef::New(vm_, true), BooleanRef::New(vm_, true));
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
        void *data = JSNApi::SerializeValue(vm_, JSValueRef::Undefined(vm_), JSValueRef::Undefined(vm_));
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
        void *data = JSNApi::SerializeValue(vm_, JSValueRef::Null(vm_), JSValueRef::Null(vm_));
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
    std::function<bool(std::string dirPath, uint8_t * *buff, size_t * buffSize)> cb = [](const std::string &inputPath,
        uint8_t **buff, size_t *buffSize) -> bool {
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

HWTEST_F_L0(JSNApiSplTest, JSNApi_InitializeIcuData)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSRuntimeOptions runtimeOptions;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::InitializeIcuData(runtimeOptions);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_InitializeIcuData);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_InitializePGOProfiler)
{
    LocalScope scope(vm_);
    CalculateForTime();
    JSRuntimeOptions runtimeOptions;
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::InitializePGOProfiler(runtimeOptions);
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_InitializePGOProfiler);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DestroyAnDataManager)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::DestroyAnDataManager();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DestroyAnDataManager);
}

HWTEST_F_L0(JSNApiSplTest, JSNApi_DestroyPGOProfiler)
{
    LocalScope scope(vm_);
    CalculateForTime();
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        JSNApi::DestroyPGOProfiler();
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSNApi_DestroyPGOProfiler);
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
        ASSERT_TRUE(object->IsMapIterator());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMapIterator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsMapIterator_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的数字
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsMapIterator());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsMapIterator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUint8ClampedArray_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int32_t length = 4; // 4 = 长度
    int32_t byteOffset = 0; // 0 = 长度
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, length);
    Local<Uint8ClampedArrayRef> object = Uint8ClampedArrayRef::New(vm_, buffer, byteOffset, length);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsUint8ClampedArray());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsUint8ClampedArray);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsUint8ClampedArray_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的数字
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsUint8ClampedArray());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsUint8ClampedArray);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt16Array_True)
{
    LocalScope scope(vm_);
    CalculateForTime();
    const int32_t length = 30; // 30 = 长度
    int32_t byteOffset = 4; // 4 = 偏移量
    int32_t len = 6; // 6 = 长度
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    Local<Int16ArrayRef> object = Int16ArrayRef::New(vm_, arrayBuffer, byteOffset, len);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsInt16Array());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsInt16Array);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsInt16Array_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的数字
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsInt16Array());
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
        object->IsJSPrimitiveNumber();
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
        ASSERT_FALSE(object->IsJSPrimitiveNumber());
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
        object->IsJSPrimitiveInt();
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
        object->IsJSPrimitiveBoolean();
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
        ASSERT_FALSE(object->IsJSPrimitiveBoolean());
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
        ASSERT_TRUE(object->IsJSCollator());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSCollator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSCollator_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的int
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSCollator());
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
        ASSERT_TRUE(object->IsJSPluralRules());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPluralRules);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSPluralRules_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的int
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSPluralRules());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsJSPluralRules);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsJSListFormat_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的int
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsJSListFormat());
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
    JSHandle<Method> method = factory->NewMethod(methodLiteral);
    JSHandle<JSFunction> asyncGeneratorFunction = factory->NewJSAsyncGeneratorFunction(method);
    JSHandle<JSTaggedValue> asyncgefu = JSHandle<JSTaggedValue>::Cast(asyncGeneratorFunction);
    Local<JSValueRef> object = JSNApiHelper::ToLocal<JSValueRef>(asyncgefu);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_TRUE(object->IsAsyncGeneratorFunction());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsAsyncGeneratorFunction);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsAsyncGeneratorFunction_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的int
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsAsyncGeneratorFunction());
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
        ASSERT_TRUE(object->IsLinkedList());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsLinkedList);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsLinkedList_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的int
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsLinkedList());
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
    uint32_t argvLength = 6; // 6 = 参数长度
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
    uint32_t elementsNum = 256; // 256 = 循环次数
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
        ASSERT_TRUE(object->IsLinkedListIterator());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsLinkedListIterator);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsLinkedListIterator_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的int
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    gettimeofday(&g_beginTime, nullptr);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsLinkedListIterator());
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
    uint32_t argvLength = 6; // 6 = 参数长度
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
        ASSERT_TRUE(object->IsList());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsList);
}

HWTEST_F_L0(JSNApiSplTest, JSValueRef_IsList_False)
{
    LocalScope scope(vm_);
    CalculateForTime();
    int num = 10; // 10 = 随机构造的int
    Local<JSValueRef> object = IntegerRef::New(vm_, num);
    for (int i = 0; i < NUM_COUNT; i++) {
        ASSERT_FALSE(object->IsList());
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
    uint32_t argvLength = 6; // 6 = 参数长度
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
        ASSERT_TRUE(object->IsPlainArray());
    }
    gettimeofday(&g_endTime, nullptr);
    TEST_TIME(JSValueRef::IsPlainArray);
}
}
