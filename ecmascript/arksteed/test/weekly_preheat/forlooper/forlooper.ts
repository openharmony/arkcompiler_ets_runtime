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
declare function print(a:any,b?:any):string

//***********************  ArkTS 获取时间戳  **************************
declare interface ArkTools {
  timeInUs(args: number): number;
}
function currentTimestamp16():number {
  return ArkTools.timeInUs()
}
function foo() {
  var x:number = 0;
  for (var i:number = 0 ; i < 30000000; i++) {
    x = (i + 0.5) * (i + 0.5);
  }
  return x
}

function main() {
  // var start = ArkTools.timeInUs()
  var start = currentTimestamp16()
  for (let i:number=0;i<10;i++){
    var result = foo()
  }
  // var end = ArkTools.timeInUs()
  var end = currentTimestamp16()
  print("Forlooper_Obj: " + ((end - start)/1000) + "\tms");
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  main();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

main();
