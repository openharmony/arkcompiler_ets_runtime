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
const DECIMALISM_256: number = 0x100;
const LOOP_I_MAX_350: number = 350;
const LOOP_Y_MAX_256: number = 256;
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

let result = 0;

// 1 op = 2 assigns, 16 compare/branches, 8 ANDs, (0-8) ADDs, 8 SHLs
// O(n)
function bitSinByte(b: number): number {
  let m = 1;
  let c = 0;
  while (m < DECIMALISM_256) {
    if ((b & m) !== 0) {
      c += 1;
    }
    m <<= 1;
  }
  return c;
}

function timeFunc(): number {
  let sum = 0;
  for (let i = 0; i < LOOP_I_MAX_350; i++) {
    for (let y = 0; y < LOOP_Y_MAX_256; y++) {
      sum += bitSinByte(y);
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
    result = timeFunc();
    let expected = 358400;
    if (result !== expected) {
      throw new Error('ERROR: bad result: expected ' + expected + ' but got ' + result);
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
    //log("bitops_bits_in_byte: ms = " +  (endTimeInLoop - startTimeInLoop) + " i= " + i)
  }
  let endTime = currentTimestamp13();
  print('bitops-bits-in-byte: ms = ' + (endTime - startTime));
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
