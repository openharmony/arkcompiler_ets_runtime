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

const NUM_TWO_CONST: number = 2;
const NUM_LOOP_CONST: number = 10;
const NUM_WHILE_CONST: number = 48;
const NUM_STIME_CONST: number = 1000;
const NUM_TIME_LOOP_CONST: number = 80;

function a(i: number, j: number): number {
  let result = ((i + j) * (i + j + 1)) / NUM_TWO_CONST + i + 1;
  return 1.0 / result;
}

function au(u: number[], v: number[]): void {
  for (let i = 0; i < u.length; ++i) {
    let t = 0;
    for (let j = 0; j < u.length; ++j) {
      t += a(i, j) * u[j];
    }
    v[i] = t;
  }
}

function atu(u: number[], v: number[]): void {
  for (let i = 0; i < u.length; ++i) {
    let t = 0;
    for (let j = 0; j < u.length; ++j) {
      t += a(j, i) * u[j];
    }
    v[i] = t;
  }
}

function atAu(u: number[], v: number[], w: number[]): void {
  au(u, w);
  atu(w, v);
}

function spectralnorm(n: number): number {
  let i: number;
  let u: number[] = [];
  let v: number[] = [];
  let w: number[] = [];
  let vv = 0;
  let vBv = 0;
  for (i = 0; i < n; ++i) {
    u.push(1.0);
    v.push(0.0);
    w.push(0.0);
  }
  for (i = 0; i < NUM_LOOP_CONST; ++i) {
    atAu(u, v, w);
    atAu(v, u, w);
  }
  for (i = 0; i < n; ++i) {
    vBv += u[i] * v[i];
    vv += v[i] * v[i];
  }
  return Math.sqrt(vBv / vv);
}
declare interface ArkTools {
  timeInUs(args: number): number;
}

/*
 *  @State
 *  @Tags Jetstream2
 */
class Benchmark {
  /*
   *  @Benchmark
   */
  run(): void {
    let total = 0;
    let n = 6;
    while (n <= NUM_WHILE_CONST) {
      total = total + spectralnorm(n);
      n = n * NUM_TWO_CONST;
    }

    let expected = 5.086694231303284;
    if (total !== expected) {
      print('ERROR: bad result: expected ' + expected + ' but got ' + total);
    }
  }
  /**
   * @Benchmark
   */
  runIterationTime(): void {
    let start = ArkTools.timeInUs() / NUM_STIME_CONST;
    for (let i = 0; i < NUM_TIME_LOOP_CONST; i++) {
      this.run();
    }
    let end = ArkTools.timeInUs() / NUM_STIME_CONST;
    print('math-spectral-norm: ms = ' + (end - start));
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
