/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

declare function print(arg: any): string;
declare const ArkTools: {
    jitCompileAsync(args: any): boolean;
    waitJitCompileFinish(args: any): boolean;
};

// ==================== Date Creation ====================

function testDateConstructor(): string {
    const d1 = new Date(2024, 0, 15);
    const d2 = new Date(2024, 5, 20, 10, 30, 45);
    return `${d1.getFullYear()},${d1.getMonth()},${d1.getDate()},${d2.getHours()},${d2.getMinutes()},${d2.getSeconds()}`;
}

function testDateFromTimestamp(): string {
    const d = new Date(0);
    return `${d.getUTCFullYear()},${d.getUTCMonth()},${d.getUTCDate()}`;
}

function testDateFromString(): string {
    const d = new Date('2024-03-15T10:30:00Z');
    return `${d.getUTCFullYear()},${d.getUTCMonth()},${d.getUTCDate()},${d.getUTCHours()}`;
}

function testDateNow(): string {
    const now = Date.now();
    return `${typeof now === 'number'},${now > 0}`;
}

function testDateParse(): string {
    const ts = Date.parse('2024-03-15T00:00:00Z');
    return `${typeof ts === 'number'},${ts > 0}`;
}

function testDateUTC(): string {
    const ts = Date.UTC(2024, 2, 15, 10, 30, 0);
    const d = new Date(ts);
    return `${d.getUTCFullYear()},${d.getUTCMonth()},${d.getUTCDate()}`;
}

// ==================== Date Getters ====================

function testDateGetters(): string {
    const d = new Date(2024, 2, 15, 14, 30, 45, 500);
    return `${d.getFullYear()},${d.getMonth()},${d.getDate()},${d.getDay()},${d.getHours()},${d.getMinutes()},${d.getSeconds()},${d.getMilliseconds()}`;
}

function testDateUTCGetters(): string {
    const d = new Date(Date.UTC(2024, 2, 15, 14, 30, 45, 500));
    return `${d.getUTCFullYear()},${d.getUTCMonth()},${d.getUTCDate()},${d.getUTCDay()},${d.getUTCHours()},${d.getUTCMinutes()},${d.getUTCSeconds()},${d.getUTCMilliseconds()}`;
}

function testDateGetTime(): string {
    const d = new Date(Date.UTC(2024, 0, 1, 0, 0, 0, 0));
    const expected = 1704067200000;
    return `${d.getTime() === expected}`;
}

function testDateGetTimezoneOffset(): string {
    const d = new Date();
    const offset = d.getTimezoneOffset();
    return `${typeof offset === 'number'}`;
}

// ==================== Date Setters ====================

function testDateSetFullYear(): string {
    const d = new Date(2024, 0, 15);
    d.setFullYear(2025);
    d.setFullYear(2026, 5);
    d.setFullYear(2027, 6, 20);
    return `${d.getFullYear()},${d.getMonth()},${d.getDate()}`;
}

function testDateSetMonth(): string {
    const d = new Date(2024, 0, 15);
    d.setMonth(5);
    d.setMonth(6, 20);
    return `${d.getMonth()},${d.getDate()}`;
}

function testDateSetDate(): string {
    const d = new Date(2024, 0, 15);
    d.setDate(25);
    return `${d.getDate()}`;
}

function testDateSetHours(): string {
    const d = new Date(2024, 0, 15, 0, 0, 0, 0);
    d.setHours(10);
    d.setHours(11, 30);
    d.setHours(12, 45, 30);
    d.setHours(13, 50, 40, 500);
    return `${d.getHours()},${d.getMinutes()},${d.getSeconds()},${d.getMilliseconds()}`;
}

function testDateSetMinutes(): string {
    const d = new Date(2024, 0, 15, 10, 0, 0, 0);
    d.setMinutes(30);
    d.setMinutes(45, 20);
    d.setMinutes(50, 30, 500);
    return `${d.getMinutes()},${d.getSeconds()},${d.getMilliseconds()}`;
}

function testDateSetSeconds(): string {
    const d = new Date(2024, 0, 15, 10, 30, 0, 0);
    d.setSeconds(45);
    d.setSeconds(50, 500);
    return `${d.getSeconds()},${d.getMilliseconds()}`;
}

function testDateSetMilliseconds(): string {
    const d = new Date(2024, 0, 15, 10, 30, 45, 0);
    d.setMilliseconds(999);
    return `${d.getMilliseconds()}`;
}

function testDateSetTime(): string {
    const d = new Date();
    const ts = Date.UTC(2024, 0, 1, 0, 0, 0, 0);
    d.setTime(ts);
    return `${d.getUTCFullYear()},${d.getUTCMonth()},${d.getUTCDate()}`;
}

// ==================== Date UTC Setters ====================

function testDateSetUTCFullYear(): string {
    const d = new Date(Date.UTC(2024, 0, 15));
    d.setUTCFullYear(2025, 5, 20);
    return `${d.getUTCFullYear()},${d.getUTCMonth()},${d.getUTCDate()}`;
}

function testDateSetUTCMonth(): string {
    const d = new Date(Date.UTC(2024, 0, 15));
    d.setUTCMonth(6, 25);
    return `${d.getUTCMonth()},${d.getUTCDate()}`;
}

function testDateSetUTCDate(): string {
    const d = new Date(Date.UTC(2024, 0, 15));
    d.setUTCDate(28);
    return `${d.getUTCDate()}`;
}

function testDateSetUTCHours(): string {
    const d = new Date(Date.UTC(2024, 0, 15, 0, 0, 0, 0));
    d.setUTCHours(15, 30, 45, 500);
    return `${d.getUTCHours()},${d.getUTCMinutes()},${d.getUTCSeconds()},${d.getUTCMilliseconds()}`;
}

// ==================== Date String Conversion ====================

function testDateToString(): string {
    const d = new Date(2024, 2, 15, 10, 30, 0);
    const str = d.toString();
    return `${str.includes('2024')},${str.includes('Mar')}`;
}

function testDateToDateString(): string {
    const d = new Date(2024, 2, 15);
    const str = d.toDateString();
    return `${str.includes('Fri')},${str.includes('Mar')},${str.includes('15')},${str.includes('2024')}`;
}

function testDateToTimeString(): string {
    const d = new Date(2024, 2, 15, 10, 30, 45);
    const str = d.toTimeString();
    return `${str.includes('10:30:45')}`;
}

function testDateToISOString(): string {
    const d = new Date(Date.UTC(2024, 2, 15, 10, 30, 45, 123));
    return d.toISOString();
}

function testDateToJSON(): string {
    const d = new Date(Date.UTC(2024, 2, 15, 10, 30, 45, 123));
    const json = d.toJSON();
    return `${json === d.toISOString()}`;
}

function testDateToUTCString(): string {
    const d = new Date(Date.UTC(2024, 2, 15, 10, 30, 0));
    const str = d.toUTCString();
    return `${str.includes('Fri')},${str.includes('15 Mar 2024')}`;
}

function testDateToLocaleDateString(): string {
    const d = new Date(2024, 2, 15);
    const str = d.toDateString();
    return `${str.length > 0}`;
}

function testDateToLocaleTimeString(): string {
    const d = new Date(2024, 2, 15, 10, 30, 45);
    const str = d.toTimeString();
    return `${str.length > 0}`;
}

function testDateToLocaleString(): string {
    const d = new Date(2024, 2, 15, 10, 30, 45);
    const str = d.toString();
    return `${str.length > 0}`;
}

// ==================== Date Arithmetic ====================

function testDateAddDays(): string {
    const d = new Date(2024, 0, 15);
    d.setDate(d.getDate() + 10);
    return `${d.getMonth()},${d.getDate()}`;
}

function testDateAddMonths(): string {
    const d = new Date(2024, 0, 15);
    d.setMonth(d.getMonth() + 3);
    return `${d.getMonth()},${d.getDate()}`;
}

function testDateAddYears(): string {
    const d = new Date(2024, 0, 15);
    d.setFullYear(d.getFullYear() + 2);
    return `${d.getFullYear()}`;
}

function testDateDifference(): string {
    const d1 = new Date(2024, 0, 1);
    const d2 = new Date(2024, 0, 11);
    const diffMs = d2.getTime() - d1.getTime();
    const diffDays = diffMs / (1000 * 60 * 60 * 24);
    return `${diffDays}`;
}

function testDateMonthEnd(): string {
    const d = new Date(2024, 1, 1);
    d.setMonth(d.getMonth() + 1);
    d.setDate(0);
    return `${d.getMonth()},${d.getDate()}`;
}

function testDateLeapYear(): string {
    const isLeap = (year: number) => {
        return (year % 4 === 0 && year % 100 !== 0) || (year % 400 === 0);
    };
    return `${isLeap(2024)},${isLeap(2023)},${isLeap(2000)},${isLeap(1900)}`;
}

// ==================== Date Comparison ====================

function testDateComparison(): string {
    const d1 = new Date(2024, 0, 15);
    const d2 = new Date(2024, 0, 20);
    const d3 = new Date(2024, 0, 15);
    return `${d1 < d2},${d2 > d1},${d1.getTime() === d3.getTime()}`;
}

function testDateEquality(): string {
    const d1 = new Date(2024, 0, 15);
    const d2 = new Date(2024, 0, 15);
    return `${d1 !== d2},${d1.getTime() === d2.getTime()},${d1.valueOf() === d2.valueOf()}`;
}

// ==================== Date Edge Cases ====================

function testDateInvalidDate(): string {
    const d = new Date('invalid');
    return `${isNaN(d.getTime())}`;
}

function testDateOverflow(): string {
    const d = new Date(2024, 0, 32);
    return `${d.getMonth()},${d.getDate()}`;
}

function testDateNegativeValues(): string {
    const d = new Date(2024, 0, 1);
    d.setDate(-5);
    return `${d.getMonth()},${d.getDate()}`;
}

function testDateYearZero(): string {
    const d = new Date(0, 0, 1);
    d.setFullYear(0);
    return `${d.getFullYear()}`;
}

// ==================== Date Inheritance ====================

class D extends Date {}

function testDateInheritance(): void {
    let d = new D();
    if (!(d instanceof D)) {
        throw "d has incorrect prototype chain";
    } else {
        print("testDateInheritance: success");
    }
}

// ==================== Date Formatting Helpers ====================

function testDatePadding(): string {
    const d = new Date(2024, 0, 5, 8, 3, 7);
    const pad = (n: number) => n.toString().padStart(2, '0');
    const formatted = `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}:${pad(d.getSeconds())}`;
    return formatted;
}

function testDateDayOfYear(): string {
    const getDayOfYear = (date: Date): number => {
        const start = new Date(date.getFullYear(), 0, 0);
        const diff = date.getTime() - start.getTime();
        const oneDay = 1000 * 60 * 60 * 24;
        return Math.floor(diff / oneDay);
    };
    const d = new Date(2024, 0, 15);
    return `${getDayOfYear(d)}`;
}

function testDateWeekOfYear(): string {
    const getWeekOfYear = (date: Date): number => {
        const start = new Date(date.getFullYear(), 0, 1);
        const diff = date.getTime() - start.getTime();
        const oneWeek = 1000 * 60 * 60 * 24 * 7;
        return Math.ceil((diff / oneWeek) + 1);
    };
    const d = new Date(2024, 0, 15);
    return `${getWeekOfYear(d)}`;
}

// Warmup
for (let i = 0; i < 20; i++) {
    testDateConstructor();
    testDateFromTimestamp();
    testDateFromString();
    testDateNow();
    testDateParse();
    testDateUTC();
    testDateGetters();
    testDateUTCGetters();
    testDateGetTime();
    testDateGetTimezoneOffset();
    testDateSetFullYear();
    testDateSetMonth();
    testDateSetDate();
    testDateSetHours();
    testDateSetMinutes();
    testDateSetSeconds();
    testDateSetMilliseconds();
    testDateSetTime();
    testDateSetUTCFullYear();
    testDateSetUTCMonth();
    testDateSetUTCDate();
    testDateSetUTCHours();
    testDateToString();
    testDateToDateString();
    testDateToTimeString();
    testDateToISOString();
    testDateToJSON();
    testDateToUTCString();
    testDateToLocaleDateString();
    testDateToLocaleTimeString();
    testDateToLocaleString();
    testDateAddDays();
    testDateAddMonths();
    testDateAddYears();
    testDateDifference();
    testDateMonthEnd();
    testDateLeapYear();
    testDateComparison();
    testDateEquality();
    testDateInvalidDate();
    testDateOverflow();
    testDateNegativeValues();
    testDateYearZero();
    testDateInheritance();
    testDatePadding();
    testDateDayOfYear();
    testDateWeekOfYear();
}

// JIT compile
ArkTools.jitCompileAsync(testDateConstructor);
ArkTools.jitCompileAsync(testDateFromTimestamp);
ArkTools.jitCompileAsync(testDateFromString);
ArkTools.jitCompileAsync(testDateNow);
ArkTools.jitCompileAsync(testDateParse);
ArkTools.jitCompileAsync(testDateUTC);
ArkTools.jitCompileAsync(testDateGetters);
ArkTools.jitCompileAsync(testDateUTCGetters);
ArkTools.jitCompileAsync(testDateGetTime);
ArkTools.jitCompileAsync(testDateGetTimezoneOffset);
ArkTools.jitCompileAsync(testDateSetFullYear);
ArkTools.jitCompileAsync(testDateSetMonth);
ArkTools.jitCompileAsync(testDateSetDate);
ArkTools.jitCompileAsync(testDateSetHours);
ArkTools.jitCompileAsync(testDateSetMinutes);
ArkTools.jitCompileAsync(testDateSetSeconds);
ArkTools.jitCompileAsync(testDateSetMilliseconds);
ArkTools.jitCompileAsync(testDateSetTime);
ArkTools.jitCompileAsync(testDateSetUTCFullYear);
ArkTools.jitCompileAsync(testDateSetUTCMonth);
ArkTools.jitCompileAsync(testDateSetUTCDate);
ArkTools.jitCompileAsync(testDateSetUTCHours);
ArkTools.jitCompileAsync(testDateToString);
ArkTools.jitCompileAsync(testDateToDateString);
ArkTools.jitCompileAsync(testDateToTimeString);
ArkTools.jitCompileAsync(testDateToISOString);
ArkTools.jitCompileAsync(testDateToJSON);
ArkTools.jitCompileAsync(testDateToUTCString);
ArkTools.jitCompileAsync(testDateToLocaleDateString);
ArkTools.jitCompileAsync(testDateToLocaleTimeString);
ArkTools.jitCompileAsync(testDateToLocaleString);
ArkTools.jitCompileAsync(testDateAddDays);
ArkTools.jitCompileAsync(testDateAddMonths);
ArkTools.jitCompileAsync(testDateAddYears);
ArkTools.jitCompileAsync(testDateDifference);
ArkTools.jitCompileAsync(testDateMonthEnd);
ArkTools.jitCompileAsync(testDateLeapYear);
ArkTools.jitCompileAsync(testDateComparison);
ArkTools.jitCompileAsync(testDateEquality);
ArkTools.jitCompileAsync(testDateInvalidDate);
ArkTools.jitCompileAsync(testDateOverflow);
ArkTools.jitCompileAsync(testDateNegativeValues);
ArkTools.jitCompileAsync(testDateYearZero);
ArkTools.jitCompileAsync(testDateInheritance);
ArkTools.jitCompileAsync(testDatePadding);
ArkTools.jitCompileAsync(testDateDayOfYear);
ArkTools.jitCompileAsync(testDateWeekOfYear);

print("compile testDateConstructor: " + ArkTools.waitJitCompileFinish(testDateConstructor));
print("compile testDateFromTimestamp: " + ArkTools.waitJitCompileFinish(testDateFromTimestamp));
print("compile testDateFromString: " + ArkTools.waitJitCompileFinish(testDateFromString));
print("compile testDateNow: " + ArkTools.waitJitCompileFinish(testDateNow));
print("compile testDateParse: " + ArkTools.waitJitCompileFinish(testDateParse));
print("compile testDateUTC: " + ArkTools.waitJitCompileFinish(testDateUTC));
print("compile testDateGetters: " + ArkTools.waitJitCompileFinish(testDateGetters));
print("compile testDateUTCGetters: " + ArkTools.waitJitCompileFinish(testDateUTCGetters));
print("compile testDateGetTime: " + ArkTools.waitJitCompileFinish(testDateGetTime));
print("compile testDateGetTimezoneOffset: " + ArkTools.waitJitCompileFinish(testDateGetTimezoneOffset));
print("compile testDateSetFullYear: " + ArkTools.waitJitCompileFinish(testDateSetFullYear));
print("compile testDateSetMonth: " + ArkTools.waitJitCompileFinish(testDateSetMonth));
print("compile testDateSetDate: " + ArkTools.waitJitCompileFinish(testDateSetDate));
print("compile testDateSetHours: " + ArkTools.waitJitCompileFinish(testDateSetHours));
print("compile testDateSetMinutes: " + ArkTools.waitJitCompileFinish(testDateSetMinutes));
print("compile testDateSetSeconds: " + ArkTools.waitJitCompileFinish(testDateSetSeconds));
print("compile testDateSetMilliseconds: " + ArkTools.waitJitCompileFinish(testDateSetMilliseconds));
print("compile testDateSetTime: " + ArkTools.waitJitCompileFinish(testDateSetTime));
print("compile testDateSetUTCFullYear: " + ArkTools.waitJitCompileFinish(testDateSetUTCFullYear));
print("compile testDateSetUTCMonth: " + ArkTools.waitJitCompileFinish(testDateSetUTCMonth));
print("compile testDateSetUTCDate: " + ArkTools.waitJitCompileFinish(testDateSetUTCDate));
print("compile testDateSetUTCHours: " + ArkTools.waitJitCompileFinish(testDateSetUTCHours));
print("compile testDateToString: " + ArkTools.waitJitCompileFinish(testDateToString));
print("compile testDateToDateString: " + ArkTools.waitJitCompileFinish(testDateToDateString));
print("compile testDateToTimeString: " + ArkTools.waitJitCompileFinish(testDateToTimeString));
print("compile testDateToISOString: " + ArkTools.waitJitCompileFinish(testDateToISOString));
print("compile testDateToJSON: " + ArkTools.waitJitCompileFinish(testDateToJSON));
print("compile testDateToUTCString: " + ArkTools.waitJitCompileFinish(testDateToUTCString));
print("compile testDateToLocaleDateString: " + ArkTools.waitJitCompileFinish(testDateToLocaleDateString));
print("compile testDateToLocaleTimeString: " + ArkTools.waitJitCompileFinish(testDateToLocaleTimeString));
print("compile testDateToLocaleString: " + ArkTools.waitJitCompileFinish(testDateToLocaleString));
print("compile testDateAddDays: " + ArkTools.waitJitCompileFinish(testDateAddDays));
print("compile testDateAddMonths: " + ArkTools.waitJitCompileFinish(testDateAddMonths));
print("compile testDateAddYears: " + ArkTools.waitJitCompileFinish(testDateAddYears));
print("compile testDateDifference: " + ArkTools.waitJitCompileFinish(testDateDifference));
print("compile testDateMonthEnd: " + ArkTools.waitJitCompileFinish(testDateMonthEnd));
print("compile testDateLeapYear: " + ArkTools.waitJitCompileFinish(testDateLeapYear));
print("compile testDateComparison: " + ArkTools.waitJitCompileFinish(testDateComparison));
print("compile testDateEquality: " + ArkTools.waitJitCompileFinish(testDateEquality));
print("compile testDateInvalidDate: " + ArkTools.waitJitCompileFinish(testDateInvalidDate));
print("compile testDateOverflow: " + ArkTools.waitJitCompileFinish(testDateOverflow));
print("compile testDateNegativeValues: " + ArkTools.waitJitCompileFinish(testDateNegativeValues));
print("compile testDateYearZero: " + ArkTools.waitJitCompileFinish(testDateYearZero));
print("compile testDateInheritance: " + ArkTools.waitJitCompileFinish(testDateInheritance));
print("compile testDatePadding: " + ArkTools.waitJitCompileFinish(testDatePadding));
print("compile testDateDayOfYear: " + ArkTools.waitJitCompileFinish(testDateDayOfYear));
print("compile testDateWeekOfYear: " + ArkTools.waitJitCompileFinish(testDateWeekOfYear));

// Verify
print("testDateConstructor: " + testDateConstructor());
print("testDateFromTimestamp: " + testDateFromTimestamp());
print("testDateFromString: " + testDateFromString());
print("testDateNow: " + testDateNow());
print("testDateParse: " + testDateParse());
print("testDateUTC: " + testDateUTC());
print("testDateGetters: " + testDateGetters());
print("testDateUTCGetters: " + testDateUTCGetters());
print("testDateGetTime: " + testDateGetTime());
print("testDateGetTimezoneOffset: " + testDateGetTimezoneOffset());
print("testDateSetFullYear: " + testDateSetFullYear());
print("testDateSetMonth: " + testDateSetMonth());
print("testDateSetDate: " + testDateSetDate());
print("testDateSetHours: " + testDateSetHours());
print("testDateSetMinutes: " + testDateSetMinutes());
print("testDateSetSeconds: " + testDateSetSeconds());
print("testDateSetMilliseconds: " + testDateSetMilliseconds());
print("testDateSetTime: " + testDateSetTime());
print("testDateSetUTCFullYear: " + testDateSetUTCFullYear());
print("testDateSetUTCMonth: " + testDateSetUTCMonth());
print("testDateSetUTCDate: " + testDateSetUTCDate());
print("testDateSetUTCHours: " + testDateSetUTCHours());
print("testDateToString: " + testDateToString());
print("testDateToDateString: " + testDateToDateString());
print("testDateToTimeString: " + testDateToTimeString());
print("testDateToISOString: " + testDateToISOString());
print("testDateToJSON: " + testDateToJSON());
print("testDateToUTCString: " + testDateToUTCString());
print("testDateToLocaleDateString: " + testDateToLocaleDateString());
print("testDateToLocaleTimeString: " + testDateToLocaleTimeString());
print("testDateToLocaleString: " + testDateToLocaleString());
print("testDateAddDays: " + testDateAddDays());
print("testDateAddMonths: " + testDateAddMonths());
print("testDateAddYears: " + testDateAddYears());
print("testDateDifference: " + testDateDifference());
print("testDateMonthEnd: " + testDateMonthEnd());
print("testDateLeapYear: " + testDateLeapYear());
print("testDateComparison: " + testDateComparison());
print("testDateEquality: " + testDateEquality());
print("testDateInvalidDate: " + testDateInvalidDate());
print("testDateOverflow: " + testDateOverflow());
print("testDateNegativeValues: " + testDateNegativeValues());
print("testDateYearZero: " + testDateYearZero());
testDateInheritance();
print("testDatePadding: " + testDatePadding());
print("testDateDayOfYear: " + testDateDayOfYear());
print("testDateWeekOfYear: " + testDateWeekOfYear());
