/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "dfx_jsnapi.h"
#include "jsnapi_helper.h"

#include <array>
#include <cstdint>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/base/path_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/accessor_data.h"
#include "ecmascript/byte_array.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/compiler/aot_file/an_file_data_manager.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/debugger/js_debugger_manager.h"
#include "ecmascript/ecma_context.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_file_path.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/js_serializer.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/jspandafile/debug_info_extractor.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/log.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/patch/quick_fix_manager.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#include "ecmascript/platform/file.h"
#include "ecmascript/regexp/regexp_parser.h"
#include "ecmascript/runtime.h"
#include "ecmascript/serializer/base_deserializer.h"
#include "ecmascript/serializer/value_serializer.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/js_weak_container.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_number_format.h"
#endif

#include "ohos/init_data.h"

#include "ecmascript/platform/mutex.h"
#include "ecmascript/platform/log.h"

namespace panda {
using ecmascript::AccessorData;
using ecmascript::BigInt;
using ecmascript::ByteArray;
using ecmascript::ECMAObject;
using ecmascript::EcmaRuntimeCallInfo;
using ecmascript::EcmaString;
using ecmascript::EcmaStringAccessor;
using ecmascript::ErrorType;
using ecmascript::FastRuntimeStub;
using ecmascript::GeneratorContext;
using ecmascript::GlobalEnv;
using ecmascript::GlobalEnvConstants;
using ecmascript::IterationKind;
using ecmascript::JSArray;
using ecmascript::JSArrayBuffer;
using ecmascript::JSDataView;
using ecmascript::JSDate;
using ecmascript::JSFunction;
using ecmascript::JSFunctionBase;
using ecmascript::JSGeneratorFunction;
using ecmascript::JSGeneratorObject;
using ecmascript::JSGeneratorState;
using ecmascript::JSHClass;
using ecmascript::JSIterator;
using ecmascript::JSMap;
using ecmascript::JSMapIterator;
using ecmascript::JSNativePointer;
using ecmascript::JSObject;
using ecmascript::JSPandaFile;
using ecmascript::JSPandaFileManager;
using ecmascript::JSPrimitiveRef;
using ecmascript::JSPromise;
using ecmascript::JSProxy;
using ecmascript::ObjectFastOperator;
using ecmascript::JSRegExp;
using ecmascript::JSRuntimeOptions;
using ecmascript::JSSerializer;
using ecmascript::JSSet;
using ecmascript::JSSetIterator;
using ecmascript::JSSymbol;
using ecmascript::JSTaggedNumber;
using ecmascript::JSTaggedType;
using ecmascript::JSTaggedValue;
using ecmascript::JSThread;
using ecmascript::JSTypedArray;
using ecmascript::LinkedHashMap;
using ecmascript::LinkedHashSet;
using ecmascript::LockHolder;
using ecmascript::MemMapAllocator;
using ecmascript::Method;
using ecmascript::Mutex;
using ecmascript::ObjectFactory;
using ecmascript::OperationResult;
using ecmascript::PromiseCapability;
using ecmascript::PropertyDescriptor;
using ecmascript::Region;
using ecmascript::TaggedArray;
using ecmascript::base::BuiltinsBase;
using ecmascript::base::JsonStringifier;
using ecmascript::base::StringHelper;
using ecmascript::base::TypedArrayHelper;
using ecmascript::base::Utf16JsonParser;
using ecmascript::base::Utf8JsonParser;
using ecmascript::builtins::BuiltinsObject;
using ecmascript::job::MicroJobQueue;
using ecmascript::job::QueueType;
#ifdef ARK_SUPPORT_INTL
using ecmascript::JSCollator;
using ecmascript::JSDateTimeFormat;
using ecmascript::JSNumberFormat;
#endif
using ecmascript::DebugInfoExtractor;
using ecmascript::EcmaContext;
using ecmascript::JSWeakMap;
using ecmascript::JSWeakSet;
using ecmascript::Log;
using ecmascript::PatchErrorCode;
using ecmascript::RegExpParser;
using ecmascript::base::NumberHelper;
template <typename T>
using JSHandle = ecmascript::JSHandle<T>;
template <typename T>
using JSMutableHandle = ecmascript::JSMutableHandle<T>;

using PathHelper = ecmascript::base::PathHelper;
using ModulePathHelper = ecmascript::ModulePathHelper;
using JsDebuggerManager = ecmascript::tooling::JsDebuggerManager;
using FrameIterator = ecmascript::FrameIterator;

namespace {
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
constexpr std::string_view ENTRY_POINTER = "_GLOBAL::func_main_0";
}

#define XPM_PROC_PREFIX "/proc/"
#define XPM_PROC_SUFFIX "/xpm_region"
#define XPM_PROC_LENGTH 50

// ----------------------------------- JSValueRef --------------------------------------
Local<PrimitiveRef> JSValueRef::Undefined(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(
        vm->GetJSThread()->GlobalConstants()->GetHandledUndefined());
}

Local<PrimitiveRef> JSValueRef::Null(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(
        vm->GetJSThread()->GlobalConstants()->GetHandledNull());
}

Local<PrimitiveRef> JSValueRef::True(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(
        vm->GetJSThread()->GlobalConstants()->GetHandledTrue());
}

Local<PrimitiveRef> JSValueRef::False(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(
        vm->GetJSThread()->GlobalConstants()->GetHandledFalse());
}

Local<ObjectRef> JSValueRef::ToObject(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    if (IsUndefined() || IsNull()) {
        return Undefined(vm);
    }
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj(JSTaggedValue::ToObject(thread, JSNApiHelper::ToJSHandle(this)));
    LOG_IF_SPECIAL(obj, ERROR);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<ObjectRef>(obj);
}

Local<StringRef> JSValueRef::ToString(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    if (!obj->IsString()) {
        obj = JSHandle<JSTaggedValue>(JSTaggedValue::ToString(thread, obj));
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    }
    return JSNApiHelper::ToLocal<StringRef>(obj);
}

Local<NativePointerRef> JSValueRef::ToNativePointer(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    return JSNApiHelper::ToLocal<NativePointerRef>(obj);
}

bool JSValueRef::BooleaValue()
{
    return JSNApiHelper::ToJSTaggedValue(this).ToBoolean();
}

int64_t JSValueRef::IntegerValue(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> tagged = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(tagged, ERROR);
    if (tagged->IsNumber()) {
        if (!NumberHelper::IsFinite(tagged.GetTaggedValue()) || NumberHelper::IsNaN(tagged.GetTaggedValue())) {
            return 0;
        } else {
            return NumberHelper::DoubleToInt64(tagged->GetNumber());
        }
    }
    JSTaggedNumber number = JSTaggedValue::ToInteger(thread, tagged);
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return NumberHelper::DoubleToInt64(number.GetNumber());
}

uint32_t JSValueRef::Uint32Value(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    uint32_t number = JSTaggedValue::ToUint32(thread, JSNApiHelper::ToJSHandle(this));
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number;
}

int32_t JSValueRef::Int32Value(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    int32_t number = JSTaggedValue::ToInt32(thread, JSNApiHelper::ToJSHandle(this));
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number;
}

Local<BooleanRef> JSValueRef::ToBoolean(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> booleanObj(thread, JSTaggedValue(obj->ToBoolean()));
    return JSNApiHelper::ToLocal<BooleanRef>(booleanObj);
}

Local<BigIntRef> JSValueRef::ToBigInt(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> bigIntObj(thread, JSTaggedValue::ToBigInt(thread, obj));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<BigIntRef>(bigIntObj);
}

Local<NumberRef> JSValueRef::ToNumber(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue::ToNumber(thread, obj));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

bool JSValueRef::IsStrictEquals(const EcmaVM *vm, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    JSHandle<JSTaggedValue> xValue = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(xValue, ERROR);
    JSHandle<JSTaggedValue> yValue = JSNApiHelper::ToJSHandle(value);
    return JSTaggedValue::StrictEqual(thread, xValue, yValue);
}

Local<StringRef> JSValueRef::Typeof(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue value = FastRuntimeStub::FastTypeOf(thread, JSNApiHelper::ToJSTaggedValue(this));
    return JSNApiHelper::ToLocal<StringRef>(JSHandle<JSTaggedValue>(thread, value));
}

bool JSValueRef::InstanceOf(const EcmaVM *vm, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> origin = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(origin, ERROR);
    JSHandle<JSTaggedValue> target = JSNApiHelper::ToJSHandle(value);
    return JSObject::InstanceOf(thread, origin, target);
}

// Omit exception check for JSValueRef::IsXxx because ark calls here may not
// cause side effect even pending exception exists.
bool JSValueRef::IsUndefined()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsUndefined();
}

bool JSValueRef::IsNull()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsNull();
}

bool JSValueRef::IsHole()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsHole();
}

bool JSValueRef::IsTrue()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsTrue();
}

bool JSValueRef::IsFalse()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsFalse();
}

bool JSValueRef::IsNumber()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsNumber();
}

bool JSValueRef::IsBigInt()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsBigInt();
}

bool JSValueRef::IsInt()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsInt();
}

bool JSValueRef::WithinInt32()
{
    return JSNApiHelper::ToJSTaggedValue(this).WithinInt32();
}

bool JSValueRef::IsBoolean()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsBoolean();
}

bool JSValueRef::IsString()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsString();
}

bool JSValueRef::IsSymbol()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsSymbol();
}

bool JSValueRef::IsObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsECMAObject();
}

bool JSValueRef::IsArray(const EcmaVM *vm)
{
    CROSS_THREAD_CHECK(vm);
    return JSNApiHelper::ToJSTaggedValue(this).IsArray(thread);
}

bool JSValueRef::IsJSArray([[maybe_unused]] const EcmaVM *vm)
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSArray();
}

bool JSValueRef::IsConstructor()
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsHeapObject() && value.IsConstructor();
}

bool JSValueRef::IsFunction()
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsCallable();
}

bool JSValueRef::IsJSFunction()
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsJSFunction();
}

bool JSValueRef::IsProxy()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSProxy();
}

bool JSValueRef::IsPromise()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPromise();
}

bool JSValueRef::IsDataView()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsDataView();
}

bool JSValueRef::IsTypedArray()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsTypedArray();
}

bool JSValueRef::IsNativePointer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSNativePointer();
}

bool JSValueRef::IsDate()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsDate();
}

bool JSValueRef::IsError()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSError();
}

bool JSValueRef::IsMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSMap();
}

bool JSValueRef::IsSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSet();
}

bool JSValueRef::IsWeakRef()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakRef();
}

bool JSValueRef::IsWeakMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakMap();
}

bool JSValueRef::IsWeakSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakSet();
}

bool JSValueRef::IsRegExp()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSRegExp();
}

bool JSValueRef::IsArrayIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSArrayIterator();
}

bool JSValueRef::IsStringIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsStringIterator();
}

bool JSValueRef::IsSetIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSetIterator();
}

bool JSValueRef::IsMapIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSMapIterator();
}

bool JSValueRef::IsArrayBuffer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsArrayBuffer();
}

bool JSValueRef::IsBuffer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsArrayBuffer();
}

bool JSValueRef::IsUint8Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint8Array();
}

bool JSValueRef::IsInt8Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt8Array();
}

bool JSValueRef::IsUint8ClampedArray()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint8ClampedArray();
}

bool JSValueRef::IsInt16Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt16Array();
}

bool JSValueRef::IsUint16Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint16Array();
}

bool JSValueRef::IsInt32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt32Array();
}

bool JSValueRef::IsUint32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint32Array();
}

bool JSValueRef::IsFloat32Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSFloat32Array();
}

bool JSValueRef::IsFloat64Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSFloat64Array();
}

bool JSValueRef::IsBigInt64Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSBigInt64Array();
}

bool JSValueRef::IsBigUint64Array()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSBigUint64Array();
}

bool JSValueRef::IsJSPrimitiveRef()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPrimitiveRef();
}

bool JSValueRef::IsJSPrimitiveNumber()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsNumber();
}

bool JSValueRef::IsJSPrimitiveInt()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsInt();
}

bool JSValueRef::IsJSPrimitiveBoolean()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsBoolean();
}

bool JSValueRef::IsJSPrimitiveString()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsString();
}

bool JSValueRef::IsJSPrimitiveSymbol()
{
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsSymbol();
}

bool JSValueRef::IsGeneratorObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsGeneratorObject();
}

bool JSValueRef::IsModuleNamespaceObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsModuleNamespace();
}

bool JSValueRef::IsSharedArrayBuffer()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsSharedArrayBuffer();
}

bool JSValueRef::IsJSLocale()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSLocale();
}

bool JSValueRef::IsJSDateTimeFormat()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSDateTimeFormat();
}

bool JSValueRef::IsJSRelativeTimeFormat()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSRelativeTimeFormat();
}

bool JSValueRef::IsJSIntl()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSIntl();
}

bool JSValueRef::IsJSNumberFormat()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSNumberFormat();
}

bool JSValueRef::IsJSCollator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSCollator();
}

bool JSValueRef::IsJSPluralRules()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPluralRules();
}

bool JSValueRef::IsJSListFormat()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSListFormat();
}

bool JSValueRef::IsAsyncGeneratorObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsAsyncGeneratorObject();
}

bool JSValueRef::IsAsyncFunction()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAsyncFunction();
}

bool JSValueRef::IsArgumentsObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsArguments();
}

bool JSValueRef::IsGeneratorFunction()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsGeneratorFunction();
}

bool JSValueRef::IsAsyncGeneratorFunction()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsAsyncGeneratorFunction();
}

bool JSValueRef::IsArrayList()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIArrayList();
}

bool JSValueRef::IsDeque()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIDeque();
}

bool JSValueRef::IsHashMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIHashMap();
}

bool JSValueRef::IsHashSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIHashSet();
}

bool JSValueRef::IsLightWeightMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILightWeightMap();
}

bool JSValueRef::IsLightWeightSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILightWeightSet();
}

bool JSValueRef::IsLinkedList()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILinkedList();
}

bool JSValueRef::IsLinkedListIterator()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILinkedListIterator();
}

bool JSValueRef::IsList()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIList();
}

bool JSValueRef::IsPlainArray()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIPlainArray();
}

bool JSValueRef::IsQueue()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIQueue();
}

bool JSValueRef::IsStack()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIStack();
}

bool JSValueRef::IsTreeMap()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPITreeMap();
}

bool JSValueRef::IsTreeSet()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPITreeSet();
}

bool JSValueRef::IsVector()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIVector();
}

bool JSValueRef::IsSharedObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedObject();
}

// ---------------------------------- DataView -----------------------------------
Local<DataViewRef> DataViewRef::New(
    const EcmaVM *vm, Local<ArrayBufferRef> arrayBuffer, uint32_t byteOffset, uint32_t byteLength)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();

    JSHandle<JSArrayBuffer> buffer(JSNApiHelper::ToJSHandle(arrayBuffer));
    JSHandle<JSDataView> dataView = factory->NewJSDataView(buffer, byteOffset, byteLength);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<DataViewRef>(JSHandle<JSTaggedValue>(dataView));
}

uint32_t DataViewRef::ByteLength()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    return dataView->GetByteLength();
}

uint32_t DataViewRef::ByteOffset()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    return dataView->GetByteOffset();
}

Local<ArrayBufferRef> DataViewRef::GetArrayBuffer(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(dataView, FATAL);
    JSHandle<JSTaggedValue> arrayBuffer(thread, dataView->GetViewedArrayBuffer());
    return JSNApiHelper::ToLocal<ArrayBufferRef>(arrayBuffer);
}
// ---------------------------------- DataView -----------------------------------

// ----------------------------------- PritimitiveRef ---------------------------------------
Local<JSValueRef> PrimitiveRef::GetValue(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    if (obj->IsJSPrimitiveRef()) {
        JSTaggedValue primitiveValue = JSPrimitiveRef::Cast(obj->GetTaggedObject())->GetValue();
        JSHandle<JSTaggedValue> value(thread, primitiveValue);
        return JSNApiHelper::ToLocal<JSValueRef>(value);
    }
    return Local<JSValueRef>();
}

// ----------------------------------- NumberRef ---------------------------------------
Local<NumberRef> NumberRef::New(const EcmaVM *vm, double input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (std::isnan(input)) {
        input = ecmascript::base::NAN_VALUE;
    }
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, int32_t input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, uint32_t input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, int64_t input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

double NumberRef::Value()
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    return JSTaggedNumber(JSNApiHelper::ToJSTaggedValue(this)).GetNumber();
}

// ----------------------------------- MapRef ---------------------------------------
Local<JSValueRef> MapRef::Get(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread,
                map->Get(thread, JSNApiHelper::ToJSTaggedValue(*key))));
}

void MapRef::Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    JSMap::Set(thread, map, JSNApiHelper::ToJSHandle(key), JSNApiHelper::ToJSHandle(value));
}

Local<MapRef> MapRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> map =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(thread);
    map->SetLinkedMap(thread, hashMap);
    JSHandle<JSTaggedValue> mapTag = JSHandle<JSTaggedValue>::Cast(map);
    return JSNApiHelper::ToLocal<MapRef>(mapTag);
}

int32_t MapRef::GetSize()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    return map->GetSize();
}

int32_t MapRef::GetTotalElements()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    return static_cast<int>((map->GetSize())) +
        LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> MapRef::GetKey(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, map->GetKey(entry)));
}

Local<JSValueRef> MapRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, map->GetValue(entry)));
}

// ----------------------------------- MapIteratorRef ---------------------------------------
int32_t MapIteratorRef::GetIndex()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, -1);
    JSHandle<JSMapIterator> jsMapIter(JSNApiHelper::ToJSHandle(this));
    return jsMapIter->GetNextIndex();
}

Local<JSValueRef> MapIteratorRef::GetKind(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSMapIterator> jsMapIter(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsMapIter, FATAL);
    IterationKind iterKind = jsMapIter->GetIterationKind();
    Local<JSValueRef> result;
    switch (iterKind) {
        case IterationKind::KEY:
            result = StringRef::NewFromUtf8(vm, "keys");
            break;
        case IterationKind::VALUE:
            result = StringRef::NewFromUtf8(vm, "values");
            break;
        case IterationKind::KEY_AND_VALUE:
            result = StringRef::NewFromUtf8(vm, "entries");
            break;
        default:
            break;
    }
    return result;
}

Local<MapIteratorRef> MapIteratorRef::New(const EcmaVM *vm, Local<MapRef> map)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSMap> jsMap(JSNApiHelper::ToJSHandle(map));
    IterationKind iterKind = IterationKind::KEY_AND_VALUE;
    JSHandle<JSTaggedValue> mapIteratorKeyAndValue =
        JSMapIterator::CreateMapIterator(vm->GetJSThread(), JSHandle<JSTaggedValue>::Cast(jsMap), iterKind);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(mapIteratorKeyAndValue);
}

ecmascript::EcmaRuntimeCallInfo *MapIteratorRef::GetEcmaRuntimeCallInfo(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, nullptr);
    JSHandle<JSMapIterator> jsMapIter(JSNApiHelper::ToJSHandle(this));
    JSHandle<LinkedHashMap> linkedHashMap(vm->GetJSThread(), jsMapIter->GetIteratedMap());
    uint32_t size = linkedHashMap->GetLength();
    return ecmascript::EcmaInterpreter::NewRuntimeCallInfo(vm->GetJSThread(),
        JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::Undefined()),
        JSHandle<JSTaggedValue>(vm->GetJSThread(), jsMapIter.GetTaggedValue()),
        JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::Undefined()), size);
}

Local<ArrayRef> MapIteratorRef::Next(const EcmaVM *vm, ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> nextTagValResult(vm->GetJSThread(), JSMapIterator::Next(ecmaRuntimeCallInfo));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> iteratorVal(vm->GetJSThread(),
        JSIterator::IteratorValue(vm->GetJSThread(), nextTagValResult).GetTaggedValue());
    return JSNApiHelper::ToLocal<ArrayRef>(iteratorVal);
}

// ----------------------------------- SetIteratorRef ---------------------------------------
int32_t SetIteratorRef::GetIndex()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, -1);
    JSHandle<JSSetIterator> jsSetIter(JSNApiHelper::ToJSHandle(this));
    return jsSetIter->GetNextIndex();
}

Local<JSValueRef> SetIteratorRef::GetKind(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSSetIterator> jsSetIter(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsSetIter, FATAL);
    IterationKind iterKind = jsSetIter->GetIterationKind();
    Local<JSValueRef> result;
    switch (iterKind) {
        case IterationKind::KEY:
            result = StringRef::NewFromUtf8(vm, "keys");
            break;
        case IterationKind::VALUE:
            result = StringRef::NewFromUtf8(vm, "values");
            break;
        case IterationKind::KEY_AND_VALUE:
            result = StringRef::NewFromUtf8(vm, "entries");
            break;
        default:
            break;
    }
    return result;
}

Local<SetIteratorRef> SetIteratorRef::New(const EcmaVM *vm, Local<SetRef> set)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSSet> jsSet(JSNApiHelper::ToJSHandle(set));
    IterationKind iterKind = IterationKind::KEY_AND_VALUE;
    JSHandle<JSTaggedValue> setIteratorKeyAndValue =
        JSSetIterator::CreateSetIterator(vm->GetJSThread(), JSHandle<JSTaggedValue>::Cast(jsSet), iterKind);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(setIteratorKeyAndValue);
}

ecmascript::EcmaRuntimeCallInfo *SetIteratorRef::GetEcmaRuntimeCallInfo(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, nullptr);
    JSHandle<JSSetIterator> jsSetIter(JSNApiHelper::ToJSHandle(this));
    JSHandle<LinkedHashSet> linkedHashSet(vm->GetJSThread(), jsSetIter->GetIteratedSet());
    uint32_t size = linkedHashSet->GetLength();
    return ecmascript::EcmaInterpreter::NewRuntimeCallInfo(vm->GetJSThread(),
        JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::Undefined()),
        JSHandle<JSTaggedValue>(vm->GetJSThread(), jsSetIter.GetTaggedValue()),
        JSHandle<JSTaggedValue>(vm->GetJSThread(), JSTaggedValue::Undefined()), size);
}

Local<ArrayRef> SetIteratorRef::Next(const EcmaVM *vm, ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> nextTagValResult(vm->GetJSThread(), JSSetIterator::Next(ecmaRuntimeCallInfo));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> iteratorVal(vm->GetJSThread(),
        JSIterator::IteratorValue(vm->GetJSThread(), nextTagValResult).GetTaggedValue());
    return JSNApiHelper::ToLocal<ArrayRef>(iteratorVal);
}

// ---------------------------------- Buffer -----------------------------------
Local<BufferRef> BufferRef::New(const EcmaVM *vm, int32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSArrayBuffer> arrayBuffer = JSHandle<JSArrayBuffer>::Cast(factory->NewJSArrayBuffer(length));
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current =
        factory->NewJSFunction(env, reinterpret_cast<void *>(BufferRef::BufferToStringCallback));
    Local<StringRef> key = StringRef::NewFromUtf8(vm, "toString");
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    JSHandle<JSTaggedValue> currentTaggedValue(current);
    JSHandle<JSTaggedValue> obj(arrayBuffer);
    bool result = JSTaggedValue::SetProperty(thread, obj, keyValue, currentTaggedValue);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    if (!result) {
        LOG_ECMA(ERROR) << "SetProperty failed ! ! !";
    }
    return JSNApiHelper::ToLocal<BufferRef>(obj);
}

Local<BufferRef> BufferRef::New(
    const EcmaVM *vm, void *buffer, int32_t length, const Deleter &deleter, void *data)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSArrayBuffer> arrayBuffer =
        factory->NewJSArrayBuffer(buffer, length, reinterpret_cast<ecmascript::DeleteEntryPoint>(deleter), data);

    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current =
        factory->NewJSFunction(env, reinterpret_cast<void *>(BufferRef::BufferToStringCallback));
    Local<StringRef> key = StringRef::NewFromUtf8(vm, "toString");
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    JSHandle<JSTaggedValue> currentTaggedValue(current);
    JSHandle<JSTaggedValue> obj(arrayBuffer);
    bool result = JSTaggedValue::SetProperty(thread, obj, keyValue, currentTaggedValue);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    if (!result) {
        LOG_ECMA(ERROR) << "SetProperty failed ! ! !";
    }
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

int32_t BufferRef::ByteLength([[maybe_unused]] const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    return arrayBuffer->GetArrayBufferByteLength();
}

void *BufferRef::GetBuffer()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, nullptr);
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue bufferData = arrayBuffer->GetArrayBufferData();
    if (!bufferData.IsJSNativePointer()) {
        return nullptr;
    }
    return JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
}

JSTaggedValue BufferRef::BufferToStringCallback(ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo)
{
    JSThread *thread = ecmaRuntimeCallInfo->GetThread();
    [[maybe_unused]] LocalScope scope(thread->GetEcmaVM());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> arrayBuff = ecmaRuntimeCallInfo->GetThis();
    JSHandle<JSArrayBuffer> arrayBuffer(arrayBuff);

    uint32_t length = arrayBuffer->GetArrayBufferByteLength();
    JSTaggedValue data = arrayBuffer->GetArrayBufferData();

    ecmascript::CVector<uint16_t> valueTable;
    valueTable.reserve(length);
    for (uint32_t i = 0; i < length; i++) {
        void* rawData = reinterpret_cast<void *>(
            ToUintPtr(JSNativePointer::Cast(data.GetTaggedObject())->GetExternalPointer()) + i);
        uint8_t *block = reinterpret_cast<uint8_t *>(rawData);
        uint16_t nextCv = static_cast<uint16_t>(*block);
        valueTable.emplace_back(nextCv);
    }

    auto *char16tData0 = reinterpret_cast<const char16_t *>(valueTable.data());
    std::u16string u16str(char16tData0, length);

    const char16_t *constChar16tData = u16str.data();
    auto *char16tData = const_cast<char16_t *>(constChar16tData);
    auto *uint16tData = reinterpret_cast<uint16_t *>(char16tData);
    uint32_t u16strSize = u16str.size();
    JSTaggedValue rString = factory->NewFromUtf16Literal(uint16tData, u16strSize).GetTaggedValue();
    JSHandle<EcmaString> StringHandle = JSTaggedValue::ToString(thread, rString);
    RETURN_VALUE_IF_ABRUPT(thread, JSTaggedValue::Undefined());
    return StringHandle.GetTaggedValue();
}

// ---------------------------------- Promise --------------------------------------
Local<PromiseCapabilityRef> PromiseCapabilityRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor(globalEnv->GetPromiseFunction());
    JSHandle<JSTaggedValue> capability(JSPromise::NewPromiseCapability(thread, constructor));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseCapabilityRef>(capability);
}

Local<PromiseRef> PromiseCapabilityRef::GetPromise(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, capacity->GetPromise()));
}

bool PromiseCapabilityRef::Resolve(const EcmaVM *vm, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(value);
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    JSHandle<JSTaggedValue> resolve(thread, capacity->GetResolve());
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    info->SetCallArg(arg.GetTaggedValue());
    JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT(thread, false);

    thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    RETURN_VALUE_IF_ABRUPT(thread, false);
    vm->GetHeap()->ClearKeptObjects();
    return true;
}

bool PromiseCapabilityRef::Reject(const EcmaVM *vm, Local<JSValueRef> reason)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(reason);
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    JSHandle<JSTaggedValue> reject(thread, capacity->GetReject());
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());

    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    info->SetCallArg(arg.GetTaggedValue());
    JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT(thread, false);

    thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    RETURN_VALUE_IF_ABRUPT(thread, false);
    vm->GetHeap()->ClearKeptObjects();
    return true;
}

// ----------------------------------- SymbolRef -----------------------------------------
Local<SymbolRef> SymbolRef::New(const EcmaVM *vm, Local<StringRef> description)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSSymbol> symbol = factory->NewJSSymbol();
    if (!description.IsEmpty()) {
        JSTaggedValue desc = JSNApiHelper::ToJSTaggedValue(*description);
        symbol->SetDescription(thread, desc);
    }
    return JSNApiHelper::ToLocal<SymbolRef>(JSHandle<JSTaggedValue>(symbol));
}

Local<StringRef> SymbolRef::GetDescription(const EcmaVM *vm)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue description = JSSymbol::Cast(
        JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetDescription();
    if (!description.IsString()) {
        auto constants = thread->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> descriptionHandle(thread, description);
    return JSNApiHelper::ToLocal<StringRef>(descriptionHandle);
}

// ----------------------------------- BooleanRef ---------------------------------------
Local<BooleanRef> BooleanRef::New(const EcmaVM *vm, bool value)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> boolean(thread, JSTaggedValue(value));
    return JSNApiHelper::ToLocal<BooleanRef>(boolean);
}

bool BooleanRef::Value()
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    return JSNApiHelper::ToJSTaggedValue(this).IsTrue();
}

// ----------------------------------- StringRef ----------------------------------------
Local<StringRef> StringRef::NewFromUtf8(const EcmaVM *vm, const char *utf8, int length)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    if (length < 0) {
        JSHandle<JSTaggedValue> current(factory->NewFromUtf8(utf8));
        return JSNApiHelper::ToLocal<StringRef>(current);
    }
    JSHandle<JSTaggedValue> current(factory->NewFromUtf8(reinterpret_cast<const uint8_t *>(utf8), length));
    return JSNApiHelper::ToLocal<StringRef>(current);
}

Local<StringRef> StringRef::NewFromUtf16(const EcmaVM *vm, const char16_t *utf16, int length)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    if (length < 0) {
        JSHandle<JSTaggedValue> current(factory->NewFromUtf16(utf16));
        return JSNApiHelper::ToLocal<StringRef>(current);
    }
    JSHandle<JSTaggedValue> current(factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(utf16), length));
    return JSNApiHelper::ToLocal<StringRef>(current);
}

std::string StringRef::ToString()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, "");
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this)).ToStdString();
}

std::string StringRef::DebuggerToString()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, "");
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this)).DebuggerToStdString();
}

uint32_t StringRef::Length()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this)).GetLength();
}

int32_t StringRef::Utf8Length(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<EcmaString> strHandle(vm->GetJSThread(), EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(this)));
    return EcmaStringAccessor(EcmaStringAccessor::Flatten(vm, strHandle)).GetUtf8Length();
}

int StringRef::WriteUtf8(char *buffer, int length, bool isWriteBuffer)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToFlatUtf8(reinterpret_cast<uint8_t *>(buffer), length, isWriteBuffer);
}

int StringRef::WriteUtf16(char16_t *buffer, int length)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToUtf16(reinterpret_cast<uint16_t *>(buffer), length);
}

int StringRef::WriteLatin1(char *buffer, int length)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToOneByte(reinterpret_cast<uint8_t *>(buffer), length);
}

Local<StringRef> StringRef::GetNapiWrapperString(const EcmaVM *vm)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> napiWapperString = thread->GlobalConstants()->GetHandledNapiWrapperString();
    return JSNApiHelper::ToLocal<StringRef>(napiWapperString);
}

// ---------------------------------- PromiseRejectInfo ---------------------------------
PromiseRejectInfo::PromiseRejectInfo(Local<JSValueRef> promise, Local<JSValueRef> reason,
                                     PromiseRejectInfo::PROMISE_REJECTION_EVENT operation, void* data)
    : promise_(promise), reason_(reason), operation_(operation), data_(data) {}

Local<JSValueRef> PromiseRejectInfo::GetPromise() const
{
    return promise_;
}

Local<JSValueRef> PromiseRejectInfo::GetReason() const
{
    return reason_;
}

PromiseRejectInfo::PROMISE_REJECTION_EVENT PromiseRejectInfo::GetOperation() const
{
    return operation_;
}

void* PromiseRejectInfo::GetData() const
{
    return data_;
}

// ----------------------------------- BigIntRef ---------------------------------------
Local<BigIntRef> BigIntRef::New(const EcmaVM *vm, uint64_t input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<BigInt> big = BigInt::Uint64ToBigInt(thread, input);
    JSHandle<JSTaggedValue> bigint = JSHandle<JSTaggedValue>::Cast(big);
    return JSNApiHelper::ToLocal<BigIntRef>(bigint);
}

Local<BigIntRef> BigIntRef::New(const EcmaVM *vm, int64_t input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<BigInt> big = BigInt::Int64ToBigInt(thread, input);
    JSHandle<JSTaggedValue> bigint = JSHandle<JSTaggedValue>::Cast(big);
    return JSNApiHelper::ToLocal<BigIntRef>(bigint);
}

Local<JSValueRef> BigIntRef::CreateBigWords(const EcmaVM *vm, bool sign, uint32_t size, const uint64_t* words)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<BigInt> big = BigInt::CreateBigWords(thread, sign, size, words);
    JSHandle<JSTaggedValue> bigint = JSHandle<JSTaggedValue>::Cast(big);
    return JSNApiHelper::ToLocal<JSValueRef>(bigint);
}

void BigIntRef::BigIntToInt64(const EcmaVM *vm, int64_t *value, bool *lossless)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> bigintVal(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(bigintVal, ERROR);
    BigInt::BigIntToInt64(thread, bigintVal, value, lossless);
}

void BigIntRef::BigIntToUint64(const EcmaVM *vm, uint64_t *value, bool *lossless)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> bigintVal(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(bigintVal, ERROR);
    BigInt::BigIntToUint64(thread, bigintVal, value, lossless);
}

void BigIntRef::GetWordsArray(bool* signBit, size_t wordCount, uint64_t* words)
{
    DCHECK_SPECIAL_VALUE(this);
    JSHandle<BigInt> bigintVal(JSNApiHelper::ToJSHandle(this));
    uint32_t len = bigintVal->GetLength();
    uint32_t count = 0;
    uint32_t index = 0;
    for (; index < wordCount - 1; ++index) {
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
        words[index] |= static_cast<uint64_t>(bigintVal->GetDigit(count++)) << 32; // 32 : int32_t bits
    }
    if (len % 2 == 0) { // 2 : len is odd or even
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
        words[index] |= static_cast<uint64_t>(bigintVal->GetDigit(count++)) << 32; // 32 : int32_t bits
    } else {
        words[index] = static_cast<uint64_t>(bigintVal->GetDigit(count++));
    }
    *signBit = bigintVal->GetSign();
}

uint32_t BigIntRef::GetWordsArraySize()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<BigInt> bigintVal(JSNApiHelper::ToJSHandle(this));
    uint32_t len = bigintVal->GetLength();
    return len % 2 != 0 ? len / 2 + 1 : len / 2; // 2 : len is odd or even
}

// ----------------------------------- HandleScope -------------------------------------
LocalScope::LocalScope(const EcmaVM *vm) : thread_(vm->GetJSThread())
{
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
    prevNext_ = context->GetHandleScopeStorageNext();
    prevEnd_ = context->GetHandleScopeStorageEnd();
    prevHandleStorageIndex_ = context->GetCurrentHandleStorageIndex();
    context->HandleScopeCountAdd();
}

LocalScope::LocalScope(const EcmaVM *vm, JSTaggedType value) : thread_(vm->GetJSThread())
{
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
    ecmascript::EcmaHandleScope::NewHandle(reinterpret_cast<JSThread *>(thread_), value);
    prevNext_ = context->GetHandleScopeStorageNext();
    prevEnd_ = context->GetHandleScopeStorageEnd();
    prevHandleStorageIndex_ = context->GetCurrentHandleStorageIndex();
    context->HandleScopeCountAdd();
}

LocalScope::~LocalScope()
{
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
    context->HandleScopeCountDec();
    context->SetHandleScopeStorageNext(static_cast<JSTaggedType *>(prevNext_));
    if (context->GetHandleScopeStorageEnd() != prevEnd_) {
        context->SetHandleScopeStorageEnd(static_cast<JSTaggedType *>(prevEnd_));
        context->ShrinkHandleStorage(prevHandleStorageIndex_);
    }
}

// ----------------------------------- EscapeLocalScope ------------------------------
EscapeLocalScope::EscapeLocalScope(const EcmaVM *vm) : LocalScope(vm, JSTaggedValue::Undefined().GetRawData())
{
    auto thread = vm->GetJSThread();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    escapeHandle_ = ToUintPtr(thread->GetCurrentEcmaContext()->GetHandleScopeStorageNext() - 1);
}

// ----------------------------------- IntegerRef ---------------------------------------
Local<IntegerRef> IntegerRef::New(const EcmaVM *vm, int input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    JSHandle<JSTaggedValue> integer(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<IntegerRef>(integer);
}

Local<IntegerRef> IntegerRef::NewFromUnsigned(const EcmaVM *vm, unsigned int input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    JSHandle<JSTaggedValue> integer(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<IntegerRef>(integer);
}

int IntegerRef::Value()
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    return JSNApiHelper::ToJSTaggedValue(this).GetInt();
}

// ----------------------------------- ObjectRef ----------------------------------------
Local<ObjectRef> ObjectRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSFunction> constructor(globalEnv->GetObjectFunction());
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(constructor));
    return JSNApiHelper::ToLocal<ObjectRef>(object);
}

Local<ObjectRef> ObjectRef::NewWithProperties(const EcmaVM *vm, size_t propertyCount,
                                              const Local<JSValueRef> *keys,
                                              const PropertyAttribute *attributes)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> obj;
    auto CreateObjImpl = [vm, propertyCount, keys, attributes] (uintptr_t head) -> JSHandle<JSTaggedValue> {
        JSThread *thread = vm->GetJSThread();
        const PropertyDescriptor *desc = reinterpret_cast<const PropertyDescriptor *>(head);
        for (size_t i = 0; i < propertyCount; ++i) {
            const PropertyAttribute &attr = attributes[i];
            new (reinterpret_cast<void *>(head)) PropertyDescriptor(thread,
                                                                    JSNApiHelper::ToJSHandle(attr.GetValue(vm)),
                                                                    attr.IsWritable(), attr.IsEnumerable(),
                                                                    attr.IsConfigurable());
            head += sizeof(PropertyDescriptor);
        }
        ObjectFactory *factory = vm->GetFactory();
        return factory->CreateJSObjectWithProperties(propertyCount, keys, desc);
    };
    if (propertyCount <= MAX_PROPERTIES_ON_STACK) {
        char desc[sizeof(PropertyDescriptor) * MAX_PROPERTIES_ON_STACK];
        obj = CreateObjImpl(reinterpret_cast<uintptr_t>(desc));
    } else {
        void *desc = malloc(sizeof(PropertyDescriptor) * propertyCount);
        obj = CreateObjImpl(reinterpret_cast<uintptr_t>(desc));
        free(desc);
    }
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return scope.Escape(JSNApiHelper::ToLocal<ObjectRef>(obj));
}

Local<ObjectRef> ObjectRef::NewWithNamedProperties(const EcmaVM *vm, size_t propertyCount,
                                                   const char **keys, const Local<JSValueRef> *values)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> obj = factory->CreateJSObjectWithNamedProperties(propertyCount, keys, values);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return scope.Escape(JSNApiHelper::ToLocal<ObjectRef>(obj));
}

Local<ObjectRef> ObjectRef::CreateAccessorData(const EcmaVM *vm,
                                               Local<FunctionRef> getter, Local<FunctionRef> setter)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> getterValue = JSNApiHelper::ToJSHandle(getter);
    JSHandle<JSTaggedValue> setterValue = JSNApiHelper::ToJSHandle(setter);
    JSHandle<AccessorData> accessor = thread->GetEcmaVM()->GetFactory()->NewAccessorData();
    accessor->SetGetter(thread, getterValue);
    accessor->SetSetter(thread, setterValue);
    return JSNApiHelper::ToLocal<ObjectRef>(JSHandle<JSTaggedValue>::Cast(accessor));
}

bool ObjectRef::ConvertToNativeBindingObject(const EcmaVM *vm, Local<NativePointerRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> keyValue = env->GetNativeBindingSymbol();
    JSHandle<JSTaggedValue> valueValue = JSNApiHelper::ToJSHandle(value);
    bool result = JSTaggedValue::SetProperty(vm->GetJSThread(), object, keyValue, valueValue);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    object->GetTaggedObject()->GetClass()->SetIsNativeBindingObject(true);
    return result;
}

bool ObjectRef::Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    JSHandle<JSTaggedValue> valueValue = JSNApiHelper::ToJSHandle(value);
    if (!obj->IsHeapObject()) {
        return JSTaggedValue::SetProperty(thread, obj, keyValue, valueValue);
    }
    return ObjectFastOperator::FastSetPropertyByValue(thread, obj.GetTaggedValue(),
                                                      keyValue.GetTaggedValue(),
                                                      valueValue.GetTaggedValue());
}

bool ObjectRef::Set(const EcmaVM *vm, uint32_t key, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> valueValue = JSNApiHelper::ToJSHandle(value);
    if (!obj->IsHeapObject()) {
        return JSTaggedValue::SetProperty(thread, obj, key, valueValue);
    }
    return ObjectFastOperator::FastSetPropertyByIndex(thread, obj.GetTaggedValue(),
                                                      key, valueValue.GetTaggedValue());
}

bool ObjectRef::SetAccessorProperty(const EcmaVM *vm, Local<JSValueRef> key, Local<FunctionRef> getter,
    Local<FunctionRef> setter, PropertyAttribute attribute)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> getterValue = JSNApiHelper::ToJSHandle(getter);
    JSHandle<JSTaggedValue> setterValue = JSNApiHelper::ToJSHandle(setter);
    PropertyDescriptor desc(thread, attribute.IsWritable(), attribute.IsEnumerable(), attribute.IsConfigurable());
    desc.SetValue(JSNApiHelper::ToJSHandle(attribute.GetValue(vm)));
    desc.SetSetter(setterValue);
    desc.SetGetter(getterValue);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    return JSTaggedValue::DefineOwnProperty(thread, obj, keyValue, desc);
}

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    if (!obj->IsHeapObject()) {
        OperationResult ret = JSTaggedValue::GetProperty(thread, obj, keyValue);
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(ret.GetValue()));
    }
    JSTaggedValue ret = ObjectFastOperator::FastGetPropertyByValue(thread, obj.GetTaggedValue(),
                                                                   keyValue.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, ret)));
}

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, int32_t key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    if (!obj->IsHeapObject()) {
        OperationResult ret = JSTaggedValue::GetProperty(thread, obj, key);
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(ret.GetValue()));
    }
    JSTaggedValue ret = ObjectFastOperator::FastGetPropertyByIndex(thread, obj.GetTaggedValue(), key);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, ret)));
}

bool ObjectRef::GetOwnProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute &property)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
    PropertyDescriptor desc(thread);
    bool ret = JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), keyValue, desc);
    if (!ret) {
        return false;
    }
    property.SetValue(JSNApiHelper::ToLocal<JSValueRef>(desc.GetValue()));
    if (desc.HasGetter()) {
        property.SetGetter(JSNApiHelper::ToLocal<JSValueRef>(desc.GetGetter()));
    }
    if (desc.HasSetter()) {
        property.SetSetter(JSNApiHelper::ToLocal<JSValueRef>(desc.GetSetter()));
    }
    if (desc.HasWritable()) {
        property.SetWritable(desc.IsWritable());
    }
    if (desc.HasEnumerable()) {
        property.SetEnumerable(desc.IsEnumerable());
    }
    if (desc.HasConfigurable()) {
        property.SetConfigurable(desc.IsConfigurable());
    }

    return true;
}

Local<ArrayRef> ObjectRef::GetOwnPropertyNames(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<TaggedArray> array(JSTaggedValue::GetOwnPropertyKeys(thread, obj));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    return JSNApiHelper::ToLocal<ArrayRef>(jsArray);
}

Local<ArrayRef> ObjectRef::GetAllPropertyNames(const EcmaVM *vm, uint32_t filter)
{
    // This interface is only used by napi.
    // This interface currently only supports normal objects.
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<TaggedArray> array(JSTaggedValue::GetAllPropertyKeys(thread, obj, filter));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    return JSNApiHelper::ToLocal<ArrayRef>(jsArray);
}

Local<ArrayRef> ObjectRef::GetOwnEnumerablePropertyNames(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSObject> obj(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(obj, ERROR);
    JSHandle<TaggedArray> array(JSObject::EnumerableOwnNames(thread, obj));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    return JSNApiHelper::ToLocal<ArrayRef>(jsArray);
}

Local<JSValueRef> ObjectRef::GetPrototype(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> prototype(thread, JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(object)));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(prototype);
}

bool ObjectRef::SetPrototype(const EcmaVM *vm, Local<ObjectRef> prototype)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSObject> proto(JSNApiHelper::ToJSHandle(prototype));
    return JSTaggedValue::SetPrototype(thread, JSHandle<JSTaggedValue>(object), JSHandle<JSTaggedValue>(proto));
}

bool ObjectRef::DefineProperty(const EcmaVM *vm, Local<JSValueRef> key, PropertyAttribute attribute)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    PropertyDescriptor desc(thread, attribute.IsWritable(), attribute.IsEnumerable(), attribute.IsConfigurable());
    desc.SetValue(JSNApiHelper::ToJSHandle(attribute.GetValue(vm)));
    return JSTaggedValue::DefinePropertyOrThrow(thread, object, keyValue, desc);
}

bool ObjectRef::Has(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    return JSTaggedValue::HasProperty(thread, object, keyValue);
}

bool ObjectRef::Has(const EcmaVM *vm, uint32_t key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    return JSTaggedValue::HasProperty(thread, object, key);
}

bool ObjectRef::Delete(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    return JSTaggedValue::DeleteProperty(thread, object, keyValue);
}

bool ObjectRef::Delete(const EcmaVM *vm, uint32_t key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
    return JSTaggedValue::DeleteProperty(thread, object, keyHandle);
}

Local<JSValueRef> ObjectRef::Freeze(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSObject> obj(object);
    bool status = JSObject::SetIntegrityLevel(thread, obj, ecmascript::IntegrityLevel::FROZEN);
    if (JSNApi::HasPendingException(vm)) {
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(exception));
    }
    if (!status) {
        LOG_ECMA(ERROR) << "Freeze: freeze failed";
        Local<StringRef> message = StringRef::NewFromUtf8(vm, "Freeze: freeze failed");
        Local<JSValueRef> error = Exception::Error(vm, message);
        JSNApi::ThrowException(vm, error);
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(exception));
    }
    JSHandle<JSTaggedValue> resultValue(obj);
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(resultValue));
}

Local<JSValueRef> ObjectRef::Seal(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSObject> obj(object);
    bool status = JSObject::SetIntegrityLevel(thread, obj, ecmascript::IntegrityLevel::SEALED);
    if (JSNApi::HasPendingException(vm)) {
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(exception));
    }
    if (!status) {
        LOG_ECMA(ERROR) << "Seal: seal failed";
        Local<StringRef> message = StringRef::NewFromUtf8(vm, "Freeze: freeze failed");
        Local<JSValueRef> error = Exception::Error(vm, message);
        JSNApi::ThrowException(vm, error);
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(exception));
    }
    JSHandle<JSTaggedValue> resultValue(obj);
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(resultValue));
}

void ObjectRef::SetNativePointerFieldCount(const EcmaVM *vm, int32_t count)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    // ObjectRef::New may return special value if exception occurs.
    // So we need do special value check before use it.
    DCHECK_SPECIAL_VALUE(this);
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    object->SetNativePointerFieldCount(thread, count);
}

int32_t ObjectRef::GetNativePointerFieldCount()
{
    // ObjectRef::New may return special value if exception occurs.
    // So we need do special value check before use it.
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    return object->GetNativePointerFieldCount();
}

void *ObjectRef::GetNativePointerField(int32_t index)
{
    // ObjectRef::New may return special value if exception occurs.
    // So we need do special value check before use it.
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, nullptr);
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    return object->GetNativePointerField(index);
}

void ObjectRef::SetNativePointerField(const EcmaVM *vm, int32_t index, void *nativePointer,
    NativePointerCallback callBack, void *data, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    // ObjectRef::New may return special value if exception occurs.
    // So we need do special value check before use it.
    DCHECK_SPECIAL_VALUE(this);
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    object->SetNativePointerField(thread, index, nativePointer, callBack, data, nativeBindingsize);
}

// -------------------------------- NativePointerRef ------------------------------------
Local<NativePointerRef> NativePointerRef::New(const EcmaVM *vm, void *nativePointer, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(nativePointer, nullptr, nullptr,
        false, nativeBindingsize);
    return JSNApiHelper::ToLocal<NativePointerRef>(JSHandle<JSTaggedValue>(obj));
}

Local<NativePointerRef> NativePointerRef::New(
    const EcmaVM *vm, void *nativePointer, NativePointerCallback callBack, void *data, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSNativePointer> obj = factory->NewJSNativePointer(nativePointer, callBack, data,
        false, nativeBindingsize);
    return JSNApiHelper::ToLocal<NativePointerRef>(JSHandle<JSTaggedValue>(obj));
}

void *NativePointerRef::Value()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, nullptr);
    JSHandle<JSTaggedValue> nativePointer = JSNApiHelper::ToJSHandle(this);
    return JSHandle<JSNativePointer>(nativePointer)->GetExternalPointer();
}

// ---------------------------------- Buffer -----------------------------------
Local<ArrayBufferRef> ArrayBufferRef::New(const EcmaVM *vm, int32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSArrayBuffer> arrayBuffer = factory->NewJSArrayBuffer(length);
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

Local<ArrayBufferRef> ArrayBufferRef::New(
    const EcmaVM *vm, void *buffer, int32_t length, const Deleter &deleter, void *data)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSArrayBuffer> arrayBuffer =
        factory->NewJSArrayBuffer(buffer, length, reinterpret_cast<ecmascript::DeleteEntryPoint>(deleter), data);
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

int32_t ArrayBufferRef::ByteLength([[maybe_unused]] const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(arrayBuffer, FATAL);
    return arrayBuffer->GetArrayBufferByteLength();
}

void *ArrayBufferRef::GetBuffer()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, nullptr);
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue bufferData = arrayBuffer->GetArrayBufferData();
    if (!bufferData.IsJSNativePointer()) {
        return nullptr;
    }
    return JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
}

void ArrayBufferRef::Detach(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    arrayBuffer->Detach(thread);
}

bool ArrayBufferRef::IsDetach()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, false);
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    return arrayBuffer->IsDetach();
}

// ---------------------------------- DateRef -----------------------------------
Local<DateRef> DateRef::New(const EcmaVM *vm, double time)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSFunction> dateFunction(globalEnv->GetDateFunction());
    JSHandle<JSDate> dateObject(factory->NewJSObjectByConstructor(dateFunction));
    dateObject->SetTimeValue(thread, JSTaggedValue(time));
    return JSNApiHelper::ToLocal<DateRef>(JSHandle<JSTaggedValue>(dateObject));
}

Local<StringRef> DateRef::ToString(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSDate> date(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(date, ERROR);
    JSTaggedValue dateStr = date->ToString(thread);
    if (!dateStr.IsString()) {
        auto constants = thread->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> dateStrHandle(thread, dateStr);
    return JSNApiHelper::ToLocal<StringRef>(dateStrHandle);
}

double DateRef::GetTime()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0.0);
    JSHandle<JSDate> date(JSNApiHelper::ToJSHandle(this));
    if (!date->IsDate()) {
        LOG_ECMA(ERROR) << "Not a Date Object";
    }
    return date->GetTime().GetDouble();
}

// ---------------------------------- TypedArray -----------------------------------
uint32_t TypedArrayRef::ByteLength([[maybe_unused]] const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetByteLength();
}

uint32_t TypedArrayRef::ByteOffset([[maybe_unused]] const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetByteOffset();
}

uint32_t TypedArrayRef::ArrayLength([[maybe_unused]] const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetArrayLength();
}

Local<ArrayBufferRef> TypedArrayRef::GetArrayBuffer(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSTypedArray> typeArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typeArray, ERROR);
    JSHandle<JSTaggedValue> arrayBuffer(thread, JSTypedArray::GetOffHeapBuffer(thread, typeArray));
    return JSNApiHelper::ToLocal<ArrayBufferRef>(arrayBuffer);
}

// ----------------------------------- FunctionRef --------------------------------------
Local<FunctionRef> FunctionRef::New(EcmaVM *vm, FunctionCallback nativeFunc,
    Deleter deleter, void *data, bool callNapi, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current(factory->NewJSFunction(env, reinterpret_cast<void *>(Callback::RegisterCallback)));
    current->SetFunctionExtraInfo(thread, reinterpret_cast<void *>(nativeFunc), deleter, data, nativeBindingsize);
    current->SetCallNapi(callNapi);
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

Local<FunctionRef> FunctionRef::New(EcmaVM *vm, InternalFunctionCallback nativeFunc,
    Deleter deleter, void *data, bool callNapi, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current(factory->NewJSFunction(env, reinterpret_cast<void *>(nativeFunc)));
    current->SetFunctionExtraInfo(thread, nullptr, deleter, data, nativeBindingsize);
    current->SetCallNapi(callNapi);
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

static void InitClassFunction(EcmaVM *vm, JSHandle<JSFunction> &func, bool callNapi)
{
    CROSS_THREAD_CHECK(vm);
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> accessor = globalConst->GetHandledFunctionPrototypeAccessor();
    func->SetPropertyInlinedProps(thread, JSFunction::CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX,
                                  accessor.GetTaggedValue());
    accessor = globalConst->GetHandledFunctionLengthAccessor();
    func->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX,
                                  accessor.GetTaggedValue());
    JSHandle<JSObject> clsPrototype = JSFunction::NewJSFunctionPrototype(thread, func);
    clsPrototype->GetClass()->SetClassPrototype(true);
    func->SetClassConstructor(true);
    JSHandle<JSTaggedValue> parent = env->GetFunctionPrototype();
    JSObject::SetPrototype(thread, JSHandle<JSObject>::Cast(func), parent);
    func->SetHomeObject(thread, clsPrototype);
    func->SetCallNapi(callNapi);
}

Local<FunctionRef> FunctionRef::NewClassFunction(EcmaVM *vm, FunctionCallback nativeFunc,
    Deleter deleter, void *data, bool callNapi, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutName());
    JSHandle<JSFunction> current =
        factory->NewJSFunctionByHClass(reinterpret_cast<void *>(Callback::RegisterCallback),
        hclass, ecmascript::FunctionKind::CLASS_CONSTRUCTOR);
    InitClassFunction(vm, current, callNapi);
    current->SetFunctionExtraInfo(thread, reinterpret_cast<void *>(nativeFunc), deleter, data, nativeBindingsize);
    Local<FunctionRef> result = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
    return scope.Escape(result);
}

Local<FunctionRef> FunctionRef::NewClassFunction(EcmaVM *vm, InternalFunctionCallback nativeFunc,
    Deleter deleter, void *data, bool callNapi, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutName());
    JSHandle<JSFunction> current =
        factory->NewJSFunctionByHClass(reinterpret_cast<void *>(nativeFunc),
        hclass, ecmascript::FunctionKind::CLASS_CONSTRUCTOR);
    InitClassFunction(vm, current, callNapi);
    current->SetFunctionExtraInfo(thread, nullptr, deleter, data, nativeBindingsize);
    Local<FunctionRef> result = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
    return scope.Escape(result);
}

Local<JSValueRef> FunctionRef::Call(const EcmaVM *vm, Local<JSValueRef> thisObj,
    const Local<JSValueRef> argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    FunctionCallScope callScope(EcmaVM::ConstCast(vm));
    if (!IsFunction()) {
        return JSValueRef::Undefined(vm);
    }
    vm->GetJsDebuggerManager()->ClearSingleStepper();
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(func, ERROR);
    JSHandle<JSTaggedValue> thisValue = JSNApiHelper::ToJSHandle(thisObj);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisValue, undefined, length);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    for (int32_t i = 0; i < length; i++) {
        JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(argv[i]);
        info->SetCallArg(i, arg.GetTaggedValue());
    }
    JSTaggedValue result = JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> resultValue(thread, result);

    vm->GetHeap()->ClearKeptObjects();
    vm->GetJsDebuggerManager()->NotifyReturnNative();
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(resultValue));
}

JSValueRef* FunctionRef::CallForNapi(const EcmaVM *vm, JSValueRef *thisObj,
    JSValueRef *const argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, *JSValueRef::Undefined(vm));
    JSTaggedValue result;
    FunctionCallScope callScope(EcmaVM::ConstCast(vm));
    ASSERT(IsFunction()); // IsFunction check has been done in napi.
    {
        ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
        LocalScope scope(vm);
        ecmascript::tooling::JsDebuggerManager *dm = vm->GetJsDebuggerManager();
        if (dm->IsDebugApp()) {
            dm->ClearSingleStepper();
        }
        JSTaggedValue func = *reinterpret_cast<JSTaggedValue *>(this);
        JSTaggedValue undefined = thread->GlobalConstants()->GetUndefined();
        JSTaggedValue thisValue = undefined;
        if (thisObj != nullptr) {
            thisValue = *reinterpret_cast<JSTaggedValue *>(thisObj);
        }
        EcmaRuntimeCallInfo *info =
            ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, thisValue, undefined, length);
        RETURN_VALUE_IF_ABRUPT(thread, *JSValueRef::Undefined(vm));
        for (int32_t i = 0; i < length; i++) {
            JSTaggedValue arg =
                argv[i] == nullptr ? JSTaggedValue::Undefined() : JSNApiHelper::ToJSTaggedValue(argv[i]);
            info->SetCallArg(i, arg);
        }
        if (thread->IsAsmInterpreter()) {
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread,
                reinterpret_cast<JSValueRef *>(JSHandle<JSTaggedValue>(thread, thread->GetException()).GetAddress()));
            STACK_LIMIT_CHECK(thread,
                reinterpret_cast<JSValueRef *>(JSHandle<JSTaggedValue>(thread, thread->GetException()).GetAddress()));
            auto *hclass = func.GetTaggedObject()->GetClass();
            if (hclass->IsClassConstructor()) {
                RETURN_STACK_BEFORE_THROW_IF_ASM(thread);
                THROW_TYPE_ERROR_AND_RETURN(thread, "class constructor cannot call",
                    reinterpret_cast<JSValueRef *>(JSHandle<JSTaggedValue>(thread, undefined).GetAddress()));
            }
            result = ecmascript::InterpreterAssembly::Execute(info);
        } else {
            result = JSFunction::Call(info);
        }
        RETURN_VALUE_IF_ABRUPT(thread, *JSValueRef::Undefined(vm));
        vm->GetHeap()->ClearKeptObjects();
        if (dm->IsMixedDebugEnabled()) {
            dm->NotifyReturnNative();
        }
    }
    JSHandle<JSTaggedValue> resultValue(thread, result);
    return reinterpret_cast<JSValueRef *>(resultValue.GetAddress());
}

Local<JSValueRef> FunctionRef::Constructor(const EcmaVM *vm,
    const Local<JSValueRef> argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    FunctionCallScope callScope(EcmaVM::ConstCast(vm));
    if (!IsFunction()) {
        return JSValueRef::Undefined(vm);
    }
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(func, ERROR);
    JSHandle<JSTaggedValue> newTarget = func;
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, undefined, newTarget, length);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    for (int32_t i = 0; i < length; i++) {
        JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(argv[i]);
        info->SetCallArg(i, arg.GetTaggedValue());
    }
    JSTaggedValue result = JSFunction::Construct(info);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> resultValue(thread, result);
    return JSNApiHelper::ToLocal<JSValueRef>(resultValue);
}

JSValueRef* FunctionRef::ConstructorOptimize(const EcmaVM *vm,
    JSValueRef* argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, *JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue result;
    FunctionCallScope callScope(EcmaVM::ConstCast(vm));
    ASSERT(IsFunction()); // IsFunction check has been done in napi.
    {
        LocalScope scope(vm);
        JSTaggedValue func = *reinterpret_cast<JSTaggedValue*>(this);
        JSTaggedValue newTarget = func;
        JSTaggedValue undefined = thread->GlobalConstants()->GetUndefined();
        EcmaRuntimeCallInfo *info =
            ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, undefined, newTarget, length);
        RETURN_VALUE_IF_ABRUPT(thread, *JSValueRef::Undefined(vm));
        for (int32_t i = 0; i < length; ++i) {
            JSTaggedValue arg =
                argv[i] == nullptr ? JSTaggedValue::Undefined() : JSNApiHelper::ToJSTaggedValue(argv[i]);
            info->SetCallArg(i, arg);
        }
        result = JSFunction::ConstructInternal(info);
        RETURN_VALUE_IF_ABRUPT(thread, *JSValueRef::Undefined(vm));
    }
    JSHandle<JSTaggedValue> resultValue(thread, result);
    return reinterpret_cast<JSValueRef*>(resultValue.GetAddress());
}

Local<JSValueRef> FunctionRef::GetFunctionPrototype(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> func = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(func, FATAL);
    JSHandle<JSTaggedValue> prototype(thread, JSHandle<JSFunction>(func)->GetFunctionPrototype());
    return JSNApiHelper::ToLocal<JSValueRef>(prototype);
}

bool FunctionRef::Inherit(const EcmaVM *vm, Local<FunctionRef> parent)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> parentValue = JSNApiHelper::ToJSHandle(parent);
    JSHandle<JSObject> parentHandle = JSHandle<JSObject>::Cast(parentValue);
    JSHandle<JSObject> thisHandle = JSHandle<JSObject>::Cast(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(thisHandle, ERROR);
    // Set this.__proto__ to parent
    bool res = JSObject::SetPrototype(thread, thisHandle, parentValue);
    if (!res) {
        return false;
    }
    // Set this.Prototype.__proto__ to parent.Prototype
    JSHandle<JSTaggedValue> parentProtoType(thread, JSFunction::PrototypeGetter(thread, parentHandle));
    JSHandle<JSTaggedValue> thisProtoType(thread, JSFunction::PrototypeGetter(thread, thisHandle));
    return JSObject::SetPrototype(thread, JSHandle<JSObject>::Cast(thisProtoType), parentProtoType);
}

void FunctionRef::SetName(const EcmaVM *vm, Local<StringRef> name)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    JSFunction *func = JSFunction::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject());
    JSTaggedValue key = JSNApiHelper::ToJSTaggedValue(*name);
    JSFunction::SetFunctionNameNoPrefix(thread, func, key);
}

Local<StringRef> FunctionRef::GetName(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSFunctionBase> func(thread, JSNApiHelper::ToJSTaggedValue(this));
    JSHandle<JSTaggedValue> name = JSFunctionBase::GetFunctionName(thread, func);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return scope.Escape(JSNApiHelper::ToLocal<StringRef>(name));
}

Local<StringRef> FunctionRef::GetSourceCode(const EcmaVM *vm, int lineNumber)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSFunctionBase> func(thread, JSNApiHelper::ToJSTaggedValue(this));
    JSHandle<Method> method(thread, func->GetMethod());
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    DebugInfoExtractor *debugExtractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    ecmascript::CString entry = JSPandaFile::ENTRY_FUNCTION_NAME;
    if (!jsPandaFile->IsBundlePack()) {
        JSFunction *function = JSFunction::Cast(func.GetTaggedValue().GetTaggedObject());
        JSTaggedValue recordName = function->GetRecordName();
        ASSERT(!recordName.IsHole());
        entry = ConvertToString(recordName);
    }

    uint32_t mainMethodIndex = jsPandaFile->GetMainMethodIndex(entry);
    JSMutableHandle<JSTaggedValue> sourceCodeHandle(thread, BuiltinsBase::GetTaggedString(thread, ""));
    if (mainMethodIndex == 0) {
        return scope.Escape(JSNApiHelper::ToLocal<StringRef>(sourceCodeHandle));
    }

    const std::string &allSourceCode = debugExtractor->GetSourceCode(panda_file::File::EntityId(mainMethodIndex));
    std::string sourceCode = StringHelper::GetSpecifiedLine(allSourceCode, lineNumber);
    uint32_t codeLen = sourceCode.length();
    if (codeLen == 0) {
        return scope.Escape(JSNApiHelper::ToLocal<StringRef>(sourceCodeHandle));
    }

    if (sourceCode[codeLen - 1] == '\r') {
        sourceCode = sourceCode.substr(0, codeLen - 1);
    }
    sourceCodeHandle.Update(BuiltinsBase::GetTaggedString(thread, sourceCode.c_str()));
    return scope.Escape(JSNApiHelper::ToLocal<StringRef>(sourceCodeHandle));
}

bool FunctionRef::IsNative(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSFunctionBase> func(thread, JSNApiHelper::ToJSTaggedValue(this));
    JSHandle<Method> method(thread, func->GetMethod());
    return method->IsNativeWithCallField();
}

void FunctionRef::SetData(const EcmaVM *vm, void *data, Deleter deleter, bool callNapi)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> funcValue = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSFunction> function(funcValue);
    function->SetFunctionExtraInfo(thread, nullptr, deleter, data, 0);
    function->SetCallNapi(callNapi);
}

void* FunctionRef::GetData(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, nullptr);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> funcValue = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSFunction> function(funcValue);
    if (!function->IsCallNapi()) {
        return nullptr;
    }
    JSTaggedValue extraInfoValue = function->GetFunctionExtraInfo();
    if (!extraInfoValue.IsNativePointer()) {
        return nullptr;
    }
    JSHandle<JSNativePointer> extraInfo(thread, extraInfoValue);
    return extraInfo->GetData();
}

// ----------------------------------- ArrayRef ----------------------------------------
Local<ArrayRef> ArrayRef::New(const EcmaVM *vm, uint32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedNumber arrayLen(length);
    JSHandle<JSTaggedValue> array = JSArray::ArrayCreate(thread, arrayLen);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<ArrayRef>(array);
}

uint32_t ArrayRef::Length([[maybe_unused]] const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    return JSArray::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetArrayLength();
}

Local<JSValueRef> ArrayRef::GetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(obj);
    JSHandle<JSTaggedValue> result = JSArray::FastGetPropertyByValue(thread, object, index);
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

bool ArrayRef::SetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> objectHandle = JSNApiHelper::ToJSHandle(obj);
    JSHandle<JSTaggedValue> valueHandle = JSNApiHelper::ToJSHandle(value);
    return JSArray::FastSetPropertyByValue(thread, objectHandle, index, valueHandle);
}

// ---------------------------------- Error ---------------------------------------
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXCEPTION_ERROR_NEW(name, type)                                                     \
    Local<JSValueRef> Exception::name(const EcmaVM *vm, Local<StringRef> message)           \
    {                                                                                       \
        CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));        \
        ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());                     \
        ObjectFactory *factory = vm->GetFactory();                                          \
                                                                                            \
        JSHandle<EcmaString> messageValue(JSNApiHelper::ToJSHandle(message));               \
        JSHandle<JSTaggedValue> result(factory->NewJSError(ErrorType::type, messageValue)); \
        return JSNApiHelper::ToLocal<JSValueRef>(result);                                   \
    }

EXCEPTION_ERROR_ALL(EXCEPTION_ERROR_NEW)

#undef EXCEPTION_ERROR_NEW
// ---------------------------------- Error ---------------------------------------

// ---------------------------------- FunctionCallScope ---------------------------------------
FunctionCallScope::FunctionCallScope(EcmaVM *vm) : vm_(vm)
{
    vm_->IncreaseCallDepth();
}

FunctionCallScope::~FunctionCallScope()
{
    vm_->DecreaseCallDepth();
    if (vm_->IsTopLevelCallDepth()) {
        JSThread *thread = vm_->GetJSThread();
        ecmascript::ThreadManagedScope managedScope(vm_->GetJSThread());
        thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    }
}

// -------------------------------------  JSExecutionScope ------------------------------
JSExecutionScope::JSExecutionScope([[maybe_unused]] const EcmaVM *vm)
{
}

JSExecutionScope::~JSExecutionScope()
{
    lastCurrentThread_ = nullptr;
    isRevert_ = false;
}

// ------------------------------------ JsiRuntimeCallInfo -----------------------------------------------
void *JsiRuntimeCallInfo::GetData()
{
    JSHandle<JSTaggedValue> constructor = BuiltinsBase::GetConstructor(reinterpret_cast<EcmaRuntimeCallInfo *>(this));
    if (!constructor->IsJSFunction()) {
        return nullptr;
    }
    JSHandle<JSFunction> function(constructor);
    JSTaggedValue extraInfoValue = function->GetFunctionExtraInfo();
    if (!extraInfoValue.IsJSNativePointer()) {
        return nullptr;
    }
    JSHandle<JSNativePointer> extraInfo(thread_, extraInfoValue);
    return extraInfo->GetData();
}

EcmaVM *JsiRuntimeCallInfo::GetVM() const
{
    return thread_->GetEcmaVM();
}

// ---------------------------------------JSNApi-------------------------------------------
PatchErrorCode JSNApi::LoadPatch(EcmaVM *vm, const std::string &patchFileName, const std::string &baseFileName)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, PatchErrorCode::INTERNAL_ERROR);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    return quickFixManager->LoadPatch(thread, patchFileName, baseFileName);
}

PatchErrorCode JSNApi::LoadPatch(EcmaVM *vm,
                                 const std::string &patchFileName, const void *patchBuffer, size_t patchSize,
                                 const std::string &baseFileName, const void *baseBuffer, size_t baseSize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, PatchErrorCode::INTERNAL_ERROR);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    return quickFixManager->LoadPatch(
        thread, patchFileName, patchBuffer, patchSize, baseFileName, baseBuffer, baseSize);
}

PatchErrorCode JSNApi::UnloadPatch(EcmaVM *vm, const std::string &patchFileName)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, PatchErrorCode::INTERNAL_ERROR);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    return quickFixManager->UnloadPatch(thread, patchFileName);
}

/*
 * check whether the exception is caused by quickfix methods.
 */
bool JSNApi::IsQuickFixCausedException(EcmaVM *vm, Local<ObjectRef> exception, const std::string &patchFileName)
{
    if (exception.IsEmpty()) {
        return false;
    }
    CROSS_THREAD_CHECK(vm);
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    JSHandle<JSTaggedValue> exceptionInfo = JSNApiHelper::ToJSHandle(exception);
    return quickFixManager->IsQuickFixCausedException(thread, exceptionInfo, patchFileName);
}

/*
 * register quickfix query function.
 */
void JSNApi::RegisterQuickFixQueryFunc(EcmaVM *vm, std::function<bool(std::string baseFileName,
                        std::string &patchFileName,
                        void **patchBuffer,
                        size_t &patchSize)> callBack)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    quickFixManager->RegisterQuickFixQueryFunc(callBack);
}

bool JSNApi::IsBundle(EcmaVM *vm)
{
    return vm->IsBundlePack();
}

void JSNApi::SetBundle(EcmaVM *vm, bool value)
{
    vm->SetIsBundlePack(value);
}

void JSNApi::SetModuleInfo(EcmaVM *vm, const std::string &assetPath, const std::string &entryPoint)
{
    SetAssetPath(vm, assetPath);
    size_t pos = entryPoint.find_first_of("/");
    SetBundleName(vm, entryPoint.substr(0, pos));
    std::string subStr = entryPoint.substr(pos + 1); // 1: remove /
    pos = subStr.find_first_of("/");
    SetModuleName(vm, subStr.substr(0, pos));
}

// note: The function SetAssetPath is a generic interface for previewing and physical machines.
void JSNApi::SetAssetPath(EcmaVM *vm, const std::string &assetPath)
{
    ecmascript::CString path = assetPath.c_str();
    vm->SetAssetPath(path);
}

void JSNApi::SetLoop(EcmaVM *vm, void *loop)
{
    vm->SetLoop(loop);
}

std::string JSNApi::GetAssetPath(EcmaVM *vm)
{
    return vm->GetAssetPath().c_str();
}

void JSNApi::SetMockModuleList(EcmaVM *vm, const std::map<std::string, std::string> &list)
{
    vm->SetMockModuleList(list);
}

void JSNApi::SetHmsModuleList(EcmaVM *vm, const std::vector<panda::HmsMap> &list)
{
    vm->SetHmsModuleList(list);
}

bool JSNApi::InitForConcurrentThread(EcmaVM *vm, ConcurrentCallback cb, void *data)
{
    vm->SetConcurrentCallback(cb, data);

    return true;
}

bool JSNApi::InitForConcurrentFunction(EcmaVM *vm, Local<JSValueRef> function, void *taskInfo)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> funcVal = JSNApiHelper::ToJSHandle(function);
    JSHandle<JSFunction> transFunc = JSHandle<JSFunction>::Cast(funcVal);
    if (transFunc->GetFunctionKind() != ecmascript::FunctionKind::CONCURRENT_FUNCTION) {
        LOG_ECMA(ERROR) << "Function is not concurrent";
        return false;
    }
    transFunc->SetFunctionExtraInfo(thread, nullptr, nullptr, taskInfo);
    transFunc->SetCallNapi(false);
    return true;
}

void* JSNApi::GetCurrentTaskInfo(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, nullptr);
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    ecmascript::FrameIterator it(current, thread);
    for (; !it.Done(); it.Advance<ecmascript::GCVisitedFlag::VISITED>()) {
        if (!it.IsJSFrame()) {
            continue;
        }
        auto method = it.CheckAndGetMethod();
        if (method == nullptr || method->IsNativeWithCallField() ||
            method->GetFunctionKind() != ecmascript::FunctionKind::CONCURRENT_FUNCTION) {
            continue;
        }
        auto functionObj = it.GetFunction();
        JSHandle<JSFunction> function(thread, functionObj);
        JSTaggedValue extraInfoValue = function->GetFunctionExtraInfo();
        if (!extraInfoValue.IsJSNativePointer()) {
            LOG_ECMA(DEBUG) << "Concurrent function donnot have taskInfo";
            continue;
        }
        JSHandle<JSNativePointer> extraInfo(thread, extraInfoValue);
        return extraInfo->GetData();
    }
    LOG_ECMA(ERROR) << "TaskInfo is nullptr";
    return nullptr;
}

void JSNApi::SetBundleName(EcmaVM *vm, const std::string &bundleName)
{
    ecmascript::CString name = bundleName.c_str();
    vm->SetBundleName(name);
}

std::string JSNApi::GetBundleName(EcmaVM *vm)
{
    return vm->GetBundleName().c_str();
}

void JSNApi::SetModuleName(EcmaVM *vm, const std::string &moduleName)
{
    ecmascript::CString name = moduleName.c_str();
    ecmascript::pgo::PGOProfilerManager::GetInstance()->SetModuleName(moduleName);
    vm->SetModuleName(name);
}

std::string JSNApi::GetModuleName(EcmaVM *vm)
{
    return vm->GetModuleName().c_str();
}

std::pair<std::string, std::string> JSNApi::GetCurrentModuleInfo(EcmaVM *vm, bool needRecordName)
{
    return vm->GetCurrentModuleInfo(needRecordName);
}

// Enable cross thread execution.
void JSNApi::AllowCrossThreadExecution(EcmaVM *vm)
{
    LOG_ECMA(WARN) << "enable cross thread execution";
    vm->GetAssociatedJSThread()->EnableCrossThreadExecution();
}

void JSNApi::SynchronizVMInfo(EcmaVM *vm, const EcmaVM *hostVM)
{
    vm->SetBundleName(hostVM->GetBundleName());
    vm->SetModuleName(hostVM->GetModuleName());
    vm->SetAssetPath(hostVM->GetAssetPath());
    vm->SetIsBundlePack(hostVM->IsBundlePack());

    ecmascript::ModuleManager *vmModuleManager =
        vm->GetAssociatedJSThread()->GetCurrentEcmaContext()->GetModuleManager();
    ecmascript::ModuleManager *hostVMModuleManager =
        hostVM->GetAssociatedJSThread()->GetCurrentEcmaContext()->GetModuleManager();
    vmModuleManager->SetExecuteMode(hostVMModuleManager->GetExecuteMode());
    vm->SetResolveBufferCallback(hostVM->GetResolveBufferCallback());
}

bool JSNApi::IsProfiling(EcmaVM *vm)
{
    return vm->GetProfilerState();
}

void JSNApi::SetProfilerState(const EcmaVM *vm, bool value)
{
    const_cast<EcmaVM*>(vm)->SetProfilerState(value);
}

void JSNApi::SetSourceMapTranslateCallback(EcmaVM *vm, SourceMapTranslateCallback callback)
{
    vm->SetSourceMapTranslateCallback(callback);
}

void JSNApi::SetSourceMapCallback(EcmaVM *vm, SourceMapCallback callback)
{
    vm->SetSourceMapCallback(callback);
}

void JSNApi::GetStackBeforeCallNapiSuccess([[maybe_unused]] EcmaVM *vm,
                                           [[maybe_unused]] bool &getStackBeforeCallNapiSuccess)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    JSThread *thread = vm->GetJSThread();
    if (thread->GetIsProfiling()) {
        getStackBeforeCallNapiSuccess = vm->GetProfiler()->GetStackBeforeCallNapi(thread);
    }
#endif
}

void JSNApi::GetStackAfterCallNapi([[maybe_unused]] EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    JSThread *thread = vm->GetJSThread();
    if (thread->GetIsProfiling()) {
        vm->GetProfiler()->GetStackAfterCallNapi(thread);
    }
#endif
}

bool JSNApi::CheckSecureMem(uintptr_t mem)
{
    static bool hasOpen = false;
    static uintptr_t secureMemStart = 0;
    static uintptr_t secureMemEnd = 0;
    if (!hasOpen) {
        char procPath[XPM_PROC_LENGTH] = {0};
        pid_t pid = getpid();
        LOG_ECMA(DEBUG) << "Check secure memory in : " << pid << " with mem: " << std::hex << mem;
        if (sprintf_s(procPath, XPM_PROC_LENGTH, "%s%d%s", XPM_PROC_PREFIX, pid, XPM_PROC_SUFFIX) <= 0) {
            LOG_ECMA(ERROR) << "sprintf proc path failed";
            return false;
        }
        int fd = open(procPath, O_RDONLY);
        if (fd < 0) {
            LOG_ECMA(ERROR) << "Can not open xpm proc file, do not check secure memory anymore.";
            // No verification is performed when a file fails to be opened.
            hasOpen = true;
            return true;
        }

        char xpmValidateRegion[XPM_PROC_LENGTH] = {0};
        int ret = read(fd, xpmValidateRegion, sizeof(xpmValidateRegion));
        if (ret <= 0) {
            LOG_ECMA(ERROR) << "Read xpm proc file failed";
            close(fd);
            return false;
        }
        close(fd);

        if (sscanf_s(xpmValidateRegion, "%lx-%lx", &secureMemStart, &secureMemEnd) <= 0) {
            LOG_ECMA(ERROR) << "sscanf_s xpm validate region failed";
            return false;
        }
        // The check is not performed when the file is already opened.
        hasOpen = true;
    }

    // xpm proc does not exist, the read value is 0, and the check is not performed.
    if (secureMemStart == 0 && secureMemEnd == 0) {
        LOG_ECMA(ERROR) << "Secure memory check: xpm proc does not exist, do not check secure memory anymore.";
        return true;
    }

    LOG_ECMA(DEBUG) << "Secure memory check in memory start: " << std::hex << secureMemStart
                   << " memory end: " << secureMemEnd;
    if (mem < secureMemStart || mem >= secureMemEnd) {
        LOG_ECMA(ERROR) << "Secure memory check failed, mem out of secure memory, mem: " << std::hex << mem;
        return false;
    }
    return true;
}

EcmaVM *JSNApi::CreateJSVM(const RuntimeOption &option)
{
    JSRuntimeOptions runtimeOptions;
    runtimeOptions.SetArkProperties(option.GetArkProperties());
    runtimeOptions.SetArkBundleName(option.GetArkBundleName());
    runtimeOptions.SetLongPauseTime(option.GetLongPauseTime());
    runtimeOptions.SetGcThreadNum(option.GetGcThreadNum());
    runtimeOptions.SetIsWorker(option.GetIsWorker());
    // Mem
    runtimeOptions.SetHeapSizeLimit(option.GetGcPoolSize());
// Disable the asm-interpreter of ark-engine for ios-platform temporarily.
#if !defined(PANDA_TARGET_IOS)
    // asmInterpreter
    runtimeOptions.SetEnableAsmInterpreter(option.GetEnableAsmInterpreter());
#else
    runtimeOptions.SetEnableAsmInterpreter(false);
#endif
    runtimeOptions.SetEnableBuiltinsLazy(option.GetEnableBuiltinsLazy());
    runtimeOptions.SetAsmOpcodeDisableRange(option.GetAsmOpcodeDisableRange());
    // aot
    runtimeOptions.SetEnableAOT(option.GetEnableAOT());
    runtimeOptions.SetEnablePGOProfiler(option.GetEnableProfile());
    runtimeOptions.SetPGOProfilerPath(option.GetProfileDir());

    // Dfx
    runtimeOptions.SetLogLevel(Log::LevelToString(Log::ConvertFromRuntime(option.GetLogLevel())));
    runtimeOptions.SetEnableArkTools(option.GetEnableArkTools());
    return CreateEcmaVM(runtimeOptions);
}

EcmaContext *JSNApi::CreateJSContext(EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    ecmascript::ThreadManagedScope managedScope(thread);
    return EcmaContext::CreateAndInitialize(thread);
}

void JSNApi::SwitchCurrentContext(EcmaVM *vm, EcmaContext *context)
{
    JSThread *thread = vm->GetJSThread();
    ecmascript::ThreadManagedScope managedScope(thread);
    thread->SwitchCurrentContext(context);
}

void JSNApi::DestroyJSContext(EcmaVM *vm, EcmaContext *context)
{
    JSThread *thread = vm->GetJSThread();
    ecmascript::ThreadManagedScope managedScope(thread);
    EcmaContext::CheckAndDestroy(thread, context);
}

EcmaVM *JSNApi::CreateEcmaVM(const JSRuntimeOptions &options)
{
    return EcmaVM::Create(options);
}

void JSNApi::DestroyJSVM(EcmaVM *ecmaVm)
{
    if (UNLIKELY(ecmaVm == nullptr)) {
        return;
    }
    ecmaVm->GetJSThread()->ManagedCodeBegin();
    auto &config = ecmaVm->GetEcmaParamConfiguration();
    MemMapAllocator::GetInstance()->DecreaseReserved(config.GetMaxHeapSize());
    EcmaVM::Destroy(ecmaVm);
}

void JSNApi::RegisterUncatchableErrorHandler(EcmaVM *ecmaVm, const UncatchableErrorHandler &handler)
{
    ecmaVm->RegisterUncatchableErrorHandler(handler);
}

void JSNApi::TriggerGC(const EcmaVM *vm, TRIGGER_GC_TYPE gcType)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (thread != nullptr && vm->IsInitialized()) {
        switch (gcType) {
            case TRIGGER_GC_TYPE::SEMI_GC:
                vm->CollectGarbage(vm->GetHeap()->SelectGCType(), ecmascript::GCReason::EXTERNAL_TRIGGER);
                break;
            case TRIGGER_GC_TYPE::OLD_GC:
                vm->CollectGarbage(ecmascript::TriggerGCType::OLD_GC, ecmascript::GCReason::EXTERNAL_TRIGGER);
                break;
            case TRIGGER_GC_TYPE::FULL_GC:
                vm->CollectGarbage(ecmascript::TriggerGCType::FULL_GC, ecmascript::GCReason::EXTERNAL_TRIGGER);
                break;
            default:
                break;
        }
    }
}

void JSNApi::ThrowException(const EcmaVM *vm, Local<JSValueRef> error)
{
    auto thread = vm->GetJSThread();
    ecmascript::ThreadManagedScope managedScope(thread);
    if (thread->HasPendingException()) {
        LOG_ECMA(ERROR) << "An exception has already occurred before, keep old exception here.";
        return;
    }
    thread->SetException(JSNApiHelper::ToJSTaggedValue(*error));
}

void JSNApi::PrintExceptionInfo(const EcmaVM *vm)
{
    JSThread* thread = vm->GetJSThread();
    ecmascript::ThreadManagedScope managedScope(thread);
    [[maybe_unused]] ecmascript::EcmaHandleScope handleScope(thread);

    if (!HasPendingException(vm)) {
        return;
    }
    Local<ObjectRef> exception = GetAndClearUncaughtException(vm);
    JSHandle<JSTaggedValue> exceptionHandle = JSNApiHelper::ToJSHandle(exception);
    if (exceptionHandle->IsJSError()) {
        vm->PrintJSErrorInfo(exceptionHandle);
        ThrowException(vm, exception);
        return;
    }
    JSHandle<EcmaString> result = JSTaggedValue::ToString(thread, exceptionHandle);
    ecmascript::CString string = ConvertToString(*result);
    LOG_ECMA(ERROR) << string;
    ThrowException(vm, exception);
}

// for previewer, cross platform and testcase debugger
bool JSNApi::StartDebugger([[maybe_unused]] EcmaVM *vm, [[maybe_unused]] const DebugOption &option,
                           [[maybe_unused]] int32_t instanceId,
                           [[maybe_unused]] const DebuggerPostTask &debuggerPostTask)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
#if !defined(PANDA_TARGET_IOS)
    LOG_ECMA(INFO) << "JSNApi::StartDebugger, isDebugMode = " << option.isDebugMode
        << ", port = " << option.port << ", instanceId = " << instanceId;
    if (vm == nullptr) {
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    const auto &handler = vm->GetJsDebuggerManager()->GetDebugLibraryHandle();
    if (handler.IsValid()) {
        return false;
    }

    if (option.libraryPath == nullptr) {
        return false;
    }
    auto handle = panda::os::library_loader::Load(std::string(option.libraryPath));
    if (!handle) {
        LOG_ECMA(ERROR) << "[StartDebugger] Load library fail: " << option.libraryPath << " " << errno;
        return false;
    }

    using StartDebugger = bool (*)(
        const std::string &, EcmaVM *, bool, int32_t, const DebuggerPostTask &, int);

    auto sym = panda::os::library_loader::ResolveSymbol(handle.Value(), "StartDebug");
    if (!sym) {
        LOG_ECMA(ERROR) << "[StartDebugger] Resolve symbol fail: " << sym.Error().ToString();
        return false;
    }

    vm->GetJsDebuggerManager()->SetDebugMode(option.isDebugMode);
    vm->GetJsDebuggerManager()->SetIsDebugApp(true);
    vm->GetJsDebuggerManager()->SetDebugLibraryHandle(std::move(handle.Value()));
    bool ret = reinterpret_cast<StartDebugger>(sym.Value())(
        "PandaDebugger", vm, option.isDebugMode, instanceId, debuggerPostTask, option.port);
    if (!ret) {
        // Reset the config
        vm->GetJsDebuggerManager()->SetDebugMode(false);
        panda::os::library_loader::LibraryHandle libraryHandle(nullptr);
        vm->GetJsDebuggerManager()->SetDebugLibraryHandle(std::move(libraryHandle));
    }
    return ret;
#else
    if (vm == nullptr) {
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    vm->GetJsDebuggerManager()->SetDebugMode(option.isDebugMode);
    bool ret = OHOS::ArkCompiler::Toolchain::StartDebug(
        DEBUGGER_NAME, vm, option.isDebugMode, instanceId, debuggerPostTask, option.port);
    if (!ret) {
        // Reset the config
        vm->GetJsDebuggerManager()->SetDebugMode(false);
    }
    return ret;
#endif // PANDA_TARGET_IOS
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

// for old process.
bool JSNApi::StartDebuggerForOldProcess([[maybe_unused]] EcmaVM *vm, [[maybe_unused]] const DebugOption &option,
                                        [[maybe_unused]] int32_t instanceId,
                                        [[maybe_unused]] const DebuggerPostTask &debuggerPostTask)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
#if !defined(PANDA_TARGET_IOS)
    LOG_ECMA(INFO) << "JSNApi::StartDebuggerForOldProcess, isDebugMode = " << option.isDebugMode
        << ", instanceId = " << instanceId;
    if (vm == nullptr) {
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    const auto &handle = vm->GetJsDebuggerManager()->GetDebugLibraryHandle();
    if (!handle.IsValid()) {
        LOG_ECMA(ERROR) << "[StartDebuggerForOldProcess] Get library handle fail: " << option.libraryPath;
        return false;
    }

    using StartDebug = bool (*)(
        const std::string &, EcmaVM *, bool, int32_t, const DebuggerPostTask &, int);

    auto sym = panda::os::library_loader::ResolveSymbol(handle, "StartDebug");
    if (!sym) {
        LOG_ECMA(ERROR) << "[StartDebuggerForOldProcess] Resolve symbol fail: " << sym.Error().ToString();
        return false;
    }

    bool ret = reinterpret_cast<StartDebug>(sym.Value())(
        "PandaDebugger", vm, option.isDebugMode, instanceId, debuggerPostTask, option.port);
    if (!ret) {
        // Reset the config
        vm->GetJsDebuggerManager()->SetDebugMode(false);
        panda::os::library_loader::LibraryHandle libraryHandle(nullptr);
        vm->GetJsDebuggerManager()->SetDebugLibraryHandle(std::move(libraryHandle));
    }
    return ret;
#else
    if (vm == nullptr) {
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    vm->GetJsDebuggerManager()->SetDebugMode(option.isDebugMode);
    bool ret = OHOS::ArkCompiler::Toolchain::StartDebug(
        DEBUGGER_NAME, vm, option.isDebugMode, instanceId, debuggerPostTask, option.port);
    if (!ret) {
        // Reset the config
        vm->GetJsDebuggerManager()->SetDebugMode(false);
    }
    return ret;
#endif // PANDA_TARGET_IOS
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

// for socketpair process in ohos platform.
bool JSNApi::StartDebuggerForSocketPair([[maybe_unused]] int tid, [[maybe_unused]] int socketfd)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    LOG_ECMA(INFO) << "JSNApi::StartDebuggerForSocketPair, tid = " << tid << ", socketfd = " << socketfd;
    JsDebuggerManager *jsDebuggerManager = JsDebuggerManager::GetJsDebuggerManager(tid);
    if (jsDebuggerManager == nullptr) {
        return false;
    }
    const auto &handle = jsDebuggerManager->GetDebugLibraryHandle();
    if (!handle.IsValid()) {
        LOG_ECMA(ERROR) << "[StartDebuggerForSocketPair] Get library handle fail";
        return false;
    }

    using StartDebugForSocketpair = bool (*)(int, int);

    auto sym = panda::os::library_loader::ResolveSymbol(handle, "StartDebugForSocketpair");
    if (!sym) {
        LOG_ECMA(ERROR) << "[StartDebuggerForSocketPair] Resolve symbol fail: " << sym.Error().ToString();
        return false;
    }

    bool ret = reinterpret_cast<StartDebugForSocketpair>(sym.Value())(tid, socketfd);
    if (!ret) {
        // Reset the config
        jsDebuggerManager->SetDebugMode(false);
        panda::os::library_loader::LibraryHandle libraryHandle(nullptr);
        jsDebuggerManager->SetDebugLibraryHandle(std::move(libraryHandle));
    }
    return ret;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

// release or debug hap : aa start
//                        aa start -D
//                        new worker
bool JSNApi::NotifyDebugMode([[maybe_unused]] int tid,
                             [[maybe_unused]] EcmaVM *vm,
                             [[maybe_unused]] const DebugOption &option,
                             [[maybe_unused]] int32_t instanceId,
                             [[maybe_unused]] const DebuggerPostTask &debuggerPostTask,
                             [[maybe_unused]] bool debugApp)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    LOG_ECMA(INFO) << "JSNApi::NotifyDebugMode, tid = " << tid << ", debugApp = " << debugApp
        << ", isDebugMode = " << option.isDebugMode << ", instanceId = " << instanceId;
    if (vm == nullptr) {
        return false;
    }

    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    JsDebuggerManager *jsDebuggerManager = vm->GetJsDebuggerManager();
    const auto &handler = jsDebuggerManager->GetDebugLibraryHandle();
    if (handler.IsValid()) {
        return false;
    }

    auto handle = panda::os::library_loader::Load(std::string(option.libraryPath));
    if (!handle) {
        LOG_ECMA(ERROR) << "[NotifyDebugMode] Load library fail: " << option.libraryPath << " " << errno;
        return false;
    }
    JsDebuggerManager::AddJsDebuggerManager(tid, jsDebuggerManager);
    jsDebuggerManager->SetDebugLibraryHandle(std::move(handle.Value()));
    jsDebuggerManager->SetDebugMode(option.isDebugMode && debugApp);
    jsDebuggerManager->SetIsDebugApp(debugApp);
    bool ret = false;
    if (debugApp) {
        ret = StartDebuggerForOldProcess(vm, option, instanceId, debuggerPostTask);
    }

    // store debugger postTask in inspector.
    using StoreDebuggerInfo = void (*)(int, EcmaVM *, const DebuggerPostTask &);
    auto symOfStoreDebuggerInfo = panda::os::library_loader::ResolveSymbol(
        jsDebuggerManager->GetDebugLibraryHandle(), "StoreDebuggerInfo");
    if (!symOfStoreDebuggerInfo) {
        LOG_ECMA(ERROR) << "[NotifyDebugMode] Resolve StoreDebuggerInfo symbol fail: " <<
            symOfStoreDebuggerInfo.Error().ToString();
        return false;
    }
    reinterpret_cast<StoreDebuggerInfo>(symOfStoreDebuggerInfo.Value())(tid, vm, debuggerPostTask);
    using InitializeDebuggerForSocketpair = bool(*)(void*);
    auto sym = panda::os::library_loader::ResolveSymbol(handler, "InitializeDebuggerForSocketpair");
    if (!sym) {
        LOG_ECMA(ERROR) << "[InitializeDebuggerForSocketpair] Resolve symbol fail: " << sym.Error().ToString();
        return false;
    }
    ret = reinterpret_cast<InitializeDebuggerForSocketpair>(sym.Value())(vm);
    if (!ret) {
    // Reset the config
        vm->GetJsDebuggerManager()->SetDebugMode(false);
        return false;
    }
    if (option.isDebugMode) {
        using WaitForDebugger = void (*)(EcmaVM *);
        auto symOfWaitForDebugger = panda::os::library_loader::ResolveSymbol(
            jsDebuggerManager->GetDebugLibraryHandle(), "WaitForDebugger");
        if (!symOfWaitForDebugger) {
            LOG_ECMA(ERROR) << "[NotifyDebugMode] Resolve symbol WaitForDebugger fail: " <<
                symOfWaitForDebugger.Error().ToString();
            return false;
        }
        reinterpret_cast<WaitForDebugger>(symOfWaitForDebugger.Value())(vm);
    }
    return ret;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

bool JSNApi::StopDebugger([[maybe_unused]] EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
#if !defined(PANDA_TARGET_IOS)
    LOG_ECMA(INFO) << "JSNApi::StopDebugger";
    if (vm == nullptr) {
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);

    const auto &handle = vm->GetJsDebuggerManager()->GetDebugLibraryHandle();

    using StopDebug = void (*)(const std::string &);

    auto sym = panda::os::library_loader::ResolveSymbol(handle, "StopDebug");
    if (!sym) {
        LOG_ECMA(ERROR) << sym.Error().ToString();
        return false;
    }

    reinterpret_cast<StopDebug>(sym.Value())("PandaDebugger");

    vm->GetJsDebuggerManager()->SetDebugMode(false);
    return true;
#else
    if (vm == nullptr) {
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);

    OHOS::ArkCompiler::Toolchain::StopDebug(DEBUGGER_NAME);
    vm->GetJsDebuggerManager()->SetDebugMode(false);
    return true;
#endif // PANDA_TARGET_IOS
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

bool JSNApi::StopDebugger([[maybe_unused]] int tid)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    LOG_ECMA(INFO) << "JSNApi::StopDebugger, tid = " << tid;
    JsDebuggerManager *jsDebuggerManager = JsDebuggerManager::GetJsDebuggerManager(tid);
    if (jsDebuggerManager == nullptr) {
        return false;
    }

    const auto &handle = jsDebuggerManager->GetDebugLibraryHandle();

    using StopOldDebug = void (*)(int, const std::string &);

    auto sym = panda::os::library_loader::ResolveSymbol(handle, "StopOldDebug");
    if (!sym) {
        LOG_ECMA(ERROR) << sym.Error().ToString();
        return false;
    }

    reinterpret_cast<StopOldDebug>(sym.Value())(tid, "PandaDebugger");

    return true;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

bool JSNApi::IsMixedDebugEnabled([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    return vm->GetJsDebuggerManager()->IsMixedDebugEnabled();
#else
    return false;
#endif
}

void JSNApi::NotifyNativeCalling([[maybe_unused]] const EcmaVM *vm, [[maybe_unused]] const void *nativeAddress)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    vm->GetJsDebuggerManager()->GetNotificationManager()->NativeCallingEvent(nativeAddress);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
#endif
}

void JSNApi::NotifyNativeReturn([[maybe_unused]] const EcmaVM *vm,  [[maybe_unused]] const void *nativeAddress)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    vm->GetJsDebuggerManager()->GetNotificationManager()->NativeReturnEvent(nativeAddress);
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
#endif
}

void JSNApi::NotifyLoadModule([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    // if load module, it needs to check whether clear singlestepper_
    vm->GetJsDebuggerManager()->ClearSingleStepper();
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
#endif
}

void JSNApi::SetDeviceDisconnectCallback(EcmaVM *vm, DeviceDisconnectCallback cb)
{
    vm->SetDeviceDisconnectCallback(cb);
}

void JSNApi::LoadAotFile(EcmaVM *vm, const std::string &moduleName)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::AnFileDataManager::GetInstance()->IsEnable()) {
        return;
    }
    std::string aotFileName;
    if (vm->GetJSOptions().WasAOTOutputFileSet()) {
        aotFileName = vm->GetJSOptions().GetAOTOutputFile();
    }  else {
        aotFileName = ecmascript::AnFileDataManager::GetInstance()->GetDir() + moduleName;
    }
    LOG_ECMA(INFO) << "start to load aot file: " << aotFileName;
    thread->GetCurrentEcmaContext()->LoadAOTFiles(aotFileName);
}

bool JSNApi::ExecuteInContext(EcmaVM *vm, const std::string &fileName, const std::string &entry, bool needUpdate)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(DEBUG) << "start to execute ark file in context: " << fileName;
    ecmascript::ThreadManagedScope scope(thread);
    EcmaContext::MountContext(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromAbcFile(thread, fileName.c_str(), entry, needUpdate)) {
        if (thread->HasPendingException()) {
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute ark file '" << fileName
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    EcmaContext::UnmountContext(thread);
    return true;
}

bool JSNApi::Execute(EcmaVM *vm, const std::string &fileName, const std::string &entry, bool needUpdate)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(DEBUG) << "start to execute ark file: " << fileName;
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromAbcFile(thread, fileName.c_str(), entry, needUpdate)) {
        if (thread->HasPendingException()) {
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute ark file '" << fileName
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    return true;
}

// The security interface needs to be modified accordingly.
bool JSNApi::Execute(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &entry,
                     const std::string &filename, bool needUpdate)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(DEBUG) << "start to execute ark buffer: " << filename;
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromBuffer(thread, data, size, entry, filename.c_str(), needUpdate)) {
        if (thread->HasPendingException()) {
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute ark buffer file '" << filename
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    return true;
}

// The security interface needs to be modified accordingly.
bool JSNApi::ExecuteModuleBuffer(EcmaVM *vm, const uint8_t *data, int32_t size, const std::string &filename,
                                 bool needUpdate)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(DEBUG) << "start to execute module buffer: " << filename;
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteModuleBuffer(thread, data, size, filename.c_str(), needUpdate)) {
        if (thread->HasPendingException()) {
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute module buffer file '" << filename;
        return false;
    }
    return true;
}

bool JSNApi::ExecuteSecure(EcmaVM *vm, uint8_t *data, int32_t size, const std::string &entry,
                           const std::string &filename, bool needUpdate)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(INFO) << "start to execute ark buffer with secure memory: " << filename;
    if (!CheckSecureMem(reinterpret_cast<uintptr_t>(data))) {
        LOG_ECMA(ERROR) << "Secure memory check failed, please execute in srcure memory.";
        return false;
    }
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromBufferSecure(thread, data, size, entry, filename.c_str(),
                                                                  needUpdate)) {
        if (thread->HasPendingException()) {
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute ark buffer file '" << filename
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    return true;
}

bool JSNApi::ExecuteModuleBufferSecure(EcmaVM *vm, uint8_t* data, int32_t size, const std::string &filename,
                                       bool needUpdate)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(INFO) << "start to execute module buffer with secure memory: " << filename;
    if (!CheckSecureMem(reinterpret_cast<uintptr_t>(data))) {
        LOG_ECMA(ERROR) << "Secure memory check failed, please execute in srcure memory.";
        return false;
    }
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteModuleBufferSecure(thread, data, size, filename.c_str(),
                                                                    needUpdate)) {
        if (thread->HasPendingException()) {
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute module buffer file '" << filename;
        return false;
    }
    return true;
}

void JSNApi::PreFork(EcmaVM *vm)
{
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    vm->PreFork();
}

void JSNApi::PostFork(EcmaVM *vm, const RuntimeOption &option)
{
    JSRuntimeOptions &jsOption = vm->GetJSOptions();
    LOG_ECMA(INFO) << "asmint: " << jsOption.GetEnableAsmInterpreter()
                    << ", aot: " << jsOption.GetEnableAOT()
                    << ", bundle name: " <<  option.GetBundleName();
    jsOption.SetEnablePGOProfiler(option.GetEnableProfile());
    ecmascript::pgo::PGOProfilerManager::GetInstance()->SetBundleName(option.GetBundleName());
    JSRuntimeOptions runtimeOptions;
    runtimeOptions.SetLogLevel(Log::LevelToString(Log::ConvertFromRuntime(option.GetLogLevel())));
    Log::Initialize(runtimeOptions);

    if (jsOption.GetEnableAOT() && option.GetAnDir().size()) {
        ecmascript::AnFileDataManager::GetInstance()->SetDir(option.GetAnDir());
        ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
    }

    vm->PostFork();
}

void JSNApi::AddWorker(EcmaVM *hostVm, EcmaVM *workerVm)
{
    if (hostVm != nullptr && workerVm != nullptr) {
        hostVm->WorkersetInfo(workerVm);
        workerVm->SetBundleName(hostVm->GetBundleName());
    }
}

bool JSNApi::DeleteWorker(EcmaVM *hostVm, EcmaVM *workerVm)
{
    if (hostVm != nullptr && workerVm != nullptr) {
        return hostVm->DeleteWorker(workerVm);
    }
    return false;
}

Local<ObjectRef> JSNApi::GetUncaughtException(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<ObjectRef>(vm->GetEcmaUncaughtException());
}

Local<ObjectRef> JSNApi::GetAndClearUncaughtException(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<ObjectRef>(vm->GetAndClearEcmaUncaughtException());
}

bool JSNApi::HasPendingException(const EcmaVM *vm)
{
    return vm->GetJSThread()->HasPendingException();
}

bool JSNApi::IsExecutingPendingJob(const EcmaVM *vm)
{
    return vm->GetAssociatedJSThread()->GetCurrentEcmaContext()->IsExecutingPendingJob();
}

bool JSNApi::HasPendingJob(const EcmaVM *vm)
{
    return vm->GetAssociatedJSThread()->GetCurrentEcmaContext()->HasPendingJob();
}

void JSNApi::EnableUserUncaughtErrorHandler(EcmaVM *vm)
{
    return vm->GetJSThread()->GetCurrentEcmaContext()->EnableUserUncaughtErrorHandler();
}

Local<ObjectRef> JSNApi::GetGlobalObject(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> global(vm->GetJSThread(), globalEnv->GetGlobalObject());
    return JSNApiHelper::ToLocal<ObjectRef>(global);
}

void JSNApi::ExecutePendingJob(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EcmaVM::ConstCast(vm)->GetJSThread()->GetCurrentEcmaContext()->ExecutePromisePendingJob();
}

uintptr_t JSNApi::GetHandleAddr(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_CHECK(vm);
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(localAddress));
    return ecmascript::EcmaHandleScope::NewHandle(thread, value);
}

uintptr_t JSNApi::GetGlobalHandleAddr(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_CHECK(vm);
    JSTaggedType value = *(reinterpret_cast<JSTaggedType *>(localAddress));
    return thread->NewGlobalHandle(value);
}

uintptr_t JSNApi::SetWeak(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_CHECK(vm);
    return thread->SetWeak(localAddress);
}

uintptr_t JSNApi::SetWeakCallback(const EcmaVM *vm, uintptr_t localAddress, void *ref,
                                  WeakRefClearCallBack freeGlobalCallBack, WeakRefClearCallBack nativeFinalizeCallback)
{
    if (localAddress == 0) {
        return 0;
    }
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_CHECK(vm);
    return thread->SetWeak(localAddress, ref, freeGlobalCallBack, nativeFinalizeCallback);
}

uintptr_t JSNApi::ClearWeak(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return 0;
    }
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    if (JSTaggedValue(reinterpret_cast<ecmascript::Node *>(localAddress)->GetObject())
        .IsUndefined()) {
        LOG_ECMA(ERROR) << "The object of weak reference has been recycled!";
        return 0;
    }
    CROSS_THREAD_CHECK(vm);
    return thread->ClearWeak(localAddress);
}

bool JSNApi::IsWeak(const EcmaVM *vm, uintptr_t localAddress)
{
    if (localAddress == 0) {
        return false;
    }
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_CHECK(vm);
    return thread->IsWeak(localAddress);
}

void JSNApi::DisposeGlobalHandleAddr(const EcmaVM *vm, uintptr_t addr)
{
    if (addr == 0 || !reinterpret_cast<ecmascript::Node *>(addr)->IsUsing()) {
        return;
    }
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_CHECK(vm);
    thread->DisposeGlobalHandle(addr);
}

void *JSNApi::SerializeValue(const EcmaVM *vm, Local<JSValueRef> value, Local<JSValueRef> transfer,
                             Local<JSValueRef> cloneList, bool defaultTransfer, bool defaultCloneShared)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, nullptr);
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    JSHandle<JSTaggedValue> arkValue = JSNApiHelper::ToJSHandle(value);
    JSHandle<JSTaggedValue> arkTransfer = JSNApiHelper::ToJSHandle(transfer);
    JSHandle<JSTaggedValue> arkCloneList = JSNApiHelper::ToJSHandle(cloneList);
#if ECMASCRIPT_ENABLE_VALUE_SERIALIZER
    ecmascript::ValueSerializer serializer(thread, defaultTransfer, defaultCloneShared);
    std::unique_ptr<ecmascript::SerializeData> data;
    if (serializer.WriteValue(thread, arkValue, arkTransfer, arkCloneList)) {
        data = serializer.Release();
    }
    if (data == nullptr) {
        return nullptr;
    } else {
        return reinterpret_cast<void *>(data.release());
    }
#else
    ecmascript::Serializer serializer(thread);
    std::unique_ptr<ecmascript::SerializationData> data;
    if (serializer.WriteValue(thread, arkValue, arkTransfer)) {
        data = serializer.Release();
    }
    if (data == nullptr) {
        return nullptr;
    } else {
        return reinterpret_cast<void *>(data.release());
    }
#endif
}

Local<JSValueRef> JSNApi::DeserializeValue(const EcmaVM *vm, void *recoder, void *hint)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
#if ECMASCRIPT_ENABLE_VALUE_SERIALIZER
    std::unique_ptr<ecmascript::SerializeData> data(reinterpret_cast<ecmascript::SerializeData *>(recoder));
    ecmascript::BaseDeserializer deserializer(thread, data.release(), hint);
    JSHandle<JSTaggedValue> result = deserializer.ReadValue();
    return JSNApiHelper::ToLocal<ObjectRef>(result);
#else
    std::unique_ptr<ecmascript::SerializationData> data(reinterpret_cast<ecmascript::SerializationData *>(recoder));
    ecmascript::Deserializer deserializer(thread, data.release(), hint);
    JSHandle<JSTaggedValue> result = deserializer.ReadValue();
    return JSNApiHelper::ToLocal<ObjectRef>(result);
#endif
}

void JSNApi::DeleteSerializationData(void *data)
{
#if ECMASCRIPT_ENABLE_VALUE_SERIALIZER
    ecmascript::SerializeData *value = reinterpret_cast<ecmascript::SerializeData *>(data);
    delete value;
    value = nullptr;
#else
    ecmascript::SerializationData *value = reinterpret_cast<ecmascript::SerializationData *>(data);
    delete value;
    value = nullptr;
#endif
}

void HostPromiseRejectionTracker(const EcmaVM *vm,
                                 const JSHandle<JSPromise> promise,
                                 const JSHandle<JSTaggedValue> reason,
                                 const ecmascript::PromiseRejectionEvent operation,
                                 void* data)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::PromiseRejectCallback promiseRejectCallback =
        thread->GetCurrentEcmaContext()->GetPromiseRejectCallback();
    if (promiseRejectCallback != nullptr) {
        Local<JSValueRef> promiseVal = JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>::Cast(promise));
        PromiseRejectInfo promiseRejectInfo(promiseVal, JSNApiHelper::ToLocal<JSValueRef>(reason),
                              static_cast<PromiseRejectInfo::PROMISE_REJECTION_EVENT>(operation), data);
        promiseRejectCallback(reinterpret_cast<void*>(&promiseRejectInfo));
    }
}

void JSNApi::SetHostPromiseRejectionTracker(EcmaVM *vm, void *cb, void* data)
{
    CROSS_THREAD_CHECK(vm);
    thread->GetCurrentEcmaContext()->SetHostPromiseRejectionTracker(HostPromiseRejectionTracker);
    thread->GetCurrentEcmaContext()->SetPromiseRejectCallback(
        reinterpret_cast<ecmascript::PromiseRejectCallback>(cb));
    thread->GetCurrentEcmaContext()->SetData(data);
}

void JSNApi::SetHostResolveBufferTracker(EcmaVM *vm,
    std::function<bool(std::string dirPath, uint8_t **buff, size_t *buffSize)> cb)
{
    vm->SetResolveBufferCallback(cb);
}

void JSNApi::SetSearchHapPathTracker(EcmaVM *vm,
    std::function<bool(const std::string moduleName, std::string &hapPath)> cb)
{
    vm->SetSearchHapPathCallBack(cb);
}

void JSNApi::SetRequestAotCallback([[maybe_unused]] EcmaVM *vm, const std::function<int32_t
    (const std::string &bundleName, const std::string &moduleName, int32_t triggerMode)> &cb)
{
    ecmascript::pgo::PGOProfilerManager::GetInstance()->SetRequestAotCallback(cb);
}

void JSNApi::SetUnloadNativeModuleCallback(EcmaVM *vm, const std::function<bool(const std::string &moduleKey)> &cb)
{
    vm->SetUnloadNativeModuleCallback(cb);
}

void JSNApi::SetNativePtrGetter(EcmaVM *vm, void* cb)
{
    vm->SetNativePtrGetter(reinterpret_cast<ecmascript::NativePtrGetter>(cb));
}

void JSNApi::SetHostEnqueueJob(const EcmaVM *vm, Local<JSValueRef> cb)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    JSHandle<JSFunction> fun = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(cb));
    JSHandle<TaggedArray> array = vm->GetFactory()->EmptyArray();
    JSHandle<MicroJobQueue> job = thread->GetCurrentEcmaContext()->GetMicroJobQueue();
    MicroJobQueue::EnqueueJob(thread, job, QueueType::QUEUE_PROMISE, fun, array);
}

bool JSNApi::ExecuteModuleFromBuffer(EcmaVM *vm, const void *data, int32_t size, const std::string &file)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromBuffer(thread, data, size, ENTRY_POINTER, file.c_str())) {
        std::cerr << "Cannot execute panda file from memory" << std::endl;
        return false;
    }
    return true;
}

Local<ObjectRef> JSNApi::GetExportObject(EcmaVM *vm, const std::string &file, const std::string &key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    ecmascript::CString entry = file.c_str();
    ecmascript::CString name = vm->GetAssetPath();
    if (!vm->IsBundlePack()) {
        ModulePathHelper::ParseOhmUrl(vm, entry, name, entry);
        std::shared_ptr<JSPandaFile> jsPandaFile =
            JSPandaFileManager::GetInstance()->LoadJSPandaFile(thread, name, entry.c_str(), false);
        if (jsPandaFile == nullptr) {
            JSHandle<JSTaggedValue> exportObj(thread, JSTaggedValue::Null());
            return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
        }
        if (!jsPandaFile->IsRecordWithBundleName()) {
            PathHelper::AdaptOldIsaRecord(entry);
        }
    }
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<ecmascript::SourceTextModule> ecmaModule = moduleManager->HostGetImportedModule(entry);
    if (ecmaModule->GetIsNewBcVersion()) {
        int index = ecmascript::ModuleManager::GetExportObjectIndex(vm, ecmaModule, key);
        JSTaggedValue result = ecmaModule->GetModuleValue(thread, index, false);
        JSHandle<JSTaggedValue> exportObj(thread, result);
        return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
    }
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<EcmaString> keyHandle = factory->NewFromASCII(key.c_str());

    JSTaggedValue result = ecmaModule->GetModuleValue(thread, keyHandle.GetTaggedValue(), false);
    JSHandle<JSTaggedValue> exportObj(thread, result);
    return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
}

Local<ObjectRef> JSNApi::GetExportObjectFromBuffer(EcmaVM *vm, const std::string &file,
                                                   const std::string &key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<ecmascript::SourceTextModule> ecmaModule = moduleManager->HostGetImportedModule(file.c_str());

    if (ecmaModule->GetIsNewBcVersion()) {
        int index = ecmascript::ModuleManager::GetExportObjectIndex(vm, ecmaModule, key);
        JSTaggedValue result = ecmaModule->GetModuleValue(thread, index, false);
        JSHandle<JSTaggedValue> exportObj(thread, result);
        return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
    }

    ObjectFactory *factory = vm->GetFactory();
    JSHandle<EcmaString> keyHandle = factory->NewFromASCII(key.c_str());
    JSTaggedValue result = ecmaModule->GetModuleValue(thread, keyHandle.GetTaggedValue(), false);
    JSHandle<JSTaggedValue> exportObj(thread, result);
    return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
}

Local<ObjectRef> JSNApi::ExecuteNativeModule(EcmaVM *vm, const std::string &key)
{
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> exportObj = moduleManager->LoadNativeModule(thread, key);
    return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
}

Local<ObjectRef> JSNApi::GetModuleNameSpaceFromFile(EcmaVM *vm, const std::string &file, const std::string &module_path)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    // need get moduleName from stack
    std::pair<std::string, std::string> moduleInfo = vm->GetCurrentModuleInfo(false);
    std::string recordNameStr;
    if (module_path.size() != 0) {
        recordNameStr = module_path + PathHelper::SLASH_TAG + file;
    } else {
        recordNameStr = std::string(vm->GetBundleName().c_str()) + PathHelper::SLASH_TAG +
            moduleInfo.first + PathHelper::SLASH_TAG + file;
    }
    LOG_ECMA(DEBUG) << "JSNApi::LoadModuleNameSpaceFromFile: Concated recordName " << recordNameStr.c_str();
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> moduleNamespace = moduleManager->
        GetModuleNameSpaceFromFile(thread, recordNameStr, moduleInfo.second);
    return JSNApiHelper::ToLocal<ObjectRef>(moduleNamespace);
}

// ---------------------------------- Promise -------------------------------------
Local<PromiseRef> PromiseRef::Catch(const EcmaVM *vm, Local<FunctionRef> handler)
{
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(promise, ERROR);
    JSHandle<JSTaggedValue> catchKey(thread, constants->GetPromiseCatchString());
    JSHandle<JSTaggedValue> reject = JSNApiHelper::ToJSHandle(handler);
    JSHandle<JSTaggedValue> undefined = constants->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 1);
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    info->SetCallArg(reject.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, catchKey);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<PromiseRef> PromiseRef::Finally(const EcmaVM *vm, Local<FunctionRef> handler)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(promise, ERROR);
    JSHandle<JSTaggedValue> finallyKey = constants->GetHandledPromiseFinallyString();
    JSHandle<JSTaggedValue> resolver = JSNApiHelper::ToJSHandle(handler);
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 2); // 2: two args
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    info->SetCallArg(resolver.GetTaggedValue(), undefined.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, finallyKey);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<PromiseRef> PromiseRef::Then(const EcmaVM *vm, Local<FunctionRef> handler)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(promise, ERROR);
    JSHandle<JSTaggedValue> thenKey(thread, constants->GetPromiseThenString());
    JSHandle<JSTaggedValue> resolver = JSNApiHelper::ToJSHandle(handler);
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 2); // 2: two args
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    info->SetCallArg(resolver.GetTaggedValue(), undefined.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, thenKey);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<PromiseRef> PromiseRef::Then(const EcmaVM *vm, Local<FunctionRef> onFulfilled, Local<FunctionRef> onRejected)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> promise = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(promise, ERROR);
    JSHandle<JSTaggedValue> thenKey(thread, constants->GetPromiseThenString());
    JSHandle<JSTaggedValue> resolver = JSNApiHelper::ToJSHandle(onFulfilled);
    JSHandle<JSTaggedValue> reject = JSNApiHelper::ToJSHandle(onRejected);
    JSHandle<JSTaggedValue> undefined(constants->GetHandledUndefined());
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, promise, undefined, 2); // 2: two args
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    info->SetCallArg(resolver.GetTaggedValue(), reject.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, thenKey);

    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, result));
}

Local<JSValueRef> PromiseRef::GetPromiseState(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    JSHandle<JSPromise> promise(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(promise, ERROR);

    ecmascript::PromiseState state = promise->GetPromiseState();
    std::string promiseStateStr;
    switch (state) {
        case ecmascript::PromiseState::PENDING:
            promiseStateStr = "Pending";
            break;
        case ecmascript::PromiseState::FULFILLED:
            promiseStateStr = "Fulfilled";
            break;
        case ecmascript::PromiseState::REJECTED:
            promiseStateStr = "Rejected";
            break;
    }

    ObjectFactory *factory = vm->GetFactory();
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(factory->NewFromStdString(promiseStateStr)));
}

Local<JSValueRef> PromiseRef::GetPromiseResult(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    JSHandle<JSPromise> promise(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(promise, ERROR);

    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(vm->GetJSThread(), promise->GetPromiseResult()));
}
// ---------------------------------- ProxyRef -----------------------------------------
Local<JSValueRef> ProxyRef::GetHandler(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    JSHandle<JSProxy> jsProxy(JSNApiHelper::ToJSHandle(this));
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, jsProxy->GetHandler()));
}

Local<JSValueRef> ProxyRef::GetTarget(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    JSHandle<JSProxy> jsProxy(JSNApiHelper::ToJSHandle(this));
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, jsProxy->GetTarget()));
}

bool ProxyRef::IsRevoked()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, false);
    JSHandle<JSProxy> jsProxy(JSNApiHelper::ToJSHandle(this));
    return jsProxy->GetIsRevoked();
}

// ---------------------------------- SetRef --------------------------------------
int32_t SetRef::GetSize()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    return set->GetSize();
}

int32_t SetRef::GetTotalElements()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    return static_cast<int>(set->GetSize()) +
        LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> SetRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, set->GetValue(entry)));
}

Local<SetRef> SetRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ObjectFactory *factory = vm->GetJSThread()->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetJSThread()->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(vm->GetJSThread());
    set->SetLinkedSet(vm->GetJSThread(), hashSet);
    JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(set);
    return JSNApiHelper::ToLocal<SetRef>(setTag);
}

void SetRef::Add(const EcmaVM *vm, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, ERROR);
    JSSet::Add(vm->GetJSThread(), set, JSNApiHelper::ToJSHandle(value));
}

// ---------------------------------- WeakMapRef --------------------------------------
int32_t WeakMapRef::GetSize()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    return weakMap->GetSize();
}

int32_t WeakMapRef::GetTotalElements()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    return weakMap->GetSize() +
                LinkedHashMap::Cast(weakMap->GetLinkedMap().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> WeakMapRef::GetKey(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakMap, FATAL);
    JSTaggedValue key = weakMap->GetKey(entry);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, key.GetWeakRawValue()));
}

Local<JSValueRef> WeakMapRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakMap, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, weakMap->GetValue(entry)));
}

Local<WeakMapRef> WeakMapRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ObjectFactory *factory = vm->GetJSThread()->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetJSThread()->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakMapFunction();
    JSHandle<JSWeakMap> weakMap =
        JSHandle<JSWeakMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> hashMap = LinkedHashMap::Create(vm->GetJSThread());
    weakMap->SetLinkedMap(vm->GetJSThread(), hashMap);
    JSHandle<JSTaggedValue> weakMapTag = JSHandle<JSTaggedValue>::Cast(weakMap);
    return JSNApiHelper::ToLocal<WeakMapRef>(weakMapTag);
}

void WeakMapRef::Set(const EcmaVM *vm, const Local<JSValueRef> &key, const Local<JSValueRef> &value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakMap, FATAL);
    JSWeakMap::Set(vm->GetJSThread(), weakMap, JSNApiHelper::ToJSHandle(key), JSNApiHelper::ToJSHandle(value));
}

bool WeakMapRef::Has(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, false);
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    return weakMap->Has(thread, JSNApiHelper::ToJSTaggedValue(*key));
}

// ---------------------------------- WeakSetRef --------------------------------------
int32_t WeakSetRef::GetSize()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    return weakSet->GetSize();
}

int32_t WeakSetRef::GetTotalElements()
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    return weakSet->GetSize() +
                LinkedHashSet::Cast(weakSet->GetLinkedSet().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> WeakSetRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakSet, FATAL);
    JSTaggedValue value = weakSet->GetValue(entry);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, value.GetWeakRawValue()));
}

Local<WeakSetRef> WeakSetRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ObjectFactory *factory = vm->GetJSThread()->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetJSThread()->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSWeakSet> weakSet =
        JSHandle<JSWeakSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> hashWeakSet = LinkedHashSet::Create(vm->GetJSThread());
    weakSet->SetLinkedSet(vm->GetJSThread(), hashWeakSet);
    JSHandle<JSTaggedValue> setTag = JSHandle<JSTaggedValue>::Cast(weakSet);
    return JSNApiHelper::ToLocal<WeakSetRef>(setTag);
}

void WeakSetRef::Add(const EcmaVM *vm, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakSet, ERROR);
    JSWeakSet::Add(vm->GetJSThread(), weakSet, JSNApiHelper::ToJSHandle(value));
}

TryCatch::~TryCatch() {}

bool TryCatch::HasCaught() const
{
    return ecmaVm_->GetJSThread()->HasPendingException();
}

void TryCatch::Rethrow()
{
    rethrow_ = true;
}

Local<ObjectRef> TryCatch::GetAndClearException()
{
    return JSNApiHelper::ToLocal<ObjectRef>(ecmaVm_->GetAndClearEcmaUncaughtException());
}

Local<ObjectRef> TryCatch::GetException()
{
    return JSNApiHelper::ToLocal<ObjectRef>(ecmaVm_->GetEcmaUncaughtException());
}

void TryCatch::ClearException()
{
    ecmaVm_->GetJSThread()->ClearException();
}

} // namespace panda
