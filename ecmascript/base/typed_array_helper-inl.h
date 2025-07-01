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

#ifndef ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H
#define ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/builtins/builtins_sendable_arraybuffer.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/base/error_type.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::base {

#define GET_ONHEAP_HCLASS_FROM_TYPE(Type)                                                          \
JSHandle<JSHClass> TypedArrayHelper::GetOnHeapHclass##Type(JSThread *thread, JSHClass* objHclass)  \
{                                                                                                  \
    JSHandle<JSHClass> result;                                                                     \
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();                                 \
    if (*JSHandle<JSHClass>(env->Get##Type##RootHclass()) == (objHclass)) {                        \
        return JSHandle<JSHClass>(env->Get##Type##RootHclassOnHeap());                             \
    }                                                                                              \
    result = JSHClass::Clone((thread), JSHandle<JSHClass>((thread), (objHclass)));                 \
    result->SetIsOnHeap(true);                                                                     \
    return result;                                                                                 \
}

TYPED_ARRAY_TYPES(GET_ONHEAP_HCLASS_FROM_TYPE)
SHARED_TYPED_ARRAY_TYPES(GET_ONHEAP_HCLASS_FROM_TYPE)
#undef GET_ONHEAP_HCLASS_FROM_TYPE

#define GET_NOT_ONHEAP_HCLASS_FROM_TYPE(Type)                                                          \
JSHandle<JSHClass> TypedArrayHelper::GetNotOnHeapHclass##Type(JSThread *thread, JSHClass* objHclass)   \
{                                                                                                      \
    JSHandle<JSHClass> result;                                                                         \
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();                                     \
    if (*JSHandle<JSHClass>(env->Get##Type##RootHclassOnHeap()) == (objHclass)) {                      \
        return JSHandle<JSHClass>(env->Get##Type##RootHclass());                                       \
    }                                                                                                  \
    result = JSHClass::Clone((thread), JSHandle<JSHClass>((thread), (objHclass)));                     \
    result->SetIsOnHeap(false);                                                                        \
    return result;                                                                                     \
}

TYPED_ARRAY_TYPES(GET_NOT_ONHEAP_HCLASS_FROM_TYPE)
SHARED_TYPED_ARRAY_TYPES(GET_NOT_ONHEAP_HCLASS_FROM_TYPE)
#undef GET_NOT_ONHEAP_HCLASS_FROM_TYPE

template<>
struct base::BuiltinsArrayBufferType<TypedArrayKind::SHARED> {
    using Type = builtins::BuiltinsSendableArrayBuffer;
};
template<>
struct base::BuiltinsArrayBufferType<TypedArrayKind::NON_SHARED> {
    using Type = builtins::BuiltinsArrayBuffer;
};

DataViewType TypedArrayHelper::GetType(const JSHandle<JSTypedArray> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    return GetType(type);
}

DataViewType TypedArrayHelper::GetType(JSType type)
{
    switch (type) {
        case JSType::JS_INT8_ARRAY:
        case JSType::JS_SHARED_INT8_ARRAY:
            return DataViewType::INT8;
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_SHARED_UINT8_ARRAY:
            return DataViewType::UINT8;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
            return DataViewType::UINT8_CLAMPED;
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_SHARED_INT16_ARRAY:
            return DataViewType::INT16;
        case JSType::JS_UINT16_ARRAY:
        case JSType::JS_SHARED_UINT16_ARRAY:
            return DataViewType::UINT16;
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_SHARED_INT32_ARRAY:
            return DataViewType::INT32;
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_SHARED_UINT32_ARRAY:
            return DataViewType::UINT32;
        case JSType::JS_FLOAT32_ARRAY:
        case JSType::JS_SHARED_FLOAT32_ARRAY:
            return DataViewType::FLOAT32;
        case JSType::JS_FLOAT64_ARRAY:
        case JSType::JS_SHARED_FLOAT64_ARRAY:
            return DataViewType::FLOAT64;
        case JSType::JS_BIGINT64_ARRAY:
        case JSType::JS_SHARED_BIGINT64_ARRAY:
            return DataViewType::BIGINT64;
        default:
            return DataViewType::BIGUINT64;
    }
}

uint32_t TypedArrayHelper::GetElementSize(const JSHandle<JSTypedArray> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    return GetElementSize(type);
}

uint32_t TypedArrayHelper::GetElementSize(JSType type)
{
    switch (type) {
        case JSType::JS_INT8_ARRAY:
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
        case JSType::JS_SHARED_INT8_ARRAY:
        case JSType::JS_SHARED_UINT8_ARRAY:
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
            return ElementSize::ONE;
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_UINT16_ARRAY:
        case JSType::JS_SHARED_INT16_ARRAY:
        case JSType::JS_SHARED_UINT16_ARRAY:
            return ElementSize::TWO;
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_FLOAT32_ARRAY:
        case JSType::JS_SHARED_INT32_ARRAY:
        case JSType::JS_SHARED_UINT32_ARRAY:
        case JSType::JS_SHARED_FLOAT32_ARRAY:
            return ElementSize::FOUR;
        default:
            return ElementSize::EIGHT;
    }
}

JSHandle<JSTaggedValue> TypedArrayHelper::GetConstructor(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSType type = obj->GetTaggedObject()->GetClass()->GetObjectType();
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            return env->GetInt8ArrayFunction();
        case JSType::JS_UINT8_ARRAY:
            return env->GetUint8ArrayFunction();
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return env->GetUint8ClampedArrayFunction();
        case JSType::JS_INT16_ARRAY:
            return env->GetInt16ArrayFunction();
        case JSType::JS_UINT16_ARRAY:
            return env->GetUint16ArrayFunction();
        case JSType::JS_INT32_ARRAY:
            return env->GetInt32ArrayFunction();
        case JSType::JS_UINT32_ARRAY:
            return env->GetUint32ArrayFunction();
        case JSType::JS_FLOAT32_ARRAY:
            return env->GetFloat32ArrayFunction();
        case JSType::JS_FLOAT64_ARRAY:
            return env->GetFloat64ArrayFunction();
        case JSType::JS_BIGINT64_ARRAY:
            return env->GetBigInt64ArrayFunction();
        case JSType::JS_BIGUINT64_ARRAY:
            return env->GetBigUint64ArrayFunction();
        case JSType::JS_SHARED_INT8_ARRAY:
            return env->GetSharedInt8ArrayFunction();
        case JSType::JS_SHARED_UINT8_ARRAY:
            return env->GetSharedUint8ArrayFunction();
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
            return env->GetSharedUint8ClampedArrayFunction();
        case JSType::JS_SHARED_INT16_ARRAY:
            return env->GetSharedInt16ArrayFunction();
        case JSType::JS_SHARED_UINT16_ARRAY:
            return env->GetSharedUint16ArrayFunction();
        case JSType::JS_SHARED_INT32_ARRAY:
            return env->GetSharedInt32ArrayFunction();
        case JSType::JS_SHARED_UINT32_ARRAY:
            return env->GetSharedUint32ArrayFunction();
        case JSType::JS_SHARED_FLOAT32_ARRAY:
            return env->GetSharedFloat32ArrayFunction();
        case JSType::JS_SHARED_FLOAT64_ARRAY:
            return env->GetSharedFloat64ArrayFunction();
        case JSType::JS_SHARED_BIGINT64_ARRAY:
            return env->GetSharedBigInt64ArrayFunction();
        default:
            return env->GetSharedBigUint64ArrayFunction();
    }
}

JSHandle<JSFunction> TypedArrayHelper::GetConstructorFromType(JSThread *thread, const DataViewType arrayType)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    switch (arrayType) {
        case DataViewType::INT8:
            return JSHandle<JSFunction>(env->GetInt8ArrayFunction());
        case DataViewType::UINT8:
            return JSHandle<JSFunction>(env->GetUint8ArrayFunction());
        case DataViewType::UINT8_CLAMPED:
            return JSHandle<JSFunction>(env->GetUint8ClampedArrayFunction());
        case DataViewType::INT16:
            return JSHandle<JSFunction>(env->GetInt16ArrayFunction());
        case DataViewType::UINT16:
            return JSHandle<JSFunction>(env->GetUint16ArrayFunction());
        case DataViewType::INT32:
            return JSHandle<JSFunction>(env->GetInt32ArrayFunction());
        case DataViewType::UINT32:
            return JSHandle<JSFunction>(env->GetUint32ArrayFunction());
        case DataViewType::FLOAT32:
            return JSHandle<JSFunction>(env->GetFloat32ArrayFunction());
        case DataViewType::FLOAT64:
            return JSHandle<JSFunction>(env->GetFloat64ArrayFunction());
        case DataViewType::BIGINT64:
            return JSHandle<JSFunction>(env->GetBigInt64ArrayFunction());
        default:
            break;
    }
    return JSHandle<JSFunction>(env->GetBigUint64ArrayFunction());
}

JSHandle<JSFunction> TypedArrayHelper::GetSharedConstructorFromType(JSThread *thread, const DataViewType arrayType)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    switch (arrayType) {
        case DataViewType::INT8:
            return JSHandle<JSFunction>(env->GetSharedInt8ArrayFunction());
        case DataViewType::UINT8:
            return JSHandle<JSFunction>(env->GetSharedUint8ArrayFunction());
        case DataViewType::UINT8_CLAMPED:
            return JSHandle<JSFunction>(env->GetSharedUint8ClampedArrayFunction());
        case DataViewType::INT16:
            return JSHandle<JSFunction>(env->GetSharedInt16ArrayFunction());
        case DataViewType::UINT16:
            return JSHandle<JSFunction>(env->GetSharedUint16ArrayFunction());
        case DataViewType::INT32:
            return JSHandle<JSFunction>(env->GetSharedInt32ArrayFunction());
        case DataViewType::UINT32:
            return JSHandle<JSFunction>(env->GetSharedUint32ArrayFunction());
        case DataViewType::FLOAT32:
            return JSHandle<JSFunction>(env->GetSharedFloat32ArrayFunction());
        case DataViewType::FLOAT64:
            return JSHandle<JSFunction>(env->GetSharedFloat64ArrayFunction());
        case DataViewType::BIGINT64:
            return JSHandle<JSFunction>(env->GetSharedBigInt64ArrayFunction());
        default:
            break;
    }
    return JSHandle<JSFunction>(env->GetSharedBigUint64ArrayFunction());
}

JSHandle<JSTaggedValue> TypedArrayHelper::GetConstructorNameFromType(JSThread *thread, const DataViewType arrayType)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    switch (arrayType) {
        case DataViewType::INT8:
            return globalConst->GetHandledInt8ArrayString();
        case DataViewType::UINT8:
            return globalConst->GetHandledUint8ArrayString();
        case DataViewType::UINT8_CLAMPED:
            return globalConst->GetHandledUint8ClampedArrayString();
        case DataViewType::INT16:
            return globalConst->GetHandledInt16ArrayString();
        case DataViewType::UINT16:
            return globalConst->GetHandledUint16ArrayString();
        case DataViewType::INT32:
            return globalConst->GetHandledInt32ArrayString();
        case DataViewType::UINT32:
            return globalConst->GetHandledUint32ArrayString();
        case DataViewType::FLOAT32:
            return globalConst->GetHandledFloat32ArrayString();
        case DataViewType::FLOAT64:
            return globalConst->GetHandledFloat64ArrayString();
        case DataViewType::BIGINT64:
            return globalConst->GetHandledBigInt64ArrayString();
        default:
            break;
    }
    return globalConst->GetHandledBigInt64ArrayString();
}

JSHandle<JSTaggedValue> TypedArrayHelper::GetSharedConstructorNameFromType(JSThread *thread,
    const DataViewType arrayType)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    switch (arrayType) {
        case DataViewType::INT8:
            return globalConst->GetHandledSharedInt8ArrayString();
        case DataViewType::UINT8:
            return globalConst->GetHandledSharedUint8ArrayString();
        case DataViewType::UINT8_CLAMPED:
            return globalConst->GetHandledSharedUint8ClampedArrayString();
        case DataViewType::INT16:
            return globalConst->GetHandledSharedInt16ArrayString();
        case DataViewType::UINT16:
            return globalConst->GetHandledSharedUint16ArrayString();
        case DataViewType::INT32:
            return globalConst->GetHandledSharedInt32ArrayString();
        case DataViewType::UINT32:
            return globalConst->GetHandledSharedUint32ArrayString();
        case DataViewType::FLOAT32:
            return globalConst->GetHandledSharedFloat32ArrayString();
        case DataViewType::FLOAT64:
            return globalConst->GetHandledSharedFloat64ArrayString();
        case DataViewType::BIGINT64:
            return globalConst->GetHandledSharedBigInt64ArrayString();
        default:
            break;
    }
    return globalConst->GetHandledSharedBigInt64ArrayString();
}

JSHandle<JSHClass> TypedArrayHelper::GetOnHeapHclassFromType(
    JSThread *thread, const JSHandle<JSTypedArray> &obj, const DataViewType arrayType)
{
    JSHClass* objHclass = JSHandle<TaggedObject>(obj)->GetClass();
    ASSERT_PRINT(!objHclass->IsOnHeapFromBitField(), "must be not on heap");
    switch (arrayType) {
        case DataViewType::INT8:
            return TypedArrayHelper::GetOnHeapHclassInt8Array(thread, objHclass);
        case DataViewType::UINT8:
            return TypedArrayHelper::GetOnHeapHclassUint8Array(thread, objHclass);
        case DataViewType::UINT8_CLAMPED:
            return TypedArrayHelper::GetOnHeapHclassUint8ClampedArray(thread, objHclass);
        case DataViewType::INT16:
            return TypedArrayHelper::GetOnHeapHclassInt16Array(thread, objHclass);
        case DataViewType::UINT16:
            return TypedArrayHelper::GetOnHeapHclassUint16Array(thread, objHclass);
        case DataViewType::INT32:
            return TypedArrayHelper::GetOnHeapHclassInt32Array(thread, objHclass);
        case DataViewType::UINT32:
            return TypedArrayHelper::GetOnHeapHclassUint32Array(thread, objHclass);
        case DataViewType::FLOAT32:
            return TypedArrayHelper::GetOnHeapHclassFloat32Array(thread, objHclass);
        case DataViewType::FLOAT64:
            return TypedArrayHelper::GetOnHeapHclassFloat64Array(thread, objHclass);
        case DataViewType::BIGINT64:
            return TypedArrayHelper::GetOnHeapHclassBigInt64Array(thread, objHclass);
        default:
            break;
    }
    return TypedArrayHelper::GetOnHeapHclassBigUint64Array(thread, objHclass);
}

JSHandle<JSHClass> TypedArrayHelper::GetSharedOnHeapHclassFromType(
    JSThread *thread, const JSHandle<JSTypedArray> &obj, const DataViewType arrayType)
{
    JSHClass* objHclass = JSHandle<TaggedObject>(obj)->GetClass();
    ASSERT_PRINT(!objHclass->IsOnHeapFromBitField(), "must be not on heap");
    switch (arrayType) {
        case DataViewType::INT8:
            return TypedArrayHelper::GetOnHeapHclassSharedInt8Array(thread, objHclass);
        case DataViewType::UINT8:
            return TypedArrayHelper::GetOnHeapHclassSharedUint8Array(thread, objHclass);
        case DataViewType::UINT8_CLAMPED:
            return TypedArrayHelper::GetOnHeapHclassSharedUint8ClampedArray(thread, objHclass);
        case DataViewType::INT16:
            return TypedArrayHelper::GetOnHeapHclassSharedInt16Array(thread, objHclass);
        case DataViewType::UINT16:
            return TypedArrayHelper::GetOnHeapHclassSharedUint16Array(thread, objHclass);
        case DataViewType::INT32:
            return TypedArrayHelper::GetOnHeapHclassSharedInt32Array(thread, objHclass);
        case DataViewType::UINT32:
            return TypedArrayHelper::GetOnHeapHclassSharedUint32Array(thread, objHclass);
        case DataViewType::FLOAT32:
            return TypedArrayHelper::GetOnHeapHclassSharedFloat32Array(thread, objHclass);
        case DataViewType::FLOAT64:
            return TypedArrayHelper::GetOnHeapHclassSharedFloat64Array(thread, objHclass);
        case DataViewType::BIGINT64:
            return TypedArrayHelper::GetOnHeapHclassSharedBigInt64Array(thread, objHclass);
        default:
            break;
    }
    return TypedArrayHelper::GetOnHeapHclassSharedBigUint64Array(thread, objHclass);
}

JSHandle<JSHClass> TypedArrayHelper::GetNotOnHeapHclassFromType(
    JSThread *thread, const JSHandle<JSTypedArray> &obj, const DataViewType arrayType)
{
    JSHClass* objHclass = JSHandle<TaggedObject>(obj)->GetClass();
    ASSERT_PRINT(objHclass->IsOnHeapFromBitField(), "must be on heap");
    switch (arrayType) {
        case DataViewType::INT8:
            return TypedArrayHelper::GetNotOnHeapHclassInt8Array(thread, objHclass);
        case DataViewType::UINT8:
            return TypedArrayHelper::GetNotOnHeapHclassUint8Array(thread, objHclass);
        case DataViewType::UINT8_CLAMPED:
            return TypedArrayHelper::GetNotOnHeapHclassUint8ClampedArray(thread, objHclass);
        case DataViewType::INT16:
            return TypedArrayHelper::GetNotOnHeapHclassInt16Array(thread, objHclass);
        case DataViewType::UINT16:
            return TypedArrayHelper::GetNotOnHeapHclassUint16Array(thread, objHclass);
        case DataViewType::INT32:
            return TypedArrayHelper::GetNotOnHeapHclassInt32Array(thread, objHclass);
        case DataViewType::UINT32:
            return TypedArrayHelper::GetNotOnHeapHclassUint32Array(thread, objHclass);
        case DataViewType::FLOAT32:
            return TypedArrayHelper::GetNotOnHeapHclassFloat32Array(thread, objHclass);
        case DataViewType::FLOAT64:
            return TypedArrayHelper::GetNotOnHeapHclassFloat64Array(thread, objHclass);
        case DataViewType::BIGINT64:
            return TypedArrayHelper::GetNotOnHeapHclassBigInt64Array(thread, objHclass);
        default:
            break;
    }
    return TypedArrayHelper::GetNotOnHeapHclassBigUint64Array(thread, objHclass);
}

JSHandle<JSHClass> TypedArrayHelper::GetSharedNotOnHeapHclassFromType(
    JSThread *thread, const JSHandle<JSSharedTypedArray> &obj, const DataViewType arrayType)
{
    JSHClass* objHclass = JSHandle<TaggedObject>(obj)->GetClass();
    ASSERT_PRINT(objHclass->IsOnHeapFromBitField(), "must be on heap");
    switch (arrayType) {
        case DataViewType::INT8:
            return TypedArrayHelper::GetNotOnHeapHclassSharedInt8Array(thread, objHclass);
        case DataViewType::UINT8:
            return TypedArrayHelper::GetNotOnHeapHclassSharedUint8Array(thread, objHclass);
        case DataViewType::UINT8_CLAMPED:
            return TypedArrayHelper::GetNotOnHeapHclassSharedUint8ClampedArray(thread, objHclass);
        case DataViewType::INT16:
            return TypedArrayHelper::GetNotOnHeapHclassSharedInt16Array(thread, objHclass);
        case DataViewType::UINT16:
            return TypedArrayHelper::GetNotOnHeapHclassSharedUint16Array(thread, objHclass);
        case DataViewType::INT32:
            return TypedArrayHelper::GetNotOnHeapHclassSharedInt32Array(thread, objHclass);
        case DataViewType::UINT32:
            return TypedArrayHelper::GetNotOnHeapHclassSharedUint32Array(thread, objHclass);
        case DataViewType::FLOAT32:
            return TypedArrayHelper::GetNotOnHeapHclassSharedFloat32Array(thread, objHclass);
        case DataViewType::FLOAT64:
            return TypedArrayHelper::GetNotOnHeapHclassSharedFloat64Array(thread, objHclass);
        case DataViewType::BIGINT64:
            return TypedArrayHelper::GetNotOnHeapHclassSharedBigInt64Array(thread, objHclass);
        default:
            break;
    }
    return TypedArrayHelper::GetNotOnHeapHclassSharedBigUint64Array(thread, objHclass);
}

uint32_t TypedArrayHelper::GetSizeFromType(const DataViewType arrayType)
{
    if (arrayType == DataViewType::INT8 ||
        arrayType == DataViewType::UINT8 ||
        arrayType == DataViewType::UINT8_CLAMPED) {
        return ElementSize::ONE;
    }

    if (arrayType == DataViewType::INT16 ||
        arrayType == DataViewType::UINT16) {
        return ElementSize::TWO;
    }

    if (arrayType == DataViewType::FLOAT32 ||
        arrayType == DataViewType::UINT32 ||
        arrayType == DataViewType::INT32) {
        return ElementSize::FOUR;
    }

    return ElementSize::EIGHT;
}

bool TypedArrayHelper::IsAccessorHasChanged(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (obj->IsHeapObject()) {
        JSTaggedValue markerValue = obj->GetTaggedObject()->GetClass()->GetProtoChangeMarker(thread);
        if (markerValue.IsProtoChangeMarker()) {
            return ProtoChangeMarker::Cast(markerValue.GetTaggedObject())->GetAccessorHasChanged();
        }
    }
    return false;
}
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H
