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
/***** Without parameters *****/
function ParameterlessFoo() : Obj {
  let res: Obj[] = [new Obj(0), new Obj(0), new Obj(0), new Obj(0), new Obj(0)];
  let resources: Obj[] = GenerateFakeRandomObject();
  for (let i = 0; i < 200; i++) {
    res[i % 5] = resources[i % 15];
  }
  return res[1];
}

function GenerateFakeRandomInteger(): Int32Array {
  let resource: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);
  return resource;
}

/***************** With parameters *****************/
function Foo(resources: Int32Array, i: number, i3: number, resourcesLength: number): number {
  if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
    i3 += 1;
  } else {
    i3 += 2;
  }
  return i3;
}
export function RunNormalCall():number {
  let count : number = 10000000;
  let i3 : number = 1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let startTime = ArkTools.timeInUs();
  let foo = Foo;
  let resourcesLength: number = resources.length;
  for(let i=0;i<count;i++) {
    i3 = foo(resources, i, i3, resourcesLength);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunNormalCall:\t"+String(time)+"\tms");
  return time
}
// RunNormalCall()

// No parameter function call
export function RunParameterlessCall():number{
  let count : number = 10000;
  global_value = 0;
  let i3 : Obj = new Obj(1);
  let startTime = ArkTools.timeInUs();
  let foo = ParameterlessFoo;
  for(let i=0;i<count;i++) {
    i3 = foo()
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime))  / 1000
  print("Function Call - RunParameterlessCall:\t"+String(time)+"\tms");
  return time
}
// RunParameterlessCall()

/***************************** Default  parameters *****************************/
function DefaultFoo(resources : Int32Array = arr, i: number = 1, i3: number = 1, resourcesLength: number = 1):number {
  if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
    i3 += 1;
  } else {
    i3 += 2;
  }
  return i3;
}

export function RunNormalDefCall():number {
  let count : number = 10000000;
  global_value = 0;
  let i3 : number = 1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let resourcesLength: number = resources.length;
  let startTime = ArkTools.timeInUs();
  let foo = DefaultFoo;
  for(let i=0;i<count;i++) {
    i3 = foo(resources, i, i3, resourcesLength);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunNormalDefCall:\t"+String(time)+"\tms");
  return time;
}
// RunNormalDefCall()

/********************* Different  parameters *********************/
function DifferentFoo(resources: Int32Array, i: number, i3: number, resourcesLength: number):number {
  if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
    i3 += 1.1;
  } else {
    i3 += 2.1;
  }
  return i3;
}

export function RunNormalDifferentCall():number {
  let count : number = 10000000;
  global_value = 0;
  let i3 : number = 1.1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let resourcesLength: number = resources.length;
  let startTime = ArkTools.timeInUs();
  let foo = DifferentFoo;
  for(let i=0;i<count;i++) {
    i3 = foo(resources, i, i3, resourcesLength);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunNormalDifferentCall:\t"+String(time)+"\tms");
  return time;
}
// RunNormalDifferentCall()

/************************* Variable  parameters *************************/
function VariableFoo(a?:number, b?:string, c?:boolean) {
  arr[global_value] += 1;
}
export function RunNormalVariableFCall():number {
  let count : number = 10000000;
  global_value = 1;
  let startTime = ArkTools.timeInUs();
  let foo = VariableFoo;
  for(let i=0;i<count;i++) {
    foo(1,"1",true)
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunNormalVariableFCall:\t"+String(time)+"\tms");
  return time;
}
// RunNormalVariableFCall()


let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunNormalCall();
  RunParameterlessCall();
  RunNormalDefCall();
  RunNormalDifferentCall();
  RunNormalVariableFCall();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunNormalCall();
RunParameterlessCall();
RunNormalDefCall();
RunNormalDifferentCall();
RunNormalVariableFCall();

print("Ts Method Call Is End, global_value value: \t" + String(global_value));
