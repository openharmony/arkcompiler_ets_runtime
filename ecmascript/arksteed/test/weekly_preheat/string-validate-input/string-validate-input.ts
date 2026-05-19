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

let letters: string[] = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'];

let endResult: string;
const TOBASE_NUMBER_4000: number = 4000;
const TOBASE_NUMBER_6: number = 6;
const TOBASE_NUMBER_2: number = 2;
const TOBASE_NUMBER_4: number = 4;
const TOBASE_NUMBER_5: number = 5;
const TOBASE_NUMBER_26: number = 26;
const TOBASE_NUMBER_9: number = 9;
const TOBASE_NUMBER_1000: number = 1000;
function doTest(): void {
  endResult = '';

  // make up email address
  for (let k = 0; k < TOBASE_NUMBER_4000; k++) {
    const username = makeName(TOBASE_NUMBER_6);
    let email: string;
    k % TOBASE_NUMBER_2 ? (email = username + '@mac.com') : (email = username + '(at)mac.com');

    // validate the email address
    let pattern = RegExp('/^[a-zA-Z0-9-._]+@[a-zA-Z0-9-_]+(.?[a-zA-Z0-9-_]*).[a-zA-Z]{2,3}$/');

    if (pattern.test(email)) {
      let r = email + ' appears to be a valid email address.';
      addResult(r);
    } else {
      let r = email + ' does NOT appear to be a valid email address.';
      addResult(r);
    }
  }

  // make up ZIP codes
  for (let s: number = 0; s < TOBASE_NUMBER_4000; s++) {
    let zipGood: boolean = true;
    let zip = makeNumber(TOBASE_NUMBER_4);
    s % TOBASE_NUMBER_2 ? (zip = zip + 'xyz') : (zip = zip.concat('7'));
    // validate the zip code
    for (let i: number = 0; i < zip.length; i++) {
      let ch = zip.charAt(i);
      let cArr: string[] = ['0', '1', '2', '3', '4', '5', '6', '7', '8', '9'];
      if (!cArr.includes(ch)) {
        zipGood = false;
        let r = zip + ' contains letters.';
        addResult(r);
      }
    }

    if (zipGood && zip.length > TOBASE_NUMBER_5) {
      zipGood = false;
      let r = zip + ' is longer than five characters.';
      addResult(r);
    }

    if (zipGood) {
      let r = zip + ' appears to be a valid ZIP code.';
      addResult(r);
    }
  }
}

function makeName(n: number): string {
  let tmp = '';
  for (let i: number = 0; i < n; i++) {
    let l = Math.floor(TOBASE_NUMBER_26 * Math.random());
    tmp += letters[l];
  }
  return tmp;
}

function makeNumber(n: number): string {
  let tmp = '';
  for (let i: number = 0; i < n; i++) {
    let l = Math.floor(TOBASE_NUMBER_9 * Math.random());
    tmp += String(l);
  }
  return tmp;
}

function addResult(r: string): void {
  endResult += '\n' + r;
}
/*
 *  @State
 *  @Tags Jetstream2
 */
class BenchMark {
  /*
   *  @Benchmark
   */
  runIteration(): void {
    doTest();
  }
}

declare interface ArkTools {
  timeInUs(args: number): number;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  const start: number = ArkTools.timeInUs();
  new BenchMark().runIteration();
  const end: number = ArkTools.timeInUs();
  print(`string-validate-input: ms = ${(end - start) / TOBASE_NUMBER_1000}`);

}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

const start: number = ArkTools.timeInUs();
new BenchMark().runIteration();
const end: number = ArkTools.timeInUs();
print(`string-validate-input: ms = ${(end - start) / TOBASE_NUMBER_1000}`);
