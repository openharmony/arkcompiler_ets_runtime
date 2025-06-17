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

#ifndef ECMASCRIPT_BASE_LOCALE_HELPER_H
#define ECMASCRIPT_BASE_LOCALE_HELPER_H

#include "ecmascript/js_handle.h"
#include "ecmascript/js_locale.h"

namespace panda::ecmascript::intl {
constexpr uint8_t INTL_INDEX_TWO = 2;
constexpr uint8_t INTL_INDEX_THREE = 3;
constexpr uint8_t INTL_INDEX_FOUR = 4;
constexpr uint8_t INTL_INDEX_FIVE = 5;
constexpr uint8_t INTL_INDEX_EIGHT = 8;
class LocaleHelper {
public:
    struct ParsedLocale {
        std::string base;
        std::string extension;
    };
    static JSHandle<EcmaString> UStringToString(JSThread *thread, const icu::UnicodeString &string);
    static JSHandle<EcmaString> UStringToString(JSThread *thread, const icu::UnicodeString &string, int32_t begin,
                                                int32_t end);

    // 9.2.1 CanonicalizeLocaleList ( locales )
    static JSHandle<TaggedArray> CanonicalizeLocaleList(JSThread *thread, const JSHandle<JSTaggedValue> &locales);
    // 6.2.3 CanonicalizeUnicodeLocaleId ( locale )
    static JSHandle<EcmaString> CanonicalizeUnicodeLocaleId(JSThread *thread, const JSHandle<EcmaString> &locale);
    static std::string ToStdStringLanguageTag(JSThread *thread, const icu::Locale &locale);
    static JSHandle<EcmaString> ToLanguageTag(JSThread *thread, const icu::Locale &locale);
    static std::vector<std::string> GetAvailableLocales(JSThread *thread, const char *key, const char *path);
    static bool IsStructurallyValidLanguageTag(JSThread *thread, const JSHandle<EcmaString> &tag);
    // 9.2.2 BestAvailableLocale ( availableLocales, locale )
    static std::string BestAvailableLocale(const std::vector<std::string> &availableLocales,
                                           const std::string &locale);
    static const std::string& StdStringDefaultLocale(JSThread *thread);
    static JSHandle<EcmaString> DefaultLocale(JSThread *thread);
    static LocaleHelper::ParsedLocale HandleLocale(JSThread *thread, const JSHandle<EcmaString> &localeString);
    static LocaleHelper::ParsedLocale HandleLocale(const std::string &localeString);
    static void HandleLocaleExtension(size_t &start, size_t &extensionEnd, const std::string result, size_t len);
    static std::string ConvertToStdString(const JSThread *thread, const JSHandle<EcmaString> &ecmaStr);
private:
    template<typename T>
    static JSHandle<TaggedArray> CanonicalizeHelper(JSThread *thread, JSHandle<T> &obj, JSHandle<TaggedArray> &seen);
    static bool DealwithLanguageTag(const std::vector<std::string> &containers, size_t &address);

    static inline constexpr int AsciiAlphaToLower(uint32_t c)
    {
        constexpr uint32_t FLAG = 0x20;
        return static_cast<int>(c | FLAG);
    }

    static bool IsLanguageSubtag(const std::string &value)
    {
        return IsAlpha(value, INTL_INDEX_TWO, INTL_INDEX_THREE) || IsAlpha(value, INTL_INDEX_FIVE, INTL_INDEX_EIGHT);
    }

    static bool IsScriptSubtag(const std::string &value)
    {
        return IsAlpha(value, INTL_INDEX_FOUR, INTL_INDEX_FOUR);
    }

    static bool IsRegionSubtag(const std::string &value)
    {
        return IsAlpha(value, INTL_INDEX_TWO, INTL_INDEX_TWO) || IsDigit(value, INTL_INDEX_THREE, INTL_INDEX_THREE);
    }

    static bool IsVariantSubtag(const std::string &value)
    {
        return IsThirdDigitAlphanum(value) || IsAlphanum(value, INTL_INDEX_FIVE, INTL_INDEX_EIGHT);
    }

    static bool IsThirdDigitAlphanum(const std::string &value)
    {
        return InRange(value[0], '0', '9') && value.length() == INTL_INDEX_FOUR &&
            IsAlphanum(value.substr(1), INTL_INDEX_THREE, INTL_INDEX_THREE);
    }

    static bool IsExtensionSingleton(const std::string &value)
    {
        return IsAlphanum(value, 1, 1);
    }

    static bool IsPrivateSubTag(std::string result, size_t len)
    {
        if ((len > 1) && (result[1] == '-')) {
            ASSERT(result[0] == 'x' || result[0] == 'i');
            return true;
        }
        return false;
    }

    template<typename T, typename U>
    static bool InRange(T value, U start, U end)
    {
        ASSERT(start <= end);
        ASSERT(sizeof(T) >= sizeof(U));
        return (value >= static_cast<T>(start)) && (value <= static_cast<T>(end));
    }

    static bool IsAsciiAlpha(char ch)
    {
        return InRange(ch, 'A', 'Z') || InRange(ch, 'a', 'z');
    }

    static bool IsAlpha(const std::string &str, size_t min, size_t max)
    {
        if (!InRange(str.length(), min, max)) {
            return false;
        }
        for (char c : str) {
            if (!IsAsciiAlpha(c)) {
                return false;
            }
        }
        return true;
    }

    static bool IsDigit(const std::string &str, size_t min, size_t max)
    {
        if (!InRange(str.length(), min, max)) {
            return false;
        }
        for (char i : str) {
            if (!InRange(i, '0', '9')) {
                return false;
            }
        }
        return true;
    }

    static bool IsAlphanum(const std::string &str, size_t min, size_t max)
    {
        if (!InRange(str.length(), min, max)) {
            return false;
        }
        for (char i : str) {
            if (!IsAsciiAlpha(i) && !InRange(i, '0', '9')) {
                return false;
            }
        }
        return true;
    }

    static bool ValidateOtherTags(const icu::Locale &locale, const char *packageName, const char *key, bool &res)
    {
        const char *localeCountry = locale.getCountry();
        const char *localeScript = locale.getScript();
        if (localeCountry[0] != '\0' && localeScript[0] != '\0') {
            std::string removeCountry = locale.getLanguage();
            removeCountry.append("-");
            removeCountry.append(localeScript);
            return CheckLocales(removeCountry.c_str(), key, packageName, res);
        }
        if (localeCountry[0] != '\0' || localeScript[0] != '\0') {
            std::string language = locale.getLanguage();
            return CheckLocales(language.c_str(), key, packageName, res);
        }
        return res;
    }

    static bool CheckLocales(const icu::Locale &locale, const char *key, const char *packageName, bool &res)
    {
        res = false;
        UErrorCode status = U_ZERO_ERROR;
        const char *formalLocale = locale.getName();
        UResourceBundle *localeRes = ures_open(packageName, formalLocale, &status);
        if (localeRes != nullptr && status == U_ZERO_ERROR) {
            if (key == nullptr) {
                res = true;
            } else {
                UResourceBundle *keyRes = ures_getByKey(localeRes, key, nullptr, &status);
                if (keyRes != nullptr && status == U_ZERO_ERROR) {
                    res = true;
                }
                ures_close(keyRes);
            }
        }
        ures_close(localeRes);
        if (res) {
            return res;
        } else {
            ValidateOtherTags(locale, packageName, key, res);
        }
        return res;
    }
};
}
#endif  // ECMASCRIPT_BASE_LOCALE_HELPER_H