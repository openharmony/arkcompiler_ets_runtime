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
let global_value = 0;
let arr: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);

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
class Example {
  //static With parameters
  static Foo(resources: Int32Array, i: number, i3: number, resourcesLength: number): number {
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
      i3 += 1;
    } else {
      i3 += 2;
    }
    return i3;
  }
  //static Without parameters
  static parameterlessFoo() : Obj {
    let res: Obj[] = [new Obj(0), new Obj(0), new Obj(0), new Obj(0), new Obj(0)];
    let resources: Obj[] = GenerateFakeRandomObject();
    for (let i = 0; i < 200; i++) {
      res[i % 5] = resources[i % 15];
    }
    return res[1];
  }
  //static Different  parameters
  static DifferentFoo(resources: Int32Array, i: number, i3: number, resourcesLength: number):number {
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
      i3 += 1.1;
    } else {
      i3 += 2.1;
    }
    return i3;
  }
  //static Default  parameters
  static DefaultFoo(resources : Int32Array = arr, i: number = 1, i3: number = 1, resourcesLength: number = 1):number {
    if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
      i3 += 1;
    } else {
      i3 += 2;
    }
    return i3;
  }
  //static Variable  parameters
  static VariableFoo(a?:number, b?:string, c?:boolean){
    arr[global_value] += 1;
  }
  //static ...Args  parameters
  static argsFoo(...args:number[]){
    global_value += 1
  }
}

export function RunStaticFunction():number{
  let count : number = 10000000;
  let i3 : number = 1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let startTime = ArkTools.timeInUs();
  let resourcesLength: number = resources.length;
  for(let i=0;i<count;i++) {
    i3 = Example.Foo(resources, i, i3, resourcesLength)
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Static Function Call - RunStaticFunction:\t"+String(time)+"\tms");
  return time
}
// RunStaticFunction()
// let runner2 = new BenchmarkRunner("Static Function Call - RunNormalCall", RunStaticFunction);
// runner2.run();

export function RunParameterlessStaticFunction():number{
  let count : number = 10000;
  global_value = 0;
  let i3 : Obj = new Obj(1);
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
    i3 = Example.parameterlessFoo()
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Static Function Call - RunParameterlessStaticFunction:\t"+String(time)+"\tms");
  return time
}
// RunParameterlessStaticFunction()
// let runner1 = new BenchmarkRunner("Static Function Call - RunParameterlessCall", RunParameterlessStaticFunction);
// runner1.run();

export function RunNormalDifStaticFunc():number{
  let count : number = 10000000;
  global_value = 0;
  let i3 : number = 1.1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let resourcesLength: number = resources.length;
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
    i3 = Example.DifferentFoo(resources, i, i3, resourcesLength)
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Static Function Call - RunNormalDifStaticFunc:\t"+String(time)+"\tms");
  return time
}
// RunNormalDifStaticFunc()
// let runner3 = new BenchmarkRunner("Static Function Call - RunNormalDifferentCall", RunNormalDifStaticFunc);
// runner3.run();

export function RunNormalVariableFStaticFunc():number{
  let count : number = 10000000;
  global_value = 0;
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
    Example.VariableFoo(1,"2",true)
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Static Function Call - RunNormalVariableFStaticFunc:\t"+String(time)+"\tms");
  return time
}
// RunNormalVariableFStaticFunc()
// let runner4 = new BenchmarkRunner("Static Function Call - RunNormalVariableFCall", RunNormalVariableFStaticFunc);
// runner4.run();
export function RunNormalDefStaticCall():number {
  let count : number = 10000000;
  global_value = 0;
  let i3 : number = 1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let resourcesLength: number = resources.length;
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
    i3 = Example.DefaultFoo(resources, i, i3, resourcesLength);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Static Function Call - RunNormalDefStaticCall:\t"+String(time)+"\tms");
  return time
}
// RunNormalDefStaticCall()
// let runner5 = new BenchmarkRunner("Static Function Call - RunNormalDefCall", RunNormalDefCall);
// runner5.run();

let loopCountForPreheat = 1;

for (let i = 0; i < loopCountForPreheat; i++) {
  RunStaticFunction();
  RunParameterlessStaticFunction();
  RunNormalDifStaticFunc();
  RunNormalVariableFStaticFunc();
  RunNormalDefStaticCall();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunStaticFunction()
RunParameterlessStaticFunction()
RunNormalDifStaticFunc()
RunNormalVariableFStaticFunc()
RunNormalDefStaticCall()


print("Ts Method Call Is End, global_value value: \t" + String(global_value));
