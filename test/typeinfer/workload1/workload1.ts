/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

declare function AssertType(value: any, type: string): void;
declare function print(arg: any): string;
declare interface ArkTools{
    timeInUs(arg: any): number
}

class Obj {
  value: number = 0
  constructor(value: number) {
    this.value = value
  }
};

let global_value = 0;

function GenerateFakeRandomInteger(): Int32Array {
  let resource: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);
  return resource;
}

let arr: Int32Array = GenerateFakeRandomInteger()

function DifferentFoo(a: number, b: string, c: boolean) {
  AssertType(global_value, "int");
  AssertType(arr, "Int32Array");
  arr[global_value] += 1;
}

function CallDifferentFoo(f: Function, a: number, b: string, c: boolean) {
  f(a, b, c)
}

export function RunDifferentFunctionPtr():number {
  let count : number = 10000000;
  global_value = 0;
  AssertType(global_value, "int");
  let startTime = ArkTools.timeInUs();
  
  for(let i = 0; i < count; i++) {
    CallDifferentFoo(DifferentFoo, 1, "1", true);
  }
  
  let endTime = ArkTools.timeInUs();
  let time = (endTime - startTime) / 1000;
  print("Function Call - RunDifferentFunctionPtr:\t" + String(time) + "\tms");
  return time
}

RunDifferentFunctionPtr()
