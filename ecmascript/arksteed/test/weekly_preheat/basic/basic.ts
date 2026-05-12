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

import { Lexer } from './lexer';
import { State, Parser } from './statement';


const ONE_THOUSAND: number = 1000;
const LOOP_COUNT: number = 120;

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  sp1 = `10 print "hello, world!"
  20 end`;

  sp2 = `10 let x = 0
  20 let x = x + 1
  30 print x
  40 if x < 10 then 20
  50 end`;

  sp3 = `10 print int(rnd * 100)
  20 end`;

  sp4 = `10 let value = int(rnd * 2000)
  20 print value
  30 if value <> 100 then 10
  40 end`;

  sp5 = `10 dim a(2000)
  20 for i = 2 to 2000
  30 let a(i) = 1
  40 next i
  50 for i = 2 to sqr(2000)
  60 if a(i) = 0 then 100
  70 for j = i ^ 2 to 2000 step i
  80 let a(j) = 0
  90 next j
  100 next i
  110 for i = 2 to 2000
  120 if a(i) = 0 then 140
  130 print i
  140 next i
  150 end`;

  /*
   * @Benchmark
   */
  runIteration(): void {
    let startTime = ArkTools.timeInUs() / ONE_THOUSAND;
    //let startTime = Date.now();
    for (let i = 0; i < LOOP_COUNT; i++) {
      this.run();
    }
    let endTime = ArkTools.timeInUs() / ONE_THOUSAND;
    //let endTime = Date.now();
    //console.log(`Basic: ms = ${endTime - startTime}`);
    print(`basic: ms = ${endTime - startTime}`);
  }

  run(): void {
    let tests = [this.sp1, this.sp2, this.sp3, this.sp4, this.sp5];
    for (let sp of tests) {
      this.prepare(sp);
    }
  }

  prepare(string: string): void {
    try {
      let program = new Parser(new Lexer(string)).parse();
      let state = new State(program);
      program.process(state);
    } catch (error) {}
  }
}

export { Benchmark };

declare interface ArkTools {
  timeInUs(args: number): number;
}

let benchmark = new Benchmark();

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  benchmark.runIteration();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

benchmark.runIteration();
