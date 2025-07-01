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

#include "ecmascript/intl/global_intl_helper.h"

#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/js_object.h"
#include "ecmascript/mem/c_string.h"

namespace panda::ecmascript::intl {
GlobalIntlHelper::GlobalIntlHelper(JSThread *thread,
    const GlobalFormatterType matterType)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    auto iter = optionMaps.find(matterType);
    if (iter == optionMaps.end()) {
        switch (matterType) {
            case GlobalFormatterType::Collator:
                InitCollatorData(globalConst);
                break;
            case GlobalFormatterType::SimpleDateFormatDate:
            case GlobalFormatterType::SimpleDateFormatTime:
            case GlobalFormatterType::DateFormatter:
                InitDateData(globalConst);
                break;
            case GlobalFormatterType::NumberFormatter:
                InitNumberData(globalConst);
                break;
        }
    }
}

uint64_t *GlobalIntlHelper::ConvertDateToUnit(uint64_t timestamp)
{
    const uint64_t baseYear = 1900;
    const uint64_t baseTimeSec = 60;
    const uint64_t baseTimeUTC = 8;
    const uint64_t baseTime = 1000;
    auto milli = timestamp + static_cast<uint64_t>(baseTimeUTC *
        baseTimeSec * baseTimeSec * baseTime);
    auto mTime = std::chrono::milliseconds(milli);
    auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(mTime);
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm *now = std::gmtime(&tt);
    dateUnitArr = new uint64_t[6] {
        static_cast<uint64_t>(now->tm_year + baseYear),
        static_cast<uint64_t>(now->tm_mon + 1),
        static_cast<uint64_t>(now->tm_mday),
        static_cast<uint64_t>(now->tm_hour),
        static_cast<uint64_t>(now->tm_min),
        static_cast<uint64_t>(now->tm_sec)
    };
    return dateUnitArr;
}

void GlobalIntlHelper::InitCollatorData(const GlobalEnvConstants *globalConst)
{
    std::map<std::string, JSHandle<JSTaggedValue>> collatorMap;
    collatorMap["localeMatcher"] = globalConst->GetHandledLocaleMatcherString();
    collatorMap["usage"] = globalConst->GetHandledUsageString();
    collatorMap["sensitivity"] = globalConst->GetHandledSensitivityString();
    collatorMap["ignorePunctuation"] = globalConst->GetHandledIgnorePunctuationString();
    collatorMap["numeric"] = globalConst->GetHandledNumericString();
    collatorMap["caseFirst"] = globalConst->GetHandledCaseFirstString();
    collatorMap["collation"] = globalConst->GetHandledCollationString();
    optionMaps.insert(make_pair(GlobalFormatterType::Collator, collatorMap));
}

void GlobalIntlHelper::InitDateData(const GlobalEnvConstants *globalConst)
{
    std::map<std::string, JSHandle<JSTaggedValue>> collatorMap;
    collatorMap["dateStyle"] = globalConst->GetHandledDateStyleString();
    collatorMap["timeStyle"] = globalConst->GetHandledTimeStyleString();
    collatorMap["calendar"] = globalConst->GetHandledCalendarString();
    collatorMap["numberingSystem"] = globalConst->GetHandledNumberingSystemString();
    collatorMap["localeMatcher"] = globalConst->GetHandledLocaleMatcherString();
    collatorMap["timeZone"] = globalConst->GetHandledTimeZoneString();
    collatorMap["hour12"] = globalConst->GetHandledHour12String();
    collatorMap["hourCycle"] = globalConst->GetHandledHourCycleString();
    collatorMap["formatMatcher"] = globalConst->GetHandledFormatMatcherString();
    collatorMap["weekday"] = globalConst->GetHandledWeekdayString();
    collatorMap["era"] = globalConst->GetHandledEraString();
    collatorMap["year"] = globalConst->GetHandledYearString();
    collatorMap["month"] = globalConst->GetHandledMonthString();
    collatorMap["day"] = globalConst->GetHandledDayString();
    collatorMap["hour"] = globalConst->GetHandledHourString();
    collatorMap["minute"] = globalConst->GetHandledMinuteString();
    collatorMap["second"] = globalConst->GetHandledSecondString();
    collatorMap["fractionalSecondDigits"] = globalConst->GetHandledFractionalSecondDigitsString();
    collatorMap["timeZoneName"] = globalConst->GetHandledTimeZoneNameString();
    optionMaps.insert(make_pair(GlobalFormatterType::DateFormatter, collatorMap));
}

void GlobalIntlHelper::InitNumberData(const GlobalEnvConstants *globalConst)
{
    std::map<std::string, JSHandle<JSTaggedValue>> collatorMap;
    collatorMap["localeMatcher"] = globalConst->GetHandledLocaleMatcherString();
    collatorMap["numberingSystem"] = globalConst->GetHandledNumberingSystemString();
    collatorMap["notation"] = globalConst->GetHandledNotationString();
    collatorMap["compactDisplay"] = globalConst->GetHandledCompactDisplayString();
    collatorMap["useGrouping"] = globalConst->GetHandledUserGroupingString();
    collatorMap["signDisplay"] = globalConst->GetHandledSignDisplayString();
    collatorMap["style"] = globalConst->GetHandledStyleString();
    collatorMap["currency"] = globalConst->GetHandledCurrencyString();
    collatorMap["currencySign"] = globalConst->GetHandledCurrencySignString();
    collatorMap["currencyDisplay"] = globalConst->GetHandledCurrencyDisplayString();
    collatorMap["unit"] = globalConst->GetHandledUnitString();
    collatorMap["unitDisplay"] = globalConst->GetHandledUnitDisplayString();
    collatorMap["minimumIntegerDigits"] = globalConst->GetHandledMinimumIntegerDigitsString();
    collatorMap["minimumFractionDigits"] = globalConst->GetHandledMinimumFractionDigitsString();
    collatorMap["maximumFractionDigits"] = globalConst->GetHandledMaximumFractionDigitsString();
    collatorMap["minimumSignificantDigits"] = globalConst->GetHandledMinimumSignificantDigitsString();
    collatorMap["maximumSignificantDigits"] = globalConst->GetHandledMaximumSignificantDigitsString();
    optionMaps.insert(make_pair(GlobalFormatterType::NumberFormatter, collatorMap));
}

std::map<std::string, std::string> GlobalIntlHelper::OptionsToMap(JSThread *thread,
    const JSHandle<JSTaggedValue> &options, GlobalFormatterType types)
{
    std::map<std::string, std::string> inputOptions;
    JSHandle<JSObject> optionsObject;
    if (options->IsUndefined()) {
        return inputOptions;
    } else {
        optionsObject = JSTaggedValue::ToObject(thread, options);
    }
    if (optionMaps.size() == 0) {
        LOG_ECMA(ERROR) << "GlobalIntlHelper::OptionsToMap size is zero";
        return inputOptions;
    }

    auto matterType = types;
    if (types == GlobalFormatterType::SimpleDateFormatDate ||
        types == GlobalFormatterType::SimpleDateFormatTime) {
        matterType = GlobalFormatterType::DateFormatter;
    }

    auto iter = optionMaps.find(matterType);
    if (iter != optionMaps.end()) {
        for (auto &opt : optionMaps[matterType]) {
            OperationResult operationResult = JSObject::GetProperty(thread, optionsObject, opt.second);
            if (!operationResult.GetValue()->IsUndefined()) {
                std::string valueStr = std::string(EcmaConvertToStr(JSTaggedValue::ToString(thread,
                    operationResult.GetValue())));
                auto inOpt = inputOptions.find(opt.first);
                if (inOpt != inputOptions.end()) {
                    inputOptions[opt.first] = valueStr;
                } else {
                    inputOptions.insert(make_pair(opt.first, valueStr));
                }
            }
        }
    }
    return OptionsWithDataFormatter(inputOptions, types);
}

std::map<std::string, std::string> GlobalIntlHelper::OptionsWithDataFormatter(std::map<std::string,
    std::string> &options, GlobalFormatterType &types)
{
    std::vector<std::string> all;
    if (types == GlobalFormatterType::DateFormatter) {
        all = {"year", "month", "day", "hour", "minute", "second"};
    }
    if (types == GlobalFormatterType::SimpleDateFormatDate) {
        all = {"year", "month", "day"};
    }
    if (types == GlobalFormatterType::SimpleDateFormatTime) {
        all = {"hour", "minute", "second"};
    }
    for (auto &item : all) {
        auto iter = options.find(item);
        if (iter == options.end()) {
            options[item] = "numeric";
        }
    }
    return options;
}

int64_t GlobalIntlHelper::DoubleToInt64(double value)
{
    return static_cast<int64_t>(round(value));
}

std::string GlobalIntlHelper::EcmaConvertToStr(const JSThread *thread, const JSHandle<EcmaString> &string)
{
    return std::string(ConvertToString(thread, *string, StringConvertedUsage::LOGICOPERATION));
}

std::vector<std::string> GlobalIntlHelper::LocalesToVector(JSThread *thread,
    const JSHandle<JSTaggedValue> &locales)
{
    JSHandle<TaggedArray> tArray = intl::LocaleHelper::CanonicalizeLocaleList(thread, locales);
    return TaggedArrayToVector(thread, tArray);
}

std::vector<std::string> GlobalIntlHelper::TaggedArrayToVector(JSThread *thread,
    JSHandle<TaggedArray> &taggedarray)
{
    std::vector<std::string> availableStringLocales;
    JSMutableHandle<EcmaString> availableItem(thread, JSTaggedValue::Undefined());
    uint32_t availablecalesLength = taggedarray->GetLength();
    for (uint32_t i = 0; i < availablecalesLength; i++) {
        availableItem.Update(taggedarray->Get(thread, i));
        availableStringLocales.emplace_back(intl::LocaleHelper::ConvertToStdString(thread, availableItem));
    }
    return availableStringLocales;
}
}  // panda::ecmascript::base
