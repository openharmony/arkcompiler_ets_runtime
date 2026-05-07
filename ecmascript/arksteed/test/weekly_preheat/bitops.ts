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
// import { ASSERT_NUMBER_EQ, ASSERT_FLOAT_EQ, ASSERT_TRUE, ASSERT_FALSE, ASSERT_EQ } from "../../../utils/assert";
// import { BenchmarkRunner } from "../../../utils/benchmarkTsSuite";
// declare function print(arg:string):string;

declare interface ArkTools{
  timeInUs(arg:any):number
}


function RunBitOp1() {
  let res: number = 0;
  // let func = ;
  let resources: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84])
  let start = ArkTools.timeInUs();
  let resourcesLength = resources.length - 1;
  for (let i = 0; i < 6000000; i++) {
    let bi3b: number = 0xE994;
    let b: number = resources[(i & res) & resourcesLength];
    res += 3 & (bi3b >> ((b << 1) & 14));
    res += 3 & (bi3b >> ((b >> 2) & 14));
    res += 3 & (bi3b >> ((b >> 5) & 6));
  }
  let end = ArkTools.timeInUs();
  print(""+res)
  let time = (end - start) / 1000
  print("Numerical Calculation - RunBitOp1:\t"+String(time)+"\tms");
  return time;
}



function RunBitOp2() {
  let res: number = 0;
  let start = ArkTools.timeInUs();
  for (let i = 0; i < 6000000; i++) {
    let b = i
    let m = 1, c = 0
    while (m < 0x100) {
      if ((b & m) != 0) {
        c += 1
      }
      m <<= 1
    }
    res += c
  }
  let end = ArkTools.timeInUs();
  print(""+res)
  let time = (end - start) / 1000
  print("Numerical Calculation - RunBitOp2:\t"+String(time)+"\tms");
  return time;
}

function RunBitOp3() {
  let res = 0;
  let start = ArkTools.timeInUs();
  for (let y = 0; y < 6000000; y++) {
    let x = y;
    let r = 0;
    while (x != 0) {
      x &= x - 1;
      ++r;
    }
    res += r;
  }
  let end = ArkTools.timeInUs();
  print(""+res);
  let time = (end - start) / 1000
  print("Numerical Calculation - RunBitOp3:\t"+String(time)+"\tms");
  return time;
}


function RunBitOp4() {
  let results: Int32Array = new Int32Array([3, 53, 76, 37, 82, 23, 66, 17, 82, 43, 77, 93, 28, 24, 85]);
  let resources: Int32Array = new Int32Array([12, 43, 56, 76, 89, 54, 45, 32, 35, 47, 46, 44, 21, 37, 84])
  let res: number = 0;
  let start = ArkTools.timeInUs();
  let resultsLength = results.length - 1;
  let resourcesLength = resources.length - 1;
  for (let i = 0; i < 6000000; i++) {
    res |= ~(1 << (resources[i & resourcesLength] ^ 31));
  }
  let end = ArkTools.timeInUs();
  print(""+res);
  let time = (end - start) / 1000
  print("Numerical Calculation - RunBitOp4:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunBitOp1()
  RunBitOp2()
  RunBitOp3()
  RunBitOp4()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunBitOp1()
RunBitOp2()
RunBitOp3()
RunBitOp4()
