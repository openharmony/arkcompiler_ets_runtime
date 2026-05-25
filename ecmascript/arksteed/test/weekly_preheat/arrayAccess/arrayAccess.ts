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

function GenerateFakeRandomInteger(): Int32Array {
  let resource: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);
  return resource;
}

function GenerateFakeRandomFloat(): Float64Array {
  let resource: Float64Array = new Float64Array([12.2, 43.5, 56.2, 76.6, 89.7, 54.9, 45.2, 32.5, 35.6, 47.2, 46.6, 44.3, 21.2, 37.6, 84.57]);
  return resource;
}

function GenerateFakeRandomBool(): boolean[] {
  let resource: boolean[] = [true, false, true, false, true, false, true, false, true, false, true, false, true, false, true];
  return resource;
}

function GenerateFakeRandomString(): string[] {
  let resource: string[] = ["Op", "HOS", "Op", "HOS", "Op", "HOS", "Op", "HOS", "Op", "HOS", "Op", "HOS", "Op", "HOS", "Op"];
  return resource;
}

function GenerateFakeRandomIndex(): Int32Array {
  let resource: Int32Array = new Int32Array([3, 14, 44, 25, 91, 38, 82, 88, 64, 81, 70, 90, 33, 63, 70]);
  return resource;
}

// primitive array
function IntegerArray() {
  let count: number = 3000000;
  let integerIndexes: Int32Array = GenerateFakeRandomIndex();
  let resources: Int32Array = GenerateFakeRandomInteger();
  let res: Int32Array = new Int32Array([0, 0, 0, 0, 0]);
  let num: number = 1;
  let start = ArkTools.timeInUs();
  let length: number = resources.length - 1
  let resLength: number = res.length - 1;
  for (let i = 0; i < count; i++) {
    num += integerIndexes[i % num & length];
    res[i & resLength] = resources[i % num & length];
  }
  let end = ArkTools.timeInUs();
  let tmp = 1;
  for (let i = 0; i < 5; i++) {
    tmp += res[i];
  }
  print(""+tmp);
  let time = (end - start) / 1000
  print("Array Access - IntegerArray:\t"+String(time)+"\tms");
  return time;
}
// IntegerArray()
// let runner1 = new BenchmarkRunner("Array Access - IntegerArray", IntegerArray);
// runner1.run();

function FloatArray() {
  let count: number = 3000000;
  let res: Float64Array = new Float64Array([0.0, 0.0, 0.0, 0.0, 0.0]);
  let resources: Float64Array = GenerateFakeRandomFloat();
  let integerIndexes: Int32Array = GenerateFakeRandomIndex();
  let num: number = 1;
  let start = ArkTools.timeInUs();
  let length: number = resources.length - 1;
  let resLength: number = res.length - 1;
  for (let i = 0; i < count; i++) {
    num += integerIndexes[i % num & length];
    res[i & resLength] = resources[i % num & length];
  }
  let end = ArkTools.timeInUs();
  let tmp: number = 0.0;
  for (let i = 0; i < 5; i++) {
    tmp += res[i];
  }
  print(""+tmp)
  let time = (end - start) / 1000
  print("Array Access - FloatArray:\t"+String(time)+"\tms");
  return time;
}
// FloatArray()
// let runner2 = new BenchmarkRunner("Array Access - FloatArray", FloatArray);
// runner2.run();

function BoolArray() {
  let count: number = 3000000;
  let resources: boolean[] = GenerateFakeRandomBool();
  let integerIndexes: Int32Array = GenerateFakeRandomIndex();
  let res: Int32Array = new Int32Array([0, 0, 0, 0, 0]);
  let num: number = 1;
  let start = ArkTools.timeInUs();
  let length: number = resources.length - 1;
  let resLength: number = res.length - 1;
  for (let i = 0; i < count; i++) {
    num += integerIndexes[i % num & length];
    if (resources[i % num & length]) {
      res[i & resLength] = integerIndexes[i & length]
    }
  }
  let end = ArkTools.timeInUs();
  let tmp: number = 0;
  for (let i = 0; i < 5; i++) {
    tmp += res[i];
  }
  print(""+tmp)
  let time = (end - start) / 1000
  print("Array Access - BoolArray:\t"+String(time)+"\tms");
  return time;
}
// BoolArray()
// let runner3 = new BenchmarkRunner("Array Access - BoolArray", BoolArray);
// runner3.run();

function StringArray() {
  let count: number = 3000000 / 100;
  let resources: string[] = GenerateFakeRandomString();
  let res: number[] = [0, 0, 0];
  let start = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    res[i % res.length] += resources[i % 15].length;
  }
  let end = ArkTools.timeInUs();
  let sum: number = 0;
  for (let i = 0; i < res.length; i++) {
    sum += res[i];
  }
  print(""+sum);
  let time = (end - start) / 1000
  print("Array Access - StringArray:\t"+String(time)+"\tms");
  return time;
}
// StringArray()
// let runner4 = new BenchmarkRunner("Array Access - StringArray", StringArray);
// runner4.run();

class Obj {
  value: number = 0
  constructor(value: number) {
    this.value = value
  }
};

function GenerateFakeRandomObject(): Obj[] {
  let resource: Obj[] = new Array(15).fill(new Obj(0));
  for (let i = 0; i < 15; i++) {
    let random = Math.random() * (10) + 1;
    resource[i] = new Obj(random)
  }
  return resource;
}

// object array
function ObjectArray() {
  let count: number = 3000000;
  // let res: number = 1;
  let res: Obj[] = [new Obj(0), new Obj(0), new Obj(0), new Obj(0), new Obj(0)];
  // let res: Obj[] =  [new Obj(0), new Obj(0), new Obj(0), new Obj(0), new Obj(0)];
  let resources: Obj[] = GenerateFakeRandomObject();
  let start = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    // res += resources[i % res % 15].value
    res[i % 5] = resources[i % 15];
  }
  let end = ArkTools.timeInUs();
  // print(res)
  let tmp = 1;
  for (let i = 0; i < 5; i++) {
    tmp += res[i].value;
  }
  print(""+tmp);
  let time = (end - start) / 1000
  print("Array Access - ObjectArray:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  IntegerArray()
  FloatArray()
  BoolArray()
  StringArray()
  ObjectArray()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

IntegerArray()
FloatArray()
BoolArray()
StringArray()
ObjectArray()
// let runner5 = new BenchmarkRunner("Array Access - ObjectArray", ObjectArray);
// runner5.run();

