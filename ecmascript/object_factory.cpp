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

#include "ecmascript/accessor_data.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/builtins/builtins.h"
#include "ecmascript/builtins/builtins_async_from_sync_iterator.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/compiler/aot_file/aot_file_manager.h"
#include "ecmascript/compiler/builtins/builtins_call_signature.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/free_object.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_api/js_api_hashmap.h"
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
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_map_iterator.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_tree_set_iterator.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_async_from_sync_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/js_pandafile.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/layout_info-inl.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/record.h"
#include "ecmascript/require/js_cjs_exports.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/shared_mm/shared_mm.h"
#include "ecmascript/symbol_table.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/tagged_list.h"
#include "ecmascript/tagged_node.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/template_map.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"
#include "ecmascript/ts_types/ts_type.h"
#include "ecmascript/ts_types/ts_type_table.h"
#include "ecmascript/vtable.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_relative_time_format.h"
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

ObjectFactory::ObjectFactory(JSThread *thread, Heap *heap)
    : thread_(thread), vm_(thread->GetEcmaVM()), heap_(heap) {}

JSHandle<Method> ObjectFactory::NewMethodForNativeFunction(const void *func, FunctionKind kind,
                                                           kungfu::BuiltinsStubCSigns::ID builtinId,
                                                           MemSpaceType spaceType)
{
    uint32_t numArgs = 2;  // function object and this
    auto method = NewMethod(nullptr, spaceType);
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
    hclass->SetClass(hclass);
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

JSHandle<JSHClass> ObjectFactory::NewEcmaReadOnlyHClass(JSHClass *hclass, uint32_t size, JSType type,
                                                          uint32_t inlinedProps)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateReadOnlyOrHugeObject(hclass, classSize));
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
        if (memset_s(newData, length, 0, length) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        pointer->ResetExternalPointer(newData);
        return;
    }

    auto newData = vm_->GetNativeAreaAllocator()->AllocateBuffer(size);
    if (memset_s(newData, length, 0, length) != EOK) {
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(newData, NativeAreaAllocator::FreeBufferFunc,
                                                           vm_->GetNativeAreaAllocator(), false, size);
    array->SetArrayBufferData(thread_, pointer);
    array->SetWithNativeAreaAllocator(true);
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
        if (memset_s(newData, length, 0, length) != EOK) {
            LOG_FULL(FATAL) << "memset_s failed";
            UNREACHABLE();
        }
        JSHandle<JSNativePointer> pointer = NewJSNativePointer(newData, NativeAreaAllocator::FreeBufferFunc,
                                                               vm_->GetNativeAreaAllocator(), false, length);
        arrayBuffer->SetArrayBufferData(thread_, pointer.GetTaggedValue());
        arrayBuffer->SetWithNativeAreaAllocator(true);
    }
    return arrayBuffer;
}

JSHandle<JSArrayBuffer> ObjectFactory::NewJSArrayBuffer(void *buffer, int32_t length, const DeleteEntryPoint &deleter,
                                                        void *data, bool share)
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
    if (arrayLength - offset < length) {
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
    if (memcpy_s(newBuffer, size, buffer, size) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    JSTaggedValue data = regexp->GetByteCodeBuffer();
    if (!data.IsUndefined()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(newBuffer);
        return;
    }
    JSHandle<JSNativePointer> pointer = NewJSNativePointer(newBuffer, NativeAreaAllocator::FreeBufferFunc,
                                                           vm_->GetNativeAreaAllocator(), false, size);
    regexp->SetByteCodeBuffer(thread_, pointer.GetTaggedValue());
    regexp->SetLength(static_cast<uint32_t>(size));
}

JSHandle<JSHClass> ObjectFactory::NewEcmaHClass(uint32_t size, JSType type, const JSHandle<JSTaggedValue> &prototype,
                                                bool isOptimized, bool canFastCall)
{
    NewObjectHook();
    uint32_t classSize = JSHClass::SIZE;
    auto *newClass = static_cast<JSHClass *>(heap_->AllocateNonMovableOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetHClassClass().GetTaggedObject()), classSize));
    newClass->Initialize(thread_, size, type, JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS, isOptimized, canFastCall);
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

    for (uint32_t i = 0; i < klass->GetInlinedProperties(); i++) {
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
            JSHandle<JSFunction> newFunc = CloneJSFuction(valueHandle);
            newFunc->SetLexicalEnv(thread_, env);
            newFunc->SetHomeObject(thread_, obj);
            newArray->Set(thread_, i, newFunc);
        }
    }
    return newArray;
}

JSHandle<JSObject> ObjectFactory::CloneObjectLiteral(JSHandle<JSObject> object, const JSHandle<JSTaggedValue> &env,
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

    for (uint32_t i = 0; i < klass->GetInlinedProperties(); i++) {
        auto layout = LayoutInfo::Cast(klass->GetLayout().GetTaggedObject());
        JSTaggedValue value = object->GetPropertyInlinedProps(i);
        if (!layout->GetAttr(i).IsTaggedRep() || !value.IsJSFunction()) {
            cloneObject->SetPropertyInlinedPropsWithRep(thread_, i, value);
        } else {
            JSHandle<JSFunction> valueHandle(thread_, value);
            JSHandle<JSFunction> newFunc = CloneJSFuction(valueHandle);
            newFunc->SetLexicalEnv(thread_, env);
            newFunc->SetHomeObject(thread_, cloneObject);
            cloneObject->SetPropertyInlinedProps(thread_, i, newFunc.GetTaggedValue());
        }
    }
    return cloneObject;
}

JSHandle<JSFunction> ObjectFactory::CloneJSFuction(JSHandle<JSFunction> func)
{
    JSHandle<JSHClass> jshclass(thread_, func->GetJSHClass());
    JSHandle<Method> method(thread_, func->GetMethod());
    JSHandle<JSFunction> cloneFunc = NewJSFunctionByHClass(method, jshclass);

    JSTaggedValue length = func->GetPropertyInlinedProps(JSFunction::LENGTH_INLINE_PROPERTY_INDEX);
    cloneFunc->SetPropertyInlinedProps(thread_, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, length);
    cloneFunc->SetModule(thread_, func->GetModule());
    return cloneFunc;
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

    for (uint32_t i = 0; i < hclass->GetInlinedProperties(); i++) {
        auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
        JSTaggedValue value = ctor->GetPropertyInlinedProps(i);
        if (!layout->GetAttr(i).IsTaggedRep() || !value.IsJSFunction()) {
            cloneCtor->SetPropertyInlinedPropsWithRep(thread_, i, value);
        } else {
            JSHandle<JSFunction> valueHandle(thread_, value);
            JSHandle<JSFunction> newFunc = CloneJSFuction(valueHandle);
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

JSHandle<JSForInIterator> ObjectFactory::NewJSForinIterator(const JSHandle<JSTaggedValue> &obj)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass(env->GetForinIteratorClass());

    JSHandle<JSForInIterator> it = JSHandle<JSForInIterator>::Cast(NewJSObject(hclass));
    it->SetObject(thread_, obj);
    it->SetVisitedObjs(thread_, thread_->GlobalConstants()->GetEmptyTaggedQueue());
    it->SetRemainingKeys(thread_, thread_->GlobalConstants()->GetEmptyTaggedQueue());
    it->ClearBitField();
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

JSHandle<JSHClass> ObjectFactory::CreateJSArrayInstanceClass(JSHandle<JSTaggedValue> proto)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> arrayClass = NewEcmaHClass(JSArray::SIZE, JSType::JS_ARRAY, proto);

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

JSHandle<JSObject> ObjectFactory::GetJSError(const ErrorType &errorType, const char *data, bool needCheckStack)
{
    ASSERT_PRINT(errorType == ErrorType::ERROR || errorType == ErrorType::EVAL_ERROR ||
                     errorType == ErrorType::RANGE_ERROR || errorType == ErrorType::REFERENCE_ERROR ||
                     errorType == ErrorType::SYNTAX_ERROR || errorType == ErrorType::TYPE_ERROR ||
                     errorType == ErrorType::URI_ERROR || errorType == ErrorType::OOM_ERROR,
                 "The error type is not in the valid range.");
    if (data != nullptr) {
        JSHandle<EcmaString> handleMsg = NewFromUtf8(data);
        return NewJSError(errorType, handleMsg, needCheckStack);
    }
    JSHandle<EcmaString> emptyString(thread_->GlobalConstants()->GetHandledEmptyString());
    return NewJSError(errorType, emptyString, needCheckStack);
}

JSHandle<JSObject> ObjectFactory::NewJSError(const ErrorType &errorType, const JSHandle<EcmaString> &message,
    bool needCheckStack)
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
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
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
        default:
            nativeConstructor = env->GetErrorFunction();
            break;
    }
    JSHandle<JSFunction> nativeFunc = JSHandle<JSFunction>::Cast(nativeConstructor);
    JSHandle<JSTaggedValue> nativePrototype(thread_, nativeFunc->GetFunctionPrototype());
    JSHandle<JSTaggedValue> ctorKey = globalConst->GetHandledConstructorString();
    JSHandle<JSTaggedValue> ctor(JSTaggedValue::GetProperty(thread_, nativePrototype, ctorKey).GetValue());
    JSHandle<JSTaggedValue> undefined = thread_->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread_, ctor, nativePrototype, undefined, 1, needCheckStack);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread_);
    info->SetCallArg(message.GetTaggedValue());
    Method *method = JSHandle<ECMAObject>::Cast(ctor)->GetCallTarget();
    JSTaggedValue obj = reinterpret_cast<EcmaEntrypoint>(const_cast<void *>(method->GetNativePointer()))(info);
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

JSHandle<JSObject> ObjectFactory::NewJSObjectByConstructor(const JSHandle<JSFunction> &constructor)
{
    if (!constructor->HasFunctionPrototype() ||
        (constructor->GetProtoOrHClass().IsHeapObject() && constructor->GetFunctionPrototype().IsECMAObject())) {
        JSHandle<JSHClass> jshclass = JSFunction::GetInstanceJSHClass(thread_, constructor,
                                                                      JSHandle<JSTaggedValue>(constructor));
        return NewJSObjectWithInit(jshclass);
    }
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
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
    return NewJSObjectWithInit(jshclass);
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
        case JSType::JS_ASYNCITERATOR:
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
        case JSType::JS_LIST_FORMAT: {
            break;
        }
#endif
        case JSType::JS_ARRAY: {
            JSArray::Cast(*obj)->SetLength(0);
            ASSERT(!obj->GetJSHClass()->IsDictionaryMode());
            auto accessor = thread_->GlobalConstants()->GetArrayLengthAccessor();
            JSArray::Cast(*obj)->SetPropertyInlinedProps(thread_, JSArray::LENGTH_INLINE_PROPERTY_INDEX, accessor);
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
        case JSType::JS_MAP:
            JSMap::Cast(*obj)->SetLinkedMap(thread_, JSTaggedValue::Undefined());
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
        case JSType::JS_PROMISE:
            JSPromise::Cast(*obj)->SetPromiseState(PromiseState::PENDING);
            JSPromise::Cast(*obj)->SetPromiseResult(thread_, JSTaggedValue::Undefined());
            JSPromise::Cast(*obj)->SetPromiseRejectReactions(thread_, GetEmptyTaggedQueue().GetTaggedValue());
            JSPromise::Cast(*obj)->SetPromiseFulfillReactions(thread_, GetEmptyTaggedQueue().GetTaggedValue());

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
        case JSType::JS_API_LIST: {
            JSAPIList::Cast(*obj)->SetSingleList(thread_, JSTaggedValue::Undefined());
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
            LOG_ECMA(FATAL) << "this branch is unreachable";
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
        if (!region->InYoungSpace()) {
            heap_->ClearSlotsRange(region, address, address + size);
        }
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
    JSTaggedType initVal = hclass->IsTS() ? JSTaggedValue::VALUE_HOLE : JSTaggedValue::VALUE_UNDEFINED;
    for (uint32_t i = 0; i < inobjPropCount; ++i) {
        paddr -= JSTaggedValue::TaggedTypeSize();
        *reinterpret_cast<JSTaggedType *>(paddr) = initVal;
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

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc,
                                                  FunctionKind kind, kungfu::BuiltinsStubCSigns::ID builtinId,
                                                  MemSpaceType spaceType)
{
    JSHandle<Method> target = NewMethodForNativeFunction(nativeFunc, kind, builtinId, spaceType);
    return NewJSFunction(env, target);
}

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<GlobalEnv> &env,
                                                  const JSHandle<Method> &method)
{
    FunctionKind kind = method->GetFunctionKind();
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

    return NewJSFunctionByHClass(method, hclass);
}

JSHandle<JSHClass> ObjectFactory::CreateFunctionClass(FunctionKind kind, uint32_t size, JSType type,
                                                      const JSHandle<JSTaggedValue> &prototype,
                                                      bool isOptimized, bool canFastCall)
{
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    JSHandle<JSHClass> functionClass = NewEcmaHClass(size, type, prototype, isOptimized, canFastCall);
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
        PropertyAttributes attributes = PropertyAttributes::Default(false, false, true);
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
    return function;
}

JSHandle<JSFunction> ObjectFactory::NewJSFunctionByHClass(const void *func, const JSHandle<JSHClass> &clazz,
                                                          FunctionKind kind)
{
    JSHandle<Method> method = NewMethodForNativeFunction(func, kind);
    JSHandle<JSFunction> function = JSHandle<JSFunction>::Cast(NewJSObject(clazz));
    clazz->SetCallable(true);
    clazz->SetExtensible(true);
    JSFunction::InitializeJSFunction(thread_, function, kind);
    function->SetMethod(thread_, method);
    return function;
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
    if (methodLiteral != nullptr) {
        method->SetCallField(methodLiteral->GetCallField());
        method->SetLiteralInfo(methodLiteral->GetLiteralInfo());
        method->SetNativePointerOrBytecodeArray(const_cast<void *>(methodLiteral->GetNativePointer()));
        method->SetExtraLiteralInfo(methodLiteral->GetExtraLiteralInfo());
    } else {
        method->SetCallField(0ULL);
        method->SetLiteralInfo(0ULL);
        method->SetNativePointerOrBytecodeArray(nullptr);
        method->SetExtraLiteralInfo(0ULL);
    }
    method->SetCodeEntryOrLiteral(reinterpret_cast<uintptr_t>(methodLiteral));
    method->SetConstantPool(thread_, JSTaggedValue::Undefined());
    method->SetProfileTypeInfo(thread_, JSTaggedValue::Undefined());
    return method;
}

JSHandle<Method> ObjectFactory::NewMethod(const JSPandaFile *jsPandaFile, MethodLiteral *methodLiteral,
    JSHandle<ConstantPool> constpool, uint32_t entryIndex, bool needSetAotFlag, bool *canFastCall)
{
    JSHandle<Method> method;
    if (jsPandaFile->IsNewVersion()) {
        method = Method::Create(thread_, jsPandaFile, methodLiteral);
    } else {
        method = NewMethod(methodLiteral);
        method->SetConstantPool(thread_, constpool);
    }
    if (needSetAotFlag) {
        thread_->GetCurrentEcmaContext()->GetAOTFileManager()->
            SetAOTFuncEntry(jsPandaFile, *method, entryIndex, canFastCall);
    }
    return method;
}

JSHandle<JSFunction> ObjectFactory::NewJSNativeErrorFunction(const JSHandle<GlobalEnv> &env, const void *nativeFunc)
{
    JSHandle<Method> target = NewMethodForNativeFunction(nativeFunc, FunctionKind::BUILTIN_CONSTRUCTOR);
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetNativeErrorFunctionClass());
    return NewJSFunctionByHClass(target, hclass);
}

JSHandle<JSFunction> ObjectFactory::NewSpecificTypedArrayFunction(const JSHandle<GlobalEnv> &env,
                                                                  const void *nativeFunc)
{
    JSHandle<Method> target = NewMethodForNativeFunction(nativeFunc, FunctionKind::BUILTIN_CONSTRUCTOR);
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetSpecificTypedArrayFunctionClass());
    return NewJSFunctionByHClass(target, hclass);
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

JSHandle<JSBoundFunction> ObjectFactory::NewJSBoundFunction(const JSHandle<JSFunctionBase> &target,
                                                            const JSHandle<JSTaggedValue> &boundThis,
                                                            const JSHandle<TaggedArray> &args)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetFunctionPrototype();
    JSHandle<JSHClass> hclass = NewEcmaHClass(JSBoundFunction::SIZE, JSType::JS_BOUND_FUNCTION, proto);

    JSHandle<JSBoundFunction> bundleFunction = JSHandle<JSBoundFunction>::Cast(NewJSObject(hclass));
    bundleFunction->SetBoundTarget(thread_, target);
    bundleFunction->SetBoundThis(thread_, boundThis);
    bundleFunction->SetBoundArguments(thread_, args);
    hclass->SetCallable(true);
    if (target.GetTaggedValue().IsConstructor()) {
        bundleFunction->SetConstructor(true);
    }
    bundleFunction->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_GLOBAL_CALL_JS_BOUND_FUNCTION));
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
    function->SetMethod(thread_, vm_->GetMethodByIndex(idx));
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
    function->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROXY_INVALIDATE_PROXY_FUNCTION));
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
    awaitFunction->SetMethod(thread_, vm_->GetMethodByIndex(idx));
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
    JSHandle<JSGeneratorObject> generatorObject = JSHandle<JSGeneratorObject>::Cast(NewJSObject(hclass));
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
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();
    if (function.GetTaggedValue() == env->GetStringFunction().GetTaggedValue()) {
        JSHandle<JSTaggedValue> lengthStr = globalConst->GetHandledLengthString();
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
            break;
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
        obj = JSHandle<JSPrimitiveRef>::Cast(NewJSObjectByConstructor(stringFunc, newTarget));
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

JSHandle<JSSymbol> ObjectFactory::NewPrivateNameSymbolWithChar(const char *description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewPrivateNameSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewWellKnownSymbolWithChar(const char *description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewWellKnownSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewPublicSymbolWithChar(const char *description)
{
    JSHandle<EcmaString> string = NewFromUtf8(description);
    return NewPublicSymbol(JSHandle<JSTaggedValue>(string));
}

JSHandle<JSSymbol> ObjectFactory::NewSymbolWithTableWithChar(const char *description)
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
    JSHandle<AccessorData> obj(thread_, AccessorData::Cast(header));
    obj->SetGetter(thread_, JSTaggedValue::Undefined());
    obj->SetSetter(thread_, JSTaggedValue::Undefined());
    if (setter != nullptr) {
        JSHandle<JSNativePointer> setFunc = NewJSNativePointer(setter, nullptr, nullptr, true);
        obj->SetSetter(thread_, setFunc.GetTaggedValue());
    } else {
        JSTaggedValue setFunc = JSTaggedValue::Undefined();
        obj->SetSetter(thread_, setFunc);
        ASSERT(!obj->HasSetter());
    }
    JSHandle<JSNativePointer> getFunc = NewJSNativePointer(getter, nullptr, nullptr, true);
    obj->SetGetter(thread_, getFunc);
    return obj;
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
#endif
    return obj;
}

JSHandle<JSProxy> ObjectFactory::NewJSProxy(const JSHandle<JSTaggedValue> &target,
                                            const JSHandle<JSTaggedValue> &handler)
{
    NewObjectHook();
    TaggedObject *header = nullptr;
    const GlobalEnvConstants *globalConst = thread_->GlobalConstants();

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
    proxy->SetIsRevoked(false);
    return proxy;
}

JSHandle<JSRealm> ObjectFactory::NewJSRealm()
{
    JSHandle<JSHClass> hClassHandle = NewEcmaHClassClass(nullptr, JSHClass::SIZE, JSType::HCLASS);
    JSHClass *hclass = reinterpret_cast<JSHClass *>(hClassHandle.GetTaggedValue().GetTaggedObject());
    hclass->SetClass(hclass);
    JSHandle<JSHClass> realmEnvClass = NewEcmaHClass(*hClassHandle, GlobalEnv::SIZE, JSType::GLOBAL_ENV);
    JSHandle<GlobalEnv> realmEnvHandle = NewGlobalEnv(*realmEnvClass);

    auto result = TemplateMap::Create(thread_);
    realmEnvHandle->SetTemplateMap(thread_, result);

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

JSHandle<TaggedArray> ObjectFactory::NewEmptyArray()
{
    NewObjectHook();
    auto header = heap_->AllocateReadOnlyOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), TaggedArray::SIZE);
    JSHandle<TaggedArray> array(thread_, header);
    array->SetLength(0);
    array->SetExtraLength(0);
    return array;
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
    if (newLength == 0) {
        return dstElements;
    }
    Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(*dstElements));
    if (region->InYoungSpace() && !region->IsMarking()) {
        size_t size = oldLength * sizeof(JSTaggedType);
        if (memcpy_s(reinterpret_cast<void *>(dstElements->GetData()), size,
            reinterpret_cast<void *>(srcElements->GetData() + k), size) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
        }
    } else {
        for (uint32_t i = 0; i < oldLength; i++) {
            dstElements->Set(thread_, i, srcElements->Get(i + k));
        }
    }
    for (uint32_t i = oldLength; i < newLength; i++) {
        dstElements->Set(thread_, i, JSTaggedValue::Hole());
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
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    JSHandle<TaggedArray> array(thread_, header);
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

JSHandle<ByteArray> ObjectFactory::NewByteArray(uint32_t length, uint32_t size)
{
    size_t byteSize = ByteArray::ComputeSize(size, length);
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetByteArrayClass().GetTaggedObject());
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(arrayClass, byteSize);
    JSHandle<ByteArray> array(thread_, header);

    void *data = array->GetData();
    if (memset_s(data, length * size, 0, length * size) != EOK) {
        LOG_FULL(FATAL) << "memset_s failed";
        UNREACHABLE();
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
                                                 JSTaggedValue initVal, MemSpaceType type)
{
    ASSERT(length > old->GetLength());
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    TaggedObject *header = AllocObjectWithSpaceType(size, arrayClass, type);
    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(length);
    newArray->SetExtraLength(old->GetExtraLength());

    uint32_t oldLength = old->GetLength();
    for (uint32_t i = 0; i < oldLength; i++) {
        JSTaggedValue value = old->Get(i);
        newArray->Set(thread_, i, value);
    }

    for (uint32_t i = oldLength; i < length; i++) {
        newArray->Set(thread_, i, initVal);
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
                                               JSTaggedValue initVal, MemSpaceType type)
{
    if (newLength == 0) {
        return EmptyArray();
    }
    if (newLength > oldLength) {
        return ExtendArray(old, newLength, initVal, type);
    }
    NewObjectHook();
    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), newLength);
    TaggedObject *header = nullptr;
    if (type == MemSpaceType::NON_MOVABLE) {
        // COW array is shared in nonmovable space.
        JSHClass *cowArrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetCOWArrayClass().GetTaggedObject());
        header = AllocObjectWithSpaceType(size, cowArrayClass, type);
    } else {
        JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
        header = AllocObjectWithSpaceType(size, arrayClass, type);
    }

    JSHandle<TaggedArray> newArray(thread_, header);
    newArray->SetLength(newLength);
    newArray->SetExtraLength(old->GetExtraLength());

    for (uint32_t i = 0; i < newLength; i++) {
        JSTaggedValue value = old->Get(i);
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
    return JSHandle<LayoutInfo>(CopyArray(JSHandle<TaggedArray>::Cast(old), newLength, newLength));
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

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(const uint8_t *utf8Data, uint32_t utf8Len,
                                                             bool canBeCompress) const
{
    NewObjectHook();
    if (utf8Len == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(utf8Data, utf8Len, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTableNonMovable(const uint8_t *utf8Data, uint32_t utf8Len) const
{
    NewObjectHook();
    if (utf8Len == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->CreateAndInternStringNonMovable(utf8Data, utf8Len));
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(const uint16_t *utf16Data, uint32_t utf16Len,
                                                             bool canBeCompress) const
{
    NewObjectHook();
    if (utf16Len == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(utf16Data, utf16Len, canBeCompress));
}

JSHandle<EcmaString> ObjectFactory::GetStringFromStringTable(EcmaString *string) const
{
    ASSERT(string != nullptr);
    if (EcmaStringAccessor(string).GetLength() == 0) {
        return GetEmptyString();
    }
    auto stringTable = vm_->GetEcmaStringTable();
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(string));
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
        return vm_->GetEcmaStringTable()->GetOrInternStringWithSpaceType(mutf8Data, utf16Len, true, type,
                                                                         isConstantString, idOffset);
    }

    CVector<uint16_t> utf16Data(utf16Len);
    auto len = utf::ConvertRegionMUtf8ToUtf16(mutf8Data, utf16Data.data(), utf::Mutf8Size(mutf8Data), utf16Len, 0);
    return vm_->GetEcmaStringTable()->GetOrInternStringWithSpaceType(utf16Data.data(), len, false, type);
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

JSHandle<ProfileTypeInfo> ObjectFactory::NewProfileTypeInfo(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    auto header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject()), size);
    JSHandle<ProfileTypeInfo> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length);

    return array;
}

JSHandle<BigInt> ObjectFactory::NewBigInt(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);
    size_t size = BigInt::ComputeSize(length);
    auto header = heap_->AllocateYoungOrHugeObject(
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
#ifndef NDEBUG
    if (vm_->GetJSOptions().EnableForceGC() && vm_->IsInitialized() && thread_->IsAllContextsInitialized()) {
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
    reactionsFunction->SetMethod(thread_, vm_->GetMethodByIndex(idx));
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
    executorFunction->SetMethod(
        thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_EXECUTOR));
    JSFunction::SetFunctionLength(thread_, function, JSTaggedValue(FunctionLength::TWO));
    return executorFunction;
}

JSHandle<JSPromiseAllResolveElementFunction> ObjectFactory::NewJSPromiseAllResolveElementFunction()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSHClass> hclass = JSHandle<JSHClass>::Cast(env->GetPromiseAllResolveElementFunctionClass());
    JSHandle<JSPromiseAllResolveElementFunction> function =
        JSHandle<JSPromiseAllResolveElementFunction>::Cast(NewJSObject(hclass));
    JSFunction::InitializeJSFunction(thread_, JSHandle<JSFunction>::Cast(function));
    function->SetMethod(
        thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_RESOLVE_ELEMENT_FUNCTION));
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
    function->SetMethod(
        thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_ANY_REJECT_ELEMENT_FUNCTION));
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
    function->SetMethod(thread_,
        vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_ALL_SETTLED_RESOLVE_ELEMENT_FUNCTION));
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
    function->SetMethod(thread_,
        vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_ALL_SETTLED_REJECT_ELEMENT_FUNCTION));
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
    function->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_THEN_FINALLY_FUNCTION));
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
    function->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_CATCH_FINALLY_FUNCTION));
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
    function->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_ASYNC_GENERATOR_NEXT_FULFILLED_FUNCTION));
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
    JSTaggedValue debugVaule =  vm_->GetMethodByIndex(MethodIndex::BUILTINS_ASYNC_FROM_SYNC_ITERATOR_FUNCTION);
    function->SetMethod(thread_, debugVaule);
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
    function->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_ASYNC_GENERATOR_NEXT_REJECTED_FUNCTION));
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
    function->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_VALUE_THUNK_FUNCTION));
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
    function->SetMethod(thread_, vm_->GetMethodByIndex(MethodIndex::BUILTINS_PROMISE_HANDLER_THROWER_FUNCTION));
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
    return stringTable->GetOrInternString(str);
}

JSHandle<TransitionHandler> ObjectFactory::NewTransitionHandler()
{
    NewObjectHook();
    TransitionHandler *handler =
        TransitionHandler::Cast(heap_->AllocateYoungOrHugeObject(
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

JSHandle<StoreTSHandler> ObjectFactory::NewStoreTSHandler()
{
    NewObjectHook();
    StoreTSHandler *header =
        StoreTSHandler::Cast(heap_->AllocateYoungOrHugeObject(
            JSHClass::Cast(thread_->GlobalConstants()->GetStoreTSHandlerClass().GetTaggedObject())));
    JSHandle<StoreTSHandler> handler(thread_, header);
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
    ASSERT(fieldOrder <= PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES);
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
        if (properties->Get(fieldOffset * 2 + 1).IsAccessor()) {  // 2: Meaning to double
            attributes.SetIsAccessor(true);
        }
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::TAGGED);
        attributes.SetOffset(fieldOffset);
        newObjHclass = JSHClass::SetPropertyOfObjHClass(thread_, newObjHclass, key, attributes);
    }
    return newObjHclass;
}

JSHandle<JSHClass> ObjectFactory::GetObjectLiteralHClass(const JSHandle<TaggedArray> &properties, size_t length)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();

    // 64 : If object literal gets too many properties, create hclass directly.
    const int HCLASS_CACHE_SIZE = 64;
    if (length >= HCLASS_CACHE_SIZE) {
        return CreateObjectClass(properties, length);
    }
    JSHandle<JSTaggedValue> maybeCache = env->GetObjectLiteralHClassCache();
    if (maybeCache->IsHole()) {
        JSHandle<TaggedArray> cacheArr = NewTaggedArray(HCLASS_CACHE_SIZE);
        env->SetObjectLiteralHClassCache(thread_, cacheArr.GetTaggedValue());
    }
    JSHandle<JSTaggedValue> hclassCache = env->GetObjectLiteralHClassCache();
    JSHandle<TaggedArray> hclassCacheArr = JSHandle<TaggedArray>::Cast(hclassCache);
    JSTaggedValue maybeHClass = hclassCacheArr->Get(length);
    if (maybeHClass.IsHole()) {
        JSHandle<JSHClass> objHClass = NewEcmaHClass(JSObject::SIZE, JSType::JS_OBJECT, length);
        objHClass->SetPrototype(thread_, proto.GetTaggedValue());
        {
            objHClass->SetNumberOfProps(0);
            objHClass->SetExtensible(true);
        }
        hclassCacheArr->Set(thread_, length, objHClass);
        return SetLayoutInObjHClass(properties, length, objHClass);
    }
    return SetLayoutInObjHClass(properties, length, JSHandle<JSHClass>(thread_, maybeHClass));
}

JSHandle<JSObject> ObjectFactory::NewOldSpaceObjLiteralByHClass(const JSHandle<TaggedArray> &properties, size_t length)
{
    JSHandle<JSHClass> hclass = GetObjectLiteralHClass(properties, length);
    JSHandle<JSObject> obj = NewOldSpaceJSObject(hclass);
    InitializeJSObject(obj, hclass);
    return obj;
}

JSHandle<JSObject> ObjectFactory::NewEmptyJSObject()
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSFunction> builtinObj(env->GetObjectFunction());
    return NewJSObjectByConstructor(builtinObj);
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

JSHandle<MachineCode> ObjectFactory::NewMachineCodeObject(size_t length, const uint8_t *data)
{
    NewObjectHook();
    TaggedObject *obj = heap_->AllocateMachineCodeObject(JSHClass::Cast(
        thread_->GlobalConstants()->GetMachineCodeClass().GetTaggedObject()), length + MachineCode::SIZE);
    MachineCode *code = MachineCode::Cast(obj);
    if (code == nullptr) {
        LOG_FULL(FATAL) << "machine code cast failed";
        UNREACHABLE();
    }
    code->SetInstructionSizeInBytes(static_cast<uint32_t>(length));
    if (data != nullptr) {
        code->SetData(data, length);
    }
    JSHandle<MachineCode> codeObj(thread_, code);
    return codeObj;
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

// ----------------------------------- new TSType ----------------------------------------
JSHandle<TSObjLayoutInfo> ObjectFactory::CreateTSObjLayoutInfo(int propNum, JSTaggedValue initVal)
{
    uint32_t arrayLength = TSObjLayoutInfo::ComputeArrayLength(propNum);
    JSHandle<TSObjLayoutInfo> tsPropInfoHandle = JSHandle<TSObjLayoutInfo>::Cast(NewTaggedArray(arrayLength, initVal));
    tsPropInfoHandle->SetNumOfProperties(thread_, 0);
    return tsPropInfoHandle;
}

JSHandle<TSObjectType> ObjectFactory::NewTSObjectType(uint32_t numOfKeys)
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSObjectTypeClass().GetTaggedObject()));
    JSHandle<TSObjectType> objectType(thread_, header);
    objectType->SetObjLayoutInfo(thread_, JSTaggedValue::Undefined());
    objectType->SetIndexSigns(thread_, JSTaggedValue::Undefined());
    objectType->SetGT(GlobalTSTypeRef::Default());

    JSHandle<TSObjLayoutInfo> tsPropInfo = CreateTSObjLayoutInfo(numOfKeys);
    objectType->SetObjLayoutInfo(thread_, tsPropInfo);
    return objectType;
}

JSHandle<TSClassType> ObjectFactory::NewTSClassType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSClassTypeClass().GetTaggedObject()));
    JSHandle<TSClassType> classType(thread_, header);

    classType->SetGT(GlobalTSTypeRef::Default());
    classType->SetInstanceType(thread_, JSTaggedValue::Undefined());
    classType->SetConstructorType(thread_, JSTaggedValue::Undefined());
    classType->SetPrototypeType(thread_, JSTaggedValue::Undefined());
    classType->SetName(thread_, JSTaggedValue::Undefined());
    classType->SetIndexSigns(thread_, JSTaggedValue::Undefined());
    classType->SetExtensionGT(GlobalTSTypeRef::Default());
    classType->ClearBitField();

    return classType;
}

JSHandle<TSInterfaceType> ObjectFactory::NewTSInterfaceType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSInterfaceTypeClass().GetTaggedObject()));
    JSHandle<TSInterfaceType> interfaceType(thread_, header);

    JSHandle<TaggedArray> extends = EmptyArray();
    interfaceType->SetGT(GlobalTSTypeRef::Default());
    interfaceType->SetExtends(thread_, extends);
    interfaceType->SetFields(thread_, JSTaggedValue::Undefined());
    interfaceType->SetIndexSigns(thread_, JSTaggedValue::Undefined());

    return interfaceType;
}


JSHandle<TSUnionType> ObjectFactory::NewTSUnionType(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSUnionTypeClass().GetTaggedObject()));
    JSHandle<TSUnionType> unionType(thread_, header);

    unionType->SetGT(GlobalTSTypeRef::Default());
    unionType->SetComponents(thread_, JSTaggedValue::Undefined());
    JSHandle<TaggedArray> componentTypes = NewTaggedArray(length, JSTaggedValue::Undefined());
    unionType->SetComponents(thread_, componentTypes);

    return unionType;
}

JSHandle<TSClassInstanceType> ObjectFactory::NewTSClassInstanceType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSClassInstanceTypeClass().GetTaggedObject()));
    JSHandle<TSClassInstanceType> classInstanceType(thread_, header);

    classInstanceType->SetGT(GlobalTSTypeRef::Default());
    classInstanceType->SetClassGT(GlobalTSTypeRef::Default());

    return classInstanceType;
}

JSHandle<TSFunctionType> ObjectFactory::NewTSFunctionType(uint32_t length)
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSFunctionTypeClass().GetTaggedObject()));
    JSHandle<TSFunctionType> functionType(thread_, header);

    functionType->SetGT(GlobalTSTypeRef::Default());
    functionType->SetName(thread_, JSTaggedValue::Undefined());
    functionType->SetParameterTypes(thread_, JSTaggedValue::Undefined());
    functionType->SetReturnGT(GlobalTSTypeRef::Default());
    functionType->SetThisGT(GlobalTSTypeRef::Default());
    functionType->ClearBitField();

    JSHandle<TaggedArray> parameterTypes = NewTaggedArray(length, JSTaggedValue::Undefined());
    functionType->SetParameterTypes(thread_, parameterTypes);

    return functionType;
}

JSHandle<TSArrayType> ObjectFactory::NewTSArrayType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSArrayTypeClass().GetTaggedObject()));

    JSHandle<TSArrayType> arrayType(thread_, header);
    arrayType->SetElementGT(GlobalTSTypeRef::Default());

    return arrayType;
}

JSHandle<TSTypeTable> ObjectFactory::NewTSTypeTable(uint32_t length)
{
    NewObjectHook();

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length + TSTypeTable::RESERVE_TABLE_LENGTH);
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    auto header = heap_->AllocateOldOrHugeObject(arrayClass, size);

    JSHandle<TSTypeTable> table(thread_, header);
    table->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length + TSTypeTable::RESERVE_TABLE_LENGTH);
    table->SetNumberOfTypes(thread_);

    return table;
}

JSHandle<TSModuleTable> ObjectFactory::NewTSModuleTable(uint32_t length)
{
    NewObjectHook();
    ASSERT(length > 0);

    size_t size = TaggedArray::ComputeSize(JSTaggedValue::TaggedTypeSize(), length);
    JSHClass *arrayClass = JSHClass::Cast(thread_->GlobalConstants()->GetArrayClass().GetTaggedObject());
    auto header = heap_->AllocateYoungOrHugeObject(arrayClass, size);
    JSHandle<TSModuleTable> array(thread_, header);
    array->InitializeWithSpecialValue(JSTaggedValue::Undefined(), length);
    array->SetNumberOfTSTypeTables(thread_, 0);

    return array;
}

JSHandle<TSIteratorInstanceType> ObjectFactory::NewTSIteratorInstanceType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSIteratorInstanceTypeClass().GetTaggedObject()));
    JSHandle<TSIteratorInstanceType> iteratorInstanceType(thread_, header);

    iteratorInstanceType->SetGT(GlobalTSTypeRef::Default());
    iteratorInstanceType->SetKindGT(GlobalTSTypeRef::Default());
    iteratorInstanceType->SetElementGT(GlobalTSTypeRef::Default());

    return iteratorInstanceType;
}

JSHandle<TSNamespaceType> ObjectFactory::NewTSNamespaceType()
{
    NewObjectHook();

    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetTSNamespaceTypeClass().GetTaggedObject()));
    JSHandle<TSNamespaceType> namespaceType(thread_, header);

    namespaceType->SetPropertyType(thread_, JSTaggedValue::Undefined());
    JSHandle<TSObjLayoutInfo> tsPropInfo = CreateTSObjLayoutInfo(TSObjLayoutInfo::DEFAULT_CAPACITY);
    namespaceType->SetPropertyType(thread_, tsPropInfo);
    return namespaceType;
}
// ----------------------------------- new string ----------------------------------------
JSHandle<EcmaString> ObjectFactory::NewFromASCII(const CString &data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.c_str());
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf8Data, data.length()));
    return GetStringFromStringTable(utf8Data, data.length(), true);
}

JSHandle<EcmaString> ObjectFactory::NewFromASCIINonMovable(const CString &data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.c_str());
    ASSERT(EcmaStringAccessor::CanBeCompressed(utf8Data, data.length()));
    return GetStringFromStringTableNonMovable(utf8Data, data.length());
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8(const CString &data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.c_str());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, data.length());
    return GetStringFromStringTable(utf8Data, data.length(), canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf16(const CS16tring &data)
{
    uint32_t length = data.length();
    auto utf16Data = reinterpret_cast<const uint16_t *>(data.c_str());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf16Data, length);
    return GetStringFromStringTable(utf16Data, length, canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromStdString(const std::string &data)
{
    auto utf8Data = reinterpret_cast<const uint8_t *>(data.c_str());
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, data.length());
    return GetStringFromStringTable(utf8Data, data.size(), canBeCompress);
}

JSHandle<EcmaString> ObjectFactory::NewFromUtf8(const uint8_t *utf8Data, uint32_t utf8Len)
{
    bool canBeCompress = EcmaStringAccessor::CanBeCompressed(utf8Data, utf8Len);
    return GetStringFromStringTable(utf8Data, utf8Len, canBeCompress);
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
    return JSHandle<EcmaString>(thread_, stringTable->GetOrInternString(firstString, secondString));
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
    return NewImportEntry(defautValue, defautValue, defautValue);
}

JSHandle<ImportEntry> ObjectFactory::NewImportEntry(const JSHandle<JSTaggedValue> &moduleRequest,
                                                    const JSHandle<JSTaggedValue> &importName,
                                                    const JSHandle<JSTaggedValue> &localName)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetImportEntryClass().GetTaggedObject()));
    JSHandle<ImportEntry> obj(thread_, header);
    obj->SetModuleRequest(thread_, moduleRequest);
    obj->SetImportName(thread_, importName);
    obj->SetLocalName(thread_, localName);
    return obj;
}

JSHandle<LocalExportEntry> ObjectFactory::NewLocalExportEntry()
{
    JSHandle<JSTaggedValue> defautValue = thread_->GlobalConstants()->GetHandledUndefined();
    return NewLocalExportEntry(defautValue, defautValue, LocalExportEntry::LOCAL_DEFAULT_INDEX);
}

JSHandle<LocalExportEntry> ObjectFactory::NewLocalExportEntry(const JSHandle<JSTaggedValue> &exportName,
    const JSHandle<JSTaggedValue> &localName, const uint32_t index)
{
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
    return NewIndirectExportEntry(defautValue, defautValue, defautValue);
}

JSHandle<IndirectExportEntry> ObjectFactory::NewIndirectExportEntry(const JSHandle<JSTaggedValue> &exportName,
                                                                    const JSHandle<JSTaggedValue> &moduleRequest,
                                                                    const JSHandle<JSTaggedValue> &importName)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetIndirectExportEntryClass().GetTaggedObject()));
    JSHandle<IndirectExportEntry> obj(thread_, header);
    obj->SetExportName(thread_, exportName);
    obj->SetModuleRequest(thread_, moduleRequest);
    obj->SetImportName(thread_, importName);
    return obj;
}

JSHandle<StarExportEntry> ObjectFactory::NewStarExportEntry()
{
    JSHandle<JSTaggedValue> defautValue = thread_->GlobalConstants()->GetHandledUndefined();
    return NewStarExportEntry(defautValue);
}

JSHandle<StarExportEntry> ObjectFactory::NewStarExportEntry(const JSHandle<JSTaggedValue> &moduleRequest)
{
    NewObjectHook();
    TaggedObject *header = heap_->AllocateYoungOrHugeObject(
        JSHClass::Cast(thread_->GlobalConstants()->GetStarExportEntryClass().GetTaggedObject()));
    JSHandle<StarExportEntry> obj(thread_, header);
    obj->SetModuleRequest(thread_, moduleRequest);
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
    obj->SetEcmaModuleFilename(thread_, undefinedValue);
    obj->SetEcmaModuleRecordName(thread_, undefinedValue);
    obj->SetRequestedModules(thread_, undefinedValue);
    obj->SetImportEntries(thread_, undefinedValue);
    obj->SetLocalExportEntries(thread_, undefinedValue);
    obj->SetIndirectExportEntries(thread_, undefinedValue);
    obj->SetStarExportEntries(thread_, undefinedValue);
    obj->SetNameDictionary(thread_, undefinedValue);
    obj->SetDFSIndex(SourceTextModule::UNDEFINED_INDEX);
    obj->SetDFSAncestorIndex(SourceTextModule::UNDEFINED_INDEX);
    obj->SetEvaluationError(SourceTextModule::UNDEFINED_INDEX);
    obj->SetStatus(ModuleStatus::UNINSTANTIATED);
    obj->SetTypes(ModuleTypes::UNKNOWN);
    obj->SetIsNewBcVersion(false);
    obj->SetRegisterCounts(UINT16_MAX);
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
            if (methodHandle->IsAotWithCallField()) {
                if (methodHandle->IsFastCall()) {
                    hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProtoOptimizedWithFastCall());
                } else {
                    hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProtoOptimized());
                }
            } else {
                hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
            }
            break;
        }
        case FunctionKind::ARROW_FUNCTION: {
            if (methodHandle->IsAotWithCallField()) {
                if (methodHandle->IsFastCall()) {
                    hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProtoOptimizedWithFastCall());
                } else {
                    hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProtoOptimized());
                }
            } else {
                hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
            }
            break;
        }
        case FunctionKind::GENERATOR_FUNCTION: {
            if (methodHandle->IsAotWithCallField()) {
                if (methodHandle->IsFastCall()) {
                    hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClassOptimizedWithFastCall());
                } else {
                    hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClassOptimized());
                }
            } else {
                hclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
            }
            break;
        }
        case FunctionKind::CONCURRENT_FUNCTION:
        case FunctionKind::ASYNC_FUNCTION: {
            if (methodHandle->IsAotWithCallField()) {
                if (methodHandle->IsFastCall()) {
                    hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClassOptimizedWithFastCall());
                } else {
                    hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClassOptimized());
                }
            } else {
                hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
            }
            break;
        }
        case FunctionKind::ASYNC_GENERATOR_FUNCTION: {
            if (methodHandle->IsAotWithCallField()) {
                if (methodHandle->IsFastCall()) {
                    hclass = JSHandle<JSHClass>::Cast(env->GetAsyncGeneratorFunctionClassOptimizedWithFastCall());
                } else {
                    hclass = JSHandle<JSHClass>::Cast(env->GetAsyncGeneratorFunctionClassOptimized());
                }
            } else {
                hclass = JSHandle<JSHClass>::Cast(env->GetAsyncGeneratorFunctionClass());
            }
            break;
        }
        case FunctionKind::ASYNC_ARROW_FUNCTION: {
            if (methodHandle->IsAotWithCallField()) {
                if (methodHandle->IsFastCall()) {
                    hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClassOptimizedWithFastCall());
                } else {
                    hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClassOptimized());
                }
            } else {
                hclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
            }
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
    JSHandle<JSHClass> hclass;
    if (methodHandle->IsAotWithCallField()) {
        if (methodHandle->IsFastCall()) {
            hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProtoOptimizedWithFastCall());
        } else {
            hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProtoOptimized());
        }
    } else {
        hclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    }
    JSHandle<JSFunction> jsFunc = NewJSFunctionByHClass(methodHandle, hclass);
    jsFunc->SetHomeObject(thread_, homeObject);
    ASSERT_NO_ABRUPT_COMPLETION(thread_);
    return jsFunc;
}

JSHandle<JSFunction> ObjectFactory::NewJSFunction(const JSHandle<Method> &methodHandle, FunctionKind kind,
                                                  bool isOptimized, bool canFastCall)
{
    JSHandle<JSHClass> functionClass;
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    if (isOptimized) {
        if (kind == FunctionKind::NORMAL_FUNCTION) {
            if (canFastCall) {
                functionClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProtoOptimizedWithFastCall());
            } else {
                functionClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProtoOptimized());
            }
        } else {
            if (canFastCall) {
                functionClass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClassOptimizedWithFastCall());
            } else {
                functionClass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClassOptimized());
            }
        }
    } else {
        if (kind == FunctionKind::NORMAL_FUNCTION) {
            functionClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
        } else {
            functionClass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
        }
    }
    return NewJSFunctionByHClass(methodHandle, functionClass, MemSpaceType::OLD_SPACE);
}

JSHandle<JSHClass> ObjectFactory::GetNonOptimizedHclass(JSHandle<JSHClass> oldHClassHandle, FunctionKind kind)
{
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> oldHclass(oldHClassHandle);
    switch (kind) {
        case FunctionKind::NORMAL_FUNCTION:
        case FunctionKind::BASE_CONSTRUCTOR: {
            if (oldHclass == env->GetFunctionClassWithProtoOptimizedWithFastCall() ||
                oldHclass == env->GetFunctionClassWithProtoOptimized()) {
                return JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
            } else if (oldHclass == env->GetFunctionClassWithoutProtoOptimizedWithFastCall() ||
                       oldHclass == env->GetFunctionClassWithoutProtoOptimized()) {
                return JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
            }
            break;
        }
        case FunctionKind::ARROW_FUNCTION: {
            if (oldHclass == env->GetFunctionClassWithoutProtoOptimizedWithFastCall() ||
                oldHclass == env->GetFunctionClassWithoutProtoOptimized()) {
                return JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
            }
            break;
        }
        case FunctionKind::GENERATOR_FUNCTION: {
            if (oldHclass == env->GetGeneratorFunctionClassOptimizedWithFastCall() ||
                oldHclass == env->GetGeneratorFunctionClassOptimized()) {
                return JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
            }
            break;
        }
        case FunctionKind::CONCURRENT_FUNCTION:
        case FunctionKind::ASYNC_FUNCTION: {
            if (oldHclass == env->GetAsyncFunctionClassOptimizedWithFastCall() ||
                oldHclass == env->GetAsyncFunctionClassOptimized()) {
                return JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
            }
            break;
        }
        case FunctionKind::ASYNC_GENERATOR_FUNCTION: {
            if (oldHclass == env->GetAsyncGeneratorFunctionClassOptimizedWithFastCall() ||
                oldHclass == env->GetAsyncGeneratorFunctionClassOptimized()) {
                return JSHandle<JSHClass>::Cast(env->GetAsyncGeneratorFunctionClass());
            }
            break;
        }
        case FunctionKind::ASYNC_ARROW_FUNCTION: {
            if (oldHclass == env->GetAsyncFunctionClassOptimizedWithFastCall() ||
                oldHclass == env->GetAsyncFunctionClassOptimized()) {
                return JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
            }
            break;
        }
        default:
            break;
    }
    // hclass isn't initialized hclass, so can't clear optimizedFlags directly
    JSHandle<JSHClass> newHClass = JSHClass::Clone(thread_, oldHClassHandle);
    newHClass->ClearOptimizedFlags();
    return newHClass;
}
}  // namespace panda::ecmascript
