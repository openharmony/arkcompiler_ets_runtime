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

function main() {
  let i:number,j:number;
  let cnt = 0;
  let start = ArkTools.timeInUs();
  for (i = 0;i < 10000; i++) {
    for (j = 0; j < 10000; j++) {
      cnt = cnt + i + j;
    }
  }
  let end = ArkTools.timeInUs()
  // print(cnt)
  // print(end - start)
  print("test_for_plus1w:\t"+String((end - start)/1000)+"\tms");
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  main()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

main()