/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/containers/containers_buffer.h"
#include <cstdint>

#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/containers/containers_errors.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_buffer.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_typed_array.h"
#include "macros.h"

namespace panda::ecmascript::containers {

#define RANGE_ERROR_CHECK(value, name, left, right)                                                                    \
    double(num##name) = (value)->GetNumber();                                                                          \
    if ((num##name) < (left) || (num##name) > (right)) {                                                               \
        std::ostringstream oss;                                                                                        \
        oss << "The value of \"" << (#name) << "\" is out of range. It must be >= " << (left) << " and <= " << (right) \
            << ". Received value is: " << (num##name);                                                                 \
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::RANGE_ERROR, oss.str().c_str());        \
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                                   \
    }

#define CHECK_NULL_OR_UNDEFINED(value)                                                                         \
    if (value->IsUndefined() || value->IsNull()) {                                                             \
        std::ostringstream oss;                                                                                \
        oss << "The parameter should not be null or undefined.";                                               \
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::TYPE_ERROR, oss.str().c_str()); \
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());                           \
    }

int32_t GetValueInt32(JSHandle<JSTaggedValue> valueHandle)
{
    JSTaggedValue value = valueHandle.GetTaggedValue();
    if (UNLIKELY(value.IsDouble())) {
        return static_cast<int32_t>(
            ecmascript::base::NumberHelper::DoubleToInt(value.GetDouble(), ecmascript::base::INT32_BITS));
    }
    return value.GetInt();
}

uint32_t GetValueUInt32(JSHandle<JSTaggedValue> valueHandle)
{
    return static_cast<uint32_t>(GetValueInt32(valueHandle));
}

bool IsNegetiveNumber(JSHandle<JSTaggedValue> &v)
{
    ASSERT(v->IsNumber());
    return v->GetNumber() < 0;
}

bool IsValidEncoding(JSHandle<JSTaggedValue> &str)
{
    if (!str->IsString()) {
        return false;
    }
    auto strAccessor = EcmaStringAccessor(JSHandle<EcmaString>(str));
    auto res = strAccessor.ToStdString();
    return JSAPIFastBuffer::GetEncodingType(res) != JSAPIFastBuffer::EncodingType::INVALID;
}

JSTaggedValue ContainersBuffer::BufferConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    if (newTarget->IsUndefined()) {
        JSTaggedValue error = ContainerError::BusinessError(thread, ErrorFlag::IS_NULL_ERROR,
                                                            "The Buffer's constructor cannot be directly invoked.");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIFastBuffer> bufferObj = JSHandle<JSAPIFastBuffer>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget));
    // parameter preprocess
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);                 // 0 means the first arg
    JSHandle<JSTaggedValue> byteOffsetOrEncoding = GetCallArg(argv, 1);  // 1 means the second arg
    JSHandle<JSTaggedValue> length = GetCallArg(argv, 2);                // 2 means the third arg
    CHECK_NULL_OR_UNDEFINED(value);
    if (value->IsNumber()) {
        RANGE_ERROR_CHECK(value, value, 0, UINT32_MAX);
        JSAPIFastBuffer::AllocateFastBuffer(thread, bufferObj, GetValueUInt32(value));
    } else if (value->IsString()) {
        if (byteOffsetOrEncoding->IsString() && !IsValidEncoding(byteOffsetOrEncoding)) {
            JSTaggedValue error = ContainerError::BusinessError(
                thread, ErrorFlag::TYPE_ERROR,
                "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        JSAPIFastBuffer::FromString(thread, bufferObj, value, byteOffsetOrEncoding);
    } else if (value->IsJSAPIBuffer()) {
        if (byteOffsetOrEncoding->IsNumber() && length->IsNumber()) {
            // share memory with another buffer(eg: create from pool)
            JSAPIFastBuffer::AllocateFromBufferObject(thread, bufferObj, value, GetValueUInt32(length),
                                                      GetValueUInt32(byteOffsetOrEncoding));
        } else {
            uint32_t bufferLength = JSAPIFastBuffer::Cast(value->GetTaggedObject())->GetLength();
            JSAPIFastBuffer::AllocateFastBuffer(thread, bufferObj, bufferLength);
            bufferObj->SetLength(bufferLength);
            auto bufferHandle = JSHandle<JSTaggedValue>::Cast(bufferObj);
            JSAPIFastBuffer::Copy(thread, bufferHandle, value, 0, 0, bufferLength);
        }
    } else if (value->IsArrayBuffer() || value->IsSharedArrayBuffer()) {
        uint32_t index = 0;
        uint32_t byteLength = JSHandle<JSArrayBuffer>(value)->GetArrayBufferByteLength();
        if (byteOffsetOrEncoding->IsNumber()) {
            RANGE_ERROR_CHECK(byteOffsetOrEncoding, byteOffsetOrEncoding, 0, UINT32_MAX);
            index = GetValueUInt32(byteOffsetOrEncoding);
        }
        if (length->IsNumber()) {
            RANGE_ERROR_CHECK(length, length, 0, UINT32_MAX);
            byteLength = GetValueUInt32(length);
        }
        JSAPIFastBuffer::FromArrayBuffer(thread, bufferObj, value, index, byteLength);
    } else if (value->IsJSUint8Array()) {
        bufferObj->SetFastBufferData(thread, value);
        bufferObj->SetLength(JSTypedArray::Cast(value->GetTaggedObject())->GetByteLength());
    } else if (value->IsJSArray()) {
        JSAPIFastBuffer::CreateBufferFromArrayLike(thread, bufferObj, value);
    }

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return bufferObj.GetTaggedValue();
}

JSTaggedValue ContainersBuffer::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, GetSize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(GetSize);

    return JSTaggedValue(JSHandle<JSAPIFastBuffer>::Cast(self)->GetLength());
}

JSTaggedValue ContainersBuffer::GetByteOffset(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, GetByteOffset);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(GetByteOffset);
    auto array = JSTypedArray::Cast(JSHandle<JSAPIFastBuffer>::Cast(self)->GetFastBufferData().GetTaggedObject());
    return JSTaggedValue(array->GetByteOffset());
}

JSTaggedValue ContainersBuffer::GetArrayBuffer(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, GetArrayBuffer);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(GetArrayBuffer);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    return JSAPIFastBuffer::GetArrayBuffer(thread, buffer);
}

JSTaggedValue ContainersBuffer::Compare(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Compare);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Compare);
    auto value = GetCallArg(argv, 0);  // 0 means the first arg
    CHECK_NULL_OR_UNDEFINED(value);
    uint32_t tEnd;
    if (value->IsJSAPIBuffer()) {
        tEnd = JSHandle<JSAPIFastBuffer>::Cast(value)->GetLength();
    } else if (value->IsJSUint8Array()) {
        tEnd = JSHandle<JSTypedArray>::Cast(value)->GetByteLength();
    }
    auto src = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto targetStart = GetCallArg(argv, 1);  // 1 means the second arg
    auto targetEnd = GetCallArg(argv, 2);    // 2 means the third arg
    auto sourceStart = GetCallArg(argv, 3);  // 3 means the fourth arg
    auto sourceEnd = GetCallArg(argv, 4);    // 4 means the fifth arg
    uint32_t tStart = 0;
    if (targetStart->IsNumber()) {
        RANGE_ERROR_CHECK(targetStart, targetStart, 0, UINT32_MAX);
        tStart = GetValueUInt32(targetStart);
    }
    if (targetEnd->IsNumber()) {
        RANGE_ERROR_CHECK(targetEnd, targetEnd, 0, UINT32_MAX);
        tEnd = GetValueUInt32(targetEnd);
    }
    uint32_t sStart = 0;
    if (sourceStart->IsNumber()) {
        RANGE_ERROR_CHECK(sourceStart, sourceStart, 0, UINT32_MAX);
        sStart = GetValueUInt32(sourceStart);
    }
    uint32_t sEnd = src->GetLength();
    if (sourceEnd->IsNumber()) {
        RANGE_ERROR_CHECK(sourceEnd, sourceEnd, 0, UINT32_MAX);
        sEnd = GetValueUInt32(sourceEnd);
    }
    if (sStart >= sEnd) {
        return JSTaggedValue(tStart >= tEnd ? 0 : -1);
    }
    if (tStart >= tEnd) {
        return JSTaggedValue(1);
    }
    auto ret = JSAPIFastBuffer::Compare(thread, src, value, sStart, sEnd, tStart, tEnd);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}

JSTaggedValue ContainersBuffer::Equals(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Equals);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Equals);
    auto value = GetCallArg(argv, 0);  // 0 means the first arg
    if (value->IsNull() || value->IsUndefined()) {
        return JSTaggedValue(false);
    }
    auto src = JSHandle<JSAPIFastBuffer>::Cast(self);
    uint32_t tEnd;
    if (value->IsJSAPIBuffer()) {
        tEnd = JSHandle<JSAPIFastBuffer>::Cast(value)->GetLength();
    } else if (value->IsJSUint8Array()) {
        tEnd = JSHandle<JSTypedArray>::Cast(value)->GetByteLength();
    } else {
        JSTaggedValue error = ContainerError::ParamError(thread, "unexpect value type, expect buffer | Uint8Array");
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
    }
    auto res = JSAPIFastBuffer::Compare(thread, src, value, 0, src->GetLength(), 0, tEnd);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res.GetInt() == 0 ? JSTaggedValue(true) : JSTaggedValue(false);
}

JSTaggedValue ContainersBuffer::IndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, IndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(IndexOf);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);   // 0 means the first arg
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);  // 1 means the second arg
    CHECK_NULL_OR_UNDEFINED(value);
    int32_t offsetIndex = 0;
    int32_t length = static_cast<int32_t>(buffer->GetLength());
    if (offset->IsNumber()) {
        if (offset->GetNumber() > length) {
            return JSTaggedValue(-1);
        } else {
            offsetIndex = GetValueInt32(offset);
            if (offsetIndex < 0) {
                offsetIndex = -offsetIndex;
                offsetIndex = length - offsetIndex;
                offsetIndex = std::max(offsetIndex, 0);
            }
        }
    }
    JSHandle<JSTaggedValue> encoding = GetCallArg(argv, 2);  // 2 means the third arg
    JSAPIFastBuffer::EncodingType encodingType = JSAPIFastBuffer::UTF8;
    if (encoding->IsString()) {
        if (!IsValidEncoding(encoding)) {
            JSTaggedValue error = ContainerError::BusinessError(
                thread, ErrorFlag::TYPE_ERROR,
                "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        encodingType = JSAPIFastBuffer::GetEncodingType(JSAPIFastBuffer::GetString(encoding));
    }
    return JSAPIFastBuffer::IndexOf(thread, buffer, value, offsetIndex, encodingType);
}

JSTaggedValue ContainersBuffer::Includes(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Includes);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Includes);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);  // 0 means the first arg
    CHECK_NULL_OR_UNDEFINED(value);
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);  // 1 means the second arg
    int32_t length = static_cast<int32_t>(buffer->GetLength());
    int32_t offsetIndex = 0;
    if (offset->IsNumber()) {
        if (offset->GetNumber() > length) {
            return JSTaggedValue(false);
        } else {
            offsetIndex = GetValueInt32(offset);
            if (offsetIndex < 0) {
                offsetIndex = -offsetIndex;
                offsetIndex = length - offsetIndex;
                offsetIndex = std::max(offsetIndex, 0);
            }
        }
    }
    JSHandle<JSTaggedValue> encoding = GetCallArg(argv, 2);  // 2 means the third arg
    JSAPIFastBuffer::EncodingType encodingType = JSAPIFastBuffer::UTF8;
    if (encoding->IsString()) {
        if (!IsValidEncoding(encoding)) {
            JSTaggedValue error = ContainerError::BusinessError(
                thread, ErrorFlag::TYPE_ERROR,
                "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        encodingType = JSAPIFastBuffer::GetEncodingType(JSAPIFastBuffer::GetString(encoding));
    }
    return JSAPIFastBuffer::Includes(thread, buffer, value, offsetIndex, encodingType);
}

JSTaggedValue ContainersBuffer::LastIndexOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, LastIndexOf);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(LastIndexOf);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);  // 0 means the first arg
    CHECK_NULL_OR_UNDEFINED(value);
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);  // 1 means the second arg
    int32_t length = static_cast<int32_t>(buffer->GetLength());
    int32_t offsetIndex = buffer->GetLength() - 1;
    if (offset->IsNumber()) {
        if (offset->GetNumber() < -length) {
            return JSTaggedValue(-1);
        }
        if (offset->GetNumber() < length) {
            offsetIndex = GetValueInt32(offset);
            if (offsetIndex < 0) {
                offsetIndex = -offsetIndex;
                offsetIndex = length - offsetIndex;
            }
        }
    }
    offsetIndex = std::min(offsetIndex, length);
    JSHandle<JSTaggedValue> encoding = GetCallArg(argv, 2);  // 2 means the third arg
    JSAPIFastBuffer::EncodingType encodingType = JSAPIFastBuffer::UTF8;
    if (encoding->IsString()) {
        if (!IsValidEncoding(encoding)) {
            JSTaggedValue error = ContainerError::BusinessError(
                thread, ErrorFlag::TYPE_ERROR,
                "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        encodingType = JSAPIFastBuffer::GetEncodingType(JSAPIFastBuffer::GetString(encoding));
    }
    return JSAPIFastBuffer::IndexOf(thread, buffer, value, offsetIndex, encodingType, true);
}

JSTaggedValue ContainersBuffer::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Entries);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    return JSAPIFastBuffer::Entries(thread, buffer);
}

JSTaggedValue ContainersBuffer::Keys(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Keys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Keys);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    return JSAPIFastBuffer::Keys(thread, buffer);
}

JSTaggedValue ContainersBuffer::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Values);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    return JSAPIFastBuffer::Values(thread, buffer);
}

JSTaggedValue ContainersBuffer::Fill(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Fill);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Fill);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    if (buffer->GetLength() == 0) {
        return buffer.GetTaggedValue();
    }
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);  // 0 means the first arg
    if (value->IsUndefined() || value->IsNull()) {
        value = JSHandle<JSTaggedValue>(thread, JSTaggedValue(0));
    }
    JSHandle<JSTaggedValue> start = GetCallArg(argv, 1);  // 1 means the second arg
    if (start->IsString()) {
        JSAPIFastBuffer::EncodingType encodingType = JSAPIFastBuffer::UTF8;
        if (!IsValidEncoding(start)) {
            JSTaggedValue error = ContainerError::BusinessError(
                thread, ErrorFlag::TYPE_ERROR,
                "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        encodingType = JSAPIFastBuffer::GetEncodingType(JSAPIFastBuffer::GetString(start));
        return JSAPIFastBuffer::Fill(thread, buffer, value, encodingType, 0, buffer->GetLength());
    }
    JSHandle<JSTaggedValue> end = GetCallArg(argv, 2);       // 2 means the third arg
    JSHandle<JSTaggedValue> encoding = GetCallArg(argv, 3);  // 3 means the fourth arg
    uint32_t startIndex = 0;
    if (start->IsNumber()) {
        RANGE_ERROR_CHECK(start, start, 0, UINT32_MAX);
        startIndex = GetValueUInt32(start);
    }
    uint32_t endIndex = buffer->GetLength();
    if (endIndex == 0) {
        return buffer.GetTaggedValue();
    }
    if (end->IsNumber()) {
        RANGE_ERROR_CHECK(end, end, 0, UINT32_MAX);
        endIndex = GetValueUInt32(end);
    }
    JSAPIFastBuffer::EncodingType encodingType = JSAPIFastBuffer::UTF8;
    if (encoding->IsString()) {
        if (!IsValidEncoding(encoding)) {
            JSTaggedValue error = ContainerError::BusinessError(
                thread, ErrorFlag::TYPE_ERROR,
                "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        encodingType = JSAPIFastBuffer::GetEncodingType(JSAPIFastBuffer::GetString(encoding));
    }
    return JSAPIFastBuffer::Fill(thread, buffer, value, encodingType, startIndex, endIndex);
}

JSTaggedValue ContainersBuffer::Write(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Write);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Write);
    JSAPIFastBuffer::EncodingType encodingType = JSAPIFastBuffer::UTF8;
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    JSHandle<JSTaggedValue> firstArg = GetCallArg(argv, 0);  // 0 means the first arg
    CHECK_NULL_OR_UNDEFINED(firstArg);
    if (firstArg->IsString()) {
        JSHandle<JSTaggedValue> secondArg = GetCallArg(argv, 1);  // 1 means the second arg
        if (secondArg->IsUndefined() || secondArg->IsNull()) {
            return JSAPIFastBuffer::WriteString(thread, buffer, firstArg, 0, buffer->GetLength(),
                                                JSAPIFastBuffer::UTF8);
        }
        if (secondArg->IsString()) {
            if (!IsValidEncoding(secondArg)) {
                JSTaggedValue error = ContainerError::BusinessError(
                    thread, ErrorFlag::TYPE_ERROR,
                    "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
            }
            encodingType = JSAPIFastBuffer::GetEncodingType(JSAPIFastBuffer::GetString(secondArg));
            return JSAPIFastBuffer::WriteString(thread, buffer, firstArg, 0, buffer->GetLength(), encodingType);
        }
        uint32_t offset = 0;
        if (secondArg->IsNumber()) {
            RANGE_ERROR_CHECK(secondArg, offset, 0, buffer->GetLength() - 1);
            offset = GetValueUInt32(secondArg);
        }
        uint32_t maxLength = buffer->GetLength() - offset;
        JSHandle<JSTaggedValue> thirdArg = GetCallArg(argv, 2);  // 2 means the third arg
        if (thirdArg->IsNumber()) {
            RANGE_ERROR_CHECK(thirdArg, length, 0, maxLength);
            maxLength = std::min(GetValueUInt32(thirdArg), maxLength);
        }
        JSHandle<JSTaggedValue> encoding = GetCallArg(argv, 3);  // 3 means the fourth arg
        if (encoding->IsString()) {
            if (!IsValidEncoding(encoding)) {
                JSTaggedValue error = ContainerError::BusinessError(
                    thread, ErrorFlag::TYPE_ERROR,
                    "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
            }
            encodingType = JSAPIFastBuffer::GetEncodingType(JSAPIFastBuffer::GetString(encoding));
        }
        return JSAPIFastBuffer::WriteString(thread, buffer, firstArg, offset, maxLength, encodingType);
    }
    return JSTaggedValue(0);
}

JSTaggedValue ContainersBuffer::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, ToString);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(ToString);
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    JSHandle<JSTaggedValue> firstArg = GetCallArg(argv, 0);  // 0 means the first arg
    JSHandle<JSTaggedValue> start = GetCallArg(argv, 1);     // 1 means the second arg
    JSHandle<JSTaggedValue> end = GetCallArg(argv, 2);       // 2 means the third arg
    JSAPIFastBuffer::EncodingType encodingType = JSAPIFastBuffer::UTF8;
    if (firstArg->IsString()) {
        if (!IsValidEncoding(firstArg)) {
            JSTaggedValue error = ContainerError::BusinessError(
                thread, ErrorFlag::TYPE_ERROR,
                "Parameter error. The type of \"encoding\" must be BufferEncoding. the encoding code is unknown");
            THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());
        }
        encodingType = JSAPIFastBuffer::GetEncodingType(firstArg);
    }
    uint32_t startIndex = 0;
    if (start->IsNumber()) {
        if (start->GetNumber() < 0) {
            startIndex = 0;
        } else {
            startIndex = GetValueUInt32(start);
        }
    }
    uint32_t endIndex = buffer->GetLength();
    if (end->IsNumber()) {
        if (end->GetNumber() <= startIndex) {
            return thread->GlobalConstants()->GetHandledEmptyString().GetTaggedValue();
        }
        if (end->GetNumber() < endIndex) {
            endIndex = GetValueUInt32(end);
        }
    }
    auto ret = JSAPIFastBuffer::ToString(thread, buffer, encodingType, startIndex, endIndex);
    return ret;
}

JSTaggedValue ContainersBuffer::Copy(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Buffer, Copy);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(Copy);
    JSHandle<JSAPIFastBuffer> src = JSHandle<JSAPIFastBuffer>::Cast(self);
    JSHandle<JSTaggedValue> dst = GetCallArg(argv, 0);  // 0 means the first arg
    CHECK_NULL_OR_UNDEFINED(dst);
    auto targetStart = GetCallArg(argv, 1);  // 1 means the second arg
    auto sourceStart = GetCallArg(argv, 2);  // 2 means the third arg
    auto sourceEnd = GetCallArg(argv, 3);    // 3 means the fourth arg
    uint32_t tStart = 0;
    if (targetStart->IsNumber()) {
        RANGE_ERROR_CHECK(targetStart, targetStart, 0, UINT32_MAX);
        tStart = GetValueUInt32(targetStart);
    }
    uint32_t sStart = 0;
    if (sourceStart->IsNumber()) {
        RANGE_ERROR_CHECK(sourceStart, sourceStart, 0, UINT32_MAX);
        sStart = GetValueUInt32(sourceStart);
    }
    uint32_t sEnd = src->GetLength();
    if (sourceEnd->IsNumber()) {
        RANGE_ERROR_CHECK(sourceEnd, sourceEnd, 0, UINT32_MAX);
        sEnd = std::min(sEnd, GetValueUInt32(sourceEnd));
    }
    uint32_t length = src->GetLength();
    if (sEnd <= sStart || sStart >= length) {
        return JSTaggedValue(0);
    }
    return JSAPIFastBuffer::Copy(thread, dst, self, tStart, sStart, sEnd);
}

JSTaggedValue ContainersBuffer::WriteUIntBE(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Buffer, WriteUIntBE);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(WriteUIntBE);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);       // 0 means the first arg
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);      // 1 means the second arg
    JSHandle<JSTaggedValue> byteLength = GetCallArg(argv, 2);  // 2 means the third arg
    CHECK_NULL_OR_UNDEFINED(value);
    uint32_t offsetIndex = 0;
    if (offset->IsNumber()) {
        RANGE_ERROR_CHECK(offset, offset, 0, UINT32_MAX);
        offsetIndex = GetValueUInt32(offset);
    }
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto ret = JSAPIFastBuffer::WriteBytesValue(thread, buffer, value, offsetIndex,
                                                static_cast<JSAPIFastBuffer::ByteLength>(byteLength->GetInt()));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}
JSTaggedValue ContainersBuffer::WriteUIntLE(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Buffer, WriteUIntLE);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(WriteUIntLE)
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);       // 0 means the first arg
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);      // 1 means the second arg
    JSHandle<JSTaggedValue> byteLength = GetCallArg(argv, 2);  // 2 means the third arg
    CHECK_NULL_OR_UNDEFINED(value);
    uint32_t offsetIndex = 0;
    if (offset->IsInt()) {
        RANGE_ERROR_CHECK(offset, offset, 0, UINT32_MAX);
        offsetIndex = GetValueUInt32(offset);
    }
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto ret = JSAPIFastBuffer::WriteBytesValue(thread, buffer, value, offsetIndex,
                                                static_cast<JSAPIFastBuffer::ByteLength>(byteLength->GetInt()), true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}
JSTaggedValue ContainersBuffer::ReadUIntBE(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Buffer, ReadUIntBE);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(ReadUIntBE)
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 0);      // 0 means the first arg
    JSHandle<JSTaggedValue> byteLength = GetCallArg(argv, 1);  // 1 means the second arg
    CHECK_NULL_OR_UNDEFINED(offset);
    uint32_t offsetIndex = 0;
    if (offset->IsNumber()) {
        RANGE_ERROR_CHECK(offset, offset, 0, UINT32_MAX);
        offsetIndex = GetValueUInt32(offset);
    }
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto ret = JSAPIFastBuffer::ReadBytes(thread, buffer, offsetIndex,
                                          static_cast<JSAPIFastBuffer::ByteLength>(byteLength->GetInt()));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}
JSTaggedValue ContainersBuffer::ReadUIntLE(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Buffer, ReadUIntLE);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(ReadUIntLE)
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 0);      // 0 means the first arg
    JSHandle<JSTaggedValue> byteLength = GetCallArg(argv, 1);  // 1 means the second arg
    CHECK_NULL_OR_UNDEFINED(offset);
    uint32_t offsetIndex = 0;
    if (offset->IsNumber()) {
        RANGE_ERROR_CHECK(offset, offset, 0, UINT32_MAX);
        offsetIndex = GetValueUInt32(offset);
    }
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto ret = JSAPIFastBuffer::ReadBytes(thread, buffer, offsetIndex,
                                          static_cast<JSAPIFastBuffer::ByteLength>(byteLength->GetInt()), true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}

JSTaggedValue ContainersBuffer::WriteIntBE(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Buffer, WriteIntBE);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(WriteIntBE)
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);       // 0 means the first arg
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);      // 1 means the second arg
    JSHandle<JSTaggedValue> byteLength = GetCallArg(argv, 2);  // 2 means the third arg
    CHECK_NULL_OR_UNDEFINED(value);
    uint32_t offsetIndex = 0;
    if (offset->IsInt()) {
        RANGE_ERROR_CHECK(offset, offset, 0, UINT32_MAX);
        offsetIndex = GetValueUInt32(offset);
    }
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto ret = JSAPIFastBuffer::WriteBytesValue(thread, buffer, value, offsetIndex,
                                                static_cast<JSAPIFastBuffer::ByteLength>(byteLength->GetInt()));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}
JSTaggedValue ContainersBuffer::WriteIntLE(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Buffer, WriteIntLE);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(WriteIntLE)
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);       // 0 means the first arg
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 1);      // 1 means the second arg
    JSHandle<JSTaggedValue> byteLength = GetCallArg(argv, 2);  // 2 means the third arg
    CHECK_NULL_OR_UNDEFINED(value);
    uint32_t offsetIndex = 0;
    if (offset->IsInt()) {
        RANGE_ERROR_CHECK(offset, offset, 0, UINT32_MAX);
        offsetIndex = GetValueUInt32(offset);
    }
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto ret = JSAPIFastBuffer::WriteBytesValue(thread, buffer, value, offsetIndex,
                                                static_cast<JSAPIFastBuffer::ByteLength>(byteLength->GetInt()), true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}
JSTaggedValue ContainersBuffer::ReadIntBE(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Buffer, ReadIntBE);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(ReadIntBE)
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 0);      // 0 means the first arg
    JSHandle<JSTaggedValue> byteLength = GetCallArg(argv, 1);  // 1 means the second arg
    CHECK_NULL_OR_UNDEFINED(offset);
    uint32_t offsetIndex = 0;
    if (offset->IsNumber()) {
        RANGE_ERROR_CHECK(offset, offset, 0, UINT32_MAX);
        offsetIndex = GetValueUInt32(offset);
    }
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto ret = JSAPIFastBuffer::ReadInt(thread, buffer, offsetIndex,
                                        static_cast<JSAPIFastBuffer::ByteLength>(byteLength->GetInt()));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}
JSTaggedValue ContainersBuffer::ReadIntLE(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Buffer, ReadIntLE);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    CONTAINER_BUFFER_CHECK(ReadIntLE)
    JSHandle<JSTaggedValue> offset = GetCallArg(argv, 0);      // 0 means the first arg
    JSHandle<JSTaggedValue> byteLength = GetCallArg(argv, 1);  // 1 means the second arg
    CHECK_NULL_OR_UNDEFINED(offset);
    uint32_t offsetIndex = 0;
    if (offset->IsNumber()) {
        RANGE_ERROR_CHECK(offset, offset, 0, UINT32_MAX);
        offsetIndex = GetValueUInt32(offset);
    }
    JSHandle<JSAPIFastBuffer> buffer = JSHandle<JSAPIFastBuffer>::Cast(self);
    auto ret = JSAPIFastBuffer::ReadInt(thread, buffer, offsetIndex,
                                        static_cast<JSAPIFastBuffer::ByteLength>(byteLength->GetInt()), true);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return ret;
}

JSTaggedValue ContainersBuffer::WriteUInt8(EcmaRuntimeCallInfo *argv)
{
    return ContainersBuffer::WriteUInt8LE(argv);
}

JSTaggedValue ContainersBuffer::WriteInt8(EcmaRuntimeCallInfo *argv)
{
    return ContainersBuffer::WriteInt8LE(argv);
}

JSTaggedValue ContainersBuffer::ReadUInt8(EcmaRuntimeCallInfo *argv)
{
    return ContainersBuffer::ReadUInt8LE(argv);
}

JSTaggedValue ContainersBuffer::ReadInt8(EcmaRuntimeCallInfo *argv)
{
    return ContainersBuffer::ReadInt8LE(argv);
}

CONTAINER_BUFFER_ACCESSORS(UInt8)
CONTAINER_BUFFER_ACCESSORS(UInt16)
CONTAINER_BUFFER_ACCESSORS(UInt32)
CONTAINER_BUFFER_ACCESSORS(BigUInt64)
CONTAINER_BUFFER_ACCESSORS(Int8)
CONTAINER_BUFFER_ACCESSORS(Int16)
CONTAINER_BUFFER_ACCESSORS(Int32)
CONTAINER_BUFFER_ACCESSORS(BigInt64)
CONTAINER_BUFFER_ACCESSORS(Float32)
CONTAINER_BUFFER_ACCESSORS(Float64)

}  // namespace panda::ecmascript::containers
