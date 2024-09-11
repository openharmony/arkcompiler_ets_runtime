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

#ifndef ECMASCRIPT_JS_COLLATOR_H
#define ECMASCRIPT_JS_COLLATOR_H

#include "ecmascript/js_locale.h"

#include "unicode/udata.h"

namespace panda::ecmascript {
enum class UsageOption : uint8_t { SORT = 0x01, SEARCH, EXCEPTION };
enum class CaseFirstOption : uint8_t { UPPER = 0x01, LOWER, FALSE_OPTION, UNDEFINED, EXCEPTION };
enum class SensitivityOption : uint8_t { BASE = 0x01, ACCENT, CASE, VARIANT, UNDEFINED, EXCEPTION };
enum class CompareStringsOption : uint8_t { NONE = 0x01, TRY_FAST_PATH };

class JSCollator : public JSObject {
public:
    // NOLINTNEXTLINE (readability-identifier-naming, fuchsia-statically-constructed-objects)
    static const CString uIcuDataColl;

    static const std::map<std::string, CaseFirstOption> caseFirstMap;

    static const std::map<CaseFirstOption, UColAttributeValue> uColAttributeValueMap;

    static const std::vector<LocaleMatcherOption> LOCALE_MATCHER_OPTION;
    static const std::vector<std::string> LOCALE_MATCHER_OPTION_NAME;

    static const std::vector<CaseFirstOption> CASE_FIRST_OPTION;
    static const std::vector<std::string> CASE_FIRST_OPTION_NAME;

    static const std::set<std::string> RELEVANT_EXTENSION_KEYS;

    static const std::vector<SensitivityOption> SENSITIVITY_OPTION;
    static const std::vector<std::string> SENSITIVITY_OPTION_NAME;

    static const std::vector<UsageOption> USAGE_OPTION;
    static const std::vector<std::string> USAGE_OPTION_NAME;

    // All the available locales that are statically known to fulfill fast path conditions.
    static const char *const FAST_LOCALE[];

    CAST_CHECK(JSCollator, IsJSCollator);

    static constexpr size_t ICU_FIELD_OFFSET = JSObject::SIZE;
    // icu field.
    ACCESSORS(IcuField, ICU_FIELD_OFFSET, LOCALE_OFFSET)
    ACCESSORS(Locale, LOCALE_OFFSET, COLLATION_OFFSET)
    ACCESSORS(Collation, COLLATION_OFFSET, BOUND_COMPARE_OFFSET)
    ACCESSORS(BoundCompare, BOUND_COMPARE_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t USAGE_BITS = 2;
    static constexpr size_t CASE_FIRST_BITS = 3;
    static constexpr size_t SENSITIVITY_BITS = 3;
    static constexpr size_t IGNORE_PUNCTUATION_BITS = 1;
    static constexpr size_t NUMERIC_BITS = 1;
    FIRST_BIT_FIELD(BitField, Usage, UsageOption, USAGE_BITS)
    NEXT_BIT_FIELD(BitField, CaseFirst, CaseFirstOption, CASE_FIRST_BITS, Usage)
    NEXT_BIT_FIELD(BitField, Sensitivity, SensitivityOption, SENSITIVITY_BITS, CaseFirst)
    NEXT_BIT_FIELD(BitField, IgnorePunctuation, bool, IGNORE_PUNCTUATION_BITS, Sensitivity)
    NEXT_BIT_FIELD(BitField, Numeric, bool, NUMERIC_BITS, IgnorePunctuation)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ICU_FIELD_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()

    icu::Collator *GetIcuCollator() const
    {
        ASSERT(GetIcuField().IsJSNativePointer());
        JSNativePointer *nativePointer = JSNativePointer::Cast(GetIcuField().GetTaggedObject());
        auto result = nativePointer->GetExternalPointer();
        return reinterpret_cast<icu::Collator *>(result);
    }

    static void FreeIcuCollator([[maybe_unused]] void *env, void *pointer, [[maybe_unused]] void *hint = nullptr)
    {
        if (pointer == nullptr) {
            return;
        }
        auto icuCollator = reinterpret_cast<icu::Collator *>(pointer);
        delete icuCollator;
    }

    static void SetIcuCollator(JSThread *thread, const JSHandle<JSCollator> &collator,
        icu::Collator *icuCollator, const NativePointerCallback &callback);

    // 11.1.1 InitializeCollator ( collator, locales, options )
    static JSHandle<JSCollator> InitializeCollator(JSThread *thread, const JSHandle<JSCollator> &collator,
                                                   const JSHandle<JSTaggedValue> &locales,
                                                   const JSHandle<JSTaggedValue> &options,
                                                   bool forIcuCache = false,
                                                   bool enableLocaleCache = false);

    static icu::Collator *GetCachedIcuCollator(JSThread *thread, const JSHandle<JSTaggedValue> &locales);
    static icu::Collator *GetCachedIcuCollator(JSThread *thread, const JSTaggedValue &locales);

    // 11.3.4 Intl.Collator.prototype.resolvedOptions ()
    static JSHandle<JSObject> ResolvedOptions(JSThread *thread, const JSHandle<JSCollator> &collator);

    static JSHandle<TaggedArray> GetAvailableLocales(JSThread *thread, bool enableLocaleCache = false);

    static CompareStringsOption CompareStringsOptionFor(JSThread *thread, JSHandle<JSTaggedValue> locales);

    static CompareStringsOption CompareStringsOptionFor(JSThread *thread, JSHandle<JSTaggedValue> locales,
                                                        JSHandle<JSTaggedValue> options);

    static JSTaggedValue CompareStrings(JSThread *thread, const icu::Collator *icuCollator,
                                        const JSHandle<EcmaString> &string1, const JSHandle<EcmaString> &string2,
                                        CompareStringsOption csOption = CompareStringsOption::NONE);

private:
    static CaseFirstOption StringToCaseFirstOption(const std::string &str);

    static UColAttributeValue OptionToUColAttribute(CaseFirstOption caseFirstOption);

    static std::set<std::string> BuildLocaleSet(const std::vector<std::string> &availableLocales, const char *path,
                                                const char *key);

    static void SetNumericOption(icu::Collator *icuCollator, bool numeric);

    static void SetCaseFirstOption(icu::Collator *icuCollator, CaseFirstOption caseFirstOption);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_COLLATOR_H
