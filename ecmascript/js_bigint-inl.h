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

#ifndef ECMASCRIPT_JS_BIGINT_INL_H
#define ECMASCRIPT_JS_BIGINT_INL_H

#include "ecmascript/js_bigint.h"
#include "ecmascript/object_factory-inl.h"

namespace panda::ecmascript {
template <MemSpaceType type>
JSHandle<BigInt> BigInt::CreateBigint(JSThread *thread, uint32_t length)
{
    if (length > MAXSIZE) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Maximum BigInt size exceeded", bigint);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<BigInt> bigint = factory->NewBigInt<type>(length);
    return bigint;
}

// Create a bigint without data initialization, additional SetSign and SetData are required
template <MemSpaceType type>
JSHandle<BigInt> BigInt::CreateRawBigInt(JSThread *thread, uint32_t length)
{
    if (length > MAXSIZE) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Maximum BigInt size exceeded", bigint);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<BigInt> bigint = factory->NewBigIntWithoutInitData<type>(length);
    return bigint;
}

template <MemSpaceType type>
JSHandle<BigInt> BigInt::CreateSubBigInt(JSThread *thread, const JSHandle<BigInt>& x, uint32_t length)
{
    if (length > MAXSIZE) {
        JSHandle<BigInt> bigint(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "Maximum BigInt size exceeded", bigint);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<BigInt> bigint = factory->NewSubBigInt<type>(x, length);
    return bigint;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_BIGINT_INL_H
