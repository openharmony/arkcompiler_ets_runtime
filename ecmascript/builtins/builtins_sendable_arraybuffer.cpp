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

#include "ecmascript/builtins/builtins_sendable_arraybuffer.h"


#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/base/typed_array_helper-inl.h"

namespace panda::ecmascript::builtins {
using TypedArrayHelper = base::TypedArrayHelper;
using ContainerError = containers::ContainerError;
// 24.1.2.1 ArrayBuffer(length)
JSTaggedValue BuiltinsSendableArrayBuffer::ArrayBufferConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SendableArrayBuffer, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    // 1. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, containers::ErrorFlag::IS_NULL_ERROR,
            "The ArkTS ArrayBuffer's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> lengthHandle = GetCallArg(argv, 0);
    JSTaggedNumber lenNum = JSTaggedValue::ToIndex(thread, lengthHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint64_t length = lenNum.GetNumber();
    return AllocateSendableArrayBuffer(thread, newTarget, length);
}

// 24.1.3.1 ArrayBuffer.isView(arg)
JSTaggedValue BuiltinsSendableArrayBuffer::IsView(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SendableArrayBuffer, IsView);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> arg = GetCallArg(argv, 0);
    // 1. If Type(arg) is not Object, return false.
    if (!arg->IsECMAObject()) {
        return BuiltinsSendableArrayBuffer::GetTaggedBoolean(false);
    }
    // 2. If arg has a [[ViewedArrayBuffer]] internal slot, return true.
    if (arg->IsDataView() || arg->IsSharedTypedArray()) {
        return BuiltinsSendableArrayBuffer::GetTaggedBoolean(true);
    }
    // 3. Return false.
    return BuiltinsSendableArrayBuffer::GetTaggedBoolean(false);
}

// 24.1.3.3 get ArrayBuffer [ @@species ]
JSTaggedValue BuiltinsSendableArrayBuffer::Species(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), SendableArrayBuffer, Species);
    return GetThis(argv).GetTaggedValue();
}

// 24.1.4.1 get ArrayBuffer.prototype.byteLength
JSTaggedValue BuiltinsSendableArrayBuffer::GetByteLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SendableArrayBuffer, GetByteLength);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an object", JSTaggedValue::Exception());
    }
    // 3. If O does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSendableArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have internal slot", JSTaggedValue::Exception());
    }
    // 4. If IsDetachedBuffer(O) is true, throw a TypeError exception.
    if (IsDetachedBuffer(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "IsDetachedBuffer", JSTaggedValue::Exception());
    }
    JSHandle<JSSendableArrayBuffer> arrBuf(thisHandle);
    // 5. Let length be the value of O’s [[ArrayBufferByteLength]] internal slot.
    uint32_t length = arrBuf->GetArrayBufferByteLength();
    // 6. Return length.
    return JSTaggedValue(length);
}

// 24.1.4.3 ArrayBuffer.prototype.slice(start, end)
JSTaggedValue BuiltinsSendableArrayBuffer::Slice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, SendableArrayBuffer, Slice);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsHeapObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an object", JSTaggedValue::Exception());
    }
    // 3. If O does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsSendableArrayBuffer()) {
        auto error = ContainerError::BusinessError(thread, containers::ErrorFlag::BIND_ERROR,
                                                   "The slice method cannot be bound.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    // 4. If IsDetachedBuffer(O) is true, throw a TypeError exception.
    if (IsDetachedBuffer(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value IsDetachedBuffer", JSTaggedValue::Exception());
    }
    JSHandle<JSSendableArrayBuffer> arrBuf(thisHandle);
    // 5. Let len be the value of O’s [[ArrayBufferByteLength]] internal slot.
    int32_t len = static_cast<int32_t>(arrBuf->GetArrayBufferByteLength());
    JSHandle<JSTaggedValue> startHandle = GetCallArg(argv, 0);
    // 6. Let relativeStart be ToInteger(start).
    JSTaggedNumber relativeStart = JSTaggedValue::ToInteger(thread, startHandle);
    // 7. ReturnIfAbrupt(relativeStart).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    int32_t start = base::NumberHelper::DoubleInRangeInt32(relativeStart.GetNumber());
    int32_t end = 0;
    int32_t first = 0;
    int32_t last = 0;
    // 8. If relativeStart < 0, let first be max((len + relativeStart),0); else let first be min(relativeStart, len).
    if (start < 0) {
        first = std::max((len + start), 0);
    } else {
        first = std::min(start, len);
    }
    // 9. If end is undefined, let relativeEnd be len; else let relativeEnd be ToInteger(end).
    JSHandle<JSTaggedValue> endHandle = GetCallArg(argv, 1);
    if (endHandle->IsUndefined()) {
        end = len;
    } else {
        JSTaggedNumber relativeEnd = JSTaggedValue::ToInteger(thread, endHandle);
        // 10. ReturnIfAbrupt(relativeEnd).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        end = base::NumberHelper::DoubleInRangeInt32(relativeEnd.GetNumber());
    }
    // 11. If relativeEnd < 0, let final be max((len + relativeEnd),0); else let final be min(relativeEnd, len).
    if (end < 0) {
        last = std::max((len + end), 0);
    } else {
        last = std::min(end, len);
    }
    // 12. Let newLen be max(final-first,0).
    uint32_t newLen = std::max((last - first), 0);
    // 13. Let ctor be SpeciesConstructor(O, %ArrayBuffer%).
    JSHandle<JSTaggedValue> defaultConstructor = env->GetSBuiltininArrayBufferFunction();
    JSHandle<JSObject> objHandle(thisHandle);
    JSHandle<JSTaggedValue> constructor = JSObject::SpeciesConstructor(thread, objHandle, defaultConstructor);
    // 14. ReturnIfAbrupt(ctor).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 15. Let new be Construct(ctor, «newLen»).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 1);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    info->SetCallArg(JSTaggedValue(newLen));
    JSTaggedValue taggedNewArrBuf = JSFunction::Construct(info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> newArrBuf(thread, taggedNewArrBuf);
    // 16. ReturnIfAbrupt(new).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 17. If new does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!newArrBuf->IsSendableArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have bufferdata internal slot", JSTaggedValue::Exception());
    }
    // 18. If IsDetachedBuffer(new) is true, throw a TypeError exception.
    if (IsDetachedBuffer(newArrBuf.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new arrayBuffer IsDetachedBuffer", JSTaggedValue::Exception());
    }
    // 19. If SameValue(new, O) is true, throw a TypeError exception.
    if (JSTaggedValue::SameValue(newArrBuf.GetTaggedValue(), thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "value of new arraybuffer and this is same", JSTaggedValue::Exception());
    }
    JSHandle<JSSendableArrayBuffer> newJsArrBuf(newArrBuf);
    // 20. If the value of new’s [[ArrayBufferByteLength]] internal slot < newLen, throw a TypeError exception.
    uint32_t newArrBufLen = newJsArrBuf->GetArrayBufferByteLength();
    if (newArrBufLen < newLen) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new array buffer length smaller than newlen", JSTaggedValue::Exception());
    }
    // 21. NOTE: Side-effects of the above steps may have detached O.
    // 22. If IsDetachedBuffer(O) is true, throw a TypeError exception.
    if (IsDetachedBuffer(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value IsDetachedBuffer", JSTaggedValue::Exception());
    }
    if (newLen > 0) {
        // 23. Let fromBuf be the value of O’s [[ArrayBufferData]] internal slot.
        void *fromBuf = GetDataPointFromBuffer(arrBuf.GetTaggedValue());
        // 24. Let toBuf be the value of new’s [[ArrayBufferData]] internal slot.
        void *toBuf = GetDataPointFromBuffer(newJsArrBuf.GetTaggedValue());
        // 25. Perform CopyDataBlockBytes(toBuf, fromBuf, first, newLen).
        JSSendableArrayBuffer::CopyDataPointBytes(toBuf, fromBuf, first, newLen);
    }
    // Return new.
    return newArrBuf.GetTaggedValue();
}

// 24.1.1.1 AllocateArrayBuffer(constructor, byteLength)
JSTaggedValue BuiltinsSendableArrayBuffer::AllocateSendableArrayBuffer(
    JSThread *thread, const JSHandle<JSTaggedValue> &newTarget, uint64_t byteLength)
{
    BUILTINS_API_TRACE(thread, SendableArrayBuffer, AllocateSendableArrayBuffer);
    /**
     * 1. Let obj be OrdinaryCreateFromConstructor(constructor, "%ArrayBufferPrototype%",
     * «[[ArrayBufferData]], [[ArrayBufferByteLength]]» ).
     * */
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> arrBufFunc = env->GetSBuiltininArrayBufferFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(arrBufFunc), newTarget);
    // 2. ReturnIfAbrupt
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    ASSERT(obj.GetTaggedValue().IsInSharedHeap());
    // 4. Let block be CreateByteDataBlock(byteLength).
    if (byteLength > INT_MAX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Out of range", JSTaggedValue::Exception());
    }
    uint32_t arrayByteLength = static_cast<uint32_t>(byteLength);
    JSHandle<JSSendableArrayBuffer> arrayBuffer(obj);
    // 6. Set obj’s [[ArrayBufferData]] internal slot to block.
    factory->NewJSSendableArrayBufferData(arrayBuffer, arrayByteLength);
    // 7. Set obj’s [[ArrayBufferByteLength]] internal slot to byteLength.
    arrayBuffer->SetArrayBufferByteLength(arrayByteLength);
    // 8. Return obj.
    return arrayBuffer.GetTaggedValue();
}

// 24.1.1.2 IsDetachedBuffer()
bool BuiltinsSendableArrayBuffer::IsDetachedBuffer(JSTaggedValue arrayBuffer)
{
    if (arrayBuffer.IsByteArray()) {
        return false;
    }
    // 1. Assert: Type(arrayBuffer) is Object and it has an [[ArrayBufferData]] internal slot.
    ASSERT(arrayBuffer.IsSendableArrayBuffer());
    JSSendableArrayBuffer *buffer = JSSendableArrayBuffer::Cast(arrayBuffer.GetTaggedObject());
    JSTaggedValue dataSlot = buffer->GetArrayBufferData();
    // 2. If arrayBuffer’s [[ArrayBufferData]] internal slot is null, return true.
    // 3. Return false.
    return dataSlot.IsNull();
}

// 24.1.1.4
JSTaggedValue BuiltinsSendableArrayBuffer::CloneArrayBuffer(JSThread *thread,
                                                            const JSHandle<JSTaggedValue> &srcBuffer,
                                                            uint32_t srcByteOffset,
                                                            JSHandle<JSTaggedValue> constructor)
{
    BUILTINS_API_TRACE(thread, SendableArrayBuffer, CloneArrayBuffer);
    // 1. Assert: Type(srcBuffer) is Object and it has an [[ArrayBufferData]] internal slot.
    ASSERT(srcBuffer->IsSendableArrayBuffer()|| srcBuffer->IsByteArray());
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 2. If cloneConstructor is not present
    if (constructor->IsUndefined()) {
        // a. Let cloneConstructor be SpeciesConstructor(srcBuffer, %ArrayBuffer%).
        JSHandle<JSTaggedValue> defaultConstructor = env->GetSBuiltininArrayBufferFunction();
        JSHandle<JSObject> objHandle(srcBuffer);
        constructor = JSObject::SpeciesConstructor(thread, objHandle, defaultConstructor);
        // b. ReturnIfAbrupt(cloneConstructor).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // c. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
        if (IsDetachedBuffer(srcBuffer.GetTaggedValue())) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
        } else {
            ASSERT(constructor->IsConstructor());
        }
    }
    // 4. Let srcLength be the value of srcBuffer’s [[ArrayBufferByteLength]] internal slot.
    uint32_t srcLen = 0;
    int32_t cloneLen = 0;
    if (srcBuffer->IsByteArray()) {
        JSHandle<ByteArray> byteArrayBuf(srcBuffer);
        srcLen = byteArrayBuf->GetArrayLength();
        int32_t byteLen = static_cast<int32_t>(byteArrayBuf->GetByteLength());
        // 5. Assert: srcByteOffset ≤ srcLength.
        ASSERT(srcByteOffset <= srcLen);
        // 6. Let cloneLength be (srcLength – srcByteOffset) * byteLen.
        cloneLen = static_cast<int32_t>(srcLen - srcByteOffset) * byteLen;
        srcByteOffset *= static_cast<uint32_t>(byteLen);
    } else {
        JSHandle<JSSendableArrayBuffer> arrBuf(srcBuffer);
        srcLen = arrBuf->GetArrayBufferByteLength();
        // 5. Assert: srcByteOffset ≤ srcLength.
        ASSERT(srcByteOffset <= srcLen);
        // 6. Let cloneLength be srcLength – srcByteOffset.
        cloneLen = static_cast<int32_t>(srcLen - srcByteOffset);
    }

    // 8. Let targetBuffer be AllocateArrayBuffer(cloneConstructor, cloneLength).
    JSTaggedValue taggedBuf = AllocateSendableArrayBuffer(thread, constructor, cloneLen);
    // 9. ReturnIfAbrupt(targetBuffer).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 10. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
    if (IsDetachedBuffer(srcBuffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
    }
    // 11. Let targetBlock be the value of targetBuffer’s [[ArrayBufferData]] internal slot.
    JSHandle<JSSendableArrayBuffer> newArrBuf(thread, taggedBuf);
    // Perform CopyDataBlockBytes(targetBlock, 0, srcBlock, srcByteOffset, cloneLength).
    // 7. Let srcBlock be the value of srcBuffer’s [[ArrayBufferData]] internal slot.
    void *fromBuf = GetDataPointFromBuffer(srcBuffer.GetTaggedValue());
    void *toBuf = GetDataPointFromBuffer(taggedBuf);
    if (cloneLen > 0) {
        JSSendableArrayBuffer::CopyDataPointBytes(toBuf, fromBuf, srcByteOffset, cloneLen);
    }
    return taggedBuf;
}

void *BuiltinsSendableArrayBuffer::GetDataPointFromBuffer(JSTaggedValue arrBuf, uint32_t byteOffset)
{
    if (arrBuf.IsByteArray()) {
        return reinterpret_cast<void *>(ToUintPtr(ByteArray::Cast(arrBuf.GetTaggedObject())->GetData()) + byteOffset);
    }

    JSSendableArrayBuffer *arrayBuffer = JSSendableArrayBuffer::Cast(arrBuf.GetTaggedObject());
    if (arrayBuffer->GetArrayBufferByteLength() == 0) {
        return nullptr;
    }

    JSTaggedValue data = arrayBuffer->GetArrayBufferData();
    return reinterpret_cast<void *>(ToUintPtr(JSNativePointer::Cast(data.GetTaggedObject())
                                    ->GetExternalPointer()) + byteOffset);
}
}  // namespace panda::ecmascript::builtins
