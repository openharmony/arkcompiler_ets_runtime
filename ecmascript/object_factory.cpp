/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/native_module_failure_info.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#include "ecmascript/js_api/js_api_bitvector_iterator.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_api/js_api_lightweightset_iterator.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_plain_array_iterator.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_queue_iterator.h"
#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_map_iterator.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_tree_set_iterator.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_async_from_sync_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/marker_cell.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/require/js_cjs_exports.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/shared_mm/shared_mm.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/shared_objects/js_shared_array_iterator.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"
#include "ecmascript/shared_objects/js_shared_typed_array.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/template_map.h"
#include "ecmascript/vtable.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/js_segmenter.h"
#include "ecmascript/js_segments.h"
#include "ecmascript/js_segment_iterator.h"
#endif
namespace panda::ecmascript {
using Error = builtins::BuiltinsError;
using RangeError = builtins::BuiltinsRangeError;
using ReferenceError = builtins::BuiltinsReferenceError;
using TypeError = builtins::BuiltinsTypeError;
using AggregateError = builtins::BuiltinsAggregateError;
using URIError = builtins::BuiltinsURIError;
using SyntaxError = builtins::BuiltinsSyntaxError;
using EvalError = builtins::BuiltinsEvalError;
using OOMError = builtins::BuiltinsOOMError;
using ErrorType = base::ErrorType;
using ErrorHelper = base::ErrorHelper;

ObjectFactory::ObjectFactory(JSThread *thread, Heap *heap, SharedHeap *sHeap)
    : thread_(thread), vm_(thread->GetEcmaVM()), heap_(heap), sHeap_(sHeap) {}

JSHandle<Method> ObjectFactory::NewMethodForNativeFunction(const void *func, FunctionKind kind,
                                                           kungfu::BuiltinsStubCSigns::ID builtinId,
                                                           MemSpaceType methodSpaceType)
{
    uint32_t numArgs = 2;  // function object and this
    auto method = NewSMethod(nullptr, methodSpaceType);
    method->SetNativePointer(const_cast<void *>(func));
    method->SetNativeBit(true);
    if (builtinId != kungfu::BuiltinsStubCSigns::INVALID) {
        bool isFast = kungfu::BuiltinsStubCSigns::IsFastBuiltin(builtinId);
        method->SetFastBuiltinBit(isFast);
        method->SetBuiltinId(static_cast<uint8_t>(builtinId));
    }
    method->SetNumArgsWithCallField(numArgs);
    method->SetFunctionKind(kind);
    return method;
}

JSHandle<JSHClass> ObjectFactory::NewEcmaHClassClass(JSHClass *hclass, uint32_t size, JSType type)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateClassClass(hclass, classSize));
    newClass->Initialize(thread_, size, type, 0);

    return JSHandle<JSHClass>(thread_, newClass);
}

JSHandle<JSHClass> ObjectFactory::InitClassClass()
{
    JSHandle<JSHClass> hClassHandle = NewEcmaHClassClass(nullptr, JSHClass::SIZE, JSType::HCLASS);
    JSHClass *hclass = reinterpret_cast<JSHClass *>(hClassHandle.GetTaggedValue().GetTaggedObject());
    hclass->SetClass(thread_, hclass);
    return hClassHandle;
}

JSHandle<JSHClass> ObjectFactory::NewEcmaHClass(JSHClass *hclass, uint32_t size, JSType type, uint32_t inlinedProps)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateNonMovableOrHugeObject(hclass, classSize));
    newClass->Initialize(thread_, size, type, inlinedProps);

    return JSHandle<JSHClass>(thread_, newClass);
}

// This function don't UpdateProtoClass
JSHandle<JSHClass> ObjectFactory::NewEcmaHClass(uint32_t size, uint32_t inlinedProps, JSType type,
    const JSHandle<JSTaggedValue> &prototype, const JSHandle<JSTaggedValue> &layout)
{
    NewSObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject()), classSize));
    newClass->Initialize(thread_, size, type, inlinedProps, layout);
    JSHandle<JSHClass> hclass(thread_, newClass);
    if (prototype->IsJSObject()) {
        prototype->GetTaggedObject()->GetClass()->SetIsPrototype(true);
    }
    hclass->SetProto(thread_, prototype.GetTaggedValue());
    hclass->SetNumberOfProps(inlinedProps);
    hclass->SetExtensible(false);
    return hclass;
}

JSHandle<JSHClass> ObjectFactory::NewEcmaReadOnlyHClass(JSHClass *hclass, uint32_t size, JSType type,
                                                        uint32_t inlinedProps)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateReadOnlyOrHugeObject(hclass, classSize));
    ASSERT(newClass != nullptr);
    newClass->Initialize(thread_, size, type, inlinedProps);

    return JSHandle<JSHClass>(thread_, newClass);
}

JSHandle<JSHClass> ObjectFactory::NewEcmaHClass(uint32_t size, JSType type, uint32_t inlinedProps)
{
    return NewEcmaHClass(JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject()),
                         size, type, inlinedProps);
}

void ObjectFactory::InitObjectFields(const TaggedObject *object)
{
    auto *klass = object->GetClass();
    auto objBodySize = klass->GetObjectSize() - TaggedObject::TaggedObjectSize();
    ASSERT(objBodySize % JSTaggedValue::TaggedTypeSize() == 0);
    uint32_t numOfFields = objBodySize / JSTaggedValue::TaggedTypeSize();
    size_t addr = reinterpret_cast<uintptr_t>(object) + TaggedObject::TaggedObjectSize();
    for (uint32_t i = 0; i < numOfFields; i++) {
        auto *fieldAddr = reinterpret_cast<JSTaggedType *>(addr + i * JSTaggedValue::TaggedTypeSize());
        *fieldAddr = JSTaggedValue::Undefined().GetRawData();
    }
}

void ObjectFactory::NewJSArrayBufferData(const JSHandle<JSArrayBuffer> &array, int32_t length)
{
    if (length == 0) {
        return;
    }

    JSTaggedValue data = array->GetArrayBufferData();
    size_t size = static_cast<size_t>(length) * sizeof(uint8_t);
    if (!data.IsUndefined()) {
        auto *pointer = JSNativePointer::Cast(data.GetTaggedObject());
        auto newData = vm_->GetNativeAreaAllocator()->AllocateBuffer(size);
        heap_->IncNativeSizeAfterLastGC(size);
        if (memset_s(newData, length, 0, length) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        pointer->ResetExternalPointer(thread_, newData);
        vm_->GetNativeAreaAllocator()->ModifyNativeSizeStats(pointer->GetBindingSize(), size,
                                                             NativeFlag::ARRAY_BUFFER);
        return;
    }

    auto newData = vm_->GetNativeAreaAllocator()->AllocateBuffer(size);
    heap_->IncNativeSizeAfterLastGC(size);
    if (memset_s(newData, length, 0, length) != EOK) {
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(newData, NativeAreaAllocator::FreeBufferFunc,
                                                           vm_->GetNativeAreaAllocator(), false, size, Concurrent::NO,
                                                           NativeFlag::ARRAY_BUFFER);
    array->SetArrayBufferData(thread_, pointer);
    array->SetWithNativeAreaAllocator(true);
    vm_->GetNativeAreaAllocator()->IncreaseNativeSizeStats(length, NativeFlag::ARRAY_BUFFER);
}

void ObjectFactory::NewJSSendableArrayBufferData(const JSHandle<JSSendableArrayBuffer> &array, int32_t length)
{
    if (length == 0) {
        return;
    }

    JSTaggedValue data = array->GetArrayBufferData();
    size_t size = static_cast<size_t>(length) * sizeof(uint8_t);
    NativeAreaAllocator *nativeAreaAllocator = sHeap_->GetNativeAreaAllocator();
    if (!data.IsUndefined()) {
        auto *pointer = JSNativePointer::Cast(data.GetTaggedObject());
        auto newData = nativeAreaAllocator->AllocateBuffer(size);
        if (newData == nullptr) {
            LOG_ECMA(FATAL) << "ObjectFactory::NewJSSendableArrayBufferData:newData is nullptr";
        }
        if (memset_s(newData, length, 0, length) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        pointer->ResetExternalPointer(thread_, newData);
        nativeAreaAllocator->ModifyNativeSizeStats(pointer->GetBindingSize(), size,
                                                   NativeFlag::ARRAY_BUFFER);
        return;
    }

    auto newData = nativeAreaAllocator->AllocateBuffer(size);
    if (newData == nullptr) {
        LOG_ECMA(FATAL) << "ObjectFactory::NewJSSendableArrayBufferData:newData is nullptr";
    }
    if (memset_s(newData, length, 0, length) != EOK) {
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
    JSHandle<JSNativePointer> pointer = NewSJSNativePointer(newData, NativeAreaAllocator::FreeBufferFunc,
                                                            nativeAreaAllocator, false, size,
                                                            NativeFlag::ARRAY_BUFFER);
    array->SetArrayBufferData(thread_, pointer);
    array->SetWithNativeAreaAllocator(true);
    nativeAreaAllocator->IncreaseNativeSizeStats(length, NativeFlag::ARRAY_BUFFER);
}

JSHandle<JSSendableArrayBuffer> ObjectFactory::NewJSSendableArrayBuffer(int32_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetSBuiltininArrayBufferFunction());
    JSHandle<JSSendableArrayBuffer> sendableArrayBuffer(NewJSObjectByConstructor(constructor));
    sendableArrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        NewJSSendableArrayBufferData(sendableArrayBuffer, length);
        sendableArrayBuffer->SetShared(false);
    }
    return sendableArrayBuffer;
}

JSHandle<JSSendableArrayBuffer> ObjectFactory::NewJSSendableArrayBuffer(void *buffer, int32_t length,
                                                                        const NativePointerCallback &deleter,
                                                                        void *data)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetSBuiltininArrayBufferFunction());
    JSHandle<JSSendableArrayBuffer> sendableArrayBuffer(NewJSObjectByConstructor(constructor));
    length = buffer == nullptr ? 0 : length;
    sendableArrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        JSHandle<JSNativePointer> pointer = NewSJSNativePointer(buffer, deleter, data, false, length);
        sendableArrayBuffer->SetArrayBufferData(thread_, pointer.GetTaggedValue());
        sendableArrayBuffer->SetShared(false);
        sendableArrayBuffer->SetWithNativeAreaAllocator(deleter == NativeAreaAllocator::FreeBufferFunc &&
                                                data == vm_->GetNativeAreaAllocator());
    }
    return sendableArrayBuffer;
}

void ObjectFactory::NewJSSharedArrayBufferData(const JSHandle<JSArrayBuffer> &array, int32_t length)
{
    if (length == 0) {
        return;
    }
    void *newData = nullptr;
    size_t size =
        JSSharedMemoryManager::GetInstance()->CreateOrLoad(&newData, length) ? static_cast<size_t>(length) : 0U;
    if (memset_s(newData, length, 0, length) != EOK) {
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(newData, JSSharedMemoryManager::RemoveSharedMemory,
                                                           JSSharedMemoryManager::GetInstance(), false, size);
    array->SetArrayBufferData(thread_, pointer);
    array->SetWithNativeAreaAllocator(false);
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSArrayBuffer(int32_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetArrayBufferFunction());
    JSHandle<JSArrayBuffer> arrayBuffer(NewJSObjectByConstructor(constructor));
    arrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        auto newData = vm_->GetNativeAreaAllocator()->AllocateBuffer(length);
        heap_->IncNativeSizeAfterLastGC(length);
        if (memset_s(newData, length, 0, length) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        JSHandle<JSNativePointer> pointer = NewJSNativePointer(newData, NativeAreaAllocator::FreeBufferFunc,
                                                               vm_->GetNativeAreaAllocator(), false, length,
                                                               Concurrent::NO, NativeFlag::ARRAY_BUFFER);
        arrayBuffer->SetArrayBufferData(thread_, pointer.GetTaggedValue());
        arrayBuffer->SetWithNativeAreaAllocator(true);
        vm_->GetNativeAreaAllocator()->IncreaseNativeSizeStats(length, NativeFlag::ARRAY_BUFFER);
    }
    return arrayBuffer;
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSArrayBuffer(void *buffer, int32_t length,
                                                        const NativePointerCallback &deleter, void *data, bool share)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetArrayBufferFunction());
    JSHandle<JSArrayBuffer> arrayBuffer(NewJSObjectByConstructor(constructor));
    length = buffer == nullptr ? 0 : length;
    arrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        JSHandle<JSNativePointer> pointer = NewJSNativePointer(buffer, deleter, data, false, length);
        arrayBuffer->SetArrayBufferData(thread_, pointer.GetTaggedValue());
        arrayBuffer->SetShared(share);
        arrayBuffer->SetWithNativeAreaAllocator(deleter == NativeAreaAllocator::FreeBufferFunc &&
                                                data == vm_->GetNativeAreaAllocator());
    }
    return arrayBuffer;
}

JSHandle<JSDataView> ObjectFactory::NewJSDataView(JSHandle<JSArrayBuffer> buffer, uint32_t offset, uint32_t length)
{
    uint32_t arrayLength = buffer->GetArrayBufferByteLength();
    if (arrayLength < static_cast<uint64_t>(offset) + length) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "offset or length error",
                                    JSHandle<JSDataView>(thread_, JSTaggedValue::Undefined()));
    }
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetDataViewFunction());
    JSHandle<JSDataView> arrayBuffer(NewJSObjectByConstructor(constructor));
    arrayBuffer->SetDataView(thread_, JSTaggedValue::True());
    arrayBuffer->SetViewedArrayBuffer(thread_, buffer.GetTaggedValue());
    arrayBuffer->SetByteLength(length);
    arrayBuffer->SetByteOffset(offset);
    return arrayBuffer;
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSSharedArrayBuffer(int32_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetSharedArrayBufferFunction());
    JSHandle<JSArrayBuffer> sharedArrayBuffer(NewJSObjectByConstructor(constructor));
    sharedArrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        NewJSSharedArrayBufferData(sharedArrayBuffer, length);
        sharedArrayBuffer->SetShared(true);
    }
    return sharedArrayBuffer;
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSSharedArrayBuffer(void *buffer, int32_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSFunction> constructor(env->GetSharedArrayBufferFunction());
    JSHandle<JSArrayBuffer> sharedArrayBuffer(NewJSObjectByConstructor(constructor));
    length = buffer == nullptr ? 0 : length;
    sharedArrayBuffer->SetArrayBufferByteLength(length);
    if (length > 0) {
        JSHandle<JSNativePointer> pointer = NewJSNativePointer(buffer, JSSharedMemoryManager::RemoveSharedMemory,
                                                               JSSharedMemoryManager::GetInstance(), false, length);
        sharedArrayBuffer->SetArrayBufferData(thread_, pointer);
        sharedArrayBuffer->SetShared(true);
        sharedArrayBuffer->SetWithNativeAreaAllocator(false);
    }
    return sharedArrayBuffer;
}

void ObjectFactory::NewJSRegExpByteCodeData(const JSHandle<JSRegExp> &regexp, void *buffer, size_t size)
{
    if (buffer == nullptr) {
        return;
    }

    auto newBuffer = vm_->GetNativeAreaAllocator()->AllocateBuffer(size);
    heap_->IncNativeSizeAfterLastGC(size);
    if (memcpy_s(newBuffer, size, buffer, size) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    JSTaggedValue data = regexp->GetByteCodeBuffer();
    if (!data.IsUndefined()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread_, newBuffer);
        return;
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(newBuffer, NativeAreaAllocator::FreeBufferFunc,
                                                           vm_->GetNativeAreaAllocator(), false, size, Concurrent::NO,
                                                           NativeFlag::REGEXP_BTYECODE);
    regexp->SetByteCodeBuffer(thread_, pointer.GetTaggedValue());
    regexp->SetLength(static_cast<uint32_t>(size));
    vm_->GetNativeAreaAllocator()->IncreaseNativeSizeStats(size, NativeFlag::REGEXP_BTYECODE);
}

JSHandle<JSHClass> ObjectFactory::NewEcmaHClass(uint32_t size, JSType type, const JSHandle<JSTaggedValue> &prototype)
{
    const int inlinedProps = JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS;
    return NewEcmaHClass(size, inlinedProps, type, prototype);
}

JSHandle<JSHClass> ObjectFactory::NewEcmaHClass(uint32_t size, uint32_t inlinedProps, JSType type,
                                                const JSHandle<JSTaggedValue> &prototype)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject()), classSize));
    newClass->Initialize(thread_, size, type, inlinedProps);
    JSHandle<JSHClass> hclass(thread_, newClass);
    hclass->SetPrototype(thread_, prototype.GetTaggedValue());
    return hclass;
}

JSHandle<JSObject> ObjectFactory::NewJSObject(const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSObject> obj(thread_, JSObject::Cast(NewObject(jshclass)));
    JSHandle<TaggedArray> emptyArray = EmptyArray();
    obj->InitializeHash();
    obj->SetElements(thread_, emptyArray, SKIP_BARRIER);
    obj->SetProperties(thread_, emptyArray, SKIP_BARRIER);
    return obj;
}

JSHandle<TaggedArray> ObjectFactory::CloneProperties(const JSHandle<TaggedArray> &old)
{
    uint32_t newLength = old->GetLength();
    if (newLength == 0) {
        return EmptyArray();
    }
    NewObjectHook();
    auto klass = old->GetClass();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(klass, size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->InitializeWithSpecialValue(JSTaggedValue::Hole(), newLength, old->GetExtraLength());
    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i);
        newArray->Set(thread_, i, value);
    }
    return newArray;
}

JSHandle<JSObject> ObjectFactory::CloneObjectLiteral(JSHandle<JSObject> object)
{
    NewObjectHook();
    auto klass = JSHandle<JSHClass>(thread_, object->GetClass());

    JSHandle<JSObject> cloneObject = NewJSObject(klass);

    JSHandle<TaggedArray> elements(thread_, object->GetElements());
    auto newElements = CloneProperties(elements);
    cloneObject->SetElements(thread_, newElements.GetTaggedValue());

    JSHandle<TaggedArray> properties(thread_, object->GetProperties());
    auto newProperties = CloneProperties(properties);
    cloneObject->SetProperties(thread_, newProperties.GetTaggedValue());
    uint32_t length = std::min(klass->GetInlinedProperties(), klass->NumberOfProps());

    for (uint32_t i = 0; i < length; i++) {
        cloneObject->SetPropertyInlinedPropsWithRep(thread_, i, object->GetPropertyInlinedProps(i));
    }
    return cloneObject;
}

JSHandle<JSArray> ObjectFactory::CloneArrayLiteral(JSHandle<JSArray> object)
{
    NewObjectHook();
    auto klass = JSHandle<JSHClass>(thread_, object->GetClass());

    JSHandle<JSArray> cloneObject(NewJSObject(klass));
    cloneObject->SetArrayLength(thread_, object->GetArrayLength());
    cloneObject->SetTrackInfo(thread_, JSTaggedValue::Undefined());

    JSHandle<TaggedArray> elements(thread_, object->GetElements());
    static constexpr uint8_t MAX_READ_ONLY_ARRAY_LENGTH = 10;
    uint32_t elementsLength = elements->GetLength();
    MemSpaceType type = elementsLength > MAX_READ_ONLY_ARRAY_LENGTH ?
        MemSpaceType::SEMI_SPACE : MemSpaceType::NON_MOVABLE;

#if !defined ENABLE_COW_ARRAY
    type = MemSpaceType::SEMI_SPACE;
#endif

    if (type == MemSpaceType::NON_MOVABLE && elements.GetTaggedValue().IsCOWArray()) {
        // share the same elements array in nonmovable space.
        cloneObject->SetElements(thread_, elements.GetTaggedValue());
    } else {
        auto newElements = CopyArray(elements, elementsLength, elementsLength, JSTaggedValue::Hole(), type);
        cloneObject->SetElements(thread_, newElements.GetTaggedValue());
    }

    if (type == MemSpaceType::NON_MOVABLE && !object->GetElements().IsCOWArray()) {
        ASSERT(!Region::ObjectAddressToRange(object->GetElements().GetTaggedObject())->InNonMovableSpace());
        // Set the first shared elements into the old object.
        object->SetElements(thread_, cloneObject->GetElements());
    }

    JSHandle<TaggedArray> properties(thread_, object->GetProperties());
    uint32_t propertiesLength = properties->GetLength();
    type = propertiesLength > MAX_READ_ONLY_ARRAY_LENGTH ?
        MemSpaceType::SEMI_SPACE : MemSpaceType::NON_MOVABLE;

#if !defined ENABLE_COW_ARRAY
    type = MemSpaceType::SEMI_SPACE;
#endif

    if (type == MemSpaceType::NON_MOVABLE && properties.GetTaggedValue().IsCOWArray()) {
        // share the same properties array in nonmovable space.
        cloneObject->SetProperties(thread_, properties.GetTaggedValue());
    } else {
        auto newProperties = CopyArray(properties, propertiesLength, propertiesLength, JSTaggedValue::Hole(), type);
        cloneObject->SetProperties(thread_, newProperties.GetTaggedValue());
    }

    if (type == MemSpaceType::NON_MOVABLE && !object->GetProperties().IsCOWArray()) {
        ASSERT(!Region::ObjectAddressToRange(object->GetProperties().GetTaggedObject())->InNonMovableSpace());
        // Set the first shared properties into the old object.
        object->SetProperties(thread_, cloneObject->GetProperties());
    }

    for (uint32_t i = 0; i < klass->GetInlinedProperties(); i++) {
        cloneObject->SetPropertyInlinedPropsWithRep(thread_, i, object->GetPropertyInlinedProps(i));
    }
    return cloneObject;
}

JSHandle<TaggedArray> ObjectFactory::CloneProperties(const JSHandle<TaggedArray> &old,
                                                     const JSHandle<JSTaggedValue> &env, const JSHandle<JSObject> &obj)
{
    uint32_t newLength = old->GetLength();
    if (newLength == 0) {
        return EmptyArray();
    }
    NewObjectHook();
    auto klass = old->GetClass();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(klass, size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->InitializeWithSpecialValue(JSTaggedValue::Hole(), newLength, old->GetExtraLength());

    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i);
        if (!value.IsJSFunction()) {
            newArray->Set(thread_, i, value);
        } else {
            JSHandle<JSFunction> valueHandle(thread_, value);
            JSHandle<JSFunction> newFunc = CloneJSFunction(valueHandle);
            newFunc->SetLexicalEnv(thread_, env);
            newFunc->SetHomeObject(thread_, obj);
            newArray->Set(thread_, i, newFunc);
        }
    }
    return newArray;
}

JSHandle<JSObject> ObjectFactory::CloneObjectLiteral(JSHandle<JSObject> object,
                                                     const JSHandle<JSTaggedValue> &env,
                                                     bool canShareHClass)
{
    NewObjectHook();
    auto klass = JSHandle<JSHClass>(thread_, object->GetClass());

    if (!canShareHClass) {
        klass = JSHClass::Clone(thread_, klass);
    }

    JSHandle<JSObject> cloneObject = NewJSObject(klass);

    JSHandle<TaggedArray> elements(thread_, object->GetElements());
    auto newElements = CloneProperties(elements, env, cloneObject);
    cloneObject->SetElements(thread_, newElements.GetTaggedValue());

    JSHandle<TaggedArray> properties(thread_, object->GetProperties());
    auto newProperties = CloneProperties(properties, env, cloneObject);
    cloneObject->SetProperties(thread_, newProperties.GetTaggedValue());
    uint32_t length = std::min(klass->GetInlinedProperties(), klass->NumberOfProps());

    for (uint32_t i = 0; i < length; i++) {
        auto layout = LayoutInfo::Cast(klass->GetLayout().GetTaggedObject());
        JSTaggedValue value = object->GetPropertyInlinedProps(i);
        if (!layout->GetAttr(i).IsTaggedRep() || (!value.IsJSFunction() && !value.IsAccessorData())) {
            cloneObject->SetPropertyInlinedPropsWithRep(thread_, i, value);
        } else if (value.IsJSFunction()) {
            JSHandle<JSFunction> valueHandle(thread_, value);
            JSHandle<JSFunction> newFunc = CloneJSFunction(valueHandle);
            newFunc->SetLexicalEnv(thread_, env);
            newFunc->SetHomeObject(thread_, cloneObject);
            cloneObject->SetPropertyInlinedProps(thread_, i, newFunc.GetTaggedValue());
        } else {
            if (value.IsAccessorData()) {
                JSHandle<AccessorData> accessor = NewAccessorData();
                value = accessor.GetTaggedValue();
            }
            cloneObject->SetPropertyInlinedPropsWithRep(thread_, i, value);
        }
    }
    return cloneObject;
}

JSHandle<JSFunction> ObjectFactory::CloneJSFunction(JSHandle<JSFunction> func)
{
    JSHandle<JSHClass> jshclass(thread_, func->GetJSHClass());
    JSHandle<Method> method(thread_, func->GetMethod());
    JSHandle<JSFunction> cloneFunc = NewJSFunctionByHClass(method, jshclass);

    JSTaggedValue length = func->GetPropertyInlinedProps(JSFunction::LENGTH_INLINE_PROPERTY_INDEX);
    cloneFunc->SetPropertyInlinedProps(thread_, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, length);
    cloneFunc->SetLength(func->GetLength());
    cloneFunc->SetModule(thread_, func->GetModule());
    return cloneFunc;
}

JSHandle<JSFunction> ObjectFactory::CloneSFunction(JSHandle<JSFunction> func)
{
    ASSERT(func.GetTaggedValue().IsJSSharedFunction());
    JSHandle<JSHClass> jshclass(thread_, func->GetJSHClass());
    JSHandle<Method> method(thread_, func->GetMethod());
    JSHandle<JSFunction> cloneFunc = NewSFunctionByHClass(method, jshclass);

    JSTaggedValue length = func->GetPropertyInlinedProps(JSFunction::LENGTH_INLINE_PROPERTY_INDEX);
    cloneFunc->SetPropertyInlinedProps(thread_, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, length);
    cloneFunc->SetLength(func->GetLength());
    cloneFunc->SetModule(thread_, func->GetModule());
    return cloneFunc;
}

JSHandle<JSFunction> ObjectFactory::CreateJSFunctionFromTemplate(JSHandle<FunctionTemplate> funcTemp)
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    auto kind = funcTemp->GetFunctionKind();
    JSHandle<JSHClass> jshclass;
    if (kind == FunctionKind::NORMAL_FUNCTION ||
        kind == FunctionKind::GETTER_FUNCTION ||
        kind == FunctionKind::SETTER_FUNCTION) {
        jshclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    } else if (kind == FunctionKind::ASYNC_FUNCTION) {
        jshclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
    } else {
        jshclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    }
    JSHandle<Method> method = JSHandle<Method>(thread_, funcTemp->GetMethod());
    JSHandle<JSFunction> newFunc = NewJSFunctionByHClass(method, jshclass);

    newFunc->SetLength(funcTemp->GetLength());
    newFunc->SetModule(thread_, funcTemp->GetModule());
    return newFunc;
}

JSHandle<JSFunction> ObjectFactory::CreateSFunctionFromTemplate(JSHandle<FunctionTemplate> funcTemp)
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    auto kind = funcTemp->GetFunctionKind();
    JSHandle<JSHClass> jshclass;
    if (kind == FunctionKind::NORMAL_FUNCTION ||
        kind == FunctionKind::GETTER_FUNCTION ||
        kind == FunctionKind::SETTER_FUNCTION) {
        jshclass = JSHandle<JSHClass>::Cast(env->GetSFunctionClassWithoutProto());
    } else if (kind == FunctionKind::ASYNC_FUNCTION) {
        jshclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
    } else {
        jshclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    }
    JSHandle<Method> method(thread_, funcTemp->GetMethod());
    JSHandle<JSFunction> newFunc = NewSFunctionByHClass(method, jshclass);

    newFunc->SetLength(funcTemp->GetLength());
    newFunc->SetModule(thread_, funcTemp->GetModule());
    return newFunc;
}

JSHandle<JSFunction> ObjectFactory::CloneClassCtor(JSHandle<JSFunction> ctor, const JSHandle<JSTaggedValue> &lexenv,
                                                   bool canShareHClass)
{
    NewObjectHook();
    JSHandle<JSHClass> hclass(thread_, ctor->GetClass());

    if (!canShareHClass) {
        hclass = JSHClass::Clone(thread_, hclass);
    }

    JSHandle<Method> method(thread_, ctor->GetMethod());
    ASSERT_PRINT(method->GetFunctionKind() == FunctionKind::CLASS_CONSTRUCTOR ||
                 method->GetFunctionKind() == FunctionKind::DERIVED_CONSTRUCTOR,
                 "cloned function is not class");
    JSHandle<JSFunction> cloneCtor = NewJSFunctionByHClass(method, hclass);
    uint32_t length = std::min(hclass->GetInlinedProperties(), hclass->NumberOfProps());

    for (uint32_t i = 0; i < length; i++) {
        auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
        JSTaggedValue value = ctor->GetPropertyInlinedProps(i);
        if (!layout->GetAttr(i).IsTaggedRep() || !value.IsJSFunction()) {
            cloneCtor->SetPropertyInlinedPropsWithRep(thread_, i, value);
        } else {
            JSHandle<JSFunction> valueHandle(thread_, value);
            JSHandle<JSFunction> newFunc = CloneJSFunction(valueHandle);
            newFunc->SetLexicalEnv(thread_, lexenv);
            newFunc->SetHomeObject(thread_, cloneCtor);
            cloneCtor->SetPropertyInlinedProps(thread_, i, newFunc.GetTaggedValue());
        }
    }

    JSHandle<TaggedArray> elements(thread_, ctor->GetElements());
    auto newElements = CloneProperties(elements, lexenv, JSHandle<JSObject>(cloneCtor));
    cloneCtor->SetElements(thread_, newElements.GetTaggedValue());

    JSHandle<TaggedArray> properties(thread_, ctor->GetProperties());
    auto newProperties = CloneProperties(properties, lexenv, JSHandle<JSObject>(cloneCtor));
    cloneCtor->SetProperties(thread_, newProperties.GetTaggedValue());

    return cloneCtor;
}

JSHandle<JSObject> ObjectFactory::NewNonMovableJSObject(const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSObject> obj(thread_,
                           JSObject::Cast(NewNonMovableObject(jshclass, jshclass->GetInlinedProperties())));
    obj->InitializeHash();
    obj->SetElements(thread_, EmptyArray(), SKIP_BARRIER);
    obj->SetProperties(thread_, EmptyArray(), SKIP_BARRIER);
    return obj;
}

JSHandle<JSPrimitiveRef> ObjectFactory::NewJSPrimitiveRef(const JSHandle<JSHClass> &hclass,
                                                          const JSHandle<JSTaggedValue> &object)
{
    JSHandle<JSPrimitiveRef> obj = JSHandle<JSPrimitiveRef>::Cast(NewJSObject(hclass));
    obj->SetValue(thread_, object);
    return obj;
}

JSHandle<JSArray> ObjectFactory::NewJSArray()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> function(env->GetArrayFunction());
    return JSHandle<JSArray>(NewJSObjectByConstructor(function));
}

JSHandle<JSArray> ObjectFactory::NewJSArray(size_t length, JSHandle<JSHClass> &hclass)
{
    JSHandle<JSObject> obj = NewJSObject(hclass);
    JSArray::Cast(*obj)->SetLength(length);
    JSArray::Cast(*obj)->SetTrackInfo(thread_, JSTaggedValue::Undefined());
    auto accessor = thread_->GlobalConstants()->GetArrayLengthAccessor();
    JSArray::Cast(*obj)->SetPropertyInlinedProps(thread_, JSArray::LENGTH_INLINE_PROPERTY_INDEX, accessor);
    return JSHandle<JSArray>(obj);
}

JSHandle<TaggedArray> ObjectFactory::NewJsonFixedArray(size_t start, size_t length,
                                                       const std::vector<JSHandle<JSTaggedValue>> &vec)
{
    if (length == 0) {
        return EmptyArray();
    }

    MemSpaceType spaceType = length < LENGTH_THRESHOLD ? MemSpaceType::SEMI_SPACE : MemSpaceType::OLD_SPACE;
    JSHandle<TaggedArray> array = NewTaggedArrayWithoutInit(length, spaceType);
    array->SetExtraLength(0);
    for (size_t i = 0; i < length; i++) {
        array->Set(thread_, i, vec[start + i]);
    }
    return array;
}

JSHandle<JSForInIterator> ObjectFactory::NewJSForinIterator(const JSHandle<JSTaggedValue> &obj,
                                                            const JSHandle<JSTaggedValue> keys,
                                                            const JSHandle<JSTaggedValue> cachedHclass)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass(env->GetForinIteratorClass());

    JSHandle<JSForInIterator> it = JSHandle<JSForInIterator>::Cast(NewJSObject(hclass));
    it->SetObject(thread_, obj);
    it->SetCachedHclass(thread_, cachedHclass);
    it->SetKeys(thread_, keys);
    it->SetIndex(EnumCache::ENUM_CACHE_HEADER_SIZE);
    uint32_t enumLength = JSHandle<TaggedArray>::Cast(keys)->GetLength();
    it->SetLength(enumLength);
    return it;
}

JSHandle<JSHClass> ObjectFactory::CreateJSRegExpInstanceClass(JSHandle<JSTaggedValue> proto)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> regexpClass = NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);

    uint32_t fieldOrder = 0;
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(1);
    {
        PropertyAttributes attributes = PropertyAttributes::Default(true, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, 0, globalConst->GetLastIndexString(), attributes);
    }

    {
        regexpClass->SetLayout(thread_, layoutInfoHandle);
        regexpClass->SetNumberOfProps(fieldOrder);
    }

    return regexpClass;
}

JSHandle<JSHClass> ObjectFactory::CreateJSArrayInstanceClass(JSHandle<JSTaggedValue> proto, uint32_t inlinedProps)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> arrayClass = NewEcmaHClass(JSArray::SIZE, inlinedProps, JSType::JS_ARRAY, proto);

    uint32_t fieldOrder = 0;
    ASSERT(JSArray::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(1);
    {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(true, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, 0, globalConst->GetLengthString(), attributes);
    }

    {
        arrayClass->SetLayout(thread_, layoutInfoHandle);
        arrayClass->SetNumberOfProps(fieldOrder);
    }
    arrayClass->SetIsStableElements(true);
    arrayClass->SetHasConstructor(false);

    return arrayClass;
}

JSHandle<JSHClass> ObjectFactory::CreateJSArguments(const JSHandle<GlobalEnv> &env)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();

    JSHandle<JSHClass> argumentsClass = NewEcmaHClass(JSArguments::SIZE, JSType::JS_ARGUMENTS, proto);

    uint32_t fieldOrder = 0;
    ASSERT(JSArguments::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(JSArguments::LENGTH_OF_INLINE_PROPERTIES);
    {
        PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, JSArguments::LENGTH_INLINE_PROPERTY_INDEX, globalConst->GetLengthString(),
                                 attributes);
    }

    ASSERT(JSArguments::ITERATOR_INLINE_PROPERTY_INDEX == fieldOrder);
    {
        PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, JSArguments::ITERATOR_INLINE_PROPERTY_INDEX,
                                 env->GetIteratorSymbol().GetTaggedValue(), attributes);
    }

    {
        ASSERT(JSArguments::CALLER_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetIsAccessor(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, JSArguments::CALLER_INLINE_PROPERTY_INDEX,
                                 thread_->GlobalConstants()->GetHandledCallerString().GetTaggedValue(), attributes);
    }

    {
        ASSERT(JSArguments::CALLEE_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetIsAccessor(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder++);
        layoutInfoHandle->AddKey(thread_, JSArguments::CALLEE_INLINE_PROPERTY_INDEX,
                                 thread_->GlobalConstants()->GetHandledCalleeString().GetTaggedValue(), attributes);
    }

    {
        argumentsClass->SetLayout(thread_, layoutInfoHandle);
        argumentsClass->SetNumberOfProps(fieldOrder);
    }
    argumentsClass->SetIsStableElements(true);
    return argumentsClass;
}

JSHandle<JSArguments> ObjectFactory::NewJSArguments()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetArgumentsClass());
    JSHandle<JSArguments> obj = JSHandle<JSArguments>::Cast(NewJSObject(hclass));
    return obj;
}

JSHandle<JSObject> ObjectFactory::GetJSError(const ErrorType &errorType, const char *data, StackCheck needCheckStack)
{
    ASSERT_PRINT(errorType == ErrorType::ERROR || errorType == ErrorType::EVAL_ERROR ||
                     errorType == ErrorType::RANGE_ERROR || errorType == ErrorType::REFERENCE_ERROR ||
                     errorType == ErrorType::SYNTAX_ERROR || errorType == ErrorType::TYPE_ERROR ||
                     errorType == ErrorType::URI_ERROR || errorType == ErrorType::OOM_ERROR ||
                     errorType == ErrorType::TERMINATION_ERROR,
                 "The error type is not in the valid range.");
    if (data != nullptr) {
        JSHandle<EcmaString> handleMsg = NewFromUtf8(data);
        return NewJSError(errorType, handleMsg, needCheckStack);
    }
    JSHandle<EcmaString> emptyString(thread_->GlobalConstants()->GetHandledEmptyString());
    return NewJSError(errorType, emptyString, needCheckStack);
}

JSHandle<JSObject> ObjectFactory::NewJSError(const ErrorType &errorType, const JSHandle<EcmaString> &message,
    StackCheck needCheckStack)
{
    // if there have exception in thread, then return current exception, no need to new js error.
    if (thread_->HasPendingException()) {
        JSHandle<JSObject> obj(thread_, thread_->GetException());
        return obj;
    }

    // current frame may be entry frame, exception happened in JSFunction::Call and JSFunction::Construct,
    // in this case sp = the prev frame (interpreter frame).
    if (!thread_->IsAsmInterpreter()) {
        FrameHandler frameHandler(thread_);
        if (frameHandler.IsInterpretedEntryFrame()) {
            thread_->SetCurrentSPFrame(frameHandler.GetPrevJSFrame());
        }
    }

    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> nativeConstructor;
    switch (errorType) {
        case ErrorType::RANGE_ERROR:
            nativeConstructor = env->GetRangeErrorFunction();
            break;
        case ErrorType::EVAL_ERROR:
            nativeConstructor = env->GetEvalErrorFunction();
            break;
        case ErrorType::REFERENCE_ERROR:
            nativeConstructor = env->GetReferenceErrorFunction();
            break;
        case ErrorType::TYPE_ERROR:
            nativeConstructor = env->GetTypeErrorFunction();
            break;
        case ErrorType::URI_ERROR:
            nativeConstructor = env->GetURIErrorFunction();
            break;
        case ErrorType::SYNTAX_ERROR:
            nativeConstructor = env->GetSyntaxErrorFunction();
            break;
        case ErrorType::OOM_ERROR:
            nativeConstructor = env->GetOOMErrorFunction();
            break;
        case ErrorType::TERMINATION_ERROR:
            nativeConstructor = env->GetTerminationErrorFunction();
            break;
        default:
            nativeConstructor = env->GetErrorFunction();
            break;
    }
    JSHandle<JSFunction> nativeFunc = JSHandle<JSFunction>::Cast(nativeConstructor);
    JSHandle<JSTaggedValue> nativePrototype(thread_, nativeFunc->GetFunctionPrototype());
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread_, nativeConstructor, nativePrototype, undefined, 1, needCheckStack);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread_);
    info->SetCallArg(message.GetTaggedValue());
    JSTaggedValue obj = reinterpret_cast<EcmaEntrypoint>(nativeFunc->GetNativePointer())(info);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread_);
    JSHandle<JSObject> handleNativeInstanceObj(thread_, obj);
    auto sp = const_cast<JSTaggedType *>(thread_->GetCurrentSPFrame());
    ASSERT(FrameHandler::GetFrameType(sp) == FrameType::INTERPRETER_ENTRY_FRAME);
    auto prevEntry = InterpretedEntryFrame::GetFrameFromSp(sp)->GetPrevFrameFp();
    thread_->SetCurrentSPFrame(prevEntry);
    return handleNativeInstanceObj;
}

JSHandle<JSObject> ObjectFactory::NewJSAggregateError()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> constructor(env->GetAggregateErrorFunction());
    return NewJSObjectByConstructor(constructor);
}

JSHandle<JSObject> ObjectFactory::CreateNapiObject()
{
    JSHandle<GlobalEnv> globalEnv = vm_->GetGlobalEnv();
    JSHandle<JSFunction> constructor(globalEnv->GetObjectFunction());
    JSHandle<JSHClass> ihc(globalEnv->GetObjectFunctionNapiClass());
    if (globalEnv->GetTaggedObjectFunctionTsNapiClass().IsJSHClass()) {
        ihc = JSHandle<JSHClass>(globalEnv->GetObjectFunctionTsNapiClass());
    }
    JSHandle<JSObject> jsObject(NewJSObjectWithInit(ihc));
    JSHandle<JSTaggedValue> object(jsObject);
    return jsObject;
}

JSHandle<JSObject> ObjectFactory::NewJSObjectByConstructor(JSHandle<GlobalEnv> env,
    const JSHandle<JSFunction> &constructor, uint32_t inlinedProps)
{
    if (!constructor->HasFunctionPrototype() ||
        (constructor->GetProtoOrHClass().IsHeapObject() && constructor->GetFunctionPrototype().IsECMAObject())) {
        JSHandle<JSHClass> jshclass;
        if (LIKELY(inlinedProps == JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS)) {
            jshclass = JSHandle<JSHClass>(thread_, JSFunction::GetOrCreateInitialJSHClass(thread_, constructor));
        } else {
            jshclass = NewEcmaHClass(JSObject::SIZE, inlinedProps, JSType::JS_OBJECT,
                                     env->GetObjectFunctionPrototype());
        }
        JSHandle<JSObject> obj;
        if (jshclass->IsJSSharedObject()) {
            obj = NewSharedOldSpaceJSObject(jshclass);
            if (jshclass->IsDictionaryMode()) {
                auto fieldLayout = jshclass->GetLayout();
                ASSERT(fieldLayout.IsDictionary());
                auto dict = JSHandle<TaggedArray>(thread_, fieldLayout);
                auto properties = NewAndCopySNameDictionary(dict, dict->GetLength());
                obj->SetProperties(thread_, properties);
            }
        } else {
            obj = NewJSObjectWithInit(jshclass);
        }
        return obj;
    }
    JSHandle<JSObject> result =
        NewJSObjectByConstructor(JSHandle<JSFunction>(env->GetObjectFunction()), JSHandle<JSTaggedValue>(constructor));
    if (thread_->HasPendingException()) {
        LOG_FULL(FATAL) << "NewJSObjectByConstructor should not throw Exception! ";
    }
    return result;
}

JSHandle<JSObject> ObjectFactory::NewJSObjectByConstructor(const JSHandle<JSFunction> &constructor,
                                                           uint32_t inlinedProps)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    if (!constructor->HasFunctionPrototype() ||
        (constructor->GetProtoOrHClass().IsHeapObject() && constructor->GetFunctionPrototype().IsECMAObject())) {
        JSHandle<JSHClass> jshclass;
        if (LIKELY(inlinedProps == JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS)) {
            jshclass = JSHandle<JSHClass>(thread_, JSFunction::GetOrCreateInitialJSHClass(thread_, constructor));
        } else {
            jshclass = NewEcmaHClass(JSObject::SIZE, inlinedProps, JSType::JS_OBJECT,
                                     env->GetObjectFunctionPrototype());
        }
        JSHandle<JSObject> obj;
        if (jshclass->IsJSShared()) {
            obj = NewSharedOldSpaceJSObject(jshclass);
            if (jshclass->IsDictionaryMode()) {
                auto fieldLayout = jshclass->GetLayout();
                ASSERT(fieldLayout.IsDictionary());
                auto dict = JSHandle<TaggedArray>(thread_, fieldLayout);
                auto properties = NewAndCopySNameDictionary(dict, dict->GetLength());
                obj->SetProperties(thread_, properties);
            }
            if (constructor->IsClassConstructor()) {
                JSTaggedValue elementsDic =
                    constructor->GetPropertyInlinedProps(ClassInfoExtractor::SENDABLE_ELEMENTS_INDEX);
                if (elementsDic.IsDictionary()) {
                    JSHandle<TaggedArray> elementsDicHld(thread_, elementsDic);
                    JSHandle<TaggedArray> elements =
                        NewAndCopySNameDictionary(elementsDicHld, elementsDicHld->GetLength());
                    obj->SetElements(thread_, elements);
                    jshclass->SetIsDictionaryElement(true);
                }
            }
            InitializeJSObject(obj, jshclass);
        } else {
            obj = NewJSObjectWithInit(jshclass);
        }
        return obj;
    }
    JSHandle<JSObject> result =
        NewJSObjectByConstructor(JSHandle<JSFunction>(env->GetObjectFunction()), JSHandle<JSTaggedValue>(constructor));
    if (thread_->HasPendingException()) {
        LOG_FULL(FATAL) << "NewJSObjectByConstructor should not throw Exception! ";
    }
    return result;
}

JSHandle<JSObject> ObjectFactory::NewJSObjectByConstructor(const JSHandle<JSFunction> &constructor,
                                                           const JSHandle<JSTaggedValue> &newTarget)
{
    if (constructor->IsJSShared() && !newTarget->IsJSShared()) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "shared ctor cannot assign unshared newTarget",
                                    JSHandle<JSObject>(thread_, JSTaggedValue::Undefined()));
    }
    JSHandle<JSHClass> jshclass;
    if (!constructor->HasFunctionPrototype() ||
        (constructor->GetProtoOrHClass().IsHeapObject() && constructor->GetFunctionPrototype().IsECMAObject())) {
        jshclass = JSFunction::GetInstanceJSHClass(thread_, constructor, newTarget);
    } else {
        JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
        jshclass = JSFunction::GetInstanceJSHClass(thread_, JSHandle<JSFunction>(env->GetObjectFunction()), newTarget);
    }
    // Check this exception elsewhere
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread_);
    JSHandle<JSObject> obj;
    if (jshclass->IsJSShared()) {
        obj = NewSharedOldSpaceJSObject(jshclass);
        if (jshclass->IsDictionaryMode()) {
            auto fieldLayout = jshclass->GetLayout();
            ASSERT(fieldLayout.IsDictionary());
            auto dict = JSHandle<TaggedArray>(thread_, fieldLayout);
            auto properties = NewAndCopySNameDictionary(dict, dict->GetLength());
            obj->SetProperties(thread_, properties);
        }
        if (constructor->IsClassConstructor() && newTarget->IsClassConstructor()) {
            JSTaggedValue elementsDicOfCtor =
                constructor->GetPropertyInlinedProps(ClassInfoExtractor::SENDABLE_ELEMENTS_INDEX);
            JSTaggedValue elementsDicOfTrg =
                JSHandle<JSFunction>(newTarget)->GetPropertyInlinedProps(ClassInfoExtractor::SENDABLE_ELEMENTS_INDEX);
            if (elementsDicOfCtor.IsDictionary() && elementsDicOfTrg.IsDictionary()) {
                JSHandle<TaggedArray> elements;
                MergeSendableClassElementsDic(elements, JSHandle<JSTaggedValue>(thread_, elementsDicOfCtor),
                                              JSHandle<JSTaggedValue>(thread_, elementsDicOfTrg));
                obj->SetElements(thread_, elements);
                jshclass->SetIsDictionaryElement(true);
            }
        }
        InitializeJSObject(obj, jshclass);
    } else {
        obj = NewJSObjectWithInit(jshclass);
    }
    return obj;
}

void ObjectFactory::MergeSendableClassElementsDic(JSHandle<TaggedArray> &elements,
                                                  const JSHandle<JSTaggedValue> &elementsDicOfCtorVal,
                                                  const JSHandle<JSTaggedValue> &elementsDicOfTrgVal)
{
    JSHandle<JSTaggedValue> undefinedVal = thread_->GlobalConstants()->GetHandledUndefined();

    JSHandle<TaggedArray> elementsDicOfCtorHdl(elementsDicOfCtorVal);
    if (elementsDicOfCtorVal.GetTaggedValue() == elementsDicOfTrgVal.GetTaggedValue()) {
        elements = NewAndCopySNameDictionary(elementsDicOfCtorHdl, elementsDicOfCtorHdl->GetLength());
    } else {
        JSHandle<TaggedArray> elementsDicDicOfTrgHdl(elementsDicOfTrgVal);
        JSHandle<NumberDictionary> elementsDicOfCtor(elementsDicOfCtorVal);
        JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());

        JSMutableHandle<NumberDictionary> newElementsDicUpdate(thread_, elementsDicOfCtor);
        uint32_t headerSize = 4;
        for (uint32_t i = headerSize; i < elementsDicDicOfTrgHdl->GetLength(); i++) {
            key.Update(elementsDicDicOfTrgHdl->Get(i));
            if (key->IsInt()) {
                JSHandle<NumberDictionary> elementsDicOfTrg(elementsDicOfTrgVal);

                int entry = elementsDicOfTrg->FindEntry(key.GetTaggedValue());
                PropertyAttributes attributes = elementsDicOfTrg->GetAttributes(entry);
                JSHandle<NumberDictionary> newElementsDic =
                    NumberDictionary::Put(thread_, elementsDicOfCtor, key, undefinedVal, attributes);
                newElementsDicUpdate.Update(newElementsDic);
            }
        }
        JSHandle<TaggedArray> newElementsDicHld(newElementsDicUpdate);
        elements = NewAndCopySNameDictionary(newElementsDicHld, newElementsDicHld->GetLength());
    }
}

JSHandle<JSObject> ObjectFactory::NewJSObjectWithInit(const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSObject> obj = NewJSObject(jshclass);
    InitializeJSObject(obj, jshclass);
    return obj;
}

void ObjectFactory::InitializeJSObject(const JSHandle<JSObject> &obj, const JSHandle<JSHClass> &jshclass)
{
    JSType type = jshclass->GetObjectType();
    switch (type) {
        case JSType::JS_OBJECT:
        case JSType::JS_ERROR:
        case JSType::JS_EVAL_ERROR:
        case JSType::JS_RANGE_ERROR:
        case JSType::JS_REFERENCE_ERROR:
        case JSType::JS_TYPE_ERROR:
        case JSType::JS_AGGREGATE_ERROR:
        case JSType::JS_URI_ERROR:
        case JSType::JS_SYNTAX_ERROR:
        case JSType::JS_OOM_ERROR:
        case JSType::JS_TERMINATION_ERROR:
        case JSType::JS_ASYNCITERATOR:
        case JSType::JS_SHARED_OBJECT:
        case JSType::JS_SHARED_FUNCTION:
        case JSType::JS_ITERATOR: {
            break;
        }
#ifdef ARK_SUPPORT_INTL
        case JSType::JS_INTL: {
            JSIntl::Cast(*obj)->SetFallbackSymbol(thread_, JSTaggedValue::Undefined());
            JSHandle<JSSymbol> jsSymbol = NewPublicSymbolWithChar("IntlLegacyConstructedSymbol");
            JSIntl::Cast(*obj)->SetFallbackSymbol(thread_, jsSymbol);
            break;
        }
        case JSType::JS_LOCALE: {
            JSLocale::Cast(*obj)->SetIcuField(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_DATE_TIME_FORMAT: {
            JSDateTimeFormat::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
            JSDateTimeFormat::Cast(*obj)->SetCalendar(thread_, JSTaggedValue::Undefined());
            JSDateTimeFormat::Cast(*obj)->SetNumberingSystem(thread_, JSTaggedValue::Undefined());
            JSDateTimeFormat::Cast(*obj)->SetTimeZone(thread_, JSTaggedValue::Undefined());
            JSDateTimeFormat::Cast(*obj)->SetLocaleIcu(thread_, JSTaggedValue::Undefined());
            JSDateTimeFormat::Cast(*obj)->SetSimpleDateTimeFormatIcu(thread_, JSTaggedValue::Undefined());
            JSDateTimeFormat::Cast(*obj)->SetIso8601(thread_, JSTaggedValue::Undefined());
            JSDateTimeFormat::Cast(*obj)->SetBoundFormat(thread_, JSTaggedValue::Undefined());
            JSDateTimeFormat::Cast(*obj)->SetHourCycle(HourCycleOption::EXCEPTION);
            JSDateTimeFormat::Cast(*obj)->SetDateStyle(DateTimeStyleOption::EXCEPTION);
            JSDateTimeFormat::Cast(*obj)->SetTimeStyle(DateTimeStyleOption::EXCEPTION);
            break;
        }
        case JSType::JS_NUMBER_FORMAT: {
            JSNumberFormat::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetNumberingSystem(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetCurrency(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetUnit(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetMinimumIntegerDigits(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetMinimumFractionDigits(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetMaximumFractionDigits(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetMinimumSignificantDigits(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetMaximumSignificantDigits(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetUseGrouping(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetBoundFormat(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetIcuField(thread_, JSTaggedValue::Undefined());
            JSNumberFormat::Cast(*obj)->SetStyle(StyleOption::EXCEPTION);
            JSNumberFormat::Cast(*obj)->SetCurrencySign(CurrencySignOption::EXCEPTION);
            JSNumberFormat::Cast(*obj)->SetCurrencyDisplay(CurrencyDisplayOption::EXCEPTION);
            JSNumberFormat::Cast(*obj)->SetUnitDisplay(UnitDisplayOption::EXCEPTION);
            JSNumberFormat::Cast(*obj)->SetSignDisplay(SignDisplayOption::EXCEPTION);
            JSNumberFormat::Cast(*obj)->SetCompactDisplay(CompactDisplayOption::EXCEPTION);
            JSNumberFormat::Cast(*obj)->SetNotation(NotationOption::EXCEPTION);
            JSNumberFormat::Cast(*obj)->SetRoundingType(RoundingType::EXCEPTION);
            break;
        }
        case JSType::JS_RELATIVE_TIME_FORMAT: {
            JSRelativeTimeFormat::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
            JSRelativeTimeFormat::Cast(*obj)->SetNumberingSystem(thread_, JSTaggedValue::Undefined());
            JSRelativeTimeFormat::Cast(*obj)->SetIcuField(thread_, JSTaggedValue::Undefined());
            JSRelativeTimeFormat::Cast(*obj)->SetStyle(RelativeStyleOption::EXCEPTION);
            JSRelativeTimeFormat::Cast(*obj)->SetNumeric(NumericOption::EXCEPTION);
            break;
        }
        case JSType::JS_COLLATOR: {
            JSCollator::Cast(*obj)->SetIcuField(thread_, JSTaggedValue::Undefined());
            JSCollator::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
            JSCollator::Cast(*obj)->SetCollation(thread_, JSTaggedValue::Undefined());
            JSCollator::Cast(*obj)->SetBoundCompare(thread_, JSTaggedValue::Undefined());
            JSCollator::Cast(*obj)->SetUsage(UsageOption::EXCEPTION);
            JSCollator::Cast(*obj)->SetCaseFirst(CaseFirstOption::EXCEPTION);
            JSCollator::Cast(*obj)->SetSensitivity(SensitivityOption::EXCEPTION);
            JSCollator::Cast(*obj)->SetIgnorePunctuation(false);
            JSCollator::Cast(*obj)->SetNumeric(false);
            break;
        }
        case JSType::JS_PLURAL_RULES: {
            JSPluralRules::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
            JSPluralRules::Cast(*obj)->SetMinimumIntegerDigits(thread_, JSTaggedValue::Undefined());
            JSPluralRules::Cast(*obj)->SetMinimumFractionDigits(thread_, JSTaggedValue::Undefined());
            JSPluralRules::Cast(*obj)->SetMaximumFractionDigits(thread_, JSTaggedValue::Undefined());
            JSPluralRules::Cast(*obj)->SetMinimumSignificantDigits(thread_, JSTaggedValue::Undefined());
            JSPluralRules::Cast(*obj)->SetMaximumSignificantDigits(thread_, JSTaggedValue::Undefined());
            JSPluralRules::Cast(*obj)->SetIcuPR(thread_, JSTaggedValue::Undefined());
            JSPluralRules::Cast(*obj)->SetIcuNF(thread_, JSTaggedValue::Undefined());
            JSPluralRules::Cast(*obj)->SetRoundingType(RoundingType::EXCEPTION);
            JSPluralRules::Cast(*obj)->SetType(TypeOption::EXCEPTION);
            break;
        }
        case JSType::JS_DISPLAYNAMES: {
            JSDisplayNames::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
            JSDisplayNames::Cast(*obj)->SetType(TypednsOption::EXCEPTION);
            JSDisplayNames::Cast(*obj)->SetStyle(StyOption::EXCEPTION);
            JSDisplayNames::Cast(*obj)->SetFallback(FallbackOption::EXCEPTION);
            JSDisplayNames::Cast(*obj)->SetIcuLDN(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_SEGMENTER: {
            JSSegmenter::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
            JSSegmenter::Cast(*obj)->SetGranularity(GranularityOption::EXCEPTION);
            JSSegmenter::Cast(*obj)->SetIcuField(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_SEGMENTS: {
            JSSegments::Cast(*obj)->SetIcuField(thread_, JSTaggedValue::Undefined());
            JSSegments::Cast(*obj)->SetSegmentsString(thread_, JSTaggedValue::Undefined());
            JSSegments::Cast(*obj)->SetUnicodeString(thread_, JSTaggedValue::Undefined());
            JSSegments::Cast(*obj)->SetGranularity(GranularityOption::EXCEPTION);
            break;
        }
        case JSType::JS_SEGMENT_ITERATOR: {
            JSSegmentIterator::Cast(*obj)->SetIcuField(thread_, JSTaggedValue::Undefined());
            JSSegmentIterator::Cast(*obj)->SetIteratedString(thread_, JSTaggedValue::Undefined());
            JSSegmentIterator::Cast(*obj)->SetUnicodeString(thread_, JSTaggedValue::Undefined());
            JSSegmentIterator::Cast(*obj)->SetGranularity(GranularityOption::EXCEPTION);
            break;
        }
        case JSType::JS_LIST_FORMAT: {
            JSListFormat::Cast(*obj)->SetLocale(thread_, JSTaggedValue::Undefined());
            JSListFormat::Cast(*obj)->SetType(ListTypeOption::EXCEPTION);
            JSListFormat::Cast(*obj)->SetStyle(ListStyleOption::EXCEPTION);
            JSListFormat::Cast(*obj)->SetIcuLF(thread_, JSTaggedValue::Undefined());
            break;
        }
#else
        case JSType::JS_INTL:
        case JSType::JS_LOCALE:
        case JSType::JS_DATE_TIME_FORMAT:
        case JSType::JS_NUMBER_FORMAT:
        case JSType::JS_RELATIVE_TIME_FORMAT:
        case JSType::JS_COLLATOR:
        case JSType::JS_PLURAL_RULES:
        case JSType::JS_DISPLAYNAMES:
        case JSType::JS_SEGMENTER:
        case JSType::JS_SEGMENTS:
        case JSType::JS_SEGMENT_ITERATOR:
        case JSType::JS_LIST_FORMAT: {
            break;
        }
#endif
        case JSType::JS_ARRAY: {
            JSArray::Cast(*obj)->SetLength(0);
            JSArray::Cast(*obj)->SetTrackInfo(thread_, JSTaggedValue::Undefined());
            ASSERT(!obj->GetJSHClass()->IsDictionaryMode());
            auto accessor = thread_->GlobalConstants()->GetArrayLengthAccessor();
            JSArray::Cast(*obj)->SetPropertyInlinedProps(thread_, JSArray::LENGTH_INLINE_PROPERTY_INDEX, accessor);
            break;
        }
        case JSType::JS_SHARED_ARRAY: {
            JSSharedArray::Cast(*obj)->SetLength(0);
            JSSharedArray::Cast(*obj)->SetTrackInfo(thread_, JSTaggedValue::Undefined());
            ASSERT(!obj->GetJSHClass()->IsDictionaryMode());
            JSSharedArray::Cast(*obj)->SetModRecord(0);
            auto accessor = thread_->GlobalConstants()->GetSharedArrayLengthAccessor();
            JSSharedArray::Cast(*obj)->SetPropertyInlinedProps(thread_, JSArray::LENGTH_INLINE_PROPERTY_INDEX,
                                                               accessor);
            break;
        }
        case JSType::JS_DATE:
            JSDate::Cast(*obj)->SetTimeValue(thread_, JSTaggedValue(0.0));
            JSDate::Cast(*obj)->SetLocalOffset(thread_, JSTaggedValue(JSDate::MAX_DOUBLE));
            break;
        case JSType::JS_TYPED_ARRAY:
        case JSType::JS_INT8_ARRAY:
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_UINT16_ARRAY:
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_FLOAT32_ARRAY:
        case JSType::JS_FLOAT64_ARRAY:
        case JSType::JS_BIGINT64_ARRAY:
        case JSType::JS_BIGUINT64_ARRAY:
            JSTypedArray::Cast(*obj)->SetViewedArrayBufferOrByteArray(thread_, JSTaggedValue::Undefined());
            JSTypedArray::Cast(*obj)->SetTypedArrayName(thread_, JSTaggedValue::Undefined());
            JSTypedArray::Cast(*obj)->SetByteLength(0);
            JSTypedArray::Cast(*obj)->SetByteOffset(0);
            JSTypedArray::Cast(*obj)->SetArrayLength(0);
            JSTypedArray::Cast(*obj)->SetContentType(ContentType::None);
            break;
        case JSType::JS_SHARED_TYPED_ARRAY:
        case JSType::JS_SHARED_INT8_ARRAY:
        case JSType::JS_SHARED_UINT8_ARRAY:
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
        case JSType::JS_SHARED_INT16_ARRAY:
        case JSType::JS_SHARED_UINT16_ARRAY:
        case JSType::JS_SHARED_INT32_ARRAY:
        case JSType::JS_SHARED_UINT32_ARRAY:
        case JSType::JS_SHARED_FLOAT32_ARRAY:
        case JSType::JS_SHARED_FLOAT64_ARRAY:
        case JSType::JS_SHARED_BIGINT64_ARRAY:
        case JSType::JS_SHARED_BIGUINT64_ARRAY:
            JSSharedTypedArray::Cast(*obj)->SetViewedArrayBufferOrByteArray(thread_, JSTaggedValue::Undefined());
            JSSharedTypedArray::Cast(*obj)->SetTypedArrayName(thread_, JSTaggedValue::Undefined());
            JSSharedTypedArray::Cast(*obj)->SetByteLength(0);
            JSSharedTypedArray::Cast(*obj)->SetByteOffset(0);
            JSSharedTypedArray::Cast(*obj)->SetArrayLength(0);
            JSSharedTypedArray::Cast(*obj)->SetContentType(ContentType::None);
            JSSharedTypedArray::Cast(*obj)->SetModRecord(0);
            break;
        case JSType::JS_REG_EXP:
            JSRegExp::Cast(*obj)->SetByteCodeBuffer(thread_, JSTaggedValue::Undefined());
            JSRegExp::Cast(*obj)->SetOriginalSource(thread_, JSTaggedValue::Undefined());
            JSRegExp::Cast(*obj)->SetOriginalFlags(thread_, JSTaggedValue(0));
            JSRegExp::Cast(*obj)->SetGroupName(thread_, JSTaggedValue::Undefined());
            JSRegExp::Cast(*obj)->SetLength(0);
            break;
        case JSType::JS_PRIMITIVE_REF:
            JSPrimitiveRef::Cast(*obj)->SetValue(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_SET:
            JSSet::Cast(*obj)->SetLinkedSet(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_SHARED_SET:
            JSSharedSet::Cast(*obj)->SetLinkedSet(thread_, JSTaggedValue::Undefined());
            JSSharedSet::Cast(*obj)->SetModRecord(0);
            break;
        case JSType::JS_MAP:
            JSMap::Cast(*obj)->SetLinkedMap(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_SHARED_MAP:
            JSSharedMap::Cast(*obj)->SetLinkedMap(thread_, JSTaggedValue::Undefined());
            JSSharedMap::Cast(*obj)->SetModRecord(0);
            break;
        case JSType::JS_WEAK_MAP:
            JSWeakMap::Cast(*obj)->SetLinkedMap(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_WEAK_SET:
            JSWeakSet::Cast(*obj)->SetLinkedSet(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_WEAK_REF:
            JSWeakRef::Cast(*obj)->SetWeakObject(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_FINALIZATION_REGISTRY:
            JSFinalizationRegistry::Cast(*obj)->SetCleanupCallback(thread_, JSTaggedValue::Undefined());
            JSFinalizationRegistry::Cast(*obj)->SetNoUnregister(thread_, JSTaggedValue::Undefined());
            JSFinalizationRegistry::Cast(*obj)->SetMaybeUnregister(thread_, JSTaggedValue::Undefined());
            JSFinalizationRegistry::Cast(*obj)->SetNext(thread_, JSTaggedValue::Null());
            JSFinalizationRegistry::Cast(*obj)->SetPrev(thread_, JSTaggedValue::Null());
            break;
        case JSType::JS_GENERATOR_OBJECT:
            JSGeneratorObject::Cast(*obj)->SetGeneratorContext(thread_, JSTaggedValue::Undefined());
            JSGeneratorObject::Cast(*obj)->SetResumeResult(thread_, JSTaggedValue::Undefined());
            JSGeneratorObject::Cast(*obj)->SetGeneratorState(JSGeneratorState::UNDEFINED);
            JSGeneratorObject::Cast(*obj)->SetResumeMode(GeneratorResumeMode::UNDEFINED);
            break;
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            JSAsyncGeneratorObject::Cast(*obj)->SetGeneratorContext(thread_, JSTaggedValue::Undefined());
            JSAsyncGeneratorObject::Cast(*obj)->SetAsyncGeneratorQueue(thread_, GetEmptyTaggedQueue().GetTaggedValue());
            JSAsyncGeneratorObject::Cast(*obj)->SetGeneratorBrand(thread_, JSTaggedValue::Undefined());
            JSAsyncGeneratorObject::Cast(*obj)->SetResumeResult(thread_, JSTaggedValue::Undefined());
            JSAsyncGeneratorObject::Cast(*obj)->SetAsyncGeneratorState(JSAsyncGeneratorState::UNDEFINED);
            JSAsyncGeneratorObject::Cast(*obj)->SetResumeMode(AsyncGeneratorResumeMode::UNDEFINED);
            break;
        case JSType::JS_STRING_ITERATOR:
            JSStringIterator::Cast(*obj)->SetStringIteratorNextIndex(0);
            JSStringIterator::Cast(*obj)->SetIteratedString(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
            JSAsyncFromSyncIterator::Cast(*obj)->SetSyncIteratorRecord(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION:
            JSAsyncFromSyncIterUnwarpFunction::Cast(*obj)->SetDone(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_ARRAY_BUFFER:
            JSArrayBuffer::Cast(*obj)->SetArrayBufferData(thread_, JSTaggedValue::Undefined());
            JSArrayBuffer::Cast(*obj)->SetArrayBufferByteLength(0);
            JSArrayBuffer::Cast(*obj)->ClearBitField();
            break;
        case JSType::JS_SHARED_ARRAY_BUFFER:
            JSArrayBuffer::Cast(*obj)->SetArrayBufferData(thread_, JSTaggedValue::Undefined());
            JSArrayBuffer::Cast(*obj)->SetArrayBufferByteLength(0);
            JSArrayBuffer::Cast(*obj)->SetShared(true);
            break;
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
            JSSendableArrayBuffer::Cast(*obj)->SetArrayBufferData(thread_, JSTaggedValue::Undefined());
            JSSendableArrayBuffer::Cast(*obj)->SetArrayBufferByteLength(0);
            break;
        case JSType::JS_PROMISE:
            JSPromise::Cast(*obj)->SetPromiseState(PromiseState::PENDING);
            JSPromise::Cast(*obj)->SetPromiseResult(thread_, JSTaggedValue::Undefined());
            JSPromise::Cast(*obj)->SetPromiseRejectReactions(thread_, GetEmptyTaggedQueue().GetTaggedValue());
            JSPromise::Cast(*obj)->SetPromiseFulfillReactions(thread_, GetEmptyTaggedQueue().GetTaggedValue());
            JSPromise::Cast(*obj)->SetAsyncTaskId(vm_->GetAsyncTaskId());

            JSPromise::Cast(*obj)->SetPromiseIsHandled(false);
            break;
        case JSType::JS_DATA_VIEW:
            JSDataView::Cast(*obj)->SetDataView(thread_, JSTaggedValue(false));
            JSDataView::Cast(*obj)->SetViewedArrayBuffer(thread_, JSTaggedValue::Undefined());
            JSDataView::Cast(*obj)->SetByteLength(0);
            JSDataView::Cast(*obj)->SetByteOffset(0);
            break;
        // non ECMA standard jsapi container
        case JSType::JS_API_ARRAY_LIST: {
            JSAPIArrayList::Cast(*obj)->SetLength(thread_, JSTaggedValue(0));
            break;
        }
        case JSType::JS_API_HASH_MAP: {
            JSAPIHashMap::Cast(*obj)->SetSize(0);
            JSAPIHashMap::Cast(*obj)->SetTable(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_API_HASH_SET: {
            JSAPIHashSet::Cast(*obj)->SetSize(0);
            JSAPIHashSet::Cast(*obj)->SetTable(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_API_TREE_MAP: {
            JSAPITreeMap::Cast(*obj)->SetTreeMap(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_API_TREE_SET: {
            JSAPITreeSet::Cast(*obj)->SetTreeSet(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_API_QUEUE: {
            JSAPIQueue::Cast(*obj)->SetLength(thread_, JSTaggedValue(0));
            JSAPIQueue::Cast(*obj)->SetFront(0);
            JSAPIQueue::Cast(*obj)->SetTail(0);
            break;
        }
        case JSType::JS_API_PLAIN_ARRAY: {
            JSAPIPlainArray::Cast(*obj)->SetLength(0);
            JSAPIPlainArray::Cast(*obj)->SetValues(thread_, JSTaggedValue(0));
            JSAPIPlainArray::Cast(*obj)->SetKeys(thread_, JSTaggedValue(0));
            break;
        }
        case JSType::JS_API_STACK: {
            JSAPIStack::Cast(*obj)->SetTop(0);
            break;
        }
        case JSType::JS_API_DEQUE: {
            JSAPIDeque::Cast(*obj)->SetFirst(0);
            JSAPIDeque::Cast(*obj)->SetLast(0);
            break;
        }
        case JSType::JS_API_LIGHT_WEIGHT_MAP: {
            JSAPILightWeightMap::Cast(*obj)->SetLength(0);
            JSAPILightWeightMap::Cast(*obj)->SetHashes(thread_, JSTaggedValue::Undefined());
            JSAPILightWeightMap::Cast(*obj)->SetKeys(thread_, JSTaggedValue::Undefined());
            JSAPILightWeightMap::Cast(*obj)->SetValues(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_API_LIGHT_WEIGHT_SET: {
            JSAPILightWeightSet::Cast(*obj)->SetLength(0);
            JSAPILightWeightSet::Cast(*obj)->SetHashes(thread_, JSTaggedValue::Undefined());
            JSAPILightWeightSet::Cast(*obj)->SetValues(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_API_VECTOR: {
            JSAPIVector::Cast(*obj)->SetLength(0);
            break;
        }
        case JSType::JS_API_BITVECTOR: {
            JSAPIBitVector::Cast(*obj)->SetNativePointer(thread_, JSTaggedValue::Undefined());
            JSAPIBitVector::Cast(*obj)->SetLength(0);
            JSAPIBitVector::Cast(*obj)->SetModRecord(0);
            break;
        }
        case JSType::JS_API_LIST: {
            JSAPIList::Cast(*obj)->SetSingleList(thread_, JSTaggedValue::Undefined());
            JSAPIList::Cast(*obj)->SetBitField(0UL);
            break;
        }
        case JSType::JS_API_LINKED_LIST: {
            JSAPILinkedList::Cast(*obj)->SetDoubleList(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_ASYNC_FUNC_OBJECT:
            JSAsyncFuncObject::Cast(*obj)->SetGeneratorContext(thread_, JSTaggedValue::Undefined());
            JSAsyncFuncObject::Cast(*obj)->SetResumeResult(thread_, JSTaggedValue::Undefined());
            JSAsyncFuncObject::Cast(*obj)->SetPromise(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_FUNCTION:
        case JSType::JS_GENERATOR_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            break;
        case JSType::JS_ASYNC_GENERATOR_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            break;
        case JSType::JS_PROXY_REVOC_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSProxyRevocFunction::Cast(*obj)->SetRevocableProxy(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSPromiseReactionsFunction::Cast(*obj)->SetPromise(thread_, JSTaggedValue::Undefined());
            JSPromiseReactionsFunction::Cast(*obj)->SetAlreadyResolved(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSPromiseExecutorFunction::Cast(*obj)->SetCapability(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSAsyncModuleFulfilledFunction::Cast(*obj)->SetModule(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSAsyncModuleRejectedFunction::Cast(*obj)->SetModule(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSAsyncGeneratorResNextRetProRstFtn::Cast(*obj)->SetAsyncGeneratorObject(thread_,
                                                                                     JSTaggedValue::Undefined());
            break;
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSPromiseAllResolveElementFunction::Cast(*obj)->SetIndex(thread_, JSTaggedValue::Undefined());
            JSPromiseAllResolveElementFunction::Cast(*obj)->SetValues(thread_, JSTaggedValue::Undefined());
            JSPromiseAllResolveElementFunction::Cast(*obj)->SetCapabilities(thread_, JSTaggedValue::Undefined());
            JSPromiseAllResolveElementFunction::Cast(*obj)->SetRemainingElements(thread_, JSTaggedValue::Undefined());
            JSPromiseAllResolveElementFunction::Cast(*obj)->SetAlreadyCalled(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSPromiseAnyRejectElementFunction::Cast(*obj)->SetIndex(0);
            JSPromiseAnyRejectElementFunction::Cast(*obj)->SetErrors(thread_, JSTaggedValue::Undefined());
            JSPromiseAnyRejectElementFunction::Cast(*obj)->SetCapability(thread_, JSTaggedValue::Undefined());
            JSPromiseAnyRejectElementFunction::Cast(*obj)->SetRemainingElements(thread_, JSTaggedValue::Undefined());
            JSPromiseAnyRejectElementFunction::Cast(*obj)->SetAlreadyCalled(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSPromiseAllSettledElementFunction::Cast(*obj)->SetIndex(0);
            JSPromiseAllSettledElementFunction::Cast(*obj)->SetValues(thread_, JSTaggedValue::Undefined());
            JSPromiseAllSettledElementFunction::Cast(*obj)->SetCapability(thread_, JSTaggedValue::Undefined());
            JSPromiseAllSettledElementFunction::Cast(*obj)->SetRemainingElements(thread_, JSTaggedValue::Undefined());
            JSPromiseAllSettledElementFunction::Cast(*obj)->SetAlreadyCalled(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_PROMISE_FINALLY_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSPromiseFinallyFunction::Cast(*obj)->SetOnFinally(thread_, JSTaggedValue::Undefined());
            JSPromiseFinallyFunction::Cast(*obj)->SetConstructor(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSPromiseValueThunkOrThrowerFunction::Cast(*obj)->SetResult(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_INTL_BOUND_FUNCTION:
            JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>(obj));
            JSIntlBoundFunction::Cast(*obj)->SetNumberFormat(thread_, JSTaggedValue::Undefined());
            JSIntlBoundFunction::Cast(*obj)->SetDateTimeFormat(thread_, JSTaggedValue::Undefined());
            JSIntlBoundFunction::Cast(*obj)->SetCollator(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_BOUND_FUNCTION:
            JSBoundFunction::Cast(*obj)->SetMethod(thread_, JSTaggedValue::Undefined());
            JSBoundFunction::Cast(*obj)->SetBoundTarget(thread_, JSTaggedValue::Undefined());
            JSBoundFunction::Cast(*obj)->SetBoundThis(thread_, JSTaggedValue::Undefined());
            JSBoundFunction::Cast(*obj)->SetBoundArguments(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_ARGUMENTS:
            break;
        case JSType::JS_FORIN_ITERATOR:
        case JSType::JS_MAP_ITERATOR:
        case JSType::JS_SET_ITERATOR:
        case JSType::JS_REG_EXP_ITERATOR:
        case JSType::JS_API_ARRAYLIST_ITERATOR:
        case JSType::JS_API_TREEMAP_ITERATOR:
        case JSType::JS_API_TREESET_ITERATOR:
        case JSType::JS_API_QUEUE_ITERATOR:
        case JSType::JS_API_DEQUE_ITERATOR:
        case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
        case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
        case JSType::JS_API_STACK_ITERATOR:
        case JSType::JS_API_VECTOR_ITERATOR:
        case JSType::JS_API_BITVECTOR_ITERATOR:
        case JSType::JS_API_HASHMAP_ITERATOR:
        case JSType::JS_API_HASHSET_ITERATOR:
        case JSType::JS_ARRAY_ITERATOR:
        case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            break;
        case JSType::JS_CJS_MODULE:
            CjsModule::Cast(*obj)->SetId(thread_, JSTaggedValue::Undefined());
            CjsModule::Cast(*obj)->SetExports(thread_, JSTaggedValue::Undefined());
            CjsModule::Cast(*obj)->SetPath(thread_, JSTaggedValue::Undefined());
            CjsModule::Cast(*obj)->SetFilename(thread_, JSTaggedValue::Undefined());
            CjsModule::Cast(*obj)->SetStatus(CjsModuleStatus::UNLOAD);
            break;
        case JSType::JS_CJS_EXPORTS:
            CjsExports::Cast(*obj)->SetExports(thread_, JSTaggedValue::Undefined());
            break;
        case JSType::JS_CJS_REQUIRE:
            CjsRequire::Cast(*obj)->SetCache(thread_, JSTaggedValue::Undefined());
            CjsRequire::Cast(*obj)->SetParent(thread_, JSTaggedValue::Undefined());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable, type: " << static_cast<size_t>(type);
            UNREACHABLE();
    }
}

FreeObject *ObjectFactory::FillFreeObject(uintptr_t address, size_t size, RemoveSlots removeSlots,
                                          uintptr_t hugeObjectHead)
{
    FreeObject *object = nullptr;
    const GlobalEnvConstants *globalConst = thread_->GetFirstGlobalConst();
    if (size >= FreeObject::SIZE_OFFSET && size < FreeObject::SIZE) {
        object = reinterpret_cast<FreeObject *>(address);
        object->SetClassWithoutBarrier(JSHClass::Cast(globalConst->GetFreeObjectWithOneFieldClass().GetTaggedObject()));
        object->SetNext(INVALID_OBJECT);
    } else if (size >= FreeObject::SIZE) {
        object = reinterpret_cast<FreeObject *>(address);
        object->SetClassWithoutBarrier(
            JSHClass::Cast(globalConst->GetFreeObjectWithTwoFieldClass().GetTaggedObject()));
        object->SetAvailable(size);
        object->SetNext(INVALID_OBJECT);
        if (UNLIKELY(heap_->ShouldVerifyHeap())) {
            FillFreeMemoryRange(address + FreeObject::SIZE, address + size);
        }
    } else if (size == FreeObject::NEXT_OFFSET) {
        object = reinterpret_cast<FreeObject *>(address);
        object->SetClassWithoutBarrier(
            JSHClass::Cast(globalConst->GetFreeObjectWithNoneFieldClass().GetTaggedObject()));
    } else {
        LOG_ECMA(DEBUG) << "Fill free object size is smaller";
    }

    if (removeSlots == RemoveSlots::YES) {
        // For huge object, the region of `object` might not be its 1st region. Use `hugeObjectHead` instead.
        Region *region = Region::ObjectAddressToRange(hugeObjectHead == 0 ? object :
                                                      reinterpret_cast<TaggedObject *>(hugeObjectHead));
        heap_->ClearSlotsRange(region, address, address + size);
    }
    return object;
}

TaggedObject *ObjectFactory::NewObject(const JSHandle<JSHClass> &hclass)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(*hclass);
    uint32_t inobjPropCount = hclass->GetInlinedProperties();
    if (inobjPropCount > 0) {
        InitializeExtraProperties(hclass, header, inobjPropCount);
    }
    return header;
}

TaggedObject *ObjectFactory::NewNonMovableObject(const JSHandle<JSHClass> &hclass, uint32_t inobjPropCount)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(*hclass);
    if (inobjPropCount > 0) {
        InitializeExtraProperties(hclass, header, inobjPropCount);
    }
    return header;
}

void ObjectFactory::InitializeExtraProperties(const JSHandle<JSHClass> &hclass,
                                              TaggedObject *obj, uint32_t inobjPropCount)
{
    ASSERT(inobjPropCount * JSTaggedValue::TaggedTypeSize() < hclass->GetObjectSize());
    auto paddr = reinterpret_cast<uintptr_t>(obj) + hclass->GetObjectSize();
    // The object which created by AOT speculative hclass, should be initialized as hole, means does not exist,
    // to follow ECMA spec.
    JSTaggedType initVal;
    bool inProgress = hclass->IsObjSizeTrackingInProgress();
    if (inProgress) {
        const GlobalEnvConstants *globalConst = thread_->GetFirstGlobalConst();
        initVal = globalConst->GetFreeObjectWithNoneFieldClass().GetRawData();
    } else {
        initVal = hclass->IsAOT() ? JSTaggedValue::VALUE_HOLE : JSTaggedValue::VALUE_UNDEFINED;
    }
    for (uint32_t i = 0; i < inobjPropCount; ++i) {
        paddr -= JSTaggedValue::TaggedTypeSize();
        *reinterpret_cast<JSTaggedType *>(paddr) = initVal;
    }
    if (inProgress) {
        hclass->ObjSizeTrackingStep();
    }
}

JSHandle<JSObject> ObjectFactory::OrdinaryNewJSObjectCreate(const JSHandle<JSTaggedValue> &proto)
{
    JSHandle<JSTaggedValue> protoValue(proto);
    JSHandle<JSHClass> hclass(thread_, thread_->GlobalConstants()->GetObjectClass().GetTaggedObject());
    JSHandle<JSHClass> newClass = JSHClass::TransProtoWithoutLayout(thread_, hclass, protoValue);
    JSHandle<JSObject> newObj = NewJSObject(newClass);
    newObj->GetJSHClass()->SetExtensible(true);
    return newObj;
}

JSHandle<JSHClass> ObjectFactory::GetHClassByFunctionKind(const JSHandle<GlobalEnv> &env, FunctionKind kind)
{
    JSHandle<JSHClass> hclass;
    if (kind == FunctionKind::BASE_CONSTRUCTOR) {
        hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    } else if (JSFunction::IsConstructorKind(kind)) {
        hclass = JSHandle<JSHClass>::Cast(env->GetConstructorFunctionClass());
    } else if (kind == FunctionKind::CONCURRENT_FUNCTION) {
        hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
    } else {
        hclass = JSHandle<JSHClass>::Cast(env->GetNormalFunctionClass());
    }
    return hclass;
}

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc,
                                                  FunctionKind kind, kungfu::BuiltinsStubCSigns::ID builtinId,
                                                  MemSpaceType methodSpaceType)
{
    JSHandle<JSHClass> hclass = GetHClassByFunctionKind(env, kind);
    if (builtinId != kungfu::BuiltinsStubCSigns::INVALID) {
        JSHandle<Method> method = NewMethodForNativeFunction(nativeFunc, kind, builtinId, methodSpaceType);
        return NewJSFunctionByHClass(method, hclass);
    }
    return NewNativeFunctionByHClass(hclass, nativeFunc, kind);
}

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<GlobalEnv> &env,
                                                  const JSHandle<Method> &method)
{
    FunctionKind kind = method->GetFunctionKind();
    JSHandle<JSHClass> hclass = GetHClassByFunctionKind(env, kind);
    return NewJSFunctionByHClass(method, hclass);
}

JSHandle<JSFunction> ObjectFactory::NewSFunction(const JSHandle<GlobalEnv> &env,
                                                 const void *nativeFunc,
                                                 FunctionKind kind,
                                                 kungfu::BuiltinsStubCSigns::ID builtinId,
                                                 MemSpaceType spaceType)
{
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetSFunctionClassWithoutProto());
    if (builtinId != kungfu::BuiltinsStubCSigns::INVALID) {
        JSHandle<Method> method = NewSMethodForNativeFunction(nativeFunc, kind, builtinId, spaceType);
        return NewSFunctionByHClass(method, hclass);
    }
    return NewNativeSFunctionByHClass(hclass, nativeFunc, kind);
}

JSHandle<JSHClass> ObjectFactory::CreateFunctionClass(FunctionKind kind, uint32_t size, JSType type,
                                                      const JSHandle<JSTaggedValue> &prototype)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> functionClass = NewEcmaHClass(size, type, prototype);
    {
        functionClass->SetCallable(true);
        // FunctionKind = BASE_CONSTRUCTOR
        if (JSFunction::IsConstructorKind(kind)) {
            functionClass->SetConstructor(true);
        }
        functionClass->SetExtensible(true);
    }

    uint32_t fieldOrder = 0;
    ASSERT(JSFunction::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(JSFunction::LENGTH_OF_INLINE_PROPERTIES);
    {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(false, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, globalConst->GetLengthString(), attributes);
        fieldOrder++;
    }

    ASSERT(JSFunction::NAME_INLINE_PROPERTY_INDEX == fieldOrder);
    // not set name in-object property on class which may have a name() method
    if (!JSFunction::IsClassConstructor(kind)) {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(false, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder,
                                 thread_->GlobalConstants()->GetHandledNameString().GetTaggedValue(), attributes);
        fieldOrder++;
    }

    if (JSFunction::HasPrototype(kind) && !JSFunction::IsClassConstructor(kind)) {
        ASSERT(JSFunction::PROTOTYPE_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(true, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, globalConst->GetPrototypeString(), attributes);
        fieldOrder++;
    } else if (JSFunction::IsClassConstructor(kind)) {
        ASSERT(JSFunction::CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(false, false, false);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, globalConst->GetPrototypeString(), attributes);
        fieldOrder++;
    }

    {
        functionClass->SetLayout(thread_, layoutInfoHandle);
        functionClass->SetNumberOfProps(fieldOrder);
    }
    return functionClass;
}

JSHandle<JSHClass> ObjectFactory::CreateBoundFunctionClass()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetFunctionPrototype();
    JSHandle<JSHClass> hclass = NewEcmaHClass(JSBoundFunction::SIZE, JSType::JS_BOUND_FUNCTION, proto);
    hclass->SetCallable(true);

    // set hclass layout
    uint32_t fieldOrder = 0;
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    ASSERT(JSFunction::LENGTH_INLINE_PROPERTY_INDEX == fieldOrder);
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(JSFunction::LENGTH_OF_INLINE_PROPERTIES);
    {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(false, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, globalConst->GetLengthString(), attributes);
        fieldOrder++;
    }

    ASSERT(JSFunction::NAME_INLINE_PROPERTY_INDEX == fieldOrder);
    // not set name in-object property on class which may have a name() method
    {
        PropertyAttributes attributes = PropertyAttributes::DefaultAccessor(false, false, true);
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder,
                                 globalConst->GetHandledNameString().GetTaggedValue(), attributes);
        fieldOrder++;
    }

    {
        hclass->SetLayout(thread_, layoutInfoHandle);
        hclass->SetNumberOfProps(fieldOrder);
    }
    return hclass;
}

JSHandle<JSHClass> ObjectFactory::CreateDefaultClassPrototypeHClass(JSHClass *hclass)
{
    uint32_t size = ClassInfoExtractor::NON_STATIC_RESERVED_LENGTH;
    JSHandle<LayoutInfo> layout = CreateLayoutInfo(size, MemSpaceType::OLD_SPACE, GrowMode::KEEP);
    PropertyAttributes attributes = PropertyAttributes::Default(true, false, true);  // non-enumerable

    attributes.SetIsInlinedProps(true);
    attributes.SetRepresentation(Representation::TAGGED);
    attributes.SetOffset(ClassInfoExtractor::CONSTRUCTOR_INDEX);
    layout->AddKey(thread_, ClassInfoExtractor::CONSTRUCTOR_INDEX,
        thread_->GlobalConstants()->GetConstructorString(), attributes);

    JSHandle<JSHClass> defaultHclass = NewEcmaHClass(hclass, JSObject::SIZE, JSType::JS_OBJECT, size);
    defaultHclass->SetLayout(thread_, layout);
    defaultHclass->SetNumberOfProps(size);
    defaultHclass->SetClassPrototype(true);
    defaultHclass->SetIsPrototype(true);
    return defaultHclass;
}

JSHandle<JSHClass> ObjectFactory::CreateDefaultClassConstructorHClass(JSHClass *hclass)
{
    uint32_t size = ClassInfoExtractor::STATIC_RESERVED_LENGTH;
    JSHandle<LayoutInfo> layout = CreateLayoutInfo(size, MemSpaceType::OLD_SPACE, GrowMode::KEEP);

    JSHandle<TaggedArray> array = NewTaggedArray(size);
    array->Set(thread_, ClassInfoExtractor::LENGTH_INDEX, thread_->GlobalConstants()->GetLengthString());
    array->Set(thread_, ClassInfoExtractor::NAME_INDEX, thread_->GlobalConstants()->GetNameString());
    array->Set(thread_, ClassInfoExtractor::PROTOTYPE_INDEX, thread_->GlobalConstants()->GetPrototypeString());
    for (uint32_t index = ClassInfoExtractor::LENGTH_INDEX; index < size; index++) {
        PropertyAttributes attributes;
        if (index == ClassInfoExtractor::PROTOTYPE_INDEX) {
            attributes = PropertyAttributes::DefaultAccessor(false, false, false);
        } else {
            attributes = PropertyAttributes::Default(false, false, true);
        }
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(index);
        layout->AddKey(thread_, index, array->Get(index), attributes);
    }

    JSHandle<JSHClass> defaultHclass = NewEcmaHClass(hclass, JSFunction::SIZE, JSType::JS_FUNCTION, size);
    defaultHclass->SetLayout(thread_, layout);
    defaultHclass->SetNumberOfProps(size);
    defaultHclass->SetClassConstructor(true);
    defaultHclass->SetConstructor(true);
    return defaultHclass;
}

void ObjectFactory::SetCodeEntryToFunctionFromMethod(const JSHandle<JSFunction> &func, const JSHandle<Method> &method)
{
    uintptr_t entry = method->GetCodeEntryOrLiteral();
    func->SetCodeEntry(entry);
}

JSHandle<JSFunction> ObjectFactory::NewJSFunctionByHClass(const JSHandle<Method> &method,
                                                          const JSHandle<JSHClass> &clazz,
                                                          MemSpaceType type)
{
    JSHandle<JSFunction> function;
    switch (type) {
        case MemSpaceType::SEMI_SPACE:
            function = JSHandle<JSFunction>::Cast(NewJSObject(clazz));
            break;
        case MemSpaceType::OLD_SPACE:
            function = JSHandle<JSFunction>::Cast(NewOldSpaceJSObject(clazz));
            break;
        case MemSpaceType::NON_MOVABLE:
            function = JSHandle<JSFunction>::Cast(NewNonMovableJSObject(clazz));
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    clazz->SetCallable(true);
    clazz->SetExtensible(true);
    JSFunction::InitializeJSFunction(thread_, function, method->GetFunctionKind());
    function->SetMethod(thread_, method);
    function->SetTaskConcurrentFuncFlag(0); // 0 : default value
    if (method->IsNativeWithCallField()) {
        SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    } else if (method->IsAotWithCallField()) {
        thread_->GetEcmaVM()->GetAOTFileManager()->
            SetAOTFuncEntry(method->GetJSPandaFile(), *function, *method);
    } else {
        SetCodeEntryToFunctionFromMethod(function, method);
    }
    return function;
}

JSTaggedValue ObjectFactory::GetReadOnlyMethodForNativeFunction(FunctionKind kind)
{
    size_t constantIndex;
    switch (kind) {
        case FunctionKind::NORMAL_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::NORMAL_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::GETTER_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::GETTER_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::SETTER_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::SETTER_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::ARROW_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::ARROW_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::ASYNC_ARROW_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::ASYNC_ARROW_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::CONCURRENT_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::CONCURRENT_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::ASYNC_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::ASYNC_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::BASE_CONSTRUCTOR:
            constantIndex = static_cast<size_t>(ConstantIndex::BASE_CONSTRUCTOR_METHOD_INDEX);
            break;
        case FunctionKind::CLASS_CONSTRUCTOR:
            constantIndex = static_cast<size_t>(ConstantIndex::CLASS_CONSTRUCTOR_METHOD_INDEX);
            break;
        case FunctionKind::BUILTIN_PROXY_CONSTRUCTOR:
            constantIndex = static_cast<size_t>(ConstantIndex::BUILTIN_PROXY_CONSTRUCTOR_METHOD_INDEX);
            break;
        case FunctionKind::BUILTIN_CONSTRUCTOR:
            constantIndex = static_cast<size_t>(ConstantIndex::BUILTIN_CONSTRUCTOR_METHOD_INDEX);
            break;
        case FunctionKind::DERIVED_CONSTRUCTOR:
            constantIndex = static_cast<size_t>(ConstantIndex::DERIVED_CONSTRUCTOR_METHOD_INDEX);
            break;
        case FunctionKind::GENERATOR_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::GENERATOR_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::NONE_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::NONE_FUNCTION_METHOD_INDEX);
            break;
        case FunctionKind::ASYNC_GENERATOR_FUNCTION:
            constantIndex = static_cast<size_t>(ConstantIndex::ASYNC_GENERATOR_FUNCTION_METHOD_INDEX);
            break;
        default:
            LOG_ECMA(FATAL) << "cannot get method for native function of kind "
                << std::to_string(static_cast<int>(kind));
            UNREACHABLE();
    }
    JSTaggedValue methodValue = thread_->GlobalConstants()->GetGlobalConstantObject(constantIndex);
    return methodValue;
}

JSHandle<JSFunction> ObjectFactory::NewNativeFunctionByHClass(const JSHandle<JSHClass> &clazz,
                                                              const void *nativeFunc,
                                                              FunctionKind kind,
                                                              MemSpaceType type)
{
    JSHandle<JSFunction> function;
    switch (type) {
        case MemSpaceType::SEMI_SPACE:
            function = JSHandle<JSFunction>::Cast(NewJSObject(clazz));
            break;
        case MemSpaceType::OLD_SPACE:
            function = JSHandle<JSFunction>::Cast(NewOldSpaceJSObject(clazz));
            break;
        case MemSpaceType::NON_MOVABLE:
            function = JSHandle<JSFunction>::Cast(NewNonMovableJSObject(clazz));
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    clazz->SetCallable(true);
    clazz->SetExtensible(true);
    JSFunction::InitializeJSFunction(thread_, function, kind);
    function->SetMethod(thread_, GetReadOnlyMethodForNativeFunction(kind));
    function->SetNativePointer(const_cast<void *>(nativeFunc));
    function->SetTaskConcurrentFuncFlag(0); // 0 : default value
    return function;
}

JSHandle<JSFunction> ObjectFactory::NewJSFunctionByHClass(const void *func, const JSHandle<JSHClass> &clazz,
                                                          FunctionKind kind)
{
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(NewJSObject(clazz));
    clazz->SetCallable(true);
    clazz->SetExtensible(true);
    JSFunction::InitializeJSFunction(thread_, function, kind);
    function->SetMethod(thread_, GetReadOnlyMethodForNativeFunction(kind));
    function->SetNativePointer(const_cast<void *>(func));
    return function;
}

// new function with name/length accessor
JSHandle<JSFunction> ObjectFactory::NewJSFunctionByHClassWithoutAccessor(const void *func,
    const JSHandle<JSHClass> &clazz, FunctionKind kind)
{
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(NewJSObject(clazz));
    clazz->SetCallable(true);
    clazz->SetExtensible(true);
    JSFunction::InitializeWithDefaultValue(thread_, function);
    function->SetMethod(thread_, GetReadOnlyMethodForNativeFunction(kind));
    function->SetNativePointer(const_cast<void *>(func));
    return function;
}

void ObjectFactory::InitializeMethod(const MethodLiteral *methodLiteral, JSHandle<Method> &method)
{
    if (methodLiteral != nullptr) {
        method->SetCallField(methodLiteral->GetCallField());
        method->SetLiteralInfo(methodLiteral->GetLiteralInfo());
        method->SetNativePointerOrBytecodeArray(const_cast<void *>(methodLiteral->GetNativePointer()));
        method->SetExtraLiteralInfo(methodLiteral->GetExtraLiteralInfo());
        method->SetExpectedPropertyCount(methodLiteral->GetExpectedPropertyCount());
    } else {
        method->SetCallField(0ULL);
        method->SetLiteralInfo(0ULL);
        method->SetNativePointerOrBytecodeArray(nullptr);
        method->SetExtraLiteralInfo(0ULL);
        method->SetExpectedPropertyCount(MethodLiteral::MAX_EXPECTED_PROPERTY_COUNT);
    }
    method->SetCodeEntryOrLiteral(reinterpret_cast<uintptr_t>(methodLiteral));
    method->SetConstantPool(thread_, JSTaggedValue::Undefined());
    method->SetFpDelta(0);
}

JSHandle<Method> ObjectFactory::NewMethod(const MethodLiteral *methodLiteral, MemSpaceType spaceType)
{
    NewObjectHook();
    TaggedObject *header = nullptr;
    if (spaceType == NON_MOVABLE) {
        header = heap_->AllocateNonMovableOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetMethodClass().GetTaggedObject()));
    } else {
        header = heap_->AllocateOldOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetMethodClass().GetTaggedObject()));
    }
    JSHandle<Method> method(thread_, header);
    InitializeMethod(methodLiteral, method);
    return method;
}

JSHandle<Method> ObjectFactory::NewMethod(const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral,
                                          JSHandle<ConstantPool> constpool, uint32_t entryIndex,
                                          bool needSetAotFlag, bool *canFastCall)
{
    JSHandle<Method> method;
    if (jsPandaFile->IsNewVersion()) {
        method = Method::Create(thread_, jsPandaFile, methodLiteral);
    } else {
        method = NewMethod(methodLiteral);
        method->SetConstantPool(thread_, constpool);
    }
    if (needSetAotFlag) {
        thread_->GetEcmaVM()->GetAOTFileManager()->
            SetAOTFuncEntry(jsPandaFile, nullptr, *method, entryIndex, canFastCall);
    } else {
        method->InitInterpreterStatusForCompiledMethod(thread_);
    }
    return method;
}

JSHandle<JSFunction> ObjectFactory::NewJSNativeErrorFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc)
{
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetNativeErrorFunctionClass());
    return NewNativeFunctionByHClass(hclass, nativeFunc, FunctionKind::BUILTIN_CONSTRUCTOR);
}

JSHandle<JSFunction> ObjectFactory::NewSpecificTypedArrayFunction(const JSHandle<GlobalEnv> &env,
                                                                  const void *nativeFunc,
                                                                  kungfu::BuiltinsStubCSigns::ID builtinId)
{
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetSpecificTypedArrayFunctionClass());
    if (builtinId != kungfu::BuiltinsStubCSigns::INVALID) {
        JSHandle<Method> method = NewMethodForNativeFunction(nativeFunc, FunctionKind::BUILTIN_CONSTRUCTOR, builtinId);
        return NewJSFunctionByHClass(method, hclass);
    }
    return NewNativeFunctionByHClass(hclass, nativeFunc, FunctionKind::BUILTIN_CONSTRUCTOR);
}

JSHandle<JSFunction> ObjectFactory::NewAotFunction(uint32_t numArgs, uintptr_t codeEntry)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<Method> method = NewMethodForNativeFunction(reinterpret_cast<void *>(codeEntry));
    method->SetAotCodeBit(true);
    method->SetNativeBit(false);
    method->SetNumArgsWithCallField(numArgs);
    method->SetCodeEntryOrLiteral(codeEntry);
    JSHandle<JSFunction> jsfunc = NewJSFunction(env, method);
    return jsfunc;
}

void ObjectFactory::SetNativePointerToFunctionFromMethod(const JSHandle<JSFunctionBase> &func,
                                                         const JSHandle<Method> &method)
{
    void *nativePointer = const_cast<void *>(method->GetNativePointer());
    func->SetNativePointer(nativePointer);
}

JSHandle<JSBoundFunction> ObjectFactory::NewJSBoundFunction(const JSHandle<JSTaggedValue> &target,
                                                            const JSHandle<JSTaggedValue> &boundThis,
                                                            const JSHandle<TaggedArray> &args)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();

    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetBoundFunctionClass());
    JSHandle<JSBoundFunction> bundleFunction = JSHandle<JSBoundFunction>::Cast(NewJSObject(hclass));
    // set properties
    JSHandle<JSTaggedValue> accessor = globalConst->GetHandledFunctionNameAccessor();
    bundleFunction->SetPropertyInlinedProps(thread_, JSFunction::NAME_INLINE_PROPERTY_INDEX,
                                            accessor.GetTaggedValue());
    accessor = globalConst->GetHandledFunctionLengthAccessor();
    bundleFunction->SetPropertyInlinedProps(thread_, JSFunction::LENGTH_INLINE_PROPERTY_INDEX,
                                            accessor.GetTaggedValue());

    bundleFunction->SetBoundTarget(thread_, target);
    bundleFunction->SetBoundThis(thread_, boundThis);
    bundleFunction->SetBoundArguments(thread_, args);

    if (target.GetTaggedValue().IsConstructor()) {
        bundleFunction->SetConstructor(true);
    }
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_GLOBAL_CALL_JS_BOUND_FUNCTION));
    bundleFunction->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(bundleFunction), method);
    return bundleFunction;
}

JSHandle<JSIntlBoundFunction> ObjectFactory::NewJSIntlBoundFunction(MethodIndex idx, int functionLength)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetJSIntlBoundFunctionClass());

    JSHandle<JSIntlBoundFunction> intlBoundFunc = JSHandle<JSIntlBoundFunction>::Cast(NewJSObject(hclass));
    intlBoundFunc->SetNumberFormat(thread_, JSTaggedValue::Undefined());
    intlBoundFunc->SetDateTimeFormat(thread_, JSTaggedValue::Undefined());
    intlBoundFunc->SetCollator(thread_, JSTaggedValue::Undefined());
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(intlBoundFunc);
    JSFunction::InitializeJSFunction(thread_, function);
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(idx));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(functionLength));
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> emptyString = globalConst->GetHandledEmptyString();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    PropertyDescriptor nameDesc(thread_, emptyString, false, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread_, JSHandle<JSTaggedValue>::Cast(function), nameKey, nameDesc);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSIntlBoundFunction, thread_);
    return intlBoundFunc;
}

JSHandle<JSProxyRevocFunction> ObjectFactory::NewJSProxyRevocFunction(const JSHandle<JSProxy> &proxy)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetProxyRevocFunctionClass());

    JSHandle<JSProxyRevocFunction> revocFunction = JSHandle<JSProxyRevocFunction>::Cast(NewJSObject(hclass));
    revocFunction->SetRevocableProxy(thread_, JSTaggedValue::Undefined());
    revocFunction->SetRevocableProxy(thread_, proxy);
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(revocFunction);
    JSFunction::InitializeJSFunction(thread_, function);
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROXY_INVALIDATE_PROXY_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(0));
    JSHandle<JSTaggedValue> emptyString = globalConst->GetHandledEmptyString();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    PropertyDescriptor nameDesc(thread_, emptyString, false, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread_, JSHandle<JSTaggedValue>::Cast(function), nameKey, nameDesc);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSProxyRevocFunction, thread_);
    return revocFunction;
}

JSHandle<JSAsyncAwaitStatusFunction> ObjectFactory::NewJSAsyncAwaitStatusFunction(MethodIndex idx)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetAsyncAwaitStatusFunctionClass());

    JSHandle<JSAsyncAwaitStatusFunction> awaitFunction =
        JSHandle<JSAsyncAwaitStatusFunction>::Cast(NewJSObject(hclass));
    awaitFunction->SetAsyncContext(thread_, JSTaggedValue::Undefined());
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(awaitFunction));
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(idx));
    awaitFunction->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(awaitFunction), method);
    return awaitFunction;
}

JSHandle<JSGeneratorObject> ObjectFactory::NewJSGeneratorObject(JSHandle<JSTaggedValue> generatorFunction)
{
    JSHandle<JSTaggedValue> proto(thread_, JSHandle<JSFunction>::Cast(generatorFunction)->GetProtoOrHClass());
    if (!proto->IsECMAObject()) {
        JSHandle<GlobalEnv> realmHandle = JSObject::GetFunctionRealm(thread_, generatorFunction);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSGeneratorObject, thread_);
        proto = realmHandle->GetGeneratorPrototype();
    }
    JSHandle<JSHClass> hclass = NewEcmaHClass(JSGeneratorObject::SIZE, JSType::JS_GENERATOR_OBJECT, proto);
    JSHandle<JSGeneratorObject> generatorObject = JSHandle<JSGeneratorObject>::Cast(NewJSObjectWithInit(hclass));
    generatorObject->SetGeneratorContext(thread_, JSTaggedValue::Undefined());
    generatorObject->SetResumeResult(thread_, JSTaggedValue::Undefined());
    return generatorObject;
}

JSHandle<JSAsyncGeneratorObject> ObjectFactory::NewJSAsyncGeneratorObject(JSHandle<JSTaggedValue> generatorFunction)
{
    JSHandle<JSTaggedValue> proto(thread_, JSHandle<JSFunction>::Cast(generatorFunction)->GetProtoOrHClass());
    if (!proto->IsECMAObject()) {
        JSHandle<GlobalEnv> realmHandle = JSObject::GetFunctionRealm(thread_, generatorFunction);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSAsyncGeneratorObject, thread_);
        proto = realmHandle->GetAsyncGeneratorPrototype();
    }
    JSHandle<JSHClass> hclass = NewEcmaHClass(JSAsyncGeneratorObject::SIZE,
                                              JSType::JS_ASYNC_GENERATOR_OBJECT, proto);
    JSHandle<JSAsyncGeneratorObject> generatorObject =
        JSHandle<JSAsyncGeneratorObject>::Cast(NewJSObjectWithInit(hclass));
    return generatorObject;
}

JSHandle<JSAsyncFuncObject> ObjectFactory::NewJSAsyncFuncObject()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetInitialGenerator();
    JSHandle<JSHClass> hclass = NewEcmaHClass(JSAsyncFuncObject::SIZE, JSType::JS_ASYNC_FUNC_OBJECT, proto);
    JSHandle<JSAsyncFuncObject> asyncFuncObject = JSHandle<JSAsyncFuncObject>::Cast(NewJSObjectWithInit(hclass));
    return asyncFuncObject;
}

JSHandle<CompletionRecord> ObjectFactory::NewCompletionRecord(CompletionRecordType type, JSHandle<JSTaggedValue> value)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetCompletionRecordClass().GetTaggedObject()));
    JSHandle<CompletionRecord> obj(thread_, header);
    obj->SetType(type);
    obj->SetValue(thread_, value);
    return obj;
}

JSHandle<GeneratorContext> ObjectFactory::NewGeneratorContext()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetGeneratorContextClass().GetTaggedObject()));
    JSHandle<GeneratorContext> obj(thread_, header);
    obj->SetRegsArray(thread_, JSTaggedValue::Undefined());
    obj->SetMethod(thread_, JSTaggedValue::Undefined());
    obj->SetThis(thread_, JSTaggedValue::Undefined());
    obj->SetAcc(thread_, JSTaggedValue::Undefined());
    obj->SetGeneratorObject(thread_, JSTaggedValue::Undefined());
    obj->SetLexicalEnv(thread_, JSTaggedValue::Undefined());
    obj->SetNRegs(0);
    obj->SetBCOffset(0);
    return obj;
}

JSHandle<JSPrimitiveRef> ObjectFactory::NewJSPrimitiveRef(const JSHandle<JSFunction> &function,
                                                          const JSHandle<JSTaggedValue> &object)
{
    JSHandle<JSPrimitiveRef> obj(NewJSObjectByConstructor(function));
    obj->SetValue(thread_, object);

    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    if (function.GetTaggedValue() == env->GetStringFunction().GetTaggedValue()) {
        JSHandle<JSTaggedValue> lengthStr = thread_->GlobalConstants()->GetHandledLengthString();
        uint32_t length = EcmaStringAccessor(object.GetTaggedValue()).GetLength();
        PropertyDescriptor desc(thread_, JSHandle<JSTaggedValue>(thread_, JSTaggedValue(length)), false, false, false);
        JSTaggedValue::DefinePropertyOrThrow(thread_, JSHandle<JSTaggedValue>(obj), lengthStr, desc);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPrimitiveRef, thread_);
    }

    return obj;
}

JSHandle<JSPrimitiveRef> ObjectFactory::NewJSPrimitiveRef(PrimitiveType type, const JSHandle<JSTaggedValue> &object)
{
    ObjectFactory *factory = vm_->GetFactory();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> function;
    switch (type) {
        case PrimitiveType::PRIMITIVE_NUMBER:
            function = env->GetNumberFunction();
            break;
        case PrimitiveType::PRIMITIVE_STRING:
            function = env->GetStringFunction();
            break;
        case PrimitiveType::PRIMITIVE_SYMBOL:
            function = env->GetSymbolFunction();
            break;
        case PrimitiveType::PRIMITIVE_BOOLEAN:
            function = env->GetBooleanFunction();
            break;
        case PrimitiveType::PRIMITIVE_BIGINT:
            function = env->GetBigIntFunction();
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable, type: " << static_cast<size_t>(type);
            UNREACHABLE();
    }
    JSHandle<JSFunction> funcHandle(function);
    return factory->NewJSPrimitiveRef(funcHandle, object);
}

JSHandle<JSPrimitiveRef> ObjectFactory::NewJSString(const JSHandle<JSTaggedValue> &str,
                                                    const JSHandle<JSTaggedValue> &newTarget)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> stringFunc(env->GetStringFunction());
    JSHandle<JSPrimitiveRef> obj;
    if (newTarget->IsUndefined()) {
        obj = JSHandle<JSPrimitiveRef>::Cast(NewJSObjectByConstructor(stringFunc));
    } else {
        JSHandle<JSObject> newObject = NewJSObjectByConstructor(stringFunc, newTarget);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSPrimitiveRef, thread_);
        obj = JSHandle<JSPrimitiveRef>::Cast(newObject);
    }
    obj->SetValue(thread_, str);
    return obj;
}

JSHandle<GlobalEnv> ObjectFactory::NewGlobalEnv(JSHClass *globalEnvClass)
{
    NewObjectHook();
    // Note: Global env must be allocated in non-movable heap, since its getters will directly return
    //       the offsets of the properties as the address of Handles.
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(globalEnvClass);
    InitObjectFields(header);
    return JSHandle<GlobalEnv>(thread_, GlobalEnv::Cast(header));
}

JSHandle<LexicalEnv> ObjectFactory::NewLexicalEnv(int numSlots)
{
    NewObjectHook();
    size_t size = LexicalEnv::ComputeSize(numSlots);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetEnvClass().GetTaggedObject()), size);
    JSHandle<LexicalEnv> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Hole(), numSlots + LexicalEnv::RESERVED_ENV_LENGTH);
    return array;
}

JSHandle<JSSymbol> ObjectFactory::NewEmptySymbol()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetDescription(thread_, JSTaggedValue::Undefined());
    obj->SetFlags(0);
    obj->SetHashField(0);
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewJSSymbol()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetDescription(thread_, JSTaggedValue::Undefined());
    obj->SetFlags(0);
    obj->SetHashField(SymbolTable::Hash(obj.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewPrivateSymbol()
{
    JSHandle<JSSymbol> obj = NewJSSymbol();
    obj->SetPrivate();
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewPrivateNameSymbol(const JSHandle<JSTaggedValue> &name)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetFlags(0);
    obj->SetPrivateNameSymbol();
    obj->SetDescription(thread_, name);
    obj->SetHashField(SymbolTable::Hash(name.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewWellKnownSymbol(const JSHandle<JSTaggedValue> &name)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetFlags(0);
    obj->SetWellKnownSymbol();
    obj->SetDescription(thread_, name);
    obj->SetHashField(SymbolTable::Hash(name.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewPublicSymbol(const JSHandle<JSTaggedValue> &name)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSymbolClass().GetTaggedObject()));
    JSHandle<JSSymbol> obj(thread_, JSSymbol::Cast(header));
    obj->SetFlags(0);
    obj->SetDescription(thread_, name);
    obj->SetHashField(SymbolTable::Hash(name.GetTaggedValue()));
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewSymbolWithTable(const JSHandle<JSTaggedValue> &name)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<SymbolTable> tableHandle(env->GetRegisterSymbols());
    if (tableHandle->ContainsKey(name.GetTaggedValue())) {
        JSTaggedValue objValue = tableHandle->GetSymbol(name.GetTaggedValue());
        return JSHandle<JSSymbol>(thread_, objValue);
    }

    JSHandle<JSSymbol> obj = NewPublicSymbol(name);
    JSHandle<JSTaggedValue> valueHandle(obj);
    JSHandle<JSTaggedValue> keyHandle(name);
    JSHandle<SymbolTable> table = SymbolTable::Insert(thread_, tableHandle, keyHandle, valueHandle);
    env->SetRegisterSymbols(thread_, table);
    return obj;
}

JSHandle<JSSymbol> ObjectFactory::NewPrivateNameSymbolWithChar(std::string_view description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewPrivateNameSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewWellKnownSymbolWithChar(std::string_view description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewWellKnownSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewPublicSymbolWithChar(std::string_view description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewPublicSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewSymbolWithTableWithChar(std::string_view description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewSymbolWithTable(JSHandle<JSTaggedValue>(string));
}

JSHandle<AccessorData> ObjectFactory::NewAccessorData()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetAccessorDataClass().GetTaggedObject()));
    JSHandle<AccessorData> acc(thread_, AccessorData::Cast(header));
    acc->SetGetter(thread_, JSTaggedValue::Undefined());
    acc->SetSetter(thread_, JSTaggedValue::Undefined());
    return acc;
}

JSHandle<AccessorData> ObjectFactory::NewInternalAccessor(void *setter, void *getter)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetInternalAccessorClass().GetTaggedObject()));
    JSHandle<InternalAccessor> obj(thread_, InternalAccessor::Cast(header));

    obj->SetSetter(reinterpret_cast<InternalAccessor::InternalSetFunc>(setter));
    obj->SetGetter(reinterpret_cast<InternalAccessor::InternalGetFunc>(getter));
    return JSHandle<AccessorData>::Cast(obj);
}

JSHandle<PromiseCapability> ObjectFactory::NewPromiseCapability()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetCapabilityRecordClass().GetTaggedObject()));
    JSHandle<PromiseCapability> obj(thread_, header);
    obj->SetPromise(thread_, JSTaggedValue::Undefined());
    obj->SetResolve(thread_, JSTaggedValue::Undefined());
    obj->SetReject(thread_, JSTaggedValue::Undefined());
    return obj;
}

JSHandle<PromiseReaction> ObjectFactory::NewPromiseReaction()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetReactionsRecordClass().GetTaggedObject()));
    JSHandle<PromiseReaction> obj(thread_, header);
    obj->SetPromiseCapability(thread_, JSTaggedValue::Undefined());
    obj->SetHandler(thread_, JSTaggedValue::Undefined());
    obj->SetType(PromiseType::RESOLVE);
    return obj;
}

JSHandle<PromiseIteratorRecord> ObjectFactory::NewPromiseIteratorRecord(const JSHandle<JSTaggedValue> &itor, bool done)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPromiseIteratorRecordClass().GetTaggedObject()));
    JSHandle<PromiseIteratorRecord> obj(thread_, header);
    obj->SetIterator(thread_, itor.GetTaggedValue());
    obj->SetDone(done);
    return obj;
}

JSHandle<job::MicroJobQueue> ObjectFactory::NewMicroJobQueue()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetMicroJobQueueClass().GetTaggedObject()));
    JSHandle<job::MicroJobQueue> obj(thread_, header);
    obj->SetPromiseJobQueue(thread_, GetEmptyTaggedQueue().GetTaggedValue());
    obj->SetScriptJobQueue(thread_, GetEmptyTaggedQueue().GetTaggedValue());
    return obj;
}

JSHandle<job::PendingJob> ObjectFactory::NewPendingJob(const JSHandle<JSFunction> &func,
                                                       const JSHandle<TaggedArray> &argv)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPendingJobClass().GetTaggedObject()));
    JSHandle<job::PendingJob> obj(thread_, header);
    obj->SetJob(thread_, func.GetTaggedValue());
    obj->SetArguments(thread_, argv.GetTaggedValue());
#if defined(ENABLE_HITRACE)
    obj->SetChainId(0);
    obj->SetSpanId(0);
    obj->SetParentSpanId(0);
    obj->SetFlags(0);
    obj->SetJobId(0);
#endif
    return obj;
}

JSHandle<JSProxy> ObjectFactory::NewJSProxy(const JSHandle<JSTaggedValue> &target,
                                            const JSHandle<JSTaggedValue> &handler)
{
    NewObjectHook();
    TaggedObject *header = nullptr;
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSObject> emptyObject = OrdinaryNewJSObjectCreate(vm_->GetGlobalEnv()->GetObjectFunctionPrototype());

    if (target->IsCallable()) {
        auto jsProxyCallableClass = JSHClass::Cast(globalConst->GetJSProxyCallableClass().GetTaggedObject());
        auto jsProxyConstructClass = JSHClass::Cast(globalConst->GetJSProxyConstructClass().GetTaggedObject());
        header = target->IsConstructor() ? heap_->AllocateYoungOrHugeObject(jsProxyConstructClass)
                                         : heap_->AllocateYoungOrHugeObject(jsProxyCallableClass);
    } else {
        header = heap_->AllocateYoungOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetJSProxyOrdinaryClass().GetTaggedObject()));
    }

    JSHandle<JSProxy> proxy(thread_, header);
    proxy->InitializeHash();
    proxy->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_GLOBAL_CALL_JS_PROXY));
    proxy->SetTarget(thread_, target.GetTaggedValue());
    proxy->SetHandler(thread_, handler.GetTaggedValue());
    proxy->SetPrivateField(thread_, emptyObject.GetTaggedValue());
    proxy->SetIsRevoked(false);
    return proxy;
}

JSHandle<JSRealm> ObjectFactory::NewJSRealm()
{
    JSHandle<JSHClass> hClassHandle = NewEcmaHClassClass(nullptr, JSHClass::SIZE, JSType::HCLASS);
    JSHClass *hclass = reinterpret_cast<JSHClass *>(hClassHandle.GetTaggedValue().GetTaggedObject());
    hclass->SetClass(thread_, hclass);
    JSHandle<JSHClass> realmEnvClass = NewEcmaHClass(*hClassHandle, GlobalEnv::SIZE, JSType::GLOBAL_ENV);
    JSHandle<GlobalEnv> realmEnvHandle = NewGlobalEnv(*realmEnvClass);

    auto result = TemplateMap::Create(thread_);
    realmEnvHandle->SetTemplateMap(thread_, result);
    realmEnvHandle->SetJSThread(thread_);
    Builtins builtins;
    builtins.Initialize(realmEnvHandle, thread_, false, true);
    JSHandle<JSTaggedValue> protoValue = thread_->GlobalConstants()->GetHandledJSRealmClass();
    JSHandle<JSHClass> hclassHandle = NewEcmaHClass(JSRealm::SIZE, JSType::JS_REALM, protoValue);
    JSHandle<JSRealm> realm(NewJSObject(hclassHandle));
    realm->SetGlobalEnv(thread_, realmEnvHandle.GetTaggedValue());
    realm->SetValue(thread_, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> realmObj = realmEnvHandle->GetJSGlobalObject();
    JSHandle<JSTaggedValue> realmkey(thread_->GlobalConstants()->GetHandledGlobalString());
    PropertyDescriptor realmDesc(thread_, JSHandle<JSTaggedValue>::Cast(realmObj), true, false, true);
    [[maybe_unused]] bool status =
        JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(realm), realmkey, realmDesc);
    ASSERT_PRINT(status == true, "Realm defineOwnProperty failed");

    return realm;
}

JSHandle<TaggedArray> ObjectFactory::NewTaggedArray(uint32_t length, JSTaggedValue initVal, bool nonMovable)
{
    if (nonMovable) {
        return NewTaggedArray(length, initVal, MemSpaceType::NON_MOVABLE);
    }
    return NewTaggedArray(length, initVal, MemSpaceType::SEMI_SPACE);
}

JSHandle<TaggedArray> ObjectFactory::NewTaggedArray(uint32_t length, JSTaggedValue initVal, MemSpaceType spaceType)
{
    NewObjectHook();
    if (length == 0) {
        return EmptyArray();
    }

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    TaggedObject *header = nullptr;
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    switch (spaceType) {
        case MemSpaceType::SEMI_SPACE:
            header = heap_->AllocateYoungOrHugeObject(arrayClass, size);
            break;
        case MemSpaceType::OLD_SPACE:
            header = heap_->AllocateOldOrHugeObject(arrayClass, size);
            break;
        case MemSpaceType::NON_MOVABLE:
            header = heap_->AllocateNonMovableOrHugeObject(arrayClass, size);
            break;
        case MemSpaceType::SHARED_OLD_SPACE:
            header = sHeap_->AllocateOldOrHugeObject(thread_, arrayClass, size);
            break;
        case MemSpaceType::SHARED_NON_MOVABLE:
            header = sHeap_->AllocateNonMovableOrHugeObject(thread_, arrayClass, size);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    JSHandle<TaggedArray> array(thread_, header);
    array->InitializeWithSpecialValue(initVal, length);
    return array;
}

JSHandle<TaggedArray> ObjectFactory::NewAndCopyTaggedArray(JSHandle<TaggedArray> &srcElements, uint32_t newLength,
                                                           uint32_t oldLength, uint32_t k)
{
    ASSERT(oldLength <= newLength);
    MemSpaceType spaceType = newLength < LENGTH_THRESHOLD ? MemSpaceType::SEMI_SPACE : MemSpaceType::OLD_SPACE;
    JSHandle<TaggedArray> dstElements = NewTaggedArrayWithoutInit(newLength, spaceType);
    dstElements->SetExtraLength(srcElements->GetExtraLength());
    if (newLength == 0) {
        return dstElements;
    }
    dstElements->Copy(thread_, 0, k, srcElements.GetObject<TaggedArray>(), oldLength);
    for (uint32_t i = oldLength; i < newLength; i++) {
        dstElements->Set(thread_, i, JSTaggedValue::Hole());
    }
    return dstElements;
}

JSHandle<TaggedArray> ObjectFactory::NewAndCopyTaggedArraySkipBarrier(JSHandle<TaggedArray> &srcElements,
    uint32_t newLength, uint32_t oldLength, uint32_t k)
{
    ASSERT(oldLength <= newLength);
    MemSpaceType spaceType = newLength < LENGTH_THRESHOLD ? MemSpaceType::SEMI_SPACE : MemSpaceType::OLD_SPACE;
    JSHandle<TaggedArray> dstElements = NewTaggedArrayWithoutInit(newLength, spaceType);
    if (newLength == 0) {
        return dstElements;
    }
    dstElements->Copy<false>(thread_, 0, k, srcElements.GetObject<TaggedArray>(), oldLength);
    for (uint32_t i = oldLength; i < newLength; i++) {
        dstElements->Set(thread_, i, JSTaggedValue::Hole());
    }
    return dstElements;
}

JSHandle<TaggedArray> ObjectFactory::NewAndCopySNameDictionary(JSHandle<TaggedArray> &srcElements, uint32_t length)
{
    JSHandle<TaggedArray> dstElements = NewSDictionaryArray(length);
    if (length == 0) {
        return dstElements;
    }
    for (uint32_t i = 0; i < length; i++) {
        dstElements->Set(thread_, i, srcElements->Get(i));
    }
    return dstElements;
}

JSHandle<JSObject> ObjectFactory::NewAndCopyJSArrayObject(JSHandle<JSObject> thisObjHandle, uint32_t newLength,
                                                          uint32_t oldLength, uint32_t k)
{
    ASSERT(oldLength <= newLength);
    JSHandle<TaggedArray> dstElements(NewTaggedArray(newLength));
    JSHandle<JSTaggedValue> holeHandle(thread_, JSTaggedValue::Hole());
    JSHandle<JSObject> arrayObj = JSHandle<JSObject>(NewJSStableArrayWithElements(dstElements));
    if (newLength == 0) {
        return JSHandle<JSObject>(arrayObj);
    }
    for (uint32_t i = 0; i < oldLength; i++) {
        JSHandle<JSTaggedValue> value(thread_, ElementAccessor::Get(thread_, thisObjHandle, i + k));
        ElementAccessor::Set(thread_, arrayObj, i, value, true);
    }
    for (uint32_t i = oldLength; i < newLength; i++) {
        ElementAccessor::Set(thread_, arrayObj, i, holeHandle, true);
    }
    return arrayObj;
}

JSHandle<TaggedArray> ObjectFactory::NewAndCopyTaggedArrayByObject(JSHandle<JSObject> thisObjHandle, uint32_t newLength,
                                                                   uint32_t oldLength, uint32_t k)
{
    ASSERT(oldLength <= newLength);
    MemSpaceType spaceType = newLength < LENGTH_THRESHOLD ? MemSpaceType::SEMI_SPACE : MemSpaceType::OLD_SPACE;
    JSHandle<TaggedArray> dstElements(NewTaggedArrayWithoutInit(newLength, spaceType));
    TaggedArray *srcElements = TaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    dstElements->SetExtraLength(srcElements->GetExtraLength());
    if (newLength == 0) {
        return dstElements;
    }

    for (uint32_t i = 0; i < oldLength; i++) {
        dstElements->Set(thread_, i, ElementAccessor::Get(thread_, thisObjHandle, i + k));
    }
    for (uint32_t i = oldLength; i < newLength; i++) {
        dstElements->Set(thread_, i, JSTaggedValue::Hole());
    }
    return dstElements;
}

JSHandle<MutantTaggedArray> ObjectFactory::NewAndCopyMutantTaggedArrayByObject(JSHandle<JSObject> thisObjHandle,
                                                                               uint32_t newLength, uint32_t oldLength,
                                                                               uint32_t k)
{
    ASSERT(oldLength <= newLength);
    MemSpaceType spaceType = newLength < LENGTH_THRESHOLD ? MemSpaceType::SEMI_SPACE : MemSpaceType::OLD_SPACE;
    JSHandle<MutantTaggedArray> dstElements(NewMutantTaggedArrayWithoutInit(newLength, spaceType));
    MutantTaggedArray *srcElements = MutantTaggedArray::Cast(thisObjHandle->GetElements().GetTaggedObject());
    dstElements->SetExtraLength(srcElements->GetExtraLength());
    if (newLength == 0) {
        return dstElements;
    }
    for (uint32_t i = 0; i < oldLength; i++) {
        ElementsKind kind = thisObjHandle->GetClass()->GetElementsKind();
        JSTaggedValue value = JSTaggedValue(ElementAccessor::ConvertTaggedValueWithElementsKind(
            ElementAccessor::Get(thread_, thisObjHandle, i + k), kind));
        dstElements->Set<false>(thread_, i, value);
    }
    for (uint32_t i = oldLength; i < newLength; i++) {
        ElementsKind kind = thisObjHandle->GetClass()->GetElementsKind();
        JSTaggedValue value = JSTaggedValue(ElementAccessor::ConvertTaggedValueWithElementsKind(JSTaggedValue::Hole(),
                                                                                                kind));
        dstElements->Set<false>(thread_, i, value);
    }
    return dstElements;
}

// private
JSHandle<TaggedArray> ObjectFactory::NewTaggedArrayWithoutInit(uint32_t length, MemSpaceType spaceType)
{
    NewObjectHook();
    if (length == 0) {
        return EmptyArray();
    }

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    TaggedObject *header = nullptr;
    auto arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    switch (spaceType) {
        case MemSpaceType::SEMI_SPACE:
            header = heap_->AllocateYoungOrHugeObject(arrayClass, size);
            break;
        case MemSpaceType::OLD_SPACE:
            header = heap_->AllocateOldOrHugeObject(arrayClass, size);
            break;
        case MemSpaceType::SHARED_OLD_SPACE:
            header = sHeap_->AllocateOldOrHugeObject(thread_, arrayClass, size);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    JSHandle<TaggedArray> array(thread_, header);
    array->SetLength(length);
    return array;
}

// private
JSHandle<MutantTaggedArray> ObjectFactory::NewMutantTaggedArrayWithoutInit(uint32_t length, MemSpaceType spaceType)
{
    NewObjectHook();
    if (length == 0) {
        return EmptyMutantArray();
    }

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    TaggedObject *header = nullptr;
    auto arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetMutantTaggedArrayClass().GetTaggedObject());
    switch (spaceType) {
        case MemSpaceType::SEMI_SPACE:
            header = heap_->AllocateYoungOrHugeObject(arrayClass, size);
            break;
        case MemSpaceType::OLD_SPACE:
            header = heap_->AllocateOldOrHugeObject(arrayClass, size);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    JSHandle<MutantTaggedArray> array(thread_, header);
    array->SetLength(length);
    return array;
}

JSHandle<TaggedArray> ObjectFactory::NewTaggedArray(uint32_t length, JSTaggedValue initVal)
{
    NewObjectHook();
    if (length == 0) {
        return EmptyArray();
    }
    MemSpaceType spaceType = length < LENGTH_THRESHOLD ? MemSpaceType::SEMI_SPACE : MemSpaceType::OLD_SPACE;
    JSHandle<TaggedArray> array = NewTaggedArrayWithoutInit(length, spaceType);
    array->InitializeWithSpecialValue(initVal, length);
    return array;
}

JSHandle<COWTaggedArray> ObjectFactory::NewCOWTaggedArray(uint32_t length, JSTaggedValue initVal)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetCOWArrayClass().GetTaggedObject()), size);
    JSHandle<COWTaggedArray> cowArray(thread_, header);
    cowArray->InitializeWithSpecialValue(initVal, length);
    return cowArray;
}

JSHandle<COWMutantTaggedArray> ObjectFactory::NewCOWMutantTaggedArray(uint32_t length, JSTaggedType initVal)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetCOWMutantTaggedArrayClass().GetTaggedObject()), size);
    JSHandle<COWMutantTaggedArray> cowMutantTaggedArray(thread_, header);
    cowMutantTaggedArray->InitializeWithSpecialValue(initVal, length);
    return cowMutantTaggedArray;
}

JSHandle<MutantTaggedArray> ObjectFactory::NewMutantTaggedArray(uint32_t length, JSTaggedType initVal)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetMutantTaggedArrayClass().GetTaggedObject()), size);
    JSHandle<MutantTaggedArray> mutantTaggedArray(thread_, header);
    mutantTaggedArray->InitializeWithSpecialValue(initVal, length);
    return mutantTaggedArray;
}

JSHandle<TaggedHashArray> ObjectFactory::NewTaggedHashArray(uint32_t length)
{
    if (length == 0) {
        return JSHandle<TaggedHashArray>::Cast(EmptyArray());
    }

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedHashArray> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Hole(), length);
    return array;
}

JSHandle<ByteArray> ObjectFactory::NewByteArray(uint32_t length, uint32_t size, void *srcData,
                                                MemSpaceType spaceType)
{
    size_t byteSize = ByteArray::ComputeSize(size, length);
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetByteArrayClass().GetTaggedObject());
    TaggedObject *header = AllocObjectWithSpaceType(byteSize, arrayClass, spaceType);
    JSHandle<ByteArray> array(thread_, header);

    void *data = array->GetData();

    if (srcData != nullptr) {
        if (memcpy_s(data, length * size, srcData, length * size) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    } else {
        if (memset_s(data, length * size, 0, length * size) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
    }

    array->SetArrayLength(length);
    array->SetByteLength(size);
    return array;
}

JSHandle<LinkedNode> ObjectFactory::NewLinkedNode(int hash, const JSHandle<JSTaggedValue> &key,
                                                  const JSHandle<JSTaggedValue> &value,
                                                  const JSHandle<LinkedNode> &next)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(globalConst->GetLinkedNode().GetTaggedObject()), LinkedNode::SIZE);
    JSHandle<LinkedNode> node(thread_, header);
    node->InitLinkedNode(thread_, hash, key, value, next);

    return node;
}

JSHandle<RBTreeNode> ObjectFactory::NewTreeNode(int hash, const JSHandle<JSTaggedValue> &key,
                                                const JSHandle<JSTaggedValue> &value)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(globalConst->GetRBTreeNode().GetTaggedObject()), RBTreeNode::SIZE);
    JSHandle<RBTreeNode> treenode(thread_, header);
    treenode->InitRBTreeNode(thread_, hash, key, value, 1);
    return treenode;
}

JSHandle<TaggedArray> ObjectFactory::NewDictionaryArray(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetDictionaryClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length);

    return array;
}

JSHandle<TaggedArray> ObjectFactory::ExtendArray(const JSHandle<TaggedArray> &old, uint32_t length,
                                                 JSTaggedValue initVal, MemSpaceType type,
                                                 [[maybe_unused]] ElementsKind kind)
{
    ASSERT(length >= old->GetLength());
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    JSHClass *arrayClass = nullptr;
    // If old element is Mutantarray, need conversion according to kind.
    if (old->GetClass()->IsMutantTaggedArray()) {
        arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetMutantTaggedArrayClass().GetTaggedObject());
    } else {
        arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    }
    TaggedObject *header = AllocObjectWithSpaceType(size, arrayClass, type);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(length);
    newArray->SetExtraLength(old->GetExtraLength());

    uint32_t oldLength = old->GetLength();
    uint32_t index = 0;
    auto isMutantTaggedArray = old->GetClass()->IsMutantTaggedArray();
    for (; index < oldLength; ++index) {
        if (isMutantTaggedArray) {
            newArray->Set<false>(thread_, index, old->Get(index));
        } else {
            newArray->Set(thread_, index, old->Get(index));
        }
    }
    auto isSpecialHole = initVal.IsHole() && isMutantTaggedArray;
    JSTaggedValue specialHole = JSTaggedValue(base::SPECIAL_HOLE);
    for (; index < length; ++index) {
        if (isSpecialHole) {
            newArray->Set<false>(thread_, index, specialHole);
        } else {
            newArray->Set(thread_, index, initVal);
        }
    }

    return newArray;
}

JSHandle<TaggedArray> ObjectFactory::CopyPartArray(const JSHandle<TaggedArray> &old, uint32_t start,
                                                   uint32_t end)
{
    ASSERT(start <= end);
    ASSERT(end <= old->GetLength());

    uint32_t newLength = end - start;
    if (newLength == 0) {
        return EmptyArray();
    }

    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->InitializeWithSpecialValue(JSTaggedValue::Hole(), newLength, old->GetExtraLength());

    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i + start);
        if (value.IsHole()) {
            break;
        }
        newArray->Set(thread_, i, value);
    }
    return newArray;
}

JSHandle<TaggedArray> ObjectFactory::CopyArray(const JSHandle<TaggedArray> &old, uint32_t oldLength, uint32_t newLength,
                                               JSTaggedValue initVal, MemSpaceType type, ElementsKind kind)
{
    if (newLength == 0) {
        return EmptyArray();
    }
    if (newLength > oldLength) {
        return ExtendArray(old, newLength, initVal, type, kind);
    }
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    TaggedObject *header = nullptr;
    if (type == MemSpaceType::NON_MOVABLE) {
        // COW array is shared in nonmovable space.
        JSHClass *cowArrayClass = nullptr;
        if (old->GetClass()->IsMutantTaggedArray()) {
            cowArrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetCOWMutantTaggedArrayClass()
                                           .GetTaggedObject());
        } else {
            cowArrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetCOWArrayClass().GetTaggedObject());
        }
        header = AllocObjectWithSpaceType(size, cowArrayClass, type);
    } else {
        JSHClass *arrayClass = nullptr;
        if (old->GetClass()->IsMutantTaggedArray()) {
            arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetMutantTaggedArrayClass().GetTaggedObject());
        } else {
            arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
        }
        header = AllocObjectWithSpaceType(size, arrayClass, type);
    }

    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);
    newArray->SetExtraLength(old->GetExtraLength());
    if (old->GetClass()->IsMutantTaggedArray()) {
        newArray->Copy<false>(thread_, 0, 0, *old, newLength);
    } else {
        newArray->Copy(thread_, 0, 0, *old, newLength);
    }

    return newArray;
}

JSHandle<TaggedArray> ObjectFactory::CopyFromEnumCache(const JSHandle<TaggedArray> &old)
{
    NewObjectHook();
    uint32_t oldLength = old->GetLength();
    uint32_t newLength = oldLength - EnumCache::ENUM_CACHE_HEADER_SIZE;
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);
    newArray->SetExtraLength(old->GetExtraLength());

    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i + EnumCache::ENUM_CACHE_HEADER_SIZE);
        newArray->Set(thread_, i, value);
    }
    return newArray;
}

JSHandle<LayoutInfo> ObjectFactory::CreateLayoutInfo(int properties, MemSpaceType type, GrowMode mode)
{
    int growLength =
        mode == GrowMode::GROW ? static_cast<int>(LayoutInfo::ComputeGrowCapacity(properties)) : properties;
    uint32_t arrayLength = LayoutInfo::ComputeArrayLength(growLength);
    JSHandle<LayoutInfo> layoutInfoHandle = JSHandle<LayoutInfo>::Cast(NewTaggedArrayWithoutInit(arrayLength, type));
    layoutInfoHandle->Initialize(thread_);
    return layoutInfoHandle;
}

JSHandle<LayoutInfo> ObjectFactory::ExtendLayoutInfo(const JSHandle<LayoutInfo> &old, int properties)
{
    ASSERT(properties >= old->NumberOfElements());
    uint32_t arrayLength = LayoutInfo::ComputeArrayLength(LayoutInfo::ComputeGrowCapacity(properties));
    ASSERT(arrayLength > old->GetLength());

    auto oldArray = JSHandle<TaggedArray>(old);
    auto newArray = NewTaggedArrayWithoutInit(arrayLength, MemSpaceType::SEMI_SPACE);
    JSHandle<LayoutInfo>::Cast(newArray)->Initialize(thread_, oldArray->GetExtraLength());

    uint32_t oldLength = old->GetLength();
    for (uint32_t i = 0; i < oldLength; i++) {
        JSTaggedValue value = oldArray->Get(i);
        newArray->Set(thread_, i, value);
    }
    return JSHandle<LayoutInfo>::Cast(newArray);
}

JSHandle<LayoutInfo> ObjectFactory::CopyLayoutInfo(const JSHandle<LayoutInfo> &old)
{
    uint32_t newLength = old->GetLength();
    return JSHandle<LayoutInfo>(CopyArray(
        JSHandle<TaggedArray>::Cast(old), newLength, newLength, JSTaggedValue::Hole(),
        old.GetTaggedValue().IsInSharedHeap() ? MemSpaceType::SHARED_OLD_SPACE : MemSpaceType::SEMI_SPACE));
}

JSHandle<LayoutInfo> ObjectFactory::CopyAndReSort(const JSHandle<LayoutInfo> &old, int end, int capacity)
{
    ASSERT(capacity >= end);
    JSHandle<LayoutInfo> newArr = CreateLayoutInfo(capacity);
    Span<struct Properties> sp(old->GetProperties(), end);
    for (int i = 0; i < end; i++) {
        newArr->AddKey(thread_, i, sp[i].key_, PropertyAttributes(sp[i].attr_));
    }

    return newArr;
}

JSHandle<ConstantPool> ObjectFactory::NewConstantPool(uint32_t capacity)
{
    NewObjectHook();
    size_t size = ConstantPool::ComputeSize(capacity);
    auto header = heap_->AllocateOldOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetConstantPoolClass().GetTaggedObject()), size);
    JSHandle<ConstantPool> array(thread_, header);
    array->InitializeWithSpecialValue(thread_, JSTaggedValue::Hole(), capacity);
    return array;
}

JSHandle<Program> ObjectFactory::NewProgram()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetProgramClass().GetTaggedObject()));
    JSHandle<Program> p(thread_, header);
    p->InitializeHash();
    p->SetMainFunction(thread_, JSTaggedValue::Undefined());
    return p;
}

JSHandle<ModuleNamespace> ObjectFactory::NewModuleNamespace()
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetModuleNamespaceClass());
    JSHandle<JSObject> obj = NewJSObject(hclass);

    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    moduleNamespace->SetModule(thread_, JSTaggedValue::Undefined());
    moduleNamespace->SetExports(thread_, JSTaggedValue::Undefined());
    moduleNamespace->SetDeregisterProcession(thread_, JSTaggedValue::Undefined());
    return moduleNamespace;
}

JSHandle<NativeModuleFailureInfo> ObjectFactory::NewNativeModuleFailureInfo()
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetNativeModuleFailureInfoClass());
    JSHandle<JSObject> obj = NewJSObject(hclass);

    JSHandle<NativeModuleFailureInfo> nativeModuleFailureInfo = JSHandle<NativeModuleFailureInfo>::Cast(obj);
    nativeModuleFailureInfo->SetArkNativeModuleFailureInfo(thread_, JSTaggedValue::Undefined());
    return nativeModuleFailureInfo;
}

JSHandle<CjsModule> ObjectFactory::NewCjsModule()
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> moduleObj(env->GetCjsModuleFunction());
    JSHandle<CjsModule> cjsModule = JSHandle<CjsModule>(NewJSObjectByConstructor(moduleObj));
    return cjsModule;
}

JSHandle<CjsExports> ObjectFactory::NewCjsExports()
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> exportsObj(env->GetCjsExportsFunction());
    JSHandle<CjsExports> cjsExports = JSHandle<CjsExports>(NewJSObjectByConstructor(exportsObj));
    return cjsExports;
}

JSHandle<CjsRequire> ObjectFactory::NewCjsRequire()
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> requireObj(env->GetCjsRequireFunction());
    JSHandle<CjsRequire> cjsRequire = JSHandle<CjsRequire>(NewJSObjectByConstructor(requireObj));
    return cjsRequire;
}

JSHandle<EcmaString> ObjectFactory::GetEmptyString() const
{
    return JSHandle<EcmaString>(thread_->GlobalConstants()->GetHandledEmptyString());
}

JSHandle<TaggedArray> ObjectFactory::EmptyArray() const
{
    return JSHandle<TaggedArray>(thread_->GlobalConstants()->GetHandledEmptyArray());
}

JSHandle<MutantTaggedArray> ObjectFactory::EmptyMutantArray() const
{
    return JSHandle<MutantTaggedArray>(thread_->GlobalConstants()->GetHandledEmptyArray());
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(const uint8_t *utf8Data, uint32_t utf8Len,
                                                             bool canBeCompress) const
{
    NewObjectHook();
    if (utf8Len == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(vm_, utf8Data, utf8Len, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::GetCompressedSubStringFromStringTable(const JSHandle<EcmaString> &string,
                                                                          uint32_t offset, uint32_t utf8Len) const
{
    NewObjectHook();
    if (UNLIKELY(utf8Len == 0)) {
        return GetEmptyString();
    }
    auto *stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_,
        stringTable->GetOrInternStringFromCompressedSubString(vm_, string, offset, utf8Len));
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTableReadOnly(const uint8_t *utf8Data, uint32_t utf8Len,
                                                                     bool canBeCompress) const
{
    NewObjectHook();
    if (utf8Len == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(vm_, utf8Data, utf8Len, canBeCompress,
                                                                        MemSpaceType::SHARED_READ_ONLY_SPACE));
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(const uint16_t *utf16Data, uint32_t utf16Len,
                                                             bool canBeCompress) const
{
    NewObjectHook();
    if (utf16Len == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(vm_, utf16Data, utf16Len, canBeCompress));
}

// NB! don't do special case for C0 80, it means '\u0000', so don't convert to UTF-8
EcmaString *ObjectFactory::GetRawStringFromStringTable(StringData sd, MemSpaceType type, bool isConstantString,
    uint32_t idOffset) const
{
    NewObjectHook();
    uint32_t utf16Len = sd.utf16_length;
    if (UNLIKELY(utf16Len == 0)) {
        return *GetEmptyString();
    }

    bool canBeCompressed = sd.is_ascii;
    const uint8_t *mutf8Data = sd.data;
    if (canBeCompressed) {
        // This branch will use constant string, which has a pointer at the string in the pandafile.
        return vm_->GetEcmaStringTable()->GetOrInternString(vm_, mutf8Data, utf16Len, true, type,
                                                            isConstantString, idOffset);
    }
    return vm_->GetEcmaStringTable()->GetOrInternString(vm_, mutf8Data, utf16Len, type);
}

// used in jit thread, which unsupport create jshandle
EcmaString *ObjectFactory::GetRawStringFromStringTableWithoutJSHandle(StringData sd, MemSpaceType type,
    bool isConstantString, uint32_t idOffset) const
{
    NewObjectHook();
    uint32_t utf16Len = sd.utf16_length;
    if (UNLIKELY(utf16Len == 0)) {
        return *GetEmptyString();
    }

    bool canBeCompressed = sd.is_ascii;
    const uint8_t *mutf8Data = sd.data;
    if (canBeCompressed) {
        // This branch will use constant string, which has a pointer at the string in the pandafile.
        return vm_->GetEcmaStringTable()->GetOrInternStringWithoutJSHandleForJit(vm_, mutf8Data, utf16Len, true, type,
                                                                                 isConstantString, idOffset);
    }
    return vm_->GetEcmaStringTable()->GetOrInternStringWithoutJSHandleForJit(vm_, mutf8Data, utf16Len, type);
}

JSHandle<PropertyBox> ObjectFactory::NewPropertyBox(const JSHandle<JSTaggedValue> &value)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPropertyBoxClass().GetTaggedObject()));
    JSHandle<PropertyBox> box(thread_, header);
    box->SetValue(thread_, value);
    return box;
}

JSHandle<ProtoChangeMarker> ObjectFactory::NewProtoChangeMarker()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetProtoChangeMarkerClass().GetTaggedObject()));
    JSHandle<ProtoChangeMarker> marker(thread_, header);
    marker->ClearBitField();
    return marker;
}

JSHandle<MarkerCell> ObjectFactory::NewMarkerCell()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetMarkerCellClass().GetTaggedObject()));
    JSHandle<MarkerCell> marker(thread_, header);
    marker->ClearBitField();
    return marker;
}

JSHandle<ProtoChangeDetails> ObjectFactory::NewProtoChangeDetails()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetProtoChangeDetailsClass().GetTaggedObject()));
    JSHandle<ProtoChangeDetails> protoInfo(thread_, header);
    protoInfo->SetChangeListener(thread_, JSTaggedValue::Undefined());
    protoInfo->SetRegisterIndex(ProtoChangeDetails::UNREGISTERED);
    return protoInfo;
}

JSHandle<ProfileTypeInfo> ObjectFactory::NewProfileTypeInfo(uint32_t icSlotSize)
{
    NewObjectHook();
    size_t size = ProfileTypeInfo::ComputeSize(icSlotSize);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetProfileTypeInfoClass().GetTaggedObject()), size);
    JSHandle<ProfileTypeInfo> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), icSlotSize);
    if (vm_->IsEnableFastJit()) {
        uint16_t threshold = vm_->GetJSOptions().GetJitHotnessThreshold();
        ASSERT(threshold != ProfileTypeInfo::JIT_DISABLE_FLAG);
        array->SetJitHotnessThreshold(threshold);
        threshold = vm_->GetJSOptions().GetOsrHotnessThreshold();
        array->SetOsrHotnessThreshold(threshold);
        uint16_t jitCallThreshold = vm_->GetJSOptions().GetJitCallThreshold();
        array->SetJitCallThreshold(jitCallThreshold);
    }
    if (vm_->IsEnableBaselineJit()) {
        uint16_t threshold = vm_->GetJSOptions().GetBaselineJitHotnessThreshold();
        array->SetBaselineJitHotnessThreshold(threshold);
    }
    return array;
}

JSHandle<BigInt> ObjectFactory::NewBigInt(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);
    size_t size = BigInt::ComputeSize(length);
    auto header = sHeap_->AllocateNonMovableOrHugeObject(thread_,
        JSHClass::Cast(thread_->GlobalConstants()->GetBigIntClass().GetTaggedObject()), size);
    JSHandle<BigInt> bigint(thread_, header);
    bigint->SetLength(length);
    bigint->SetSign(false);
    bigint->InitializationZero();
    return bigint;
}

// static
void ObjectFactory::NewObjectHook() const
{
    CHECK_NO_HEAP_ALLOC;
#ifndef NDEBUG
    if (vm_->GetJSOptions().EnableForceGC() && vm_->IsInitialized() && thread_->IsAllContextsInitialized() &&
        !heap_->InSensitiveStatus() && heap_->TriggerCollectionOnNewObjectEnabled()) {
        if (vm_->GetJSOptions().ForceFullGC()) {
            vm_->CollectGarbage(TriggerGCType::YOUNG_GC);
            vm_->CollectGarbage(TriggerGCType::OLD_GC);
            vm_->CollectGarbage(TriggerGCType::FULL_GC);
        } else {
            vm_->CollectGarbage(TriggerGCType::YOUNG_GC);
            vm_->CollectGarbage(TriggerGCType::OLD_GC);
        }
    }
#endif
}

JSHandle<TaggedQueue> ObjectFactory::NewTaggedQueue(uint32_t length)
{
    uint32_t queueLength = TaggedQueue::QueueToArrayIndex(length);
    auto queue = JSHandle<TaggedQueue>::Cast(NewTaggedArray(queueLength, JSTaggedValue::Hole()));
    queue->SetStart(thread_, JSTaggedValue(0));  // equal to 0 when add 1.
    queue->SetEnd(thread_, JSTaggedValue(0));
    queue->SetCapacity(thread_, JSTaggedValue(length));

    return queue;
}

JSHandle<TaggedQueue> ObjectFactory::GetEmptyTaggedQueue() const
{
    return JSHandle<TaggedQueue>(thread_->GlobalConstants()->GetHandledEmptyTaggedQueue());
}

JSHandle<JSSharedSetIterator> ObjectFactory::NewJSSetIterator(const JSHandle<JSSharedSet> &set, IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetSharedSetIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSSharedSetIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSSharedSetIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedSet(thread_, set.GetTaggedValue());
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    ASSERT(iter.GetTaggedValue().IsJSSharedSetIterator());
    return iter;
}

JSHandle<JSSetIterator> ObjectFactory::NewJSSetIterator(const JSHandle<JSSet> &set, IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetSetIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSSetIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSSetIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedSet(thread_, set->GetLinkedSet());
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSRegExpIterator> ObjectFactory::NewJSRegExpIterator(const JSHandle<JSTaggedValue> &matcher,
                                                              const JSHandle<EcmaString> &inputStr, bool global,
                                                              bool fullUnicode)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetRegExpIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSRegExpIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSRegExpIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratingRegExp(thread_, matcher.GetTaggedValue());
    iter->SetIteratedString(thread_, inputStr.GetTaggedValue());
    iter->SetGlobal(global);
    iter->SetUnicode(fullUnicode);
    iter->SetDone(false);
    return iter;
}

JSHandle<JSMapIterator> ObjectFactory::NewJSMapIterator(const JSHandle<JSMap> &map, IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetMapIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSMapIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSMapIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedMap(thread_, map->GetLinkedMap());
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSSharedMapIterator> ObjectFactory::NewJSMapIterator(const JSHandle<JSSharedMap> &map, IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetSharedMapIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSSharedMapIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSSharedMapIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedMap(thread_, map.GetTaggedValue());
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    ASSERT(iter.GetTaggedValue().IsJSSharedMapIterator());
    return iter;
}

JSHandle<JSAPIHashMapIterator> ObjectFactory::NewJSAPIHashMapIterator(const JSHandle<JSAPIHashMap> &hashMap,
                                                                      IterationKind kind)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> undefinedHandle = globalConst->GetHandledUndefined();
    JSHandle<JSTaggedValue> proto(thread_, globalConst->GetHashMapIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIHashMapIteratorClass());
    hclassHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPIHashMapIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetCurrentNodeResult(thread_, undefinedHandle);
    iter->SetIteratedHashMap(thread_, hashMap);
    iter->SetNextIndex(0);
    iter->SetTaggedQueue(thread_, JSTaggedValue::Undefined());
    JSHandle<TaggedQueue> queue = NewTaggedQueue(0);
    iter->SetTaggedQueue(thread_, queue);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSAPIHashSetIterator> ObjectFactory::NewJSAPIHashSetIterator(const JSHandle<JSAPIHashSet> &hashSet,
                                                                      IterationKind kind)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> undefinedHandle = globalConst->GetHandledUndefined();
    JSHandle<JSTaggedValue> proto(thread_, globalConst->GetHashSetIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIHashSetIteratorClass());
    hclassHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPIHashSetIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetCurrentNodeResult(thread_, undefinedHandle);
    iter->SetIteratedHashSet(thread_, hashSet);
    iter->SetNextIndex(0);
    iter->SetTableIndex(0);
    iter->SetTaggedQueue(thread_, JSTaggedValue::Undefined());
    JSHandle<TaggedQueue> queue = NewTaggedQueue(0);
    iter->SetTaggedQueue(thread_, queue);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSArrayIterator> ObjectFactory::NewJSArrayIterator(const JSHandle<JSObject> &array, IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetArrayIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSArrayIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSArrayIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedArray(thread_, array);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSSharedArrayIterator> ObjectFactory::NewJSSharedArrayIterator(const JSHandle<JSObject> &array,
                                                                        IterationKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> protoValue = env->GetSharedArrayIteratorPrototype();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSSharedArrayIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSSharedArrayIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedArray(thread_, array);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSPromiseReactionsFunction> ObjectFactory::CreateJSPromiseReactionsFunction(MethodIndex idx)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseReactionFunctionClass());

    JSHandle<JSPromiseReactionsFunction> reactionsFunction =
        JSHandle<JSPromiseReactionsFunction>::Cast(NewJSObject(hclass));
    reactionsFunction->SetPromise(thread_, JSTaggedValue::Hole());
    reactionsFunction->SetAlreadyResolved(thread_, JSTaggedValue::Hole());
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(reactionsFunction);
    JSFunction::InitializeJSFunction(thread_, function);
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(idx));
    reactionsFunction->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(reactionsFunction), method);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(1));
    return reactionsFunction;
}

JSHandle<JSPromiseExecutorFunction> ObjectFactory::CreateJSPromiseExecutorFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseExecutorFunctionClass());
    JSHandle<JSPromiseExecutorFunction> executorFunction =
        JSHandle<JSPromiseExecutorFunction>::Cast(NewJSObject(hclass));
    executorFunction->SetCapability(thread_, JSTaggedValue::Undefined());
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(executorFunction);
    JSFunction::InitializeJSFunction(thread_, function);
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_EXECUTOR));
    executorFunction->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(executorFunction), method);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(FunctionLength::TWO));
    return executorFunction;
}

JSHandle<JSAsyncModuleFulfilledFunction> ObjectFactory::CreateJSAsyncModuleFulfilledFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetAsyncModuleFulfilledFunctionClass());
    JSHandle<JSAsyncModuleFulfilledFunction> fulfilledFunction =
        JSHandle<JSAsyncModuleFulfilledFunction>::Cast(NewJSObject(hclass));
    fulfilledFunction->SetModule(thread_, JSTaggedValue::Undefined());
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(fulfilledFunction);
    JSFunction::InitializeJSFunction(thread_, function);
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_ASYNC_MODULE_FULFILLED_FUNCTION));
    fulfilledFunction->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(fulfilledFunction), method);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(FunctionLength::ONE));
    return fulfilledFunction;
}

JSHandle<JSAsyncModuleRejectedFunction> ObjectFactory::CreateJSAsyncModuleRejectedFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetAsyncModuleRejectedFunctionClass());
    JSHandle<JSAsyncModuleRejectedFunction> rejectedFunction =
        JSHandle<JSAsyncModuleRejectedFunction>::Cast(NewJSObject(hclass));
    rejectedFunction->SetModule(thread_, JSTaggedValue::Undefined());
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(rejectedFunction);
    JSFunction::InitializeJSFunction(thread_, function);
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_ASYNC_MODULE_REJECTED_FUNCTION));
    rejectedFunction->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(rejectedFunction), method);
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(FunctionLength::ONE));
    return rejectedFunction;
}

JSHandle<JSPromiseAllResolveElementFunction> ObjectFactory::NewJSPromiseAllResolveElementFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseAllResolveElementFunctionClass());
    JSHandle<JSPromiseAllResolveElementFunction> function =
        JSHandle<JSPromiseAllResolveElementFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
                            vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_RESOLVE_ELEMENT_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetIndex(thread_, JSTaggedValue::Undefined());
    function->SetValues(thread_, JSTaggedValue::Undefined());
    function->SetCapabilities(thread_, JSTaggedValue::Undefined());
    function->SetRemainingElements(thread_, JSTaggedValue::Undefined());
    function->SetAlreadyCalled(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSPromiseAnyRejectElementFunction> ObjectFactory::NewJSPromiseAnyRejectElementFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseAnyRejectElementFunctionClass());
    JSHandle<JSPromiseAnyRejectElementFunction> function =
        JSHandle<JSPromiseAnyRejectElementFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
                            vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_ANY_REJECT_ELEMENT_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetIndex(0);
    function->SetErrors(thread_, JSTaggedValue::Undefined());
    function->SetCapability(thread_, JSTaggedValue::Undefined());
    function->SetRemainingElements(thread_, JSTaggedValue::Undefined());
    function->SetAlreadyCalled(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSPromiseAllSettledElementFunction> ObjectFactory::NewJSPromiseAllSettledResolveElementFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseAllSettledElementFunctionClass());
    JSHandle<JSPromiseAllSettledElementFunction> function =
        JSHandle<JSPromiseAllSettledElementFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
        vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_ALL_SETTLED_RESOLVE_ELEMENT_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetIndex(0);
    function->SetValues(thread_, JSTaggedValue::Undefined());
    function->SetCapability(thread_, JSTaggedValue::Undefined());
    function->SetRemainingElements(thread_, JSTaggedValue::Undefined());
    function->SetAlreadyCalled(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSPromiseAllSettledElementFunction> ObjectFactory::NewJSPromiseAllSettledRejectElementFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseAllSettledElementFunctionClass());
    JSHandle<JSPromiseAllSettledElementFunction> function =
        JSHandle<JSPromiseAllSettledElementFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
        vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_ALL_SETTLED_REJECT_ELEMENT_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetIndex(0);
    function->SetValues(thread_, JSTaggedValue::Undefined());
    function->SetCapability(thread_, JSTaggedValue::Undefined());
    function->SetRemainingElements(thread_, JSTaggedValue::Undefined());
    function->SetAlreadyCalled(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSPromiseFinallyFunction> ObjectFactory::NewJSPromiseThenFinallyFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseFinallyFunctionClass());
    JSHandle<JSPromiseFinallyFunction> function =
        JSHandle<JSPromiseFinallyFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
                            vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_THEN_FINALLY_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetConstructor(thread_, JSTaggedValue::Undefined());
    function->SetOnFinally(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSPromiseFinallyFunction> ObjectFactory::NewJSPromiseCatchFinallyFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseFinallyFunctionClass());
    JSHandle<JSPromiseFinallyFunction> function =
        JSHandle<JSPromiseFinallyFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
                            vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_CATCH_FINALLY_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetConstructor(thread_, JSTaggedValue::Undefined());
    function->SetOnFinally(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSAsyncGeneratorResNextRetProRstFtn> ObjectFactory::NewJSAsyGenResNextRetProRstFulfilledFtn()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(
        env->GetAsyncGeneratorResNextRetProRstFtnClass());
    JSHandle<JSAsyncGeneratorResNextRetProRstFtn> function =
        JSHandle<JSAsyncGeneratorResNextRetProRstFtn>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
                            vm_->GetMethodByIndex(MethodIndex::BUILTINS_ASYNC_GENERATOR_NEXT_FULFILLED_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetAsyncGeneratorObject(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSAsyncFromSyncIterUnwarpFunction> ObjectFactory::NewJSAsyncFromSyncIterUnwarpFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFromSyncIterUnwarpClass());
    JSHandle<JSAsyncFromSyncIterUnwarpFunction> function =
        JSHandle<JSAsyncFromSyncIterUnwarpFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_ASYNC_FROM_SYNC_ITERATOR_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetDone(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSAsyncGeneratorResNextRetProRstFtn> ObjectFactory::NewJSAsyGenResNextRetProRstRejectedFtn()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(
        env->GetAsyncGeneratorResNextRetProRstFtnClass());
    JSHandle<JSAsyncGeneratorResNextRetProRstFtn> function =
        JSHandle<JSAsyncGeneratorResNextRetProRstFtn>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
                            vm_->GetMethodByIndex(MethodIndex::BUILTINS_ASYNC_GENERATOR_NEXT_REJECTED_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetAsyncGeneratorObject(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(1));
    return function;
}

JSHandle<JSPromiseValueThunkOrThrowerFunction> ObjectFactory::NewJSPromiseValueThunkFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseValueThunkOrThrowerFunctionClass());
    JSHandle<JSPromiseValueThunkOrThrowerFunction> function =
        JSHandle<JSPromiseValueThunkOrThrowerFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_,
                            vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_VALUE_THUNK_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetResult(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(0));
    return function;
}

JSHandle<JSPromiseValueThunkOrThrowerFunction> ObjectFactory::NewJSPromiseThrowerFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseValueThunkOrThrowerFunctionClass());
    JSHandle<JSPromiseValueThunkOrThrowerFunction> function =
        JSHandle<JSPromiseValueThunkOrThrowerFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    JSHandle<Method> method(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_THROWER_FUNCTION));
    function->SetMethod(thread_, method);
    SetNativePointerToFunctionFromMethod(JSHandle<JSFunctionBase>::Cast(function), method);
    function->SetResult(thread_, JSTaggedValue::Undefined());
    JSFunction::SetFunctionLength(thread_, JSHandle<JSFunction>::Cast(function), JSTaggedValue(0));
    return function;
}

EcmaString *ObjectFactory::InternString(const JSHandle<JSTaggedValue> &key)
{
    EcmaString *str = EcmaString::Cast(key->GetTaggedObject());
    if (EcmaStringAccessor(str).IsInternString()) {
        return str;
    }

    EcmaStringTable *stringTable = vm_->GetEcmaStringTable();
    return stringTable->GetOrInternString(vm_, str);
}

JSHandle<TransitionHandler> ObjectFactory::NewTransitionHandler()
{
    NewObjectHook();
    TransitionHandler *handler = TransitionHandler::Cast(heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTransitionHandlerClass().GetTaggedObject())));
    handler->SetHandlerInfo(thread_, JSTaggedValue::Undefined());
    handler->SetTransitionHClass(thread_, JSTaggedValue::Undefined());
    return JSHandle<TransitionHandler>(thread_, handler);
}

JSHandle<PrototypeHandler> ObjectFactory::NewPrototypeHandler()
{
    NewObjectHook();
    PrototypeHandler *header =
        PrototypeHandler::Cast(heap_->AllocateYoungOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetPrototypeHandlerClass().GetTaggedObject())));
    JSHandle<PrototypeHandler> handler(thread_, header);
    handler->SetHandlerInfo(thread_, JSTaggedValue::Undefined());
    handler->SetProtoCell(thread_, JSTaggedValue::Undefined());
    handler->SetHolder(thread_, JSTaggedValue::Undefined());
    handler->SetAccessorJSFunction(thread_, JSTaggedValue::Undefined());
    handler->SetAccessorMethodId(0);
    return handler;
}

JSHandle<TransWithProtoHandler> ObjectFactory::NewTransWithProtoHandler()
{
    NewObjectHook();
    TransWithProtoHandler *header =
        TransWithProtoHandler::Cast(heap_->AllocateYoungOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetTransWithProtoHandlerClass().GetTaggedObject())));
    JSHandle<TransWithProtoHandler> handler(thread_, header);
    handler->SetHandlerInfo(thread_, JSTaggedValue::Undefined());
    handler->SetProtoCell(thread_, JSTaggedValue::Undefined());
    handler->SetTransitionHClass(thread_, JSTaggedValue::Undefined());
    return handler;
}

JSHandle<StoreAOTHandler> ObjectFactory::NewStoreAOTHandler()
{
    NewObjectHook();
    StoreAOTHandler *header =
        StoreAOTHandler::Cast(heap_->AllocateYoungOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetStoreAOTHandlerClass().GetTaggedObject())));
    JSHandle<StoreAOTHandler> handler(thread_, header);
    handler->SetHandlerInfo(thread_, JSTaggedValue::Undefined());
    handler->SetProtoCell(thread_, JSTaggedValue::Undefined());
    handler->SetHolder(thread_, JSTaggedValue::Undefined());
    return handler;
}

JSHandle<PromiseRecord> ObjectFactory::NewPromiseRecord()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPromiseRecordClass().GetTaggedObject()));
    JSHandle<PromiseRecord> obj(thread_, header);
    obj->SetValue(thread_, JSTaggedValue::Undefined());
    return obj;
}

JSHandle<ResolvingFunctionsRecord> ObjectFactory::NewResolvingFunctionsRecord()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetPromiseResolvingFunctionsRecordClass().GetTaggedObject()));
    JSHandle<ResolvingFunctionsRecord> obj(thread_, header);
    obj->SetResolveFunction(thread_, JSTaggedValue::Undefined());
    obj->SetRejectFunction(thread_, JSTaggedValue::Undefined());
    return obj;
}

JSHandle<JSHClass> ObjectFactory::CreateObjectClass(const JSHandle<TaggedArray> &properties, size_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();

    uint32_t fieldOrder = 0;
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(length);
    while (fieldOrder < length) {
        key.Update(properties->Get(fieldOrder * 2));  // 2: Meaning to double
        ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
        PropertyAttributes attributes = PropertyAttributes::Default();

        if (properties->Get(fieldOrder * 2 + 1).IsAccessor()) {  // 2: Meaning to double
            attributes.SetIsAccessor(true);
        }

        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder, key.GetTaggedValue(), attributes);
        fieldOrder++;
    }
    ASSERT(fieldOrder <= PropertyAttributes::MAX_FAST_PROPS_CAPACITY);
    JSHandle<JSHClass> objClass = NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, fieldOrder);
    objClass->SetPrototype(thread_, proto.GetTaggedValue());
    {
        objClass->SetExtensible(true);
        objClass->SetLayout(thread_, layoutInfoHandle);
        objClass->SetNumberOfProps(fieldOrder);
    }
    return objClass;
}

JSHandle<JSHClass> ObjectFactory::SetLayoutInObjHClass(const JSHandle<TaggedArray> &properties, size_t length,
                                                       const JSHandle<JSHClass> &objClass)
{
    JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
    JSHandle<JSHClass> newObjHclass(objClass);

    for (size_t fieldOffset = 0; fieldOffset < length; fieldOffset++) {
        key.Update(properties->Get(fieldOffset * 2)); // 2 : pair of key and value
        ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
        PropertyAttributes attributes = PropertyAttributes::Default();
        auto value = properties->Get(fieldOffset * 2 + 1);
        if (value.IsAccessor()) {  // 2: Meaning to double
            attributes.SetIsAccessor(true);
        }
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOffset);
        attributes.SetRepresentation(Representation::TAGGED);
        auto rep = PropertyAttributes::TranslateToRep(value);
        newObjHclass = JSHClass::SetPropertyOfObjHClass(thread_, newObjHclass, key, attributes, rep);
    }
    return newObjHclass;
}

bool ObjectFactory::CanObjectLiteralHClassCache(size_t length)
{
    return length <= PropertyAttributes::MAX_LITERAL_HCLASS_CACHE_SIZE;
}

JSHandle<JSHClass> ObjectFactory::CreateObjectLiteralRootHClass(size_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();
    JSHandle<JSHClass> hclass = NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, length);
    hclass->SetPrototype(thread_, proto.GetTaggedValue());
    {
        hclass->SetNumberOfProps(0);
        hclass->SetExtensible(true);
    }
    return hclass;
}

JSHandle<JSHClass> ObjectFactory::GetObjectLiteralRootHClass(size_t literalLength, size_t maxPropsNum)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> maybeCache = env->GetObjectLiteralHClassCache();
    if (UNLIKELY(maybeCache->IsHole())) {
        JSHandle<TaggedArray> cacheArr = NewTaggedArray(PropertyAttributes::MAX_LITERAL_HCLASS_CACHE_SIZE + 1);
        env->SetObjectLiteralHClassCache(thread_, cacheArr.GetTaggedValue());
        JSHandle<JSHClass> objHClass = CreateObjectLiteralRootHClass(maxPropsNum);
        cacheArr->Set(thread_, literalLength, objHClass);
        return objHClass;
    }
    JSHandle<TaggedArray> hclassCacheArr = JSHandle<TaggedArray>::Cast(maybeCache);
    JSTaggedValue maybeHClass = hclassCacheArr->Get(literalLength);
    if (UNLIKELY(maybeHClass.IsHole())) {
        JSHandle<JSHClass> objHClass = CreateObjectLiteralRootHClass(maxPropsNum);
        hclassCacheArr->Set(thread_, literalLength, objHClass);
        return objHClass;
    }
    return JSHandle<JSHClass>(thread_, maybeHClass);
}

JSHandle<JSHClass> ObjectFactory::GetObjectLiteralHClass(const JSHandle<TaggedArray> &properties, size_t length)
{
    ASSERT(length <= PropertyAttributes::MAX_FAST_PROPS_CAPACITY);
    // 64 : If object literal gets too many properties, create hclass directly.
    if (!CanObjectLiteralHClassCache(length)) {
        return CreateObjectClass(properties, length);
    }
    JSHandle<JSHClass> rootHClass = GetObjectLiteralRootHClass(length, length);
    return SetLayoutInObjHClass(properties, length, rootHClass);
}

JSHandle<JSObject> ObjectFactory::NewOldSpaceObjLiteralByHClass(const JSHandle<JSHClass> &hclass)
{
    JSHandle<JSObject> obj = NewOldSpaceJSObject(hclass);
    InitializeJSObject(obj, hclass);
    return obj;
}

JSHandle<JSObject> ObjectFactory::NewEmptyJSObject(uint32_t inlinedProps)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> builtinObj(env->GetObjectFunction());
    return NewJSObjectByConstructor(builtinObj, inlinedProps);
}

JSHandle<JSObject> ObjectFactory::CreateNullJSObject()
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> nullValue = globalConst->GetHandledNull();
    return OrdinaryNewJSObjectCreate(nullValue);
}

uintptr_t ObjectFactory::NewSpaceBySnapshotAllocator(size_t size)
{
    NewObjectHook();
    return heap_->AllocateSnapshotSpace(size);
}

TaggedObject *ObjectFactory::NewMachineCodeObject(size_t length,
    MachineCodeDesc &desc)
{
    NewObjectHook();
    TaggedObject *obj = heap_->AllocateMachineCodeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetMachineCodeClass().GetTaggedObject()),
        length + MachineCode::SIZE, &desc);
    return (obj);
}

JSHandle<MachineCode> ObjectFactory::NewMachineCodeObject(size_t length,
    const MachineCodeDesc &desc, JSHandle<Method> &method, RelocMap &relocInfo)
{
    NewObjectHook();
    TaggedObject *obj = heap_->AllocateMachineCodeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetMachineCodeClass().GetTaggedObject()),
        length + MachineCode::SIZE);
    return SetMachineCodeObjectData(obj, length, desc, method, relocInfo);
}

JSHandle<MachineCode> ObjectFactory::SetMachineCodeObjectData(TaggedObject *obj, size_t length,
    const MachineCodeDesc &desc, JSHandle<Method> &method, RelocMap &relocInfo)
{
    MachineCode *code = MachineCode::Cast(obj);
    if (code == nullptr) {
        LOG_FULL(FATAL) << "machine code cast failed";
        UNREACHABLE();
    }
    if (code->SetData(desc, method, length, relocInfo)) {
        JSHandle<MachineCode> codeObj(thread_, code);
        return codeObj;
    } else {
        JSHandle<MachineCode> codeObj;
        return codeObj;
    }
}

JSHandle<ClassInfoExtractor> ObjectFactory::NewClassInfoExtractor(JSHandle<JSTaggedValue> method)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetClassInfoExtractorHClass().GetTaggedObject()));
    JSHandle<ClassInfoExtractor> obj(thread_, header);
    obj->ClearBitField();
    obj->SetConstructorMethod(thread_, method.GetTaggedValue());
    JSHandle<TaggedArray> emptyArray = EmptyArray();
    obj->SetNonStaticKeys(thread_, emptyArray, SKIP_BARRIER);
    obj->SetNonStaticProperties(thread_, emptyArray, SKIP_BARRIER);
    obj->SetNonStaticElements(thread_, emptyArray, SKIP_BARRIER);
    obj->SetStaticKeys(thread_, emptyArray, SKIP_BARRIER);
    obj->SetStaticProperties(thread_, emptyArray, SKIP_BARRIER);
    obj->SetStaticElements(thread_, emptyArray, SKIP_BARRIER);
    return obj;
}

// ----------------------------------- new string ----------------------------------------
JSHandle<EcmaString> ObjectFactory::NewFromASCII(std::string_view data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.data());
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf8Data, data.length()));
    return GetStringFromStringTable(utf8Data, data.length(), true);
}

// At this situation, Create string directly without using a StringTable.
JSHandle<EcmaString> ObjectFactory::NewFromASCIISkippingStringTable(std::string_view data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.data());
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf8Data, data.length()));
    EcmaString *str = EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, data.length(), true);
    return JSHandle<EcmaString>(thread_, str);
}

JSHandle<EcmaString> ObjectFactory::NewFromASCIIReadOnly(std::string_view data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.data());
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf8Data, data.length()));
    return GetStringFromStringTableReadOnly(utf8Data, data.length());
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8WithoutStringTable(std::string_view data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.data());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, data.length());
    uint32_t utf8Len = data.length();
    NewObjectHook();
    if (utf8Len == 0) {
        return GetEmptyString();
    }
    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress, MemSpaceType::SHARED_OLD_SPACE);
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    str->SetMixHashcode(hashCode);
    return JSHandle<EcmaString>(thread_, str);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8(std::string_view data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.data());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, data.length());
    return GetStringFromStringTable(utf8Data, data.length(), canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8(std::string_view data, bool canBeCompress)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.data());
    ASSERT(canBeCompress == EcmaStringAccessor::CanBeCompressed(utf8Data, data.length()));
    return GetStringFromStringTable(utf8Data, data.length(), canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8ReadOnly(std::string_view data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.data());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, data.length());
    return GetStringFromStringTableReadOnly(utf8Data, data.length(), canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16WithoutStringTable(std::u16string_view data)
{
    uint32_t utf16Len = data.length();
    auto utf16Data = reinterpret_cast<const uint16_t *>(data.data());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf16Data, utf16Len);
    NewObjectHook();
    if (utf16Len == 0) {
        return GetEmptyString();
    }
    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf16(vm_, utf16Data, utf16Len, canBeCompress, MemSpaceType::SHARED_OLD_SPACE);
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);
    str->SetMixHashcode(hashCode);
    return JSHandle<EcmaString>(thread_, str);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16(std::u16string_view data)
{
    uint32_t length = data.length();
    auto utf16Data = reinterpret_cast<const uint16_t *>(data.data());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf16Data, length);
    return GetStringFromStringTable(utf16Data, length, canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromStdString(const std::string &data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.c_str());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, data.length());
    return GetStringFromStringTable(utf8Data, data.size(), canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8WithoutStringTable(const uint8_t *utf8Data, uint32_t utf8Len)
{
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, utf8Len);
    NewObjectHook();
    if (utf8Len == 0) {
        return GetEmptyString();
    }
    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress, MemSpaceType::SHARED_OLD_SPACE);
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    str->SetMixHashcode(hashCode);
    return JSHandle<EcmaString>(thread_, str);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8(const uint8_t *utf8Data, uint32_t utf8Len)
{
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, utf8Len);
    return GetStringFromStringTable(utf8Data, utf8Len, canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16WithoutStringTable(const uint16_t *utf16Data, uint32_t utf16Len)
{
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf16Data, utf16Len);
    NewObjectHook();
    if (utf16Len == 0) {
        return GetEmptyString();
    }
    EcmaString *str =
        EcmaStringAccessor::CreateFromUtf16(vm_, utf16Data, utf16Len, canBeCompress, MemSpaceType::SHARED_OLD_SPACE);
    uint32_t hashCode = EcmaStringAccessor::ComputeHashcodeUtf16(const_cast<uint16_t *>(utf16Data), utf16Len);
    str->SetMixHashcode(hashCode);
    return JSHandle<EcmaString>(thread_, str);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16(const uint16_t *utf16Data, uint32_t utf16Len)
{
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf16Data, utf16Len);
    return GetStringFromStringTable(utf16Data, utf16Len, canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16Compress(const uint16_t *utf16Data, uint32_t utf16Len)
{
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf16Data, utf16Len));
    return GetStringFromStringTable(utf16Data, utf16Len, true);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16NotCompress(const uint16_t *utf16Data, uint32_t utf16Len)
{
    ASSERT(!EcmaStringAccessor::CanBeCompressed(utf16Data, utf16Len));
    return GetStringFromStringTable(utf16Data, utf16Len, false);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8Literal(const uint8_t *utf8Data, uint32_t utf8Len)
{
    NewObjectHook();
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, utf8Len);
    return JSHandle<EcmaString>(thread_, EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8LiteralCompress(const uint8_t *utf8Data, uint32_t utf8Len)
{
    NewObjectHook();
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf8Data, utf8Len));
    return JSHandle<EcmaString>(thread_, EcmaStringAccessor::CreateFromUtf8(vm_, utf8Data, utf8Len, true));
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8LiteralCompressSubString(const JSHandle<EcmaString> &string,
                                                                        uint32_t offset, uint32_t utf8Len)
{
    NewObjectHook();
    ASSERT(EcmaStringAccessor::CanBeCompressed(EcmaStringAccessor(string).GetDataUtf8() + offset, utf8Len));
    return JSHandle<EcmaString>(thread_, EcmaStringAccessor::CreateFromUtf8CompressedSubString(vm_, string,
        offset, utf8Len));
}

JSHandle<EcmaString> ObjectFactory::NewCompressedUtf8(const uint8_t *utf8Data, uint32_t utf8Len)
{
    NewObjectHook();
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf8Data, utf8Len));
    return GetStringFromStringTable(utf8Data, utf8Len, true);
}

JSHandle<EcmaString> ObjectFactory::NewCompressedUtf8SubString(const JSHandle<EcmaString> &string,
                                                               uint32_t offset, uint32_t utf8Len)
{
    NewObjectHook();
    ASSERT(EcmaStringAccessor::CanBeCompressed(EcmaStringAccessor(string).GetDataUtf8() + offset, utf8Len));
    return GetCompressedSubStringFromStringTable(string, offset, utf8Len);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16Literal(const uint16_t *utf16Data, uint32_t utf16Len)
{
    NewObjectHook();
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf16Data, utf16Len);
    return JSHandle<EcmaString>(thread_, EcmaStringAccessor::CreateFromUtf16(vm_, utf16Data, utf16Len, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16LiteralCompress(const uint16_t *utf16Data, uint32_t utf16Len)
{
    NewObjectHook();
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf16Data, utf16Len));
    return JSHandle<EcmaString>(thread_, EcmaStringAccessor::CreateFromUtf16(vm_, utf16Data, utf16Len, true));
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16LiteralNotCompress(const uint16_t *utf16Data, uint32_t utf16Len)
{
    NewObjectHook();
    ASSERT(!EcmaStringAccessor::CanBeCompressed(utf16Data, utf16Len));
    return JSHandle<EcmaString>(thread_, EcmaStringAccessor::CreateFromUtf16(vm_, utf16Data, utf16Len, false));
}

JSHandle<EcmaString> ObjectFactory::ConcatFromString(const JSHandle<EcmaString> &firstString,
                                                     const JSHandle<EcmaString> &secondString)
{
    if (EcmaStringAccessor(firstString).GetLength() == 0) {
        return secondString;
    }
    if (EcmaStringAccessor(secondString).GetLength() == 0) {
        return firstString;
    }
    return GetStringFromStringTable(firstString, secondString);
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(const JSHandle<EcmaString> &firstString,
                                                             const JSHandle<EcmaString> &secondString)
{
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(vm_, firstString, secondString));
}

JSHandle<JSAPIArrayList> ObjectFactory::NewJSAPIArrayList(uint32_t capacity)
{
    NewObjectHook();
    JSHandle<JSFunction> builtinObj(thread_, thread_->GlobalConstants()->GetArrayListFunction());
    JSHandle<JSAPIArrayList> obj = JSHandle<JSAPIArrayList>(NewJSObjectByConstructor(builtinObj));
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> elements = factory->NewTaggedArray(capacity);
    obj->SetElements(thread_, elements);

    return obj;
}

JSHandle<JSAPIArrayListIterator> ObjectFactory::NewJSAPIArrayListIterator(const JSHandle<JSAPIArrayList> &arrayList)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> protoValue(thread_, globalConst->GetArrayListIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIArrayListIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIArrayListIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedArrayList(thread_, arrayList);
    iter->SetNextIndex(0);
    return iter;
}

JSHandle<JSAPILightWeightMapIterator> ObjectFactory::NewJSAPILightWeightMapIterator(
    const JSHandle<JSAPILightWeightMap> &obj, IterationKind kind)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> protoValue(thread_, globalConst->GetLightWeightMapIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPILightWeightMapIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPILightWeightMapIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedLightWeightMap(thread_, obj);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSAPILightWeightSetIterator> ObjectFactory::NewJSAPILightWeightSetIterator(
    const JSHandle<JSAPILightWeightSet> &obj, IterationKind kind)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> protoValue(thread_, globalConst->GetLightWeightSetIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPILightWeightSetIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPILightWeightSetIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedLightWeightSet(thread_, obj);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSAPIPlainArray> ObjectFactory::NewJSAPIPlainArray(uint32_t capacity)
{
    NewObjectHook();
    JSHandle<JSFunction> builtinObj(thread_, thread_->GlobalConstants()->GetPlainArrayFunction());
    JSHandle<JSAPIPlainArray> obj = JSHandle<JSAPIPlainArray>(NewJSObjectByConstructor(builtinObj));
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(capacity);
    JSHandle<TaggedArray> valueArray = factory->NewTaggedArray(capacity);
    obj->SetKeys(thread_, keyArray);
    obj->SetValues(thread_, valueArray);

    return obj;
}

JSHandle<JSAPIPlainArrayIterator> ObjectFactory::NewJSAPIPlainArrayIterator(const JSHandle<JSAPIPlainArray> &plainarray,
                                                                            IterationKind kind)
{
    NewObjectHook();
    JSHandle<JSTaggedValue> protoValue(thread_, thread_->GlobalConstants()->GetPlainArrayIteratorPrototype());
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIPlainArrayIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIPlainArrayIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedPlainArray(thread_, plainarray);
    iter->SetNextIndex(0);
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSAPIStackIterator> ObjectFactory::NewJSAPIStackIterator(const JSHandle<JSAPIStack> &stack)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> protoValue(thread_, globalConst->GetStackIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIStackIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIStackIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedStack(thread_, stack);
    iter->SetNextIndex(0);
    return iter;
}

JSHandle<TaggedArray> ObjectFactory::CopyDeque(const JSHandle<TaggedArray> &old, uint32_t newLength,
                                               [[maybe_unused]] uint32_t oldLength, uint32_t first, uint32_t last)
{
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->InitializeWithSpecialValue(JSTaggedValue::Hole(), newLength, old->GetExtraLength());

    uint32_t curIndex = first;
    // newIndex use in new TaggedArray, 0 : New TaggedArray index
    uint32_t newIndex = 0;
    uint32_t oldCapacity = old->GetLength();
    while (curIndex != last) {
        JSTaggedValue value = old->Get(curIndex);
        newArray->Set(thread_, newIndex, value);
        ASSERT(oldCapacity != 0);
        curIndex = (curIndex + 1) % oldCapacity;
        newIndex = newIndex + 1;
    }
    return newArray;
}

JSHandle<JSAPIDequeIterator> ObjectFactory::NewJSAPIDequeIterator(const JSHandle<JSAPIDeque> &deque)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> protoValue(thread_, globalConst->GetDequeIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIDequeIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIDequeIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedDeque(thread_, deque);
    iter->SetNextIndex(deque->GetFirst());
    return iter;
}

JSHandle<TaggedArray> ObjectFactory::CopyQueue(const JSHandle<TaggedArray> &old, uint32_t newLength,
                                               uint32_t front, uint32_t tail)
{
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->InitializeWithSpecialValue(JSTaggedValue::Hole(), newLength, old->GetExtraLength());

    uint32_t curIndex = front;
    // newIndex use in new TaggedArray, 0 : New TaggedArray index
    uint32_t newIndex = 0;
    uint32_t oldCapacity = old->GetLength();
    while (curIndex != tail) {
        JSTaggedValue value = old->Get(curIndex);
        newArray->Set(thread_, newIndex, value);
        ASSERT(oldCapacity != 0);
        curIndex = (curIndex + 1) % oldCapacity;
        newIndex = newIndex + 1;
    }
    return newArray;
}

JSHandle<JSAPIQueueIterator> ObjectFactory::NewJSAPIQueueIterator(const JSHandle<JSAPIQueue> &queue)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> protoValue(thread_, globalConst->GetQueueIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIQueueIteratorClass());
    hclassHandle->SetPrototype(thread_, protoValue);
    JSHandle<JSAPIQueueIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedQueue(thread_, queue); // IteratedQueue
    iter->SetNextIndex(0);
    return iter;
}

JSHandle<JSAPITreeMapIterator> ObjectFactory::NewJSAPITreeMapIterator(const JSHandle<JSAPITreeMap> &map,
                                                                      IterationKind kind)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> proto(thread_, globalConst->GetTreeMapIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPITreeMapIteratorClass());
    hclassHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPITreeMapIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedMap(thread_, map);
    iter->SetNextIndex(0);
    iter->SetEntries(thread_, JSTaggedValue::Hole());
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSAPITreeSetIterator> ObjectFactory::NewJSAPITreeSetIterator(const JSHandle<JSAPITreeSet> &set,
                                                                      IterationKind kind)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> proto(thread_, globalConst->GetTreeSetIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPITreeSetIteratorClass());
    hclassHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPITreeSetIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedSet(thread_, set);
    iter->SetNextIndex(0);
    iter->SetEntries(thread_, JSTaggedValue::Hole());
    iter->SetIterationKind(kind);
    return iter;
}

JSHandle<JSAPIVector> ObjectFactory::NewJSAPIVector(uint32_t capacity)
{
    NewObjectHook();
    JSHandle<JSFunction> builtinObj(thread_, thread_->GlobalConstants()->GetVectorFunction());
    JSHandle<JSAPIVector> obj = JSHandle<JSAPIVector>(NewJSObjectByConstructor(builtinObj));
    JSHandle<TaggedArray> newVector = NewTaggedArray(capacity);
    obj->SetElements(thread_, newVector);

    return obj;
}

JSHandle<JSAPIVectorIterator> ObjectFactory::NewJSAPIVectorIterator(const JSHandle<JSAPIVector> &vector)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> proto(thread_, globalConst->GetVectorIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIVectorIteratorClass());
    hclassHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPIVectorIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedVector(thread_, vector);
    iter->SetNextIndex(0);
    return iter;
}

JSHandle<JSAPIBitVector> ObjectFactory::NewJSAPIBitVector(uint32_t capacity)
{
    NewObjectHook();
    JSHandle<JSFunction> builtinObj(thread_->GetEcmaVM()->GetGlobalEnv()->GetBitVectorFunction());
    JSHandle<JSAPIBitVector> obj = JSHandle<JSAPIBitVector>(NewJSObjectByConstructor(builtinObj));
    uint32_t taggedArrayCapacity = (capacity >> JSAPIBitVector::TAGGED_VALUE_BIT_SIZE) + 1;
    auto *newBitSetVector = new std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>>();
    newBitSetVector->resize(taggedArrayCapacity, 0);
    auto deleter = []([[maybe_unused]] void *env, void *pointer, [[maybe_unused]] void *data) {
        if (pointer == nullptr) {
            return;
        }
        delete reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(pointer);
    };
    JSHandle<JSNativePointer> pointer = NewSJSNativePointer(newBitSetVector, deleter, newBitSetVector);
    obj->SetNativePointer(thread_, pointer);

    return obj;
}

JSHandle<JSAPIBitVectorIterator> ObjectFactory::NewJSAPIBitVectorIterator(const JSHandle<JSAPIBitVector> &bitVector)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto(env->GetBitVectorIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIBitVectorIteratorClass());
    hclassHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPIBitVectorIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedBitVector(thread_, bitVector);
    iter->SetNextIndex(0);
    return iter;
}

JSHandle<JSAPILinkedListIterator> ObjectFactory::NewJSAPILinkedListIterator(const JSHandle<JSAPILinkedList> &linkedList)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> proto(thread_, globalConst->GetLinkedListIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPILinkedListIteratorClass());
    hclassHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPILinkedListIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedLinkedList(thread_, linkedList->GetDoubleList());
    iter->SetNextIndex(0);
    const uint32_t linkedListElementStartIndex = 4;
    iter->SetDataIndex(linkedListElementStartIndex);
    return iter;
}

JSHandle<JSAPIListIterator> ObjectFactory::NewJSAPIListIterator(const JSHandle<JSAPIList> &List)
{
    NewObjectHook();
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> proto(thread_, globalConst->GetListIteratorPrototype());
    JSHandle<JSHClass> hclassHandle(globalConst->GetHandledJSAPIListIteratorClass());
    hclassHandle->SetPrototype(thread_, proto);
    JSHandle<JSAPIListIterator> iter(NewJSObject(hclassHandle));
    iter->GetJSHClass()->SetExtensible(true);
    iter->SetIteratedList(thread_, List->GetSingleList());
    iter->SetNextIndex(0);
    const uint32_t linkedListElementStartIndex = 4;
    iter->SetDataIndex(linkedListElementStartIndex);
    return iter;
}

JSHandle<JSAPIList> ObjectFactory::NewJSAPIList()
{
    NewObjectHook();
    JSHandle<JSFunction> function(thread_, thread_->GlobalConstants()->GetListFunction());
    return JSHandle<JSAPIList>::Cast(NewJSObjectByConstructor(function));
}

JSHandle<JSAPILinkedList> ObjectFactory::NewJSAPILinkedList()
{
    NewObjectHook();
    JSHandle<JSFunction> function(thread_, thread_->GlobalConstants()->GetLinkedListFunction());
    return JSHandle<JSAPILinkedList>::Cast(NewJSObjectByConstructor(function));
}

JSHandle<ImportEntry> ObjectFactory::NewImportEntry()
{
    JSHandle<JSTaggedValue> defautValue = thread_->GlobalConstants()->GetHandledUndefined();
    return NewImportEntry(0, defautValue, defautValue, SharedTypes::UNSENDABLE_MODULE); // 0: random number
}

JSHandle<ImportEntry> ObjectFactory::NewImportEntry(const uint32_t moduleRequestIdx,
                                                    const JSHandle<JSTaggedValue> &importName,
                                                    const JSHandle<JSTaggedValue> &localName,
                                                    SharedTypes sharedTypes)
{
    if (sharedTypes == SharedTypes::SHARED_MODULE) {
        return NewSImportEntry(moduleRequestIdx, importName, localName);
    }
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetImportEntryClass().GetTaggedObject()));
    JSHandle<ImportEntry> obj(thread_, header);
    obj->SetImportName(thread_, importName);
    obj->SetLocalName(thread_, localName);
    obj->SetModuleRequestIndex(moduleRequestIdx);
    return obj;
}

JSHandle<LocalExportEntry> ObjectFactory::NewLocalExportEntry()
{
    JSHandle<JSTaggedValue> defautValue = thread_->GlobalConstants()->GetHandledUndefined();
    return NewLocalExportEntry(defautValue, defautValue, LocalExportEntry::LOCAL_DEFAULT_INDEX,
                               SharedTypes::UNSENDABLE_MODULE);
}

JSHandle<LocalExportEntry> ObjectFactory::NewLocalExportEntry(const JSHandle<JSTaggedValue> &exportName,
    const JSHandle<JSTaggedValue> &localName, const uint32_t index, SharedTypes sharedTypes)
{
    if (sharedTypes == SharedTypes::SHARED_MODULE) {
        return NewSLocalExportEntry(exportName, localName, index);
    }
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetLocalExportEntryClass().GetTaggedObject()));
    JSHandle<LocalExportEntry> obj(thread_, header);
    obj->SetExportName(thread_, exportName);
    obj->SetLocalName(thread_, localName);
    obj->SetLocalIndex(index);
    return obj;
}

JSHandle<IndirectExportEntry> ObjectFactory::NewIndirectExportEntry()
{
    JSHandle<JSTaggedValue> defautValue = thread_->GlobalConstants()->GetHandledUndefined();
    return NewIndirectExportEntry(defautValue, 0, defautValue, SharedTypes::UNSENDABLE_MODULE);
}

JSHandle<IndirectExportEntry> ObjectFactory::NewIndirectExportEntry(const JSHandle<JSTaggedValue> &exportName,
                                                                    const uint32_t moduleRequestIdx,
                                                                    const JSHandle<JSTaggedValue> &importName,
                                                                    SharedTypes sharedTypes)
{
    if (sharedTypes == SharedTypes::SHARED_MODULE) {
        return NewSIndirectExportEntry(exportName, moduleRequestIdx, importName);
    }
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetIndirectExportEntryClass().GetTaggedObject()));
    JSHandle<IndirectExportEntry> obj(thread_, header);
    obj->SetExportName(thread_, exportName);
    obj->SetModuleRequestIndex(moduleRequestIdx);
    obj->SetImportName(thread_, importName);
    return obj;
}

JSHandle<StarExportEntry> ObjectFactory::NewStarExportEntry()
{
    return NewStarExportEntry(0, SharedTypes::UNSENDABLE_MODULE);
}

JSHandle<StarExportEntry> ObjectFactory::NewStarExportEntry(const uint32_t moduleRequestIdx,
                                                            SharedTypes sharedTypes)
{
    if (sharedTypes == SharedTypes::SHARED_MODULE) {
        return NewSStarExportEntry(moduleRequestIdx);
    }
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetStarExportEntryClass().GetTaggedObject()));
    JSHandle<StarExportEntry> obj(thread_, header);
    obj->SetModuleRequestIndex(moduleRequestIdx);
    return obj;
}

JSHandle<SourceTextModule> ObjectFactory::NewSourceTextModule()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetSourceTextModuleClass().GetTaggedObject()));
    JSHandle<SourceTextModule> obj(thread_, header);
    JSTaggedValue undefinedValue = thread_->GlobalConstants()->GetUndefined();
    obj->SetEnvironment(thread_, undefinedValue);
    obj->SetNamespace(thread_, undefinedValue);
    obj->SetModuleRequests(thread_, undefinedValue);
    obj->SetRequestedModules(thread_, undefinedValue);
    obj->SetImportEntries(thread_, undefinedValue);
    obj->SetLocalExportEntries(thread_, undefinedValue);
    obj->SetIndirectExportEntries(thread_, undefinedValue);
    obj->SetStarExportEntries(thread_, undefinedValue);
    obj->SetNameDictionary(thread_, undefinedValue);
    // [[CycleRoot]]: For a module not in a cycle, this would be the module itself.
    obj->SetCycleRoot(thread_, obj);
    obj->SetTopLevelCapability(thread_, undefinedValue);
    obj->SetAsyncParentModules(thread_, undefinedValue);
    obj->SetHasTLA(false);
    obj->SetAsyncEvaluatingOrdinal(SourceTextModule::NOT_ASYNC_EVALUATED);
    obj->SetPendingAsyncDependencies(SourceTextModule::UNDEFINED_INDEX);
    obj->SetDFSIndex(SourceTextModule::UNDEFINED_INDEX);
    obj->SetDFSAncestorIndex(SourceTextModule::UNDEFINED_INDEX);
    obj->SetException(thread_, JSTaggedValue::Hole());
    obj->SetStatus(ModuleStatus::UNINSTANTIATED);
    obj->SetTypes(ModuleTypes::UNKNOWN);
    obj->SetIsNewBcVersion(false);
    obj->SetRegisterCounts(UINT16_MAX);
    obj->SetLazyImportStatus(ToUintPtr(nullptr));
    obj->SetEcmaModuleFilename(ToUintPtr(nullptr));
    obj->SetEcmaModuleRecordName(ToUintPtr(nullptr));
    obj->SetSharedType(SharedTypes::UNSENDABLE_MODULE);
    obj->SetSendableEnv(thread_, undefinedValue);
    return obj;
}

JSHandle<ResolvedBinding> ObjectFactory::NewResolvedBindingRecord()
{
    JSHandle<JSTaggedValue> undefinedValue = thread_->GlobalConstants()->GetHandledUndefined();
    JSHandle<SourceTextModule> ecmaModule(undefinedValue);
    JSHandle<JSTaggedValue> bindingName(undefinedValue);
    return NewResolvedBindingRecord(ecmaModule, bindingName);
}

JSHandle<ResolvedBinding> ObjectFactory::NewResolvedBindingRecord(const JSHandle<SourceTextModule> &module,
                                                                  const JSHandle<JSTaggedValue> &bindingName)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetResolvedBindingClass().GetTaggedObject()));
    JSHandle<ResolvedBinding> obj(thread_, header);
    obj->SetModule(thread_, module);
    obj->SetBindingName(thread_, bindingName);
    return obj;
}

JSHandle<ResolvedIndexBinding> ObjectFactory::NewResolvedIndexBindingRecord()
{
    JSHandle<JSTaggedValue> undefinedValue = thread_->GlobalConstants()->GetHandledUndefined();
    JSHandle<SourceTextModule> ecmaModule(undefinedValue);
    int32_t index = 0;
    return NewResolvedIndexBindingRecord(ecmaModule, index);
}

JSHandle<ResolvedIndexBinding> ObjectFactory::NewResolvedIndexBindingRecord(const JSHandle<SourceTextModule> &module,
                                                                            int32_t index)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetResolvedIndexBindingClass().GetTaggedObject()));
    JSHandle<ResolvedIndexBinding> obj(thread_, header);
    obj->SetModule(thread_, module);
    obj->SetIndex(index);
    return obj;
}

JSHandle<CellRecord> ObjectFactory::NewCellRecord()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetCellRecordClass().GetTaggedObject()));
    JSHandle<CellRecord> obj(thread_, header);
    obj->SetWeakRefTarget(thread_, JSTaggedValue::Undefined());
    obj->SetHeldValue(thread_, JSTaggedValue::Undefined());
    return obj;
}

JSHandle<JSHClass> ObjectFactory::CreateIteratorResultInstanceClass(const JSHandle<GlobalEnv> &env)
{
    auto globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();
    JSHandle<JSHClass> iterResultClass = NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, proto);

    uint32_t fieldOrder = 0;
    JSHandle<LayoutInfo> layoutInfoHandle = CreateLayoutInfo(2); // 2 means two field
    {
        ASSERT(JSIterator::VALUE_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::Default();
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder++, globalConst->GetValueString(), attributes);
    }
    {
        ASSERT(JSIterator::DONE_INLINE_PROPERTY_INDEX == fieldOrder);
        PropertyAttributes attributes = PropertyAttributes::Default();
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOrder);
        layoutInfoHandle->AddKey(thread_, fieldOrder++, globalConst->GetDoneString(), attributes);
    }

    {
        iterResultClass->SetLayout(thread_, layoutInfoHandle);
        iterResultClass->SetNumberOfProps(fieldOrder);
    }
    return iterResultClass;
}

TaggedObject *ObjectFactory::NewOldSpaceObject(const JSHandle<JSHClass> &hclass)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateOldOrHugeObject(*hclass);
    uint32_t inobjPropCount = hclass->GetInlinedProperties();
    if (inobjPropCount > 0) {
        InitializeExtraProperties(hclass, header, inobjPropCount);
    }
    return header;
}

JSHandle<JSObject> ObjectFactory::NewOldSpaceJSObject(const JSHandle<JSHClass> &jshclass)
{
    JSHandle<JSObject> obj(thread_, JSObject::Cast(NewOldSpaceObject(jshclass)));
    JSHandle<TaggedArray> emptyArray = EmptyArray();
    obj->InitializeHash();
    obj->SetElements(thread_, emptyArray);
    obj->SetProperties(thread_, emptyArray);
    return obj;
}

JSHandle<TaggedArray> ObjectFactory::NewOldSpaceTaggedArray(uint32_t length, JSTaggedValue initVal)
{
    return NewTaggedArray(length, initVal, MemSpaceType::OLD_SPACE);
}

JSHandle<JSArray> ObjectFactory::NewJSStableArrayWithElements(const JSHandle<TaggedArray> &elements)
{
    JSHandle<JSHClass> cls(thread_,
                           JSHandle<JSFunction>::Cast(vm_->GetGlobalEnv()->GetArrayFunction())->GetProtoOrHClass());
    JSHandle<JSArray> array = JSHandle<JSArray>::Cast(NewJSObject(cls));
    array->SetElements(thread_, elements);
    array->SetLength(elements->GetLength());
    array->SetTrackInfo(thread_, JSTaggedValue::Undefined());
    auto accessor = thread_->GlobalConstants()->GetArrayLengthAccessor();
    array->SetPropertyInlinedProps(thread_, JSArray::LENGTH_INLINE_PROPERTY_INDEX, accessor);
    return array;
}

JSHandle<JSFunction> ObjectFactory::NewJSAsyncGeneratorFunction(const JSHandle<Method> &method)
{
    NewObjectHook();
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();

    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetAsyncGeneratorFunctionClass());
    JSHandle<JSFunction> asyncGeneratorFunc = JSHandle<JSFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, asyncGeneratorFunc, FunctionKind::ASYNC_GENERATOR_FUNCTION);
    asyncGeneratorFunc->SetMethod(thread_, method);
    return asyncGeneratorFunc;
}

JSHandle<AsyncGeneratorRequest> ObjectFactory::NewAsyncGeneratorRequest()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetAsyncGeneratorRequestRecordClass().GetTaggedObject()));
    JSHandle<AsyncGeneratorRequest> obj(thread_, header);
    obj->SetCompletion(thread_, JSTaggedValue::Undefined());
    obj->SetCapability(thread_, JSTaggedValue::Undefined());
    return obj;
}

JSHandle<AsyncIteratorRecord> ObjectFactory::NewAsyncIteratorRecord(const JSHandle<JSTaggedValue> &itor,
                                                                    const JSHandle<JSTaggedValue> &next, bool done)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetAsyncIteratorRecordClass().GetTaggedObject()));
    JSHandle<AsyncIteratorRecord> obj(thread_, header);
    obj->SetIterator(thread_, itor.GetTaggedValue());
    obj->SetNextMethod(thread_, next.GetTaggedValue());
    obj->SetDone(done);
    return obj;
}

JSHandle<AOTLiteralInfo> ObjectFactory::NewAOTLiteralInfo(uint32_t length, JSTaggedValue initVal)
{
    NewObjectHook();
    size_t size = AOTLiteralInfo::ComputeSize(length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetAOTLiteralInfoClass().GetTaggedObject()), size);

    JSHandle<AOTLiteralInfo> aotLiteralInfo(thread_, header);
    aotLiteralInfo->InitializeWithSpecialValue(initVal, length);
    return aotLiteralInfo;
}

JSHandle<ExtraProfileTypeInfo> ObjectFactory::NewExtraProfileTypeInfo()
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetExtraProfileTypeInfoClass().GetTaggedObject()));
    JSHandle<ExtraProfileTypeInfo> extraProfileTypeInfo(thread_, header);
    extraProfileTypeInfo->SetReceiverObject(thread_, JSTaggedValue::Undefined());
    extraProfileTypeInfo->SetHolderObject(thread_, JSTaggedValue::Undefined());
    return extraProfileTypeInfo;
}

JSHandle<ProfileTypeInfoCell> ObjectFactory::NewProfileTypeInfoCell(const JSHandle<JSTaggedValue> &value)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetProfileTypeInfoCell0Class().GetTaggedObject()));
    JSHandle<ProfileTypeInfoCell> profileTypeInfoCell(thread_, header);
    profileTypeInfoCell->SetValue(thread_, value.GetTaggedValue());
    profileTypeInfoCell->SetMachineCode(thread_, JSTaggedValue::Hole());
    profileTypeInfoCell->SetBaselineCode(thread_, JSTaggedValue::Hole());
    profileTypeInfoCell->SetHandle(thread_, JSTaggedValue::Undefined());
    return profileTypeInfoCell;
}

JSHandle<FunctionTemplate> ObjectFactory::NewFunctionTemplate(
    const JSHandle<Method> &method, const JSHandle<JSTaggedValue> &module, int32_t length)
{
    NewObjectHook();
    auto globalConstants = thread_->GlobalConstants();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(globalConstants->GetFunctionTemplateClass().GetTaggedObject()));
    JSHandle<FunctionTemplate> funcTemp(thread_, header);
    funcTemp->SetMethod(thread_, method);
    funcTemp->SetModule(thread_, module);
    funcTemp->SetRawProfileTypeInfo(thread_, globalConstants->GetEmptyProfileTypeInfoCell(), SKIP_BARRIER);
    funcTemp->SetLength(length);
    return funcTemp;
}

JSHandle<VTable> ObjectFactory::NewVTable(uint32_t length, JSTaggedValue initVal)
{
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length * VTable::TUPLE_SIZE);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetVTableClass().GetTaggedObject()), size);

    JSHandle<VTable> vtable(thread_, header);
    vtable->InitializeWithSpecialValue(initVal, length * VTable::TUPLE_SIZE);
    return vtable;
}

JSHandle<ClassLiteral> ObjectFactory::NewClassLiteral()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetClassLiteralClass().GetTaggedObject()));
    JSHandle<TaggedArray> emptyArray = EmptyArray();

    JSHandle<ClassLiteral> classLiteral(thread_, header);
    classLiteral->SetArray(thread_, emptyArray);
    classLiteral->SetIsAOTUsed(false);

    return classLiteral;
}

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<Method> &methodHandle)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    FunctionKind kind = methodHandle->GetFunctionKind();
    JSHandle<JSHClass> hclass;
    switch (kind) {
        case FunctionKind::NORMAL_FUNCTION:
        case FunctionKind::BASE_CONSTRUCTOR: {
            hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
            break;
        }
        case FunctionKind::ARROW_FUNCTION: {
            hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
            break;
        }
        case FunctionKind::GENERATOR_FUNCTION: {
            hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
            break;
        }
        case FunctionKind::CONCURRENT_FUNCTION:
        case FunctionKind::ASYNC_ARROW_FUNCTION:
        case FunctionKind::ASYNC_FUNCTION: {
            hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
            break;
        }
        case FunctionKind::ASYNC_GENERATOR_FUNCTION: {
            hclass = JSHandle<JSHClass>::Cast(env->GetAsyncGeneratorFunctionClass());
            break;
        }
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    JSHandle<JSFunction> jsfunc = NewJSFunctionByHClass(methodHandle, hclass);
    ASSERT_NO_ABRUPT_COMPLETION(thread_);
    return jsfunc;
}

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<Method> &methodHandle,
                                                  const JSHandle<JSTaggedValue> &homeObject)
{
    ASSERT(homeObject->IsECMAObject());
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());

    JSHandle<JSFunction> jsFunc = NewJSFunctionByHClass(methodHandle, hclass);
    jsFunc->SetHomeObject(thread_, homeObject);
    ASSERT_NO_ABRUPT_COMPLETION(thread_);
    return jsFunc;
}

JSHandle<JSTaggedValue> ObjectFactory::CreateJSObjectWithProperties(size_t propertyCount,
                                                                    const Local<JSValueRef> *keys,
                                                                    const PropertyDescriptor *descs)
{
    if (!CanObjectLiteralHClassCache(propertyCount)) {
        return CreateLargeJSObjectWithProperties(propertyCount, keys, descs);
    }

    // At least 4 inlined slot
    int inlineProps = std::max(static_cast<int>(propertyCount), JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);
    JSMutableHandle<JSHClass> hclassHandle(thread_, GetObjectLiteralRootHClass(inlineProps, inlineProps));
    for (size_t i = 0; i < propertyCount; ++i) {
        JSMutableHandle<JSTaggedValue> key(JSNApiHelper::ToJSMutableHandle(keys[i]));
        if (key->IsString() && !EcmaStringAccessor(key.GetTaggedValue()).IsInternString()) {
            // update string stable
            key.Update(JSTaggedValue(InternString(key)));
        }
        ASSERT(EcmaStringAccessor(key->GetTaggedObject()).IsInternString());
        if (UNLIKELY(!JSTaggedValue::IsPureString(key.GetTaggedValue()))) {
            THROW_TYPE_ERROR_AND_RETURN(thread_, "property key must be string and can not convert into element index",
                                        JSHandle<JSTaggedValue>());
        }
        PropertyAttributes attr(descs[i]);
        attr.SetIsInlinedProps(true);
        attr.SetOffset(i);
        attr.SetRepresentation(Representation::TAGGED);
        auto value = descs[i].GetValue().GetTaggedValue();
        auto rep = PropertyAttributes::TranslateToRep(value);
        hclassHandle.Update(JSHClass::SetPropertyOfObjHClass<true>(thread_, hclassHandle, key, attr, rep));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, JSHandle<JSTaggedValue>());
    }
    JSHandle<JSObject> object = NewJSObject(hclassHandle);

    for (size_t i = 0; i < propertyCount; ++i) {
        object->SetPropertyInlinedProps<true>(thread_, i, descs[i].GetValue().GetTaggedValue());
    }
    return JSHandle<JSTaggedValue>(object);
}

JSHandle<JSTaggedValue> ObjectFactory::CreateLargeJSObjectWithProperties(size_t propertyCount,
                                                                         const Local<JSValueRef> *keys,
                                                                         const PropertyDescriptor *descs)
{
    ASSERT(!CanObjectLiteralHClassCache(propertyCount));
    if (UNLIKELY(propertyCount > PropertyAttributes::MAX_FAST_PROPS_CAPACITY)) {
        return CreateDictionaryJSObjectWithProperties(propertyCount, keys, descs);
    }
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFuncProto = env->GetObjectFunctionPrototype();

    JSHandle<JSHClass> hClassHandle = NewEcmaHClass(JSObject::SIZE, propertyCount, JSType::JS_OBJECT, objFuncProto);
    JSHandle<LayoutInfo> layoutHandle = CreateLayoutInfo(propertyCount);
    JSHandle<JSObject> object = NewJSObject(hClassHandle);

    for (size_t i = 0; i < propertyCount; ++i) {
        JSMutableHandle<JSTaggedValue> key(JSNApiHelper::ToJSMutableHandle(keys[i]));
        if (key->IsString() && !EcmaStringAccessor(key.GetTaggedValue()).IsInternString()) {
            // update string stable
            key.Update(JSTaggedValue(InternString(key)));
        }
        ASSERT(EcmaStringAccessor(key->GetTaggedObject()).IsInternString());
        if (UNLIKELY(!JSTaggedValue::IsPureString(key.GetTaggedValue()))) {
            THROW_TYPE_ERROR_AND_RETURN(thread_, "property key must be string and can not convert into element index",
                                        JSHandle<JSTaggedValue>());
        }
        PropertyAttributes attr(descs[i]);
        attr.SetIsInlinedProps(true);
        attr.SetOffset(i);
        LayoutInfo *layout = LayoutInfo::Cast(layoutHandle.GetTaggedValue().GetTaggedObject());
        layout->AddKey<true>(thread_, i, key.GetTaggedValue(), attr);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, JSHandle<JSTaggedValue>());
        object->SetPropertyInlinedProps<true>(thread_, i, descs[i].GetValue().GetTaggedValue());
    }
    hClassHandle->SetNumberOfProps(propertyCount);
    hClassHandle->SetLayout(thread_, layoutHandle);
    return JSHandle<JSTaggedValue>(object);
}

JSHandle<JSTaggedValue> ObjectFactory::CreateDictionaryJSObjectWithProperties(size_t propertyCount,
                                                                              const Local<JSValueRef> *keys,
                                                                              const PropertyDescriptor *descs)
{
    ASSERT(propertyCount > PropertyAttributes::MAX_FAST_PROPS_CAPACITY);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFuncProto = env->GetObjectFunctionPrototype();
    JSHandle<JSHClass> hClassHandle = NewEcmaHClass(JSObject::SIZE, 0, JSType::JS_OBJECT, objFuncProto);
    hClassHandle->SetNumberOfProps(0);
    hClassHandle->SetIsDictionaryMode(true);
    JSHandle<JSObject> object = NewJSObject(hClassHandle);

    JSMutableHandle<NameDictionary> dict(
        thread_, NameDictionary::Create(thread_, NameDictionary::ComputeHashTableSize(propertyCount)));
    for (size_t i = 0; i < propertyCount; ++i) {
        JSMutableHandle<JSTaggedValue> key(JSNApiHelper::ToJSMutableHandle(keys[i]));
        if (key->IsString() && !EcmaStringAccessor(key.GetTaggedValue()).IsInternString()) {
            // update string stable
            key.Update(JSTaggedValue(InternString(key)));
        }
        ASSERT(EcmaStringAccessor(key->GetTaggedObject()).IsInternString());
        if (UNLIKELY(!JSTaggedValue::IsPureString(key.GetTaggedValue()))) {
            THROW_TYPE_ERROR_AND_RETURN(thread_, "property key must be string and can not convert into element index",
                                        JSHandle<JSTaggedValue>());
        }
        PropertyAttributes attr(descs[i]);
        dict.Update(NameDictionary::PutIfAbsent(thread_, dict, key, descs[i].GetValue(), attr));
    }
    if (UNLIKELY(dict->EntriesCount() != static_cast<int>(propertyCount))) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "property keys can not duplicate", JSHandle<JSTaggedValue>());
    }
    object->SetProperties(thread_, dict);
    return JSHandle<JSTaggedValue>(object);
}

JSHandle<JSTaggedValue> ObjectFactory::CreateJSObjectWithNamedProperties(size_t propertyCount, const char **keys,
                                                                         const Local<JSValueRef> *values)
{
    if (!CanObjectLiteralHClassCache(propertyCount)) {
        return CreateLargeJSObjectWithNamedProperties(propertyCount, keys, values);
    }

    // At least 4 inlined slot
    int inlineProps = std::max(static_cast<int>(propertyCount), JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS);
    JSMutableHandle<JSHClass> hclassHandle(thread_, GetObjectLiteralRootHClass(inlineProps, inlineProps));
    for (size_t i = 0; i < propertyCount; ++i) {
        JSHandle<JSTaggedValue> key(NewFromUtf8(keys[i]));
        ASSERT(EcmaStringAccessor(key->GetTaggedObject()).IsInternString());
        if (UNLIKELY(!JSTaggedValue::IsPureString(key.GetTaggedValue()))) {
            THROW_TYPE_ERROR_AND_RETURN(thread_, "property key must be string and can not convert into element index",
                                        JSHandle<JSTaggedValue>());
        }
        PropertyAttributes attr(PropertyAttributes::GetDefaultAttributes());
        attr.SetIsInlinedProps(true);
        attr.SetOffset(i);
        attr.SetRepresentation(Representation::TAGGED);
        auto value = JSNApiHelper::ToJSHandle(values[i]).GetTaggedValue();
        auto rep = PropertyAttributes::TranslateToRep(value);
        hclassHandle.Update(JSHClass::SetPropertyOfObjHClass<true>(thread_, hclassHandle, key, attr, rep));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, JSHandle<JSTaggedValue>());
    }
    JSHandle<JSObject> object = NewJSObject(hclassHandle);

    for (size_t i = 0; i < propertyCount; ++i) {
        object->SetPropertyInlinedProps<true>(thread_, i, JSNApiHelper::ToJSHandle(values[i]).GetTaggedValue());
    }
    return JSHandle<JSTaggedValue>(object);
}

JSHandle<JSTaggedValue> ObjectFactory::CreateLargeJSObjectWithNamedProperties(size_t propertyCount, const char **keys,
                                                                              const Local<JSValueRef> *values)
{
    ASSERT(!CanObjectLiteralHClassCache(propertyCount));
    if (UNLIKELY(propertyCount > PropertyAttributes::MAX_FAST_PROPS_CAPACITY)) {
        return CreateDictionaryJSObjectWithNamedProperties(propertyCount, keys, values);
    }
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFuncProto = env->GetObjectFunctionPrototype();

    JSHandle<JSHClass> hClassHandle = NewEcmaHClass(JSObject::SIZE, propertyCount, JSType::JS_OBJECT, objFuncProto);
    JSHandle<LayoutInfo> layoutHandle = CreateLayoutInfo(propertyCount);
    JSHandle<JSObject> object = NewJSObject(hClassHandle);

    for (size_t i = 0; i < propertyCount; ++i) {
        JSHandle<JSTaggedValue> key(NewFromUtf8(keys[i]));
        ASSERT(EcmaStringAccessor(key->GetTaggedObject()).IsInternString());
        if (UNLIKELY(!JSTaggedValue::IsPureString(key.GetTaggedValue()))) {
            THROW_TYPE_ERROR_AND_RETURN(thread_, "property key must be string and can not convert into element index",
                                        JSHandle<JSTaggedValue>());
        }
        PropertyAttributes attr(PropertyAttributes::GetDefaultAttributes());
        attr.SetIsInlinedProps(true);
        attr.SetOffset(i);
        LayoutInfo *layout = LayoutInfo::Cast(layoutHandle.GetTaggedValue().GetTaggedObject());
        layout->AddKey<true>(thread_, i, key.GetTaggedValue(), attr);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread_, JSHandle<JSTaggedValue>());
        object->SetPropertyInlinedProps<true>(thread_, i, JSNApiHelper::ToJSHandle(values[i]).GetTaggedValue());
    }
    hClassHandle->SetNumberOfProps(propertyCount);
    hClassHandle->SetLayout(thread_, layoutHandle);
    return JSHandle<JSTaggedValue>(object);
}

JSHandle<JSTaggedValue> ObjectFactory::CreateDictionaryJSObjectWithNamedProperties(size_t propertyCount,
                                                                                   const char **keys,
                                                                                   const Local<JSValueRef> *values)
{
    ASSERT(propertyCount > PropertyAttributes::MAX_FAST_PROPS_CAPACITY);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFuncProto = env->GetObjectFunctionPrototype();
    JSHandle<JSHClass> hClassHandle = NewEcmaHClass(JSObject::SIZE, 0, JSType::JS_OBJECT, objFuncProto);
    hClassHandle->SetNumberOfProps(0);
    hClassHandle->SetIsDictionaryMode(true);
    JSHandle<JSObject> object = NewJSObject(hClassHandle);

    JSMutableHandle<NameDictionary> dict(
        thread_, NameDictionary::Create(thread_, NameDictionary::ComputeHashTableSize(propertyCount)));
    for (size_t i = 0; i < propertyCount; ++i) {
        JSHandle<JSTaggedValue> key(NewFromUtf8(keys[i]));
        ASSERT(EcmaStringAccessor(key->GetTaggedObject()).IsInternString());
        if (UNLIKELY(!JSTaggedValue::IsPureString(key.GetTaggedValue()))) {
            THROW_TYPE_ERROR_AND_RETURN(thread_, "property key must be string and can not convert into element index",
                                        JSHandle<JSTaggedValue>());
        }
        PropertyAttributes attr(PropertyAttributes::GetDefaultAttributes());
        dict.Update(NameDictionary::PutIfAbsent(thread_, dict, key, JSNApiHelper::ToJSHandle(values[i]), attr));
    }
    if (UNLIKELY(dict->EntriesCount() != static_cast<int>(propertyCount))) {
        THROW_TYPE_ERROR_AND_RETURN(thread_, "property keys can not duplicate", JSHandle<JSTaggedValue>());
    }
    object->SetProperties(thread_, dict);
    return JSHandle<JSTaggedValue>(object);
}

void ObjectFactory::FillFreeMemoryRange(uintptr_t start, uintptr_t end)
{
    ASSERT(start <= end);
    ASSERT(start % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT) == 0);
    ASSERT(end % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT) == 0);
    while (start < end) {
        Barriers::SetPrimitive<JSTaggedType>(reinterpret_cast<void*>(start), 0, FREE_MEMMORY_ADDRESS_ZAM_VALUE);
        start += sizeof(JSTaggedType);
    }
}
}  // namespace panda::ecmascript
