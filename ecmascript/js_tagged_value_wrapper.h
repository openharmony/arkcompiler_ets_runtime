/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_TAGGED_VALUE_WRAPPER_H
#define ECMASCRIPT_JS_TAGGED_VALUE_WRAPPER_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/reference_type.h"
#ifdef USE_COMPRESSED_POINTER
#include "ecmascript/compressed_js_tagged_value.h"
#else
#include "ecmascript/dummy_compressed_js_tagged_value.h"
#endif

namespace panda::ecmascript {
#ifdef USE_COMPRESSED_POINTER
namespace __ReferenceValueTypeDispatcherImpl {
template <ReferenceType refType>
struct ReferenceValueTypeDispatcher {
    static_assert(refType == ReferenceType::NORMAL || refType == ReferenceType::COMPRESSED);

    static constexpr bool IS_COMPRESSED = refType == ReferenceType::COMPRESSED;
    using RawDataType = std::conditional_t<IS_COMPRESSED,
                                           CompressedJSTaggedType,
                                           JSTaggedType>;
    using TaggedValueType = std::conditional_t<IS_COMPRESSED,
                                               TemporaryJSTaggedValue,
                                               JSTaggedValue>;
};
}  // namespace panda::__ReferenceValueTypeDispatcherImpl

template <ReferenceType refType>
using RawDataType =
    typename __ReferenceValueTypeDispatcherImpl::ReferenceValueTypeDispatcher<refType>::RawDataType;
template <ReferenceType refType>
using TaggedValueType =
    typename __ReferenceValueTypeDispatcherImpl::ReferenceValueTypeDispatcher<refType>::TaggedValueType;
template <ReferenceType refType>
static constexpr bool ReferenceIsCompressed =
    __ReferenceValueTypeDispatcherImpl::ReferenceValueTypeDispatcher<refType>::IS_COMPRESSED;
#else
template <ReferenceType refType>
using RawDataType = JSTaggedType;
template <ReferenceType refType>
using TaggedValueType = JSTaggedValue;
template <ReferenceType refType>
static constexpr bool ReferenceIsCompressed = false;
#endif
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_TAGGED_VALUE_WRAPPER_H
