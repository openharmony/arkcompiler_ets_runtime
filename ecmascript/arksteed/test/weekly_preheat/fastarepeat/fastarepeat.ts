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

let n: number = input;
const width: number = 60;

const aluString: string =
  "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" +
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" +
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" +
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" +
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" +
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" +
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";
let aluLen : number = aluString.length - 1;
let alu = new Int32Array(aluLen);
for (let i = 0; i < aluLen; i++) {
  alu[i] = aluString[i].charCodeAt(0);
}

function repeatFasta(geneLength: number, buffer: number[], gene2: Int32Array) : number {
  let pos = 0;
  let rpos = 0;
  let cnt = 500000;
  let lwidth = width;

  while (cnt > 0) {
    if (pos + lwidth > buffer.length) {
      pos = 0;
    }
    if (rpos + lwidth > geneLength) {
      rpos = rpos % geneLength;
    }
    if (cnt < lwidth) {
      lwidth = cnt;
    }
    for (let i = 0; i < lwidth; i++) {
      buffer[pos + i] = gene2[rpos + i];
    }

    buffer[pos + lwidth] = 10;
    pos += lwidth + 1;
    rpos += lwidth;
    cnt -= lwidth;
  }
  if (pos > 0 && pos < buffer.length) {
    buffer[pos] = 10;
  } else if (pos === buffer.length) {
    buffer[0] = 10;
  }

  let result: number = 0;
  for (let i = 0; i < buffer.length; i++) {
    result += buffer[i];
  }
  return result;
}

function RunFastaRepeat() {
  const bufferSize: number = 256 * 1024;
  let buffer = new Array<number>();
  for (let i = 0; i < bufferSize; i++) {
    buffer.push(10);
  }

  let gene2 = new Int32Array(2 * aluLen);
  for (let i = 0; i < gene2.length; i++) {
    gene2[i] = alu[i % aluLen];
  }
  let res: number = 0;
  let start = ArkTools.timeInUs();
  for (let i = 0; i < 500; i++) {
    res += repeatFasta(aluLen, buffer, gene2);
  }
  let end = ArkTools.timeInUs();
  let time = (end - start) / 1000
  print("Array Access - RunFastaRepeat:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunFastaRepeat()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunFastaRepeat()
