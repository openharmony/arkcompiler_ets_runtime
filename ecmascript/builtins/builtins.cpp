/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins.h"

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/builtins/builtins_async_from_sync_iterator.h"
#include "ecmascript/builtins/builtins_async_function.h"
#include "ecmascript/builtins/builtins_async_iterator.h"
#include "ecmascript/builtins/builtins_async_generator.h"
#include "ecmascript/builtins/builtins_atomics.h"
#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/builtins/builtins_cjs_module.h"
#include "ecmascript/builtins/builtins_cjs_require.h"
#include "ecmascript/builtins/builtins_cjs_exports.h"
#include "ecmascript/builtins/builtins_dataview.h"
#include "ecmascript/builtins/builtins_date.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_finalization_registry.h"
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_gc.h"
#include "ecmascript/builtins/builtins_generator.h"
#include "ecmascript/builtins/builtins_global.h"
#include "ecmascript/builtins/builtins_iterator.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/builtins/builtins_map.h"
#include "ecmascript/builtins/builtins_math.h"
#include "ecmascript/builtins/builtins_number.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_promise.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/builtins/builtins_reflect.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/builtins/builtins_set.h"
#include "ecmascript/builtins/builtins_sharedarraybuffer.h"
#include "ecmascript/builtins/builtins_shared_typedarray.h"
#include "ecmascript/builtins/builtins_string_iterator.h"
#include "ecmascript/builtins/builtins_symbol.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/builtins/builtins_weak_map.h"
#include "ecmascript/builtins/builtins_weak_ref.h"
#include "ecmascript/builtins/builtins_weak_set.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/dfx/native_module_failure_info.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_async_from_sync_iterator.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/shared_objects/js_shared_array_iterator.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"
#include "ecmascript/marker_cell.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/builtins/builtins_collator.h"
#include "ecmascript/builtins/builtins_date_time_format.h"
#include "ecmascript/builtins/builtins_displaynames.h"
#include "ecmascript/builtins/builtins_intl.h"
#include "ecmascript/builtins/builtins_list_format.h"
#include "ecmascript/builtins/builtins_locale.h"
#include "ecmascript/builtins/builtins_number_format.h"
#include "ecmascript/builtins/builtins_plural_rules.h"
#include "ecmascript/builtins/builtins_relative_time_format.h"
#include "ecmascript/builtins/builtins_segmenter.h"
#include "ecmascript/builtins/builtins_segments.h"
#include "ecmascript/builtins/builtins_segment_iterator.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_segments.h"
#include "ecmascript/js_segment_iterator.h"
#endif


namespace panda::ecmascript {
using Number = builtins::BuiltinsNumber;
using BuiltinsBigInt = builtins::BuiltinsBigInt;
using Object = builtins::BuiltinsObject;
using Date = builtins::BuiltinsDate;
using Symbol = builtins::BuiltinsSymbol;
using Boolean = builtins::BuiltinsBoolean;
using BuiltinsLazyCallback = builtins::BuiltinsLazyCallback;
using BuiltinsMap = builtins::BuiltinsMap;
using BuiltinsSet = builtins::BuiltinsSet;
using BuiltinsWeakMap = builtins::BuiltinsWeakMap;
using BuiltinsWeakSet = builtins::BuiltinsWeakSet;
using BuiltinsWeakRef = builtins::BuiltinsWeakRef;
using BuiltinsFinalizationRegistry = builtins::BuiltinsFinalizationRegistry;
using BuiltinsArray = builtins::BuiltinsArray;
using BuiltinsTypedArray = builtins::BuiltinsTypedArray;
using BuiltinsIterator = builtins::BuiltinsIterator;

using Error = builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using AggregateError = builtins::BuiltinsAggregateError;
using URIError = builtins::BuiltinsURIError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using EvalError = builtins::BuiltinsEvalError;
using OOMError = builtins::BuiltinsOOMError;
using TerminationError = builtins::BuiltinsTerminationError;
using ErrorType = base::ErrorType;
using RandomGenerator = base::RandomGenerator;
using Global = builtins::BuiltinsGlobal;
using BuiltinsString = builtins::BuiltinsString;
using StringIterator = builtins::BuiltinsStringIterator;
using BuiltinsAsyncFromSyncIterator = builtins::BuiltinsAsyncFromSyncIterator;
using RegExp = builtins::BuiltinsRegExp;
using Function = builtins::BuiltinsFunction;
using Math = builtins::BuiltinsMath;
using Atomics = builtins::BuiltinsAtomics;
using ArrayBuffer = builtins::BuiltinsArrayBuffer;
using Json = builtins::BuiltinsJson;
using SendableJson = builtins::BuiltinsSendableJson;
using BigIntJson = builtins::BuiltinsBigIntJson;
using Proxy = builtins::BuiltinsProxy;
using Reflect = builtins::BuiltinsReflect;
using AsyncFunction = builtins::BuiltinsAsyncFunction;
using GeneratorObject = builtins::BuiltinsGenerator;
using Promise = builtins::BuiltinsPromise;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;
using BuiltinsPromiseJob = builtins::BuiltinsPromiseJob;
using ErrorType = base::ErrorType;
using DataView = builtins::BuiltinsDataView;
#ifdef ARK_SUPPORT_INTL
using Intl = builtins::BuiltinsIntl;
using Locale = builtins::BuiltinsLocale;
using DateTimeFormat = builtins::BuiltinsDateTimeFormat;
using RelativeTimeFormat = builtins::BuiltinsRelativeTimeFormat;
using NumberFormat = builtins::BuiltinsNumberFormat;
using Collator = builtins::BuiltinsCollator;
using PluralRules = builtins::BuiltinsPluralRules;
using DisplayNames = builtins::BuiltinsDisplayNames;
using Segmenter = builtins::BuiltinsSegmenter;
using Segments = builtins::BuiltinsSegments;
using SegmentIterator = builtins::BuiltinsSegmentIterator;
using ListFormat = builtins::BuiltinsListFormat;
#endif
using BuiltinsCjsModule = builtins::BuiltinsCjsModule;
using BuiltinsCjsExports = builtins::BuiltinsCjsExports;
using BuiltinsCjsRequire = builtins::BuiltinsCjsRequire;

using ContainersPrivate = containers::ContainersPrivate;
using SharedArrayBuffer = builtins::BuiltinsSharedArrayBuffer;

using BuiltinsAsyncIterator = builtins::BuiltinsAsyncIterator;
using AsyncGeneratorObject = builtins::BuiltinsAsyncGenerator;

static constexpr size_t REGEXP_INLINE_PROPS = 18;

void Builtins::Initialize(const JSHandle<GlobalEnv> &env, JSThread *thread, bool lazyInit, bool isRealm)
{
    thread->CheckSafepointIfSuspended();
    thread_ = thread;
    vm_ = thread->GetEcmaVM();
    factory_ = vm_->GetFactory();
    sHeap_ = SharedHeap::GetInstance();
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSTaggedValue> nullHandle(thread, JSTaggedValue::Null());

    // Object.prototype[hclass]
    JSHandle<JSHClass> objPrototypeHClass = factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, nullHandle);

    // Object.prototype
    JSHandle<JSObject> objFuncPrototype = factory_->NewJSObject(objPrototypeHClass);
    JSHandle<JSTaggedValue> objFuncPrototypeVal(objFuncPrototype);

    // Object.prototype_or_hclass
    JSHandle<JSHClass> objFuncClass =
        factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, objFuncPrototypeVal);
    env->SetObjectFunctionClass(thread_, objFuncClass);

    // GLobalObject.prototype_or_hclass
    JSHandle<JSHClass> globalObjFuncClass =
        factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_GLOBAL_OBJECT, 0);
    globalObjFuncClass->SetPrototype(thread_, objFuncPrototypeVal.GetTaggedValue());
    globalObjFuncClass->SetIsDictionaryMode(true);

    // PrimitiveRef.prototype_or_hclass
    JSHandle<JSHClass> primRefObjHClass =
        factory_->NewEcmaHClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, objFuncPrototypeVal);

    // init global object
    JSHandle<JSObject> globalObject = factory_->NewNonMovableJSObject(globalObjFuncClass);
    env->SetJSGlobalObject(thread_, globalObject);

    // init global patch
    JSHandle<TaggedArray> globalPatch = factory_->EmptyArray();
    env->SetGlobalPatch(thread, globalPatch);

    auto runtimeGlobalEnv = Runtime::GetInstance()->GetGlobalEnv();
    if (runtimeGlobalEnv.IsHole()) {
        InitializeSSymbolAttributes(env);
        InitializeSObjectAndSFunction(env);
    } else {
        CopySObjectAndSFunction(env, runtimeGlobalEnv);
        RegisterSendableContainers(env);
    }
    InitializeFunction(env, objFuncPrototypeVal);

    thread->CheckSafepointIfSuspended();
    JSHandle<JSHClass> asyncAwaitStatusFuncClass =
        factory_->CreateFunctionClass(FunctionKind::NORMAL_FUNCTION, JSAsyncAwaitStatusFunction::SIZE,
                                      JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, env->GetFunctionPrototype());
    env->SetAsyncAwaitStatusFunctionClass(thread_, asyncAwaitStatusFuncClass);

    JSHandle<JSHClass> asyncGeneratorResNextRetProRstFtnClass =
        factory_->NewEcmaHClass(JSAsyncGeneratorResNextRetProRstFtn::SIZE,
                                JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN,
                                env->GetFunctionPrototype());
    asyncGeneratorResNextRetProRstFtnClass->SetCallable(true);
    asyncGeneratorResNextRetProRstFtnClass->SetExtensible(true);
    env->SetAsyncGeneratorResNextRetProRstFtnClass(thread_, asyncGeneratorResNextRetProRstFtnClass);

    JSHandle<JSHClass> proxyRevocFuncClass = factory_->NewEcmaHClass(
        JSProxyRevocFunction::SIZE, JSType::JS_PROXY_REVOC_FUNCTION, env->GetFunctionPrototype());
    proxyRevocFuncClass->SetCallable(true);
    proxyRevocFuncClass->SetExtensible(true);
    env->SetProxyRevocFunctionClass(thread_, proxyRevocFuncClass);

    // Object = new Function()
    JSHandle<JSObject> objectFunction(
        NewBuiltinConstructor(env, objFuncPrototype, Object::ObjectConstructor, "Object",
                              FunctionLength::ONE, BUILTINS_STUB_ID(ObjectConstructor)));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, JSHandle<JSFunction>(objectFunction),
                                                     objFuncClass.GetTaggedValue());
    // initialize object method.
    env->SetObjectFunction(thread_, objectFunction);
    env->SetObjectFunctionPrototype(thread_, objFuncPrototype);
    thread_->SetInitialBuiltinHClass(
        BuiltinTypeId::OBJECT, objectFunction->GetJSHClass(), *objFuncClass, objFuncPrototype->GetJSHClass());

    JSHandle<JSHClass> functionClass = factory_->CreateFunctionClass(FunctionKind::BASE_CONSTRUCTOR, JSFunction::SIZE,
                                                                     JSType::JS_FUNCTION, env->GetFunctionPrototype());
    env->SetFunctionClassWithProto(thread_, functionClass);
    functionClass = factory_->CreateFunctionClass(FunctionKind::NORMAL_FUNCTION, JSFunction::SIZE, JSType::JS_FUNCTION,
                                                  env->GetFunctionPrototype());
    env->SetFunctionClassWithoutProto(thread_, functionClass);
    functionClass = factory_->CreateFunctionClass(FunctionKind::CLASS_CONSTRUCTOR, JSFunction::SIZE,
                                                  JSType::JS_FUNCTION, env->GetFunctionPrototype());
    env->SetFunctionClassWithoutName(thread_, functionClass);

    thread->CheckSafepointIfSuspended();
    functionClass = factory_->CreateBoundFunctionClass();
    env->SetBoundFunctionClass(thread_, functionClass);
    if (!isRealm) {
        InitializeAllTypeError(env, objFuncClass);
        InitializeSymbol(env, primRefObjHClass);
        InitializeBigInt(env, primRefObjHClass);
    } else {
        // error and symbol need to be shared when initialize realm
        InitializeAllTypeErrorWithRealm(env);
        InitializeSymbolWithRealm(env, primRefObjHClass);
        InitializeBigIntWithRealm(env);
    }

    thread->CheckSafepointIfSuspended();
    InitializeArray(env, objFuncPrototypeVal, isRealm);
    if (lazyInit) {
        LazyInitializeDate(env);
        LazyInitializeSet(env);
        LazyInitializeMap(env);
        LazyInitializeWeakMap(env);
        LazyInitializeWeakSet(env);
        LazyInitializeWeakRef(env);
        LazyInitializeFinalizationRegistry(env);
        LazyInitializeTypedArray(env);
        LazyInitializeArrayBuffer(env);
        LazyInitializeDataView(env);
        LazyInitializeSharedArrayBuffer(env);
    } else {
        InitializeDate(env, objFuncPrototypeVal);
        InitializeSet(env, objFuncPrototypeVal);
        InitializeMap(env, objFuncPrototypeVal);
        InitializeWeakMap(env, objFuncClass);
        InitializeWeakSet(env, objFuncClass);
        InitializeWeakRef(env, objFuncClass);
        InitializeFinalizationRegistry(env, objFuncClass);
        InitializeTypedArray(env, objFuncPrototypeVal);
        InitializeArrayBuffer(env, objFuncClass);
        InitializeDataView(env, objFuncPrototypeVal);
        InitializeSharedArrayBuffer(env, objFuncClass);
    }
    thread->CheckSafepointIfSuspended();
    InitializeNumber(env, globalObject, primRefObjHClass);
    InitializeObject(env, objFuncPrototype, objectFunction);
    InitializeBoolean(env, primRefObjHClass);
    InitializeRegExp(env);
    InitializeString(env, objFuncPrototypeVal);
    JSHandle<JSHClass> argumentsClass = factory_->CreateJSArguments(env);
    env->SetArgumentsClass(thread_, argumentsClass);
    SetArgumentsSharedAccessor(env);

    InitializeMath(env, objFuncPrototypeVal);
    InitializeGlobalObject(env, globalObject);
    InitializeAtomics(env, objFuncPrototypeVal);
    InitializeJson(env, objFuncPrototypeVal);
    InitializeIterator(env, objFuncClass);
    InitializeAsyncIterator(env, objFuncClass);
    InitializeAsyncFromSyncIterator(env, objFuncClass);
    InitializeProxy(env);
    InitializeReflect(env, objFuncPrototypeVal);
    InitializeAsyncFunction(env, objFuncClass);
    InitializeGenerator(env, objFuncClass);
    InitializeAsyncGenerator(env, objFuncClass);
    InitializeGeneratorFunction(env, objFuncClass);
    InitializeAsyncGeneratorFunction(env, objFuncClass);
    InitializePromise(env, objFuncClass);
    InitializePromiseJob(env);
    thread->CheckSafepointIfSuspended();
#ifdef ARK_SUPPORT_INTL
    InitializeIntl(env, objFuncPrototypeVal);
    if (lazyInit) {
        LazyInitializeLocale(env);
        LazyInitializeDateTimeFormat(env);
        LazyInitializeNumberFormat(env);
        LazyInitializeRelativeTimeFormat(env);
        LazyInitializeCollator(env);
        LazyInitializePluralRules(env);
        LazyInitializeDisplayNames(env);
        LazyInitializeListFormat(env);
        LazyInitializeSegments(env);
        LazyInitializeSegmenter(env);
    } else {
        InitializeLocale(env);
        InitializeDateTimeFormat(env);
        InitializeNumberFormat(env);
        InitializeRelativeTimeFormat(env);
        InitializeCollator(env);
        InitializePluralRules(env);
        InitializeDisplayNames(env);
        InitializeListFormat(env);
        InitializeSegments(env);
        InitializeSegmenter(env);
    }
    thread->CheckSafepointIfSuspended();
#endif
    InitializeModuleNamespace(env, objFuncClass);
    InitializeNativeModuleFailureInfo(env, objFuncClass);
    InitializeCjsModule(env);
    InitializeCjsExports(env);
    InitializeCjsRequire(env);
    InitializeDefaultExportOfScript(env);
    InitializePropertyDetector(env, lazyInit);
    InitializeNapiHClass(env, objFuncClass);
    JSHandle<JSHClass> generatorFuncClass =
        factory_->CreateFunctionClass(FunctionKind::GENERATOR_FUNCTION, JSFunction::SIZE, JSType::JS_GENERATOR_FUNCTION,
                                      env->GetGeneratorFunctionPrototype());
    env->SetGeneratorFunctionClass(thread_, generatorFuncClass);

    JSHandle<JSHClass> asyncGenetatorFuncClass =
        factory_->CreateFunctionClass(FunctionKind::ASYNC_GENERATOR_FUNCTION, JSFunction::SIZE,
                                      JSType::JS_ASYNC_GENERATOR_FUNCTION, env->GetAsyncGeneratorFunctionPrototype());
    env->SetAsyncGeneratorFunctionClass(thread_, asyncGenetatorFuncClass);
    env->SetObjectFunctionPrototypeClass(thread_, JSTaggedValue(objFuncPrototype->GetClass()));
    JSHandle<JSHClass> asyncFuncClass = factory_->CreateFunctionClass(
        FunctionKind::ASYNC_FUNCTION, JSAsyncFunction::SIZE, JSType::JS_ASYNC_FUNCTION,
        env->GetAsyncFunctionPrototype());
    env->SetAsyncFunctionClass(thread_, asyncFuncClass);
    thread_->ResetGuardians();

    thread->CheckSafepointIfSuspended();
    if (vm_->GetJSOptions().IsEnableLoweringBuiltin() && !isRealm) {
        if (!lazyInit) {
            thread_->InitializeBuiltinObject();
        }
    }
}

void Builtins::InitializePropertyDetector(const JSHandle<GlobalEnv> &env, bool lazyInit) const
{
#define INITIALIZE_PROPERTY_DETECTOR(type, name, index)              \
    JSHandle<MarkerCell> name##detector = factory_->NewMarkerCell(); \
    if (lazyInit) {                                                  \
        name##detector->InvalidatePropertyDetector();                \
    }                                                                \
    env->Set##name(thread_, name##detector);
    GLOBAL_ENV_DETECTOR_FIELDS(INITIALIZE_PROPERTY_DETECTOR)
#undef INITIALIZE_PROPERTY_DETECTOR
}

void Builtins::InitializeNapiHClass(const JSHandle<GlobalEnv> &env,
                                    const JSHandle<JSHClass> &objFuncClass) const
{
    JSHandle<JSHClass> newJsHClass = JSHClass::Clone(thread_, objFuncClass);
    env->SetObjectFunctionNapiClass(thread_, newJsHClass);
}

void Builtins::SetLazyAccessor(const JSHandle<JSObject> &object, const JSHandle<JSTaggedValue> &key,
    const JSHandle<AccessorData> &accessor) const
{
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(accessor), true, false, true);
    JSObject::DefineOwnProperty(thread_, object, key, descriptor);
}

void Builtins::InitializeForSnapshot(JSThread *thread)
{
    thread_ = thread;
    vm_ = thread->GetEcmaVM();
    factory_ = vm_->GetFactory();

    // Initialize ArkTools
    if (vm_->GetJSOptions().EnableArkTools()) {
        auto env = vm_->GetGlobalEnv();
        auto globalObject = JSHandle<JSObject>::Cast(env->GetJSGlobalObject());
        JSHandle<JSTaggedValue> arkTools(InitializeArkTools(env));
        SetConstantObject(globalObject, "ArkTools", arkTools);
    }
}

void Builtins::InitializeGlobalObject(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);

    // Global object test
    SetFunction(env, globalObject, "print", Global::PrintEntrypoint, 0);
    SetFunction(env, globalObject, "markModuleCollectable", Global::MarkModuleCollectable, 0);
    SetFunction(env, globalObject, "isSendable", Global::IsSendable, 0);
    SetFunction(env, globalObject, "loadNativeModule", Global::LoadNativeModule, 0);
#if ECMASCRIPT_ENABLE_RUNTIME_STAT
    SetFunction(env, globalObject, "startRuntimeStat", Global::StartRuntimeStat, 0);
    SetFunction(env, globalObject, "stopRuntimeStat", Global::StopRuntimeStat, 0);
#endif

#if ECMASCRIPT_ENABLE_OPT_CODE_PROFILER
    SetFunction(env, globalObject, "printOptStat", Global::PrintOptStat, 0);
#endif

#if ECMASCRIPT_ENABLE_MEGA_PROFILER
    SetFunction(env, globalObject, "printMegaICStat", Global::PrintMegaICStat, 0);
#endif

#if ECMASCRIPT_ENABLE_FUNCTION_CALL_TIMER
    SetFunction(env, globalObject, "printFunctionCallStat", Global::PrintFunctionCallStat, 0);
#endif

    if (vm_->GetJSOptions().EnableArkTools()) {
        JSHandle<JSTaggedValue> arkTools(InitializeArkTools(env));
        SetConstantObject(globalObject, "ArkTools", arkTools);
    }

#if ECMASCRIPT_ENABLE_ARK_CONTAINER
    // Set ArkPrivate
    JSHandle<JSTaggedValue> arkPrivate(InitializeArkPrivate(env));
    SetConstantObject(globalObject, "ArkPrivate", arkPrivate);
#endif

    // Global object function
    SetFunction(env, globalObject, "eval", Global::NotSupportEval, FunctionLength::ONE);
    SetFunction(env, globalObject, "isFinite", Global::IsFinite, FunctionLength::ONE,
                kungfu::BuiltinsStubCSigns::GlobalIsFinite);
    SetFunction(env, globalObject, "isNaN", Global::IsNaN, FunctionLength::ONE,
                kungfu::BuiltinsStubCSigns::GlobalIsNan);
    SetFunction(env, globalObject, "decodeURI", Global::DecodeURI, FunctionLength::ONE);
    SetFunction(env, globalObject, "encodeURI", Global::EncodeURI, FunctionLength::ONE);
    SetFunction(env, globalObject, "escape", Global::Escape, FunctionLength::ONE);
    SetFunction(env, globalObject, "unescape", Global::Unescape, FunctionLength::ONE);
    SetFunction(env, globalObject, "decodeURIComponent", Global::DecodeURIComponent, FunctionLength::ONE,
                kungfu::BuiltinsStubCSigns::GlobalDecodeURIComponent);
    SetFunction(env, globalObject, "encodeURIComponent", Global::EncodeURIComponent, FunctionLength::ONE);
    SetFunction(env, globalObject, "__getCurrentModuleName__", Global::GetCurrentModuleName, FunctionLength::ZERO);
    SetFunction(env, globalObject, "__getCurrentBundleName__", Global::GetCurrentBundleName, FunctionLength::ZERO);

    // Global object property
    SetGlobalThis(globalObject, "globalThis", JSHandle<JSTaggedValue>::Cast(globalObject));
    SetConstant(globalObject, "Infinity", JSTaggedValue(base::POSITIVE_INFINITY));
    SetConstant(globalObject, "NaN", JSTaggedValue(base::NAN_VALUE));
    SetConstant(globalObject, "undefined", JSTaggedValue::Undefined());
}

JSHandle<JSHClass> Builtins::CreateFunctionPrototypeHClass(const JSHandle<GlobalEnv> &env,
    const JSHandle<JSTaggedValue> &ObjPrototypeVal) const
{
    uint32_t index = 0;
    PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
    attributes.SetIsInlinedProps(true);
    attributes.SetRepresentation(Representation::TAGGED);
    auto properties = Function::GetFunctionPrototypeProperties();
    uint32_t length = properties.size();
    JSHandle<LayoutInfo> layout = factory_->CreateLayoutInfo(length);
    JSHandle<JSTaggedValue> keyString;
    for (const base::BuiltinsPropertyConfig &each : properties) {
        attributes.SetOffset(index);
        attributes.SetIsAccessor(each.GetIsAccessor());
        attributes.SetWritable(each.GetWritable());
        attributes.SetEnumerable(each.GetEnumerable());
        attributes.SetConfigurable(each.GetConfigurable());
        if (each.GetName() == "[Symbol.hasInstance]") {
            keyString = env->GetHasInstanceSymbol();
        } else {
            keyString = JSHandle<JSTaggedValue>(factory_->NewFromUtf8ReadOnly(each.GetName()));
        }
        layout->AddKey(thread_, index++, keyString.GetTaggedValue(), attributes);
    }
    JSHandle<JSHClass> objPrototypeHClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, length, JSType::JS_FUNCTION, ObjPrototypeVal,
                                JSHandle<JSTaggedValue>(layout));
    objPrototypeHClass->SetCallable(true);
    return objPrototypeHClass;
}

JSHandle<JSHClass> Builtins::CreateFunctionHClass(const JSHandle<JSFunction> &funcPrototype) const
{
    uint32_t index = 0;
    PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
    attributes.SetIsInlinedProps(true);
    attributes.SetRepresentation(Representation::TAGGED);
    auto properties = Function::GetFunctionProperties();
    uint32_t length = properties.size();
    JSHandle<LayoutInfo> layout = factory_->CreateLayoutInfo(length);
    for (const base::BuiltinsPropertyConfig &each : properties) {
        attributes.SetOffset(index);
        attributes.SetIsAccessor(each.GetIsAccessor());
        attributes.SetWritable(each.GetWritable());
        attributes.SetEnumerable(each.GetEnumerable());
        attributes.SetConfigurable(each.GetConfigurable());
        JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(each.GetName()));
        layout->AddKey(thread_, index++, keyString.GetTaggedValue(), attributes);
    }
    JSHandle<JSHClass> objPrototypeHClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, length, JSType::JS_FUNCTION,
                                JSHandle<JSTaggedValue>(funcPrototype), JSHandle<JSTaggedValue>(layout));
    objPrototypeHClass->SetConstructor(true);
    objPrototypeHClass->SetCallable(true);
    return objPrototypeHClass;
}

void Builtins::SetFunctionName(const JSHandle<JSFunction> &ctor, std::string_view name) const
{
    JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly(name));
    SetSFunctionName(ctor, nameString);
}

void Builtins::SetFunctionName(const JSHandle<JSFunction> &ctor, const JSHandle<JSTaggedValue> &name) const
{
    auto nameIndex = JSFunction::NAME_INLINE_PROPERTY_INDEX;
    ctor->SetPropertyInlinedProps(thread_, nameIndex, name.GetTaggedValue());
}

void Builtins::SetFunctionLength(const JSHandle<JSFunction> &ctor, int length) const
{
    JSTaggedValue taggedLength(length);
    auto lengthIndex = JSFunction::LENGTH_INLINE_PROPERTY_INDEX;
    ctor->SetPropertyInlinedProps(thread_, lengthIndex, taggedLength);
}

void Builtins::SetFunctionPrototype(const JSHandle<JSFunction> &ctor, const JSTaggedValue &prototype) const
{
    auto prototypeIndex = JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX;
    ctor->SetPropertyInlinedProps(thread_, prototypeIndex, prototype);
}

void Builtins::SetInlineFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, std::string_view key,
                                 EcmaEntrypoint func, uint32_t index, int length,
                                 kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    SetInlineFunction(env, obj, keyString, func, index, length, builtinId);
}

void Builtins::SetInlineFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                                 const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func, uint32_t index, int length,
                                 kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSFunction> function(NewFunction(env, key, func, length, builtinId));
    obj->SetPropertyInlinedProps(thread_, index, function.GetTaggedValue());
}

JSHandle<JSFunction> Builtins::SetInlineFunctionAndRetJSFunction(const JSHandle<GlobalEnv> &env,
    const JSHandle<JSObject> &obj, std::string_view key,
    EcmaEntrypoint func, uint32_t index, int length,
    kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    JSHandle<JSFunction> function(NewFunction(env, keyString, func, length, builtinId));
    obj->SetPropertyInlinedProps(thread_, index, function.GetTaggedValue());
    return function;
}

void Builtins::SetInlineAccessor(const JSHandle<JSObject> &obj, uint32_t index,
                                 const JSHandle<JSTaggedValue> &getter, const JSHandle<JSTaggedValue> &setter) const
{
    JSHandle<AccessorData> accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, getter);
    accessor->SetSetter(thread_, setter);
    obj->SetPropertyInlinedProps(thread_, index, accessor.GetTaggedValue());
}

void Builtins::StrictModeForbiddenAccessCallerArguments(const JSHandle<GlobalEnv> &env, uint32_t &index,
                                                        const JSHandle<JSObject> &prototype) const
{
    auto function = JSHandle<JSTaggedValue>::Cast(
        factory_->NewJSFunction(env, reinterpret_cast<void *>(JSFunction::AccessCallerArgumentsThrowTypeError)));
    // "caller"
    SetInlineAccessor(prototype, index++, function, function);
    // "arguments"
    SetInlineAccessor(prototype, index++, function, function);
}

void Builtins::InitializeFunctionPrototype(const JSHandle<GlobalEnv> &env, JSHandle<JSFunction> &funcFuncPrototype,
                                           JSHandle<JSFunction> &funcFunc) const
{
    auto funcFuncPrototypeObj = JSHandle<JSObject>(funcFuncPrototype);
    SetFunctionLength(funcFuncPrototype, FunctionLength::ZERO);
    SetFunctionName(funcFuncPrototype, thread_->GlobalConstants()->GetHandledEmptyString());
    uint32_t fieldIndex = 2; // 2: length and name
    funcFuncPrototype->SetPropertyInlinedProps(thread_, fieldIndex++, funcFunc.GetTaggedValue()); // constructor
    StrictModeForbiddenAccessCallerArguments(env, fieldIndex, funcFuncPrototypeObj);
    // Function.prototype method
    for (const base::BuiltinFunctionEntry &entry: Function::GetFunctionPrototypeFunctions()) {
        SetInlineFunction(env, funcFuncPrototypeObj, entry.GetName(), entry.GetEntrypoint(), fieldIndex++,
                          entry.GetLength(), entry.GetBuiltinStubId());
    }
    // 19.2.3.5 Function.prototype.toString ( )
    SetInlineFunction(env, funcFuncPrototypeObj, thread_->GlobalConstants()->GetHandledToStringString(),
                      Function::FunctionPrototypeToString, fieldIndex++, FunctionLength::ZERO);
    JSHandle<JSFunction> function = SetInlineFunctionAndRetJSFunction(
        env, funcFuncPrototypeObj, "[Symbol.hasInstance]",
        Function::FunctionPrototypeHasInstance, fieldIndex++, FunctionLength::ONE,
        BUILTINS_STUB_ID(FunctionPrototypeHasInstance));
    env->SetHasInstanceFunction(thread_, function);
}

void Builtins::InitializeFunction(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Function.prototype.hclass
    JSHandle<JSHClass> funcPrototypeHClass = CreateFunctionPrototypeHClass(env, objFuncPrototypeVal);
    // Function.prototype
    JSHandle<JSFunction> funcFuncPrototype = factory_->NewJSFunctionByHClassWithoutAccessor(
        reinterpret_cast<void *>(Function::FunctionPrototypeInvokeSelf), funcPrototypeHClass);

    JSHandle<JSTaggedValue> funcFuncPrototypeValue(funcFuncPrototype);
    // Function.prototype_or_hclass
    JSHandle<JSHClass> funcFuncIntanceHClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, funcFuncPrototypeValue);
    funcFuncIntanceHClass->SetConstructor(true);
    // Function.hclass
    JSHandle<JSHClass> funcHClass = CreateFunctionHClass(funcFuncPrototype);
    // Function = new Function() (forbidden use NewBuiltinConstructor)
    JSHandle<JSFunction> funcFunc =
        factory_->NewJSFunctionByHClassWithoutAccessor(reinterpret_cast<void *>(Function::FunctionConstructor),
        funcHClass, FunctionKind::BUILTIN_CONSTRUCTOR);

    // set properties for Function
    SetFunctionLength(funcFunc, FunctionLength::ONE);
    SetFunctionName(funcFunc, "Function");
    SetFunctionPrototype(funcFunc, funcFuncPrototypeValue.GetTaggedValue());

    JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly("Function"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor descriptor2(thread_, JSHandle<JSTaggedValue>::Cast(funcFunc), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, nameString, descriptor2);

    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, funcFunc, funcFuncIntanceHClass.GetTaggedValue());
    env->SetFunctionFunction(thread_, funcFunc);
    env->SetFunctionPrototype(thread_, funcFuncPrototype);

    JSHandle<JSHClass> normalFuncClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, env->GetFunctionPrototype());
    env->SetNormalFunctionClass(thread_, normalFuncClass);

    JSHandle<JSHClass> jSIntlBoundFunctionClass =
        factory_->CreateFunctionClass(FunctionKind::NORMAL_FUNCTION, JSIntlBoundFunction::SIZE,
                                      JSType::JS_INTL_BOUND_FUNCTION, env->GetFunctionPrototype());
    env->SetJSIntlBoundFunctionClass(thread_, jSIntlBoundFunctionClass);

    JSHandle<JSHClass> constructorFunctionClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, env->GetFunctionPrototype());
    constructorFunctionClass->SetConstructor(true);
    env->SetConstructorFunctionClass(thread_, constructorFunctionClass);
    InitializeFunctionPrototype(env, funcFuncPrototype, funcFunc);
}

void Builtins::InitializeObject(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &objFuncPrototype,
                                const JSHandle<JSObject> &objFunc)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Object method.
    for (const base::BuiltinFunctionEntry &entry: Object::GetObjectFunctions()) {
        SetFunction(env, objFunc, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // Object.prototype method
    for (const base::BuiltinFunctionEntry &entry: Object::GetObjectPrototypeFunctions()) {
        SetFunction(env, objFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // B.2.2.1 Object.prototype.__proto__
    JSHandle<JSTaggedValue> protoKey(factory_->NewFromASCIIReadOnly("__proto__"));
    JSHandle<JSTaggedValue> protoGetter = CreateGetter(env, Object::ProtoGetter, "__proto__", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> protoSetter = CreateSetter(env, Object::ProtoSetter, "__proto__", FunctionLength::ONE);
    SetAccessor(objFuncPrototype, protoKey, protoGetter, protoSetter);

    GlobalEnvConstants *globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    globalConst->SetConstant(ConstantIndex::OBJECT_GET_PROTO_INDEX, protoGetter);

    GlobalIndex globalIndex;
    globalIndex.UpdateGlobalEnvId(static_cast<size_t>(GlobalEnvField::OBJECT_FUNCTION_INDEX));
    thread_->SetInitialBuiltinGlobalHClass(objFunc->GetJSHClass(), globalIndex);
}

void Builtins::InitializeSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // Symbol.prototype
    JSHandle<JSObject> symbolFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> symbolFuncPrototypeValue(symbolFuncPrototype);

    // Symbol.prototype_or_hclass
    JSHandle<JSHClass> symbolFuncInstanceHClass =
        factory_->NewEcmaHClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, symbolFuncPrototypeValue);

    // Symbol = new Function()
    JSHandle<JSObject> symbolFunction(
        NewBuiltinConstructor(env, symbolFuncPrototype, Symbol::SymbolConstructor, "Symbol", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
        JSHandle<JSFunction>(symbolFunction), symbolFuncInstanceHClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(symbolFunction), true, false, true);
    JSObject::DefineOwnProperty(thread_, symbolFuncPrototype, constructorKey, descriptor);

    for (const base::BuiltinFunctionEntry &entry: Symbol::GetSymbolFunctions()) {
        SetFunction(env, symbolFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // Symbol attributes
#define REGISTER_SYMBOL(name, Name) \
    SetNoneAttributeProperty(symbolFunction, #name, env->Get##Name##Symbol());

BUILTIN_ALL_SYMBOLS(REGISTER_SYMBOL)
    env->SetSymbolFunction(thread_, symbolFunction);

    // symbol.prototype.description
    PropertyDescriptor descriptionDesc(thread_);
    JSHandle<JSTaggedValue> getterKey(factory_->NewFromASCIIReadOnly("description"));
    JSHandle<JSTaggedValue> getter(factory_->NewJSFunction(env, reinterpret_cast<void *>(Symbol::DescriptionGetter)));
    SetGetter(symbolFuncPrototype, getterKey, getter);

    // Setup symbol.prototype[@@toPrimitive]
    SetFunctionAtSymbol<JSSymbol::SYMBOL_TO_PRIMITIVE_TYPE>(
        env, symbolFuncPrototype, env->GetToPrimitiveSymbol(), "[Symbol.toPrimitive]",
        Symbol::ToPrimitive, FunctionLength::ONE);
    // install the Symbol.prototype methods
    SetFunction(env, symbolFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), Symbol::ToString,
                FunctionLength::ZERO);
    SetFunction(env, symbolFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(), Symbol::ValueOf,
                FunctionLength::ZERO);

    // Setup %SymbolPrototype%
    SetStringTagSymbol(env, symbolFuncPrototype, "Symbol");

    JSHandle<JSTaggedValue> holeySymbol(factory_->NewPrivateNameSymbolWithChar("holey"));
    env->SetHoleySymbol(thread_, holeySymbol.GetTaggedValue());
    JSHandle<JSTaggedValue> elementIcSymbol(factory_->NewPrivateNameSymbolWithChar("element-ic"));
    env->SetElementICSymbol(thread_, elementIcSymbol.GetTaggedValue());
}

void Builtins::InitializeSymbolWithRealm(const JSHandle<GlobalEnv> &realm,
                                         const JSHandle<JSHClass> &objFuncInstanceHClass)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    // Symbol.prototype
    JSHandle<JSObject> symbolFuncPrototype = factory_->NewJSObjectWithInit(objFuncInstanceHClass);
    JSHandle<JSTaggedValue> symbolFuncPrototypeValue(symbolFuncPrototype);

    // Symbol.prototype_or_hclass
    JSHandle<JSHClass> symbolFuncInstanceHClass =
        factory_->NewEcmaHClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, symbolFuncPrototypeValue);

    // Symbol = new Function()
    JSHandle<JSObject> symbolFunction(
        NewBuiltinConstructor(realm, symbolFuncPrototype, Symbol::SymbolConstructor, "Symbol", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(symbolFunction),
                                                     symbolFuncInstanceHClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = thread_->GlobalConstants()->GetHandledConstructorString();
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(symbolFunction), true, false, true);
    JSObject::DefineOwnProperty(thread_, symbolFuncPrototype, constructorKey, descriptor);

    for (const base::BuiltinFunctionEntry &entry: Symbol::GetSymbolFunctions()) {
        SetFunction(realm, symbolFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

#define BUILTIN_SYMBOL_CREATE_WITH_REALM(name, Name)                            \
    SetNoneAttributeProperty(symbolFunction, #name, env->Get##Name##Symbol());  \
    realm->Set##Name##Symbol(thread_, env->Get##Name##Symbol());

    realm->SetSymbolFunction(thread_, symbolFunction);
    // Symbol attribute
    BUILTIN_ALL_SYMBOLS(BUILTIN_SYMBOL_CREATE_WITH_REALM)

    // symbol.prototype.description
    PropertyDescriptor descriptionDesc(thread_);
    JSHandle<JSTaggedValue> getterKey(factory_->NewFromASCIIReadOnly("description"));
    JSHandle<JSTaggedValue> getter(factory_->NewJSFunction(realm, reinterpret_cast<void *>(Symbol::DescriptionGetter)));
    SetGetter(symbolFuncPrototype, getterKey, getter);

    // Setup symbol.prototype[@@toPrimitive]
    SetFunctionAtSymbol<JSSymbol::SYMBOL_TO_PRIMITIVE_TYPE>(realm, symbolFuncPrototype, env->GetToPrimitiveSymbol(),
                                                            "[Symbol.toPrimitive]", Symbol::ToPrimitive,
                                                            FunctionLength::ONE);
    // install the Symbol.prototype methods
    for (const base::BuiltinFunctionEntry &entry: Symbol::GetSymbolPrototypeFunctions()) {
        SetFunction(realm, symbolFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // Setup %SymbolPrototype%
    SetStringTagSymbol(realm, symbolFuncPrototype, "Symbol");

    JSHandle<JSTaggedValue> holeySymbol(factory_->NewPrivateNameSymbolWithChar("holey"));
    realm->SetHoleySymbol(thread_, holeySymbol.GetTaggedValue());
    JSHandle<JSTaggedValue> elementIcSymbol(factory_->NewPrivateNameSymbolWithChar("element-ic"));
    realm->SetElementICSymbol(thread_, elementIcSymbol.GetTaggedValue());
}
#undef BUILTIN_SYMBOL_CREATE_WITH_REALM

void Builtins::InitializeNumber(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject,
                                const JSHandle<JSHClass> &primRefObjHClass)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Number.prototype
    JSHandle<JSTaggedValue> toObject(thread_, JSTaggedValue(FunctionLength::ZERO));
    JSHandle<JSObject> numFuncPrototype =
        JSHandle<JSObject>::Cast(factory_->NewJSPrimitiveRef(primRefObjHClass, toObject));
    JSHandle<JSTaggedValue> numFuncPrototypeValue(numFuncPrototype);

    // Number.prototype_or_hclass
    JSHandle<JSHClass> numFuncInstanceHClass =
        factory_->NewEcmaHClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, numFuncPrototypeValue);

    // Number = new Function()
    JSHandle<JSObject> numFunction(
        NewBuiltinConstructor(env, numFuncPrototype, Number::NumberConstructor, "Number", FunctionLength::ONE,
            BUILTINS_STUB_ID(NumberConstructor)));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(numFunction),
                                                     numFuncInstanceHClass.GetTaggedValue());

    // Number.prototype method
    for (const base::BuiltinFunctionEntry &entry: Number::GetNumberPrototypeFunctions()) {
        SetFunction(env, numFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // Number method
    for (const base::BuiltinFunctionEntry &entry: Number::GetNumberNonGlobalFunctions()) {
        SetFunction(env, numFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    for (const base::BuiltinFunctionEntry &entry: Number::GetNumberGlobalFunctions()) {
        SetFuncToObjAndGlobal(env, globalObject, numFunction,
                              entry.GetName(), entry.GetEntrypoint(), entry.GetLength(), entry.GetBuiltinStubId());
    }
    // Number constant
    for (const base::BuiltinConstantEntry &entry: Number::GetNumberConstants()) {
        SetConstant(numFunction, entry.GetName(), entry.GetTaggedValue());
    }

    env->SetNumberFunction(thread_, numFunction);
    env->SetNumberPrototype(thread_, numFuncPrototype);
}
void Builtins::InitializeBigIntWithRealm(const JSHandle<GlobalEnv> &realm) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    realm->SetBigIntFunction(thread_, env->GetBigIntFunction());

    JSHandle<JSTaggedValue> nameString(factory_->NewFromASCIIReadOnly("BigInt"));
    JSHandle<JSObject> globalObject(thread_, realm->GetGlobalObject());
    PropertyDescriptor descriptor(thread_, env->GetBigIntFunction(), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, nameString, descriptor);
}

void Builtins::InitializeBigInt(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &primRefObjHClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // BigInt.prototype
    JSHandle<JSObject> bigIntFuncPrototype = factory_->NewJSObjectWithInit(primRefObjHClass);
    JSHandle<JSTaggedValue> bigIntFuncPrototypeValue(bigIntFuncPrototype);

    // BigInt.prototype_or_hclass
    JSHandle<JSHClass> bigIntFuncInstanceHClass =
        factory_->NewEcmaHClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, bigIntFuncPrototypeValue);
    // BigInt = new Function()
    JSHandle<JSObject> bigIntFunction(
        NewBuiltinConstructor(env, bigIntFuncPrototype, BuiltinsBigInt::BigIntConstructor, "BigInt",
                              FunctionLength::ONE, kungfu::BuiltinsStubCSigns::BigIntConstructor));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(bigIntFunction),
                                                     bigIntFuncInstanceHClass.GetTaggedValue());

    // BigInt.prototype method
    for (const auto &entry : BuiltinsBigInt::GetBigIntPrototypeFunctions()) {
        SetFunction(env, bigIntFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // BigInt method
    for (const auto &entry : BuiltinsBigInt::GetBigIntFunctions()) {
        SetFunction(env, bigIntFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // @@ToStringTag
    SetStringTagSymbol(env, bigIntFuncPrototype, "BigInt");
    env->SetBigIntFunction(thread_, bigIntFunction);
}

void Builtins::InitializeDate(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Date.prototype
    JSHandle<JSHClass> dateFuncPrototypeHClass = factory_->NewEcmaHClass(
        JSObject::SIZE, Date::GetNumPrototypeInlinedProperties(), JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> dateFuncPrototype = factory_->NewJSObjectWithInit(dateFuncPrototypeHClass);
    JSHandle<JSTaggedValue> dateFuncPrototypeValue(dateFuncPrototype);

    // Date.prototype_or_hclass
    JSHandle<JSHClass> dateFuncInstanceHClass =
        factory_->NewEcmaHClass(JSDate::SIZE, JSType::JS_DATE, dateFuncPrototypeValue);

    // Date = new Function()
    JSHandle<JSObject> dateFunction(
        NewBuiltinConstructor(env, dateFuncPrototype, Date::DateConstructor, "Date", FunctionLength::ONE,
                              BUILTINS_STUB_ID(DateConstructor)));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(dateFunction),
                                                     dateFuncInstanceHClass.GetTaggedValue());

    // Date.prototype method
    for (const base::BuiltinFunctionEntry &entry: Date::GetDatePrototypeFunctions()) {
        SetFunction(env, dateFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    SetFunctionAtSymbol(env, dateFuncPrototype, env->GetToPrimitiveSymbol(), "[Symbol.toPrimitive]", Date::ToPrimitive,
                        FunctionLength::ONE);

    // Date method
    for (const base::BuiltinFunctionEntry &entry: Date::GetDateFunctions()) {
        SetFunction(env, dateFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // Date.length
    SetConstant(dateFunction, "length", JSTaggedValue(Date::UTC_LENGTH));

    env->SetDateFunction(thread_, dateFunction);
    env->SetDatePrototype(thread_, dateFuncPrototype);
    thread_->SetInitialBuiltinHClass(BuiltinTypeId::DATE,
        dateFunction->GetJSHClass(),
        *dateFuncInstanceHClass,
        dateFuncPrototype->GetJSHClass());
}

void Builtins::LazyInitializeDate(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("Date"));
    auto accessor = factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::Date));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetDateFunction(thread_, accessor);
    env->SetDatePrototype(thread_, accessor);
}

void Builtins::InitializeBoolean(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &primRefObjHClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Boolean.prototype
    JSHandle<JSTaggedValue> toObject(thread_, JSTaggedValue::False());
    JSHandle<JSObject> booleanFuncPrototype =
        JSHandle<JSObject>::Cast(factory_->NewJSPrimitiveRef(primRefObjHClass, toObject));
    JSHandle<JSTaggedValue> booleanFuncPrototypeValue(booleanFuncPrototype);

    // Boolean.prototype_or_hclass
    JSHandle<JSHClass> booleanFuncInstanceHClass =
        factory_->NewEcmaHClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, booleanFuncPrototypeValue);

    // new Boolean Function()
    JSHandle<JSFunction> booleanFunction = NewBuiltinConstructor(env, booleanFuncPrototype, Boolean::BooleanConstructor,
        "Boolean", FunctionLength::ONE, BUILTINS_STUB_ID(BooleanConstructor));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, booleanFunction,
                                                     booleanFuncInstanceHClass.GetTaggedValue());

    // Boolean.prototype method
    SetFunction(env, booleanFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(),
                Boolean::BooleanPrototypeToString, FunctionLength::ZERO);
    SetFunction(env, booleanFuncPrototype, thread_->GlobalConstants()->GetHandledValueOfString(),
                Boolean::BooleanPrototypeValueOf, FunctionLength::ZERO);

    env->SetBooleanFunction(thread_, booleanFunction);
    env->SetBooleanPrototype(thread_, booleanFuncPrototype);
}

void Builtins::InitializeProxy(const JSHandle<GlobalEnv> &env)
{
    // 2: The number of parameters is 2
    JSHandle<JSObject> proxyFunction(InitializeExoticConstructor(env, Proxy::ProxyConstructor, "Proxy", 2));

    // Proxy method
    SetFunction(env, proxyFunction, "revocable", Proxy::Revocable, FunctionLength::TWO);
    env->SetProxyFunction(thread_, proxyFunction);
}

JSHandle<JSFunction> Builtins::InitializeExoticConstructor(const JSHandle<GlobalEnv> &env, EcmaEntrypoint ctorFunc,
                                                           std::string_view name, int length)
{
    JSHandle<JSFunction> ctor =
        factory_->NewJSFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_PROXY_CONSTRUCTOR,
                                BUILTINS_STUB_ID(ProxyConstructor));

    JSFunction::SetFunctionLength(thread_, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly(name));
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(ctor), nameString,
                                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined()));

    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, nameString, descriptor);
    return ctor;
}

void Builtins::InitializeAsyncFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // AsyncFunction.prototype
    JSHandle<JSObject> asyncFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSObject::SetPrototype(thread_, asyncFuncPrototype, env->GetFunctionPrototype());
    JSHandle<JSTaggedValue> asyncFuncPrototypeValue(asyncFuncPrototype);

    // AsyncFunction.prototype_or_hclass
    JSHandle<JSHClass> asyncFuncInstanceHClass =
        factory_->NewEcmaHClass(JSAsyncFunction::SIZE, JSType::JS_ASYNC_FUNCTION, asyncFuncPrototypeValue);

    // AsyncFunction = new Function()
    JSHandle<JSFunction> asyncFunction = NewBuiltinConstructor(
        env, asyncFuncPrototype, AsyncFunction::AsyncFunctionConstructor, "AsyncFunction", FunctionLength::ONE);
    JSObject::SetPrototype(thread_, JSHandle<JSObject>::Cast(asyncFunction), env->GetFunctionFunction());
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor asyncDesc(thread_, JSHandle<JSTaggedValue>::Cast(asyncFunction), false, false, true);
    JSObject::DefineOwnProperty(thread_, asyncFuncPrototype, constructorKey, asyncDesc);
    asyncFunction->SetProtoOrHClass(thread_, asyncFuncInstanceHClass.GetTaggedValue());

    // AsyncFunction.prototype property
    SetStringTagSymbol(env, asyncFuncPrototype, "AsyncFunction");
    env->SetAsyncFunction(thread_, asyncFunction);
    env->SetAsyncFunctionPrototype(thread_, asyncFuncPrototype);
}

void Builtins::InitializeAllTypeError(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    // Error.prototype
    JSHandle<JSObject> errorFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> errorFuncPrototypeValue(errorFuncPrototype);
    // Error.prototype_or_hclass
    JSHandle<JSHClass> errorFuncInstanceHClass =
        factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_ERROR, errorFuncPrototypeValue);
    // Error() = new Function()
    JSHandle<JSFunction> errorFunction(
        NewBuiltinConstructor(env, errorFuncPrototype, Error::ErrorConstructor, "Error", FunctionLength::ONE));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, errorFunction, errorFuncInstanceHClass.GetTaggedValue());

    // Error.prototype method
    SetFunction(env, errorFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(), Error::ToString,
                FunctionLength::ZERO);

    // Error.prototype Attribute
    SetAttribute(errorFuncPrototype, "name", "Error");
    SetAttribute(errorFuncPrototype, "message", "");
    env->SetErrorFunction(thread_, errorFunction);

    JSHandle<JSHClass> nativeErrorFuncClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, env->GetErrorFunction());
    nativeErrorFuncClass->SetConstructor(true);
    env->SetNativeErrorFunctionClass(thread_, nativeErrorFuncClass);

    JSHandle<JSHClass> errorNativeFuncInstanceHClass =
        factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, errorFuncPrototypeValue);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_RANGE_ERROR);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_REFERENCE_ERROR);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_TYPE_ERROR);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_AGGREGATE_ERROR);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_URI_ERROR);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_SYNTAX_ERROR);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_EVAL_ERROR);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_OOM_ERROR);
    InitializeError(env, errorNativeFuncInstanceHClass, JSType::JS_TERMINATION_ERROR);

    JSHandle<EcmaString> handleMsg = factory_->NewFromUtf8ReadOnly("Default oom error");
    JSHandle<JSObject> oomError = factory_->NewJSError(ErrorType::OOM_ERROR, handleMsg, StackCheck::YES);
    env->SetOOMErrorObject(thread_, oomError);
}

void Builtins::InitializeAllTypeErrorWithRealm(const JSHandle<GlobalEnv> &realm) const
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    realm->SetErrorFunction(thread_, env->GetErrorFunction());
    realm->SetNativeErrorFunctionClass(thread_, env->GetNativeErrorFunctionClass());

    SetErrorWithRealm(realm, JSType::JS_RANGE_ERROR);
    SetErrorWithRealm(realm, JSType::JS_REFERENCE_ERROR);
    SetErrorWithRealm(realm, JSType::JS_TYPE_ERROR);
    SetErrorWithRealm(realm, JSType::JS_AGGREGATE_ERROR);
    SetErrorWithRealm(realm, JSType::JS_URI_ERROR);
    SetErrorWithRealm(realm, JSType::JS_SYNTAX_ERROR);
    SetErrorWithRealm(realm, JSType::JS_EVAL_ERROR);
    SetErrorWithRealm(realm, JSType::JS_OOM_ERROR);
    SetErrorWithRealm(realm, JSType::JS_TERMINATION_ERROR);
}

void Builtins::SetErrorWithRealm(const JSHandle<GlobalEnv> &realm, const JSType &errorTag) const
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread_, realm->GetGlobalObject());
    JSHandle<JSTaggedValue> nameString;
    JSHandle<JSTaggedValue> nativeErrorFunction;
    switch (errorTag) {
        case JSType::JS_RANGE_ERROR:
            nativeErrorFunction = env->GetRangeErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledRangeErrorString());
            realm->SetRangeErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_EVAL_ERROR:
            nativeErrorFunction = env->GetEvalErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledEvalErrorString());
            realm->SetEvalErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_REFERENCE_ERROR:
            nativeErrorFunction = env->GetReferenceErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledReferenceErrorString());
            realm->SetReferenceErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_TYPE_ERROR:
            nativeErrorFunction = env->GetTypeErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledTypeErrorString());
            realm->SetTypeErrorFunction(thread_, nativeErrorFunction);
            realm->SetThrowTypeError(thread_, env->GetThrowTypeError());
            break;
        case JSType::JS_AGGREGATE_ERROR:
            nativeErrorFunction = env->GetAggregateErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledAggregateErrorString());
            realm->SetAggregateErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_URI_ERROR:
            nativeErrorFunction = env->GetURIErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledURIErrorString());
            realm->SetURIErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_SYNTAX_ERROR:
            nativeErrorFunction = env->GetSyntaxErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledSyntaxErrorString());
            realm->SetSyntaxErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_OOM_ERROR:
            nativeErrorFunction = env->GetOOMErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledOOMErrorString());
            realm->SetOOMErrorFunction(thread_, nativeErrorFunction);
            break;
        case JSType::JS_TERMINATION_ERROR:
            nativeErrorFunction = env->GetTerminationErrorFunction();
            nameString = JSHandle<JSTaggedValue>(thread_->GlobalConstants()->GetHandledTerminationErrorString());
            realm->SetTerminationErrorFunction(thread_, nativeErrorFunction);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable, errorTag: " << static_cast<size_t>(errorTag);
            UNREACHABLE();
    }
    PropertyDescriptor descriptor(thread_, nativeErrorFunction, true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, nameString, descriptor);
}

void Builtins::GeneralUpdateError(ErrorParameter *error, EcmaEntrypoint constructor, EcmaEntrypoint method,
                                  std::string_view name, JSType type) const
{
    error->nativeConstructor = constructor;
    error->nativeMethod = method;
    error->nativePropertyName = name;
    error->nativeJstype = type;
}

void Builtins::InitializeError(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass,
                               const JSType &errorTag) const
{
    // NativeError.prototype
    JSHandle<JSObject> nativeErrorFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> nativeErrorFuncPrototypeValue(nativeErrorFuncPrototype);

    ErrorParameter errorParameter{RangeError::RangeErrorConstructor, RangeError::ToString, "RangeError",
                                  JSType::JS_RANGE_ERROR};
    switch (errorTag) {
        case JSType::JS_RANGE_ERROR:
            GeneralUpdateError(&errorParameter, RangeError::RangeErrorConstructor, RangeError::ToString, "RangeError",
                               JSType::JS_RANGE_ERROR);
            break;
        case JSType::JS_EVAL_ERROR:
            GeneralUpdateError(&errorParameter, EvalError::EvalErrorConstructor, EvalError::ToString, "EvalError",
                               JSType::JS_EVAL_ERROR);
            break;
        case JSType::JS_REFERENCE_ERROR:
            GeneralUpdateError(&errorParameter, ReferenceError::ReferenceErrorConstructor, ReferenceError::ToString,
                               "ReferenceError", JSType::JS_REFERENCE_ERROR);
            break;
        case JSType::JS_TYPE_ERROR:
            GeneralUpdateError(&errorParameter, TypeError::TypeErrorConstructor, TypeError::ToString, "TypeError",
                               JSType::JS_TYPE_ERROR);
            break;
        case JSType::JS_AGGREGATE_ERROR:
            GeneralUpdateError(&errorParameter, AggregateError::AggregateErrorConstructor, AggregateError::ToString,
                               "AggregateError", JSType::JS_AGGREGATE_ERROR);
            break;
        case JSType::JS_URI_ERROR:
            GeneralUpdateError(&errorParameter, URIError::URIErrorConstructor, URIError::ToString, "URIError",
                               JSType::JS_URI_ERROR);
            break;
        case JSType::JS_SYNTAX_ERROR:
            GeneralUpdateError(&errorParameter, SyntaxError::SyntaxErrorConstructor, SyntaxError::ToString,
                               "SyntaxError", JSType::JS_SYNTAX_ERROR);
            break;
        case JSType::JS_OOM_ERROR:
            GeneralUpdateError(&errorParameter, OOMError::OOMErrorConstructor, OOMError::ToString,
                               "OutOfMemoryError", JSType::JS_OOM_ERROR);
            break;
        case JSType::JS_TERMINATION_ERROR:
            GeneralUpdateError(&errorParameter, TerminationError::TerminationErrorConstructor,
                               TerminationError::ToString, "TerminationError", JSType::JS_TERMINATION_ERROR);
            break;
        default:
            break;
    }

    // NativeError.prototype_or_hclass
    JSHandle<JSHClass> nativeErrorFuncInstanceHClass =
        factory_->NewEcmaHClass(JSObject::SIZE, errorParameter.nativeJstype, nativeErrorFuncPrototypeValue);

    // NativeError() = new Error()
    FunctionLength functionLength = FunctionLength::ONE;
    if (errorTag == JSType::JS_AGGREGATE_ERROR) {
        functionLength = FunctionLength::TWO;
    }
    JSHandle<JSFunction> nativeErrorFunction =
        factory_->NewJSNativeErrorFunction(env, reinterpret_cast<void *>(errorParameter.nativeConstructor));
    InitializeCtor(env, nativeErrorFuncPrototype, nativeErrorFunction, errorParameter.nativePropertyName,
                   functionLength);

    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, nativeErrorFunction,
                                                     nativeErrorFuncInstanceHClass.GetTaggedValue());

    // NativeError.prototype method
    SetFunction(env, nativeErrorFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(),
                errorParameter.nativeMethod, FunctionLength::ZERO);

    // Error.prototype Attribute
    SetAttribute(nativeErrorFuncPrototype, "name", errorParameter.nativePropertyName);
    SetAttribute(nativeErrorFuncPrototype, "message", "");

    if (errorTag == JSType::JS_RANGE_ERROR) {
        env->SetRangeErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_REFERENCE_ERROR) {
        env->SetReferenceErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_TYPE_ERROR) {
        env->SetTypeErrorFunction(thread_, nativeErrorFunction);
        JSHandle<JSFunction> throwTypeErrorFunction =
            factory_->NewJSFunction(env, reinterpret_cast<void *>(TypeError::ThrowTypeError));
        JSFunction::SetFunctionLength(thread_, throwTypeErrorFunction, JSTaggedValue(1), false);
        JSObject::PreventExtensions(thread_, JSHandle<JSObject>::Cast(throwTypeErrorFunction));
        env->SetThrowTypeError(thread_, throwTypeErrorFunction);
    } else if (errorTag == JSType::JS_AGGREGATE_ERROR) {
        env->SetAggregateErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_URI_ERROR) {
        env->SetURIErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_SYNTAX_ERROR) {
        env->SetSyntaxErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_EVAL_ERROR) {
        env->SetEvalErrorFunction(thread_, nativeErrorFunction);
    } else if (errorTag == JSType::JS_OOM_ERROR) {
        env->SetOOMErrorFunction(thread_, nativeErrorFunction);
    } else {
        env->SetTerminationErrorFunction(thread_, nativeErrorFunction);
    }
}  // namespace panda::ecmascript

void Builtins::InitializeCtor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                              const JSHandle<JSFunction> &ctor, std::string_view name, int length) const
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSFunction::SetFunctionLength(thread_, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly(name));
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(ctor), nameString,
                                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined()));
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor1(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread_, prototype, constructorKey, descriptor1);

    /* set "prototype" in constructor */
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, ctor, prototype.GetTaggedValue());

    if (!JSTaggedValue::SameValue(nameString, thread_->GlobalConstants()->GetHandledAsyncFunctionString())) {
        JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
        PropertyDescriptor descriptor2(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
        JSObject::DefineOwnProperty(thread_, globalObject, nameString, descriptor2);
    }
}

void Builtins::InitializeSet(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // Set.prototype
    JSHandle<JSHClass> setFuncPrototypeHClass = factory_->NewEcmaHClass(
        JSObject::SIZE, BuiltinsSet::GetNumPrototypeInlinedProperties(), JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> setFuncPrototype = factory_->NewJSObjectWithInit(setFuncPrototypeHClass);
    JSHandle<JSTaggedValue> setFuncPrototypeValue(setFuncPrototype);
    // Set.prototype_or_hclass
    JSHandle<JSHClass> setFuncInstanceHClass =
        factory_->NewEcmaHClass(JSSet::SIZE, JSType::JS_SET, setFuncPrototypeValue);
    // Set() = new Function()
    JSHandle<JSTaggedValue> setFunction(
        NewBuiltinConstructor(env, setFuncPrototype, BuiltinsSet::SetConstructor, "Set", FunctionLength::ZERO,
                              BUILTINS_STUB_ID(SetConstructor)));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(setFunction),
                                                     setFuncInstanceHClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(setFuncPrototype), constructorKey, setFunction);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    // Set.prototype functions, excluding keys()
    for (const base::BuiltinFunctionEntry &entry: BuiltinsSet::GetSetPrototypeFunctions()) {
        SetFunction(env, setFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // Set.prototype.keys, which is strictly equal to Set.prototype.values
    JSHandle<JSTaggedValue> keys(factory_->NewFromASCIIReadOnly("keys"));
    JSHandle<JSTaggedValue> values(factory_->NewFromASCIIReadOnly("values"));
    JSHandle<JSTaggedValue> valuesFunc =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>::Cast(setFuncPrototype), values);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    PropertyDescriptor descriptor(thread_, valuesFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, setFuncPrototype, keys, descriptor);

    // @@ToStringTag
    SetStringTagSymbol(env, setFuncPrototype, "Set");

    // 23.1.3.10get Set.prototype.size
    JSHandle<JSTaggedValue> sizeGetter = CreateGetter(env, BuiltinsSet::GetSize, "size", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> sizeKey(factory_->NewFromASCIIReadOnly("size"));
    SetGetter(setFuncPrototype, sizeKey, sizeGetter);

    // 23.1.2.2get Set [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsSet::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(setFunction), speciesSymbol, speciesGetter);

    // %SetPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSObject::DefineOwnProperty(thread_, setFuncPrototype, iteratorSymbol, descriptor);

    env->SetBuiltinsSetFunction(thread_, setFunction);
    env->SetSetPrototype(thread_, setFuncPrototype);
    env->SetSetProtoValuesFunction(thread_, valuesFunc);
    thread_->SetInitialBuiltinHClass(BuiltinTypeId::SET,
        setFunction->GetTaggedObject()->GetClass(),
        *setFuncInstanceHClass,
        setFuncPrototype->GetJSHClass());
}

void Builtins::LazyInitializeSet(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("Set"));
    auto accessor = factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::Set));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetBuiltinsSetFunction(thread_, accessor);
    env->SetSetPrototype(thread_, accessor);
    env->SetSetProtoValuesFunction(thread_, accessor);
}

void Builtins::InitializeMap(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // Map.prototype
    JSHandle<JSHClass> mapFuncPrototypeHClass = factory_->NewEcmaHClass(
        JSObject::SIZE, BuiltinsMap::GetNumPrototypeInlinedProperties(), JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> mapFuncPrototype = factory_->NewJSObjectWithInit(mapFuncPrototypeHClass);
    JSHandle<JSTaggedValue> mapFuncPrototypeValue(mapFuncPrototype);
    // Map.prototype_or_hclass
    JSHandle<JSHClass> mapFuncInstanceHClass =
        factory_->NewEcmaHClass(JSMap::SIZE, JSType::JS_MAP, mapFuncPrototypeValue);
    env->SetMapClass(thread_, mapFuncInstanceHClass);

    // Map() = new Function()
    JSHandle<JSTaggedValue> mapFunction(
        NewBuiltinConstructor(env, mapFuncPrototype, BuiltinsMap::MapConstructor, "Map", FunctionLength::ZERO,
                              BUILTINS_STUB_ID(MapConstructor)));
    // Map().prototype = Map.Prototype & Map.prototype.constructor = Map()
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(mapFunction),
                                                     mapFuncInstanceHClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(mapFuncPrototype), constructorKey, mapFunction);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    // Map.prototype functions
    for (const base::BuiltinFunctionEntry &entry: BuiltinsMap::GetMapPrototypeFunctions()) {
        SetFunction(env, mapFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // @@ToStringTag
    SetStringTagSymbol(env, mapFuncPrototype, "Map");

    // 23.1.3.10get Map.prototype.size
    JSHandle<JSTaggedValue> sizeGetter = CreateGetter(env, BuiltinsMap::GetSize, "size", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> sizeKey(factory_->NewFromASCIIReadOnly("size"));
    SetGetter(mapFuncPrototype, sizeKey, sizeGetter);

    // 23.1.2.2get Map [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsMap::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(mapFunction), speciesSymbol, speciesGetter);

    // %MapPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> entries(factory_->NewFromASCIIReadOnly("entries"));
    JSHandle<JSTaggedValue> entriesFunc =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>::Cast(mapFuncPrototype), entries);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    PropertyDescriptor descriptor(thread_, entriesFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, mapFuncPrototype, iteratorSymbol, descriptor);

    env->SetBuiltinsMapFunction(thread_, mapFunction);
    env->SetMapPrototype(thread_, mapFuncPrototype);
    env->SetMapProtoEntriesFunction(thread_, entriesFunc);
    thread_->SetInitialBuiltinHClass(BuiltinTypeId::MAP,
        mapFunction->GetTaggedObject()->GetClass(),
        *mapFuncInstanceHClass,
        mapFuncPrototype->GetJSHClass());
}

void Builtins::LazyInitializeMap(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("Map"));
    auto accessor = factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::Map));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetBuiltinsMapFunction(thread_, accessor);
    env->SetMapPrototype(thread_, accessor);
    env->SetMapProtoEntriesFunction(thread_, accessor);
    env->SetMapClass(thread_, accessor);
}

void Builtins::InitializeWeakMap(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // WeakMap.prototype
    JSHandle<JSObject> weakMapFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> weakMapFuncPrototypeValue(weakMapFuncPrototype);
    // WeakMap.prototype_or_hclass
    JSHandle<JSHClass> weakMapFuncInstanceHClass =
        factory_->NewEcmaHClass(JSWeakMap::SIZE, JSType::JS_WEAK_MAP, weakMapFuncPrototypeValue);
    // WeakMap() = new Function()
    JSHandle<JSTaggedValue> weakMapFunction(NewBuiltinConstructor(
        env, weakMapFuncPrototype, BuiltinsWeakMap::WeakMapConstructor, "WeakMap", FunctionLength::ZERO));
    // WeakMap().prototype = WeakMap.Prototype & WeakMap.prototype.constructor = WeakMap()
    JSFunction::Cast(weakMapFunction->GetTaggedObject())
        ->SetProtoOrHClass(thread_, weakMapFuncInstanceHClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(weakMapFuncPrototype), constructorKey, weakMapFunction);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    // weakmap.prototype.set()
    SetFunction(env, weakMapFuncPrototype, globalConst->GetHandledSetString(), BuiltinsWeakMap::Set,
                FunctionLength::TWO);
    // weakmap.prototype.delete()
    SetFunction(env, weakMapFuncPrototype, "delete", BuiltinsWeakMap::Delete, FunctionLength::ONE);
    // weakmap.prototype.has()
    SetFunction(env, weakMapFuncPrototype, "has", BuiltinsWeakMap::Has, FunctionLength::ONE);
    // weakmap.prototype.get()
    SetFunction(env, weakMapFuncPrototype, thread_->GlobalConstants()->GetHandledGetString(), BuiltinsWeakMap::Get,
                FunctionLength::ONE);
    // @@ToStringTag
    SetStringTagSymbol(env, weakMapFuncPrototype, "WeakMap");

    env->SetBuiltinsWeakMapFunction(thread_, weakMapFunction);
}

void Builtins::LazyInitializeWeakMap(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("WeakMap"));
    auto accessor = factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::WeakMap));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetBuiltinsWeakMapFunction(thread_, accessor);
}

void Builtins::InitializeWeakSet(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // Set.prototype
    JSHandle<JSObject> weakSetFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> weakSetFuncPrototypeValue(weakSetFuncPrototype);
    // Set.prototype_or_hclass
    JSHandle<JSHClass> weakSetFuncInstanceHClass =
        factory_->NewEcmaHClass(JSWeakSet::SIZE, JSType::JS_WEAK_SET, weakSetFuncPrototypeValue);
    // Set() = new Function()
    JSHandle<JSTaggedValue> weakSetFunction(NewBuiltinConstructor(
        env, weakSetFuncPrototype, BuiltinsWeakSet::WeakSetConstructor, "WeakSet", FunctionLength::ZERO));
    JSHandle<JSFunction>(weakSetFunction)->SetProtoOrHClass(thread_, weakSetFuncInstanceHClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(weakSetFuncPrototype), constructorKey, weakSetFunction);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    // set.prototype.add()
    SetFunction(env, weakSetFuncPrototype, "add", BuiltinsWeakSet::Add, FunctionLength::ONE);
    // set.prototype.delete()
    SetFunction(env, weakSetFuncPrototype, "delete", BuiltinsWeakSet::Delete, FunctionLength::ONE);
    // set.prototype.has()
    SetFunction(env, weakSetFuncPrototype, "has", BuiltinsWeakSet::Has, FunctionLength::ONE);

    // @@ToStringTag
    SetStringTagSymbol(env, weakSetFuncPrototype, "WeakSet");

    env->SetBuiltinsWeakSetFunction(thread_, weakSetFunction);
}

void Builtins::LazyInitializeWeakSet(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("WeakSet"));
    auto accessor = factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::WeakSet));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetBuiltinsWeakSetFunction(thread_, accessor);
}

void Builtins::InitializeAtomics(const JSHandle<GlobalEnv> &env,
                                 const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> atomicsHClass = factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT,
                                                               objFuncPrototypeVal);
    JSHandle<JSObject> atomicsObject = factory_->NewJSObject(atomicsHClass);
    // Atomics functions
    for (const base::BuiltinFunctionEntry &entry: Atomics::GetAtomicsFunctions()) {
        SetFunction(env, atomicsObject, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    JSHandle<JSTaggedValue> atomicsString(factory_->NewFromASCIIReadOnly("Atomics"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor atomicsDesc(thread_, JSHandle<JSTaggedValue>::Cast(atomicsObject), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, atomicsString, atomicsDesc);
    // @@ToStringTag
    SetStringTagSymbol(env, atomicsObject, "Atomics");
    env->SetAtomicsFunction(thread_, atomicsObject);
}

void Builtins::InitializeWeakRef(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // WeakRef.prototype
    JSHandle<JSObject> weakRefFuncPrototype = factory_->NewJSObject(objFuncClass);
    JSHandle<JSTaggedValue> weakRefFuncPrototypeValue(weakRefFuncPrototype);
    // WeakRef.prototype_or_hclass
    JSHandle<JSHClass> weakRefFuncInstanceHClass =
        factory_->NewEcmaHClass(JSWeakRef::SIZE, JSType::JS_WEAK_REF, weakRefFuncPrototypeValue);
    // WeakRef() = new Function()
    JSHandle<JSTaggedValue> weakRefFunction(NewBuiltinConstructor(
        env, weakRefFuncPrototype, BuiltinsWeakRef::WeakRefConstructor, "WeakRef", FunctionLength::ONE));
    JSHandle<JSFunction>(weakRefFunction)->SetProtoOrHClass(thread_, weakRefFuncInstanceHClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(weakRefFuncPrototype), constructorKey, weakRefFunction);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    // WeakRef.prototype.deref()
    SetFunction(env, weakRefFuncPrototype, "deref", BuiltinsWeakRef::Deref, FunctionLength::ZERO);

    // @@ToStringTag
    SetStringTagSymbol(env, weakRefFuncPrototype, "WeakRef");

    env->SetBuiltinsWeakRefFunction(thread_, weakRefFunction);
}

void Builtins::LazyInitializeWeakRef(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("WeakRef"));
    auto accessor = factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::WeakRef));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetBuiltinsWeakRefFunction(thread_, accessor);
}

void Builtins::InitializeFinalizationRegistry(const JSHandle<GlobalEnv> &env,
                                              const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    // FinalizationRegistry.prototype
    JSHandle<JSObject> finalizationRegistryFuncPrototype = factory_->NewJSObject(objFuncClass);
    JSHandle<JSTaggedValue> finalizationRegistryFuncPrototypeValue(finalizationRegistryFuncPrototype);
    // FinalizationRegistry.prototype_or_hclass
    JSHandle<JSHClass> finalizationRegistryFuncInstanceHClass =
        factory_->NewEcmaHClass(JSFinalizationRegistry::SIZE, JSType::JS_FINALIZATION_REGISTRY,
                                finalizationRegistryFuncPrototypeValue);
    // FinalizationRegistry() = new Function()
    JSHandle<JSTaggedValue> finalizationRegistryFunction(NewBuiltinConstructor(
        env, finalizationRegistryFuncPrototype, BuiltinsFinalizationRegistry::FinalizationRegistryConstructor,
        "FinalizationRegistry", FunctionLength::ONE));
    JSHandle<JSFunction>(finalizationRegistryFunction)->SetProtoOrHClass(
        thread_, finalizationRegistryFuncInstanceHClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread_, JSHandle<JSTaggedValue>(finalizationRegistryFuncPrototype),
                          constructorKey, finalizationRegistryFunction);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    // FinalizationRegistry.prototype.deref()
    SetFunction(env, finalizationRegistryFuncPrototype, "register",
                BuiltinsFinalizationRegistry::Register, FunctionLength::TWO);
    SetFunction(env, finalizationRegistryFuncPrototype, "unregister",
                BuiltinsFinalizationRegistry::Unregister, FunctionLength::ONE);
    // @@ToStringTag
    SetStringTagSymbol(env, finalizationRegistryFuncPrototype, "FinalizationRegistry");

    env->SetBuiltinsFinalizationRegistryFunction(thread_, finalizationRegistryFunction);
}

void Builtins::LazyInitializeFinalizationRegistry(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("FinalizationRegistry"));
    auto accessor = factory_->NewInternalAccessor(nullptr,
        reinterpret_cast<void *>(BuiltinsLazyCallback::FinalizationRegistry));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetBuiltinsFinalizationRegistryFunction(thread_, accessor);
}

void Builtins::InitializeMath(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> mathClass = factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> mathObject = factory_->NewJSObjectWithInit(mathClass);
    RandomGenerator::InitRandom(thread_);

    for (const base::BuiltinFunctionEntry &entry: Math::GetMathFunctions()) {
        SetFunction(env, mathObject, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    for (const base::BuiltinConstantEntry &entry: Math::GetMathConstants()) {
        SetConstant(mathObject, entry.GetName(), entry.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> mathString(factory_->NewFromASCIIReadOnly("Math"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor mathDesc(thread_, JSHandle<JSTaggedValue>::Cast(mathObject), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, mathString, mathDesc);
    // @@ToStringTag
    SetStringTagSymbol(env, mathObject, "Math");
    env->SetMathFunction(thread_, mathObject);
    env->SetMathFunctionClass(thread_, JSHandle<JSHClass>(thread_, mathObject->GetJSHClass()));
}

void Builtins::InitializeJson(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> jsonHClass = factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> jsonObject = factory_->NewJSObjectWithInit(jsonHClass);

    SetFunction(env, jsonObject, "parse", Json::Parse, FunctionLength::TWO);
    SetFunction(env, jsonObject, "parseSendable", SendableJson::Parse, FunctionLength::THREE);
    SetFunction(env, jsonObject, "parseBigInt", BigIntJson::Parse, FunctionLength::THREE);
    SetFunction(env, jsonObject, "stringify", Json::Stringify, FunctionLength::THREE, BUILTINS_STUB_ID(JsonStringify));
    SetFunction(env, jsonObject, "stringifySendable", SendableJson::Stringify, FunctionLength::THREE);
    SetFunction(env, jsonObject, "stringifyBigInt", BigIntJson::Stringify, FunctionLength::THREE);

    PropertyDescriptor jsonDesc(thread_, JSHandle<JSTaggedValue>::Cast(jsonObject), true, false, true);
    JSHandle<JSTaggedValue> jsonString(factory_->NewFromASCIIReadOnly("JSON"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSObject::DefineOwnProperty(thread_, globalObject, jsonString, jsonDesc);
    // @@ToStringTag
    SetStringTagSymbol(env, jsonObject, "JSON");
    env->SetJsonFunction(thread_, jsonObject);
}

void Builtins::InitializeString(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // String.prototype
    JSHandle<JSTaggedValue> toObject(factory_->GetEmptyString());
    JSHandle<JSHClass> primRefObjHClass =
        factory_->NewEcmaHClass(JSPrimitiveRef::SIZE, BuiltinsString::GetNumPrototypeInlinedProperties(),
                                JSType::JS_PRIMITIVE_REF, objFuncPrototypeVal);
    JSHandle<JSObject> stringFuncPrototype =
        JSHandle<JSObject>::Cast(factory_->NewJSPrimitiveRef(primRefObjHClass, toObject));
    JSHandle<JSTaggedValue> stringFuncPrototypeValue(stringFuncPrototype);

    // String.prototype_or_hclass
    JSHandle<JSHClass> stringFuncInstanceHClass =
        factory_->NewEcmaHClass(JSPrimitiveRef::SIZE, JSType::JS_PRIMITIVE_REF, stringFuncPrototypeValue);

    // String = new Function()
    JSHandle<JSObject> stringFunction(NewBuiltinConstructor(env, stringFuncPrototype, BuiltinsString::StringConstructor,
                                                            "String", FunctionLength::ONE));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(stringFunction),
                                                     stringFuncInstanceHClass.GetTaggedValue());

    // String.prototype method
    for (const base::BuiltinFunctionEntry &entry: BuiltinsString::GetStringPrototypeFunctions()) {
        SetFunction(env, stringFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    JSHandle<JSTaggedValue> stringIter = SetAndReturnFunctionAtSymbol(env, stringFuncPrototype,
        env->GetIteratorSymbol(), "[Symbol.iterator]", BuiltinsString::GetStringIterator, FunctionLength::ZERO,
        BUILTINS_STUB_ID(StringGetStringIterator));

    // String method
    for (const base::BuiltinFunctionEntry &entry: BuiltinsString::GetStringFunctions()) {
        SetFunction(env, stringFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // String.prototype.length
    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env, BuiltinsString::GetLength, "length", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory_->NewFromASCIIReadOnly("length"));
    SetGetter(stringFuncPrototype, lengthKey, lengthGetter);

    env->SetStringFunction(thread_, stringFunction);
    env->SetStringPrototype(thread_, stringFuncPrototype);
    env->SetStringProtoIterFunction(thread_, stringIter);
    thread_->SetInitialBuiltinHClass(BuiltinTypeId::STRING, nullptr, nullptr, stringFuncPrototype->GetJSHClass());
}

void Builtins::InitializeStringIterator(const JSHandle<GlobalEnv> &env,
                                        const JSHandle<JSHClass> &iteratorFuncClass) const
{
    // StringIterator.prototype
    JSHandle<JSObject> strIterPrototype(factory_->NewJSObjectWithInit(iteratorFuncClass));

    // StringIterator.prototype_or_hclass
    JSHandle<JSHClass> strIterFuncInstanceHClass = factory_->NewEcmaHClass(
        JSStringIterator::SIZE, JSType::JS_STRING_ITERATOR, JSHandle<JSTaggedValue>(strIterPrototype));

    JSHandle<JSFunction> strIterFunction(
        factory_->NewJSFunction(env, static_cast<void *>(nullptr), FunctionKind::BASE_CONSTRUCTOR));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, strIterFunction,
                                                     strIterFuncInstanceHClass.GetTaggedValue());

    SetFunction(env, strIterPrototype, "next", StringIterator::Next, FunctionLength::ZERO,
                BUILTINS_STUB_ID(StringIteratorProtoNext));
    SetStringTagSymbol(env, strIterPrototype, "String Iterator");

    env->SetStringIterator(thread_, strIterFunction);
    env->SetStringIteratorClass(thread_, strIterFuncInstanceHClass);
    env->SetStringIteratorPrototype(thread_, strIterPrototype);
}

void Builtins::InitializeAsyncFromSyncIterator(const JSHandle<GlobalEnv> &env,
                                               const JSHandle<JSHClass> &iteratorFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);

    JSHandle<JSObject> asyncItPrototype = factory_->NewJSObjectWithInit(iteratorFuncClass);
    SetFunction(env, asyncItPrototype, "next", BuiltinsAsyncFromSyncIterator::Next, FunctionLength::ONE);
    SetFunction(env, asyncItPrototype, "return", BuiltinsAsyncFromSyncIterator::Return, FunctionLength::ONE);
    SetFunction(env, asyncItPrototype, "throw", BuiltinsAsyncFromSyncIterator::Throw, FunctionLength::ONE);
    JSHandle<JSHClass> hclass = factory_->NewEcmaHClass(JSAsyncFromSyncIterator::SIZE,
                                                        JSType::JS_ASYNC_FROM_SYNC_ITERATOR,
                                                        JSHandle<JSTaggedValue>(asyncItPrototype));
    JSHandle<JSFunction> iterFunction(
        factory_->NewJSFunction(env, static_cast<void *>(nullptr), FunctionKind::BASE_CONSTRUCTOR));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, iterFunction, hclass.GetTaggedValue());
    env->SetAsyncFromSyncIterator(thread_, iterFunction);
    env->SetAsyncFromSyncIteratorPrototype(thread_, asyncItPrototype);

    JSHandle<JSHClass> asyncFromSyncIterUnwarpClass =
        factory_->NewEcmaHClass(JSAsyncFromSyncIterUnwarpFunction::SIZE,
                                JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION,
                                env->GetFunctionPrototype());
    asyncFromSyncIterUnwarpClass->SetCallable(true);
    asyncFromSyncIterUnwarpClass->SetExtensible(true);
    env->SetAsyncFromSyncIterUnwarpClass(thread_, asyncFromSyncIterUnwarpClass);
}

void Builtins::InitializeIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Iterator.prototype
    JSHandle<JSObject> iteratorPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    // Iterator.prototype.next()
    SetFunction(env, iteratorPrototype, "next", BuiltinsIterator::Next, FunctionLength::ONE);
    // Iterator.prototype.return()
    SetFunction(env, iteratorPrototype, "return", BuiltinsIterator::Return, FunctionLength::ONE,
        BUILTINS_STUB_ID(IteratorProtoReturn));
    // Iterator.prototype.throw()
    SetFunction(env, iteratorPrototype, "throw", BuiltinsIterator::Throw, FunctionLength::ONE);
    // %IteratorPrototype% [ @@iterator ]
    SetFunctionAtSymbol(env, iteratorPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        BuiltinsIterator::GetIteratorObj, FunctionLength::ZERO);
    env->SetIteratorPrototype(thread_, iteratorPrototype);

    // Iterator.hclass
    JSHandle<JSHClass> iteratorFuncClass =
        factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_ITERATOR, JSHandle<JSTaggedValue>(iteratorPrototype));

    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    globalConst->SetConstant(ConstantIndex::JS_API_ITERATOR_FUNC_CLASS_INDEX, iteratorFuncClass);

    // Iterator result hclass
    JSHandle<JSHClass> iterResultHClass = factory_->CreateIteratorResultInstanceClass(env);
    globalConst->SetConstant(ConstantIndex::ITERATOR_RESULT_CLASS, iterResultHClass);

    // use for CloseIterator
    JSHandle<CompletionRecord> record =
        factory_->NewCompletionRecord(CompletionRecordType::NORMAL, globalConst->GetHandledUndefined());
    globalConst->SetConstant(ConstantIndex::UNDEFINED_COMPLRTION_RECORD_INDEX, record);

    thread_->SetInitialBuiltinHClass(BuiltinTypeId::ITERATOR, nullptr,
        *iteratorFuncClass, iteratorPrototype->GetJSHClass());

    // iteratorPrototype hclass
    JSHandle<JSHClass> iteratorPrototypeHClass(thread_, iteratorPrototype->GetJSHClass());

    InitializeForinIterator(env, iteratorFuncClass);
    InitializeSetIterator(env, iteratorFuncClass);
    InitializeSSetIterator(env, iteratorFuncClass);
    InitializeMapIterator(env, iteratorFuncClass);
    InitializeSMapIterator(env, iteratorFuncClass);
    InitializeArrayIterator(env, iteratorFuncClass, iteratorPrototypeHClass);
    InitializeSArrayIterator(env, iteratorFuncClass);
    InitializeStringIterator(env, iteratorFuncClass);
    InitializeRegexpIterator(env, iteratorFuncClass);
#ifdef ARK_SUPPORT_INTL
    InitializeSegmentIterator(env, iteratorFuncClass);
#endif
}

void Builtins::InitializeAsyncIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncDynclass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // AsyncIterator.prototype
    JSHandle<JSObject> asyncIteratorPrototype = factory_->NewJSObjectWithInit(objFuncDynclass);
    // AsyncIterator.prototype.next()
    SetFunction(env, asyncIteratorPrototype, "next", BuiltinsAsyncIterator::Next, FunctionLength::ONE);
    // AsyncIterator.prototype.return()
    SetFunction(env, asyncIteratorPrototype, "return", BuiltinsAsyncIterator::Return, FunctionLength::ONE);
    // AsyncIterator.prototype.throw()
    SetFunction(env, asyncIteratorPrototype, "throw", BuiltinsAsyncIterator::Throw, FunctionLength::ONE);
    // %AsyncIteratorPrototype% [ @@AsyncIterator ]
    SetFunctionAtSymbol(env, asyncIteratorPrototype, env->GetAsyncIteratorSymbol(), "[Symbol.asyncIterator]",
                        BuiltinsAsyncIterator::GetAsyncIteratorObj, FunctionLength::ZERO);
    env->SetAsyncIteratorPrototype(thread_, asyncIteratorPrototype);

    // AsyncIterator.dynclass
    JSHandle<JSHClass> asyncIteratorFuncDynclass =
        factory_->NewEcmaHClass(JSObject::SIZE,
                                JSType::JS_ASYNCITERATOR, JSHandle<JSTaggedValue>(asyncIteratorPrototype));

    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    globalConst->SetConstant(ConstantIndex::JS_API_ASYNCITERATOR_FUNC_CLASS_INDEX, asyncIteratorFuncDynclass);
}

void Builtins::InitializeForinIterator(const JSHandle<GlobalEnv> &env,
                                       const JSHandle<JSHClass> &iteratorFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Iterator.prototype
    JSHandle<JSObject> forinIteratorPrototype = factory_->NewJSObjectWithInit(iteratorFuncClass);
    JSHandle<JSHClass> hclass = factory_->NewEcmaHClass(JSForInIterator::SIZE, JSType::JS_FORIN_ITERATOR,
                                                        JSHandle<JSTaggedValue>(forinIteratorPrototype));

    // Iterator.prototype.next()
    SetFunction(env, forinIteratorPrototype, "next", JSForInIterator::Next, FunctionLength::ONE);
    env->SetForinIteratorPrototype(thread_, forinIteratorPrototype);
    env->SetForinIteratorClass(thread_, hclass);
}

void Builtins::InitializeSetIterator(const JSHandle<GlobalEnv> &env,
                                     const JSHandle<JSHClass> &iteratorFuncClass) const
{
    // SetIterator.prototype
    JSHandle<JSObject> setIteratorPrototype(factory_->NewJSObjectWithInit(iteratorFuncClass));
    // Iterator.prototype.next()
    SetFunction(env, setIteratorPrototype, "next", JSSetIterator::Next, FunctionLength::ZERO,
                BUILTINS_STUB_ID(SetIteratorProtoNext));
    SetStringTagSymbol(env, setIteratorPrototype, "Set Iterator");
    env->SetSetIteratorPrototype(thread_, setIteratorPrototype);
    JSHandle<JSTaggedValue> protoValue = env->GetSetIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSSetIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    hclassHandle->SetExtensible(true);
}

void Builtins::InitializeSSetIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const
{
    // SetIterator.prototype
    JSHandle<JSObject> setIteratorPrototype(factory_->NewJSObjectWithInit(iteratorFuncClass));
    // Iterator.prototype.next()
    SetFunction(env, setIteratorPrototype, "next", JSSharedSetIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, setIteratorPrototype, "SharedSet Iterator");
    env->SetSharedSetIteratorPrototype(thread_, setIteratorPrototype);
    JSHandle<JSTaggedValue> protoValue = env->GetSharedSetIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSSharedSetIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    hclassHandle->SetExtensible(true);
}

void Builtins::InitializeMapIterator(const JSHandle<GlobalEnv> &env,
                                     const JSHandle<JSHClass> &iteratorFuncClass) const
{
    // MapIterator.prototype
    JSHandle<JSObject> mapIteratorPrototype(factory_->NewJSObjectWithInit(iteratorFuncClass));
    // Iterator.prototype.next()
    SetFunction(env, mapIteratorPrototype, "next", JSMapIterator::Next, FunctionLength::ZERO,
                BUILTINS_STUB_ID(MapIteratorProtoNext));
    SetStringTagSymbol(env, mapIteratorPrototype, "Map Iterator");
    env->SetMapIteratorPrototype(thread_, mapIteratorPrototype);
    JSHandle<JSTaggedValue> protoValue = env->GetMapIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSMapIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    hclassHandle->SetExtensible(true);
}

void Builtins::InitializeSMapIterator(const JSHandle<GlobalEnv> &env,
                                      const JSHandle<JSHClass> &iteratorFuncClass) const
{
    // MapIterator.prototype
    JSHandle<JSObject> mapIteratorPrototype(factory_->NewJSObjectWithInit(iteratorFuncClass));
    // Iterator.prototype.next()
    SetFunction(env, mapIteratorPrototype, "next", JSSharedMapIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, mapIteratorPrototype, "SharedMap Iterator");
    env->SetSharedMapIteratorPrototype(thread_, mapIteratorPrototype);
    JSHandle<JSTaggedValue> protoValue = env->GetSharedMapIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSSharedMapIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    hclassHandle->SetExtensible(true);
}

void Builtins::InitializeArrayIterator(const JSHandle<GlobalEnv> &env,
                                       const JSHandle<JSHClass> &iteratorFuncClass,
                                       const JSHandle<JSHClass> &iteratorPrototypeClass) const
{
    // ArrayIterator.prototype
    JSHandle<JSObject> arrayIteratorPrototype(factory_->NewJSObjectWithInit(iteratorFuncClass));
    JSHandle<JSTaggedValue> arrayIteratorPrototypeValue(arrayIteratorPrototype);
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    JSHandle<JSHClass> arrayIteratorInstanceHClass(globalConst->GetHandledJSArrayIteratorClass());
    // Iterator.prototype.next()
    SetFunction(env, arrayIteratorPrototype, "next", JSArrayIterator::Next, FunctionLength::ZERO,
                BUILTINS_STUB_ID(ArrayIteratorProtoNext));
    arrayIteratorInstanceHClass->SetPrototype(thread_, arrayIteratorPrototypeValue);
    SetStringTagSymbol(env, arrayIteratorPrototype, "Array Iterator");
    thread_->SetInitialBuiltinHClass(BuiltinTypeId::ARRAY_ITERATOR, nullptr,
        *arrayIteratorInstanceHClass, arrayIteratorPrototype->GetJSHClass(), *iteratorPrototypeClass);
    env->SetArrayIteratorPrototype(thread_, arrayIteratorPrototype);
}

void Builtins::InitializeSArrayIterator(const JSHandle<GlobalEnv> &env,
                                        const JSHandle<JSHClass> &iteratorFuncClass) const
{
    // ArrayIterator.prototype
    JSHandle<JSObject> arrayIteratorPrototype(factory_->NewJSObjectWithInit(iteratorFuncClass));
    // Iterator.prototype.next()
    SetFunction(env, arrayIteratorPrototype, "next", JSSharedArrayIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, arrayIteratorPrototype, "SharedArray Iterator");
    env->SetSharedArrayIteratorPrototype(thread_, arrayIteratorPrototype);
}

void Builtins::InitializeRegexpIterator(const JSHandle<GlobalEnv> &env,
                                        const JSHandle<JSHClass> &iteratorFuncClass) const
{
    // RegExpIterator.prototype
    JSHandle<JSObject> regExpIteratorPrototype(factory_->NewJSObject(iteratorFuncClass));
    // Iterator.prototype.next()
    SetFunction(env, regExpIteratorPrototype, "next", JSRegExpIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, regExpIteratorPrototype, "RegExp String Iterator");
    env->SetRegExpIteratorPrototype(thread_, regExpIteratorPrototype);
}

void Builtins::InitializeRegExp(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // RegExp.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> regPrototype = factory_->NewJSObjectByConstructor(env, objFun, REGEXP_INLINE_PROPS);
    JSHandle<JSTaggedValue> regPrototypeValue(regPrototype);

    // RegExp.prototype_or_hclass
    JSHandle<JSHClass> regexpFuncInstanceHClass = factory_->CreateJSRegExpInstanceClass(regPrototypeValue);

    // RegExp = new Function()
    JSHandle<JSObject> regexpFunction(
        NewBuiltinConstructor(env, regPrototype, RegExp::RegExpConstructor, "RegExp", FunctionLength::TWO));

    // initialize RegExp.$1 .. $9 static and read-only attributes
    InitializeGlobalRegExp(regexpFunction);

    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(regexpFunction),
                                                     regexpFuncInstanceHClass.GetTaggedValue());

    const GlobalEnvConstants *globalConstants = thread_->GlobalConstants();
    // RegExp.prototype method
    JSHandle<JSFunction> execFunc = SetAndReturnFunction(env, regPrototype, "exec", RegExp::Exec, FunctionLength::ONE);
    SetFunction(env, regPrototype, "test", RegExp::Test, FunctionLength::ONE);
    SetFunction(env, regPrototype, globalConstants->GetHandledToStringString(), RegExp::ToString,
                FunctionLength::ZERO);
    JSHandle<JSFunction>(execFunc)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> flagsGetter = CreateGetter(env, RegExp::GetFlags, "flags", FunctionLength::ZERO,
        kungfu::BuiltinsStubCSigns::RegExpGetFlags);
    JSHandle<JSTaggedValue> flagsKey(globalConstants->GetHandledFlagsString());
    SetGetter(regPrototype, flagsKey, flagsGetter);
    JSHandle<JSFunction>(flagsGetter)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> sourceGetter = CreateGetter(env, RegExp::GetSource, "source", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> sourceKey(globalConstants->GetHandledSourceString());
    SetGetter(regPrototype, sourceKey, sourceGetter);
    JSHandle<JSFunction>(sourceGetter)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> globalGetter = CreateGetter(env, RegExp::GetGlobal, "global", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> globalKey(globalConstants->GetHandledGlobalString());
    SetGetter(regPrototype, globalKey, globalGetter);
    JSHandle<JSFunction>(globalGetter)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> hasIndicesGetter =
        CreateGetter(env, RegExp::GetHasIndices, "hasIndices", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> hasIndicesKey(factory_->NewFromASCIIReadOnly("hasIndices"));
    SetGetter(regPrototype, hasIndicesKey, hasIndicesGetter);
    JSHandle<JSFunction>(hasIndicesGetter)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> ignoreCaseGetter =
        CreateGetter(env, RegExp::GetIgnoreCase, "ignoreCase", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> ignoreCaseKey(factory_->NewFromASCIIReadOnly("ignoreCase"));
    SetGetter(regPrototype, ignoreCaseKey, ignoreCaseGetter);
    JSHandle<JSFunction>(ignoreCaseGetter)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> multilineGetter =
        CreateGetter(env, RegExp::GetMultiline, "multiline", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> multilineKey(factory_->NewFromASCIIReadOnly("multiline"));
    SetGetter(regPrototype, multilineKey, multilineGetter);
    JSHandle<JSFunction>(multilineGetter)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> dotAllGetter = CreateGetter(env, RegExp::GetDotAll, "dotAll", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> dotAllKey(factory_->NewFromASCIIReadOnly("dotAll"));
    SetGetter(regPrototype, dotAllKey, dotAllGetter);
    JSHandle<JSFunction>(dotAllGetter)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> stickyGetter = CreateGetter(env, RegExp::GetSticky, "sticky", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> stickyKey(globalConstants->GetHandledStickyString());
    SetGetter(regPrototype, stickyKey, stickyGetter);
    JSHandle<JSFunction>(stickyGetter)->SetLexicalEnv(thread_, env);

    JSHandle<JSTaggedValue> unicodeGetter = CreateGetter(env, RegExp::GetUnicode, "unicode", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> unicodeKey(globalConstants->GetHandledUnicodeString());
    SetGetter(regPrototype, unicodeKey, unicodeGetter);
    JSHandle<JSFunction>(unicodeGetter)->SetLexicalEnv(thread_, env);

    // Set RegExp [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsMap::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(regexpFunction), speciesSymbol, speciesGetter);
    JSHandle<JSFunction>(speciesGetter)->SetLexicalEnv(thread_, env);

    // Set RegExp.prototype[@@split]
    JSHandle<JSTaggedValue> splitFunc = SetAndReturnFunctionAtSymbol(
        env, regPrototype, env->GetSplitSymbol(), "[Symbol.split]", RegExp::Split, FunctionLength::TWO);
    // Set RegExp.prototype[@@search]
    JSHandle<JSTaggedValue> searchFunc = SetAndReturnFunctionAtSymbol(
        env, regPrototype, env->GetSearchSymbol(), "[Symbol.search]", RegExp::Search, FunctionLength::ONE);
    // Set RegExp.prototype[@@match]
    JSHandle<JSTaggedValue> matchFunc = SetAndReturnFunctionAtSymbol(
        env, regPrototype, env->GetMatchSymbol(), "[Symbol.match]", RegExp::Match, FunctionLength::ONE);
    // Set RegExp.prototype[@@matchAll]
    JSHandle<JSTaggedValue> matchAllFunc = SetAndReturnFunctionAtSymbol(
        env, regPrototype, env->GetMatchAllSymbol(), "[Symbol.matchAll]", RegExp::MatchAll, FunctionLength::ONE);
    // Set RegExp.prototype[@@replace]
    JSHandle<JSTaggedValue> replaceFunc = SetAndReturnFunctionAtSymbol(
        env, regPrototype, env->GetReplaceSymbol(), "[Symbol.replace]", RegExp::Replace, FunctionLength::TWO);

    env->SetRegExpFunction(thread_, regexpFunction);
    env->SetRegExpPrototype(thread_, regPrototype);
    env->SetRegExpExecFunction(thread_, execFunc);
    env->SetRegExpSplitFunction(thread_, splitFunc);
    env->SetRegExpSearchFunction(thread_, searchFunc);
    env->SetRegExpMatchFunction(thread_, matchFunc);
    env->SetRegExpMatchAllFunction(thread_, matchAllFunc);
    env->SetRegExpReplaceFunction(thread_, replaceFunc);
    // Set RegExp.prototype hclass
    JSHandle<JSHClass> regPrototypeClass(thread_, regPrototype->GetJSHClass());
    env->SetRegExpPrototypeClass(thread_, regPrototypeClass.GetTaggedValue());

    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    globalConst->SetConstant(ConstantIndex::JS_REGEXP_CLASS_INDEX, regexpFuncInstanceHClass.GetTaggedValue());
}

void Builtins::InitializeArray(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal,
                               bool isRealm) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Arraybase.prototype
    JSHandle<JSHClass> arrBaseFuncInstanceHClass = JSArray::CreateJSArrayPrototypeClass(thread_, factory_,
        objFuncPrototypeVal, BuiltinsArray::GetNumPrototypeInlinedProperties());
    // Since we don't want the operations on the array prototype to go through the IR code.
    // Specially we set the bit of the array prototype to true.
    arrBaseFuncInstanceHClass->SetIsJSArrayPrototypeModified(true);
    // Array.prototype
    JSHandle<JSObject> arrFuncPrototype = factory_->NewJSObjectWithInit(arrBaseFuncInstanceHClass);
    JSHandle<JSArray>::Cast(arrFuncPrototype)->SetLength(FunctionLength::ZERO);
    auto accessor = thread_->GlobalConstants()->GetArrayLengthAccessor();
    JSArray::Cast(*arrFuncPrototype)->SetPropertyInlinedProps(thread_, JSArray::LENGTH_INLINE_PROPERTY_INDEX, accessor);
    JSHandle<JSTaggedValue> arrFuncPrototypeValue(arrFuncPrototype);

    //  Array.prototype_or_hclass
    JSMutableHandle<JSHClass> arrFuncInstanceHClass(thread_, JSTaggedValue::Undefined());
    arrFuncInstanceHClass.Update(factory_->CreateJSArrayInstanceClass(arrFuncPrototypeValue));
    auto globalConstant = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    if (!isRealm) {
        globalConstant->InitElementKindHClass(thread_, arrFuncInstanceHClass);
    }
    if (thread_->GetEcmaVM()->IsEnableElementsKind()) {
        // for all JSArray, the initial ElementsKind should be NONE
        // For PGO, currently we do not support elementsKind for builtins
        #if ECMASCRIPT_ENABLE_ELEMENTSKIND_ALWAY_GENERIC
        auto index = static_cast<size_t>(ConstantIndex::ELEMENT_HOLE_TAGGED_HCLASS_INDEX);
        #else
        auto index = static_cast<size_t>(ConstantIndex::ELEMENT_NONE_HCLASS_INDEX);
        #endif
        auto hclassVal = globalConstant->GetGlobalConstantObject(index);
        arrFuncInstanceHClass.Update(hclassVal);
    }

    JSHandle<JSHClass> arrayFunctionHClass = JSArray::CreateJSArrayFunctionClass(thread_, factory_, env);
    // Array = new Function()
    JSHandle<JSObject> arrayFunction(
        NewBuiltinConstructor(env, arrFuncPrototype, BuiltinsArray::ArrayConstructor, "Array", FunctionLength::ONE,
                              BUILTINS_STUB_ID(ArrayConstructor), arrayFunctionHClass));
    JSHandle<JSFunction> arrayFuncFunction(arrayFunction);

    // Set the [[Realm]] internal slot of F to the running execution context's Realm
    JSHandle<LexicalEnv> lexicalEnv = factory_->NewLexicalEnv(0);
    lexicalEnv->SetParentEnv(thread_, env.GetTaggedValue());
    arrayFuncFunction->SetLexicalEnv(thread_, lexicalEnv.GetTaggedValue());

    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, arrayFuncFunction,
                                                     arrFuncInstanceHClass.GetTaggedValue());

    // Array.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: BuiltinsArray::GetArrayPrototypeFunctions()) {
        SetFunction(env, arrFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // %ArrayPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> values(factory_->NewFromASCIIReadOnly("values"));
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> valuesFunc =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>::Cast(arrFuncPrototype), values);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    PropertyDescriptor iteartorDesc(thread_, valuesFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, arrFuncPrototype, iteratorSymbol, iteartorDesc);

    // Array methods (excluding '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: BuiltinsArray::GetArrayFunctions()) {
        SetFunction(env, arrayFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // 22.1.2.5 get %Array% [ @@species ]
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsArray::Species, "[Symbol.species]", FunctionLength::ZERO);
    JSHandle<AccessorData> speciesAccessor = factory_->NewAccessorData();
    speciesAccessor->SetGetter(thread_, speciesGetter);
    arrayFunction->SetPropertyInlinedProps(thread_, JSArray::ARRAY_FUNCTION_SPECIES_INDEX,
                                           speciesAccessor.GetTaggedValue());
    globalConstant->SetConstant(ConstantIndex::ARRAY_SPECIES_ACCESSOR, speciesAccessor.GetTaggedValue());
    constexpr int arrProtoLen = 0;
    JSHandle<JSTaggedValue> keyString = thread_->GlobalConstants()->GetHandledLengthString();
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(thread_, JSTaggedValue(arrProtoLen)), true, false,
                                  false);
    JSObject::DefineOwnProperty(thread_, arrFuncPrototype, keyString, descriptor);

    JSHandle<JSTaggedValue> valuesKey(factory_->NewFromASCIIReadOnly("values"));
    PropertyDescriptor desc(thread_);
    JSObject::GetOwnProperty(thread_, arrFuncPrototype, valuesKey, desc);

    // Array.prototype [ @@unscopables ]
    JSHandle<JSTaggedValue> unscopablesSymbol = env->GetUnscopablesSymbol();
    JSHandle<JSTaggedValue> unscopables = CreateArrayUnscopables(thread_);
    PropertyDescriptor unscopablesDesc(thread_, unscopables, false, false, true);
    JSObject::DefineOwnProperty(thread_, arrFuncPrototype, unscopablesSymbol, unscopablesDesc);

    env->SetArrayProtoValuesFunction(thread_, desc.GetValue());
    env->SetArrayFunction(thread_, arrayFunction);
    env->SetArrayPrototype(thread_, arrFuncPrototype);

    if (!isRealm) {
        thread_->SetInitialBuiltinHClass(BuiltinTypeId::ARRAY, arrayFunction->GetJSHClass(),
                                         *arrFuncInstanceHClass, arrFuncPrototype->GetJSHClass());
    }
}

void Builtins::InitializeTypedArray(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // TypedArray.prototype
    JSHandle<JSHClass> typedArrFuncPrototypeHClass = factory_->NewEcmaHClass(
        JSObject::SIZE, BuiltinsTypedArray::GetNumPrototypeInlinedProperties(),
        JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> typedArrFuncPrototype = factory_->NewJSObjectWithInit(typedArrFuncPrototypeHClass);
    JSHandle<JSTaggedValue> typedArrFuncPrototypeValue(typedArrFuncPrototype);

    // TypedArray.prototype_or_hclass
    JSHandle<JSHClass> typedArrFuncInstanceHClass = factory_->NewEcmaHClass(
        JSTypedArray::SIZE, JSType::JS_TYPED_ARRAY, typedArrFuncPrototypeValue);

    // TypedArray = new Function()
    JSHandle<JSObject> typedArrayFunction(NewBuiltinConstructor(
        env, typedArrFuncPrototype, BuiltinsTypedArray::TypedArrayBaseConstructor, "TypedArray", FunctionLength::ZERO));

    JSHandle<JSFunction>(typedArrayFunction)
        ->SetProtoOrHClass(thread_, typedArrFuncInstanceHClass.GetTaggedValue());

    // TypedArray.prototype method
    for (const base::BuiltinFunctionEntry &entry: BuiltinsTypedArray::GetTypedArrayPrototypeFunctions()) {
        SetFunction(env, typedArrFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // TypedArray.prototype get accessor
    for (const base::BuiltinFunctionEntry &entry: BuiltinsTypedArray::GetTypedArrayPrototypeAccessors()) {
        JSHandle<JSTaggedValue> getter =
            CreateGetter(env, entry.GetEntrypoint(), entry.GetName(), entry.GetLength());
        JSHandle<JSTaggedValue> key(factory_->NewFromASCIIReadOnly(entry.GetName()));
        SetGetter(typedArrFuncPrototype, key, getter);
    }

    // %TypedArray%.prototype.toString(), which is strictly equal to Array.prototype.toString
    JSHandle<JSTaggedValue> arrFuncPrototype = env->GetArrayPrototype();
    JSHandle<JSTaggedValue> toStringFunc =
        JSObject::GetMethod(thread_, arrFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString());
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    PropertyDescriptor toStringDesc(thread_, toStringFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, typedArrFuncPrototype, thread_->GlobalConstants()->GetHandledToStringString(),
                                toStringDesc);

    // %TypedArray%.prototype [ @@iterator ] ( )
    JSHandle<JSTaggedValue> values(factory_->NewFromASCIIReadOnly("values"));
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> valuesFunc =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>::Cast(typedArrFuncPrototype), values);
    RETURN_IF_ABRUPT_COMPLETION(thread_);
    PropertyDescriptor iteartorDesc(thread_, valuesFunc, true, false, true);
    JSObject::DefineOwnProperty(thread_, typedArrFuncPrototype, iteratorSymbol, iteartorDesc);

    // 22.2.3.31 get %TypedArray%.prototype [ @@toStringTag ]
    JSHandle<JSTaggedValue> toStringTagSymbol = env->GetToStringTagSymbol();
    JSHandle<JSTaggedValue> toStringTagGetter =
        CreateGetter(env, BuiltinsTypedArray::ToStringTag, "[Symbol.toStringTag]", FunctionLength::ZERO);
    SetGetter(typedArrFuncPrototype, toStringTagSymbol, toStringTagGetter);

    // TypedArray method
    for (const base::BuiltinFunctionEntry &entry: BuiltinsTypedArray::GetTypedArrayFunctions()) {
        SetFunction(env, typedArrayFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // 22.2.2.4 get %TypedArray% [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, BuiltinsTypedArray::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(typedArrayFunction), speciesSymbol, speciesGetter);

    env->SetTypedArrayFunction(thread_, typedArrayFunction.GetTaggedValue());
    env->SetTypedArrayPrototype(thread_, typedArrFuncPrototype);
    env->SetTypedArrayProtoValuesFunction(thread_, valuesFunc);
    thread_->SetInitialBuiltinHClass(BuiltinTypeId::TYPED_ARRAY,
        typedArrayFunction->GetJSHClass(),
        *typedArrFuncInstanceHClass,
        typedArrFuncPrototype->GetJSHClass());

    JSHandle<JSHClass> specificTypedArrayFuncClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, JSType::JS_FUNCTION, env->GetTypedArrayFunction());
    specificTypedArrayFuncClass->SetConstructor(true);
    env->SetSpecificTypedArrayFunctionClass(thread_, specificTypedArrayFuncClass);

#define BUILTIN_TYPED_ARRAY_CALL_INITIALIZE(Type, TYPE, bytesPerElement) \
    Initialize##Type(env, typedArrFuncInstanceHClass);
    BUILTIN_TYPED_ARRAY_TYPES(BUILTIN_TYPED_ARRAY_CALL_INITIALIZE)
#undef BUILTIN_TYPED_ARRAY_CALL_INITIALIZE
}

void Builtins::LazyInitializeTypedArray(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("TypedArray"));
    auto accessor = factory_->NewInternalAccessor(nullptr,
        reinterpret_cast<void *>(BuiltinsLazyCallback::TypedArray));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetTypedArrayFunction(thread_, accessor);
    env->SetTypedArrayPrototype(thread_, accessor);
    env->SetSpecificTypedArrayFunctionClass(thread_, accessor);

#define BUILTIN_TYPED_ARRAY_CALL_LAZY_INITIALIZE(Type, TYPE, bytesPerElement) \
    LazyInitialize##Type(env);
    BUILTIN_TYPED_ARRAY_TYPES(BUILTIN_TYPED_ARRAY_CALL_LAZY_INITIALIZE)
#undef BUILTIN_TYPED_ARRAY_CALL_LAZY_INITIALIZE
}

#define BUILTIN_TYPED_ARRAY_DEFINE_INITIALIZE(Type, TYPE, bytesPerElement)                                      \
void Builtins::Initialize##Type(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &arrFuncClass) const   \
{                                                                                                               \
    [[maybe_unused]] EcmaHandleScope scope(thread_);                                                            \
    /* %TypedArray%.prototype (where %TypedArray% is one of Int8Array, Uint8Array, etc.) */                     \
    JSHandle<JSObject> arrFuncPrototype = factory_->NewJSObjectWithInit(arrFuncClass);                          \
    JSHandle<JSTaggedValue> arrFuncPrototypeValue(arrFuncPrototype);                                            \
    /* %TypedArray%.prototype_or_hclass */                                                                      \
    JSHandle<JSHClass> arrFuncInstanceHClass = factory_->NewEcmaHClass(                                         \
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_##TYPE, arrFuncPrototypeValue);                       \
    JSHandle<JSHClass> arrFuncInstanceHClassOnHeap = factory_->NewEcmaHClass(                                   \
        panda::ecmascript::JSTypedArray::SIZE, JSType::JS_##TYPE, arrFuncPrototypeValue);                       \
    arrFuncInstanceHClassOnHeap->SetIsOnHeap(true);                                                             \
    arrFuncInstanceHClass->SetHasConstructor(false);                                                            \
    /* %TypedArray% = new Function() */                                                                         \
    JSHandle<JSFunction> arrayFunction = factory_->NewSpecificTypedArrayFunction(                               \
        env, reinterpret_cast<void *>(BuiltinsTypedArray::Type##Constructor),                                   \
        kungfu::BuiltinsStubCSigns::Type##Constructor);                                                         \
    InitializeCtor(env, arrFuncPrototype, arrayFunction, #Type, FunctionLength::THREE);                         \
                                                                                                                \
    arrayFunction->SetProtoOrHClass(thread_, arrFuncInstanceHClass.GetTaggedValue());                           \
    SetConstant(arrFuncPrototype, "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));                         \
    SetConstant(JSHandle<JSObject>(arrayFunction), "BYTES_PER_ELEMENT", JSTaggedValue(bytesPerElement));        \
    /* %TypedArray%.protoofprototype (where %TypedArray% is one of Int8Array, Uint8Array, etc.) */              \
    JSTaggedValue protoOfPrototypeValue = arrFuncPrototype->GetJSHClass()->GetPrototype();                      \
    env->Set##Type##Function(thread_, arrayFunction);                                                           \
    env->Set##Type##FunctionPrototype(thread_, arrFuncPrototypeValue);                                          \
    env->Set##Type##RootHclass(thread_, arrFuncInstanceHClass);                                                 \
    env->Set##Type##RootHclassOnHeap(thread_, arrFuncInstanceHClassOnHeap);                                     \
    /* Initializes HClass record of %TypedArray% */                                                             \
    thread_->SetInitialBuiltinHClass(BuiltinTypeId::TYPE,                                                       \
        arrayFunction->GetJSHClass(),                                                                           \
        *arrFuncInstanceHClass,                                                                                 \
        arrFuncPrototype->GetJSHClass(),                                                                        \
        protoOfPrototypeValue.IsHeapObject() ? protoOfPrototypeValue.GetTaggedObject()->GetClass() : nullptr,   \
        *arrFuncInstanceHClassOnHeap);                                                                          \
}

BUILTIN_TYPED_ARRAY_TYPES(BUILTIN_TYPED_ARRAY_DEFINE_INITIALIZE)
#undef BUILTIN_TYPED_ARRAY_DEFINE_INITIALIZE

#define BUILTIN_TYPED_ARRAY_DEFINE_LAZY_INITIALIZE(Type, TYPE, bytesPerElement)                                     \
void Builtins::LazyInitialize##Type(const JSHandle<GlobalEnv> &env) const                                           \
{                                                                                                                   \
    [[maybe_unused]] EcmaHandleScope scope(thread_);                                                                \
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());                                               \
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly(#Type));                                              \
    auto accessor = factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::Type));   \
    SetLazyAccessor(globalObject, key, accessor);                                                                   \
    env->Set##Type##Function(thread_, accessor);                                                                    \
    env->Set##Type##FunctionPrototype(thread_, accessor);                                                           \
    env->Set##Type##RootHclass(thread_, accessor);                                                                  \
    env->Set##Type##RootHclassOnHeap(thread_, accessor);                                                            \
}

BUILTIN_TYPED_ARRAY_TYPES(BUILTIN_TYPED_ARRAY_DEFINE_LAZY_INITIALIZE)
#undef BUILTIN_TYPED_ARRAY_DEFINE_LAZY_INITIALIZE

void Builtins::InitializeArrayBuffer(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // ArrayBuffer.prototype
    JSHandle<JSObject> arrayBufferFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> arrayBufferFuncPrototypeValue(arrayBufferFuncPrototype);

    //  ArrayBuffer.prototype_or_hclass
    JSHandle<JSHClass> arrayBufferFuncInstanceHClass =
        factory_->NewEcmaHClass(JSArrayBuffer::SIZE, JSType::JS_ARRAY_BUFFER, arrayBufferFuncPrototypeValue);

    // ArrayBuffer = new Function()
    JSHandle<JSObject> arrayBufferFunction(NewBuiltinConstructor(
        env, arrayBufferFuncPrototype, ArrayBuffer::ArrayBufferConstructor, "ArrayBuffer", FunctionLength::ONE));

    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(arrayBufferFunction),
                                                     arrayBufferFuncInstanceHClass.GetTaggedValue());

    // ArrayBuffer prototype method
    SetFunction(env, arrayBufferFuncPrototype, "slice", ArrayBuffer::Slice, FunctionLength::TWO);

    // ArrayBuffer method
    for (const base::BuiltinFunctionEntry& entry: ArrayBuffer::GetArrayBufferFunctions()) {
        SetFunction(env,
                    arrayBufferFunction,
                    entry.GetName(),
                    entry.GetEntrypoint(),
                    entry.GetLength(),
                    entry.GetBuiltinStubId());
    }

    // 24.1.3.3 get ArrayBuffer[@@species]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, ArrayBuffer::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(arrayBufferFunction), speciesSymbol, speciesGetter);

    // 24.1.4.1 get ArrayBuffer.prototype.byteLength
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, ArrayBuffer::GetByteLength, "byteLength", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory_->NewFromASCIIReadOnly("byteLength"));
    SetGetter(arrayBufferFuncPrototype, lengthKey, lengthGetter);

    // 24.1.4.4 ArrayBuffer.prototype[@@toStringTag]
    SetStringTagSymbol(env, arrayBufferFuncPrototype, "ArrayBuffer");

    env->SetArrayBufferFunction(thread_, arrayBufferFunction.GetTaggedValue());
}

void Builtins::LazyInitializeArrayBuffer(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("ArrayBuffer"));
    auto accessor = factory_->NewInternalAccessor(nullptr,
        reinterpret_cast<void *>(BuiltinsLazyCallback::ArrayBuffer));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetArrayBufferFunction(thread_, accessor);
}

void Builtins::InitializeReflect(const JSHandle<GlobalEnv> &env,
                                 const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> reflectHClass =
        factory_->NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> reflectObject = factory_->NewJSObjectWithInit(reflectHClass);

    // Reflect functions
    for (const base::BuiltinFunctionEntry &entry: Reflect::GetReflectFunctions()) {
        SetFunction(env, reflectObject, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> reflectString(factory_->NewFromASCIIReadOnly("Reflect"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor reflectDesc(thread_, JSHandle<JSTaggedValue>::Cast(reflectObject), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, reflectString, reflectDesc);

    // @@ToStringTag
    SetStringTagSymbol(env, reflectObject, "Reflect");

    env->SetReflectFunction(thread_, reflectObject.GetTaggedValue());
}

void Builtins::InitializeSharedArrayBuffer(const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // SharedArrayBuffer.prototype
    JSHandle<JSObject> sharedArrayBufferFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> sharedArrayBufferFuncPrototypeValue(sharedArrayBufferFuncPrototype);

    //  SharedArrayBuffer.prototype_or_hclass
    JSHandle<JSHClass> sharedArrayBufferFuncInstanceHClass =
        factory_->NewEcmaHClass(
            JSArrayBuffer::SIZE, JSType::JS_SHARED_ARRAY_BUFFER, sharedArrayBufferFuncPrototypeValue);

    // SharedArrayBuffer = new Function()
    JSHandle<JSObject> SharedArrayBufferFunction(NewBuiltinConstructor(env, sharedArrayBufferFuncPrototype,
        SharedArrayBuffer::SharedArrayBufferConstructor, "SharedArrayBuffer", FunctionLength::ONE));

    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(SharedArrayBufferFunction),
                                                     sharedArrayBufferFuncInstanceHClass.GetTaggedValue());

    // SharedArrayBuffer prototype method
    SetFunction(env, sharedArrayBufferFuncPrototype, "slice", SharedArrayBuffer::Slice, FunctionLength::TWO);

    // SharedArrayBuffer method
    SetFunction(env, SharedArrayBufferFunction,
                "IsSharedArrayBuffer", SharedArrayBuffer::IsSharedArrayBuffer, FunctionLength::ONE);

    // 25.2.3.2 get SharedArrayBuffer [ @@species ]
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, SharedArrayBuffer::Species, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(JSHandle<JSObject>(SharedArrayBufferFunction), speciesSymbol, speciesGetter);

    // 25.2.4.1 get SharedArrayBuffer.prototype.byteLength
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, SharedArrayBuffer::GetByteLength, "byteLength", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory_->NewFromASCIIReadOnly("byteLength"));
    SetGetter(sharedArrayBufferFuncPrototype, lengthKey, lengthGetter);

    // 25.2.4.4 SharedArrayBuffer.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, sharedArrayBufferFuncPrototype, "SharedArrayBuffer");

    env->SetSharedArrayBufferFunction(thread_, SharedArrayBufferFunction.GetTaggedValue());
}

void Builtins::LazyInitializeSharedArrayBuffer(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("SharedArrayBuffer"));
    auto accessor =
        factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::SharedArrayBuffer));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetSharedArrayBufferFunction(thread_, accessor);
}

void Builtins::InitializePromise(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &promiseFuncClass)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Promise.prototype
    JSHandle<JSObject> promiseFuncPrototype = factory_->NewJSObjectWithInit(promiseFuncClass);
    JSHandle<JSTaggedValue> promiseFuncPrototypeValue(promiseFuncPrototype);
    // Promise.prototype_or_hclass
    JSHandle<JSHClass> promiseFuncInstanceHClass =
        factory_->NewEcmaHClass(JSPromise::SIZE, JSType::JS_PROMISE, promiseFuncPrototypeValue);
    // Promise() = new Function()
    JSHandle<JSObject> promiseFunction(
        NewBuiltinConstructor(env, promiseFuncPrototype, Promise::PromiseConstructor, "Promise", FunctionLength::ONE));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(promiseFunction),
                                                     promiseFuncInstanceHClass.GetTaggedValue());

    // Promise method
    for (const base::BuiltinFunctionEntry &entry: Promise::GetPromiseFunctions()) {
        SetFunction(env, promiseFunction, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // promise.prototype method
    for (const base::BuiltinFunctionEntry &entry: Promise::GetPromisePrototypeFunctions()) {
        SetFunction(env, promiseFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    // Promise.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, promiseFuncPrototype, "Promise");

    // Set Promise [@@species]
    JSHandle<JSTaggedValue> speciesSymbol(env->GetSpeciesSymbol());
    JSHandle<JSTaggedValue> speciesGetter =
        CreateGetter(env, Promise::GetSpecies, "[Symbol.species]", FunctionLength::ZERO);
    SetGetter(promiseFunction, speciesSymbol, speciesGetter);

    env->SetPromiseFunction(thread_, promiseFunction);
    InitializeForPromiseFuncClass(env);
}


void Builtins::InitializeForPromiseFuncClass(const JSHandle<GlobalEnv> &env)
{
    vm_ = thread_->GetEcmaVM();
    factory_ = vm_->GetFactory();

    JSHandle<JSHClass> promiseReactionFuncClass = factory_->NewEcmaHClass(
        JSPromiseReactionsFunction::SIZE, JSType::JS_PROMISE_REACTIONS_FUNCTION, env->GetFunctionPrototype());
    promiseReactionFuncClass->SetCallable(true);
    promiseReactionFuncClass->SetExtensible(true);
    env->SetPromiseReactionFunctionClass(thread_, promiseReactionFuncClass);

    JSHandle<JSHClass> promiseExecutorFuncClass = factory_->NewEcmaHClass(
        JSPromiseExecutorFunction::SIZE, JSType::JS_PROMISE_EXECUTOR_FUNCTION, env->GetFunctionPrototype());
    promiseExecutorFuncClass->SetCallable(true);
    promiseExecutorFuncClass->SetExtensible(true);
    env->SetPromiseExecutorFunctionClass(thread_, promiseExecutorFuncClass);

    JSHandle<JSHClass> asyncModuleFulfilledFuncClass = factory_->NewEcmaHClass(
        JSAsyncModuleFulfilledFunction::SIZE, JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION, env->GetFunctionPrototype());
    asyncModuleFulfilledFuncClass->SetCallable(true);
    asyncModuleFulfilledFuncClass->SetExtensible(true);
    env->SetAsyncModuleFulfilledFunctionClass(thread_, asyncModuleFulfilledFuncClass);

    JSHandle<JSHClass> asyncModuleRejectedFuncClass = factory_->NewEcmaHClass(
        JSAsyncModuleRejectedFunction::SIZE, JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION, env->GetFunctionPrototype());
    asyncModuleRejectedFuncClass->SetCallable(true);
    asyncModuleRejectedFuncClass->SetExtensible(true);
    env->SetAsyncModuleRejectedFunctionClass(thread_, asyncModuleRejectedFuncClass);

    JSHandle<JSHClass> promiseAllResolveElementFunctionClass =
        factory_->NewEcmaHClass(JSPromiseAllResolveElementFunction::SIZE,
                                JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, env->GetFunctionPrototype());
    promiseAllResolveElementFunctionClass->SetCallable(true);
    promiseAllResolveElementFunctionClass->SetExtensible(true);
    env->SetPromiseAllResolveElementFunctionClass(thread_, promiseAllResolveElementFunctionClass);

    JSHandle<JSHClass> promiseAnyRejectElementFunctionClass =
        factory_->NewEcmaHClass(JSPromiseAnyRejectElementFunction::SIZE,
                                JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION, env->GetFunctionPrototype());
    promiseAnyRejectElementFunctionClass->SetCallable(true);
    promiseAnyRejectElementFunctionClass->SetExtensible(true);
    env->SetPromiseAnyRejectElementFunctionClass(thread_, promiseAnyRejectElementFunctionClass);

    JSHandle<JSHClass> promiseAllSettledElementFunctionClass =
        factory_->NewEcmaHClass(JSPromiseAllSettledElementFunction::SIZE,
                                JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION, env->GetFunctionPrototype());
    promiseAllSettledElementFunctionClass->SetCallable(true);
    promiseAllSettledElementFunctionClass->SetExtensible(true);
    env->SetPromiseAllSettledElementFunctionClass(thread_, promiseAllSettledElementFunctionClass);

    JSHandle<JSHClass> promiseFinallyFunctionClass =
        factory_->NewEcmaHClass(JSPromiseFinallyFunction::SIZE,
                                JSType::JS_PROMISE_FINALLY_FUNCTION, env->GetFunctionPrototype());
    promiseFinallyFunctionClass->SetCallable(true);
    promiseFinallyFunctionClass->SetExtensible(true);
    env->SetPromiseFinallyFunctionClass(thread_, promiseFinallyFunctionClass);

    JSHandle<JSHClass> promiseValueThunkOrThrowerFunctionClass =
        factory_->NewEcmaHClass(JSPromiseValueThunkOrThrowerFunction::SIZE,
                                JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION, env->GetFunctionPrototype());
    promiseValueThunkOrThrowerFunctionClass->SetCallable(true);
    promiseValueThunkOrThrowerFunctionClass->SetExtensible(true);
    env->SetPromiseValueThunkOrThrowerFunctionClass(thread_, promiseValueThunkOrThrowerFunctionClass);
}

void Builtins::InitializePromiseJob(const JSHandle<GlobalEnv> &env)
{
    JSHandle<JSTaggedValue> keyString(thread_->GlobalConstants()->GetHandledEmptyString());
    auto func = NewFunction(env, keyString, BuiltinsPromiseJob::PromiseReactionJob, FunctionLength::TWO);
    env->SetPromiseReactionJob(thread_, func);
    func = NewFunction(env, keyString, BuiltinsPromiseJob::PromiseResolveThenableJob, FunctionLength::THREE);
    env->SetPromiseResolveThenableJob(thread_, func);
    func = NewFunction(env, keyString, BuiltinsPromiseJob::DynamicImportJob, FunctionLength::FOUR);
    env->SetDynamicImportJob(thread_, func);
}

void Builtins::InitializeDataView(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // ArrayBuffer.prototype
    JSHandle<JSHClass> dataViewFuncPrototypeHClass = factory_->NewEcmaHClass(
        JSObject::SIZE, DataView::GetNumPrototypeInlinedProperties(), JSType::JS_OBJECT, objFuncPrototypeVal);
    JSHandle<JSObject> dataViewFuncPrototype = factory_->NewJSObjectWithInit(dataViewFuncPrototypeHClass);
    JSHandle<JSTaggedValue> dataViewFuncPrototypeValue(dataViewFuncPrototype);

    //  ArrayBuffer.prototype_or_hclass
    JSHandle<JSHClass> dataViewFuncInstanceHClass =
        factory_->NewEcmaHClass(JSDataView::SIZE, JSType::JS_DATA_VIEW, dataViewFuncPrototypeValue);

    // ArrayBuffer = new Function()
    JSHandle<JSObject> dataViewFunction(NewBuiltinConstructor(env, dataViewFuncPrototype, DataView::DataViewConstructor,
                                                              "DataView", FunctionLength::ONE));

    JSHandle<JSFunction>(dataViewFunction)->SetProtoOrHClass(thread_, dataViewFuncInstanceHClass.GetTaggedValue());
    // DataView.prototype method
    for (const base::BuiltinFunctionEntry &entry: DataView::GetDataViewPrototypeFunctions()) {
        SetFunction(env,  dataViewFuncPrototype, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }

    // 24.2.4.1 get DataView.prototype.buffer
    JSHandle<JSTaggedValue> bufferGetter = CreateGetter(env, DataView::GetBuffer, "buffer", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> bufferKey(factory_->NewFromASCIIReadOnly("buffer"));
    SetGetter(dataViewFuncPrototype, bufferKey, bufferGetter);

    // 24.2.4.2 get DataView.prototype.byteLength
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, DataView::GetByteLength, "byteLength", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory_->NewFromASCIIReadOnly("byteLength"));
    SetGetter(dataViewFuncPrototype, lengthKey, lengthGetter);

    // 24.2.4.3 get DataView.prototype.byteOffset
    JSHandle<JSTaggedValue> offsetGetter = CreateGetter(env, DataView::GetOffset, "byteOffset", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> offsetKey(factory_->NewFromASCIIReadOnly("byteOffset"));
    SetGetter(dataViewFuncPrototype, offsetKey, offsetGetter);

    // 24.2.4.21 DataView.prototype[ @@toStringTag ]
    SetStringTagSymbol(env, dataViewFuncPrototype, "DataView");

    env->SetDataViewFunction(thread_, dataViewFunction.GetTaggedValue());
    env->SetDataViewPrototype(thread_, dataViewFuncPrototype.GetTaggedValue());
    thread_->SetInitialBuiltinHClass(BuiltinTypeId::DATA_VIEW,
        dataViewFunction->GetJSHClass(),
        *dataViewFuncInstanceHClass,
        dataViewFuncPrototype->GetJSHClass());
}

void Builtins::LazyInitializeDataView(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly("DataView"));
    auto accessor = factory_->NewInternalAccessor(nullptr, reinterpret_cast<void *>(BuiltinsLazyCallback::DataView));
    SetLazyAccessor(globalObject, key, accessor);
    env->SetDataViewFunction(thread_, accessor);
    env->SetDataViewPrototype(thread_, accessor);
}

JSHandle<JSFunction> Builtins::NewBuiltinConstructor(const JSHandle<GlobalEnv> &env,
                                                     const JSHandle<JSObject> &prototype, EcmaEntrypoint ctorFunc,
                                                     std::string_view name, int length,
                                                     kungfu::BuiltinsStubCSigns::ID builtinId,
                                                     JSHandle<JSHClass> hclass) const
{
    JSHandle<Method> target = factory_->NewMethodForNativeFunction(reinterpret_cast<void *>(ctorFunc),
                                                                   FunctionKind::BUILTIN_CONSTRUCTOR, builtinId);
    JSHandle<JSFunction> ctor;
    if (!hclass.IsEmpty()) {
        ctor = factory_->NewJSFunctionByHClass(target, hclass);
    } else {
        ctor = factory_->NewJSFunction(env, target);
    }
    InitializeCtor(env, prototype, ctor, name, length);
    return ctor;
}

JSHandle<JSFunction> Builtins::NewBuiltinCjsCtor(const JSHandle<GlobalEnv> &env,
                                                 const JSHandle<JSObject> &prototype, EcmaEntrypoint ctorFunc,
                                                 std::string_view name, int length) const
{
    JSHandle<JSFunction> ctor =
        factory_->NewJSFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_CONSTRUCTOR);

    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSFunction::SetFunctionLength(thread_, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly(name));
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(ctor), nameString,
                                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined()));
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread_, prototype, constructorKey, descriptor);

    return ctor;
}

void Builtins::RegisterBuiltinToGlobal(kungfu::BuiltinsStubCSigns::ID builtinId, JSHandle<JSFunction> function) const
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    if (IS_TYPED_BUILTINS_ID(builtinId) || IS_TYPED_INLINE_BUILTINS_ID(builtinId)) {
        globalConst->SetConstant(GET_TYPED_CONSTANT_INDEX(builtinId), function);
    }
}

JSHandle<JSFunction> Builtins::NewFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &key,
                                           EcmaEntrypoint func, int length,
                                           kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    MemSpaceType methodSpaceType = MemSpaceType::SHARED_NON_MOVABLE;
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func),
        FunctionKind::NORMAL_FUNCTION, builtinId, methodSpaceType);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSFunctionBase> baseFunction(function);
    auto globalConst = const_cast<GlobalEnvConstants *>(thread_->GlobalConstants());
    JSFunction::SetFunctionName(thread_, baseFunction, key, globalConst->GetHandledUndefined());
    RegisterBuiltinToGlobal(builtinId, function);
    return function;
}

void Builtins::SetFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, std::string_view key,
                           EcmaEntrypoint func, int length, kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    SetFunction(env, obj, keyString, func, length, builtinId);
}

void Builtins::SetFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                           const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func, int length,
                           kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSFunction> function(NewFunction(env, key, func, length, builtinId));
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(function), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, key, descriptor);
}

JSHandle<JSFunction> Builtins::SetAndReturnFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                                                    const char *key, EcmaEntrypoint func, int length,
                                                    kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    return SetAndReturnFunction(env, obj, keyString, func, length, builtinId);
}

JSHandle<JSFunction> Builtins::SetAndReturnFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                                                    const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func, int length,
                                                    kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSFunction> function(NewFunction(env, key, func, length, builtinId));
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(function), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, key, descriptor);
    return function;
}

void Builtins::SetFrozenFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, std::string_view key,
                                 EcmaEntrypoint func, int length) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    JSHandle<JSFunction> function = NewFunction(env, keyString, func, length);
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(function), false, false, false);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

template<int flag>
void Builtins::SetFunctionAtSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                                   const JSHandle<JSTaggedValue> &symbol, std::string_view name,
                                   EcmaEntrypoint func, int length) const
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly(name));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSHandle<JSTaggedValue> handleUndefine(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread_, baseFunction, nameString, handleUndefine);
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (flag == JSSymbol::SYMBOL_TO_PRIMITIVE_TYPE) {
        PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), false, false, true);
        JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
        return;
    } else if constexpr (flag == JSSymbol::SYMBOL_HAS_INSTANCE_TYPE) {  // NOLINTE(readability-braces-around-statements)
        // ecma 19.2.3.6 Function.prototype[@@hasInstance] has the attributes
        // { [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: false }.
        PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), false, false, false);
        JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
        env->SetHasInstanceFunction(thread_, function);
        return;
    }
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
}

template<int flag>
JSHandle<JSTaggedValue> Builtins::SetAndReturnFunctionAtSymbol(const JSHandle<GlobalEnv> &env,
                                                               const JSHandle<JSObject> &obj,
                                                               const JSHandle<JSTaggedValue> &symbol,
                                                               std::string_view name,
                                                               EcmaEntrypoint func,
                                                               int length,
                                                               kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func),
        FunctionKind::NORMAL_FUNCTION, builtinId);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly(name));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSHandle<JSTaggedValue> handleUndefine(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread_, baseFunction, nameString, handleUndefine);
    RegisterBuiltinToGlobal(builtinId, function);
    // NOLINTNEXTLINE(readability-braces-around-statements, bugprone-suspicious-semicolon)
    if constexpr (flag == JSSymbol::SYMBOL_TO_PRIMITIVE_TYPE) {
        PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), false, false, true);
        JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
        return JSHandle<JSTaggedValue>(function);
    } else if constexpr (flag == JSSymbol::SYMBOL_HAS_INSTANCE_TYPE) {  // NOLINTE(readability-braces-around-statements)
        // ecma 19.2.3.6 Function.prototype[@@hasInstance] has the attributes
        // { [[Writable]]: false, [[Enumerable]]: false, [[Configurable]]: false }.
        PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), false, false, false);
        JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
        env->SetHasInstanceFunction(thread_, function);
        return JSHandle<JSTaggedValue>(function);
    }
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, symbol, descriptor);
    return JSHandle<JSTaggedValue>(function);
}

void Builtins::SetStringTagSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                                  std::string_view key) const
{
    JSHandle<JSTaggedValue> tag(factory_->NewFromUtf8ReadOnly(key));
    JSHandle<JSTaggedValue> symbol = env->GetToStringTagSymbol();
    PropertyDescriptor desc(thread_, tag, false, false, true);
    JSObject::DefineOwnProperty(thread_, obj, symbol, desc);
}

JSHandle<JSTaggedValue> Builtins::CreateGetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                               std::string_view name, int length,
                                               kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSTaggedValue> funcName(factory_->NewFromUtf8ReadOnly(name));
    return CreateGetter(env, func, funcName, length, builtinId);
}

JSHandle<JSTaggedValue> Builtins::CreateGetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                               JSHandle<JSTaggedValue> key, int length,
                                               kungfu::BuiltinsStubCSigns::ID builtinId) const
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func),
        FunctionKind::GETTER_FUNCTION, builtinId);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> prefix = thread_->GlobalConstants()->GetHandledGetString();
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(function), key, prefix);
    RegisterBuiltinToGlobal(builtinId, function);
    return JSHandle<JSTaggedValue>(function);
}

JSHandle<JSTaggedValue> Builtins::CreateSetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                               std::string_view name, int length) const
{
    JSHandle<JSTaggedValue> funcName(factory_->NewFromUtf8ReadOnly(name));
    return CreateSetter(env, func, funcName, length);
}

JSHandle<JSTaggedValue> Builtins::CreateSetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                               JSHandle<JSTaggedValue> key, int length) const
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> prefix = thread_->GlobalConstants()->GetHandledSetString();
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(function), key, prefix);
    return JSHandle<JSTaggedValue>(function);
}

void Builtins::SetConstant(const JSHandle<JSObject> &obj, std::string_view key, JSTaggedValue value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(thread_, value), false, false, false);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetConstantObject(const JSHandle<JSObject> &obj, std::string_view key,
                                 JSHandle<JSTaggedValue> &value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    PropertyDescriptor descriptor(thread_, value, false, false, false);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetNonConstantObject(const JSHandle<JSObject> &obj, std::string_view key,
                                    JSHandle<JSTaggedValue> &value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    PropertyDescriptor descriptor(thread_, value, true, true, true);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetGlobalThis(const JSHandle<JSObject> &obj, std::string_view key,
                             const JSHandle<JSTaggedValue> &globalValue)
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    PropertyDescriptor descriptor(thread_, globalValue, true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetAttribute(const JSHandle<JSObject> &obj, std::string_view key, std::string_view value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>(factory_->NewFromUtf8ReadOnly(value)),
                                                                   true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
}

void Builtins::SetNoneAttributeProperty(const JSHandle<JSObject> &obj, std::string_view key,
                                        const JSHandle<JSTaggedValue> &value) const
{
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    PropertyDescriptor des(thread_, value, false, false, false);
    JSObject::DefineOwnProperty(thread_, obj, keyString, des);
}

void Builtins::SetFuncToObjAndGlobal(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject,
                                     const JSHandle<JSObject> &obj, std::string_view key,
                                     EcmaEntrypoint func, int length, kungfu::BuiltinsStubCSigns::ID builtinId)
{
    JSHandle<JSFunction> function = factory_->NewJSFunction(env, reinterpret_cast<void *>(func),
        FunctionKind::NORMAL_FUNCTION, builtinId);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> keyString(factory_->NewFromUtf8ReadOnly(key));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSHandle<JSTaggedValue> handleUndefine(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread_, baseFunction, keyString, handleUndefine);
    RegisterBuiltinToGlobal(builtinId, function);
    PropertyDescriptor descriptor(thread_, JSHandle<JSTaggedValue>::Cast(function), true, false, true);
    JSObject::DefineOwnProperty(thread_, obj, keyString, descriptor);
    JSObject::DefineOwnProperty(thread_, globalObject, keyString, descriptor);
}

void Builtins::InitializeGeneratorFunction(const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSObject> generatorFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> generatorFuncPrototypeValue(generatorFuncPrototype);

    // 26.3.3.1 GeneratorFunction.prototype.constructor
    // GeneratorFunction.prototype_or_hclass
    JSHandle<JSHClass> generatorFuncInstanceHClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, JSType::JS_GENERATOR_FUNCTION, generatorFuncPrototypeValue);
    generatorFuncInstanceHClass->SetCallable(true);
    generatorFuncInstanceHClass->SetExtensible(true);
    // GeneratorFunction = new GeneratorFunction()
    JSHandle<JSFunction> generatorFunction =
        NewBuiltinConstructor(env, generatorFuncPrototype, GeneratorObject::GeneratorFunctionConstructor,
                              "GeneratorFunction", FunctionLength::ONE);
    JSObject::SetPrototype(thread_, JSHandle<JSObject>::Cast(generatorFunction), env->GetFunctionFunction());
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor generatorDesc(thread_, JSHandle<JSTaggedValue>::Cast(generatorFunction), false, false, true);
    JSObject::DefineOwnProperty(thread_, generatorFuncPrototype, constructorKey, generatorDesc);
    generatorFunction->SetProtoOrHClass(thread_, generatorFuncInstanceHClass.GetTaggedValue());
    env->SetGeneratorFunctionFunction(thread_, generatorFunction);

    // 26.3.3.2 GeneratorFunction.prototype.prototype -> Generator prototype object.
    PropertyDescriptor descriptor(thread_, env->GetGeneratorPrototype(), false, false, true);
    JSObject::DefineOwnProperty(thread_, generatorFuncPrototype, globalConst->GetHandledPrototypeString(), descriptor);

    // 26.3.3.3 GeneratorFunction.prototype[@@toStringTag]
    SetStringTagSymbol(env, generatorFuncPrototype, "GeneratorFunction");

    // GeneratorFunction prototype __proto__ -> Function.
    JSObject::SetPrototype(thread_, generatorFuncPrototype, env->GetFunctionPrototype());

    // 26.5.1.1 Generator.prototype.constructor -> %GeneratorFunction.prototype%.
    PropertyDescriptor generatorObjDesc(thread_, generatorFuncPrototypeValue, false, false, true);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>(env->GetGeneratorPrototype()),
                                globalConst->GetHandledConstructorString(), generatorObjDesc);

    // Generator instances prototype -> GeneratorFunction.prototype.prototype
    PropertyDescriptor generatorObjProtoDesc(thread_, generatorFuncPrototypeValue, true, false, false);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>(env->GetInitialGenerator()),
                                globalConst->GetHandledPrototypeString(), generatorObjProtoDesc);

    env->SetGeneratorFunctionPrototype(thread_, generatorFuncPrototype);
}

void Builtins::InitializeAsyncGeneratorFunction(const JSHandle<GlobalEnv> &env,
                                                const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSObject> asyncGeneratorFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> asyncGeneratorFuncPrototypeValue(asyncGeneratorFuncPrototype);
     // 27.4.3.1 AsyncGeneratorFunction.prototype.constructor
    JSHandle<JSHClass> asyncGeneratorFuncInstanceHClass =
        factory_->NewEcmaHClass(JSFunction::SIZE, JSType::JS_ASYNC_GENERATOR_FUNCTION,
                                asyncGeneratorFuncPrototypeValue);
    asyncGeneratorFuncInstanceHClass->SetCallable(true);
    asyncGeneratorFuncInstanceHClass->SetExtensible(true);
    JSHandle<JSFunction> asyncGeneratorFunction =
        NewBuiltinConstructor(env, asyncGeneratorFuncPrototype,
                              AsyncGeneratorObject::AsyncGeneratorFunctionConstructor, "AsyncGeneratorFunction",
                              FunctionLength::ONE);
    JSObject::SetPrototype(thread_, JSHandle<JSObject>::Cast(asyncGeneratorFunction), env->GetFunctionFunction());
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor asyncGeneratorDesc(thread_, JSHandle<JSTaggedValue>::Cast(asyncGeneratorFunction),
                                          false, false, true);
    JSObject::DefineOwnProperty(thread_, asyncGeneratorFuncPrototype, constructorKey, asyncGeneratorDesc);
    asyncGeneratorFunction->SetProtoOrHClass(thread_, asyncGeneratorFuncInstanceHClass.GetTaggedValue());
    env->SetAsyncGeneratorFunctionFunction(thread_, asyncGeneratorFunction);

    // 27.4.3.2 AsyncGeneratorFunction.prototype.prototype
    PropertyDescriptor descriptor(thread_, env->GetAsyncGeneratorPrototype(), false, false, true);
    JSObject::DefineOwnProperty(thread_, asyncGeneratorFuncPrototype, globalConst->GetHandledPrototypeString(),
                                descriptor);

    // 27.4.3.3 AsyncGeneratorFunction.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, asyncGeneratorFuncPrototype, "AsyncGeneratorFunction");
    // AsyncGeneratorFunction prototype __proto__ -> Function.
    JSObject::SetPrototype(thread_, asyncGeneratorFuncPrototype, env->GetFunctionPrototype());

    PropertyDescriptor asyncGeneratorObjDesc(thread_, asyncGeneratorFuncPrototypeValue, false, false, true);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>(env->GetInitialAsyncGenerator()),
                                globalConst->GetHandledConstructorString(), asyncGeneratorObjDesc);

    PropertyDescriptor asyncGeneratorObjProtoDesc(thread_, asyncGeneratorFuncPrototypeValue, true, false, false);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>(env->GetInitialAsyncGenerator()),
                                globalConst->GetHandledPrototypeString(), asyncGeneratorObjProtoDesc);

    env->SetAsyncGeneratorFunctionPrototype(thread_, asyncGeneratorFuncPrototype);
}

void Builtins::InitializeGenerator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSObject> generatorPrototype = factory_->NewJSObjectWithInit(objFuncClass);

    // GeneratorObject.prototype method
    // Generator.prototype.constructor(value)
    SetFunction(env, generatorPrototype, "constructor",
                GeneratorObject::GeneratorFunctionConstructor, FunctionLength::ONE);
    // 26.5.1.2 Generator.prototype.next(value)
    SetFunction(env, generatorPrototype, "next", GeneratorObject::GeneratorPrototypeNext, FunctionLength::ONE);
    // 26.5.1.3 Generator.prototype.return(value)
    SetFunction(env, generatorPrototype, "return", GeneratorObject::GeneratorPrototypeReturn, FunctionLength::ONE);
    // 26.5.1.4 Generator.prototype.throw(exception)
    SetFunction(env, generatorPrototype, "throw", GeneratorObject::GeneratorPrototypeThrow, FunctionLength::ONE);

    // 26.5.1.5 Generator.prototype[@@toStringTag]
    SetStringTagSymbol(env, generatorPrototype, "Generator");

    env->SetGeneratorPrototype(thread_, generatorPrototype);
    JSObject::SetPrototype(thread_, generatorPrototype, env->GetIteratorPrototype());

    // Generator {}
    JSHandle<JSObject> initialGeneratorFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSObject::SetPrototype(thread_, initialGeneratorFuncPrototype, JSHandle<JSTaggedValue>(generatorPrototype));
    env->SetInitialGenerator(thread_, initialGeneratorFuncPrototype);
}

void Builtins::InitializeAsyncGenerator(const JSHandle<GlobalEnv> &env,
                                        const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);

    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSObject> asyncGeneratorFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);

    // GeneratorObject.prototype method
    // 27.6.1.2 AsyncGenerator.prototype.next ( value )
    SetFunction(env, asyncGeneratorFuncPrototype, "next", AsyncGeneratorObject::AsyncGeneratorPrototypeNext,
                FunctionLength::ONE);
    // 27.6.1.3 AsyncGenerator.prototype.return ( value )
    SetFunction(env, asyncGeneratorFuncPrototype, "return", AsyncGeneratorObject::AsyncGeneratorPrototypeReturn,
                FunctionLength::ONE);
    // 27.6.1.4 AsyncGenerator.prototype.throw ( exception )
    SetFunction(env, asyncGeneratorFuncPrototype, "throw", AsyncGeneratorObject::AsyncGeneratorPrototypeThrow,
                FunctionLength::ONE);

    // 27.6.1.5 AsyncGenerator.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, asyncGeneratorFuncPrototype, "AsyncGenerator");

    PropertyDescriptor descriptor(thread_, env->GetAsyncIteratorPrototype(), true, false, false);
    JSObject::DefineOwnProperty(thread_, asyncGeneratorFuncPrototype,
                                globalConst->GetHandledPrototypeString(), descriptor);
    env->SetAsyncGeneratorPrototype(thread_, asyncGeneratorFuncPrototype);
    JSObject::SetPrototype(thread_, asyncGeneratorFuncPrototype, env->GetAsyncIteratorPrototype());

    JSHandle<JSObject> initialAsyncGeneratorFuncPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSObject::SetPrototype(thread_, initialAsyncGeneratorFuncPrototype,
                           JSHandle<JSTaggedValue>(asyncGeneratorFuncPrototype));
    env->SetInitialAsyncGenerator(thread_, initialAsyncGeneratorFuncPrototype);
}

void Builtins::SetArgumentsSharedAccessor(const JSHandle<GlobalEnv> &env)
{
    JSHandle<JSTaggedValue> throwFunction = env->GetThrowTypeError();

    JSHandle<AccessorData> accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, throwFunction);
    accessor->SetSetter(thread_, throwFunction);
    env->SetArgumentsCallerAccessor(thread_, accessor);

    accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, throwFunction);
    accessor->SetSetter(thread_, throwFunction);
    env->SetArgumentsCalleeAccessor(thread_, accessor);
}

void Builtins::SetAccessor(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &getter, const JSHandle<JSTaggedValue> &setter) const
{
    JSHandle<AccessorData> accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, getter);
    accessor->SetSetter(thread_, setter);
    PropertyAttributes attr = PropertyAttributes::DefaultAccessor(false, false, true);
    JSObject::AddAccessor(thread_, JSHandle<JSTaggedValue>::Cast(obj), key, accessor, attr);
}

void Builtins::SetGetter(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                         const JSHandle<JSTaggedValue> &getter) const
{
    JSHandle<AccessorData> accessor = factory_->NewAccessorData();
    accessor->SetGetter(thread_, getter);
    PropertyAttributes attr = PropertyAttributes::DefaultAccessor(false, false, true);
    JSObject::AddAccessor(thread_, JSHandle<JSTaggedValue>::Cast(obj), key, accessor, attr);
}

#ifdef ARK_SUPPORT_INTL
JSHandle<JSFunction> Builtins::NewIntlConstructor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                                                  EcmaEntrypoint ctorFunc, std::string_view name, int length)
{
    JSHandle<JSFunction> ctor =
        factory_->NewJSFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_CONSTRUCTOR);
    InitializeIntlCtor(env, prototype, ctor, name, length);
    return ctor;
}

#define INTL_LAZY_INITIALIZE(type)                                               \
    void Builtins::LazyInitialize##type(const JSHandle<GlobalEnv> &env) const    \
    {                                                                            \
        [[maybe_unused]] EcmaHandleScope scope(thread_);                         \
        JSHandle<JSObject> intlObject(env->GetIntlFunction());                   \
        JSHandle<JSTaggedValue> key(factory_->NewFromUtf8ReadOnly(#type));       \
        auto accessor = factory_->NewInternalAccessor(nullptr,                   \
            reinterpret_cast<void *>(BuiltinsLazyCallback::type));               \
        SetLazyAccessor(intlObject, key, accessor);                              \
        env->Set##type##Function(thread_, accessor);                             \
    }

ITERATE_INTL(INTL_LAZY_INITIALIZE)
#undef INTL_LAZY_INITIALIZE

void Builtins::InitializeIntlCtor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                                  const JSHandle<JSFunction> &ctor, std::string_view name, int length)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSFunction::SetFunctionLength(thread_, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly(name));
    JSFunction::SetFunctionName(thread_, JSHandle<JSFunctionBase>(ctor), nameString,
                                JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined()));
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor1(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread_, prototype, constructorKey, descriptor1);

    // set "prototype" in constructor.
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_, ctor, prototype.GetTaggedValue());

    if (!JSTaggedValue::SameValue(nameString, thread_->GlobalConstants()->GetHandledAsyncFunctionString())) {
        JSHandle<JSObject> intlObject(thread_, env->GetIntlFunction().GetTaggedValue());
        PropertyDescriptor descriptor2(thread_, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
        JSObject::DefineOwnProperty(thread_, intlObject, nameString, descriptor2);
    }
}

void Builtins::InitializeIntl(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeValue)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<JSHClass> intlHClass = factory_->NewEcmaHClass(JSIntl::SIZE, JSType::JS_INTL, objFuncPrototypeValue);
    JSHandle<JSObject> intlObject = factory_->NewJSObjectWithInit(intlHClass);

    JSHandle<JSTaggedValue> initIntlSymbol(factory_->NewPublicSymbolWithChar("Symbol.IntlLegacyConstructedSymbol"));
    SetNoneAttributeProperty(intlObject, "fallbackSymbol", initIntlSymbol);

    SetFunction(env, intlObject, "getCanonicalLocales", Intl::GetCanonicalLocales, FunctionLength::ONE);

    // initial value of the "Intl" property of the global object.
    JSHandle<JSTaggedValue> intlString(factory_->NewFromASCIIReadOnly("Intl"));
    JSHandle<JSObject> globalObject(thread_, env->GetGlobalObject());
    PropertyDescriptor intlDesc(thread_, JSHandle<JSTaggedValue>::Cast(intlObject), true, false, true);
    JSObject::DefineOwnProperty(thread_, globalObject, intlString, intlDesc);

    SetStringTagSymbol(env, intlObject, "Intl");

    env->SetIntlFunction(thread_, intlObject);
}

void Builtins::InitializeDateTimeFormat(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // DateTimeFormat.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> dtfPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> dtfPrototypeValue(dtfPrototype);

    // DateTimeFormat.prototype_or_hclass
    JSHandle<JSHClass> dtfFuncInstanceHClass =
        factory_->NewEcmaHClass(JSDateTimeFormat::SIZE, JSType::JS_DATE_TIME_FORMAT, dtfPrototypeValue);

    // DateTimeFormat = new Function()
    // 13.4.1 Intl.DateTimeFormat.prototype.constructor
    JSHandle<JSObject> dtfFunction(NewIntlConstructor(env, dtfPrototype, DateTimeFormat::DateTimeFormatConstructor,
                                                      "DateTimeFormat", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(dtfFunction),
                                                     JSTaggedValue(*dtfFuncInstanceHClass));

    // 13.3.2 Intl.DateTimeFormat.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, dtfFunction, "supportedLocalesOf", DateTimeFormat::SupportedLocalesOf, FunctionLength::ONE);

    // DateTimeFormat.prototype method
    // 13.4.2 Intl.DateTimeFormat.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, dtfPrototype, "Intl.DateTimeFormat");
    env->SetDateTimeFormatFunction(thread_, dtfFunction);

    // 13.4.3 get Intl.DateTimeFormat.prototype.format
    JSHandle<JSTaggedValue> formatGetter = CreateGetter(env, DateTimeFormat::Format, "format", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> formatSetter(thread_, JSTaggedValue::Undefined());
    SetAccessor(dtfPrototype, thread_->GlobalConstants()->GetHandledFormatString(), formatGetter, formatSetter);

    // 13.4.4 Intl.DateTimeFormat.prototype.formatToParts ( date )
    SetFunction(env, dtfPrototype, "formatToParts", DateTimeFormat::FormatToParts, FunctionLength::ONE);

    // 13.4.5 Intl.DateTimeFormat.prototype.resolvedOptions ()
    SetFunction(env, dtfPrototype, "resolvedOptions", DateTimeFormat::ResolvedOptions, FunctionLength::ZERO);

    SetFunction(env, dtfPrototype, "formatRange", DateTimeFormat::FormatRange, FunctionLength::TWO);

    SetFunction(env, dtfPrototype, "formatRangeToParts", DateTimeFormat::FormatRangeToParts, FunctionLength::TWO);
}

void Builtins::InitializeRelativeTimeFormat(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // RelativeTimeFormat.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> rtfPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> rtfPrototypeValue(rtfPrototype);

    // RelativeTimeFormat.prototype_or_hclass
    JSHandle<JSHClass> rtfFuncInstanceHClass =
        factory_->NewEcmaHClass(JSRelativeTimeFormat::SIZE, JSType::JS_RELATIVE_TIME_FORMAT, rtfPrototypeValue);

    // RelativeTimeFormat = new Function()
    // 14.2.1 Intl.RelativeTimeFormat.prototype.constructor
    JSHandle<JSObject> rtfFunction(NewIntlConstructor(env, rtfPrototype,
                                                      RelativeTimeFormat::RelativeTimeFormatConstructor,
                                                      "RelativeTimeFormat", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(rtfFunction),
                                                     JSTaggedValue(*rtfFuncInstanceHClass));

    // 14.3.2 Intl.RelativeTimeFormat.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, rtfFunction, "supportedLocalesOf", RelativeTimeFormat::SupportedLocalesOf, FunctionLength::ONE);

    // RelativeTimeFormat.prototype method
    // 14.4.2 Intl.RelativeTimeFormat.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, rtfPrototype, "Intl.RelativeTimeFormat");
    env->SetRelativeTimeFormatFunction(thread_, rtfFunction);

    // 14.4.3 get Intl.RelativeTimeFormat.prototype.format
    SetFunction(env, rtfPrototype, "format", RelativeTimeFormat::Format, FunctionLength::TWO);

    // 14.4.4  Intl.RelativeTimeFormat.prototype.formatToParts( value, unit )
    SetFunction(env, rtfPrototype, "formatToParts", RelativeTimeFormat::FormatToParts, FunctionLength::TWO);

    // 14.4.5 Intl.RelativeTimeFormat.prototype.resolvedOptions ()
    SetFunction(env, rtfPrototype, "resolvedOptions", RelativeTimeFormat::ResolvedOptions, FunctionLength::ZERO);
}

void Builtins::InitializeNumberFormat(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // NumberFormat.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> nfPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> nfPrototypeValue(nfPrototype);

    // NumberFormat.prototype_or_hclass
    JSHandle<JSHClass> nfFuncInstanceHClass =
        factory_->NewEcmaHClass(JSNumberFormat::SIZE, JSType::JS_NUMBER_FORMAT, nfPrototypeValue);

    // NumberFormat = new Function()
    // 12.4.1 Intl.NumberFormat.prototype.constructor
    JSHandle<JSObject> nfFunction(NewIntlConstructor(env, nfPrototype, NumberFormat::NumberFormatConstructor,
                                                     "NumberFormat", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(nfFunction),
                                                     JSTaggedValue(*nfFuncInstanceHClass));

    // 12.3.2 Intl.NumberFormat.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, nfFunction, "supportedLocalesOf", NumberFormat::SupportedLocalesOf, FunctionLength::ONE);

    // NumberFormat.prototype method
    // 12.4.2 Intl.NumberFormat.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, nfPrototype, "Intl.NumberFormat");
    env->SetNumberFormatFunction(thread_, nfFunction);

    // 12.4.3 get Intl.NumberFormat.prototype.format
    JSHandle<JSTaggedValue> formatGetter = CreateGetter(env, NumberFormat::Format, "format", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> formatSetter(thread_, JSTaggedValue::Undefined());
    SetAccessor(nfPrototype, thread_->GlobalConstants()->GetHandledFormatString(), formatGetter, formatSetter);

    // 12.4.4 Intl.NumberFormat.prototype.formatToParts ( date )
    SetFunction(env, nfPrototype, "formatToParts", NumberFormat::FormatToParts, FunctionLength::ONE);

    // 12.4.5 Intl.NumberFormat.prototype.resolvedOptions ()
    SetFunction(env, nfPrototype, "resolvedOptions", NumberFormat::ResolvedOptions, FunctionLength::ZERO);
}

void Builtins::InitializeLocale(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Locale.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> localePrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> localePrototypeValue(localePrototype);

    // Locale.prototype_or_hclass
    JSHandle<JSHClass> localeFuncInstanceHClass =
        factory_->NewEcmaHClass(JSLocale::SIZE, JSType::JS_LOCALE, localePrototypeValue);

    // Locale = new Function()
    JSHandle<JSObject> localeFunction(
        NewIntlConstructor(env, localePrototype, Locale::LocaleConstructor, "Locale", FunctionLength::ONE));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(localeFunction),
                                                     JSTaggedValue(*localeFuncInstanceHClass));

    // Locale.prototype method
    SetFunction(env, localePrototype, "maximize", Locale::Maximize, FunctionLength::ZERO);
    SetFunction(env, localePrototype, "minimize", Locale::Minimize, FunctionLength::ZERO);
    SetFunction(env, localePrototype, "toString", Locale::ToString, FunctionLength::ZERO);

    JSHandle<JSTaggedValue> baseNameGetter = CreateGetter(env, Locale::GetBaseName, "baseName", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledBaseNameString(), baseNameGetter);

    JSHandle<JSTaggedValue> calendarGetter = CreateGetter(env, Locale::GetCalendar, "calendar", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledCalendarString(), calendarGetter);

    JSHandle<JSTaggedValue> caseFirstGetter =
        CreateGetter(env, Locale::GetCaseFirst, "caseFirst", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledCaseFirstString(), caseFirstGetter);

    JSHandle<JSTaggedValue> collationGetter =
        CreateGetter(env, Locale::GetCollation, "collation", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledCollationString(), collationGetter);

    JSHandle<JSTaggedValue> hourCycleGetter =
        CreateGetter(env, Locale::GetHourCycle, "hourCycle", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledHourCycleString(), hourCycleGetter);

    JSHandle<JSTaggedValue> numericGetter = CreateGetter(env, Locale::GetNumeric, "numeric", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledNumericString(), numericGetter);

    JSHandle<JSTaggedValue> numberingSystemGetter =
        CreateGetter(env, Locale::GetNumberingSystem, "numberingSystem", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledNumberingSystemString(), numberingSystemGetter);

    JSHandle<JSTaggedValue> languageGetter = CreateGetter(env, Locale::GetLanguage, "language", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledLanguageString(), languageGetter);

    JSHandle<JSTaggedValue> scriptGetter = CreateGetter(env, Locale::GetScript, "script", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledScriptString(), scriptGetter);

    JSHandle<JSTaggedValue> regionGetter = CreateGetter(env, Locale::GetRegion, "region", FunctionLength::ZERO);
    SetGetter(localePrototype, thread_->GlobalConstants()->GetHandledRegionString(), regionGetter);

    // 10.3.2 Intl.Locale.prototype[ @@toStringTag ]
    SetStringTagSymbol(env, localePrototype, "Intl.Locale");
    env->SetLocaleFunction(thread_, localeFunction);
}

void Builtins::InitializeCollator(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Collator.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> collatorPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> collatorPrototypeValue(collatorPrototype);

    // Collator.prototype_or_hclass
    JSHandle<JSHClass> collatorFuncInstanceHClass =
        factory_->NewEcmaHClass(JSCollator::SIZE, JSType::JS_COLLATOR, collatorPrototypeValue);

    // Collator = new Function()
    // 11.1.2 Intl.Collator.prototype.constructor
    JSHandle<JSObject> collatorFunction(
        NewIntlConstructor(env, collatorPrototype, Collator::CollatorConstructor, "Collator", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(collatorFunction),
                                                     JSTaggedValue(*collatorFuncInstanceHClass));

    // 11.2.2 Intl.Collator.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, collatorFunction, "supportedLocalesOf", Collator::SupportedLocalesOf, FunctionLength::ONE);

    // Collator.prototype method
    // 11.3.2 Intl.Collator.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, collatorPrototype, "Intl.Collator");
    env->SetCollatorFunction(thread_, collatorFunction);

    // 11.3.3 get Intl.Collator.prototype.compare
    JSHandle<JSTaggedValue> compareGetter = CreateGetter(env, Collator::Compare, "compare", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> compareSetter(thread_, JSTaggedValue::Undefined());
    SetAccessor(collatorPrototype, thread_->GlobalConstants()->GetHandledCompareString(), compareGetter, compareSetter);

    // 11.3.4 Intl.Collator.prototype.resolvedOptions ()
    SetFunction(env, collatorPrototype, "resolvedOptions", Collator::ResolvedOptions, FunctionLength::ZERO,
                kungfu::BuiltinsStubCSigns::CollatorResolvedOptions);
}

void Builtins::InitializePluralRules(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // PluralRules.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> prPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> prPrototypeValue(prPrototype);

    // PluralRules.prototype_or_hclass
    JSHandle<JSHClass> prFuncInstanceHClass =
        factory_->NewEcmaHClass(JSPluralRules::SIZE, JSType::JS_PLURAL_RULES, prPrototypeValue);

    // PluralRules = new Function()
    // 15.2.1 Intl.PluralRules.prototype.constructor
    JSHandle<JSObject> prFunction(
        NewIntlConstructor(env, prPrototype, PluralRules::PluralRulesConstructor, "PluralRules", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(prFunction),
                                                     JSTaggedValue(*prFuncInstanceHClass));

    // 15.3.2 Intl.PluralRules.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, prFunction, "supportedLocalesOf", PluralRules::SupportedLocalesOf, FunctionLength::ONE);

    // PluralRules.prototype method
    // 15.4.2 Intl.PluralRules.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, prPrototype, "Intl.PluralRules");
    env->SetPluralRulesFunction(thread_, prFunction);

    // 15.4.3 get Intl.PluralRules.prototype.select
    SetFunction(env, prPrototype, "select", PluralRules::Select, FunctionLength::ONE);

    // 15.4.5 Intl.PluralRules.prototype.resolvedOptions ()
    SetFunction(env, prPrototype, "resolvedOptions", PluralRules::ResolvedOptions, FunctionLength::ZERO);
}

void Builtins::InitializeDisplayNames(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // DisplayNames.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> dnPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> dnPrototypeValue(dnPrototype);

    // DisplayNames.prototype_or_hclass
    JSHandle<JSHClass> dnFuncInstanceHClass =
        factory_->NewEcmaHClass(JSDisplayNames::SIZE, JSType::JS_DISPLAYNAMES, dnPrototypeValue);

    // DisplayNames = new Function()
    // 12.4.1 Intl.DisplayNames.prototype.constructor
    JSHandle<JSObject> dnFunction(NewIntlConstructor(env, dnPrototype, DisplayNames::DisplayNamesConstructor,
                                                     "DisplayNames", FunctionLength::TWO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(dnFunction),
                                                     JSTaggedValue(*dnFuncInstanceHClass));

    // 12.3.2 Intl.DisplayNames.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, dnFunction, "supportedLocalesOf", DisplayNames::SupportedLocalesOf, FunctionLength::ONE);

    // DisplayNames.prototype method
    // 12.4.2 Intl.DisplayNames.prototype[ @@toStringTag ]
    SetStringTagSymbol(env, dnPrototype, "Intl.DisplayNames");
    env->SetDisplayNamesFunction(thread_, dnFunction);

    // 12.4.3 get Intl.DisplayNames.prototype.of
    SetFunction(env, dnPrototype, "of", DisplayNames::Of, FunctionLength::ONE);

    // 12.4.4 Intl.DisplayNames.prototype.resolvedOptions ()
    SetFunction(env, dnPrototype, "resolvedOptions", DisplayNames::ResolvedOptions, FunctionLength::ZERO);
}

void Builtins::InitializeListFormat(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // JSListFormat.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> lfPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> lfPrototypeValue(lfPrototype);

    // JSListFormat.prototype_or_hclass
    JSHandle<JSHClass> lfFuncInstanceHClass =
        factory_->NewEcmaHClass(JSListFormat::SIZE, JSType::JS_LIST_FORMAT, lfPrototypeValue);

    // JSListFormat = new Function()
    // 13.4.1 Intl.ListFormat.prototype.constructor
    JSHandle<JSObject> lfFunction(NewIntlConstructor(env, lfPrototype, ListFormat::ListFormatConstructor,
                                                     "ListFormat", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(lfFunction),
                                                     JSTaggedValue(*lfFuncInstanceHClass));

    // 13.3.2 Intl.ListFormat.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, lfFunction, "supportedLocalesOf", ListFormat::SupportedLocalesOf, FunctionLength::ONE);

    // ListFormat.prototype method
    // 13.4.2 Intl.ListFormat.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, lfPrototype, "Intl.ListFormat");
    env->SetListFormatFunction(thread_, lfFunction);

    // 13.4.3 get Intl.ListFormat.prototype.format( list )
    SetFunction(env, lfPrototype, "format", ListFormat::Format, FunctionLength::ONE);

    // 13.4.4 Intl.ListFormat.prototype.formatToParts( list )
    SetFunction(env, lfPrototype, "formatToParts", ListFormat::FormatToParts, FunctionLength::ONE);

    // 13.4.5 Intl.ListFormat.prototype.resolvedOptions()
    SetFunction(env, lfPrototype, "resolvedOptions", ListFormat::ResolvedOptions, FunctionLength::ZERO);
}

void Builtins::InitializeSegmenter(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Segmenter.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> sgPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> sgPrototypeValue(sgPrototype);

    // Segmenter.prototype_or_hclass
    JSHandle<JSHClass> sgFuncInstanceHClass =
        factory_->NewEcmaHClass(JSSegmenter::SIZE, JSType::JS_SEGMENTER, sgPrototypeValue);

    // Segmenter = new Function()
    // 18.3.1 Intl.Segmenter.prototype.constructor
    JSHandle<JSObject> sgFunction(NewIntlConstructor(env, sgPrototype, Segmenter::SegmenterConstructor,
                                                     "Segmenter", FunctionLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(sgFunction),
                                                     sgFuncInstanceHClass.GetTaggedValue());

    // 18.2.2 Intl.Segmenter.supportedLocalesOf ( locales [ , options ] )
    SetFunction(env, sgFunction, "supportedLocalesOf", Segmenter::SupportedLocalesOf, FunctionLength::ONE);

    // Segmenter.prototype method
    // 18.3.2 Intl.Segmenter.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, sgPrototype, "Intl.Segmenter");
    env->SetSegmenterFunction(thread_, sgFunction);

    // 18.3.4 Intl.Segmenter.prototype.resolvedOptions ( )
    SetFunction(env, sgPrototype, "resolvedOptions", Segmenter::ResolvedOptions, FunctionLength::ZERO);

    // 18.3.3 Intl.Segmenter.prototype.segment ( string )
    SetFunction(env, sgPrototype, "segment", Segmenter::Segment, FunctionLength::ONE);
}

void Builtins::InitializeSegments(const JSHandle<GlobalEnv> &env)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // Segments.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> segmentsPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> segmentsPrototypeValue(segmentsPrototype);

    // Segments.prototype_or_hclass
    JSHandle<JSHClass> segmentsFuncInstanceHClass =
        factory_->NewEcmaHClass(JSSegments::SIZE, JSType::JS_SEGMENTS, segmentsPrototypeValue);

    JSHandle<JSFunction> segmentsFunction(
        factory_->NewJSFunction(env, static_cast<void *>(nullptr), FunctionKind::BASE_CONSTRUCTOR));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(segmentsFunction),
                                                     segmentsFuncInstanceHClass.GetTaggedValue());

    env->SetSegmentsFunction(thread_, segmentsFunction);

    // %SegmentsPrototype%.containing ( index )
    SetFunction(env, segmentsPrototype, "containing", Segments::Containing, FunctionLength::ONE);
    SetAndReturnFunctionAtSymbol(env, segmentsPrototype,
        env->GetIteratorSymbol(), "[Symbol.iterator]", Segments::GetSegmentIterator, FunctionLength::ZERO);
}

void Builtins::InitializeSegmentIterator(const JSHandle<GlobalEnv> &env,
                                         const JSHandle<JSHClass> &iteratorFuncClass) const
{
    // SegmentIterator.prototype
    JSHandle<JSObject> segIterPrototype(factory_->NewJSObjectWithInit(iteratorFuncClass));

    // SegmentIterator.prototype_or_hclass
    JSHandle<JSHClass> segIterFuncInstanceHClass = factory_->NewEcmaHClass(
        JSSegmentIterator::SIZE, JSType::JS_SEGMENT_ITERATOR, JSHandle<JSTaggedValue>(segIterPrototype));

    JSHandle<JSFunction> segIterFunction(
        factory_->NewJSFunction(env, static_cast<void *>(nullptr), FunctionKind::BASE_CONSTRUCTOR));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(segIterFunction),
                                                     segIterFuncInstanceHClass.GetTaggedValue());

    SetFunction(env, segIterPrototype, "next", SegmentIterator::Next, FunctionLength::ZERO);
    SetStringTagSymbol(env, segIterPrototype, "Segmenter String Iterator");

    env->SetSegmentIterator(thread_, segIterFunction);
    env->SetSegmentIteratorPrototype(thread_, segIterPrototype);
}
#endif // #ifdef ARK_SUPPORT_INTL

JSHandle<JSObject> Builtins::InitializeArkTools(const JSHandle<GlobalEnv> &env) const
{
    JSHandle<JSObject> tools = factory_->NewEmptyJSObject();
    for (const base::BuiltinFunctionEntry &entry: builtins::BuiltinsArkTools::GetArkToolsFunctions()) {
        SetFunction(env, tools, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    JSHandle<JSTaggedValue> gcBuiltins(InitializeGcBuiltins(env));
    SetConstantObject(tools, "GC", gcBuiltins);
    return tools;
}

JSHandle<JSObject> Builtins::InitializeGcBuiltins(const JSHandle<GlobalEnv> &env) const
{
    JSHandle<JSObject> builtins = factory_->NewEmptyJSObject();
    for (const base::BuiltinFunctionEntry &entry: builtins::BuiltinsGc::GetGcFunctions()) {
        SetFunction(env, builtins, entry.GetName(), entry.GetEntrypoint(),
                    entry.GetLength(), entry.GetBuiltinStubId());
    }
    return builtins;
}

void Builtins::InitializeGlobalRegExp(JSHandle<JSObject> &obj) const
{
    // $1
    auto accessor1 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture1),
                                                   reinterpret_cast<void *>(RegExp::GetCapture1));
    PropertyDescriptor descriptor1(thread_, JSHandle<JSTaggedValue>::Cast(accessor1), true, false, true);
    JSHandle<JSTaggedValue> dollar1Key = thread_->GlobalConstants()->GetHandledDollarStringOne();
    JSObject::DefineOwnProperty(thread_, obj, dollar1Key, descriptor1);
    // $2
    auto accessor2 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture2),
                                                   reinterpret_cast<void *>(RegExp::GetCapture2));
    PropertyDescriptor descriptor2(thread_, JSHandle<JSTaggedValue>::Cast(accessor2), true, false, true);
    JSHandle<JSTaggedValue> dollar2Key = thread_->GlobalConstants()->GetHandledDollarStringTwo();
    JSObject::DefineOwnProperty(thread_, obj, dollar2Key, descriptor2);
    // $3
    auto accessor3 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture3),
                                                   reinterpret_cast<void *>(RegExp::GetCapture3));
    PropertyDescriptor descriptor3(thread_, JSHandle<JSTaggedValue>::Cast(accessor3), true, false, true);
    JSHandle<JSTaggedValue> dollar3Key = thread_->GlobalConstants()->GetHandledDollarStringThree();
    JSObject::DefineOwnProperty(thread_, obj, dollar3Key, descriptor3);
    // $4
    auto accessor4 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture4),
                                                   reinterpret_cast<void *>(RegExp::GetCapture4));
    PropertyDescriptor descriptor4(thread_, JSHandle<JSTaggedValue>::Cast(accessor4), true, false, true);
    JSHandle<JSTaggedValue> dollar4Key = thread_->GlobalConstants()->GetHandledDollarStringFour();
    JSObject::DefineOwnProperty(thread_, obj, dollar4Key, descriptor4);
    // $5
    auto accessor5 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture5),
                                                   reinterpret_cast<void *>(RegExp::GetCapture5));
    PropertyDescriptor descriptor5(thread_, JSHandle<JSTaggedValue>::Cast(accessor5), true, false, true);
    JSHandle<JSTaggedValue> dollar5Key = thread_->GlobalConstants()->GetHandledDollarStringFive();
    JSObject::DefineOwnProperty(thread_, obj, dollar5Key, descriptor5);
    // $6
    auto accessor6 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture6),
                                                   reinterpret_cast<void *>(RegExp::GetCapture6));
    PropertyDescriptor descriptor6(thread_, JSHandle<JSTaggedValue>::Cast(accessor6), true, false, true);
    JSHandle<JSTaggedValue> dollar6Key = thread_->GlobalConstants()->GetHandledDollarStringSix();
    JSObject::DefineOwnProperty(thread_, obj, dollar6Key, descriptor6);
    // $7
    auto accessor7 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture7),
                                                   reinterpret_cast<void *>(RegExp::GetCapture7));
    PropertyDescriptor descriptor7(thread_, JSHandle<JSTaggedValue>::Cast(accessor7), true, false, true);
    JSHandle<JSTaggedValue> dollar7Key = thread_->GlobalConstants()->GetHandledDollarStringSeven();
    JSObject::DefineOwnProperty(thread_, obj, dollar7Key, descriptor7);
    // $8
    auto accessor8 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture8),
                                                   reinterpret_cast<void *>(RegExp::GetCapture8));
    PropertyDescriptor descriptor8(thread_, JSHandle<JSTaggedValue>::Cast(accessor8), true, false, true);
    JSHandle<JSTaggedValue> dollar8Key = thread_->GlobalConstants()->GetHandledDollarStringEight();
    JSObject::DefineOwnProperty(thread_, obj, dollar8Key, descriptor8);
    // $9
    auto accessor9 = factory_->NewInternalAccessor(reinterpret_cast<void *>(RegExp::SetCapture9),
                                                   reinterpret_cast<void *>(RegExp::GetCapture9));
    PropertyDescriptor descriptor9(thread_, JSHandle<JSTaggedValue>::Cast(accessor9), true, false, true);
    JSHandle<JSTaggedValue> dollar9Key = thread_->GlobalConstants()->GetHandledDollarStringNine();
    JSObject::DefineOwnProperty(thread_, obj, dollar9Key, descriptor9);
}

JSHandle<JSObject> Builtins::InitializeArkPrivate(const JSHandle<GlobalEnv> &env) const
{
    JSHandle<JSObject> arkPrivate = factory_->NewEmptyJSObject();
    SetFrozenFunction(env, arkPrivate, "Load", ContainersPrivate::Load, FunctionLength::ZERO);

    // It is used to provide non ECMA standard jsapi containers.
    SetConstant(arkPrivate, "ArrayList", JSTaggedValue(static_cast<int>(containers::ContainerTag::ArrayList)));
    SetConstant(arkPrivate, "Queue", JSTaggedValue(static_cast<int>(containers::ContainerTag::Queue)));
    SetConstant(arkPrivate, "Deque", JSTaggedValue(static_cast<int>(containers::ContainerTag::Deque)));
    SetConstant(arkPrivate, "Stack", JSTaggedValue(static_cast<int>(containers::ContainerTag::Stack)));
    SetConstant(arkPrivate, "Vector", JSTaggedValue(static_cast<int>(containers::ContainerTag::Vector)));
    SetConstant(arkPrivate, "BitVector", JSTaggedValue(static_cast<int>(containers::ContainerTag::BitVector)));
    SetConstant(arkPrivate, "List", JSTaggedValue(static_cast<int>(containers::ContainerTag::List)));
    SetConstant(arkPrivate, "LinkedList", JSTaggedValue(static_cast<int>(containers::ContainerTag::LinkedList)));
    SetConstant(arkPrivate, "TreeMap", JSTaggedValue(static_cast<int>(containers::ContainerTag::TreeMap)));
    SetConstant(arkPrivate, "TreeSet", JSTaggedValue(static_cast<int>(containers::ContainerTag::TreeSet)));
    SetConstant(arkPrivate, "HashMap", JSTaggedValue(static_cast<int>(containers::ContainerTag::HashMap)));
    SetConstant(arkPrivate, "HashSet", JSTaggedValue(static_cast<int>(containers::ContainerTag::HashSet)));
    SetConstant(arkPrivate, "LightWeightMap",
                JSTaggedValue(static_cast<int>(containers::ContainerTag::LightWeightMap)));
    SetConstant(arkPrivate, "LightWeightSet",
                JSTaggedValue(static_cast<int>(containers::ContainerTag::LightWeightSet)));
    SetConstant(arkPrivate, "PlainArray", JSTaggedValue(static_cast<int>(containers::ContainerTag::PlainArray)));
    return arkPrivate;
}

void Builtins::InitializeModuleNamespace(const JSHandle<GlobalEnv> &env,
                                         const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // ModuleNamespace.prototype
    JSHandle<JSObject> moduleNamespacePrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> moduleNamespacePrototypeValue(moduleNamespacePrototype);

    //  ModuleNamespace.prototype_or_hclass
    JSHandle<JSHClass> moduleNamespaceHClass =
        factory_->NewEcmaHClass(ModuleNamespace::SIZE, JSType::JS_MODULE_NAMESPACE, moduleNamespacePrototypeValue);
    moduleNamespaceHClass->SetPrototype(thread_, JSTaggedValue::Null());
    env->SetModuleNamespaceClass(thread_, moduleNamespaceHClass.GetTaggedValue());

    // moduleNamespace.prototype [ @@toStringTag ]
    SetStringTagSymbol(env, moduleNamespacePrototype, "Module");
}

void Builtins::InitializeNativeModuleFailureInfo(const JSHandle<GlobalEnv> &env,
                                                 const JSHandle<JSHClass> &objFuncClass) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // NativeModuleFailureInfo.prototype
    JSHandle<JSObject> nativeModuleFailureInfoPrototype = factory_->NewJSObjectWithInit(objFuncClass);
    JSHandle<JSTaggedValue> nativeModuleFailureInfoPrototypeValue(nativeModuleFailureInfoPrototype);

    // NativeModuleFailureInfo.prototype_or_hclass
    JSHandle<JSHClass> nativeModuleFailureInfoHClass =
        factory_->NewEcmaHClass(NativeModuleFailureInfo::SIZE, JSType::NATIVE_MODULE_FAILURE_INFO,
            nativeModuleFailureInfoPrototypeValue);
    nativeModuleFailureInfoHClass->SetPrototype(thread_, JSTaggedValue::Null());
    env->SetNativeModuleFailureInfoClass(thread_, nativeModuleFailureInfoHClass.GetTaggedValue());
}

void Builtins::InitializeCjsModule(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // CjsModule.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> cjsModulePrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> cjsModulePrototypeValue(cjsModulePrototype);

    // CjsModule.prototype_or_hclass
    JSHandle<JSHClass> cjsModuleHClass =
        factory_->NewEcmaHClass(CjsModule::SIZE, JSType::JS_CJS_MODULE, cjsModulePrototypeValue);

    // CjsModule.prototype.Constructor
    JSHandle<JSObject> cjsModuleFunction(
        NewBuiltinCjsCtor(env, cjsModulePrototype, BuiltinsCjsModule::CjsModuleConstructor, "Module",
                          FunctionLength::TWO));

    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(cjsModuleFunction),
                                                     cjsModuleHClass.GetTaggedValue());

    // CjsModule method
    SetFunction(env, cjsModuleFunction, "_load", BuiltinsCjsModule::Load, FunctionLength::ONE);
    SetFunction(env, cjsModuleFunction, "_resolveFilename", BuiltinsCjsModule::ResolveFilename, FunctionLength::ONE);

    // CjsModule.prototype method
    SetFunction(env, cjsModulePrototype, "require", BuiltinsCjsModule::Require, FunctionLength::ONE);
    SetFunction(env, cjsModulePrototype, "getExportsForCircularRequire",
                BuiltinsCjsModule::GetExportsForCircularRequire, FunctionLength::ONE);
    SetFunction(env, cjsModulePrototype, "updateChildren", BuiltinsCjsModule::UpdateChildren, FunctionLength::ONE);

    JSHandle<JSTaggedValue> id(thread_->GlobalConstants()->GetHandledEmptyString());
    JSHandle<JSTaggedValue> path(thread_->GlobalConstants()->GetHandledEmptyString());
    JSHandle<JSTaggedValue> exports(factory_->NewEmptyJSObject());
    JSHandle<JSTaggedValue> parent(factory_->NewEmptyJSObject());
    JSHandle<JSTaggedValue> filename(thread_->GlobalConstants()->GetHandledEmptyString());
    JSHandle<JSTaggedValue> loaded(factory_->NewEmptyJSObject());
    JSHandle<JSTaggedValue> children(factory_->NewEmptyJSObject());
    JSHandle<JSTaggedValue> cache = JSHandle<JSTaggedValue>::Cast(CjsModuleCache::Create(thread_,
        CjsModuleCache::DEAULT_DICTIONART_CAPACITY));

    // CjsModule.prototype members
    SetNonConstantObject(cjsModulePrototype, "id", id);
    SetNonConstantObject(cjsModulePrototype, "path", path);
    SetNonConstantObject(cjsModulePrototype, "exports", exports);
    SetNonConstantObject(cjsModulePrototype, "parent", parent);
    SetNonConstantObject(cjsModulePrototype, "filename", filename);
    SetNonConstantObject(cjsModulePrototype, "loaded", loaded);
    SetNonConstantObject(cjsModulePrototype, "children", children);

    // CjsModule members
    SetNonConstantObject(cjsModuleFunction, "_cache", cache);

    env->SetCjsModuleFunction(thread_, cjsModuleFunction);
}

void Builtins::InitializeCjsExports(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);

    // CjsExports.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> cjsExportsPrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> cjsExportsPrototypeValue(cjsExportsPrototype);

    // CjsExports.prototype_or_hclass
    JSHandle<JSHClass> cjsExportsHClass =
        factory_->NewEcmaHClass(CjsExports::SIZE, JSType::JS_CJS_EXPORTS, cjsExportsPrototypeValue);

    // CjsExports.prototype.Constructor
    JSHandle<JSObject> cjsExportsFunction(
        NewBuiltinCjsCtor(env, cjsExportsPrototype, BuiltinsCjsExports::CjsExportsConstructor, "Exports",
                          FunctionLength::TWO));

    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(cjsExportsFunction),
                                                     cjsExportsHClass.GetTaggedValue());

    env->SetCjsExportsFunction(thread_, cjsExportsFunction);
}

void Builtins::InitializeCjsRequire(const JSHandle<GlobalEnv> &env) const
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    // CjsRequire.prototype
    JSHandle<JSFunction> objFun(env->GetObjectFunction());
    JSHandle<JSObject> cjsRequirePrototype = factory_->NewJSObjectByConstructor(objFun);
    JSHandle<JSTaggedValue> cjsRequirePrototypeValue(cjsRequirePrototype);

    // CjsExports.prototype_or_hclass
    JSHandle<JSHClass> cjsRequireHClass =
        factory_->NewEcmaHClass(CjsRequire::SIZE, JSType::JS_CJS_REQUIRE, cjsRequirePrototypeValue);

    // CjsExports.prototype.Constructor
    JSHandle<JSFunction> cjsRequireFunction =
        NewBuiltinCjsCtor(env, cjsRequirePrototype, BuiltinsCjsRequire::CjsRequireConstructor, "require",
                          FunctionLength::ONE);
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread_,
                                                     JSHandle<JSFunction>(cjsRequireFunction),
                                                     cjsRequireHClass.GetTaggedValue());

    // CjsModule.prototype method
    SetFunction(env, cjsRequirePrototype, "Main", BuiltinsCjsRequire::Main, FunctionLength::ONE);

    env->SetCjsRequireFunction(thread_, cjsRequireFunction);
}

void Builtins::InitializeDefaultExportOfScript(const JSHandle<GlobalEnv> &env) const
{
    JSHandle<JSFunction> builtinObj(env->GetObjectFunction());
    JSHandle<JSTaggedValue> emptyObj(factory_->NewJSObjectByConstructor(builtinObj));
    JSHandle<JSTaggedValue> defaultKey(factory_->NewFromUtf8ReadOnly("default"));

    JSHandle<TaggedArray> props(factory_->NewTaggedArray(2)); // 2 : two propertise
    props->Set(thread_, 0, defaultKey);
    props->Set(thread_, 1, emptyObj);
    JSHandle<JSHClass> hclass = factory_->CreateObjectClass(props, 1);
    JSHandle<JSObject> obj = factory_->NewJSObject(hclass);
    obj->SetPropertyInlinedProps(thread_, 0, props->Get(1));
    env->SetExportOfScript(thread_, obj);
    return;
}

JSHandle<JSTaggedValue> Builtins::CreateArrayUnscopables(JSThread *thread) const
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    JSHandle<JSObject> unscopableList = factory->CreateNullJSObject();

    JSHandle<JSTaggedValue> trueVal(thread, JSTaggedValue::True());

    JSHandle<JSTaggedValue> atKey((factory->NewFromASCIIReadOnly("at")));
    JSObject::CreateDataProperty(thread, unscopableList, atKey, trueVal);

    JSHandle<JSTaggedValue> copyWithKey = globalConst->GetHandledCopyWithinString();
    JSObject::CreateDataProperty(thread, unscopableList, copyWithKey, trueVal);

    JSHandle<JSTaggedValue> entriesKey = globalConst->GetHandledEntriesString();
    JSObject::CreateDataProperty(thread, unscopableList, entriesKey, trueVal);

    JSHandle<JSTaggedValue> fillKey = globalConst->GetHandledFillString();
    JSObject::CreateDataProperty(thread, unscopableList, fillKey, trueVal);

    JSHandle<JSTaggedValue> findKey = globalConst->GetHandledFindString();
    JSObject::CreateDataProperty(thread, unscopableList, findKey, trueVal);

    JSHandle<JSTaggedValue> findIndexKey = globalConst->GetHandledFindIndexString();
    JSObject::CreateDataProperty(thread, unscopableList, findIndexKey, trueVal);

    JSHandle<JSTaggedValue> findLastKey((factory->NewFromASCIIReadOnly("findLast")));
    JSObject::CreateDataProperty(thread, unscopableList, findLastKey, trueVal);

    JSHandle<JSTaggedValue> findLastIndexKey((factory->NewFromASCIIReadOnly("findLastIndex")));
    JSObject::CreateDataProperty(thread, unscopableList, findLastIndexKey, trueVal);

    JSHandle<JSTaggedValue> flatKey = globalConst->GetHandledFlatString();
    JSObject::CreateDataProperty(thread, unscopableList, flatKey, trueVal);

    JSHandle<JSTaggedValue> flatMapKey = globalConst->GetHandledFlatMapString();
    JSObject::CreateDataProperty(thread, unscopableList, flatMapKey, trueVal);

    JSHandle<JSTaggedValue> includesKey = globalConst->GetHandledIncludesString();
    JSObject::CreateDataProperty(thread, unscopableList, includesKey, trueVal);

    JSHandle<JSTaggedValue> keysKey = globalConst->GetHandledKeysString();
    JSObject::CreateDataProperty(thread, unscopableList, keysKey, trueVal);

    JSHandle<JSTaggedValue> valuesKey = globalConst->GetHandledValuesString();
    JSObject::CreateDataProperty(thread, unscopableList, valuesKey, trueVal);

    JSHandle<JSTaggedValue> toReversedKey((factory->NewFromASCIIReadOnly("toReversed")));
    JSObject::CreateDataProperty(thread, unscopableList, toReversedKey, trueVal);

    JSHandle<JSTaggedValue> toSortedKey((factory->NewFromASCIIReadOnly("toSorted")));
    JSObject::CreateDataProperty(thread, unscopableList, toSortedKey, trueVal);

    JSHandle<JSTaggedValue> toSplicedKey((factory->NewFromASCIIReadOnly("toSpliced")));
    JSObject::CreateDataProperty(thread, unscopableList, toSplicedKey, trueVal);
    return JSHandle<JSTaggedValue>::Cast(unscopableList);
}

void Builtins::RegisterSendableContainers(const JSHandle<GlobalEnv> &env) const
{
    auto globalObject = JSHandle<JSObject>::Cast(env->GetJSGlobalObject());
    {
        JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly("SendableMap"));
        PropertyDescriptor desc(thread_, env->GetSBuiltininMapFunction(), true, false, true);
        JSObject::DefineOwnProperty(thread_, globalObject, nameString, desc);
    }
    {
        JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly("SendableSet"));
        PropertyDescriptor desc(thread_, env->GetSBuiltininSetFunction(), true, false, true);
        JSObject::DefineOwnProperty(thread_, globalObject, nameString, desc);
    }
    {
        JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly("SendableArray"));
        PropertyDescriptor desc(thread_, env->GetSharedArrayFunction(), true, false, true);
        JSObject::DefineOwnProperty(thread_, globalObject, nameString, desc);
    }
    {
        JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly("SendableArrayBuffer"));
        PropertyDescriptor desc(thread_, env->GetSBuiltininArrayBufferFunction(), true, false, true);
        JSObject::DefineOwnProperty(thread_, globalObject, nameString, desc);
    }
// todo: remove sendableName when refactor
#define REGISTER_BUILTIN_SHARED_TYPED_ARRAY(Type, ctorName, TYPE, bytesPerElement, sendableName)           \
    {                                                                                        \
        JSHandle<JSTaggedValue> nameString(factory_->NewFromUtf8ReadOnly(#sendableName));    \
        PropertyDescriptor desc(thread_, env->Get##ctorName##Function(), true, false, true); \
        JSObject::DefineOwnProperty(thread_, globalObject, nameString, desc);                \
    }
    BUILTIN_SHARED_TYPED_ARRAY_TYPES(REGISTER_BUILTIN_SHARED_TYPED_ARRAY)
#undef REGISTER_BUILTIN_SHARED_TYPED_ARRAY
}
}  // namespace panda::ecmascript
