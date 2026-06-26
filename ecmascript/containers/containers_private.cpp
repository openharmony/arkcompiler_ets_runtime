/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/containers/containers_private.h"

#include "containers_arraylist.h"
#include "containers_buffer.h"
#include "containers_deque.h"
#include "containers_hashmap.h"
#include "containers_hashset.h"
#include "containers_lightweightmap.h"
#include "containers_lightweightset.h"
#include "containers_linked_list.h"
#include "containers_list.h"
#include "containers_plainarray.h"
#include "containers_queue.h"
#include "containers_stack.h"
#include "containers_treemap.h"
#include "containers_treeset.h"
#include "containers_vector.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_bitvector_iterator.h"
#include "ecmascript/js_api/js_api_buffer.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightset_iterator.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_api/js_api_plain_array_iterator.h"
#include "ecmascript/js_api/js_api_queue_iterator.h"
#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/js_api/js_api_tree_map_iterator.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_set_iterator.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/object_fast_operator-inl.h"
#include <numeric>

namespace panda::ecmascript::containers {
JSTaggedValue ContainersPrivate::Load(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg != nullptr);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> argv = GetCallArg(msg, 0);
    JSHandle<JSObject> thisValue(GetThis(msg));

    uint32_t tag = 0;
    if (!JSTaggedValue::ToElementIndex(thread, argv.GetTaggedValue(), &tag) || tag >= ContainerTag::END) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Incorrect input parameters", JSTaggedValue::Exception());
    }

    // Lazy set an undefinedIteratorResult to global constants
    JSHandle env = thread->GetEcmaVM()->GetGlobalEnv();
    if (!env->GetUndefinedIteratorResult()->IsUndefined()) {
        auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
        JSHandle<JSTaggedValue> undefinedHandle = globalConst->GetHandledUndefined();
        JSHandle<JSObject> undefinedIteratorResult = JSIterator::CreateIterResultObject(thread, undefinedHandle, true);
        env->SetUndefinedIteratorResult(thread, undefinedIteratorResult);
    }

    JSTaggedValue res = JSTaggedValue::Undefined();
    switch (tag) {
        case ContainerTag::Deque: {
            res = InitializeContainer(thread, thisValue, InitializeDeque, "DequeConstructor");
            break;
        }
        case ContainerTag::LightWeightMap: {
            res = InitializeContainer(thread, thisValue, InitializeLightWeightMap, "LightWeightMapConstructor");
            break;
        }
        case ContainerTag::LightWeightSet: {
            res = InitializeContainer(thread, thisValue, InitializeLightWeightSet, "LightWeightSetConstructor");
            break;
        }
        case ContainerTag::PlainArray: {
            res = InitializeContainer(thread, thisValue, InitializePlainArray, "PlainArrayConstructor");
            break;
        }
        case ContainerTag::Queue: {
            res = InitializeContainer(thread, thisValue, InitializeQueue, "QueueConstructor");
            break;
        }
        case ContainerTag::Stack: {
            res = InitializeContainer(thread, thisValue, InitializeStack, "StackConstructor");
            break;
        }
        case ContainerTag::TreeMap: {
            res = InitializeContainer(thread, thisValue, InitializeTreeMap, "TreeMapConstructor");
            break;
        }
        case ContainerTag::Vector: {
            res = InitializeContainer(thread, thisValue, InitializeVector, "VectorConstructor");
            break;
        }
        case ContainerTag::BitVector: {
            res = InitializeContainer(thread, thisValue, InitializeBitVector, "BitVectorConstructor");
            break;
        }
        case ContainerTag::FastBuffer: {
            res = InitializeContainer(thread, thisValue, InitializeBuffer, "BufferConstructor");
            break;
        }
        case ContainerTag::ArrayList: {
            res = env->GetArrayListConstructor().GetTaggedValue();
            break;
        }
        case ContainerTag::TreeSet: {
            res = env->GetTreeSetConstructor().GetTaggedValue();
            break;
        }
        case ContainerTag::List: {
            res = env->GetListConstructor().GetTaggedValue();
            break;
        }
        case ContainerTag::LinkedList: {
            res = env->GetLinkedListConstructor().GetTaggedValue();
            break;
        }
        case ContainerTag::HashMap: {
            res = env->GetHashMapConstructor().GetTaggedValue();
            break;
        }
        case ContainerTag::HashSet: {
            res = env->GetHashSetConstructor().GetTaggedValue();
            break;
        }
        case ContainerTag::END:
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    return res;
}

JSTaggedValue ContainersPrivate::InitializeContainer(JSThread *thread, const JSHandle<JSObject> &obj,
                                                     InitializeFunction func, const char *name)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII(name));
    JSTaggedValue value = ObjectFastOperator::GetPropertyByName<ObjectFastOperator::Status::UseOwn>
        (thread, obj.GetTaggedValue(), key.GetTaggedValue());
    if (!value.IsUndefined()) {
        return value;
    }
    JSHandle<JSTaggedValue> map = func(thread);
    SetFrozenConstructor(thread, obj, name, map);
    return map.GetTaggedValue();
}

JSHandle<JSFunction> ContainersPrivate::NewContainerConstructor(const JSHandle<GlobalEnv> &env,
                                                                const JSHandle<JSObject> &prototype,
                                                                EcmaEntrypoint ctorFunc, const char *name, int length)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> ctor =
        factory->NewJSBuiltinFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_CONSTRUCTOR);

    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSFunction::SetFunctionLength(thread, ctor, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory->NewFromASCII(name));
    JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(ctor), nameString,
                                globalConst->GetHandledUndefined());
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    PropertyDescriptor descriptor1(thread, JSHandle<JSTaggedValue>::Cast(ctor), true, false, true);
    JSObject::DefineOwnProperty(thread, prototype, constructorKey, descriptor1);

    /* set "prototype" in constructor */
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread, ctor, prototype.GetTaggedValue());

    return ctor;
}

void ContainersPrivate::SetFrozenFunction(const JSHandle<GlobalEnv> &env,
                                          const JSHandle<JSObject> &obj, const char *key,
                                          EcmaEntrypoint func, int length, kungfu::BuiltinsStubCSigns::ID builtinId)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> keyString(factory->NewFromASCII(key));
    JSHandle<JSFunction> function = NewFunction(env, keyString, func, length, builtinId);
    PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>(function), false, false, false);
    JSObject::DefineOwnProperty(thread, obj, keyString, descriptor);
}

void ContainersPrivate::SetFrozenConstructor(JSThread *thread, const JSHandle<JSObject> &obj, const char *keyChar,
                                             JSHandle<JSTaggedValue> &value)
{
    if (value->IsECMAObject()) {
        JSObject::PreventExtensions(thread, JSHandle<JSObject>::Cast(value));
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyChar));
    PropertyDescriptor descriptor(thread, value, false, false, false);
    JSObject::DefineOwnProperty(thread, obj, key, descriptor);
}

JSHandle<JSFunction> ContainersPrivate::NewFunction(const JSHandle<GlobalEnv> &env,
                                                    const JSHandle<JSTaggedValue> &key,
                                                    EcmaEntrypoint func, int length,
                                                    kungfu::BuiltinsStubCSigns::ID builtinId)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function =
        factory->NewJSBuiltinFunction(env, reinterpret_cast<void *>(func),
                                      FunctionKind::NORMAL_FUNCTION, builtinId);
    JSFunction::SetFunctionLength(thread, function, JSTaggedValue(length));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSFunction::SetFunctionName(thread, baseFunction, key, thread->GlobalConstants()->GetHandledUndefined());
    return function;
}

JSHandle<JSTaggedValue> ContainersPrivate::CreateGetter(const JSHandle<GlobalEnv> &env,
                                                        EcmaEntrypoint func, const char *name, int length)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function = factory->NewJSBuiltinFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> funcName(factory->NewFromASCII(name));
    JSHandle<JSTaggedValue> prefix = thread->GlobalConstants()->GetHandledGetString();
    JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(function), funcName, prefix);
    return JSHandle<JSTaggedValue>(function);
}

void ContainersPrivate::SetGetter(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                  const JSHandle<JSTaggedValue> &getter)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<AccessorData> accessor = factory->NewAccessorData();
    accessor->SetGetter(thread, getter);
    PropertyAttributes attr = PropertyAttributes::DefaultAccessor(false, false, false);
    JSObject::AddAccessor(thread, JSHandle<JSTaggedValue>::Cast(obj), key, accessor, attr);
}

void ContainersPrivate::SetFunctionAtSymbol(const JSHandle<GlobalEnv> &env,
                                            const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &symbol,
                                            const char *name, EcmaEntrypoint func, int length)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function = factory->NewJSBuiltinFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory->NewFromASCII(name));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSFunction::SetFunctionName(thread, baseFunction, nameString, thread->GlobalConstants()->GetHandledUndefined());
    PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>::Cast(function), false, false, false);
    JSObject::DefineOwnProperty(thread, obj, symbol, descriptor);
}

void ContainersPrivate::SetStringTagSymbol(const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSObject> &obj, const char *key)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> tag(factory->NewFromASCII(key));
    JSHandle<JSTaggedValue> symbol = thread->GlobalConstants()->GetHandledToStringTagSymbol();
    PropertyDescriptor desc(thread, tag, false, false, false);
    JSObject::DefineOwnProperty(thread, obj, symbol, desc);
}

void ContainersPrivate::InitializeArrayListIterator(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Iterator.hclass
    JSHandle<JSHClass> iteratorFuncHClass =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_ITERATOR, env->GetIteratorPrototype());
    // ArrayListIterator.prototype
    JSHandle<JSObject> arrayListIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // Iterator.prototype.next()
    SetFrozenFunction(env, arrayListIteratorPrototype, "next", JSAPIArrayListIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, arrayListIteratorPrototype, "ArrayList Iterator");
    env->SetArrayListIteratorPrototype(thread, arrayListIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeLightWeightMap(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> funcPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> mapFuncPrototypeValue(funcPrototype);
    JSHandle<JSHClass> lightWeightMapInstanceClass =
        factory->NewEcmaHClass(JSAPILightWeightMap::SIZE, JSType::JS_API_LIGHT_WEIGHT_MAP, mapFuncPrototypeValue);
    JSHandle<JSTaggedValue> lightWeightMapFunction(NewContainerConstructor(
        env, funcPrototype, ContainersLightWeightMap::LightWeightMapConstructor, "LightWeightMap",
        FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(lightWeightMapFunction), lightWeightMapInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(funcPrototype), constructorKey, lightWeightMapFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // LightWeightMap.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersLightWeightMap::GetLightWeightMapPrototypeFunctions()) {
        SetFrozenFunction(env, funcPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env, ContainersLightWeightMap::Length, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    SetGetter(thread, funcPrototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(env, funcPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersLightWeightMap::Entries, FuncLength::ONE);

    ContainersPrivate::InitializeLightWeightMapIterator(thread);
    return lightWeightMapFunction;
}

void ContainersPrivate::InitializeLightWeightMapIterator(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorClass = factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_ITERATOR,
                                                              env->GetIteratorPrototype());
    JSHandle<JSObject> lightWeightMapIteratorPrototype(factory->NewJSObject(iteratorClass));
    SetFrozenFunction(env, lightWeightMapIteratorPrototype, "next", JSAPILightWeightMapIterator::Next,
                      FuncLength::ONE);
    SetStringTagSymbol(env, lightWeightMapIteratorPrototype, "LightWeightMap Iterator");
    env->SetLightWeightMapIteratorPrototype(thread, lightWeightMapIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeLightWeightSet(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // LightWeightSet.prototype
    JSHandle<JSObject> funcPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> funcPrototypeValue(funcPrototype);
    // LightWeightSet.prototype_or_hclass
    JSHandle<JSHClass> lightweightSetInstanceClass =
        factory->NewEcmaHClass(JSAPILightWeightSet::SIZE, JSType::JS_API_LIGHT_WEIGHT_SET, funcPrototypeValue);
    JSHandle<JSTaggedValue> lightweightSetFunction(
        NewContainerConstructor(env, funcPrototype, ContainersLightWeightSet::LightWeightSetConstructor,
                                "LightWeightSet", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(lightweightSetFunction), lightweightSetInstanceClass.GetTaggedValue());
    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(funcPrototype), constructorKey, lightweightSetFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // LightWeightSet.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersLightWeightSet::GetLightWeightSetPrototypeFunctions()) {
        SetFrozenFunction(env, funcPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, ContainersLightWeightSet::GetSize, "length", FuncLength::ZERO);

    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    SetGetter(thread, funcPrototype, lengthKey, lengthGetter);
    SetFunctionAtSymbol(env, funcPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersLightWeightSet::GetIteratorObj, FuncLength::ONE);

    InitializeLightWeightSetIterator(thread);
    return lightweightSetFunction;
}

void ContainersPrivate::InitializeLightWeightSetIterator(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSHClass> iteratorClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> lightWeightSetIteratorPrototype(factory->NewJSObject(iteratorClass));
    SetFrozenFunction(
        env, lightWeightSetIteratorPrototype, "next", JSAPILightWeightSetIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, lightWeightSetIteratorPrototype, "LightWeightSet Iterator");
    env->SetLightWeightSetIteratorPrototype(thread, lightWeightSetIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeTreeMap(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // TreeMap.prototype
    JSHandle<JSObject> mapFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> mapFuncPrototypeValue(mapFuncPrototype);
    // TreeMap.prototype_or_hclass
    JSHandle<JSHClass> mapInstanceClass =
        factory->NewEcmaHClass(JSAPITreeMap::SIZE, JSType::JS_API_TREE_MAP, mapFuncPrototypeValue);
    // TreeMap() = new Function()
    JSHandle<JSTaggedValue> mapFunction(NewContainerConstructor(
        env, mapFuncPrototype, ContainersTreeMap::TreeMapConstructor, "TreeMap", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(mapFunction), mapInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(mapFuncPrototype), constructorKey, mapFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // TreeMap.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersTreeMap::GetTreeMapPrototypeFunctions()) {
        SetFrozenFunction(env, mapFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    // @@ToStringTag
    SetStringTagSymbol(env, mapFuncPrototype, "TreeMap");
    // %TreeMapPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> iteratorSymbol = thread->GlobalConstants()->GetHandledIteratorSymbol();
    JSHandle<JSTaggedValue> entries = globalConst->GetHandledEntriesString();
    JSHandle<JSTaggedValue> entriesFunc =
        JSObject::GetMethod(thread, JSHandle<JSTaggedValue>::Cast(mapFuncPrototype), entries);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    PropertyDescriptor descriptor(thread, entriesFunc, false, false, false);
    JSObject::DefineOwnProperty(thread, mapFuncPrototype, iteratorSymbol, descriptor);
    // length
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, ContainersTreeMap::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, mapFuncPrototype, lengthKey, lengthGetter);

    InitializeTreeMapIterator(thread);
    return mapFunction;
}

void ContainersPrivate::InitializeTreeMapIterator(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Iterator.hclass
    JSHandle<JSHClass> iteratorClass =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_ITERATOR, env->GetIteratorPrototype());

    // TreeMapIterator.prototype
    JSHandle<JSObject> mapIteratorPrototype(factory->NewJSObject(iteratorClass));
    // TreeIterator.prototype.next()
    SetFrozenFunction(env, mapIteratorPrototype, "next", JSAPITreeMapIterator::Next, FuncLength::ZERO);
    SetStringTagSymbol(env, mapIteratorPrototype, "TreeMap Iterator");
    env->SetTreeMapIteratorPrototype(thread, mapIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializePlainArray(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // PlainArray.prototype
    JSHandle<JSObject> plainArrayFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> plainArrayFuncPrototypeValue(plainArrayFuncPrototype);
    // PlainArray.prototype_or_hclass
    JSHandle<JSHClass> plainArrayInstanceClass =
        factory->NewEcmaHClass(JSAPIPlainArray::SIZE, JSType::JS_API_PLAIN_ARRAY, plainArrayFuncPrototypeValue);
    JSHandle<JSTaggedValue> plainArrayFunction(
        NewContainerConstructor(env, plainArrayFuncPrototype, ContainersPlainArray::PlainArrayConstructor,
                                "PlainArray", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(plainArrayFunction), plainArrayInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(plainArrayFuncPrototype), constructorKey,
                          plainArrayFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // PlainArray.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersPlainArray::GetPlainArrayPrototypeFunctions()) {
        SetFrozenFunction(env, plainArrayFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env, ContainersPlainArray::GetSize, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    SetGetter(thread, plainArrayFuncPrototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(env, plainArrayFuncPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersPlainArray::GetIteratorObj, FuncLength::ONE);
    InitializePlainArrayIterator(thread);
    env->SetPlainArrayFunction(thread, plainArrayFunction);
    return plainArrayFunction;
}

void ContainersPrivate::InitializePlainArrayIterator(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> plainarrayIteratorPrototype(factory->NewJSObject(iteratorClass));
    SetFrozenFunction(env, plainarrayIteratorPrototype, "next", JSAPIPlainArrayIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, plainarrayIteratorPrototype, "PlainArray Iterator");
    env->SetPlainArrayIteratorPrototype(thread, plainarrayIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeStack(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // Stack.prototype
    JSHandle<JSObject> stackFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> stackFuncPrototypeValue(stackFuncPrototype);
    // Stack.prototype_or_hclass
    JSHandle<JSHClass> stackInstanceClass =
        factory->NewEcmaHClass(JSAPIStack::SIZE, JSType::JS_API_STACK, stackFuncPrototypeValue);
    // Stack() = new Function()
    JSHandle<JSTaggedValue> stackFunction(NewContainerConstructor(
        env, stackFuncPrototype, ContainersStack::StackConstructor, "Stack", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(stackFunction), stackInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(stackFuncPrototype), constructorKey, stackFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // Stack.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersStack::GetStackPrototypeFunctions()) {
        SetFrozenFunction(env, stackFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    SetStringTagSymbol(env, stackFuncPrototype, "Stack");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env,
                                                        ContainersStack::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    SetGetter(thread, stackFuncPrototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(env, stackFuncPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersStack::Iterator, FuncLength::ONE);

    ContainersPrivate::InitializeStackIterator(thread);
    return stackFunction;
}

void ContainersPrivate::InitializeStackIterator(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSHClass> iteratorFuncHClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    // StackIterator.prototype
    JSHandle<JSObject> stackIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // Iterator.prototype.next()
    SetFrozenFunction(env, stackIteratorPrototype, "next", JSAPIStackIterator::Next, FuncLength::ONE);
    env->SetStackIteratorPrototype(thread, stackIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeVector(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // Vector.prototype
    JSHandle<JSObject> prototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> vectorFuncPrototypeValue(prototype);
    // Vector.prototype_or_hclass
    JSHandle<JSHClass> vectorInstanceClass =
        factory->NewEcmaHClass(JSAPIVector::SIZE, JSType::JS_API_VECTOR, vectorFuncPrototypeValue);
    // Vector() = new Function()
    JSHandle<JSTaggedValue> vectorFunction(NewContainerConstructor(
        env, prototype, ContainersVector::VectorConstructor, "Vector", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(vectorFunction), vectorInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(prototype), constructorKey, vectorFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // Vector.prototype
    SetFrozenFunction(env, prototype, "add", ContainersVector::Add, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "insert", ContainersVector::Insert, FuncLength::TWO);
    SetFrozenFunction(env, prototype, "setLength", ContainersVector::SetLength, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "getCapacity", ContainersVector::GetCapacity, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "increaseCapacityTo",
                      ContainersVector::IncreaseCapacityTo, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "get", ContainersVector::Get, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "getIndexOf", ContainersVector::GetIndexOf, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "getIndexFrom", ContainersVector::GetIndexFrom, FuncLength::TWO);
    SetFrozenFunction(env, prototype, "isEmpty", ContainersVector::IsEmpty, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "getLastElement", ContainersVector::GetLastElement, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "getLastIndexOf", ContainersVector::GetLastIndexOf, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "getLastIndexFrom", ContainersVector::GetLastIndexFrom, FuncLength::TWO);
    SetFrozenFunction(env, prototype, "remove", ContainersVector::Remove, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "removeByIndex", ContainersVector::RemoveByIndex, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "removeByRange", ContainersVector::RemoveByRange, FuncLength::TWO);
    SetFrozenFunction(env, prototype, "set", ContainersVector::Set, FuncLength::TWO);
    SetFrozenFunction(env, prototype, "subVector", ContainersVector::SubVector, FuncLength::TWO);
    SetFrozenFunction(env, prototype, "toString", ContainersVector::ToString, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "forEach", ContainersVector::ForEach, FuncLength::TWO,
                      BUILTINS_STUB_ID(VectorForEach));
    SetFrozenFunction(env, prototype, "replaceAllElements", ContainersVector::ReplaceAllElements,
                      FuncLength::TWO, BUILTINS_STUB_ID(VectorReplaceAllElements));
    SetFrozenFunction(env, prototype, "has", ContainersVector::Has, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "sort", ContainersVector::Sort, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "clear", ContainersVector::Clear, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "clone", ContainersVector::Clone, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "copyToArray", ContainersVector::CopyToArray, FuncLength::ONE);
    SetFrozenFunction(env, prototype, "convertToArray", ContainersVector::ConvertToArray, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "getFirstElement", ContainersVector::GetFirstElement, FuncLength::ZERO);
    SetFrozenFunction(env, prototype, "trimToCurrentLength",
                      ContainersVector::TrimToCurrentLength, FuncLength::ZERO);

    SetStringTagSymbol(env, prototype, "Vector");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env,
                                                        ContainersVector::GetSize, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, prototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(env, prototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersVector::GetIteratorObj, FuncLength::ONE);

    ContainersPrivate::InitializeVectorIterator(thread, env);
    env->SetVectorFunction(thread, vectorFunction);
    return vectorFunction;
}

void ContainersPrivate::InitializeVectorIterator(JSThread *thread, const JSHandle<GlobalEnv> &env)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncHClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    // VectorIterator.prototype
    JSHandle<JSObject> vectorIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // Iterator.prototype.next()
    SetFrozenFunction(env, vectorIteratorPrototype, "next", JSAPIVectorIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, vectorIteratorPrototype, "Vector Iterator");
    env->SetVectorIteratorPrototype(thread, vectorIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeBitVector(JSThread* thread)
{
    auto vm = thread->GetEcmaVM();
    ObjectFactory* factory = vm->GetFactory();
    JSHandle<GlobalEnv> env = vm->GetGlobalEnv();
    Builtins builtin(thread, factory, vm);
    JSHandle<JSObject> sObjPrototype = JSHandle<JSObject>::Cast(env->GetSObjectFunctionPrototype());
    JSHandle<JSFunction> sFuncPrototype = JSHandle<JSFunction>::Cast(env->GetSFunctionPrototype());
    builtin.InitializeSharedBitVector(env, sObjPrototype, sFuncPrototype);
    InitializeBitVectorIterator(thread, env);
    JSHandle<JSTaggedValue> bitVectorFunction = env->GetBitVectorFunction();
    return bitVectorFunction;
}

void ContainersPrivate::InitializeBitVectorIterator(JSThread* thread, const JSHandle<GlobalEnv>& env)
{
    ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> bitVectorIteratorPrototype(factory->NewJSObjectWithInit(iteratorFuncClass));
    // Iterator.prototype.next()
    SetFrozenFunction(env, bitVectorIteratorPrototype, "next", JSAPIBitVectorIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, bitVectorIteratorPrototype, "BitVector Iterator");
    env->SetBitVectorIteratorPrototype(thread, bitVectorIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeQueue(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // Queue.prototype
    JSHandle<JSObject> queueFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> queueFuncPrototypeValue(queueFuncPrototype);
    // Queue.prototype_or_hclass
    JSHandle<JSHClass> queueInstanceClass =
        factory->NewEcmaHClass(JSAPIQueue::SIZE, JSType::JS_API_QUEUE, queueFuncPrototypeValue);
    // Queue() = new Function()
    JSHandle<JSTaggedValue> queueFunction(NewContainerConstructor(
        env, queueFuncPrototype, ContainersQueue::QueueConstructor, "Queue", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(queueFunction), queueInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(queueFuncPrototype), constructorKey, queueFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // Queue.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersQueue::GetQueuePrototypeFunctions()) {
        SetFrozenFunction(env, queueFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    SetStringTagSymbol(env, queueFuncPrototype, "Queue");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env,
                                                        ContainersQueue::GetSize, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, queueFuncPrototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(env, queueFuncPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersQueue::GetIteratorObj, FuncLength::ONE);

    ContainersPrivate::InitializeQueueIterator(thread, env);
    return queueFunction;
}

void ContainersPrivate::InitializeQueueIterator(JSThread *thread, const JSHandle<GlobalEnv> &env)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncHClass =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_ITERATOR, env->GetIteratorPrototype());
    // QueueIterator.prototype
    JSHandle<JSObject> queueIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // Iterator.prototype.next()
    SetFrozenFunction(env, queueIteratorPrototype, "next", JSAPIQueueIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, queueIteratorPrototype, "Queue Iterator");
    env->SetQueueIteratorPrototype(thread, queueIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeDeque(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // Deque.prototype
    JSHandle<JSObject> dequeFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> dequeFuncPrototypeValue(dequeFuncPrototype);
    // Deque.prototype_or_hclass
    JSHandle<JSHClass> dequeInstanceClass =
        factory->NewEcmaHClass(JSAPIDeque::SIZE, JSType::JS_API_DEQUE, dequeFuncPrototypeValue);
    // Deque() = new Function()
    JSHandle<JSTaggedValue> dequeFunction(NewContainerConstructor(
        env, dequeFuncPrototype, ContainersDeque::DequeConstructor, "Deque", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(dequeFunction), dequeInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(dequeFuncPrototype), constructorKey, dequeFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // Deque.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersDeque::GetDequePrototypeFunctions()) {
        SetFrozenFunction(env, dequeFuncPrototype, entry.GetName().data(),
                          entry.GetEntrypoint(), entry.GetLength(), entry.GetBuiltinStubId());
    }

    SetStringTagSymbol(env, dequeFuncPrototype, "Deque");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env,
                                                        ContainersDeque::GetSize, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    SetGetter(thread, dequeFuncPrototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(env, dequeFuncPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersDeque::GetIteratorObj, FuncLength::ONE);

    ContainersPrivate::InitializeDequeIterator(thread, env);

    return dequeFunction;
}

void ContainersPrivate::InitializeDequeIterator(JSThread *thread, const JSHandle<GlobalEnv> &env)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncHClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> dequeIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    SetFrozenFunction(env, dequeIteratorPrototype, "next", JSAPIDequeIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, dequeIteratorPrototype, "Deque Iterator");
    env->SetDequeIteratorPrototype(thread, dequeIteratorPrototype);
}

void ContainersPrivate::InitializeLinkedListIterator(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> linkedListIteratorPrototype(factory->NewJSObject(iteratorClass));
    SetFrozenFunction(env, linkedListIteratorPrototype, "next", JSAPILinkedListIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, linkedListIteratorPrototype, "linkedlist Iterator");
    env->SetLinkedListIteratorPrototype(thread, linkedListIteratorPrototype);
}

void ContainersPrivate::InitializeListIterator(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> listIteratorPrototype(factory->NewJSObject(iteratorClass));
    SetFrozenFunction(env, listIteratorPrototype, "next", JSAPIListIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(env, listIteratorPrototype, "list Iterator");
    env->SetListIteratorPrototype(thread, listIteratorPrototype);
}

void ContainersPrivate::InitializeHashMapIterator(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncHClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    // HashMapIterator.prototype
    JSHandle<JSObject> hashMapIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // HashMapIterator.prototype.next()
    SetFrozenFunction(env, hashMapIteratorPrototype, "next", JSAPIHashMapIterator::Next, FuncLength::ZERO);
    SetStringTagSymbol(env, hashMapIteratorPrototype, "HashMap Iterator");
    env->SetHashMapIteratorPrototype(thread, hashMapIteratorPrototype);
}

void ContainersPrivate::InitializeHashSetIterator(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncHClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());

    // HashSetIterator.prototype
    JSHandle<JSObject> hashSetIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // HashSetIterator.prototype.next()
    SetFrozenFunction(env, hashSetIteratorPrototype, "next", JSAPIHashSetIterator::Next, FuncLength::ZERO);
    SetStringTagSymbol(env, hashSetIteratorPrototype, "HashSet Iterator");
    env->SetHashSetIteratorPrototype(thread, hashSetIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeBuffer(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // Buffer.prototype
    JSHandle<JSObject> bufferFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> bufferFuncPrototypeValue(bufferFuncPrototype);
    // Buffer.prototype_or_hclass
    JSHandle<JSHClass> bufferInstanceClass =
        factory->NewEcmaHClass(JSAPIFastBuffer::SIZE, JSType::JS_API_FAST_BUFFER, bufferFuncPrototypeValue);
    // Buffer() = new Function()
    JSHandle<JSTaggedValue> bufferFunction(NewContainerConstructor(
        env, bufferFuncPrototype, ContainersBuffer::BufferConstructor, "FastBuffer", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread, JSHandle<JSFunction>::Cast(bufferFunction),
                                                     bufferInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(bufferFuncPrototype), constructorKey, bufferFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // FastBuffer.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry : ContainersBuffer::GetFastBufferPrototypeFunctions()) {
        SetFrozenFunction(env, bufferFuncPrototype, entry.GetName().data(),
                          entry.GetEntrypoint(), entry.GetLength(), entry.GetBuiltinStubId());
    }

    SetStringTagSymbol(env, bufferFuncPrototype, "FastBuffer");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env,
                                                        ContainersBuffer::GetSize, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, bufferFuncPrototype, lengthKey, lengthGetter);
    JSHandle<JSTaggedValue> bufferGetter =
        CreateGetter(env, ContainersBuffer::GetArrayBuffer, "buffer", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> bufferKey(factory->NewFromASCII("buffer"));
    SetGetter(thread, bufferFuncPrototype, bufferKey, bufferGetter);
    JSHandle<JSTaggedValue> offsetGetter =
        CreateGetter(env, ContainersBuffer::GetByteOffset, "byteOffset", FunctionLength::ZERO);
    JSHandle<JSTaggedValue> offsetKey(factory->NewFromASCII("byteOffset"));
    SetGetter(thread, bufferFuncPrototype, offsetKey, offsetGetter);

    env->SetBufferFunction(thread, bufferFunction);
    return bufferFunction;
}

void ContainersPrivate::InitializeTreeSetIterator(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorClass =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_ITERATOR, env->GetIteratorPrototype());
    JSHandle<JSObject> setIteratorPrototype(factory->NewJSObject(iteratorClass));
    SetFrozenFunction(env, setIteratorPrototype, "next", JSAPITreeSetIterator::Next, FuncLength::ZERO);
    SetStringTagSymbol(env, setIteratorPrototype, "TreeSet Iterator");
    env->SetTreeSetIteratorPrototype(thread, setIteratorPrototype);
}

}  // namespace panda::ecmascript::containers
