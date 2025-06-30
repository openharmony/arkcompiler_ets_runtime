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

#include "ecmascript/dynamic_type_converter.h"
#include "ecmascript/js_bigint-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "common_interfaces/objects/base_object_dispatcher.h"
#include "common_interfaces/objects/base_type.h"

namespace panda::ecmascript {

DynamicTypeConverter DynamicTypeConverter::dynTypeConverter_;

void DynamicTypeConverter::Initialize()
{
    common::BaseObjectDispatcher::GetDispatcher().RegisterDynamicTypeConverter(&dynTypeConverter_);
}

JSTaggedValue DynamicTypeConverter::WrapTagged(ThreadHolder *thread, BaseType value)
{
    JSTaggedValue result;
    std::visit(
        [&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                result = JSTaggedValue::Hole();
            } else if constexpr (std::is_same_v<T, bool>) {
                result = JSTaggedValue(arg);
            } else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
                result = JSTaggedValue(static_cast<double>(arg));
            } else if constexpr (std::is_same_v<T, uint32_t>) {
                result = JSTaggedValue(arg);
            } else if constexpr (std::is_integral_v<T>) {
                result = JSTaggedValue(static_cast<int32_t>(arg));
            } else if constexpr (std::is_floating_point_v<T>) {
                result = JSTaggedValue(static_cast<double>(arg));
            } else if constexpr (std::is_same_v<T, common::BaseUndefined>) {
                result = JSTaggedValue::Undefined();
            } else if constexpr (std::is_same_v<T, common::BaseNull>) {
                result = JSTaggedValue::Null();
            } else if constexpr (std::is_same_v<T, common::BaseBigInt>) {
                BigInt* bigInt = *BigInt::CreateBigint(thread->GetJSThread(), arg.length);
                bigInt->SetSign(arg.sign);
                for (uint32_t i = 0; i < arg.length; i++) {
                    bigInt->SetDigit(i, arg.data[i]);
                }
                result = JSTaggedValue(static_cast<TaggedObject*>(bigInt));
            } else if constexpr (std::is_same_v<T, BaseObject*>) {
                result = JSTaggedValue(static_cast<TaggedObject*>(arg));
            } else if constexpr (std::is_same_v<T, BaseString*>) {
                result = JSTaggedValue(EcmaString::FromBaseString(arg));
            } else {
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
            }
        },
        value);
    return result;
}

BaseType DynamicTypeConverter::UnWrapTagged(JSTaggedValue value)
{
    if (value.IsBoolean()) {
        return value.ToBoolean();
    } else if (value.IsInt()) {
        return static_cast<int32_t>(value.GetInt());
    } else if (value.IsDouble()) {
        return static_cast<double>(value.GetDouble());
    } else if (value.IsUndefined()) {
        return common::BaseUndefined();
    } else if (value.IsNull()) {
        return common::BaseNull();
    } else if (value.IsBigInt()) {
        BigInt *bigInt = BigInt::Cast(value.GetTaggedObject());
        common::BaseBigInt baseBigInt;
        baseBigInt.length = bigInt->GetLength();
        baseBigInt.sign = bigInt->GetSign();
        baseBigInt.data.resize(baseBigInt.length);
        for (uint32_t i = 0; i < baseBigInt.length; i++) {
            baseBigInt.data[i] = bigInt->GetDigit(i);
        }
        return baseBigInt;
    } else if (value.IsString()) {
        return EcmaString::Cast(value)->ToBaseString();
    } else if (value.IsHeapObject()) {
        return static_cast<BaseObject*>(value.GetTaggedObject());
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
    return std::monostate();
}
}
