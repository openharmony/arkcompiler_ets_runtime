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

#include <array>
#include <cstdint>
#include <fcntl.h>

#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/pgo_profiler/pgo_profiler_manager.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/jsnapi_sendable.h"
#include "ecmascript/jspandafile/js_pandafile_executor.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/module/napi_module_loader.h"
#if defined(ENABLE_EXCEPTION_BACKTRACE)
#include "ecmascript/platform/backtrace.h"
#endif
#include "ecmascript/regexp/regexp_parser.h"
#include "ecmascript/serializer/base_deserializer.h"
#include "ecmascript/serializer/value_serializer.h"
#include "ecmascript/ohos/ohos_preload_app_info.h"
#include "ecmascript/platform/aot_crash_info.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_number_format.h"
#endif

namespace panda {
using ecmascript::AccessorData;
using ecmascript::BigInt;
using ecmascript::ByteArray;
using ecmascript::DataViewType;
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
using ecmascript::JSNapiSendable;
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
using ecmascript::NativeModuleFailureInfo;
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
using Concurrent = ecmascript::Concurrent;
using EnableAotJitListHelper = ecmascript::ohos::EnableAotJitListHelper;
using PGOProfilerManager = ecmascript::pgo::PGOProfilerManager;
using AotRuntimeInfo = ecmascript::ohos::AotRuntimeInfo;

namespace {
// NOLINTNEXTLINE(fuchsia-statically-constructed-objects)
constexpr std::string_view ENTRY_POINTER = "_GLOBAL::func_main_0";
}

int JSNApi::vmCount_ = 0;
bool JSNApi::initialize_ = false;
bool JSNApi::isForked_ = false;
static Mutex *mutex = new panda::Mutex();
StartIdleMonitorCallback JSNApi::startIdleMonitorCallback_ = nullptr;

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

Local<PrimitiveRef> JSValueRef::Hole(const EcmaVM *vm)
{
    return JSNApiHelper::ToLocal<PrimitiveRef>(
        vm->GetJSThread()->GlobalConstants()->GetHandledHole());
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

Local<ObjectRef> JSValueRef::ToEcmaObject(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    if (obj->IsECMAObject()) {
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return JSNApiHelper::ToLocal<ObjectRef>(obj);
    }

    return Undefined(vm);
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
    // The function just get handle' value, and will not read and write js object. Don't need to switch state.
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    return JSNApiHelper::ToLocal<NativePointerRef>(obj);
}

bool JSValueRef::BooleaValue(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> tagged = JSNApiHelper::ToJSHandle(this);
    uint32_t number = 0;
    if (!tagged->IsECMAObject()) {
        number = JSTaggedValue::ToUint32(thread, tagged);
    } else {
        // EcmaObject may call [Symbol.toPrimitive].
        number = JSTaggedValue::ToUint32(thread, tagged);
    }
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number;
}

int32_t JSValueRef::Int32Value(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> tagged = JSNApiHelper::ToJSHandle(this);
    int32_t number = 0;
    if (!tagged->IsECMAObject()) {
        number = JSTaggedValue::ToInt32(thread, tagged);
    } else {
        // EcmaObject may call [Symbol.toPrimitive].
        number = JSTaggedValue::ToInt32(thread, tagged);
    }
    RETURN_VALUE_IF_ABRUPT(thread, 0);
    return number;
}

double JSValueRef::GetValueDouble(bool &isNumber)
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    if (value.IsInt()) {
        isNumber = true;
        return static_cast<double>(value.GetInt());
    }
    if (value.IsDouble()) {
        isNumber = true;
        return value.GetDouble();
    }
    isNumber = false;
    return 0.0;
}

int32_t JSValueRef::GetValueInt32(bool &isNumber)
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    if (value.IsInt()) {
        isNumber = true;
        return value.GetInt();
    }
    if (value.IsDouble()) {
        isNumber = true;
        return static_cast<int32_t>(ecmascript::base::NumberHelper::DoubleToInt(value.GetDouble(),
            ecmascript::base::INT32_BITS));
    }
    isNumber = false;
    return 0;
}

uint32_t JSValueRef::GetValueUint32(bool &isNumber)
{
    return static_cast<uint32_t>(GetValueInt32(isNumber));
}

int64_t JSValueRef::GetValueInt64(bool &isNumber)
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    if (value.IsInt()) {
        isNumber = true;
        return static_cast<int64_t>(value.GetInt());
    }
    if (value.IsDouble()) {
        isNumber = true;
        double getVale = value.GetDouble();
        if (!std::isfinite(getVale) || std::isnan(getVale)) {
            return 0;
        }
        return NumberHelper::DoubleToInt64(getVale);
    }
    isNumber = false;
    return 0;
}

bool JSValueRef::GetValueBool(bool &isBool)
{
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    if (value.IsTrue()) {
        isBool = true;
        return true;
    }
    if (value.IsFalse()) {
        isBool = true;
        return false;
    }
    isBool = false;
    return false;
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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

bool JSValueRef::IsBigInt(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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

bool JSValueRef::IsString(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsString();
}

bool JSValueRef::IsSymbol(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsSymbol();
}

bool JSValueRef::IsObject(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsECMAObject();
}

bool JSValueRef::IsArray(const EcmaVM *vm)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsArray(thread);
}

bool JSValueRef::IsJSArray(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSArray();
}

bool JSValueRef::IsConstructor(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsHeapObject() && value.IsConstructor();
}

bool JSValueRef::IsFunction(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsCallable();
}

bool JSValueRef::IsJSFunction(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue value = JSNApiHelper::ToJSTaggedValue(this);
    return value.IsJSFunction();
}

bool JSValueRef::IsProxy(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSProxy();
}

bool JSValueRef::IsPromise(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPromise();
}

bool JSValueRef::IsDataView(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsDataView();
}

bool JSValueRef::IsTypedArray(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsTypedArray();
}

bool JSValueRef::IsNativePointer(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSNativePointer();
}

bool JSValueRef::IsDate(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsDate();
}

bool JSValueRef::IsError(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSError();
}

bool JSValueRef::IsMap(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSMap();
}

bool JSValueRef::IsSet(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSet();
}

bool JSValueRef::IsWeakRef(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakRef();
}

bool JSValueRef::IsWeakMap(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakMap();
}

bool JSValueRef::IsWeakSet(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSWeakSet();
}

bool JSValueRef::IsRegExp(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSRegExp();
}

bool JSValueRef::IsArrayIterator(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSArrayIterator();
}

bool JSValueRef::IsStringIterator(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsStringIterator();
}

bool JSValueRef::IsSetIterator(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSetIterator();
}

bool JSValueRef::IsMapIterator(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSMapIterator();
}

bool JSValueRef::IsArrayBuffer(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsArrayBuffer();
}

bool JSValueRef::IsBuffer(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsArrayBuffer();
}

bool JSValueRef::IsUint8Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint8Array();
}

bool JSValueRef::IsInt8Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt8Array();
}

bool JSValueRef::IsUint8ClampedArray(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint8ClampedArray();
}

bool JSValueRef::IsInt16Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt16Array();
}

bool JSValueRef::IsUint16Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint16Array();
}

bool JSValueRef::IsInt32Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSInt32Array();
}

bool JSValueRef::IsUint32Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSUint32Array();
}

bool JSValueRef::IsFloat32Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSFloat32Array();
}

bool JSValueRef::IsFloat64Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSFloat64Array();
}

bool JSValueRef::IsBigInt64Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSBigInt64Array();
}

bool JSValueRef::IsBigUint64Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSBigUint64Array();
}

bool JSValueRef::IsJSSharedInt8Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedInt8Array();
}

bool JSValueRef::IsJSSharedUint8Array([[maybe_unused]]const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedUint8Array();
}

bool JSValueRef::IsJSSharedUint8ClampedArray([[maybe_unused]]const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedUint8ClampedArray();
}

bool JSValueRef::IsJSSharedInt16Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedInt16Array();
}

bool JSValueRef::IsJSSharedUint16Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedUint16Array();
}

bool JSValueRef::IsJSSharedInt32Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedInt32Array();
}

bool JSValueRef::IsJSSharedFloat32Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedFloat32Array();
}

bool JSValueRef::IsJSSharedUint32Array(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedUint32Array();
}

bool JSValueRef::IsJSPrimitiveRef(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPrimitiveRef();
}

bool JSValueRef::IsJSPrimitiveNumber(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return IsJSPrimitiveRef(vm) ? JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsNumber() : false;
}

bool JSValueRef::IsJSPrimitiveInt(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return IsJSPrimitiveRef(vm) ? JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsInt() : false;
}

bool JSValueRef::IsJSPrimitiveBoolean(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return IsJSPrimitiveRef(vm) ? JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsBoolean() : false;
}

bool JSValueRef::IsJSPrimitiveString(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return IsJSPrimitiveRef(vm) ? JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsString() : false;
}

bool JSValueRef::IsJSPrimitiveSymbol(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, FATAL);
    return IsJSPrimitiveRef(vm) ? JSPrimitiveRef::Cast(obj->GetTaggedObject())->IsSymbol() : false;
}

bool JSValueRef::IsGeneratorObject(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsGeneratorObject();
}

bool JSValueRef::IsModuleNamespaceObject(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsModuleNamespace();
}

bool JSValueRef::IsNativeModuleFailureInfoObject(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsNativeModuleFailureInfo();
}

bool JSValueRef::IsSharedArrayBuffer(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsSharedArrayBuffer();
}

bool JSValueRef::IsSendableArrayBuffer(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsSendableArrayBuffer();
}

bool JSValueRef::IsJSLocale(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSLocale();
}

bool JSValueRef::IsJSDateTimeFormat(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSDateTimeFormat();
}

bool JSValueRef::IsJSRelativeTimeFormat(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSRelativeTimeFormat();
}

bool JSValueRef::IsJSIntl(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSIntl();
}

bool JSValueRef::IsJSNumberFormat(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSNumberFormat();
}

bool JSValueRef::IsJSCollator(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSCollator();
}

bool JSValueRef::IsJSPluralRules(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSPluralRules();
}

bool JSValueRef::IsJSListFormat(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSListFormat();
}

bool JSValueRef::IsAsyncGeneratorObject(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsAsyncGeneratorObject();
}

bool JSValueRef::IsAsyncFunction(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAsyncFunction();
}

bool JSValueRef::IsConcurrentFunction(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> funcVal = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSFunction> transFunc = JSHandle<JSFunction>::Cast(funcVal);
    return transFunc->GetFunctionKind() == ecmascript::FunctionKind::CONCURRENT_FUNCTION;
}

bool JSValueRef::IsArgumentsObject(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsArguments();
}

bool JSValueRef::IsGeneratorFunction(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsGeneratorFunction();
}

bool JSValueRef::IsAsyncGeneratorFunction(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsAsyncGeneratorFunction();
}

bool JSValueRef::IsArrayList(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIArrayList();
}

bool JSValueRef::IsDeque(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIDeque();
}

bool JSValueRef::IsHashMap(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIHashMap();
}

bool JSValueRef::IsHashSet(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIHashSet();
}

bool JSValueRef::IsLightWeightMap(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILightWeightMap();
}

bool JSValueRef::IsLightWeightSet(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILightWeightSet();
}

bool JSValueRef::IsLinkedList(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILinkedList();
}

bool JSValueRef::IsLinkedListIterator(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPILinkedListIterator();
}

bool JSValueRef::IsList(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIList();
}

bool JSValueRef::IsPlainArray(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIPlainArray();
}

bool JSValueRef::IsQueue(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIQueue();
}

bool JSValueRef::IsStack(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIStack();
}

bool JSValueRef::IsTreeMap(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPITreeMap();
}

bool JSValueRef::IsTreeSet(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPITreeSet();
}

bool JSValueRef::IsVector(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSAPIVector();
}

bool JSValueRef::IsSendableObject(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return IsJSShared(vm) && IsObject(vm);
}

bool JSValueRef::IsJSShared(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSShared();
}

bool JSValueRef::IsSharedArray(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedArray();
}

bool JSValueRef::IsSharedTypedArray(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsSharedTypedArray();
}

bool JSValueRef::IsSharedSet(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedSet();
}

bool JSValueRef::IsSharedMap(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedMap();
}

bool JSValueRef::IsSharedMapIterator(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToJSTaggedValue(this).IsJSSharedMapIterator();
}

bool JSValueRef::IsHeapObject()
{
    return JSNApiHelper::ToJSTaggedValue(this).IsHeapObject();
}

void *JSValueRef::GetNativePointerValue(const EcmaVM* vm, bool &isNativePointer)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (IsJSShared(vm)) {
        return GetNativePointerValueImpl(vm, isNativePointer);
    } else {
        return GetNativePointerValueImpl(vm, isNativePointer);
    }
}

// private
void *JSValueRef::GetNativePointerValueImpl(const EcmaVM* vm, bool &isNativePointer)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (!IsNativePointer(vm)) {
        isNativePointer = false;
        return nullptr;
    }
    isNativePointer = true;
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, nullptr);
    JSHandle<JSTaggedValue> nativePointer = JSNApiHelper::ToJSHandle(this);
    return JSHandle<JSNativePointer>(nativePointer)->GetExternalPointer();
}

bool JSValueRef::IsDetachedArraybuffer(const EcmaVM *vm, bool &isArrayBuffer)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    // arraybuffer is not shared. Do not need to switch state
    if (!IsArrayBuffer(vm)) {
        isArrayBuffer = false;
        return false;
    }
    isArrayBuffer = true;
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    return arrayBuffer->IsDetach();
}

void JSValueRef::DetachedArraybuffer(const EcmaVM *vm, bool &isArrayBuffer)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (IsArrayBuffer(vm)) {
        JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
        if (arrayBuffer->IsDetach()) {
            return;
        }
        arrayBuffer->Detach(vm->GetJSThread());
        isArrayBuffer = true;
    } else if (IsSendableArrayBuffer(vm)) {
        JSHandle<ecmascript::JSSendableArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
        if (arrayBuffer->IsDetach()) {
            return;
        }
        arrayBuffer->Detach(vm->GetJSThread());
        isArrayBuffer = true;
    } else {
        isArrayBuffer = false;
    }
}

void JSValueRef::GetDataViewInfo(const EcmaVM *vm,
                                 bool &isDataView,
                                 size_t *byteLength,
                                 void **data,
                                 JSValueRef **arrayBuffer,
                                 size_t *byteOffset)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (!IsDataView(vm)) {
        isDataView = false;
        return;
    }
    isDataView = true;
    JSHandle<JSDataView> dataView(JSNApiHelper::ToJSHandle(this));

    if (byteLength) {
        *byteLength = dataView->GetByteLength();
    }
    if (data || arrayBuffer) {
        JSThread* thread = vm->GetJSThread();
        JSHandle<JSArrayBuffer> retArrayBuffer(thread, dataView->GetViewedArrayBuffer());
        if (data) {
            JSTaggedValue bufferData = retArrayBuffer->GetArrayBufferData();
            if (!bufferData.IsJSNativePointer()) {
                *data = nullptr;
            }
            *data = JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
        }
        if (arrayBuffer) {
            *arrayBuffer = reinterpret_cast<JSValueRef*>(retArrayBuffer.GetAddress());
        }
    }
    if (byteOffset) {
        *byteOffset = dataView->GetByteOffset();
    }
}

void JSValueRef::TryGetArrayLength(const EcmaVM *vm, bool *isArrayOrSharedArray, uint32_t *arrayLength)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue thisValue = JSNApiHelper::ToJSTaggedValue(this);
    if (thisValue.IsJSArray()) {
        *isArrayOrSharedArray = true;
        *arrayLength = JSArray::Cast(thisValue.GetTaggedObject())->GetArrayLength();
    } else if (thisValue.IsJSSharedArray()) {
        *isArrayOrSharedArray = true;
        *arrayLength = ecmascript::JSSharedArray::Cast(thisValue.GetTaggedObject())->GetArrayLength();
    } else {
        *isArrayOrSharedArray = false;
    }
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
    if (std::isnan(input)) {
        input = ecmascript::base::NAN_VALUE;
    }
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input), true);
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, int32_t input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input), true);
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, uint32_t input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input), true);
    return JSNApiHelper::ToLocal<NumberRef>(number);
}

Local<NumberRef> NumberRef::New(const EcmaVM *vm, int64_t input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    JSHandle<JSTaggedValue> number(thread, JSTaggedValue(input), true);
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

Local<JSValueRef> MapRef::Get(const EcmaVM *vm, const char *utf8)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8(utf8));
    auto result = JSHandle<JSTaggedValue>(thread, map->Get(thread, key.GetTaggedValue()));
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

void MapRef::Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    JSMap::Set(thread, map, JSNApiHelper::ToJSHandle(key), JSNApiHelper::ToJSHandle(value));
}

void MapRef::Set(const EcmaVM *vm, const char *utf8, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8(utf8));
    JSMap::Set(thread, map, key, JSNApiHelper::ToJSHandle(value));
}

bool MapRef::Has(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    bool result = map->Has(thread, JSNApiHelper::ToJSTaggedValue(*key));
    return result;
}

bool MapRef::Has(const EcmaVM *vm, const char *utf8)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8(utf8));
    bool result = map->Has(thread, key.GetTaggedValue());
    return result;
}

void MapRef::Delete(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    JSMap::Delete(thread, map, JSNApiHelper::ToJSHandle(key));
}

void MapRef::Clear(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    JSMap::Clear(thread, map);
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

int32_t MapRef::GetSize(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    return map->GetSize();
}

int32_t MapRef::GetTotalElements(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    return static_cast<int>((map->GetSize())) +
        LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> MapRef::GetKey(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, map->GetKey(entry)));
}

Local<JSValueRef> MapRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, map->GetValue(entry)));
}

Local<MapIteratorRef> MapRef::GetEntries(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSTaggedValue> map(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> mapIter =
        ecmascript::JSMapIterator::CreateMapIterator(thread, map, IterationKind::KEY_AND_VALUE);
    return JSNApiHelper::ToLocal<MapIteratorRef>(mapIter);
}

Local<MapIteratorRef> MapRef::GetKeys(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSTaggedValue> map(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> mapIter = ecmascript::JSMapIterator::CreateMapIterator(thread, map, IterationKind::KEY);
    return JSNApiHelper::ToLocal<MapIteratorRef>(mapIter);
}

Local<MapIteratorRef> MapRef::GetValues(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSTaggedValue> map(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> mapIter = ecmascript::JSMapIterator::CreateMapIterator(thread, map, IterationKind::VALUE);
    return JSNApiHelper::ToLocal<MapIteratorRef>(mapIter);
}

// SendableMapRef
Local<SendableMapRef> SendableMapRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetSBuiltininMapFunction();
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    JSHandle<ecmascript::JSSharedMap> sharedMap = JSHandle<ecmascript::JSSharedMap>::Cast(obj);
    JSHandle<LinkedHashMap> linkedMap =
        LinkedHashMap::Create(thread, LinkedHashMap::MIN_CAPACITY, ecmascript::MemSpaceKind::SHARED);
    sharedMap->SetLinkedMap(thread, linkedMap);
    JSHandle<JSTaggedValue> sharedMapTag = JSHandle<JSTaggedValue>::Cast(sharedMap);
    return JSNApiHelper::ToLocal<SendableMapRef>(sharedMapTag);
}

Local<JSValueRef> SendableMapRef::Get(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread,
        ecmascript::JSSharedMap::Get(thread, map, JSNApiHelper::ToJSTaggedValue(*key))));
}

Local<JSValueRef> SendableMapRef::Get(const EcmaVM *vm, const char *utf8)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8(utf8));
    auto result = JSHandle<JSTaggedValue>(thread, ecmascript::JSSharedMap::Get(thread, map, key.GetTaggedValue()));
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

void SendableMapRef::Set(const EcmaVM *vm, Local<JSValueRef> key, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    ecmascript::JSSharedMap::Set(thread, map, JSNApiHelper::ToJSHandle(key), JSNApiHelper::ToJSHandle(value));
}

void SendableMapRef::Set(const EcmaVM *vm, const char *utf8, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8(utf8));
    ecmascript::JSSharedMap::Set(thread, map, key, JSNApiHelper::ToJSHandle(value));
}

bool SendableMapRef::Has(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    bool result = ecmascript::JSSharedMap::Has(thread, map, JSNApiHelper::ToJSTaggedValue(*key));
    return result;
}

bool SendableMapRef::Has(const EcmaVM *vm, const char *utf8)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromUtf8(utf8));
    bool result = ecmascript::JSSharedMap::Has(thread, map, key.GetTaggedValue());
    return result;
}

void SendableMapRef::Delete(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    ecmascript::JSSharedMap::Delete(thread, map, JSNApiHelper::ToJSHandle(key));
}

void SendableMapRef::Clear(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    ecmascript::JSSharedMap::Clear(thread, map);
}

uint32_t SendableMapRef::GetSize(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    return ecmascript::JSSharedMap::GetSize(thread, map);
}

uint32_t SendableMapRef::GetTotalElements(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    return static_cast<int>(ecmascript::JSSharedMap::GetSize(thread, map)) +
           LinkedHashMap::Cast(map->GetLinkedMap().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> SendableMapRef::GetKey(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread,
        ecmascript::JSSharedMap::GetKey(thread, map, entry)));
}

Local<JSValueRef> SendableMapRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedMap> map(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(map, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread,
        ecmascript::JSSharedMap::GetValue(thread, map, entry)));
}

Local<SendableMapIteratorRef> SendableMapRef::GetEntries(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSTaggedValue> map(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> sharedMapIter =
        ecmascript::JSSharedMapIterator::CreateMapIterator(thread, map, IterationKind::KEY_AND_VALUE);
    return JSNApiHelper::ToLocal<SendableMapIteratorRef>(sharedMapIter);
}

Local<SendableMapIteratorRef> SendableMapRef::GetKeys(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSTaggedValue> map(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> sharedMapIter =
        ecmascript::JSSharedMapIterator::CreateMapIterator(thread, map, IterationKind::KEY);
    return JSNApiHelper::ToLocal<SendableMapIteratorRef>(sharedMapIter);
}

Local<SendableMapIteratorRef> SendableMapRef::GetValues(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSTaggedValue> map(JSNApiHelper::ToJSHandle(this));
    JSHandle<JSTaggedValue> sharedMapIter =
        ecmascript::JSSharedMapIterator::CreateMapIterator(thread, map, IterationKind::VALUE);
    return JSNApiHelper::ToLocal<SendableMapIteratorRef>(sharedMapIter);
}

// SendableSetRef
Local<SendableSetRef> SendableSetRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetSBuiltininSetFunction();
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    JSHandle<ecmascript::JSSharedSet> set = JSHandle<ecmascript::JSSharedSet>::Cast(obj);
    JSHandle<LinkedHashSet> linkedSet =
        LinkedHashSet::Create(thread, LinkedHashSet::MIN_CAPACITY, ecmascript::MemSpaceKind::SHARED);
    set->SetLinkedSet(thread, linkedSet);
    JSHandle<JSTaggedValue> sharedSetTag = JSHandle<JSTaggedValue>::Cast(set);
    return JSNApiHelper::ToLocal<SendableSetRef>(sharedSetTag);
}

uint32_t SendableSetRef::GetSize(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedSet> set(JSNApiHelper::ToJSHandle(this));
    return ecmascript::JSSharedSet::GetSize(thread, set);
}

uint32_t SendableSetRef::GetTotalElements(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedSet> set(JSNApiHelper::ToJSHandle(this));
    return static_cast<int>(ecmascript::JSSharedSet::GetSize(thread, set)) +
           LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> SendableSetRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(
        JSHandle<JSTaggedValue>(thread, ecmascript::JSSharedSet::GetValue(thread, set, entry)));
}

void SendableSetRef::Add(const EcmaVM *vm, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, FATAL);
    ecmascript::JSSharedSet::Add(thread, set, JSNApiHelper::ToJSHandle(value));
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSMapIterator> jsMapIter(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsMapIter, FATAL);
    IterationKind iterKind = jsMapIter->GetIterationKind();
    Local<JSValueRef> result;
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    switch (iterKind) {
        case IterationKind::KEY:
            result = JSNApiHelper::ToLocal<JSValueRef>(globalConst->GetHandledKeysString());
            break;
        case IterationKind::VALUE:
            result = JSNApiHelper::ToLocal<JSValueRef>(globalConst->GetHandledValuesString());
            break;
        case IterationKind::KEY_AND_VALUE:
            result = JSNApiHelper::ToLocal<JSValueRef>(globalConst->GetHandledEntriesString());
            break;
        default:
            break;
    }
    return result;
}

Local<MapIteratorRef> MapIteratorRef::New(const EcmaVM *vm, Local<MapRef> map)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTaggedValue> nextTagValResult(vm->GetJSThread(), JSMapIterator::Next(ecmaRuntimeCallInfo));
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> iteratorVal(vm->GetJSThread(),
        JSIterator::IteratorValue(vm->GetJSThread(), nextTagValResult).GetTaggedValue());
    return JSNApiHelper::ToLocal<ArrayRef>(iteratorVal);
}

Local<JSValueRef> MapIteratorRef::Next(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTaggedValue> mapIter(JSNApiHelper::ToJSHandle(this));
    auto result = JSMapIterator::NextInternal(thread, mapIter);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, result));
}

// SendableMapIteratorRef
Local<JSValueRef> SendableMapIteratorRef::Next(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTaggedValue> sharedMapIter(JSNApiHelper::ToJSHandle(this));
    auto result = ecmascript::JSSharedMapIterator::NextInternal(thread, sharedMapIter);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, result));
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSSetIterator> jsSetIter(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsSetIter, FATAL);
    IterationKind iterKind = jsSetIter->GetIterationKind();
    Local<JSValueRef> result;
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    switch (iterKind) {
        case IterationKind::KEY:
            result = JSNApiHelper::ToLocal<JSValueRef>(globalConst->GetHandledKeysString());
            break;
        case IterationKind::VALUE:
            result = JSNApiHelper::ToLocal<JSValueRef>(globalConst->GetHandledValuesString());
            break;
        case IterationKind::KEY_AND_VALUE:
            result = JSNApiHelper::ToLocal<JSValueRef>(globalConst->GetHandledEntriesString());
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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
    const EcmaVM *vm, void *buffer, int32_t length, const NativePointerCallback &deleter, void *data)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSArrayBuffer> arrayBuffer = factory->NewJSArrayBuffer(buffer, length, deleter, data);

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

int32_t BufferRef::ByteLength(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    return arrayBuffer->GetArrayBufferByteLength();
}

void *BufferRef::GetBuffer(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, nullptr);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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
    ecmascript::ThreadManagedScope managedScope(thread);
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
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    return JSNApiHelper::ToLocal<PromiseRef>(JSHandle<JSTaggedValue>(thread, capacity->GetPromise()));
}

bool PromiseCapabilityRef::Resolve(const EcmaVM *vm, uintptr_t value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSTaggedValue arg = *reinterpret_cast<JSTaggedValue *>(value);
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
    thread->CheckJSTaggedType(arg.GetRawData());
#endif
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    JSTaggedValue resolve = capacity->GetResolve();
    JSTaggedValue undefined = constants->GetUndefined();
    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    info->SetCallArg(arg);
    JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT(thread, false);

    thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    RETURN_VALUE_IF_ABRUPT(thread, false);
    thread->GetCurrentEcmaContext()->ClearKeptObjects();
    return true;
}

bool PromiseCapabilityRef::Resolve(const EcmaVM *vm, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(value);
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
    thread->CheckJSTaggedType(arg.GetTaggedValue().GetRawData());
#endif
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
    thread->GetCurrentEcmaContext()->ClearKeptObjects();
    return true;
}

bool PromiseCapabilityRef::Reject(const EcmaVM *vm, uintptr_t reason)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSTaggedValue arg = *reinterpret_cast<JSTaggedValue *>(reason);
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
    thread->CheckJSTaggedType(arg.GetRawData());
#endif
    JSHandle<PromiseCapability> capacity(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(capacity, FATAL);
    JSTaggedValue reject = capacity->GetReject();
    JSTaggedValue undefined = constants->GetUndefined();

    EcmaRuntimeCallInfo *info =
        ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefined, undefined, 1);
    RETURN_VALUE_IF_ABRUPT(thread, false);
    info->SetCallArg(arg);
    JSFunction::Call(info);
    RETURN_VALUE_IF_ABRUPT(thread, false);

    thread->GetCurrentEcmaContext()->ExecutePromisePendingJob();
    RETURN_VALUE_IF_ABRUPT(thread, false);
    thread->GetCurrentEcmaContext()->ClearKeptObjects();
    return true;
}

bool PromiseCapabilityRef::Reject(const EcmaVM *vm, Local<JSValueRef> reason)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    const GlobalEnvConstants *constants = thread->GlobalConstants();

    JSHandle<JSTaggedValue> arg = JSNApiHelper::ToJSHandle(reason);
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
    thread->CheckJSTaggedType(arg.GetTaggedValue().GetRawData());
#endif
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
    thread->GetCurrentEcmaContext()->ClearKeptObjects();
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

Local<StringRef> StringRef::NewFromUtf8WithoutStringTable(const EcmaVM *vm, const char *utf8, int length)
{
    // This only supports for napi_create_string_utf8
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    if (length < 0) {
        JSHandle<JSTaggedValue> current(factory->NewFromUtf8WithoutStringTable(utf8));
        return JSNApiHelper::ToLocal<StringRef>(current);
    }
    JSHandle<JSTaggedValue> current(factory->NewFromUtf8WithoutStringTable(reinterpret_cast<const uint8_t *>(utf8),
                                                                           length));
    return JSNApiHelper::ToLocal<StringRef>(current);
}

Local<StringRef> StringRef::NewFromUtf16WithoutStringTable(const EcmaVM *vm, const char16_t *utf16, int length)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    if (length < 0) {
        JSHandle<JSTaggedValue> current(factory->NewFromUtf16WithoutStringTable(utf16));
        return JSNApiHelper::ToLocal<StringRef>(current);
    }
    JSHandle<JSTaggedValue> current(factory->NewFromUtf16WithoutStringTable(reinterpret_cast<const uint16_t *>(utf16),
                                                                            length));
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

std::string StringRef::ToString(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, "");
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this)).ToStdString();
}

std::string StringRef::DebuggerToString(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, "");
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this)).DebuggerToStdString();
}

uint32_t StringRef::Length(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this)).GetLength();
}

size_t StringRef::Utf8Length(const EcmaVM *vm, bool isGetBufferSize)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<EcmaString> strHandle(vm->GetJSThread(), EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(this)));
    return EcmaStringAccessor(EcmaStringAccessor::Flatten(vm, strHandle)).GetUtf8Length(isGetBufferSize);
}

uint32_t StringRef::WriteUtf8(const EcmaVM *vm, char *buffer, uint32_t length, bool isWriteBuffer)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToFlatUtf8(reinterpret_cast<uint8_t *>(buffer), length, isWriteBuffer);
}

uint32_t StringRef::WriteUtf16(const EcmaVM *vm, char16_t *buffer, uint32_t length)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToUtf16(reinterpret_cast<uint16_t *>(buffer), length);
}

uint32_t StringRef::WriteLatin1(const EcmaVM *vm, char *buffer, uint32_t length)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return EcmaStringAccessor(JSNApiHelper::ToJSTaggedValue(this))
        .WriteToOneByte(reinterpret_cast<uint8_t *>(buffer), length);
}

Local<StringRef> StringRef::GetNapiWrapperString(const EcmaVM *vm)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> napiWapperString = thread->GlobalConstants()->GetHandledNapiWrapperString();
    return JSNApiHelper::ToLocal<StringRef>(napiWapperString);
}

Local<TypedArrayRef> StringRef::EncodeIntoUint8Array(const EcmaVM *vm)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> string = JSNApiHelper::ToJSHandle(this);
    uint32_t length = this->Utf8Length(vm, true);
    // 1 because Utf8Length adds 1 for the return value
    if (length <= 1) {
        return Undefined(vm);
    }

    JSHandle<JSObject> obj =
        TypedArrayHelper::FastCreateTypedArray(thread, thread->GlobalConstants()->GetHandledUint8ArrayString(),
                                               length - 1, DataViewType::UINT8);
    JSHandle<JSObject> arrayBuffer(thread, JSTypedArray::Cast(*obj)->GetViewedArrayBufferOrByteArray());
    JSTaggedValue bufferData = JSHandle<JSArrayBuffer>::Cast(arrayBuffer)->GetArrayBufferData();
    void *buffer = JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();

    JSHandle<EcmaString> stringHandle = JSHandle<EcmaString>::Cast(string);
    EcmaStringAccessor(stringHandle).WriteToFlatUtf8(reinterpret_cast<uint8_t*>(buffer), length - 1, true);
    JSHandle<JSTaggedValue> typedArrayTag = JSHandle<JSTaggedValue>::Cast(obj);
    return JSNApiHelper::ToLocal<TypedArrayRef>(typedArrayTag);
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

void BigIntRef::GetWordsArray(const EcmaVM *vm, bool* signBit, size_t wordCount, uint64_t* words)
{
    DCHECK_SPECIAL_VALUE(this);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<BigInt> bigintVal(JSNApiHelper::ToJSHandle(this));
    uint32_t len = bigintVal->GetLength();
    uint32_t count = 0;
    uint32_t index = 0;
    ASSERT(wordCount > 0);
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

uint32_t BigIntRef::GetWordsArraySize(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<BigInt> bigintVal(JSNApiHelper::ToJSHandle(this));
    uint32_t len = bigintVal->GetLength();
    return len % 2 != 0 ? len / 2 + 1 : len / 2; // 2 : len is odd or even
}

// ----------------------------------- HandleScope -------------------------------------
LocalScope::LocalScope(const EcmaVM *vm) : thread_(vm->GetJSThread())
{
    // Only get handle ptr here. Do not need to swtich state.
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
    prevNext_ = context->GetHandleScopeStorageNext();
    prevEnd_ = context->GetHandleScopeStorageEnd();
    prevHandleStorageIndex_ = context->GetCurrentHandleStorageIndex();

    prevPrimitiveNext_ = context->GetPrimitiveScopeStorageNext();
    prevPrimitiveEnd_ = context->GetPrimitiveScopeStorageEnd();
    prevPrimitiveStorageIndex_ = context->GetCurrentPrimitiveStorageIndex();
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    context->HandleScopeCountAdd();
    context->PrimitiveScopeCountAdd();
#endif
}

LocalScope::LocalScope(const EcmaVM *vm, JSTaggedType value) : thread_(vm->GetJSThread())
{
    ecmascript::ThreadManagedScope managedScope(reinterpret_cast<JSThread *>(thread_));
    // Simply reserve a slot on the handlescope. The escaped handle will still be retained in this slot.
    ecmascript::EcmaHandleScope::NewHandle(reinterpret_cast<JSThread *>(thread_), value);
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
    prevNext_ = context->GetHandleScopeStorageNext();
    prevEnd_ = context->GetHandleScopeStorageEnd();
    prevHandleStorageIndex_ = context->GetCurrentHandleStorageIndex();

    prevPrimitiveNext_ = context->GetPrimitiveScopeStorageNext();
    prevPrimitiveEnd_ = context->GetPrimitiveScopeStorageEnd();
    prevPrimitiveStorageIndex_ = context->GetCurrentPrimitiveStorageIndex();
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    context->HandleScopeCountAdd();
    context->PrimitiveScopeCountAdd();
#endif
}

LocalScope::~LocalScope()
{
    ecmascript::ThreadManagedScope managedScope(reinterpret_cast<JSThread *>(thread_));
    auto context = reinterpret_cast<JSThread *>(thread_)->GetCurrentEcmaContext();
#ifdef ECMASCRIPT_ENABLE_HANDLE_LEAK_CHECK
    context->HandleScopeCountDec();
    context->PrimitiveScopeCountDec();
#endif
    context->SetHandleScopeStorageNext(static_cast<JSTaggedType *>(prevNext_));
    context->SetPrimitiveScopeStorageNext(static_cast<JSTaggedType *>(prevPrimitiveNext_));

    if (context->GetHandleScopeStorageEnd() != prevEnd_) {
        context->SetHandleScopeStorageEnd(static_cast<JSTaggedType *>(prevEnd_));
        context->ShrinkHandleStorage(prevHandleStorageIndex_);
    }

    if (context->GetPrimitiveScopeStorageEnd() != prevPrimitiveEnd_) {
        context->SetPrimitiveScopeStorageEnd(static_cast<JSTaggedType *>(prevPrimitiveEnd_));
        context->ShrinkPrimitiveStorage(prevPrimitiveStorageIndex_);
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
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTaggedValue> integer(thread, JSTaggedValue(input));
    return JSNApiHelper::ToLocal<IntegerRef>(integer);
}

Local<IntegerRef> IntegerRef::NewFromUnsigned(const EcmaVM *vm, unsigned int input)
{
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(thread);
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

uintptr_t ObjectRef::NewObject(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm,
        (vm->GetJSThread()->GlobalConstants()->GetHandledUndefined()).GetAddress());
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSFunction> constructor(globalEnv->GetObjectFunction());
    JSHandle<JSTaggedValue> object(factory->NewJSObjectByConstructor(constructor));
    return object.GetAddress();
}

Local<ObjectRef> ObjectRef::NewS(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = vm->GetGlobalEnv();
    JSHandle<JSFunction> constructor(globalEnv->GetSObjectFunction());
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

Local<ObjectRef> ObjectRef::NewSWithProperties(const EcmaVM *vm, SendablePropertiesInfo &info)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    ObjectFactory *factory = vm->GetFactory();
    std::vector<PropertyDescriptor> descs;
    JSNapiSendable::InitWithPropertiesInfo(thread, info, descs);
    auto obj = factory->CreateSObjectWithProperties(descs);
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

Local<ObjectRef> ObjectRef::CreateNativeModuleFailureInfo(const EcmaVM *vm, const std::string &failureInfo)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (EcmaVM::GetErrorInfoEnhance()) {
        JSHandle<NativeModuleFailureInfo> nativeModuleErrorFailureInfo =
            NativeModuleFailureInfo::CreateNativeModuleFailureInfo(vm, failureInfo);
        return JSNApiHelper::ToLocal<ObjectRef>(JSHandle<JSTaggedValue>::Cast(nativeModuleErrorFailureInfo));
    }
    return JSValueRef::Undefined(vm);
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

Local<ObjectRef> ObjectRef::CreateSendableAccessorData(const EcmaVM *vm,
                                                       Local<FunctionRef> getter,
                                                       Local<FunctionRef> setter)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTaggedValue> getterValue = JSNApiHelper::ToJSHandle(getter);
    JSHandle<JSTaggedValue> setterValue = JSNApiHelper::ToJSHandle(setter);
    JSHandle<AccessorData> accessor = thread->GetEcmaVM()->GetFactory()->NewSAccessorData();
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

bool ObjectRef::Set(const EcmaVM *vm, const char *utf8, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
    LOG_IF_SPECIAL(obj, ERROR);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> keyValue(factory->NewFromUtf8(utf8));
    JSTaggedValue valueValue = JSNApiHelper::ToJSTaggedValue(*value);
    if (!obj->IsHeapObject()) {
        return JSTaggedValue::SetProperty(thread, obj, keyValue, JSHandle<JSTaggedValue>(thread, valueValue));
    }
    JSTaggedValue res = ObjectFastOperator::TrySetPropertyByNameThroughCacheAtLocal(thread, obj.GetTaggedValue(),
                                                                                    keyValue.GetTaggedValue(),
                                                                                    valueValue);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!res.IsHole()) {
        return !res.IsException();
    }
    if (!JSNApi::KeyIsNumber(utf8)) {
        res = ObjectFastOperator::SetPropertyByName(thread, obj.GetTaggedValue(), keyValue.GetTaggedValue(),
                                                    valueValue);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        if (!res.IsHole()) {
            return !res.IsException();
        }
        return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, obj.GetTaggedValue()), keyValue,
                                          JSHandle<JSTaggedValue>(thread, valueValue), true);
    }
    return ObjectFastOperator::FastSetPropertyByValue(thread, obj.GetTaggedValue(),
                                                      keyValue.GetTaggedValue(),
                                                      valueValue);
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
    JSTaggedValue result;
    {
        LocalScope scope(vm);
        JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
        LOG_IF_SPECIAL(obj, ERROR);
        JSHandle<JSTaggedValue> keyValue = JSNApiHelper::ToJSHandle(key);
        if (UNLIKELY(!obj->IsHeapObject())) {
            OperationResult ret = JSTaggedValue::GetProperty(thread, obj, keyValue);
            RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
            result = ret.GetValue().GetTaggedValue();
        } else {
            result = ObjectFastOperator::FastGetPropertyByValue(thread, obj.GetTaggedValue(),
                                                                keyValue.GetTaggedValue());
            RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        }
    }
    JSHandle<JSTaggedValue> resultValue(thread, result);
    return JSNApiHelper::ToLocal<JSValueRef>(resultValue);
}

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, const char *utf8)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue result;
    {
        LocalScope scope(vm);
        JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
        LOG_IF_SPECIAL(obj, ERROR);
        ObjectFactory *factory = vm->GetFactory();
        JSHandle<JSTaggedValue> keyValue(factory->NewFromUtf8(utf8));
        if (UNLIKELY(!obj->IsHeapObject())) {
            OperationResult ret = JSTaggedValue::GetProperty(thread, obj, keyValue);
            RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
            result = ret.GetValue().GetTaggedValue();
        } else {
            result = ObjectFastOperator::FastGetPropertyByValue(thread, obj.GetTaggedValue(),
                                                                keyValue.GetTaggedValue());
            RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        }
    }
    JSHandle<JSTaggedValue> resultValue(thread, result);
    return JSNApiHelper::ToLocal<JSValueRef>(resultValue);
}

Local<JSValueRef> ObjectRef::Get(const EcmaVM *vm, int32_t key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedValue result;
    {
        LocalScope scope(vm);
        JSHandle<JSTaggedValue> obj = JSNApiHelper::ToJSHandle(this);
        LOG_IF_SPECIAL(obj, ERROR);
        if (!obj->IsHeapObject()) {
            OperationResult ret = JSTaggedValue::GetProperty(thread, obj, key);
            RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
            result = ret.GetValue().GetTaggedValue();
        } else {
            result = ObjectFastOperator::FastGetPropertyByIndex(thread, obj.GetTaggedValue(), key);
            RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        }
    }
    JSHandle<JSTaggedValue> resultValue(thread, result);
    return JSNApiHelper::ToLocal<JSValueRef>(resultValue);
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

bool ObjectRef::HasOwnProperty(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> object(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(object, ERROR);
    JSHandle<JSTaggedValue> keyValue(JSNApiHelper::ToJSHandle(key));
    return JSTaggedValue::HasOwnProperty(thread, object, keyValue);
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

int32_t ObjectRef::GetNativePointerFieldCount(const EcmaVM *vm)
{
    // ObjectRef::New may return special value if exception occurs.
    // So we need do special value check before use it.
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    return object->GetNativePointerFieldCount();
}

void *ObjectRef::GetNativePointerField(const EcmaVM *vm, int32_t index)
{
    // ObjectRef::New may return special value if exception occurs.
    // So we need do special value check before use it.
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, nullptr);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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

void ObjectRef::SetConcurrentNativePointerField(const EcmaVM *vm, int32_t index, void *nativePointer,
    NativePointerCallback callBack, void *data, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    // ObjectRef::New may return special value if exception occurs.
    // So we need do special value check before use it.
    DCHECK_SPECIAL_VALUE(this);
    JSHandle<JSObject> object(JSNApiHelper::ToJSHandle(this));
    object->SetNativePointerField(thread, index, nativePointer, callBack, data, nativeBindingsize, Concurrent::YES);
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

Local<NativePointerRef> NativePointerRef::NewConcurrent(
    const EcmaVM *vm, void *nativePointer, NativePointerCallback callBack, void *data, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSNativePointer> obj =
        factory->NewJSNativePointer(nativePointer, callBack, data, false, nativeBindingsize, Concurrent::YES);
    return JSNApiHelper::ToLocal<NativePointerRef>(JSHandle<JSTaggedValue>(obj));
}

Local<NativePointerRef> NativePointerRef::NewSendable(
    const EcmaVM *vm, void *nativePointer, NativePointerCallback callBack, void *data, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSNativePointer> obj =
        factory->NewSJSNativePointer(nativePointer, callBack, data, false, nativeBindingsize);
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
    const EcmaVM *vm, void *buffer, int32_t length, const NativePointerCallback &deleter, void *data)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSArrayBuffer> arrayBuffer = factory->NewJSArrayBuffer(buffer, length, deleter, data);
    return JSNApiHelper::ToLocal<ArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

int32_t ArrayBufferRef::ByteLength(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(arrayBuffer, FATAL);
    return arrayBuffer->GetArrayBufferByteLength();
}

void *ArrayBufferRef::GetBuffer(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, nullptr);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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
    // arraybuffer is not shared. Do not need to switch state
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    arrayBuffer->Detach(thread);
}

bool ArrayBufferRef::IsDetach(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    return arrayBuffer->IsDetach();
}

Local<SendableArrayBufferRef> SendableArrayBufferRef::New(const EcmaVM *vm, int32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<ecmascript::JSSendableArrayBuffer> arrayBuffer = factory->NewJSSendableArrayBuffer(length);
    return JSNApiHelper::ToLocal<SendableArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

Local<SendableArrayBufferRef> SendableArrayBufferRef::New(
    const EcmaVM *vm, void *buffer, int32_t length, const NativePointerCallback &deleter, void *data)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<ecmascript::JSSendableArrayBuffer> arrayBuffer =
        factory->NewJSSendableArrayBuffer(buffer, length, deleter, data);
    return JSNApiHelper::ToLocal<SendableArrayBufferRef>(JSHandle<JSTaggedValue>(arrayBuffer));
}

int32_t SendableArrayBufferRef::ByteLength(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSendableArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(arrayBuffer, FATAL);
    return arrayBuffer->GetArrayBufferByteLength();
}

void SendableArrayBufferRef::Detach(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSendableArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    arrayBuffer->Detach(thread);
}

bool SendableArrayBufferRef::IsDetach(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSendableArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    return arrayBuffer->IsDetach();
}

void *SendableArrayBufferRef::GetBuffer(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, nullptr);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSendableArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue bufferData = arrayBuffer->GetArrayBufferData();
    if (!bufferData.IsJSNativePointer()) {
        return nullptr;
    }
    return JSNativePointer::Cast(bufferData.GetTaggedObject())->GetExternalPointer();
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

double DateRef::GetTime(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0.0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSDate> date(JSNApiHelper::ToJSHandle(this));
    if (!date->IsDate()) {
        LOG_ECMA(ERROR) << "Not a Date Object";
    }
    return date->GetTime().GetDouble();
}

// ---------------------------------- TypedArray -----------------------------------
uint32_t TypedArrayRef::ByteLength(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetByteLength();
}

uint32_t TypedArrayRef::ByteOffset(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetByteOffset();
}

uint32_t TypedArrayRef::ArrayLength(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetArrayLength();
}

Local<ArrayBufferRef> TypedArrayRef::GetArrayBuffer(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTypedArray> typeArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typeArray, ERROR);
    JSHandle<JSTaggedValue> arrayBuffer(thread, JSTypedArray::GetOffHeapBuffer(thread, typeArray));
    return JSNApiHelper::ToLocal<ArrayBufferRef>(arrayBuffer);
}

uint32_t SendableTypedArrayRef::ByteLength(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetByteLength();
}

uint32_t SendableTypedArrayRef::ByteOffset(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetByteOffset();
}

uint32_t SendableTypedArrayRef::ArrayLength(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<ecmascript::JSSharedTypedArray> typedArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typedArray, FATAL);
    return typedArray->GetArrayLength();
}

Local<SendableArrayBufferRef> SendableTypedArrayRef::GetArrayBuffer(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<ecmascript::JSSharedTypedArray> typeArray(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(typeArray, ERROR);
    JSHandle<JSTaggedValue> arrayBuffer(thread,
        ecmascript::JSSharedTypedArray::GetSharedOffHeapBuffer(thread, typeArray));
    return JSNApiHelper::ToLocal<SendableArrayBufferRef>(arrayBuffer);
}

// ----------------------------------- FunctionRef --------------------------------------
Local<FunctionRef> FunctionRef::New(EcmaVM *vm, FunctionCallback nativeFunc,
    NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)
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

Local<FunctionRef> FunctionRef::NewConcurrent(EcmaVM *vm, FunctionCallback nativeFunc,
    NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current(factory->NewJSFunction(env, reinterpret_cast<void *>(Callback::RegisterCallback)));
    current->SetFunctionExtraInfo(thread, reinterpret_cast<void *>(nativeFunc), deleter,
                                  data, nativeBindingsize, Concurrent::YES);
    current->SetCallNapi(callNapi);
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

Local<FunctionRef> FunctionRef::New(EcmaVM *vm, InternalFunctionCallback nativeFunc,
    NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)
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

Local<FunctionRef> FunctionRef::NewSendable(EcmaVM *vm,
                                            InternalFunctionCallback nativeFunc,
                                            NativePointerCallback deleter,
                                            void *data,
                                            bool callNapi,
                                            size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current(factory->NewSFunction(env, reinterpret_cast<void *>(nativeFunc)));
    current->SetSFunctionExtraInfo(thread, nullptr, deleter, data, nativeBindingsize);
    current->SetCallNapi(callNapi);
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

Local<FunctionRef> FunctionRef::NewConcurrent(EcmaVM *vm, InternalFunctionCallback nativeFunc,
    NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    JSHandle<JSFunction> current(factory->NewJSFunction(env, reinterpret_cast<void *>(nativeFunc)));
    current->SetFunctionExtraInfo(thread, nullptr, deleter, data, nativeBindingsize, Concurrent::YES);
    current->SetCallNapi(callNapi);
    return JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
}

static void InitClassFunction(EcmaVM *vm, JSHandle<JSFunction> &func, bool callNapi)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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
    NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)
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

Local<FunctionRef> FunctionRef::NewConcurrentClassFunction(EcmaVM *vm, InternalFunctionCallback nativeFunc,
    NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)
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
    current->SetFunctionExtraInfo(thread, nullptr, deleter, data, nativeBindingsize, Concurrent::YES);
    Local<FunctionRef> result = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(current));
    return scope.Escape(result);
}

Local<FunctionRef> FunctionRef::NewClassFunction(EcmaVM *vm, InternalFunctionCallback nativeFunc,
    NativePointerCallback deleter, void *data, bool callNapi, size_t nativeBindingsize)
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

Local<FunctionRef> FunctionRef::NewSendableClassFunction(const EcmaVM *vm,
                                                         InternalFunctionCallback nativeFunc,
                                                         NativePointerCallback deleter,
                                                         void *data,
                                                         Local<StringRef> name,
                                                         SendablePropertiesInfos &infos,
                                                         Local<FunctionRef> parent,
                                                         bool callNapi,
                                                         size_t nativeBindingSize)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    EscapeLocalScope scope(vm);
    ObjectFactory *factory = vm->GetFactory();

    bool hasParent = !parent->IsNull();
    JSNapiSendable sendable(thread, infos, name);
    JSHandle<JSHClass> prototypeHClass = JSHClass::CreateSPrototypeHClass(thread, sendable.GetNonStaticDescs());
    JSHandle<JSObject> prototype = factory->NewSharedOldSpaceJSObject(prototypeHClass);
    JSHandle<JSHClass> constructorHClass = JSHClass::CreateSConstructorHClass(thread, sendable.GetStaticDescs());
    JSHandle<JSFunction> constructor = factory->NewSFunctionByHClass(
        reinterpret_cast<void *>(nativeFunc), constructorHClass, ecmascript::FunctionKind::CLASS_CONSTRUCTOR);

    sendable.SetSConstructor(constructor);
    JSObject::SetSProperties(thread, prototype, sendable.GetNonStaticDescs());
    JSObject::SetSProperties(thread, JSHandle<JSObject>::Cast(constructor), sendable.GetStaticDescs());

    if (hasParent) {
        auto parentPrototype = parent->GetFunctionPrototype(vm);
        prototypeHClass->SetPrototype(thread, JSNApiHelper::ToJSHandle(parentPrototype));
        constructorHClass->SetPrototype(thread, JSNApiHelper::ToJSHandle(parent));
    }
    prototypeHClass->SetExtensible(false);
    constructor->SetHomeObject(thread, prototype);
    constructor->SetProtoOrHClass(thread, prototype);
    constructor->SetLexicalEnv(thread, constructor);
    constructor->SetCallNapi(callNapi);
    constructor->SetSFunctionExtraInfo(thread, nullptr, deleter, data, nativeBindingSize);

    JSHClass *parentIHClass{nullptr};
    if (hasParent) {
        JSHandle<JSFunction> parentHandle(JSNApiHelper::ToJSHandle(parent));
        parentIHClass = reinterpret_cast<JSHClass *>(parentHandle->GetProtoOrHClass().GetTaggedObject());
    }
    JSHandle<JSHClass> iHClass = JSHClass::CreateSHClass(thread, sendable.GetInstanceDescs(), parentIHClass);
    iHClass->SetPrototype(thread, JSHandle<JSTaggedValue>(prototype));
    iHClass->SetExtensible(false);
    constructor->SetProtoOrHClass(thread, iHClass);
    constructorHClass->SetExtensible(false);

    Local<FunctionRef> result = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(constructor));
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
    if (!IsFunction(vm)) {
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
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
        thread->CheckJSTaggedType(arg.GetTaggedValue().GetRawData());
#endif
        info->SetCallArg(i, arg.GetTaggedValue());
    }
    JSTaggedValue result = JSFunction::Call(info);
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
    thread->CheckJSTaggedType(result.GetRawData());
#endif
    if (thread->HasPendingException()) {
        ecmascript::JsStackInfo::BuildCrashInfo(thread);
    }
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> resultValue(thread, result);

    thread->GetCurrentEcmaContext()->ClearKeptObjects();
    vm->GetJsDebuggerManager()->NotifyReturnNative();
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(resultValue));
}

JSValueRef* FunctionRef::CallForNapi(const EcmaVM *vm, JSValueRef *thisObj,
    JSValueRef *const argv[],  // NOLINTNEXTLINE(modernize-avoid-c-arrays)
    int32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, *JSValueRef::Hole(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSTaggedValue result;
    FunctionCallScope callScope(EcmaVM::ConstCast(vm));
    ASSERT(IsFunction(vm)); // IsFunction check has been done in napi.
    {
        LocalScope scope(vm);
        ecmascript::tooling::JsDebuggerManager *dm = vm->GetJsDebuggerManager();
        bool isDebugApp = dm->IsDebugApp();
        if (isDebugApp) {
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
        RETURN_VALUE_IF_ABRUPT(thread, *JSValueRef::Hole(vm));
        for (int32_t i = 0; i < length; i++) {
            if (argv[i]) {
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
                thread->CheckJSTaggedType(JSNApiHelper::ToJSTaggedValue(argv[i]).GetRawData());
#endif
                // NewRuntimeCallInfo has set Undefined defaultly in Argv's slot.
                info->SetCallArg(i, JSNApiHelper::ToJSTaggedValue(argv[i]));
            }
        }
        if (LIKELY(thread->IsAsmInterpreter())) {
            STACK_LIMIT_CHECK(thread, reinterpret_cast<JSValueRef *>(*JSValueRef::Hole(vm)));
            auto *hclass = func.GetTaggedObject()->GetClass();
            if (hclass->IsClassConstructor()) {
                RETURN_STACK_BEFORE_THROW_IF_ASM(thread);
                THROW_TYPE_ERROR_AND_RETURN(thread, "class constructor cannot call",
                    reinterpret_cast<JSValueRef *>(*JSValueRef::Hole(vm)));
            }
            result = ecmascript::InterpreterAssembly::Execute(info);
        } else {
            result = JSFunction::Call(info);
        }
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
        thread->CheckJSTaggedType(result.GetRawData());
#endif
        RETURN_VALUE_IF_ABRUPT(thread, *JSValueRef::Hole(vm));
        if (thread->GetCurrentEcmaContext()->HasKeptObjects()) {
            thread->GetCurrentEcmaContext()->ClearKeptObjects();
        }
        if (isDebugApp && dm->IsMixedDebugEnabled()) {
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
    if (!IsFunction(vm)) {
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
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
        thread->CheckJSTaggedType(arg.GetTaggedValue().GetRawData());
#endif
        info->SetCallArg(i, arg.GetTaggedValue());
    }
    JSTaggedValue result = JSFunction::Construct(info);
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
    thread->CheckJSTaggedType(result.GetRawData());
#endif
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
    ASSERT(IsFunction(vm)); // IsFunction check has been done in napi.
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
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
                thread->CheckJSTaggedType(arg.GetRawData());
#endif
            info->SetCallArg(i, arg);
        }
        result = JSFunction::ConstructInternal(info);
        RETURN_VALUE_IF_ABRUPT(thread, *JSValueRef::Undefined(vm));
    }
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
    thread->CheckJSTaggedType(result.GetRawData());
#endif
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
        ecmascript::CString recordName = function->GetRecordName();
        ASSERT(!recordName.empty());
        entry = recordName;
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

void FunctionRef::SetData(const EcmaVM *vm, void *data, NativePointerCallback deleter, [[maybe_unused]] bool callNapi)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> funcValue = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSFunction> function(funcValue);
    if (function->IsJSShared()) {
        function->SetSFunctionExtraInfo(thread, nullptr, deleter, data, 0);
    } else {
        function->SetFunctionExtraInfo(thread, nullptr, deleter, data, 0);
    }
}

void* FunctionRef::GetData(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, nullptr);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> funcValue = JSNApiHelper::ToJSHandle(this);
    JSHandle<JSFunctionBase> function(funcValue);
    JSTaggedValue extraInfoValue = function->GetFunctionExtraInfo();
    if (!extraInfoValue.IsNativePointer()) {
        return nullptr;
    }
    auto extraInfo = JSNativePointer::Cast(extraInfoValue.GetTaggedObject());
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

uint32_t ArrayRef::Length(const EcmaVM *vm)
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

// ----------------------------------- SendableArrayRef ----------------------------------------
Local<SendableArrayRef> SendableArrayRef::New(const EcmaVM *vm, uint32_t length)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSTaggedNumber arrayLen(length);
    JSHandle<JSTaggedValue> array = ecmascript::JSSharedArray::ArrayCreate(thread, arrayLen);
    JSHandle<JSTaggedValue> initialValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; i++) {
        key.Update(JSTaggedValue(i));
        JSObject::CreateDataPropertyOrThrow(
            thread, JSHandle<JSObject>(array), key, initialValue, ecmascript::JSShared::SCheckMode::SKIP);
    }
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<SendableArrayRef>(array);
}

uint32_t SendableArrayRef::Length(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return ecmascript::JSSharedArray::Cast(JSNApiHelper::ToJSTaggedValue(this).GetTaggedObject())->GetArrayLength();
}

Local<JSValueRef> SendableArrayRef::GetValueAt(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> object = JSNApiHelper::ToJSHandle(obj);
    JSHandle<JSTaggedValue> result = ecmascript::JSSharedArray::FastGetPropertyByValue(thread, object, index);
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

bool SendableArrayRef::SetProperty(const EcmaVM *vm, Local<JSValueRef> obj, uint32_t index, Local<JSValueRef> value)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSTaggedValue> objectHandle = JSNApiHelper::ToJSHandle(obj);
    JSHandle<JSTaggedValue> valueHandle = JSNApiHelper::ToJSHandle(value);
    return ecmascript::JSSharedArray::SetProperty(
        thread, objectHandle, index, valueHandle, true, ecmascript::SCheckMode::CHECK);
}

// ---------------------------------- Error ---------------------------------------
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXCEPTION_ERROR_NEW(name, type)                                                                 \
    Local<JSValueRef> Exception::name(const EcmaVM *vm, Local<StringRef> message)                       \
    {                                                                                                   \
        CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));                    \
        ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());                                 \
        ObjectFactory *factory = vm->GetFactory();                                                      \
                                                                                                        \
        JSHandle<EcmaString> messageValue(JSNApiHelper::ToJSHandle(message));                           \
        JSHandle<JSTaggedValue> result(factory->NewJSError(ErrorType::type,                             \
                                                           messageValue, ecmascript::StackCheck::NO));  \
        return JSNApiHelper::ToLocal<JSValueRef>(result);                                               \
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

// ------------------------------------ JsiNativeScope -----------------------------------------------

JsiNativeScope::JsiNativeScope(const EcmaVM *vm)
{
    thread_ = vm->GetAssociatedJSThread();
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        const_cast<EcmaVM*>(vm)->IncreaseEnterJsiNativeScopeCount();
        const_cast<EcmaVM*>(vm)->IncreaseUpdateThreadStateTransCount();
    }
#endif
    oldThreadState_ = static_cast<uint16_t>(thread_->GetState());
    thread_->UpdateState(ecmascript::ThreadState::NATIVE);
}

JsiNativeScope::~JsiNativeScope()
{
    thread_->UpdateState(static_cast<ecmascript::ThreadState>(oldThreadState_));
}

// ------------------------------------ JsiFastNativeScope -----------------------------------------------

JsiFastNativeScope::JsiFastNativeScope(const EcmaVM *vm)
{
    thread_ = vm->GetAssociatedJSThread();
#if ECMASCRIPT_ENABLE_SCOPE_LOCK_STAT
    if (vm->IsCollectingScopeLockStats()) {
        const_cast<EcmaVM*>(vm)->IncreaseEnterFastNativeScopeCount();
        const_cast<EcmaVM*>(vm)->IncreaseUpdateThreadStateTransCount();
    }
#endif
    ecmascript::ThreadState oldState = thread_->GetState();
    if (oldState == ecmascript::ThreadState::RUNNING) {
        return;
    }
    oldThreadState_ = static_cast<uint16_t>(oldState);
    hasSwitchState_ = true;
    thread_->UpdateState(ecmascript::ThreadState::RUNNING);
}

JsiFastNativeScope::~JsiFastNativeScope()
{
    if (hasSwitchState_) {
        thread_->UpdateState(static_cast<ecmascript::ThreadState>(oldThreadState_));
    }
}

// ------------------------------------ JsiRuntimeCallInfo -----------------------------------------------
void *JsiRuntimeCallInfo::GetData()
{
    ecmascript::ThreadManagedScope managedScope(thread_);
    JSHandle<JSTaggedValue> constructor = BuiltinsBase::GetConstructor(reinterpret_cast<EcmaRuntimeCallInfo *>(this));
    if (!constructor->IsJSFunction()) {
        return nullptr;
    }
    JSHandle<JSFunctionBase> function(constructor);
    JSTaggedValue extraInfoValue = function->GetFunctionExtraInfo();
    if (!extraInfoValue.IsJSNativePointer()) {
        return nullptr;
    }
    return JSNativePointer::Cast(extraInfoValue.GetTaggedObject())->GetData();
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
                                 const std::string &patchFileName, uint8_t *patchBuffer, size_t patchSize,
                                 const std::string &baseFileName, uint8_t *baseBuffer, size_t baseSize)
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
    ecmascript::ThreadManagedScope managedScope(thread);
    ecmascript::QuickFixManager *quickFixManager = vm->GetQuickFixManager();
    JSHandle<JSTaggedValue> exceptionInfo = JSNApiHelper::ToJSHandle(exception);
    return quickFixManager->IsQuickFixCausedException(thread, exceptionInfo, patchFileName);
}

/*
 * register quickfix query function.
 */
void JSNApi::RegisterQuickFixQueryFunc(EcmaVM *vm, std::function<bool(std::string baseFileName,
                        std::string &patchFileName,
                        uint8_t **patchBuffer,
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

bool JSNApi::IsNormalizedOhmUrlPack(EcmaVM *vm)
{
    return vm->IsNormalizedOhmUrlPack();
}

bool JSNApi::IsOhmUrl(const std::string &srcName)
{
    return ModulePathHelper::IsOhmUrl(srcName.c_str());
}

void JSNApi::SetModuleInfo(EcmaVM *vm, const std::string &assetPath, const std::string &entryPoint)
{
    SetAssetPath(vm, assetPath);
    size_t pos = entryPoint.find_first_of("/");
    if (pos != std::string::npos) {
        SetBundleName(vm, entryPoint.substr(0, pos));
        ecmascript::CString moduleName = ModulePathHelper::GetModuleName(entryPoint.c_str());
        if (!moduleName.empty()) {
            SetModuleName(vm, moduleName.c_str());
            return;
        }
    }
    std::string errmsg = "SetModuleInfo: entryPoint:" + entryPoint + "is invalid.";
    LOG_ECMA(ERROR) << errmsg;
    Local<StringRef> message = StringRef::NewFromUtf8(vm, errmsg.c_str());
    Local<JSValueRef> error = Exception::Error(vm, message);
    JSNApi::ThrowException(vm, error);
}

// note: The function SetAssetPath is a generic interface for previewing and physical machines.
void JSNApi::SetAssetPath(EcmaVM *vm, const std::string &assetPath)
{
    ecmascript::CString path = assetPath.c_str();
    // check input assetPath
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
    if (!ModulePathHelper::ValidateAbcPath(path, ecmascript::ValidateFilePath::ABC)) {
        LOG_FULL(FATAL) << "Invalid input assetPath: " << assetPath.c_str();
    }
#endif
    vm->SetAssetPath(path);
}

void JSNApi::SetLoop(EcmaVM *vm, void *loop)
{
    vm->SetLoop(loop);
}

void JSNApi::SetWeakFinalizeTaskCallback(EcmaVM *vm, const WeakFinalizeTaskCallback &callback)
{
    vm->GetAssociatedJSThread()->SetWeakFinalizeTaskCallback(callback);
}

void JSNApi::SetAsyncCleanTaskCallback(EcmaVM *vm, const NativePointerTaskCallback &callback)
{
    vm->GetAssociatedJSThread()->SetAsyncCleanTaskCallback(callback);
}

void JSNApi::SetTriggerGCTaskCallback(EcmaVM *vm, const TriggerGCTaskCallback& callback)
{
    vm->GetHeap()->GetIdleGCTrigger()->SetTriggerGCTaskCallback(callback);
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

void JSNApi::SetPkgAliasList(EcmaVM *vm, const std::map<std::string, std::string> &list)
{
    ecmascript::CMap<ecmascript::CString, ecmascript::CString> pkgAliasList;
    for (auto it = list.begin(); it != list.end(); ++it) {
        pkgAliasList.emplace(it->first.c_str(), it->second.c_str());
    }
    vm->SetPkgAliasList(pkgAliasList);
}

void JSNApi::SetPkgNameList(EcmaVM *vm, const std::map<std::string, std::string> &list)
{
    ecmascript::CMap<ecmascript::CString, ecmascript::CString> pkgNameList;
    for (auto it = list.begin(); it != list.end(); ++it) {
        pkgNameList.emplace(it->first.c_str(), it->second.c_str());
    }
    vm->SetPkgNameList(pkgNameList);
}
std::string JSNApi::GetPkgName(EcmaVM *vm, const std::string &moduleName)
{
    return vm->GetPkgName(moduleName.c_str()).c_str();
}

void JSNApi::SetpkgContextInfoList(EcmaVM *vm, const std::map<std::string,
    std::vector<std::vector<std::string>>> &list)
{
    ecmascript::CMap<ecmascript::CString, ecmascript::CMap<ecmascript::CString,
        ecmascript::CVector<ecmascript::CString>>> pkgContextInfoList;
    for (auto it = list.begin(); it != list.end(); it++) {
        const std::vector<std::vector<std::string>> vec = it->second;
        ecmascript::CMap<ecmascript::CString, ecmascript::CVector<ecmascript::CString>> map;
        for (size_t i = 0; i < vec.size(); i++) {
            ecmascript::CString pkgName = vec[i][0].c_str();
            ecmascript::CVector<ecmascript::CString> pkgContextInfo;
            for (size_t j = 1; j < vec[i].size(); j++) {
                pkgContextInfo.emplace_back(vec[i][j].c_str());
            }
            map.emplace(pkgName, pkgContextInfo);
        }
        pkgContextInfoList.emplace(it->first.c_str(), map);
    }
    vm->SetpkgContextInfoList(pkgContextInfoList);
}

// Only used for env created by napi to set module execution mode
void JSNApi::SetExecuteBufferMode(const EcmaVM *vm)
{
    ecmascript::ModuleManager *moduleManager =
        vm->GetAssociatedJSThread()->GetCurrentEcmaContext()->GetModuleManager();
    moduleManager->SetExecuteMode(ecmascript::ModuleExecuteMode::ExecuteBufferMode);
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
    transFunc->SetTaskConcurrentFuncFlag(1); // 1 : concurrent function flag
    thread->SetTaskInfo(reinterpret_cast<uintptr_t>(taskInfo));
    thread->SetIsInConcurrentScope(true);
    return true;
}

void* JSNApi::GetCurrentTaskInfo(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, nullptr);
    return reinterpret_cast<void*>(thread->GetTaskInfo());
}

void JSNApi::ClearCurrentTaskInfo(const EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    thread->SetTaskInfo(reinterpret_cast<uintptr_t>(nullptr));
    thread->SetIsInConcurrentScope(false);
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return vm->GetCurrentModuleInfo(needRecordName);
}

std::string JSNApi::NormalizePath(const std::string &string)
{
    return PathHelper::NormalizePath(string.c_str()).c_str();
}

// Enable cross thread execution.
void JSNApi::AllowCrossThreadExecution(EcmaVM *vm)
{
    LOG_ECMA(WARN) << "enable cross thread execution";
    vm->GetAssociatedJSThread()->EnableCrossThreadExecution();
}

void* JSNApi::GetEnv(EcmaVM *vm)
{
    JSThread *thread = vm->GetJSThread();
    return thread->GetEnv();
}

void JSNApi::SetEnv(EcmaVM *vm, void *env)
{
    JSThread *thread = vm->GetJSThread();
    thread->SetEnv(env);
}

void JSNApi::SynchronizVMInfo(EcmaVM *vm, const EcmaVM *hostVM)
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    vm->SetBundleName(hostVM->GetBundleName());
    vm->SetModuleName(hostVM->GetModuleName());
    vm->SetAssetPath(hostVM->GetAssetPath());
    vm->SetIsBundlePack(hostVM->IsBundlePack());
    vm->SetPkgNameList(hostVM->GetPkgNameList());
    vm->SetPkgAliasList(hostVM->GetPkgAliasList());
    vm->SetpkgContextInfoList(hostVM->GetPkgContextInfoLit());

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
        ecmascript::ThreadManagedScope managedScope(thread);
        getStackBeforeCallNapiSuccess = vm->GetProfiler()->GetStackBeforeCallNapi(thread);
    }
#endif
}

void JSNApi::GetStackAfterCallNapi([[maybe_unused]] EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    JSThread *thread = vm->GetJSThread();
    if (thread->GetIsProfiling()) {
        ecmascript::ThreadManagedScope managedScope(thread);
        vm->GetProfiler()->GetStackAfterCallNapi(thread);
    }
#endif
}

EcmaVM *JSNApi::CreateJSVM(const RuntimeOption &option)
{
    JSRuntimeOptions runtimeOptions;
    runtimeOptions.SetArkProperties(option.GetArkProperties());
    runtimeOptions.SetMemConfigProperty(option.GetMemConfigProperty());
    runtimeOptions.SetArkBundleName(option.GetArkBundleName());
    runtimeOptions.SetLongPauseTime(option.GetLongPauseTime());
    runtimeOptions.SetGcThreadNum(option.GetGcThreadNum());
    runtimeOptions.SetIsWorker(option.GetIsWorker());
    runtimeOptions.SetIsRestrictedWorker(option.GetIsRestrictedWorker());
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
    EcmaVM::Destroy(ecmaVm);
}

void JSNApi::RegisterUncatchableErrorHandler(EcmaVM *ecmaVm, const UncatchableErrorHandler &handler)
{
    ecmaVm->RegisterUncatchableErrorHandler(handler);
}

void JSNApi::TriggerGC(const EcmaVM *vm, TRIGGER_GC_TYPE gcType)
{
    TriggerGC(vm, ecmascript::GCReason::EXTERNAL_TRIGGER, gcType);
}

void JSNApi::TriggerGC(const EcmaVM *vm, ecmascript::GCReason reason, TRIGGER_GC_TYPE gcType)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (thread != nullptr && vm->IsInitialized()) {
#if defined(ENABLE_EXCEPTION_BACKTRACE)
    if (thread->IsMainThreadFast()) {
        LOG_ECMA(INFO) << "JSNApi::TriggerGC gcType: " << static_cast<int>(gcType);
        std::ostringstream stack;
        ecmascript::Backtrace(stack, true);
        LOG_ECMA(INFO) << stack.str();
    }
#endif
        auto sHeap = ecmascript::SharedHeap::GetInstance();
        switch (gcType) {
            case TRIGGER_GC_TYPE::SEMI_GC:
                vm->CollectGarbage(vm->GetHeap()->SelectGCType(), reason);
                break;
            case TRIGGER_GC_TYPE::OLD_GC:
                vm->CollectGarbage(ecmascript::TriggerGCType::OLD_GC, reason);
                break;
            case TRIGGER_GC_TYPE::FULL_GC:
                vm->CollectGarbage(ecmascript::TriggerGCType::FULL_GC, reason);
                break;
            case TRIGGER_GC_TYPE::SHARED_GC:
                sHeap->CollectGarbage<ecmascript::TriggerGCType::SHARED_GC,
                    ecmascript::GCReason::EXTERNAL_TRIGGER>(thread);
                break;
            case TRIGGER_GC_TYPE::SHARED_FULL_GC:
                sHeap->CollectGarbage<ecmascript::TriggerGCType::SHARED_FULL_GC,
                    ecmascript::GCReason::EXTERNAL_TRIGGER>(thread);
                break;
            default:
                break;
        }
    }
}

void JSNApi::HintGC(const EcmaVM *vm, MemoryReduceDegree degree, ecmascript::GCReason reason)
{
    CROSS_THREAD_CHECK(vm);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    if (thread != nullptr && vm->IsInitialized()) {
        const_cast<ecmascript::Heap *>(vm->GetHeap())->CheckAndTriggerHintGC(degree, reason);
    }
}

void JSNApi::TriggerIdleGC(const EcmaVM *vm, TRIGGER_IDLE_GC_TYPE gcType)
{
    CROSS_THREAD_CHECK(vm);
    if (thread != nullptr && vm->IsInitialized()) {
        ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
        vm->GetHeap()->GetIdleGCTrigger()->TryTriggerIdleGC(gcType);
    }
}

void JSNApi::SetStartIdleMonitorCallback(const StartIdleMonitorCallback& callback)
{
    startIdleMonitorCallback_ = callback;
}

StartIdleMonitorCallback JSNApi::GetStartIdleMonitorCallback()
{
    return startIdleMonitorCallback_;
}

void JSNApi::ThrowException(const EcmaVM *vm, Local<JSValueRef> error)
{
    auto thread = vm->GetJSThread();
    ecmascript::ThreadManagedScope managedScope(thread);
    if (thread->HasPendingException()) {
        LOG_ECMA(DEBUG) << "An exception has already occurred before, keep old exception here.";
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

#if defined(ECMASCRIPT_SUPPORT_DEBUGGER) && !defined(PANDA_TARGET_IOS)
bool JSNApi::StartDebuggerCheckParameters(EcmaVM *vm, const DebugOption &option, int32_t instanceId,
                                          const DebuggerPostTask &debuggerPostTask)
{
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "[StartDebugger] vm is nullptr";
        return false;
    }

    if (option.port < 0) {
        LOG_ECMA(ERROR) << "[StartDebugger] option.port is -1" ;
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    const auto &handler = vm->GetJsDebuggerManager()->GetDebugLibraryHandle();
    if (handler.IsValid()) {
        LOG_ECMA(ERROR) << "[StartDebugger] handler has already loaded";
        return false;
    }
    if (option.libraryPath == nullptr) {
        LOG_ECMA(ERROR) << "[StartDebugger] option.libraryPath is nullptr";
        return false;
    }
    auto handle = panda::os::library_loader::Load(std::string(option.libraryPath));
    if (!handle) {
        LOG_ECMA(ERROR) << "[StartDebugger] Load library fail: " << option.libraryPath << " " << errno;
        return false;
    }
    auto sym = panda::os::library_loader::ResolveSymbol(handle.Value(), "StartDebug");
    if (!sym) {
        LOG_ECMA(ERROR) << "[StartDebugger] Resolve symbol fail: " << sym.Error().ToString();
        return false;
    }
    using StartDebugger = bool (*)(
        const std::string &, EcmaVM *, bool, int32_t, const DebuggerPostTask &, int);

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
}
#endif

// for previewer, cross platform and testcase debugger
bool JSNApi::StartDebugger([[maybe_unused]] EcmaVM *vm, [[maybe_unused]] const DebugOption &option,
                           [[maybe_unused]] int32_t instanceId,
                           [[maybe_unused]] const DebuggerPostTask &debuggerPostTask)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
#if !defined(PANDA_TARGET_IOS)
    LOG_ECMA(INFO) << "JSNApi::StartDebugger, isDebugMode = " << option.isDebugMode
        << ", port = " << option.port << ", instanceId = " << instanceId;
    return StartDebuggerCheckParameters(vm, option, instanceId, debuggerPostTask);
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

// rk
// FA or Stage
bool JSNApi::StartDebuggerForOldProcess([[maybe_unused]] EcmaVM *vm, [[maybe_unused]] const DebugOption &option,
                                        [[maybe_unused]] int32_t instanceId,
                                        [[maybe_unused]] const DebuggerPostTask &debuggerPostTask)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
#if !defined(PANDA_TARGET_IOS)
    LOG_ECMA(INFO) << "JSNApi::StartDebuggerForOldProcess, isDebugMode = " << option.isDebugMode
        << ", instanceId = " << instanceId;
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "[StartDebuggerForOldProcess] vm is nullptr";
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
        LOG_ECMA(ERROR) << "[StartDebuggerForOldProcess] vm is nullptr";
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

// ohos or emulator
// FA or Stage
bool JSNApi::StartDebuggerForSocketPair([[maybe_unused]] int tid, [[maybe_unused]] int socketfd)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    LOG_ECMA(INFO) << "JSNApi::StartDebuggerForSocketPair, tid = " << tid << ", socketfd = " << socketfd;
    JsDebuggerManager *jsDebuggerManager = JsDebuggerManager::GetJsDebuggerManager(tid);
    if (jsDebuggerManager == nullptr) {
        LOG_ECMA(ERROR) << "[StartDebuggerForSocketPair] jsDebuggerManager is nullptr";
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

// ohos or emulator
// FA or Stage
// release or debug hap : aa start
//                        aa start -D
//                        aa start -p
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
        LOG_ECMA(ERROR) << "[NotifyDebugMode] vm is nullptr";
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);

    bool ret = false;
    if (!debugApp) {
        return true;
    }

    if (option.libraryPath == nullptr) {
        LOG_ECMA(ERROR) << "[NotifyDebugMode] option.libraryPath is nullptr";
        return false;
    }
    JsDebuggerManager *jsDebuggerManager = vm->GetJsDebuggerManager();
    auto handle = panda::os::library_loader::Load(std::string(option.libraryPath));
    if (!handle) {
        LOG_ECMA(ERROR) << "[NotifyDebugMode] Load library fail: " << option.libraryPath << " " << errno;
        return false;
    }
    JsDebuggerManager::AddJsDebuggerManager(tid, jsDebuggerManager);
    jsDebuggerManager->SetDebugLibraryHandle(std::move(handle.Value()));
    jsDebuggerManager->SetDebugMode(option.isDebugMode && debugApp);
    jsDebuggerManager->SetIsDebugApp(debugApp);
#ifdef PANDA_TARGET_ARM32
    ret = StartDebuggerForOldProcess(vm, option, instanceId, debuggerPostTask);
#else
    ret = true;
#endif

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

#ifndef PANDA_TARGET_ARM32
    // Initialize debugger
    using InitializeDebuggerForSocketpair = bool(*)(void*);
    auto sym = panda::os::library_loader::ResolveSymbol(
        jsDebuggerManager->GetDebugLibraryHandle(), "InitializeDebuggerForSocketpair");
    if (!sym) {
        LOG_ECMA(ERROR) << "[NotifyDebugMode] Resolve InitializeDebuggerForSocketpair symbol fail: "
            << sym.Error().ToString();
        return false;
    }
    if (!reinterpret_cast<InitializeDebuggerForSocketpair>(sym.Value())(vm)) {
        LOG_ECMA(ERROR) << "[NotifyDebugMode] InitializeDebuggerForSocketpair fail";
        return false;
    }
#endif

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
    auto anFileDataMgr = ecmascript::AnFileDataManager::GetInstance();
    if (anFileDataMgr != nullptr && anFileDataMgr->SafeGetStubFileInfo()) {
        anFileDataMgr->SafeGetStubFileInfo()->RegisterToDebugger();
    }
    return ret;

#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
    return false;
#endif // ECMASCRIPT_SUPPORT_DEBUGGER
}

bool JSNApi::StoreDebugInfo([[maybe_unused]] int tid,
                            [[maybe_unused]] EcmaVM *vm,
                            [[maybe_unused]] const DebugOption &option,
                            [[maybe_unused]] const DebuggerPostTask &debuggerPostTask,
                            [[maybe_unused]] bool debugApp)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    LOG_ECMA(INFO) << "JSNApi::StoreDebugInfo, tid = " << tid;
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "[StoreDebugInfo] vm is nullptr";
        return false;
    }

    JsDebuggerManager *jsDebuggerManager = vm->GetJsDebuggerManager();
    const auto &handler = jsDebuggerManager->GetDebugLibraryHandle();
    if (handler.IsValid()) {
        LOG_ECMA(INFO) << "[StoreDebugInfo] handler has already loaded";
        return false;
    }

    if (option.libraryPath == nullptr) {
        LOG_ECMA(ERROR) << "[StoreDebugInfo] option.libraryPath is nullptr";
        return false;
    }
    auto handle = panda::os::library_loader::Load(std::string(option.libraryPath));
    if (!handle) {
        LOG_ECMA(ERROR) << "[StoreDebugInfo] Load library fail: " << option.libraryPath << " " << errno;
        return false;
    }
    JsDebuggerManager::AddJsDebuggerManager(tid, jsDebuggerManager);
    jsDebuggerManager->SetDebugLibraryHandle(std::move(handle.Value()));
    jsDebuggerManager->SetDebugMode(option.isDebugMode && debugApp);
    jsDebuggerManager->SetIsDebugApp(debugApp);
    // store debugger postTask in inspector.
    using StoreDebuggerInfo = void (*)(int, EcmaVM *, const DebuggerPostTask &);
    auto symOfStoreDebuggerInfo = panda::os::library_loader::ResolveSymbol(
        jsDebuggerManager->GetDebugLibraryHandle(), "StoreDebuggerInfo");
    if (!symOfStoreDebuggerInfo) {
        LOG_ECMA(ERROR) << "[StoreDebugInfo] Resolve StoreDebuggerInfo symbol fail: " <<
            symOfStoreDebuggerInfo.Error().ToString();
        return false;
    }
    reinterpret_cast<StoreDebuggerInfo>(symOfStoreDebuggerInfo.Value())(tid, vm, debuggerPostTask);
    bool ret = false;
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
    LOG_ECMA(DEBUG) << "JSNApi::StopDebugger";
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "[StopDebugger] vm is nullptr";
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);

    const auto &handle = vm->GetJsDebuggerManager()->GetDebugLibraryHandle();

    using StopDebug = void (*)(void *);

    auto sym = panda::os::library_loader::ResolveSymbol(handle, "StopDebug");
    if (!sym) {
        LOG_ECMA(ERROR) << sym.Error().ToString();
        return false;
    }

    reinterpret_cast<StopDebug>(sym.Value())(vm);

    vm->GetJsDebuggerManager()->SetDebugMode(false);
    uint32_t tid = vm->GetTid();
    JsDebuggerManager::DeleteJsDebuggerManager(tid);
    return true;
#else
    if (vm == nullptr) {
        LOG_ECMA(ERROR) << "[StopDebugger] vm is nullptr";
        return false;
    }
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);

    OHOS::ArkCompiler::Toolchain::StopDebug(vm);
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
    LOG_ECMA(DEBUG) << "JSNApi::StopDebugger, tid = " << tid;
    JsDebuggerManager *jsDebuggerManager = JsDebuggerManager::GetJsDebuggerManager(tid);
    if (jsDebuggerManager == nullptr) {
        LOG_ECMA(ERROR) << "[StopDebugger] jsDebuggerManager is nullptr";
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
    return vm->GetJsDebuggerManager()->IsMixedDebugEnabled();
#else
    return false;
#endif
}

bool JSNApi::IsDebugModeEnabled([[maybe_unused]] const EcmaVM *vm)
{
#if defined(ECMASCRIPT_SUPPORT_DEBUGGER)
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    if (vm != nullptr && vm->GetJsDebuggerManager() != nullptr) {
        return vm->GetJsDebuggerManager()->IsDebugMode();
    }
    return false;
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler debugger";
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

void JSNApi::NotifyUIIdle(const EcmaVM *vm, [[maybe_unused]] int idleTime)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    vm->GetHeap()->GetIdleGCTrigger()->NotifyVsyncIdleStart();
}

void JSNApi::NotifyLooperIdleStart(const EcmaVM *vm, int64_t timestamp, int idleTime)
{
    if (vm->IsPostForked()) {
        vm->GetHeap()->GetIdleGCTrigger()->NotifyLooperIdleStart(timestamp, idleTime);
    }
}

void JSNApi::NotifyLooperIdleEnd(const EcmaVM *vm, int64_t timestamp)
{
    if (vm->IsPostForked()) {
        vm->GetHeap()->GetIdleGCTrigger()->NotifyLooperIdleEnd(timestamp);
    }
}

bool JSNApi::IsJSMainThreadOfEcmaVM(const EcmaVM *vm)
{
    return vm->GetJSThread()->IsMainThreadFast();
}

void JSNApi::SetDeviceDisconnectCallback(EcmaVM *vm, DeviceDisconnectCallback cb)
{
    vm->SetDeviceDisconnectCallback(cb);
}

bool JSNApi::KeyIsNumber(const char* utf8)
{
    const char *ptr = utf8;
    for (char c = *ptr; c; c = *++ptr) {
        if (c >= '0' && c <= '9') {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

bool JSNApi::IsSerializationTimeoutCheckEnabled(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    // Open Control Timeout Consumption
    if (const_cast<EcmaVM *>(vm)->GetJSOptions().EnableSerializationTimeoutCheck()) {
        return thread->IsMainThread();
    }

    // Currently only log trace on main thread
    auto jsDebuggerManager = vm->GetJsDebuggerManager();
    if (jsDebuggerManager != nullptr) {
        if (jsDebuggerManager->IsSerializationTimeoutCheckEnabled()) {
            return thread->IsMainThread();
        }
    }
    return false;
}

void JSNApi::GenerateTimeoutTraceIfNeeded(const EcmaVM *vm, std::chrono::system_clock::time_point &start,
                                          std::chrono::system_clock::time_point &end, bool isSerialization)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope scope(thread);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto threshold = std::chrono::duration_cast<std::chrono::milliseconds>
            (std::chrono::milliseconds(vm->GetJsDebuggerManager()->GetSerializationCheckThreshold())).count();
    LOG_ECMA(DEBUG) << "JSNAPI::" << (isSerialization ? "SerializeValue" : "DeserializeValue") << " tid: "
        << thread->GetThreadId() << " threshold: " << threshold << " duration: " << duration;
    if (duration >= threshold) {
        std::stringstream tagMsg;
        auto startTimeMS = std::chrono::time_point_cast<std::chrono::nanoseconds>(start);
        tagMsg << (isSerialization ? "SerializationTimeout::tid=" : "DeserializationTimeout::tid=");
        tagMsg << thread->GetThreadId();
        tagMsg << (isSerialization ? ";task=serialization;startTime=" : ";task=deserialization;startTime=");
        tagMsg << startTimeMS.time_since_epoch().count() << ";duration=" << duration;
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, tagMsg.str());
    }
}

void JSNApi::LoadAotFileInternal(EcmaVM *vm, const std::string &moduleName, std::string &aotFileName)
{
    if (vm->GetJSOptions().WasAOTOutputFileSet()) {
        aotFileName = vm->GetJSOptions().GetAOTOutputFile();
    }
#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
    else if (vm->GetJSOptions().GetEnableAOT())
#else
    else if (ecmascript::AnFileDataManager::GetInstance()->IsEnable())
#endif
    {
        aotFileName = ecmascript::AnFileDataManager::GetInstance()->GetDir() + moduleName;
    } else {
        std::string hapPath = "";
        ecmascript::SearchHapPathCallBack callback = vm->GetSearchHapPathCallBack();
        if (callback) {
            callback(moduleName, hapPath);
        }
        aotFileName = ecmascript::OhosPreloadAppInfo::GetPreloadAOTFileName(hapPath, moduleName);
    }
    if (aotFileName.empty()) {
        LOG_ECMA(INFO) << "can not find aot file";
        return;
    }
    if (ecmascript::pgo::PGOProfilerManager::GetInstance()->IsDisableAot()) {
        LOG_ECMA(INFO) << "can't load disable aot file: " << aotFileName;
        return;
    }
    LOG_ECMA(INFO) << "start to load aot file: " << aotFileName;
}

void JSNApi::LoadAotFile(EcmaVM *vm, const std::string &moduleName)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope scope(thread);

    std::string aotFileName;
    LoadAotFileInternal(vm, moduleName, aotFileName);
    // Disable PGO for applications when an/ai file exists
    if (isForked_) {
        vm->DisablePGOProfilerWithAOTFile(aotFileName);
    }
    thread->GetCurrentEcmaContext()->LoadAOTFiles(aotFileName);
}

#if defined(CROSS_PLATFORM) && defined(ANDROID_PLATFORM)
void JSNApi::LoadAotFile(EcmaVM *vm, [[maybe_unused]] const std::string &bundleName, const std::string &moduleName,
                         std::function<bool(std::string fileName, uint8_t **buff, size_t *buffSize)> cb)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope scope(thread);

    std::string aotFileName;
    LoadAotFileInternal(vm, moduleName, aotFileName);
    thread->GetCurrentEcmaContext()->LoadAOTFiles(aotFileName, cb);
}
#endif

bool JSNApi::ExecuteInContext(EcmaVM *vm, const std::string &fileName, const std::string &entry, bool needUpdate)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(DEBUG) << "start to execute ark file in context: " << fileName;
    ecmascript::ThreadManagedScope scope(thread);
    EcmaContext::MountContext(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromAbcFile(thread, fileName.c_str(), entry, needUpdate)) {
        if (thread->HasPendingException()) {
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute ark file '" << fileName
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    EcmaContext::UnmountContext(thread);
    return true;
}

// function for bundle abc
bool JSNApi::ExecuteForAbsolutePath(const EcmaVM *vm, const std::string &fileName, const std::string &entry,
                                    bool needUpdate, bool executeFromJob)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(DEBUG) << "start to execute absolute path ark file: " << fileName;
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromAbsolutePathAbcFile(
        thread, fileName.c_str(), entry, needUpdate, executeFromJob)) {
        if (thread->HasPendingException()) {
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute absolute path ark file '" << fileName
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    return true;
}

bool JSNApi::Execute(const EcmaVM *vm, const std::string &fileName, const std::string &entry,
                     bool needUpdate, bool executeFromJob)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(DEBUG) << "start to execute ark file: " << fileName;
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromAbcFile(
        thread, fileName.c_str(), entry, needUpdate, executeFromJob)) {
        if (thread->HasPendingException()) {
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
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
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute ark buffer file '" << filename
                        << "' with entry '" << entry << "'" << std::endl;
        return false;
    }
    return true;
}

int JSNApi::ExecuteWithSingletonPatternFlag(EcmaVM *vm, const std::string &bundleName,
    const std::string &moduleName, const std::string &ohmurl, bool isSingletonPattern)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, ecmascript::JSPandaFileExecutor::ROUTE_INTERNAL_ERROR);
    ecmascript::ThreadManagedScope scope(thread);
    int result = ecmascript::JSPandaFileExecutor::ExecuteAbcFileWithSingletonPatternFlag(thread, bundleName.c_str(),
        moduleName.c_str(), ohmurl.c_str(), isSingletonPattern);
    if (!result) {
        if (thread->HasPendingException()) {
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Execute with singleton-pattern flag failed with bundle name is'" << bundleName
                        << "' and module name is '" << moduleName << "', entry is'" << ohmurl << "'" << std::endl;
    }
    return result;
}

bool JSNApi::IsExecuteModuleInAbcFile(EcmaVM *vm, const std::string &bundleName,
    const std::string &moduleName, const std::string &ohmurl)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope scope(thread);
    bool result = ecmascript::JSPandaFileExecutor::IsExecuteModuleInAbcFile(thread, bundleName.c_str(),
        moduleName.c_str(), ohmurl.c_str());
    return result;
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
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute module buffer file '" << filename;
        return false;
    }
    return true;
}

/*
 * srcFilename: data/storage/el1/bundle/modulename/ets/modules.abc
 * ohmUrl :     1. @bundle:bundleName/moduleName@namespace/ets/pages/Index
 *              2. @package:pkg_modules/.ohpm/pkgName/pkg_modules/pkgName/xxx/xxx
 *              3. @normalized:N&moduleName&bundleName&entryPath&version
 *              4. @normalized:N&moduleName&bundleName&entryPath&
 */
bool JSNApi::ExecuteSecureWithOhmUrl(EcmaVM *vm, uint8_t *data, int32_t size, const std::string &srcFilename,
                                     const std::string &ohmUrl)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(INFO) << "start to execute ark buffer with secure memory use file: " << srcFilename <<
                      ", entrypoint: " << ohmUrl;
    ecmascript::ThreadManagedScope scope(thread);
    ecmascript::CString filename = PathHelper::NormalizePath(srcFilename.c_str());
    // check input filePath
#if !defined(PANDA_TARGET_WINDOWS) && !defined(PANDA_TARGET_MACOS)
    if (!ModulePathHelper::ValidateAbcPath(filename, ecmascript::ValidateFilePath::ETS_MODULES)) {
        LOG_FULL(FATAL) << "ExecuteSecureWithOhmUrl: Invalid input filePath: " << srcFilename <<
                           ", input OhmUrl:" << ohmUrl;
    }
#endif
    ecmascript::CString entryPoint;
    // Check and translate OhmUrl to recordName
    if (!ModulePathHelper::CheckAndGetRecordName(thread, ohmUrl.c_str(), entryPoint)) {
        LOG_FULL(FATAL) << "ExecuteSecureWithOhmUrl: Invalid input OhmUrl: " << ohmUrl <<
                           ", input filePath:" << filename;
    }
    if (!ecmascript::JSPandaFileExecutor::ExecuteSecureWithOhmUrl(thread, data, size, filename, entryPoint)) {
        if (thread->HasPendingException()) {
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
        }
        LOG_ECMA(ERROR) << "Cannot execute ark buffer file '" << srcFilename
                        << "' with entry '" << ohmUrl << "'" << std::endl;
        return false;
    }
    return true;
}

bool JSNApi::ExecuteSecure(EcmaVM *vm, uint8_t *data, int32_t size, const std::string &entry,
                           const std::string &filename, bool needUpdate)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    LOG_ECMA(INFO) << "start to execute ark buffer with secure memory: " << filename;
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromBufferSecure(thread, data, size, entry, filename.c_str(),
                                                                  needUpdate)) {
        if (thread->HasPendingException()) {
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
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
    ecmascript::ThreadManagedScope scope(thread);
    if (!ecmascript::JSPandaFileExecutor::ExecuteModuleBufferSecure(thread, data, size, filename.c_str(),
                                                                    needUpdate)) {
        if (thread->HasPendingException()) {
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
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

void JSNApi::UpdateAOTCompileStatus(ecmascript::JSRuntimeOptions &jsOption, const RuntimeOption &option)
{
    // When AOT compilation failed, disable PGO and JIT
    bool aotHasException = false;
    for (const auto &value : option.GetAOTCompileStatusMap()) {
        auto moduleCompileStatus = static_cast<RuntimeOption::AOTCompileStatus>(value.second);
        if (moduleCompileStatus == RuntimeOption::AOTCompileStatus::COMPILE_FAILED ||
            moduleCompileStatus == RuntimeOption::AOTCompileStatus::COMPILE_CRASH) {
            aotHasException = true;
            break;
        }
    }
    jsOption.SetAOTHasException(aotHasException);
}

void JSNApi::PostFork(EcmaVM *vm, const RuntimeOption &option)
{
    isForked_ = true;
    JSRuntimeOptions &jsOption = vm->GetJSOptions();
    jsOption.SetEnablePGOProfiler(option.GetEnableProfile());
    jsOption.SetEnableJIT(option.GetEnableJIT());
    jsOption.SetEnableBaselineJIT(option.GetEnableBaselineJIT());
    jsOption.SetMaxAotMethodSize(JSRuntimeOptions::MAX_APP_COMPILE_METHOD_SIZE);
    jsOption.SetEnableForceIC(false);
    ecmascript::pgo::PGOProfilerManager::GetInstance()->SetBundleName(option.GetBundleName());
    ecmascript::pgo::PGOProfilerManager::GetInstance()->SetMaxAotMethodSize(jsOption.GetMaxAotMethodSize());
    ecmascript::pgo::PGOProfilerManager::GetInstance()->SetEnableForceIC(jsOption.IsEnableForceIC());
    JSRuntimeOptions runtimeOptions;
    runtimeOptions.SetLogLevel(Log::LevelToString(Log::ConvertFromRuntime(option.GetLogLevel())));
    Log::Initialize(runtimeOptions);

    // 1. system switch 2. an file dir exits 3. whitelist 4. escape mechanism
    bool enableAOT = jsOption.GetEnableAOT() &&
                     !option.GetAnDir().empty() &&
                     EnableAotJitListHelper::GetInstance()->IsEnableAot(option.GetBundleName()) &&
                     !ecmascript::AotCrashInfo::IsAotEscaped();
    if (enableAOT) {
        ecmascript::AnFileDataManager::GetInstance()->SetDir(option.GetAnDir());
        ecmascript::AnFileDataManager::GetInstance()->SetEnable(true);
    }
    UpdateAOTCompileStatus(jsOption, option);

    LOG_ECMA(INFO) << "asmint: " << jsOption.GetEnableAsmInterpreter()
                    << ", aot: " << enableAOT
                    << ", jit: " << option.GetEnableJIT()
                    << ", baseline jit: " << option.GetEnableBaselineJIT()
                    << ", bundle name: " <<  option.GetBundleName();

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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return JSNApiHelper::ToLocal<ObjectRef>(vm->GetEcmaUncaughtException());
}

Local<ObjectRef> JSNApi::GetAndClearUncaughtException(const EcmaVM *vm)
{
    if (!HasPendingException(vm)) {
        return Local<ObjectRef>();
    }
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
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

int JSNApi::GetStartRealTime(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    return vm->GetProcessStartRealtime();
}

void JSNApi::NotifyTaskBegin(const EcmaVM *vm)
{
    const_cast<ecmascript::Heap *>(vm->GetHeap())->NotifyRecordMemorySize();
}

void JSNApi::NotifyTaskFinished(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    const_cast<ecmascript::Heap *>(vm->GetHeap())->CheckAndTriggerTaskFinishedGC();
}

bool JSNApi::IsMultiThreadCheckEnabled(const EcmaVM *vm)
{
    return vm->GetThreadCheckStatus();
}

uint32_t JSNApi::GetCurrentThreadId()
{
    return JSThread::GetCurrentThreadId();
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
    bool serializationTimeoutCheckEnabled = IsSerializationTimeoutCheckEnabled(vm);
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    if (serializationTimeoutCheckEnabled) {
        startTime = std::chrono::system_clock::now();
    }
    ecmascript::ValueSerializer serializer(thread, defaultTransfer, defaultCloneShared);
    std::unique_ptr<ecmascript::SerializeData> data;
    if (serializer.WriteValue(thread, arkValue, arkTransfer, arkCloneList)) {
        data = serializer.Release();
    }
    if (serializationTimeoutCheckEnabled) {
        endTime = std::chrono::system_clock::now();
        GenerateTimeoutTraceIfNeeded(vm, startTime, endTime, true);
    }
    if (data == nullptr) {
        return nullptr;
    } else {
        return reinterpret_cast<void *>(data.release());
    }
}

Local<JSValueRef> JSNApi::DeserializeValue(const EcmaVM *vm, void *recoder, void *hint)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    std::unique_ptr<ecmascript::SerializeData> data(reinterpret_cast<ecmascript::SerializeData *>(recoder));
    ecmascript::BaseDeserializer deserializer(thread, data.release(), hint);
    bool serializationTimeoutCheckEnabled = IsSerializationTimeoutCheckEnabled(vm);
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    if (serializationTimeoutCheckEnabled) {
        startTime = std::chrono::system_clock::now();
    }
    JSHandle<JSTaggedValue> result = deserializer.ReadValue();
    if (serializationTimeoutCheckEnabled) {
        endTime = std::chrono::system_clock::now();
        GenerateTimeoutTraceIfNeeded(vm, startTime, endTime, false);
    }
    return JSNApiHelper::ToLocal<ObjectRef>(result);
}

void JSNApi::DeleteSerializationData(void *data)
{
    ecmascript::SerializeData *value = reinterpret_cast<ecmascript::SerializeData *>(data);
    delete value;
    value = nullptr;
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
    std::function<bool(std::string dirPath, uint8_t **buff, size_t *buffSize, std::string &errorMsg)> cb)
{
    vm->SetResolveBufferCallback(cb);
}

void JSNApi::SetSearchHapPathTracker(EcmaVM *vm,
    std::function<bool(const std::string moduleName, std::string &hapPath)> cb)
{
    vm->SetSearchHapPathCallBack(cb);
}

void JSNApi::SetMultiThreadCheck(bool multiThreadCheck)
{
    EcmaVM::SetMultiThreadCheck(multiThreadCheck);
}

void JSNApi::SetErrorInfoEnhance(bool errorInfoEnhance)
{
    EcmaVM::SetErrorInfoEnhance(errorInfoEnhance);
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

void JSNApi::SetHostEnqueueJob(const EcmaVM *vm, Local<JSValueRef> cb, QueueType queueType)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK(vm);
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    JSHandle<JSFunction> fun = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(cb));
    JSHandle<TaggedArray> array = vm->GetFactory()->EmptyArray();
    JSHandle<MicroJobQueue> job = thread->GetCurrentEcmaContext()->GetMicroJobQueue();
    MicroJobQueue::EnqueueJob(thread, job, queueType, fun, array);
}

bool JSNApi::ExecuteModuleFromBuffer(EcmaVM *vm, const void *data, int32_t size, const std::string &file)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    if (!ecmascript::JSPandaFileExecutor::ExecuteFromBuffer(thread, data, size, ENTRY_POINTER, file.c_str(), false,
        true)) {
        if (thread->HasPendingException()) {
            ecmascript::JsStackInfo::BuildCrashInfo(thread);
        }
        std::cerr << "Cannot execute panda file from memory" << std::endl;
        return false;
    }
    return true;
}

Local<JSValueRef>  JSNApi::NapiHasProperty(const EcmaVM *vm, uintptr_t nativeObj, uintptr_t key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> obj(nativeObj);
    if (!(obj->IsECMAObject() || obj->IsCallable())) {
        // When input validation is failed, we return JSTaggedValue::Hole to napi native engine.
        // Using JSTaggedValue::Hole as the "hand-shaking-protocol" to tell native engine to change error state.
        JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(holeHandle));
    }
    LOG_IF_SPECIAL(obj, ERROR);
    JSMutableHandle<JSTaggedValue> keyValue(key);
    JSTaggedValue res = ObjectFastOperator::TryFastHasProperty(thread, obj.GetTaggedValue(),
                                                               keyValue);
    if (!res.IsHole()) {
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, res)));
    }
    auto ret = JSTaggedValue(JSTaggedValue::HasProperty(thread, obj, keyValue));
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, ret)));
}

Local<JSValueRef> JSNApi::NapiHasOwnProperty(const EcmaVM *vm, uintptr_t nativeObj, uintptr_t key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> obj(nativeObj);
    if (!(obj->IsECMAObject() || obj->IsCallable())) {
        // When input validation is failed, we return JSTaggedValue::Hole to napi native engine.
        // Using JSTaggedValue::Hole as the "hand-shaking-protocol" to tell native engine to change error state.
        JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(holeHandle));
    }
    LOG_IF_SPECIAL(obj, ERROR);
    JSMutableHandle<JSTaggedValue> keyValue(key);
    JSTaggedValue res = ObjectFastOperator::TryFastHasProperty(thread, obj.GetTaggedValue(),
                                                               keyValue);
    if (!res.IsHole()) {
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, res)));
    }
    auto ret = JSTaggedValue(JSTaggedValue::HasProperty(thread, obj, keyValue));
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, ret)));
}

Local<JSValueRef> JSNApi::NapiGetProperty(const EcmaVM *vm, uintptr_t nativeObj, uintptr_t key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> obj(nativeObj);
    if (!(obj->IsECMAObject() || obj->IsCallable())) {
        // When input validation is failed, we return JSTaggedValue::Hole to napi native engine.
        // Using JSTaggedValue::Hole as the "hand-shaking-protocol" to tell native engine to change error state.
        JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(holeHandle));
    }
    LOG_IF_SPECIAL(obj, ERROR);
    JSMutableHandle<JSTaggedValue> keyValue(key);
    if (!obj->IsHeapObject()) {
        OperationResult ret = JSTaggedValue::GetProperty(thread, obj, keyValue);
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(ret.GetValue()));
    }

    JSTaggedValue res = ObjectFastOperator::TryFastGetPropertyByValue(thread, obj.GetTaggedValue(),
                                                                      keyValue);
    if (!res.IsHole()) {
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, res)));
    }

    JSTaggedValue ret = ObjectFastOperator::FastGetPropertyByValue(thread, obj.GetTaggedValue(),
                                                                   keyValue.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, ret)));
}

Local<JSValueRef> JSNApi::NapiDeleteProperty(const EcmaVM *vm, uintptr_t nativeObj, uintptr_t key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> obj(nativeObj);
    if (!(obj->IsECMAObject() || obj->IsCallable())) {
        // When input validation is failed, we return JSTaggedValue::Hole to napi native engine.
        // Using JSTaggedValue::Hole as the "hand-shaking-protocol" to tell native engine to change error state.
        JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(holeHandle));
    }
    LOG_IF_SPECIAL(obj, ERROR);
    JSMutableHandle<JSTaggedValue> keyValue(key);
    if (keyValue->IsString() && !EcmaStringAccessor(keyValue.GetTaggedValue()).IsInternString()) {
        [[maybe_unused]] ecmascript::EcmaHandleScope handleScope(thread);
        auto string = thread->GetEcmaVM()->GetFactory()->InternString(keyValue);
        EcmaStringAccessor(string).SetInternString();
        keyValue.Update(JSTaggedValue(string));
    }
    auto ret = JSTaggedValue(JSTaggedValue::DeleteProperty(thread, obj, keyValue));
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, ret)));
}

Local<JSValueRef> JSNApi::NapiGetNamedProperty(const EcmaVM *vm, uintptr_t nativeObj, const char* utf8Key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    EscapeLocalScope scope(vm);
    JSHandle<JSTaggedValue> obj(nativeObj);
    if (!(obj->IsECMAObject() || obj->IsCallable())) {
        // When input validation is failed, we return JSTaggedValue::Hole to napi native engine.
        // Using JSTaggedValue::Hole as the "hand-shaking-protocol" to tell native engine to change error state.
        JSHandle<JSTaggedValue> holeHandle(thread, JSTaggedValue::Hole());
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(holeHandle));
    }
    LOG_IF_SPECIAL(obj, ERROR);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<JSTaggedValue> keyValue(factory->NewFromUtf8(utf8Key));
    if (!obj->IsHeapObject()) {
        OperationResult ret = JSTaggedValue::GetProperty(thread, obj, keyValue);
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(ret.GetValue()));
    }

    // FastPath - Try find key entry in cache directly.
    JSTaggedValue res = ObjectFastOperator::TryGetPropertyByNameThroughCacheAtLocal(thread, obj.GetTaggedValue(),
                                                                                    keyValue.GetTaggedValue());
    if (!res.IsHole()) {
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, res)));
    }
    if (!KeyIsNumber(utf8Key)) {
        res = ObjectFastOperator::GetPropertyByName(thread, obj.GetTaggedValue(), keyValue.GetTaggedValue());
        if (res.IsHole()) {
            res = JSTaggedValue::GetProperty(thread, obj, keyValue).GetValue().GetTaggedValue();
            RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
            return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, res)));
        }
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
        return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, res)));
    }
    res = ObjectFastOperator::FastGetPropertyByValue(thread, obj.GetTaggedValue(), keyValue.GetTaggedValue());
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return scope.Escape(JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, res)));
}

Local<JSValueRef> JSNApi::CreateLocal(const EcmaVM *vm, panda::JSValueRef src)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    JSHandle<JSTaggedValue> handle(vm->GetJSThread(), JSNApiHelper::ToJSTaggedValue(&src));
    return JSNApiHelper::ToLocal<JSValueRef>(handle);
}

Local<ObjectRef> JSNApi::GetExportObject(EcmaVM *vm, const std::string &file, const std::string &key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    ecmascript::CString entry = file.c_str();
    ecmascript::CString name = vm->GetAssetPath();
    if (!vm->IsBundlePack()) {
        ModulePathHelper::ParseAbcPathAndOhmUrl(vm, entry, name, entry);
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
    JSHandle<ecmascript::SourceTextModule> ecmaModule = moduleManager->GetImportedModule(entry);
    if (ecmaModule->GetIsNewBcVersion()) {
        int index = ecmascript::ModuleManager::GetExportObjectIndex(vm, ecmaModule, key.c_str());
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
    JSHandle<ecmascript::SourceTextModule> ecmaModule = moduleManager->GetImportedModule(file.c_str());
    if (ecmaModule->GetIsNewBcVersion()) {
        int index = ecmascript::ModuleManager::GetExportObjectIndex(vm, ecmaModule, key.c_str());
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

Local<ObjectRef> JSNApi::GetExportObjectFromOhmUrl(EcmaVM *vm, const std::string &ohmUrl, const std::string &key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    ecmascript::CString recordName;
    // Check and translate OhmUrl to recordName
    if (!ModulePathHelper::CheckAndGetRecordName(thread, ohmUrl.c_str(), recordName)) {
        LOG_FULL(FATAL) << "ExecuteSecureWithOhmUrl: Invalid input OhmUrl: " << ohmUrl;
        return JSValueRef::Undefined(vm);
    }
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<ecmascript::SourceTextModule> ecmaModule = moduleManager->GetImportedModule(recordName.c_str());
    int index = ecmascript::ModuleManager::GetExportObjectIndex(vm, ecmaModule, key.c_str());
    JSTaggedValue result = ecmaModule->GetModuleValue(thread, index, false);
    JSHandle<JSTaggedValue> exportObj(thread, result);
    return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
}

Local<ObjectRef> JSNApi::ExecuteNativeModule(EcmaVM *vm, const std::string &key)
{
    ecmascript::ThreadManagedScope scope(vm->GetJSThread());
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> exportObj = moduleManager->LoadNativeModule(thread, key.c_str());
    return JSNApiHelper::ToLocal<ObjectRef>(exportObj);
}

Local<ObjectRef> JSNApi::GetModuleNameSpaceFromFile(EcmaVM *vm, const std::string &file, const std::string &module_path)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ecmascript::CString recordNameStr;
    ecmascript::CString abcFilePath;
    if (module_path.size() != 0) {
        ecmascript::CString moduleName = ModulePathHelper::GetModuleNameWithPath(module_path.c_str());
        abcFilePath = ModulePathHelper::ConcatPandaFilePath(moduleName);
        recordNameStr = ModulePathHelper::TranslateNapiFileRequestPath(thread, module_path.c_str(), file.c_str());
    } else {
        // need get moduleName from stack
        std::pair<std::string, std::string> moduleInfo = vm->GetCurrentModuleInfo(false);
        if (thread->HasPendingException()) {
            thread->GetCurrentEcmaContext()->HandleUncaughtException();
            return JSValueRef::Undefined(vm);
        }
        std::string path = std::string(vm->GetBundleName().c_str()) + PathHelper::SLASH_TAG +
            moduleInfo.first;
        abcFilePath = moduleInfo.second;
        recordNameStr = ModulePathHelper::TranslateNapiFileRequestPath(thread, path.c_str(), file.c_str());
    }
    LOG_ECMA(DEBUG) << "JSNApi::LoadModuleNameSpaceFromFile: Concated recordName " << recordNameStr;
    ecmascript::ModuleManager *moduleManager = thread->GetCurrentEcmaContext()->GetModuleManager();
    JSHandle<JSTaggedValue> moduleNamespace = moduleManager->
        GetModuleNameSpaceFromFile(thread, recordNameStr, abcFilePath);
    return JSNApiHelper::ToLocal<ObjectRef>(moduleNamespace);
}

Local<ObjectRef> JSNApi::GetModuleNameSpaceWithModuleInfo(EcmaVM *vm, const std::string &file,
                                                          const std::string &module_path)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ecmascript::CString moduleStr = ModulePathHelper::GetModuleNameWithPath(module_path.c_str());
    ecmascript::CString srcPrefix = moduleStr + ModulePathHelper::PHYCICAL_FILE_PATH;
    std::string prefix = ConvertToStdString(srcPrefix);
    if (file.find(srcPrefix) == 0)  {
        std::string fileName = file.substr(prefix.size() + 1);
        return GetModuleNameSpaceFromFile(vm, fileName, module_path);
    }
    ecmascript::CString requestPath = file.c_str();
    ecmascript::CString modulePath = module_path.c_str();
    JSHandle<JSTaggedValue> nameSp = ecmascript::NapiModuleLoader::LoadModuleNameSpaceWithModuleInfo(vm,
        requestPath, modulePath);
    return JSNApiHelper::ToLocal<ObjectRef>(nameSp);
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
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
    thread->CheckJSTaggedType(reject.GetTaggedValue().GetRawData());
#endif
    info->SetCallArg(reject.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, catchKey);
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
    thread->CheckJSTaggedType(result.GetRawData());
#endif
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
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
    thread->CheckJSTaggedType(resolver.GetTaggedValue().GetRawData());
    thread->CheckJSTaggedType(undefined.GetTaggedValue().GetRawData());
#endif
    info->SetCallArg(resolver.GetTaggedValue(), undefined.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, finallyKey);
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
    thread->CheckJSTaggedType(result.GetRawData());
#endif
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
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
    thread->CheckJSTaggedType(resolver.GetTaggedValue().GetRawData());
    thread->CheckJSTaggedType(undefined.GetTaggedValue().GetRawData());
#endif
    info->SetCallArg(resolver.GetTaggedValue(), undefined.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, thenKey);
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
    thread->CheckJSTaggedType(result.GetRawData());
#endif
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
#if ECMASCRIPT_ENABLE_STUB_ARGV_CHECK
    thread->CheckJSTaggedType(resolver.GetTaggedValue().GetRawData());
    thread->CheckJSTaggedType(reject.GetTaggedValue().GetRawData());
#endif
    info->SetCallArg(resolver.GetTaggedValue(), reject.GetTaggedValue());
    JSTaggedValue result = JSFunction::Invoke(info, thenKey);
#if ECMASCRIPT_ENABLE_STUB_RESULT_CHECK
    thread->CheckJSTaggedType(result.GetRawData());
#endif
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
int32_t SetRef::GetSize(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    return set->GetSize();
}

int32_t SetRef::GetTotalElements(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    return static_cast<int>(set->GetSize()) +
        LinkedHashSet::Cast(set->GetLinkedSet().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> SetRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, FATAL);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, set->GetValue(entry)));
}

Local<SetRef> SetRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
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
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSSet> set(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(set, ERROR);
    JSSet::Add(vm->GetJSThread(), set, JSNApiHelper::ToJSHandle(value));
}

// ---------------------------------- WeakMapRef --------------------------------------
int32_t WeakMapRef::GetSize(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    return weakMap->GetSize();
}

int32_t WeakMapRef::GetTotalElements(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    return weakMap->GetSize() +
                LinkedHashMap::Cast(weakMap->GetLinkedMap().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> WeakMapRef::GetKey(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
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
    ecmascript::ThreadManagedScope managedScope(thread);
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
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakMap, FATAL);
    JSWeakMap::Set(vm->GetJSThread(), weakMap, JSNApiHelper::ToJSHandle(key), JSNApiHelper::ToJSHandle(value));
}

bool WeakMapRef::Has(const EcmaVM *vm, Local<JSValueRef> key)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, false);
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, false);
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSWeakMap> weakMap(JSNApiHelper::ToJSHandle(this));
    return weakMap->Has(thread, JSNApiHelper::ToJSTaggedValue(*key));
}

// ---------------------------------- WeakSetRef --------------------------------------
int32_t WeakSetRef::GetSize(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    return weakSet->GetSize();
}

int32_t WeakSetRef::GetTotalElements(const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, 0);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    return weakSet->GetSize() +
                LinkedHashSet::Cast(weakSet->GetLinkedSet().GetTaggedObject())->NumberOfDeletedElements();
}

Local<JSValueRef> WeakSetRef::GetValue(const EcmaVM *vm, int entry)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSWeakSet> weakSet(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(weakSet, FATAL);
    JSTaggedValue value = weakSet->GetValue(entry);
    return JSNApiHelper::ToLocal<JSValueRef>(JSHandle<JSTaggedValue>(thread, value.GetWeakRawValue()));
}

Local<WeakSetRef> WeakSetRef::New(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    ObjectFactory *factory = vm->GetJSThread()->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetJSThread()->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsWeakSetFunction();
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
    ecmascript::ThreadManagedScope managedScope(thread);
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
    ecmascript::ThreadManagedScope managedScope(ecmaVm_->GetJSThread());
    return JSNApiHelper::ToLocal<ObjectRef>(ecmaVm_->GetAndClearEcmaUncaughtException());
}

Local<ObjectRef> TryCatch::GetException()
{
    ecmascript::ThreadManagedScope managedScope(ecmaVm_->GetJSThread());
    return JSNApiHelper::ToLocal<ObjectRef>(ecmaVm_->GetEcmaUncaughtException());
}

void TryCatch::ClearException()
{
    ecmaVm_->GetJSThread()->ClearException();
}

bool ExternalStringCache::RegisterStringCacheTable(const EcmaVM *vm, uint32_t size)
{
    auto instance = ecmascript::Runtime::GetInstance();
    ASSERT(instance != nullptr);

    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return instance->CreateStringCacheTable(size);
}

bool ExternalStringCache::SetCachedString(const EcmaVM *vm, const char *name, uint32_t propertyIndex)
{
    auto instance = ecmascript::Runtime::GetInstance();
    ASSERT(instance != nullptr);

    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    [[maybe_unused]] LocalScope scope(vm);
    ObjectFactory *factory = vm->GetFactory();
    JSHandle<EcmaString> str = factory->NewFromUtf8(name);
    return instance->SetCachedString(str, propertyIndex);
}

Local<StringRef> ExternalStringCache::GetCachedString(const EcmaVM *vm, uint32_t propertyIndex)
{
    auto instance = ecmascript::Runtime::GetInstance();
    ASSERT(instance != nullptr);

    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<EcmaString> str = instance->GetCachedString(vm->GetJSThread(), propertyIndex);
    return JSNApiHelper::ToLocal<StringRef>(JSHandle<JSTaggedValue>(str));
}

bool ExternalStringCache::HasCachedString([[maybe_unused]] const EcmaVM *vm, uint32_t propertyIndex)
{
    auto instance = ecmascript::Runtime::GetInstance();
    ASSERT(instance != nullptr);
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    return instance->HasCachedString(propertyIndex);
}
} // namespace panda
