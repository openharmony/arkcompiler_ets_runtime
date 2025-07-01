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

#include "ecmascript/builtins/builtins_dataview.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript::builtins {
// 24.2.2.1
JSTaggedValue BuiltinsDataView::DataViewConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, DataView, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> ctor = GetConstructor(argv);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    // 1. If NewTarget is undefined, throw a TypeError exception.
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "newtarget is undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> bufferHandle = GetCallArg(argv, 0);
    // 2. If Type(buffer) is not Object, throw a TypeError exception.
    if (!bufferHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "buffer is not Object", JSTaggedValue::Exception());
    }
    // 3. If buffer does not have an [[ArrayBufferData]] internal slot, throw a TypeError exception.
    if (!bufferHandle->IsArrayBuffer() && !bufferHandle->IsSharedArrayBuffer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "buffer is not ArrayBuffer", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> offsetHandle = GetCallArg(argv, 1);
    // 4. Let numberOffset be ToIndex(byteOffset).
    JSTaggedNumber offsetNumber = JSTaggedValue::ToIndex(thread, offsetHandle);
    // 6. ReturnIfAbrupt(offset).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    uint64_t offset = base::NumberHelper::DoubleToUInt64(offsetNumber.GetNumber());
    // 7. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(thread, bufferHandle.GetTaggedValue())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "buffer is Detached Buffer", JSTaggedValue::Exception());
    }
    // 8. Let bufferByteLength be the value of buffer’s [[ArrayBufferByteLength]] internal slot.
    JSHandle<JSArrayBuffer> arrBufHandle(bufferHandle);
    uint32_t bufByteLen = arrBufHandle->GetArrayBufferByteLength();
    // 9. If offset > bufferByteLength, throw a RangeError exception.
    if (offset > bufByteLen) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "offset > bufferByteLength", JSTaggedValue::Exception());
    }
    uint64_t viewByteLen = 0;
    JSHandle<JSTaggedValue> byteLenHandle = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    // 10. If byteLength is undefined, then Let viewByteLength be bufferByteLength – offset.
    if (byteLenHandle->IsUndefined()) {
        viewByteLen = bufByteLen - offset;
    } else {
        // Let viewByteLength be ToIndex(byteLength).
        JSTaggedNumber byteLen = JSTaggedValue::ToIndex(thread, byteLenHandle);
        // ReturnIfAbrupt(viewByteLength).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        viewByteLen = base::NumberHelper::DoubleToUInt64(byteLen.GetNumber());
        // If offset+viewByteLength > bufferByteLength, throw a RangeError exception.
        if (offset + viewByteLen > bufByteLen) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "offset + viewByteLen > bufByteLen", JSTaggedValue::Exception());
        }
    }
    // 11. Let O be OrdinaryCreateFromConstructor OrdinaryCreateFromConstructor(NewTarget, "%DataViewPrototype%",
    // «[[DataView]],[[ViewedArrayBuffer]], [[ByteLength]], [[ByteOffset]]» ).
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), newTarget);
    // 12. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSDataView> dataView(obj);
    // 13. Set O’s [[DataView]] internal slot to true.
    dataView->SetDataView(thread, JSTaggedValue::True());
    // 14. Set O’s [[ViewedArrayBuffer]] internal slot to buffer.
    dataView->SetViewedArrayBuffer(thread, bufferHandle.GetTaggedValue());
    // 15. Set O’s [[ByteLength]] internal slot to viewByteLength.
    dataView->SetByteLength(static_cast<uint32_t>(viewByteLen));
    // 16. Set O’s [[ByteOffset]] internal slot to offset.
    dataView->SetByteOffset(static_cast<uint32_t>(offset));
    // 17. Return O.
    return JSTaggedValue(dataView.GetTaggedValue());
}

// 24.2.4.1
JSTaggedValue BuiltinsDataView::GetBuffer(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, DataView, GetBuffer);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. f Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Type(O) is not Object", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsDataView()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "O does not have a [[ViewedArrayBuffer]]", JSTaggedValue::Exception());
    }
    JSHandle<JSDataView> dataView(thisHandle);
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = dataView->GetViewedArrayBuffer(thread);
    // 5. Return buffer.
    return JSTaggedValue(buffer);
}

// 24.2.4.2
JSTaggedValue BuiltinsDataView::GetByteLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, DataView, GetByteLength);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Type(O) is not Object", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsDataView()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "O does not have a [[ViewedArrayBuffer]]", JSTaggedValue::Exception());
    }
    JSHandle<JSDataView> dataView(thisHandle);
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = dataView->GetViewedArrayBuffer(thread);
    // 5. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(thread, buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
    }
    // 6. Let size be the value of O’s [[ByteLength]] internal slot.
    uint32_t size = dataView->GetByteLength();
    // 7. Return size.
    return JSTaggedValue(size);
}

// 24.2.4.3
JSTaggedValue BuiltinsDataView::GetOffset(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, DataView, GetOffset);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let O be the this value.
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    // 2. If Type(O) is not Object, throw a TypeError exception.
    if (!thisHandle->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Type(O) is not Object", JSTaggedValue::Exception());
    }
    // 3. If O does not have a [[ViewedArrayBuffer]] internal slot, throw a TypeError exception.
    if (!thisHandle->IsDataView()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "O does not have a [[ViewedArrayBuffer]]", JSTaggedValue::Exception());
    }
    JSHandle<JSDataView> dataView(thisHandle);
    // 4. Let buffer be the value of O’s [[ViewedArrayBuffer]] internal slot.
    JSTaggedValue buffer = dataView->GetViewedArrayBuffer(thread);
    // 5. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(thread, buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
    }
    // 6. Let offset be the value of O’s [[ByteOffset]] internal slot.
    uint32_t offset = dataView->GetByteOffset();
    // 7. Return offset.
    return JSTaggedValue(offset);
}

// 24.2.4.5
JSTaggedValue BuiltinsDataView::GetFloat32(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetFloat32);
    return GetTypedValue(argv, DataViewType::FLOAT32);
}

// 24.2.4.6
JSTaggedValue BuiltinsDataView::GetFloat64(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetFloat64);
    return GetTypedValue(argv, DataViewType::FLOAT64);
}

// 24.2.4.7
JSTaggedValue BuiltinsDataView::GetInt8(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetInt8);
    return GetTypedValue(argv, DataViewType::INT8);
}

// 24.2.4.8
JSTaggedValue BuiltinsDataView::GetInt16(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetInt16);
    return GetTypedValue(argv, DataViewType::INT16);
}

// 24.2.4.9
JSTaggedValue BuiltinsDataView::GetInt32(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetInt32);
    return GetTypedValue(argv, DataViewType::INT32);
}

// 24.2.4.10
JSTaggedValue BuiltinsDataView::GetUint8(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetUint8);
    return GetTypedValue(argv, DataViewType::UINT8);
}

// 24.2.4.11
JSTaggedValue BuiltinsDataView::GetUint16(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetUint16);
    return GetTypedValue(argv, DataViewType::UINT16);
}

// 24.2.4.12
JSTaggedValue BuiltinsDataView::GetUint32(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetUint32);
    return GetTypedValue(argv, DataViewType::UINT32);
}
// 25.3.4.5
JSTaggedValue BuiltinsDataView::GetBigInt64(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetBigInt64);
    return GetTypedValue(argv, DataViewType::BIGINT64);
}
// 25.3.4.6
JSTaggedValue BuiltinsDataView::GetBigUint64(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, GetBigUint64);
    return GetTypedValue(argv, DataViewType::BIGUINT64);
}
// 24.2.4.13
JSTaggedValue BuiltinsDataView::SetFloat32(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetFloat32);
    return SetTypedValue(argv, DataViewType::FLOAT32);
}

// 24.2.4.14
JSTaggedValue BuiltinsDataView::SetFloat64(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetFloat64);
    return SetTypedValue(argv, DataViewType::FLOAT64);
}

// 24.2.4.15
JSTaggedValue BuiltinsDataView::SetInt8(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetInt8);
    return SetTypedValue(argv, DataViewType::INT8);
}

// 24.2.4.16
JSTaggedValue BuiltinsDataView::SetInt16(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetInt16);
    return SetTypedValue(argv, DataViewType::INT16);
}

// 24.2.4.17
JSTaggedValue BuiltinsDataView::SetInt32(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetInt32);
    return SetTypedValue(argv, DataViewType::INT32);
}

// 24.2.4.18
JSTaggedValue BuiltinsDataView::SetUint8(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetUint8);
    return SetTypedValue(argv, DataViewType::UINT8);
}

// 24.2.4.19
JSTaggedValue BuiltinsDataView::SetUint16(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetUint16);
    return SetTypedValue(argv, DataViewType::UINT16);
}

// 24.2.4.20
JSTaggedValue BuiltinsDataView::SetUint32(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetUint32);
    return SetTypedValue(argv, DataViewType::UINT32);
}

// 25.3.4.15
JSTaggedValue BuiltinsDataView::SetBigInt64(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetBigInt64);
    return SetTypedValue(argv, DataViewType::BIGINT64);
}

// 25.3.4.16
JSTaggedValue BuiltinsDataView::SetBigUint64(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), DataView, SetBigUint64);
    return SetTypedValue(argv, DataViewType::BIGUINT64);
}

// 24.2.1.1
JSTaggedValue BuiltinsDataView::GetViewValue(JSThread *thread, const JSHandle<JSTaggedValue> &view,
                                             const JSHandle<JSTaggedValue> &requestIndex,
                                             const JSHandle<JSTaggedValue> &littleEndian,
                                             DataViewType type)
{
    BUILTINS_API_TRACE(thread, DataView, GetViewValue);
    // 1. If Type(view) is not Object, throw a TypeError exception.
    if (!view->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Type(O) is not Object", JSTaggedValue::Exception());
    }
    // 2. If view does not have a [[DataView]] internal slot, throw a TypeError exception.
    if (!view->IsDataView()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "view is not dataview", JSTaggedValue::Exception());
    }

    uint64_t index = 0;
    if (requestIndex->IsInt()) {
        // fast get index if requestIndex is int
        auto indexInt = requestIndex->GetInt();
        if (indexInt < 0) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "getIndex < 0", JSTaggedValue::Exception());
        }
        index = static_cast<uint64_t>(indexInt);
    } else {
        // 3. Let numberIndex be ToIndex(requestIndex).
        JSTaggedNumber numberIndex = JSTaggedValue::ToIndex(thread, requestIndex);
        // 5. ReturnIfAbrupt(getIndex).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        index = base::NumberHelper::DoubleToUInt64(numberIndex.GetNumber());
    }
    // 7. Let isLittleEndian be ToBoolean(isLittleEndian).
    bool isLittleEndian = false;
    if (littleEndian->IsUndefined()) {
        isLittleEndian = false;
    } else {
        isLittleEndian = littleEndian->ToBoolean();
    }
    // 8. Let buffer be the value of view’s [[ViewedArrayBuffer]] internal slot.
    JSHandle<JSDataView> dataView(view);
    JSTaggedValue buffer = dataView->GetViewedArrayBuffer(thread);
    // 9. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(thread, buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
    }
    // 10. Let viewOffset be the value of view’s [[ByteOffset]] internal slot.
    uint32_t offset = dataView->GetByteOffset();
    // 11. Let viewSize be the value of view’s [[ByteLength]] internal slot.
    uint32_t size = dataView->GetByteLength();
    // 12. Let elementSize be the Number value of the Element Size value specified in Table 49 for Element Type type.
    uint32_t elementSize = JSDataView::GetElementSize(type);
    // 13. If getIndex +elementSize > viewSize, throw a RangeError exception.
    if (index + elementSize > size) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "getIndex +elementSize > viewSize", JSTaggedValue::Exception());
    }
    // 14. Let bufferIndex be getIndex + viewOffset.
    uint32_t bufferIndex = static_cast<uint32_t>(index + offset);
    // 15. Return GetValueFromBuffer(buffer, bufferIndex, type, isLittleEndian).
    return BuiltinsArrayBuffer::GetValueFromBuffer(thread, buffer, bufferIndex, type, isLittleEndian);
}

// 24.2.1.2
JSTaggedValue BuiltinsDataView::SetViewValue(JSThread *thread, const JSHandle<JSTaggedValue> &view,
                                             const JSHandle<JSTaggedValue> &requestIndex,
                                             const JSHandle<JSTaggedValue> &littleEndian,
                                             DataViewType type, const JSHandle<JSTaggedValue> &value)
{
    // 1. If Type(view) is not Object, throw a TypeError exception.
    BUILTINS_API_TRACE(thread, DataView, SetViewValue);
    if (!view->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Type(O) is not Object", JSTaggedValue::Exception());
    }
    // 2. If view does not have a [[DataView]] internal slot, throw a TypeError exception.
    if (!view->IsDataView()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "view is not dataview", JSTaggedValue::Exception());
    }
    uint64_t index = 0;
    if (requestIndex->IsInt()) {
        // fast get index if requestIndex is int
        auto indexInt = requestIndex->GetInt();
        // If numberIndex ≠ getIndex or getIndex < 0, throw a RangeError exception.
        if (indexInt < 0) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "getIndex < 0", JSTaggedValue::Exception());
        }
        index = static_cast<uint64_t>(indexInt);
    } else {
        // 3. Let numberIndex be ToNumber(requestIndex).
        JSTaggedNumber numberIndex = JSTaggedValue::ToIndex(thread, requestIndex);
        // 5. ReturnIfAbrupt(getIndex).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        index = base::NumberHelper::DoubleToUInt64(numberIndex.GetNumber());
    }
    JSMutableHandle<JSTaggedValue> numValueHandle = JSMutableHandle<JSTaggedValue>(thread, value);
    if (!value->IsNumber()) {
        numValueHandle.Update(JSTaggedValue::ToNumeric(thread, value));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }
    // 7. Let isLittleEndian be ToBoolean(isLittleEndian).
    bool isLittleEndian = false;
    if (littleEndian->IsUndefined()) {
        isLittleEndian = false;
    } else {
        isLittleEndian = littleEndian->ToBoolean();
    }
    // 8. Let buffer be the value of view’s [[ViewedArrayBuffer]] internal slot.
    JSHandle<JSDataView> dataView(view);
    JSTaggedValue buffer = dataView->GetViewedArrayBuffer(thread);
    // 9. If IsDetachedBuffer(buffer) is true, throw a TypeError exception.
    if (BuiltinsArrayBuffer::IsDetachedBuffer(thread, buffer)) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Is Detached Buffer", JSTaggedValue::Exception());
    }
    // 10. Let viewOffset be the value of view’s [[ByteOffset]] internal slot.
    uint32_t offset = dataView->GetByteOffset();
    // 11. Let viewSize be the value of view’s [[ByteLength]] internal slot.
    uint32_t size = dataView->GetByteLength();
    // 12. Let elementSize be the Number value of the Element Size value specified in Table 49 for Element Type type.
    uint32_t elementSize = JSDataView::GetElementSize(type);
    // 13. If getIndex +elementSize > viewSize, throw a RangeError exception.
    if (index + elementSize > size) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "getIndex +elementSize > viewSize", JSTaggedValue::Exception());
    }
    // 14. Let bufferIndex be getIndex + viewOffset.
    uint32_t bufferIndex = static_cast<uint32_t>(index + offset);
    // 15. Return SetValueFromBuffer(buffer, bufferIndex, type, value, isLittleEndian).
    return BuiltinsArrayBuffer::SetValueInBuffer(thread, buffer, bufferIndex, type, numValueHandle, isLittleEndian);
}

JSTaggedValue BuiltinsDataView::GetTypedValue(EcmaRuntimeCallInfo *argv, DataViewType type)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, DataView, GetTypedValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSTaggedValue> offsetHandle = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> trueHandle(thread, JSTaggedValue::True());
    if (type == DataViewType::UINT8 || type == DataViewType::INT8) {
        return GetViewValue(thread, thisHandle, offsetHandle, trueHandle, type);
    }
    JSHandle<JSTaggedValue> littleEndianHandle = GetCallArg(argv, 1);
    return GetViewValue(thread, thisHandle, offsetHandle, littleEndianHandle, type);
}

JSTaggedValue BuiltinsDataView::SetTypedValue(EcmaRuntimeCallInfo *argv, DataViewType type)
{
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, DataView, SetTypedValue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSTaggedValue> offsetHandle = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> trueHandle(thread, JSTaggedValue::True());
    if (type == DataViewType::UINT8 || type == DataViewType::INT8) {
        return SetViewValue(thread, thisHandle, offsetHandle, trueHandle, type, value);
    }
    JSHandle<JSTaggedValue> littleEndianHandle = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);
    return SetViewValue(thread, thisHandle, offsetHandle, littleEndianHandle, type, value);
}
}  // namespace panda::ecmascript::builtins
