/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

const TIME_STAMP_SECONDS_CONST: number = 1167585071;
const TIME_STAMP_MILLISECONDS_CONST: number = 1167585071000;
const TIME_ADD_CONST: number = 84266956;
const TIME_ZONE_OFFSET_CONST: number = -480;
const YEAR_CONST: number = 2007;
const TIME_CONVERT_CONST: number = 60;
const TIME_HOUR_12_CONST: number = 12;
const YEAR_STARD_CONST: number = 100;
const YEAR_LEEP_CONST: number = 400;
const NUM_1_CONST: number = 1;
const NUM_2_CONST: number = 2;
const NUM_4_CONST: number = 4;
const NUM_3_CONST: number = 3;
const NUM_7_CONST: number = 7;
const NUM_11_CONST: number = 11;
const NUM_21_CONST: number = 21;
const NUM_22_CONST: number = 22;
const NUM_23_CONST: number = 23;
const NUM_28_CONST: number = 28;
const NUM_29_CONST: number = 29;
const NUM_30_CONST: number = 30;
const NUM_31_CONST: number = 31;
const NUM_500_CONST: number = 500;
const NUM_TIME_CONST: number = 1000;
const NUM_LOOPCOUNT_CONST: number = 80;

class MyDate {
  public timeIntervalSince1970(): number {
    return TIME_STAMP_SECONDS_CONST;
  }

  public timeStamp(): number {
    return TIME_STAMP_SECONDS_CONST;
  }

  public milliStamp(): number {
    return TIME_STAMP_MILLISECONDS_CONST;
  }

  public getDate(): number {
    //Returns a day of the month
    return NUM_1_CONST;
  }

  public getTimezoneOffset(): number {
    //Returns the time difference between Greenwich Mean Time and local time:
    return TIME_ZONE_OFFSET_CONST;
  }

  public getYear(): number {
    return YEAR_CONST;
  }

  public getFullYear(): number {
    //return a year
    return YEAR_CONST;
  }

  public getMonth(): number {
    //return a month
    return NUM_1_CONST;
  }

  public getHours(): number {
    //Returns the hour field of the time based on the specified time:
    return NUM_1_CONST;
  }

  public getMinutes(): number {
    //Returns the minute field of the time based on the specified time:
    return NUM_11_CONST;
  }

  public getSeconds(): number {
    //Returns the second field of the time based on the specified time:
    return NUM_11_CONST;
  }

  public getDay(): number {
    //Returns the number of a day of the week.
    return NUM_1_CONST;
  }

  public getTime(): number {
    //Returns the number of milliseconds between January 1, 1970:
    let timeInterval: number = this.timeIntervalSince1970();
    return timeInterval * NUM_TIME_CONST;
  }

  public setTime(millisedcond: number): void {
    //Method to set the Date object in milliseconds.
  }
}

class DateClass extends MyDate {
  daysInMonth = [
    NUM_31_CONST,
    NUM_28_CONST,
    NUM_31_CONST,
    NUM_30_CONST,
    NUM_31_CONST,
    NUM_30_CONST,
    NUM_31_CONST,
    NUM_31_CONST,
    NUM_30_CONST,
    NUM_31_CONST,
    NUM_30_CONST,
    NUM_31_CONST
  ];
  monthNames = ['January', 'February', 'March', 'April', 'May', 'June', 'July', 'August', 'September', 'October', 'November', 'December'];
  dayNames = ['Sunday', 'Monday', 'Tuesday', 'Wednesday', 'Thursday', 'Friday', 'Saturday'];

  constructor(str: string) {
    super();
  }

  dateFormat(format: string): string {
    return this.createNewFormat(format);
  }

  createNewFormat(format: string): string {
    let result = '';
    let special = false;
    let arr = this.stringToArray(format);
    for (let ch of arr) {
      if (!special && ch === '\\') {
        special = true;
      } else if (special) {
        special = false;
        result += ch;
      } else {
        let value = this.getFormatCode(ch);
        result += value;
      }
    }
    return result;
  }

  getFormatCode(character: string): string {
    switch (character) {
      case 'd':
        return this.leftPad(this.getDate(), NUM_2_CONST, '0');
      case 'D':
        return this.dayNames[this.getDay()].substring(0, NUM_3_CONST);
      case 'j':
        return `${this.getDate()}`;
      case 'l':
        return this.dayNames[this.getDay()];
      case 'S':
        return this.getSuffix();
      case 'w':
        return `${this.getDay()}`;
      case 'z':
        return `${this.getDayOfYear()}`;
      case 'W':
        return `${this.getWeekOfYear()}`;
      case 'F':
        return this.monthNames[this.getMonth() - 1];
      case 'm':
        return this.leftPad(this.getMonth(), NUM_2_CONST, '0');
      case 'M':
        return this.monthNames[this.getMonth()].substring(0, NUM_3_CONST);
      case 'n':
        return `${this.getMonth() + 1}`;
      case 't':
        return `${this.getDaysInMonth()}`;
      case 'L':
        return `${this.isLeapYear() ? 1 : 0}`;
      case 'Y':
        return `${this.getFullYear()}`;
      case 'y':
        return `${this.getFullYear()}`.substring(NUM_2_CONST, NUM_4_CONST);
      case 'a':
        return this.getHours() < TIME_HOUR_12_CONST ? 'am' : 'pm';
      case 'A':
        return this.getHours() < TIME_HOUR_12_CONST ? 'AM' : 'PM';
      case 'g':
        return `${this.getHours() % TIME_HOUR_12_CONST ? this.getHours() % TIME_HOUR_12_CONST : TIME_HOUR_12_CONST}`;
      case 'G':
        return `${this.getHours()}`;
      case 'h':
        return this.leftPad(this.getHours() % TIME_HOUR_12_CONST ? this.getHours() % TIME_HOUR_12_CONST : TIME_HOUR_12_CONST, NUM_2_CONST, '0');
      case 'H':
        return this.leftPad(this.getHours(), NUM_2_CONST, '0');
      case 'i':
        return this.leftPad(this.getMinutes(), NUM_2_CONST, '0');
      case 's':
        return this.leftPad(this.getSeconds(), NUM_2_CONST, '0');
      case 'O':
        return this.getGMTOffset();
      case 'Z':
        return `${this.getTimezoneOffset() * -TIME_CONVERT_CONST}`;
      default:
        return character;
    }
  }

  getGMTOffset(): string {
    return (
      (this.getTimezoneOffset() > 0 ? '-' : '+') +
      this.leftPad(Math.floor(this.getTimezoneOffset() / TIME_CONVERT_CONST), NUM_2_CONST, '0') +
      this.leftPad(this.getTimezoneOffset() % TIME_CONVERT_CONST, NUM_2_CONST, '0')
    );
  }

  getDayOfYear(): number {
    let num = 0;
    this.daysInMonth[1] = this.isLeapYear() ? NUM_29_CONST : NUM_28_CONST;
    for (let i = 0; i < this.getMonth(); ++i) {
      num += this.daysInMonth[i];
    }
    return num + this.getDate() - 1;
  }

  getWeekOfYear(): string {
    // Skip to Thursday of this week
    let now = this.getDayOfYear() + (NUM_4_CONST - this.getDay());
    // Find the first Thursday of the year
    let jan1 = new DateClass(`${this.getFullYear()},0,1`);
    let then = NUM_7_CONST - jan1.getDay() + NUM_4_CONST;
    return this.leftPad((now - then) / NUM_7_CONST + 1, NUM_2_CONST, '0');
  }

  isLeapYear(): Boolean {
    let year = this.getFullYear();
    return (year & NUM_3_CONST) === 0 && (year % YEAR_STARD_CONST !== 0 || (year % YEAR_LEEP_CONST === 0 && year !== 0));
  }

  getDaysInMonth(): number {
    this.daysInMonth[1] = this.isLeapYear() ? NUM_29_CONST : NUM_28_CONST;
    return this.daysInMonth[this.getMonth()];
  }

  getSuffix(): string {
    switch (this.getDate()) {
      case 1:
      case NUM_21_CONST:
      case NUM_31_CONST:
        return 'st';
      case NUM_2_CONST:
      case NUM_22_CONST:
        return 'nd';
      case NUM_3_CONST:
      case NUM_23_CONST:
        return 'rd';
      default:
        return 'th';
    }
  }

  leftPad(val: number, size: number, ch: string): string {
    let str = ch;
    let result = `${val}}`;
    if (str === null) {
      str = ' ';
    }
    while (result.length < size) {
      result = str + result;
    }
    return result;
  }

  stringToArray(str: String): string[] {
    const arr: string[] = [];
    for (let i = 0; i < str.length; i++) {
      arr.push(str[i]);
    }
    return arr;
  }
}

declare interface ArkTools {
  timeInUs(args: number): number;
}

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  /*
   * @Benchmark
   */
  run(): void {
    let date = new DateClass('1/1/2007 01:11:11');
    for (let i = 0; i < NUM_500_CONST; ++i) {
      let shortFormat: string = date.dateFormat('Y-m-d');
      let longFormat: string = date.dateFormat('l, F d, Y g:i:s A');
      date.setTime(date.getTime() + TIME_ADD_CONST);
    }
  }
  runIterationTime(): void {
    let start = ArkTools.timeInUs();
    for (let i = 0; i < NUM_LOOPCOUNT_CONST; i++) {
      this.run();
    }
    let end = ArkTools.timeInUs();
    let duration = (end - start) / NUM_TIME_CONST;
    print(`date-format-xparb: ms = ${duration}`);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().runIterationTime();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().runIterationTime();
