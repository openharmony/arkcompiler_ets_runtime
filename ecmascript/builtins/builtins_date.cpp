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

#include "ecmascript/builtins/builtins_date.h"

#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"

#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#else
#ifndef ARK_NOT_SUPPORT_INTL_GLOBAL
#include "ecmascript/intl/global_intl_helper.h"
#endif
#endif

namespace panda::ecmascript::builtins {
// constructor
JSTaggedValue BuiltinsDate::DateConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_ENTRY_DEBUG_LOG();
    BUILTINS_API_TRACE(argv->GetThread(), Date, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        double now = JSDate::Now().GetDouble();
        CString str = JSDate::ToDateString(thread, now);
        return GetTaggedString(thread, str.c_str());
    }

    JSTaggedValue timeValue(0.0);
    uint32_t length = argv->GetArgsNumber();
    if (length == 0) {  // no value
        timeValue = JSDate::Now();
    } else if (length == 1) {  // one value
        JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
        if (value->IsDate()) {  // The value is a date object.
            JSHandle<JSDate> jsDate(thread, JSDate::Cast(value->GetTaggedObject()));
            timeValue = jsDate->GetTimeValue();
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        } else {
            JSHandle<JSTaggedValue> objValue(thread, JSTaggedValue::ToPrimitive(thread, value));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            if (objValue->IsString()) {  // The value is a string object.
                timeValue = JSDate::Parse(argv);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            } else {  // The value is a number.
                JSTaggedNumber val = JSTaggedValue::ToNumber(thread, objValue);
                RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
                timeValue = JSTaggedValue(val.GetNumber());
            }
            timeValue = JSTaggedValue(JSDate::TimeClip(timeValue.GetDouble()));
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        }
    } else {  // two or more values
        timeValue = ExtractDateFields(thread, length, argv, timeValue);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> constructor(GetConstructor(argv));
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(constructor, newTarget);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSDate> dateObject = JSHandle<JSDate>::Cast(obj);
    dateObject->SetTimeValue(thread, timeValue);
    return dateObject.GetTaggedValue();
}

// 20.4.3.1
JSTaggedValue BuiltinsDate::Now([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Date, Now);
    return JSDate::Now();
}

// 20.4.3.2
JSTaggedValue BuiltinsDate::Parse(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Date, Parse);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    return JSDate::Parse(argv);
}

// 20.4.3.4
JSTaggedValue BuiltinsDate::UTC(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Date, UTC);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
    return JSDate::UTC(argv);
}

// 20.4.4.10
JSTaggedValue BuiltinsDate::GetTime(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, GetTime);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    return JSDate::Cast(msg->GetTaggedObject())->GetTime();
}

JSTaggedValue BuiltinsDate::SetTime(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, SetTime);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    JSHandle<JSDate> jsDate(thread, JSDate::Cast(msg->GetTaggedObject()));
    JSTaggedNumber res = JSTaggedValue::ToNumber(thread, GetCallArg(argv, 0));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    double number = res.GetNumber();
    double value = JSDate::TimeClip(number);
    jsDate->SetTimeValue(thread, JSTaggedValue(value));
    return GetTaggedDouble(value);
}

// 20.4.4.37
JSTaggedValue BuiltinsDate::ToJSON(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, ToJSON);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1. Let O be ToObject(this value).
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    JSHandle<JSObject> object = JSTaggedValue::ToObject(thread, msg);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2. Let tv be ToPrimitive(hint Number)
    JSHandle<JSTaggedValue> objectHandle = JSHandle<JSTaggedValue>::Cast(object);
    JSHandle<JSTaggedValue> tv(thread,
                               JSTaggedValue::ToPrimitive(thread, objectHandle, PreferredPrimitiveType::PREFER_NUMBER));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 3. If Type(tv) is Number and tv is not finite, return null
    if (tv->IsNumber()) {
        if (tv->IsDouble() && !std::isfinite(tv->GetDouble())) {
            return JSTaggedValue::Null();
        }
    }
    JSHandle<JSTaggedValue> calleeKey(thread->GetEcmaVM()->GetFactory()->NewFromASCII("toISOString"));
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, objectHandle, undefined, 0);
    return JSFunction::Invoke(info, calleeKey);
}

// 20.4.4.44
JSTaggedValue BuiltinsDate::ValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, ValueOf);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    return JSDate::Cast(msg->GetTaggedObject())->ValueOf();
}

// 20.4.4.45
JSTaggedValue BuiltinsDate::ToPrimitive(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Date, ToPrimitive);
    JSThread *thread = argv->GetThread();
    auto vm = thread->GetEcmaVM();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> object = GetThis(argv);
    if (!object->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a JSObject", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> hint = GetCallArg(argv, 0);
    PreferredPrimitiveType tryFirst = PREFER_STRING;
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    if (hint->IsString()) {
        JSHandle<EcmaString> numberStrHandle = JSHandle<EcmaString>::Cast(globalConst->GetHandledNumberString());
        if (EcmaStringAccessor::StringsAreEqual(vm, JSHandle<EcmaString>(hint), numberStrHandle)) {
            tryFirst = PREFER_NUMBER;
        } else {
            JSHandle<EcmaString> stringStrHandle = JSHandle<EcmaString>::Cast(globalConst->GetHandledStringString());
            JSHandle<EcmaString> defaultStrHandle = JSHandle<EcmaString>::Cast(globalConst->GetHandledDefaultString());
            if (EcmaStringAccessor::StringsAreEqual(vm, JSHandle<EcmaString>(hint), stringStrHandle) ||
                EcmaStringAccessor::StringsAreEqual(vm, JSHandle<EcmaString>(hint), defaultStrHandle)) {
                tryFirst = PREFER_STRING;
            } else {
                THROW_TYPE_ERROR_AND_RETURN(thread, "This is not a primitiveType.", JSTaggedValue::Exception());
            }
        }
    } else {
        THROW_TYPE_ERROR_AND_RETURN(thread, "This is not an primitiveType.", JSTaggedValue::Exception());
    }
    return JSTaggedValue::OrdinaryToPrimitive(thread, object, tryFirst);
}

// ecma 402 16.4.1 Date.prototype.toLocaleString ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsDate::ToLocaleString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Date, ToLocaleString);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    [[maybe_unused]] ObjectFactory *factory = ecmaVm->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let x be ? thisTimeValue(this value).
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSDate::Cast(msg->GetTaggedObject())->GetTime();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If x is NaN, return "Invalid Date".
    double x = value.GetNumber();
    if (std::isnan(x)) {
        return thread->GlobalConstants()->GetInvalidDateString();
    }

    // Let options be ? ToDateTimeOptions(options, "any", "all").
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    [[maybe_unused]] bool cacheable = (locales->IsUndefined() || locales->IsString()) && options->IsUndefined();
#ifdef ARK_SUPPORT_INTL
    if (cacheable) {
        auto simpleDateFormat = JSDateTimeFormat::GetCachedIcuSimpleDateFormat(thread, locales,
            IcuFormatterType::SIMPLE_DATE_FORMAT_DEFAULT);
        if (simpleDateFormat != nullptr) {
            JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, simpleDateFormat, x);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return result.GetTaggedValue();
        }
    }
    JSHandle<JSObject> dateTimeOptions =
        JSDateTimeFormat::ToDateTimeOptions(thread, options, RequiredOption::ANY, DefaultsOption::ALL);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    // Let dateFormat be ? Construct(%DateTimeFormat%, « locales, options »).
    JSHandle<JSFunction> ctor(env->GetDateTimeFormatFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(ctor);
    IcuCacheType type = cacheable ? IcuCacheType::DEFAULT : IcuCacheType::NOT_CACHE;
    JSHandle<JSDateTimeFormat> dtf = JSDateTimeFormat::InitializeDateTimeFormat(thread,
        JSHandle<JSDateTimeFormat>::Cast(obj), locales, JSHandle<JSTaggedValue>::Cast(dateTimeOptions), type);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (cacheable) {
        auto simpleDateFormat = JSDateTimeFormat::GetCachedIcuSimpleDateFormat(thread, locales,
            IcuFormatterType::SIMPLE_DATE_FORMAT_DEFAULT);
        ASSERT(simpleDateFormat != nullptr);
        JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, simpleDateFormat, x);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return result.GetTaggedValue();
    }

    // Return ? FormatDateTime(dateFormat, x).
    JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, dtf, x);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
#else
#ifdef ARK_NOT_SUPPORT_INTL_GLOBAL
    ARK_SUPPORT_INTL_RETURN_JSVALUE(thread, "ToLocaleString");
#else
    intl::GlobalIntlHelper gh(thread, intl::GlobalFormatterType::DateFormatter);
    auto dateFormatter = gh.GetGlobalObject<intl::GlobalDateFormatter>(thread,
        locales, options, intl::GlobalFormatterType::DateFormatter, cacheable);
    if (dateFormatter == nullptr) {
        LOG_ECMA(ERROR) << "BuiltinsDate::ToLocaleString:dateFormatter is nullptr";
    }
    ASSERT(dateFormatter != nullptr);
    std::string result = dateFormatter->Format(intl::GlobalIntlHelper::DoubleToInt64(x));
    JSHandle returnValue = factory->NewFromStdString(result);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return returnValue.GetTaggedValue();
#endif
#endif
}

// ecma 402 16.4.1 Date.prototype.toLocaleString ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsDate::ToLocaleDateString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Date, ToLocaleDateString);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    [[maybe_unused]] ObjectFactory *factory = ecmaVm->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let x be ? thisTimeValue(this value).
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSDate::Cast(msg->GetTaggedObject())->GetTime();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If x is NaN, return "Invalid Date".
    double x = value.GetNumber();
    if (std::isnan(x)) {
        return thread->GlobalConstants()->GetInvalidDateString();
    }

    // Let options be ? ToDateTimeOptions(options, "any", "all").
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    [[maybe_unused]] bool cacheable = (locales->IsUndefined() || locales->IsString()) && options->IsUndefined();
#ifdef ARK_SUPPORT_INTL
    if (cacheable) {
        auto simpleDateFormat = JSDateTimeFormat::GetCachedIcuSimpleDateFormat(thread, locales,
            IcuFormatterType::SIMPLE_DATE_FORMAT_DATE);
        if (simpleDateFormat != nullptr) {
            JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, simpleDateFormat, x);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return result.GetTaggedValue();
        }
    }
    JSHandle<JSObject> dateTimeOptions =
        JSDateTimeFormat::ToDateTimeOptions(thread, options, RequiredOption::DATE, DefaultsOption::DATE);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    // Let dateFormat be ? Construct(%DateTimeFormat%, « locales, options »).
    JSHandle<JSFunction> ctor(env->GetDateTimeFormatFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(ctor);
    IcuCacheType type = cacheable ? IcuCacheType::DATE : IcuCacheType::NOT_CACHE;
    JSHandle<JSDateTimeFormat> dtf = JSDateTimeFormat::InitializeDateTimeFormat(thread,
        JSHandle<JSDateTimeFormat>::Cast(obj), locales, JSHandle<JSTaggedValue>::Cast(dateTimeOptions), type);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (cacheable) {
        auto simpleDateFormat = JSDateTimeFormat::GetCachedIcuSimpleDateFormat(thread, locales,
            IcuFormatterType::SIMPLE_DATE_FORMAT_DATE);
        ASSERT(simpleDateFormat != nullptr);
        JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, simpleDateFormat, x);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return result.GetTaggedValue();
    }

    // Return ? FormatDateTime(dateFormat, x).
    JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, dtf, x);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
#else
#ifdef ARK_NOT_SUPPORT_INTL_GLOBAL
    ARK_SUPPORT_INTL_RETURN_JSVALUE(thread, "LocaleCompare");
#else
    intl::GlobalIntlHelper gh(thread, intl::GlobalFormatterType::SimpleDateFormatDate);
    auto dateFormatter = gh.GetGlobalObject<intl::GlobalDateFormatter>(thread,
        locales, options, intl::GlobalFormatterType::SimpleDateFormatDate, cacheable);
    if (dateFormatter == nullptr) {
        LOG_ECMA(ERROR) << "BuiltinsDate::ToLocaleDateString:dateFormatter is nullptr";
    }
    ASSERT(dateFormatter != nullptr);
    std::string result = dateFormatter->Format(intl::GlobalIntlHelper::DoubleToInt64(x));
    JSHandle returnValue = factory->NewFromStdString(result);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return returnValue.GetTaggedValue();
#endif
#endif
}

// ecma 402 16.4.1 Date.prototype.toLocaleString ( [ locales [ , options ] ] )
JSTaggedValue BuiltinsDate::ToLocaleTimeString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    BUILTINS_API_TRACE(thread, Date, ToLocaleTimeString);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    [[maybe_unused]] ObjectFactory *factory = ecmaVm->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // Let x be ? thisTimeValue(this value).
    JSHandle<JSTaggedValue> msg = GetThis(argv);
    if (!msg->IsDate()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());
    }
    JSTaggedValue value = JSDate::Cast(msg->GetTaggedObject())->GetTime();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // If x is NaN, return "Invalid Date".
    double x = value.GetNumber();
    if (std::isnan(x)) {
        return thread->GlobalConstants()->GetInvalidDateString();
    }

    // Let options be ? ToDateTimeOptions(options, "any", "all").
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> options = GetCallArg(argv, 1);
    [[maybe_unused]] bool cacheable = (locales->IsUndefined() || locales->IsString()) && options->IsUndefined();
#ifdef ARK_SUPPORT_INTL
    if (cacheable) {
        auto simpleDateFormat = JSDateTimeFormat::GetCachedIcuSimpleDateFormat(thread, locales,
            IcuFormatterType::SIMPLE_DATE_FORMAT_TIME);
        if (simpleDateFormat != nullptr) {
            JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, simpleDateFormat, x);
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            return result.GetTaggedValue();
        }
    }
    JSHandle<JSObject> dateTimeOptions =
        JSDateTimeFormat::ToDateTimeOptions(thread, options, RequiredOption::TIME, DefaultsOption::TIME);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    // Let dateFormat be ? Construct(%DateTimeFormat%, « locales, options »).
    JSHandle<JSFunction> ctor(env->GetDateTimeFormatFunction());
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(ctor);
    IcuCacheType type = cacheable ? IcuCacheType::TIME : IcuCacheType::NOT_CACHE;
    JSHandle<JSDateTimeFormat> dtf = JSDateTimeFormat::InitializeDateTimeFormat(thread,
        JSHandle<JSDateTimeFormat>::Cast(obj), locales, JSHandle<JSTaggedValue>::Cast(dateTimeOptions), type);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    if (cacheable) {
        auto simpleDateFormat = JSDateTimeFormat::GetCachedIcuSimpleDateFormat(thread, locales,
            IcuFormatterType::SIMPLE_DATE_FORMAT_TIME);
        ASSERT(simpleDateFormat != nullptr);
        JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, simpleDateFormat, x);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        return result.GetTaggedValue();
    }

    // Return ? FormatDateTime(dateFormat, x).
    JSHandle<EcmaString> result = JSDateTimeFormat::FormatDateTime(thread, dtf, x);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return result.GetTaggedValue();
#else
#ifdef ARK_NOT_SUPPORT_INTL_GLOBAL
    ARK_SUPPORT_INTL_RETURN_JSVALUE(thread, "LocaleCompare");
#else
    intl::GlobalIntlHelper gh(thread, intl::GlobalFormatterType::SimpleDateFormatTime);
    auto dateFormatter = gh.GetGlobalObject<intl::GlobalDateFormatter>(thread,
        locales, options, intl::GlobalFormatterType::SimpleDateFormatTime, cacheable);
    if (dateFormatter == nullptr) {
        LOG_ECMA(ERROR) << "BuiltinsDate::ToLocaleTimeString:dateFormatter is nullptr";
    }
    ASSERT(dateFormatter != nullptr);
    std::string result = dateFormatter->Format(intl::GlobalIntlHelper::DoubleToInt64(x));
    JSHandle returnValue = factory->NewFromStdString(result);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return returnValue.GetTaggedValue();
#endif
#endif
}

JSTaggedValue BuiltinsDate::ExtractDateFields(JSThread *thread, uint32_t &length, EcmaRuntimeCallInfo *argv,
    JSTaggedValue &timeValue)
{
    std::array<int64_t, DATE_LENGTH> fields = {0, 0, 1, 0, 0, 0, 0, 0, 0};
    if (length > CONSTRUCTOR_MAX_LENGTH) {  // The max length is 7.
        length = CONSTRUCTOR_MAX_LENGTH;
    }
    uint32_t i = 0;
    for (; i < length; ++i) {
        JSHandle<JSTaggedValue> value = GetCallArg(argv, i);
        JSTaggedNumber res = JSTaggedValue::ToNumber(thread, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double temp = JSDate::TimeClip(res.GetNumber());
        if (std::isnan(temp) || !std::isfinite(temp)) {  // Check the double value is finite.
            break;
        }
        fields[i] = static_cast<int64_t>(temp);
        if (i == 0 && fields[0] >= 0 && fields[0] < JSDate::HUNDRED) {
            fields[0] += JSDate::NINETEEN_HUNDRED_YEAR;
        }
    }
    timeValue = JSTaggedValue((i == length) ? JSDate::SetDateValues(&fields, true) : base::NAN_VALUE);
    return timeValue;
}
}  // namespace panda::ecmascript::builtins
