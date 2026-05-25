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

const SEEDINIT: number = 74755;
const SEED_1309: number = 1309;
const SEED_13849: number = 13849;
const SEED_65535: number = 65535;
const DIVISOR_120: number = 120;
const SUBTRACT_60: number = 60;
const DIVISOR_3: number = 3;
const LOOP_TIMES: number = 5000;
const TIME_CONVERSION: number = 1000;
const TEST_TIMES: number = 3;

/* Intmm, Mm */
let rowsize = 40;

/* global */
let seed: number = 0; /* converted to long for 16 bit WR*/

let rma: number[][] = Array.from({ length: rowsize + 1 }, (v: number) => Array.from({ length: rowsize + 1 }, (v: number) => 0.0));
let rmb: number[][] = Array.from({ length: rowsize + 1 }, (v: number) => Array.from({ length: rowsize + 1 }, (v: number) => 0.0));
let rmr: number[][] = Array.from({ length: rowsize + 1 }, (v: number) => Array.from({ length: rowsize + 1 }, (v: number) => 0.0));

function initrand(): void {
  seed = SEEDINIT; /* constant to long WR*/
}

function rand(): number {
  seed = (seed * SEED_1309 + SEED_13849) & SEED_65535; /* constants to long WR*/
  return seed; /* typecast back to int WR*/
}

/* Multiplies two real matrices. */

function rInitmatrix(m: number[][]): void {
  let temp: number;
  for (let i = 1; i <= rowsize; i++) {
    for (let j = 1; j <= rowsize; j++) {
      temp = rand();
      m[i][j] = (temp - Math.floor(temp / DIVISOR_120) * DIVISOR_120 - SUBTRACT_60) / DIVISOR_3;
    }
  }
}

function rInnerproduct(result: number[][], a: number[][], b: number[][], row: number, column: number): void {
  /* computes the inner product of A[row,*] and B[*,column] */
  result[row][column] = 0.0;
  for (let i = 1; i <= rowsize; i++) {
    result[row][column] = result[row][column] + a[row][i] * b[i][column];
  }
}

function mm(run: number): void {
  initrand();

  rInitmatrix(rma);

  rInitmatrix(rmb);
  for (let i = 1; i <= rowsize; i++) {
    for (let j = 1; j <= rowsize; j++) {
      rInnerproduct(rmr, rma, rmb, i, j);
    }
  }
  if (run < rowsize) {
    //printLog(`${rmr[run + 1][run + 1]}`)
  }
}
/*
   * @State
   * @Tags Jetstream2
   */
class Benchmark {
  /*
   * @Benchmark
   */
  runIteration(): void {
    for (let i = 0; i < LOOP_TIMES; i++) {
      mm(i);
    }
  }
}

//以下是测试打印日志相关代码
const isDebug = false;
function printLog(str: string): void {
  if (isDebug) {
    print(str);
  }
}
declare interface ArkTools {
  timeInUs(args: number): number;
}
function startRun(times: number, ben: Benchmark = new Benchmark()): void {
  let start = ArkTools.timeInUs() / TIME_CONVERSION;
  for (let i = 0; i < times; i++) {
    ben.runIteration();
  }
  let end = ArkTools.timeInUs() / TIME_CONVERSION;
  print('float-mm: ms = ' + (end - start).toString());
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  startRun(TEST_TIMES);
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

startRun(TEST_TIMES);
