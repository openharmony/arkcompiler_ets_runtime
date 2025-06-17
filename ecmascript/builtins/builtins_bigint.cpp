/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/builtins/builtins_bigint.h"

#include "ecmascript/js_primitive_ref.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_number_format.h"
#else
#ifndef ARK_NOT_SUPPORT_INTL_GLOBAL
#include "ecmascript/intl/global_intl_helper.h"
#endif
#endif

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsBigInt::BigIntConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    // 1. If NewTarget is not undefined, throw a TypeError exception.
    if (!newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "BigInt is not a constructor", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    return BigIntConstructorInternal(thread, value);
}

JSTaggedValue BuiltinsBigInt::BigIntConstructorInternal(JSThread *thread, JSHandle<JSTaggedValue> value)
{
    BUILTINS_API_TRACE(thread, BigInt, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 2. Let prim be ? ToPrimitive(value).
    JSHandle<JSTaggedValue> Primitive(thread, JSTaggedValue::ToPrimitive(thread, value));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. If Type(prim) is Number, return ? NumberToBigInt(prim).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (Primitive->IsNumber()) {
        return BigInt::NumberToBigInt(thread, Primitive);
    }
    // 4. Otherwise, return ? ToBigInt(value).
    return JSTaggedValue::ToBigInt(thread, value);
}

JSTaggedValue BuiltinsBigInt::AsUintN(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, AsUintN);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> bits = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> bigint = GetCallArg(argv, 1);
    // 1. Let bits be ? ToIndex(bits).
    JSTaggedNumber index = JSTaggedValue::ToIndex(thread, bits);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 2. Let bigint be ? ToBigInt(bigint).
    JSTaggedValue jsBigint = JSTaggedValue::ToBigInt(thread, bigint);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> jsBigintVal(thread, jsBigint);
    // 3. Return a BigInt representing bigint modulo 2bits.
    return BigInt::AsUintN(thread, index, jsBigintVal);
}

JSTaggedValue BuiltinsBigInt::AsIntN(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, AsIntN);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> bits = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> bigint = GetCallArg(argv, 1);
    // 1. Let bits be ? ToIndex(bits).
    JSTaggedNumber index = JSTaggedValue::ToIndex(thread, bits);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 2. Let bigint be ? ToBigInt(bigint).
    JSTaggedValue jsBigint = JSTaggedValue::ToBigInt(thread, bigint);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> jsBigintVal(thread, jsBigint);
    // 3. Let mod be ℝ(bigint) modulo 2bits.
    // 4. If mod ≥ 2bits - 1, return ℤ(mod - 2bits); otherwise, return ℤ(mod).
    return BigInt::AsintN(thread, index, jsBigintVal);
}

JSTaggedValue BuiltinsBigInt::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, ToLocaleString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let x be ? ThisBigIntValue(this value).
    JSTaggedValue value = ThisBigIntValue(argv);
    [[maybe_unused]] JSHandle<JSTaggedValue> x(thread, value);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    [[maybe_unused]] bool cacheable = (locales->IsUndefined() || locales->IsString()) && options->IsUndefined();
#ifdef ARK_SUPPORT_INTL
    if (cacheable) {
        auto numberFormatter = JSNumberFormat::GetCachedIcuNumberFormatter(thread, locales);
        if (numberFormatter != nullptr) {
            JSHandle<JSTaggedValue> result = JSNumberFormat::FormatNumeric(thread, numberFormatter, x.GetTaggedValue());
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return result.GetTaggedValue();
        }
    }
    // 2. Let numberFormat be ? Construct(%NumberFormat%, « locales, options »).
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<JSFunction> ctor(ecmaVm->GetGlobalEnv()->GetNumberFormatFunction());
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(ctor);
    JSHandle<JSNumberFormat> numberFormat = JSHandle<JSNumberFormat>::Cast(obj);
    JSNumberFormat::InitializeNumberFormat(thread, numberFormat, locales, options, cacheable);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (cacheable) {
        auto numberFormatter = JSNumberFormat::GetCachedIcuNumberFormatter(thread, locales);
        ASSERT(numberFormatter != nullptr);
        JSHandle<JSTaggedValue> result = JSNumberFormat::FormatNumeric(thread, numberFormatter, x.GetTaggedValue());
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return result.GetTaggedValue();
    }

    // Return ? FormatNumeric(numberFormat, x).
    JSHandle<JSTaggedValue> result = JSNumberFormat::FormatNumeric(thread, numberFormat, x.GetTaggedValue());
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
#else
#ifdef ARK_NOT_SUPPORT_INTL_GLOBAL
    ARK_SUPPORT_INTL_RETURN_JSVALUE(thread, "LocaleCompare");
#else
    intl::GlobalIntlHelper gh(thread, intl::GlobalFormatterType::NumberFormatter);
    auto numberFormatter = gh.GetGlobalObject<intl::GlobalNumberFormat>(thread,
        locales, options, intl::GlobalFormatterType::NumberFormatter, cacheable);
    if (numberFormatter == nullptr) {
        LOG_ECMA(ERROR) << "BuiltinsBigInt:numberFormatter is nullptr";
    }
    ASSERT(numberFormatter != nullptr);
    std::string result = numberFormatter->Format(x->GetDouble());
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle returnValue = factory->NewFromStdString(result);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return returnValue.GetTaggedValue();
#endif
#endif
}

JSTaggedValue BuiltinsBigInt::ToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, ToString);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let x be ? thisBigIntValue(this value).
    JSTaggedValue value = ThisBigIntValue(argv);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> thisBigint(thread, value);
    // 2. If radix is not present, let radixNumber be 10
    double radix = base::DECIMAL;
    JSHandle<JSTaggedValue> radixValue = GetCallArg(argv, 0);
    // 3. Else, let radixNumber be ? ToIntegerOrInfinity(radix).
    if (!radixValue->IsUndefined()) {
        JSTaggedNumber radixNumber = JSTaggedValue::ToInteger(thread, radixValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        radix = radixNumber.GetNumber();
    }
    // 4. If radixNumber < 2 or radixNumber > 36, throw a RangeError exception.
    if (radix < base::MIN_RADIX || radix > base::MAX_RADIX) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "toString() radix argument must be between 2 and 36",
                                     JSTaggedValue::Exception());
    }
    // 5. If radixNumber = 10, return ToString(x).
    if (radix == base::DECIMAL) {
        return BigInt::ToString(thread, thisBigint).GetTaggedValue();
    }
    // 6. Return the String representation of this BigInt value using the radix specified by radixNumber
    return BigInt::ToString(thread, thisBigint, static_cast<int>(radix)).GetTaggedValue();
}

JSTaggedValue BuiltinsBigInt::ValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, ValueOf);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let x be ? thisBigIntValue(this value).
    return ThisBigIntValue(argv);
}

JSTaggedValue BuiltinsBigInt::ThisBigIntValue(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, BigInt, ThisBigIntValue);
    JSHandle<JSTaggedValue> value = GetThis(argv);
    // 1. If Type(value) is BigInt, return value.
    if (value->IsBigInt()) {
        return value.GetTaggedValue();
    }
    // 2. If Type(value) is Object and value has a [[BigIntData]] internal slot, then
    if (value->IsJSPrimitiveRef()) {
        JSTaggedValue primitive = JSPrimitiveRef::Cast(value->GetTaggedObject())->GetValue(thread);
        // a. Assert: Type(value.[[BigIntData]]) is BigInt.
        if (primitive.IsBigInt()) {
            // b. Return value.[[BigIntData]].
            return primitive;
        }
    }
    // 3. Throw a TypeError exception.
    THROW_TYPE_ERROR_AND_RETURN(thread, "not BigInt type", JSTaggedValue::Exception());
}
}  // namespace panda::ecmascript::builtins