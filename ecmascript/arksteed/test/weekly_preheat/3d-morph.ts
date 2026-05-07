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

const NX_NUMBER: number = 120;
const LOOP_COUNT: number = 15;
const NUM_EIGHT: number = 8;
const NUM_TWO: number = 2;
const NUM_THREE: number = 3;
const MAGNIFICATION: number = 50;
const TIME_UNIT: number = 1000;
const TEST_COUNT: number = 80;

class Morph {
  loops: number = LOOP_COUNT;
  nx: number = NX_NUMBER;
  nz: number = NX_NUMBER;

  morph(arr: Float64Array, f: number): void {
    let pI2nx = (Math.PI * NUM_EIGHT) / this.nx;
    let f30 = -(MAGNIFICATION * Math.sin(f * Math.PI * NUM_TWO));
    for (let i = 0; i < this.nz; i++) {
      for (let j = 0; j < this.nx; j++) {
        let index = NUM_THREE * (i * this.nx + j) + 1;
        let x = (j - 1) * pI2nx;
        arr[index] = Math.sin(x) * -f30;
      }
    }
  }

  runTest(): void {
    let a: Float64Array = new Float64Array(this.nx * this.nx * NUM_THREE);
    for (let index = 0; index < this.nx * this.nx * NUM_THREE; index++) {
      a[index] = 0;
    }

    for (let i = 0; i < this.loops; i++) {
      this.morph(a, i / this.loops);
    }
    let testOutput: number = 0;
    for (let i = 0; i < this.nx; i++) {
      let index = NUM_THREE * (i * this.nx + 1) + 1;
      if (index >= a.length) {
        break;
      }
      testOutput += a[index];
    }
    //debugLog("testOutput value is" + testOutput)

    let epsilon = 1e-13;
    if (Math.abs(testOutput) >= epsilon) {
      //debugLog("Error: bad test output: expected magnitude below" + epsilon + "but got" + testOutput)
    }
  }
}

let isdebug: boolean = false;
function debugLog(msg: string): void {
  if (isdebug) {
    print(msg);
  }
}

declare interface ArkTools {
  timeInUs(args: number): number;
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
    let start = ArkTools.timeInUs();
    let morph = new Morph();
    for (let i = 0; i < TEST_COUNT; i++) {
      morph.runTest();
    }
    let end = ArkTools.timeInUs();
    print('3d-morph: ms = ' + (end - start) / TIME_UNIT);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().run();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().run();
