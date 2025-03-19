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

#include "ecmascript/js_collator.h"

#include "ecmascript/ecma_context.h"
#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ecma_string-inl.h"
namespace panda::ecmascript {
// NOLINTNEXTLINE (readability-identifier-naming, fuchsia-statically-constructed-objects)
const CString JSCollator::uIcuDataColl = U_ICUDATA_NAME U_TREE_SEPARATOR_STRING "coll";
const std::map<std::string, CaseFirstOption> JSCollator::caseFirstMap = {
    {"upper", CaseFirstOption::UPPER},
    {"lower", CaseFirstOption::LOWER},
    {"false", CaseFirstOption::FALSE_OPTION}
};
const std::map<CaseFirstOption, UColAttributeValue> JSCollator::uColAttributeValueMap = {
    {CaseFirstOption::UPPER, UCOL_UPPER_FIRST},
    {CaseFirstOption::LOWER, UCOL_LOWER_FIRST},
    {CaseFirstOption::FALSE_OPTION, UCOL_OFF},
    {CaseFirstOption::UNDEFINED, UCOL_OFF}
};
const std::vector<LocaleMatcherOption> JSCollator::LOCALE_MATCHER_OPTION = {
    LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT
};
const std::vector<std::string>  JSCollator::LOCALE_MATCHER_OPTION_NAME = {"lookup", "best fit"};

const std::vector<CaseFirstOption>  JSCollator::CASE_FIRST_OPTION = {
    CaseFirstOption::UPPER, CaseFirstOption::LOWER, CaseFirstOption::FALSE_OPTION
};
const std::vector<std::string>  JSCollator::CASE_FIRST_OPTION_NAME = {"upper", "lower", "false"};

const std::set<std::string>  JSCollator::RELEVANT_EXTENSION_KEYS = {"co", "kn", "kf"};

const std::vector<SensitivityOption>  JSCollator::SENSITIVITY_OPTION = {
    SensitivityOption::BASE, SensitivityOption::ACCENT,
    SensitivityOption::CASE, SensitivityOption::VARIANT
};
const std::vector<std::string>  JSCollator::SENSITIVITY_OPTION_NAME = {"base", "accent", "case", "variant"};

const std::vector<UsageOption> JSCollator::USAGE_OPTION = {UsageOption::SORT, UsageOption::SEARCH};
const std::vector<std::string> JSCollator::USAGE_OPTION_NAME = {"sort", "search"};

// All the available locales that are statically known to fulfill fast path conditions.
const char* const  JSCollator::FAST_LOCALE[] = {
    "en-US", "en", "fr", "es", "de", "pt", "it", "ca",
    "de-AT", "fi", "id", "id-ID", "ms", "nl", "pl", "ro",
    "sl", "sv", "sw", "vi", "en-DE", "en-GB",
};


JSHandle<TaggedArray> JSCollator::GetAvailableLocales(JSThread *thread, bool enableLocaleCache)
{
    const char *key = nullptr;
    const char *path = JSCollator::uIcuDataColl.c_str();
    // key and path are const, so we can cache the result
    if (enableLocaleCache) {
        JSHandle<JSTaggedValue> cachedLocales = thread->GlobalConstants()->GetHandledCachedJSCollatorLocales();
        if (cachedLocales->IsHeapObject()) {
            return JSHandle<TaggedArray>(cachedLocales);
        }
    }
    std::vector<std::string> availableStringLocales = intl::LocaleHelper::GetAvailableLocales(thread, key, path);
    JSHandle<TaggedArray> availableLocales = JSLocale::ConstructLocaleList(thread, availableStringLocales);
    if (enableLocaleCache) {
        GlobalEnvConstants *constants = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
        constants->SetCachedLocales(availableLocales.GetTaggedValue());
    }
    return availableLocales;
}

/* static */
void JSCollator::SetIcuCollator(JSThread *thread, const JSHandle<JSCollator> &collator,
    icu::Collator *icuCollator, const NativePointerCallback &callback)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();

    ASSERT(icuCollator != nullptr);
    JSTaggedValue data = collator->GetIcuField();
    if (data.IsJSNativePointer()) {
        JSNativePointer *native = JSNativePointer::Cast(data.GetTaggedObject());
        native->ResetExternalPointer(thread, icuCollator);
        return;
    }
    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(icuCollator, callback);
    collator->SetIcuField(thread, pointer.GetTaggedValue());
}

JSHandle<JSCollator> JSCollator::InitializeCollator(JSThread *thread,
                                                    const JSHandle<JSCollator> &collator,
                                                    const JSHandle<JSTaggedValue> &locales,
                                                    const JSHandle<JSTaggedValue> &options,
                                                    bool forIcuCache,
                                                    bool enableLocaleCache)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 1. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = intl::LocaleHelper::CanonicalizeLocaleList(thread, locales);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);

    // 2. If options is undefined, then
    //      a. Let options be ObjectCreate(null).
    // 3. Else,
    //      a. Let options be ? ToObject(options).
    JSHandle<JSObject> optionsObject;
    if (options->IsUndefined()) {
        optionsObject = factory->CreateNullJSObject();
    } else {
        optionsObject = JSTaggedValue::ToObject(thread, options);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);
    }
    // 4. Let usage be ? GetOption(options, "usage", "string", « "sort", "search" », "sort").
    auto usage = JSLocale::GetOptionOfString<UsageOption>(thread, optionsObject, globalConst->GetHandledUsageString(),
                                                          JSCollator::USAGE_OPTION, JSCollator::USAGE_OPTION_NAME,
                                                          UsageOption::SORT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);
    collator->SetUsage(usage);

    // 5. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    auto matcher = JSLocale::GetOptionOfString<LocaleMatcherOption>(
        thread, optionsObject, globalConst->GetHandledLocaleMatcherString(),
        JSCollator::LOCALE_MATCHER_OPTION, JSCollator::LOCALE_MATCHER_OPTION_NAME,
        LocaleMatcherOption::BEST_FIT);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);

    // 6. Let collation be ? GetOption(options, "collation", "string", undefined, undefined).
    // 7. If collation is not undefined, then
    //    a. If collation does not match the Unicode Locale Identifier type nonterminal, throw a RangeError exception.
    JSHandle<JSTaggedValue> collation =
        JSLocale::GetOption(thread, optionsObject, globalConst->GetHandledCollationString(), OptionType::STRING,
                            globalConst->GetHandledUndefined(), globalConst->GetHandledUndefined());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);
    collator->SetCollation(thread, collation);
    std::string collationStr;
    if (!collation->IsUndefined()) {
        JSHandle<EcmaString> collationEcmaStr = JSHandle<EcmaString>::Cast(collation);
        collationStr = intl::LocaleHelper::ConvertToStdString(collationEcmaStr);
        if (!JSLocale::IsWellAlphaNumList(collationStr)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid collation", collator);
        }
    }

    // 8. Let numeric be ? GetOption(options, "numeric", "boolean", undefined, undefined).
    bool numeric = false;
    bool foundNumeric =
        JSLocale::GetOptionOfBool(thread, optionsObject, globalConst->GetHandledNumericString(), false, &numeric);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);
    collator->SetNumeric(numeric);

    // 14. Let caseFirst be ? GetOption(options, "caseFirst", "string", « "upper", "lower", "false" », undefined).
    CaseFirstOption caseFirst = JSLocale::GetOptionOfString<CaseFirstOption>(
        thread, optionsObject, globalConst->GetHandledCaseFirstString(),
        JSCollator::CASE_FIRST_OPTION, JSCollator::CASE_FIRST_OPTION_NAME,
        CaseFirstOption::UNDEFINED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);
    collator->SetCaseFirst(caseFirst);

    // 16. Let relevantExtensionKeys be %Collator%.[[RelevantExtensionKeys]].

    // 17. Let r be ResolveLocale(%Collator%.[[AvailableLocales]], requestedLocales, opt,
    //     %Collator%.[[RelevantExtensionKeys]], localeData).
    JSHandle<TaggedArray> availableLocales;
    if (requestedLocales->GetLength() == 0) {
        availableLocales = factory->EmptyArray();
    } else {
        availableLocales = GetAvailableLocales(thread, enableLocaleCache);
    }
    ResolvedLocale r =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, RELEVANT_EXTENSION_KEYS);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);
    icu::Locale icuLocale = r.localeData;
    JSHandle<EcmaString> localeStr = intl::LocaleHelper::ToLanguageTag(thread, icuLocale);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);
    collator->SetLocale(thread, localeStr.GetTaggedValue());
    ASSERT_PRINT(!icuLocale.isBogus(), "icuLocale is bogus");

    // If collation is undefined iterate RelevantExtensionKeys to find "co"
    //  if found, set ICU collator UnicodeKeyword to iterator->second
    UErrorCode status = U_ZERO_ERROR;
    if (!collation->IsUndefined()) {
        auto extensionIter = r.extensions.find("co");
        if (extensionIter != r.extensions.end() && extensionIter->second != collationStr) {
            icuLocale.setUnicodeKeywordValue("co", nullptr, status);
            ASSERT_PRINT(U_SUCCESS(status), "icuLocale set co failed");
        }
    }

    // If usage is serach set co-serach to icu locale key word value
    // Eles set collation string to icu locale key word value
    if (usage == UsageOption::SEARCH) {
        icuLocale.setUnicodeKeywordValue("co", "search", status);
        ASSERT(U_SUCCESS(status));
    } else {
        if (!collationStr.empty() && JSLocale::IsWellCollation(icuLocale, collationStr)) {
            icuLocale.setUnicodeKeywordValue("co", collationStr, status);
            ASSERT(U_SUCCESS(status));
        }
    }

    std::unique_ptr<icu::Collator> icuCollator(icu::Collator::createInstance(icuLocale, status));
    if (U_FAILURE(status) || icuCollator == nullptr) {  // NOLINT(readability-implicit-bool-conversion)
        if (status == UErrorCode::U_MISSING_RESOURCE_ERROR) {
            THROW_REFERENCE_ERROR_AND_RETURN(thread, "can not find icu data resources", collator);
        }
        status = U_ZERO_ERROR;
        icu::Locale localeName(icuLocale.getBaseName());
        icuCollator.reset(icu::Collator::createInstance(localeName, status));
        if (U_FAILURE(status) || icuCollator == nullptr) {  // NOLINT(readability-implicit-bool-conversion)
            THROW_RANGE_ERROR_AND_RETURN(thread, "invalid collation", collator);
        }
    }
    ASSERT(U_SUCCESS(status));
    icu::Locale collatorLocale(icuCollator->getLocale(ULOC_VALID_LOCALE, status));

    icuCollator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    ASSERT(U_SUCCESS(status));

    // If numeric is found set ICU collator UCOL_NUMERIC_COLLATION to numeric
    // Else iterate RelevantExtensionKeys to find "kn"
    //  if found, set ICU collator UCOL_NUMERIC_COLLATION to iterator->second
    status = U_ZERO_ERROR;
    if (foundNumeric) {
        ASSERT(icuCollator.get() != nullptr);
        icuCollator.get()->setAttribute(UCOL_NUMERIC_COLLATION, numeric ? UCOL_ON : UCOL_OFF, status);
        ASSERT(U_SUCCESS(status));
    } else {
        auto extensionIter = r.extensions.find("kn");
        if (extensionIter != r.extensions.end()) {
            ASSERT(icuCollator.get() != nullptr);
            bool found = (extensionIter->second == "true");
            collator->SetNumeric(found);
            icuCollator.get()->setAttribute(UCOL_NUMERIC_COLLATION, found ? UCOL_ON : UCOL_OFF, status);
            ASSERT(U_SUCCESS(status));
        }
    }

    // If caseFirst is not undefined set ICU collator UColAttributeValue to caseFirst
    // Else iterate RelevantExtensionKeys to find "kf"
    //  if found, set ICU collator UColAttributeValue to iterator->second
    status = U_ZERO_ERROR;
    if (caseFirst != CaseFirstOption::UNDEFINED) {
        ASSERT(icuCollator.get() != nullptr);
        icuCollator.get()->setAttribute(UCOL_CASE_FIRST, OptionToUColAttribute(caseFirst), status);
        ASSERT(U_SUCCESS(status));
    } else {
        auto extensionIter = r.extensions.find("kf");
        if (extensionIter != r.extensions.end()) {
            ASSERT(icuCollator.get() != nullptr);
            auto mapIter = caseFirstMap.find(extensionIter->second);
            if (mapIter != caseFirstMap.end()) {
                icuCollator.get()->setAttribute(UCOL_CASE_FIRST, OptionToUColAttribute(mapIter->second), status);
                collator->SetCaseFirst(mapIter->second);
            } else {
                icuCollator.get()->setAttribute(UCOL_CASE_FIRST, OptionToUColAttribute(CaseFirstOption::UNDEFINED),
                                                status);
            }
            ASSERT(U_SUCCESS(status));
        }
    }

    // 24. Let sensitivity be ? GetOption(options, "sensitivity", "string", « "base", "accent", "case", "variant" »,
    //     undefined).
    SensitivityOption sensitivity = JSLocale::GetOptionOfString<SensitivityOption>(
        thread, optionsObject, globalConst->GetHandledSensitivityString(),
        JSCollator::SENSITIVITY_OPTION, JSCollator::SENSITIVITY_OPTION_NAME,
        SensitivityOption::UNDEFINED);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSCollator, thread);
    // 25. If sensitivity is undefined, then
    //     a. If usage is "sort", then
    //        i. Let sensitivity be "variant".
    if (sensitivity == SensitivityOption::UNDEFINED) {
        if (usage == UsageOption::SORT) {
            sensitivity = SensitivityOption::VARIANT;
        }
    }
    collator->SetSensitivity(sensitivity);

    // Trans SensitivityOption to Icu strength option
    switch (sensitivity) {
        case SensitivityOption::BASE:
            icuCollator->setStrength(icu::Collator::PRIMARY);
            break;
        case SensitivityOption::ACCENT:
            icuCollator->setStrength(icu::Collator::SECONDARY);
            break;
        case SensitivityOption::CASE:
            icuCollator->setStrength(icu::Collator::PRIMARY);
            icuCollator->setAttribute(UCOL_CASE_LEVEL, UCOL_ON, status);
            break;
        case SensitivityOption::VARIANT:
            icuCollator->setStrength(icu::Collator::TERTIARY);
            break;
        case SensitivityOption::UNDEFINED:
            break;
        case SensitivityOption::EXCEPTION:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }

    // 27. Let ignorePunctuation be ? GetOption(options, "ignorePunctuation", "boolean", undefined, false).
    // 28. Set collator.[[IgnorePunctuation]] to ignorePunctuation.
    bool ignorePunctuation = false;
    bool defaultIgnorePunctuation = false;
    // If the ignorePunctuation is not defined, which in "th" locale that is true but false on other locales.
    JSHandle<EcmaString> thKey = factory->NewFromUtf8("th");
    if (JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>::Cast(thKey), locales)) {
        defaultIgnorePunctuation = true;
    }
    JSLocale::GetOptionOfBool(thread, optionsObject, globalConst->GetHandledIgnorePunctuationString(),
                              defaultIgnorePunctuation, &ignorePunctuation);
    collator->SetIgnorePunctuation(ignorePunctuation);
    if (ignorePunctuation) {
        status = U_ZERO_ERROR;
        icuCollator->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, status);
        ASSERT(U_SUCCESS(status));
    }

    if (forIcuCache) {
        std::string cacheEntry =
            locales->IsUndefined() ? "" : EcmaStringAccessor(locales.GetTaggedValue()).ToStdString();
        ecmaVm->GetIntlCache().SetIcuFormatterToCache(IcuFormatterType::COLLATOR,
            cacheEntry, icuCollator.release(), JSCollator::FreeIcuCollator);
    } else {
        SetIcuCollator(thread, collator, icuCollator.release(), JSCollator::FreeIcuCollator);
    }
    collator->SetBoundCompare(thread, JSTaggedValue::Undefined());
    // 29. Return collator.
    return collator;
}

icu::Collator *JSCollator::GetCachedIcuCollator(JSThread *thread, const JSTaggedValue &locales)
{
    std::string cacheEntry = locales.IsUndefined() ? "" : EcmaStringAccessor(locales).ToStdString();
    void *cachedCollator =
        thread->GetEcmaVM()->GetIntlCache().GetIcuFormatterFromCache(IcuFormatterType::COLLATOR, cacheEntry);
    if (cachedCollator != nullptr) {
        return reinterpret_cast<icu::Collator*>(cachedCollator);
    }
    return nullptr;
}

icu::Collator *JSCollator::GetCachedIcuCollator(JSThread *thread, const JSHandle<JSTaggedValue> &locales)
{
    return GetCachedIcuCollator(thread, locales.GetTaggedValue());
}

UColAttributeValue JSCollator::OptionToUColAttribute(CaseFirstOption caseFirstOption)
{
    auto iter = uColAttributeValueMap.find(caseFirstOption);
    if (iter != uColAttributeValueMap.end()) {
        return iter->second;
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

JSHandle<JSTaggedValue> OptionsToEcmaString(JSThread *thread, UsageOption usage)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (usage) {
        case UsageOption::SORT:
            result.Update(globalConst->GetSortString());
            break;
        case UsageOption::SEARCH:
            result.Update(globalConst->GetSearchString());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> OptionsToEcmaString(JSThread *thread, SensitivityOption sensitivity)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (sensitivity) {
        case SensitivityOption::BASE:
            result.Update(globalConst->GetBaseString());
            break;
        case SensitivityOption::ACCENT:
            result.Update(globalConst->GetAccentString());
            break;
        case SensitivityOption::CASE:
            result.Update(globalConst->GetCaseString());
            break;
        case SensitivityOption::VARIANT:
            result.Update(globalConst->GetVariantString());
            break;
        case SensitivityOption::UNDEFINED:
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> OptionsToEcmaString(JSThread *thread, CaseFirstOption caseFirst)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (caseFirst) {
        case CaseFirstOption::UPPER:
            result.Update(globalConst->GetUpperString());
            break;
        case CaseFirstOption::LOWER:
            result.Update(globalConst->GetLowerString());
            break;
        case CaseFirstOption::FALSE_OPTION:
            result.Update(globalConst->GetFalseString());
            break;
        case CaseFirstOption::UNDEFINED:
            result.Update(globalConst->GetUpperString());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

// 11.3.4 Intl.Collator.prototype.resolvedOptions ()
JSHandle<JSObject> JSCollator::ResolvedOptions(JSThread *thread, const JSHandle<JSCollator> &collator)
{
    auto ecmaVm = thread->GetEcmaVM();
    auto globalConst = thread->GlobalConstants();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSFunction> funCtor(env->GetObjectFunction());
    JSHandle<JSObject> options(factory->NewJSObjectByConstructor(funCtor));

    // [[Locale]]
    JSHandle<JSTaggedValue> property = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> locale(thread, collator->GetLocale());
    JSObject::CreateDataPropertyOrThrow(thread, options, property, locale);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);

    // [[Usage]]
    UsageOption usageOption = collator->GetUsage();
    JSHandle<JSTaggedValue> usageValue = OptionsToEcmaString(thread, usageOption);
    JSObject::CreateDataProperty(thread, options, globalConst->GetHandledUsageString(), usageValue);

    // [[Sensitivity]]
    auto sentivityOption = collator->GetSensitivity();
    JSHandle<JSTaggedValue> sensitivityValue = OptionsToEcmaString(thread, sentivityOption);
    JSObject::CreateDataProperty(thread, options, globalConst->GetHandledSensitivityString(), sensitivityValue);

    // [[IgnorePunctuation]]
    JSHandle<JSTaggedValue> ignorePunctuationValue(thread, JSTaggedValue(collator->GetIgnorePunctuation()));
    JSObject::CreateDataProperty(thread, options, globalConst->GetHandledIgnorePunctuationString(),
                                 ignorePunctuationValue);

    // [[Collation]]
    JSMutableHandle<JSTaggedValue> collationValue(thread, collator->GetCollation());
    UErrorCode status = U_ZERO_ERROR;
    icu::Collator *icuCollator = collator->GetIcuCollator();
    icu::Locale icu_locale(icuCollator->getLocale(ULOC_VALID_LOCALE, status));
    std::string collation_value =
        icu_locale.getUnicodeKeywordValue<std::string>("co", status);
    if (collationValue->IsUndefined()) {
        if (collation_value != "search" && collation_value != "") {
            collationValue.Update(factory->NewFromStdString(collation_value).GetTaggedValue());
        } else {
            collationValue.Update(globalConst->GetDefaultString());
        }
    }
    JSObject::CreateDataProperty(thread, options, globalConst->GetHandledCollationString(), collationValue);

    // [[Numeric]]
    JSHandle<JSTaggedValue> numericValue(thread, JSTaggedValue(collator->GetNumeric()));
    JSObject::CreateDataProperty(thread, options, globalConst->GetHandledNumericString(), numericValue);

    // [[CaseFirst]]
    CaseFirstOption caseFirstOption = collator->GetCaseFirst();
    // In Ecma402 spec, caseFirst is an optional property so we set it to Upper when input is undefined
    // the requirement maybe change in the future
    JSHandle<JSTaggedValue> caseFirstValue = OptionsToEcmaString(thread, caseFirstOption);
    JSObject::CreateDataProperty(thread, options, globalConst->GetHandledCaseFirstString(), caseFirstValue);
    return options;
}

CompareStringsOption JSCollator::CompareStringsOptionFor(JSThread* thread,
                                                         JSHandle<JSTaggedValue> locales)
{
    if (locales->IsUndefined()) {
        auto& intlCache = thread->GetEcmaVM()->GetIntlCache();
        auto defaultCompareOption = intlCache.GetDefaultCompareStringsOption();
        if (defaultCompareOption.has_value()) {
            return defaultCompareOption.value();
        }
        auto defaultLocale = intl::LocaleHelper::StdStringDefaultLocale(thread);
        for (const char *fastLocale : FAST_LOCALE) {
            if (strcmp(fastLocale, defaultLocale.c_str()) == 0) {
                intlCache.SetDefaultCompareStringsOption(CompareStringsOption::TRY_FAST_PATH);
                return CompareStringsOption::TRY_FAST_PATH;
            }
        }
        intlCache.SetDefaultCompareStringsOption(CompareStringsOption::NONE);
        return CompareStringsOption::NONE;
    }

    if (!locales->IsString()) {
        return CompareStringsOption::NONE;
    }

    JSHandle<EcmaString> localesString = JSHandle<EcmaString>::Cast(locales);
    CString localesStr = ConvertToString(*localesString, StringConvertedUsage::LOGICOPERATION);
    for (const char *fastLocale : FAST_LOCALE) {
        if (strcmp(fastLocale, localesStr.c_str()) == 0) {
            return CompareStringsOption::TRY_FAST_PATH;
        }
    }

    return CompareStringsOption::NONE;
}

CompareStringsOption JSCollator::CompareStringsOptionFor(JSThread* thread,
                                                         JSHandle<JSTaggedValue> locales,
                                                         JSHandle<JSTaggedValue> options)
{
    if (!options->IsUndefined()) {
        return CompareStringsOption::NONE;
    }
    return CompareStringsOptionFor(thread, locales);
}

// Anonymous namespace for ComapreStrings
namespace {
constexpr uint8_t COLLATION_WEIGHT_L1[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  6,  12, 16, 28, 38, 29, 27, 15,
    17, 18, 24, 32, 9,  8,  14, 25, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 11, 10,
    33, 34, 35, 13, 23, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 19, 26, 20, 31, 7,  30, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
    72, 73, 74, 21, 36, 22, 37, 0,
};
constexpr uint8_t COLLATION_WEIGHT_L3[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  0,
};
constexpr int COLLATION_WEIGHT_LENGTH = sizeof(COLLATION_WEIGHT_L1) / sizeof(COLLATION_WEIGHT_L1[0]);

constexpr UCollationResult ToUCollationResult(int delta)
{
    return delta < 0 ? UCollationResult::UCOL_LESS
                     : (delta > 0 ? UCollationResult::UCOL_GREATER
                                  : UCollationResult::UCOL_EQUAL);
}

struct FastCompareStringsData {
    UCollationResult l1Result = UCollationResult::UCOL_EQUAL;
    UCollationResult l3Result = UCollationResult::UCOL_EQUAL;
    int processedUntil = 0;
    int firstDiffAt = 0;  // The first relevant diff (L1 if exists, else L3).
    bool hasDiff = false;

    std::optional<UCollationResult> FastCompareFailed(int& processedUntilOut) const
    {
        if (hasDiff) {
            // Found some difference, continue there to ensure the generic algorithm picks it up.
            processedUntilOut = firstDiffAt;
        } else {
            // No difference found, reprocess the last processed character since it may be
            // followed by a unicode combining character.
            processedUntilOut = std::max(processedUntil - 1, 0);
        }
        return {};
    }
};

template <class T>
constexpr bool CanFastCompare(T ch)
{
    return ch < COLLATION_WEIGHT_LENGTH && COLLATION_WEIGHT_L1[ch] != 0;
}

// Check canFastCompare, L1 weight, and L3 weight together.
// Use FastCompareStringsData to store these results.
template <class T1, class T2>
bool FastCompareFlatString(const T1* lhs, const T2* rhs, int length, FastCompareStringsData& fastCompareData)
{
    for (int i = 0; i < length; i++) {
        const T1 l = lhs[i];
        const T2 r = rhs[i];
        if (!CanFastCompare(l) || !CanFastCompare(r)) {
            fastCompareData.processedUntil = i;
            return false;
        }
        auto l1Result = ToUCollationResult(COLLATION_WEIGHT_L1[l] - COLLATION_WEIGHT_L1[r]);
        if (l1Result != UCollationResult::UCOL_EQUAL) {
            fastCompareData.hasDiff = true;
            fastCompareData.firstDiffAt = i;
            fastCompareData.processedUntil = i;
            fastCompareData.l1Result = l1Result;
            return true;
        }
        if (l != r && fastCompareData.l3Result == UCollationResult::UCOL_EQUAL) {
            auto l3Result = ToUCollationResult(COLLATION_WEIGHT_L3[l] - COLLATION_WEIGHT_L3[r]);
            fastCompareData.l3Result = l3Result;
            if (!fastCompareData.hasDiff) {
                fastCompareData.hasDiff = true;
                fastCompareData.firstDiffAt = i;
            }
        }
    }
    fastCompareData.processedUntil = length;
    return true;
}

bool FastCompareStringFlatContent(EcmaString* string1, EcmaString* string2,
                                  int length, FastCompareStringsData& fastCompareData)
{
    EcmaStringAccessor string1Acc(string1);
    EcmaStringAccessor string2Acc(string2);
    if (string1Acc.IsUtf8()) {
        auto l = EcmaStringAccessor::GetNonTreeUtf8Data(string1);
        if (string2Acc.IsUtf8()) {
            auto r = EcmaStringAccessor::GetNonTreeUtf8Data(string2);
            return FastCompareFlatString(l, r, length, fastCompareData);
        } else {
            auto r = EcmaStringAccessor::GetNonTreeUtf16Data(string2);
            return FastCompareFlatString(l, r, length, fastCompareData);
        }
    } else {
        auto l = EcmaStringAccessor::GetNonTreeUtf16Data(string1);
        if (string2Acc.IsUtf8()) {
            auto r = EcmaStringAccessor::GetNonTreeUtf8Data(string2);
            return FastCompareFlatString(l, r, length, fastCompareData);
        } else {
            auto r = EcmaStringAccessor::GetNonTreeUtf16Data(string2);
            return FastCompareFlatString(l, r, length, fastCompareData);
        }
    }
    UNREACHABLE();
}

bool CharIsAsciiOrOutOfBounds(EcmaString* string, int stringLength, int index)
{
    return index >= stringLength || EcmaStringAccessor::IsASCIICharacter(EcmaStringAccessor(string).Get<false>(index));
}

bool CharCanFastCompareOrOutOfBounds(EcmaString* string, int stringLength, int index)
{
    return index >= stringLength || CanFastCompare(EcmaStringAccessor(string).Get<false>(index));
}

// Pseudo-code for simplified multi-pass algorithm is:
//     // Only a certain subset of the ASCII range can be fast-compared.
//     // In the actual single-pass algorithm below, we tolerate non-ASCII contents.
//     1. Check string1 and string2 can fastcompare.
//     2. Compare L1 weight for each char, the greater wins.
//     3. Is two strings are L1 equal in common length, the longer wins.
//     4. Compare L3 weight for each char, the greater wins.
//     5. If all equal, return equal.
//     6. Once some chars cannot be fastcompared, use icu.

std::optional<UCollationResult> TryFastCompareStrings(EcmaString* string1, EcmaString* string2,
                                                      int& processedUntilOut)
{
    processedUntilOut = 0;

    const auto length1 = static_cast<int>(EcmaStringAccessor(string1).GetLength());
    const auto length2 = static_cast<int>(EcmaStringAccessor(string2).GetLength());
    int commonLength = std::min(length1, length2);

    FastCompareStringsData fastCompareData;
    if (!FastCompareStringFlatContent(string1, string2, commonLength, fastCompareData)) {
        return fastCompareData.FastCompareFailed(processedUntilOut);
    }
    // The result is only valid if the last processed character is not followed
    // by a unicode combining character.
    if (!CharIsAsciiOrOutOfBounds(string1, length1, fastCompareData.processedUntil + 1) ||
        !CharIsAsciiOrOutOfBounds(string2, length2, fastCompareData.processedUntil + 1)) {
        return fastCompareData.FastCompareFailed(processedUntilOut);
    }
    if (fastCompareData.l1Result != UCollationResult::UCOL_EQUAL) {
        return fastCompareData.l1Result;
    }
    // Strings are L1-equal up to their common length, length differences win.
    UCollationResult lengthResult = ToUCollationResult(length1 - length2);
    if (lengthResult != UCollationResult::UCOL_EQUAL) {
        // Strings of different lengths may still compare as equal if the longer
        // string has a fully ignored suffix, e.g. "a" vs. "a\u{1}".
        if (!CharCanFastCompareOrOutOfBounds(string1, length1, commonLength) ||
            !CharCanFastCompareOrOutOfBounds(string2, length2, commonLength)) {
            return fastCompareData.FastCompareFailed(processedUntilOut);
        }
        return lengthResult;
    }
    // L1-equal and same length, the L3 result wins.
    return fastCompareData.l3Result;
}
} // namespace

//StringPiece is similar to std::string_view
icu::StringPiece ToICUStringPiece(EcmaString* string, int offset = 0)
{
    EcmaStringAccessor stringAcc(string);
    ASSERT(stringAcc.IsUtf8());
    ASSERT(!stringAcc.IsTreeString());
    return icu::StringPiece(reinterpret_cast<const char*>(EcmaStringAccessor::GetNonTreeUtf8Data(string)) + offset,
                            static_cast<int>(stringAcc.GetLength()) - offset);
}

// Convert to a UTF16 string and partially convert to ICUUnicodeString
icu::UnicodeString ToICUUnicodeString(EcmaString* string, int offset = 0)
{
    EcmaStringAccessor stringAcc(string);
    ASSERT(!stringAcc.IsTreeString());
    int strLength = static_cast<int>(stringAcc.GetLength());
    int partialLength = strLength - offset;
    if (stringAcc.IsUtf8()) {
        constexpr int shortStringLength = 80;  // 80: short string length
        if (partialLength <= shortStringLength) {
            // short string on stack
            UChar shortStringBuffer[shortStringLength];
            // utf8 is within ascii, std::copy_n from utf8 to utf16 is OK
            std::copy_n(EcmaStringAccessor::GetNonTreeUtf8Data(string) + offset, partialLength, shortStringBuffer);
            return icu::UnicodeString(shortStringBuffer, partialLength);
        }
        CVector<uint16_t> ucharBuffer(partialLength);
        std::copy_n(EcmaStringAccessor::GetNonTreeUtf8Data(string) + offset, partialLength, ucharBuffer.begin());
        return icu::UnicodeString(ucharBuffer.data(), partialLength);
    } else {
        return icu::UnicodeString(EcmaStringAccessor::GetNonTreeUtf16Data(string) + offset, partialLength);
    }
}

JSTaggedValue JSCollator::SlowCompareStrings(const icu::Collator *icuCollator,
                                             EcmaString* flatString1,
                                             EcmaString* flatString2,
                                             int processedUntil)
{
    UCollationResult result;
    UErrorCode status = U_ZERO_ERROR;
    if (EcmaStringAccessor(flatString1).IsUtf8() && EcmaStringAccessor(flatString2).IsUtf8()) {
        auto string1Piece = ToICUStringPiece(flatString1, processedUntil);
        if (!string1Piece.empty()) {
            auto string2Piece = ToICUStringPiece(flatString2, processedUntil);
            if (!string2Piece.empty()) {
                result = icuCollator->compareUTF8(string1Piece, string2Piece, status);
                return JSTaggedValue(result);
            }
        }
    }

    auto uString1 = ToICUUnicodeString(flatString1, processedUntil);
    auto uString2 = ToICUUnicodeString(flatString2, processedUntil);
    result = icuCollator->compare(uString1, uString2, status);
    ASSERT(U_SUCCESS(status));
    return JSTaggedValue(result);
}

JSTaggedValue JSCollator::CompareStrings(JSThread *thread, const icu::Collator *icuCollator,
                                         const JSHandle<EcmaString> &string1, const JSHandle<EcmaString> &string2,
                                         [[maybe_unused]]CompareStringsOption csOption)
{
    if (*string1 == *string2) {
        return JSTaggedValue(UCollationResult::UCOL_EQUAL);
    }

    // Since Unicode has ignorable characters,
    // we cannot return early for 0-length strings.
    auto flatString1 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), string1));
    auto flatString2 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), string2));

    int processedUntil = 0;
    if (csOption == CompareStringsOption::TRY_FAST_PATH) {
        auto maybeResult = TryFastCompareStrings(*flatString1, *flatString2, processedUntil);
        if (maybeResult.has_value()) {
            return JSTaggedValue(maybeResult.value());
        }
    }
    return SlowCompareStrings(icuCollator, *flatString1, *flatString2, processedUntil);
}

JSTaggedValue JSCollator::FastCachedCompareStrings(JSThread *thread, JSHandle<JSTaggedValue> locales,
                                                   const JSHandle<EcmaString> &string1,
                                                   const JSHandle<EcmaString> &string2,
                                                   CompareStringsOption csOption)
{
    if (*string1 == *string2) {
        return JSTaggedValue(UCollationResult::UCOL_EQUAL);
    }

    // Since Unicode has ignorable characters,
    // we cannot return early for 0-length strings.
    auto flatString1 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), string1));
    auto flatString2 = JSHandle<EcmaString>(thread, EcmaStringAccessor::Flatten(thread->GetEcmaVM(), string2));

    int processedUntil = 0;
    if (csOption == CompareStringsOption::TRY_FAST_PATH) {
        auto maybeResult = TryFastCompareStrings(*flatString1, *flatString2, processedUntil);
        if (maybeResult.has_value()) {
            return JSTaggedValue(maybeResult.value());
        }
    }

    auto icuCollator = JSCollator::GetCachedIcuCollator(thread, locales);
    if (icuCollator != nullptr) {
        return SlowCompareStrings(icuCollator, *flatString1, *flatString2, processedUntil);
    }
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript
