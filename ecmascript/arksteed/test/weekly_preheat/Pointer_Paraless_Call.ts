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

/***************** No parameters *****************/
function ParameterlessFoo() : Obj {
  let res: Obj[] = [new Obj(0), new Obj(0), new Obj(0), new Obj(0), new Obj(0)];
  let resources: Obj[] = GenerateFakeRandomObject();
  for (let i = 0; i < 200; i++) {
    res[i % 5] = resources[i % 15];
  }
  return res[1];
}
function CallParameterlessFoo(f:() => Obj) : Obj {
  return f();
}
export function RunParameterlessFunctionPtr():number {
  let count : number = 10000;
  global_value = 0;
  let i3 : Obj = new Obj(1);
  let startTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++){
    i3 = CallParameterlessFoo(ParameterlessFoo);
  }
  let midTime = ArkTools.timeInUs();
  for(let i=0;i<count;i++) {
  }
  let endTime = ArkTools.timeInUs();
  let time = ((midTime - startTime) - (endTime - midTime)) / 1000
  print("Function Call - RunParameterlessFunctionPtr:\t"+String(time)+"\tms");
  return time
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunParameterlessFunctionPtr()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunParameterlessFunctionPtr()

print("Ts Method Call Is End, global_value value: \t" + String(global_value));

