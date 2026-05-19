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

const NUM_1000: number = 1000;
const NUM_2: number = 2;
const NUM_3: number = 3;
const NUM_4: number = 4;
const NUM_121: number = 121;
const NUM_501: number = 501;
const NUM_120: number = 120;
const NUM_500: number = 500;

class Test {
  arr = new Int32Array(NUM_1000);

  constructor(num1: number, num2: number) {
    for (let i = 0; i < NUM_1000; i++) {
      switch (i % NUM_4) {
        case 0:
          this.arr[i] = this.add(num1, num2);
          break;
        case 1:
          this.arr[i] = this.subtract(num1, num2);
          break;
        case NUM_2:
          this.arr[i] = this.multiply(num1, num2);
          break;
        case NUM_3:
          this.arr[i] = this.divide(num1, num2);
          break;
        default:
          break;
      }
    }
  }
  add(num1: number, num2: number): number {
    return num1 + num2;
  }
  subtract(num1: number, num2: number): number {
    return num1 - num2;
  }
  multiply(num1: number, num2: number): number {
    return num1 * num2;
  }
  divide(num1: number, num2: number): number {
    return num1 / num2;
  }
}

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  test?: Test;
  /*
   * @Benchmark
   */
  runIteration(): void {
    let startTime = ArkTools.timeInUs() / NUM_1000;
    for (let i = 1; i < NUM_121; i++) {
      for (let j = 1; j < NUM_501; j++) {
        let test = new Test(i, j);
        if (i === NUM_120 && j === NUM_500) {
          this.test = test;
        }
      }
    }
    let endTime = ArkTools.timeInUs() / NUM_1000;
    print(`code-first-load: ms = ${endTime - startTime}`);
    //deBugLog("对象创建完成");
    this.test!.arr[0] = 0;
  }
}
declare interface ArkTools {
  timeInUs(args: number): number;
}
let isDebug: boolean = false;
function deBugLog(msg: string): void {
  if (isDebug) {
    print(msg);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().runIteration();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().runIteration();