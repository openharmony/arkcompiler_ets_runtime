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

/////. Start CORDIC
const FI_CONST: number = 65536.0;
const DEG_CONST: number = 0.017453;
const AG_CONST: number = 0.607252935;
const ANGLE_1_CONST: number = 45.0;
const ANGLE_2_CONST: number = 26.565;
const ANGLE_3_CONST: number = 14.0362;
const ANGLE_4_CONST: number = 7.12502;
const ANGLE_5_CONST: number = 3.57633;
const ANGLE_6_CONST: number = 1.78991;
const ANGLE_7_CONST: number = 0.895174;
const ANGLE_8_CONST: number = 0.447614;
const ANGLE_9_CONST: number = 0.223811;
const ANGLE_10_CONST: number = 0.111906;
const ANGLE_11_CONST: number = 0.055953;
const ANGLE_12_CONST: number = 0.027977;
const STEP_CONST: number = 12;
const CORDIC_CONST: number = 25000;
const NUM_1000_CONST: number = 1000;
const NUM_TIME_LOOP1_CONST: number = 80;
function fixed(x: number): number {
  return x * FI_CONST;
}

function float(x: number): number {
  return x / FI_CONST;
}

function deg2rad(x: number): number {
  return DEG_CONST * x;
}

let angles: number[] = [
  fixed(ANGLE_1_CONST),
  fixed(ANGLE_2_CONST),
  fixed(ANGLE_3_CONST),
  fixed(ANGLE_4_CONST),
  fixed(ANGLE_5_CONST),
  fixed(ANGLE_6_CONST),
  fixed(ANGLE_7_CONST),
  fixed(ANGLE_8_CONST),
  fixed(ANGLE_9_CONST),
  fixed(ANGLE_10_CONST),
  fixed(ANGLE_11_CONST),
  fixed(ANGLE_12_CONST)
];

let target: number = 28.027;

function cordicsincos(target: number): number {
  let x: number;
  let y: number;
  let targetAngle: number;
  let currAngle: number;

  x = fixed(AG_CONST); /* AG_CONST * cos(0) */
  y = 0; /* AG_CONST * sin(0) */

  targetAngle = fixed(target);
  currAngle = 0;

  for (let step = 0; step < STEP_CONST; step++) {
    let newX: number;
    if (targetAngle > currAngle) {
      newX = x - (y >> step);
      y = (x >> step) + y;
      x = newX;
      currAngle += angles[step];
    } else {
      newX = x + (y >> step);
      y = -(x >> step) + y;
      x = newX;
      currAngle -= angles[step];
    }
  }
  return float(x) * float(y);
}

///// End CORDIC
let total: number = 0.0;

function cordic(runs: number): number {
  let start = new Date().getTime();
  for (let i = 0; i < runs; i++) {
    total += cordicsincos(target);
  }
  let end = new Date().getTime();
  return end - start;
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
    cordic(CORDIC_CONST);
    let expected = 10362.570468755888;
    if (total !== expected) {
      print('ERROR: bad result: expected  (expected)   but got (total)');
    }
  }

  /**
   * @Benchmark
   */
  runIterationTime(): void {
    let start = ArkTools.timeInUs() / NUM_1000_CONST;
    for (let i = 0; i < NUM_TIME_LOOP1_CONST; i++) {
      total = 0.0;
      this.run();
    }
    let end = ArkTools.timeInUs() / NUM_1000_CONST;
    print('math-cordic: ms = ' + (end - start));
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
