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
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_bitvector_iterator.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightset_iterator.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
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

namespace panda::ecmascript::containers {
JSTaggedValue ContainersPrivate::Load(EcmaRuntimeCallInfo *msg)
{
    ASSERT(msg != nullptr);
    JSThread *thread = msg->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> argv = GetCallArg(msg, 0);
    JSHandle<JSObject> thisValue(GetThis(msg));

    uint32_t tag = 0;
    if (!JSTaggedValue::ToElementIndex(argv.GetTaggedValue(), &tag) || tag >= ContainerTag::END) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Incorrect input parameters", JSTaggedValue::Exception());
    }

    // Lazy set an undefinedIteratorResult to global constants
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    JSHandle<JSTaggedValue> undefinedHandle = globalConst->GetHandledUndefined();
    JSHandle<JSObject> undefinedIteratorResult = JSIterator::CreateIterResultObject(thread, undefinedHandle, true);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    env->SetUndefinedIteratorResult(thread, undefinedIteratorResult);

    JSTaggedValue res = JSTaggedValue::Undefined();
    switch (tag) {
        case ContainerTag::ArrayList: {
            res = InitializeContainer(thread, thisValue, InitializeArrayList, "ArrayListConstructor");
            break;
        }
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
        case ContainerTag::TreeSet: {
            res = InitializeContainer(thread, thisValue, InitializeTreeSet, "TreeSetConstructor");
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
        case ContainerTag::List: {
            res = InitializeContainer(thread, thisValue, InitializeList, "ListConstructor");
            break;
        }
        case ContainerTag::LinkedList: {
            res = InitializeContainer(thread, thisValue, InitializeLinkedList, "LinkedListConstructor");
            break;
        }
        case ContainerTag::HashMap: {
            res = InitializeContainer(thread, thisValue, InitializeHashMap, "HashMapConstructor");
            break;
        }
        case ContainerTag::HashSet: {
            res = InitializeContainer(thread, thisValue, InitializeHashSet, "HashSetConstructor");
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

JSHandle<JSFunction> ContainersPrivate::NewContainerConstructor(JSThread *thread, const JSHandle<JSObject> &prototype,
                                                                EcmaEntrypoint ctorFunc, const char *name, int length)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> ctor =
        factory->NewJSFunction(env, reinterpret_cast<void *>(ctorFunc), FunctionKind::BUILTIN_CONSTRUCTOR);

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

void ContainersPrivate::SetFrozenFunction(JSThread *thread, const JSHandle<JSObject> &obj, const char *key,
                                          EcmaEntrypoint func, int length, kungfu::BuiltinsStubCSigns::ID builtinId)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> keyString(factory->NewFromASCII(key));
    JSHandle<JSFunction> function = NewFunction(thread, keyString, func, length, builtinId);
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

JSHandle<JSFunction> ContainersPrivate::NewFunction(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                                    EcmaEntrypoint func, int length,
                                                    kungfu::BuiltinsStubCSigns::ID builtinId)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function =
        factory->NewJSFunction(thread->GetEcmaVM()->GetGlobalEnv(), reinterpret_cast<void *>(func),
                               FunctionKind::NORMAL_FUNCTION, builtinId);
    JSFunction::SetFunctionLength(thread, function, JSTaggedValue(length));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSFunction::SetFunctionName(thread, baseFunction, key, thread->GlobalConstants()->GetHandledUndefined());
    return function;
}

JSHandle<JSTaggedValue> ContainersPrivate::CreateGetter(JSThread *thread, EcmaEntrypoint func, const char *name,
                                                        int length)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function = factory->NewJSFunction(env, reinterpret_cast<void *>(func));
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

void ContainersPrivate::SetFunctionAtSymbol(JSThread *thread, const JSHandle<GlobalEnv> &env,
                                            const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &symbol,
                                            const char *name, EcmaEntrypoint func, int length)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> function = factory->NewJSFunction(env, reinterpret_cast<void *>(func));
    JSFunction::SetFunctionLength(thread, function, JSTaggedValue(length));
    JSHandle<JSTaggedValue> nameString(factory->NewFromASCII(name));
    JSHandle<JSFunctionBase> baseFunction(function);
    JSFunction::SetFunctionName(thread, baseFunction, nameString, thread->GlobalConstants()->GetHandledUndefined());
    PropertyDescriptor descriptor(thread, JSHandle<JSTaggedValue>::Cast(function), false, false, false);
    JSObject::DefineOwnProperty(thread, obj, symbol, descriptor);
}

void ContainersPrivate::SetStringTagSymbol(JSThread *thread, const JSHandle<GlobalEnv> &env,
                                           const JSHandle<JSObject> &obj, const char *key)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> tag(factory->NewFromASCII(key));
    JSHandle<JSTaggedValue> symbol = env->GetToStringTagSymbol();
    PropertyDescriptor desc(thread, tag, false, false, false);
    JSObject::DefineOwnProperty(thread, obj, symbol, desc);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeArrayList(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // ArrayList.prototype
    JSHandle<JSObject> prototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> arrayListFuncPrototypeValue(prototype);
    // ArrayList.prototype_or_hclass
    JSHandle<JSHClass> arrayListInstanceClass =
        factory->NewEcmaHClass(JSAPIArrayList::SIZE, JSType::JS_API_ARRAY_LIST, arrayListFuncPrototypeValue);
    // ArrayList() = new Function()
    JSHandle<JSTaggedValue> arrayListFunction(NewContainerConstructor(
        thread, prototype, ContainersArrayList::ArrayListConstructor, "ArrayList", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(arrayListFunction), arrayListInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(prototype), constructorKey, arrayListFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // ArrayList.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersArrayList::GetArrayListPrototypeFunctions()) {
        SetFrozenFunction(thread, prototype, entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(),
                          entry.GetBuiltinStubId());
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetStringTagSymbol(thread, env, prototype, "ArrayList");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersArrayList::GetSize, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, prototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(thread, env, prototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersArrayList::GetIteratorObj, FuncLength::ONE);
    ContainersPrivate::InitializeArrayListIterator(thread, env);
    env->SetArrayListFunction(thread, arrayListFunction);
    return arrayListFunction;
}

void ContainersPrivate::InitializeArrayListIterator(JSThread *thread, const JSHandle<GlobalEnv> &env)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Iterator.hclass
    JSHandle<JSHClass> iteratorFuncHClass =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_ITERATOR, env->GetIteratorPrototype());
    // ArrayListIterator.prototype
    JSHandle<JSObject> arrayListIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // Iterator.prototype.next()
    SetFrozenFunction(thread, arrayListIteratorPrototype, "next", JSAPIArrayListIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, arrayListIteratorPrototype, "ArrayList Iterator");
    env->SetArrayListIteratorPrototype(thread, arrayListIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeLightWeightMap(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> funcPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> mapFuncPrototypeValue(funcPrototype);
    JSHandle<JSHClass> lightWeightMapInstanceClass =
        factory->NewEcmaHClass(JSAPILightWeightMap::SIZE, JSType::JS_API_LIGHT_WEIGHT_MAP, mapFuncPrototypeValue);
    JSHandle<JSTaggedValue> lightWeightMapFunction(NewContainerConstructor(
        thread, funcPrototype, ContainersLightWeightMap::LightWeightMapConstructor, "LightWeightMap",
        FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(lightWeightMapFunction), lightWeightMapInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(funcPrototype), constructorKey, lightWeightMapFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // LightWeightMap.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersLightWeightMap::GetLightWeightMapPrototypeFunctions()) {
        SetFrozenFunction(thread, funcPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersLightWeightMap::Length, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    SetGetter(thread, funcPrototype, lengthKey, lengthGetter);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetFunctionAtSymbol(thread, env, funcPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersLightWeightMap::Entries, FuncLength::ONE);

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
    SetFrozenFunction(thread, lightWeightMapIteratorPrototype, "next", JSAPILightWeightMapIterator::Next,
                      FuncLength::ONE);
    SetStringTagSymbol(thread, env, lightWeightMapIteratorPrototype, "LightWeightMap Iterator");
    env->SetLightWeightMapIteratorPrototype(thread, lightWeightMapIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeLightWeightSet(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // LightWeightSet.prototype
    JSHandle<JSObject> funcPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> funcPrototypeValue(funcPrototype);
    // LightWeightSet.prototype_or_hclass
    JSHandle<JSHClass> lightweightSetInstanceClass =
        factory->NewEcmaHClass(JSAPILightWeightSet::SIZE, JSType::JS_API_LIGHT_WEIGHT_SET, funcPrototypeValue);
    JSHandle<JSTaggedValue> lightweightSetFunction(
        NewContainerConstructor(thread, funcPrototype, ContainersLightWeightSet::LightWeightSetConstructor,
                                "LightWeightSet", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(lightweightSetFunction), lightweightSetInstanceClass.GetTaggedValue());
    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(funcPrototype), constructorKey, lightweightSetFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // LightWeightSet.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersLightWeightSet::GetLightWeightSetPrototypeFunctions()) {
        SetFrozenFunction(thread, funcPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(thread, ContainersLightWeightSet::GetSize, "length", FuncLength::ZERO);

    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    SetGetter(thread, funcPrototype, lengthKey, lengthGetter);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetFunctionAtSymbol(thread, env, funcPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersLightWeightSet::GetIteratorObj, FuncLength::ONE);

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
        thread, lightWeightSetIteratorPrototype, "next", JSAPILightWeightSetIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, lightWeightSetIteratorPrototype, "LightWeightSet Iterator");
    env->SetLightWeightSetIteratorPrototype(thread, lightWeightSetIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeTreeMap(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // TreeMap.prototype
    JSHandle<JSObject> mapFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> mapFuncPrototypeValue(mapFuncPrototype);
    // TreeMap.prototype_or_hclass
    JSHandle<JSHClass> mapInstanceClass =
        factory->NewEcmaHClass(JSAPITreeMap::SIZE, JSType::JS_API_TREE_MAP, mapFuncPrototypeValue);
    // TreeMap() = new Function()
    JSHandle<JSTaggedValue> mapFunction(NewContainerConstructor(
        thread, mapFuncPrototype, ContainersTreeMap::TreeMapConstructor, "TreeMap", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(mapFunction), mapInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(mapFuncPrototype), constructorKey, mapFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // TreeMap.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersTreeMap::GetTreeMapPrototypeFunctions()) {
        SetFrozenFunction(thread, mapFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    // @@ToStringTag
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetStringTagSymbol(thread, env, mapFuncPrototype, "TreeMap");
    // %TreeMapPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> entries = globalConst->GetHandledEntriesString();
    JSHandle<JSTaggedValue> entriesFunc =
        JSObject::GetMethod(thread, JSHandle<JSTaggedValue>::Cast(mapFuncPrototype), entries);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    PropertyDescriptor descriptor(thread, entriesFunc, false, false, false);
    JSObject::DefineOwnProperty(thread, mapFuncPrototype, iteratorSymbol, descriptor);
    // length
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(thread, ContainersTreeMap::GetLength, "length", FuncLength::ZERO);
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
    SetFrozenFunction(thread, mapIteratorPrototype, "next", JSAPITreeMapIterator::Next, FuncLength::ZERO);
    SetStringTagSymbol(thread, env, mapIteratorPrototype, "TreeMap Iterator");
    env->SetTreeMapIteratorPrototype(thread, mapIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeTreeSet(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // TreeSet.prototype
    JSHandle<JSObject> setFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> setFuncPrototypeValue(setFuncPrototype);
    // TreeSet.prototype_or_hclass
    JSHandle<JSHClass> setInstanceClass =
        factory->NewEcmaHClass(JSAPITreeSet::SIZE, JSType::JS_API_TREE_SET, setFuncPrototypeValue);
    // TreeSet() = new Function()
    JSHandle<JSTaggedValue> setFunction(NewContainerConstructor(
        thread, setFuncPrototype, ContainersTreeSet::TreeSetConstructor, "TreeSet", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(setFunction), setInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(setFuncPrototype), constructorKey, setFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // TreeSet.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersTreeSet::GetTreeSetPrototypeFunctions()) {
        SetFrozenFunction(thread, setFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    // @@ToStringTag
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetStringTagSymbol(thread, env, setFuncPrototype, "TreeSet");
    // %TreeSetPrototype% [ @@iterator ]
    JSHandle<JSTaggedValue> iteratorSymbol = env->GetIteratorSymbol();
    JSHandle<JSTaggedValue> values(thread, globalConst->GetValuesString());
    JSHandle<JSTaggedValue> valuesFunc =
        JSObject::GetMethod(thread, JSHandle<JSTaggedValue>::Cast(setFuncPrototype), values);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    PropertyDescriptor descriptor(thread, valuesFunc, false, false, false);
    JSObject::DefineOwnProperty(thread, setFuncPrototype, iteratorSymbol, descriptor);
    // length
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(thread, ContainersTreeSet::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, setFuncPrototype, lengthKey, lengthGetter);

    InitializeTreeSetIterator(thread);
    return setFunction;
}

void ContainersPrivate::InitializeTreeSetIterator(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Iterator.hclass
    JSHandle<JSHClass> iteratorClass =
        factory->NewEcmaHClass(JSObject::SIZE, JSType::JS_ITERATOR, env->GetIteratorPrototype());

    // TreeSetIterator.prototype
    JSHandle<JSObject> setIteratorPrototype(factory->NewJSObject(iteratorClass));
    // TreeSetIterator.prototype.next()
    SetFrozenFunction(thread, setIteratorPrototype, "next", JSAPITreeSetIterator::Next, FuncLength::ZERO);
    SetStringTagSymbol(thread, env, setIteratorPrototype, "TreeSet Iterator");
    env->SetTreeSetIteratorPrototype(thread, setIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializePlainArray(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // PlainArray.prototype
    JSHandle<JSObject> plainArrayFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> plainArrayFuncPrototypeValue(plainArrayFuncPrototype);
    // PlainArray.prototype_or_hclass
    JSHandle<JSHClass> plainArrayInstanceClass =
        factory->NewEcmaHClass(JSAPIPlainArray::SIZE, JSType::JS_API_PLAIN_ARRAY, plainArrayFuncPrototypeValue);
    JSHandle<JSTaggedValue> plainArrayFunction(
        NewContainerConstructor(thread, plainArrayFuncPrototype, ContainersPlainArray::PlainArrayConstructor,
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
        SetFrozenFunction(thread, plainArrayFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersPlainArray::GetSize, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    SetGetter(thread, plainArrayFuncPrototype, lengthKey, lengthGetter);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetFunctionAtSymbol(thread, env, plainArrayFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersPlainArray::GetIteratorObj, FuncLength::ONE);
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
    SetFrozenFunction(thread, plainarrayIteratorPrototype, "next", JSAPIPlainArrayIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, plainarrayIteratorPrototype, "PlainArray Iterator");
    env->SetPlainArrayIteratorPrototype(thread, plainarrayIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeStack(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Stack.prototype
    JSHandle<JSObject> stackFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> stackFuncPrototypeValue(stackFuncPrototype);
    // Stack.prototype_or_hclass
    JSHandle<JSHClass> stackInstanceClass =
        factory->NewEcmaHClass(JSAPIStack::SIZE, JSType::JS_API_STACK, stackFuncPrototypeValue);
    // Stack() = new Function()
    JSHandle<JSTaggedValue> stackFunction(NewContainerConstructor(
        thread, stackFuncPrototype, ContainersStack::StackConstructor, "Stack", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(stackFunction), stackInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(stackFuncPrototype), constructorKey, stackFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // Stack.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersStack::GetStackPrototypeFunctions()) {
        SetFrozenFunction(thread, stackFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetStringTagSymbol(thread, env, stackFuncPrototype, "Stack");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersStack::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    SetGetter(thread, stackFuncPrototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(thread, env, stackFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersStack::Iterator, FuncLength::ONE);

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
    SetFrozenFunction(thread, stackIteratorPrototype, "next", JSAPIStackIterator::Next, FuncLength::ONE);
    env->SetStackIteratorPrototype(thread, stackIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeVector(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Vector.prototype
    JSHandle<JSObject> prototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> vectorFuncPrototypeValue(prototype);
    // Vector.prototype_or_hclass
    JSHandle<JSHClass> vectorInstanceClass =
        factory->NewEcmaHClass(JSAPIVector::SIZE, JSType::JS_API_VECTOR, vectorFuncPrototypeValue);
    // Vector() = new Function()
    JSHandle<JSTaggedValue> vectorFunction(NewContainerConstructor(
        thread, prototype, ContainersVector::VectorConstructor, "Vector", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(vectorFunction), vectorInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(prototype), constructorKey, vectorFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // Vector.prototype
    SetFrozenFunction(thread, prototype, "add", ContainersVector::Add, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "insert", ContainersVector::Insert, FuncLength::TWO);
    SetFrozenFunction(thread, prototype, "setLength", ContainersVector::SetLength, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "getCapacity", ContainersVector::GetCapacity, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "increaseCapacityTo", ContainersVector::IncreaseCapacityTo, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "get", ContainersVector::Get, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "getIndexOf", ContainersVector::GetIndexOf, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "getIndexFrom", ContainersVector::GetIndexFrom, FuncLength::TWO);
    SetFrozenFunction(thread, prototype, "isEmpty", ContainersVector::IsEmpty, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "getLastElement", ContainersVector::GetLastElement, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "getLastIndexOf", ContainersVector::GetLastIndexOf, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "getLastIndexFrom", ContainersVector::GetLastIndexFrom, FuncLength::TWO);
    SetFrozenFunction(thread, prototype, "remove", ContainersVector::Remove, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "removeByIndex", ContainersVector::RemoveByIndex, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "removeByRange", ContainersVector::RemoveByRange, FuncLength::TWO);
    SetFrozenFunction(thread, prototype, "set", ContainersVector::Set, FuncLength::TWO);
    SetFrozenFunction(thread, prototype, "subVector", ContainersVector::SubVector, FuncLength::TWO);
    SetFrozenFunction(thread, prototype, "toString", ContainersVector::ToString, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "forEach", ContainersVector::ForEach, FuncLength::TWO,
                      BUILTINS_STUB_ID(VectorForEach));
    SetFrozenFunction(thread, prototype, "replaceAllElements", ContainersVector::ReplaceAllElements, FuncLength::TWO,
                      BUILTINS_STUB_ID(VectorReplaceAllElements));
    SetFrozenFunction(thread, prototype, "has", ContainersVector::Has, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "sort", ContainersVector::Sort, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "clear", ContainersVector::Clear, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "clone", ContainersVector::Clone, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "copyToArray", ContainersVector::CopyToArray, FuncLength::ONE);
    SetFrozenFunction(thread, prototype, "convertToArray", ContainersVector::ConvertToArray, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "getFirstElement", ContainersVector::GetFirstElement, FuncLength::ZERO);
    SetFrozenFunction(thread, prototype, "trimToCurrentLength",
                      ContainersVector::TrimToCurrentLength, FuncLength::ZERO);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetStringTagSymbol(thread, env, prototype, "Vector");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersVector::GetSize, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, prototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(thread, env, prototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersVector::GetIteratorObj, FuncLength::ONE);

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
    SetFrozenFunction(thread, vectorIteratorPrototype, "next", JSAPIVectorIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, vectorIteratorPrototype, "Vector Iterator");
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
    SetFrozenFunction(thread, bitVectorIteratorPrototype, "next", JSAPIBitVectorIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, bitVectorIteratorPrototype, "BitVector Iterator");
    env->SetBitVectorIteratorPrototype(thread, bitVectorIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeQueue(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Queue.prototype
    JSHandle<JSObject> queueFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> queueFuncPrototypeValue(queueFuncPrototype);
    // Queue.prototype_or_hclass
    JSHandle<JSHClass> queueInstanceClass =
        factory->NewEcmaHClass(JSAPIQueue::SIZE, JSType::JS_API_QUEUE, queueFuncPrototypeValue);
    // Queue() = new Function()
    JSHandle<JSTaggedValue> queueFunction(NewContainerConstructor(
        thread, queueFuncPrototype, ContainersQueue::QueueConstructor, "Queue", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(queueFunction), queueInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(queueFuncPrototype), constructorKey, queueFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // Queue.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersQueue::GetQueuePrototypeFunctions()) {
        SetFrozenFunction(thread, queueFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetStringTagSymbol(thread, env, queueFuncPrototype, "Queue");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersQueue::GetSize, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, queueFuncPrototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(thread, env, queueFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersQueue::GetIteratorObj, FuncLength::ONE);

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
    SetFrozenFunction(thread, queueIteratorPrototype, "next", JSAPIQueueIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, queueIteratorPrototype, "Queue Iterator");
    env->SetQueueIteratorPrototype(thread, queueIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeDeque(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Deque.prototype
    JSHandle<JSObject> dequeFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> dequeFuncPrototypeValue(dequeFuncPrototype);
    // Deque.prototype_or_hclass
    JSHandle<JSHClass> dequeInstanceClass =
        factory->NewEcmaHClass(JSAPIDeque::SIZE, JSType::JS_API_DEQUE, dequeFuncPrototypeValue);
    // Deque() = new Function()
    JSHandle<JSTaggedValue> dequeFunction(NewContainerConstructor(
        thread, dequeFuncPrototype, ContainersDeque::DequeConstructor, "Deque", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(dequeFunction), dequeInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(dequeFuncPrototype), constructorKey, dequeFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // Deque.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersDeque::GetDequePrototypeFunctions()) {
        SetFrozenFunction(thread, dequeFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(),
                          entry.GetBuiltinStubId());
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetStringTagSymbol(thread, env, dequeFuncPrototype, "Deque");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersDeque::GetSize, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    SetGetter(thread, dequeFuncPrototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(thread, env, dequeFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersDeque::GetIteratorObj, FuncLength::ONE);

    ContainersPrivate::InitializeDequeIterator(thread, env);

    return dequeFunction;
}

void ContainersPrivate::InitializeDequeIterator(JSThread *thread, const JSHandle<GlobalEnv> &env)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncHClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> dequeIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    SetFrozenFunction(thread, dequeIteratorPrototype, "next", JSAPIDequeIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, dequeIteratorPrototype, "Deque Iterator");
    env->SetDequeIteratorPrototype(thread, dequeIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeList(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> listFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> listFuncPrototypeValue(listFuncPrototype);
    JSHandle<JSHClass> listInstanceClass =
        factory->NewEcmaHClass(JSAPIList::SIZE, JSType::JS_API_LIST, listFuncPrototypeValue);
    JSHandle<JSTaggedValue> listFunction(NewContainerConstructor(
        thread, listFuncPrototype, ContainersList::ListConstructor, "List", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(listFunction), listInstanceClass.GetTaggedValue());

    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(listFuncPrototype), constructorKey, listFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // List.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersList::GetListPrototypeFunctions()) {
        SetFrozenFunction(thread, listFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(),
                          entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersList::Length, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    SetGetter(thread, listFuncPrototype, lengthKey, lengthGetter);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetFunctionAtSymbol(thread, env, listFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersList::GetIteratorObj, FuncLength::ONE);

    InitializeListIterator(thread, env);
    env->SetListFunction(thread, listFunction);
    return listFunction;
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeLinkedList(JSThread *thread)
{
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> linkedListFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> linkedListFuncPrototypeValue(linkedListFuncPrototype);
    JSHandle<JSHClass> linkedListInstanceClass =
        factory->NewEcmaHClass(JSAPILinkedList::SIZE, JSType::JS_API_LINKED_LIST, linkedListFuncPrototypeValue);
    JSHandle<JSTaggedValue> linkedListFunction(NewContainerConstructor(
        thread, linkedListFuncPrototype, ContainersLinkedList::LinkedListConstructor, "LinkedList", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(linkedListFunction), linkedListInstanceClass.GetTaggedValue());

    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(linkedListFuncPrototype), constructorKey, linkedListFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // LinkedList.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersLinkedList::GetLinkedListPrototypeFunctions()) {
        SetFrozenFunction(thread, linkedListFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(),
                          entry.GetBuiltinStubId());
    }

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(thread, ContainersLinkedList::Length, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    SetGetter(thread, linkedListFuncPrototype, lengthKey, lengthGetter);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    SetFunctionAtSymbol(thread, env, linkedListFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersLinkedList::GetIteratorObj, FuncLength::ONE);

    InitializeLinkedListIterator(thread, env);
    env->SetLinkedListFunction(thread, linkedListFunction);
    return linkedListFunction;
}

void ContainersPrivate::InitializeLinkedListIterator(JSThread *thread, const JSHandle<GlobalEnv> &env)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> linkedListIteratorPrototype(factory->NewJSObject(iteratorClass));
    SetFrozenFunction(thread, linkedListIteratorPrototype, "next", JSAPILinkedListIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, linkedListIteratorPrototype, "linkedlist Iterator");
    env->SetLinkedListIteratorPrototype(thread, linkedListIteratorPrototype);
}

void ContainersPrivate::InitializeListIterator(JSThread *thread, const JSHandle<GlobalEnv> &env)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    JSHandle<JSObject> listIteratorPrototype(factory->NewJSObject(iteratorClass));
    SetFrozenFunction(thread, listIteratorPrototype, "next", JSAPIListIterator::Next, FuncLength::ONE);
    SetStringTagSymbol(thread, env, listIteratorPrototype, "list Iterator");
    env->SetListIteratorPrototype(thread, listIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeHashMap(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // HashMap.prototype
    JSHandle<JSObject> hashMapFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> hashMapFuncPrototypeValue(hashMapFuncPrototype);
    // HashMap.prototype_or_hclass
    JSHandle<JSHClass> hashMapInstanceClass =
        factory->NewEcmaHClass(JSAPIHashMap::SIZE, JSType::JS_API_HASH_MAP, hashMapFuncPrototypeValue);

    JSHandle<JSTaggedValue> hashMapFunction(NewContainerConstructor(
        thread, hashMapFuncPrototype, ContainersHashMap::HashMapConstructor, "HashMap", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(hashMapFunction), hashMapInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(hashMapFuncPrototype), constructorKey, hashMapFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // HashMap.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersHashMap::GetHashMapPrototypeFunctions()) {
        SetFrozenFunction(thread, hashMapFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // @@ToStringTag
    SetStringTagSymbol(thread, env, hashMapFuncPrototype, "HashMap");
    // %HashMapPrototype% [ @@iterator ]
    SetFunctionAtSymbol(thread, env, hashMapFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersHashMap::GetIteratorObj, FuncLength::ONE);

    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(thread, ContainersHashMap::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, hashMapFuncPrototype, lengthKey, lengthGetter);
    InitializeHashMapIterator(thread);
    return hashMapFunction;
}

void ContainersPrivate::InitializeHashMapIterator(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncHClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());
    // HashMapIterator.prototype
    JSHandle<JSObject> hashMapIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // HashMapIterator.prototype.next()
    SetFrozenFunction(thread, hashMapIteratorPrototype, "next", JSAPIHashMapIterator::Next, FuncLength::ZERO);
    SetStringTagSymbol(thread, env, hashMapIteratorPrototype, "HashMap Iterator");
    env->SetHashMapIteratorPrototype(thread, hashMapIteratorPrototype);
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeHashSet(JSThread *thread)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // HashSet.prototype
    JSHandle<JSObject> hashSetFuncPrototype = factory->NewEmptyJSObject();
    JSHandle<JSTaggedValue> hashSetFuncPrototypeValue(hashSetFuncPrototype);
    // HashSet.prototype_or_hclass
    JSHandle<JSHClass> hashSetInstanceClass =
        factory->NewEcmaHClass(JSAPIHashSet::SIZE, JSType::JS_API_HASH_SET, hashSetFuncPrototypeValue);

    JSHandle<JSTaggedValue> hashSetFunction(NewContainerConstructor(
        thread, hashSetFuncPrototype, ContainersHashSet::HashSetConstructor, "HashSet", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(hashSetFunction), hashSetInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(hashSetFuncPrototype), constructorKey, hashSetFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // HashSet.prototype methods (excluding constructor and '@@' internal properties)
    for (const base::BuiltinFunctionEntry &entry: ContainersHashSet::GetHashSetPrototypeFunctions()) {
        SetFrozenFunction(thread, hashSetFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // @@ToStringTag
    SetStringTagSymbol(thread, env, hashSetFuncPrototype, "HashSet");
    // %HashSetPrototype% [ @@iterator ]
    SetFunctionAtSymbol(thread, env, hashSetFuncPrototype, env->GetIteratorSymbol(), "[Symbol.iterator]",
                        ContainersHashSet::GetIteratorObj, FuncLength::ONE);

    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(thread, ContainersHashSet::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, hashSetFuncPrototype, lengthKey, lengthGetter);
    InitializeHashSetIterator(thread);
    return hashSetFunction;
}

void ContainersPrivate::InitializeHashSetIterator(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSHClass> iteratorFuncHClass = JSHandle<JSHClass>::Cast(env->GetIteratorFuncClass());

    // HashSetIterator.prototype
    JSHandle<JSObject> hashSetIteratorPrototype(factory->NewJSObject(iteratorFuncHClass));
    // HashSetIterator.prototype.next()
    SetFrozenFunction(thread, hashSetIteratorPrototype, "next", JSAPIHashSetIterator::Next, FuncLength::ZERO);
    SetStringTagSymbol(thread, env, hashSetIteratorPrototype, "HashSet Iterator");
    env->SetHashSetIteratorPrototype(thread, hashSetIteratorPrototype);
}
}  // namespace panda::ecmascript::containers
