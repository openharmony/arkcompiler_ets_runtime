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

#include "ecmascript/js_number_format.h"
#include "ecmascript/js_function.h"
#include "ecmascript/object_factory-inl.h"

namespace panda::ecmascript {

const std::vector<StyleOption> JSNumberFormat::STYLE_OPTION = {
    StyleOption::DECIMAL, StyleOption::PERCENT, StyleOption::CURRENCY, StyleOption::UNIT
};
const std::vector<std::string> JSNumberFormat::STYLE_OPTION_NAME = {
    "decimal", "percent", "currency", "unit"
};

const std::vector<CurrencyDisplayOption> JSNumberFormat::CURRENCY_DISPLAY_OPTION = {
    CurrencyDisplayOption::CODE, CurrencyDisplayOption::SYMBOL,
    CurrencyDisplayOption::NARROWSYMBOL, CurrencyDisplayOption::NAME
};
const std::vector<std::string> JSNumberFormat::CURRENCY_DISPLAY_OPTION_NAME = {
    "code", "symbol", "narrowSymbol", "name"
};

const std::vector<CurrencySignOption> JSNumberFormat::CURRENCY_SIGN_OPTION = {
    CurrencySignOption::STANDARD, CurrencySignOption::ACCOUNTING
};
const std::vector<std::string> JSNumberFormat::CURRENCY_SIGN_OPTION_NAME = {"standard", "accounting"};

const std::vector<UnitDisplayOption> JSNumberFormat::UNIT_DISPLAY_OPTION = {
    UnitDisplayOption::SHORT, UnitDisplayOption::NARROW, UnitDisplayOption::LONG
};
const std::vector<std::string> JSNumberFormat::UNIT_DISPLAY_OPTION_NAME = {"short", "narrow", "long"};

const std::vector<LocaleMatcherOption> JSNumberFormat::LOCALE_MATCHER_OPTION = {
    LocaleMatcherOption::LOOKUP, LocaleMatcherOption::BEST_FIT
};
const std::vector<std::string> JSNumberFormat::LOCALE_MATCHER_OPTION_NAME = {"lookup", "best fit"};

const std::vector<NotationOption> JSNumberFormat::NOTATION_OPTION = {
    NotationOption::STANDARD, NotationOption::SCIENTIFIC,
    NotationOption::ENGINEERING, NotationOption::COMPACT
};
const std::vector<std::string> JSNumberFormat::NOTATION_OPTION_NAME = {
    "standard", "scientific", "engineering", "compact"
};

const std::vector<SignDisplayOption> JSNumberFormat::SIGN_DISPLAY_OPTION = {
    SignDisplayOption::AUTO, SignDisplayOption::NEVER,
    SignDisplayOption::ALWAYS, SignDisplayOption::EXCEPTZERO
};
const std::vector<std::string> JSNumberFormat::SIGN_DISPLAY_OPTION_NAME = {
    "auto", "never", "always", "exceptZero"
};

const std::vector<CompactDisplayOption> JSNumberFormat::COMPACT_DISPLAY_OPTION = {
    CompactDisplayOption::SHORT, CompactDisplayOption::LONG
};
const std::vector<std::string> JSNumberFormat::COMPACT_DISPLAY_OPTION_NAME = {"short", "long"};

const std::set<std::string> SANCTIONED_UNIT({ "acre", "bit", "byte", "celsius", "centimeter", "day", "degree",
                                            "fahrenheit", "fluid-ounce", "foot", "gallon", "gigabit", "gigabyte",
                                            "gram", "hectare", "hour", "inch", "kilobit", "kilobyte", "kilogram",
                                            "kilometer", "liter", "megabit", "megabyte", "meter", "mile",
                                            "mile-scandinavian", "millimeter", "milliliter", "millisecond",
                                            "minute", "month", "ounce", "percent", "petabyte", "pound", "second",
                                            "stone", "terabit", "terabyte", "week", "yard", "year" });

JSHandle<JSTaggedValue> OptionToEcmaString(JSThread *thread, StyleOption style)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (style) {
        case StyleOption::DECIMAL:
            result.Update(globalConst->GetHandledDecimalString().GetTaggedValue());
            break;
        case StyleOption::CURRENCY:
            result.Update(globalConst->GetHandledCurrencyString().GetTaggedValue());
            break;
        case StyleOption::PERCENT:
            result.Update(globalConst->GetHandledPercentString().GetTaggedValue());
            break;
        case StyleOption::UNIT:
            result.Update(globalConst->GetHandledUnitString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> OptionToEcmaString(JSThread *thread, CurrencyDisplayOption currencyDisplay)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (currencyDisplay) {
        case CurrencyDisplayOption::CODE:
            result.Update(globalConst->GetHandledCodeString().GetTaggedValue());
            break;
        case CurrencyDisplayOption::SYMBOL:
            result.Update(globalConst->GetHandledSymbolString().GetTaggedValue());
            break;
        case CurrencyDisplayOption::NARROWSYMBOL:
            result.Update(globalConst->GetHandledNarrowSymbolString().GetTaggedValue());
            break;
        case CurrencyDisplayOption::NAME:
            result.Update(globalConst->GetHandledNameString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> OptionToEcmaString(JSThread *thread, CurrencySignOption currencySign)
{
    auto globalConst = thread->GlobalConstants();
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    switch (currencySign) {
        case CurrencySignOption::STANDARD:
            result.Update(globalConst->GetHandledStandardString().GetTaggedValue());
            break;
        case CurrencySignOption::ACCOUNTING:
            result.Update(globalConst->GetHandledAccountingString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> OptionToEcmaString(JSThread *thread, UnitDisplayOption unitDisplay)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (unitDisplay) {
        case UnitDisplayOption::SHORT:
            result.Update(globalConst->GetHandledShortString().GetTaggedValue());
            break;
        case UnitDisplayOption::NARROW:
            result.Update(globalConst->GetHandledNarrowString().GetTaggedValue());
            break;
        case UnitDisplayOption::LONG:
            result.Update(globalConst->GetHandledLongString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> OptionToEcmaString(JSThread *thread, NotationOption notation)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (notation) {
        case NotationOption::STANDARD:
            result.Update(globalConst->GetHandledStandardString().GetTaggedValue());
            break;
        case NotationOption::SCIENTIFIC:
            result.Update(globalConst->GetHandledScientificString().GetTaggedValue());
            break;
        case NotationOption::ENGINEERING:
            result.Update(globalConst->GetHandledEngineeringString().GetTaggedValue());
            break;
        case NotationOption::COMPACT:
            result.Update(globalConst->GetHandledCompactString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> OptionToEcmaString(JSThread *thread, CompactDisplayOption compactDisplay)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (compactDisplay) {
        case CompactDisplayOption::SHORT:
            result.Update(globalConst->GetHandledShortString().GetTaggedValue());
            break;
        case CompactDisplayOption::LONG:
            result.Update(globalConst->GetHandledLongString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

JSHandle<JSTaggedValue> OptionToEcmaString(JSThread *thread, SignDisplayOption signDisplay)
{
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    auto globalConst = thread->GlobalConstants();
    switch (signDisplay) {
        case SignDisplayOption::AUTO:
            result.Update(globalConst->GetHandledAutoString().GetTaggedValue());
            break;
        case SignDisplayOption::ALWAYS:
            result.Update(globalConst->GetHandledAlwaysString().GetTaggedValue());
            break;
        case SignDisplayOption::NEVER:
            result.Update(globalConst->GetHandledNeverString().GetTaggedValue());
            break;
        case SignDisplayOption::EXCEPTZERO:
            result.Update(globalConst->GetHandledExceptZeroString().GetTaggedValue());
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    return result;
}

icu::MeasureUnit ToMeasureUnit(const std::string &sanctionedUnit)
{
    UErrorCode status = U_ZERO_ERROR;
    // Get All ICU measure unit
    int32_t total = icu::MeasureUnit::getAvailable(nullptr, 0, status);
    status = U_ZERO_ERROR;
    std::vector<icu::MeasureUnit> units(total);
    icu::MeasureUnit::getAvailable(units.data(), total, status);
    ASSERT(U_SUCCESS(status));

    // Find measure unit according to sanctioned unit
    //  then return measure unit
    for (auto &unit : units) {
        if (std::strcmp(sanctionedUnit.c_str(), unit.getSubtype()) == 0) {
            return unit;
        }
    }
    return icu::MeasureUnit();
}

// ecma402 #sec-issanctionedsimpleunitidentifier
bool IsSanctionedSimpleUnitIdentifier(const std::string &unit)
{
    // 1. If unitIdentifier is listed in sanctioned unit set, return true.
    auto it = SANCTIONED_UNIT.find(unit);
    if (it != SANCTIONED_UNIT.end()) {
        return true;
    }

    // 2. Else, Return false.
    return false;
}

// 6.5.1 IsWellFormedUnitIdentifier ( unitIdentifier )
bool IsWellFormedUnitIdentifier(const std::string &unit, icu::MeasureUnit &icuUnit, icu::MeasureUnit &icuPerUnit)
{
    // 1. If the result of IsSanctionedSimpleUnitIdentifier(unitIdentifier) is true, then
    //      a. Return true.
    icu::MeasureUnit result = icu::MeasureUnit();
    icu::MeasureUnit emptyUnit = icu::MeasureUnit();
    auto pos = unit.find("-per-");
    if (IsSanctionedSimpleUnitIdentifier(unit) && pos == std::string::npos) {
        result = ToMeasureUnit(unit);
        icuUnit = result;
        icuPerUnit = emptyUnit;
        return true;
    }

    // 2. If the substring "-per-" does not occur exactly once in unitIdentifier,
    //      a. then false
    size_t afterPos = pos + JSNumberFormat::PERUNIT_STRING;
    if (pos == std::string::npos || unit.find("-per-", afterPos) != std::string::npos) {
        return false;
    }

    // 3. Let numerator be the substring of unitIdentifier from the beginning to just before "-per-".
    std::string numerator = unit.substr(0, pos);
    // 4. If the result of IsSanctionedUnitIdentifier(numerator) is false, then
    //      a. return false
    if (IsSanctionedSimpleUnitIdentifier(numerator)) {
        result = ToMeasureUnit(numerator);
    } else {
        return false;
    }

    // 5. Let denominator be the substring of unitIdentifier from just after "-per-" to the end.
    std::string denominator = unit.substr(pos + JSNumberFormat::PERUNIT_STRING);

    // 6. If the result of IsSanctionedUnitIdentifier(denominator) is false, then
    //      a. Return false
    icu::MeasureUnit perResult = icu::MeasureUnit();
    if (IsSanctionedSimpleUnitIdentifier(denominator)) {
        perResult = ToMeasureUnit(denominator);
    } else {
        return false;
    }

    // 7. Return true.
    icuUnit = result;
    icuPerUnit = perResult;
    return true;
}

// 12.1.13 SetNumberFormatUnitOptions ( intlObj, options )
FractionDigitsOption SetNumberFormatUnitOptions(JSThread *thread,
                                                const JSHandle<JSNumberFormat> &numberFormat,
                                                const JSHandle<JSObject> &optionsObject,
                                                icu::number::LocalizedNumberFormatter *icuNumberFormatter)
{
    auto globalConst = thread->GlobalConstants();
    FractionDigitsOption fractionDigitsOption;
    // 3. Let style be ? GetOption(options, "style", "string", « "decimal", "percent", "currency", "unit" », "decimal").
    JSHandle<JSTaggedValue> property = globalConst->GetHandledStyleString();
    auto style = JSLocale::GetOptionOfString<StyleOption>(
        thread, optionsObject, property,
        JSNumberFormat::STYLE_OPTION, JSNumberFormat::STYLE_OPTION_NAME,
        StyleOption::DECIMAL);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fractionDigitsOption);

    // 4. Set intlObj.[[Style]] to style.
    numberFormat->SetStyle(style);

    // 5. Let currency be ? GetOption(options, "currency", "string", undefined, undefined).
    property = globalConst->GetHandledCurrencyString();
    JSHandle<JSTaggedValue> undefinedValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> currency =
        JSLocale::GetOption(thread, optionsObject, property, OptionType::STRING, undefinedValue, undefinedValue);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fractionDigitsOption);

    // 6. If currency is not undefined, then
    //    a. If the result of IsWellFormedCurrencyCode(currency) is false, throw a RangeError exception.
    if (!currency->IsUndefined()) {
        JSHandle<EcmaString> currencyStr = JSHandle<EcmaString>::Cast(currency);
        if (EcmaStringAccessor(currencyStr).IsUtf16()) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "not a utf-8", fractionDigitsOption);
        }
        std::string currencyCStr = intl::LocaleHelper::ConvertToStdString(thread, currencyStr);
        if (!JSLocale::IsWellFormedCurrencyCode(currencyCStr)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "not a wellformed code", fractionDigitsOption);
        }
    } else {
        // 7. If style is "currency" and currency is undefined, throw a TypeError exception.
        if (style == StyleOption::CURRENCY) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "style is currency but currency is undefined", fractionDigitsOption);
        }
    }

    // 8. Let currencyDisplay be ? GetOption(options, "currencyDisplay", "string",
    //  « "code", "symbol", "narrowSymbol", "name" », "symbol").
    property = globalConst->GetHandledCurrencyDisplayString();
    auto currencyDisplay = JSLocale::GetOptionOfString<CurrencyDisplayOption>(
        thread, optionsObject, property,
        JSNumberFormat::CURRENCY_DISPLAY_OPTION, JSNumberFormat::CURRENCY_DISPLAY_OPTION_NAME,
        CurrencyDisplayOption::SYMBOL);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fractionDigitsOption);
    numberFormat->SetCurrencyDisplay(currencyDisplay);

    // 9. Let currencySign be ? GetOption(options, "currencySign", "string", « "standard", "accounting" », "standard").
    property = globalConst->GetHandledCurrencySignString();
    auto currencySign = JSLocale::GetOptionOfString<CurrencySignOption>(
        thread, optionsObject, property,
        JSNumberFormat::CURRENCY_SIGN_OPTION, JSNumberFormat::CURRENCY_SIGN_OPTION_NAME,
        CurrencySignOption::STANDARD);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fractionDigitsOption);
    numberFormat->SetCurrencySign(currencySign);

    // 10. Let unit be ? GetOption(options, "unit", "string", undefined, undefined).
    property = globalConst->GetHandledUnitString();
    JSHandle<JSTaggedValue> unit =
        JSLocale::GetOption(thread, optionsObject, property, OptionType::STRING, undefinedValue, undefinedValue);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fractionDigitsOption);
    numberFormat->SetUnit(thread, unit);

    // 11. If unit is not undefined, then
    // If the result of IsWellFormedUnitIdentifier(unit) is false, throw a RangeError exception.
    icu::MeasureUnit icuUnit;
    icu::MeasureUnit icuPerUnit;
    if (!unit->IsUndefined()) {
        JSHandle<EcmaString> unitStr = JSHandle<EcmaString>::Cast(unit);
        if (EcmaStringAccessor(unitStr).IsUtf16()) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "Unit input is illegal", fractionDigitsOption);
        }
        std::string str = intl::LocaleHelper::ConvertToStdString(thread, unitStr);
        if (!IsWellFormedUnitIdentifier(str, icuUnit, icuPerUnit)) {
            THROW_RANGE_ERROR_AND_RETURN(thread, "Unit input is illegal", fractionDigitsOption);
        }
    } else {
        // 15.12. if style is "unit" and unit is undefined, throw a TypeError exception.
        if (style == StyleOption::UNIT) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "style is unit but unit is undefined", fractionDigitsOption);
        }
    }

    // 13. Let unitDisplay be ? GetOption(options, "unitDisplay", "string", « "short", "narrow", "long" », "short").
    property = globalConst->GetHandledUnitDisplayString();
    auto unitDisplay = JSLocale::GetOptionOfString<UnitDisplayOption>(
        thread, optionsObject, property,
        JSNumberFormat::UNIT_DISPLAY_OPTION, JSNumberFormat::UNIT_DISPLAY_OPTION_NAME,
        UnitDisplayOption::SHORT);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, fractionDigitsOption);
    numberFormat->SetUnitDisplay(unitDisplay);

    // 14. If style is "currency", then
    //      a. Let currency be the result of converting currency to upper case as specified in 6.1.
    //      b. Set intlObj.[[Currency]] to currency.
    //      c. Set intlObj.[[CurrencyDisplay]] to currencyDisplay.
    //      d. Set intlObj.[[CurrencySign]] to currencySign.
    icu::UnicodeString currencyUStr;
    UErrorCode status = U_ZERO_ERROR;
    if (style == StyleOption::CURRENCY) {
        JSHandle<EcmaString> currencyStr = JSHandle<EcmaString>::Cast(currency);
        std::string currencyCStr = intl::LocaleHelper::ConvertToStdString(thread, currencyStr);
        std::transform(currencyCStr.begin(), currencyCStr.end(), currencyCStr.begin(), toupper);
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSTaggedValue> currencyValue = JSHandle<JSTaggedValue>::Cast(factory->NewFromStdString(currencyCStr));
        numberFormat->SetCurrency(thread, currencyValue);
        currencyUStr = currencyCStr.c_str();
        if (!currencyUStr.isEmpty()) {  // NOLINT(readability-implicit-bool-conversion)
            *icuNumberFormatter = icuNumberFormatter->unit(icu::CurrencyUnit(currencyUStr.getBuffer(), status));
            ASSERT(U_SUCCESS(status));
            UNumberUnitWidth uNumberUnitWidth;
            // Trans currencyDisplayOption to ICU format number display option
            switch (currencyDisplay) {
                case CurrencyDisplayOption::CODE:
                    uNumberUnitWidth = UNumberUnitWidth::UNUM_UNIT_WIDTH_ISO_CODE;
                    break;
                case CurrencyDisplayOption::SYMBOL:
                    uNumberUnitWidth = UNumberUnitWidth::UNUM_UNIT_WIDTH_SHORT;
                    break;
                case CurrencyDisplayOption::NARROWSYMBOL:
                    uNumberUnitWidth = UNumberUnitWidth::UNUM_UNIT_WIDTH_NARROW;
                    break;
                case CurrencyDisplayOption::NAME:
                    uNumberUnitWidth = UNumberUnitWidth::UNUM_UNIT_WIDTH_FULL_NAME;
                    break;
                default:
                    LOG_ECMA(FATAL) << "this branch is unreachable";
                    UNREACHABLE();
            }
            *icuNumberFormatter = icuNumberFormatter->unitWidth(uNumberUnitWidth);
        }
    }

    // 15. If style is "unit", then
    // if unit is not undefiend set unit to LocalizedNumberFormatter
    //    then if perunit is not undefiend set perunit to LocalizedNumberFormatter
    if (style == StyleOption::UNIT) {
        icu::MeasureUnit emptyUnit = icu::MeasureUnit();
        if (icuUnit != emptyUnit) {  // NOLINT(readability-implicit-bool-conversion)
            *icuNumberFormatter = icuNumberFormatter->unit(icuUnit);
        }
        if (icuPerUnit != emptyUnit) {  // NOLINT(readability-implicit-bool-conversion)
            *icuNumberFormatter = icuNumberFormatter->perUnit(icuPerUnit);
        }
    }

    // 17. If style is "currency", then
    //      a. Let cDigits be CurrencyDigits(currency).
    //      b. Let mnfdDefault be cDigits.
    //      c. Let mxfdDefault be cDigits.
    if (style == StyleOption::CURRENCY) {
        int32_t cDigits = JSNumberFormat::CurrencyDigits(currencyUStr);
        fractionDigitsOption.mnfdDefault = cDigits;
        fractionDigitsOption.mxfdDefault = cDigits;
    } else {
        // 18. Else,
        //      a. Let mnfdDefault be 0.
        //      b. If style is "percent", then
        //          i. Let mxfdDefault be 0.
        //      c. else,
        //          i. Let mxfdDefault be 3.
        fractionDigitsOption.mnfdDefault = 0;
        if (style == StyleOption::PERCENT) {
            fractionDigitsOption.mxfdDefault = 0;
        } else {
            fractionDigitsOption.mxfdDefault = 3;  // Max decimal precision is 3
        }
    }
    return fractionDigitsOption;
}

// 12.1.2 InitializeNumberFormat ( numberFormat, locales, options )
// NOLINTNEXTLINE(readability-function-size)
void JSNumberFormat::InitializeNumberFormat(JSThread *thread, const JSHandle<JSNumberFormat> &numberFormat,
                                            const JSHandle<JSTaggedValue> &locales,
                                            const JSHandle<JSTaggedValue> &options,
                                            bool forIcuCache)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    // 1. Let requestedLocales be ? CanonicalizeLocaleList(locales).
    JSHandle<TaggedArray> requestedLocales = intl::LocaleHelper::CanonicalizeLocaleList(thread, locales);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    // 2. If options is undefined, then
    //      a. Let options be ObjectCreate(null).
    // 3. Else,
    //      a. Let options be ? ToObject(options).
    JSHandle<JSObject> optionsObject;
    if (options->IsUndefined()) {
        optionsObject = factory->CreateNullJSObject();
    } else {
        optionsObject = JSTaggedValue::ToObject(thread, options);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }

    auto globalConst = thread->GlobalConstants();
    // 5. Let matcher be ? GetOption(options, "localeMatcher", "string", « "lookup", "best fit" », "best fit").
    JSHandle<JSTaggedValue> property = globalConst->GetHandledLocaleMatcherString();
    auto matcher = JSLocale::GetOptionOfString<LocaleMatcherOption>(
        thread, optionsObject, property,
        JSNumberFormat::LOCALE_MATCHER_OPTION, JSNumberFormat::LOCALE_MATCHER_OPTION_NAME,
        LocaleMatcherOption::BEST_FIT);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // 7. Let numberingSystem be ? GetOption(options, "numberingSystem", "string", undefined, undefined).
    property = globalConst->GetHandledNumberingSystemString();
    JSHandle<JSTaggedValue> undefinedValue(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> numberingSystemTaggedValue =
        JSLocale::GetOption(thread, optionsObject, property, OptionType::STRING, undefinedValue, undefinedValue);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    numberFormat->SetNumberingSystem(thread, numberingSystemTaggedValue);

    // 8. If numberingSystem is not undefined, then
    //      a. If numberingSystem does not match the Unicode Locale Identifier type nonterminal,
    //      throw a RangeError exception. `(3*8alphanum) *("-" (3*8alphanum))`
    std::string numberingSystemStr;
    if (!numberingSystemTaggedValue->IsUndefined()) {
        JSHandle<EcmaString> numberingSystemEcmaString = JSHandle<EcmaString>::Cast(numberingSystemTaggedValue);
        if (EcmaStringAccessor(numberingSystemEcmaString).IsUtf16()) {
            THROW_ERROR(thread, ErrorType::RANGE_ERROR, "invalid numberingSystem");
        }
        numberingSystemStr = intl::LocaleHelper::ConvertToStdString(thread, numberingSystemEcmaString);
        if (!JSLocale::IsNormativeNumberingSystem(numberingSystemStr)) {
            THROW_ERROR(thread, ErrorType::RANGE_ERROR, "invalid numberingSystem");
        }
    }

    if (!numberingSystemStr.empty()) {
        // If numberingSystem is invalid, Let numberingSystem be undefined.
        if (!JSLocale::IsWellNumberingSystem(numberingSystemStr)) {
            numberFormat->SetNumberingSystem(thread, undefinedValue);
        }
    }

    // 10. Let localeData be %NumberFormat%.[[LocaleData]].
    JSHandle<TaggedArray> availableLocales;
    if (requestedLocales->GetLength() == 0) {
        availableLocales = factory->EmptyArray();
    } else {
        availableLocales = GetAvailableLocales(thread);
    }

    // 11. Let r be ResolveLocale( %NumberFormat%.[[AvailableLocales]], requestedLocales, opt,
    // %NumberFormat%.[[RelevantExtensionKeys]], localeData).
    std::set<std::string> relevantExtensionKeys{"nu"};
    ResolvedLocale r =
        JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, matcher, relevantExtensionKeys);

    // 12. Set numberFormat.[[Locale]] to r.[[locale]].
    icu::Locale icuLocale = r.localeData;
    JSHandle<EcmaString> localeStr = intl::LocaleHelper::ToLanguageTag(thread, icuLocale);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    numberFormat->SetLocale(thread, localeStr.GetTaggedValue());

    // Set numberingSystemStr to UnicodeKeyWord "nu"
    UErrorCode status = U_ZERO_ERROR;
    if (!numberingSystemStr.empty()) {
        if (JSLocale::IsWellNumberingSystem(numberingSystemStr)) {
            icuLocale.setUnicodeKeywordValue("nu", numberingSystemStr, status);
            ASSERT(U_SUCCESS(status));
        }
    }

    icu::number::LocalizedNumberFormatter icuNumberFormatter =
        icu::number::NumberFormatter::withLocale(icuLocale).roundingMode(UNUM_ROUND_HALFUP);

    int32_t mnfdDefault = 0;
    int32_t mxfdDefault = 0;
    FractionDigitsOption fractionOptions =
        SetNumberFormatUnitOptions(thread, numberFormat, optionsObject, &icuNumberFormatter);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    mnfdDefault = fractionOptions.mnfdDefault;
    mxfdDefault = fractionOptions.mxfdDefault;
    UnitDisplayOption unitDisplay = numberFormat->GetUnitDisplay();

    // Trans unitDisplay option to ICU display option
    UNumberUnitWidth uNumberUnitWidth;
    switch (unitDisplay) {
        case UnitDisplayOption::SHORT:
            uNumberUnitWidth = UNumberUnitWidth::UNUM_UNIT_WIDTH_SHORT;
            break;
        case UnitDisplayOption::NARROW:
            uNumberUnitWidth = UNumberUnitWidth::UNUM_UNIT_WIDTH_NARROW;
            break;
        case UnitDisplayOption::LONG:
            // UNUM_UNIT_WIDTH_FULL_NAME Print the full name of the unit, without any abbreviations.
            uNumberUnitWidth = UNumberUnitWidth::UNUM_UNIT_WIDTH_FULL_NAME;
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
    }
    icuNumberFormatter = icuNumberFormatter.unitWidth(uNumberUnitWidth);

    // 16. Let style be numberFormat.[[Style]].
    StyleOption style = numberFormat->GetStyle();
    if (style == StyleOption::PERCENT) {
        icuNumberFormatter = icuNumberFormatter.unit(icu::MeasureUnit::getPercent()).
            scale(icu::number::Scale::powerOfTen(2));  // means 10^2
    }

    // 19. Let notation be ? GetOption(
    //  options, "notation", "string", « "standard", "scientific", "engineering", "compact" », "standard").
    property = globalConst->GetHandledNotationString();
    auto notation = JSLocale::GetOptionOfString<NotationOption>(
        thread, optionsObject, property,
        JSNumberFormat::NOTATION_OPTION, JSNumberFormat::NOTATION_OPTION_NAME,
        NotationOption::STANDARD);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    numberFormat->SetNotation(notation);

    // 21. Perform ? SetNumberFormatDigitOptions(numberFormat, options, mnfdDefault, mxfdDefault, notation).
    JSLocale::SetNumberFormatDigitOptions(thread, numberFormat, JSHandle<JSTaggedValue>::Cast(optionsObject),
                                          mnfdDefault, mxfdDefault, notation);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    icuNumberFormatter = SetICUFormatterDigitOptions(thread, icuNumberFormatter, numberFormat);

    // 22. Let compactDisplay be ? GetOptionOfString(options, "compactDisplay", "string", « "short", "long" », "short").
    property = globalConst->GetHandledCompactDisplayString();
    auto compactDisplay = JSLocale::GetOptionOfString<CompactDisplayOption>(
        thread, optionsObject, property,
        JSNumberFormat::COMPACT_DISPLAY_OPTION, JSNumberFormat::COMPACT_DISPLAY_OPTION_NAME,
        CompactDisplayOption::SHORT);
    numberFormat->SetCompactDisplay(compactDisplay);

    // Trans NotationOption to ICU Noation and set to icuNumberFormatter
    if (notation == NotationOption::COMPACT) {
        switch (compactDisplay) {
            case CompactDisplayOption::SHORT:
                icuNumberFormatter = icuNumberFormatter.notation(icu::number::Notation::compactShort());
                break;
            case CompactDisplayOption::LONG:
                icuNumberFormatter = icuNumberFormatter.notation(icu::number::Notation::compactLong());
                break;
            default:
                break;
        }
    }
    switch (notation) {
        case NotationOption::STANDARD:
            icuNumberFormatter = icuNumberFormatter.notation(icu::number::Notation::simple());
            break;
        case NotationOption::SCIENTIFIC:
            icuNumberFormatter = icuNumberFormatter.notation(icu::number::Notation::scientific());
            break;
        case NotationOption::ENGINEERING:
            icuNumberFormatter = icuNumberFormatter.notation(icu::number::Notation::engineering());
            break;
        default:
            break;
    }

    // 24. Let useGrouping be ? GetOption(options, "useGrouping", "boolean", undefined, true).
    property = globalConst->GetHandledUserGroupingString();
    bool useGrouping = false;
    [[maybe_unused]] bool find = JSLocale::GetOptionOfBool(thread, optionsObject, property, true, &useGrouping);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSTaggedValue> useGroupingValue(thread, JSTaggedValue(useGrouping));
    numberFormat->SetUseGrouping(thread, useGroupingValue);

    // 25. Set numberFormat.[[UseGrouping]] to useGrouping.
    if (!useGrouping) {
        icuNumberFormatter = icuNumberFormatter.grouping(UNumberGroupingStrategy::UNUM_GROUPING_OFF);
    }

    // 26. Let signDisplay be ?
    //  GetOption(options, "signDisplay", "string", « "auto", "never", "always", "exceptZero" », "auto").
    property = globalConst->GetHandledSignDisplayString();
    auto signDisplay = JSLocale::GetOptionOfString<SignDisplayOption>(
        thread, optionsObject, property,
        JSNumberFormat::SIGN_DISPLAY_OPTION, JSNumberFormat::SIGN_DISPLAY_OPTION_NAME,
        SignDisplayOption::AUTO);
    RETURN_IF_ABRUPT_COMPLETION(thread);
    numberFormat->SetSignDisplay(signDisplay);

    // 27. Set numberFormat.[[SignDisplay]] to signDisplay.
    // The default sign in ICU is UNUM_SIGN_AUTO which is mapped from
    // SignDisplay::AUTO and CurrencySign::STANDARD so we can skip setting
    // under that values for optimization.
    CurrencySignOption currencySign = numberFormat->GetCurrencySign();

    // Trans SignDisPlayOption to ICU UNumberSignDisplay and set to icuNumberFormatter
    switch (signDisplay) {
        case SignDisplayOption::AUTO:
            // if CurrencySign is ACCOUNTING, Use the locale-dependent accounting format on negative numbers
            if (currencySign == CurrencySignOption::ACCOUNTING) {
                icuNumberFormatter = icuNumberFormatter.sign(UNumberSignDisplay::UNUM_SIGN_ACCOUNTING);
            } else {
                icuNumberFormatter = icuNumberFormatter.sign(UNumberSignDisplay::UNUM_SIGN_AUTO);
            }
            break;
        case SignDisplayOption::NEVER:
            icuNumberFormatter = icuNumberFormatter.sign(UNumberSignDisplay::UNUM_SIGN_NEVER);
            break;
        case SignDisplayOption::ALWAYS:
            // if CurrencySign is ACCOUNTING, Use the locale-dependent accounting format on negative numbers
            if (currencySign == CurrencySignOption::ACCOUNTING) {
                icuNumberFormatter = icuNumberFormatter.sign(UNumberSignDisplay::UNUM_SIGN_ACCOUNTING_ALWAYS);
            } else {
                icuNumberFormatter = icuNumberFormatter.sign(UNumberSignDisplay::UNUM_SIGN_ALWAYS);
            }
            break;
        case SignDisplayOption::EXCEPTZERO:
            // if CurrencySign is ACCOUNTING, Use the locale-dependent accounting format on negative numbers
            if (currencySign == CurrencySignOption::ACCOUNTING) {
                icuNumberFormatter = icuNumberFormatter.sign(UNumberSignDisplay::UNUM_SIGN_ACCOUNTING_EXCEPT_ZERO);
            } else {
                icuNumberFormatter = icuNumberFormatter.sign(UNumberSignDisplay::UNUM_SIGN_EXCEPT_ZERO);
            }
            break;
        default:
            break;
    }

    if (forIcuCache) {
        std::string cacheEntry =
            locales->IsUndefined() ? "" : EcmaStringAccessor(locales.GetTaggedValue()).ToStdString(thread);
        auto formatterPointer = new icu::number::LocalizedNumberFormatter(icuNumberFormatter);
        ecmaVm->GetIntlCache().SetIcuFormatterToCache(IcuFormatterType::NUMBER_FORMATTER, cacheEntry,
            formatterPointer, JSNumberFormat::FreeIcuNumberformat);
    } else {
        // Set numberFormat.[[IcuNumberForma]] to handleNumberFormatter
        factory->NewJSIntlIcuData(numberFormat, icuNumberFormatter, JSNumberFormat::FreeIcuNumberformat);
    }
    // Set numberFormat.[[BoundFormat]] to undefinedValue
    numberFormat->SetBoundFormat(thread, undefinedValue);
}

// 12.1.3 CurrencyDigits ( currency )
int32_t JSNumberFormat::CurrencyDigits(const icu::UnicodeString &currency)
{
    UErrorCode status = U_ZERO_ERROR;
    // If the ISO 4217 currency and funds code list contains currency as an alphabetic code,
    // return the minor unit value corresponding to the currency from the list; otherwise, return 2.
    int32_t fractionDigits =
        ucurr_getDefaultFractionDigits(reinterpret_cast<const UChar *>(currency.getBuffer()), &status);
    if (U_SUCCESS(status)) {
        return fractionDigits;
    }
    return JSNumberFormat::DEFAULT_FRACTION_DIGITS;
}

icu::number::LocalizedNumberFormatter *JSNumberFormat::GetCachedIcuNumberFormatter(JSThread *thread,
    const JSHandle<JSTaggedValue> &locales)
{
    std::string cacheEntry =
        locales->IsUndefined() ? "" : EcmaStringAccessor(locales.GetTaggedValue()).ToStdString(thread);
    void *cachedNumberFormatter = thread->GetEcmaVM()->GetIntlCache().GetIcuFormatterFromCache(
        IcuFormatterType::NUMBER_FORMATTER, cacheEntry);
    if (cachedNumberFormatter) {
        return reinterpret_cast<icu::number::LocalizedNumberFormatter*>(cachedNumberFormatter);
    }
    return nullptr;
}

// 12.1.8 FormatNumeric( numberFormat, x )
JSHandle<JSTaggedValue> JSNumberFormat::FormatNumeric(JSThread *thread, const JSHandle<JSNumberFormat> &numberFormat,
                                                      JSTaggedValue x)
{
    icu::number::LocalizedNumberFormatter *icuNumberFormat = numberFormat->GetIcuCallTarget(thread);
    ASSERT(icuNumberFormat != nullptr);
    JSHandle<JSTaggedValue> res = FormatNumeric(thread, icuNumberFormat, x);
    return res;
}

JSHandle<JSTaggedValue> JSNumberFormat::FormatNumeric(JSThread *thread,
                                                      const icu::number::LocalizedNumberFormatter *icuNumberFormat,
                                                      JSTaggedValue x)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::number::FormattedNumber formattedNumber;
    if (x.IsBigInt()) {
        JSHandle<BigInt> bigint(thread, x);
        JSHandle<EcmaString> bigintStr = BigInt::ToString(thread, bigint);
        std::string stdString = EcmaStringAccessor(bigintStr).ToStdString(thread);
        formattedNumber = icuNumberFormat->formatDecimal(icu::StringPiece(stdString), status);
    } else {
        double number = x.GetNumber();
        formattedNumber = icuNumberFormat->formatDouble(number, status);
    }
    if (U_FAILURE(status)) {
        JSHandle<JSTaggedValue> errorResult(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "icu formatter format failed", errorResult);
    }
    icu::UnicodeString result = formattedNumber.toString(status);
    if (U_FAILURE(status)) {
        JSHandle<JSTaggedValue> errorResult(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "formatted number toString failed", errorResult);
    }
    JSHandle<EcmaString> stringValue = intl::LocaleHelper::UStringToString(thread, result);
    return JSHandle<JSTaggedValue>::Cast(stringValue);
}

void GroupToParts(JSThread *thread, const icu::number::FormattedNumber &formatted, const JSHandle<JSArray> &receiver,
                  const JSHandle<JSNumberFormat> &numberFormat, JSTaggedValue x)
{
    UErrorCode status = U_ZERO_ERROR;
    icu::UnicodeString formattedText = formatted.toString(status);
    if (U_FAILURE(status)) {  // NOLINT(readability-implicit-bool-conversion)
        THROW_TYPE_ERROR(thread, "formattedNumber toString failed");
    }
    ASSERT(x.IsNumber() || x.IsBigInt());

    StyleOption styleOption = numberFormat->GetStyle();

    icu::ConstrainedFieldPosition cfpo;
    // Set constrainCategory to UFIELD_CATEGORY_NUMBER which is specified for UNumberFormatFields
    cfpo.constrainCategory(UFIELD_CATEGORY_NUMBER);
    auto globalConst = thread->GlobalConstants();
    JSMutableHandle<JSTaggedValue> typeString(thread, JSTaggedValue::Undefined());
    int index = 0;
    int previousLimit = 0;
    /**
     * From ICU header file document @unumberformatter.h
     * Sets a constraint on the field category.
     *
     * When this instance of ConstrainedFieldPosition is passed to FormattedValue#nextPosition,
     * positions are skipped unless they have the given category.
     *
     * Any previously set constraints are cleared.
     *
     * For example, to loop over only the number-related fields:
     *
     *     ConstrainedFieldPosition cfpo;
     *     cfpo.constrainCategory(UFIELDCATEGORY_NUMBER_FORMAT);
     *     while (fmtval.nextPosition(cfpo, status)) {
     *         // handle the number-related field position
     *     }
     */
    bool lastFieldGroup = false;
    int groupLeapLength = 0;
    while (formatted.nextPosition(cfpo, status)) {
        int32_t fieldId = cfpo.getField();
        int32_t start = cfpo.getStart();
        int32_t limit = cfpo.getLimit();
        typeString.Update(globalConst->GetLiteralString());
        // If start greater than previousLimit, means a literal type exists before number fields
        // so add a literal type with value of formattedText.sub(0, start)
        // Special case when fieldId is UNUM_GROUPING_SEPARATOR_FIELD
        if (static_cast<UNumberFormatFields>(fieldId) == UNUM_GROUPING_SEPARATOR_FIELD) {
            JSHandle<EcmaString> substring =
                intl::LocaleHelper::UStringToString(thread, formattedText, previousLimit, start);
            typeString.Update(globalConst->GetIntegerString());
            JSLocale::PutElement(thread, index, receiver, typeString, JSHandle<JSTaggedValue>::Cast(substring));
            RETURN_IF_ABRUPT_COMPLETION(thread);
            index++;
            {
                typeString.Update(JSLocale::GetNumberFieldType(thread, x, fieldId).GetTaggedValue());
                substring = intl::LocaleHelper::UStringToString(thread, formattedText, start, limit);
                JSLocale::PutElement(thread, index, receiver, typeString, JSHandle<JSTaggedValue>::Cast(substring));
                RETURN_IF_ABRUPT_COMPLETION(thread);
                index++;
            }
            lastFieldGroup = true;
            groupLeapLength = start - previousLimit + 1;
            previousLimit = limit;
            continue;
        } else if (start > previousLimit) {
            JSHandle<EcmaString> substring =
                intl::LocaleHelper::UStringToString(thread, formattedText, previousLimit, start);
            JSLocale::PutElement(thread, index, receiver, typeString, JSHandle<JSTaggedValue>::Cast(substring));
            RETURN_IF_ABRUPT_COMPLETION(thread);
            index++;
        }
        if (lastFieldGroup) {
            start = start + groupLeapLength;
            lastFieldGroup = false;
        }
        // Special case in ICU when style is unit and unit is percent
        if (styleOption == StyleOption::UNIT && static_cast<UNumberFormatFields>(fieldId) == UNUM_PERCENT_FIELD) {
            typeString.Update(globalConst->GetUnitString());
        } else {
            typeString.Update(JSLocale::GetNumberFieldType(thread, x, fieldId).GetTaggedValue());
        }
        JSHandle<EcmaString> substring = intl::LocaleHelper::UStringToString(thread, formattedText, start, limit);
        JSLocale::PutElement(thread, index, receiver, typeString, JSHandle<JSTaggedValue>::Cast(substring));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        index++;
        previousLimit = limit;
    }
    // If iterated length is smaller than formattedText.length, means a literal type exists after number fields
    // so add a literal type with value of formattedText.sub(previousLimit, formattedText.length)
    if (formattedText.length() > previousLimit) {
        typeString.Update(globalConst->GetLiteralString());
        JSHandle<EcmaString> substring =
            intl::LocaleHelper::UStringToString(thread, formattedText, previousLimit, formattedText.length());
        JSLocale::PutElement(thread, index, receiver, typeString, JSHandle<JSTaggedValue>::Cast(substring));
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
}

// 12.1.9 FormatNumericToParts( numberFormat, x )
JSHandle<JSArray> JSNumberFormat::FormatNumericToParts(JSThread *thread, const JSHandle<JSNumberFormat> &numberFormat,
                                                       JSTaggedValue x)
{
    ASSERT(x.IsNumber() || x.IsBigInt());
    icu::number::LocalizedNumberFormatter *icuNumberFormatter = numberFormat->GetIcuCallTarget(thread);
    ASSERT(icuNumberFormatter != nullptr);

    UErrorCode status = U_ZERO_ERROR;
    icu::number::FormattedNumber formattedNumber;
    if (x.IsBigInt()) {
        JSHandle<BigInt> bigint(thread, x);
        JSHandle<EcmaString> bigintStr = BigInt::ToString(thread, bigint);
        std::string stdString = EcmaStringAccessor(bigintStr).ToStdString(thread);
        formattedNumber = icuNumberFormatter->formatDecimal(icu::StringPiece(stdString), status);
    } else {
        double number = x.GetNumber();
        formattedNumber = icuNumberFormatter->formatDouble(number, status);
    }
    if (U_FAILURE(status)) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSArray> emptyArray = factory->NewJSArray();
        THROW_RANGE_ERROR_AND_RETURN(thread, "icu formatter format failed", emptyArray);
    }

    JSHandle<JSTaggedValue> arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
    JSHandle<JSArray> result = JSHandle<JSArray>::Cast(arr);
    GroupToParts(thread, formattedNumber, result, numberFormat, x);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSArray, thread);
    return result;
}

// 12.1.12 UnwrapNumberFormat( nf )
JSHandle<JSTaggedValue> JSNumberFormat::UnwrapNumberFormat(JSThread *thread, const JSHandle<JSTaggedValue> &nf)
{
    // 1. Assert: Type(nf) is Object.
    ASSERT(nf->IsJSObject());

    // 2. If nf does not have an [[InitializedNumberFormat]] internal slot and ?
    //  InstanceofOperator(nf, %NumberFormat%) is true, then Let nf be ? Get(nf, %Intl%.[[FallbackSymbol]]).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    bool hasInstance = JSFunction::OrdinaryHasInstance(thread, env->GetNumberFormatFunction(), nf);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));

    bool isJSNumberFormat = nf->IsJSNumberFormat();
    // If nf does not have an [[InitializedNumberFormat]] internal slot and ?
    // InstanceofOperator(nf, %NumberFormat%) is true, then
    //      a. Let nf be ? Get(nf, %Intl%.[[FallbackSymbol]]).
    if (!isJSNumberFormat && hasInstance) {
        JSHandle<JSTaggedValue> key(thread, JSHandle<JSIntl>::Cast(env->GetIntlFunction())->GetFallbackSymbol(thread));
        OperationResult operationResult = JSTaggedValue::GetProperty(thread, nf, key);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
        return operationResult.GetValue();
    }
    // 3. Perform ? RequireInternalSlot(nf, [[InitializedNumberFormat]]).
    if (!isJSNumberFormat) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "this is not object",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    return nf;
}

JSHandle<TaggedArray> JSNumberFormat::GetAvailableLocales(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> numberFormatLocales = env->GetNumberFormatLocales();
    if (!numberFormatLocales->IsUndefined()) {
        return JSHandle<TaggedArray>::Cast(numberFormatLocales);
    }
    const char *key = "NumberElements";
    const char *path = nullptr;
    std::vector<std::string> availableStringLocales = intl::LocaleHelper::GetAvailableLocales(thread, key, path);
    JSHandle<TaggedArray> availableLocales = JSLocale::ConstructLocaleList(thread, availableStringLocales);
    env->SetNumberFormatLocales(thread, availableLocales);
    return availableLocales;
}

void JSNumberFormat::ResolvedOptions(JSThread *thread, const JSHandle<JSNumberFormat> &numberFormat,
                                     const JSHandle<JSObject> &options)
{
    // Table 5: Resolved Options of NumberFormat Instances
    // Internal Slot                    Property
    //    [[Locale]]                      "locale"
    //    [[NumberingSystem]]             "numberingSystem"
    //    [[Style]]                       "style"
    //    [[Currency]]                    "currency"
    //    [[CurrencyDisplay]]             "currencyDisplay"
    //    [[CurrencySign]]                "currencySign"
    //    [[Unit]]                        "unit"
    //    [[UnitDisplay]]                 "unitDisplay"
    //    [[MinimumIntegerDigits]]        "minimumIntegerDigits"
    //    [[MinimumFractionDigits]]       "minimumFractionDigits"
    //    [[MaximumFractionDigits]]       "maximumFractionDigits"
    //    [[MinimumSignificantDigits]]    "minimumSignificantDigits"
    //    [[MaximumSignificantDigits]]    "maximumSignificantDigits"
    //    [[UseGrouping]]                 "useGrouping"
    //    [[Notation]]                    "notation"
    //    [[CompactDisplay]]              "compactDisplay"
    //    [SignDisplay]]                  "signDisplay"
    // [[Locale]]
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> property = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> locale(thread, numberFormat->GetLocale(thread));
    JSObject::CreateDataPropertyOrThrow(thread, options, property, locale);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[NumberingSystem]]
    JSHandle<JSTaggedValue> numberingSystem(thread, numberFormat->GetNumberingSystem(thread));
    if (numberingSystem->IsUndefined()) {
        numberingSystem = globalConst->GetHandledLatnString();
    }
    property = globalConst->GetHandledNumberingSystemString();
    JSObject::CreateDataPropertyOrThrow(thread, options, property, numberingSystem);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[Style]]
    StyleOption style = numberFormat->GetStyle();
    property = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> styleString = OptionToEcmaString(thread, style);
    JSObject::CreateDataPropertyOrThrow(thread, options, property, styleString);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[currency]]
    JSHandle<JSTaggedValue> currency(thread, JSTaggedValue::Undefined());
    // If style is not currency the currency should be undefined
    if (style == StyleOption::CURRENCY) {
        currency = JSHandle<JSTaggedValue>(thread, numberFormat->GetCurrency(thread));
    }
    if (!currency->IsUndefined()) {  // NOLINT(readability-implicit-bool-conversion)
        property = globalConst->GetHandledCurrencyString();
        JSObject::CreateDataPropertyOrThrow(thread, options, property, currency);
        RETURN_IF_ABRUPT_COMPLETION(thread);

        // [[CurrencyDisplay]]
        property = globalConst->GetHandledCurrencyDisplayString();
        CurrencyDisplayOption currencyDisplay = numberFormat->GetCurrencyDisplay();
        JSHandle<JSTaggedValue> currencyDisplayString = OptionToEcmaString(thread, currencyDisplay);
        JSObject::CreateDataPropertyOrThrow(thread, options, property, currencyDisplayString);
        RETURN_IF_ABRUPT_COMPLETION(thread);

        // [[CurrencySign]]
        property = globalConst->GetHandledCurrencySignString();
        CurrencySignOption currencySign = numberFormat->GetCurrencySign();
        JSHandle<JSTaggedValue> currencySignString = OptionToEcmaString(thread, currencySign);
        JSObject::CreateDataPropertyOrThrow(thread, options, property, currencySignString);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }

    if (style == StyleOption::UNIT) {
        JSHandle<JSTaggedValue> unit(thread, numberFormat->GetUnit(thread));
        if (!unit->IsUndefined()) {
            // [[Unit]]
            property = globalConst->GetHandledUnitString();
            JSObject::CreateDataPropertyOrThrow(thread, options, property, unit);
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
        // [[UnitDisplay]]
        property = globalConst->GetHandledUnitDisplayString();
        UnitDisplayOption unitDisplay = numberFormat->GetUnitDisplay();
        JSHandle<JSTaggedValue> unitDisplayString = OptionToEcmaString(thread, unitDisplay);
        JSObject::CreateDataPropertyOrThrow(thread, options, property, unitDisplayString);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }
    // [[MinimumIntegerDigits]]
    property = globalConst->GetHandledMinimumIntegerDigitsString();
    JSHandle<JSTaggedValue> minimumIntegerDigits(thread, numberFormat->GetMinimumIntegerDigits(thread));
    JSObject::CreateDataPropertyOrThrow(thread, options, property, minimumIntegerDigits);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    RoundingType roundingType = numberFormat->GetRoundingType();
    if (roundingType == RoundingType::SIGNIFICANTDIGITS) {
        // [[MinimumSignificantDigits]]
        property = globalConst->GetHandledMinimumSignificantDigitsString();
        JSHandle<JSTaggedValue> minimumSignificantDigits(thread, numberFormat->GetMinimumSignificantDigits(thread));
        JSObject::CreateDataPropertyOrThrow(thread, options, property, minimumSignificantDigits);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        // [[MaximumSignificantDigits]]
        property = globalConst->GetHandledMaximumSignificantDigitsString();
        JSHandle<JSTaggedValue> maximumSignificantDigits(thread, numberFormat->GetMaximumSignificantDigits(thread));
        JSObject::CreateDataPropertyOrThrow(thread, options, property, maximumSignificantDigits);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    } else {
        // [[MinimumFractionDigits]]
        property = globalConst->GetHandledMinimumFractionDigitsString();
        JSHandle<JSTaggedValue> minimumFractionDigits(thread, numberFormat->GetMinimumFractionDigits(thread));
        JSObject::CreateDataPropertyOrThrow(thread, options, property, minimumFractionDigits);
        RETURN_IF_ABRUPT_COMPLETION(thread);
        // [[MaximumFractionDigits]]
        property = globalConst->GetHandledMaximumFractionDigitsString();
        JSHandle<JSTaggedValue> maximumFractionDigits(thread, numberFormat->GetMaximumFractionDigits(thread));
        JSObject::CreateDataPropertyOrThrow(thread, options, property, maximumFractionDigits);
        RETURN_IF_ABRUPT_COMPLETION(thread);

        // in v3, should contain BOTH significant and fraction digits
        if (roundingType == RoundingType::COMPACTROUNDING) {
            // [[MinimumSignificantDigits]]
            property = globalConst->GetHandledMinimumSignificantDigitsString();
            JSHandle<JSTaggedValue> minimumSignificantDigits(thread, numberFormat->GetMinimumSignificantDigits(thread));
            JSObject::CreateDataPropertyOrThrow(thread, options, property, minimumSignificantDigits);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            // [[MaximumSignificantDigits]]
            property = globalConst->GetHandledMaximumSignificantDigitsString();
            JSHandle<JSTaggedValue> maximumSignificantDigits(thread, numberFormat->GetMaximumSignificantDigits(thread));
            JSObject::CreateDataPropertyOrThrow(thread, options, property, maximumSignificantDigits);
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
    }

    // [[UseGrouping]]
    property = globalConst->GetHandledUserGroupingString();
    JSObject::CreateDataPropertyOrThrow(thread, options, property,
                                        JSHandle<JSTaggedValue>(thread, numberFormat->GetUseGrouping(thread)));
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // [[Notation]]
    property = globalConst->GetHandledNotationString();
    NotationOption notation = numberFormat->GetNotation();
    JSHandle<JSTaggedValue> notationString = OptionToEcmaString(thread, notation);
    JSObject::CreateDataPropertyOrThrow(thread, options, property, notationString);
    RETURN_IF_ABRUPT_COMPLETION(thread);

    // Only output compactDisplay when notation is compact.
    if (notation == NotationOption::COMPACT) {
        // [[CompactDisplay]]
        property = globalConst->GetHandledCompactDisplayString();
        CompactDisplayOption compactDisplay = numberFormat->GetCompactDisplay();
        JSHandle<JSTaggedValue> compactDisplayString = OptionToEcmaString(thread, compactDisplay);
        JSObject::CreateDataPropertyOrThrow(thread, options, property, compactDisplayString);
        RETURN_IF_ABRUPT_COMPLETION(thread);
    }

    // [[SignDisplay]]
    property = globalConst->GetHandledSignDisplayString();
    SignDisplayOption signDisplay = numberFormat->GetSignDisplay();
    JSHandle<JSTaggedValue> signDisplayString = OptionToEcmaString(thread, signDisplay);
    JSObject::CreateDataPropertyOrThrow(thread, options, property, signDisplayString);
    RETURN_IF_ABRUPT_COMPLETION(thread);
}
}  // namespace panda::ecmascript
