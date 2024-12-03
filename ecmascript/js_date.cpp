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

#include "ecmascript/js_date.h"

#include <ctime>
#include <regex>
#include <sys/time.h>

#include "ecmascript/date_parse.h"
#include "ecmascript/object_fast_operator-inl.h"
#include "ecmascript/platform/time.h"

namespace panda::ecmascript {
using NumberHelper = base::NumberHelper;
static const std::array<CString, WEEKDAY> WEEK_DAY_NAME = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const std::array<CString, MOUTH_PER_YEAR> MONTH_NAME  = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
void DateUtils::TransferTimeToDate(int64_t timeMs, std::array<int64_t, DATE_LENGTH> *date)
{
    (*date)[HOUR] = Mod(timeMs, MS_PER_DAY);                                 // ms from hour, minutes, second, ms
    (*date)[DAYS] = (timeMs - (*date)[HOUR]) / MS_PER_DAY;                   // days from year, month, day
    (*date)[MS] = (*date)[HOUR] % MS_PER_SECOND;                             // ms
    (*date)[HOUR] = ((*date)[HOUR] - (*date)[MS]) / MS_PER_SECOND;           // s from hour, minutes, second
    (*date)[SEC] = (*date)[HOUR] % SEC_PER_MINUTE;                           // second
    (*date)[HOUR] = ((*date)[HOUR] - (*date)[SEC]) / SEC_PER_MINUTE;         // min from hour, minutes
    (*date)[MIN] = (*date)[HOUR] % SEC_PER_MINUTE;                           // min
    (*date)[HOUR] = ((*date)[HOUR] - (*date)[MIN]) / SEC_PER_MINUTE;         // hour
    (*date)[WEEKDAY] = Mod(((*date)[DAYS] + LEAP_NUMBER[0]), DAY_PER_WEEK);  // weekday
    GetYearFromDays(date);
}
// static
bool DateUtils::IsLeap(int64_t year)
{
    return year % LEAP_NUMBER[0] == 0 && (year % LEAP_NUMBER[1] != 0 || year % LEAP_NUMBER[2] == 0);  // 2: means index
}

// static
int64_t DateUtils::GetDaysInYear(int64_t year)
{
    int64_t number;
    number = IsLeap(year) ? (DAYS_IN_YEAR + 1) : DAYS_IN_YEAR;
    return number;
}

// static
int64_t DateUtils::GetDaysFromYear(int64_t year)
{
    return DAYS_IN_YEAR * (year - YEAR_NUMBER[0]) + FloorDiv(year - YEAR_NUMBER[1], LEAP_NUMBER[0]) -
           FloorDiv(year - YEAR_NUMBER[2], LEAP_NUMBER[1]) +  // 2: year index
           FloorDiv(year - YEAR_NUMBER[3], LEAP_NUMBER[2]);   // 3, 2: year index
}

// static
int64_t DateUtils::FloorDiv(int64_t a, int64_t b)
{
    ASSERT(b != 0);
    int64_t m = a % b;
    int64_t res = m < 0 ? ((a - m - b) / b) : ((a - m) / b);
    return res;
}

// static
void DateUtils::GetYearFromDays(std::array<int64_t, DATE_LENGTH> *date)
{
    if (date == nullptr) {
        return;
    }
    if (isCached_) {
        int64_t t = (*date)[DAYS];
        int64_t newDays = preDays_ + (t - preSumDays_);
        if (newDays >= 1 && newDays < DAYS_FEBRUARY) {
            preSumDays_ = t;
            preDays_ = newDays;
            (*date)[DAYS] = newDays;
            (*date)[MONTH] = preMonth_;
            (*date)[YEAR] = preYear_;
            return;
        }
    }
    int64_t realDay;
    int64_t d = (*date)[DAYS];
    preSumDays_ = d;
    d += DAYS_1970_TO_0000;                                               // shift from 1970-01-01 to 0000-03-01
    int64_t era = (d >= 0 ? d : d - DAYS_IN_400_YEARS + 1) / DAYS_IN_400_YEARS;   // an era is a 400 year period
    int64_t doe = static_cast<int64_t>(d - era * DAYS_IN_400_YEARS);              // days of era
    int64_t yoe = (doe - doe / DAYS_IN_4_YEARS + doe / DAYS_IN_100_YEARS -
                   doe / (DAYS_IN_400_YEARS - 1)) / DAYS_IN_YEAR;                 // year of era
    int64_t y = static_cast<int64_t>(yoe) + era * LEAP_NUMBER[2];
    int64_t doy = doe - (DAYS_IN_YEAR * yoe + yoe / LEAP_NUMBER[0] -
                  yoe / LEAP_NUMBER[1]);                                          // days of year
    int64_t mp = (COEFFICIENT_TO_CIVIL[0] * doy + MONTH_COEFFICIENT) /
                  COEFFICIENT_TO_CIVIL[1];                                        // [0, 11] / [Mar,Feb] system
    int64_t month = mp + (mp < MONTH_TRANSFORM[1] ?
                MONTH_TRANSFORM[0] : MONTH_TRANSFORM[2]);                         // transform month to civil system
    int64_t year = y + (month <= MONTH_COEFFICIENT);
    month -= 1;
    realDay = doy - (COEFFICIENT_TO_CIVIL[1] * mp + 2) / COEFFICIENT_TO_CIVIL[0] + 1;   // 2: shift from 03-01 to 01-01
    (*date)[YEAR] = year;
    (*date)[MONTH] = month;
    (*date)[DAYS] = realDay;
    preDays_ = realDay;
    preMonth_ = month;
    preYear_ = year;
    isCached_ = true;
}

// static
int64_t DateUtils::Mod(int64_t a, int b)
{
    ASSERT(b != 0);
    int64_t m = a % b;
    int64_t res = m < 0 ? (m + b) : m;
    return res;
}

// static
// 20.4.1.11
double JSDate::MakeTime(double hour, double min, double sec, double ms)
{
    if (std::isfinite(hour) && std::isfinite(min) && std::isfinite(sec) && std::isfinite(ms)) {
        double hourInteger = NumberHelper::TruncateDouble(hour);
        double minInteger = NumberHelper::TruncateDouble(min);
        double secInteger = NumberHelper::TruncateDouble(sec);
        double msInteger = NumberHelper::TruncateDouble(ms);
        return hourInteger * MS_PER_HOUR + minInteger * MS_PER_MINUTE + secInteger * MS_PER_SECOND + msInteger;
    }
    return base::NAN_VALUE;
}

// static
// 20.4.1.12
double JSDate::MakeDay(double year, double month, double date)
{
    if (std::isfinite(year) && std::isfinite(month) && std::isfinite(date)) {
        double yearInteger = NumberHelper::TruncateDouble(year);
        double monthInteger = NumberHelper::TruncateDouble(month);
        int64_t y = static_cast<int64_t>(yearInteger) + static_cast<int64_t>(monthInteger / MOUTH_PER_YEAR);
        int64_t m = static_cast<int64_t>(monthInteger) % MOUTH_PER_YEAR;
        if (m < 0) {
            m += MOUTH_PER_YEAR;
            y -= 1;
        }

        int64_t days = DateUtils::GetDaysFromYear(y);
        int index = DateUtils::IsLeap(year) ? 1 : 0;
        days += DAYS_FROM_MONTH[index][m];
        return static_cast<double>(days - 1) + NumberHelper::TruncateDouble(date);
    }
    return base::NAN_VALUE;
}

// static
// 20.4.1.13
double JSDate::MakeDate(double day, double time)
{
    if (std::isfinite(day) && std::isfinite(time)) {
        return time + day * MS_PER_DAY;
    }
    return base::NAN_VALUE;
}

// static
// 20.4.1.14
double JSDate::TimeClip(double time)
{
    if (-MAX_TIME_IN_MS <= time && time <= MAX_TIME_IN_MS) {
        return NumberHelper::TruncateDouble(time);
    }
    return base::NAN_VALUE;
}

// 20.4.1.8
double JSDate::LocalTime(double timeMs) const
{
    return timeMs + GetLocalOffsetFromOS(timeMs, true);
}

// 20.4.1.9
double JSDate::UTCTime(double timeMs) const
{
    return timeMs - GetLocalOffsetFromOS(timeMs, false);
}

// static
int JSDate::GetSignedNumFromString(const CString &str, int len, int *index)
{
    int res = 0;
    GetNumFromString(str, len, index, &res);
    if (str.at(0) == NEG) {
        return -res;
    }
    return res;
}

// static
bool JSDate::GetNumFromString(const CString &str, int len, int *index, int *num)
{
    int indexStr = *index;
    char oneByte = 0;
    while (indexStr < len) {
        oneByte = str.at(indexStr);
        if (oneByte >= '0' && oneByte <= '9') {
            break;
        }
        indexStr++;
    }
    if (indexStr >= len) {
        return false;
    }
    int value = 0;
    while (indexStr < len) {
        oneByte = str.at(indexStr);
        int val = static_cast<int>(oneByte - '0');
        if (val >= 0 && val <= NUM_NINE) {
            value = value * TEN + val;
            indexStr++;
        } else {
            break;
        }
    }
    *num = value;
    *index = indexStr;
    return true;
}

// 20.4.1.7
int64_t JSDate::GetLocalOffsetInMin(const JSThread *thread, int64_t timeMs, bool isLocal)
{
    if (!isLocal) {
        return 0;
    }
    double localOffset = this->GetLocalOffset().GetDouble();
    if (localOffset == MAX_DOUBLE) {
        localOffset = static_cast<double>(GetLocalOffsetFromOS(timeMs, isLocal));
        SetLocalOffset(thread, JSTaggedValue(localOffset));
    }
    return localOffset;
}

// static
JSTaggedValue JSDate::LocalParseStringToMs(const CString &str)
{
    int year = 0;
    int month = 0;
    int date = 1;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms = 0;
    int index = 0;
    int len = static_cast<int>(str.length());
    bool isLocal = false;
    CString::size_type indexGmt;
    CString::size_type indexPlus = CString::npos;
    int localTime = 0;
    int localHours = 0;
    int localMinutes = 0;
    int64_t localMs = 0;
    CString::size_type localSpace;
    localSpace = str.find(' ', index);
    CString strMonth = str.substr(localSpace + 1, LENGTH_MONTH_NAME);
    for (int i = 0; i < MOUTH_PER_YEAR; i++) {
        if (strMonth == MONTH_NAME[i]) {
            month = i;
            break;
        }
    }
    index += (LENGTH_MONTH_NAME + 1);
    GetNumFromString(str, len, &index, &date);
    GetNumFromString(str, len, &index, &year);
    indexGmt = str.find("GMT", index);
    if (indexGmt == CString::npos) {
        GetNumFromString(str, len, &index, &hours);
        GetNumFromString(str, len, &index, &minutes);
        GetNumFromString(str, len, &index, &seconds);
        isLocal = true;
        localMs -= (GetLocalOffsetFromOS(localMs, true) * MS_PER_MINUTE);
    } else {
        indexPlus = str.find(PLUS, indexGmt);
        int indexLocal = static_cast<int>(indexGmt);
        GetNumFromString(str, indexGmt, &index, &hours);
        GetNumFromString(str, indexGmt, &index, &minutes);
        GetNumFromString(str, indexGmt, &index, &seconds);
        GetNumFromString(str, len, &indexLocal, &localTime);
        localHours = localTime / HUNDRED;
        localMinutes = localTime % HUNDRED;
        localMs = static_cast<int64_t>(MakeTime(localHours, localMinutes, 0, 0));
        if (indexPlus != CString::npos) {
            localMs = -localMs;
        }
    }
    double day = MakeDay(year, month, date);
    double time = MakeTime(hours, minutes, seconds, ms);
    double timeValue = TimeClip(MakeDate(day, time));
    if (std::isnan(timeValue)) {
        return JSTaggedValue(timeValue);
    }
    if (isLocal && timeValue < CHINA_1901_MS && (-localMs / MS_PER_MINUTE) == CHINA_AFTER_1901_MIN) {
        timeValue += static_cast<double>(localMs - CHINA_BEFORE_1901_MS);
    } else {
        timeValue += localMs;
    }
    return JSTaggedValue(timeValue);
}

// static
JSTaggedValue JSDate::UtcParseStringToMs(const CString &str)
{
    int year = 0;
    int month = 0;
    int date = 1;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms = 0;
    int index = 0;
    int len = static_cast<int>(str.length());
    CString::size_type indexGmt;
    CString::size_type indexPlus = CString::npos;
    int localTime = 0;
    int localHours = 0;
    int localMinutes = 0;
    int64_t localMs = 0;
    bool isLocal = false;
    GetNumFromString(str, len, &index, &date);
    CString strMonth = str.substr(index + 1, LENGTH_MONTH_NAME);
    for (int i = 0; i < MOUTH_PER_YEAR; i++) {
        if (strMonth == MONTH_NAME[i]) {
            month = i;
            break;
        }
    }
    index += (LENGTH_MONTH_NAME + 1);
    GetNumFromString(str, len, &index, &year);
    indexGmt = str.find("GMT", index);
    if (indexGmt == CString::npos) {
        GetNumFromString(str, len, &index, &hours);
        GetNumFromString(str, len, &index, &minutes);
        GetNumFromString(str, len, &index, &seconds);
        isLocal = true;
        localMs -= (GetLocalOffsetFromOS(localMs, true) * MS_PER_MINUTE);
    } else {
        indexPlus = str.find(PLUS, indexGmt);
        int indexLocal = static_cast<int>(indexGmt);
        GetNumFromString(str, indexGmt, &index, &hours);
        GetNumFromString(str, indexGmt, &index, &minutes);
        GetNumFromString(str, indexGmt, &index, &seconds);
        GetNumFromString(str, len, &indexLocal, &localTime);
        localHours = localTime / HUNDRED;
        localMinutes = localTime % HUNDRED;
        localMs = static_cast<int64_t>(MakeTime(localHours, localMinutes, 0, 0));
        if (indexPlus != CString::npos) {
            localMs = -localMs;
        }
    }
    double day = MakeDay(year, month, date);
    double time = MakeTime(hours, minutes, seconds, ms);
    double timeValue = TimeClip(MakeDate(day, time));
    if (std::isnan(timeValue)) {
        return JSTaggedValue(timeValue);
    }
    if (isLocal && timeValue < CHINA_1901_MS && (-localMs / MS_PER_MINUTE) == CHINA_AFTER_1901_MIN) {
        timeValue += static_cast<double>(localMs - CHINA_BEFORE_1901_MS);
    } else {
        timeValue += localMs;
    }
    return JSTaggedValue(timeValue);
}
// static
JSTaggedValue JSDate::IsoParseStringToMs(const CString &str)
{
    char flag = 0;
    int year;
    int month = 1;
    int date = 1;
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    int ms = 0;
    int index = 0;
    int len = static_cast<int>(str.length());
    year = GetSignedNumFromString(str, len, &index);
    CString::size_type indexT = str.find(FLAG_TIME, index);
    CString::size_type indexZ = str.find(FLAG_UTC, index);
    CString::size_type indexEndFlag = 0;
    int64_t localMs = 0;
    if (indexZ != CString::npos) {
        indexEndFlag = indexZ;
    } else if (len >= MIN_LENGTH && str.at(len - INDEX_PLUS_NEG) == NEG) {
        indexEndFlag = static_cast<CString::size_type>(len - INDEX_PLUS_NEG);
        flag = NEG;
    } else if (len >= MIN_LENGTH && str.at(len - INDEX_PLUS_NEG) == PLUS) {
        indexEndFlag = static_cast<CString::size_type>(len - INDEX_PLUS_NEG);
        flag = PLUS;
    }
    if (indexT != CString::npos) {
        if (static_cast<int>(indexT) - index == LENGTH_PER_TIME) {
            GetNumFromString(str, len, &index, &month);
        } else if (static_cast<int>(indexT) - index == (LENGTH_PER_TIME + LENGTH_PER_TIME)) {
            GetNumFromString(str, len, &index, &month);
            GetNumFromString(str, len, &index, &date);
        }
        GetNumFromString(str, len, &index, &hours);
        GetNumFromString(str, len, &index, &minutes);
        if (indexEndFlag > 0) {
            if (static_cast<int>(indexEndFlag) - index == LENGTH_PER_TIME) {
                GetNumFromString(str, len, &index, &seconds);
            } else if (static_cast<int>(indexEndFlag) - index == (LENGTH_PER_TIME + LENGTH_PER_TIME + 1)) {
                GetNumFromString(str, len, &index, &seconds);
                GetNumFromString(str, len, &index, &ms);
            }
        } else {
            if (len - index == LENGTH_PER_TIME) {
                GetNumFromString(str, len, &index, &seconds);
            } else if (len - index == (LENGTH_PER_TIME + LENGTH_PER_TIME + 1)) {
                GetNumFromString(str, len, &index, &seconds);
                GetNumFromString(str, len, &index, &ms);
            }
        }
    } else {
        GetNumFromString(str, len, &index, &month);
        GetNumFromString(str, len, &index, &date);
    }
    if (indexEndFlag > 0) {
        int localHours = 0;
        int localMinutes = 0;
        if (indexZ == CString::npos) {
            GetNumFromString(str, len, &index, &localHours);
            GetNumFromString(str, len, &index, &localMinutes);
            if (flag == PLUS) {
                localMs = static_cast<int64_t>(-MakeTime(localHours, localMinutes, 0, 0));
            } else {
                localMs = static_cast<int64_t>(MakeTime(localHours, localMinutes, 0, 0));
            }
        }
    }
    if (indexEndFlag == 0 && indexT != CString::npos) {
        localMs -= (GetLocalOffsetFromOS(localMs, true) * MS_PER_MINUTE);
    }

    double day = MakeDay(year, month - 1, date);
    double time = MakeTime(hours, minutes, seconds, ms);
    double timeValue = TimeClip(MakeDate(day, time));
    if (std::isnan(timeValue)) {
        return JSTaggedValue(timeValue);
    }
    if (flag == 0 && timeValue < CHINA_1901_MS && (-localMs / MS_PER_MINUTE) == CHINA_AFTER_1901_MIN) {
        timeValue += static_cast<double>(localMs - CHINA_BEFORE_1901_MS);
    } else {
        timeValue += localMs;
    }
    return JSTaggedValue(timeValue);
}

JSTaggedValue JSDate::GetTimeFromString(const char *str, int len)
{
    int time[TIMEZONE + 1];
    bool res = DateParse::ParseDateString(str, len, time);
    if (res) {
        double day = MakeDay(time[YEAR], time[MONTH], time[DAYS]);
        double dateTime = MakeTime(time[HOUR], time[MIN], time[SEC], time[MS]);
        double timeValue = TimeClip(MakeDate(day, dateTime));
        if (std::isnan(timeValue)) {
            return JSTaggedValue(timeValue);
        }
        int64_t localMs;
        if (time[TIMEZONE] == INT_MAX) {
            localMs = GetLocalOffsetFromOS(static_cast<int64_t>(timeValue), true) * MS_PER_MINUTE;
        } else {
            localMs = time[TIMEZONE] * MS_PER_SECOND;
        }
        timeValue -= localMs;
        return JSTaggedValue(timeValue);
    }
    return JSTaggedValue(base::NAN_VALUE);
}

// 20.4.3.2 static
JSTaggedValue JSDate::Parse(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> msg = base::BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<EcmaString> ecmaStr = JSTaggedValue::ToString(thread, msg);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    CVector<uint8_t> tmpBuf;
    EcmaStringAccessor strAccessor(const_cast<EcmaString *>(*ecmaStr));
    if (strAccessor.IsUtf16()) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    int len = static_cast<int>(strAccessor.GetLength());
    auto data = reinterpret_cast<const char *>(strAccessor.GetUtf8DataFlat(*ecmaStr, tmpBuf));
    return GetTimeFromString(data, len);
}

// 20.4.3.1
JSTaggedValue JSDate::Now()
{
    // time from now is in ms.
    int64_t ans;
    struct timeval tv {
    };
    gettimeofday(&tv, nullptr);
    ans = static_cast<int64_t>(tv.tv_sec) * MS_PER_SECOND + (tv.tv_usec / MS_PER_SECOND);
    return JSTaggedValue(static_cast<double>(ans));
}

// 20.4.4.2 static
JSTaggedValue JSDate::UTC(EcmaRuntimeCallInfo *argv)
{
    double year = 0.0;
    double month = 0.0;
    double date = 1.0;
    double hours = 0.0;
    double minutes = 0.0;
    double seconds = 0.0;
    double ms = 0.0;
    JSThread *thread = argv->GetThread();
    JSHandle<JSTaggedValue> yearArg = base::BuiltinsBase::GetCallArg(argv, 0);
    JSTaggedNumber yearValue = JSTaggedValue::ToNumber(thread, yearArg);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
    if (yearValue.IsNumber()) {
        year = yearValue.GetNumber();
        if (std::isfinite(year) && !yearValue.IsInt()) {
            year = NumberHelper::TruncateDouble(year);
        }
        if (year >= 0 && year <= (HUNDRED - 1)) {
            year = year + NINETEEN_HUNDRED_YEAR;
        }
    } else {
        year = base::NAN_VALUE;
    }
    uint32_t index = 1;
    uint32_t numArgs = argv->GetArgsNumber();
    JSTaggedValue res;
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        month = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        date = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        hours = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        minutes = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        seconds = res.GetNumber();
        index++;
    }
    if (numArgs > index) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, index);
        res = JSTaggedValue::ToNumber(thread, value);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());
        ms = res.GetNumber();
    }
    double day = MakeDay(year, month, date);
    double time = MakeTime(hours, minutes, seconds, ms);
    return JSTaggedValue(TimeClip(MakeDate(day, time)));
}

// 20.4.4.10
JSTaggedValue JSDate::GetTime() const
{
    return GetTimeValue();
}

// static
CString JSDate::StrToTargetLength(const CString &str, int length)
{
    int len = 0;
    CString sub;
    if (str[0] == NEG) {
        sub.reserve(length + 1);
        ASSERT(str.length() > 0);
        len = static_cast<int>(str.length() - 1);
        sub += NEG;
    } else {
        sub.reserve(length);
        len = static_cast<int>(str.length());
    }
    int dif = length - len;
    for (int i = 0; i < dif; i++) {
        sub += '0';
    }
    if (str[0] == NEG) {
        sub += str.substr(1, len);
    } else {
        sub += str;
    }
    return sub;
}

// static
void JSDate::AppendStrToTargetLength(const CString &str, int length, CString &target)
{
    int len = 0;
    if (str[0] == NEG) {
        len = static_cast<int>(str.length() - 1);
        target += NEG;
    } else {
        len = static_cast<int>(str.length());
    }
    int dif = length - len;
    for (int i = 0; i < dif; i++) {
        target += '0';
    }
    if (str[0] == NEG) {
        target += str.substr(1, len);
    } else {
        target += str;
    }
}

bool JSDate::GetThisDateValues(JSThread *thread, std::array<int64_t, DATE_LENGTH> *date, bool isLocal) const
{
    double timeMs = this->GetTimeValue().GetDouble();
    if (std::isnan(timeMs)) {
        return false;
    }
    GetDateValues(thread, timeMs, date, isLocal);
    return true;
}

// 20.4.4.35
JSTaggedValue JSDate::ToDateString(JSThread *thread) const
{
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(thread, &fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString str;
    str.reserve(DATE_STRING_LENGTH);
    str.append(WEEK_DAY_NAME[fields[WEEKDAY]]) // Append weekdy name
        .append(SPACE_STR) // Append SPACE
        .append(MONTH_NAME[fields[MONTH]]) // Append mouth name
        .append(SPACE_STR); // Append SPACE
    ConvertAndAppend(fields[DAYS], STR_LENGTH_OTHERS, str);
    str += SPACE;
    ConvertAndAppend(fields[YEAR], STR_LENGTH_YEAR, str);
    JSHandle<EcmaString> result = thread->GetEcmaVM()->GetFactory()->NewFromASCII(str);
    return result.GetTaggedValue();
}

// static
CString JSDate::ToDateString(JSThread *thread, double timeMs)
{
    if (std::isnan(timeMs)) {
        return "Invalid Date";
    }
    std::array<int64_t, DATE_LENGTH> fields = {0};
    GetDateValues(thread, timeMs, &fields, true);
    CString localTime;
    int localMin = 0;
    localMin = GetLocalOffsetFromOS(timeMs, true);
    if (localMin >= 0) {
        localTime += PLUS;
    } else {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    CString str;
    str.reserve(DATE_CSTRING_LENGTH);
    str.append(WEEK_DAY_NAME[fields[WEEKDAY]]) // Append weekday name
        .append(SPACE_STR)  // Append SPACE
        .append(MONTH_NAME[fields[MONTH]]) // Append mouth name
        .append(SPACE_STR); // Append SPACE
    ConvertAndAppend(fields[DAYS], STR_LENGTH_OTHERS, str);
    str += SPACE;
    ConvertAndAppend(fields[YEAR], STR_LENGTH_YEAR, str);
    str += SPACE;
    ConvertAndAppend(fields[HOUR], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[MIN], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[SEC], STR_LENGTH_OTHERS, str);
    str.append(SPACE_STR) // Append SPACE
        .append("GMT") // Append GMT
        .append(localTime); // Append localTime
    return str;
}
// 20.4.4.36
JSTaggedValue JSDate::ToISOString(JSThread *thread) const
{
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(thread, &fields, false)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString year = ToCString(fields[YEAR]);
    if (year[0] == NEG) {
        year = StrToTargetLength(year, STR_LENGTH_YEAR + STR_LENGTH_OTHERS);
    } else if (year.length() > STR_LENGTH_YEAR) {
        year = PLUS + StrToTargetLength(year, STR_LENGTH_YEAR + STR_LENGTH_OTHERS);
    } else {
        year = StrToTargetLength(year, STR_LENGTH_YEAR);
    }
    CString str;
    str.reserve(ISO_STRING_LENGTH);
    str.append(year) // Append year
        .append(NEG_STR); // Append NEG
    ConvertAndAppend(fields[MONTH] + 1, STR_LENGTH_OTHERS, str);
    str += NEG;
    ConvertAndAppend(fields[DAYS], STR_LENGTH_OTHERS, str);
    str += FLAG_TIME;
    ConvertAndAppend(fields[HOUR], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[MIN], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[SEC], STR_LENGTH_OTHERS, str);
    str += POINT;
    ConvertAndAppend(fields[MS], STR_LENGTH_OTHERS + 1, str);
    str += FLAG_UTC;
    return thread->GetEcmaVM()->GetFactory()->NewFromASCII(str).GetTaggedValue();
}

// 20.4.4.41
JSTaggedValue JSDate::ToString(JSThread *thread) const
{
    int localMin = 0;
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(thread, &fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString localTime;
    localMin = GetLocalOffsetFromOS(static_cast<int64_t>(this->GetTimeValue().GetDouble()), true);
    if (localMin >= 0) {
        localTime += PLUS;
    } else {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    CString str;
    str.reserve(TO_STRING_LENGTH);
    str.append(WEEK_DAY_NAME[fields[WEEKDAY]]) // Append weekday name
        .append(SPACE_STR) // Append SPACE
        .append(MONTH_NAME[fields[MONTH]]) // Append mouth name
        .append(SPACE_STR); // Append SPACE
    ConvertAndAppend(fields[DAYS], STR_LENGTH_OTHERS, str);
    str += SPACE;
    ConvertAndAppend(fields[YEAR], STR_LENGTH_YEAR, str);
    str += SPACE;
    ConvertAndAppend(fields[HOUR], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[MIN], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[SEC], STR_LENGTH_OTHERS, str);
    str.append(SPACE_STR) // Append SPACE
        .append("GMT") // Append GMT
        .append(localTime); // Append localTime
    return thread->GetEcmaVM()->GetFactory()->NewFromASCII(str).GetTaggedValue();
}

// 20.4.4.42
JSTaggedValue JSDate::ToTimeString(JSThread *thread) const
{
    int localMin = 0;
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(thread, &fields, true)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString localTime;
    localMin = GetLocalOffsetFromOS(static_cast<int64_t>(this->GetTimeValue().GetDouble()), true);
    if (localMin >= 0) {
        localTime += PLUS;
    } else {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    CString str;
    str.reserve(TIME_STRING_LENGTH);
    ConvertAndAppend(fields[HOUR], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[MIN], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[SEC], STR_LENGTH_OTHERS, str);
    str.append(SPACE_STR) // Append SPACE
        .append("GMT") // Append GMT
        .append(localTime); // Append localTime
    return thread->GetEcmaVM()->GetFactory()->NewFromASCII(str).GetTaggedValue();
}

// 20.4.4.43
JSTaggedValue JSDate::ToUTCString(JSThread *thread) const
{
    std::array<int64_t, DATE_LENGTH> fields = {0};
    if (!GetThisDateValues(thread, &fields, false)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    CString str;
    str.reserve(UTC_STRING_LENGTH);
    str.append(WEEK_DAY_NAME[fields[WEEKDAY]]) // Append weekday name
        .append(COMMA_STR) // Append COMMA
        .append(SPACE_STR); // Append SPACE
    ConvertAndAppend(fields[DAYS], STR_LENGTH_OTHERS, str);
    str.append(SPACE_STR) // Append SPACE
        .append(MONTH_NAME[fields[MONTH]]) // Append mouth name
        .append(SPACE_STR); // Append SPACE
    ConvertAndAppend(fields[YEAR], STR_LENGTH_YEAR, str);
    str += SPACE;
    ConvertAndAppend(fields[HOUR], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[MIN], STR_LENGTH_OTHERS, str);
    str += COLON;
    ConvertAndAppend(fields[SEC], STR_LENGTH_OTHERS, str);
    str.append(SPACE_STR) // Append SPACE
        .append("GMT"); // Append GMT
    return thread->GetEcmaVM()->GetFactory()->NewFromASCII(str).GetTaggedValue();
}

// 20.4.4.44
JSTaggedValue JSDate::ValueOf() const
{
    return this->GetTimeValue();
}

// static
void JSDate::GetDateValues(JSThread *thread, double timeMs, std::array<int64_t, DATE_LENGTH> *date, bool isLocal)
{
    int64_t tz = 0;
    int64_t timeMsInt = static_cast<int64_t>(timeMs);
    if (isLocal) {  // timezone offset
        tz = GetLocalOffsetFromOS(timeMsInt, isLocal);
        timeMsInt += tz * MS_PER_SECOND * SEC_PER_MINUTE;
    }

    thread->GetDateUtils()->TransferTimeToDate(timeMsInt, date);
    (*date)[TIMEZONE] = -tz;
}

double JSDate::GetDateValue(JSThread *thread, double timeMs, uint8_t code, bool isLocal) const
{
    if (std::isnan(timeMs)) {
        return base::NAN_VALUE;
    }
    std::array<int64_t, DATE_LENGTH> date = {0};
    GetDateValues(thread, timeMs, &date, isLocal);
    return static_cast<double>(date[code]);
}

JSTaggedValue JSDate::SetDateValue(EcmaRuntimeCallInfo *argv, uint32_t code, bool isLocal) const
{
    // get values from argv.
    uint32_t argc = argv->GetArgsNumber();
    if (argc == 0) {
        return JSTaggedValue(base::NAN_VALUE);
    }

    uint32_t firstValue = code & CODE_FLAG;
    uint32_t endValue = (code >> CODE_4_BIT) & CODE_FLAG;
    uint32_t count = endValue - firstValue;
    if (argc < count) {
        count = argc;
    }

    // get date values.
    double timeMs = this->GetTimeValue().GetDouble();
    std::array<int64_t, DATE_LENGTH> date = {0};
    bool isSelectLocal = isLocal;
    // setUTCFullYear, setFullYear
    if (std::isnan(timeMs) && (firstValue == 0)) {
        timeMs = 0.0;
        isSelectLocal = false;
    }
    JSThread *thread = argv->GetThread();
    if (!std::isnan(timeMs)) {
        GetDateValues(thread, timeMs, &date, isSelectLocal);
    }
    // When timeMs is NaN, the corresponding parameters still need to be obtained
    for (uint32_t i = 0; i < count; i++) {
        JSHandle<JSTaggedValue> value = base::BuiltinsBase::GetCallArg(argv, i);
        JSTaggedNumber res = JSTaggedValue::ToNumber(thread, value);
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        double temp = res.GetNumber();
        if (std::isnan(temp)) {
            return JSTaggedValue(base::NAN_VALUE);
        }
        date[firstValue + i] = NumberHelper::TruncateDouble(temp);
    }

    if (std::isnan(timeMs)) {
        return JSTaggedValue(base::NAN_VALUE);
    }
    // set date values.
    return JSTaggedValue(SetDateValues(&date, isLocal));
}

// static
double JSDate::SetDateValues(const std::array<int64_t, DATE_LENGTH> *date, bool isLocal)
{
    int64_t month = DateUtils::Mod((*date)[MONTH], MONTH_PER_YEAR);
    int64_t year = (*date)[YEAR] + ((*date)[MONTH] - month) / MONTH_PER_YEAR;
    int64_t days = DateUtils::GetDaysFromYear(year);
    int index = DateUtils::IsLeap(year) ? 1 : 0;
    days += DAYS_FROM_MONTH[index][month];

    days += (*date)[DAYS] - 1;
    int64_t millisecond =
        (((*date)[HOUR] * MIN_PER_HOUR + (*date)[MIN]) * SEC_PER_MINUTE + (*date)[SEC]) * MS_PER_SECOND + (*date)[MS];
    int64_t result = days * MS_PER_DAY + millisecond;
    if (isLocal) {
        int64_t offset = GetLocalOffsetFromOS(result, isLocal) * SEC_PER_MINUTE * MS_PER_SECOND;
        result -= offset;
    }
    return TimeClip(result);
}

double JSDate::SetDateValues(int64_t year, int64_t month, int64_t day)
{
    if (year >= 0 && year < HUNDRED) {
        year += NINETEEN_HUNDRED_YEAR;
    }
    int64_t m = DateUtils::Mod(month, MONTH_PER_YEAR);
    int64_t y = year + (month - m) / MONTH_PER_YEAR;
    int64_t d = DateUtils::GetDaysFromYear(y);
    int index = DateUtils::IsLeap(y) ? 1 : 0;
    d += DAYS_FROM_MONTH[index][m] + day - 1;
    int64_t result = d * MS_PER_DAY;

    int64_t offset = GetLocalOffsetFromOS(result, true) * SEC_PER_MINUTE * MS_PER_SECOND;
    result -= offset;
    return TimeClip(result);
}
}  // namespace panda::ecmascript
