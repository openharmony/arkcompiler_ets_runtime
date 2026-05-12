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
// import { ASSERT_NUMBER_EQ, ASSERT_FLOAT_EQ, ASSERT_TRUE, ASSERT_FALSE, ASSERT_EQ } from "../../../utils/assert";
// import { BenchmarkRunner } from "../../../utils/benchmarkTsSuite";
// declare function print(arg:string) : string;
declare interface ArkTools{
  timeInUs(arg:any):number
}
// Modify n value to change the difficulty of the benchmark
// suggesting using input = 12;
const input: number = 8;

function fannkuch(n : number): number {
  let perm: Int32Array = new Int32Array(n);
  let count: Int32Array = new Int32Array(n);
  let perm1: Int32Array = new Int32Array(n);
  for (let i = 0; i < n; i++) {
    perm[i] = 0;
    count[i] = 0;
    perm1[i] = 0;
  }
  for (let j = 0; j < (n - 1); j++) {
    perm1[j] = j;
  }

  let f: number = 0;
  let i: number = 0;
  let k: number = 0;
  let r: number = n;
  let flips: number = 0;
  let nperm: number = 0;
  let checksum: number = 0;

  while (r > 0) {
    i = 0;
    while(r != 1) {
      count[r-1] = r;
      r -= 1;
    }
    while (i < n) {
      perm[i] = perm1[i];
      i += 1;
    }

    // Count flips and update max and checksum
    f = 0;
    k = perm[0];
    while(k != 0) {
      i = 0;
      while (2*i < k) {
        const t = perm[i];
        perm[i] = perm[k-i];
        perm[k-i] = t;
        i += 1;
      }
      k = perm[0];
      f += 1;
    }
    if (f > flips) {
      flips = f;
    }
    if ((nperm & 0x1) == 0) {
      checksum += f;
    } else {
      checksum -= f;
    }

    // Use incremental change to generate another permutation
    let go = true;
    while (go) {
      if (r == n) {
        return flips;
      }
      const p0 = perm1[0];
      i = 0;
      while (i < r) {
        const j = i + 1;
        perm1[i] = perm1[j];
        i = j;
      }
      perm1[r] = p0;

      count[r] -= 1;
      if (count[r] > 0) {
        go = false;
      } else {
        r += 1;
      }
    }
    nperm += 1;
  }
  return flips;
}

export function RunFannkucRedux() {
  let n: number = input;
  let res = 0;
  let start = ArkTools.timeInUs();
  res = fannkuch(n);
  let end = ArkTools.timeInUs();
  // ASSERT_NUMBER_EQ(res, 16);
  let time = (end - start) / 1000
  print("Array Access - RunFannkucRedux:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunFannkucRedux()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunFannkucRedux()
// let runner = new BenchmarkRunner("Array Access - RunFannkucRedux", RunFannkucRedux);
// runner.run();
