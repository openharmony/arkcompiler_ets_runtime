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
// Modify n value to change the difficulty of the benchmark
// suggesting using input = 5500;
//
declare interface ArkTools{
  timeInUs(arg:any):number
}


const input: number = 500;

function approximate(u: Float64Array, v: Float64Array, buffer: Float64Array): number {
  for (let count = 0; count < 10; count++) {
    multiplyAv(u, buffer);
    multiplyAtv(buffer, v);

    multiplyAv(v, buffer);
    multiplyAtv(buffer, u);
  }

  let vLen: number = v.length;
  let vBv: number = 0.0, vv: number = 0.0;
  for (let i = 0; i < vLen; i++) {
    vBv += u[i] * v[i];
    vv += v[i] * v[i];
  }

  return Math.sqrt(vBv / vv);
}

function multiplyAv(v: Float64Array, av: Float64Array): void {
  let vLen = v.length;
  let avLen = av.length;
  for (let i = 0; i < avLen; i++) {
    av[i] = 0.0;
    for (let j = 0; j < vLen; j++) {
      const ij = i + j;
      let ret = 1.0 / (ij * (ij + 1) / 2 + i + 1);
      av[i] +=ret * v[j];
    }
  }
}

function multiplyAtv(v: Float64Array, atv: Float64Array): void {
  let vLen = v.length;
  let atvLen = atv.length;
  for (let i = 0; i < atvLen; i++) {
    atv[i] = 0;
    for (let j = 0; j < vLen; j++) {
      const ij = i + j;
      let ret = 1.0 / (ij * (ij + 1) / 2 + j + 1);
      atv[i] += ret * v[j];
    }
  }
}

function RunSpectralNorm() {
  const n: number = input;
  const u: number[] = Array(n).fill(1.0);
  const v: number[] = Array(n).fill(0.0);
  const buffer: number[] = Array(n).fill(0.0);
  let uF: Float64Array = new Float64Array(u);
  let vF: Float64Array = new Float64Array(v);
  let bufferF: Float64Array = new Float64Array(buffer);
  let res = 0.0;
  const start = ArkTools.timeInUs();
  res = approximate(uF, vF, bufferF);
  const end = ArkTools.timeInUs();
  // ASSERT_FLOAT_EQ(res, 1.2742241159529055);
  let time = (end - start) / 1000
  print("Array Access - RunSpectralNorm:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunSpectralNorm()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunSpectralNorm()
// let runner = new BenchmarkRunner("Array Access - RunSpectralNorm", RunSpectralNorm);
// runner.run();
