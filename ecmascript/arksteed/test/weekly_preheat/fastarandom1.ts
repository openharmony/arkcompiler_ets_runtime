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
// Modify n value to change the difficulty of the benchmark
// suggesting using input = 25000000;
const input: number = 250000;

class AminoAcid {
  prob: number = 0.0;
  sym: number = 0;
  constructor(prob: number, sym: number) {
    this.prob = prob;
    this.sym = sym;
  }
}

const IM = 139968;
const IA = 3877;
const IC = 29573;
const SEED = 42;

let n: number = input;
const width: number = 60;

const iub: AminoAcid[] = [
  new AminoAcid(0.27, 97), // "a"),
  new AminoAcid(0.12, 99), // "c"),
  new AminoAcid(0.12, 103), // "g"),
  new AminoAcid(0.27, 116), // "t"),
  new AminoAcid(0.02, 66), // "B"),
  new AminoAcid(0.02, 68), // "D"),
  new AminoAcid(0.02, 72), // "H"),
  new AminoAcid(0.02, 75), // "K"),
  new AminoAcid(0.02, 77), // "M"),
  new AminoAcid(0.02, 78), // "N"),
  new AminoAcid(0.02, 82), // "R"),
  new AminoAcid(0.02, 83), // "S"),
  new AminoAcid(0.02, 86), // "V"),
  new AminoAcid(0.02, 87), // "W"),
  new AminoAcid(0.02, 89), // "Y"),
];

function binarySearch(rnd: number, arr: AminoAcid[]): number {
  let low = 0;
  let high = arr.length - 1;
  for (; low <= high;) {
    let middle = (low + high) >>> 1;
    if (arr[middle].prob >= rnd) {
      high = middle - 1;
    } else {
      low = middle + 1;
    }
  }
  return arr[high + 1].sym;
}

function accumulateProbabilities(acid: AminoAcid[]): void {
  for (let i = 1; i < acid.length; i++) {
    acid[i].prob += acid[i-1].prob;
  }
}

function randomFasta(buffer: number[], acid: AminoAcid[], n: number) {
  let cnt = n;
  accumulateProbabilities(acid);
  let pos = 0;
  let seed = SEED;
  while (cnt > 0) {
    let m = cnt > width ? width : cnt;
    let f = 1.0 / IM;
    let myrand = seed;
    for (let i = 0; i < m; i++) {
      myrand = (myrand * IA + IC) % IM;
      let r = myrand * f;
      buffer[pos] = binarySearch(r, acid);
      pos++;
      if (pos === buffer.length) {
        pos = 0;
      }
    }
    seed = myrand;
    buffer[pos] = 10;
    pos++;
    if (pos === buffer.length) {
      pos = 0;
    }
    cnt -= m;
  }
}

export function RunFastaRandom1() {
  const bufferSize: number = 256 * 1024;
  let buffer = new Array<number>();
  for (let i = 0; i < bufferSize; i++) {
    buffer.push(10);
  }
  let start = ArkTools.timeInUs();
  randomFasta(buffer, iub, 3*n);
  let end = ArkTools.timeInUs();
  let time = (end - start) / 1000
  print("Array Access - RunFastaRandom1:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunFastaRandom1()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunFastaRandom1()
