/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_shared_map.h"

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsSharedMap::Constructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The ArkTS Map's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSSharedMap> map = JSHandle<JSSharedMap>::Cast(obj);

    // 4.Set map’s [[MapData]] internal slot to a new empty List.
    JSHandle<LinkedHashMap> linkedMap = LinkedHashMap::Create(thread,
        LinkedHashMap::MIN_CAPACITY, MemSpaceKind::SHARED);
    map->SetLinkedMap(thread, linkedMap);
    // add data into set from iterable
    // 5.If iterable is not present, let iterable be undefined.
    // 6.If iterable is either undefined or null, let iter be undefined.
    JSHandle<JSTaggedValue> iterable = GetCallArg(argv, 0);
    if (iterable->IsUndefined() || iterable->IsNull()) {
        return map.GetTaggedValue();
    }
    if (!iterable->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "iterable is not object", JSTaggedValue::Exception());
    }
    // Let adder be Get(map, "set").
    JSHandle<JSTaggedValue> adderKey = thread->GlobalConstants()->GetHandledSetString();
    JSHandle<JSTaggedValue> adder = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(map), adderKey).GetValue();
    // ReturnIfAbrupt(adder).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, adder.GetTaggedValue());
    return AddEntriesFromIterable(thread, obj, iterable, adder, factory);
}

JSTaggedValue BuiltinsSharedMap::Set(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Set);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The set method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);
    JSHandle<JSSharedMap> map(self);
    JSSharedMap::Set(thread, map, key, value);
    return map.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedMap::Clear(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Clear);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The clear method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSSharedMap> map(self);
    JSSharedMap::Clear(thread, map);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSharedMap::Delete(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Delete);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The delete method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSSharedMap> map(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    bool flag = JSSharedMap::Delete(thread, map, key);
    return GetTaggedBoolean(flag);
}

JSTaggedValue BuiltinsSharedMap::Has(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The has method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSSharedMap> map(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    bool flag = JSSharedMap::Has(thread, map, key.GetTaggedValue());
    return GetTaggedBoolean(flag);
}

JSTaggedValue BuiltinsSharedMap::Get(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Get);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The get method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSSharedMap> map(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSTaggedValue value = JSSharedMap::Get(thread, map, key.GetTaggedValue());
    return value;
}

JSTaggedValue BuiltinsSharedMap::ForEach(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedMap, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The forEach method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedMap> scope(thread, self);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    JSHandle<JSSharedMap> map(self);
    JSHandle<JSTaggedValue> func(GetCallArg(argv, 0));
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not Callable", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 1);
    JSMutableHandle<LinkedHashMap> hashMap(thread, map->GetLinkedMap());
    const uint32_t argsLength = 3;
    int index = 0;
    int totalElements = hashMap->NumberOfElements() + hashMap->NumberOfDeletedElements();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    // Repeat for each e that is an element of entries, in original insertion order
    while (index < totalElements) {
        JSHandle<JSTaggedValue> key(thread, hashMap->GetKey(index++));
        if (!key->IsHole()) {
            JSHandle<JSTaggedValue> value(thread, hashMap->GetValue(index - 1));
            EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(
                thread, func, thisArg, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(value.GetTaggedValue(), key.GetTaggedValue(), map.GetTaggedValue());
            JSTaggedValue ret = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ret);
        }
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSharedMap::Species(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Species);
    return GetThis(argv).GetTaggedValue();
}

JSTaggedValue BuiltinsSharedMap::GetSize(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, GetSize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSSharedMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not SharedMap", JSTaggedValue::Exception());
    }
    JSHandle<JSSharedMap> map(self);
    uint32_t size = JSSharedMap::GetSize(thread, map);
    return JSTaggedValue(size);
}

JSTaggedValue BuiltinsSharedMap::Entries(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The entries method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter = JSSharedMapIterator::CreateMapIterator(thread, self, IterationKind::KEY_AND_VALUE);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedMap::Keys(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Keys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The keys method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter = JSSharedMapIterator::CreateMapIterator(thread, self, IterationKind::KEY);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedMap::Values(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), SharedMap, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedMap()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The values method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter = JSSharedMapIterator::CreateMapIterator(thread, self, IterationKind::VALUE);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedMap::AddEntriesFromIterable(JSThread *thread, const JSHandle<JSObject> &target,
    const JSHandle<JSTaggedValue> &iterable, const JSHandle<JSTaggedValue> &adder, ObjectFactory *factory)
{
    BUILTINS_API_TRACE(thread, SharedMap, AddEntriesFromIterable);
    // If IsCallable(adder) is false, throw a TypeError exception
    if (!adder->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "adder is not callable", adder.GetTaggedValue());
    }
    // Let iter be GetIterator(iterable).
    JSHandle<JSTaggedValue> iter(JSIterator::GetIterator(thread, iterable));
    // ReturnIfAbrupt(iter).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, iter.GetTaggedValue());
    JSHandle<JSTaggedValue> keyIndex(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> valueIndex(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> next = JSIterator::IteratorStep(thread, iter);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
    while (!next->IsFalse()) {
        // Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextValue(JSIterator::IteratorValue(thread, next));
        // ReturnIfAbrupt(nextValue).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());

        // If Type(nextItem) is not Object
        if (!nextValue->IsECMAObject()) {
            JSHandle<JSObject> typeError = factory->GetJSError(ErrorType::TYPE_ERROR,
                                                               "nextItem is not Object", StackCheck::NO);
            JSHandle<JSTaggedValue> record(
                factory->NewCompletionRecord(CompletionRecordType::THROW, JSHandle<JSTaggedValue>(typeError)));
            JSTaggedValue ret = JSIterator::IteratorClose(thread, iter, record).GetTaggedValue();
            if (!thread->HasPendingException()) {
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, typeError.GetTaggedValue(), ret);
            }
            return ret;
        }
        // Let k be Get(nextItem, "0").
        JSHandle<JSTaggedValue> key = JSTaggedValue::GetProperty(thread, nextValue, keyIndex).GetValue();
        // If k is an abrupt completion, return IteratorClose(iter, k).
        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter);
        }
        // Let v be Get(nextItem, "1").
        JSHandle<JSTaggedValue> value = JSTaggedValue::GetProperty(thread, nextValue, valueIndex).GetValue();
        // If v is an abrupt completion, return IteratorClose(iter, v).
        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter);
        }
        const uint32_t argsLength = 2;  // 2: key and value pair
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, adder, JSHandle<JSTaggedValue>(target), undefined, argsLength);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
        info->SetCallArg(key.GetTaggedValue(), value.GetTaggedValue());
        JSFunction::Call(info);
        // If status is an abrupt completion, return IteratorClose(iter, status).
        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter);
        }
        // Let next be IteratorStep(iter).
        next = JSIterator::IteratorStep(thread, iter);
        // ReturnIfAbrupt(next).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
    }
    return target.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
