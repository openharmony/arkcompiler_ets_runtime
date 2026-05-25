/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
// declare function print(arg:string) : string;

declare interface ArkTools{
  timeInUs(arg:any):number
}

function ComplexIntegerNumeric() {
  let count: number = 5000000;
  let testArray: Int32Array = new Int32Array([1, 2, 5, 7, 9, 12, 14, 23, 32, 43, 67, 77, 89, 90, 101, 122, 125, 142, 153]); // GenerateSearchArrayInteger();
  let start: number = ArkTools.timeInUs();
  let res: number = testArray[2];
  let testArrayLength: number = testArray.length;
  for (let i: number = 1; i < count; i++) {
    let value: number = testArray[i % res & (testArrayLength - 1)];
    let low: number = 0;
    let tmp: number = 0;
    let high: number = testArrayLength - 1; // 1023
    let middle: number = high >>> 1;
    for (; low <= high; middle = (low + high) >>> 1) {
      const test: number = testArray[middle];
      if (test > value) {
        high = middle - 1;
      } else if (test < value) {
        low = middle + 1;
      } else {
        tmp = middle;
        break;
      }
    }
    res += tmp;
  }
  let end: number = ArkTools.timeInUs();
  print(""+res);
  let time = (end - start) / 1000
  print("Numerical Calculation - ComplexIntegerNumeric:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  ComplexIntegerNumeric()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

ComplexIntegerNumeric()

