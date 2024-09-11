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

#ifndef ECMASCRIPT_DATE_PARSE_H
#define ECMASCRIPT_DATE_PARSE_H

#include "ecmascript/js_date.h"

namespace panda::ecmascript {
class DateParse {
public:
    static const std::array<CString, MOUTH_PER_YEAR> MONTH_NAME;
    static bool ParseDateString(const char *str, int length, int *time);

private:
    static bool IsBetween(int n, int lower, int hign)
    {
        if (n < lower || n > hign) {
            return false;
        }
        return true;
    }
    class StringReader {
        public:
            explicit StringReader(const char *str, int length) : data_(str), length_(length)
            {
                NextChar();
            }

            void NextChar()
            {
                value_ = (index_ < length_) ? data_[index_] : DEL;
                index_++;
            }

            int GetIndex() const
            {
                return index_;
            }

            int ReadNumber(int *len)
            {
                int index = 0;
                int num = 0;
                while (IsDigit()) {
                    num = (value_ - '0') + num * JSDate::TEN;
                    index++;
                    const int maxDecimal = (std::numeric_limits<int>::max() - JSDate::NUM_NINE) / JSDate::TEN;
                    if (num > maxDecimal) {
                        break;
                    }
                    NextChar();
                }
                // read the followed digit
                while (IsDigit()) {
                    NextChar();
                }
                *len = index;
                return num;
            }

            int ReadAlphabet(char *word, int size)
            {
                int length = 0;
                for (; IsAlpha(); length++) {
                    if (length < size) {
                        word[length] = GetLower(value_);
                    }
                    NextChar();
                }
                return length;
            }

            char GetLower(char ch)
            {
                if (ch >= 'A' && ch <= 'Z') {
                    // 32: 'a' - 'A'
                    return ch + 32;
                }
                return ch;
            }

            bool IsDigit() const
            {
                if (value_ >= '0' && value_ <= '9') {
                    return true;
                }
                return false;
            }

            bool IsSign() const
            {
                if (value_ == '+' || value_ == '-') {
                    return true;
                }
                return false;
            }

            bool IsEnd() const
            {
                return value_ == DEL;
            }

            bool IsThisChar(char ch) const
            {
                return value_ == ch;
            }

            bool IsAlpha() const
            {
                if (value_ >= 'A' && value_ <= 'z') {
                    return true;
                }
                return false;
            }

            bool IsSpaceOrTab() const
            {
                if (value_ == ' ' || value_ == '\t') {
                    return true;
                }
                return false;
            }

            bool IsChar(char ch)
            {
                if (value_ != ch) {
                    return false;
                }
                return true;
            }
        private:
            const char *data_;
            int index_ {0};
            int length_;
            char value_;
    };

    enum DateValueType : int8_t {
        DATE_INVALID,
        DATE_UNKNOWN,
        DATE_NUMBER,
        DATE_SYMBOL,
        DATE_SPACE,
        DATE_STRING_END,
        DATE_TIME_ZONE,
        DATE_TIME_FALG,
        DATE_MONTH,
        DATE_INVALID_WORD,
        DATE_WORD_START = DATE_TIME_ZONE,
    };

    class DateUnit {
        public:
            bool IsInvalid() const
            {
                return type_ == DATE_INVALID;
            }

            bool IsUnknown() const
            {
                return type_ == DATE_UNKNOWN;
            }

            bool IsNumber() const
            {
                return type_ == DATE_NUMBER;
            }

            bool IsSymbol() const
            {
                return type_ == DATE_SYMBOL;
            }

            bool IsSymbol(char ch) const
            {
                return type_ == DATE_SYMBOL && static_cast<int>(ch) == value_;
            }

            bool IsStringEnd() const
            {
                return type_ == DATE_STRING_END;
            }

            bool IsTimeZone() const
            {
                return type_ == DATE_TIME_ZONE;
            }

            bool IsTimeFlag() const
            {
                return type_ == DATE_TIME_FALG;
            }

            bool IsInvalidWord() const
            {
                return type_ == DATE_INVALID_WORD;
            }

            bool IsMonth() const
            {
                return type_ == DATE_MONTH;
            }

            bool IsWord() const
            {
                return type_ >= DATE_TIME_ZONE;
            }

            bool IsSign() const
            {
                return type_ == DATE_SYMBOL && (value_ == '-' || value_ == '+');
            }

            bool IsSixDecimalDigit() const
            {
                // 6: 6 decimal digit
                return type_ == DATE_NUMBER && len_ == 6;
            }

            bool IsFourDecimalDigit() const
            {
                // 4: 4 decimal digit
                return type_ == DATE_NUMBER && len_ == 4;
            }

            bool IsTwoDecimalDigit() const
            {
                // 2: 2 decimal digit
                return type_ == DATE_NUMBER && len_ == 2;
            }

            bool IsWordZ() const
            {
                return type_ == DATE_TIME_ZONE && value_ == 0;
            }

            bool IsSpaceOrTab() const
            {
                return type_ == DATE_SPACE;
            }

            bool IsValidFinallyTime()
            {
                return IsStringEnd() || IsSign() || IsWordZ() || IsSpaceOrTab();
            }

            static DateUnit Number(int value, int len)
            {
                return DateUnit(DATE_NUMBER, value, len);
            }

            static DateUnit Symbol(char ch)
            {
                return DateUnit(DATE_SYMBOL, static_cast<int>(ch), 1);
            }

            static DateUnit Word(DateValueType type, int value, int len)
            {
                return DateUnit(type, value, len);
            }

            static DateUnit Space()
            {
                return DateUnit(DATE_SPACE, 0, 1);
            }

            static DateUnit StringEnd()
            {
                return DateUnit(DATE_STRING_END, 0, 0);
            }

            static DateUnit Invalid()
            {
                return DateUnit(DATE_INVALID, 0, 0);
            }

            static DateUnit Unknown()
            {
                return DateUnit(DATE_UNKNOWN, 0, 1);
            }

            int GetValue() const
            {
                return value_;
            }

            char GetSymbol() const
            {
                return static_cast<char>(value_);
            }

            DateValueType GetType() const
            {
                return type_;
            }

            uint32_t GetLength() const
            {
                return len_;
            }
        private:
            explicit DateUnit(DateValueType type, int value, int len) : type_(type), value_(value), len_(len) {}
            DateValueType type_;
            int value_;
            uint32_t len_;
    };

    class DateProxy {
        public:
            explicit DateProxy(StringReader *str) : str_(str), date_(Read()) {}
            DateUnit GetDate() const
            {
                return date_;
            }

            DateUnit NextDate()
            {
                DateUnit cur = GetDate();
                date_ = Read();
                return cur;
            }

            bool IgnoreSymbol(char ch)
            {
                if (!date_.IsSymbol(ch)) {
                    return false;
                }
                date_ = Read();
                return true;
            }
        private:
            DateUnit Read();
            DateValueType MatchKeyWord(const CString &str, int *value);

            StringReader *str_;
            DateUnit date_;
    };

    class TimeZone {
        public:
            void SetSign(int sign)
            {
                sign_ = sign;
            }

            void SetHour(int hour)
            {
                hour_ = hour;
            }

            void SetMin(int min)
            {
                min_ = min;
            }

            void SetUTC()
            {
                sign_ = 1;
                hour_ = 0;
                min_ = 0;
            }

            bool IsUTC()
            {
                return (hour_ == 0 && min_ == 0);
            }

            bool IsLocal() const
            {
                return hour_ == INT_MAX;
            }

            bool SetTimeZone(int *time);
        private:
            int sign_ {INT_MAX};
            int hour_ {INT_MAX};
            int min_ {INT_MAX};
    };

    class TimeValue {
        public:
            bool SetData(int data)
            {
                if (index_ < TIME_LEN) {
                    data_[index_] = data;
                    index_++;
                    return true;
                }
                return false;
            }

            static bool MinuteIsValid(int n)
            {
                // 59 : max min
                return IsBetween(n, 0, 59);
            }

            static bool SecondIsValid(int n)
            {
                // 59 : max sec
                return IsBetween(n, 0, 59);
            }

            static bool HourIsValid(int n)
            {
                // 24: max hour
                return IsBetween(n, 0, 24);
            }

            static bool MilliSecondIsValid(int n)
            {
                // 999 : max millisecond
                return IsBetween(n, 0, 999);
            }

            static int NormMilliSecond(DateUnit sec)
            {
                uint32_t len = sec.GetLength();
                int value = sec.GetValue();
                // 3: "sss" norm length
                if (len == 3) {
                    return value;
                }
                // 2: ms length
                if (len == 2) {
                    return value * JSDate::TEN;
                }
                if (len == 1) {
                    return value * JSDate::HUNDRED;
                }
                int divisor = 1;
                // 3: "sss" norm length
                while (len > 3) {
                    divisor *= JSDate::TEN;
                    len--;
                }
                return value / divisor;
            }

            int GetIndex() const
            {
                return index_;
            }

            bool IsValid(int n) const
            {
                // 2: index of second
                return (index_ == 1 && MinuteIsValid(n)) || (index_ == 2 && SecondIsValid(n));
            }

            bool IsValidSecond(int n) const
            {
                // 2: index of second
                return (index_ == 2 && SecondIsValid(n));
            }

            bool SetTimeValue(int *time);
        private:
            static constexpr int TIME_LEN = 4;
            int data_[TIME_LEN];
            int index_ {0};
    };

    class DayValue {
        public:
            bool SetData(int data)
            {
                if (index_ < DAY_LEN) {
                    data_[index_++] = data;
                    return true;
                }
                return false;
            }

            void SetIsoFlag(bool flag)
            {
                isIsoFlag_ = flag;
            }

            void SetMonth(int month)
            {
                month_ = month;
            }

            static bool MonthIsValid(int n)
            {
                return IsBetween(n, 1, MOUTH_PER_YEAR);
            }

            static bool DayIsValid(int n)
            {
                return IsBetween(n, 1, JSDate::MAX_DAYS_MONTH);
            }

            bool IsIso() const
            {
                return isIsoFlag_;
            }

            bool IsFull() const
            {
                return index_ == DAY_LEN;
            }

            int GetIndex() const
            {
                return index_;
            }

            bool SetDayValue(int *time);
        private:
            static constexpr int DAY_LEN = 3;
            int data_[DAY_LEN];
            int index_ {0};
            int month_ {INT_MAX};
            bool isIsoFlag_ {false};
    };
    static bool IsIsoDateTime(DateProxy *proxy, DayValue *dayValue);
    static bool ParseIsoDateTime(DateProxy *proxy, DayValue *dayValue, TimeValue *timeValue,
        TimeZone *timeZone);
    static bool ParseLegacyDates(DateProxy *proxy, DayValue *dayValue, TimeValue *timeValue,
        TimeZone *timeZone);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_DATE_PARSE_H
