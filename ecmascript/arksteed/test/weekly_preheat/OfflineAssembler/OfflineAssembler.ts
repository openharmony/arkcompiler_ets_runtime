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

import { resetAST } from './ast';
import type { Sequence } from './ast';
import { parse } from './parser';
import lowLevelInterpreter64 from './LowLevelInterpreter64';
import { expectedASTDumpedAsLines } from './expected';

('use strict');

const ONE_THOUSAND: number = 1000;

/**
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  ast: Sequence | null;

  constructor() {
    this.ast = null;
  }

  /**
   * @Setup
   */
  setup(): void {
//    lowLevelInterpreter64();
  }

  /**
   * @Benchmark
   */
  runIteration(): void {
    resetAST();
    this.ast = parse('LowLevelInterpreter64.asm')
  }

  validate(): void {
    let astDumpedAsLines: Array<string> = this.ast!.dump().split('\n');


    if (astDumpedAsLines.length !== expectedASTDumpedAsLines.length) {
      throw new Error(
        'Actual number of lines (' + astDumpedAsLines.length + ') differs from expected number of lines(' + expectedASTDumpedAsLines.length + ')'
      );
    }

    let index: number = 0;
    for (let line of astDumpedAsLines) {
      let expectedLine = expectedASTDumpedAsLines[index];
      if (line !== expectedLine) {
        throw new Error('Line #' + (index + 1) + ' differs.  Expected: "' + expectedLine + '", got "' + line + '"');
      }
      index += 1;
    }
  }
}

function run(): void {
  let startTime = ArkTools.timeInUs() / ONE_THOUSAND;

  let benchmark = new Benchmark();
  benchmark.setup();
  benchmark.runIteration();
  benchmark.validate();
  let endTime = ArkTools.timeInUs() / ONE_THOUSAND;
  print(`OfflineAssembler: ms = ${endTime - startTime}`);
}

lowLevelInterpreter64();

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  run();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

run();
