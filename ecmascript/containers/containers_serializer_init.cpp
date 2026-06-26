/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "containers_hashmap.h"
#include "containers_hashset.h"
#include "containers_linked_list.h"
#include "containers_list.h"
#include "containers_treeset.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_iterator.h"

namespace panda::ecmascript::containers {
JSHandle<JSTaggedValue> ContainersPrivate::InitializeTreeSet(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // TreeSet.prototype
    JSHandle<JSObject> setFuncPrototype = factory->NewEmptyJSObject(env);
    JSHandle<JSTaggedValue> setFuncPrototypeValue(setFuncPrototype);

    // TreeSet.prototype_or_hclass
    JSHandle<JSHClass> setInstanceClass =
        factory->NewEcmaHClass(JSAPITreeSet::SIZE, JSType::JS_API_TREE_SET, setFuncPrototypeValue);

    // TreeSet() = new Function()
    JSHandle<JSTaggedValue> setFunction(NewContainerConstructor(
        env, setFuncPrototype, ContainersTreeSet::TreeSetConstructor,
        "TreeSet", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(setFunction), setInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(setFuncPrototype), constructorKey, setFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // TreeSet.prototype methods
    for (const base::BuiltinFunctionEntry &entry : ContainersTreeSet::GetTreeSetPrototypeFunctions()) {
        SetFrozenFunction(env, setFuncPrototype, entry.GetName().data(), entry.GetEntrypoint(),
                          entry.GetLength(), entry.GetBuiltinStubId());
    }

    // @@ToStringTag
    SetStringTagSymbol(env, setFuncPrototype, "TreeSet");

    // @@iterator
    JSHandle<JSTaggedValue> iteratorSymbol = globalConst->GetHandledIteratorSymbol();
    JSHandle<JSTaggedValue> values = globalConst->GetHandledValuesString();
    JSHandle<JSTaggedValue> valuesFunc =
        JSObject::GetMethod(thread, JSHandle<JSTaggedValue>::Cast(setFuncPrototype), values);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    PropertyDescriptor descriptor(thread, valuesFunc, false, false, false);
    JSObject::DefineOwnProperty(thread, setFuncPrototype, iteratorSymbol, descriptor);

    // length
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, ContainersTreeSet::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, setFuncPrototype, lengthKey, lengthGetter);

    InitializeTreeSetIterator(env);

    JSHandle<JSTaggedValue> undefinedHandle = globalConst->GetHandledUndefined();
    JSHandle<JSHClass> klass = JSHandle<JSHClass>::Cast(env->GetIteratorResultClass());
    JSHandle<JSObject> undefinedIteratorResult = factory->NewJSObject(klass);
    undefinedIteratorResult->SetPropertyInlinedPropsWithSize<JSIterator::SIZE, JSIterator::VALUE_INLINE_PROPERTY_INDEX>(
        thread, undefinedHandle.GetTaggedValue());
    undefinedIteratorResult->SetPropertyInlinedPropsWithSize<JSIterator::SIZE, JSIterator::DONE_INLINE_PROPERTY_INDEX>(
        thread, JSTaggedValue(true));

    env->SetUndefinedIteratorResult(thread, undefinedIteratorResult);

    env->SetTreeSetConstructor(thread, setFunction);
    return setFunction;
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeArrayList(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // ArrayList.prototype
    JSHandle<JSObject> prototype = factory->NewEmptyJSObject(env);
    JSHandle<JSTaggedValue> arrayListFuncPrototypeValue(prototype);

    // ArrayList.prototype_or_hclass
    JSHandle<JSHClass> arrayListInstanceClass =
        factory->NewEcmaHClass(JSAPIArrayList::SIZE, JSType::JS_API_ARRAY_LIST, arrayListFuncPrototypeValue);
    // ArrayList() = new Function()
    JSHandle<JSTaggedValue> arrayListFunction(NewContainerConstructor(
        env, prototype, ContainersArrayList::ArrayListConstructor, "ArrayList", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(arrayListFunction), arrayListInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(prototype), constructorKey, arrayListFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // ArrayList.prototype methods
    for (const base::BuiltinFunctionEntry &entry : ContainersArrayList::GetArrayListPrototypeFunctions()) {
        SetFrozenFunction(env, prototype,
                          entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(), entry.GetBuiltinStubId());
    }

    SetStringTagSymbol(env, prototype, "ArrayList");

    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env, ContainersArrayList::GetSize, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, prototype, lengthKey, lengthGetter);

    SetFunctionAtSymbol(env, prototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersArrayList::GetIteratorObj, FuncLength::ONE);

    InitializeArrayListIterator(env);
    env->SetArrayListFunction(thread, arrayListFunction);
    env->SetArrayListConstructor(thread, arrayListFunction);
    return arrayListFunction;
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeHashMap(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // HashMap.prototype
    JSHandle<JSObject> hashMapFuncPrototype = factory->NewEmptyJSObject(env);
    JSHandle<JSTaggedValue> hashMapFuncPrototypeValue(hashMapFuncPrototype);

    // HashMap.prototype_or_hclass
    JSHandle<JSHClass> hashMapInstanceClass =
        factory->NewEcmaHClass(JSAPIHashMap::SIZE, JSType::JS_API_HASH_MAP, hashMapFuncPrototypeValue);

    // HashMap() = new Function()
    JSHandle<JSTaggedValue> hashMapFunction(NewContainerConstructor(
        env, hashMapFuncPrototype, ContainersHashMap::HashMapConstructor, "HashMap", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(hashMapFunction), hashMapInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(hashMapFuncPrototype), constructorKey, hashMapFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // HashMap.prototype methods
    for (const base::BuiltinFunctionEntry &entry : ContainersHashMap::GetHashMapPrototypeFunctions()) {
        SetFrozenFunction(env, hashMapFuncPrototype,
                          entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(), entry.GetBuiltinStubId());
    }

    // @@ToStringTag
    SetStringTagSymbol(env, hashMapFuncPrototype, "HashMap");
    // @@iterator
    SetFunctionAtSymbol(env, hashMapFuncPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersHashMap::GetIteratorObj, FuncLength::ONE);

    // length
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, ContainersHashMap::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, hashMapFuncPrototype, lengthKey, lengthGetter);

    InitializeHashMapIterator(env);
    env->SetHashMapConstructor(thread, hashMapFunction);
    return hashMapFunction;
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeHashSet(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // HashSet.prototype
    JSHandle<JSObject> hashSetFuncPrototype = factory->NewEmptyJSObject(env);
    JSHandle<JSTaggedValue> hashSetFuncPrototypeValue(hashSetFuncPrototype);

    // HashSet.prototype_or_hclass
    JSHandle<JSHClass> hashSetInstanceClass =
        factory->NewEcmaHClass(JSAPIHashSet::SIZE, JSType::JS_API_HASH_SET, hashSetFuncPrototypeValue);

    // HashSet() = new Function()
    JSHandle<JSTaggedValue> hashSetFunction(NewContainerConstructor(
        env, hashSetFuncPrototype, ContainersHashSet::HashSetConstructor, "HashSet", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(hashSetFunction), hashSetInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(hashSetFuncPrototype), constructorKey, hashSetFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // HashSet.prototype methods
    for (const base::BuiltinFunctionEntry &entry : ContainersHashSet::GetHashSetPrototypeFunctions()) {
        SetFrozenFunction(env, hashSetFuncPrototype,
                          entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(), entry.GetBuiltinStubId());
    }

    // @@ToStringTag
    SetStringTagSymbol(env, hashSetFuncPrototype, "HashSet");
    // @@iterator
    SetFunctionAtSymbol(env, hashSetFuncPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersHashSet::GetIteratorObj, FuncLength::ONE);

    // length
    JSHandle<JSTaggedValue> lengthGetter =
        CreateGetter(env, ContainersHashSet::GetLength, "length", FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(thread, globalConst->GetLengthString());
    SetGetter(thread, hashSetFuncPrototype, lengthKey, lengthGetter);

    InitializeHashSetIterator(env);
    env->SetHashSetConstructor(thread, hashSetFunction);
    return hashSetFunction;
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeList(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // List.prototype
    JSHandle<JSObject> listFuncPrototype = factory->NewEmptyJSObject(env);
    JSHandle<JSTaggedValue> listFuncPrototypeValue(listFuncPrototype);

    // List.prototype_or_hclass
    JSHandle<JSHClass> listInstanceClass =
        factory->NewEcmaHClass(JSAPIList::SIZE, JSType::JS_API_LIST, listFuncPrototypeValue);

    // List() = new Function()
    JSHandle<JSTaggedValue> listFunction(NewContainerConstructor(
        env, listFuncPrototype, ContainersList::ListConstructor, "List", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(listFunction), listInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(listFuncPrototype), constructorKey, listFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // List.prototype methods
    for (const base::BuiltinFunctionEntry &entry : ContainersList::GetListPrototypeFunctions()) {
        SetFrozenFunction(env, listFuncPrototype,
                          entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(), entry.GetBuiltinStubId());
    }

    // length
    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env, ContainersList::Length, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    SetGetter(thread, listFuncPrototype, lengthKey, lengthGetter);

    // @@iterator
    SetFunctionAtSymbol(env, listFuncPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersList::GetIteratorObj, FuncLength::ONE);

    InitializeListIterator(env);
    env->SetListFunction(thread, listFunction);
    env->SetListConstructor(thread, listFunction);
    return listFunction;
}

JSHandle<JSTaggedValue> ContainersPrivate::InitializeLinkedList(const JSHandle<GlobalEnv> &env)
{
    JSThread *thread = env->GetJSThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // LinkedList.prototype
    JSHandle<JSObject> linkedListFuncPrototype = factory->NewEmptyJSObject(env);
    JSHandle<JSTaggedValue> linkedListFuncPrototypeValue(linkedListFuncPrototype);

    // LinkedList.prototype_or_hclass
    JSHandle<JSHClass> linkedListInstanceClass =
        factory->NewEcmaHClass(JSAPILinkedList::SIZE, JSType::JS_API_LINKED_LIST, linkedListFuncPrototypeValue);

    // LinkedList() = new Function()
    JSHandle<JSTaggedValue> linkedListFunction(NewContainerConstructor(
        env, linkedListFuncPrototype, ContainersLinkedList::LinkedListConstructor,
        "LinkedList", FuncLength::ZERO));
    JSFunction::SetFunctionPrototypeOrInstanceHClass(thread,
        JSHandle<JSFunction>::Cast(linkedListFunction), linkedListInstanceClass.GetTaggedValue());

    // "constructor" property on the prototype
    JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(linkedListFuncPrototype), constructorKey, linkedListFunction);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // LinkedList.prototype methods
    for (const base::BuiltinFunctionEntry &entry : ContainersLinkedList::GetLinkedListPrototypeFunctions()) {
        SetFrozenFunction(env, linkedListFuncPrototype,
                          entry.GetName().data(), entry.GetEntrypoint(), entry.GetLength(), entry.GetBuiltinStubId());
    }

    // length
    JSHandle<JSTaggedValue> lengthGetter = CreateGetter(env, ContainersLinkedList::Length, "length",
                                                        FuncLength::ZERO);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    SetGetter(thread, linkedListFuncPrototype, lengthKey, lengthGetter);

    // @@iterator
    SetFunctionAtSymbol(env, linkedListFuncPrototype, thread->GlobalConstants()->GetHandledIteratorSymbol(),
                        "Symbol.iterator", ContainersLinkedList::GetIteratorObj, FuncLength::ONE);

    InitializeLinkedListIterator(env);
    env->SetLinkedListFunction(thread, linkedListFunction);
    env->SetLinkedListConstructor(thread, linkedListFunction);
    return linkedListFunction;
}

void ContainersPrivate::InitializeContainer(const JSHandle<GlobalEnv> &env)
{
    InitializeTreeSet(env);
    InitializeArrayList(env);
    InitializeHashMap(env);
    InitializeHashSet(env);
    InitializeList(env);
    InitializeLinkedList(env);
}
}  // namespace panda::ecmascript::containers
