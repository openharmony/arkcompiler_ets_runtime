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
// declare function print(args: string): string;
declare interface ArkTools {
  timeInUs(args: any): number;
}

class Obj {
  num = 0;
  constructor (num: number) {
    this.num = num;
  }
}

function AllocateObj() {
  let count: number = 100000;
  let data: Obj[] = [new Obj(1), new Obj(1), new Obj(1), new Obj(1), new Obj(1)]// new Array();
  let resources: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84]);
  let  start: number = ArkTools.timeInUs();
  let resourcesLength = resources.length - 1;
  for (let i = 0; i < count; i++) {
    if ((resources[i & resourcesLength] & 1) == 0) {
      data[i & 4] = new Obj(i);
    } else {
      data[i & 4] = new Obj(i - 5);
    }
  }

  let end: number = ArkTools.timeInUs();
  let res: number = 0;
  for (let i = 0; i < data.length; i++) {
    res += data[i].num;
  }
  print(""+res);
  let time = (end - start) / 1000;
  print("Allocate Obj :\t" + time + "\tms");
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  AllocateObj();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

AllocateObj();

