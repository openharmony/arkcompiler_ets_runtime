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

#include "ecmascript/builtins/builtins_shared_set.h"

#include "ecmascript/js_function.h"
#include "ecmascript/interpreter/interpreter.h"

#include "ecmascript/linked_hash_table.h"
#include "ecmascript/shared_objects/concurrent_api_scope.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsSharedSet::Constructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedSet, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If NewTarget is undefined, throw exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = containers::ContainerError::BusinessError(
            thread, containers::ErrorFlag::IS_NULL_ERROR, "The ArkTS Set's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 2.Let set be OrdinaryCreateFromConstructor(NewTarget, "%SetPrototype%", «‍[[SetData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    ASSERT(constructor->IsJSSharedFunction() && constructor.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    JSHandle<JSSharedSet> set = JSHandle<JSSharedSet>::Cast(obj);
    // 3.ReturnIfAbrupt(set).
    // 4.Set set’s [[SetData]] internal slot to a new empty List.
    JSHandle<LinkedHashSet> linkedSet = LinkedHashSet::Create(thread,
        LinkedHashSet::MIN_CAPACITY, MemSpaceKind::SHARED);
    set->SetLinkedSet(thread, linkedSet);
    // add data into set from iterable
    // 5.If iterable is not present, let iterable be undefined.
    // 6.If iterable is either undefined or null, let iter be undefined.
    JSHandle<JSTaggedValue> iterable(GetCallArg(argv, 0));
    // 8.If iter is undefined, return set
    if (iterable->IsUndefined() || iterable->IsNull()) {
        return set.GetTaggedValue();
    }
    // Let adder be Get(set, "add").
    JSHandle<JSTaggedValue> adderKey(thread->GlobalConstants()->GetHandledAddString());
    JSHandle<JSTaggedValue> setHandle(set);
    JSHandle<JSTaggedValue> adder = JSObject::GetProperty(thread, setHandle, adderKey).GetValue();
    // ReturnIfAbrupt(adder).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, adder.GetTaggedValue());
    // If IsCallable(adder) is false, throw a TypeError exception
    if (!adder->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "adder is not callable", adder.GetTaggedValue());
    }
    // Let iter be GetIterator(iterable).
    JSHandle<JSTaggedValue> iter(JSIterator::GetIterator(thread, iterable));
    // ReturnIfAbrupt(iter).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, iter.GetTaggedValue());
    // values in iterator_result may be a JSArray, values[0] = key values[1]=value, used valueIndex to get value from
    // jsarray
    JSHandle<JSTaggedValue> valueIndex(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> next = JSIterator::IteratorStep(thread, iter);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
    while (!next->IsFalse()) {
        // Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextValue(JSIterator::IteratorValue(thread, next));
        // ReturnIfAbrupt(nextValue).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, nextValue.GetTaggedValue());
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, adder, setHandle, undefined, 1);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, nextValue.GetTaggedValue());
        info->SetCallArg(nextValue.GetTaggedValue());
        if (nextValue->IsArray(thread)) {
            auto prop = JSTaggedValue::GetProperty(thread, nextValue, valueIndex).GetValue();
            info->SetCallArg(prop.GetTaggedValue());
        }
        JSFunction::Call(info);
        // Let status be Call(adder, set, «nextValue.[[value]]»).
        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter);
        }
        // Let next be IteratorStep(iter).
        next = JSIterator::IteratorStep(thread, iter);
        // ReturnIfAbrupt(next).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
    }
    return set.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedSet::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedSet, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedSet()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The add method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSHandle<JSSharedSet> set(self);
    JSSharedSet::Add(thread, set, value);
    return set.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedSet::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedSet, Clear);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedSet()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The clear method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSSharedSet> set(self);
    JSSharedSet::Clear(thread, set);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSharedSet::Delete(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedSet, Delete);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedSet()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The delete method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSSharedSet> set(self);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    bool flag = JSSharedSet::Delete(thread, set, value);
    return GetTaggedBoolean(flag);
}

JSTaggedValue BuiltinsSharedSet::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedSet, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedSet()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The has method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSSharedSet> set(self);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    bool flag = JSSharedSet::Has(thread, set, value.GetTaggedValue());
    return GetTaggedBoolean(flag);
}

JSTaggedValue BuiltinsSharedSet::ForEach(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SharedSet, ForEach);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedSet()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The forEach method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    [[maybe_unused]] ConcurrentApiScope<JSSharedSet> scope(thread, self);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());

    JSHandle<JSSharedSet> set(self);
    JSHandle<JSTaggedValue> func(GetCallArg(argv, 0));
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "callback is not callable", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 1);
    JSMutableHandle<LinkedHashSet> hashSet(thread, set->GetLinkedSet());
    const uint32_t argsLength = 3;
    int index = 0;
    int totalElements = hashSet->NumberOfElements() + hashSet->NumberOfDeletedElements();
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    // Repeat for each e that is an element of entries, in original insertion order
    while (index < totalElements) {
        JSHandle<JSTaggedValue> key(thread, hashSet->GetKey(index++));
        if (!key->IsHole()) {
            EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(
                thread, func, thisArg, undefined, argsLength);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            info->SetCallArg(key.GetTaggedValue(), key.GetTaggedValue(), set.GetTaggedValue());
            JSTaggedValue ret = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ret);
        }
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSharedSet::Species(EcmaRuntimeCallInfo *argv)
{
    return GetThis(argv).GetTaggedValue();
}

JSTaggedValue BuiltinsSharedSet::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedSet, GetSize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    if (!self->IsJSSharedSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not SharedSet", JSTaggedValue::Exception());
    }
    JSHandle<JSSharedSet> set(self);
    uint32_t size = JSSharedSet::GetSize(thread, set);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue(0));
    return JSTaggedValue(size);
}

JSTaggedValue BuiltinsSharedSet::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedSet, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedSet()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The entries method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter = JSSharedSetIterator::CreateSetIterator(thread, self, IterationKind::KEY_AND_VALUE);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsSharedSet::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SharedSet, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSSharedSet()) {
        auto error = containers::ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                               "The values method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> iter = JSSharedSetIterator::CreateSetIterator(thread, self, IterationKind::VALUE);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Undefined());
    return iter.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
