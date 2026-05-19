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

const SIEVE_COMMON_NUMBER: number = 10000;
const CURRENT_LOOP_COUNT: number = 3;
const MAX_LOOP_COUNT: number = 80;
const MS_CONVERSION_RATIO: number = 1000;

function pad(number: number, width: number): string {
  let s = number.toString();
  let prefixWidth = width - s.length;
  if (prefixWidth > 0) {
    for (let i = 1; i <= prefixWidth; i++) {
      s = ' ' + s;
    }
  }
  return s;
}

function nsieve(m: number, isPrime: Int32Array): number {
  let count: number;

  for (let i = 2; i <= m; i++) {
    isPrime[i] = 1;
  }
  count = 0;

  for (let i = 2; i <= m; i++) {
    if (isPrime[i]) {
      for (let k = i + i; k <= m; k += i) {
        isPrime[k] = 0;
      }
      count++;
    }
  }
  return count;
}

function sieves(): number {
  let sum: number = 0;
  for (let i = 1; i <= CURRENT_LOOP_COUNT; i++) {
    let m: number = (1 << i) * SIEVE_COMMON_NUMBER;
    let flags = new Int32Array(m + 1).fill(0);
    sum += nsieve(m, flags);
  }
  return sum;
}

declare interface ArkTools {
  timeInUs(args: number): number;
}

/*
 *@State
 *@Tags Jetstream2
 */
class Benchmark {
  run(): void {
    let result = sieves();
    let expected: number = 14302;
    if (result !== expected) {
      print('ERROR: bad result: expected ' + expected + ' but got ' + result);
    }
  }

  /*
   * @Benchmark
   */
  runIterationTime(): void {
    let start: number = ArkTools.timeInUs();
    for (let i = 0; i < MAX_LOOP_COUNT; i++) {
      this.run();
    }
    let end: number = ArkTools.timeInUs();
    print('access-nsieve: ms = ' + (end - start) / MS_CONVERSION_RATIO);
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
