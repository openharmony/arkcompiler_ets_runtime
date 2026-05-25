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

const LOOP_NUMBER = 40000;
const TIMES_KILO_MILLISECONDS = 1000;
const BENCHMARK_COUNT = 120;
const NUM_1 = 1;
const NUM_2 = 2;
const NUM_3 = 3;
const NUM_4 = 4;
const NUM_5 = 5;
const NUM_6 = 6;
const NUM_7 = 7;
declare interface ArkTools {
  timeInUs(args: number): number;
}
function deBugLog(str: string): void {
  print(str);
}

function run(): void {
  print('Hello world!');

  const m = LOOP_NUMBER;
  let j = 0;
  let strItems:string[] = new Array();
  function addStrElement(): void {
    while (j < m) {
      strItems.push('digit${j}');
      j = j + 1;
    }
  }
  addStrElement();

  const n = LOOP_NUMBER;
  let i = 0;
  let items:number[] = new Array();
  function addElement(): void {
    while (i < n) {
      items.push(i);
      i += 1;
    }
  }
  addElement();

  function firstWhere(list: number[], fn: (x: number) => boolean): number {
    for (const x of list) {
      if (fn(x)) {
        return x;
      }
    }
    return 0;
  }

  const nums: number[] = [NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7];
  function isEven(x: number): boolean {
    return (x & 1) === 0;
  }
  const firstEven = firstWhere(nums, isEven);
  
}

class Benchmark {
  /*
   * @Benchmark
   */
  runIteration(): void {
    const before = ArkTools.timeInUs() / TIMES_KILO_MILLISECONDS;
    for (let i = 0; i < BENCHMARK_COUNT; i++) {
      run();
    }
    const after = ArkTools.timeInUs() / TIMES_KILO_MILLISECONDS;
    print('LuaJSFight: ms = ' + (after - before));
  }
}


let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  let benchmark = new Benchmark();
  benchmark.runIteration();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

let benchmark = new Benchmark();
benchmark.runIteration();






