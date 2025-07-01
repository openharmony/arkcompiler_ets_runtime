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

#ifndef ECMASCRIPT_JS_NUMBER_FORMAT_H
#define ECMASCRIPT_JS_NUMBER_FORMAT_H

#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_object.h"

namespace panda::ecmascript {
enum class StyleOption : uint8_t { DECIMAL = 0x01, CURRENCY, PERCENT, UNIT, EXCEPTION };

enum class CompactDisplayOption : uint8_t { SHORT = 0x01, LONG, EXCEPTION };

enum class SignDisplayOption : uint8_t { AUTO = 0x01, ALWAYS, NEVER, EXCEPTZERO, EXCEPTION };

enum class CurrencyDisplayOption : uint8_t { CODE = 0x01, SYMBOL, NARROWSYMBOL, NAME, EXCEPTION };

enum class CurrencySignOption : uint8_t { STANDARD = 0x01, ACCOUNTING, EXCEPTION };

enum class UnitDisplayOption : uint8_t { SHORT = 0x01, NARROW, LONG, EXCEPTION };

struct FractionDigitsOption {
    int32_t mnfdDefault = 0;
    int32_t mxfdDefault = 0;
};

class JSNumberFormat : public JSObject {
public:
    static constexpr uint32_t DEFAULT_FRACTION_DIGITS = 2;
    static constexpr uint32_t PERUNIT_STRING = 5;
    static const std::vector<StyleOption> STYLE_OPTION;
    static const std::vector<std::string> STYLE_OPTION_NAME;

    static const std::vector<CurrencyDisplayOption> CURRENCY_DISPLAY_OPTION;
    static const std::vector<std::string> CURRENCY_DISPLAY_OPTION_NAME;

    static const std::vector<CurrencySignOption> CURRENCY_SIGN_OPTION;
    static const std::vector<std::string> CURRENCY_SIGN_OPTION_NAME;

    static const std::vector<UnitDisplayOption> UNIT_DISPLAY_OPTION;
    static const std::vector<std::string> UNIT_DISPLAY_OPTION_NAME;

    static const std::vector<LocaleMatcherOption> LOCALE_MATCHER_OPTION;
    static const std::vector<std::string> LOCALE_MATCHER_OPTION_NAME;

    static const std::vector<NotationOption> NOTATION_OPTION;
    static const std::vector<std::string> NOTATION_OPTION_NAME;

    static const std::vector<SignDisplayOption> SIGN_DISPLAY_OPTION;
    static const std::vector<std::string> SIGN_DISPLAY_OPTION_NAME;

    static const std::vector<CompactDisplayOption> COMPACT_DISPLAY_OPTION;
    static const std::vector<std::string> COMPACT_DISPLAY_OPTION_NAME;
    CAST_CHECK(JSNumberFormat, IsJSNumberFormat);

    static constexpr size_t LOCALE_OFFSET = JSObject::SIZE;
    ACCESSORS(Locale, LOCALE_OFFSET, NUMBER_STRING_SYSTEM_OFFSET)
    ACCESSORS(NumberingSystem, NUMBER_STRING_SYSTEM_OFFSET, CURRENCY_OFFSET)
    ACCESSORS(Currency, CURRENCY_OFFSET, UNIT_OFFSET)
    ACCESSORS(Unit, UNIT_OFFSET, MINIMUM_INTEGER_DIGITS_OFFSET)
    ACCESSORS(MinimumIntegerDigits, MINIMUM_INTEGER_DIGITS_OFFSET, MINIMUM_FRACTION_DIGITS_OFFSET)
    ACCESSORS(MinimumFractionDigits, MINIMUM_FRACTION_DIGITS_OFFSET, MAXIMUM_FRACTION_DIGITS_OFFSET)
    ACCESSORS(MaximumFractionDigits, MAXIMUM_FRACTION_DIGITS_OFFSET, MINIMUM_SIGNIFICANT_DIGITS_OFFSET)
    ACCESSORS(MinimumSignificantDigits, MINIMUM_SIGNIFICANT_DIGITS_OFFSET, MAXIMUM_SIGNIFICANT_DIGITS_OFFSET)
    ACCESSORS(MaximumSignificantDigits, MAXIMUM_SIGNIFICANT_DIGITS_OFFSET, USER_GROUPING_OFFSET)
    ACCESSORS(UseGrouping, USER_GROUPING_OFFSET, BOUND_FORMAT_OFFSET)
    ACCESSORS(BoundFormat, BOUND_FORMAT_OFFSET, ICU_FIELD_OFFSET)
    ACCESSORS(IcuField, ICU_FIELD_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t STYLE_BITS = 3;
    static constexpr size_t CURRENCY_SIGN_BITS = 2;
    static constexpr size_t CURRENCY_DISPLAY_BITS = 3;
    static constexpr size_t UNIT_DISPLAY_BITS = 3;
    static constexpr size_t SIGN_DISPLAY_BITS = 3;
    static constexpr size_t COMPACT_DISPLAY_BITS = 2;
    static constexpr size_t NOTATION_BITS = 3;
    static constexpr size_t ROUNDING_TYPE_BITS = 3;
    FIRST_BIT_FIELD(BitField, Style, StyleOption, STYLE_BITS)
    NEXT_BIT_FIELD(BitField, CurrencySign, CurrencySignOption, CURRENCY_SIGN_BITS, Style)
    NEXT_BIT_FIELD(BitField, CurrencyDisplay, CurrencyDisplayOption, CURRENCY_DISPLAY_BITS, CurrencySign)
    NEXT_BIT_FIELD(BitField, UnitDisplay, UnitDisplayOption, UNIT_DISPLAY_BITS, CurrencyDisplay)
    NEXT_BIT_FIELD(BitField, SignDisplay, SignDisplayOption, SIGN_DISPLAY_BITS, UnitDisplay)
    NEXT_BIT_FIELD(BitField, CompactDisplay, CompactDisplayOption, COMPACT_DISPLAY_BITS, SignDisplay)
    NEXT_BIT_FIELD(BitField, Notation, NotationOption, NOTATION_BITS, CompactDisplay)
    NEXT_BIT_FIELD(BitField, RoundingType, RoundingType, ROUNDING_TYPE_BITS, Notation)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LOCALE_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()

    icu::number::LocalizedNumberFormatter *GetIcuCallTarget(JSThread *thread) const
    {
        ASSERT(GetIcuField(thread).IsJSNativePointer());
        auto result = JSNativePointer::Cast(GetIcuField(thread).GetTaggedObject())->GetExternalPointer();
        return reinterpret_cast<icu::number::LocalizedNumberFormatter *>(result);
    }

    static void FreeIcuNumberformat([[maybe_unused]] void *env, void *pointer, void *data)
    {
        if (pointer == nullptr) {
            return;
        }
        auto icuNumberformat = reinterpret_cast<icu::number::LocalizedNumberFormatter *>(pointer);
        icuNumberformat->~LocalizedNumberFormatter();
        if (data != nullptr) {
            reinterpret_cast<EcmaVM *>(data)->GetNativeAreaAllocator()->FreeBuffer(icuNumberformat);
        }
    }

    // 12.1.2 InitializeNumberFormat ( numberFormat, locales, options )
    static void InitializeNumberFormat(JSThread *thread,
                                       const JSHandle<JSNumberFormat> &numberFormat,
                                       const JSHandle<JSTaggedValue> &locales,
                                       const JSHandle<JSTaggedValue> &options,
                                       bool forIcuCache = false);

    // 12.1.3 CurrencyDigits ( currency )
    static int32_t CurrencyDigits(const icu::UnicodeString &currency);

    static icu::number::LocalizedNumberFormatter *GetCachedIcuNumberFormatter(JSThread *thread,
                                                                              const JSHandle<JSTaggedValue> &locales);

    // 12.1.8 FormatNumeric( numberFormat, x )
    static JSHandle<JSTaggedValue> FormatNumeric(JSThread *thread, const JSHandle<JSNumberFormat> &numberFormat,
                                                 JSTaggedValue x);
    static JSHandle<JSTaggedValue> FormatNumeric(JSThread *thread,
                                                 const icu::number::LocalizedNumberFormatter *icuNumberFormat,
                                                 JSTaggedValue x);

    // 12.1.9 FormatNumericToParts( numberFormat, x )
    static JSHandle<JSArray> FormatNumericToParts(JSThread *thread, const JSHandle<JSNumberFormat> &numberFormat,
                                                  JSTaggedValue x);

    // 12.1.12 UnwrapNumberFormat( nf )
    static JSHandle<JSTaggedValue> UnwrapNumberFormat(JSThread *thread, const JSHandle<JSTaggedValue> &nf);

    static JSHandle<TaggedArray> GetAvailableLocales(JSThread *thread);
    static void ResolvedOptions(JSThread *thread, const JSHandle<JSNumberFormat> &numberFormat,
                                const JSHandle<JSObject> &options);

    template<typename T>
    static icu::number::LocalizedNumberFormatter SetICUFormatterDigitOptions(JSThread *thread,
        icu::number::LocalizedNumberFormatter &icuNumberformatter, const JSHandle<T> &formatter)
    {
        int minimumIntegerDigits = formatter->GetMinimumIntegerDigits(thread).GetInt();
        // Set ICU formatter IntegerWidth to MinimumIntegerDigits
        icuNumberformatter =
            icuNumberformatter.integerWidth(icu::number::IntegerWidth::zeroFillTo(minimumIntegerDigits));

        int minimumSignificantDigits = formatter->GetMinimumSignificantDigits(thread).GetInt();
        int maximumSignificantDigits = formatter->GetMaximumSignificantDigits(thread).GetInt();
        int minimumFractionDigits = formatter->GetMinimumFractionDigits(thread).GetInt();
        int maximumFractionDigits = formatter->GetMaximumFractionDigits(thread).GetInt();

        // If roundingtype is "compact-rounding" return ICU formatter
        RoundingType roundingType = formatter->GetRoundingType();
        if (roundingType == RoundingType::COMPACTROUNDING) {
            return icuNumberformatter;
        }
        // Else, Set ICU formatter FractionDigits and SignificantDigits
        //   a. Set ICU formatter minFraction, maxFraction to MinimumFractionDigits, MaximumFractionDigits
        icu::number::Precision precision =
            icu::number::Precision::minMaxFraction(minimumFractionDigits, maximumFractionDigits);
        //   b. if MinimumSignificantDigits is not 0,
        //      Set ICU formatter minSignificantDigits, maxSignificantDigits to MinimumSignificantDigits,
        //      MaximumSignificantDigits
        if (minimumSignificantDigits != 0) {
            precision =
                icu::number::Precision::minMaxSignificantDigits(minimumSignificantDigits, maximumSignificantDigits);
        }
        return icuNumberformatter.precision(precision);
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_NUMBER_FORMAT_H