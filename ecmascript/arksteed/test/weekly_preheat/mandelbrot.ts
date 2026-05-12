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
// import { BenchmarkRunner } from "../../../utils/benchmarkTsSuite";
// declare function print(arg:string) : string;
declare interface ArkTools{
  timeInUs(arg:any):number
}
// Modify n value to change the difficulty of the benchmark
// suggesting using input = 16000;

function Mandelbrot(): number {

  const w: number = 1000;
  const h = w;

  let bit_num: number = 0;
  let i: number = 0;
  let byte_acc: number = 0;
  const iter: number = 50;
  const limit = 2.0;
  let Zr: number = 0;
  let Zi: number = 0;
  let Tr: number = 0;
  let Ti: number = 0;
  let res: number = 0;
  const limit2: number = limit * limit;

  for (let y = 0; y < h; y++) {
    const Ci = 2.0 * y / h - 1.0;
    for (let x = 0; x < w; x++) {
      Zr = 0.0;
      Zi = 0.0;
      Tr = 0.0;
      Ti = 0.0;
      const Cr = 2.0 * x / w - 1.5;


      i = 0;
      while (i < iter && (Tr + Ti <= limit2)) {
        i += 1;
        Zi = 2.0 * Zr * Zi + Ci;
        Zr = Tr - Ti + Cr;
        Tr = Zr * Zr;
        Ti = Zi * Zi;
      }

      byte_acc <<= 1;
      if ((Tr + Ti) <= (limit2)) {
        byte_acc |= 0x01;
      }

      bit_num += 1;

      if (bit_num == 8) {
        res += byte_acc;
        byte_acc = 0;
        bit_num = 0;
      } else if (x == (w - 1)) {
        byte_acc <<= (8 - w % 8);
        res += byte_acc;
        byte_acc = 0;
        bit_num = 0;
      }
    }
  }
  return res;
}

function RunMandelbrot() {
  let start = ArkTools.timeInUs();
  let res = Mandelbrot();
  let end = ArkTools.timeInUs();
  let time = (end - start) / 1000
  print(""+res);
  print("Array Access - RunMandelbrot:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunMandelbrot()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunMandelbrot()
// let runner = new BenchmarkRunner("Array Access - RunMandelbrot", RunMandelbrot);
// runner.run();
