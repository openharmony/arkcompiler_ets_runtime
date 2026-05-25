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

const NUM_FLOAT_TWO_CONST: number = 2.0;
const NUM_FLOAT_THREE_CONST: number = 3.0;
const NUM_NEGATIVE_CONST: number = -0.5;
const NUM_INT_TWO_CONST: number = 2;
const NUM_WHILE_LOOP_CONST: number = 16384;
const NUM_SETIME_CONST: number = 1000;
const NUM_LOOP_COUNT_CONST: number = 80;

function partial(n: number): number {
  let a1 = 0.0;
  let a2 = 0.0;
  let a3 = 0.0;
  let a4 = 0.0;
  let a5 = 0.0;
  let a6 = 0.0;
  let a7 = 0.0;
  let a8 = 0.0;
  let a9 = 0.0;
  let twothirds = NUM_FLOAT_TWO_CONST / NUM_FLOAT_THREE_CONST;
  let alt = -1.0;
  let k2 = 0.0;
  let k3 = 0.0;
  let sk = 0.0;
  let ck = 0.0;

  for (let m = 1; m <= n; m++) {
    k2 = m * m;
    k3 = k2 * m;
    sk = Math.sin(m);
    ck = Math.cos(m);
    alt = -alt;
    a1 += Math.pow(twothirds, m - 1);
    a2 += Math.pow(m, NUM_NEGATIVE_CONST);
    a3 += 1.0 / (m * (m + 1.0));
    a4 += 1.0 / (k3 * sk * sk);
    a5 += 1.0 / (k3 * ck * ck);
    a6 += 1.0 / m;
    a7 += 1.0 / k2;
    a8 += alt / m;
    a9 += alt / (NUM_INT_TWO_CONST * m - 1);
  }
  // NOTE: We don't try to validate anything from pow(),  sin() or cos() because those aren't
  // well-specified in ECMAScript;
  return a6 + a7 + a8 + a9;
}

declare interface ArkTools {
  timeInUs(args: number): number;
}
/*
 *  @State
 *  @Tags Jetstream2
 */
class Benchmark {
  run(): void {
    let total = 0;
    let n = 1024;
    while (n <= NUM_WHILE_LOOP_CONST) {
      total = total + partial(n);
      n = n * NUM_INT_TWO_CONST;
    }

    let expected = 60.08994194659945;
    if (total !== expected) {
      print('ERROR: bad result: expected ' + expected + ' but got ' + total);
    }
  }

  /**
   * @Benchmark
   */
  runIterationTime(): void {
    let start = ArkTools.timeInUs() / NUM_SETIME_CONST;
    for (let i = 0; i < NUM_LOOP_COUNT_CONST; i++) {
      this.run();
    }
    let end = ArkTools.timeInUs() / NUM_SETIME_CONST;
    print('math-partial-sums: ms = ' + (end - start));
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
