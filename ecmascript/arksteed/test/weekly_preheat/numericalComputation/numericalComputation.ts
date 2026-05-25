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
// declare function print(arg:string) : string;

declare interface ArkTools{
  timeInUs(arg:any):number
}

let input: number = 3000000;

function GenerateFloatInput(): Float64Array {
  return new Float64Array([1.0, 2.0]);
}

function GenerateBitOpsInput(): Int32Array {
  return new Int32Array([2, 3]);
}

function GenerateIntegerInput(): Int32Array {
  return new Int32Array([2, 4]);
}

function GenerateFakeRandomInteger(): Int32Array {
  let resource: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);
  return resource;
}

function GenerateFakeRandomFloat(): Float64Array {
  let resource: Float64Array = new Float64Array([12.2, 43.5, 56.2, 76.6, 89.7, 54.9, 45.2, 32.5, 35.6, 47.2, 46.6, 44.3, 21.2, 37.6, 84.57]);
  return resource;
}

// 浮点计算
function FloatNumAddition() {
  let count: number = 3000000;
  let floatInputs: Float64Array = GenerateFloatInput();
  let f1: number = 1.0 // floatInputs[0];
  let f2: number = 2.0 // floatInputs[1];
  let f3: number = 1.0;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    // f3 += f1 + f2; // 86ms
    if ((resources[i % f3 & (resourcesLength - 1)] & 1) == 0) { // 2100ms
      f3 += f1;
    } else {
      f3 += f2;
    }
  }
  let end: number = ArkTools.timeInUs();
  print("FloatNumAddition" + f3);
  let time = (end - start) / 1000
  print("Numerical Calculation - FloatNumAddition:\t"+String(time)+"\tms");
  return time;
}
// FloatNumAddition()
// let runner1 = new BenchmarkRunner("Numerical Calculation - FloatNumAddition", FloatNumAddition);
// runner1.run();


function FloatNumSubstraction() {
  let count: number = 3000000;
  let floatInputs: Float64Array = GenerateFloatInput();
  let f1: number = 1.0 // floatInputs[0];
  let f2: number = 2.0 // floatInputs[1];
  let f3: number = 13000000.0;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % f3 & (resourcesLength - 1)] & 1) == 0) {
      f3 -= f1
    } else {
      f3 -= f2
    }
  }
  let end: number = ArkTools.timeInUs();
  print("FloatNumSubstraction" + f3);
  let time = (end - start) / 1000
  print("Numerical Calculation - FloatNumSubstraction:\t"+String(time)+"\tms");
  return time;
}
// FloatNumSubstraction()
// let runner2 = new BenchmarkRunner("Numerical Calculation - FloatNumSubstraction", FloatNumSubstraction);
// runner2.run();

function FloatNumProduction() {
  let count: number = 3000000;
  let floatInputs: Float64Array = GenerateFloatInput();
  let f1: number = 1.0 // floatInputs[0];
  let f2: number = 2.0 // floatInputs[1];
  let f3: number = 1.0;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i = 0; i < count; i++) {
    if ((resources[i & (resourcesLength - 1)] & 1) == 0) {
      f3 += f3 * f1
    } else {
      f3 += f3 * f2
    }
  }
  let end: number = ArkTools.timeInUs();
  print("FloatNumProduction" + f3);
  let time = (end - start) / 1000
  print("Numerical Calculation - FloatNumProduction:\t"+String(time)+"\tms");
  return time;
}
// FloatNumProduction()
// let runner3 = new BenchmarkRunner("Numerical Calculation - FloatNumProduction", FloatNumProduction);
// runner3.run();

function FloatNumDivision() {
  let count: number = 3000000;
  let f3: number = 1.0;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let inputs: Float64Array = GenerateFakeRandomFloat();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  let inputsLength: number = inputs.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i & (resourcesLength - 1)] & 1) == 0) {
      f3 += f3 / inputs[i & (inputsLength - 1)] / 0.1
    } else {
      f3 += f3 / inputs[(i + resources[i & (resourcesLength - 1)]) & (inputsLength - 1)] / 0.1
    }
  }
  let end: number = ArkTools.timeInUs();
  print("FloatNumDivision" + f3);
  let time = (end - start) / 1000
  print("Numerical Calculation - FloatNumDivision:\t"+String(time)+"\tms");
  return time;
}
// FloatNumDivision()
// let runner4 = new BenchmarkRunner("Numerical Calculation - FloatNumDivision", FloatNumDivision);
// runner4.run();

// 位运算
function BitOpsAND() {
  let count: number = 30000000;
  let bitInputs: Int32Array = GenerateBitOpsInput();
  let b1: number = 2 // bitInputs[0]; // 10
  let b2: number = 3 // bitInputs[1]; // 11
  let b3: number = 1;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % b3 & (resourcesLength - 1)] & 1) == 0) {
      b3 = (b3 & b1) + 1
    } else {
      b3 = (b3 & b2) + 1
    }
  }
  let end: number = ArkTools.timeInUs();
  print("BitOpsAND" + b3);
  let time = (end - start) / 1000
  print("Numerical Calculation - BitOpsAND:\t"+String(time)+"\tms");
  return time;
}
// BitOpsAND()
// let runner5 = new BenchmarkRunner("Numerical Calculation - BitOpsAND", BitOpsAND);
// runner5.run();

function BitOpsOR() {
  let count: number = 30000000;
  let b1: number = 2 // bitInputs[0]; // 10
  let b2: number = 3 // bitInputs[1]; // 11
  let b3: number = 1;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % b3 & (resourcesLength - 1)] & 1) == 0) {
      b3 = (b3 | b1) + 1
      // b3 = b3 | resources[i & (resourcesLength - 1)]
    } else {
      b3 = (b3 | b2) + 1
      // b3 = b3 | resources[(i + 5) & (resourcesLength - 1)]
    }
  }
  let end: number = ArkTools.timeInUs();
  print("BitOpsOR" + b3);
  let time = (end - start) / 1000
  print("Numerical Calculation - BitOpsOR:\t"+String(time)+"\tms");
  return time;
}
// BitOpsOR()
// let runner6 = new BenchmarkRunner("Numerical Calculation - BitOpsOR", BitOpsOR);
// runner6.run();

function BitOpsXOR() {
  let count: number = 30000000;
  let b3: number = 1;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if (b3 <= 0) {
      b3 = 1
    }
    if ((resources[i % b3 & (resourcesLength - 1)] & 1) == 0) {
      b3 = b3 ^ resources[i & (resourcesLength - 1)]
    } else {
      b3 = b3 ^ resources[(i + 5) & (resourcesLength - 1)]
    }
  }
  let end: number = ArkTools.timeInUs();
  print("BitOpsXOR" + b3);
  let time = (end - start) / 1000
  print("Numerical Calculation - BitOpsXOR:\t"+String(time)+"\tms");
  return time;
}
// BitOpsXOR()
// let runner7 = new BenchmarkRunner("Numerical Calculation - BitOpsXOR", BitOpsXOR);
// runner7.run();

function BitOpsNOT() {
  let count: number = 30000000;
  let b1: number = -2;
  let b2: number = -3;
  let b3: number = 1;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % b3 & (resourcesLength - 1)] & 1) == 0) {
      b3 += ~b1
    } else {
      b3 += ~b2
    }
  }
  let end: number = ArkTools.timeInUs();
  print("BitOpsNOT" + b3);
  let time = (end - start) / 1000
  print("Numerical Calculation - BitOpsNOT:\t"+String(time)+"\tms");
  return time;
}

// BitOpsNOT()
// let runner8 = new BenchmarkRunner("Numerical Calculation - BitOpsNOT", BitOpsNOT);
// runner8.run();

function BitOpsShiftLeft() {
  let count: number = 30000000;
  let bitInputs: Int32Array = GenerateBitOpsInput();
  let b1: number = 2 // bitInputs[0]; // 10
  let b3: number = 1;
  let resources: Int32Array = GenerateFakeRandomInteger()
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % b3 & (resourcesLength - 1)] & 1) == 0) {
      b3 += b1 << 1;
    } else {
      b3 += b1 << 2;
    }
  }
  let end: number = ArkTools.timeInUs();
  print("BitOpsShiftLeft" + b3);
  let time = (end - start) / 1000
  print("Numerical Calculation - BitOpsShiftLeft:\t"+String(time)+"\tms");
  return time;
}
// BitOpsShiftLeft()
// let runner9 = new BenchmarkRunner("Numerical Calculation - BitOpsShiftLeft", BitOpsShiftLeft);
// runner9.run();

function BitOpsShiftRight() {
  let count: number = 30000000;
  let bitInputs: Int32Array = GenerateBitOpsInput();
  let b1: number = 2 // bitInputs[0]; // 10
  let b3: number = 1;
  let resources: Int32Array = GenerateFakeRandomInteger()
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % b3 & (resourcesLength - 1)] & 1) == 0) {
      b3 += b1 >> 1;
    } else {
      b3 += b1 >> 2;
    }
  }
  let end: number = ArkTools.timeInUs();
  print("BitOpsShiftRight" + b3);
  let time = (end - start) / 1000
  print("Numerical Calculation - BitOpsShiftRight:\t"+String(time)+"\tms");
  return time;
}
// BitOpsShiftRight()
// let runner10 = new BenchmarkRunner("Numerical Calculation - BitOpsShiftRight", BitOpsShiftRight);
// runner10.run();


// 整数计算
function IntegerNumAddition() {
  let count: number = 30000000;
  let IntegerInputs: Int32Array = GenerateIntegerInput();
  let resources: Int32Array = GenerateFakeRandomInteger();
  let i1: number = 2// IntegerInputs[0];
  let i2: number = 4// IntegerInputs[1];
  let i3: number = resources[0];
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) { // 1.07ms
      i3 += i1
    } else {
      i3 += i2
    }
  }
  let end = ArkTools.timeInUs();
  // print("IntegerNumAddition" + i3);
  let time = (end - start) / 1000
  print("Numerical Calculation - IntegerNumAddition:\t"+String(time)+"\tms");
  return time;
}
// IntegerNumAddition()
// print("IntegerNumAddition time: " + IntegerNumAddition())
// let runner12 = new BenchmarkRunner("Numerical Calculation - IntegerNumAddition", IntegerNumAddition);
// runner12.run();


function IntegerNumSubstraction() {
  let count: number = 30000000;
  let IntegerInputs: Int32Array = GenerateIntegerInput();
  let i1: number = 2 // IntegerInputs[0];
  let i2: number = 4 // IntegerInputs[1];
  let i3: number = 130000000;
  let resources: Int32Array = GenerateFakeRandomInteger()
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) { // 1.07ms
      i3 -= i1
    } else {
      i3 -= i2
    }
  }
  let end: number = ArkTools.timeInUs();
  print("IntegerNumSubstraction" + i3);
  let time = (end - start) / 1000
  print("Numerical Calculation - IntegerNumSubstraction:\t"+String(time)+"\tms");
  return time;
}
// IntegerNumSubstraction()
// console.log("IntegerNumSubs time: " + IntegerNumSubstraction());
// let runner13 = new BenchmarkRunner("Numerical Calculation - IntegerNumSubstraction", IntegerNumSubstraction);
// runner13.run();

function IntegerNumProduction() {
  let count: number = 30000000;
  let i3: number = 1;
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length - 1;
  for (let i: number = 0; i < count; i++) {
    if ((resources[i % i3 & resourcesLength] & 1) == 0) { // ark:21ms node:4ms
      i3 *= 3 // resources[i & resourcesLength]
    } else {
      i3 *= 2 // resources[(i + 5) & resourcesLength]
    }
    if (i3 > 10000000) {
      i3 = 1
    }
  }
  let end: number = ArkTools.timeInUs();
  print("IntegerNumProduction" + i3);
  let time = (end - start) / 1000
  print("Numerical Calculation - IntegerNumProduction:\t"+String(time)+"\tms");
  return time;
}
// IntegerNumProduction()
// let runner14 = new BenchmarkRunner("Numerical Calculation - IntegerNumProduction", IntegerNumProduction);
// runner14.run();


function IntegerNumDivision() {
  let count: number = 30000000;// input;
  let i1: number = 2 // IntegerInputs[0];
  let i2: number = 4 // IntegerInputs[1];
  let i3: number = 32768;
  let resources: Int32Array = GenerateFakeRandomInteger();
  // let results: number[] = [0, 0, 0, 0, 0];
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    if (i3 <= 4) {
      i3 = 32768;
    }
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) { // ark:21ms node:4ms
      // i3 += i2 / i1
      i3 = i3 / i1
      // results[i % 5] = i2 / i1
    } else {
      // i3 += i2 / i1 / 2
      i3 = i3 / i2
      // results[i % 5] = i2 / i1 / 2
    }

  }
  let end: number = ArkTools.timeInUs();
  // for (let i = 0; i < 5; i++) {
  // i3 += results[i];
  // }
  print("IntegerNumDivision" + i3);
  let time = (end - start) / 1000
  print("Numerical Calculation - IntegerNumDivision:\t" + String(time) + "\tms");
  return time;
}
// IntegerNumDivision()
// let runner15 = new BenchmarkRunner("Numerical Calculation - IntegerNumDivision", IntegerNumDivision);
// runner15.run();


// 浮点数比较
function FloatNumComparision() {
  let count: number = 3000000; // input;
  let resources: Float64Array = GenerateFakeRandomFloat();
  let res: number = 1;
  let start: number = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for (let i: number = 0; i < count; i++) {
    let next_index: number = i + 5
    let next_res: number = res + 1
    if (resources[i % res & (resourcesLength - 1)] > resources[next_index % next_res & (resourcesLength - 1)]) {
      res += 1
    } else {
      res += 2
    }
  }
  let end: number = ArkTools.timeInUs();
  print("FloatNumComparision" + res);
  let time = (end - start) / 1000
  print("Numerical Calculation - FloatNumComparision:\t"+String(time)+"\tms");
  return time;
}
// FloatNumComparision()
// let runner16 = new BenchmarkRunner("Numerical Calculation - FloatNumComparision", FloatNumComparision);
// runner16.run();

// String运算
function StringCalculation() {
  let count: number = 3000000 / 1000;
  let str1: string = "h";
  let res: string = "11";
  let resources: Int32Array = GenerateFakeRandomInteger();
  let start: number = ArkTools.timeInUs();
  for (let i: number = 0; i < count; i++) {
    if (resources[i % res.length % 15] > resources[(i + resources[i % 15]) % res.length % 15]) {
      res += str1 + i;
    } else {
      res += str1;
//       res += str1;
    }
  }
  let end: number = ArkTools.timeInUs();
  print(""+res.length);
  let time = (end - start) / 1000
  print("Numerical Calculation - StringCalculation:\t"+String(time)+"\tms");
  return time;
}
// StringCalculation()
// let runner18 = new BenchmarkRunner("Numerical Calculation - StringCalculation", StringCalculation);
// runner18.run();


let loopCountForPreheat = 1;

for (let i = 0; i < loopCountForPreheat; i++) {
  FloatNumAddition()
  FloatNumSubstraction()
  FloatNumProduction()
  FloatNumDivision()
  BitOpsAND()
  BitOpsOR()
  BitOpsXOR()
  BitOpsNOT()
  BitOpsShiftLeft()
  BitOpsShiftRight()
  IntegerNumAddition()
  IntegerNumSubstraction()
  IntegerNumProduction()
  IntegerNumDivision()
  FloatNumComparision()
  StringCalculation()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

FloatNumAddition()
FloatNumSubstraction()
FloatNumProduction()
FloatNumDivision()
BitOpsAND()
BitOpsOR()
BitOpsXOR()
BitOpsNOT()
BitOpsShiftLeft()
BitOpsShiftRight()
IntegerNumAddition()
IntegerNumSubstraction()
IntegerNumProduction()
IntegerNumDivision()
FloatNumComparision()
StringCalculation()
