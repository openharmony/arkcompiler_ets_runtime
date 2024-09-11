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

#include "ecmascript/builtins/builtins_arraybuffer.h"

#include <typeinfo>

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/base/typed_array_helper-inl.h"

#include "cstdio"
#include "cstring"

namespace panda::ecmascript::builtins {
using TypedArrayHelper = base::TypedArrayHelper;
// 24.1.2.1 ArrayBuffer(length)
JSTaggedValue BuiltinsArrayBuffer::ArrayBufferConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, ArrayBuffer, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    // 1. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newtarget is undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> lengthHandle = GetCallArg(argv, 0);
    JSTaggedNumber lenNum = JSTaggedValue::ToIndex(thread, lengthHandle);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint64_t length = lenNum.GetNumber();
    return AllocateArrayBuffer(thread, newTarget, length);
}

// 24.1.3.1 ArrayBuffer.isView(arg)
JSTaggedValue BuiltinsArrayBuffer::IsView(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, ArrayBuffer, IsView);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> arg = GetCallArg(argv, 0);
    // 1. If Type(arg) is not Object, return false.
    if (!arg->IsECMAObject()) {
        return BuiltinsArrayBuffer::GetTaggedBoolean(false);
    }
    // 2. If arg has a [[ViewedArrayBuffer]] internal slot, return true.
    if (arg->IsDataView() || arg->IsTypedArray()) {
        return BuiltinsArrayBuffer::GetTaggedBoolean(true);
    }
    // 3. Return false.
    return BuiltinsArrayBuffer::GetTaggedBoolean(false);
}

// 24.1.3.3 get ArrayBuffer [ @@species ]
JSTaggedValue BuiltinsArrayBuffer::Species(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayBuffer, Species);
    return GetThis(argv).GetTaggedValue();
}

// 24.1.4.1 get ArrayBuffer.prototype.byteLength
JSTaggedValue BuiltinsArrayBuffer::GetByteLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, ArrayBuffer, GetByteLength);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an object", JSTaggedValue::Exception());
    }
    // 3. If O does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have internal slot", JSTaggedValue::Exception());
    }
    // 4. If IsDetachedBuffer(O) is true, throw a TypeError exception.
    if (IsDetachedBuffer(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "IsDetachedBuffer", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> arrBuf(thisHandle);
    // 5. Let length be the value of O’s [[ArrayBufferByteLength]] internal slot.
    uint32_t length = arrBuf->GetArrayBufferByteLength();
    // 6. Return length.
    return JSTaggedValue(length);
}

// 24.1.4.3 ArrayBuffer.prototype.slice(start, end)
JSTaggedValue BuiltinsArrayBuffer::Slice(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, ArrayBuffer, Slice);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsHeapObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value is not an object", JSTaggedValue::Exception());
    }
    JSHandle<JSArrayBuffer> arrBuf(thisHandle);
    // 3. If O does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "don't have internal slot", JSTaggedValue::Exception());
    }
    // 4. If IsDetachedBuffer(O) is true, throw a TypeError exception.
    if (IsDetachedBuffer(thisHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this value IsDetachedBuffer", JSTaggedValue::Exception());
    }
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
    JSHandle<JSTaggedValue> defaultConstructor = env->GetArrayBufferFunction();
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
    if (!newArrBuf->IsArrayBuffer()) {
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
    JSHandle<JSArrayBuffer> newJsArrBuf(newArrBuf);
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
        JSArrayBuffer::CopyDataPointBytes(toBuf, fromBuf, first, newLen);
    }
    // Return new.
    return newArrBuf.GetTaggedValue();
}

// 24.1.1.1 AllocateArrayBuffer(constructor, byteLength)
JSTaggedValue BuiltinsArrayBuffer::AllocateArrayBuffer(JSThread *thread, const JSHandle<JSTaggedValue> &newTarget,
                                                       uint64_t byteLength)
{
    BUILTINS_API_TRACE(thread, ArrayBuffer, AllocateArrayBuffer);
    /**
     * 1. Let obj be OrdinaryCreateFromConstructor(constructor, "%ArrayBufferPrototype%",
     * «[[ArrayBufferData]], [[ArrayBufferByteLength]]» ).
     * */
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> arrBufFunc = env->GetArrayBufferFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(arrBufFunc), newTarget);
    // 2. ReturnIfAbrupt
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 4. Let block be CreateByteDataBlock(byteLength).
    if (byteLength > INT_MAX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Out of range", JSTaggedValue::Exception());
    }
    uint64_t totalNativeSize = static_cast<uint64_t>(thread->GetNativeAreaAllocator()->GetArrayBufferNativeSize());
    if (UNLIKELY(totalNativeSize > MAX_NATIVE_SIZE_LIMIT)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, NATIVE_SIZE_OUT_OF_LIMIT_MESSAGE, JSTaggedValue::Exception());
    }
    uint32_t arrayByteLength = static_cast<uint32_t>(byteLength);
    JSHandle<JSArrayBuffer> arrayBuffer(obj);
    // 6. Set obj’s [[ArrayBufferData]] internal slot to block.
    factory->NewJSArrayBufferData(arrayBuffer, arrayByteLength);
    // 7. Set obj’s [[ArrayBufferByteLength]] internal slot to byteLength.
    arrayBuffer->SetArrayBufferByteLength(arrayByteLength);
    // 8. Return obj.
    return arrayBuffer.GetTaggedValue();
}

// 24.1.1.2 IsDetachedBuffer()
void BuiltinsArrayBuffer::IsDetachedBuffer(JSThread *thread, const JSHandle<JSTypedArray> &arrayBuffer)
{
    JSTaggedValue detachedBuffer = arrayBuffer->GetViewedArrayBufferOrByteArray();
    if (IsDetachedBuffer(detachedBuffer)) {
        THROW_TYPE_ERROR(thread, "The ArrayBuffer of this value is detached buffer.");
    }
}

bool BuiltinsArrayBuffer::IsDetachedBuffer(JSTaggedValue arrayBuffer)
{
    if (arrayBuffer.IsByteArray()) {
        return false;
    }
    // 1. Assert: Type(arrayBuffer) is Object and it has an [[ArrayBufferData]] internal slot.
    ASSERT(arrayBuffer.IsArrayBuffer() || arrayBuffer.IsSharedArrayBuffer());
    JSArrayBuffer *buffer = JSArrayBuffer::Cast(arrayBuffer.GetTaggedObject());
    if (buffer == nullptr) {
        LOG_ECMA(FATAL) << "BuiltinsArrayBuffer::IsDetachedBuffer:buffer is nullptr";
    }
    JSTaggedValue dataSlot = buffer->GetArrayBufferData();
    // 2. If arrayBuffer’s [[ArrayBufferData]] internal slot is null, return true.
    // 3. Return false.
    return dataSlot.IsNull();
}

// 24.1.1.4
JSTaggedValue BuiltinsArrayBuffer::CloneArrayBuffer(JSThread *thread, const JSHandle<JSTaggedValue> &srcBuffer,
                                                    uint32_t srcByteOffset, JSHandle<JSTaggedValue> constructor)
{
    BUILTINS_API_TRACE(thread, ArrayBuffer, CloneArrayBuffer);
    // 1. Assert: Type(srcBuffer) is Object and it has an [[ArrayBufferData]] internal slot.
    ASSERT(srcBuffer->IsArrayBuffer() || srcBuffer->IsSharedArrayBuffer() || srcBuffer->IsByteArray());
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    // 2. If cloneConstructor is not present
    if (constructor->IsUndefined()) {
        // a. Let cloneConstructor be SpeciesConstructor(srcBuffer, %ArrayBuffer%).
        JSHandle<JSTaggedValue> defaultConstructor = env->GetArrayBufferFunction();
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
        JSHandle<JSArrayBuffer> arrBuf(srcBuffer);
        srcLen = arrBuf->GetArrayBufferByteLength();
        // 5. Assert: srcByteOffset ≤ srcLength.
        ASSERT(srcByteOffset <= srcLen);
        // 6. Let cloneLength be srcLength – srcByteOffset.
        cloneLen = static_cast<int32_t>(srcLen - srcByteOffset);
    }
    // 8. Let targetBuffer be AllocateArrayBuffer(cloneConstructor, cloneLength).
    JSTaggedValue taggedBuf = AllocateArrayBuffer(thread, constructor, cloneLen);
    // 9. ReturnIfAbrupt(targetBuffer).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 10. If IsDetachedBuffer(srcBuffer) is true, throw a TypeError exception.
    if (IsDetachedBuffer(srcBuffer.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
    }
    // 11. Let targetBlock be the value of targetBuffer’s [[ArrayBufferData]] internal slot.
    JSHandle<JSArrayBuffer> newArrBuf(thread, taggedBuf);
    // Perform CopyDataBlockBytes(targetBlock, 0, srcBlock, srcByteOffset, cloneLength).
    // 7. Let srcBlock be the value of srcBuffer’s [[ArrayBufferData]] internal slot.
    if (cloneLen > 0) {
        void *fromBuf = GetDataPointFromBuffer(srcBuffer.GetTaggedValue());
        void *toBuf = GetDataPointFromBuffer(taggedBuf);
        JSArrayBuffer::CopyDataPointBytes(toBuf, fromBuf, srcByteOffset, cloneLen);
    }
    return taggedBuf;
}

// 24.1.1.5
// NOLINTNEXTLINE(readability-function-size)
JSTaggedValue BuiltinsArrayBuffer::GetValueFromBuffer(JSThread *thread, JSTaggedValue arrBuf, uint32_t byteIndex,
                                                      DataViewType type, bool littleEndian)
{
    void *pointer = GetDataPointFromBuffer(arrBuf);
    uint8_t *block = reinterpret_cast<uint8_t *>(pointer);
    return GetValueFromBuffer(thread, byteIndex, block, type, littleEndian);
}

JSTaggedValue BuiltinsArrayBuffer::GetValueFromBuffer(JSThread *thread, uint32_t byteIndex, uint8_t *block,
                                                      DataViewType type, bool littleEndian)
{
    ASSERT(block != nullptr);
    switch (type) {
        case DataViewType::UINT8:
        case DataViewType::UINT8_CLAMPED: {
            uint8_t res = block[byteIndex];  // NOLINT
            return GetTaggedInt(res);
        }
        case DataViewType::INT8: {
            uint8_t res = block[byteIndex];  // NOLINT
            auto int8Res = static_cast<int8_t>(res);
            return GetTaggedInt(int8Res);
        }
        case DataViewType::UINT16:
            return GetValueFromBufferForInteger<uint16_t, NumberSize::UINT16>(block, byteIndex, littleEndian);
        case DataViewType::INT16:
            return GetValueFromBufferForInteger<int16_t, NumberSize::INT16>(block, byteIndex, littleEndian);
        case DataViewType::UINT32:
            return GetValueFromBufferForInteger<uint32_t, NumberSize::UINT32>(block, byteIndex, littleEndian);
        case DataViewType::INT32:
            return GetValueFromBufferForInteger<int32_t, NumberSize::INT32>(block, byteIndex, littleEndian);
        case DataViewType::FLOAT32:
            return GetValueFromBufferForFloat<float, UnionType32, NumberSize::FLOAT32>(block, byteIndex, littleEndian);
        case DataViewType::FLOAT64:
            return GetValueFromBufferForFloat<double, UnionType64, NumberSize::FLOAT64>(block, byteIndex, littleEndian);
        case DataViewType::BIGINT64:
            return GetValueFromBufferForBigInt<int64_t, NumberSize::BIGINT64>(thread, block, byteIndex, littleEndian);
        case DataViewType::BIGUINT64:
            return GetValueFromBufferForBigInt<uint64_t, NumberSize::BIGUINT64>(thread, block, byteIndex, littleEndian);
        default:
            break;
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

// 24.1.1.6
JSTaggedValue BuiltinsArrayBuffer::SetValueInBuffer(JSThread *thread, JSTaggedValue arrBuf, uint32_t byteIndex,
                                                    DataViewType type, const JSHandle<JSTaggedValue> &value,
                                                    bool littleEndian)
{
    if (UNLIKELY(IsBigIntElementType(type))) {
        JSHandle<JSTaggedValue> arrBufHandle(thread, arrBuf);
        switch (type) {
            case DataViewType::BIGINT64:
                SetValueInBufferForBigInt<int64_t>(thread, value, arrBufHandle, byteIndex, littleEndian);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                break;
            case DataViewType::BIGUINT64:
                SetValueInBufferForBigInt<uint64_t>(thread, value, arrBufHandle, byteIndex, littleEndian);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        return JSTaggedValue::Undefined();
    }
    void *pointer = GetDataPointFromBuffer(arrBuf);
    uint8_t *block = reinterpret_cast<uint8_t *>(pointer);
    JSTaggedNumber numberValue = JSTaggedValue::ToNumber(thread, value.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    double val = numberValue.GetNumber();
    return SetValueInBuffer(thread, byteIndex, block, type, val, littleEndian);
}

// es12 25.1.2.7 IsBigIntElementType ( type )
bool BuiltinsArrayBuffer::IsBigIntElementType(DataViewType type)
{
    if (type == DataViewType::BIGINT64 || type == DataViewType::BIGUINT64) {
        return true;
    }
    return false;
}

// es12 25.1.2.6 IsUnclampedIntegerElementType ( type )
bool BuiltinsArrayBuffer::IsUnclampedIntegerElementType(DataViewType type)
{
    switch (type) {
        case DataViewType::INT8:
        case DataViewType::INT16:
        case DataViewType::INT32:
        case DataViewType::UINT8:
        case DataViewType::UINT16:
        case DataViewType::UINT32:
            return true;
        default:
            return false;
    }
}

template<typename T>
void BuiltinsArrayBuffer::SetTypeData(uint8_t *block, T value, uint32_t index)
{
    uint32_t sizeCount = sizeof(T);
    uint8_t *res = reinterpret_cast<uint8_t *>(&value);
    for (uint32_t i = 0; i < sizeCount; i++) {
        *(block + index + i) = *(res + i);  // NOLINT
    }
}

template<typename T>
void BuiltinsArrayBuffer::FastSetTypeData(uint8_t *byteBeginOffset, uint8_t *byteEndOffset, T value)
{
    ASSERT(byteBeginOffset != nullptr);
    ASSERT(byteEndOffset != nullptr);
    uint32_t sizeCount = sizeof(T);
    if (sizeCount == 1) {
        memset_s(byteBeginOffset, byteEndOffset-byteBeginOffset, value, byteEndOffset-byteBeginOffset);
    } else {
        uint8_t *resAddr = reinterpret_cast<uint8_t *>(&value);
        for (uint8_t *addr = byteBeginOffset; addr < byteEndOffset; addr += sizeCount) {
            for (uint32_t i = 0; i < sizeCount; ++i) {
                *(addr + i) = *(resAddr + i);
            }
        }
    }
}

template <typename T>
T BuiltinsArrayBuffer::LittleEndianToBigEndian(T liValue)
{
    uint8_t sizeCount = sizeof(T);
    T biValue;
    switch (sizeCount) {
        case NumberSize::UINT16:
            biValue = ((liValue & 0x00FF) << BITS_EIGHT)     // NOLINT
                      | ((liValue & 0xFF00) >> BITS_EIGHT);  // NOLINT
            break;
        case NumberSize::UINT32:
            biValue = ((liValue & 0x000000FF) << BITS_TWENTY_FOUR)     // NOLINT
                      | ((liValue & 0x0000FF00) << BITS_EIGHT)         // NOLINT
                      | ((liValue & 0x00FF0000) >> BITS_EIGHT)         // NOLINT
                      | ((liValue & 0xFF000000) >> BITS_TWENTY_FOUR);  // NOLINT
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    return biValue;
}
template <typename T>
T BuiltinsArrayBuffer::LittleEndianToBigEndian64Bit(T liValue)
{
    return ((liValue & 0x00000000000000FF) << BITS_FIFTY_SIX)      // NOLINT
           | ((liValue & 0x000000000000FF00) << BITS_FORTY)        // NOLINT
           | ((liValue & 0x0000000000FF0000) << BITS_TWENTY_FOUR)  // NOLINT
           | ((liValue & 0x00000000FF000000) << BITS_EIGHT)        // NOLINT
           | ((liValue & 0x000000FF00000000) >> BITS_EIGHT)        // NOLINT
           | ((liValue & 0x0000FF0000000000) >> BITS_TWENTY_FOUR)  // NOLINT
           | ((liValue & 0x00FF000000000000) >> BITS_FORTY)        // NOLINT
           | ((liValue & 0xFF00000000000000) >> BITS_FIFTY_SIX);   // NOLINT
}

template<typename T, BuiltinsArrayBuffer::NumberSize size>
JSTaggedValue BuiltinsArrayBuffer::GetValueFromBufferForInteger(uint8_t *block, uint32_t byteIndex, bool littleEndian)
{
    ASSERT(block != nullptr);
    ASSERT_PRINT(std::is_integral_v<T>, "T must be integral");
    ASSERT_PRINT(sizeof(T) == size, "Invalid number size");
    ASSERT_PRINT(sizeof(T) >= sizeof(uint16_t), "T must have a size more than uint8");

    ASSERT(size >= NumberSize::UINT16 || size <= NumberSize::FLOAT64);
    T res = *reinterpret_cast<T *>(block + byteIndex);
    if (!littleEndian) {
        res = LittleEndianToBigEndian(res);
    }

    // uint32_t maybe overflow with TaggedInt
    // NOLINTNEXTLINE(readability-braces-around-statements,bugprone-suspicious-semicolon)
    if constexpr (std::is_same_v<T, uint32_t>) {
        // NOLINTNEXTLINE(clang-diagnostic-sign-compare)
        if (res > static_cast<uint32_t>(std::numeric_limits<int32_t>::max())) {
            return GetTaggedDouble(static_cast<double>(res));
        }
    }
    return GetTaggedInt(res);
}

template<typename T, typename UnionType, BuiltinsArrayBuffer::NumberSize size>
JSTaggedValue BuiltinsArrayBuffer::GetValueFromBufferForFloat(uint8_t *block, uint32_t byteIndex, bool littleEndian)
{
    ASSERT(block != nullptr);
    ASSERT_PRINT((std::is_same_v<T, float> || std::is_same_v<T, double>), "T must be correct type");
    ASSERT_PRINT(sizeof(T) == size, "Invalid number size");

    UnionType unionValue = {0};
    // NOLINTNEXTLINE(readability-braces-around-statements)
    if constexpr (std::is_same_v<T, float>) {
        unionValue.uValue = *reinterpret_cast<uint32_t *>(block + byteIndex);
        uint32_t res = LittleEndianToBigEndian(unionValue.uValue);
        return CommonConvert<T, uint32_t>(unionValue.value, res, littleEndian);
    } else if constexpr (std::is_same_v<T, double>) {  // NOLINTNEXTLINE(readability-braces-around-statements)
        unionValue.uValue = *reinterpret_cast<uint64_t *>(block + byteIndex);
        uint64_t res = LittleEndianToBigEndian64Bit(unionValue.uValue);
        return CommonConvert<T, uint64_t>(unionValue.value, res, littleEndian);
    }

    return GetTaggedDouble(unionValue.value);
}

template<typename T1, typename T2>
JSTaggedValue BuiltinsArrayBuffer::CommonConvert(T1 &value, T2 &res, bool littleEndian)
{
    if (std::isnan(value) && !JSTaggedValue::IsImpureNaN(value)) {
        return GetTaggedDouble(value);
    }
    if (!littleEndian) {
        T1 d = base::bit_cast<T1>(res);
        if (JSTaggedValue::IsImpureNaN(d)) {
            return GetTaggedDouble(base::NAN_VALUE);
        }
        return GetTaggedDouble(d);
    } else {
        if (JSTaggedValue::IsImpureNaN(value)) {
            return GetTaggedDouble(base::NAN_VALUE);
        }
    }
    return GetTaggedDouble(value);
}


template<typename T, BuiltinsArrayBuffer::NumberSize size>
JSTaggedValue BuiltinsArrayBuffer::GetValueFromBufferForBigInt(JSThread *thread, uint8_t *block,
                                                               uint32_t byteIndex, bool littleEndian)
{
    ASSERT(block != nullptr);
    ASSERT_PRINT((std::is_same_v<T, uint64_t> || std::is_same_v<T, int64_t>), "T must be uint64_t/int64_t");
    auto pTmp = *reinterpret_cast<uint64_t *>(block + byteIndex);
    if (!littleEndian) {
        pTmp = LittleEndianToBigEndian64Bit(pTmp);
    }
    if constexpr (std::is_same_v<T, uint64_t>) {
        return BigInt::Uint64ToBigInt(thread, pTmp).GetTaggedValue();
    }
    return BigInt::Int64ToBigInt(thread, pTmp).GetTaggedValue();
}


template<typename T>
void BuiltinsArrayBuffer::SetValueInBufferForByte(double val, uint8_t *block, uint32_t byteIndex)
{
    ASSERT_PRINT((std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>), "T must be int8/uint8");
    T res;
    if (std::isnan(val) || std::isinf(val)) {
        res = 0;
        SetTypeData(block, res, byteIndex);
        return;
    }
    auto int64Val = static_cast<int64_t>(val);
    auto *resArr = reinterpret_cast<T *>(&int64Val);
    res = *resArr;
    SetTypeData(block, res, byteIndex);
}

void BuiltinsArrayBuffer::SetValueInBufferForUint8Clamped(double val, uint8_t *block, uint32_t byteIndex)
{
    uint8_t res;
    if (std::isnan(val) || val <= 0) {
        res = 0;
    } else if (val > UINT8_MAX) {
        res = UINT8_MAX;
    } else {
        // same as ToUint8Clamp
        res = std::lrint(val);
    }
    SetTypeData(block, res, byteIndex);
}

template<typename T>
void BuiltinsArrayBuffer::SetValueInBufferForInteger(double val, uint8_t *block, uint32_t byteIndex, bool littleEndian)
{
    ASSERT_PRINT(std::is_integral_v<T>, "T must be integral");
    ASSERT_PRINT(sizeof(T) >= sizeof(uint16_t), "T must have a size more than uint8");
    T res;
    if (std::isnan(val) || std::isinf(val)) {
        res = 0;
        SetTypeData(block, res, byteIndex);
        return;
    }
    auto int64Val = static_cast<int64_t>(val);
    // NOLINTNEXTLINE(readability-braces-around-statements)
    if constexpr (std::is_same_v<T, uint16_t>) {
        auto *pTmp = reinterpret_cast<int16_t *>(&int64Val);
        int16_t tmp = *pTmp;
        res = static_cast<T>(tmp);
    } else {  // NOLINTNEXTLINE(readability-braces-around-statements)
        auto *pTmp = reinterpret_cast<T *>(&int64Val);
        res = *pTmp;
    }

    if (!littleEndian) {
        res = LittleEndianToBigEndian<T>(res);
    }
    SetTypeData(block, res, byteIndex);
}

template<typename T>
void BuiltinsArrayBuffer::SetValueInBufferForFloat(double val, uint8_t *block, uint32_t byteIndex, bool littleEndian)
{
    ASSERT_PRINT((std::is_same_v<T, float> || std::is_same_v<T, double>), "T must be float type");
    auto data = static_cast<T>(val);
    if (std::isnan(val)) {
        SetTypeData(block, data, byteIndex);
        return;
    }
    if (!littleEndian) {
        if constexpr (std::is_same_v<T, float>) {
            uint32_t res = base::bit_cast<uint32_t>(data);
            data = base::bit_cast<T>(LittleEndianToBigEndian(res));
        } else if constexpr (std::is_same_v<T, double>) {
            uint64_t res = base::bit_cast<uint64_t>(data);
            data = base::bit_cast<T>(LittleEndianToBigEndian64Bit(res));
        }
    }
    SetTypeData(block, data, byteIndex);
}

template<typename T>
void BuiltinsArrayBuffer::SetValueInBufferForBigInt(JSThread *thread,
                                                    const JSHandle<JSTaggedValue> &val,
                                                    JSHandle<JSTaggedValue> &arrBuf,
                                                    uint32_t byteIndex, bool littleEndian)
{
    ASSERT_PRINT((std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>), "T must be int64_t/uint64_t");
    T value = 0;
    bool lossless = true;
    if constexpr(std::is_same_v<T, uint64_t>) {
        BigInt::BigIntToUint64(thread, val, reinterpret_cast<uint64_t *>(&value), &lossless);
    } else {
        BigInt::BigIntToInt64(thread, val, reinterpret_cast<int64_t *>(&value), &lossless);
    }
    RETURN_IF_ABRUPT_COMPLETION(thread);
    if (!littleEndian) {
        value = LittleEndianToBigEndian64Bit<T>(value);
    }
    void *pointer = GetDataPointFromBuffer(arrBuf.GetTaggedValue());
    uint8_t *block = reinterpret_cast<uint8_t *>(pointer);
    SetTypeData(block, value, byteIndex);
}

template<typename T>
void BuiltinsArrayBuffer::SetValueInBufferForBigInt(JSThread *thread,
                                                    double val, uint8_t *block,
                                                    uint32_t byteIndex, bool littleEndian)
{
    ASSERT_PRINT((std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>), "T must be int64_t/uint64_t");
    T value = 0;
    bool lossless = true;

    JSHandle<JSTaggedValue> valHandle(thread, GetTaggedDouble(val));
    if constexpr(std::is_same_v<T, uint64_t>) {
        BigInt::BigIntToUint64(thread, valHandle, reinterpret_cast<uint64_t *>(&value), &lossless);
    } else {
        BigInt::BigIntToInt64(thread, valHandle, reinterpret_cast<int64_t *>(&value), &lossless);
    }
    RETURN_IF_ABRUPT_COMPLETION(thread);
    if (!littleEndian) {
        value = LittleEndianToBigEndian64Bit<T>(value);
    }
    SetTypeData(block, value, byteIndex);
}

JSTaggedValue BuiltinsArrayBuffer::FastSetValueInBuffer(JSThread *thread, JSTaggedValue arrBuf, uint32_t byteIndex,
                                                        DataViewType type, double val, bool littleEndian)
{
    // origin type maybe native JSType or shared JSType.
    if (arrBuf.IsSendableArrayBuffer() && BuiltinsSendableArrayBuffer::IsDetachedBuffer(arrBuf)) {
        return JSTaggedValue::Undefined();
    }
    if (!arrBuf.IsSendableArrayBuffer() && BuiltinsArrayBuffer::IsDetachedBuffer(arrBuf)) {
        return JSTaggedValue::Undefined();
    }
    void *pointer = GetDataPointFromBuffer(arrBuf);
    uint8_t *block = reinterpret_cast<uint8_t *>(pointer);
    return SetValueInBuffer(thread, byteIndex, block, type, val, littleEndian);
}

JSTaggedValue BuiltinsArrayBuffer::SetValueInBuffer(JSThread* thread, uint32_t byteIndex, uint8_t *block,
                                                    DataViewType type, double val, bool littleEndian)
{
    switch (type) {
        case DataViewType::UINT8:
            SetValueInBufferForByte<uint8_t>(val, block, byteIndex);
            break;
        case DataViewType::UINT8_CLAMPED:
            SetValueInBufferForUint8Clamped(val, block, byteIndex);
            break;
        case DataViewType::INT8:
            SetValueInBufferForByte<int8_t>(val, block, byteIndex);
            break;
        case DataViewType::UINT16:
            SetValueInBufferForInteger<uint16_t>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::INT16:
            SetValueInBufferForInteger<int16_t>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::UINT32:
            SetValueInBufferForInteger<uint32_t>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::INT32:
            SetValueInBufferForInteger<int32_t>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::FLOAT32:
            SetValueInBufferForFloat<float>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::FLOAT64:
            SetValueInBufferForFloat<double>(val, block, byteIndex, littleEndian);
            break;
        case DataViewType::BIGINT64:
            SetValueInBufferForBigInt<int64_t>(thread, val, block, byteIndex, littleEndian);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            break;
        case DataViewType::BIGUINT64:
            SetValueInBufferForBigInt<uint64_t>(thread, val, block, byteIndex, littleEndian);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return JSTaggedValue::Undefined();
}

void *BuiltinsArrayBuffer::GetDataPointFromBuffer(JSTaggedValue arrBuf, uint32_t byteOffset)
{
    if (arrBuf.IsByteArray()) {
        return reinterpret_cast<void *>(ToUintPtr(ByteArray::Cast(arrBuf.GetTaggedObject())->GetData()) + byteOffset);
    }

    JSArrayBuffer *arrayBuffer = JSArrayBuffer::Cast(arrBuf.GetTaggedObject());
    if (arrayBuffer == nullptr) {
        LOG_ECMA(FATAL) << "BuiltinsArrayBuffer::GetDataPointFromBuffer:arrayBuffer is nullptr";
        UNREACHABLE();
    }
    if (arrayBuffer->GetArrayBufferByteLength() == 0) {
        LOG_ECMA(FATAL) << "BuiltinsArrayBuffer::GetDataPointFromBuffer:arrayBuffer length is 0";
        UNREACHABLE();
    }

    JSTaggedValue data = arrayBuffer->GetArrayBufferData();
    return reinterpret_cast<void *>(ToUintPtr(JSNativePointer::Cast(data.GetTaggedObject())
                                    ->GetExternalPointer()) + byteOffset);
}

JSTaggedValue BuiltinsArrayBuffer::TypedArrayToList(JSThread *thread, JSHandle<JSTypedArray>& items)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> bufferHandle(thread, items->GetViewedArrayBufferOrByteArray());
    uint32_t arrayLen = items->GetArrayLength();
    JSHandle<JSObject> newArrayHandle(thread, JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<TaggedArray> oldElements(thread, newArrayHandle->GetElements());
    JSHandle<TaggedArray> elements = (oldElements->GetLength() < arrayLen) ?
        factory->ExtendArray(oldElements, arrayLen) : oldElements;
    newArrayHandle->SetElements(thread, elements);
    uint32_t offset = items->GetByteOffset();
    uint32_t elementSize = TypedArrayHelper::GetElementSize(items);
    DataViewType elementType = TypedArrayHelper::GetType(items);
    uint32_t index = 0;
    while (index < arrayLen) {
        uint32_t byteIndex = index * elementSize + offset;
        JSHandle<JSTaggedValue> result(thread, GetValueFromBuffer(thread, bufferHandle.GetTaggedValue(),
                                       byteIndex, elementType, true));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        ElementAccessor::Set(thread, newArrayHandle, index, result, true);
        index++;
    }
    JSHandle<JSArray>(newArrayHandle)->SetArrayLength(thread, arrayLen);
    return newArrayHandle.GetTaggedValue();
}

template<typename T>
void BuiltinsArrayBuffer::FastSetValueInBufferForByte(uint8_t *byteBeginOffset,
                                                      uint8_t *byteEndOffset,
                                                      double val)
{
    ASSERT_PRINT(sizeof(T) == 1, "sizeof(T) must be one");
    ASSERT_PRINT((std::is_same_v<T, uint8_t> || std::is_same_v<T, int8_t>), "T must be int8/uint8");
    T res;
    if (std::isnan(val) || std::isinf(val)) {
        res = 0;
    } else {
        auto int64Val = static_cast<int64_t>(val);
        auto *resArr = reinterpret_cast<T *>(&int64Val);
        res = *resArr;
    }
    FastSetTypeData(byteBeginOffset, byteEndOffset, res);
}

void BuiltinsArrayBuffer::FastSetValueInBufferForUint8Clamped(uint8_t *byteBeginOffset,
                                                              uint8_t *byteEndOffset,
                                                              double val)
{
    uint8_t res;
    if (std::isnan(val) || val <= 0) {
        res = 0;
    } else {
        val = val >= UINT8_MAX ? UINT8_MAX : val;
        constexpr double HALF = 0.5;
        val = val == HALF ? 0 : std::round(val);
        res = static_cast<uint64_t>(val);
    }
    FastSetTypeData(byteBeginOffset, byteEndOffset, res);
}

template<typename T>
void BuiltinsArrayBuffer::FastSetValueInBufferForInteger(uint8_t *byteBeginOffset,
                                                         uint8_t *byteEndOffset,
                                                         double val, bool littleEndian)
{
    ASSERT_PRINT(std::is_integral_v<T>, "T must be integral");
    ASSERT_PRINT(sizeof(T) >= sizeof(uint16_t), "T must have a size more than uint8");
    T res;
    if (std::isnan(val) || std::isinf(val)) {
        res = 0;
    } else {
        auto int64Val = static_cast<int64_t>(val);
        // NOLINTNEXTLINE(readability-braces-around-statements)
        if constexpr (std::is_same_v<T, uint16_t>) {
            auto *pTmp = reinterpret_cast<int16_t *>(&int64Val);
            int16_t tmp = *pTmp;
            res = static_cast<T>(tmp);
        } else {  // NOLINTNEXTLINE(readability-braces-around-statements)
            auto *pTmp = reinterpret_cast<T *>(&int64Val);
            res = *pTmp;
        }
        if (!littleEndian) {
            res = LittleEndianToBigEndian<T>(res);
        }
    }
    FastSetTypeData(byteBeginOffset, byteEndOffset, res);
}

template<typename T>
void BuiltinsArrayBuffer::FastSetValueInBufferForFloat(uint8_t *byteBeginOffset,
                                                       uint8_t *byteEndOffset,
                                                       double val, bool littleEndian)
{
    ASSERT_PRINT((std::is_same_v<T, float> || std::is_same_v<T, double>), "T must be float type");
    auto data = static_cast<T>(val);
    if (!std::isnan(val)) {
        if (!littleEndian) {
            if constexpr (std::is_same_v<T, float>) {
                uint32_t res = base::bit_cast<uint32_t>(data);
                data = base::bit_cast<T>(LittleEndianToBigEndian(res));
            } else if constexpr (std::is_same_v<T, double>) {
                uint64_t res = base::bit_cast<uint64_t>(data);
                data = base::bit_cast<T>(LittleEndianToBigEndian64Bit(res));
            }
        }
    }
    FastSetTypeData(byteBeginOffset, byteEndOffset, data);
}

template<typename T>
void BuiltinsArrayBuffer::FastSetValueInBufferForBigInt(JSThread *thread,
                                                        uint8_t *byteBeginOffset,
                                                        uint8_t *byteEndOffset,
                                                        double val, bool littleEndian)
{
    ASSERT_PRINT((std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>), "T must be int64_t/uint64_t");
    T value = 0;
    bool lossless = true;

    JSHandle<JSTaggedValue> valHandle(thread, GetTaggedDouble(val));
    if constexpr(std::is_same_v<T, uint64_t>) {
        BigInt::BigIntToUint64(thread, valHandle, reinterpret_cast<uint64_t *>(&value), &lossless);
    } else {
        BigInt::BigIntToInt64(thread, valHandle, reinterpret_cast<int64_t *>(&value), &lossless);
    }
    RETURN_IF_ABRUPT_COMPLETION(thread);
    if (!littleEndian) {
        value = LittleEndianToBigEndian64Bit<T>(value);
    }
    FastSetTypeData(byteBeginOffset, byteEndOffset, value);
}

JSTaggedValue BuiltinsArrayBuffer::TryFastSetValueInBuffer([[maybe_unused]] JSThread *thread, JSTaggedValue arrBuf,
                                                           uint32_t byteBeginOffset, uint32_t byteEndOffset,
                                                           DataViewType type, double val, bool littleEndian)
{
    uint8_t *beginPointer = reinterpret_cast<uint8_t *>(GetDataPointFromBuffer(arrBuf, byteBeginOffset));
    uint8_t *endPointer = reinterpret_cast<uint8_t *>(GetDataPointFromBuffer(arrBuf, byteEndOffset));
    switch (type) {
        case DataViewType::UINT8:
            FastSetValueInBufferForByte<uint8_t>(beginPointer, endPointer, val);
            break;
        case DataViewType::UINT8_CLAMPED:
            FastSetValueInBufferForUint8Clamped(beginPointer, endPointer, val);
            break;
        case DataViewType::INT8:
            FastSetValueInBufferForByte<int8_t>(beginPointer, endPointer, val);
            break;
        case DataViewType::UINT16:
            FastSetValueInBufferForInteger<uint16_t>(beginPointer, endPointer, val, littleEndian);
            break;
        case DataViewType::INT16:
            FastSetValueInBufferForInteger<int16_t>(beginPointer, endPointer, val, littleEndian);
            break;
        case DataViewType::UINT32:
            FastSetValueInBufferForInteger<uint32_t>(beginPointer, endPointer, val, littleEndian);
            break;
        case DataViewType::INT32:
            FastSetValueInBufferForInteger<int32_t>(beginPointer, endPointer, val, littleEndian);
            break;
        case DataViewType::FLOAT32:
            FastSetValueInBufferForFloat<float>(beginPointer, endPointer, val, littleEndian);
            break;
        case DataViewType::FLOAT64:
            FastSetValueInBufferForFloat<double>(beginPointer, endPointer, val, littleEndian);
            break;
        case DataViewType::BIGINT64:
            FastSetValueInBufferForBigInt<int64_t>(thread, beginPointer, endPointer, val, littleEndian);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            break;
        case DataViewType::BIGUINT64:
            FastSetValueInBufferForBigInt<uint64_t>(thread, beginPointer, endPointer, val, littleEndian);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript::builtins
