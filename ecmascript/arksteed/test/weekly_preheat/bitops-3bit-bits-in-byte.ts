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
const BIT_BINARY_NUMBER_3: number = 3;
const NUMBER_14: number = 14;
const NUMBER_6: number = 6;
const BINARY_2: number = 2;
const BINARY_5: number = 5;
const LOOP_I_MAX_500: number = 500;
const LOOP_Y_MAX_256: number = 256;
const EXECUTION_MAX_NUMBER_80: number = 80;

let inDebug = false;
function log(str: string): void {
  if (inDebug) {
    // print(str)
  }
}

function currentTimestamp13(): number {
  return ArkTools.timeInUs() / BASE_VALUE;
}

let result = 0;

// 1 op = 6 ANDs, 3 SHRs, 3 SHLs, 4 assigns, 2 ADDs
// O(1)
function fast3bitLookup(b: number): number {
  let c1 = 0; // 0b1110 1001 1001 0100;
  let bi3b = 0xe994; // 3 2 2 1  2 1 1 0
  c1 = BIT_BINARY_NUMBER_3 & (bi3b >> ((b << 1) & NUMBER_14));
  c1 += BIT_BINARY_NUMBER_3 & (bi3b >> ((b >> BINARY_2) & NUMBER_14));
  c1 += BIT_BINARY_NUMBER_3 & (bi3b >> ((b >> BINARY_5) & NUMBER_6));
  return c1;

  /*
    lir4,0xE994; 9 instructions, no memory access, minimal register dependence, 6 shifts, 2 adds, 1 inline assign
    rlwinmr5,r3,1,28,30;
    rlwinmr6,r3,30,28,30;
    rlwinmr7,r3,27,29,30;
    rlwnmr8,r4,r5,30,31;
    rlwnmr9,r4,r6,30,31;
    rlwnmr10,r4,r7,30,31;
    addr3,r8,r9;
    addr3,r3,r10;
    */
}

function timeFunc(func1: (b: number) => number): number {
  let x = 0;
  let y = 0;
  let t = 0;
  let sum = 0;
  for (let i = 0; i < LOOP_I_MAX_500; i++) {
    for (let y = 0; y < LOOP_Y_MAX_256; y++) {
      sum += func1(y);
    }
  }
  return sum;
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
    let sum = timeFunc(fast3bitLookup);
    let expected = 512000;
    if (sum !== expected) {
      throw new Error('ERROR: bad result: expected ' + expected + ' but got ' + sum);
    }
  }
}

function main() {
  let startTime = currentTimestamp13();
  let benchmark = new Benchmark();
  for (let i = 0; i < EXECUTION_MAX_NUMBER_80; i++) {
    let startTimeInLoop = currentTimestamp13();
    benchmark.run();
    let endTimeInLoop = currentTimestamp13();
    //log("bitops_3bit_bits_in_byte: ms = " +  (endTimeInLoop - startTimeInLoop) + " i= " + i)
  }
  let endTime = currentTimestamp13();
  print('bitops-3bit-bits-in-byte: ms = ' + (endTime - startTime));
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
