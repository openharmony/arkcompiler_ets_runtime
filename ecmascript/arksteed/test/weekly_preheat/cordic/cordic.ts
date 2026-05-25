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
// declare function print(arg: string): string;

declare interface ArkTools {
  timeInUs(arg: any): number
}

function FIXED(X: number): number {
  return X * 65536.0;
}

function FLOAT(X: number): number {
  return X / 65536.0;
}

let AG_CONST: number = 0.6072529350;

function cordicsincos(Target: number, Angles: Float64Array): number {
  let X: number = 0.0;
  let Y: number = 0.0;
  let TargetAngle: number = 0.0;
  let CurrAngle: number = 0.0;
  let Step: number = 0;

  X = FIXED(AG_CONST);
  TargetAngle = FIXED(Target);
  CurrAngle = 0.0;
  for (Step = 0; Step < 12; Step++) {
    let NewX: number = 0.0;
    if (TargetAngle > CurrAngle) {
      NewX = X - (Y >> Step);
      Y = (X >> Step) + Y;
      X = NewX;
      CurrAngle += Angles[Step];
    } else {
      NewX = X + (Y >> Step);
      Y = -(X >> Step) + Y;
      X = NewX;
      CurrAngle -= Angles[Step];
    }
  }
  return FLOAT(X) * FLOAT(Y);
}

function RunCordic() {
  let res = 0.0;
  let Target: number = 28.027;
  let input: number = 150000;
  let Angles: Float64Array = new Float64Array([
  FIXED(45.0), FIXED(26.565), FIXED(14.0362), FIXED(7.12502),
  FIXED(3.57633), FIXED(1.78991), FIXED(0.895174), FIXED(0.447614),
  FIXED(0.223811), FIXED(0.111906), FIXED(0.055953),
  FIXED(0.027977)
  ]);
  let start = ArkTools.timeInUs();
  for (let i = 0; i < input; i++) {
    res += cordicsincos(Target + i, Angles);
  }
  let end = ArkTools.timeInUs();
  let time = (end - start) / 1000
  print(""+res);
  print("Numerical Calculation - RunCordic:\t" + String(time) + "\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunCordic()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);


RunCordic()
// let runner1 = new BenchmarkRunner("Numerical Calculation - RunCordic", RunCordic);
// runner1.run();
