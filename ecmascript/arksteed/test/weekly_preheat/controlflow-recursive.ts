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
const NUM_SETIME_CONST: number = 1000;
const NUM_LOOP_COUNT_CONST: number = 80;
let inDebug: boolean = false;
function log(str: string): void {
  if (inDebug) {
    print(str);
  }
}
function currentTimestamp13(): number {
  return ArkTools.timeInUs() / NUM_SETIME_CONST;
}

function ack(m: number, n: number): number {
  if (m === 0) {
    return n + 1;
  }
  if (n === 0) {
    return ack(m - 1, 1);
  }
  return ack(m - 1, ack(m, n - 1));
}

function fib(n: number): number {
  let a: number = 2;
  if (n < a) {
    return 1;
  }
  return fib(n - a) + fib(n - 1);
}

function tak(x: number, y: number, z: number): number {
  if (y >= x) {
    return z;
  }
  return tak(tak(x - 1, y, z), tak(y - 1, z, x), tak(z - 1, x, y));
}
/*
 *  @State
 *  @Tags Jetstream2
 */
class Benchmark {
  /**
   * @Benchmark
   */
  run(): void {
    let result: number = 0;
    let a1: number = 5;
    let a2: number = 3;
    let b1: number = 17.0;
    let c1: number = 3;
    let c2: number = 2;
    let c3: number = 1;

    for (let i = 3; i <= a1; i++) {
      result += ack(a2, i);
      result += fib(b1 + i);
      result += tak(c1 * i + c1, c2 * i + c2, i + c3);
    }

    let expected: number = 57775;
    if (result !== expected) {
      throw new Error(`ERROR: bad result: expected ${expected} but got ${result}`);
    }
  }
}

function main() {
  let startTime = currentTimestamp13();
  let benchMark = new Benchmark();
  for (let i = 0; i < NUM_LOOP_COUNT_CONST; i++) {
    let startTimeInLoop = currentTimestamp13();
    benchMark.run();
    let endTimeInLoop = currentTimestamp13();
    //log(`controlflow-recursive: ms = ${endTimeInLoop - startTimeInLoop} i = ${i}`)
  }
  let endTime = currentTimestamp13();
  print(`controlflow-recursive: ms = ${endTime - startTime}`);
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
