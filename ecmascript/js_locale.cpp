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

#include "ecmascript/js_locale.h"

#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/checkpoint/thread_state_transition.h"

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
const std::string JSLocale::LATN_STRING = "latn";

const std::vector<LocaleMatcherOption> JSLocale::LOCALE_MATCHER_OPTION = {
    LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT
};
const std::vector<std::string> JSLocale::LOCALE_MATCHER_OPTION_NAME = {
    "lookup", "best fit"
};

const std::map<std::string, std::set<std::string>> JSLocale::LOCALE_MAP = {
    {"hc", {"h11", "h12", "h23", "h24"}},
    {"lb", {"strict", "normal", "loose"}},
    {"kn", {"true", "false"}},
    {"kf", {"upper", "lower", "false"}}
};

const std::vector<std::string> JSLocale::HOUR_CYCLE = {"h11", "h12", "h23", "h24"};
const std::vector<std::string> JSLocale::CASE_FIRST = {"upper", "lower", "false"};

const std::set<std::string> JSLocale::WELL_NUMBER_SYSTEM = {"native", "traditio", "finance"};
const std::set<std::string> JSLocale::WELL_COLLATION = {"standard", "search"};
// 6.4.1 IsValidTimeZoneName ( timeZone )
bool JSLocale::IsValidTimeZoneName(const icu::TimeZone &tz)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString id;
    tz.getID(id);
    icu::UnicodeString canonical;
    icu::TimeZone::getCanonicalID(id, canonical, status);
    UBool canonicalFlag = (canonical != icu::UnicodeString("Etc/Unknown", -1, US_INV));
    return (U_SUCCESS(status) != 0) && (canonicalFlag != 0);
}

// 9.2.3 LookupMatcher ( availableLocales, requestedLocales )
JSHandle<EcmaString> JSLocale::LookupMatcher(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                             const JSHandle<TaggedArray> &requestedLocales)
{
    MatcherResult result = {std::string(), std::string()};
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. Let result be a new Record.
    // 2. For each element locale of requestedLocales in List order, do
    std::vector<std::string> availableStringLocales = GetAvailableStringLocales(thread, availableLocales);
    uint32_t length = requestedLocales->GetLength();
    JSMutableHandle<EcmaString> locale(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < length; ++i) {
        locale.Update(requestedLocales->Get(thread, i));
        // 2. a. Let noExtensionsLocale be the String value that is locale
        //       with all Unicode locale extension sequences removed.
        intl::LocaleHelper::ParsedLocale parsedResult = intl::LocaleHelper::HandleLocale(locale);
        // 2. b. Let availableLocale be BestAvailableLocale(availableLocales, noExtensionsLocale).
        std::string availableLocale =
            intl::LocaleHelper::BestAvailableLocale(availableStringLocales, parsedResult.base);
        // 2. c. If availableLocale is not undefined, append locale to the end of subset.
        if (!availableLocale.empty()) {
            result = {std::string(), std::string()};
            // 2. c. i. Set result.[[locale]] to availableLocale.
            result.locale = availableLocale;
            // 2. c. ii. If locale and noExtensionsLocale are not the same String value, then
            // 2. c. ii. 1. Let extension be the String value consisting of  the first substring of locale that is a
            //              Unicode locale extension sequence.
            if (!parsedResult.extension.empty()) {
                result.extension = parsedResult.extension;
            }
            // 2. c. ii. 2. Set result.[[extension]] to extension.
            std::string res = result.locale + result.extension;
            // 2. c. iii. Return result.
            return factory->NewFromStdString(res);
        }
    }

    // 3. Let defLocale be DefaultLocale();
    // 4. Set result.[[locale]] to defLocale.
    // 5. Return result.
    auto defLocale = intl::LocaleHelper::StdStringDefaultLocale(thread);
    result.locale = defLocale;
    return factory->NewFromStdString(result.locale);
}

icu::LocaleMatcher BuildLocaleMatcher(JSThread *thread, uint32_t *availableLength, UErrorCode *status,
                                      const JSHandle<TaggedArray> &availableLocales)
{
    std::string locale = intl::LocaleHelper::StdStringDefaultLocale(thread);
    icu::Locale defaultLocale = icu::Locale::forLanguageTag(locale, *status);
    ASSERT_PRINT(U_SUCCESS(*status), "icu::Locale::forLanguageTag failed");
    icu::LocaleMatcher::Builder builder;
    builder.setDefaultLocale(&defaultLocale);
    uint32_t length = availableLocales->GetLength();

    JSMutableHandle<EcmaString> item(thread, JSTaggedValue::Undefined());
    for (*availableLength = 0; *availableLength < length; ++(*availableLength)) {
        item.Update(availableLocales->Get(thread, *availableLength));
        std::string itemStr = intl::LocaleHelper::ConvertToStdString(item);
        icu::Locale localeForLanguageTag = icu::Locale::forLanguageTag(itemStr, *status);
        if (U_SUCCESS(*status) != 0) {
            builder.addSupportedLocale(localeForLanguageTag);
        } else {
            break;
        }
    }
    *status = U_ZERO_ERROR;
    return builder.build(*status);
}

// 9.2.4 BestFitMatcher ( availableLocales, requestedLocales )
JSHandle<EcmaString> JSLocale::BestFitMatcher(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                              const JSHandle<TaggedArray> &requestedLocales)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    UErrorCode status = U_ZERO_ERROR;
    uint32_t availableLength = availableLocales->GetLength();
    icu::LocaleMatcher matcher = BuildLocaleMatcher(thread, &availableLength, &status, availableLocales);
    ASSERT(U_SUCCESS(status));

    uint32_t requestedLocalesLength = requestedLocales->GetLength();
    JSIntlIterator iter(requestedLocales, requestedLocalesLength);
    auto bestFit = matcher.getBestMatch(iter, status)->toLanguageTag<std::string>(status);

    if (U_FAILURE(status) != 0) {
        return intl::LocaleHelper::DefaultLocale(thread);
    }

    for (uint32_t i = 0; i < requestedLocalesLength; ++i) {
        if (iter[i] == bestFit) {
            return JSHandle<EcmaString>(thread, requestedLocales->Get(thread, i));
        }
    }
    return factory->NewFromStdString(bestFit);
}

// 9.2.8 LookupSupportedLocales ( availableLocales, requestedLocales )
JSHandle<TaggedArray> JSLocale::LookupSupportedLocales(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                                       const JSHandle<TaggedArray> &requestedLocales)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t length = requestedLocales->GetLength();
    JSMutableHandle<EcmaString> item(thread, JSTaggedValue::Undefined());
    std::vector<std::string> availableStringLocales = GetAvailableStringLocales(thread, availableLocales);
    std::vector<std::string> convertedLocales;
    convertedLocales.reserve(length);
    std::vector<uint32_t> indexAvailableLocales;
    indexAvailableLocales.reserve(length);
    for (uint32_t i = 0; i < length; ++i) {
        item.Update(requestedLocales->Get(thread, i));
        convertedLocales.push_back(intl::LocaleHelper::ConvertToStdString(item));
    }
    // 1. For each element locale of requestedLocales in List order, do
    //    a. Let noExtensionsLocale be the String value that is locale with all Unicode locale extension sequences
    //       removed.
    //    b. Let availableLocale be BestAvailableLocale(availableLocales, noExtensionsLocale).
    //    c. If availableLocale is not undefined, append locale to the end of subset.
    {
        ThreadNativeScope nativeScope(thread);
        for (uint32_t i = 0; i < length; ++i) {
            intl::LocaleHelper::ParsedLocale foundationResult = intl::LocaleHelper::HandleLocale(convertedLocales[i]);
            std::string availableLocale =
                intl::LocaleHelper::BestAvailableLocale(availableStringLocales, foundationResult.base);
            if (!availableLocale.empty()) {
                indexAvailableLocales.push_back(i);
            }
        }
    }
    // 2. Let subset be a new empty List.
    JSHandle<TaggedArray> subset = factory->NewTaggedArray(indexAvailableLocales.size());
    uint32_t index = 0;
    for (uint32_t i = 0; i < indexAvailableLocales.size(); ++i) {
        subset->Set(thread, index++, requestedLocales->Get(thread, indexAvailableLocales[i]));
    }
    // 3. Return subset.
    return subset;
}

// 9.2.9 BestFitSupportedLocales ( availableLocales, requestedLocales )
JSHandle<TaggedArray> JSLocale::BestFitSupportedLocales(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                                        const JSHandle<TaggedArray> &requestedLocales)
{
    UErrorCode status = U_ZERO_ERROR;
    uint32_t requestLength = requestedLocales->GetLength();
    uint32_t availableLength = availableLocales->GetLength();
    icu::LocaleMatcher matcher = BuildLocaleMatcher(thread, &availableLength, &status, availableLocales);
    ASSERT(U_SUCCESS(status));

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> defaultLocale = intl::LocaleHelper::DefaultLocale(thread);
    JSHandle<TaggedArray> result = factory->NewTaggedArray(requestLength);

    uint32_t index = 0;
    JSMutableHandle<EcmaString> locale(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < requestLength; ++i) {
        locale.Update(requestedLocales->Get(thread, i));
        if (EcmaStringAccessor::StringsAreEqual(thread->GetEcmaVM(), locale, defaultLocale)) {
            result->Set(thread, index++, locale.GetTaggedValue());
        } else {
            status = U_ZERO_ERROR;
            std::string localeStr = intl::LocaleHelper::ConvertToStdString(locale);
            icu::Locale desired = icu::Locale::forLanguageTag(localeStr, status);
            auto bestFit = matcher.getBestMatch(desired, status)->toLanguageTag<std::string>(status);
            if ((U_SUCCESS(status) != 0) &&
                EcmaStringAccessor::StringsAreEqual(thread->GetEcmaVM(), locale, factory->NewFromStdString(bestFit))) {
                result->Set(thread, index++, locale.GetTaggedValue());
            }
        }
    }
    result = TaggedArray::SetCapacity(thread, result, index);
    return result;
}

// 9.2.10 SupportedLocales ( availableLocales, requestedLocales, options )
JSHandle<JSArray> JSLocale::SupportedLocales(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                             const JSHandle<TaggedArray> &requestedLocales,
                                             const JSHandle<JSTaggedValue> &options)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. If options is not undefined, then
    //    a. Let options be ? ToObject(options).
    //    b. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    // 2. Else, let matcher be "best fit".
    if (!options->IsUndefined()) {
        JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, options);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);

        [[maybe_unused]] LocaleMatcherOption matcher = GetOptionOfString<LocaleMatcherOption>(thread,
            obj, globalConst->GetHandledLocaleMatcherString(),
            JSLocale::LOCALE_MATCHER_OPTION, JSLocale::LOCALE_MATCHER_OPTION_NAME,
            LocaleMatcherOption::BEST_FIT);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
    }

    // 3. If matcher is "best fit", then
    //    a. Let supportedLocales be BestFitSupportedLocales(availableLocales, requestedLocales).
    // 4. Else,
    //    a. Let supportedLocales be LookupSupportedLocales(availableLocales, requestedLocales).
    JSMutableHandle<TaggedArray> supportedLocales(thread, JSTaggedValue::Undefined());
    supportedLocales.Update(LookupSupportedLocales(thread, availableLocales, requestedLocales).GetTaggedValue());

    JSHandle<JSArray> subset = JSArray::CreateArrayFromList(thread, supportedLocales);
    // 5. Return CreateArrayFromList(supportedLocales).
    return subset;
}

// 9.2.11 GetOption ( options, property, type, values, fallback )
JSHandle<JSTaggedValue> JSLocale::GetOption(JSThread *thread, const JSHandle<JSObject> &options,
                                            const JSHandle<JSTaggedValue> &property, OptionType type,
                                            const JSHandle<JSTaggedValue> &values,
                                            const JSHandle<JSTaggedValue> &fallback)
{
    // 1. Let value be ? Get(options, property).
    JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, options, property).GetValue();
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);

    // 2. If value is not undefined, then
    if (!value->IsUndefined()) {
        // a. Assert: type is "boolean" or "string".
        ASSERT_PRINT(type == OptionType::BOOLEAN || type == OptionType::STRING, "type is not boolean or string");

        // b. If type is "boolean", then
        //    i. Let value be ToBoolean(value).
        if (type == OptionType::BOOLEAN) {
            value = JSHandle<JSTaggedValue>(thread, JSTaggedValue(value->ToBoolean()));
        }
        // c. If type is "string", then
        //    i. Let value be ? ToString(value).
        if (type == OptionType::STRING) {
            JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, value);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            value = JSHandle<JSTaggedValue>(thread, str.GetTaggedValue());
        }

        // d. If values is not undefined, then
        //    i. If values does not contain an element equal to value, throw a RangeError exception.
        if (!values->IsUndefined()) {
            bool isExist = false;
            JSHandle<TaggedArray> valuesArray = JSHandle<TaggedArray>::Cast(values);
            uint32_t length = valuesArray->GetLength();
            for (uint32_t i = 0; i < length; i++) {
                if (JSTaggedValue::SameValue(valuesArray->Get(thread, i), value.GetTaggedValue())) {
                    isExist = true;
                }
            }
            if (!isExist) {
                JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
                THROW_RANGE_ERROR_AND_RETURN(thread, "values does not contain an element equal to value", exception);
            }
        }
        // e. Return value.
        return value;
    }
    // 3. Else, return fallback.
    return fallback;
}

bool JSLocale::GetOptionOfString(JSThread *thread, const JSHandle<JSObject> &options,
                                 const JSHandle<JSTaggedValue> &property, const std::vector<std::string> &values,
                                 std::string *optionValue)
{
    // 1. Let value be ? Get(options, property).
    OperationResult operationResult = JSObject::GetProperty(thread, options, property);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    JSHandle<JSTaggedValue> value = operationResult.GetValue();
    // 2. If value is not undefined, then
    if (value->IsUndefined()) {
        return false;
    }
    //    c. If type is "string" "string", then
    //       i. Let value be ? ToString(value).
    JSHandle<EcmaString> valueEStr = JSTaggedValue::ToString(thread, value);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (EcmaStringAccessor(valueEStr).IsUtf16()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "Value out of range for locale options property", false);
    }
    *optionValue = intl::LocaleHelper::ConvertToStdString(valueEStr);
    if (values.empty()) {
        return true;
    }
    // d. If values is not undefined, then
    //    i. If values does not contain an element equal to value, throw a RangeError exception.
    for (const auto &item : values) {
        if (item == *optionValue) {
            return true;
        }
    }
    THROW_RANGE_ERROR_AND_RETURN(thread, "Value out of range for locale options property", false);
}

// 9.2.12 DefaultNumberOption ( value, minimum, maximum, fallback )
int JSLocale::DefaultNumberOption(JSThread *thread, const JSHandle<JSTaggedValue> &value, int minimum, int maximum,
                                  int fallback)
{
    // 1. If value is not undefined, then
    if (!value->IsUndefined()) {
        // a. Let value be ? ToNumber(value).
        JSTaggedNumber number = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fallback);
        // b. If value is NaN or less than minimum or greater than maximum, throw a RangeError exception.
        double num = JSTaggedValue(number).GetNumber();
        if (std::isnan(num) || num < minimum || num > maximum) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "", fallback);
        }
        // c. Return floor(value).
        return std::floor(num);
    }
    // 2. Else, return fallback.
    return fallback;
}

// 9.2.13 GetNumberOption ( options, property, minimum, maximum, fallback )
int JSLocale::GetNumberOption(JSThread *thread, const JSHandle<JSObject> &options,
                              const JSHandle<JSTaggedValue> &property, int min, int max, int fallback)
{
    // 1. Let value be ? Get(options, property).
    JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, options, property).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fallback);

    // 2. Return ? DefaultNumberOption(value, minimum, maximum, fallback).
    int result = DefaultNumberOption(thread, value, min, max, fallback);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fallback);
    return result;
}

// 9.2.5 UnicodeExtensionValue ( extension, key )
std::string JSLocale::UnicodeExtensionValue(const std::string extension, const std::string key)
{
    // 1. Assert: The number of elements in key is 2.
    // 2. Let size be the number of elements in extension.
    ASSERT(key.size() == INTL_INDEX_TWO || key.size() == INTL_INDEX_ZERO);
    size_t size = extension.size();
    // 3. Let searchValue be the concatenation of "-" , key, and "-".
    std::string searchValue = "-" + key + "-";
    // 4. Let pos be Call(%StringProto_indexOf%, extension, « searchValue »).
    size_t pos = extension.find(searchValue);
    // 5. If pos ≠ -1, then
    if (pos != std::string::npos) {
        // a. Let start be pos + 4.
        size_t start = pos + INTL_INDEX_FOUR;
        // b. Let end be start.
        size_t end = start;
        // c. Let k be start.
        size_t k = start;
        // d. Let done be false.
        bool done = false;
        // e. Repeat, while done is false
        while (!done) {
            // i. Let e be Call(%StringProto_indexOf%, extension, « "-", k »).
            size_t e = extension.find("-", k);
            size_t len;
            // ii. If e = -1, let len be size - k; else let len be e - k.
            if (e == std::string::npos) {
                len = size - k;
            } else {
                len = e - k;
            }
            // iii. If len = 2, then
            //     1. Let done be true.
            if (len == INTL_INDEX_TWO) {
                done = true;
            // iv. Else if e = -1, then
            //    1. Let end be size.
            //    2. Let done be true.
            } else if (e == std::string::npos) {
                end = size;
                done = true;
            // v. Else,
            //   1. Let end be e.
            //   2. Let k be e + 1.
            } else {
                end = e;
                k = e + INTL_INDEX_ONE;
            }
        }
        // f. Return the String value equal to the substring of extension consisting of the code units at indices.
        // start (inclusive) through end (exclusive).
        std::string result = extension.substr(start, end - start);
        return result;
    }
    // 6. Let searchValue be the concatenation of "-" and key.
    searchValue = "-" + key;
    // 7. Let pos be Call(%StringProto_indexOf%, extension, « searchValue »).
    pos = extension.find(searchValue);
    // 8. If pos ≠ -1 and pos + 3 = size, then
    //    a. Return the empty String.
    if (pos != std::string::npos && pos + INTL_INDEX_THREE == size) {
        return "";
    }
    // 9. Return undefined.
    return "undefined";
}

ResolvedLocale JSLocale::ResolveLocale(JSThread *thread, const JSHandle<TaggedArray> &availableLocales,
                                       const JSHandle<TaggedArray> &requestedLocales,
                                       [[maybe_unused]] LocaleMatcherOption matcher,
                                       const std::set<std::string> &relevantExtensionKeys)
{
    // 1. Let matcher be options.[[localeMatcher]].
    // 2. If matcher is "lookup" "lookup", then
    //    a. Let r be LookupMatcher(availableLocales, requestedLocales).
    // 3. Else,
    //    a. Let r be BestFitMatcher(availableLocales, requestedLocales).
    JSMutableHandle<EcmaString> locale(thread, JSTaggedValue::Undefined());
    if (availableLocales->GetLength() == 0 && requestedLocales->GetLength() == 0) {
        locale.Update(intl::LocaleHelper::DefaultLocale(thread).GetTaggedValue());
    } else {
        locale.Update(LookupMatcher(thread, availableLocales, requestedLocales).GetTaggedValue());
    }

    // 4. Let foundLocale be r.[[locale]].
    // 5. Let result be a new Record.
    // 6. Set result.[[dataLocale]] to foundLocale.
    // 7. Let supportedExtension be "-u".
    std::string foundLocale = intl::LocaleHelper::ConvertToStdString(locale);
    icu::Locale foundLocaleData = BuildICULocale(foundLocale);
    ResolvedLocale result;
    result.localeData = foundLocaleData;
    JSHandle<EcmaString> tag = intl::LocaleHelper::ToLanguageTag(thread, foundLocaleData);
    result.locale = intl::LocaleHelper::ConvertToStdString(tag);
    std::string supportedExtension = "-u";
    icu::LocaleBuilder localeBuilder;
    localeBuilder.setLocale(foundLocaleData).clearExtensions();
    // 8. For each element key of relevantExtensionKeys in List order, do
    for (auto &key : relevantExtensionKeys) {
        auto doubleMatch = foundLocale.find(key);
        if (doubleMatch == std::string::npos) {
            continue;
        }
        UErrorCode status = U_ZERO_ERROR;
        std::set<std::string> keyLocaleData;
        std::unique_ptr<icu::StringEnumeration> wellFormKey(foundLocaleData.createKeywords(status));
        if (U_FAILURE(status) != 0) {
            return result;
        }
        if (!wellFormKey) {
            return result;
        }
        std::string value;

        // c. Let keyLocaleData be foundLocaleData.[[<key>]].
        // e. Let value be keyLocaleData[0].
        if ((key != "ca") && (key != "co") && (key != "nu")) {
            auto find = JSLocale::LOCALE_MAP.find(key);
            if (find != JSLocale::LOCALE_MAP.end()) {
                keyLocaleData = find->second;
            }
            if (key == "") {
                keyLocaleData = JSLocale::LOCALE_MAP.at("lb");
            }
            value = *keyLocaleData.begin();
        }

        // g. Let supportedExtensionAddition be "".
        // h. If r has an [[extension]] field, then
        std::string supportedExtensionAddition;
        size_t found = foundLocale.find("-u-");
        if (found != std::string::npos) {
            std::string extension = foundLocale.substr(found + INTL_INDEX_ONE);

            // i. Let requestedValue be UnicodeExtensionValue(r.[[extension]], key).
            std::string requestedValue = UnicodeExtensionValue(extension, key);
            if (key == "kn" && requestedValue.empty()) {
                requestedValue = "true";
            }

            // ii. If requestedValue is not undefined, then
            if (requestedValue != "undefined") {
                // 1. If requestedValue is not the empty String, then
                if (!requestedValue.empty()) {
                    // a. If keyLocaleData contains requestedValue, then
                    //    i. Let value be requestedValue.
                    //    ii. Let supportedExtensionAddition be the concatenation of "-", key, "-", and value.
                    if (key == "ca" || key == "co") {
                        if (key == "co") {
                            bool isValidValue = IsWellCollation(foundLocaleData, requestedValue);
                            if (!isValidValue) {
                                continue;
                            }
                            value = requestedValue;
                            supportedExtensionAddition = "-" + key + "-" + value;
                            localeBuilder.setUnicodeLocaleKeyword(key, requestedValue);
                        } else {
                            bool isValidValue = IsWellCalendar(foundLocaleData, requestedValue);
                            if (!isValidValue) {
                                continue;
                            }
                            value = requestedValue;
                            supportedExtensionAddition = "-" + key + "-" + value;
                            localeBuilder.setUnicodeLocaleKeyword(key, requestedValue);
                        }
                    } else if (key == "nu") {
                        bool isValidValue = IsWellNumberingSystem(requestedValue);
                        if (!isValidValue) {
                            continue;
                        }
                        value = requestedValue;
                        supportedExtensionAddition = "-" + key + "-" + value;
                        localeBuilder.setUnicodeLocaleKeyword(key, requestedValue);
                    } else if (keyLocaleData.find(requestedValue) != keyLocaleData.end()) {
                        value = requestedValue;
                        supportedExtensionAddition = "-" + key + "-" + value;
                        localeBuilder.setUnicodeLocaleKeyword(key, requestedValue);
                    }
                }
            }
        }
        result.extensions.emplace(key, value);
        supportedExtension += supportedExtensionAddition;
    }
    size_t found = foundLocale.find("-u-");
    if (found != std::string::npos) {
        foundLocale = foundLocale.substr(0, found);
    }

    // 9. If the number of elements in supportedExtension is greater than 2, then
    if (supportedExtension.size() > 2) {
        // a. Let privateIndex be Call(%StringProto_indexOf%, foundLocale, « "-x-" »).
        size_t privateIndex = foundLocale.find("-x-");
        // b. If privateIndex = -1, then
        //    i. Let foundLocale be the concatenation of foundLocale and supportedExtension.
        if (privateIndex == std::string::npos) {
            foundLocale = foundLocale + supportedExtension;
        } else {
            std::string preExtension = foundLocale.substr(0, privateIndex);
            std::string postExtension = foundLocale.substr(privateIndex);
            foundLocale = preExtension + supportedExtension + postExtension;
        }

        tag = intl::LocaleHelper::ToLanguageTag(thread, foundLocaleData);
        if (!intl::LocaleHelper::IsStructurallyValidLanguageTag(tag)) {
            result.extensions.erase(result.extensions.begin(), result.extensions.end());
            result.locale = foundLocale;
        }
        tag = intl::LocaleHelper::CanonicalizeUnicodeLocaleId(thread, tag);
        foundLocale = intl::LocaleHelper::ConvertToStdString(tag);
    }

    // 10. Set result.[[locale]] to foundLocale.
    result.locale = foundLocale;
    UErrorCode status = U_ZERO_ERROR;
    foundLocaleData = localeBuilder.build(status);
    result.localeData = foundLocaleData;

    // 11. Return result.
    return result;
}

icu::Locale JSLocale::BuildICULocale(const std::string &bcp47Locale)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale icuLocale = icu::Locale::forLanguageTag(bcp47Locale, status);
    return icuLocale;
}

JSHandle<TaggedArray> JSLocale::ConstructLocaleList(JSThread *thread,
                                                    const std::vector<std::string> &icuAvailableLocales)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<TaggedArray> locales = factory->NewTaggedArray(icuAvailableLocales.size());
    int32_t index = 0;
    for (const std::string &locale : icuAvailableLocales) {
        JSHandle<EcmaString> localeStr = factory->NewFromStdString(locale);
        locales->Set(thread, index++, localeStr);
    }
    return locales;
}

std::string JSLocale::GetNumberingSystem(const icu::Locale &icuLocale)
{
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::NumberingSystem> numberingSystem(icu::NumberingSystem::createInstance(icuLocale, status));
    if (U_SUCCESS(status) != 0) {
        return numberingSystem->getName();
    }
    return JSLocale::LATN_STRING;
}

bool JSLocale::IsWellFormedCurrencyCode(const std::string &currency)
{
    if (currency.length() != INTL_INDEX_THREE) {
        return false;
    }
    return (IsAToZ(currency[INTL_INDEX_ZERO]) && IsAToZ(currency[INTL_INDEX_ONE]) && IsAToZ(currency[INTL_INDEX_TWO]));
}

bool JSLocale::IsWellFormedCalendarCode(const std::string& calendar)
{
    std::string value = calendar;
    while (true) {
        std::size_t found_dash = value.find('-');
        if (found_dash == std::string::npos) {
            return IsAlphanum(value, INTL_INDEX_THREE, INTL_INDEX_EIGHT);
        }
        if (!IsAlphanum(value.substr(0, found_dash), INTL_INDEX_THREE, INTL_INDEX_EIGHT)) {
            return false;
        }
        value = value.substr(found_dash + 1);
    }
}

JSHandle<JSObject> JSLocale::PutElement(JSThread *thread, int index, const JSHandle<JSArray> &array,
                                        const JSHandle<JSTaggedValue> &fieldTypeString,
                                        const JSHandle<JSTaggedValue> &value)
{
    auto ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    // Let record be ! ObjectCreate(%ObjectPrototype%).
    JSHandle<JSObject> record = factory->NewEmptyJSObject();

    auto globalConst = thread->GlobalConstants();
    // obj.type = field_type_string
    JSObject::CreateDataPropertyOrThrow(thread, record, globalConst->GetHandledTypeString(), fieldTypeString);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    // obj.value = value
    JSObject::CreateDataPropertyOrThrow(thread, record, globalConst->GetHandledValueString(), value);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);

    JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(array), index,
                               JSHandle<JSTaggedValue>::Cast(record), true);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    return record;
}

// 9.2.11 GetOption ( options, property, type, values, fallback )
bool JSLocale::GetOptionOfBool(JSThread *thread, const JSHandle<JSObject> &options,
                               const JSHandle<JSTaggedValue> &property, bool fallback, bool *res)
{
    // 1. Let value be ? Get(options, property).
    OperationResult operationResult = JSObject::GetProperty(thread, options, property);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    JSHandle<JSTaggedValue> value = operationResult.GetValue();
    *res = fallback;
    // 2. If value is not undefined, then
    if (!value->IsUndefined()) {
        // b. Let value be ToBoolean(value).
        *res = value->ToBoolean();
        return true;
    }
    // 3. not found
    return false;
}

JSHandle<JSTaggedValue> JSLocale::GetNumberFieldType(JSThread *thread, JSTaggedValue x, int32_t fieldId)
{
    ASSERT(x.IsNumber() || x.IsBigInt());
    double number = 0;
    auto globalConst = thread->GlobalConstants();
    if (static_cast<UNumberFormatFields>(fieldId) == UNUM_INTEGER_FIELD) {
        number = x.IsBigInt() ? number : x.GetNumber();
        if (x.IsBigInt() || std::isfinite(number)) {
            return globalConst->GetHandledIntegerString();
        }
        if (std::isnan(number)) {
            return globalConst->GetHandledNanString();
        }
        return globalConst->GetHandledInfinityString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_FRACTION_FIELD) {
        return globalConst->GetHandledFractionString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_DECIMAL_SEPARATOR_FIELD) {
        return globalConst->GetHandledDecimalString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_GROUPING_SEPARATOR_FIELD) {
        return globalConst->GetHandledGroupString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_CURRENCY_FIELD) {
        return globalConst->GetHandledCurrencyString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_PERCENT_FIELD) {
        return globalConst->GetHandledPercentSignString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_SIGN_FIELD) {
        if (x.IsBigInt()) {
            JSHandle<JSTaggedValue> bigint(thread, x);
            JSHandle<BigInt> value(thread, JSTaggedValue::ToBigInt(thread, bigint));
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
            return value->GetSign() ? globalConst->GetHandledMinusSignString()
                                    : globalConst->GetHandledPlusSignString();
        }
        number = x.GetNumber();
        return std::signbit(number) ? globalConst->GetHandledMinusSignString()
                                    : globalConst->GetHandledPlusSignString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_EXPONENT_SYMBOL_FIELD) {
        return globalConst->GetHandledExponentSeparatorString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_EXPONENT_SIGN_FIELD) {
        return globalConst->GetHandledExponentMinusSignString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_EXPONENT_FIELD) {
        return globalConst->GetHandledExponentIntegerString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_COMPACT_FIELD) {
        return globalConst->GetHandledCompactString();
    } else if (static_cast<UNumberFormatFields>(fieldId) == UNUM_MEASURE_UNIT_FIELD) {
        return globalConst->GetHandledUnitString();
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

// 10.1.1 ApplyOptionsToTag( tag, options )
bool JSLocale::ApplyOptionsToTag(JSThread *thread, const JSHandle<EcmaString> &tag, const JSHandle<JSObject> &options,
                                 TagElements &tagElements)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    if (*tag == *(factory->GetEmptyString())) {
        return false;
    }
    // 2. If intl::LocaleHelper::IsStructurallyValidLanguageTag(tag) is false, throw a RangeError exception.
    if (!intl::LocaleHelper::IsStructurallyValidLanguageTag(tag)) {
        return false;
    }

    tagElements.language =
        GetOption(thread, options, globalConst->GetHandledLanguageString(), OptionType::STRING,
                  globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 4. If language is not undefined, then
    //    a. If language does not match the unicode_language_subtag production, throw a RangeError exception.
    if (!tagElements.language->IsUndefined()) {
        std::string languageStr =
            intl::LocaleHelper::ConvertToStdString(JSHandle<EcmaString>::Cast(tagElements.language));
        if (languageStr[INTL_INDEX_ZERO] == '\0' ||
            IsAlpha(languageStr, INTL_INDEX_FOUR, INTL_INDEX_FOUR)) {
            return false;
        }
    }

    // 5. Let script be ? GetOption(options, "script", "string", undefined, undefined).
    tagElements.script =
        GetOption(thread, options, globalConst->GetHandledScriptString(), OptionType::STRING,
                  globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 6. If script is not undefined, then
    //    a. If script does not match the unicode_script_subtag production, throw a RangeError exception.
    if (!tagElements.script->IsUndefined()) {
        std::string scriptStr =
            intl::LocaleHelper::ConvertToStdString((JSHandle<EcmaString>::Cast(tagElements.script)));
        if (scriptStr[INTL_INDEX_ZERO] == '\0') {
            return false;
        }
    }

    // 7. Let region be ? GetOption(options, "region", "string", undefined, undefined).
    // 8. If region is not undefined, then
    //    a. If region does not match the unicode_region_subtag production, throw a RangeError exception.
    tagElements.region =
        GetOption(thread, options, globalConst->GetHandledRegionString(), OptionType::STRING,
                  globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    if (!tagElements.region->IsUndefined()) {
        std::string regionStr = intl::LocaleHelper::ConvertToStdString(JSHandle<EcmaString>::Cast(tagElements.region));
        if (regionStr[INTL_INDEX_ZERO] == '\0') {
            return false;
        }
    }
    return true;
}

bool BuildOptionsTags(const JSHandle<EcmaString> &tag, icu::LocaleBuilder *builder, JSHandle<JSTaggedValue> language,
                      JSHandle<JSTaggedValue> script, JSHandle<JSTaggedValue> region)
{
    std::string tagStr = intl::LocaleHelper::ConvertToStdString(tag);
    int32_t len = static_cast<int32_t>(tagStr.length());
    ASSERT(len > 0);
    builder->setLanguageTag({ tagStr.c_str(), len });
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale locale = builder->build(status);
    locale.canonicalize(status);
    if (U_FAILURE(status) != 0) {
        return false;
    }
    builder->setLocale(locale);

    if (!language->IsUndefined()) {
        std::string languageStr = intl::LocaleHelper::ConvertToStdString(JSHandle<EcmaString>::Cast(language));
        builder->setLanguage(languageStr);
        builder->build(status);
        if ((U_FAILURE(status) != 0)) {
            return false;
        }
    }

    if (!script->IsUndefined()) {
        std::string scriptStr = intl::LocaleHelper::ConvertToStdString((JSHandle<EcmaString>::Cast(script)));
        builder->setScript(scriptStr);
        builder->build(status);
        if ((U_FAILURE(status) != 0)) {
            return false;
        }
    }

    if (!region->IsUndefined()) {
        std::string regionStr = intl::LocaleHelper::ConvertToStdString(JSHandle<EcmaString>::Cast(region));
        builder->setRegion(regionStr);
        builder->build(status);
        if ((U_FAILURE(status) != 0)) {
            return false;
        }
    }
    return true;
}

bool InsertOptions(JSThread *thread, const JSHandle<JSObject> &options, icu::LocaleBuilder *builder)
{
    const std::vector<std::string> emptyValues = {};
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    std::string strResult;
    bool findca =
        JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledCalendarString(), emptyValues, &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findca) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("ca"), strResult.c_str())) {
            return false;
        }
        ThreadNativeScope nativeScope(thread);
        builder->setUnicodeLocaleKeyword("ca", strResult.c_str());
    }

    bool findco =
        JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledCollationString(), emptyValues, &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findco) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("co"), strResult.c_str())) {
            return false;
        }
        ThreadNativeScope nativeScope(thread);
        builder->setUnicodeLocaleKeyword("co", strResult.c_str());
    }

    bool findhc = JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledHourCycleString(),
                                              JSLocale::HOUR_CYCLE, &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findhc) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("hc"), strResult.c_str())) {
            return false;
        }
        ThreadNativeScope nativeScope(thread);
        builder->setUnicodeLocaleKeyword("hc", strResult.c_str());
    }

    bool findkf = JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledCaseFirstString(),
                                              JSLocale::CASE_FIRST, &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findkf) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("kf"), strResult.c_str())) {
            return false;
        }
        ThreadNativeScope nativeScope(thread);
        builder->setUnicodeLocaleKeyword("kf", strResult.c_str());
    }

    bool boolResult = false;
    bool findkn =
        JSLocale::GetOptionOfBool(thread, options, globalConst->GetHandledNumericString(), false, &boolResult);
    if (findkn) {
        strResult = boolResult ? "true" : "false";
        if (!uloc_toLegacyType(uloc_toLegacyKey("kn"), strResult.c_str())) {
            return false;
        }
        ThreadNativeScope nativeScope(thread);
        builder->setUnicodeLocaleKeyword("kn", strResult.c_str());
    }

    bool findnu =
        JSLocale::GetOptionOfString(thread, options, globalConst->GetHandledNumberingSystemString(), emptyValues,
                                    &strResult);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (findnu) {
        if (!uloc_toLegacyType(uloc_toLegacyKey("nu"), strResult.c_str())) {
            return false;
        }
        ThreadNativeScope nativeScope(thread);
        builder->setUnicodeLocaleKeyword("nu", strResult.c_str());
    }
    return true;
}

JSHandle<JSLocale> JSLocale::InitializeLocale(JSThread *thread, const JSHandle<JSLocale> &locale,
                                              const JSHandle<EcmaString> &localeString,
                                              const JSHandle<JSObject> &options)
{
    icu::LocaleBuilder builder;
    TagElements tagElements;
    if (!ApplyOptionsToTag(thread, localeString, options, tagElements)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "apply option to tag failed", locale);
    }

    bool res = BuildOptionsTags(localeString, &builder, tagElements.language, tagElements.script, tagElements.region);
    if (!res) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "apply option to tag failed", locale);
    }
    bool insertResult = InsertOptions(thread, options, &builder);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, locale);
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale icuLocale = builder.build(status);
    icuLocale.canonicalize(status);

    if (!insertResult || (U_FAILURE(status) != 0)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "insert or build failed", locale);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    factory->NewJSIntlIcuData(locale, icuLocale, JSLocale::FreeIcuLocale);
    return locale;
}

int ConvertValue(const UErrorCode &status, std::string &value, const std::string &key)
{
    if (status == U_ILLEGAL_ARGUMENT_ERROR || value.empty()) {
        return 1;
    }

    if (value == "yes") {
        value = "true";
    }

    if (key == "kf" && value == "true") {
        return 2;  // 2: in this case normalizedKeyword is empty string
    }
    return 0;
}

JSTaggedValue JSLocale::NormalizeKeywordValue(JSThread *thread, const JSHandle<JSLocale> &locale,
                                              const std::string &key)
{
    icu::Locale *icuLocale = locale->GetIcuLocale();
    UErrorCode status = U_ZERO_ERROR;
    auto value = icuLocale->getUnicodeKeywordValue<std::string>(key, status);

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    int result = ConvertValue(status, value, key);
    if (result == 1) {
        return JSTaggedValue::Undefined();
    }
    if (result == 2) {  // 2: in this case normalizedKeyword is empty string
        return factory->GetEmptyString().GetTaggedValue();
    }
    return factory->NewFromStdString(value).GetTaggedValue();
}

JSHandle<EcmaString> JSLocale::ToString(JSThread *thread, const JSHandle<JSLocale> &locale)
{
    icu::Locale *icuLocale = locale->GetIcuLocale();
    if (icuLocale == nullptr) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        return factory->GetEmptyString();
    }
    JSHandle<EcmaString> result = intl::LocaleHelper::ToLanguageTag(thread, *icuLocale);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
    return result;
}

std::vector<std::string> JSLocale::GetAvailableStringLocales(JSThread *thread,
                                                             const JSHandle<TaggedArray> &availableLocales)
{
    std::vector<std::string> availableStringLocales;
    JSMutableHandle<EcmaString> availableItem(thread, JSTaggedValue::Undefined());
    uint32_t availablecalesLength = availableLocales->GetLength();
    for (uint32_t i = 0; i < availablecalesLength; i++) {
        availableItem.Update(availableLocales->Get(thread, i));
        availableStringLocales.emplace_back(intl::LocaleHelper::ConvertToStdString(availableItem));
    }
    return availableStringLocales;
}

}  // namespace panda::ecmascript
