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

#include "ecmascript/intl/locale_helper.h"

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/global_env.h"
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

namespace panda::ecmascript::intl {
JSHandle<EcmaString> LocaleHelper::UStringToString(JSThread *thread, const icu::UnicodeString &string)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromUtf16(reinterpret_cast<const uint16_t *>(string.getBuffer()), string.length());
}

JSHandle<EcmaString> LocaleHelper::UStringToString(JSThread *thread, const icu::UnicodeString &string, int32_t begin,
                                                   int32_t end)
{
    return UStringToString(thread, string.tempSubStringBetween(begin, end));
}

// 9.2.1 CanonicalizeLocaleList ( locales )
JSHandle<TaggedArray> LocaleHelper::CanonicalizeLocaleList(JSThread *thread, const JSHandle<JSTaggedValue> &locales)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1. If locales is undefined, then
    //    a. Return a new empty List.
    if (locales->IsUndefined()) {
        return factory->EmptyArray();
    }
    // 2. Let seen be a new empty List.
    JSHandle<TaggedArray> localeSeen = factory->NewTaggedArray(1);
    // 3. If Type(locales) is String or Type(locales) is Object and locales has an [[InitializedLocale]] internal slot,
    //    then
    //    a. Let O be CreateArrayFromList(« locales »).
    // 4. Else,
    //    a.Let O be ? ToObject(locales).
    if (locales->IsString()) {
        JSHandle<EcmaString> tag = JSHandle<EcmaString>::Cast(locales);
        JSHandle<TaggedArray> temp = factory->NewTaggedArray(1);
        temp->Set(thread, 0, tag.GetTaggedValue());
        JSHandle<JSArray> obj = JSArray::CreateArrayFromList(thread, temp);
        JSHandle<TaggedArray> finalSeen = CanonicalizeHelper<JSArray>(thread, obj, localeSeen);
        return finalSeen;
#ifdef ARK_SUPPORT_INTL
    } else if (locales->IsJSLocale()) {
        JSHandle<EcmaString> tag = JSLocale::ToString(thread, JSHandle<JSLocale>::Cast(locales));
        JSHandle<TaggedArray> temp = factory->NewTaggedArray(1);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
        temp->Set(thread, 0, tag.GetTaggedValue());
        JSHandle<JSArray> obj = JSArray::CreateArrayFromList(thread, temp);
        JSHandle<TaggedArray> finalSeen = CanonicalizeHelper<JSArray>(thread, obj, localeSeen);
        return finalSeen;
#endif
    } else {
        JSHandle<JSObject> obj = JSTaggedValue::ToObject(thread, locales);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
        JSHandle<TaggedArray> finalSeen = CanonicalizeHelper<JSObject>(thread, obj, localeSeen);
        return finalSeen;
    }
    return localeSeen;
}

template<typename T>
JSHandle<TaggedArray> LocaleHelper::CanonicalizeHelper(JSThread *thread, JSHandle<T> &obj, JSHandle<TaggedArray> &seen)
{
    OperationResult operationResult = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj),
                                                                 thread->GlobalConstants()->GetHandledLengthString());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
    JSTaggedNumber len = JSTaggedValue::ToLength(thread, operationResult.GetValue());
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 2. Let seen be a new empty List.
    uint32_t requestedLocalesLen = len.ToUint32();
    seen = factory->NewTaggedArray(requestedLocalesLen);
    // 6. Let k be 0.
    // 7. Repeat, while k < len
    JSMutableHandle<JSTaggedValue> pk(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> tag(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    JSHandle<JSTaggedValue> objTagged = JSHandle<JSTaggedValue>::Cast(obj);
    for (uint32_t k = 0; k < requestedLocalesLen; k++) {
        // a. Let Pk be ToString(k).
        JSHandle<JSTaggedValue> kHandle(thread, JSTaggedValue(k));
        JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, kHandle);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
        pk.Update(str.GetTaggedValue());
        // b. Let kPresent be ? HasProperty(O, Pk).
        bool kPresent = JSTaggedValue::HasProperty(thread, objTagged, pk);
        RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

        // c. If kPresent is true, then
        if (kPresent) {
            // i. Let kValue be ? Get(O, Pk).
            OperationResult result = JSTaggedValue::GetProperty(thread, objTagged, pk);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
            JSHandle<JSTaggedValue> kValue = result.GetValue();
            // ii. If Type(kValue) is not String or Object, throw a TypeError exception.
            if (!kValue->IsString() && !kValue->IsJSObject()) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "kValue is not String or Object.", factory->EmptyArray());
            }
            // iii. If Type(kValue) is Object and kValue has an [[InitializedLocale]] internal slot, then
            //        1. Let tag be kValue.[[Locale]].
            // iv.  Else,
            //        1. Let tag be ? ToString(kValue).
#ifdef ARK_SUPPORT_INTL
            if (kValue->IsJSLocale()) {
                JSHandle<EcmaString> kValueStr = JSLocale::ToString(thread, JSHandle<JSLocale>::Cast(kValue));
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
                tag.Update(kValueStr.GetTaggedValue());
            } else {
                JSHandle<EcmaString> kValueString = JSTaggedValue::ToString(thread, kValue);
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
                JSHandle<EcmaString> canonicalStr = CanonicalizeUnicodeLocaleId(thread, kValueString);
                RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
                tag.Update(canonicalStr.GetTaggedValue());
            }
#else
            JSHandle<EcmaString> kValueString = JSTaggedValue::ToString(thread, kValue);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
            JSHandle<EcmaString> canonicalStr = CanonicalizeUnicodeLocaleId(thread, kValueString);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
            tag.Update(canonicalStr.GetTaggedValue());
#endif
            // vii. If canonicalizedTag is not an element of seen, append canonicalizedTag as the last element of seen.
            bool isExist = false;
            uint32_t seenLen = seen->GetLength();
            for (uint32_t i = 0; i < seenLen; i++) {
                if (JSTaggedValue::SameValue(thread, seen->Get(thread, i), tag.GetTaggedValue())) {
                    isExist = true;
                }
            }
            if (!isExist) {
                seen->Set(thread, index++, JSHandle<JSTaggedValue>::Cast(tag));
            }
        }
        // d. Increase k by 1.
    }
    // set capacity
    seen = TaggedArray::SetCapacity(thread, seen, index);
    // 8. Return seen.
    return seen;
}

// 6.2.3 CanonicalizeUnicodeLocaleId( locale )
JSHandle<EcmaString> LocaleHelper::CanonicalizeUnicodeLocaleId(JSThread *thread, const JSHandle<EcmaString> &locale)
{
    [[maybe_unused]] ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (!IsStructurallyValidLanguageTag(thread, locale)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }

    if (EcmaStringAccessor(locale).GetLength() == 0 || EcmaStringAccessor(locale).IsUtf16()) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }

    std::string localeCStr = ConvertToStdString(thread, locale);
    std::transform(localeCStr.begin(), localeCStr.end(), localeCStr.begin(), AsciiAlphaToLower);
    UErrorCode status = U_ZERO_ERROR;
    icu::Locale formalLocale;
    {
        // Third party libs call can be in Native state
        ThreadNativeScope nativeScope(thread);
        formalLocale = icu::Locale::forLanguageTag(localeCStr.c_str(), status);
    }
    if ((U_FAILURE(status) != 0) || (formalLocale.isBogus() != 0)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }

    // Resets the LocaleBuilder to match the locale.
    // Returns an instance of Locale created from the fields set on this builder.
    formalLocale = icu::LocaleBuilder().setLocale(formalLocale).build(status);
    // Canonicalize the locale ID of this object according to CLDR.
    formalLocale.canonicalize(status);
    if ((U_FAILURE(status) != 0) || (formalLocale.isBogus() != 0)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", factory->GetEmptyString());
    }
    JSHandle<EcmaString> languageTag = ToLanguageTag(thread, formalLocale);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(EcmaString, thread);
    return languageTag;
}

std::string LocaleHelper::ToStdStringLanguageTag(JSThread *thread, const icu::Locale &locale)
{
    UErrorCode status = U_ZERO_ERROR;
    auto result = locale.toLanguageTag<std::string>(status);
    if (U_FAILURE(status) != 0) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "invalid locale", "");
    }
    size_t findBeginning = result.find("-u-");
    std::string finalRes;
    std::string tempRes;
    if (findBeginning == std::string::npos) {
        return result;
    }
    size_t specialBeginning = findBeginning + INTL_INDEX_THREE;
    size_t specialCount = 0;
    while ((specialBeginning < result.size()) && (result[specialBeginning] != '-')) {
        specialCount++;
        specialBeginning++;
    }
    thread->CheckSafepointIfSuspended();
    if (findBeginning != std::string::npos) {
        // It begin with "-u-xx" or with more elements.
        tempRes = result.substr(0, findBeginning + INTL_INDEX_THREE + specialCount);
        if (result.size() <= findBeginning + INTL_INDEX_THREE + specialCount) {
            return result;
        }
        std::string leftStr = result.substr(findBeginning + INTL_INDEX_THREE + specialCount + 1);
        std::istringstream temp(leftStr);
        std::string buffer;
        std::vector<std::string> resContainer;
        while (getline(temp, buffer, '-')) {
            if (buffer != "true" && buffer != "yes") {
                resContainer.push_back(buffer);
            }
        }
        for (auto it = resContainer.begin(); it != resContainer.end(); it++) {
            std::string tag = "-";
            tag += *it;
            finalRes += tag;
        }
    }
    if (!finalRes.empty()) {
        tempRes += finalRes;
    }
    result = tempRes;
    return result;
}

JSHandle<EcmaString> LocaleHelper::ToLanguageTag(JSThread *thread, const icu::Locale &locale)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromStdString(ToStdStringLanguageTag(thread, locale));
}

// 6.2.2 IsStructurallyValidLanguageTag( locale )
bool LocaleHelper::IsStructurallyValidLanguageTag(JSThread *thread, const JSHandle<EcmaString> &tag)
{
    std::string tagCollection = ConvertToStdString(thread, tag);
    std::vector<std::string> containers;
    std::string substring;
    std::set<std::string> uniqueSubtags;
    size_t address = 1;
    for (auto it = tagCollection.begin(); it != tagCollection.end(); it++) {
        if (*it != '-' && it != tagCollection.end() - 1) {
            substring += *it;
        } else {
            if (it == tagCollection.end() - 1) {
                substring += *it;
            }
            containers.push_back(substring);
            if (IsVariantSubtag(substring)) {
                std::transform(substring.begin(), substring.end(), substring.begin(), AsciiAlphaToLower);
                if (!uniqueSubtags.insert(substring).second) {
                    return false;
                }
            }
            substring.clear();
        }
    }
    bool result = DealwithLanguageTag(containers, address);
    return result;
}

std::string LocaleHelper::ConvertToStdString(const JSThread *thread, const JSHandle<EcmaString> &ecmaStr)
{
    return std::string(ConvertToString(thread, *ecmaStr, StringConvertedUsage::LOGICOPERATION));
}

bool LocaleHelper::DealwithLanguageTag(const std::vector<std::string> &containers, size_t &address)
{
    // The abstract operation returns true if locale can be generated from the ABNF grammar in section 2.1 of the RFC,
    // starting with Language-Tag, and does not contain duplicate variant or singleton subtags
    // If language tag is empty, return false.
    if (containers.empty()) {
        return false;
    }

    // a. if the first tag is not language, return false.
    if (!IsLanguageSubtag(containers[0])) {
        return false;
    }

    // if the tag include language only, like "zh" or "de", return true;
    if (containers.size() == 1) {
        return true;
    }

    // Else, then
    // if is unique singleton subtag, script and region tag.
    if (IsExtensionSingleton(containers[1])) {
        return true;
    }

    if (IsScriptSubtag(containers[address])) {
        address++;
        if (containers.size() == address) {
            return true;
        }
    }

    if (IsRegionSubtag(containers[address])) {
        address++;
    }

    for (size_t i = address; i < containers.size(); i++) {
        if (IsExtensionSingleton(containers[i])) {
            return true;
        }
        if (!IsVariantSubtag(containers[i])) {
            return false;
        }
    }
    return true;
}

// 6.2.4 DefaultLocale ()
JSHandle<EcmaString> LocaleHelper::DefaultLocale(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewFromStdString(StdStringDefaultLocale(thread));
}

const std::string& LocaleHelper::StdStringDefaultLocale(JSThread *thread)
{
    auto& intlCache = thread->GetEcmaVM()->GetIntlCache();
    const std::string& cachedLocale = intlCache.GetDefaultLocale();
    if (!cachedLocale.empty()) {
        return cachedLocale;
    }
    icu::Locale defaultLocale;
    if (strcmp(defaultLocale.getName(), "en_US_POSIX") == 0 || strcmp(defaultLocale.getName(), "c") == 0) {
        intlCache.SetDefaultLocale("en-US");
    } else if (defaultLocale.isBogus() != 0) {
        intlCache.SetDefaultLocale("und");
    } else {
        intlCache.SetDefaultLocale(ToStdStringLanguageTag(thread, defaultLocale));
    }
    return intlCache.GetDefaultLocale();
}

void LocaleHelper::HandleLocaleExtension(size_t &start, size_t &extensionEnd, const std::string result, size_t len)
{
    while (start < len - INTL_INDEX_TWO) {
        if (result[start] != '-') {
            start++;
            continue;
        }
        if (result[start + INTL_INDEX_TWO] == '-') {
            extensionEnd = start;
            break;
        }
        start += INTL_INDEX_THREE;
    }
}

LocaleHelper::ParsedLocale LocaleHelper::HandleLocale(JSThread *thread, const JSHandle<EcmaString> &localeString)
{
    return LocaleHelper::HandleLocale(ConvertToStdString(thread, localeString));
}

LocaleHelper::ParsedLocale LocaleHelper::HandleLocale(const std::string &localeString)
{
    size_t len = localeString.size();
    ParsedLocale parsedResult;

    // a. The single-character subtag ’x’ as the primary subtag indicates
    //    that the language tag consists solely of subtags whose meaning is
    //    defined by private agreement.
    // b. Extensions cannot be used in tags that are entirely private use.
    if (IsPrivateSubTag(localeString, len)) {
        parsedResult.base = localeString;
        return parsedResult;
    }
    // If cannot find "-u-", return the whole string as base.
    size_t foundExtension = localeString.find("-u-");
    if (foundExtension == std::string::npos) {
        parsedResult.base = localeString;
        return parsedResult;
    }
    // Let privateIndex be Call(%StringProto_indexOf%, foundLocale, « "-x-" »).
    size_t privateIndex = localeString.find("-x-");
    if (privateIndex != std::string::npos && privateIndex < foundExtension) {
        parsedResult.base = localeString;
        return parsedResult;
    }
    const std::string basis = localeString.substr(0, foundExtension);
    size_t extensionEnd = len;
    ASSERT(len > INTL_INDEX_TWO);
    size_t start = foundExtension + 1;
    HandleLocaleExtension(start, extensionEnd, localeString, len);
    const std::string end = localeString.substr(extensionEnd);
    parsedResult.base = basis + end;
    parsedResult.extension = localeString.substr(foundExtension, extensionEnd - foundExtension);
    return parsedResult;
}

std::vector<std::string> LocaleHelper::GetAvailableLocales(JSThread *thread, const char *localeKey,
                                                           const char *localePath)
{
    UErrorCode status = U_ZERO_ERROR;
    auto globalConst = thread->GlobalConstants();
    JSHandle<EcmaString> specialValue = JSHandle<EcmaString>::Cast(globalConst->GetHandledEnUsPosixString());
    std::string specialString = ConvertToStdString(thread, specialValue);
    UEnumeration *uenum = nullptr;
    {
        ThreadNativeScope nativeScope(thread);
        uenum = uloc_openAvailableByType(ULOC_AVAILABLE_WITH_LEGACY_ALIASES, &status);
    }
    std::vector<std::string> allLocales;
    const char *loc = nullptr;
    // Third party libs computing can be in Native state
    ThreadNativeScope nativeScope(thread);
    for (loc = uenum_next(uenum, nullptr, &status); loc != nullptr; loc = uenum_next(uenum, nullptr, &status)) {
        ASSERT(U_SUCCESS(status));
        std::string locStr(loc);
        std::replace(locStr.begin(), locStr.end(), '_', '-');
        if (locStr == specialString) {
            locStr = "en-US-u-va-posix";
        }

        if (localePath != nullptr || localeKey != nullptr) {
            icu::Locale locale(locStr.c_str());
            bool res = false;
            if (!CheckLocales(locale, localeKey, localePath, res)) {
                continue;
            }
        }
        allLocales.push_back(locStr);
        icu::Locale formalLocale = icu::Locale::createCanonical(locStr.c_str());
        std::string scriptStr = formalLocale.getScript();
        if (!scriptStr.empty()) {
            std::string languageStr = formalLocale.getLanguage();
            std::string countryStr = formalLocale.getCountry();
            std::string shortLocale = icu::Locale(languageStr.c_str(), countryStr.c_str()).getName();
            std::replace(shortLocale.begin(), shortLocale.end(), '_', '-');
            allLocales.push_back(shortLocale);
        }
    }
    uenum_close(uenum);
    return allLocales;
}

// 9.2.2 BestAvailableLocale ( availableLocales, locale )
std::string LocaleHelper::BestAvailableLocale(const std::vector<std::string> &availableLocales,
                                              const std::string &locale)
{
    // 1. Let candidate be locale.
    std::string localeCandidate = locale;
    std::string undefined = std::string();
    // 2. Repeat,
    uint32_t length = availableLocales.size();
    while (true) {
        // a. If availableLocales contains an element equal to candidate, return candidate.
        for (uint32_t i = 0; i < length; ++i) {
            std::string itemStr = availableLocales[i];
            if (itemStr == localeCandidate) {
                return localeCandidate;
            }
        }
        // b. Let pos be the character index of the last occurrence of "-" (U+002D) within candidate.
        //    If that character does not occur, return undefined.
        size_t pos = localeCandidate.rfind('-');
        if (pos == std::string::npos) {
            return undefined;
        }
        // c. If pos ≥ 2 and the character "-" occurs at index pos-2 of candidate, decrease pos by 2.
        if (pos >= INTL_INDEX_TWO && localeCandidate[pos - INTL_INDEX_TWO] == '-') {
            pos -= INTL_INDEX_TWO;
        }
        // d. Let candidate be the substring of candidate from position 0, inclusive, to position pos, exclusive.
        localeCandidate.resize(pos);
    }
}
} // namespace panda::ecmascript::base