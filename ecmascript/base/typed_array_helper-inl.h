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
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/base/error_type.h"
#include "ecmascript/base/typed_array_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::base {
DataViewType TypedArrayHelper::GetType(const JSHandle<JSTypedArray> &obj)
{
    JSType type = obj->GetJSHClass()->GetObjectType();
    return GetType(type);
}

DataViewType TypedArrayHelper::GetType(JSType type)
{
    switch (type) {
        case JSType::JS_INT8_ARRAY:
            return DataViewType::INT8;
        case JSType::JS_UINT8_ARRAY:
            return DataViewType::UINT8;
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return DataViewType::UINT8_CLAMPED;
        case JSType::JS_INT16_ARRAY:
            return DataViewType::INT16;
        case JSType::JS_UINT16_ARRAY:
            return DataViewType::UINT16;
        case JSType::JS_INT32_ARRAY:
            return DataViewType::INT32;
        case JSType::JS_UINT32_ARRAY:
            return DataViewType::UINT32;
        case JSType::JS_FLOAT32_ARRAY:
            return DataViewType::FLOAT32;
        case JSType::JS_FLOAT64_ARRAY:
            return DataViewType::FLOAT64;
        case JSType::JS_BIGINT64_ARRAY:
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
            return ElementSize::ONE;
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_UINT16_ARRAY:
            return ElementSize::TWO;
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_FLOAT32_ARRAY:
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
        default:
            return env->GetBigUint64ArrayFunction();
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
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_BASE_TYPED_ARRAY_HELPER_INL_H
