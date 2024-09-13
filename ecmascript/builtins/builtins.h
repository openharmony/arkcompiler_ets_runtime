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

#ifndef ECMASCRIPT_BUILTINS_H
#define ECMASCRIPT_BUILTINS_H

#include "ecmascript/builtins/builtins_lazy_callback.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
struct ErrorParameter {
    EcmaEntrypoint nativeConstructor{nullptr};
    EcmaEntrypoint nativeMethod{nullptr};
    std::string_view nativePropertyName{};
    JSType nativeJstype{JSType::INVALID};
};

enum FunctionLength : uint8_t { ZERO = 0, ONE, TWO, THREE, FOUR };

class Builtins {
public:
    Builtins() = default;
    Builtins(JSThread *thread, ObjectFactory *factory, EcmaVM *vm)
        : thread_(thread), factory_(factory), vm_(vm) {}
    ~Builtins() = default;
    NO_COPY_SEMANTIC(Builtins);
    NO_MOVE_SEMANTIC(Builtins);

    void Initialize(const JSHandle<GlobalEnv> &env, JSThread *thread, bool lazyInit = false, bool isRealm = false);
    void InitializeForSnapshot(JSThread *thread);

private:
    JSThread *thread_{nullptr};
    ObjectFactory *factory_{nullptr};
    EcmaVM *vm_{nullptr};
    SharedHeap *sHeap_{nullptr};

    JSHandle<JSFunction> NewBuiltinConstructor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                                               EcmaEntrypoint ctorFunc, std::string_view name, int length,
                                               kungfu::BuiltinsStubCSigns::ID builtinId =
                                               kungfu::BuiltinsStubCSigns::INVALID) const;

    JSHandle<JSFunction> NewBuiltinCjsCtor(const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSObject> &prototype, EcmaEntrypoint ctorFunc,
                                           std::string_view name, int length) const;

    JSHandle<JSFunction> NewFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &key,
                                     EcmaEntrypoint func, int length,
                                     kungfu::BuiltinsStubCSigns::ID builtinId =
                                     kungfu::BuiltinsStubCSigns::INVALID) const;

    void InitializePropertyDetector(const JSHandle<GlobalEnv> &env, bool lazyInit) const;

    void SetLazyAccessor(const JSHandle<JSObject> &object, const JSHandle<JSTaggedValue> &key,
        const JSHandle<AccessorData> &accessor) const;

    void InitializeCtor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                        const JSHandle<JSFunction> &ctor, std::string_view name, int length) const;

    void InitializeGlobalObject(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject);

    void InitializeFunction(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> &objFuncPrototypeVal) const;

    void InitializeFunctionPrototype(const JSHandle<GlobalEnv> &env, JSHandle<JSFunction> &funcFuncPrototype,
        JSHandle<JSFunction> &funcFunc) const;

    JSHandle<JSHClass> CreateFunctionHClass(const JSHandle<JSFunction> &funcPrototype) const;

    JSHandle<JSHClass> CreateFunctionPrototypeHClass(const JSHandle<GlobalEnv> &env,
                                                     const JSHandle<JSTaggedValue> &ObjPrototypeVal) const;

    void InitializeObject(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &objFuncPrototype,
                          const JSHandle<JSObject> &objFunc);

    void InitializeNumber(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject,
                          const JSHandle<JSHClass> &primRefObjClass);

    void InitializeBigInt(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &primRefObjClass) const;

    void InitializeBigIntWithRealm(const JSHandle<GlobalEnv> &realm) const;

    void InitializeDate(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const;
    void LazyInitializeDate(const JSHandle<GlobalEnv> &env) const;

    void InitializeBoolean(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &primRefObjClass) const;

    void InitializeSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeSymbolWithRealm(const JSHandle<GlobalEnv> &realm, const JSHandle<JSHClass> &objFuncInstanceHClass);

    void InitializeArray(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const;

    JSHandle<JSObject> InitializeArrayPrototype(JSHandle<JSHClass> &arrBaseFuncInstanceHClass) const;
    
    JSHandle<JSHClass> InitializeArrayPrototypeHClass(const JSHandle<JSObject> &arrFuncPrototype) const;

    void InitializeSharedArray(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &sObjIHClass,
                               JSHandle<JSFunction> &sFuncPrototype) const;

    void InitializeTypedArray(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const;
    void LazyInitializeTypedArray(const JSHandle<GlobalEnv> &env) const;

    void InitializeInt8Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeInt8Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeUint8Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeUint8Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeUint8ClampedArray(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeUint8ClampedArray(const JSHandle<GlobalEnv> &env) const;

    void InitializeInt16Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeInt16Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeUint16Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeUint16Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeInt32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeInt32Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeUint32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeUint32Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeFloat32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeFloat32Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeFloat64Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeFloat64Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeBigInt64Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeBigInt64Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeBigUint64Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeBigUint64Array(const JSHandle<GlobalEnv> &env) const;

    void InitializeAllTypeError(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeAllTypeErrorWithRealm(const JSHandle<GlobalEnv> &realm) const;

    void InitializeError(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass,
                         const JSType &errorTag) const;

    void SetErrorWithRealm(const JSHandle<GlobalEnv> &realm, const JSType &errorTag) const;

    void InitializeRegExp(const JSHandle<GlobalEnv> &env);

    // for Intl.
    JSHandle<JSFunction> NewIntlConstructor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                                            EcmaEntrypoint ctorFunc, std::string_view name, int length);
    void InitializeIntlCtor(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &prototype,
                            const JSHandle<JSFunction> &ctor, std::string_view name, int length);
    void InitializeIntl(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeValue);
    void InitializeLocale(const JSHandle<GlobalEnv> &env);
    void InitializeDateTimeFormat(const JSHandle<GlobalEnv> &env);
    void InitializeRelativeTimeFormat(const JSHandle<GlobalEnv> &env);
    void InitializeNumberFormat(const JSHandle<GlobalEnv> &env);
    void InitializeCollator(const JSHandle<GlobalEnv> &env);
    void InitializePluralRules(const JSHandle<GlobalEnv> &env);
    void InitializeDisplayNames(const JSHandle<GlobalEnv> &env);
    void InitializeListFormat(const JSHandle<GlobalEnv> &env);
    void InitializeSegmenter(const JSHandle<GlobalEnv> &env);
    void InitializeSegments(const JSHandle<GlobalEnv> &env);

    void LazyInitializeLocale(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializeDateTimeFormat(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializeNumberFormat(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializeRelativeTimeFormat(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializeCollator(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializePluralRules(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializeDisplayNames(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializeListFormat(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializeSegmenter(const JSHandle<GlobalEnv> &env) const;
    void LazyInitializeSegments(const JSHandle<GlobalEnv> &env) const;

    void GeneralUpdateError(ErrorParameter *error, EcmaEntrypoint constructor, EcmaEntrypoint method,
                            std::string_view name, JSType type) const;

    void InitializeSet(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const;
    void LazyInitializeSet(const JSHandle<GlobalEnv> &env);

    void InitializeMap(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const;
    void LazyInitializeMap(const JSHandle<GlobalEnv> &env) const;

    void InitializeWeakMap(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeWeakMap(const JSHandle<GlobalEnv> &env) const;

    void InitializeWeakSet(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeWeakSet(const JSHandle<GlobalEnv> &env) const;

    void InitializeWeakRef(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeWeakRef(const JSHandle<GlobalEnv> &env) const;

    void InitializeFinalizationRegistry(const JSHandle<GlobalEnv> &env,
                                        const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeFinalizationRegistry(const JSHandle<GlobalEnv> &env) const;

    void InitializeMath(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const;

    void InitializeAtomics(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const;

    void InitializeJson(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const;

    void InitializeSendableJson(const JSHandle<GlobalEnv> &env,
                                const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const;

    void InitializeString(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const;

    void InitializeIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeRegexpIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;

    void InitializeStringIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;

    void InitializeSegmentIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;

    void InitializeAsyncFromSyncIterator(const JSHandle<GlobalEnv> &env,
                                         const JSHandle<JSHClass> &iteratorFuncClass) const;

    void InitializeForinIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;

    void InitializeMapIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;
    void InitializeSMapIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;

    void InitializeSetIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;
    void InitializeSSetIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;

    void InitializeArrayIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass,
                                 const JSHandle<JSHClass> &iteratorPrototypeClass) const;
    void InitializeSArrayIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &iteratorFuncClass) const;

    void InitializeArrayBuffer(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeArrayBuffer(const JSHandle<GlobalEnv> &env) const;

    void InitializeSharedArrayBuffer(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void LazyInitializeSharedArrayBuffer(const JSHandle<GlobalEnv> &env) const;

    void InitializeDataView(const JSHandle<GlobalEnv> &env, JSHandle<JSTaggedValue> objFuncPrototypeVal) const;
    void LazyInitializeDataView(const JSHandle<GlobalEnv> &env) const;

    void InitializeForPromiseFuncClass(const JSHandle<GlobalEnv> &env);

    void InitializeProxy(const JSHandle<GlobalEnv> &env);

    void InitializeReflect(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &objFuncPrototypeVal) const;

    void InitializeAsyncFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeGeneratorFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeAsyncGeneratorFunction(const JSHandle<GlobalEnv> &env,
                                          const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeAsyncGenerator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeAsyncIterator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeGenerator(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    JSHandle<JSFunction> InitializeExoticConstructor(const JSHandle<GlobalEnv> &env, EcmaEntrypoint ctorFunc,
                                                     std::string_view name, int length);

    void InitializePromise(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &promiseFuncClass);

    void InitializePromiseJob(const JSHandle<GlobalEnv> &env);

    void InitializeModuleNamespace(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeSModuleNamespace(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &sObjIHClass) const;

    void InitializeNativeModuleFailureInfo(const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSHClass> &objFuncClass) const;

    void InitializeCjsModule(const JSHandle<GlobalEnv> &env) const;

    void InitializeCjsExports(const JSHandle<GlobalEnv> &env) const;

    void InitializeCjsRequire(const JSHandle<GlobalEnv> &env) const;

    void InitializeDefaultExportOfScript(const JSHandle<GlobalEnv> &env) const;

    void SetFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, std::string_view key,
                     EcmaEntrypoint func, int length, kungfu::BuiltinsStubCSigns::ID builtinId =
                     kungfu::BuiltinsStubCSigns::INVALID) const;

    void SetFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                     EcmaEntrypoint func, int length, kungfu::BuiltinsStubCSigns::ID builtinId =
                     kungfu::BuiltinsStubCSigns::INVALID) const;

    JSHandle<JSFunction> SetAndReturnFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                                              const char *key, EcmaEntrypoint func, int length,
                                              kungfu::BuiltinsStubCSigns::ID builtinId =
                                              kungfu::BuiltinsStubCSigns::INVALID) const ;

    JSHandle<JSFunction> SetAndReturnFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                                              const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func, int length,
                                              kungfu::BuiltinsStubCSigns::ID builtinId =
                                              kungfu::BuiltinsStubCSigns::INVALID) const;

    void SetFuncToObjAndGlobal(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &globalObject,
                               const JSHandle<JSObject> &obj, std::string_view key, EcmaEntrypoint func, int length,
                               kungfu::BuiltinsStubCSigns::ID builtinId = kungfu::BuiltinsStubCSigns::INVALID);

    template<int type = JSSymbol::SYMBOL_DEFAULT_TYPE>
    void SetFunctionAtSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                             const JSHandle<JSTaggedValue> &symbol, std::string_view name, EcmaEntrypoint func,
                             int length) const;

    template<int type = JSSymbol::SYMBOL_DEFAULT_TYPE>
    JSHandle<JSTaggedValue> SetAndReturnFunctionAtSymbol(const JSHandle<GlobalEnv> &env,
                                                         const JSHandle<JSObject> &obj,
                                                         const JSHandle<JSTaggedValue> &symbol,
                                                         std::string_view name,
                                                         EcmaEntrypoint func,
                                                         int length,
                                                         kungfu::BuiltinsStubCSigns::ID builtinId =
                                                         kungfu::BuiltinsStubCSigns::INVALID) const;

    void SetStringTagSymbol(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                            std::string_view key) const;

    JSHandle<JSTaggedValue> CreateGetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                         std::string_view name, int length,
                                         kungfu::BuiltinsStubCSigns::ID builtinId =
                                         kungfu::BuiltinsStubCSigns::INVALID) const;

    JSHandle<JSTaggedValue> CreateGetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                         JSHandle<JSTaggedValue> key, int length,
                                         kungfu::BuiltinsStubCSigns::ID builtinId =
                                         kungfu::BuiltinsStubCSigns::INVALID) const;

    void SetConstant(const JSHandle<JSObject> &obj, std::string_view key, JSTaggedValue value) const;

    void SetGlobalThis(const JSHandle<JSObject> &obj, std::string_view key,
                       const JSHandle<JSTaggedValue> &globalValue);

    void SetAttribute(const JSHandle<JSObject> &obj, std::string_view key,  std::string_view value) const;

    void SetNoneAttributeProperty(const JSHandle<JSObject> &obj, std::string_view key,
                                  const JSHandle<JSTaggedValue> &value) const;

    void StrictModeForbiddenAccessCallerArguments(const JSHandle<GlobalEnv> &env, uint32_t &index,
                                                  const JSHandle<JSObject> &prototype) const;
    void SetInlineAccessor(const JSHandle<JSObject> &obj, uint32_t index,
                           const JSHandle<JSTaggedValue> &getter, const JSHandle<JSTaggedValue> &setter) const;

    void SetInlineFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                           const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func, uint32_t index, int length,
                           kungfu::BuiltinsStubCSigns::ID builtinId = kungfu::BuiltinsStubCSigns::INVALID) const;

    void SetInlineFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, std::string_view key,
                           EcmaEntrypoint func, uint32_t index, int length,
                           kungfu::BuiltinsStubCSigns::ID builtinId = kungfu::BuiltinsStubCSigns::INVALID) const;

    JSHandle<JSFunction> SetInlineFunctionAndRetJSFunction(const JSHandle<GlobalEnv> &env,
        const JSHandle<JSObject> &obj, std::string_view key,
        EcmaEntrypoint func, uint32_t index, int length,
        kungfu::BuiltinsStubCSigns::ID builtinId) const;
    
    void SetFunctionPrototype(const JSHandle<JSFunction> &ctor, const JSTaggedValue &prototype) const;

    void SetFunctionLength(const JSHandle<JSFunction> &ctor, int length) const;

    void SetFunctionName(const JSHandle<JSFunction> &ctor, std::string_view name) const;

    void SetFunctionName(const JSHandle<JSFunction> &ctor, const JSHandle<JSTaggedValue> &name) const;

    JSHandle<JSTaggedValue> CreateSetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                         std::string_view name, int length) const;
    JSHandle<JSTaggedValue> CreateSetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                         JSHandle<JSTaggedValue> key, int length) const;

    void SetArgumentsSharedAccessor(const JSHandle<GlobalEnv> &env);
    void SetAccessor(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                     const JSHandle<JSTaggedValue> &getter, const JSHandle<JSTaggedValue> &setter) const;
    void SetGetter(const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                   const JSHandle<JSTaggedValue> &getter) const;
    JSHandle<JSObject> InitializeArkTools(const JSHandle<GlobalEnv> &env) const;
    JSHandle<JSObject> InitializeGcBuiltins(const JSHandle<GlobalEnv> &env) const;
    void InitializeGlobalRegExp(JSHandle<JSObject> &obj) const;
    // Using to initialize jsapi container
    JSHandle<JSObject> InitializeArkPrivate(const JSHandle<GlobalEnv> &env) const;
    void SetConstantObject(const JSHandle<JSObject> &obj, std::string_view key,
                           JSHandle<JSTaggedValue> &value) const;
    void SetFrozenFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, std::string_view key,
                           EcmaEntrypoint func, int length) const;
    void SetNonConstantObject(const JSHandle<JSObject> &obj, std::string_view key,
                              JSHandle<JSTaggedValue> &value) const;
    void RegisterSendableContainers(const JSHandle<GlobalEnv> &env) const;

    // For SharedObject/SharedFunction
    void InitializeSObjectAndSFunction(const JSHandle<GlobalEnv> &env) const;
    void CopySObjectAndSFunction(const JSHandle<GlobalEnv> &env, const JSTaggedValue &srcEnv) const;
    void InitializeSObject(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &sObjIHClass,
                           const JSHandle<JSObject> &sObjPrototype,
                           const JSHandle<JSFunction> &sFuncPrototype) const;
    void InitializeSFunction(const JSHandle<GlobalEnv> &env,
                             const JSHandle<JSFunction> &sFuncPrototype) const;
    void InitializeSAsyncFunction(const JSHandle<GlobalEnv> &env,
                                  const JSHandle<JSHClass> &sObjIHClass) const;
    void InitializeSArrayBuffer(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &objFuncClass,
                                const JSHandle<JSFunction> &sFuncPrototype) const;
    void InitializeSSet(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &sObjPrototype,
                        const JSHandle<JSFunction> &sFuncPrototype) const;
    void InitializeSMap(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &sObjPrototype,
                        const JSHandle<JSFunction> &sFuncPrototype) const;
    void InitializeSTypedArray(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &sObjPrototype,
                               const JSHandle<JSFunction> &sFuncPrototype) const;
    void InitializeSInt8Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSUint8Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSUint8ClampedArray(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSInt16Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSUint16Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSInt32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSUint32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSFloat32Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSFloat64Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSBigInt64Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;
    void InitializeSBigUint64Array(const JSHandle<GlobalEnv> &env, const JSHandle<JSHClass> &objFuncClass) const;

    JSHandle<JSHClass> CreateSObjectFunctionHClass(const JSHandle<JSFunction> &sFuncPrototype) const;
    JSHandle<JSHClass> CreateSObjectPrototypeHClass() const;
    JSHandle<JSHClass> CreateSFunctionHClass(const JSHandle<JSFunction> &sFuncPrototype) const;
    JSHandle<JSHClass> CreateSFunctionPrototypeHClass(const JSHandle<JSTaggedValue> &sObjPrototypeVal) const;
    JSHandle<JSHClass> CreateSSetPrototypeHClass(const JSHandle<JSObject> &sObjPrototype) const;
    JSHandle<JSHClass> CreateSSetFunctionHClass(const JSHandle<JSFunction> &sFuncPrototype) const;
    JSHandle<JSHClass> CreateSMapPrototypeHClass(const JSHandle<JSObject> &sObjPrototype) const;
    JSHandle<JSHClass> CreateSMapFunctionHClass(const JSHandle<JSFunction> &sFuncPrototype) const;
    JSHandle<JSHClass> CreateSArrayPrototypeHClass(const JSHandle<JSObject> &sObjPrototype) const;
    JSHandle<JSHClass> CreateSArrayFunctionHClass(const JSHandle<JSFunction> &sFuncPrototype) const;
    JSHandle<JSHClass> CreateSTypedArrayPrototypeHClass(const JSHandle<JSObject> &sObjPrototype) const;
    JSHandle<JSHClass> CreateSTypedArrayFunctionHClass(const JSHandle<JSFunction> &sFuncPrototype) const;
    JSHandle<JSHClass> CreateSSpecificTypedArrayFuncHClass(const JSHandle<JSFunction> &sFuncPrototype) const;
    JSHandle<JSHClass> CreateSSpecificTypedArrayInstanceHClass(const JSHandle<JSObject> &sObjPrototype) const;
    JSHandle<JSHClass> CreateSArrayBufferPrototypeHClass(const JSHandle<JSObject> &sObjPrototype) const;
    JSHandle<JSHClass> CreateSArrayBufferFunctionHClass(const JSHandle<JSFunction> &sFuncPrototype) const;

    void InitializeSCtor(const JSHandle<JSHClass> &protoHClass, const JSHandle<JSFunction> &ctor,
                         std::string_view name, int length) const;

    void SetSFunctionName(const JSHandle<JSFunction> &ctor, std::string_view name) const;
    void SetSFunctionName(const JSHandle<JSFunction> &ctor, const JSHandle<JSTaggedValue> &name) const;
    void SetSFunctionLength(const JSHandle<JSFunction> &ctor, int length) const;
    void SetSFunctionPrototype(const JSHandle<JSFunction> &ctor, const JSTaggedValue &prototype) const;

    JSHandle<JSFunction> NewSFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSTaggedValue> &key,
                                      EcmaEntrypoint func, int length,
                                      kungfu::BuiltinsStubCSigns::ID builtinId =
                                      kungfu::BuiltinsStubCSigns::INVALID) const;

    void SetSFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj, std::string_view key,
                      EcmaEntrypoint func, uint32_t index, int length, kungfu::BuiltinsStubCSigns::ID builtinId =
                      kungfu::BuiltinsStubCSigns::INVALID) const;

    void SetSFunction(const JSHandle<GlobalEnv> &env, const JSHandle<JSObject> &obj,
                      const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func, uint32_t index, int length,
                      kungfu::BuiltinsStubCSigns::ID builtinId = kungfu::BuiltinsStubCSigns::INVALID) const;
    void SetSAccessor(const JSHandle<JSObject> &obj, uint32_t index,
                      const JSHandle<JSTaggedValue> &getter, const JSHandle<JSTaggedValue> &setter) const;

    JSHandle<JSTaggedValue> CreateSGetterSetter(const JSHandle<GlobalEnv> &env, EcmaEntrypoint func,
                                                std::string_view name, int length) const;
    void SharedStrictModeForbiddenAccessCallerArguments(const JSHandle<GlobalEnv> &env, uint32_t &index,
                                                        const JSHandle<JSObject> &prototype) const;
    JSHandle<JSTaggedValue> CreateArrayUnscopables(JSThread *thread) const;
    void InitializeSSymbolAttributes(const JSHandle<GlobalEnv> &env);
    friend class builtins::BuiltinsLazyCallback;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_BUILTINS_H
