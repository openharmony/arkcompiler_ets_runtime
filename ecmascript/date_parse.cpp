/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/date_parse.h"

namespace panda::ecmascript {
    const std::array<CString, MOUTH_PER_YEAR> DateParse::MONTH_NAME = {
    "jan", "feb", "mar", "apr", "may", "jun",
    "jul", "aug", "sep", "oct", "nov", "dec"
};
bool DateParse::ParseDateString(const char *str, int length, int *time)
{
    StringReader reader(str, length);
    DateProxy proxy(&reader);
    DayValue dayValue;
    TimeValue timeValue;
    TimeZone timeZone;
    bool isIso = IsIsoDateTime(&proxy, &dayValue);
    bool result;
    if (isIso) {
        result =  ParseIsoDateTime(&proxy, &dayValue, &timeValue, &timeZone);
    } else {
        result = ParseLegacyDates(&proxy, &dayValue, &timeValue, &timeZone);
    }
    if (result) {
        bool success = timeZone.SetTimeZone(time) && timeValue.SetTimeValue(time) && dayValue.SetDayValue(time);
        return success;
    } else {
        return false;
    }
}

bool DateParse::IsIsoDateTime(DateProxy *proxy, DayValue *dayValue)
{
    if (proxy->GetDate().IsSign()) {
        DateUnit sign = proxy->NextDate();
        if (!proxy->GetDate().IsSixDecimalDigit()) {
            return false;
        }
        int signYear = proxy->NextDate().GetValue();
        if (sign.IsSymbol('-') && signYear == 0) {
            return false;
        }
        if (sign.IsSymbol('-')) {
            signYear = -signYear;
        }
        dayValue->SetData(signYear);
    } else if (proxy->GetDate().IsFourDecimalDigit()) {
        int year = proxy->NextDate().GetValue();
        dayValue->SetData(year);
    } else {
        return false;
    }
    if (proxy->GetDate().IsSymbol('-')) {
        proxy->NextDate();
        DateUnit mon = proxy->GetDate();
        if (!mon.IsTwoDecimalDigit()) {
            return false;
        }
        dayValue->SetData(mon.GetValue());
        proxy->NextDate();
        if (proxy->GetDate().IsSymbol('-')) {
            proxy->NextDate();
            DateUnit day = proxy->GetDate();
            if (!day.IsTwoDecimalDigit()) {
                return false;
            }
            dayValue->SetData(day.GetValue());
            proxy->NextDate();
        }
    }
    if (!proxy->GetDate().IsTimeFlag()) {
        if (!proxy->GetDate().IsStringEnd()) {
            return false;
        }
    }
    return true;
}

bool DateParse::ParseIsoDateTime(DateProxy *proxy, DayValue *dayValue, TimeValue *timeValue, TimeZone *timeZone)
{
    if (proxy->GetDate().IsTimeFlag()) {
        // skip 'T'
        proxy->NextDate();
        DateUnit hour = proxy->GetDate();
        if (!hour.IsTwoDecimalDigit()) {
            return false;
        }
        timeValue->SetData(hour.GetValue());
        proxy->NextDate();
        if (!proxy->GetDate().IsSymbol(':')) {
            return false;
        }
        // skip ':'
        proxy->NextDate();
        DateUnit min = proxy->GetDate();
        if (!min.IsTwoDecimalDigit()) {
            return false;
        }
        timeValue->SetData(min.GetValue());
        proxy->NextDate();
        if (proxy->GetDate().IsSymbol(':')) {
            // skip ':'
            proxy->NextDate();
            DateUnit second = proxy->GetDate();
            if (!second.IsTwoDecimalDigit()) {
                return false;
            }
            timeValue->SetData(second.GetValue());
            proxy->NextDate();
            if (proxy->GetDate().IsSymbol('.')) {
                // skip '.'
                proxy->NextDate();
                DateUnit milliSec = proxy->GetDate();
                if (!milliSec.IsNumber()) {
                    return false;
                }
                timeValue->SetData(TimeValue::NormMilliSecond(milliSec));
                proxy->NextDate();
            }
        }
        // parse 'z' | '+' | '-' time zone
        if (proxy->GetDate().IsWordZ()) {
            timeZone->SetUTC();
            proxy->NextDate();
        } else if (proxy->GetDate().IsSign()) {
            if (proxy->GetDate().IsSymbol('-')) {
                timeZone->SetSign(-1);
            } else {
                timeZone->SetSign(1);
            }
            // skip '+' | '-'
            proxy->NextDate();
            DateUnit hourZone = proxy->GetDate();
            if (hourZone.IsTwoDecimalDigit()) {
                timeZone->SetHour(hourZone.GetValue());
                proxy->NextDate();
                if (!proxy->GetDate().IsSymbol(':')) {
                    return false;
                }
                proxy->NextDate();
                DateUnit minZone = proxy->GetDate();
                if (!minZone.IsTwoDecimalDigit()) {
                    return false;
                }
                timeZone->SetMin(minZone.GetValue());
                proxy->NextDate();
            } else if (hourZone.IsFourDecimalDigit()) {
                timeZone->SetHour(hourZone.GetValue() / JSDate::HUNDRED);
                timeZone->SetMin(hourZone.GetValue() % JSDate::HUNDRED);
                proxy->NextDate();
            } else {
                return false;
            }
        } else {
            if (!proxy->GetDate().IsStringEnd()) {
                return false;
            }
        }
    }
    if (timeZone->IsLocal() && timeValue->GetIndex() == 0) {
        timeZone->SetUTC();
    }
    dayValue->SetIsoFlag(true);
    return true;
}

bool DateParse::ParseLegacyDates(DateProxy *proxy, DayValue *dayValue, TimeValue *timeValue, TimeZone *timeZone)
{
    DateUnit date = proxy->NextDate();
    bool hasNumber = (dayValue->GetIndex() > 0);
    while (!date.IsStringEnd()) {
        if (date.IsNumber()) {
            hasNumber = true;
            int num = date.GetValue();
            // first parse as time "hh:" or "mm:"
            if (proxy->GetDate().IsSymbol(':')) {
                // skip ':'
                proxy->NextDate();
                if (!proxy->GetDate().IsNumber()) {
                    return false;
                }
                if (!timeValue->SetData(num)) {
                    return false;
                }
            // second parse as "ss.sss"
            } else if (proxy->GetDate().IsSymbol('.') && timeValue->IsValidSecond(num)) {
                // skip '.'
                proxy->NextDate();
                timeValue->SetData(num);
                DateUnit milliSec = proxy->GetDate();
                if (!milliSec.IsNumber()) {
                    return false;
                }
                timeValue->SetData(TimeValue::NormMilliSecond(milliSec));
                // ship "sss"
                proxy->NextDate();
                if (!proxy->GetDate().IsValidFinallyTime()) {
                    return false;
                }
            // then parse time "mm" or "ss"
            } else if (timeValue->IsValid(num)) {
                timeValue->SetData(num);
                if (!proxy->GetDate().IsValidFinallyTime()) {
                    return false;
                }
            } else {
                if (!dayValue->SetData(num)) {
                    return false;
                }
            }
        } else if (date.IsMonth()) {
            dayValue->SetMonth(date.GetValue());
        } else if (date.IsTimeZone() && hasNumber) {
            timeZone->SetUTC();
        } else if (date.IsTimeFlag() || date.IsInvalidWord()) {
            if (hasNumber) {
                return false;
            }
            if (proxy->GetDate().IsNumber()) {
                return false;
            }
        } else if (date.IsSign() && ((timeValue->GetIndex() > 0) || timeZone->IsUTC())) {
            if (date.IsSymbol('-')) {
                timeZone->SetSign(-1);
            } else {
                timeZone->SetSign(1);
            }
            DateUnit timeNumUnit = proxy->GetDate();
            if (!timeNumUnit.IsNumber()) {
                return false;
            }
            int timeNum = timeNumUnit.GetValue();
            uint32_t numLength = timeNumUnit.GetLength();
            proxy->NextDate();
            // parse +hh:mm
            if (proxy->GetDate().IsSymbol(':')) {
                // skip ':'
                proxy->NextDate();
                if (!proxy->GetDate().IsNumber()) {
                    return false;
                }
                timeZone->SetHour(timeNum);
                timeZone->SetMin(proxy->GetDate().GetValue());
                proxy->NextDate();
            // 2: hour length
            } else if (numLength == 1 || numLength == 2) {
                // parse GMT+hh
                timeZone->SetHour(timeNum);
                timeZone->SetMin(0);
            // 3,4:"GMT+hhmm" hhmm length
            } else if (numLength == 3 || numLength == 4) {
                // parse GMT+hhmm
                timeZone->SetHour(timeNum / JSDate::HUNDRED);
                timeZone->SetMin(timeNum % JSDate::HUNDRED);
            } else {
                return false;
            }
        }
        date = proxy->NextDate();
    }
    return true;
}

DateParse::DateUnit DateParse::DateProxy::Read()
{
    if (str_->IsDigit()) {
        int len = 0;
        int num = str_->ReadNumber(&len);
        return DateUnit::Number(num, len);
    }
    if (str_->IsEnd()) {
        return DateUnit::StringEnd();
    }
    if (str_->IsChar(':')) {
        str_->NextChar();
        return DateUnit::Symbol(':');
    }
    if (str_->IsChar('+')) {
        str_->NextChar();
        return DateUnit::Symbol('+');
    }
    if (str_->IsChar('-')) {
        str_->NextChar();
        return DateUnit::Symbol('-');
    }
    if (str_->IsChar('.')) {
        str_->NextChar();
        return DateUnit::Symbol('.');
    }
    if (str_->IsAlpha()) {
        // 3: month name length
        char buf[3] = {0};
        int len = str_->ReadAlphabet(buf, 3);
        int minLen = len < 3 ? len : 3;
        CString str(buf, minLen);
        int value = 0;
        DateValueType type = MatchKeyWord(str, &value);
        return DateUnit::Word(type, value, len);
    }
    if (str_->IsSpaceOrTab()) {
        str_->NextChar();
        return DateUnit::Space();
    }
    str_->NextChar();
    return DateUnit::Unknown();
}

DateParse::DateValueType DateParse::DateProxy::MatchKeyWord(const CString &str, int *value)
{
    if (str == "t") {
        return DATE_TIME_FALG;
    }
    if (str == "z") {
        return DATE_TIME_ZONE;
    }

    if (str == "utc" || str == "gmt") {
        *value = 1;
        return DATE_TIME_ZONE;
    }
    for (int i = 0; i < MOUTH_PER_YEAR; i++) {
        if (str == DateParse::MONTH_NAME[i]) {
            *value = i + 1;
            return DATE_MONTH;
        }
    }
    return DATE_INVALID_WORD;
}

bool DateParse::TimeZone::SetTimeZone(int *time)
{
    if (!IsLocal()) {
        if (!TimeValue::HourIsValid(hour_) || !TimeValue::MinuteIsValid(min_)) {
            return false;
        }
        time[TIMEZONE] = sign_ * (hour_ * SEC_PER_HOUR + min_ * SEC_PER_MINUTE);
    } else {
        time[TIMEZONE] = INT_MAX;
    }
    return true;
}

bool DateParse::TimeValue::SetTimeValue(int *time)
{
    for (int i = 0; i < TIME_LEN; i++) {
        if (i < index_) {
            time[HOUR + i] = data_[i];
        } else {
            time[HOUR + i] = 0;
        }
    }
    // 24: allow 24:00:00
    if (time[HOUR] == 24) {
        if (time[MIN] == 0 && time[SEC] == 0 && time[MS] == 0) {
            return true;
        }
        return false;
    }
    if (!HourIsValid(time[HOUR]) || !MinuteIsValid(time[MIN]) || !SecondIsValid(time[SEC]) ||
        !MilliSecondIsValid(time[MS])) {
        return false;
    }
    return true;
}

bool DateParse::DayValue::SetDayValue(int *time)
{
    if (index_ == 0) {
        return false;
    }
    int year = 1;
    int mon = 1;
    int day = 1;
    for (int i = index_; i < DAY_LEN; i++) {
        data_[i] = 1;
    }
    if (month_ == INT_MAX) {
        // 2:index of year
        if (isIsoFlag_ || (IsFull() && DayIsValid(data_[2]) && !MonthIsValid(data_[0]))) {
            year = data_[YEAR];
            mon = data_[MONTH];
            day = data_[DAYS];
        } else {
            if (IsFull()) {
                // input:month-day-year
                year = data_[2]; // 2:index of year
                mon = data_[0];  // 0:index of month
                day = data_[1];  // 1:index of day
            } else {
                // input:year-month
                year = data_[0]; // 0:index of year
                mon = data_[1];  // 1:index of month
                day = data_[2];  // 2:index of day
            }
        }
    } else {
        mon = month_;
        if (IsFull()) {
            return false;
        }
        // 2: day and year
        if (index_ == 2) {
            if (!DayIsValid(data_[0])) {
                year = data_[0];
                day = data_[1];
            } else {
                day = data_[0];
                year = data_[1];
            }
        } else {
            day = data_[0];
        }
    }
    if (!IsIso()) {
        if (IsBetween(year, 0, 49)) { // if year is between 0-49
            year += 2000; // 2000 : it means 2001-2049
        } else if (IsBetween(year, 50, 99)) { // if year is between 50-99
            year += 1900; // 1900 : it may means 1950-1999
        }
    }
    if (!MonthIsValid(mon) || !DayIsValid(day)) {
        return false;
    }
    time[YEAR] = year;
    time[MONTH] = mon - 1;
    time[DAYS] = day;
    return true;
}
}  // namespace panda::ecmascript