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

const CURRENT_LOOP_COUNT: number = 30;
const MAX_LOOP_COUNT: number = 80;
const MS_CONVERSION_RATIO: number = 1000;
function fannkuch(n: number): number {
  let check = 0;
  let perm = new Array(n).fill(0);
  let perm1 = new Array(n).fill(0);
  let count = new Array(n).fill(0);
  let maxPerm = new Array(n).fill(0);
  let maxFlipsCount = 0;
  let m = n - 1;

  for (let i = 0; i < n; i++) {
    perm1[i] = i;
  }
  let r = n;
  while (true) {
    if (check < CURRENT_LOOP_COUNT) {
      let s = '';
      for (let i = 0; i < n; i++) {
        s = s + `${perm1[i] + 1}`;
        check += 1;
      }
    }
    while (r !== 1) {
      count[r - 1] = r;
      r -= 1;
    }

    if (!(perm1[0] === 0 || perm1[m] === m)) {
      for (let i = 0; i < n; i++) {
        perm[i] = perm1[i];
      }

      let flipsCount = 0;
      let k = 0;

      while (!(perm[0] === 0)) {
        k = perm[0];
        let k2 = (k + 1) >> 1;
        for (let i = 0; i < k2; i++) {
          let temp = perm[i];
          perm[i] = perm[k - i];
          perm[k - i] = temp;
        }
        flipsCount += 1;
      }

      if (flipsCount > maxFlipsCount) {
        maxFlipsCount = flipsCount;
        for (let i = 0; i < n; i++) {
          maxPerm[i] = perm1[i];
        }
      }
    }

    while (true) {
      if (r === n) {
        return maxFlipsCount;
      }
      let perm0 = perm1[0];
      let i = 0;
      while (i < r) {
        let j = i + 1;
        perm1[i] = perm1[j];
        i = j;
      }
      perm1[r] = perm0;

      count[r] = count[r] - 1;
      if (count[r] > 0) {
        break;
      }
      r += 1;
    }
  }
}

declare interface ArkTools {
  timeInUs(args: number): number;
}

/*
 *@State
 * @Tags Jetstream2
 */
class Benchmark {
  run(): void {
    let n = 8;
    let ret = fannkuch(n);
    let expected = 22;
    if (ret !== expected) {
      print(`ERROR: bad result: expected ${expected} but got ${ret}`);
    }
  }

  /*
   * @Benchmark
   */
  runIterationTime(): void {
    let start = ArkTools.timeInUs();
    for (let i = 0; i < MAX_LOOP_COUNT; i++) {
      this.run();
    }
    let end = ArkTools.timeInUs();
    print('access-fannkuch: ms = ' + (end - start) / MS_CONVERSION_RATIO);
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
