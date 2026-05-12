/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

const BASE_VALUE: number = 1000;
const LOOP_I_MAX_600000: number = 600000;
const NUMBER_100000: number = 100000;
const EXECUTION_MAX_80: number = 80;

let inDebug = false;
function log(str: string): void {
  if (inDebug) {
    print(str);
  }
}
function currentTimestamp13(): number {
  return ArkTools.timeInUs() / BASE_VALUE;
}

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  /*
   * @Benchmark
   */
  run(): Number {
    let bitwiseAndValue = 4294967296;
    for (let i = 0; i < LOOP_I_MAX_600000; i++) {
      bitwiseAndValue = bitwiseAndValue & i;
//      if (inDebug && i % NUMBER_100000 === 0) {
//        //log(`bitops-bitwise-and: i = ${i} bitwiseAndValue = ${bitwiseAndValue}`)
//      }
    }

    let result = bitwiseAndValue;
    let expected = 0;
    if (result !== expected) {
      throw new Error(`ERROR: bad result: expected ${expected} but got ${result}`);
    }

    return 1;
  }
}

function main() {
  let benchmark = new Benchmark();
  let startTime = currentTimestamp13();
  for (let i = 0; i < EXECUTION_MAX_80; i++) {
//    let startTimeInLoop = currentTimestamp13();
    let a = benchmark.run();
//    let endTimeInLoop = currentTimestamp13();
    //log(`bitops-bitwise-and: ms = ${endTimeInLoop - startTimeInLoop} i = ${i}`)
  }
  let endTime = currentTimestamp13();
  print(`bitops-bitwise-and: ms = ${endTime - startTime}`);
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


declare interface ArkTools {
  timeInUs(args: number): number;
}
