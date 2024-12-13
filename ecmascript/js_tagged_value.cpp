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

#include "ecmascript/js_tagged_value.h"

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/symbol_table.h"

namespace panda::ecmascript {

bool JSTaggedValue::ToBoolean() const
{
    if (IsInt()) {
        return GetInt() != 0;
    }
    if (IsDouble()) {
        double d = GetDouble();
        return !std::isnan(d) && d != 0;
    }
    switch (GetRawData()) {
        case JSTaggedValue::VALUE_UNDEFINED:
            [[fallthrough]];
        case JSTaggedValue::VALUE_HOLE:
            [[fallthrough]];
        case JSTaggedValue::VALUE_NULL:
            [[fallthrough]];
        case JSTaggedValue::VALUE_FALSE: {
            return false;
        }
        case JSTaggedValue::VALUE_TRUE: {
            return true;
        }
        default: {
            break;
        }
    }

    if (IsBigInt()) {
        BigInt *bigint = BigInt::Cast(GetTaggedObject());
        return !bigint->IsZero();
    }
    if (IsHeapObject()) {
        TaggedObject *obj = GetTaggedObject();
        if (IsString()) {
            auto str = static_cast<EcmaString *>(obj);
            return EcmaStringAccessor(str).GetLength() != 0;
        }
        return true;
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

JSTaggedNumber JSTaggedValue::ToNumber(JSThread *thread, JSTaggedValue tagged)
{
    {
        DISALLOW_GARBAGE_COLLECTION;
        if (tagged.IsInt() || tagged.IsDouble()) {
            return JSTaggedNumber(tagged);
        }
        switch (tagged.GetRawData()) {
            case JSTaggedValue::VALUE_UNDEFINED:
            case JSTaggedValue::VALUE_HOLE: {
                return JSTaggedNumber(base::NAN_VALUE);
            }
            case JSTaggedValue::VALUE_TRUE: {
                return JSTaggedNumber(1);
            }
            case JSTaggedValue::VALUE_FALSE:
            case JSTaggedValue::VALUE_NULL: {
                return JSTaggedNumber(0);
            }
            default: {
                break;
            }
        }
        if (tagged.IsString()) {
            return StringToNumber(tagged);
        }
    }
    if (tagged.IsECMAObject()) {
        JSHandle<JSTaggedValue>taggedHandle(thread, tagged);
        JSTaggedValue primValue = ToPrimitive(thread, taggedHandle, PREFER_NUMBER);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());
        return ToNumber(thread, primValue);
    }
    if (tagged.IsSymbol()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Symbol value to a number", JSTaggedNumber::Exception());
    }
    if (tagged.IsBigInt()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a BigInt value to a number", JSTaggedNumber::Exception());
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Unknown value to a number", JSTaggedNumber::Exception());
}

JSTaggedNumber JSTaggedValue::ToNumber(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    return ToNumber(thread, tagged.GetTaggedValue());
}

JSTaggedValue JSTaggedValue::ToBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSHandle<JSTaggedValue> primValue(thread, ToPrimitive(thread, tagged));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    switch (primValue->GetRawData()) {
        case JSTaggedValue::VALUE_UNDEFINED:
        case JSTaggedValue::VALUE_NULL: {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a undefine or null value to a BigInt",
                                        JSTaggedValue::Exception());
        }
        case JSTaggedValue::VALUE_TRUE: {
            return BigInt::Int32ToBigInt(thread, 1).GetTaggedValue();
        }
        case JSTaggedValue::VALUE_FALSE: {
            return BigInt::Int32ToBigInt(thread, 0).GetTaggedValue();
        }
        default: {
            break;
        }
    }

    if (primValue->IsNumber()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Number value to a BigInt", JSTaggedNumber::Exception());
    }
    if (primValue->IsString()) {
        JSHandle<JSTaggedValue> value(thread, base::NumberHelper::StringToBigInt(thread, primValue));
        if (value->IsBigInt()) {
            return value.GetTaggedValue();
        }
        THROW_SYNTAX_ERROR_AND_RETURN(thread, "Cannot convert string to a BigInt,"
                                      "because not allow Infinity, decimal points, or exponents",
                                      JSTaggedValue::Exception());
    }
    if (primValue->IsSymbol()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Symbol value to a BigInt", JSTaggedNumber::Exception());
    }
    if (primValue->IsBigInt()) {
        return primValue.GetTaggedValue();
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Unknown value to a BigInt", JSTaggedNumber::Exception());
}

JSTaggedValue JSTaggedValue::ToBigInt64(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSHandle<BigInt> value(thread, ToBigInt(thread, tagged));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> tVal = BigInt::GetUint64MaxBigInt(thread);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<BigInt> int64bitVal = BigInt::FloorMod(thread, value, tVal);
    JSHandle<BigInt> resValue = BigInt::GetInt64MaxBigInt(thread);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (!BigInt::LessThan(int64bitVal.GetTaggedValue(), resValue.GetTaggedValue())) {
        return BigInt::Subtract(thread, int64bitVal, tVal).GetTaggedValue();
    } else {
        return int64bitVal.GetTaggedValue();
    }
}

JSTaggedValue JSTaggedValue::ToBigUint64(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSHandle<BigInt> value(thread, ToBigInt(thread, tagged));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    bool signFlag = value->GetSign();
    uint32_t len = value->GetLength();
    if (!signFlag && len <= 2) { // 2:2 int equal int64
        return value.GetTaggedValue();
    }
    JSHandle<BigInt> tVal = BigInt::GetUint64MaxBigInt(thread);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return BigInt::FloorMod(thread, value, tVal).GetTaggedValue();
}

JSTaggedNumber JSTaggedValue::ToInteger(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());

    return JSTaggedNumber(base::NumberHelper::TruncateDouble(number.GetNumber()));
}

int32_t JSTaggedValue::ToInt32(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);
    return base::NumberHelper::DoubleToInt(number.GetNumber(), base::INT32_BITS);
}

uint32_t JSTaggedValue::ToUint32(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    return ToInt32(thread, tagged);
}

int16_t JSTaggedValue::ToInt16(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);

    return base::NumberHelper::DoubleToInt(number.GetNumber(), base::INT16_BITS);
}

uint16_t JSTaggedValue::ToUint16(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    return ToInt16(thread, tagged);
}

int8_t JSTaggedValue::ToInt8(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);

    return base::NumberHelper::DoubleToInt(number.GetNumber(), base::INT8_BITS);
}

uint8_t JSTaggedValue::ToUint8(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    return ToInt8(thread, tagged);
}

uint8_t JSTaggedValue::ToUint8Clamp(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber number = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, 0);

    double d = number.GetNumber();
    if (std::isnan(d) || d <= 0) {
        return 0;
    }
    if (d >= UINT8_MAX) {
        return UINT8_MAX;
    }

    return lrint(d);
}

JSTaggedNumber JSTaggedValue::ToLength(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSTaggedNumber len = ToInteger(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());
    if (len.GetNumber() < 0.0) {
        return JSTaggedNumber(static_cast<double>(0));
    }
    if (len.GetNumber() > SAFE_NUMBER) {
        return JSTaggedNumber(static_cast<double>(SAFE_NUMBER));
    }
    return len;
}

// ecma6 7.2 Testing and Comparison Operations
JSHandle<JSTaggedValue> JSTaggedValue::RequireObjectCoercible(JSThread *thread,
                                                              const JSHandle<JSTaggedValue> &tagged,
                                                              const char *message)
{
    if (tagged->IsUndefinedOrNull()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, message, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    return tagged;
}

JSHandle<EcmaString> GetTypeString(JSThread *thread, PreferredPrimitiveType type)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (type == NO_PREFERENCE) {
        return JSHandle<EcmaString>::Cast(globalConst->GetHandledDefaultString());
    }
    if (type == PREFER_NUMBER) {
        return JSHandle<EcmaString>::Cast(globalConst->GetHandledNumberString());
    }
    return JSHandle<EcmaString>::Cast(globalConst->GetHandledStringString());
}

bool JSTaggedValue::IsInSharedHeap() const
{
    if (IsHeapObject()) {
        Region *region = Region::ObjectAddressToRange(value_);
        return region->InSharedHeap();
    }
    return false;
}

JSHandle<JSTaggedValue> JSTaggedValue::ToPropertyKey(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsStringOrSymbol() || tagged->IsNumber()) {
        return tagged;
    }
    JSHandle<JSTaggedValue> key(thread, ToPrimitive(thread, tagged, PREFER_STRING));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    if (key->IsSymbol()) {
        return key;
    }
    JSHandle<EcmaString> string = ToString(thread, key);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    return JSHandle<JSTaggedValue>::Cast(string);
}

bool JSTaggedValue::IsInteger() const
{
    if (!IsNumber()) {
        return false;
    }

    if (IsInt()) {
        return true;
    }

    double thisValue = GetDouble();
    // If argument is NaN, +∞, or -∞, return false.
    if (!std::isfinite(thisValue)) {
        return false;
    }

    // If floor(abs(argument)) ≠ abs(argument), return false.
    if (std::floor(std::abs(thisValue)) != std::abs(thisValue)) {
        return false;
    }

    return true;
}

bool JSTaggedValue::IsJSCOWArray() const
{
    // Elements of JSArray are shared and properties are not yet.
    return IsJSArray() && JSArray::Cast(GetTaggedObject())->GetElements().IsCOWArray();
}

bool JSTaggedValue::IsStableJSArray(JSThread *thread) const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsStableJSArray() &&
           !thread->IsArrayPrototypeChangedGuardiansInvalid() &&
           !GetTaggedObject()->GetClass()->IsJSArrayPrototypeModifiedFromBitField();
}

bool JSTaggedValue::IsStableJSArguments(JSThread *thread) const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsStableJSArguments() &&
           !thread->IsArrayPrototypeChangedGuardiansInvalid();
}

bool JSTaggedValue::IsTaggedArray() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsTaggedArray();
}

bool JSTaggedValue::IsJSProxy() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsJSProxy();
}

bool JSTaggedValue::IsGeneratorObject() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsGeneratorObject();
}

bool JSTaggedValue::IsGeneratorContext() const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsGeneratorContext();
}

bool JSTaggedValue::HasStableElements(JSThread *thread) const
{
    return IsHeapObject() && GetTaggedObject()->GetClass()->IsStableElements() &&
           !thread->IsArrayPrototypeChangedGuardiansInvalid();
}

bool JSTaggedValue::WithinInt32() const
{
    if (!IsNumber()) {
        return false;
    }

    double doubleValue = GetNumber();
    if (base::bit_cast<int64_t>(doubleValue) == base::bit_cast<int64_t>(-0.0)) {
        return false;
    }

    int32_t intvalue = base::NumberHelper::DoubleToInt(doubleValue, base::INT32_BITS);
    return doubleValue == static_cast<double>(intvalue);
}

bool JSTaggedValue::IsZero() const
{
    if (GetRawData() == VALUE_ZERO) {
        return true;
    }
    if (IsDouble()) {
        const double limit = 1e-8;
        return (std::abs(GetDouble() - 0.0) <= limit);
    }
    return false;
}

bool JSTaggedValue::Equal(JSThread *thread, const JSHandle<JSTaggedValue> &x, const JSHandle<JSTaggedValue> &y)
{
    if (x->IsNumber()) {
        return EqualNumber(thread, x, y);
    }

    if (x->IsString()) {
        return EqualString(thread, x, y);
    }

    if (x->IsBoolean()) {
        JSTaggedNumber xNumber = ToNumber(thread, x);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return Equal(thread, JSHandle<JSTaggedValue>(thread, xNumber), y);
    }

    if (x->IsSymbol()) {
        return EqualSymbol(thread, x, y);
    }

    if (x->IsBigInt()) {
        return EqualBigInt(thread, x, y);
    }

    if (x->IsHeapObject()) {
        return EqualHeapObject(thread, x, y);
    }

    return EqualNullOrUndefined(x, y);
}

bool JSTaggedValue::EqualNumber(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                const JSHandle<JSTaggedValue> &y)
{
    if (y->IsNumber()) {
        return StrictNumberEquals(x->ExtractNumber(), y->ExtractNumber());
    }
    if (y->IsString()) {
            JSTaggedNumber yNumber = ToNumber(thread, y);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return StrictNumberEquals(x->ExtractNumber(), yNumber.GetNumber());
    }
    if (y->IsBoolean()) {
        JSTaggedNumber yNumber = ToNumber(thread, y);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return StrictNumberEquals(x->ExtractNumber(), yNumber.GetNumber());
    }
    if (y->IsBigInt()) {
        return Equal(thread, y, x);
    }
    if (y->IsECMAObject()) {
        JSHandle<JSTaggedValue> yPrimitive(thread, ToPrimitive(thread, y));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return Equal(thread, x, yPrimitive);
    }
    return false;
}

bool JSTaggedValue::EqualString(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                const JSHandle<JSTaggedValue> &y)
{
    if (y->IsString()) {
        return EcmaStringAccessor::StringsAreEqual(thread->GetEcmaVM(),
                                                   JSHandle<EcmaString>(x),
                                                   JSHandle<EcmaString>(y));
    }
    if (y->IsNumber()) {
        JSTaggedNumber xNumber = ToNumber(thread, x);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return StrictNumberEquals(xNumber.GetNumber(), y->ExtractNumber());
    }
    if (y->IsBoolean()) {
        JSTaggedNumber xNumber = ToNumber(thread, x);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        JSTaggedNumber yNumber = ToNumber(thread, y);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return StrictNumberEquals(xNumber.GetNumber(), yNumber.GetNumber());
    }
    if (y->IsBigInt()) {
        return Equal(thread, y, x);
    }
    if (y->IsECMAObject()) {
        JSHandle<JSTaggedValue> yPrimitive(thread, ToPrimitive(thread, y));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return Equal(thread, x, yPrimitive);
    }
    return false;
}

bool JSTaggedValue::EqualSymbol(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                const JSHandle<JSTaggedValue> &y)
{
    if (y->IsSymbol()) {
        return x.GetTaggedValue() == y.GetTaggedValue();
    }
    if (y->IsBigInt() || y->IsString()) {
        return false;
    }
    if (y->IsECMAObject()) {
        JSHandle<JSTaggedValue> yPrimitive(thread, ToPrimitive(thread, y));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return Equal(thread, x, yPrimitive);
    }
    return false;
}

bool JSTaggedValue::EqualBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                const JSHandle<JSTaggedValue> &y)
{
    if (y->IsBigInt()) {
        return BigInt::Equal(x.GetTaggedValue(), y.GetTaggedValue());
    }
    if (y->IsString()) {
        JSHandle<JSTaggedValue> yNumber(thread, base::NumberHelper::StringToBigInt(thread, y));
        if (!yNumber->IsBigInt()) {
            return false;
        }
        return BigInt::Equal(x.GetTaggedValue(), yNumber.GetTaggedValue());
    }
    if (y->IsBoolean()) {
        JSHandle<JSTaggedValue> yNumber(thread, ToBigInt(thread, y));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return BigInt::Equal(x.GetTaggedValue(), yNumber.GetTaggedValue());
    }
    if (y->IsNumber()) {
        JSHandle<BigInt> bigint = JSHandle<BigInt>::Cast(x);
        return BigInt::CompareWithNumber(bigint, y) == ComparisonResult::EQUAL;
    }
    if (y->IsECMAObject()) {
        JSHandle<JSTaggedValue> yPrimitive(thread, ToPrimitive(thread, y));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return Equal(thread, x, yPrimitive);
    }
    return false;
}

bool JSTaggedValue::EqualHeapObject(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                    const JSHandle<JSTaggedValue> &y)
{
    if (y->IsHeapObject()) {
        // if same type, must call Type::StrictEqual()
        JSType xType = x.GetTaggedValue().GetTaggedObject()->GetClass()->GetObjectType();
        JSType yType = y.GetTaggedValue().GetTaggedObject()->GetClass()->GetObjectType();
        if (xType == yType) {
            return StrictEqual(thread, x, y);
        }
    }
    if (y->IsNumber() || y->IsStringOrSymbol() || y->IsBoolean() || y->IsBigInt()) {
        if (x->IsECMAObject()) {
            JSHandle<JSTaggedValue> xPrimitive(thread, ToPrimitive(thread, x));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            return Equal(thread, xPrimitive, y);
        }
    }
    return false;
}

bool JSTaggedValue::EqualNullOrUndefined(const JSHandle<JSTaggedValue> &x,
                                         const JSHandle<JSTaggedValue> &y)
{
    if (x->IsNull() && y->IsNull()) {
        return true;
    }
    if (x->IsUndefined() && y->IsUndefined()) {
        return true;
    }
    if (x->IsNull() && y->IsUndefined()) {
        return true;
    }
    if (x->IsUndefined() && y->IsNull()) {
        return true;
    }
    return false;
}

static const uint32_t POWERS_OF_10[] = {
    1,
    10,
    100,
    1000,
    10 * 1000,
    100 * 1000,
    1000 * 1000,
    10 * 1000 * 1000,
    100 * 1000 * 1000,
    1000 * 1000 * 1000,
};

int JSTaggedValue::IntLexicographicCompare(JSTaggedValue x, JSTaggedValue y)
{
    ASSERT(x.IsInt() && y.IsInt());
    int xValue = x.GetInt();
    int yValue = y.GetInt();
    if (xValue == yValue) {
        return 0;
    }
    if (xValue == 0 || yValue == 0) {
        return xValue > yValue ? 1 : -1;
    }
    uint32_t unsignedX = static_cast<uint32_t>(xValue);
    uint32_t unsignedY = static_cast<uint32_t>(yValue);
    if (yValue > 0) {
        if (xValue < 0) {
            return -1;
        }
    } else {
        if (xValue > 0) {
            return 1;
        }
        unsignedX = static_cast<uint32_t>(base::NegateWithWraparound(xValue));
        unsignedY = static_cast<uint32_t>(base::NegateWithWraparound(yValue));
    }
    int xLog2 = 31 - __builtin_clz(unsignedX);
    int xDigit = ((xLog2 + 1) * 1233) >> 12; // 1233 、12 : Algorithm implementation
    xDigit -= unsignedX < POWERS_OF_10[xDigit];

    int yLog2 = 31 - __builtin_clz(unsignedY);
    int yDigit = ((yLog2 + 1) * 1233) >> 12; // 1233 、12 : Algorithm implementation
    yDigit -= unsignedY < POWERS_OF_10[yDigit];

    int res = 0;
    if (xDigit > yDigit) {
        // X has fewer digits.  We would like to simply scale up X but that
        // might overflow, e.g when comparing 9 with 1_000_000_000, 9 would
        // be scaled up to 9_000_000_000. So we scale up by the next
        // smallest power and scale down Y to drop one digit. It is OK to
        // drop one digit from the longer integer since the final digit is
        // past the length of the shorter integer.
        unsignedY *= POWERS_OF_10[xDigit - yDigit - 1];
        unsignedX /= 10; // 10 : Decimal
        res = 1;
    }
    if (yDigit > xDigit) {
        unsignedX *= POWERS_OF_10[yDigit - xDigit - 1];
        unsignedY /= 10; // 10 : Decimal
        res = -1;
    }
    if (unsignedX > unsignedY) {
        return 1;
    }

    if (unsignedY > unsignedX) {
        return -1;
    }
    return res;
}

ComparisonResult JSTaggedValue::Compare(JSThread *thread, const JSHandle<JSTaggedValue> &x,
                                        const JSHandle<JSTaggedValue> &y)
{
    if (x->IsDate() && y->IsDate()) {
        double timeX = JSDate::Cast(x->GetTaggedObject())->GetTimeValue().GetDouble();
        double timeY = JSDate::Cast(y->GetTaggedObject())->GetTimeValue().GetDouble();
        return StrictNumberCompare(timeX, timeY);
    }
    JSHandle<JSTaggedValue> primX(thread, ToPrimitive(thread, x));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
    JSHandle<JSTaggedValue> primY(thread, ToPrimitive(thread, y));
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
    if (primX->IsString() && primY->IsString()) {
        auto xHandle = JSHandle<EcmaString>(primX);
        auto yHandle = JSHandle<EcmaString>(primY);
        int result = EcmaStringAccessor::Compare(thread->GetEcmaVM(), xHandle, yHandle);
        if (result < 0) {
            return ComparisonResult::LESS;
        }
        if (result == 0) {
            return ComparisonResult::EQUAL;
        }
        return ComparisonResult::GREAT;
    }
    if (primX->IsBigInt()) {
        if (primY->IsNumber()) {
            JSHandle<BigInt> bigint = JSHandle<BigInt>::Cast(primX);
            return BigInt::CompareWithNumber(bigint, primY);
        } else if (primY->IsString()) {
            JSHandle<JSTaggedValue> bigY(thread, base::NumberHelper::StringToBigInt(thread, primY));
            if (!bigY->IsBigInt()) {
                return ComparisonResult::UNDEFINED;
            }
            return BigInt::Compare(primX.GetTaggedValue(), bigY.GetTaggedValue());
        } else {
            JSHandle<JSTaggedValue> bigY(thread, ToBigInt(thread, primY));
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
            return BigInt::Compare(primX.GetTaggedValue(), bigY.GetTaggedValue());
        }
    }
    if (primY->IsBigInt()) {
        ComparisonResult res = Compare(thread, primY, primX);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
        if (res == ComparisonResult::GREAT) {
            return ComparisonResult::LESS;
        } else if (res == ComparisonResult::LESS) {
            return ComparisonResult::GREAT;
        }
        return res;
    }
    double resultX = 0;
    double resultY = 0;
    if (primX->IsNumber()) {
        resultX = primX->GetNumber();
    } else {
        JSTaggedNumber xNumber = ToNumber(thread, x);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
        resultX = xNumber.GetNumber();
    }
    if (primY->IsNumber()) {
        resultY = primY->GetNumber();
    } else {
        JSTaggedNumber yNumber = ToNumber(thread, y);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ComparisonResult::UNDEFINED);
        resultY = yNumber.GetNumber();
    }
    return StrictNumberCompare(resultX, resultY);
}

bool JSTaggedValue::IsSameTypeOrHClass(JSTaggedValue x, JSTaggedValue y)
{
    if (x.IsNumber() && y.IsNumber()) {
        return true;
    }
    if (x.IsBoolean() && y.IsBoolean()) {
        return true;
    }
    if (x.IsString() && y.IsString()) {
        return true;
    }
    if (x.IsHeapObject() && y.IsHeapObject()) {
        return x.GetTaggedObject()->GetClass() == y.GetTaggedObject()->GetClass();
    }

    return false;
}

JSTaggedValue JSTaggedValue::ToPrimitive(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                         PreferredPrimitiveType type)
{
    if (tagged->IsECMAObject()) {
        EcmaVM *vm = thread->GetEcmaVM();
        JSHandle<JSTaggedValue> keyString = vm->GetGlobalEnv()->GetToPrimitiveSymbol();

        JSHandle<JSTaggedValue> exoticToprim = JSObject::GetMethod(thread, tagged, keyString);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        if (!exoticToprim->IsUndefined()) {
            JSTaggedValue value = GetTypeString(thread, type).GetTaggedValue();
            JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, exoticToprim, tagged, undefined, 1);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            info->SetCallArg(value);
            JSTaggedValue valueResult = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            if (!valueResult.IsECMAObject()) {
                return valueResult;
            }
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert object to primitive value",
                JSTaggedValue::Exception());
        } else {
            type = (type == NO_PREFERENCE) ? PREFER_NUMBER : type;
            return OrdinaryToPrimitive(thread, tagged, type);
        }
    }
    return tagged.GetTaggedValue();
}

JSTaggedValue JSTaggedValue::OrdinaryToPrimitive(JSThread *thread, const JSHandle<JSTaggedValue> &tagged,
                                                 PreferredPrimitiveType type)
{
    static_assert(PREFER_NUMBER == 0 && PREFER_STRING == 1);
    ASSERT(tagged->IsECMAObject());
    auto globalConst = thread->GlobalConstants();
    for (uint8_t i = 0; i < 2; i++) {  // 2: 2 means value has 2 target types, string or value.
        JSHandle<JSTaggedValue> keyString;
        if ((static_cast<uint8_t>(type) ^ i) != 0) {
            keyString = globalConst->GetHandledToStringString();
        } else {
            keyString = globalConst->GetHandledValueOfString();
        }
        JSHandle<JSTaggedValue> entryfunc = GetProperty(thread, tagged, keyString).GetValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        if (entryfunc->IsCallable()) {
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
            EcmaRuntimeCallInfo *info =
                EcmaInterpreter::NewRuntimeCallInfo(thread, entryfunc, tagged, undefined, 0);
            JSTaggedValue valueResult = JSFunction::Call(info);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
            if (!valueResult.IsECMAObject()) {
                return valueResult;
            }
        }
    }
    DumpExceptionObject(thread, tagged);
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a illegal value to a Primitive", JSTaggedValue::Undefined());
}

JSHandle<EcmaString> JSTaggedValue::ToString(JSThread *thread, JSTaggedValue val)
{
    JSHandle<JSTaggedValue> tagged(thread, val);
    return ToString(thread, tagged);
}

JSHandle<EcmaString> JSTaggedValue::ToString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsString()) {
        return JSHandle<EcmaString>(tagged);
    }
    auto globalConst = thread->GlobalConstants();
    if (tagged->IsSpecial()) {
        switch (tagged->GetRawData()) {
            case VALUE_UNDEFINED: {
                return JSHandle<EcmaString>(globalConst->GetHandledUndefinedString());
            }
            case VALUE_NULL: {
                return JSHandle<EcmaString>(globalConst->GetHandledNullString());
            }
            case VALUE_TRUE: {
                return JSHandle<EcmaString>(globalConst->GetHandledTrueString());
            }
            case VALUE_FALSE: {
                return JSHandle<EcmaString>(globalConst->GetHandledFalseString());
            }
            case VALUE_HOLE: {
                return JSHandle<EcmaString>(globalConst->GetHandledEmptyString());
            }
            default:
                break;
        }
    }

    if (tagged->IsNumber()) {
        return base::NumberHelper::NumberToString(thread, tagged.GetTaggedValue());
    }

    if (tagged->IsBigInt()) {
        JSHandle<BigInt> taggedValue(tagged);
        return BigInt::ToString(thread, taggedValue);
    }

    if (tagged->IsNativePointer()) {
        return NativePointerToString(thread, tagged);
    }

    auto emptyStr = globalConst->GetHandledEmptyString();
    if (tagged->IsECMAObject()) {
        JSHandle<JSTaggedValue> primValue(thread, ToPrimitive(thread, tagged, PREFER_STRING));
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<EcmaString>(emptyStr));
        return ToString(thread, primValue);
    }
    DumpExceptionObject(thread, tagged);
    // Already Include Symbol
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a illegal value to a String", JSHandle<EcmaString>(emptyStr));
}

JSHandle<EcmaString> JSTaggedValue::NativePointerToString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    JSHandle<JSNativePointer> taggedHandle(tagged);
    std::stringstream stringstream;
    stringstream << std::hex << taggedHandle->GetExternalPointer();
    std::string nativePtrStr = "[External: " + stringstream.str() + "]";

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromASCII(nativePtrStr);
}

JSTaggedValue JSTaggedValue::CanonicalNumericIndexString(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsNumber()) {
        return tagged.GetTaggedValue();
    }

    if (tagged->IsString()) {
        JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("-0");
        if (EcmaStringAccessor::StringsAreEqual(thread->GetEcmaVM(), JSHandle<EcmaString>(tagged), str)) {
            return JSTaggedValue(-0.0);
        }
        JSHandle<JSTaggedValue> tmp(thread, ToNumber(thread, tagged));
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        if (SameValue(ToString(thread, tmp).GetTaggedValue(), tagged.GetTaggedValue())) {
            return tmp.GetTaggedValue();
        }
    }
    return JSTaggedValue::Undefined();
}

JSHandle<JSObject> JSTaggedValue::ToObject(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (tagged->IsInt() || tagged->IsDouble()) {
        return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_NUMBER, tagged));
    }

    switch (tagged->GetRawData()) {
        case JSTaggedValue::VALUE_UNDEFINED: {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a UNDEFINED value to a JSObject",
                                        JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        }
        case JSTaggedValue::VALUE_HOLE: {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a HOLE value to a JSObject",
                                        JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        }
        case JSTaggedValue::VALUE_NULL: {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a NULL value to a JSObject",
                                        JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
        }
        case JSTaggedValue::VALUE_TRUE:
        case JSTaggedValue::VALUE_FALSE: {
            return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BOOLEAN, tagged));
        }
        default: {
            break;
        }
    }

    if (tagged->IsECMAObject()) {
        return JSHandle<JSObject>::Cast(tagged);
    }
    if (tagged->IsSymbol()) {
        return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_SYMBOL, tagged));
    }
    if (tagged->IsString()) {
        return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_STRING, tagged));
    }
    if (tagged->IsBigInt()) {
        return JSHandle<JSObject>::Cast(factory->NewJSPrimitiveRef(PrimitiveType::PRIMITIVE_BIGINT, tagged));
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot convert a Unknown object value to a JSObject",
                                JSHandle<JSObject>(thread, JSTaggedValue::Exception()));
}

// 7.3.1 Get ( O, P )
OperationResult JSTaggedValue::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                           const JSHandle<JSTaggedValue> &key, SCheckMode sCheckMode)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        std::string keyStr = EcmaStringAccessor(ToString(thread, key)).ToStdString();
        std::string objStr = EcmaStringAccessor(ToString(thread, obj)).ToStdString();
        std::string message = "Cannot read property ";
        message.append(keyStr).append(" of ").append(objStr);
        THROW_TYPE_ERROR_AND_RETURN(thread, message.c_str(),
                                    OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    if (obj->IsJSProxy()) {
        return JSProxy::GetProperty(thread, JSHandle<JSProxy>(obj), key);
    }
    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        return JSTypedArray::GetProperty(thread, obj, key);
    }
    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::GetProperty(thread, obj, key);
    }

    if (obj->IsSpecialContainer()) {
        return GetJSAPIProperty(thread, obj, key);
    }

    return JSObject::GetProperty(thread, obj, key, sCheckMode);
}

OperationResult JSTaggedValue::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        std::string objStr = EcmaStringAccessor(ToString(thread, obj)).ToStdString();
        std::string message = "Cannot read property ";
        message.append(ToCString(key)).append(" of ").append(objStr);
        THROW_TYPE_ERROR_AND_RETURN(thread, message.c_str(),
                                    OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }

    if (obj->IsJSProxy()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        return JSProxy::GetProperty(thread, JSHandle<JSProxy>(obj), keyHandle);
    }

    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        return JSTypedArray::GetProperty(thread, obj, key);
    }

    if (obj->IsSpecialContainer()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        return GetJSAPIProperty(thread, obj, keyHandle);
    }

    return JSObject::GetProperty(thread, obj, key);
}

OperationResult JSTaggedValue::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                           const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        std::string keyStr = EcmaStringAccessor(ToString(thread, key)).ToStdString();
        std::string objStr = EcmaStringAccessor(ToString(thread, obj)).ToStdString();
        std::string message = "Cannot read property ";
        message.append(keyStr).append(" of ").append(objStr);
        THROW_TYPE_ERROR_AND_RETURN(thread, message.c_str(),
                                    OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false)));
    }
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    if (obj->IsJSProxy()) {
        return JSProxy::GetProperty(thread, JSHandle<JSProxy>(obj), key, receiver);
    }
    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        return JSTypedArray::GetProperty(thread, obj, key, receiver);
    }

    if (obj->IsSpecialContainer()) {
        return GetJSAPIProperty(thread, obj, key);
    }

    return JSObject::GetProperty(thread, obj, key, receiver);
}

// 7.3.3 Set (O, P, V, Throw)
bool JSTaggedValue::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value, bool mayThrow,
                                SCheckMode checkMode)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a Valid object", false);
    }

    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 4. Let success be O.[[Set]](P, V, O).
    bool success = false;
    if (obj->IsJSProxy()) {
        success = JSProxy::SetProperty(thread, JSHandle<JSProxy>(obj), key, value, mayThrow);
    } else if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        success = JSTypedArray::SetProperty(thread, obj, key, value, mayThrow);
    } else if (obj->IsModuleNamespace()) {
        success = ModuleNamespace::SetProperty(thread, mayThrow);
    } else if (obj->IsSpecialContainer()) {
        success = SetJSAPIProperty(thread, obj, key, value);
    } else {
        success = JSObject::SetProperty(thread, obj, key, value, mayThrow, checkMode);
    }
    // 5. ReturnIfAbrupt(success).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, success);
    // 6. If success is false and Throw is true, throw a TypeError exception.
    // have done in JSObject::SetPropert.
    return success;
}

bool JSTaggedValue::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key,
                                const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a Valid object", false);
    }

    // 4. Let success be O.[[Set]](P, V, O).
    bool success = false;
    if (obj->IsJSProxy()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        success = JSProxy::SetProperty(thread, JSHandle<JSProxy>(obj), keyHandle, value, mayThrow);
    } else if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        success = JSTypedArray::SetProperty(thread, obj, keyHandle, value, mayThrow);
    } else if (obj->IsModuleNamespace()) {
        success = ModuleNamespace::SetProperty(thread, mayThrow);
    } else if (obj->IsSpecialContainer()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        success = SetJSAPIProperty(thread, obj, keyHandle, value);
    } else {
        success = JSObject::SetProperty(thread, obj, key, value, mayThrow);
    }
    // 5. ReturnIfAbrupt(success).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, success);
    // 6. If success is false and Throw is true, throw a TypeError exception.
    // have done in JSObject::SetPropert.
    return success;
}

bool JSTaggedValue::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                const JSHandle<JSTaggedValue> &receiver, bool mayThrow)
{
    if (obj->IsUndefined() || obj->IsNull() || obj->IsHole()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a Valid object", false);
    }

    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 4. Let success be O.[[Set]](P, V, O).
    bool success = false;
    if (obj->IsJSProxy()) {
        success = JSProxy::SetProperty(thread, JSHandle<JSProxy>(obj), key, value, receiver, mayThrow);
    } else if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        success = JSTypedArray::SetProperty(thread, obj, key, value, receiver, mayThrow);
    } else if (obj->IsModuleNamespace()) {
        success = ModuleNamespace::SetProperty(thread, mayThrow);
    } else if (obj->IsSpecialContainer()) {
        success = SetJSAPIProperty(thread, obj, key, value);
    } else {
        success = JSObject::SetProperty(thread, obj, key, value, receiver, mayThrow);
    }
    // 5. ReturnIfAbrupt(success).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, success);
    // 6. If success is false and Throw is true, throw a TypeError exception.
    // have done in JSObject::SetPropert.
    return success;
}

bool JSTaggedValue::DeleteProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &key)
{
    if (obj->IsJSProxy()) {
        return JSProxy::DeleteProperty(thread, JSHandle<JSProxy>(obj), key);
    }

    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::DeleteProperty(thread, obj, key);
    }

    if (obj->IsTypedArray()) {
        return JSTypedArray::DeleteProperty(thread, obj, key);
    }

    if (obj->IsSpecialContainer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not delete property in Container Object", false);
    }

    return JSObject::DeleteProperty(thread, JSHandle<JSObject>(obj), key);
}

// 7.3.8 DeletePropertyOrThrow (O, P)
bool JSTaggedValue::DeletePropertyOrThrow(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                          const JSHandle<JSTaggedValue> &key)
{
    if (!obj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Obj is not a valid object", false);
    }
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 3. Let success be O.[[Delete]](P).
    bool success = DeleteProperty(thread, obj, key);

    // 4. ReturnIfAbrupt(success).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, success);
    // 5. If success is false, throw a TypeError exception
    if (!success) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot delete property", false);
    }
    return success;
}

// 7.3.7 DefinePropertyOrThrow (O, P, desc)
bool JSTaggedValue::DefinePropertyOrThrow(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                          const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc)
{
    // 1. Assert: Type(O) is Object.
    // 2. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 3. Let success be ? O.[[DefineOwnProperty]](P, desc).
    bool success = DefineOwnProperty(thread, obj, key, desc);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    // 4. If success is false, throw a TypeError exception.
    if (!success) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot define property", false);
    }
    return success;
}

bool JSTaggedValue::DefineOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc,
                                      SCheckMode sCheckMode)
{
    if (obj->IsJSArray()) {
        return JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
    }

    if (obj->IsJSSharedArray()) {
        return JSSharedArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key, desc, sCheckMode);
    }

    if (obj->IsJSProxy()) {
        return JSProxy::DefineOwnProperty(thread, JSHandle<JSProxy>(obj), key, desc);
    }

    if (obj->IsTypedArray()) {
        return JSTypedArray::DefineOwnProperty(thread, obj, key, desc);
    }

    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::DefineOwnProperty(thread, obj, key, desc);
    }

    if (obj->IsSpecialContainer()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not defineProperty on Container Object", false);
    }

    return JSObject::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
}

bool JSTaggedValue::GetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    if (obj->IsJSProxy()) {
        return JSProxy::GetOwnProperty(thread, JSHandle<JSProxy>(obj), key, desc);
    }
    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        return JSTypedArray::GetOwnProperty(thread, obj, key, desc);
    }
    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::GetOwnProperty(thread, obj, key, desc);
    }
    if (obj->IsSpecialContainer()) {
        return GetContainerProperty(thread, obj, key, desc);
    }
    return JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
}

bool JSTaggedValue::SetPrototype(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                 const JSHandle<JSTaggedValue> &proto, bool isChangeProto)
{
    if (obj->IsJSShared() || proto->IsJSShared()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, GET_MESSAGE_STRING(SetProtoWithSendable), false);
    }

    if (obj->IsJSProxy()) {
        return JSProxy::SetPrototype(thread, JSHandle<JSProxy>(obj), proto);
    }
    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::SetPrototype(obj, proto);
    }
    if (obj->IsSpecialContainer() || !obj->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not set Prototype on Container or non ECMA Object", false);
    }
    if (obj->IsJSFunction() && proto->IsJSFunction()) {
        JSHandle<JSFunction> objFunc = JSHandle<JSFunction>::Cast(obj);
        JSTaggedValue objProtoOrHClass(objFunc->GetProtoOrHClass());
        if (objProtoOrHClass.IsJSHClass() && objFunc->IsDerivedConstructor()) {
            JSHandle<JSHClass> cachedJSHClass = JSHandle<JSHClass>(thread, objProtoOrHClass);
            objFunc->SetProtoOrHClass(thread, cachedJSHClass->GetPrototype());
        }
    }

    return JSObject::SetPrototype(thread, JSHandle<JSObject>(obj), proto, isChangeProto);
}

JSTaggedValue JSTaggedValue::GetPrototype(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (!obj->IsECMAObject()) {
        DumpExceptionObject(thread, obj);
        THROW_TYPE_ERROR_AND_RETURN(thread, "Can not get Prototype on non ECMA Object", JSTaggedValue::Exception());
    }
    if (obj->IsJSProxy()) {
        return JSProxy::GetPrototype(thread, JSHandle<JSProxy>(obj));
    }
    return JSObject::GetPrototype(JSHandle<JSObject>(obj));
}

bool JSTaggedValue::PreventExtensions(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (obj->IsJSProxy()) {
        return JSProxy::PreventExtensions(thread, JSHandle<JSProxy>(obj));
    }
    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::PreventExtensions();
    }
    return JSObject::PreventExtensions(thread, JSHandle<JSObject>(obj));
}

JSHandle<TaggedArray> JSTaggedValue::GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (obj->IsJSProxy()) {
        return JSProxy::OwnPropertyKeys(thread, JSHandle<JSProxy>(obj));
    }
    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        return JSTypedArray::OwnPropertyKeys(thread, obj);
    }
    if (obj->IsSpecialContainer()) {
        return GetOwnContainerPropertyKeys(thread, obj);
    }
    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::OwnPropertyKeys(thread, obj);
    }
    return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(obj));
}

JSHandle<TaggedArray> JSTaggedValue::GetAllPropertyKeys(JSThread *thread,
                                                        const JSHandle<JSTaggedValue> &obj, uint32_t filter)
{
    if (obj->IsJSProxy()) {
        return JSProxy::GetAllPropertyKeys(thread, JSHandle<JSProxy>(obj), filter);
    }
    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        LOG_ECMA(WARN) << "GetAllPropertyKeys do not support TypedArray yet";
        return thread->GetEcmaVM()->GetFactory()->EmptyArray();
    }
    if (obj->IsSpecialContainer()) {
        LOG_ECMA(WARN) << "GetAllPropertyKeys do not support SpecialContainer yet";
        return thread->GetEcmaVM()->GetFactory()->EmptyArray();
    }
    if (obj->IsModuleNamespace()) {
        LOG_ECMA(WARN) << "GetAllPropertyKeys do not support ModuleNamespace yet";
        return thread->GetEcmaVM()->GetFactory()->EmptyArray();
    }
    return JSObject::GetAllPropertyKeys(thread, JSHandle<JSObject>(obj), filter);
}

JSHandle<TaggedArray> JSTaggedValue::GetOwnEnumPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    ASSERT(!obj->IsJSProxy());
    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        return JSTypedArray::OwnEnumPropertyKeys(thread, obj);
    }
    if (obj->IsSpecialContainer()) {
        return GetOwnContainerEnumPropertyKeys(thread, obj);
    }
    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::OwnEnumPropertyKeys(thread, obj);
    }
    return JSObject::GetOwnEnumPropertyKeys(thread, JSHandle<JSObject>(obj));
}

// 7.3.10 HasProperty (O, P)
bool JSTaggedValue::HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                const JSHandle<JSTaggedValue> &key)
{
    if (obj->IsJSProxy()) {
        return JSProxy::HasProperty(thread, JSHandle<JSProxy>(obj), key);
    }
    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        return JSTypedArray::HasProperty(thread, obj, key);
    }
    if (obj->IsModuleNamespace()) {
        return ModuleNamespace::HasProperty(thread, obj, key);
    }
    if (obj->IsSpecialContainer()) {
        return HasContainerProperty(thread, obj, key);
    }
    return JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key);
}

bool JSTaggedValue::HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t key)
{
    if (obj->IsJSProxy()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        return JSProxy::HasProperty(thread, JSHandle<JSProxy>(obj), keyHandle);
    }
    if (obj->IsTypedArray() || obj->IsSharedTypedArray()) {
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
        return JSTypedArray::HasProperty(thread, obj, keyHandle);
    }
    if (obj->IsSpecialContainer()) {
        return HasContainerProperty(thread, obj, JSHandle<JSTaggedValue>(thread, JSTaggedValue(key)));
    }
    return JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key);
}

// 7.3.11 HasOwnProperty (O, P)
bool JSTaggedValue::HasOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    PropertyDescriptor desc(thread);
    return JSTaggedValue::GetOwnProperty(thread, obj, key, desc);
}

bool JSTaggedValue::GlobalHasOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    PropertyDescriptor desc(thread);
    return JSObject::GlobalGetOwnProperty(thread, key, desc);
}

bool JSTaggedValue::CanBeHeldWeakly(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    // 1. If v is an Object, return true.
    if (tagged->IsECMAObject()) {
        return true;
    }
    // 2. If v is a Symbol and KeyForSymbol(v) is undefined, return true.
    if (tagged->IsSymbol()) {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        auto *table = env->GetRegisterSymbols().GetObject<SymbolTable>();
        JSTaggedValue key = table->FindSymbol(tagged.GetTaggedValue());
        if (key.IsUndefined()) {
            return true;
        }
    }
    // 3. Return false.
    return false;
}

JSTaggedNumber JSTaggedValue::ToIndex(JSThread *thread, const JSHandle<JSTaggedValue> &tagged)
{
    if (tagged->IsInt() && tagged->GetInt() >= 0) {
        return JSTaggedNumber(tagged.GetTaggedValue());
    }
    if (tagged->IsUndefined()) {
        return JSTaggedNumber(0);
    }
    JSTaggedNumber integerIndex = ToNumber(thread, tagged);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedNumber::Exception());
    if (integerIndex.IsInt() && integerIndex.GetInt() >= 0) {
        return integerIndex;
    }
    double len = base::NumberHelper::TruncateDouble(integerIndex.GetNumber());
    if (len < 0.0 || len > SAFE_NUMBER) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "integerIndex < 0 or integerIndex > SAFE_NUMBER",
                                     JSTaggedNumber::Exception());
    }
    return JSTaggedNumber(len);
}

// 9.4.2.4 ArraySetLength 3 to 7
bool JSTaggedValue::ToArrayLength(JSThread *thread, const JSHandle<JSTaggedValue> &tagged, uint32_t *output)
{
    // 3. Let newLen be ToUint32(Desc.[[Value]]).
    uint32_t newLen = ToUint32(thread, tagged);
    // 4. ReturnIfAbrupt(newLen).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 5. Let numberLen be ToNumber(Desc.[[Value]]).
    JSTaggedNumber numberLen = ToNumber(thread, tagged);
    // 6. ReturnIfAbrupt(newLen).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 7. If newLen != numberLen, throw a RangeError exception.
    if (JSTaggedNumber(newLen) != numberLen) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Not a valid array length", false);
    }

    *output = newLen;
    return true;
}

JSHandle<JSTaggedValue> JSTaggedValue::ToPrototypeOrObj(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    if (obj->IsNumber()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetNumberFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }
    if (obj->IsBoolean()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetBooleanFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }
    if (obj->IsString()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetStringFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }
    if (obj->IsSymbol()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetSymbolFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }
    if (obj->IsBigInt()) {
        return JSHandle<JSTaggedValue>(thread,
                                       env->GetBigIntFunction().GetObject<JSFunction>()->GetFunctionPrototype());
    }
    return obj;
}

JSTaggedValue JSTaggedValue::GetSuperBase(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (obj->IsUndefined()) {
        return JSTaggedValue::Undefined();
    }

    ASSERT(obj->IsECMAObject());
    return JSTaggedValue::GetPrototype(thread, obj);
}

bool JSTaggedValue::HasContainerProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                         const JSHandle<JSTaggedValue> &key)
{
    auto *hclass = obj->GetTaggedObject()->GetClass();
    JSType jsType = hclass->GetObjectType();
    switch (jsType) {
        case JSType::JS_API_ARRAY_LIST: {
            return JSHandle<JSAPIArrayList>::Cast(obj)->Has(key.GetTaggedValue());
        }
        case JSType::JS_API_QUEUE: {
            return JSHandle<JSAPIQueue>::Cast(obj)->Has(key.GetTaggedValue());
        }
        case JSType::JS_API_PLAIN_ARRAY: {
            return JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key);
        }
        case JSType::JS_API_DEQUE: {
            return JSHandle<JSAPIDeque>::Cast(obj)->Has(key.GetTaggedValue());
        }
        case JSType::JS_API_STACK: {
            return JSHandle<JSAPIStack>::Cast(obj)->Has(key.GetTaggedValue());
        }
        case JSType::JS_API_LIST: {
            JSHandle<JSAPIList> list = JSHandle<JSAPIList>::Cast(obj);
            return list->Has(key.GetTaggedValue());
        }
        case JSType::JS_API_LINKED_LIST: {
            JSHandle<JSAPILinkedList> linkedList = JSHandle<JSAPILinkedList>::Cast(obj);
            return linkedList->Has(key.GetTaggedValue());
        }
        case JSType::JS_API_HASH_MAP:
        case JSType::JS_API_HASH_SET:
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
        case JSType::JS_API_LIGHT_WEIGHT_SET:
        case JSType::JS_API_TREE_MAP:
        case JSType::JS_API_TREE_SET: {
            return JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key);
        }
        case JSType::JS_API_VECTOR: {
            return JSHandle<JSAPIVector>::Cast(obj)->Has(key.GetTaggedValue());
        }
        case JSType::JS_API_BITVECTOR: {
            return JSHandle<JSAPIBitVector>::Cast(obj)->Has(key.GetTaggedValue());
        }
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
    return false;
}

JSHandle<TaggedArray> JSTaggedValue::GetOwnContainerPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    auto *hclass = obj->GetTaggedObject()->GetClass();
    JSType jsType = hclass->GetObjectType();
    switch (jsType) {
        case JSType::JS_API_ARRAY_LIST: {
            return JSAPIArrayList::OwnKeys(thread, JSHandle<JSAPIArrayList>::Cast(obj));
        }
        case JSType::JS_API_QUEUE: {
            return JSAPIQueue::OwnKeys(thread, JSHandle<JSAPIQueue>::Cast(obj));
        }
        case JSType::JS_API_PLAIN_ARRAY: {
            return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(obj));
        }
        case JSType::JS_API_DEQUE: {
            return JSAPIDeque::OwnKeys(thread, JSHandle<JSAPIDeque>::Cast(obj));
        }
        case JSType::JS_API_STACK: {
            return JSAPIStack::OwnKeys(thread, JSHandle<JSAPIStack>::Cast(obj));
        }
        case JSType::JS_API_LIST: {
            return JSAPIList::OwnKeys(thread, JSHandle<JSAPIList>::Cast(obj));
        }
        case JSType::JS_API_LINKED_LIST: {
            return JSAPILinkedList::OwnKeys(thread, JSHandle<JSAPILinkedList>::Cast(obj));
        }
        case JSType::JS_API_HASH_MAP:
        case JSType::JS_API_HASH_SET:
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
        case JSType::JS_API_LIGHT_WEIGHT_SET:
        case JSType::JS_API_TREE_MAP:
        case JSType::JS_API_TREE_SET: {
            return JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(obj));
        }
        case JSType::JS_API_VECTOR: {
            return JSAPIVector::OwnKeys(thread, JSHandle<JSAPIVector>::Cast(obj));
        }
        case JSType::JS_API_BITVECTOR: {
            return JSAPIBitVector::OwnKeys(thread, JSHandle<JSAPIBitVector>::Cast(obj));
        }
        default: {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }
    return thread->GetEcmaVM()->GetFactory()->EmptyArray();
}

JSHandle<TaggedArray> JSTaggedValue::GetOwnContainerEnumPropertyKeys(JSThread *thread,
    const JSHandle<JSTaggedValue> &obj)
{
    auto *hclass = obj->GetTaggedObject()->GetClass();
    JSType jsType = hclass->GetObjectType();
    switch (jsType) {
        case JSType::JS_API_QUEUE: {
            return JSAPIQueue::OwnEnumKeys(thread, JSHandle<JSAPIQueue>::Cast(obj));
        }
        case JSType::JS_API_DEQUE: {
            return JSAPIDeque::OwnEnumKeys(thread, JSHandle<JSAPIDeque>::Cast(obj));
        }
        case JSType::JS_API_LIST: {
            return JSAPIList::OwnKeys(thread, JSHandle<JSAPIList>::Cast(obj));
        }
        case JSType::JS_API_LINKED_LIST: {
            return JSAPILinkedList::OwnKeys(thread, JSHandle<JSAPILinkedList>::Cast(obj));
        }
        case JSType::JS_API_VECTOR:
        case JSType::JS_API_BITVECTOR:
        case JSType::JS_API_STACK:
        case JSType::JS_API_ARRAY_LIST:
        case JSType::JS_API_PLAIN_ARRAY:
        case JSType::JS_API_HASH_MAP:
        case JSType::JS_API_HASH_SET:
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
        case JSType::JS_API_LIGHT_WEIGHT_SET:
        case JSType::JS_API_TREE_MAP:
        case JSType::JS_API_TREE_SET: {
            return JSObject::GetOwnEnumPropertyKeys(thread, JSHandle<JSObject>(obj));
        }
        default: {
            UNREACHABLE();
        }
    }
    return thread->GetEcmaVM()->GetFactory()->EmptyArray();
}

bool JSTaggedValue::GetContainerProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                         const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    if (key->IsInteger()) {
        auto *hclass = obj->GetTaggedObject()->GetClass();
        JSType jsType = hclass->GetObjectType();
        switch (jsType) {
            case JSType::JS_API_ARRAY_LIST: {
                return JSAPIArrayList::GetOwnProperty(thread, JSHandle<JSAPIArrayList>::Cast(obj), key);
            }
            case JSType::JS_API_QUEUE: {
                return JSAPIQueue::GetOwnProperty(thread, JSHandle<JSAPIQueue>::Cast(obj), key);
            }
            case JSType::JS_API_DEQUE: {
                return JSAPIDeque::GetOwnProperty(thread, JSHandle<JSAPIDeque>::Cast(obj), key);
            }
            case JSType::JS_API_STACK: {
                return JSAPIStack::GetOwnProperty(thread, JSHandle<JSAPIStack>::Cast(obj), key);
            }
            case JSType::JS_API_LIST: {
                return JSAPIList::GetOwnProperty(thread, JSHandle<JSAPIList>::Cast(obj), key);
            }
            case JSType::JS_API_LINKED_LIST: {
                return JSAPILinkedList::GetOwnProperty(thread, JSHandle<JSAPILinkedList>::Cast(obj), key);
            }
            case JSType::JS_API_PLAIN_ARRAY: {
                return JSAPIPlainArray::GetOwnProperty(thread, JSHandle<JSAPIPlainArray>::Cast(obj), key);
            }
            case JSType::JS_API_VECTOR: {
                return JSAPIVector::GetOwnProperty(thread, JSHandle<JSAPIVector>::Cast(obj), key);
            }
            case JSType::JS_API_BITVECTOR: {
                return JSAPIBitVector::GetOwnProperty(thread, JSHandle<JSAPIBitVector>::Cast(obj), key);
            }
            default: {
                return JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
            }
        }
    } else {
        return JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
    }
    return false;
}

JSTaggedNumber JSTaggedValue::StringToNumber(JSTaggedValue tagged)
{
    EcmaStringAccessor strAccessor(tagged);
    size_t strLen = strAccessor.GetLength();
    if (strLen == 0) {
        return JSTaggedNumber(0);
    }
    if (strLen < MAX_ELEMENT_INDEX_LEN && strAccessor.IsUtf8()) {
        uint32_t index;
        // fast path: get integer from string's hash value
        if (strAccessor.TryToGetInteger(&index)) {
            return JSTaggedNumber(index);
        }
        Span<const uint8_t> str = strAccessor.FastToUtf8Span();
        if (strAccessor.GetLength() == 0) {
            return JSTaggedNumber(0);
        }
        auto [isSuccess, result] = base::NumberHelper::FastStringToNumber(str.begin(), str.end(), tagged);
        if (isSuccess) {
            return result;
        }
    }
    CVector<uint8_t> buf;
    Span<const uint8_t> str = strAccessor.ToUtf8Span(buf);
    double d = base::NumberHelper::StringToDouble(str.begin(), str.end(), 0,
                                                  base::ALLOW_BINARY + base::ALLOW_OCTAL + base::ALLOW_HEX);
    return JSTaggedNumber(d);
}

JSHandle<JSTaggedValue> JSTaggedValue::ToNumeric(JSThread *thread, JSHandle<JSTaggedValue> tagged)
{
    // 1. Let primValue be ? ToPrimitive(value, number)
    JSTaggedValue primValue = ToPrimitive(thread, tagged, PREFER_NUMBER);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // 2. If Type(primValue) is BigInt, return primValue.
    if (primValue.IsBigInt()) {
        return JSHandle<JSTaggedValue>(thread, primValue);
    }
    // 3. Return ? ToNumber(primValue).
    JSTaggedNumber number = ToNumber(thread, primValue);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    JSHandle<JSTaggedValue> value(thread, number);
    return value;
}

OperationResult JSTaggedValue::GetJSAPIProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                const JSHandle<JSTaggedValue> &key)
{
    if (key->IsInteger()) {
        auto *hclass = obj->GetTaggedObject()->GetClass();
        JSType jsType = hclass->GetObjectType();
        switch (jsType) {
            case JSType::JS_API_ARRAY_LIST: {
                return JSAPIArrayList::GetProperty(thread, JSHandle<JSAPIArrayList>::Cast(obj), key);
            }
            case JSType::JS_API_LIST: {
                return JSAPIList::GetProperty(thread, JSHandle<JSAPIList>::Cast(obj), key);
            }
            case JSType::JS_API_LINKED_LIST: {
                return JSAPILinkedList::GetProperty(thread, JSHandle<JSAPILinkedList>::Cast(obj), key);
            }
            case JSType::JS_API_QUEUE: {
                return JSAPIQueue::GetProperty(thread, JSHandle<JSAPIQueue>::Cast(obj), key);
            }
            case JSType::JS_API_DEQUE: {
                return JSAPIDeque::GetProperty(thread, JSHandle<JSAPIDeque>::Cast(obj), key);
            }
            case JSType::JS_API_STACK: {
                return JSAPIStack::GetProperty(thread, JSHandle<JSAPIStack>::Cast(obj), key);
            }
            case JSType::JS_API_PLAIN_ARRAY: {
                return JSAPIPlainArray::GetProperty(thread, JSHandle<JSAPIPlainArray>::Cast(obj), key);
            }
            case JSType::JS_API_VECTOR: {
                return JSAPIVector::GetProperty(thread, JSHandle<JSAPIVector>::Cast(obj), key);
            }
            case JSType::JS_API_BITVECTOR: {
                return JSAPIBitVector::GetProperty(thread, JSHandle<JSAPIBitVector>::Cast(obj), key);
            }
            default: {
                return JSObject::GetProperty(thread, JSHandle<JSObject>(obj), key);
            }
        }
    } else {
        return JSObject::GetProperty(thread, JSHandle<JSObject>(obj), key);
    }
    return OperationResult(thread, JSTaggedValue::Exception(), PropertyMetaData(false));
}

bool JSTaggedValue::SetJSAPIProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key,
                                     const JSHandle<JSTaggedValue> &value)
{
    if (key->IsInteger()) {
        auto *hclass = obj->GetTaggedObject()->GetClass();
        JSType jsType = hclass->GetObjectType();
        switch (jsType) {
            case JSType::JS_API_ARRAY_LIST: {
                return JSAPIArrayList::SetProperty(thread, JSHandle<JSAPIArrayList>::Cast(obj), key, value);
            }
            case JSType::JS_API_LIST: {
                return JSAPIList::SetProperty(thread, JSHandle<JSAPIList>::Cast(obj), key, value);
            }
            case JSType::JS_API_LINKED_LIST: {
                JSAPILinkedList::SetProperty(thread, JSHandle<JSAPILinkedList>::Cast(obj), key, value);
                return true;
            }
            case JSType::JS_API_QUEUE: {
                return JSAPIQueue::SetProperty(thread, JSHandle<JSAPIQueue>::Cast(obj), key, value);
            }
            case JSType::JS_API_DEQUE: {
                return JSAPIDeque::SetProperty(thread, JSHandle<JSAPIDeque>::Cast(obj), key, value);
            }
            case JSType::JS_API_STACK: {
                return JSAPIStack::SetProperty(thread, JSHandle<JSAPIStack>::Cast(obj), key, value);
            }
            case JSType::JS_API_PLAIN_ARRAY: {
                return JSAPIPlainArray::SetProperty(thread, JSHandle<JSAPIPlainArray>::Cast(obj), key, value);
            }
            case JSType::JS_API_VECTOR: {
                return JSAPIVector::SetProperty(thread, JSHandle<JSAPIVector>::Cast(obj), key, value);
            }
            case JSType::JS_API_BITVECTOR: {
                return JSAPIBitVector::SetProperty(thread, JSHandle<JSAPIBitVector>::Cast(obj), key, value);
            }
            default: {
                return JSObject::SetProperty(thread, JSHandle<JSObject>::Cast(obj), key, value);
            }
        }
    } else {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set property on Container", false);
    }
    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set property on Container", false);
}

void JSTaggedValue::DumpExceptionObject(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    if (thread->GetEcmaVM()->GetJSOptions().EnableExceptionBacktrace()) {
        std::ostringstream oss;
        obj->Dump(oss, true);
        std::regex reg("0x[0-9a-fA-F]+");
        std::string sensitiveStr = std::regex_replace(oss.str(), reg, "");
        LOG_ECMA(ERROR) << "DumpExceptionObject: " << sensitiveStr;
    }
}

JSHandle<JSTaggedValue> JSTaggedValue::PublishSharedValueSlow(JSThread *thread, JSHandle<JSTaggedValue> value)
{
    SharedValuePublishGuard guard;

    ASSERT(value->IsTreeString());
    EcmaString *flatStr = EcmaStringAccessor::Flatten(thread->GetEcmaVM(), JSHandle<EcmaString>(value));
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue(flatStr));
}
}  // namespace panda::ecmascript
