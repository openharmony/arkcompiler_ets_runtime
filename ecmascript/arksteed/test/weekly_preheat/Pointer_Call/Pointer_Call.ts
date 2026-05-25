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

let global_value = 0;

function GenerateFakeRandomInteger(): Int32Array {
  let resource: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);
  return resource;
}

let arr: Int32Array = GenerateFakeRandomInteger()
/***** Without parameters *****/

/***************** With parameters *****************/
function Foo(a:number, b:number, c:number) {
  arr[global_value] += 1
}
function CallFoo(f:(n1: number, n2: number, n3: number) => void,a:number,b:number,c:number){
  f(a,b,c)
}
export function RunFunctionPtr():number {
  let count : number = 10000000;
  global_value = 0;
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++){
    CallFoo(Foo,1,2,i);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunFunctionPtr:\t"+String(time)+"\tms");
  return time
}
// RunFunctionPtr()

/***************************** Default  parameters *****************************/
function DefaultFoo(resources : Int32Array = arr, i: number = 1, i3: number = 1, resourcesLength: number = 1): number {
  if ((resources[i % i3 & (resourcesLength - 1)] & 1) == 0) {
    i3 += 1;
  } else {
    i3 += 2;
  }
  return i3;
}
function CallDefaultFoo(f:(n1: Int32Array, n2: number, n3: number, n4: number) => number,
                        resources : Int32Array = arr, i: number = 1, i3: number = 1, resourcesLength: number = 1): number{
  return f(resources, i, i3, resourcesLength)
}
export function RunDefaultfunctionPtr():number {
  let count : number = 10000000;
  global_value = 0;
  let i3 : number = 1;
  let resources : Int32Array = GenerateFakeRandomInteger();
  let resourcesLength: number = resources.length;
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++){
    i3 = CallDefaultFoo(DefaultFoo, resources, i, i3, resourcesLength);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunDefaultfunctionPtr:\t"+String(time)+"\tms");
  return time
}
// RunDefaultfunctionPtr()

/********************* Different  parameters *********************/
function DifferentFoo(a:number, b:string, c:boolean) {
  arr[global_value] += 1
}
function CallDifferentFoo(f:Function,a:number, b:string, c:boolean){
  f(a,b,c)
}
export function RunDifferentFunctionPtr():number {
  let count : number = 10000000;
  global_value = 0;
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++){
    CallDifferentFoo(DifferentFoo,1,"1",true);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunDifferentFunctionPtr:\t"+String(time)+"\tms");
  return time
}
// RunDifferentFunctionPtr()

/************************* Variable  parameters *************************/
function VariableFoo(a?:number, b?:string, c?:boolean) {
  arr[global_value] += 1
}
function CallVariableFoo(f:Function,a?:number, b?:string, c?:boolean){
  f(a,b,c)
}
export function RunVariableFunctionPtr():number {
  let count : number = 10000000;
  global_value = 0;
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++){
    CallVariableFoo(VariableFoo,1,"1",true);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunVariableFunctionPtr:\t"+String(time)+"\tms");
  return time
}
// RunVariableFunctionPtr()



let loopCountForPreheat = 1;

for (let i = 0; i < loopCountForPreheat; i++) {
  RunFunctionPtr();
  RunDefaultfunctionPtr();
  RunDifferentFunctionPtr();
  RunVariableFunctionPtr();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunFunctionPtr();
RunDefaultfunctionPtr();
RunDifferentFunctionPtr();
RunVariableFunctionPtr();


print("Ts Method Call Is End, global_value value: \t" + String(global_value));

