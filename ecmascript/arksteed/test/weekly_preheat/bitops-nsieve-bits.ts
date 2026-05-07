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

const BASE_VALUE: number = 1000;
const LOOP_I_MAX_600000: number = 600000;
const LOOP_MAX_4: number = 4;
const NUMBER_31: number = 31;
const NUMBER_32: number = 32;
const NUMBER_1000: number = 1000;
const NUMBER_N100000: number = 100000;
const NUMBER_2147483647: number = 2147483647;
const NUMBER_N2147483648: number = -2147483648;
const BIT_BINARY_NUMBER_5: number = 5;
const BIT_BINARY_NUMBER_10000: number = 10000;
const EXECUTION_MAX_80: number = 80;

let inDebug = false;
function log(str: string): void {
  if (inDebug) {
    print(str);
  }
}
function currentTimestamp13(): number {
  return ArkTools.timeInUs() / BASE_VALUE;
}

function pad(number: number, width: number): string {
  let s = number.toString();
  let prefixWidth = width - s.length;
  if (prefixWidth > 0) {
    for (let i = 0; i < prefixWidth; i++) {
      s = ' ' + s;
    }
  }
  return s;
}

function primes(isPrime: number[], n: number): void {
  let count = 0;
  let m = BIT_BINARY_NUMBER_10000 << n;
  let size = (m + NUMBER_31) >> BIT_BINARY_NUMBER_5;
  for (let i = 0; i < size; i++) {
    isPrime[i] = 0xffffffff;
  }
  for (let i = 2; i < m; i++) {
    if ((isPrime[i >> BIT_BINARY_NUMBER_5] & (1 << i % NUMBER_32)) !== 0) {
      let j = i + i;
      while (j < m) {
        let tmp = transBigInt32(isPrime[j >> BIT_BINARY_NUMBER_5]);
        tmp &= transBigInt32(~(1 << (j & NUMBER_31)));
        isPrime[j >> BIT_BINARY_NUMBER_5] = tmp;
        j += i;
      }
      count += 1;
    }
  }
}

function sieve(): number[] {
  let isPrime = new Array<number>();
  for (let i = LOOP_MAX_4; i <= LOOP_MAX_4; i++) {
    isPrime = new Array<number>(((BIT_BINARY_NUMBER_10000 << i) + NUMBER_31) >> BIT_BINARY_NUMBER_5);
    primes(isPrime, i);
  }

  return isPrime;
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
    let result = sieve();
    let sum = 0;
    for (let i = 0; i < result.length; i++) {
      sum += result[i];
      if (inDebug && i % NUMBER_1000 === 0) {
        //log(`bitops-nsieve-bits: i = ${i} sum= ${sum}`)
      }
    }

    let expected = -1286749544853;
    if (sum !== expected) {
      throw new Error('ERROR: bad result: expected ' + expected + ' but got ' + sum);
    }
  }
}

function main() {
  let startTime = currentTimestamp13();
  let benchmark = new Benchmark();
  for (let i = 0; i < EXECUTION_MAX_80; i++) {
    let startTimeInLoop = currentTimestamp13();
    benchmark.run();
    let endTimeInLoop = currentTimestamp13();
    //log(`bitops-nsieve-bits: ms = ${endTimeInLoop - startTimeInLoop} i = ${i}`)
  }
  let endTime = currentTimestamp13();
  print(`bitops-nsieve-bits: ms = ${endTime - startTime}`);
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  main()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

main()

declare interface ArkTools {
  timeInUs(args: number): number;
}
function transBigInt32(bigInt32Num: number): number {
  let tmp = bigInt32Num;
  if (tmp > NUMBER_2147483647) {
    let max = 4294967295;
    tmp = tmp % (max + 1);
    if (tmp > NUMBER_2147483647) {
      tmp = tmp - max - 1;
    }
  } else if (tmp < NUMBER_N2147483648) {
    let max = 4294967295;
    tmp = tmp % (max + 1);
    if (tmp < NUMBER_N2147483648) {
      tmp = tmp + max + 1;
    }
  }
  return tmp;
}
