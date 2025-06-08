/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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


#include <cstdint>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/builtins/builtins_object.h"
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_runtime_options.h"
#include "ecmascript/module/module_path_helper.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/regexp/regexp_parser.h"
#include "ecmascript/js_weak_container.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_number_format.h"
#endif

#if defined(ECMASCRIPT_SUPPORT_DEBUGGER) && defined(PANDA_TARGET_IOS)
namespace OHOS::ArkCompiler::Toolchain {
using DebuggerPostTask = std::function<void(std::function<void()> &&)>;
extern "C" {
    bool StartDebug(const std::string& componentName, void* vm, bool isDebugMode, int32_t instanceId,
        const DebuggerPostTask& debuggerPostTask, int port);
    void StopDebug(void* vm);
    void WaitForDebugger(void* vm);
}
} // namespace OHOS::ArkCompiler::Toolchain
const std::string DEBUGGER_NAME = "PandaDebugger";
#endif

namespace panda {
using ecmascript::ECMAObject;
using ecmascript::EcmaString;
using ecmascript::EcmaStringAccessor;
using ecmascript::ErrorType;
using ecmascript::FastRuntimeStub;
using ecmascript::GlobalEnv;
using ecmascript::GlobalEnvConstants;
using ecmascript::EcmaRuntimeCallInfo;
using ecmascript::JSArray;
using ecmascript::JSArrayBuffer;
using ecmascript::JSDataView;
using ecmascript::ByteArray;
using ecmascript::JSDate;
using ecmascript::JSFunction;
using ecmascript::JSFunctionBase;
using ecmascript::JSHClass;
using ecmascript::JSMap;
using ecmascript::Method;
using ecmascript::JSNativePointer;
using ecmascript::JSObject;
using ecmascript::JSPandaFile;
using ecmascript::JSPandaFileManager;
using ecmascript::JSPrimitiveRef;
using ecmascript::JSPromise;
using ecmascript::JSRegExp;
using ecmascript::JSSet;
using ecmascript::JSSymbol;
using ecmascript::JSTaggedNumber;
using ecmascript::JSTaggedType;
using ecmascript::JSTaggedValue;
using ecmascript::JSThread;
using ecmascript::LinkedHashMap;
using ecmascript::LinkedHashSet;
using ecmascript::ObjectFactory;
using ecmascript::PromiseCapability;
using ecmascript::PropertyDescriptor;
using ecmascript::OperationResult;
using ecmascript::Region;
using ecmascript::TaggedArray;
using ecmascript::JSTypedArray;
using ecmascript::base::BuiltinsBase;
using ecmascript::builtins::BuiltinsObject;
using ecmascript::base::Utf8JsonParser;
using ecmascript::base::Utf16JsonParser;
using ecmascript::base::JsonStringifier;
using ecmascript::base::StringHelper;
using ecmascript::base::TypedArrayHelper;
using ecmascript::job::MicroJobQueue;
using ecmascript::job::QueueType;
using ecmascript::JSRuntimeOptions;
using ecmascript::BigInt;
using ecmascript::MemMapAllocator;
using ecmascript::Mutex;
using ecmascript::LockHolder;
using ecmascript::JSMapIterator;
using ecmascript::JSSetIterator;
using ecmascript::IterationKind;
using ecmascript::JSGeneratorState;
using ecmascript::JSIterator;
using ecmascript::JSGeneratorFunction;
using ecmascript::JSGeneratorObject;
using ecmascript::GeneratorContext;
using ecmascript::JSProxy;
#ifdef ARK_SUPPORT_INTL
using ecmascript::JSCollator;
using ecmascript::JSDateTimeFormat;
using ecmascript::JSNumberFormat;
#endif
using ecmascript::RegExpParser;
using ecmascript::DebugInfoExtractor;
using ecmascript::base::NumberHelper;
using ecmascript::JSWeakMap;
using ecmascript::JSWeakSet;
using ecmascript::JSSendableArrayBuffer;
template<typename T>
using JSHandle = ecmascript::JSHandle<T>;

template<typename T>
using JSMutableHandle = ecmascript::JSMutableHandle<T>;

using PathHelper = ecmascript::base::PathHelper;
using ModulePathHelper = ecmascript::ModulePathHelper;
using TransformType = ecmascript::base::JsonHelper::TransformType;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_ARRAY_NEW(Type)                                                                             \
    Local<Type##Ref> Type##Ref::New(                                                                      \
        const EcmaVM *vm, Local<ArrayBufferRef> buffer, int32_t byteOffset, int32_t length)               \
    {                                                                                                     \
        CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));                      \
        ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());                                   \
        JSHandle<GlobalEnv> env = vm->GetGlobalEnv();                                                     \
                                                                                                          \
        JSHandle<JSTaggedValue> func = env->Get##Type##Function();                                        \
        JSHandle<JSArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(buffer));                            \
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();             \
        const uint32_t argsLength = 3;                                                                    \
        EcmaRuntimeCallInfo *info =                                                                       \
            ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, undefined, func, argsLength);   \
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));                                        \
        info->SetCallArg(arrayBuffer.GetTaggedValue(), JSTaggedValue(byteOffset), JSTaggedValue(length)); \
        JSTaggedValue result = JSFunction::Construct(info);                                               \
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));                                        \
        JSHandle<JSTaggedValue> resultHandle(thread, result);                                             \
        return JSNApiHelper::ToLocal<Type##Ref>(resultHandle);                                            \
    }

TYPED_ARRAY_ALL(TYPED_ARRAY_NEW)

#undef TYPED_ARRAY_NEW

#define SENDABLE_TYPED_ARRAY_NEW(Type)                                                                    \
    Local<Type##Ref> Type##Ref::New(                                                                      \
        const EcmaVM *vm, Local<SendableArrayBufferRef> buffer, int32_t byteOffset, int32_t length)       \
    {                                                                                                     \
        CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));                      \
        ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());                                   \
        JSHandle<GlobalEnv> env = vm->GetGlobalEnv();                                                     \
                                                                                                          \
        JSHandle<JSTaggedValue> func = env->Get##Type##Function();                                        \
        JSHandle<JSSendableArrayBuffer> arrayBuffer(JSNApiHelper::ToJSHandle(buffer));                    \
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();             \
        const uint32_t argsLength = 3;                                                                    \
        EcmaRuntimeCallInfo *info =                                                                       \
            ecmascript::EcmaInterpreter::NewRuntimeCallInfo(thread, func, undefined, func, argsLength);   \
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));                                        \
        info->SetCallArg(arrayBuffer.GetTaggedValue(), JSTaggedValue(byteOffset), JSTaggedValue(length)); \
        JSTaggedValue result = JSFunction::Construct(info);                                               \
        RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));                                        \
        JSHandle<JSTaggedValue> resultHandle(thread, result);                                             \
        return JSNApiHelper::ToLocal<Type##Ref>(resultHandle);                                            \
    }

SENDABLE_TYPED_ARRAY_ALL(SENDABLE_TYPED_ARRAY_NEW)

#undef SENDABLE_TYPED_ARRAY_NEW

// ---------------------------------- JSON ------------------------------------------
Local<JSValueRef> JSON::Parse(const EcmaVM *vm, Local<StringRef> string)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    auto ecmaStr = EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(*string).GetTaggedObject());
    JSHandle<JSTaggedValue> result;
    if (EcmaStringAccessor(ecmaStr).IsUtf8()) {
        Utf8JsonParser parser(thread, TransformType::NORMAL);
        JSHandle<EcmaString> str(thread, JSNApiHelper::ToJSTaggedValue(*string));
        result = parser.Parse(str);
    } else {
        Utf16JsonParser parser(thread, TransformType::NORMAL);
        result = parser.Parse(EcmaString::Cast(JSNApiHelper::ToJSTaggedValue(*string).GetTaggedObject()));
    }
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(result);
}

Local<JSValueRef> JSON::Stringify(const EcmaVM *vm, Local<JSValueRef> json)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    auto constants = thread->GlobalConstants();
    JsonStringifier stringifier(thread);
    JSHandle<JSTaggedValue> str = stringifier.Stringify(
        JSNApiHelper::ToJSHandle(json), constants->GetHandledUndefined(), constants->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT(thread, JSValueRef::Undefined(vm));
    return JSNApiHelper::ToLocal<JSValueRef>(str);
}

Local<StringRef> RegExpRef::GetOriginalSource(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue source = regExp->GetOriginalSource();
    if (!source.IsString()) {
        auto constants = thread->GlobalConstants();
        return JSNApiHelper::ToLocal<StringRef>(constants->GetHandledEmptyString());
    }
    JSHandle<JSTaggedValue> sourceHandle(thread, source);
    return JSNApiHelper::ToLocal<StringRef>(sourceHandle);
}

std::string RegExpRef::GetOriginalFlags([[maybe_unused]] const EcmaVM *vm)
{
    DCHECK_SPECIAL_VALUE_WITH_RETURN(this, "");
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    JSTaggedValue regExpFlags = regExp->GetOriginalFlags();
    uint32_t regExpFlagsInt = static_cast<uint32_t>(regExpFlags.GetInt());
    std::string strFlags = "";
    if (regExpFlagsInt & RegExpParser::FLAG_GLOBAL) {
        strFlags += "g";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_IGNORECASE) {
        strFlags += "i";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_MULTILINE) {
        strFlags += "m";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_DOTALL) {
        strFlags += "s";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_UTF16) {
        strFlags += "u";
    }
    if (regExpFlagsInt & RegExpParser::FLAG_STICKY) {
        strFlags += "y";
    }
    std::sort(strFlags.begin(), strFlags.end());
    return strFlags;
}

Local<JSValueRef> RegExpRef::IsGlobal(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_GLOBAL;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsIgnoreCase(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_IGNORECASE;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsMultiline(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_MULTILINE;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsDotAll(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_DOTALL;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsUtf16(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_UTF16;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

Local<JSValueRef> RegExpRef::IsStick(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSRegExp> regExp(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(regExp, FATAL);
    JSTaggedValue flags = regExp->GetOriginalFlags();
    bool result = flags.GetInt() & RegExpParser::FLAG_STICKY;
    Local<JSValueRef> jsValue = BooleanRef::New(vm, result);
    return jsValue;
}

bool GeneratorFunctionRef::IsGenerator(const EcmaVM *vm)
{
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    // Omit exception check because ark calls here may not
    // cause side effect even pending exception exists.
    return IsGeneratorFunction(vm);
}

Local<JSValueRef> GeneratorObjectRef::GetGeneratorState(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSGeneratorObject> jsGenerator(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsGenerator, FATAL);
    if (jsGenerator->GetGeneratorState() == JSGeneratorState::COMPLETED) {
        return StringRef::NewFromUtf8(vm, "closed");
    }
    return StringRef::NewFromUtf8(vm, "suspended");
}

Local<JSValueRef> GeneratorObjectRef::GetGeneratorFunction(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSGeneratorObject> jsGenerator(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsGenerator, FATAL);
    JSHandle<GeneratorContext> generatorContext(thread, jsGenerator->GetGeneratorContext());
    JSTaggedValue jsTagValue = generatorContext->GetMethod();
    return JSNApiHelper::ToLocal<GeneratorFunctionRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
}

Local<JSValueRef> GeneratorObjectRef::GetGeneratorReceiver(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
    JSHandle<JSGeneratorObject> jsGenerator(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsGenerator, FATAL);
    JSHandle<GeneratorContext> generatorContext(thread, jsGenerator->GetGeneratorContext());
    JSTaggedValue jsTagValue = generatorContext->GetAcc();
    return JSNApiHelper::ToLocal<GeneratorObjectRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
}

Local<JSValueRef> CollatorRef::GetCompareFunction(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
#ifdef ARK_SUPPORT_INTL
    JSHandle<JSCollator> jsCollator(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsCollator, FATAL);
    JSTaggedValue jsTagValue = jsCollator->GetBoundCompare();
    return JSNApiHelper::ToLocal<CollatorRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler intl";
    return JSNApiHelper::ToLocal<CollatorRef>(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
#endif
}

Local<JSValueRef> DataTimeFormatRef::GetFormatFunction(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
#ifdef ARK_SUPPORT_INTL
    JSHandle<JSDateTimeFormat> jsDateTimeFormat(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsDateTimeFormat, FATAL);
    JSTaggedValue jsTagValue = jsDateTimeFormat->GetBoundFormat();
    return JSNApiHelper::ToLocal<DataTimeFormatRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler intl";
    return JSNApiHelper::ToLocal<DataTimeFormatRef>(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
#endif
}

Local<JSValueRef> NumberFormatRef::GetFormatFunction(const EcmaVM *vm)
{
    CROSS_THREAD_AND_EXCEPTION_CHECK_WITH_RETURN(vm, JSValueRef::Undefined(vm));
    ecmascript::ThreadManagedScope managedScope(vm->GetJSThread());
#ifdef ARK_SUPPORT_INTL
    JSHandle<JSNumberFormat> jsNumberFormat(JSNApiHelper::ToJSHandle(this));
    LOG_IF_SPECIAL(jsNumberFormat, FATAL);
    JSTaggedValue jsTagValue = jsNumberFormat->GetBoundFormat();
    return JSNApiHelper::ToLocal<NumberFormatRef>(JSHandle<JSTaggedValue>(thread, jsTagValue));
#else
    LOG_ECMA(ERROR) << "Not support arkcompiler intl";
    return JSNApiHelper::ToLocal<NumberFormatRef>(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
#endif
}

// ----------------------------------- FunctionCallback ---------------------------------
JSTaggedValue Callback::RegisterCallback(ecmascript::EcmaRuntimeCallInfo *ecmaRuntimeCallInfo)
{
    // Constructor
    JSThread *thread = ecmaRuntimeCallInfo->GetThread();
    ecmascript::ThreadManagedScope managedScope(thread);
    JSHandle<JSTaggedValue> constructor = BuiltinsBase::GetConstructor(ecmaRuntimeCallInfo);
    if (!constructor->IsJSFunction()) {
        return JSTaggedValue::False();
    }
    [[maybe_unused]] LocalScope scope(thread->GetEcmaVM());
    JSHandle<JSFunctionBase> function(constructor);
    JSTaggedValue extraInfoValue = function->GetFunctionExtraInfo();
    if (!extraInfoValue.IsJSNativePointer()) {
        return JSTaggedValue::False();
    }
    JSHandle<JSNativePointer> extraInfo(thread, extraInfoValue);
    // callBack
    FunctionCallback nativeFunc = reinterpret_cast<FunctionCallback>(extraInfo->GetExternalPointer());

    JsiRuntimeCallInfo *jsiRuntimeCallInfo = reinterpret_cast<JsiRuntimeCallInfo *>(ecmaRuntimeCallInfo);
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    bool getStackBeforeCallNapiSuccess = false;
    if (thread->GetIsProfiling() && function->IsCallNapi()) {
        getStackBeforeCallNapiSuccess = thread->GetEcmaVM()->GetProfiler()->GetStackBeforeCallNapi(thread);
    }
#endif
    Local<JSValueRef> result;
    {
        ecmascript::ThreadNativeScope nativeScope(thread);
#if ECMASCRIPT_ENABLE_INTERPRETER_ARKUINAITVE_TRACE
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_MAX, HITRACE_TAG_ARK, "ArkUI::NativeCallBack", "");
#endif
        result = nativeFunc(jsiRuntimeCallInfo);
    }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    if (thread->GetIsProfiling() && function->IsCallNapi() && getStackBeforeCallNapiSuccess) {
        thread->GetEcmaVM()->GetProfiler()->GetStackAfterCallNapi(thread);
    }
#endif
    return JSNApiHelper::ToJSHandle(result).GetTaggedValue();
}
}  // namespace panda
