/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/serializer/value_serializer.h"

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/base/array_helper.h"

namespace panda::ecmascript {

bool ValueSerializer::CheckObjectCanSerialize(TaggedObject *object, bool &findSharedObject)
{
    JSType type = object->GetClass()->GetObjectType();
    if (IsInternalJSType(type)) {
        return true;
    }
    switch (type) {
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
        case JSType::JS_DATE:
        case JSType::JS_ARRAY:
        case JSType::JS_MAP:
        case JSType::JS_SET:
        case JSType::JS_REG_EXP:
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
        case JSType::JS_ARRAY_BUFFER:
        case JSType::JS_SHARED_ARRAY_BUFFER:
        case JSType::LINE_STRING:
        case JSType::CONSTANT_STRING:
        case JSType::TREE_STRING:
        case JSType::SLICED_STRING:
        case JSType::JS_OBJECT:
        case JSType::JS_ASYNC_FUNCTION:  // means CONCURRENT_FUNCTION
            return true;
        case JSType::JS_SHARED_SET:
        case JSType::JS_SHARED_MAP:
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
        case JSType::JS_SHARED_ARRAY:
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
        case JSType::JS_SHARED_OBJECT:
        case JSType::JS_SHARED_FUNCTION:
        case JSType::JS_SHARED_ASYNC_FUNCTION: {
            if (serializeSharedEvent_ > 0) {
                return true;
            }
            if (defaultCloneShared_ || cloneSharedSet_.find(ToUintPtr(object)) != cloneSharedSet_.end()) {
                findSharedObject = true;
                serializeSharedEvent_++;
            }
            return true;
        }
        default:
            break;
    }
    LOG_ECMA(ERROR) << "Unsupport serialize object type: " << JSHClass::DumpJSType(type);
    return false;
}

bool ValueSerializer::WriteValue(JSThread *thread,
                                 const JSHandle<JSTaggedValue> &value,
                                 const JSHandle<JSTaggedValue> &transfer,
                                 const JSHandle<JSTaggedValue> &cloneList)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "ValueSerializer::WriteValue");
    ASSERT(!value->IsWeak());
    if (!defaultTransfer_ && !PrepareTransfer(thread, transfer)) {
        LOG_ECMA(ERROR) << "ValueSerialize: PrepareTransfer fail";
        data_->SetIncompleteData(true);
        return false;
    }
    if (!defaultCloneShared_ && !PrepareClone(thread, cloneList)) {
        LOG_ECMA(ERROR) << "ValueSerialize: PrepareClone fail";
        data_->SetIncompleteData(true);
        return false;
    }
    SerializeJSTaggedValue(value.GetTaggedValue());
    // ThreadNativeScope may trigger moving gc, so PushSerializationRoot must do before native state.
    // Push share root object to runtime map
    uint32_t index = data_->GetDataIndex();
    bool chunkEmpty = sharedObjChunk_->Empty();
    if (!chunkEmpty) {
        index = Runtime::GetInstance()->PushSerializationRoot(thread_, std::move(sharedObjChunk_));
    }
    {
        ThreadNativeScope nativeScope(thread);
        for (auto &entry : detachCallbackInfo_) {
            auto info = entry.second;
            DetachFunc detachNative = reinterpret_cast<DetachFunc>(info->detachFunc);
            if (detachNative == nullptr || entry.first < 0) {
                LOG_ECMA(ERROR) << "ValueSerialize: SerializeNativeBindingObject detachNative == nullptr";
                notSupport_ = true;
                break;
            }
            void *buffer = detachNative(info->env, info->nativeValue, info->hint, info->detachData);
            data_->EmitU64(reinterpret_cast<uint64_t>(buffer), static_cast<size_t>(entry.first));
        }
    }
    if (notSupport_) {
        LOG_ECMA(ERROR) << "ValueSerialize: serialize data is incomplete";
        data_->SetIncompleteData(true);
        if (!chunkEmpty) {
            // If notSupport, serializeRoot should be removed.
            Runtime::GetInstance()->RemoveSerializationRoot(thread_, index);
        }
        return false;
    }
    if (!chunkEmpty) {
        data_->SetDataIndex(index);
    }
    size_t maxSerializerSize = vm_->GetEcmaParamConfiguration().GetMaxJSSerializerSize();
    if (data_->Size() > maxSerializerSize) {
        LOG_ECMA(ERROR) << "The serialization data size has exceed limit Size, current size is: " << data_->Size()
                        << " max size is: " << maxSerializerSize;
        return false;
    }
    return true;
}

void ValueSerializer::SerializeObjectImpl(TaggedObject *object, bool isWeak)
{
    if (notSupport_) {
        return;
    }
    bool cloneSharedObject = false;
    if (!CheckObjectCanSerialize(object, cloneSharedObject)) {
        notSupport_ = true;
        return;
    }
    if (isWeak) {
        data_->WriteEncodeFlag(EncodeFlag::WEAK);
    }
    if (SerializeReference(object) || SerializeRootObject(object)) {
        return;
    }
    Region *region = Region::ObjectAddressToRange(object);
    if (object->GetClass()->IsString() || object->GetClass()->IsMethod() || region->InSharedReadOnlySpace() ||
        (serializeSharedEvent_ == 0 && region->InSharedHeap())) {
        SerializeSharedObject(object);
        return;
    }
    if (object->GetClass()->IsNativeBindingObject()) {
        SerializeNativeBindingObject(object);
        return;
    }
    if (object->GetClass()->IsJSError()) {
        SerializeJSError(object);
        return;
    }
    bool arrayBufferDeferDetach = false;
    JSTaggedValue trackInfo;
    JSTaggedType hashfield = JSTaggedValue::VALUE_ZERO;
    JSType type = object->GetClass()->GetObjectType();
    // serialize prologue
    switch (type) {
        case JSType::JS_ARRAY_BUFFER: {
            supportJSNativePointer_ = true;
            arrayBufferDeferDetach = SerializeJSArrayBufferPrologue(object);
            break;
        }
        case JSType::JS_SHARED_ARRAY_BUFFER: {
            supportJSNativePointer_ = true;
            SerializeJSSharedArrayBufferPrologue(object);
            break;
        }
        case JSType::JS_SENDABLE_ARRAY_BUFFER: {
            supportJSNativePointer_ = true;
            SerializeJSSendableArrayBufferPrologue(object);
            break;
        }
        case JSType::JS_ARRAY: {
            JSArray *array = reinterpret_cast<JSArray *>(object);
            trackInfo = array->GetTrackInfo();
            array->SetTrackInfo(thread_, JSTaggedValue::Undefined());
            break;
        }
        case JSType::JS_REG_EXP: {
            supportJSNativePointer_ = true;
            SerializeJSRegExpPrologue(reinterpret_cast<JSRegExp *>(object));
            break;
        }
        case JSType::JS_OBJECT: {
            hashfield = Barriers::GetValue<JSTaggedType>(object, JSObject::HASH_OFFSET);
            Barriers::SetPrimitive<JSTaggedType>(object, JSObject::HASH_OFFSET, JSTaggedValue::VALUE_ZERO);
            break;
        }
        default:
            break;
    }

    // serialize object here
    SerializeTaggedObject<SerializeType::VALUE_SERIALIZE>(object);

    // serialize epilogue
    switch (type) {
        case JSType::JS_ARRAY_BUFFER:
        case JSType::JS_SHARED_ARRAY_BUFFER:
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
        case JSType::JS_REG_EXP:
            // JSNativePointer supports serialization only during serialize JSArrayBuffer,
            // JSSharedArrayBuffer and JSRegExp
            supportJSNativePointer_ = false;
            break;
        case JSType::JS_ARRAY: {
            JSArray *array = reinterpret_cast<JSArray *>(object);
            array->SetTrackInfo(thread_, trackInfo);
            break;
        }
        case JSType::JS_OBJECT: {
            if (JSTaggedValue(hashfield).IsHeapObject()) {
                Barriers::SetObject<true>(thread_, object, JSObject::HASH_OFFSET, hashfield);
            } else {
                Barriers::SetPrimitive<JSTaggedType>(object, JSObject::HASH_OFFSET, hashfield);
            }
            break;
        }
        default:
            break;
    }
    if (cloneSharedObject) {
        serializeSharedEvent_--;
    }
    if (arrayBufferDeferDetach) {
        ASSERT(object->GetClass()->IsArrayBuffer());
        JSArrayBuffer *arrayBuffer = reinterpret_cast<JSArrayBuffer *>(object);
        arrayBuffer->Detach(thread_, arrayBuffer->GetWithNativeAreaAllocator(), true);
    }
}

void ValueSerializer::SerializeJSError(TaggedObject *object)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    data_->WriteEncodeFlag(EncodeFlag::JS_ERROR);
    JSType type = object->GetClass()->GetObjectType();
    ASSERT(type >= JSType::JS_ERROR_FIRST && type <= JSType::JS_ERROR_LAST);
    data_->WriteUint8(static_cast<uint8_t>(type));
    auto globalConst = thread_->GlobalConstants();
    JSHandle<JSTaggedValue> handleMsg = globalConst->GetHandledMessageString();
    JSHandle<JSTaggedValue> msg =
        JSObject::GetProperty(thread_, JSHandle<JSTaggedValue>(thread_, object), handleMsg).GetValue();
    if (msg->IsString()) {
        data_->WriteUint8(1); // 1: msg is string
        // string must be shared
        SerializeSharedObject(msg->GetTaggedObject());
    } else {
        data_->WriteUint8(0); // 0: msg is undefined
    }
}

void ValueSerializer::SerializeNativeBindingObject(TaggedObject *object)
{
    [[maybe_unused]] EcmaHandleScope scope(thread_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    JSHandle<JSTaggedValue> nativeBindingSymbol = env->GetNativeBindingSymbol();
    JSHandle<JSTaggedValue> nativeBindingValue =
        JSObject::GetProperty(thread_, JSHandle<JSObject>(thread_, object), nativeBindingSymbol).GetRawValue();
    if (!nativeBindingValue->IsJSNativePointer()) {
        LOG_ECMA(ERROR) << "ValueSerialize: SerializeNativeBindingObject nativeBindingValue is not JSNativePointer";
        notSupport_ = true;
        return;
    }
    auto info = reinterpret_cast<panda::JSNApi::NativeBindingInfo *>(
        JSNativePointer::Cast(nativeBindingValue->GetTaggedObject())->GetExternalPointer());
    if (info == nullptr) {
        LOG_ECMA(ERROR) << "ValueSerialize: SerializeNativeBindingObject NativeBindingInfo is nullptr";
        notSupport_ = true;
        return;
    }
    void *hint = info->hint;
    void *attachData = info->attachData;
    AttachFunc attachNative = reinterpret_cast<AttachFunc>(info->attachFunc);
    data_->WriteEncodeFlag(EncodeFlag::NATIVE_BINDING_OBJECT);
    data_->WriteJSTaggedType(reinterpret_cast<JSTaggedType>(attachNative));
    ssize_t offset = data_->EmitU64(0); // 0 is a placeholder which will be filled later
    detachCallbackInfo_.push_back({offset, info});
    data_->WriteJSTaggedType(reinterpret_cast<JSTaggedType>(hint));
    data_->WriteJSTaggedType(reinterpret_cast<JSTaggedType>(attachData));
}

bool ValueSerializer::SerializeJSArrayBufferPrologue(TaggedObject *object)
{
    ASSERT(object->GetClass()->IsArrayBuffer());
    JSArrayBuffer *arrayBuffer = reinterpret_cast<JSArrayBuffer *>(object);
    if (arrayBuffer->IsDetach()) {
        LOG_ECMA(ERROR) << "ValueSerialize: don't support serialize detached array buffer";
        notSupport_ = true;
        return false;
    }
    bool transfer = transferDataSet_.find(ToUintPtr(object)) != transferDataSet_.end();
    bool clone = cloneArrayBufferSet_.find(ToUintPtr(object)) != cloneArrayBufferSet_.end();
    size_t arrayLength = arrayBuffer->GetArrayBufferByteLength();
    if (arrayLength > 0) {
        if (transfer) {
            if (clone) {
                notSupport_ = true;
                LOG_ECMA(ERROR) << "ValueSerialize: can't put arraybuffer in both transfer list and clone list";
                return false;
            }
            data_->WriteEncodeFlag(EncodeFlag::TRANSFER_ARRAY_BUFFER);
            return true;
        } else if (clone || !defaultTransfer_) {
            bool nativeAreaAllocated = arrayBuffer->GetWithNativeAreaAllocator();
            if (!nativeAreaAllocated) {
                LOG_ECMA(ERROR) << "ValueSerialize: don't support clone arraybuffer has external allocated buffer, \
                    considering transfer it";
                notSupport_ = true;
                return false;
            }
            data_->WriteEncodeFlag(EncodeFlag::ARRAY_BUFFER);
            data_->WriteUint32(arrayLength);
            JSNativePointer *np =
                reinterpret_cast<JSNativePointer *>(arrayBuffer->GetArrayBufferData().GetTaggedObject());
            data_->WriteRawData(static_cast<uint8_t *>(np->GetExternalPointer()), arrayLength);
            return false;
        } else {
            data_->WriteEncodeFlag(EncodeFlag::TRANSFER_ARRAY_BUFFER);
            return true;
        }
    }
    return false;
}

void ValueSerializer::SerializeJSSharedArrayBufferPrologue(TaggedObject *object)
{
    ASSERT(object->GetClass()->IsSharedArrayBuffer());
    JSArrayBuffer *arrayBuffer = reinterpret_cast<JSArrayBuffer *>(object);
    bool transfer = transferDataSet_.find(ToUintPtr(object)) != transferDataSet_.end();
    if (arrayBuffer->IsDetach() || transfer) {
        LOG_ECMA(ERROR) << "ValueSerialize: don't support serialize detached or transfer shared array buffer";
        notSupport_ = true;
        return;
    }
    size_t arrayLength = arrayBuffer->GetArrayBufferByteLength();
    if (arrayLength > 0) {
        JSNativePointer *np = reinterpret_cast<JSNativePointer *>(arrayBuffer->GetArrayBufferData().GetTaggedObject());
        void *buffer = np->GetExternalPointer();
        if (JSSharedMemoryManager::GetInstance()->CreateOrLoad(&buffer, arrayLength)) {
            LOG_ECMA(ERROR) << "ValueSerialize: can't find buffer form shared memory pool";
            notSupport_ = true;
            return;
        }
        data_->WriteEncodeFlag(EncodeFlag::SHARED_ARRAY_BUFFER);
        data_->insertSharedArrayBuffer(reinterpret_cast<uintptr_t>(buffer));
    }
}

void ValueSerializer::SerializeJSSendableArrayBufferPrologue(TaggedObject *object)
{
    ASSERT(object->GetClass()->IsSendableArrayBuffer());
    JSSendableArrayBuffer *arrayBuffer = reinterpret_cast<JSSendableArrayBuffer *>(object);
    if (arrayBuffer->IsDetach()) {
        LOG_ECMA(ERROR) << "ValueSerialize: don't support serialize detached sendable array buffer";
        notSupport_ = true;
        return;
    }
    size_t arrayLength = arrayBuffer->GetArrayBufferByteLength();
    if (arrayLength > 0) {
        bool nativeAreaAllocated = arrayBuffer->GetWithNativeAreaAllocator();
        if (!nativeAreaAllocated) {
            LOG_ECMA(ERROR) << "ValueSerialize: don't support clone sendablearraybuffer has external allocated buffer";
            notSupport_ = true;
            return;
        }
        data_->WriteEncodeFlag(EncodeFlag::SENDABLE_ARRAY_BUFFER);
        data_->WriteUint32(arrayLength);
        JSNativePointer *np =
            reinterpret_cast<JSNativePointer *>(arrayBuffer->GetArrayBufferData().GetTaggedObject());
        data_->WriteRawData(static_cast<uint8_t *>(np->GetExternalPointer()), arrayLength);
    }
}

void ValueSerializer::SerializeJSRegExpPrologue(JSRegExp *jsRegExp)
{
    uint32_t bufferSize = jsRegExp->GetLength();
    if (bufferSize == 0) {
        LOG_ECMA(ERROR) << "ValueSerialize: JSRegExp buffer size is 0";
        notSupport_ = true;
        return;
    }

    data_->WriteEncodeFlag(EncodeFlag::JS_REG_EXP);
    data_->WriteUint32(bufferSize);
    JSNativePointer *np =
        reinterpret_cast<JSNativePointer *>(jsRegExp->GetByteCodeBuffer().GetTaggedObject());
    data_->WriteRawData(static_cast<uint8_t *>(np->GetExternalPointer()), bufferSize);
}

bool ValueSerializer::PrepareTransfer(JSThread *thread, const JSHandle<JSTaggedValue> &transfer)
{
    if (transfer->IsUndefined()) {
        return true;
    }
    if (!transfer->IsJSArray()) {
        return false;
    }
    int len = base::ArrayHelper::GetArrayLength(thread, transfer);
    int k = 0;
    while (k < len) {
        bool exists = JSTaggedValue::HasProperty(thread, transfer, k);
        if (exists) {
            JSHandle<JSTaggedValue> element = JSArray::FastGetPropertyByValue(thread, transfer, k);
            if (!element->IsArrayBuffer()) {
                return false;
            }
            transferDataSet_.insert(static_cast<uintptr_t>(element.GetTaggedType()));
        }
        k++;
    }
    return true;
}

bool ValueSerializer::PrepareClone(JSThread *thread, const JSHandle<JSTaggedValue> &cloneList)
{
    if (cloneList->IsUndefined()) {
        return true;
    }
    if (!cloneList->IsJSArray()) {
        return false;
    }
    int len = base::ArrayHelper::GetArrayLength(thread, cloneList);
    int index = 0;
    while (index < len) {
        bool exists = JSTaggedValue::HasProperty(thread, cloneList, index);
        if (exists) {
            JSHandle<JSTaggedValue> element = JSArray::FastGetPropertyByValue(thread, cloneList, index);
            if (element->IsArrayBuffer()) {
                cloneArrayBufferSet_.insert(static_cast<uintptr_t>(element.GetTaggedType()));
            } else if (element->IsJSShared()) {
                cloneSharedSet_.insert(static_cast<uintptr_t>(element.GetTaggedType()));
            } else {
                return false;
            }
        }
        index++;
    }
    return true;
}
}  // namespace panda::ecmascript

