/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "assembler/assembly-emitter.h"
#include "assembler/assembly-parser.h"
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
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/ic/ic_info.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_path_helper.h"
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

static constexpr char16_t UTF_16[] = u"This is a char16 array";
static constexpr char ASCII[] = "hello world!";
static constexpr int TEST_REPEAT_COUNT = 32;
static constexpr const char *DUPLICATE_KEY = "duplicateKey";
static constexpr const char *SIMPLE_KEY = "simpleKey";
static constexpr const char ERROR_MESSAGE[] = "ErrorTest";
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
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
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

    uintptr_t JSNApiGetXRefGlobalHandleAddr(const EcmaVM *vm, uintptr_t localAddress)
    {
        return JSNApi::GetXRefGlobalHandleAddr(vm, localAddress);
    }

    void JSNApiDisposeXRefGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr)
    {
        JSNApi::DisposeXRefGlobalHandleAddr(vm, addr);
    }

protected:
    void RegisterStringCacheTable();
    std::shared_ptr<JSPandaFile> NewMockJSPandaFile(const char *data, const CString &filename,
        const CString &targetFileName, CString &entryPoint) const;

    JSThread *thread_ = nullptr;
    EcmaVM *vm_ = nullptr;
    bool isStringCacheTableCreated_ = false;
};

void JSNApiTests::RegisterStringCacheTable()
{
    constexpr uint32_t STRING_CACHE_TABLE_SIZE = 100;
    auto res = ExternalStringCache::RegisterStringCacheTable(vm_, STRING_CACHE_TABLE_SIZE);
    if (isStringCacheTableCreated_) {
        ASSERT_FALSE(res);
    } else {
        ASSERT_TRUE(res);
    }
}

std::shared_ptr<JSPandaFile> JSNApiTests::NewMockJSPandaFile(const char *data,
    const CString &filename, const CString &targetFileName, CString &entryPoint) const
{
    panda::pandasm::Parser parser;
    auto res = parser.Parse(data);
    JSPandaFileManager *pfManager = JSPandaFileManager::GetInstance();
    std::unique_ptr<const panda::panda_file::File> pfPtr = pandasm::AsmEmitter::Emit(res.Value());
    std::shared_ptr<JSPandaFile> pf = pfManager->NewJSPandaFile(pfPtr.release(), filename);
    pf->SetBundlePack(false);
    entryPoint = panda::ecmascript::ModulePathHelper::ConcatFileNameWithMerge(thread_, pf.get(),
        const_cast<CString &>(filename), "", targetFileName);
    pf->InsertJSRecordInfo(entryPoint);
    pfManager->AddJSPandaFile(pf);
    return pf;
}

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

struct StackInfo {
    uint64_t stackLimit;
    uint64_t lastLeaveFrame;
};

/**
 * @tc.number: ffi_interface_api_105
 * @tc.name: JSValueRef_IsGeneratorObject
 * @tc.desc: Determine if it is a Generator generator object
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsGeneratorObject)
{
    LocalScope scope(vm_);
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
    Local<JSValueRef> genObjectRef = JSNApiHelper::ToLocal<GeneratorObjectRef>(genObjTagHandleVal);
    ASSERT_TRUE(genObjectRef->IsGeneratorObject(vm_));
}

static JSFunction *JSObjectTestCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

/**
 * @tc.number: ffi_interface_api_106
 * @tc.name: JSValueRef_IsProxy
 * @tc.desc: Determine if it is the type of proxy
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSValueRef_IsProxy)
{
    LocalScope scope(vm_);
    JSHandle<JSTaggedValue> hclass(thread_, JSObjectTestCreate(thread_));
    JSHandle<JSTaggedValue> targetHandle(
        thread_->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));

    JSHandle<JSTaggedValue> key(thread_->GetEcmaVM()->GetFactory()->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread_, JSTaggedValue(1));
    JSObject::SetProperty(thread_, targetHandle, key, value);
    EXPECT_EQ(JSObject::GetProperty(thread_, targetHandle, key).GetValue()->GetInt(), 1);

    JSHandle<JSTaggedValue> handlerHandle(
        thread_->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(hclass), hclass));
    EXPECT_TRUE(handlerHandle->IsECMAObject());

    JSHandle<JSProxy> proxyHandle = JSProxy::ProxyCreate(thread_, targetHandle, handlerHandle);
    EXPECT_TRUE(*proxyHandle != nullptr);

    EXPECT_EQ(JSProxy::GetProperty(thread_, proxyHandle, key).GetValue()->GetInt(), 1);
    PropertyDescriptor desc(thread_);
    JSProxy::GetOwnProperty(thread_, proxyHandle, key, desc);
    EXPECT_EQ(desc.GetValue()->GetInt(), 1);
    Local<JSValueRef> proxy = JSNApiHelper::ToLocal<JSProxy>(JSHandle<JSTaggedValue>(proxyHandle));
    ASSERT_TRUE(proxy->IsProxy(vm_));
}

/**
 * @tc.number: ffi_interface_api_107
 * @tc.name: BufferRef_New_ByteLength
 * @tc.desc:
 * New：Create a buffer and specify the length size
 * ByteLength：Returns the length of the buffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, BufferRef_New_ByteLength)
{
    LocalScope scope(vm_);
    int32_t length = 10;
    Local<BufferRef> buffer = BufferRef::New(vm_, length);
    ASSERT_TRUE(buffer->IsBuffer(vm_));
    EXPECT_EQ(buffer->ByteLength(vm_), length);
}

/**
 * @tc.number: ffi_interface_api_108
 * @tc.name: BufferRef_New_ByteLength_GetBuffer
 * @tc.desc:
 * New：Create a buffer and specify the length size
 * ByteLength：Returns the length of the buffer
 * GetBuffer：Return buffer pointer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, BufferRef_New_ByteLength_GetBuffer)
{
    LocalScope scope(vm_);
    int32_t length = 10;
    Local<BufferRef> buffer = BufferRef::New(vm_, length);
    ASSERT_TRUE(buffer->IsBuffer(vm_));
    EXPECT_EQ(buffer->ByteLength(vm_), 10U);
    ASSERT_NE(buffer->GetBuffer(vm_), nullptr);
}

/**
 * @tc.number: ffi_interface_api_109
 * @tc.name: BufferRef_New01_ByteLength_GetBuffer_BufferToStringCallback
 * @tc.desc:
 * BufferToStringCallback：Implement callback when calling ToString (vm) mode in buffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, BufferRef_New01_ByteLength_GetBuffer_BufferToStringCallback)
{
    LocalScope scope(vm_);
    static bool isFree = false;
    struct Data {
        int32_t length;
    };
    const int32_t length = 15;
    NativePointerCallback deleter = []([[maybe_unused]] void *env, void *buffer, void *data) -> void {
        delete[] reinterpret_cast<uint8_t *>(buffer);
        Data *currentData = reinterpret_cast<Data *>(data);
        delete currentData;
        isFree = true;
    };
    isFree = false;
    uint8_t *buffer = new uint8_t[length]();
    Data *data = new Data();
    data->length = length;
    Local<BufferRef> bufferRef = BufferRef::New(vm_, buffer, length, deleter, data);
    ASSERT_TRUE(bufferRef->IsBuffer(vm_));
    ASSERT_TRUE(bufferRef->IsBuffer(vm_));
    EXPECT_EQ(bufferRef->ByteLength(vm_), 15U);
    Local<StringRef> bufferStr = bufferRef->ToString(vm_);
    ASSERT_TRUE(bufferStr->IsString(vm_));
}

/**
 * @tc.number: ffi_interface_api_110
 * @tc.name: LocalScope_LocalScope
 * @tc.desc: Build Escape LocalScope sub Vm scope
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, LocalScope_LocalScope)
{
    EscapeLocalScope scope(vm_);
    LocalScope *perScope = nullptr;
    perScope = &scope;
    ASSERT_TRUE(perScope != nullptr);
}

/**
 * @tc.number: ffi_interface_api_112
 * @tc.name: JSNApi_CreateJSVM_DestroyJSVM
 * @tc.desc: Create/delete JSVM
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_CreateJSVM_DestroyJSVM)
{
    LocalScope scope(vm_);
    std::thread t1([]() {
        EcmaVM *vm1_ = nullptr;
        RuntimeOption option;
        option.SetLogLevel(common::LOG_LEVEL::ERROR);
        vm1_ = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm1_ != nullptr) << "Cannot create Runtime";
        vm1_->SetEnableForceGC(true);
        vm1_->SetEnableForceGC(false);
        JSNApi::DestroyJSVM(vm1_);
    });
    {
        ecmascript::ThreadSuspensionScope suspensionScope(thread_);
        t1.join();
    }
}

/**
 * @tc.number: ffi_interface_api_114
 * @tc.name: JSNApi_GetUncaughtException
 * @tc.desc: Get uncaught exceptions
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSNApi_GetUncaughtException)
{
    LocalScope scope(vm_);
    Local<ObjectRef> excep = JSNApi::GetUncaughtException(vm_);
    ASSERT_TRUE(excep.IsNull()) << "CreateInstance occur Exception";
}

Local<JSValueRef> FuncRefNewCallbackForTest(JsiRuntimeCallInfo *info)
{
    GTEST_LOG_(INFO) << "runing FuncRefNewCallbackForTest";
    EscapeLocalScope scope(info->GetVM());
    return scope.Escape(ArrayRef::New(info->GetVM(), info->GetArgsNumber()));
}

Local<JSValueRef> ThrowingAccessorCallback(JsiRuntimeCallInfo *info)
{
    EcmaVM *vm = info->GetVM();
    Local<JSValueRef> error = Exception::Error(vm, StringRef::NewFromUtf8(vm, "FastPathAccessorError"));
    JSNApi::ThrowException(vm, error);
    return JSValueRef::Undefined(vm);
}

Local<JSValueRef> UndefinedAccessorCallback(JsiRuntimeCallInfo *info)
{
    return JSValueRef::Undefined(info->GetVM());
}

static bool g_throwInFastLoadIc = false;
static bool g_throwInFastStoreIc = false;
static int32_t g_nonThrowingGetterValue = 0;
static int32_t g_lastSetterValue = 0;
static uint32_t g_setterCallCount = 0;

Local<JSValueRef> ConditionalThrowingGetterCallback(JsiRuntimeCallInfo *info)
{
    EcmaVM *vm = info->GetVM();
    if (g_throwInFastLoadIc) {
        Local<JSValueRef> error = Exception::Error(vm, StringRef::NewFromUtf8(vm, "Tier1LoadAccessorError"));
        JSNApi::ThrowException(vm, error);
        return IntegerRef::New(vm, 123);
    }
    return IntegerRef::New(vm, 123);
}

Local<JSValueRef> ConditionalThrowingSetterCallback(JsiRuntimeCallInfo *info)
{
    EcmaVM *vm = info->GetVM();
    if (g_throwInFastStoreIc) {
        Local<JSValueRef> error = Exception::Error(vm, StringRef::NewFromUtf8(vm, "Tier1StoreAccessorError"));
        JSNApi::ThrowException(vm, error);
    }
    return JSValueRef::Undefined(vm);
}

Local<JSValueRef> StableGetterCallback(JsiRuntimeCallInfo *info)
{
    return IntegerRef::New(info->GetVM(), g_nonThrowingGetterValue);
}

Local<JSValueRef> RecordingSetterCallback(JsiRuntimeCallInfo *info)
{
    EcmaVM *vm = info->GetVM();
    if (info->GetArgsNumber() > 0) {
        Local<JSValueRef> value = info->GetCallArgRef(0);
        g_lastSetterValue = value->Int32Value(vm);
        g_setterCallCount++;
    }
    return JSValueRef::Undefined(vm);
}

IcAccessor::ICState GetCallsiteState(JSThread *thread, uintptr_t info, ICKind kind)
{
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    JSHandle<ICInfo> callsiteInfo(thread, ICInfo::Cast(infoVal.GetTaggedObject()));
    IcAccessor accessor(thread, callsiteInfo, 0, kind);
    return accessor.GetICState();
}

// Helper to verify IC state transitions: UNINIT -> MONO -> POLY -> MEGA/IC_MEGA
void VerifyICStateTransition(IcAccessor::ICState state, uint32_t index)
{
    // MONO: first access (index 0) after UNINIT
    if (index == 0) {
        EXPECT_EQ(state, IcAccessor::ICState::MONO);
        return;
    }

    // POLY: accesses 1-3 (total POLY_CASE_NUM = 4, so 0 + 4 = 4 is the last POLY)
    if (index < IcAccessor::POLY_CASE_NUM) {
        EXPECT_EQ(state, IcAccessor::ICState::POLY);
        return;
    }

    // MEGA/IC_MEGA: accesses 4 and beyond (POLY_CASE_NUM = 4 triggers MEGA transition)
    if (index >= IcAccessor::POLY_CASE_NUM) {
        EXPECT_TRUE(state == IcAccessor::ICState::MEGA ||
                    state == IcAccessor::ICState::IC_MEGA);
        return;
    }

    FAIL() << "Unexpected IC state " << static_cast<int>(state) << " at index " << index;
}

/**
 * @tc.number: ffi_interface_api_115
 * @tc.name: ObjectRef_SetAccessorProperty
 * @tc.desc:
 * SetAccessorProperty：Set AccessorPro Properties
 * GetVM：Obtain environment information for runtime calls
 * LocalScope：Build Vm Scope
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, ObjectRef_SetAccessorProperty_JsiRuntimeCallInfo_GetVM)
{
    LocalScope scope(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    FunctionForRef nativeFunc = FuncRefNewCallbackForTest;
    Local<FunctionRef> getter = FunctionRef::New(vm_, nativeFunc);
    Local<FunctionRef> setter = FunctionRef::New(vm_, nativeFunc);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    ASSERT_TRUE(object->SetAccessorProperty(vm_, key, getter, setter));
}

/*
 * @tc.number: ffi_interface_api_116
 * @tc.name: JSNApi_IsBoolean
 * @tc.desc: Judge  whether obj is a boolean type
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsBoolean)
{
    LocalScope scope(vm_);
    char16_t utf16[] = u"This is a char16 array";
    int size = sizeof(utf16);
    Local<StringRef> obj = StringRef::NewFromUtf16(vm_, utf16, size);
    ASSERT_FALSE(obj->IsBoolean());
}

/*
 * @tc.number: ffi_interface_api_117
 * @tc.name: JSNApi_IsNULL
 * @tc.desc: Judge  whether obj is a null type
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsNULL)
{
    LocalScope scope(vm_);
    char16_t utf16[] = u"This is a char16 array";
    int size = sizeof(utf16);
    Local<StringRef> obj = StringRef::NewFromUtf16(vm_, utf16, size);
    ASSERT_FALSE(obj->IsNull());
}

/*
 * @tc.number: ffi_interface_api_118
 * @tc.name: JSNApi_GetTime
 * @tc.desc: This function is to catch time
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetTime)
{
    LocalScope scope(vm_);
    const double time = 14.29;
    Local<DateRef> date = DateRef::New(vm_, time);
    ASSERT_EQ(date->GetTime(vm_), time);
}

/*
 * @tc.number: ffi_interface_api_119
 * @tc.name: JSNApi_IsDetach
 * @tc.desc: Judge whether arraybuffer is detached
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsDetach)
{
    LocalScope scope(vm_);
    const int32_t length = 33;
    Local<ArrayBufferRef> arraybuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_FALSE(arraybuffer->IsDetach(vm_));
}

/*
 * @tc.number: ffi_interface_api_120
 * @tc.name: JSNApi_Detach
 * @tc.desc: This function is to detach arraybuffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Detach)
{
    LocalScope scope(vm_);
    const int32_t length = 33;
    Local<ArrayBufferRef> arraybuffer = ArrayBufferRef::New(vm_, length);
    arraybuffer->Detach(vm_);
}

/*
 * @tc.number: ffi_interface_api_121
 * @tc.name: JSNApi_New1
 * @tc.desc:  create a obj that is a arraybuffer type
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, New1)
{
    LocalScope scope(vm_);
    const int32_t length = 33;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer(vm_));
    ASSERT_EQ(arrayBuffer->ByteLength(vm_), length);
    ASSERT_NE(arrayBuffer->GetBuffer(vm_), nullptr);
}

/*
 * @tc.number: ffi_interface_api_122
 * @tc.name: JSNApi_New1
 * @tc.desc:  create a obj that is a arraybuffer type
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, New2)
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
}

/*
 * @tc.number: ffi_interface_api_123
 * @tc.name: JSNApi_Bytelength
 * @tc.desc:   capture bytelength of arraybuffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Bytelength)
{
    LocalScope scope(vm_);
    const int32_t length = 33;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_EQ(arrayBuffer->ByteLength(vm_), length);
}

/*
 * @tc.number: ffi_interface_api_124
 * @tc.name: JSNApi_GetBuffer
 * @tc.desc:  capture buffer of arraybuffer
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetBuffer)
{
    LocalScope scope(vm_);
    const int32_t length = 33;
    Local<ArrayBufferRef> arraybuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_NE(arraybuffer->GetBuffer(vm_), nullptr);
}

/*
 * @tc.number: ffi_interface_api_125
 * @tc.name: JSNApi_Is32Arraytest
 * @tc.desc:  judge  whether obj is a int32array type,
 * judge  whether obj is a uint32array type,
 * judge  whether obj is a float32array type,
 * judge  whether obj is a float64array type,
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Is32Arraytest)
{
    LocalScope scope(vm_);
    char16_t utf16[] = u"This is a char16 array";
    int size = sizeof(utf16);
    Local<StringRef> obj = StringRef::NewFromUtf16(vm_, utf16, size);
    ASSERT_FALSE(obj->IsInt32Array(vm_));
    ASSERT_FALSE(obj->IsUint32Array(vm_));
    ASSERT_FALSE(obj->IsFloat32Array(vm_));
    ASSERT_FALSE(obj->IsFloat64Array(vm_));
}

/*
 * @tc.number: ffi_interface_api_126
 * @tc.name: JSNApi_SynchronizVMInfo
 * @tc.desc:  capture  synchronous info of vm
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SynchronizVMInfo)
{
    LocalScope scope(vm_);
    std::thread t1([this]() {
        JSRuntimeOptions option;
        EcmaVM *hostVM = JSNApi::CreateEcmaVM(option);
        {
            LocalScope scope2(hostVM);
            JSNApi::SynchronizVMInfo(vm_, hostVM);
        }
        JSNApi::DestroyJSVM(hostVM);
    });
    {
        ecmascript::ThreadSuspensionScope suspensionScope(thread_);
        t1.join();
    }
}

/*
 * @tc.number: ffi_interface_api_127
 * @tc.name: JSNApi_IsProfiling
 * @tc.desc:  judge whether vm is profiling
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsProfiling)
{
    LocalScope scope(vm_);
    ASSERT_FALSE(JSNApi::IsProfiling(vm_));
}

/*
 * @tc.number: ffi_interface_api_128
 * @tc.name: JSNApi_SetProfilerState
 * @tc.desc:  This function is to set state of profiler
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetProfilerState)
{
    bool value = true;
    bool value2 = false;
    LocalScope scope(vm_);
    JSNApi::SetProfilerState(vm_, value);
    JSNApi::SetProfilerState(vm_, value2);
}

/*
 * @tc.number: ffi_interface_api_129
 * @tc.name: JSNApi_SetLoop
 * @tc.desc:  This function is to set loop
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetLoop)
{
    LocalScope scope(vm_);
    void *data = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    JSNApi::SetLoop(vm_, data);
}

/*
 * @tc.number: ffi_interface_api_130
 * @tc.name: JSNApi_SetHostPromiseRejectionTracker
 * @tc.desc:  This function is to set host promise rejection about tracker
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetHostPromiseRejectionTracker)
{
    LocalScope scope(vm_);
    void *data = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    JSNApi::SetHostPromiseRejectionTracker(vm_, cb, data);
}

/*
 * @tc.number: ffi_interface_api_131
 * @tc.name: JSNApi_SetHostResolveBufferTracker
 * @tc.desc:   This function is to set host resolve buffer about tracker
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetHostResolveBufferTracker)
{
    LocalScope scope(vm_);
    JSNApi::SetHostResolveBufferTracker(vm_,
        [](std::string, uint8_t **, size_t *, std::string &) -> bool { return true; });
}

/*
 * @tc.number: ffi_interface_api_132
 * @tc.name: JSNApi_SetUnloadNativeModuleCallback
 * @tc.desc:   This function is to set unload native module  about callback
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetUnloadNativeModuleCallback)
{
    LocalScope scope(vm_);
    JSNApi::SetUnloadNativeModuleCallback(vm_, [](const std::string &) -> bool { return true; });
}

/*
 * @tc.number: ffi_interface_api_133
 * @tc.name: JSNApi_SetNativePtrGetter
 * @tc.desc:   This function is to set a native pointer about getter
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, SetNativePtrGetter)
{
    LocalScope scope(vm_);
    void *cb = reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf);
    JSNApi::SetNativePtrGetter(vm_, cb);
}

/*
 * @tc.number: ffi_interface_api_134
 * @tc.name: JSNApi_PreFork
 * @tc.desc:  This function is to set prefork
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, PreFork)
{
    RuntimeOption option;
    ecmascript::ThreadNativeScope nativeScope(vm_->GetJSThread());
    LocalScope scope(vm_);
    JSNApi::PreFork(vm_);
    JSNApi::PostFork(vm_, option);
}

/*
 * @tc.number: ffi_interface_api_135
 * @tc.name: JSNApi_PostFork
 * @tc.desc:  This function is to set postfork
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, PostFork)
{
    RuntimeOption option;
    ecmascript::ThreadNativeScope nativeScope(vm_->GetJSThread());
    LocalScope scope(vm_);
    JSNApi::PreFork(vm_);
    JSNApi::PostFork(vm_, option);
}


/*
 * @tc.number: ffi_interface_api_136
 * @tc.name: JSNApi_NewFromUtf8
 * @tc.desc:  create a newfromutf8 object
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, NewFromUtf8)
{
    LocalScope scope(vm_);
    char utf8[] = "hello world!";
    int length = strlen(utf8);
    Local<StringRef> result = StringRef::NewFromUtf8(vm_, utf8, length);
    ASSERT_EQ(result->Utf8Length(vm_), length + 1);
}

/*
 * @tc.number: ffi_interface_api_137
 * @tc.name: JSNApi_NewFromUtf16
 * @tc.desc:  create a newfromutf16 object
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, NewFromUtf16)
{
    LocalScope scope(vm_);
    char16_t utf16[] = u"This is a char16 array";
    int length = sizeof(utf16);
    Local<StringRef> result = StringRef::NewFromUtf16(vm_, utf16, length);
    ASSERT_EQ(result->Length(vm_), length);
}

HWTEST_F_L0(JSNApiTests, NewExternalFromAsciiLength)
{
    LocalScope scope(vm_);
    int length = strlen(ASCII);
    char *str = new char[length];
    std::copy(ASCII, ASCII + length, str);
    Local<StringRef> result = StringRef::NewExternalFromAscii(vm_, const_cast<char *>(str),
                                                              length, nullptr, nullptr);
    ASSERT_EQ(result->Utf8Length(vm_), length + 1);
    delete[] str;
}

HWTEST_F_L0(JSNApiTests, NewExternalFromUtf16Length)
{
    LocalScope scope(vm_);
    int length = sizeof(UTF_16) / sizeof(char16_t) - 1;
    char16_t *str = new char16_t[length];
    std::copy(UTF_16, UTF_16 + length, str);
    Local<StringRef> result = StringRef::NewExternalFromUtf16(vm_, const_cast<char16_t *>(str),
                                                              length, nullptr, nullptr);
    ASSERT_EQ(result->Length(vm_), length);
    delete[] str;
}

HWTEST_F_L0(JSNApiTests, NewExternalFromAsciiContent)
{
    LocalScope scope(vm_);
    int length = strlen(ASCII);
    char *str = new char[length];
    std::copy(ASCII, ASCII + length, str);
    Local<StringRef> result = StringRef::NewExternalFromAscii(vm_, const_cast<char *>(str),
                                                              length, nullptr, nullptr);
    EXPECT_EQ(result->ToString(vm_), "hello world!");
    delete[] str;
}

HWTEST_F_L0(JSNApiTests, NewExternalFromUtf16Content)
{
    LocalScope scope(vm_);
    int length = sizeof(UTF_16) / sizeof(char16_t) - 1;
    char16_t *str = new char16_t[length];
    std::copy(UTF_16, UTF_16 + length, str);
    Local<StringRef> result = StringRef::NewExternalFromUtf16(vm_, const_cast<char16_t *>(str),
                                                              length, nullptr, nullptr);
    EXPECT_EQ(result->ToString(vm_), "This is a char16 array");
    delete[] str;
}

HWTEST_F_L0(JSNApiTests, NewExternalFromAsciiOnGC)
{
    auto sHeap = SharedHeap::GetInstance();
    auto callback = [] (void* data, void* hint) -> void {
        *reinterpret_cast<int *>(hint) += 1;
        delete[] reinterpret_cast<char *>(data);
    };
    int hintNum = 0;
    {
        LocalScope scope(vm_);
        for (int i = 0; i < TEST_REPEAT_COUNT; i++) {
            int length = strlen(ASCII);
            char *str = new char[length];
            std::copy(ASCII, ASCII + length, str);
            Local<StringRef> result = StringRef::NewExternalFromAscii(vm_, const_cast<char *>(str),
                                                                      length, callback, &hintNum);
            EXPECT_EQ(result->ToString(vm_), "hello world!");
        }
        EXPECT_EQ(hintNum, 0);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), TEST_REPEAT_COUNT);
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(vm_->GetJSThread());
    EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 0);
    EXPECT_EQ(hintNum, TEST_REPEAT_COUNT);
}

HWTEST_F_L0(JSNApiTests, NewExternalFromUtf16OnGC)
{
    auto sHeap = SharedHeap::GetInstance();
    auto callback = [] (void* data, void* hint) -> void {
        *reinterpret_cast<int *>(hint) += 1;
        delete[] reinterpret_cast<char16_t *>(data);
    };
    int hintNum = 0;
    {
        LocalScope scope(vm_);
        for (int i = 0; i < TEST_REPEAT_COUNT; i++) {
            int length = sizeof(UTF_16) / sizeof(char16_t) - 1;
            char16_t *str = new char16_t[length];
            std::copy(UTF_16, UTF_16 + length, str);
            Local<StringRef> result = StringRef::NewExternalFromUtf16(vm_, const_cast<char16_t *>(str),
                                                                      length, callback, &hintNum);
            EXPECT_EQ(result->ToString(vm_), "This is a char16 array");
        }
        EXPECT_EQ(hintNum, 0);
        EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), TEST_REPEAT_COUNT);
    }
    sHeap->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(vm_->GetJSThread());
    EXPECT_EQ(sHeap->GetExternalStringTable()->GetListSize(), 0);
    EXPECT_EQ(hintNum, TEST_REPEAT_COUNT);
}

/*
 * @tc.number: ffi_interface_api_138
 * @tc.name: JSNApi_GetNapiWrapperString
 * @tc.desc:  This function is to get a napiwrapper string
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetNapiWrapperString)
{
    LocalScope scope(vm_);
    Local<StringRef> result = StringRef::GetNapiWrapperString(vm_);
    ASSERT_TRUE(result->IsString(vm_));
}

/*
 * @tc.number: ffi_interface_api_139
 * @tc.name: JSNApi_JSExecutionScope
 * @tc.desc:  This function is to construct a object of jsexecutionscope
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, JSExecutionScope)
{
    LocalScope scope(vm_);
    JSExecutionScope jsexecutionScope(vm_);
}

/*
 * @tc.number: ffi_interface_api_140
 * @tc.name: WeakMapRef_GetSize_GetTotalElements_GetKey_GetValue
 * @tc.desc:  This function is to set a weakmap and capture it's size ,
 * key, value and total elements
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, WeakMapRef_GetSize_GetTotalElements_GetKey_GetValue)
{
    LocalScope scope(vm_);
    JSThread *thread = vm_->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakMapFunction();
    JSHandle<JSWeakMap> weakMap =
        JSHandle<JSWeakMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<WeakLinkedHashMap> hashMap = WeakLinkedHashMap::Create(thread);
    weakMap->SetWeakLinkedMap(thread, hashMap);
    JSHandle<JSTaggedValue> weakMapTag = JSHandle<JSTaggedValue>::Cast(weakMap);

    Local<WeakMapRef> map = JSNApiHelper::ToLocal<WeakMapRef>(weakMapTag);
    EXPECT_TRUE(map->IsWeakMap(vm_));
    JSHandle<JSTaggedValue> value(factory->NewFromASCII("value"));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key"));
    JSWeakMap::Set(thread, weakMap, key, value);
    int32_t num = map->GetSize(vm_);
    int32_t num1 = map->GetTotalElements(vm_);
    ASSERT_EQ(num, 1);
    ASSERT_EQ(num1, 1);
    Local<JSValueRef> res1 = map->GetKey(vm_, 0);
    ASSERT_EQ(res1->ToString(vm_)->ToString(vm_), "key");
    Local<JSValueRef> res2 = map->GetValue(vm_, 0);
    ASSERT_EQ(res2->ToString(vm_)->ToString(vm_), "value");
}

/*
 * @tc.number: ffi_interface_api_141
 * @tc.name: JSNApi_ IsAGJA
 * @tc.desc:  This function is to judge whether object is a argumentsobject or
 * is a generatorfunction or is a asyncfunction
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, IsAGJA)
{
    LocalScope scope(vm_);
    char16_t utf16[] = u"This is a char16 array";
    int size = sizeof(utf16);
    Local<StringRef> obj = StringRef::NewFromUtf16(vm_, utf16, size);
    ASSERT_FALSE(obj->IsArgumentsObject(vm_));
    ASSERT_FALSE(obj->IsGeneratorFunction(vm_));
    ASSERT_FALSE(obj->IsAsyncFunction(vm_));
}

/**
 * @tc.number: ffi_interface_api_143
 * @tc.name: Int32Array
 * @tc.desc: Catch exceptions correctly
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, HasCaught)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    ASSERT_TRUE(error->IsError(vm_));
    JSNApi::ThrowException(vm_, error);
    TryCatch tryCatch(vm_);
    ASSERT_TRUE(tryCatch.HasCaught());
    vm_->GetJSThread()->ClearException();
    ASSERT_FALSE(tryCatch.HasCaught());
}

/**
 * @tc.number: ffi_interface_api_144
 * @tc.name: Int32Array
 * @tc.desc: Rethrow the exception
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, Rethrow)
{
    LocalScope scope(vm_);
    TryCatch tryCatch(vm_);
    tryCatch.Rethrow();
    ASSERT_TRUE(tryCatch.getrethrow_());
}

/**
 * @tc.number: ffi_interface_api_145
 * @tc.name: Int32Array
 * @tc.desc: Clear caught exceptions
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, GetAndClearException)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    ASSERT_TRUE(error->IsError(vm_));
    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(vm_->GetJSThread()->HasPendingException());
    TryCatch tryCatch(vm_);
    tryCatch.GetAndClearException();
    EXPECT_FALSE(vm_->GetJSThread()->HasPendingException());
}
/**
 * @tc.number: ffi_interface_api_146
 * @tc.name: Int32Array
 * @tc.desc: trycatch class construction
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, TryCatch001)
{
    LocalScope scope(vm_);
    TryCatch tryCatch(vm_);
    EXPECT_EQ(tryCatch.getrethrow_(), false);
    EXPECT_FALSE(tryCatch.HasCaught());
    tryCatch.Rethrow();
    EXPECT_EQ(tryCatch.getrethrow_(), true);
}

/**
 * @tc.number: ffi_interface_api_147
 * @tc.name: Int32Array
 * @tc.desc: trycatch class construction
 * @tc.type: FUNC
 * @tc.require:  parameter
 */
HWTEST_F_L0(JSNApiTests, TryCatch002)
{
    LocalScope scope(vm_);
    TryCatch tryCatch(vm_);
    EXPECT_EQ(JSNApi::GetEnv(vm_), tryCatch.GetEnv());
}

HWTEST_F_L0(JSNApiTests, NewObjectWithProperties)
{
    LocalScope scope(vm_);
    Local<JSValueRef> keys[1100];
    Local<JSValueRef> values[1100];
    PropertyAttribute attributes[1100];
    for (int i = 0; i < 1100; i += (i < 80 ? (i < 10 ? 1 : 80) : 1000)) {
        for (int j = 0; j <= i; ++j) {
            std::string strKey("TestKey" + std::to_string(i) + "_" + std::to_string(j));
            std::string strVal("TestValue" + std::to_string(i) + "_" + std::to_string(j));
            keys[j] = StringRef::NewFromUtf8(vm_, strKey.c_str());
            values[j] = StringRef::NewFromUtf8(vm_, strVal.c_str());
            attributes[j] = PropertyAttribute(values[j], true, true, true);
        }
        Local<ObjectRef> object = ObjectRef::NewWithProperties(vm_, i + 1, keys, attributes);
        for (int j = 0; j <= i; ++j) {
            Local<JSValueRef> value1 = object->Get(vm_, keys[j]);
            EXPECT_TRUE(values[j]->IsStrictEquals(vm_, value1));
        }
        JSHandle<JSObject> obj(JSNApiHelper::ToJSHandle(object));
        uint32_t propCount = obj->GetJSHClass()->NumberOfProps();
        if (i + 1 > PropertyAttributes::MAX_FAST_PROPS_CAPACITY) {
            EXPECT_TRUE(propCount == 0);
            EXPECT_TRUE(obj->GetJSHClass()->IsDictionaryMode());
            JSHandle<NameDictionary> dict(thread_, obj->GetProperties(thread_));
            EXPECT_TRUE(dict->EntriesCount() == i + 1);
        } else {
            EXPECT_TRUE(propCount == i + 1);
            int32_t in_idx = obj->GetJSHClass()->GetNextInlinedPropsIndex();
            int32_t nonin_idx = obj->GetJSHClass()->GetNextNonInlinedPropsIndex();
            if (i + 1 < JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS) {
                EXPECT_TRUE(in_idx == i + 1);
                EXPECT_TRUE(nonin_idx == -1);
            } else {
                EXPECT_TRUE(in_idx == -1);
                EXPECT_TRUE(nonin_idx == 0);
            }
        }
    }
}

HWTEST_F_L0(JSNApiTests, NewObjectWithPropertieNonPureStringKey)
{
    LocalScope scope(vm_);
    Local<JSValueRef> keys[] = {
        StringRef::NewFromUtf8(vm_, "1"),
    };
    Local<JSValueRef> values[] = {
        StringRef::NewFromUtf8(vm_, "value1"),
    };
    PropertyAttribute attributes[] = {
        PropertyAttribute(values[0], true, true, true),
    };
    Local<ObjectRef> object = ObjectRef::NewWithProperties(vm_, 1, keys, attributes);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(object);
    EXPECT_TRUE(obj.GetTaggedValue() == JSTaggedValue::Undefined());
    thread_->ClearException();
}

HWTEST_F_L0(JSNApiTests, NewObjectWithPropertiesDuplicate)
{
    LocalScope scope(vm_);
    Local<JSValueRef> keys[] = {
        StringRef::NewFromUtf8(vm_, DUPLICATE_KEY),
        StringRef::NewFromUtf8(vm_, SIMPLE_KEY),
        StringRef::NewFromUtf8(vm_, DUPLICATE_KEY),
    };
    Local<JSValueRef> values[] = {
        StringRef::NewFromUtf8(vm_, "value1"),
        StringRef::NewFromUtf8(vm_, "value2"),
        StringRef::NewFromUtf8(vm_, "value3"),
    };
    PropertyAttribute attributes[] = {
        PropertyAttribute(values[0], true, true, true),
        PropertyAttribute(values[1], true, true, true),
        PropertyAttribute(values[2], true, true, true),
    };
    Local<ObjectRef> object = ObjectRef::NewWithProperties(vm_, 3, keys, attributes);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(object);
    EXPECT_TRUE(obj.GetTaggedValue() == JSTaggedValue::Undefined());
    thread_->ClearException();
}

HWTEST_F_L0(JSNApiTests, NewObjectWithNamedProperties)
{
    LocalScope scope(vm_);
    const char *keys[1100];
    std::string strKeys[1100];
    Local<JSValueRef> values[1100];
    for (int i = 0; i < 1100; i += (i < 80 ? (i < 10 ? 1 : 80) : 1000)) {
        for (int j = 0; j <= i; ++j) {
            strKeys[j] = "TestKey" + std::to_string(i) + "_" + std::to_string(j);
            std::string strVal("TestValue" + std::to_string(i) + "_" + std::to_string(j));
            keys[j] = const_cast<char *>(strKeys[j].c_str());
            values[j] = StringRef::NewFromUtf8(vm_, strVal.c_str());
        }
        Local<ObjectRef> object = ObjectRef::NewWithNamedProperties(vm_, i + 1, keys, values);
        for (int j = 0; j <= i; ++j) {
            Local<JSValueRef> value1 = object->Get(vm_, StringRef::NewFromUtf8(vm_, keys[j]));
            EXPECT_TRUE(values[j]->IsStrictEquals(vm_, value1));
        }
        JSHandle<JSObject> obj(JSNApiHelper::ToJSHandle(object));
        uint32_t propCount = obj->GetJSHClass()->NumberOfProps();
        if (i + 1 > PropertyAttributes::MAX_FAST_PROPS_CAPACITY) {
            EXPECT_TRUE(propCount == 0);
            EXPECT_TRUE(obj->GetJSHClass()->IsDictionaryMode());
            JSHandle<NameDictionary> dict(thread_, obj->GetProperties(thread_));
            EXPECT_TRUE(dict->EntriesCount() == i + 1);
        } else {
            EXPECT_TRUE(propCount == i + 1);
            int32_t in_idx = obj->GetJSHClass()->GetNextInlinedPropsIndex();
            int32_t nonin_idx = obj->GetJSHClass()->GetNextNonInlinedPropsIndex();
            if (i + 1 < JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS) {
                EXPECT_TRUE(in_idx == i + 1);
                EXPECT_TRUE(nonin_idx == -1);
            } else {
                EXPECT_TRUE(in_idx == -1);
                EXPECT_TRUE(nonin_idx == 0);
            }
        }
    }
}

HWTEST_F_L0(JSNApiTests, NewObjectWithNamedPropertieNonPureStringKey)
{
    LocalScope scope(vm_);
    const char *keys[] = {
        "1",
    };
    Local<JSValueRef> values[] = {
        StringRef::NewFromUtf8(vm_, "value1"),
    };
    Local<ObjectRef> object = ObjectRef::NewWithNamedProperties(vm_, 2, keys, values);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(object);
    EXPECT_TRUE(obj.GetTaggedValue() == JSTaggedValue::Undefined());
    thread_->ClearException();
}

HWTEST_F_L0(JSNApiTests, NewObjectWithNamedPropertiesDuplicate)
{
    LocalScope scope(vm_);
    const char *keys[] = {
        DUPLICATE_KEY,
        SIMPLE_KEY,
        DUPLICATE_KEY,
    };
    Local<JSValueRef> values[] = {
        StringRef::NewFromUtf8(vm_, "value1"),
        StringRef::NewFromUtf8(vm_, "value2"),
        StringRef::NewFromUtf8(vm_, "value3"),
    };
    Local<ObjectRef> object = ObjectRef::NewWithNamedProperties(vm_, 3, keys, values);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(object);
    EXPECT_TRUE(obj.GetTaggedValue() == JSTaggedValue::Undefined());
    thread_->ClearException();
}

HWTEST_F_L0(JSNApiTests, GetNamedPropertyByPassingUtf8Key)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    const char* utf8Key = "TestKey";
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, utf8Key);
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    Local<JSValueRef> value1 = object->Get(vm_, utf8Key);
    ASSERT_TRUE(value->IsStrictEquals(vm_, value1));
}

HWTEST_F_L0(JSNApiTests, SetNamedPropertyByPassingUtf8Key)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    const char* utf8Key = "TestKey";
    Local<JSValueRef> value = ObjectRef::New(vm_);

    ASSERT_TRUE(object->Set(vm_, utf8Key, value));
    Local<JSValueRef> value1 = object->Get(vm_, utf8Key);
    ASSERT_TRUE(value->IsStrictEquals(vm_, value1));
}

HWTEST_F_L0(JSNApiTests, NapiFastPathGetNamedProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    const char* utf8Key = "TestKey";
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, utf8Key);
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    Local<JSValueRef> value1 = JSNApi::NapiGetNamedProperty(vm_, reinterpret_cast<uintptr_t>(*object), utf8Key);
    ASSERT_TRUE(value->IsStrictEquals(vm_, value1));
}

HWTEST_F_L0(JSNApiTests, NewObjectWithPropertiesDuplicateWithKeyNotFromStringTable)
{
    LocalScope scope(vm_);
    Local<JSValueRef> keys[] = {
        StringRef::NewFromUtf8WithoutStringTable(vm_, DUPLICATE_KEY),
        StringRef::NewFromUtf8WithoutStringTable(vm_, SIMPLE_KEY),
        StringRef::NewFromUtf8WithoutStringTable(vm_, DUPLICATE_KEY),
    };
    Local<JSValueRef> values[] = {
        StringRef::NewFromUtf8WithoutStringTable(vm_, "value1"),
        StringRef::NewFromUtf8WithoutStringTable(vm_, "value2"),
        StringRef::NewFromUtf8WithoutStringTable(vm_, "value3"),
    };
    PropertyAttribute attributes[] = {
        PropertyAttribute(values[0], true, true, true),
        PropertyAttribute(values[1], true, true, true),
        PropertyAttribute(values[2], true, true, true),
    };
    Local<ObjectRef> object = ObjectRef::NewWithProperties(vm_, 3, keys, attributes);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(object);
    EXPECT_TRUE(obj.GetTaggedValue() == JSTaggedValue::Undefined());
    thread_->ClearException();
}

HWTEST_F_L0(JSNApiTests, NewObjectWithPropertiesDuplicateWithKeyNotFromStringTable1)
{
    LocalScope scope(vm_);
    Local<JSValueRef> keys[] = {
        StringRef::NewFromUtf8WithoutStringTable(vm_, DUPLICATE_KEY, 0),
        StringRef::NewFromUtf8WithoutStringTable(vm_, SIMPLE_KEY, 0),
        StringRef::NewFromUtf8WithoutStringTable(vm_, DUPLICATE_KEY, 0),
    };
    Local<JSValueRef> values[] = {
        StringRef::NewFromUtf16WithoutStringTable(vm_, UTF_16, 0),
        StringRef::NewFromUtf16WithoutStringTable(vm_, UTF_16, 0),
        StringRef::NewFromUtf16WithoutStringTable(vm_, UTF_16, 0),
    };
    PropertyAttribute attributes[] = {
        PropertyAttribute(values[0], true, true, true),
        PropertyAttribute(values[1], true, true, true),
        PropertyAttribute(values[2], true, true, true),
    };
    Local<ObjectRef> object = ObjectRef::NewWithProperties(vm_, 3, keys, attributes);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(object);
    EXPECT_TRUE(obj.GetTaggedValue() == JSTaggedValue::Undefined());
    thread_->ClearException();
}

HWTEST_F_L0(JSNApiTests, EcmaObjectToInt)
{
    LocalScope scope(vm_);
    Local<FunctionRef> toPrimitiveFunc = FunctionRef::New(vm_,
        [](JsiRuntimeCallInfo *runtimeInfo) -> Local<JSValueRef> {
            EcmaVM *vm = runtimeInfo->GetVM();
            return JSValueRef::True(vm);
        });
    Local<ObjectRef> obj = ObjectRef::New(vm_);
    PropertyAttribute attribute(toPrimitiveFunc, true, true, true);
    Local<JSValueRef> toPrimitiveKey = JSNApiHelper::ToLocal<JSValueRef>(
        vm_->GetJSThread()->GlobalConstants()->GetHandledToPrimitiveSymbol());
    obj->DefineProperty(vm_, toPrimitiveKey, attribute);
    {
        // Test that Uint32Value and Int32Value should transition to Running if needed.
        ecmascript::ThreadNativeScope nativeScope(thread_);
        uint32_t res = obj->Uint32Value(vm_);
        EXPECT_TRUE(res == 1);
        res = obj->Int32Value(vm_);
        EXPECT_TRUE(res == 1);
    }
}

HWTEST_F_L0(JSNApiTests, NapiTryFastTest)
{
    Local<ObjectRef> object = ObjectRef::New(vm_);
    JSTaggedValue a(0);
    JSHandle<JSTaggedValue> handle(thread_, a);
    Local<JSValueRef> key = JSNApiHelper::ToLocal<JSValueRef>(handle);
    const char* utf8Key = "TestKey";
    Local<JSValueRef> key2 = StringRef::NewFromUtf8(vm_, utf8Key);
    Local<JSValueRef> value = ObjectRef::New(vm_);
    object->Set(vm_, key, value);
    object->Set(vm_, key2, value);
    Local<JSValueRef> res1 = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*object),
                                                       reinterpret_cast<uintptr_t>(*key));
    ASSERT_TRUE(value->IsStrictEquals(vm_, res1));
    Local<JSValueRef> res2 = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*object),
                                                       reinterpret_cast<uintptr_t>(*key2));
    ASSERT_TRUE(value->IsStrictEquals(vm_, res2));
    
    Local<JSValueRef> flag = JSNApi::NapiHasProperty(vm_, reinterpret_cast<uintptr_t>(*object),
                                                     reinterpret_cast<uintptr_t>(*key));
    ASSERT_TRUE(flag->BooleaValue(vm_));
    flag = JSNApi::NapiHasProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key2));
    ASSERT_TRUE(flag->BooleaValue(vm_));

    flag = JSNApi::NapiDeleteProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key));
    ASSERT_TRUE(flag->BooleaValue(vm_));
    flag = JSNApi::NapiDeleteProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key2));
    ASSERT_TRUE(flag->BooleaValue(vm_));

    flag = JSNApi::NapiHasProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key));
    ASSERT_FALSE(flag->BooleaValue(vm_));
    flag = JSNApi::NapiHasProperty(vm_, reinterpret_cast<uintptr_t>(*object), reinterpret_cast<uintptr_t>(*key2));
    ASSERT_FALSE(flag->BooleaValue(vm_));
}

HWTEST_F_L0(JSNApiTests, NapiTryFastHasMethodTest)
{
    LocalScope scope(vm_);
    auto array = JSArray::ArrayCreate(thread_, JSTaggedNumber(0));
    auto arr = JSNApiHelper::ToLocal<ObjectRef>(array);
    const char* utf8Key = "concat";
    Local<JSValueRef> key3 = StringRef::NewFromUtf8(vm_, utf8Key);
    auto flag = JSNApi::NapiHasProperty(vm_, reinterpret_cast<uintptr_t>(*arr), reinterpret_cast<uintptr_t>(*key3));
    ASSERT_TRUE(flag->BooleaValue(vm_));
}

// Verifies callsite info can be created and deleted safely.
HWTEST_F_L0(JSNApiTests, NapiCallsiteInfoCreateDelete)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    JSNApi::NapiDeleteCallsiteInfo(vm_, 0);
}

// Verifies fast property loads work with reusable callsite info and hit tracking.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastWithCallsiteInfo)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> keyA = StringRef::NewFromUtf8(vm_, "fastKeyA");
    Local<JSValueRef> keyB = StringRef::NewFromUtf8(vm_, "fastKeyB");
    Local<JSValueRef> valueA = IntegerRef::New(vm_, 11);
    Local<JSValueRef> valueB = IntegerRef::New(vm_, 22);
    ASSERT_TRUE(object->Set(vm_, keyA, valueA));
    ASSERT_TRUE(object->Set(vm_, keyB, valueB));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAAddr = reinterpret_cast<uintptr_t>(*keyA);
    uintptr_t keyBAddr = reinterpret_cast<uintptr_t>(*keyB);
    bool hit = true;  // intentionally wrong initial value to verify write
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAAddr, info, &hit);
    EXPECT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 11);
    EXPECT_FALSE(hit);  // first access: IC miss (UNINIT → MONO)

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAAddr, info, &hit);
    EXPECT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 11);
    EXPECT_TRUE(hit);  // second access with same key: IC hit

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyBAddr, info);
    EXPECT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 22);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAAddr, info);
    EXPECT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 11);

    // nullptr hit is safe (no-op)
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAAddr, info, nullptr);
    EXPECT_TRUE(res->IsNumber());

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast property stores work with reusable callsite info.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastWithCallsiteInfo)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> keyA = StringRef::NewFromUtf8(vm_, "setKeyA");
    Local<JSValueRef> keyB = StringRef::NewFromUtf8(vm_, "setKeyB");
    ASSERT_TRUE(object->Set(vm_, keyA, IntegerRef::New(vm_, 1)));
    ASSERT_TRUE(object->Set(vm_, keyB, IntegerRef::New(vm_, 2)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAAddr = reinterpret_cast<uintptr_t>(*keyA);
    uintptr_t keyBAddr = reinterpret_cast<uintptr_t>(*keyB);
    Local<JSValueRef> setValueA = IntegerRef::New(vm_, 101);
    Local<JSValueRef> setValueB = IntegerRef::New(vm_, 202);
    Local<JSValueRef> setValueA2 = IntegerRef::New(vm_, 303);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAAddr,
        reinterpret_cast<uintptr_t>(*setValueA), info);
    EXPECT_TRUE(setRes);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyBAddr,
        reinterpret_cast<uintptr_t>(*setValueB), info);
    EXPECT_TRUE(setRes);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAAddr,
        reinterpret_cast<uintptr_t>(*setValueA2), info);
    EXPECT_TRUE(setRes);

    Local<JSValueRef> valueA = object->Get(vm_, keyA);
    Local<JSValueRef> valueB = object->Get(vm_, keyB);
    EXPECT_TRUE(valueA->IsNumber());
    EXPECT_TRUE(valueB->IsNumber());
    EXPECT_EQ(valueA->Int32Value(vm_), 303);
    EXPECT_EQ(valueB->Int32Value(vm_), 202);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies repeated stores hit the Tier-1 store IC.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastTier1StoreHit)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "tier1StoreKey");
    Local<JSValueRef> setValue1 = IntegerRef::New(vm_, 1001);
    Local<JSValueRef> setValue2 = IntegerRef::New(vm_, 1002);
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 0)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);
    bool hit = true;  // intentionally wrong initial value
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*setValue1), info, &hit);
    ASSERT_TRUE(setRes);
    EXPECT_FALSE(hit);  // first store: IC miss (UNINIT → MONO)
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 1001);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*setValue2), info, &hit);
    ASSERT_TRUE(setRes);
    EXPECT_TRUE(hit);  // second store with same key+shape: IC hit
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 1002);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast property loads reject invalid receivers.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastInvalidReceiver)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> invalidObj = IntegerRef::New(vm_, 1);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "invalidReceiverKey");
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*invalidObj),
        reinterpret_cast<uintptr_t>(*key), 0);
    ASSERT_TRUE(res->IsHole());
}

// Verifies fast property stores reject invalid receivers.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastInvalidReceiver)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> invalidObj = IntegerRef::New(vm_, 1);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "invalidReceiverKey");
    Local<JSValueRef> value = IntegerRef::New(vm_, 1);
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*invalidObj),
        reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), 0);
    ASSERT_FALSE(setRes);
}

// Verifies callable receivers participate in fast property loads.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastFunctionReceiverWithCallsiteInfo)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<FunctionRef> function = FunctionRef::New(vm_, FunctionCallback);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "funcFastGetKey");
    Local<JSValueRef> value = IntegerRef::New(vm_, 123);
    ASSERT_TRUE(function->Set(vm_, key, value));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t functionAddr = reinterpret_cast<uintptr_t>(*function);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, functionAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 123);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, functionAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 123);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies callable receivers participate in fast property stores.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastFunctionReceiverWithCallsiteInfo)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<FunctionRef> function = FunctionRef::New(vm_, FunctionCallback);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "funcFastSetKey");
    Local<JSValueRef> value1 = IntegerRef::New(vm_, 456);
    Local<JSValueRef> value2 = IntegerRef::New(vm_, 789);

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t functionAddr = reinterpret_cast<uintptr_t>(*function);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, functionAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*value1), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(function->Get(vm_, key)->Int32Value(vm_), 456);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, functionAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*value2), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(function->Get(vm_, key)->Int32Value(vm_), 789);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast loads fall back correctly for null and fake callsite info.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastInfoVariants)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "infoVariantKey");
    Local<JSValueRef> value = IntegerRef::New(vm_, 66);
    ASSERT_TRUE(object->Set(vm_, key, value));

    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, 0);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 66);

    JSTaggedValue fakeInfo = JSTaggedValue::Undefined();
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, reinterpret_cast<uintptr_t>(&fakeInfo));
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 66);
    ASSERT_FALSE(thread_->HasPendingException());
}

// Verifies fast stores fall back correctly for null and fake callsite info.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastInfoVariants)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "setInfoVariantKey");
    Local<JSValueRef> initValue = IntegerRef::New(vm_, 10);
    Local<JSValueRef> setValue1 = IntegerRef::New(vm_, 111);
    Local<JSValueRef> setValue2 = IntegerRef::New(vm_, 222);
    ASSERT_TRUE(object->Set(vm_, key, initValue));

    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*setValue1), 0);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 111);

    JSTaggedValue fakeInfo = JSTaggedValue::Undefined();
    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*setValue2), reinterpret_cast<uintptr_t>(&fakeInfo));
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 222);
    ASSERT_FALSE(thread_->HasPendingException());
}

// Verifies equivalent string keys keep the load IC in MONO state.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastEquivalentStringKeysKeepMono)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<StringRef> initKey = StringRef::NewFromUtf8WithoutStringTable(vm_, "equivalentGetKey");
    ASSERT_TRUE(object->Set(vm_, initKey, IntegerRef::New(vm_, 77)));
    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<StringRef> key1 = StringRef::NewFromUtf8WithoutStringTable(vm_, "equivalentGetKey");
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(
        vm_, objectAddr, reinterpret_cast<uintptr_t>(*key1), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 77);

    Local<StringRef> key2 = StringRef::NewFromUtf8WithoutStringTable(vm_, "equivalentGetKey");
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*key2), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 77);

    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_FALSE(callsiteInfo->GetIcSlot(thread_, 0).IsHole());
    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies equivalent string keys keep the store IC in MONO state.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastEquivalentStringKeysKeepMono)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<StringRef> initKey = StringRef::NewFromUtf8WithoutStringTable(vm_, "equivalentSetKey");
    ASSERT_TRUE(object->Set(vm_, initKey, IntegerRef::New(vm_, 0)));
    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<StringRef> key1 = StringRef::NewFromUtf8WithoutStringTable(vm_, "equivalentSetKey");
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*key1),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 101)), info);
    ASSERT_TRUE(setRes);

    Local<StringRef> key2 = StringRef::NewFromUtf8WithoutStringTable(vm_, "equivalentSetKey");
    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*key2),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 202)), info);
    ASSERT_TRUE(setRes);

    EXPECT_EQ(object->Get(vm_, initKey)->Int32Value(vm_), 202);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_FALSE(callsiteInfo->GetIcSlot(thread_, 0).IsHole());
    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies numeric keys route through the element load IC.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastNumericKeyGoesViaElementIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> initKey = IntegerRef::New(vm_, 0);
    ASSERT_TRUE(object->Set(vm_, initKey, IntegerRef::New(vm_, 9)));
    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<JSValueRef> key1 = IntegerRef::New(vm_, 0);
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(
        vm_, objectAddr, reinterpret_cast<uintptr_t>(*key1), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 9);

    Local<JSValueRef> key2 = IntegerRef::New(vm_, 0);
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*key2), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 9);

    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsWeak());
    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies numeric keys route through the element store IC.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastNumericKeyGoesViaElementIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> initKey = IntegerRef::New(vm_, 0);
    ASSERT_TRUE(object->Set(vm_, initKey, IntegerRef::New(vm_, 0)));
    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<JSValueRef> key1 = IntegerRef::New(vm_, 0);
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*key1),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 17)), info);
    ASSERT_TRUE(setRes);

    Local<JSValueRef> key2 = IntegerRef::New(vm_, 0);
    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*key2),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 29)), info);
    ASSERT_TRUE(setRes);

    EXPECT_EQ(object->Get(vm_, initKey)->Int32Value(vm_), 29);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsWeak());
    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies symbol keys work with fast loads and callsite info.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastSymbolKeyWithCallsiteInfo)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<StringRef> desc = StringRef::NewFromUtf8(vm_, "symbolHeapInfoDesc");
    Local<JSValueRef> symbolKey = SymbolRef::New(vm_, desc);
    Local<JSValueRef> symbolValue = IntegerRef::New(vm_, 21);
    ASSERT_TRUE(object->Set(vm_, symbolKey, symbolValue));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*symbolKey);
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 21);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 21);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies integer-key loads populate the element IC.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastIntegerKeyHitsElementIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> intKey = IntegerRef::New(vm_, 5);
    ASSERT_TRUE(object->Set(vm_, intKey, IntegerRef::New(vm_, 42)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*intKey), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 42);

    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsWeak());

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies element-index string loads use the element IC.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastElementIndexStringKey)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> intKey = IntegerRef::New(vm_, 0);
    ASSERT_TRUE(object->Set(vm_, intKey, IntegerRef::New(vm_, 77)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<JSValueRef> strKey = StringRef::NewFromUtf8(vm_, "0");
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*strKey), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 77);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*strKey), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 77);

    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsWeak());

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies element-index string stores use the element IC.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastElementIndexStringKey)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> intKey = IntegerRef::New(vm_, 0);
    ASSERT_TRUE(object->Set(vm_, intKey, IntegerRef::New(vm_, 0)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<JSValueRef> strKey = StringRef::NewFromUtf8(vm_, "0");
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*strKey),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 88)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, intKey)->Int32Value(vm_), 88);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*strKey),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 99)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, intKey)->Int32Value(vm_), 99);

    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsWeak());

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies symbol-key loads use the key-based IC path.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastSymbolKeyViaKeyIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<StringRef> desc = StringRef::NewFromUtf8(vm_, "keyICSymbol");
    Local<JSValueRef> symKey = SymbolRef::New(vm_, desc);
    ASSERT_TRUE(object->Set(vm_, symKey, IntegerRef::New(vm_, 123)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*symKey);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 123);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 123);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsSymbol());

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies mixed key types drive the load IC to a broader state without changing results.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastMixedKeyTypes)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> intKey = IntegerRef::New(vm_, 0);
    Local<JSValueRef> strKey = StringRef::NewFromUtf8(vm_, "mixedKey");
    ASSERT_TRUE(object->Set(vm_, intKey, IntegerRef::New(vm_, 10)));
    ASSERT_TRUE(object->Set(vm_, strKey, IntegerRef::New(vm_, 20)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*intKey), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 10);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*strKey), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 20);

    auto state = GetCallsiteState(thread_, info, ICKind::LoadIC);
    ASSERT_TRUE(state == IcAccessor::ICState::MEGA ||
                state == IcAccessor::ICState::IC_MEGA);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*intKey), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 10);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*strKey), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 20);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies equal string content hits the same load IC even with different string objects.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastSameContentDifferentPointer)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<StringRef> initKey = StringRef::NewFromUtf8(vm_, "contentKey");
    ASSERT_TRUE(object->Set(vm_, initKey, IntegerRef::New(vm_, 55)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<StringRef> key1 = StringRef::NewFromUtf8WithoutStringTable(vm_, "contentKey");
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*key1), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 55);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    Local<StringRef> key2 = StringRef::NewFromUtf8WithoutStringTable(vm_, "contentKey");
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, reinterpret_cast<uintptr_t>(*key2), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 55);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsString());

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies stale load IC entries fall back safely on a different object shape.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastStaleICFallsBack)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object1 = ObjectRef::New(vm_);
    Local<ObjectRef> object2 = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "staleLoadKey");
    Local<JSValueRef> extraKey = StringRef::NewFromUtf8(vm_, "extraKey");
    ASSERT_TRUE(object1->Set(vm_, key, IntegerRef::New(vm_, 101)));
    ASSERT_TRUE(object2->Set(vm_, key, IntegerRef::New(vm_, 202)));
    ASSERT_TRUE(object2->Set(vm_, extraKey, IntegerRef::New(vm_, 303)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(
        vm_, reinterpret_cast<uintptr_t>(*object1), keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 101);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object2), keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 202);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies stale store IC entries fall back safely on a different object shape.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastStaleICFallsBack)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object1 = ObjectRef::New(vm_);
    Local<ObjectRef> object2 = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "staleStoreKey");
    Local<JSValueRef> extraKey = StringRef::NewFromUtf8(vm_, "extraKey");
    Local<JSValueRef> setValue1 = IntegerRef::New(vm_, 111);
    Local<JSValueRef> setValue2 = IntegerRef::New(vm_, 222);
    ASSERT_TRUE(object1->Set(vm_, key, IntegerRef::New(vm_, 1)));
    ASSERT_TRUE(object2->Set(vm_, key, IntegerRef::New(vm_, 2)));
    ASSERT_TRUE(object2->Set(vm_, extraKey, IntegerRef::New(vm_, 3)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object1),
        keyAddr, reinterpret_cast<uintptr_t>(*setValue1), info);
    ASSERT_TRUE(setRes);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object2),
        keyAddr, reinterpret_cast<uintptr_t>(*setValue2), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object2->Get(vm_, key)->Int32Value(vm_), 222);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast loads resolve properties from the prototype chain.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastPrototypeChain)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> proto = ObjectRef::New(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "protoKey");
    Local<JSValueRef> value = IntegerRef::New(vm_, 333);
    ASSERT_TRUE(proto->Set(vm_, key, value));
    ASSERT_TRUE(object->SetPrototype(vm_, proto));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 333);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies prototype-chain loads fall back correctly without valid callsite info.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastPrototypeChainBaselineFallback)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> proto = ObjectRef::New(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "protoFallbackKey");
    Local<JSValueRef> value = IntegerRef::New(vm_, 334);
    ASSERT_TRUE(proto->Set(vm_, key, value));
    ASSERT_TRUE(object->SetPrototype(vm_, proto));

    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, 0);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 334);

    JSTaggedValue fakeInfo = JSTaggedValue::Undefined();
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, reinterpret_cast<uintptr_t>(&fakeInfo));
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 334);
    ASSERT_FALSE(thread_->HasPendingException());
}

// Verifies callsite info creation fails cleanly when an exception is already pending.
HWTEST_F_L0(JSNApiTests, NapiCreateCallsiteInfoWithPendingException)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> error = Exception::Error(vm_, StringRef::NewFromUtf8(vm_, ERROR_MESSAGE));
    ASSERT_TRUE(error->IsError(vm_));
    JSNApi::ThrowException(vm_, error);

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    EXPECT_EQ(info, 0U);
    thread_->ClearException();
}

// Verifies fast loads respect an existing pending exception.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastWithPendingException)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "pendingGetKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 88)));

    Local<JSValueRef> error = Exception::Error(vm_, StringRef::NewFromUtf8(vm_, ERROR_MESSAGE));
    ASSERT_TRUE(error->IsError(vm_));
    JSNApi::ThrowException(vm_, error);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), 0);
    EXPECT_TRUE(res->IsUndefined());
    thread_->ClearException();
}

// Verifies fast stores respect an existing pending exception.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastWithPendingException)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "pendingSetKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 1)));
    Local<JSValueRef> value = IntegerRef::New(vm_, 99);

    Local<JSValueRef> error = Exception::Error(vm_, StringRef::NewFromUtf8(vm_, ERROR_MESSAGE));
    ASSERT_TRUE(error->IsError(vm_));
    JSNApi::ThrowException(vm_, error);

    bool res = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), 0);
    EXPECT_FALSE(res);
    thread_->ClearException();
}

// Verifies dictionary-mode loads succeed even when the IC has no cached slot.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastDictionaryCachedKeyWithoutICSlot)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "dictLoadKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 1234)));
    JSHandle<JSObject> jsObj(JSNApiHelper::ToJSHandle(object));
    JSObject::TransitionToDictionary(thread_, jsObj);

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> first = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(first->IsNumber());
    EXPECT_EQ(first->Int32Value(vm_), 1234);

    Local<JSValueRef> second = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(second->IsNumber());
    EXPECT_EQ(second->Int32Value(vm_), 1234);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies dictionary-mode stores succeed even when the IC has no cached slot.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastDictionaryCachedKeyWithoutICSlot)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "dictStoreKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 10)));
    JSHandle<JSObject> jsObj(JSNApiHelper::ToJSHandle(object));
    JSObject::TransitionToDictionary(thread_, jsObj);

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);
    Local<JSValueRef> value1 = IntegerRef::New(vm_, 1010);
    Local<JSValueRef> value2 = IntegerRef::New(vm_, 2020);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*value1), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 1010);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(
        vm_, objectAddr, keyAddr, reinterpret_cast<uintptr_t>(*value2), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 2020);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies typed-array loads fall back for generic non-index keys.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastTypedArrayGenericFallback)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    constexpr uint32_t length = 8;
    Local<ArrayBufferRef> buffer = ArrayBufferRef::New(vm_, length);
    Local<Uint8ArrayRef> typedArray = Uint8ArrayRef::New(vm_, buffer, 0, length);
    Local<JSValueRef> maxIndexKey = StringRef::NewFromUtf8(vm_, "4294967295");
    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*typedArray),
        reinterpret_cast<uintptr_t>(*maxIndexKey), info);
    EXPECT_TRUE(res->IsUndefined());

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast loads surface exceptions thrown by accessors.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastAccessorThrows)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "throwingGetterKey");
    Local<FunctionRef> getter = FunctionRef::New(vm_, ThrowingAccessorCallback);
    Local<FunctionRef> setter = FunctionRef::New(vm_, UndefinedAccessorCallback);
    ASSERT_TRUE(object->SetAccessorProperty(vm_, key, getter, setter));

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), 0);
    EXPECT_TRUE(res->IsUndefined());
    EXPECT_TRUE(thread_->HasPendingException());
    thread_->ClearException();
}

// Verifies prototype accessors that throw are propagated by fast loads.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastPrototypeAccessorThrows)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> proto = ObjectRef::New(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "throwingProtoGetterKey");
    Local<FunctionRef> getter = FunctionRef::New(vm_, ThrowingAccessorCallback);
    Local<FunctionRef> setter = FunctionRef::New(vm_, UndefinedAccessorCallback);
    ASSERT_TRUE(proto->SetAccessorProperty(vm_, key, getter, setter));
    ASSERT_TRUE(object->SetPrototype(vm_, proto));

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), 0);
    EXPECT_TRUE(res->IsUndefined());
    EXPECT_TRUE(thread_->HasPendingException());
    thread_->ClearException();
}

// Verifies fast stores surface exceptions thrown by setters.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastAccessorSetterThrows)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "throwingSetterKey");
    Local<FunctionRef> getter = FunctionRef::New(vm_, UndefinedAccessorCallback);
    Local<FunctionRef> setter = FunctionRef::New(vm_, ThrowingAccessorCallback);
    ASSERT_TRUE(object->SetAccessorProperty(vm_, key, getter, setter));
    Local<JSValueRef> value = IntegerRef::New(vm_, 42);

    bool res = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), 0);
    EXPECT_FALSE(res);
    EXPECT_TRUE(thread_->HasPendingException());
    thread_->ClearException();
}

// Verifies Tier-1 accessor load hits still propagate thrown exceptions.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastTier1AccessorThrows)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "tier1AccessorGetKey");
    Local<FunctionRef> getter = FunctionRef::New(vm_, ConditionalThrowingGetterCallback);
    Local<FunctionRef> setter = FunctionRef::New(vm_, UndefinedAccessorCallback);
    ASSERT_TRUE(object->SetAccessorProperty(vm_, key, getter, setter));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    g_throwInFastLoadIc = false;
    Local<JSValueRef> first = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(first->IsNumber());
    EXPECT_EQ(first->Int32Value(vm_), 123);
    EXPECT_FALSE(thread_->HasPendingException());

    g_throwInFastLoadIc = true;
    Local<JSValueRef> second = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    EXPECT_TRUE(second->IsUndefined());
    EXPECT_TRUE(thread_->HasPendingException());
    thread_->ClearException();
    g_throwInFastLoadIc = false;

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies Tier-1 accessor store hits still propagate thrown exceptions.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastTier1AccessorThrows)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "tier1AccessorSetKey");
    Local<FunctionRef> getter = FunctionRef::New(vm_, UndefinedAccessorCallback);
    Local<FunctionRef> setter = FunctionRef::New(vm_, ConditionalThrowingSetterCallback);
    ASSERT_TRUE(object->SetAccessorProperty(vm_, key, getter, setter));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);
    Local<JSValueRef> value1 = IntegerRef::New(vm_, 1);
    Local<JSValueRef> value2 = IntegerRef::New(vm_, 2);

    g_throwInFastStoreIc = false;
    bool first = JSNApi::NapiSetPropertyWithCallsiteInfo(
        vm_, objectAddr, keyAddr, reinterpret_cast<uintptr_t>(*value1), info);
    ASSERT_TRUE(first);
    EXPECT_FALSE(thread_->HasPendingException());

    g_throwInFastStoreIc = true;
    bool second = JSNApi::NapiSetPropertyWithCallsiteInfo(
        vm_, objectAddr, keyAddr, reinterpret_cast<uintptr_t>(*value2), info);
    EXPECT_FALSE(second);
    EXPECT_TRUE(thread_->HasPendingException());
    thread_->ClearException();
    g_throwInFastStoreIc = false;

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies repeated accessor loads remain stable on the non-throwing path.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastTier2AccessorNoThrow)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "tier2AccessorGetNoThrow");
    Local<FunctionRef> getter = FunctionRef::New(vm_, StableGetterCallback);
    Local<FunctionRef> setter = FunctionRef::New(vm_, UndefinedAccessorCallback);
    ASSERT_TRUE(object->SetAccessorProperty(vm_, key, getter, setter));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    g_nonThrowingGetterValue = 512;
    Local<JSValueRef> first = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(first->IsNumber());
    EXPECT_EQ(first->Int32Value(vm_), 512);
    EXPECT_FALSE(thread_->HasPendingException());

    Local<JSValueRef> second = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(second->IsNumber());
    EXPECT_EQ(second->Int32Value(vm_), 512);
    EXPECT_FALSE(thread_->HasPendingException());

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies repeated accessor stores remain stable on the non-throwing path.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastTier2AccessorNoThrow)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "tier2AccessorSetNoThrow");
    Local<FunctionRef> getter = FunctionRef::New(vm_, UndefinedAccessorCallback);
    Local<FunctionRef> setter = FunctionRef::New(vm_, RecordingSetterCallback);
    ASSERT_TRUE(object->SetAccessorProperty(vm_, key, getter, setter));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);
    Local<JSValueRef> value1 = IntegerRef::New(vm_, 700);
    Local<JSValueRef> value2 = IntegerRef::New(vm_, 701);

    g_lastSetterValue = -1;
    g_setterCallCount = 0;
    bool first = JSNApi::NapiSetPropertyWithCallsiteInfo(
        vm_, objectAddr, keyAddr, reinterpret_cast<uintptr_t>(*value1), info);
    ASSERT_TRUE(first);
    EXPECT_FALSE(thread_->HasPendingException());
    EXPECT_EQ(g_setterCallCount, 1U);
    EXPECT_EQ(g_lastSetterValue, 700);

    bool second = JSNApi::NapiSetPropertyWithCallsiteInfo(
        vm_, objectAddr, keyAddr, reinterpret_cast<uintptr_t>(*value2), info);
    ASSERT_TRUE(second);
    EXPECT_FALSE(thread_->HasPendingException());
    EXPECT_EQ(g_setterCallCount, 2U);
    EXPECT_EQ(g_lastSetterValue, 701);

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies load IC state transitions from UNINIT through MONO, POLY, and MEGA.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastMonoPolyMegaTransition)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "icTransitionGetKey");
    const char *extraKeys[] = {"shapeGetA", "shapeGetB", "shapeGetC", "shapeGetD", "shapeGetE",
                                "shapeGetF", "shapeGetG", "shapeGetH"};

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::UNINIT);

    for (uint32_t i = 0; i < IcAccessor::POLY_CASE_NUM + 4; i++) {
        Local<ObjectRef> object = ObjectRef::New(vm_);
        ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, static_cast<int32_t>(100 + i))));
        Local<JSValueRef> extraKey = StringRef::NewFromUtf8(vm_, extraKeys[i]);
        ASSERT_TRUE(object->Set(vm_, extraKey, IntegerRef::New(vm_, static_cast<int32_t>(i))));

        Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
            reinterpret_cast<uintptr_t>(*key), info);
        ASSERT_TRUE(res->IsNumber());
        EXPECT_EQ(res->Int32Value(vm_), static_cast<int32_t>(100 + i));

        auto state = GetCallsiteState(thread_, info, ICKind::LoadIC);
        VerifyICStateTransition(state, i);
    }

    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsHole());
    auto finalState = GetCallsiteState(thread_, info, ICKind::LoadIC);
    if (finalState == IcAccessor::ICState::IC_MEGA) {
        EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 1).IsString());
    } else {
        EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 1).IsHole());
    }

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies store IC state transitions from UNINIT through MONO, POLY, and MEGA.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastMonoPolyMegaTransition)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "icTransitionSetKey");
    const char *extraKeys[] = {"shapeSetA", "shapeSetB", "shapeSetC", "shapeSetD", "shapeSetE",
                                "shapeSetF", "shapeSetG", "shapeSetH"};

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::UNINIT);

    for (uint32_t i = 0; i < IcAccessor::POLY_CASE_NUM + 4; i++) {
        Local<ObjectRef> object = ObjectRef::New(vm_);
        ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 0)));
        Local<JSValueRef> extraKey = StringRef::NewFromUtf8(vm_, extraKeys[i]);
        ASSERT_TRUE(object->Set(vm_, extraKey, IntegerRef::New(vm_, static_cast<int32_t>(i))));
        Local<JSValueRef> value = IntegerRef::New(vm_, static_cast<int32_t>(1000 + i));

        bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
            reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), info);
        ASSERT_TRUE(setRes);
        EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), static_cast<int32_t>(1000 + i));

        auto state = GetCallsiteState(thread_, info, ICKind::StoreIC);
        VerifyICStateTransition(state, i);
    }

    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *callsiteInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 0).IsHole());
    auto finalState = GetCallsiteState(thread_, info, ICKind::StoreIC);
    if (finalState == IcAccessor::ICState::IC_MEGA) {
        EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 1).IsString());
    } else {
        EXPECT_TRUE(callsiteInfo->GetIcSlot(thread_, 1).IsHole());
    }

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast loads match interpreter results across representative object cases.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastParityWithInterpreterMatrix)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    auto runSlowGet = [this](Local<JSValueRef> obj, Local<JSValueRef> key) {
        thread_->ClearException();
        Local<JSValueRef> result = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*obj),
            reinterpret_cast<uintptr_t>(*key));
        bool hasException = thread_->HasPendingException();
        if (hasException) {
            thread_->ClearException();
        }
        return std::pair<Local<JSValueRef>, bool>(result, hasException);
    };
    auto runFastGet = [this](Local<JSValueRef> obj, Local<JSValueRef> key, uintptr_t info) {
        thread_->ClearException();
        Local<JSValueRef> result = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*obj),
            reinterpret_cast<uintptr_t>(*key), info);
        bool hasException = thread_->HasPendingException();
        if (hasException) {
            thread_->ClearException();
        }
        return std::pair<Local<JSValueRef>, bool>(result, hasException);
    };
    auto expectParity = [this, &runSlowGet, &runFastGet](
                            Local<JSValueRef> slowObj, Local<JSValueRef> fastObj,
                            Local<JSValueRef> key, uintptr_t info) {
        auto [slowResult, slowEx] = runSlowGet(slowObj, key);
        auto [fastResult, fastEx] = runFastGet(fastObj, key, info);
        EXPECT_EQ(slowEx, fastEx);
        if (!slowEx) {
            EXPECT_TRUE(slowResult->IsStrictEquals(vm_, fastResult));
        } else {
            EXPECT_TRUE(slowResult->IsUndefined());
            EXPECT_TRUE(fastResult->IsUndefined());
        }
    };

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "parityOwnKey");
        Local<JSValueRef> value = IntegerRef::New(vm_, 101);
        ASSERT_TRUE(slowObj->Set(vm_, key, value));
        ASSERT_TRUE(fastObj->Set(vm_, key, value));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "parityMissingKey");
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowProto = ObjectRef::New(vm_);
        Local<ObjectRef> fastProto = ObjectRef::New(vm_);
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "parityProtoKey");
        Local<JSValueRef> value = IntegerRef::New(vm_, 202);
        ASSERT_TRUE(slowProto->Set(vm_, key, value));
        ASSERT_TRUE(fastProto->Set(vm_, key, value));
        ASSERT_TRUE(slowObj->SetPrototype(vm_, slowProto));
        ASSERT_TRUE(fastObj->SetPrototype(vm_, fastProto));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> symbolKey = SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "paritySym"));
        Local<JSValueRef> value = IntegerRef::New(vm_, 303);
        ASSERT_TRUE(slowObj->Set(vm_, symbolKey, value));
        ASSERT_TRUE(fastObj->Set(vm_, symbolKey, value));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, symbolKey, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "parityAccessorKey");
        Local<FunctionRef> getter = FunctionRef::New(vm_, StableGetterCallback);
        Local<FunctionRef> setter = FunctionRef::New(vm_, UndefinedAccessorCallback);
        ASSERT_TRUE(slowObj->SetAccessorProperty(vm_, key, getter, setter));
        ASSERT_TRUE(fastObj->SetAccessorProperty(vm_, key, getter, setter));
        g_nonThrowingGetterValue = 404;
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "parityThrowAccessorKey");
        Local<FunctionRef> getter = FunctionRef::New(vm_, ThrowingAccessorCallback);
        Local<FunctionRef> setter = FunctionRef::New(vm_, UndefinedAccessorCallback);
        ASSERT_TRUE(slowObj->SetAccessorProperty(vm_, key, getter, setter));
        ASSERT_TRUE(fastObj->SetAccessorProperty(vm_, key, getter, setter));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "parityDictKey");
        Local<JSValueRef> value = IntegerRef::New(vm_, 505);
        ASSERT_TRUE(slowObj->Set(vm_, key, value));
        ASSERT_TRUE(fastObj->Set(vm_, key, value));
        JSHandle<JSObject> slowJsObj(JSNApiHelper::ToJSHandle(slowObj));
        JSHandle<JSObject> fastJsObj(JSNApiHelper::ToJSHandle(fastObj));
        JSObject::TransitionToDictionary(thread_, slowJsObj);
        JSObject::TransitionToDictionary(thread_, fastJsObj);
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }
}

// Verifies fast stores match interpreter results across representative object cases.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastParityWithInterpreterMatrix)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    auto runSlowSet = [this](Local<ObjectRef> obj, Local<JSValueRef> key, Local<JSValueRef> value) {
        thread_->ClearException();
        bool ok = obj->Set(vm_, key, value);
        bool hasException = thread_->HasPendingException();
        if (hasException) {
            thread_->ClearException();
        }
        return std::pair<bool, bool>(ok, hasException);
    };
    auto runFastSet = [this](Local<ObjectRef> obj, Local<JSValueRef> key, Local<JSValueRef> value, uintptr_t info) {
        thread_->ClearException();
        bool ok = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*obj),
            reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), info);
        bool hasException = thread_->HasPendingException();
        if (hasException) {
            thread_->ClearException();
        }
        return std::pair<bool, bool>(ok, hasException);
    };
    auto expectSetParity = [this, &runSlowSet, &runFastSet](
                               Local<ObjectRef> slowObj, Local<ObjectRef> fastObj,
                               Local<JSValueRef> key, Local<JSValueRef> value, uintptr_t info) {
        auto [slowOk, slowEx] = runSlowSet(slowObj, key, value);
        auto [fastOk, fastEx] = runFastSet(fastObj, key, value, info);
        EXPECT_EQ(slowEx, fastEx);
        if (!slowEx) {
            EXPECT_TRUE(fastOk);
            Local<JSValueRef> slowAfter = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*slowObj),
                reinterpret_cast<uintptr_t>(*key));
            bool slowAfterEx = thread_->HasPendingException();
            if (slowAfterEx) {
                thread_->ClearException();
            }
            Local<JSValueRef> fastAfter = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*fastObj),
                reinterpret_cast<uintptr_t>(*key));
            bool fastAfterEx = thread_->HasPendingException();
            if (fastAfterEx) {
                thread_->ClearException();
            }
            EXPECT_EQ(slowAfterEx, fastAfterEx);
            if (!slowAfterEx) {
                EXPECT_TRUE(slowAfter->IsStrictEquals(vm_, fastAfter));
            }
            if (!slowOk) {
                EXPECT_TRUE(slowAfter->IsStrictEquals(vm_, fastAfter));
            }
        } else {
            EXPECT_FALSE(fastOk);
        }
    };

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "setParityOwnKey");
        ASSERT_TRUE(slowObj->Set(vm_, key, IntegerRef::New(vm_, 1)));
        ASSERT_TRUE(fastObj->Set(vm_, key, IntegerRef::New(vm_, 1)));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 111), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "setParityNewKey");
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 222), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowProto = ObjectRef::New(vm_);
        Local<ObjectRef> fastProto = ObjectRef::New(vm_);
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "setParityProtoKey");
        ASSERT_TRUE(slowProto->Set(vm_, key, IntegerRef::New(vm_, 10)));
        ASSERT_TRUE(fastProto->Set(vm_, key, IntegerRef::New(vm_, 10)));
        ASSERT_TRUE(slowObj->SetPrototype(vm_, slowProto));
        ASSERT_TRUE(fastObj->SetPrototype(vm_, fastProto));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 333), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = SymbolRef::New(vm_, StringRef::NewFromUtf8(vm_, "setParitySym"));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 444), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "setParityAccessorKey");
        Local<FunctionRef> getter = FunctionRef::New(vm_, UndefinedAccessorCallback);
        Local<FunctionRef> setter = FunctionRef::New(vm_, RecordingSetterCallback);
        ASSERT_TRUE(slowObj->SetAccessorProperty(vm_, key, getter, setter));
        ASSERT_TRUE(fastObj->SetAccessorProperty(vm_, key, getter, setter));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        g_lastSetterValue = -1;
        g_setterCallCount = 0;
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 555), info);
        EXPECT_GE(g_setterCallCount, 2U);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "setParityThrowAccessorKey");
        Local<FunctionRef> getter = FunctionRef::New(vm_, UndefinedAccessorCallback);
        Local<FunctionRef> setter = FunctionRef::New(vm_, ThrowingAccessorCallback);
        ASSERT_TRUE(slowObj->SetAccessorProperty(vm_, key, getter, setter));
        ASSERT_TRUE(fastObj->SetAccessorProperty(vm_, key, getter, setter));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 666), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "setParityDictKey");
        ASSERT_TRUE(slowObj->Set(vm_, key, IntegerRef::New(vm_, 1)));
        ASSERT_TRUE(fastObj->Set(vm_, key, IntegerRef::New(vm_, 1)));
        JSHandle<JSObject> slowJsObj(JSNApiHelper::ToJSHandle(slowObj));
        JSHandle<JSObject> fastJsObj(JSNApiHelper::ToJSHandle(fastObj));
        JSObject::TransitionToDictionary(thread_, slowJsObj);
        JSObject::TransitionToDictionary(thread_, fastJsObj);
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 777), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "setParityReadonlyKey");
        Local<JSValueRef> init = IntegerRef::New(vm_, 9);
        PropertyAttribute attr(init, false, true, true); // non-writable data descriptor
        ASSERT_TRUE(slowObj->DefineProperty(vm_, key, attr));
        ASSERT_TRUE(fastObj->DefineProperty(vm_, key, attr));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 888), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }
}

// Verifies load correctness remains stable after the IC reaches MEGA state.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastPostMegaSoakCorrectness)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "postMegaGetKey");
    const char *shapeKeys[] = {
        "pmgShape0", "pmgShape1", "pmgShape2", "pmgShape3", "pmgShape4", "pmgShape5", "pmgShape6"
    };
    constexpr int32_t warmupCount = 5;
    constexpr int32_t soakCount = 200;

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    for (int32_t i = 0; i < warmupCount; i++) {
        Local<ObjectRef> object = ObjectRef::New(vm_);
        ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, i)));
        ASSERT_TRUE(object->Set(vm_, StringRef::NewFromUtf8(vm_, shapeKeys[i]), IntegerRef::New(vm_, i)));
        Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
            reinterpret_cast<uintptr_t>(*key), info);
        ASSERT_TRUE(res->IsNumber());
    }
    auto state = GetCallsiteState(thread_, info, ICKind::LoadIC);
    ASSERT_TRUE(state == IcAccessor::ICState::MEGA || state == IcAccessor::ICState::IC_MEGA);

    for (int32_t i = 0; i < soakCount; i++) {
        Local<ObjectRef> object = ObjectRef::New(vm_);
        Local<JSValueRef> expected = IntegerRef::New(vm_, 10000 + i);
        ASSERT_TRUE(object->Set(vm_, key, expected));
        ASSERT_TRUE(object->Set(vm_, StringRef::NewFromUtf8(vm_, shapeKeys[i % 7]), IntegerRef::New(vm_, i)));

        thread_->ClearException();
        Local<JSValueRef> slow = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*object),
            reinterpret_cast<uintptr_t>(*key));
        bool slowEx = thread_->HasPendingException();
        if (slowEx) {
            thread_->ClearException();
        }
        thread_->ClearException();
        Local<JSValueRef> fast = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
            reinterpret_cast<uintptr_t>(*key), info);
        bool fastEx = thread_->HasPendingException();
        if (fastEx) {
            thread_->ClearException();
        }
        ASSERT_EQ(slowEx, fastEx);
        ASSERT_FALSE(slowEx);
        ASSERT_TRUE(slow->IsStrictEquals(vm_, fast));

        if ((i % 40) == 0) {
            auto runningState = GetCallsiteState(thread_, info, ICKind::LoadIC);
            EXPECT_TRUE(runningState == IcAccessor::ICState::MEGA ||
                        runningState == IcAccessor::ICState::IC_MEGA);
        }
    }

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies store correctness remains stable after the IC reaches MEGA state.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastPostMegaSoakCorrectness)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "postMegaSetKey");
    const char *shapeKeys[] = {
        "pmsShape0", "pmsShape1", "pmsShape2", "pmsShape3", "pmsShape4", "pmsShape5", "pmsShape6"
    };
    constexpr int32_t warmupCount = 5;
    constexpr int32_t soakCount = 200;

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    for (int32_t i = 0; i < warmupCount; i++) {
        Local<ObjectRef> object = ObjectRef::New(vm_);
        ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 0)));
        ASSERT_TRUE(object->Set(vm_, StringRef::NewFromUtf8(vm_, shapeKeys[i]), IntegerRef::New(vm_, i)));
        Local<JSValueRef> value = IntegerRef::New(vm_, i + 1);
        bool res = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
            reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), info);
        ASSERT_TRUE(res);
    }
    auto state = GetCallsiteState(thread_, info, ICKind::StoreIC);
    ASSERT_TRUE(state == IcAccessor::ICState::MEGA || state == IcAccessor::ICState::IC_MEGA);

    for (int32_t i = 0; i < soakCount; i++) {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        ASSERT_TRUE(slowObj->Set(vm_, key, IntegerRef::New(vm_, 0)));
        ASSERT_TRUE(fastObj->Set(vm_, key, IntegerRef::New(vm_, 0)));
        Local<JSValueRef> shapeKey = StringRef::NewFromUtf8(vm_, shapeKeys[i % 7]);
        ASSERT_TRUE(slowObj->Set(vm_, shapeKey, IntegerRef::New(vm_, i)));
        ASSERT_TRUE(fastObj->Set(vm_, shapeKey, IntegerRef::New(vm_, i)));
        Local<JSValueRef> value = IntegerRef::New(vm_, 20000 + i);

        thread_->ClearException();
        bool slowOk = slowObj->Set(vm_, key, value);
        bool slowEx = thread_->HasPendingException();
        if (slowEx) {
            thread_->ClearException();
        }
        thread_->ClearException();
        bool fastOk = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*fastObj),
            reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), info);
        bool fastEx = thread_->HasPendingException();
        if (fastEx) {
            thread_->ClearException();
        }
        ASSERT_EQ(slowEx, fastEx);
        ASSERT_FALSE(slowEx);
        ASSERT_TRUE(slowOk);
        ASSERT_TRUE(fastOk);

        Local<JSValueRef> slowAfter = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*slowObj),
            reinterpret_cast<uintptr_t>(*key));
        bool slowAfterEx = thread_->HasPendingException();
        if (slowAfterEx) {
            thread_->ClearException();
        }
        Local<JSValueRef> fastAfter = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*fastObj),
            reinterpret_cast<uintptr_t>(*key));
        bool fastAfterEx = thread_->HasPendingException();
        if (fastAfterEx) {
            thread_->ClearException();
        }
        ASSERT_EQ(slowAfterEx, fastAfterEx);
        ASSERT_FALSE(slowAfterEx);
        ASSERT_TRUE(slowAfter->IsStrictEquals(vm_, fastAfter));

        if ((i % 40) == 0) {
            auto runningState = GetCallsiteState(thread_, info, ICKind::StoreIC);
            EXPECT_TRUE(runningState == IcAccessor::ICState::MEGA ||
                        runningState == IcAccessor::ICState::IC_MEGA);
        }
    }

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast numeric-key loads match interpreter behavior.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastNumericKeyParity)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    auto runSlowGet = [this](Local<JSValueRef> obj, Local<JSValueRef> key) {
        thread_->ClearException();
        Local<JSValueRef> result = JSNApi::NapiGetProperty(vm_, reinterpret_cast<uintptr_t>(*obj),
            reinterpret_cast<uintptr_t>(*key));
        bool hasException = thread_->HasPendingException();
        if (hasException) {
            thread_->ClearException();
        }
        return std::pair<Local<JSValueRef>, bool>(result, hasException);
    };
    auto runFastGet = [this](Local<JSValueRef> obj, Local<JSValueRef> key, uintptr_t info) {
        thread_->ClearException();
        Local<JSValueRef> result = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*obj),
            reinterpret_cast<uintptr_t>(*key), info);
        bool hasException = thread_->HasPendingException();
        if (hasException) {
            thread_->ClearException();
        }
        return std::pair<Local<JSValueRef>, bool>(result, hasException);
    };
    auto expectParity = [this, &runSlowGet, &runFastGet](
                            Local<JSValueRef> slowObj, Local<JSValueRef> fastObj,
                            Local<JSValueRef> key, uintptr_t info) {
        auto [slowResult, slowEx] = runSlowGet(slowObj, key);
        auto [fastResult, fastEx] = runFastGet(fastObj, key, info);
        EXPECT_EQ(slowEx, fastEx);
        if (!slowEx) {
            EXPECT_TRUE(slowResult->IsStrictEquals(vm_, fastResult));
        }
    };

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = IntegerRef::New(vm_, 0);
        Local<JSValueRef> value = IntegerRef::New(vm_, 42);
        ASSERT_TRUE(slowObj->Set(vm_, key, value));
        ASSERT_TRUE(fastObj->Set(vm_, key, value));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        auto [res, ex] = runFastGet(fastObj, key, info);
        EXPECT_FALSE(ex);
        EXPECT_TRUE(res->IsStrictEquals(vm_, value));
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> numKey = IntegerRef::New(vm_, 0);
        Local<JSValueRef> strKey = StringRef::NewFromUtf8(vm_, "0");
        Local<JSValueRef> value = IntegerRef::New(vm_, 77);
        ASSERT_TRUE(slowObj->Set(vm_, numKey, value));
        ASSERT_TRUE(fastObj->Set(vm_, numKey, value));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, strKey, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = IntegerRef::New(vm_, 1000);
        Local<JSValueRef> value = IntegerRef::New(vm_, 999);
        ASSERT_TRUE(slowObj->Set(vm_, key, value));
        ASSERT_TRUE(fastObj->Set(vm_, key, value));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = IntegerRef::New(vm_, 5);
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectParity(slowObj, fastObj, key, info);
        auto [res, ex] = runFastGet(fastObj, key, info);
        EXPECT_FALSE(ex);
        EXPECT_TRUE(res->IsUndefined());
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = IntegerRef::New(vm_, 3);
        Local<JSValueRef> value = IntegerRef::New(vm_, 33);
        ASSERT_TRUE(slowObj->Set(vm_, key, value));
        ASSERT_TRUE(fastObj->Set(vm_, key, value));
        expectParity(slowObj, fastObj, key, 0);
    }
}

// Verifies fast numeric-key stores match interpreter behavior.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastNumericKeyParity)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    auto runSlowSet = [this](Local<ObjectRef> obj, Local<JSValueRef> key, Local<JSValueRef> value) {
        thread_->ClearException();
        bool ok = obj->Set(vm_, key, value);
        bool hasException = thread_->HasPendingException();
        if (hasException) {
            thread_->ClearException();
        }
        return std::pair<bool, bool>(ok, hasException);
    };
    auto runFastSet = [this](Local<ObjectRef> obj, Local<JSValueRef> key,
                             Local<JSValueRef> value, uintptr_t info) {
        thread_->ClearException();
        bool ok = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*obj),
            reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), info);
        bool hasException = thread_->HasPendingException();
        if (hasException) {
            thread_->ClearException();
        }
        return std::pair<bool, bool>(ok, hasException);
    };
    auto expectSetParity = [this, &runSlowSet, &runFastSet](
                               Local<ObjectRef> slowObj, Local<ObjectRef> fastObj,
                               Local<JSValueRef> key, Local<JSValueRef> value, uintptr_t info) {
        auto [slowOk, slowEx] = runSlowSet(slowObj, key, value);
        auto [fastOk, fastEx] = runFastSet(fastObj, key, value, info);
        EXPECT_EQ(slowEx, fastEx);
        if (!slowEx) {
            EXPECT_TRUE(fastOk);
            Local<JSValueRef> slowAfter = JSNApi::NapiGetProperty(vm_,
                reinterpret_cast<uintptr_t>(*slowObj), reinterpret_cast<uintptr_t>(*key));
            if (thread_->HasPendingException()) {
                thread_->ClearException();
            }
            Local<JSValueRef> fastAfter = JSNApi::NapiGetProperty(vm_,
                reinterpret_cast<uintptr_t>(*fastObj), reinterpret_cast<uintptr_t>(*key));
            if (thread_->HasPendingException()) {
                thread_->ClearException();
            }
            EXPECT_TRUE(slowAfter->IsStrictEquals(vm_, fastAfter));
        }
    };

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = IntegerRef::New(vm_, 0);
        ASSERT_TRUE(slowObj->Set(vm_, key, IntegerRef::New(vm_, 0)));
        ASSERT_TRUE(fastObj->Set(vm_, key, IntegerRef::New(vm_, 0)));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 42), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> numKey = IntegerRef::New(vm_, 0);
        Local<JSValueRef> strKey = StringRef::NewFromUtf8(vm_, "0");
        ASSERT_TRUE(slowObj->Set(vm_, numKey, IntegerRef::New(vm_, 0)));
        ASSERT_TRUE(fastObj->Set(vm_, numKey, IntegerRef::New(vm_, 0)));
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, strKey, IntegerRef::New(vm_, 88), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = IntegerRef::New(vm_, 500);
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 500), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = IntegerRef::New(vm_, 7);
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 77), info);
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        Local<ObjectRef> slowObj = ObjectRef::New(vm_);
        Local<ObjectRef> fastObj = ObjectRef::New(vm_);
        Local<JSValueRef> key = IntegerRef::New(vm_, 2);
        expectSetParity(slowObj, fastObj, key, IntegerRef::New(vm_, 22), 0);
    }
}

// Verifies load IC key mismatches reset and refill the cached key.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastKeyMismatchResetsIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> keyAlpha = StringRef::NewFromUtf8(vm_, "kmAlpha");
    Local<JSValueRef> keyBeta = StringRef::NewFromUtf8(vm_, "kmBeta");
    ASSERT_TRUE(object->Set(vm_, keyAlpha, IntegerRef::New(vm_, 10)));
    ASSERT_TRUE(object->Set(vm_, keyBeta, IntegerRef::New(vm_, 20)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAlphaAddr = reinterpret_cast<uintptr_t>(*keyAlpha);
    uintptr_t keyBetaAddr = reinterpret_cast<uintptr_t>(*keyBeta);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAlphaAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 10);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyBetaAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 20);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyBetaAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 20);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies store IC key mismatches reset and refill the cached key.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastKeyMismatchResetsIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> keyAlpha = StringRef::NewFromUtf8(vm_, "skmAlpha");
    Local<JSValueRef> keyBeta = StringRef::NewFromUtf8(vm_, "skmBeta");
    ASSERT_TRUE(object->Set(vm_, keyAlpha, IntegerRef::New(vm_, 0)));
    ASSERT_TRUE(object->Set(vm_, keyBeta, IntegerRef::New(vm_, 0)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAlphaAddr = reinterpret_cast<uintptr_t>(*keyAlpha);
    uintptr_t keyBetaAddr = reinterpret_cast<uintptr_t>(*keyBeta);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAlphaAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 11)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyBetaAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 22)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, keyBeta)->Int32Value(vm_), 22);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyBetaAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 33)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, keyBeta)->Int32Value(vm_), 33);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast loads ignore heap-object info that is not an ICInfo.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastNonICInfoHeapObjectAsInfo)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "nonICInfoGetKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 55)));

    Local<ObjectRef> fakeInfoObj = ObjectRef::New(vm_);
    uintptr_t fakeInfo = reinterpret_cast<uintptr_t>(*fakeInfoObj);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), fakeInfo);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 55);
    ASSERT_FALSE(thread_->HasPendingException());
}

// Verifies fast stores ignore heap-object info that is not an ICInfo.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastNonICInfoHeapObjectAsInfo)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "nonICInfoSetKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 1)));

    Local<ObjectRef> fakeInfoObj = ObjectRef::New(vm_);
    uintptr_t fakeInfo = reinterpret_cast<uintptr_t>(*fakeInfoObj);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 99)), fakeInfo);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 99);
    ASSERT_FALSE(thread_->HasPendingException());
}

// Verifies stale load IC hclass entries fall back after object shape changes.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastHclassTransitionMidIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "hclassMidKey");
    Local<JSValueRef> extraKey = StringRef::NewFromUtf8(vm_, "hclassMidExtra");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 77)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 77);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    ASSERT_TRUE(object->Set(vm_, extraKey, IntegerRef::New(vm_, 999)));

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 77);
    EXPECT_FALSE(thread_->HasPendingException());

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies stale store IC hclass entries fall back after object shape changes.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastHclassTransitionMidIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "hclassMidSetKey");
    Local<JSValueRef> extraKey = StringRef::NewFromUtf8(vm_, "hclassMidSetExtra");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 1)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 10)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    ASSERT_TRUE(object->Set(vm_, extraKey, IntegerRef::New(vm_, 999)));

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 20)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 20);
    EXPECT_FALSE(thread_->HasPendingException());

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies missing-property loads return undefined without crashing.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastNonExistentProperty)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "noSuchIcKey");

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    EXPECT_TRUE(res->IsUndefined());
    EXPECT_FALSE(thread_->HasPendingException());

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    EXPECT_TRUE(res->IsUndefined());
    EXPECT_FALSE(thread_->HasPendingException());

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast stores can add a missing property.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastNewProperty)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "newIcPropKey");

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 42)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 42);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast loads preserve undefined and null property values.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastUndefinedAndNullValues)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> keyUndef = StringRef::NewFromUtf8(vm_, "valueUndefIcKey");
    Local<JSValueRef> keyNull = StringRef::NewFromUtf8(vm_, "valueNullIcKey");
    ASSERT_TRUE(object->Set(vm_, keyUndef, JSValueRef::Undefined(vm_)));
    ASSERT_TRUE(object->Set(vm_, keyNull, JSValueRef::Null(vm_)));

    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    {
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*keyUndef);

        Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
        EXPECT_TRUE(res->IsUndefined());
        EXPECT_FALSE(thread_->HasPendingException());

        res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
        EXPECT_TRUE(res->IsUndefined());

        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*keyNull);

        Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
        EXPECT_TRUE(res->IsNull());
        EXPECT_FALSE(thread_->HasPendingException());

        res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
        EXPECT_TRUE(res->IsNull());

    ASSERT_FALSE(thread_->HasPendingException());
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }
}

// Verifies fast stores fail cleanly on frozen objects.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastFrozenObject)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "frozenIcKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 100)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 200)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    JSHandle<JSObject> jsObj(JSNApiHelper::ToJSHandle(object));
    ASSERT_TRUE(JSObject::SetIntegrityLevel(thread_, jsObj, FROZEN));

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 300)), info);
    EXPECT_FALSE(setRes);
    if (thread_->HasPendingException()) {
        thread_->ClearException();
    }
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 200);

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies repeated interned-string loads hit the scope-free key IC path.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastScopeFreePath)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "scopeFreeIcKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 999)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<JSValueRef> key1 = StringRef::NewFromUtf8(vm_, "scopeFreeIcKey");
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*key1), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 999);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    Local<JSValueRef> key2 = StringRef::NewFromUtf8(vm_, "scopeFreeIcKey");
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*key2), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 999);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies repeated interned-string stores hit the scope-free key IC path.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastScopeFreePath)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "storeIcHitKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 0)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    Local<JSValueRef> key1 = StringRef::NewFromUtf8(vm_, "storeIcHitKey");
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*key1),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 11)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    Local<JSValueRef> key2 = StringRef::NewFromUtf8(vm_, "storeIcHitKey");
    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr,
        reinterpret_cast<uintptr_t>(*key2),
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 22)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 22);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast loads preserve double and boolean property values.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastDoubleAndBooleanValues)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> keyDouble = StringRef::NewFromUtf8(vm_, "doubleIcKey");
    Local<JSValueRef> keyBool = StringRef::NewFromUtf8(vm_, "boolIcKey");
    constexpr double pi = 3.14159;
    ASSERT_TRUE(object->Set(vm_, keyDouble, NumberRef::New(vm_, pi)));
    ASSERT_TRUE(object->Set(vm_, keyBool, BooleanRef::New(vm_, true)));

    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);

    {
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*keyDouble);

        Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
        ASSERT_TRUE(res->IsNumber());
        EXPECT_DOUBLE_EQ(res->ToNumber(vm_)->Value(), pi);

        res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
        ASSERT_TRUE(res->IsNumber());
        EXPECT_DOUBLE_EQ(res->ToNumber(vm_)->Value(), pi);

        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }

    {
        uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
        ASSERT_NE(info, 0U);
        uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*keyBool);

        Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
        ASSERT_TRUE(res->IsBoolean());
        EXPECT_TRUE(res->IsTrue());

        res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
        ASSERT_TRUE(res->IsBoolean());
        EXPECT_TRUE(res->IsTrue());

    ASSERT_FALSE(thread_->HasPendingException());
        JSNApi::NapiDeleteCallsiteInfo(vm_, info);
    }
}

// Verifies cached load entries recover after the property is deleted.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastDeletedPropertyAfterIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "deleteAfterIcKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 1)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 1);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    ASSERT_TRUE(object->Delete(vm_, key));

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    EXPECT_TRUE(res->IsUndefined());
    EXPECT_FALSE(thread_->HasPendingException());

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies one store callsite can be reused across objects with the same shape.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastMultipleObjectsSameCallsite)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "sharedCallsiteIcKey");

    Local<ObjectRef> obj1 = ObjectRef::New(vm_);
    Local<ObjectRef> obj2 = ObjectRef::New(vm_);
    ASSERT_TRUE(obj1->Set(vm_, key, IntegerRef::New(vm_, 1)));
    ASSERT_TRUE(obj2->Set(vm_, key, IntegerRef::New(vm_, 2)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*obj1),
        keyAddr, reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 10)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(obj1->Get(vm_, key)->Int32Value(vm_), 10);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*obj2),
        keyAddr, reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 20)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(obj2->Get(vm_, key)->Int32Value(vm_), 20);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies callable receivers satisfy the fast load receiver guard.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastCallableNotECMAObject)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<FunctionRef> func = FunctionRef::New(vm_, FunctionCallback);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "callableGuardIcKey");
    ASSERT_TRUE(func->Set(vm_, key, IntegerRef::New(vm_, 42)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t funcAddr = reinterpret_cast<uintptr_t>(*func);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, funcAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 42);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, funcAddr, keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 42);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies load ICs transition to POLY and still hit on a previously seen shape.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastPolyStateHit)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "polyGetHitKey");

    Local<ObjectRef> obj1 = ObjectRef::New(vm_);
    ASSERT_TRUE(obj1->Set(vm_, key, IntegerRef::New(vm_, 10)));

    Local<ObjectRef> obj2 = ObjectRef::New(vm_);
    Local<JSValueRef> extra = StringRef::NewFromUtf8(vm_, "polyGetExtra");
    ASSERT_TRUE(obj2->Set(vm_, extra, IntegerRef::New(vm_, 0)));
    ASSERT_TRUE(obj2->Set(vm_, key, IntegerRef::New(vm_, 20)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_,
        reinterpret_cast<uintptr_t>(*obj1), keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 10);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::MONO);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_,
        reinterpret_cast<uintptr_t>(*obj2), keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 20);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::POLY);

    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_,
        reinterpret_cast<uintptr_t>(*obj1), keyAddr, info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 10);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::LoadIC), IcAccessor::ICState::POLY);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies store ICs transition to POLY and still hit on a previously seen shape.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastPolyStateHit)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "polySetHitKey");

    Local<ObjectRef> obj1 = ObjectRef::New(vm_);
    ASSERT_TRUE(obj1->Set(vm_, key, IntegerRef::New(vm_, 0)));

    Local<ObjectRef> obj2 = ObjectRef::New(vm_);
    Local<JSValueRef> extra = StringRef::NewFromUtf8(vm_, "polySetExtra");
    ASSERT_TRUE(obj2->Set(vm_, extra, IntegerRef::New(vm_, 0)));
    ASSERT_TRUE(obj2->Set(vm_, key, IntegerRef::New(vm_, 0)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_,
        reinterpret_cast<uintptr_t>(*obj1), keyAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 10)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(obj1->Get(vm_, key)->Int32Value(vm_), 10);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::MONO);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_,
        reinterpret_cast<uintptr_t>(*obj2), keyAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 20)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(obj2->Get(vm_, key)->Int32Value(vm_), 20);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::POLY);

    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_,
        reinterpret_cast<uintptr_t>(*obj1), keyAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 30)), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(obj1->Get(vm_, key)->Int32Value(vm_), 30);
    EXPECT_EQ(GetCallsiteState(thread_, info, ICKind::StoreIC), IcAccessor::ICState::POLY);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies fast stores fail cleanly when adding a new property to a sealed object.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastSealedObjectNewKey)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);

    JSHandle<JSObject> jsObj(JSNApiHelper::ToJSHandle(object));
    ASSERT_TRUE(JSObject::SetIntegrityLevel(thread_, jsObj, SEALED));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "sealedNewPropKey");
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*IntegerRef::New(vm_, 42)), info);
    EXPECT_FALSE(setRes);
    if (thread_->HasPendingException()) {
        thread_->ClearException();
    }

    Local<JSValueRef> val = object->Get(vm_, key);
    EXPECT_TRUE(val->IsUndefined());

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies NapiDeleteCallsiteInfo is a safe no-op when info is 0.
HWTEST_F_L0(JSNApiTests, NapiDeleteCallsiteInfoZeroIsNoop)
{
    thread_->ClearException();
    // Must not crash or do anything observable.
    JSNApi::NapiDeleteCallsiteInfo(vm_, 0);
    ASSERT_FALSE(thread_->HasPendingException());
}

// Verifies NapiGetPropertyFast falls back correctly when IC reaches MEGA
// by directly writing Hole into the IC slots before calling the API.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastDirectMegaSlotFallback)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "directMegaGetKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 777)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    // Manually set slot[0] = Hole to simulate MEGA state.
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *icInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    icInfo->SetIcSlot(thread_, 0, JSTaggedValue::Hole());
    icInfo->SetIcSlot(thread_, 1, JSTaggedValue::Hole());

    // With MEGA state, should fall back to NapiGetProperty slow path.
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 777);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies NapiSetPropertyFast falls back correctly when IC is directly in MEGA state.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastDirectMegaSlotFallback)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "directMegaSetKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 1)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    // Manually set slot[0] = Hole to simulate MEGA state.
    JSTaggedValue infoVal = *reinterpret_cast<JSTaggedValue *>(info);
    ICInfo *icInfo = ICInfo::Cast(infoVal.GetTaggedObject());
    icInfo->SetIcSlot(thread_, 0, JSTaggedValue::Hole());
    icInfo->SetIcSlot(thread_, 1, JSTaggedValue::Hole());

    // With MEGA state, should fall back to normal Set() slow path.
    Local<JSValueRef> value = IntegerRef::New(vm_, 888);
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value), info);
    ASSERT_TRUE(setRes);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 888);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies NapiGetPropertyFast with a non-intern string key that requires interning
// in the scoped fast path (non-KeyIC, named IC path).
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastNonInternStringNamedIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "namedICKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 321)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    // First call: UNINIT → MONO (goes through LoadValueMiss, sets up named IC)
    Local<JSValueRef> res1 = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), info);
    ASSERT_TRUE(res1->IsNumber());
    EXPECT_EQ(res1->Int32Value(vm_), 321);

    // Second call: MONO hit — should go through scoped fast path with interning
    // Create a non-intern copy of the same string content
    std::string rawStr = "namedICKey";
    Local<JSValueRef> freshKey = StringRef::NewFromUtf8(vm_, rawStr.c_str());
    Local<JSValueRef> res2 = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*freshKey), info);
    ASSERT_TRUE(res2->IsNumber());
    EXPECT_EQ(res2->Int32Value(vm_), 321);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies NapiSetPropertyFast with a non-intern string key exercises the interning path.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastNonInternStringNamedIC)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "namedICSetKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 0)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    // First call: UNINIT → MONO
    Local<JSValueRef> value1 = IntegerRef::New(vm_, 111);
    bool setRes1 = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*key), reinterpret_cast<uintptr_t>(*value1), info);
    ASSERT_TRUE(setRes1);

    // Second call with fresh (non-intern) key copy
    std::string rawStr = "namedICSetKey";
    Local<JSValueRef> freshKey = StringRef::NewFromUtf8(vm_, rawStr.c_str());
    Local<JSValueRef> value2 = IntegerRef::New(vm_, 222);
    bool setRes2 = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*object),
        reinterpret_cast<uintptr_t>(*freshKey), reinterpret_cast<uintptr_t>(*value2), info);
    ASSERT_TRUE(setRes2);
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 222);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies NapiGetPropertyFast handles Proxy objects (HasOrdinaryGet) via MEGA fallback.
HWTEST_F_L0(JSNApiTests, NapiGetPropertyFastProxyObject)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    ObjectFactory *factory = vm_->GetFactory();

    // Create target + handler, then build a JSProxy via the object factory
    Local<ObjectRef> target = ObjectRef::New(vm_);
    Local<JSValueRef> propKey = StringRef::NewFromUtf8(vm_, "proxyGetKey");
    ASSERT_TRUE(target->Set(vm_, propKey, IntegerRef::New(vm_, 999)));

    Local<ObjectRef> handlerObj = ObjectRef::New(vm_);
    JSHandle<JSTaggedValue> targetHandle = JSNApiHelper::ToJSHandle(target);
    JSHandle<JSTaggedValue> handlerHandle = JSNApiHelper::ToJSHandle(handlerObj);
    JSHandle<JSProxy> proxy = factory->NewJSProxy(targetHandle, handlerHandle);
    Local<JSValueRef> proxyLocal = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(proxy));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    // Proxy has HasOrdinaryGet, so LoadValueMiss will SetAsMega and return the value.
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*proxyLocal),
        reinterpret_cast<uintptr_t>(*propKey), info);
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 999);

    // After proxy access, IC should be MEGA
    auto state = GetCallsiteState(thread_, info, ICKind::LoadIC);
    EXPECT_TRUE(state == IcAccessor::ICState::MEGA || state == IcAccessor::ICState::IC_MEGA);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies NapiSetPropertyFast handles Proxy objects via MEGA fallback.
HWTEST_F_L0(JSNApiTests, NapiSetPropertyFastProxyObject)
{
    LocalScope scope(vm_);
    thread_->ClearException();
    ObjectFactory *factory = vm_->GetFactory();

    Local<ObjectRef> target = ObjectRef::New(vm_);
    Local<JSValueRef> propKey = StringRef::NewFromUtf8(vm_, "proxySetKey");
    ASSERT_TRUE(target->Set(vm_, propKey, IntegerRef::New(vm_, 0)));

    Local<ObjectRef> handlerObj = ObjectRef::New(vm_);
    JSHandle<JSTaggedValue> targetHandle = JSNApiHelper::ToJSHandle(target);
    JSHandle<JSTaggedValue> handlerHandle = JSNApiHelper::ToJSHandle(handlerObj);
    JSHandle<JSProxy> proxy = factory->NewJSProxy(targetHandle, handlerHandle);
    Local<JSValueRef> proxyLocal = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(proxy));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);

    Local<JSValueRef> value = IntegerRef::New(vm_, 555);
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*proxyLocal),
        reinterpret_cast<uintptr_t>(*propKey), reinterpret_cast<uintptr_t>(*value), info);
    ASSERT_TRUE(setRes);

    auto state = GetCallsiteState(thread_, info, ICKind::StoreIC);
    EXPECT_TRUE(state == IcAccessor::ICState::MEGA || state == IcAccessor::ICState::IC_MEGA);

    // Now that IC is MEGA, a second call should take the early MEGA fallback path (line 6889)
    Local<JSValueRef> value2 = IntegerRef::New(vm_, 666);
    bool setRes2 = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, reinterpret_cast<uintptr_t>(*proxyLocal),
        reinterpret_cast<uintptr_t>(*propKey), reinterpret_cast<uintptr_t>(*value2), info);
    ASSERT_TRUE(setRes2);

    ASSERT_FALSE(thread_->HasPendingException());
    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}


// Verifies that using a get-populated callsite info for set falls back to slow path.
HWTEST_F_L0(JSNApiTests, NapiCallsiteInfoCrossUseGetThenSetFallsBack)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "crossUseKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 42)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    // First call: get → populates IC with LOAD direction tag
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_FALSE(thread_->HasPendingException());
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 42);

    // Second call: set with same info → falls back to slow path (no exception)
    Local<JSValueRef> newValue = IntegerRef::New(vm_, 99);
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*newValue), info);
    EXPECT_TRUE(setRes);
    ASSERT_FALSE(thread_->HasPendingException());
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 99);

    // Original direction (get) should still work
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_FALSE(thread_->HasPendingException());
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 99);

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies that using a set-populated callsite info for get falls back to slow path.
HWTEST_F_L0(JSNApiTests, NapiCallsiteInfoCrossUseSetThenGetFallsBack)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "crossUseKey2");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 10)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    // First call: set → populates IC with STORE direction tag
    Local<JSValueRef> newValue = IntegerRef::New(vm_, 20);
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*newValue), info);
    ASSERT_TRUE(setRes);
    ASSERT_FALSE(thread_->HasPendingException());

    // Second call: get with same info → falls back to slow path (no exception)
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info);
    ASSERT_FALSE(thread_->HasPendingException());
    ASSERT_TRUE(res->IsNumber());
    EXPECT_EQ(res->Int32Value(vm_), 20);

    // Original direction (set) should still work
    Local<JSValueRef> newValue2 = IntegerRef::New(vm_, 30);
    setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*newValue2), info);
    ASSERT_TRUE(setRes);
    ASSERT_FALSE(thread_->HasPendingException());
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 30);

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

// Verifies that cross-use falls back to slow path with hit tracking.
HWTEST_F_L0(JSNApiTests, NapiCallsiteInfoCrossUseHitTracking)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "crossUseHitKey");
    ASSERT_TRUE(object->Set(vm_, key, IntegerRef::New(vm_, 77)));

    uintptr_t info = JSNApi::NapiCreateCallsiteInfo(vm_);
    ASSERT_NE(info, 0U);
    uintptr_t objectAddr = reinterpret_cast<uintptr_t>(*object);
    uintptr_t keyAddr = reinterpret_cast<uintptr_t>(*key);

    // First get: miss (populates IC)
    bool hit = true;
    Local<JSValueRef> res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info, &hit);
    ASSERT_FALSE(thread_->HasPendingException());
    EXPECT_FALSE(hit);
    EXPECT_EQ(res->Int32Value(vm_), 77);

    // Second get: hit
    res = JSNApi::NapiGetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr, info, &hit);
    ASSERT_FALSE(thread_->HasPendingException());
    EXPECT_TRUE(hit);

    // Cross-use set: succeeds via slow path, hit stays false
    hit = true;
    Local<JSValueRef> newValue = IntegerRef::New(vm_, 88);
    bool setRes = JSNApi::NapiSetPropertyWithCallsiteInfo(vm_, objectAddr, keyAddr,
        reinterpret_cast<uintptr_t>(*newValue), info, &hit);
    EXPECT_TRUE(setRes);
    EXPECT_FALSE(hit);
    ASSERT_FALSE(thread_->HasPendingException());
    EXPECT_EQ(object->Get(vm_, key)->Int32Value(vm_), 88);

    JSNApi::NapiDeleteCallsiteInfo(vm_, info);
}

HWTEST_F_L0(JSNApiTests, NapiExternalStringCacheTest001)
{
    isStringCacheTableCreated_ = ExternalStringCache::RegisterStringCacheTable(vm_, 0);
    ASSERT_FALSE(isStringCacheTableCreated_);
}

HWTEST_F_L0(JSNApiTests, NapiExternalStringCacheTest002)
{
    constexpr uint32_t STRING_CACHE_TABLE_SIZE = 300;
    isStringCacheTableCreated_ = ExternalStringCache::RegisterStringCacheTable(vm_, STRING_CACHE_TABLE_SIZE);
    ASSERT_FALSE(isStringCacheTableCreated_);
}

HWTEST_F_L0(JSNApiTests, NapiExternalStringCacheTest003)
{
    constexpr uint32_t STRING_CACHE_TABLE_SIZE = 100;
    isStringCacheTableCreated_ = ExternalStringCache::RegisterStringCacheTable(vm_, STRING_CACHE_TABLE_SIZE);
    ASSERT_TRUE(isStringCacheTableCreated_);
}

HWTEST_F_L0(JSNApiTests, NapiExternalStringCacheTest004)
{
    RegisterStringCacheTable();
    constexpr uint32_t PROPERTY_INDEX = 101;
    constexpr char property[] = "hello";
    auto res = ExternalStringCache::SetCachedString(vm_, property, PROPERTY_INDEX);
    ASSERT_FALSE(res);
}

HWTEST_F_L0(JSNApiTests, NapiExternalStringCacheTest005)
{
    RegisterStringCacheTable();
    constexpr uint32_t PROPERTY_INDEX = 0;
    constexpr char property[] = "hello";
    auto res = ExternalStringCache::SetCachedString(vm_, property, PROPERTY_INDEX);
    ASSERT_TRUE(res);
}

HWTEST_F_L0(JSNApiTests, NapiExternalStringCacheTest006)
{
    RegisterStringCacheTable();
    constexpr uint32_t PROPERTY_INDEX = 1;
    constexpr char property[] = "hello";
    auto res = ExternalStringCache::SetCachedString(vm_, property, PROPERTY_INDEX);
    ASSERT_TRUE(res);

    constexpr uint32_t QUERY_PROPERTY_INDEX = 2;
    res = ExternalStringCache::HasCachedString(vm_, QUERY_PROPERTY_INDEX);
    ASSERT_FALSE(res);
}

HWTEST_F_L0(JSNApiTests, NapiExternalStringCacheTest007)
{
    RegisterStringCacheTable();
    constexpr uint32_t PROPERTY_INDEX = 2;
    constexpr char property[] = "hello";
    auto res = ExternalStringCache::SetCachedString(vm_, property, PROPERTY_INDEX);
    ASSERT_TRUE(res);

    constexpr uint32_t QUERY_PROPERTY_INDEX = 2;
    res = ExternalStringCache::HasCachedString(vm_, QUERY_PROPERTY_INDEX);
    ASSERT_TRUE(res);
}

HWTEST_F_L0(JSNApiTests, NapiExternalStringCacheTest008)
{
    RegisterStringCacheTable();
    constexpr uint32_t PROPERTY_INDEX = 3;
    constexpr char property[] = "hello world";
    auto res = ExternalStringCache::SetCachedString(vm_, property, PROPERTY_INDEX);
    ASSERT_TRUE(res);
    Local<StringRef> value = ExternalStringCache::GetCachedString(vm_, PROPERTY_INDEX);
    ASSERT_FALSE(value->IsUndefined());
    EXPECT_EQ(value->ToString(vm_), property);
}

HWTEST_F_L0(JSNApiTests, SetExecuteMode)
{
    ecmascript::ModuleManager *moduleManager = thread_->GetModuleManager();
    ecmascript::ModuleExecuteMode res1 = moduleManager->GetExecuteMode();
    EXPECT_EQ(res1, ecmascript::ModuleExecuteMode::ExecuteZipMode);

    JSNApi::SetExecuteBufferMode(vm_);
    ecmascript::ModuleExecuteMode res2 = moduleManager->GetExecuteMode();
    EXPECT_EQ(res2, ecmascript::ModuleExecuteMode::ExecuteBufferMode);
}

HWTEST_F_L0(JSNApiTests, ToEcmaObject)
{
    LocalScope scope(vm_);
    Local<ObjectRef> res = ObjectRef::New(vm_);
    res->ToEcmaObject(vm_);
    ASSERT_TRUE(res->IsObject(vm_));
}

HWTEST_F_L0(JSNApiTests, GetValueInt64)
{
    LocalScope scope(vm_);
    bool isNumber = true;
    int32_t input = 4;
    Local<IntegerRef> res = IntegerRef::New(vm_, input);
    res->ToBigInt(vm_);
    ASSERT_TRUE(res->GetValueInt64(isNumber));
    res->ToNumber(vm_);
    ASSERT_TRUE(res->GetValueInt64(isNumber));
    isNumber = false;
    ASSERT_TRUE(res->GetValueInt64(isNumber));
}

HWTEST_F_L0(JSNApiTests, GetDataViewInfo)
{
    LocalScope scope(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    bool isDataView = false;
    key->GetDataViewInfo(vm_, isDataView, nullptr, nullptr, nullptr, nullptr);
    ASSERT_FALSE(isDataView);
    isDataView = true;
    key->GetDataViewInfo(vm_, isDataView, nullptr, nullptr, nullptr, nullptr);
    ASSERT_FALSE(isDataView);
}

HWTEST_F_L0(JSNApiTests, ByteLength002)
{
    LocalScope scope(vm_);
    const int32_t length = 4; // define array length
    Local<ArrayBufferRef> array = ArrayBufferRef::New(vm_, length);
    int32_t arrayLen = array->ByteLength(vm_);
    EXPECT_EQ(length, arrayLen);
}

HWTEST_F_L0(JSNApiTests, SetData)
{
    LocalScope scope(vm_);
    Local<FunctionRef> functioncallback = FunctionRef::New(vm_, FunctionCallback);
    struct Data {
        int32_t length;
    };
    const int32_t length = 15;
    Data *data = new Data();
    data->length = length;
    functioncallback->SetData(vm_, data);
}

HWTEST_F_L0(JSNApiTests, GetData)
{
    LocalScope scope(vm_);
    Local<FunctionRef> functioncallback = FunctionRef::New(vm_, FunctionCallback);
    functioncallback->GetData(vm_);
}

HWTEST_F_L0(JSNApiTests, GetData002)
{
    LocalScope scope(vm_);
    int32_t argvLength = 10;
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(vm_->GetJSThread(), JSTaggedValue::Undefined(), argvLength);
    JsiRuntimeCallInfo *jsiRuntimeCallInfo = reinterpret_cast<JsiRuntimeCallInfo *>(ecmaRuntimeCallInfo);
    jsiRuntimeCallInfo->GetData();
}

HWTEST_F_L0(JSNApiTests, SetStopPreLoadSoCallback)
{
    auto callback = []()->void {
        LOG_FULL(INFO) << "Call stopPreLoadSoCallback";
    };
    JSNApi::SetStopPreLoadSoCallback(vm_, callback);
    auto stopPreLoadCallbacks = vm_->GetStopPreLoadCallbacks();
    EXPECT_EQ(stopPreLoadCallbacks.size(), 1);
    vm_->StopPreLoadSoOrAbc();

    stopPreLoadCallbacks = vm_->GetStopPreLoadCallbacks();
    EXPECT_EQ(stopPreLoadCallbacks.size(), 0);
}

HWTEST_F_L0(JSNApiTests, UpdatePkgContextInfoList)
{
    std::map<std::string, std::vector<std::vector<std::string>>> pkgList;
    std::vector<std::string> entryList = {
        "entry",
        "packageName", "entry",
        "bundleName", "",
        "moduleName", "",
        "version", "",
        "entryPath", "src/main/",
        "isSO", "false"
    };
    pkgList["entry"] = {entryList};
    JSNApi::SetpkgContextInfoList(vm_, pkgList);

    std::map<std::string, std::vector<std::vector<std::string>>> newPkgList;
    std::vector<std::string> hspList = {
        "hsp",
        "packageName", "hsp",
        "bundleName", "",
        "moduleName", "",
        "version", "1.1.0",
        "entryPath", "Index.ets",
        "isSO", "false"
    };
    newPkgList["hsp"] = {hspList};
    JSNApi::UpdatePkgContextInfoList(vm_, newPkgList);

    CMap<CString, CMap<CString, CVector<CString>>> vmPkgList = vm_->GetPkgContextInfoList();
    EXPECT_EQ(vmPkgList.size(), 2);
    EXPECT_EQ(vmPkgList["entry"]["entry"].size(), 12);
    EXPECT_EQ(vmPkgList["hsp"].size(), 1);
    EXPECT_EQ(vmPkgList["hsp"]["hsp"].size(), 12);

    CVector<CString> hapInfo{};
    vm_->GetPkgContextInfoListElements("entry", "entry", hapInfo);
    EXPECT_EQ(hapInfo.size(), 12);
    EXPECT_EQ(hapInfo[1], "entry");
    EXPECT_EQ(hapInfo[3], "");
    EXPECT_EQ(hapInfo[5], "");
    EXPECT_EQ(hapInfo[7], "");
    EXPECT_EQ(hapInfo[9], "src/main/");
    EXPECT_EQ(hapInfo[11], "false");

    CVector<CString> hspInfo{};
    vm_->GetPkgContextInfoListElements("hsp", "hsp", hspInfo);
    EXPECT_EQ(hspInfo.size(), 12);
    EXPECT_EQ(hspInfo[1], "hsp");
    EXPECT_EQ(hspInfo[3], "");
    EXPECT_EQ(hspInfo[5], "");
    EXPECT_EQ(hspInfo[7], "1.1.0");
    EXPECT_EQ(hspInfo[9], "Index.ets");
    EXPECT_EQ(hspInfo[11], "false");
}

HWTEST_F_L0(JSNApiTests, UpdatePkgNameList)
{
    std::map<std::string, std::string> pkgNameList;
    pkgNameList["moduleName1"] = "pkgName1";
    JSNApi::SetPkgNameList(vm_, pkgNameList);

    std::map<std::string, std::string> newPkgNameList;
    newPkgNameList["moduleName2"] = "pkgName2";
    JSNApi::UpdatePkgNameList(vm_, newPkgNameList);

    CMap<CString, CString> vmPkgNameList = vm_->GetPkgNameList();
    EXPECT_EQ(vmPkgNameList.size(), 2);
    EXPECT_EQ(vmPkgNameList["moduleName1"], "pkgName1");
    EXPECT_EQ(vmPkgNameList["moduleName2"], "pkgName2");
}

HWTEST_F_L0(JSNApiTests, UpdatePkgAliasList)
{
    std::map<std::string, std::string> aliasNameList;
    aliasNameList["aliasName1"] = "pkgName1";
    JSNApi::SetPkgAliasList(vm_, aliasNameList);

    std::map<std::string, std::string> newAliasNameList;
    newAliasNameList["aliasName2"] = "pkgName2";
    JSNApi::UpdatePkgAliasList(vm_, newAliasNameList);

    CMap<CString, CString> vmAliasNameList = vm_->GetPkgAliasList();
    EXPECT_EQ(vmAliasNameList.size(), 2);
    EXPECT_EQ(vmAliasNameList["aliasName1"], "pkgName1");
    EXPECT_EQ(vmAliasNameList["aliasName2"], "pkgName2");
}

HWTEST_F_L0(JSNApiTests, SetPkgContextInfoListWithBuffer)
{
    std::unordered_map<std::string, std::pair<std::unique_ptr<uint8_t[]>, size_t>> modulePkgContentMap;
    std::string entryString = R"({"entry":{"packageName":"entry", "bundleName":"", "moduleName":
        "", "version":"", "entryPath":"src/main/", "isSO":false, "dependencyAlias":""}})";
    std::string libraryString = R"({"library":{"packageName":"library", "bundleName":"com.xxx.xxxx", "moduleName":
        "library", "version":"1.0.0", "entryPath":"Index.ets", "isSO":false, "dependencyAlias":"har"}})";
    auto createBufferFromString = [](const std::string& str) {
        size_t size = str.size();
        auto buffer = std::make_unique<uint8_t[]>(size);
        std::copy(str.begin(), str.end(), buffer.get());
        return std::make_pair(std::move(buffer), size);
    };
    modulePkgContentMap["entry"] = createBufferFromString(entryString);
    modulePkgContentMap["library"] = createBufferFromString(libraryString);
    JSNApi::SetPkgContextInfoList(vm_, modulePkgContentMap);
    CMap<CString, CMap<CString, CVector<CString>>> vmPkgList = vm_->GetPkgContextInfoList();
    EXPECT_EQ(vmPkgList.size(), 2);
    EXPECT_EQ(vmPkgList["entry"]["entry"].size(), 12);
    EXPECT_EQ(vmPkgList["library"].size(), 1);
    EXPECT_EQ(vmPkgList["library"]["library"].size(), 12);

    CVector<CString> hapInfo{};
    vm_->GetPkgContextInfoListElements("entry", "entry", hapInfo);
    EXPECT_EQ(hapInfo.size(), 12);
    EXPECT_EQ(hapInfo[1], "entry");
    EXPECT_EQ(hapInfo[3], "");
    EXPECT_EQ(hapInfo[5], "");
    EXPECT_EQ(hapInfo[7], "");
    EXPECT_EQ(hapInfo[9], "src/main/");
    EXPECT_EQ(hapInfo[11], "false");

    CVector<CString> libraryInfo{};
    vm_->GetPkgContextInfoListElements("library", "library", libraryInfo);
    EXPECT_EQ(libraryInfo.size(), 12);
    EXPECT_EQ(libraryInfo[1], "library");
    EXPECT_EQ(libraryInfo[3], "com.xxx.xxxx");
    EXPECT_EQ(libraryInfo[5], "library");
    EXPECT_EQ(libraryInfo[7], "1.0.0");
    EXPECT_EQ(libraryInfo[9], "Index.ets");
    EXPECT_EQ(libraryInfo[11], "false");
}

HWTEST_F_L0(JSNApiTests, UpdatePkgContextInfoListWithBuffer)
{
    std::string hspString = R"({"hsp":{"packageName":"hsp", "bundleName":"", "moduleName":
        "hsp", "version":"1.1.0", "entryPath":"Index.ets", "isSO":false, "dependencyAlias": "har"}})";
    auto createBufferFromString = [](const std::string& str) {
        size_t size = str.size();
        auto buffer = std::make_unique<uint8_t[]>(size);
        std::copy(str.begin(), str.end(), buffer.get());
        return std::make_pair(std::move(buffer), size);
    };
    std::unordered_map<std::string, std::pair<std::unique_ptr<uint8_t[]>, size_t>> updatePkgContentMap;
    updatePkgContentMap["hsp"] = createBufferFromString(hspString);
    JSNApi::UpdatePkgContextInfoList(vm_, updatePkgContentMap);
    CVector<CString> hspInfo{};
    vm_->GetPkgContextInfoListElements("hsp", "hsp", hspInfo);
    EXPECT_EQ(hspInfo.size(), 12);
    EXPECT_EQ(hspInfo[1], "hsp");
    EXPECT_EQ(hspInfo[3], "");
    EXPECT_EQ(hspInfo[5], "hsp");
    EXPECT_EQ(hspInfo[7], "1.1.0");
    EXPECT_EQ(hspInfo[9], "Index.ets");
    EXPECT_EQ(hspInfo[11], "false");
}

HWTEST_F_L0(JSNApiTests, TryGetArrayLengthTest001)
{
    LocalScope scope(vm_);
    const uint32_t ARRAY_LENGTH = 10; // 10 means array length
    Local<ArrayRef> array = ArrayRef::New(vm_, ARRAY_LENGTH);
    Local<JSValueRef> value(array);
    bool isJSArray = value->IsJSArray(vm_);
    ASSERT_EQ(isJSArray, true);

    bool isPendingException = true;
    bool isArrayOrSharedArray = false;
    uint32_t arrayLength = 0;
    value->TryGetArrayLength(vm_, &isPendingException, &isArrayOrSharedArray, &arrayLength);
    ASSERT_EQ(isPendingException, false);
    ASSERT_EQ(isArrayOrSharedArray, true);
    ASSERT_EQ(arrayLength, ARRAY_LENGTH);
}

HWTEST_F_L0(JSNApiTests, TryGetArrayLengthTest002)
{
    LocalScope scope(vm_);
    const uint32_t ARRAY_LENGTH = 10; // 10 means array length
    Local<SendableArrayRef> sArray = SendableArrayRef::New(vm_, ARRAY_LENGTH);
    Local<JSValueRef> value(sArray);
    bool isSArray = value->IsSharedArray(vm_);
    ASSERT_EQ(isSArray, true);

    bool isPendingException = true;
    bool isArrayOrSharedArray = false;
    uint32_t arrayLength = 0;
    value->TryGetArrayLength(vm_, &isPendingException, &isArrayOrSharedArray, &arrayLength);
    ASSERT_EQ(isPendingException, false);
    ASSERT_EQ(isArrayOrSharedArray, true);
    ASSERT_EQ(arrayLength, ARRAY_LENGTH);
}

HWTEST_F_L0(JSNApiTests, TryGetArrayLengthTest003)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> value(object);
    bool isObject = value->IsObject(vm_);
    ASSERT_EQ(isObject, true);

    bool isPendingException = true;
    bool isArrayOrSharedArray = false;
    const uint32_t INIT_VALUE = 10; // 10 means a randon initial value
    uint32_t arrayLength = INIT_VALUE;
    value->TryGetArrayLength(vm_, &isPendingException, &isArrayOrSharedArray, &arrayLength);
    ASSERT_EQ(isPendingException, false);
    ASSERT_EQ(isArrayOrSharedArray, false);
    ASSERT_EQ(arrayLength, INIT_VALUE);
}

HWTEST_F_L0(JSNApiTests, TryGetArrayLengthTest004)
{
    LocalScope scope(vm_);
    // create array
    const uint32_t ARRAY_LENGTH = 10; // 10 means array length
    Local<ArrayRef> array = ArrayRef::New(vm_, ARRAY_LENGTH);
    Local<JSValueRef> value(array);
    bool isJSArray = value->IsJSArray(vm_);
    ASSERT_EQ(isJSArray, true);

    // throw error in thread
    Local<JSValueRef> error = Exception::Error(vm_, StringRef::NewFromUtf8(vm_, ERROR_MESSAGE));
    ASSERT_EQ(error->IsError(vm_), true);
    JSNApi::ThrowException(vm_, error);
    JSThread *thread = vm_->GetJSThread();
    ASSERT_EQ(thread->HasPendingException(), true);

    // test TryGetArrayLength
    bool isPendingException = false;
    bool isArrayOrSharedArray = false;
    uint32_t arrayLength = ARRAY_LENGTH;
    value->TryGetArrayLength(vm_, &isPendingException, &isArrayOrSharedArray, &arrayLength);
    ASSERT_EQ(isPendingException, true);
    ASSERT_EQ(isArrayOrSharedArray, true);
    ASSERT_EQ(arrayLength, 0);

    // clear exception
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ(thread->HasPendingException(), false);
}

HWTEST_F_L0(JSNApiTests, TryGetArrayLengthTest005)
{
    LocalScope scope(vm_);
    // create array
    const uint32_t ARRAY_LENGTH = 10; // 10 means array length
    Local<SendableArrayRef> sArray = SendableArrayRef::New(vm_, ARRAY_LENGTH);
    Local<JSValueRef> value(sArray);
    bool isSArray = value->IsSharedArray(vm_);
    ASSERT_EQ(isSArray, true);

    // throw error in thread
    Local<JSValueRef> error = Exception::Error(vm_, StringRef::NewFromUtf8(vm_, ERROR_MESSAGE));
    ASSERT_EQ(error->IsError(vm_), true);
    JSNApi::ThrowException(vm_, error);
    JSThread *thread = vm_->GetJSThread();
    ASSERT_EQ(thread->HasPendingException(), true);

    // test TryGetArrayLength
    bool isPendingException = false;
    bool isArrayOrSharedArray = false;
    uint32_t arrayLength = ARRAY_LENGTH;
    value->TryGetArrayLength(vm_, &isPendingException, &isArrayOrSharedArray, &arrayLength);
    ASSERT_EQ(isPendingException, true);
    ASSERT_EQ(isArrayOrSharedArray, true);
    ASSERT_EQ(arrayLength, 0);

    // clear exception
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ(thread->HasPendingException(), false);
}

HWTEST_F_L0(JSNApiTests, TryGetArrayLengthTest006)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> value(object);
    bool isObject = value->IsObject(vm_);
    ASSERT_EQ(isObject, true);

    // throw error in thread
    Local<JSValueRef> error = Exception::Error(vm_, StringRef::NewFromUtf8(vm_, ERROR_MESSAGE));
    ASSERT_EQ(error->IsError(vm_), true);
    JSNApi::ThrowException(vm_, error);
    JSThread *thread = vm_->GetJSThread();
    ASSERT_EQ(thread->HasPendingException(), true);

    // test TryGetArrayLength
    bool isPendingException = false;
    bool isArrayOrSharedArray = true;
    const uint32_t INIT_VALUE = 10; // 10 means a randon initial value
    uint32_t arrayLength = INIT_VALUE;
    value->TryGetArrayLength(vm_, &isPendingException, &isArrayOrSharedArray, &arrayLength);
    ASSERT_EQ(isPendingException, true);
    ASSERT_EQ(isArrayOrSharedArray, false);
    ASSERT_EQ(arrayLength, INIT_VALUE);

    // clear exception
    JSNApi::GetAndClearUncaughtException(vm_);
    ASSERT_EQ(thread->HasPendingException(), false);
}

/**
 * @tc.number: ffi_interface_api_147
 * @tc.name: UpdateStackInfo
 * @tc.desc: Used to verify whether the function of update stack info was successful.
 * @tc.type: FUNC
 * @tc.require: parameter
 */
HWTEST_F_L0(JSNApiTests, UpdateStackInfo)
{
    LocalScope scope(vm_);
    StackInfo stackInfo = { 0x10000, 0 };
    uint64_t currentStackLimit = vm_->GetJSThread()->GetStackLimit();
    JSNApi::UpdateStackInfo(vm_, &stackInfo, 0);
    ASSERT_EQ(vm_->GetJSThread()->GetStackLimit(), 0x10000);
    JSNApi::UpdateStackInfo(vm_, &stackInfo, 1);
    ASSERT_EQ(vm_->GetJSThread()->GetStackLimit(), currentStackLimit);
    JSNApi::UpdateStackInfo(vm_, nullptr, 0);
    ASSERT_EQ(vm_->GetJSThread()->GetStackLimit(), currentStackLimit);
}

HWTEST_F_L0(JSNApiTests, JSNApi_CreateContext001)
{
    LocalScope scope(vm_);
    Local<JSValueRef> contextValue = JSNApi::CreateContext(vm_);
    EXPECT_TRUE(contextValue->IsHeapObject());
    EXPECT_TRUE(contextValue->IsJsGlobalEnv(vm_));
}

HWTEST_F_L0(JSNApiTests, JSNApi_GetCurrentContext001)
{
    LocalScope scope(vm_);
    Local<JSValueRef> contextValue = JSNApi::GetCurrentContext(vm_);
    EXPECT_TRUE(contextValue->IsHeapObject());
    EXPECT_TRUE(contextValue->IsJsGlobalEnv(vm_));
}

HWTEST_F_L0(JSNApiTests, JSNApi_Local_Operator_equal001)
{
    LocalScope scope(vm_);
    Local<JSValueRef> contextValue = JSNApi::CreateContext(vm_);
    Local<JSValueRef> newContext = Local<JSValueRef>(contextValue);
    EXPECT_TRUE(contextValue == newContext);
}

HWTEST_F_L0(JSNApiTests, JSNApi_SwitchContext001)
{
    LocalScope scope(vm_);
    Local<JSValueRef> contextValue = JSNApi::CreateContext(vm_);
    EXPECT_TRUE(contextValue->IsHeapObject());
    EXPECT_TRUE(contextValue->IsJsGlobalEnv(vm_));
    Local<JSValueRef> currentContext = JSNApi::GetCurrentContext(vm_);
    EXPECT_FALSE(currentContext == contextValue);

    JSNApi::SwitchContext(vm_, contextValue);
    Local<JSValueRef> switchedContext = JSNApi::GetCurrentContext(vm_);
    EXPECT_TRUE(switchedContext == contextValue);
}

HWTEST_F_L0(JSNApiTests, XRefGlobalHandleAddr)
{
    JSNApi::InitHybridVMEnv(vm_);
    JSHandle<TaggedArray> weakRefArray = vm_->GetFactory()->NewTaggedArray(2, JSTaggedValue::Hole());
    uintptr_t xRefArrayAddress;
    vm_->SetEnableForceGC(false);
    {
        [[maybe_unused]] EcmaHandleScope scope(thread_);
        JSHandle<JSTaggedValue> xRefArray = JSArray::ArrayCreate(thread_, JSTaggedNumber(1));
        JSHandle<JSTaggedValue> normalArray = JSArray::ArrayCreate(thread_, JSTaggedNumber(2));
        xRefArrayAddress = JSNApiGetXRefGlobalHandleAddr(vm_, xRefArray.GetAddress());
        weakRefArray->Set(thread_, 0, xRefArray.GetTaggedValue().CreateAndGetWeakRef());
        weakRefArray->Set(thread_, 1, normalArray.GetTaggedValue().CreateAndGetWeakRef());
    }
    vm_->CollectGarbage(TriggerGCType::FULL_GC);
    EXPECT_TRUE(!weakRefArray->Get(thread_, 0).IsUndefined());
    EXPECT_TRUE(weakRefArray->Get(thread_, 1).IsUndefined());

    JSNApiDisposeXRefGlobalHandleAddr(vm_, xRefArrayAddress);
    vm_->CollectGarbage(TriggerGCType::FULL_GC);
    vm_->SetEnableForceGC(true);
    EXPECT_TRUE(weakRefArray->Get(thread_, 0).IsUndefined());
}

HWTEST_F_L0(JSNApiTests, InitHybridVMEnv)
{
    LocalScope scope(vm_);
    JSNApi::InitHybridVMEnv(vm_);

    auto instance = ecmascript::Runtime::GetInstance();
    ASSERT(instance != nullptr);

    EXPECT_TRUE(instance->IsHybridVm());
}

HWTEST_F_L0(JSNApiTests, NewFromUtf8Replacement)
{
    uint8_t u8Data[1024] = {0xcc, 0x5c, 0x0};
    uint8_t u8Out[1024] = {0};
    size_t u8OutLen = 0;
    
    Local<StringRef> resStr = StringRef::NewFromUtf8Replacement(thread_->GetEcmaVM(),
                                                                reinterpret_cast<char*>(u8Data), 2);
    u8OutLen = resStr-> Utf8Length(thread_->GetEcmaVM());
    resStr -> WriteUtf8(thread_->GetEcmaVM(), reinterpret_cast<char*>(u8Out), u8OutLen);
    ASSERT_TRUE(u8Out[0] == 0xef);
    ASSERT_TRUE(u8Out[1] == 0xbf);
    ASSERT_TRUE(u8Out[2] == 0xbd);
    ASSERT_TRUE(u8Out[3] == 0x5c);
    ASSERT_TRUE(u8Out[4] == 0x0);
    {
        // test for utf8 decode
        std::string input[4];
        input[0] = "ahskdjashdjkasdhashiwyqoieysodahlkdhjaldqdwqwertyuiopp;kjsxcvbnm,kqqaxvbnkhd";
        input[1] = "埃里克多少啊收到了賀卡收到和拉丁八點幺五i一起玩按時打算到i的後期維護公司付款就大概的";
        input[2] = "🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗";
        input[3] = input[0] + input[1] + input[2];
        for (int i = 0; i < 3; i++) {
            input[0] = input[0] + input[0];
            input[1] = input[1] + input[1];
            input[2] = input[2] + input[2];
            input[3] = input[3] + input[3];
        }
        input[0] += "\0";
        input[1] += "\0";
        input[2] += "\0";
        input[3] += "\0";
        for (int i = 0; i < 4; i++) {
            Local<StringRef> resStr = StringRef::NewFromUtf8Replacement(
                thread_->GetEcmaVM(), input[i].c_str(), input[i].length());
            u8OutLen = resStr-> Utf8Length(thread_->GetEcmaVM());
            uint8_t *u8Out = static_cast<uint8_t *>(malloc(u8OutLen + 1));
            resStr -> WriteUtf8(thread_->GetEcmaVM(), reinterpret_cast<char*>(u8Out), u8OutLen);
            for (int j = 0; j <= input[i].size(); j++) {
                ASSERT_TRUE(u8Out[j] == (uint8_t)input[i][j]);
            }
            free(u8Out);
            Local<StringRef> resStr1 = StringRef::NewFromUtf8(
                thread_->GetEcmaVM(), input[i].c_str(), input[i].length());
            uint32_t u8OutLen2 = resStr1-> Utf8Length(thread_->GetEcmaVM());
            ASSERT_TRUE(u8OutLen2 == u8OutLen);
        }
    }
}
HWTEST_F_L0(JSNApiTests, NewFromUtf8WithoutStringTableReplacement)
{
    uint8_t u8Data[1024] = {0xcc, 0x5c, 0x0};
    uint8_t u8Out[1024] = {0};
    size_t u8OutLen = 0;
    
    Local<StringRef> resStr = StringRef::NewFromUtf8Replacement(thread_->GetEcmaVM(),
                                                                reinterpret_cast<char*>(u8Data), 2);
    u8OutLen = resStr-> Utf8Length(thread_->GetEcmaVM());
    resStr -> WriteUtf8(thread_->GetEcmaVM(), reinterpret_cast<char*>(u8Out), u8OutLen);
    ASSERT_TRUE(u8Out[0] == 0xef);
    ASSERT_TRUE(u8Out[1] == 0xbf);
    ASSERT_TRUE(u8Out[2] == 0xbd);
    ASSERT_TRUE(u8Out[3] == 0x5c);
    ASSERT_TRUE(u8Out[4] == 0x0);
    {
        // test for utf8 decode
        std::string input[4];
        input[0] = "ahskdjashdjkasdhashiwyqoieysodahlkdhjaldqdwqwertyuiopp;kjsxcvbnm,kqqaxvbnkhd";
        input[1] = "埃里克多少啊收到了賀卡收到和拉丁八點幺五i一起玩按時打算到i的後期維護公司付款就大概的";
        input[2] = "🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗🤗";
        input[3] = input[0] + input[1] + input[2];
        for (int i = 0; i < 3; i++) {
            input[0] = input[0] + input[0];
            input[1] = input[1] + input[1];
            input[2] = input[2] + input[2];
            input[3] = input[3] + input[3];
        }
        input[0] += "\0";
        input[1] += "\0";
        input[2] += "\0";
        input[3] += "\0";
        for (int i = 0; i < 4; i++) {
            Local<StringRef> resStr = StringRef::NewFromUtf8Replacement(
                thread_->GetEcmaVM(), input[i].c_str(), input[i].length());
            u8OutLen = resStr-> Utf8Length(thread_->GetEcmaVM());
            uint8_t *u8Out = static_cast<uint8_t *>(malloc(u8OutLen + 1));
            resStr -> WriteUtf8(thread_->GetEcmaVM(), reinterpret_cast<char*>(u8Out), u8OutLen);
            for (int j = 0; j <= input[i].size(); j++) {
                ASSERT_TRUE(u8Out[j] == (uint8_t)input[i][j]);
            }
            free(u8Out);
            Local<StringRef> resStr1 = StringRef::NewFromUtf8(
                thread_->GetEcmaVM(), input[i].c_str(), input[i].length());
            uint32_t u8OutLen2 = resStr1-> Utf8Length(thread_->GetEcmaVM());
            ASSERT_TRUE(u8OutLen2 == u8OutLen);
        }
    }
}

HWTEST_F_L0(JSNApiTests, FindModuleInAbcFile001)
{
    LocalScope scope(vm_);
    const std::string filename = "data/storage/el1/bundle/modulename/ets/modules.abc";
    const std::string ohmUrl1 = "@normalized:N&&&entry/build/generated/r/table";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    CString entryPoint = "";
    std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile(data, filename.c_str(), ohmUrl1.c_str(), entryPoint);
    bool res = JSNApi::FindModuleInAbcFile(vm_, filename, ohmUrl1);
    EXPECT_TRUE(res);
}

HWTEST_F_L0(JSNApiTests, FindModuleInAbcFile002)
{
    LocalScope scope(vm_);
    const std::string filename = "data/storage/el1/bundle/modulename/ets/test.abc";
    const std::string ohmUrl1 = "@normalized:N&&&entry/build/generated/r/table";
    const std::string ohmUrl2 = "@normalized:N&&&entry/build/generated/r/desk";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    CString entryPoint = "";
    std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile(data, filename.c_str(), ohmUrl1.c_str(), entryPoint);
    bool res = JSNApi::FindModuleInAbcFile(vm_, filename, ohmUrl2);
    EXPECT_FALSE(res);
}

HWTEST_F_L0(JSNApiTests, FindModuleInAbcFile003)
{
    LocalScope scope(vm_);
    const std::string filename = "data/storage/el1/bundle/modulename/ets/test1.abc";
    const std::string ohmUrl1 = "@normalized:N&&&entry/build/generated/r/desk";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    CString entryPoint = "";
    std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile(data, filename.c_str(), ohmUrl1.c_str(), entryPoint);
    bool res = JSNApi::FindModuleInAbcFile(vm_, filename, ohmUrl1);
    EXPECT_TRUE(res);
}

void UncatchableErrorHandlerImpl([[maybe_unused]]panda::TryCatch& tryCatch)
{
    return;
}

HWTEST_F_L0(JSNApiTests, RegisterUncatchableErrorHandler001)
{
    LocalScope scope(vm_);
    UncatchableErrorHandler func1 = JSNApi::GetUncatchableErrorHandler(vm_);
    EXPECT_EQ(func1, nullptr);
    JSNApi::RegisterUncatchableErrorHandler(vm_, UncatchableErrorHandlerImpl);
    UncatchableErrorHandler func2 = JSNApi::GetUncatchableErrorHandler(vm_);
    EXPECT_NE(func2, nullptr);
}

HWTEST_F_L0(JSNApiTests, FindModuleInAbcFile004)
{
    LocalScope scope(vm_);
    const std::string filename = "data/storage/el1/bundle/modulename/ets/test2.abc";
    const std::string ohmUrl1 = "@normalized:N&&&entry/build/generated/r/box";
    const std::string ohmUrl2 = "@normalized:N&&&entry/build/generated/r/desk";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    CString entryPoint = "";
    std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile(data, filename.c_str(), ohmUrl1.c_str(), entryPoint);
    bool res = JSNApi::FindModuleInAbcFile(vm_, filename, ohmUrl2);
    EXPECT_FALSE(res);
}

HWTEST_F_L0(JSNApiTests, CrossThreadExecution)
{
    bool res = JSNApi::CheckAndSetAllowCrossThreadExecution(vm_);
    if (ecmascript::g_isEnableCMCGC) {
        EXPECT_FALSE(res);
    } else {
        if (res) {
            JSNApi::DisallowCrossThreadExecution(vm_);
        } else {
            GTEST_LOG_(INFO) << "vm is in the gc or shared gc";
        }
    }
}

HWTEST_F_L0(JSNApiTests, IsCrossBundleHsp)
{
    JSNApi::SetBundleName(vm_, "com.application.demo");

    const std::string ohmurl1 = "@normalized:N&hsp&com.application.demo&hsp/Index&";
    bool res1 = JSNApi::IsCrossBundleHsp(vm_, ohmurl1);
    EXPECT_EQ(res1, false);

    const std::string ohmurl2 = "@normalized:N&crosshsp&com.application.demo1&crosshsp/Index&";
    bool res2 = JSNApi::IsCrossBundleHsp(vm_, ohmurl2);
    EXPECT_EQ(res2, true);
}

HWTEST_F_L0(JSNApiTests, GetModuleNameSpaceWithOhmurlForHybridApp)
{
    LocalScope scope(vm_);
    std::map<std::string, std::vector<std::vector<std::string>>> pkgList;
    std::vector<std::string> entryList = {
        "entry",
        "packageName", "entry",
        "bundleName", "",
        "moduleName", "",
        "version", "",
        "entryPath", "src/main/",
        "isSO", "false"
    };
    pkgList["entry"] = {entryList};
    JSNApi::SetpkgContextInfoList(vm_, pkgList);

    const std::string ohmurl = "@normalized:N&crosshsp&com.application.demo&crosshsp/Index&";
    const std::string filename = "/data/storage/el1/bundle/com.application.demo/crosshsp/crosshsp/ets/modules.abc";
    const char *data = R"(
        .language ECMAScript
        .function any func_main_0(any a0, any a1, any a2) {
            ldai 1
            return
        }
    )";
    CString entryPoint = "";
    std::shared_ptr<JSPandaFile> pf = NewMockJSPandaFile(data, filename.c_str(), ohmurl.c_str(), entryPoint);

    JSHandle<SourceTextModule> testModule = vm_->GetFactory()->NewSourceTextModule();
    ecmascript::ModuleManager *moduleManager = thread_->GetModuleManager();
    testModule->SetStatus(ModuleStatus::EVALUATED);
    moduleManager->AddResolveImportedModule("com.application.demo&crosshsp/Index&", testModule.GetTaggedValue());

    Local<ObjectRef> res = JSNApi::GetModuleNameSpaceWithOhmurlForHybridApp(vm_, ohmurl);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(res);
    EXPECT_TRUE(obj.GetTaggedValue() != JSTaggedValue::Undefined());
}
} // namespace panda::test
