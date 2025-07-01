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

#include "ecmascript/js_displaynames.h"


#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/object_factory-inl.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "unicode/localebuilder.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace panda::ecmascript {

const std::vector<LocaleMatcherOption> JSDisplayNames::LOCALE_MATCHER_OPTION = {
    LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT
};
const std::vector<std::string> JSDisplayNames::LOCALE_MATCHER_OPTION_NAME = {"lookup", "best fit"};

const std::vector<StyOption> JSDisplayNames::STY_OPTION = {
    StyOption::NARROW, StyOption::SHORT, StyOption::LONG
};
const std::vector<std::string> JSDisplayNames::STY_OPTION_NAME = {"narrow", "short", "long"};

const std::vector<TypednsOption> JSDisplayNames::TYPED_NS_OPTION = {
    TypednsOption::LANGUAGE, TypednsOption::REGION,
    TypednsOption::SCRIPT, TypednsOption::CURRENCY,
    TypednsOption::CALENDAR, TypednsOption::DATETIMEFIELD
};
const std::vector<std::string> JSDisplayNames::TYPED_NS_OPTION_NAME = {
    "language", "region", "script", "currency",
    "calendar", "dateTimeField"
};

const std::vector<FallbackOption> JSDisplayNames::FALLBACK_OPTION = {
    FallbackOption::CODE, FallbackOption::NONE
};
const std::vector<std::string> JSDisplayNames::FALLBACK_OPTION_OPTION_NAME = {
    "code", "none"
};

const std::vector<LanguageDisplayOption> JSDisplayNames::LANGUAGE_DISPLAY_OPTION = {
    LanguageDisplayOption::DIALECT, LanguageDisplayOption::STANDARD
};
const std::vector<std::string> JSDisplayNames::LANGUAGE_DISPLAY_OPTION_NAME = {
    "dialect", "standard"
};

icu::LocaleDisplayNames *JSDisplayNames::GetIcuLocaleDisplayNames(JSThread *thread) const
{
    ASSERT(GetIcuLDN(thread).IsJSNativePointer());
    auto result = JSNativePointer::Cast(GetIcuLDN(thread).GetTaggedObject())->GetExternalPointer();
    return reinterpret_cast<icu::LocaleDisplayNames *>(result);
}

void JSDisplayNames::FreeIcuLocaleDisplayNames([[maybe_unused]] void *env, void *pointer, [[maybe_unused]] void* hint)
{
    if (pointer == nullptr) {
        return;
    }
    auto icuLocaleDisplayNames = reinterpret_cast<icu::LocaleDisplayNames *>(pointer);
    delete icuLocaleDisplayNames;
}

void JSDisplayNames::SetIcuLocaleDisplayNames(JSThread *thread, const JSHandle<JSDisplayNames> &displayNames,
                                              icu::LocaleDisplayNames* iculocaledisplaynames,
                                              const NativePointerCallback &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    ASSERT(iculocaledisplaynames != nullptr);
    JSTaggedValue data = displayNames->GetIcuLDN(thread);
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread, iculocaledisplaynames);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(iculocaledisplaynames, callback);
    displayNames->SetIcuLDN(thread, pointer.GetTaggedValue());
}

JSHandle<TaggedArray> JSDisplayNames::GetAvailableLocales(JSThread *thread)
{
    const char *key = "calendar";
    const char *path = nullptr;
    std::vector<std::string> availableStringLocales = intl::LocaleHelper::GetAvailableLocales(thread, key, path);
    JSHandle<TaggedArray> availableLocales = JSLocale::ConstructLocaleList(thread, availableStringLocales);
    return availableLocales;
}

namespace {
    bool IsUnicodeScriptSubtag(const std::string& value)
    {
        UErrorCode status = U_ZERO_ERROR;
        icu::LocaleBuilder builder;
        builder.setScript(value).build(status);
        return U_SUCCESS(status);
    }

    bool IsUnicodeRegionSubtag(const std::string& value)
    {
        UErrorCode status = U_ZERO_ERROR;
        icu::LocaleBuilder builder;
        builder.setRegion(value).build(status);
        return U_SUCCESS(status);
    }
}

// InitializeDisplayNames ( displayNames, locales, options )
JSHandle<JSDisplayNames> JSDisplayNames::InitializeDisplayNames(JSThread *thread,
                                                                const JSHandle<JSDisplayNames> &displayNames,
                                                                const JSHandle<JSTaggedValue> &locales,
                                                                const JSHandle<JSTaggedValue> &options)
{
    [[maybe_unused]] EcmaHandleScope scope(thread);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    auto globalConst = thread->GlobalConstants();
    // 3. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = intl::LocaleHelper::CanonicalizeLocaleList(thread, locales);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 4. If options is undefined, throw a TypeError exception.
    if (options->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "options is undefined", displayNames);
    }

    // 5. Let options be ? GetOptionsObject(options).
    JSHandle<JSObject> optionsObject = JSTaggedValue::ToObject(thread, options);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // Note: No need to create a record. It's not observable.
    // 6. Let opt be a new Record.
    // 7. Let localeData be %DisplayNames%.[[LocaleData]].
    // 8. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    JSHandle<JSTaggedValue> property = globalConst->GetHandledLocaleMatcherString();
    auto matcher = JSLocale::GetOptionOfString<LocaleMatcherOption>(
        thread, optionsObject, property,
        JSDisplayNames::LOCALE_MATCHER_OPTION, JSDisplayNames::LOCALE_MATCHER_OPTION_NAME,
        LocaleMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 10. Let r be ResolveLocale(%DisplayNames%.[[AvailableLocales]], requestedLocales, opt,
    // %DisplayNames%.[[RelevantExtensionKeys]]).
    JSHandle<TaggedArray> availableLocales;
    if (requestedLocales->GetLength() == 0) {
        availableLocales = factory->EmptyArray();
    } else {
        availableLocales = JSDisplayNames::GetAvailableLocales(thread);
    }
    std::set<std::string> relevantExtensionKeys {""};
    ResolvedLocale r =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, relevantExtensionKeys);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);
    icu::Locale icuLocale = r.localeData;

    // 11. Let style be ? GetOption(options, "style", "string", « "narrow", "short", "long" », "long").
    property = globalConst->GetHandledStyleString();
    auto StyOpt = JSLocale::GetOptionOfString<StyOption>(thread, optionsObject, property,
        JSDisplayNames::STY_OPTION, JSDisplayNames::STY_OPTION_NAME,
        StyOption::LONG);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 12. Set DisplayNames.[[Style]] to style.
    displayNames->SetStyle(StyOpt);

    // 13. Let type be ? GetOption(options, "type", "string", « "language", "region", "script", "currency" »,
    // "undefined").
    property = globalConst->GetHandledTypeString();
    auto type = JSLocale::GetOptionOfString<TypednsOption>(thread, optionsObject, property,
        JSDisplayNames::TYPED_NS_OPTION, JSDisplayNames::TYPED_NS_OPTION_NAME,
        TypednsOption::UNDEFINED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 14. If type is undefined, throw a TypeError exception.
    if (type == TypednsOption::UNDEFINED) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "type is undefined", displayNames);
    }

    // 15. Set displayNames.[[Type]] to type.
    displayNames->SetType(type);

    // 16. Let fallback be ? GetOption(options, "fallback", "string", « "code", "none" », "code").
    property = globalConst->GetHandledFallbackString();
    auto fallback = JSLocale::GetOptionOfString<FallbackOption>(thread, optionsObject, property,
        JSDisplayNames::FALLBACK_OPTION, JSDisplayNames::FALLBACK_OPTION_OPTION_NAME,
        FallbackOption::CODE);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);

    // 17. Set displayNames.[[Fallback]] to fallback.
    displayNames->SetFallback(fallback);

    // Let languageDisplay be ? GetOption(options, "languageDisplay", string, « "dialect", "standard" », "dialect").
    property = globalConst->GetHandledLanguageDisplayString();
    auto langDisplay = JSLocale::GetOptionOfString<LanguageDisplayOption>(
        thread, optionsObject, property,
        JSDisplayNames::LANGUAGE_DISPLAY_OPTION,
        JSDisplayNames::LANGUAGE_DISPLAY_OPTION_NAME, LanguageDisplayOption::DIALECT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);
    displayNames->SetLanguageDisplay(langDisplay);

    // 18. Set displayNames.[[Locale]] to the value of r.[[Locale]].
    JSHandle<EcmaString> localeStr = intl::LocaleHelper::ToLanguageTag(thread, icuLocale);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSDisplayNames, thread);
    displayNames->SetLocale(thread, localeStr.GetTaggedValue());
    // 19. Let dataLocale be r.[[dataLocale]].
    // 20. Let dataLocaleData be localeData.[[<dataLocale>]].
    // 21. Let types be dataLocaleData.[[types]].
    // 22. Assert: types is a Record (see 12.3.3).
    // 23. Let typeFields be types.[[<type>]].
    // 24. Assert: typeFields is a Record (see 12.3.3).
    // 25. Let styleFields be typeFields.[[<style>]].
    // 26. Assert: styleFields is a Record (see 12.3.3).
    // 27. Set displayNames.[[Fields]] to styleFields.
    // 28. Return displayNames.

    // Trans StyOption to ICU Style
    UDisplayContext uStyle;
    switch (StyOpt) {
        case StyOption::LONG:
            uStyle = UDISPCTX_LENGTH_FULL;
            break;
        case StyOption::SHORT:
            uStyle = UDISPCTX_LENGTH_SHORT;
            break;
        case StyOption::NARROW:
            uStyle = UDISPCTX_LENGTH_SHORT;
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    UDisplayContext displayContext[] = {uStyle};
    icu::LocaleDisplayNames *icudisplaynames(icu::LocaleDisplayNames::createInstance(icuLocale, displayContext, 1));
    if (icudisplaynames == nullptr) {
        delete icudisplaynames;
        THROW_RANGE_ERROR_AND_RETURN(thread, "create icu::LocaleDisplayNames failed", displayNames);
    }
    SetIcuLocaleDisplayNames(thread, displayNames, icudisplaynames, JSDisplayNames::FreeIcuLocaleDisplayNames);
    return displayNames;
}

// CanonicalCodeForDisplayNames ( type, code )
JSHandle<EcmaString> JSDisplayNames::CanonicalCodeForDisplayNames(JSThread *thread,
                                                                  const JSHandle<JSDisplayNames> &displayNames,
                                                                  const TypednsOption &typeOpt,
                                                                  const JSHandle<EcmaString> &code)
{
    if (typeOpt == TypednsOption::LANGUAGE) {
        // a. If code does not match the unicode_language_id production, throw a RangeError exception.
        UErrorCode status = U_ZERO_ERROR;
        std::string codeSt = intl::LocaleHelper::ConvertToStdString(thread, code);
        icu::Locale loc = icu::Locale(icu::Locale::forLanguageTag(codeSt, status).getBaseName());
        std::string checked = loc.toLanguageTag<std::string>(status);
        if (checked.size() == 0) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "not match the language id", code);
        }
        if (U_FAILURE(status)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "not match the unicode_language_id", code);
        }
        // b. If IsStructurallyValidLanguageTag(code) is false, throw a RangeError exception.
        // c. Set code to CanonicalizeUnicodeLocaleId(code).
        // d. Return code.
        if (!intl::LocaleHelper::IsStructurallyValidLanguageTag(thread, code)) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "not a structurally valid", code);
        }
        JSHandle<EcmaString> codeStr = intl::LocaleHelper::CanonicalizeUnicodeLocaleId(thread, code);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
        icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames(thread);
        icu::UnicodeString result;
        std::string codeString = intl::LocaleHelper::ConvertToStdString(thread, codeStr);
        icuLocaldisplaynames->languageDisplayName(codeString.c_str(), result);
        JSHandle<EcmaString> codeResult = intl::LocaleHelper::UStringToString(thread, result);
        return codeResult;
    } else if (typeOpt == TypednsOption::REGION) {
        // a. If code does not match the unicode_region_subtag production, throw a RangeError exception.
        std::string regionCode = intl::LocaleHelper::ConvertToStdString(thread, code);
        if (!IsUnicodeRegionSubtag(regionCode)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid region", code);
        }
        // b. Let code be the result of mapping code to upper case as described in 6.1.
        // c. Return code.
        icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames(thread);
        icu::UnicodeString result;
        icuLocaldisplaynames->regionDisplayName(regionCode.c_str(), result);
        JSHandle<EcmaString> codeResult = intl::LocaleHelper::UStringToString(thread, result);
        return codeResult;
    } else if (typeOpt == TypednsOption::SCRIPT) {
        std::string scriptCode = intl::LocaleHelper::ConvertToStdString(thread, code);
        if (!IsUnicodeScriptSubtag(scriptCode)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid script", code);
        }
        icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames(thread);
        icu::UnicodeString result;
        icuLocaldisplaynames->scriptDisplayName(scriptCode.c_str(), result);
        JSHandle<EcmaString> codeResult = intl::LocaleHelper::UStringToString(thread, result);
        return codeResult;
    } else if (typeOpt == TypednsOption::CALENDAR) {
        std::string calendarCode = intl::LocaleHelper::ConvertToStdString(thread, code);
        if (!JSLocale::IsWellFormedCalendarCode(calendarCode)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid calendar", code);
        }

        icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames(thread);
        icu::UnicodeString result;
        std::string calendarStrCode = std::strcmp(calendarCode.c_str(), "gregory") == 0
                                        ? "gregorian"
                                        : std::strcmp(calendarCode.c_str(), "ethioaa") == 0
                                            ? "ethiopic-amete-alem"
                                            : calendarCode;
        icuLocaldisplaynames->keyValueDisplayName("calendar", calendarStrCode.c_str(), result);
        JSHandle<EcmaString> codeResult = intl::LocaleHelper::UStringToString(thread, result);
        return codeResult;
    } else if (typeOpt == TypednsOption::DATETIMEFIELD) {
        StyOption style = displayNames->GetStyle();
        UDateTimePGDisplayWidth width;
        switch (style) {
            case StyOption::LONG:
                width = UDATPG_WIDE;
                break;
            case StyOption::SHORT:
                width = UDATPG_ABBREVIATED;
                break;
            case StyOption::NARROW:
                width = UDATPG_NARROW;
                break;
            default:
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
        }
        std::string datetimeCode = intl::LocaleHelper::ConvertToStdString(thread, code);
        UDateTimePatternField field = StringToUDateTimePatternField(datetimeCode.c_str());
        if (field == UDATPG_FIELD_COUNT) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid datetimefield", code);
        }

        UErrorCode status = U_ZERO_ERROR;
        icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames(thread);
        icu::Locale locales = icuLocaldisplaynames->getLocale();
        std::unique_ptr<icu::DateTimePatternGenerator> generator(
            icu::DateTimePatternGenerator::createInstance(locales, status));
        icu::UnicodeString result = generator->getFieldDisplayName(field, width);
        return intl::LocaleHelper::UStringToString(thread, result);
    }
    // 4. 4. Assert: type is "currency".
    // 5. If ! IsWellFormedCurrencyCode(code) is false, throw a RangeError exception.
    ASSERT(typeOpt == TypednsOption::CURRENCY);
    std::string cCode = intl::LocaleHelper::ConvertToStdString(thread, code);
    if (!JSLocale::IsWellFormedCurrencyCode(cCode)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "not a wellformed currency code", code);
    }
    icu::LocaleDisplayNames *icuLocaldisplaynames = displayNames->GetIcuLocaleDisplayNames(thread);
    icu::UnicodeString result;
    icuLocaldisplaynames->keyValueDisplayName("currency", cCode.c_str(), result);
    JSHandle<EcmaString> codeResult = intl::LocaleHelper::UStringToString(thread, result);
    return codeResult;
}

UDateTimePatternField JSDisplayNames::StringToUDateTimePatternField(const char* code)
{
    if (std::strcmp(code, "day") == 0) {
        return UDATPG_DAY_FIELD;
    }
    if (std::strcmp(code, "dayPeriod") == 0) {
        return UDATPG_DAYPERIOD_FIELD;
    }
    if (std::strcmp(code, "era") == 0) {
        return UDATPG_ERA_FIELD;
    }
    if (std::strcmp(code, "hour") == 0) {
        return UDATPG_HOUR_FIELD;
    }
    if (std::strcmp(code, "minute") == 0) {
        return UDATPG_MINUTE_FIELD;
    }
    if (std::strcmp(code, "month") == 0) {
        return UDATPG_MONTH_FIELD;
    }
    if (std::strcmp(code, "quarter") == 0) {
        return UDATPG_QUARTER_FIELD;
    }
    if (std::strcmp(code, "second") == 0) {
        return UDATPG_SECOND_FIELD;
    }
    if (std::strcmp(code, "timeZoneName") == 0) {
        return UDATPG_ZONE_FIELD;
    }
    if (std::strcmp(code, "weekOfYear") == 0) {
        return UDATPG_WEEK_OF_YEAR_FIELD;
    }
    if (std::strcmp(code, "weekday") == 0) {
        return UDATPG_WEEKDAY_FIELD;
    }
    if (std::strcmp(code, "year") == 0) {
        return UDATPG_YEAR_FIELD;
    }
    return UDATPG_FIELD_COUNT;
}

JSHandle<JSTaggedValue> StyOptionToEcmaString(JSThread *thread, StyOption style)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (style) {
        case StyOption::LONG:
            result.Update(globalConst->GetHandledLongString().GetTaggedValue());
            break;
        case StyOption::SHORT:
            result.Update(globalConst->GetHandledShortString().GetTaggedValue());
            break;
        case StyOption::NARROW:
            result.Update(globalConst->GetHandledNarrowString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> TypeOptionToEcmaString(JSThread *thread, TypednsOption type)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (type) {
        case TypednsOption::LANGUAGE:
            result.Update(globalConst->GetHandledLanguageString().GetTaggedValue());
            break;
        case TypednsOption::CALENDAR:
            result.Update(globalConst->GetHandledCalendarString().GetTaggedValue());
            break;
        case TypednsOption::CURRENCY:
            result.Update(globalConst->GetHandledCurrencyString().GetTaggedValue());
            break;
        case TypednsOption::DATETIMEFIELD:
            result.Update(globalConst->GetHandledDateTimeFieldString().GetTaggedValue());
            break;
        case TypednsOption::REGION:
            result.Update(globalConst->GetHandledRegionString().GetTaggedValue());
            break;
        case TypednsOption::SCRIPT:
            result.Update(globalConst->GetHandledScriptString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> FallbackOptionToEcmaString(JSThread *thread, FallbackOption fallback)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (fallback) {
        case FallbackOption::CODE:
            result.Update(globalConst->GetHandledCodeString().GetTaggedValue());
            break;
        case FallbackOption::NONE:
            result.Update(globalConst->GetHandledNoneString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> LanguageDisplayOptionToEcmaString(JSThread *thread, LanguageDisplayOption langDisplay)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (langDisplay) {
        case LanguageDisplayOption::DIALECT:
            result.Update(globalConst->GetHandledDialectString().GetTaggedValue());
            break;
        case LanguageDisplayOption::STANDARD:
            result.Update(globalConst->GetHandledStandardString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

void JSDisplayNames::ResolvedOptions(JSThread *thread, const JSHandle<JSDisplayNames> &displayNames,
                                     const JSHandle<JSObject> &options)
{
    auto globalConst = thread->GlobalConstants();

    // [[Locale]]
    JSHandle<JSTaggedValue> propertyKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> locale(thread, displayNames->GetLocale(thread));
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, locale);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[Style]]
    StyOption style = displayNames->GetStyle();
    propertyKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleString = StyOptionToEcmaString(thread, style);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, styleString);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[type]]
    TypednsOption type = displayNames->GetType();
    propertyKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> typeString = TypeOptionToEcmaString(thread, type);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, typeString);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[fallback]]
    FallbackOption fallback = displayNames->GetFallback();
    propertyKey = globalConst->GetHandledFallbackString();
    JSHandle<JSTaggedValue> fallbackString = FallbackOptionToEcmaString(thread, fallback);
    JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, fallbackString);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[languageDisplay]]
    // The default languageDisplay is 'dialect' if type is 'language'
    if (type == TypednsOption::LANGUAGE) {
        LanguageDisplayOption langDisplay = displayNames->GetLanguageDisplay();
        propertyKey = globalConst->GetHandledLanguageDisplayString();
        JSHandle<JSTaggedValue> langDisplayString = LanguageDisplayOptionToEcmaString(thread, langDisplay);
        JSObject::CreateDataPropertyOrThrow(thread, options, propertyKey, langDisplayString);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
}
}  // namespace panda::ecmascript
