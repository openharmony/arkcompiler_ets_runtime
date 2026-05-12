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

function GenerateRandoms(): number[] {
  const result : number[] = [];
  for (let i = 0; i < 2; i++) {
    const randomNum: number = Math.floor(Math.random() * 2) + 1; // 生成介于1和2之间的随机整数
    result.push(randomNum);
  }
  return result;
}
let generaterandoms = GenerateRandoms();

let global_value = 0;

function GenerateFakeRandomInteger(): Int32Array {
  let resource: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);
  return resource;
}
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

let arr: Int32Array = GenerateFakeRandomInteger()

class ClassFunc {
  foo(resources: Int32Array, i: number, i3: number, resourcesLength: number): number {
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
      i3 += 1;
    } else {
      i3 += 2;
    }
    return i3;
  }
  parameterlessFoo(): Obj {
    let res: Obj[] = [new Obj(0), new Obj(0), new Obj(0), new Obj(0), new Obj(0)];
    let resources: Obj[] = GenerateFakeRandomObject();
    for (let i = 0; i < 200; i++) {
      res[i % 5] = resources[i % 15];
    }
    return res[1];
  }
  differentFoo(resources : Int32Array = arr, i: number = 1, i3: number = 1, resourcesLength: number = 1): number {
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
      i3 += 1.1;
    } else {
      i3 += 2.1;
    }
    return i3;
  }
  defaultFoo(resources : Int32Array = arr, i: number = 1, i3: number = 1, resourcesLength: number = 1): number {
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
      i3 += 1;
    } else {
      i3 += 2;
    }
    return i3;
  }
  variableFoo(a?: number, b?: string, c?: boolean): void {
    arr[global_value] += 1;
  }
  argsFoo(...args: number[]): void {
    global_value += 1
  }
}

/***************************** With parameters *****************************/
// Method call
function RunMethodCall(): number {
  let count : number = 10000000;
  const cf = new ClassFunc();
  let i3 : number = 1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let func = cf.foo;
  let resourcesLength: number = resources.length;
  let startTime = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    i3 = func(resources, i, i3, resourcesLength);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime =ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunMethodCall:\t"+String(time)+"\tms");
  // print("Method Call:\t"+String(time)+"\tms");

  return time;
}

function RunParameterlessMethodCall(): number {
  let count : number = 10000;
  const cf = new ClassFunc();
  global_value = 0;
  let i3 : Obj = new Obj(1);
  let func = cf.parameterlessFoo;
  let startTime = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    i3 = func();
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunParameterlessMethodCall:\t"+String(time)+"\tms");
  return time
}

/***************************** Default parameters *****************************/

// Method call
function RunDefMethodCall(): number {
  let count : number = 10000000;
  const cf = new ClassFunc();
  global_value = 0;
  let i3 : number = 1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let resourcesLength: number = resources.length;
  let func = cf.defaultFoo;
  let startTime = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    i3 = func(resources, i, i3, resourcesLength);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunDefMethodCall:\t"+String(time)+"\tms");
  return time
}

/********************* Different parameters *********************/

// Method call
function RunDifMethodCall(): number {
  let count : number = 10000000;
  const cf = new ClassFunc();
  global_value = 0;
  let i3 : number = 1.1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let resourcesLength: number = resources.length;
  let func = cf.differentFoo;
  let startTime = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    i3 = func(resources, i, i3, resourcesLength);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunDifMethodCall:\t"+String(time)+"\tms");
  return time
}

/************************* Variable parameters *************************/

// Method call
function RunVariableMethodCall(): number {
  let count : number = 10000000;
  const cf = new ClassFunc();
  global_value = 0;
  let func = cf.variableFoo;
  let startTime = ArkTools.timeInUs();
  for (let i = 0; i < count; i++) {
    func(1, '1', true);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunVariableMethodCall:\t"+String(time)+"\tms");
  return time
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunMethodCall();
  RunParameterlessMethodCall();
  RunDefMethodCall();
  RunDifMethodCall();
  RunVariableMethodCall();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunMethodCall();
RunParameterlessMethodCall();
RunDefMethodCall();
RunDifMethodCall();
RunVariableMethodCall();

print("Ts Method Call Is End, global_value value: \t" + String(global_value));