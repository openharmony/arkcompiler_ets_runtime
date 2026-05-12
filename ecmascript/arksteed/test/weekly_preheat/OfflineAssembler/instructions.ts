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
'use strict';

// Interesting invariant, which we take advantage of: branching instructions
// always begin with "b", and no non-branching instructions begin with "b".
// Terminal instructions are "jmp" and "ret".

let macroInstructions = [
  'emit', 'addi', 'andi', 'lshifti', 'lshiftp', 'lshiftq', 'muli',
  'negi', 'negp', 'negq', 'noti', 'ori', 'rshifti', 'urshifti',
  'rshiftp', 'urshiftp', 'rshiftq', 'urshiftq', 'subi', 'xori', 'loadi',
  'loadis', 'loadb', 'loadbs', 'loadh', 'loadhs', 'storei', 'storeb',
  'loadd', 'moved', 'stored', 'addd', 'divd', 'subd', 'muld', 'sqrtd', 'ci2d',
  'fii2d', // usage: fii2d <gpr with least significant bits>, <gpr with most significant bits>, <fpr>
  'fd2ii', // usage: fd2ii <fpr>, <gpr with least significant bits>, <gpr with most significant bits>
  'fq2d', 'fd2q', 'bdeq', 'bdneq', 'bdgt', 'bdgteq', 'bdlt', 'bdlteq',
  'bdequn', 'bdnequn', 'bdgtun', 'bdgtequn', 'bdltun', 'bdltequn', 'btd2i',
  'td2i', 'bcd2i', 'movdz', 'pop', 'push', 'move', 'sxi2q', 'zxi2q',
  'nop', 'bieq', 'bineq', 'bia', 'biaeq', 'bib', 'bibeq', 'bigt',
  'bigteq', 'bilt', 'bilteq', 'bbeq', 'bbneq', 'bba', 'bbaeq',
  'bbb', 'bbbeq', 'bbgt', 'bbgteq', 'bblt', 'bblteq', 'btis',
  'btiz', 'btinz', 'btbs', 'btbz', 'btbnz', 'jmp',
  'baddio', 'baddis', 'baddiz', 'baddinz', 'bsubio', 'bsubis',
  'bsubiz', 'bsubinz', 'bmulio', 'bmulis', 'bmuliz', 'bmulinz',
  'borio', 'boris', 'boriz', 'borinz', 'break', 'call', 'ret', 'cbeq',
  'cbneq', 'cba', 'cbaeq', 'cbb', 'cbbeq', 'cbgt', 'cbgteq', 'cblt',
  'cblteq', 'cieq', 'cineq', 'cia', 'ciaeq', 'cib', 'cibeq', 'cigt',
  'cigteq', 'cilt', 'cilteq', 'tis', 'tiz', 'tinz', 'tbs', 'tbz',
  'tbnz', 'tps', 'tpz', 'tpnz', 'peek', 'poke', 'bpeq', 'bpneq',
  'bpa', 'bpaeq', 'bpb', 'bpbeq', 'bpgt', 'bpgteq', 'bplt', 'bplteq',
  'addp', 'mulp', 'andp', 'orp', 'subp', 'xorp', 'loadp', 'cpeq',
  'cpneq', 'cpa', 'cpaeq', 'cpb', 'cpbeq', 'cpgt', 'cpgteq', 'cplt',
  'cplteq', 'storep', 'btps', 'btpz', 'btpnz', 'baddpo', 'baddps',
  'baddpz', 'baddpnz', 'tqs', 'tqz', 'tqnz', 'bqeq', 'bqneq', 'bqa',
  'bqaeq', 'bqb', 'bqbeq', 'bqgt', 'bqgteq', 'bqlt', 'bqlteq', 'addq',
  'mulq', 'andq', 'orq', 'subq', 'xorq', 'loadq', 'cqeq', 'cqneq',
  'cqa', 'cqaeq', 'cqb', 'cqbeq', 'cqgt', 'cqgteq', 'cqlt', 'cqlteq',
  'storeq', 'btqs', 'btqz', 'btqnz', 'baddqo', 'baddqs', 'baddqz',
  'baddqnz', 'bo', 'bs', 'bz', 'bnz', 'leai', 'leap', 'memfence'
];

let armInstructions = ['clrbp', 'mvlbl'];
let x86Instructions = ['cdqi', 'idivi'];

let arm64Instructions = [
  'nopFixCortexA53Err835769', // nop on Cortex-A53 (nothing otherwise)
  'pcrtoaddr' // Address from PC relative offset - adr instruction
];

let riscInstructions = [
  'smulli', // Multiply two 32-bit words and produce a 64-bit word
  'oris', // Same, but for bitwise or.
  'subis', // Same, but for subtraction.
  'addis', // Add integers and set a flag.
  'addps' // addis but for pointers.
];

let mipsInstructions = [
  'la',
  'movn',
  'slt',
  'movz',
  'setcallreg',
  'sltu',
  'pichdr'
];

let cxxInstructions = [
  'cloopCrash', // no operands
  'cloopCallSlowPath', // operands: callTarget, currentFrame, currentPC
  'cloopCallJSFunction', // operands: callee
  'cloopCallSlowPathVoid', // operands: callTarget, currentFrame, currentPC
  'cloopCallNative', // operands: callee

  /* For debugging only:
   * Takes no operands but simply emits whatever follows in // comments as
   * a line of C++ code in the generated LLIntAssembly.h file.
   * This can be used to insert instrumentation into the interpreter loop to inspect variables of interest.
   * Do not leave these instructions in production code.
   */
  'cloopDo' // no operands
];

let instructions = macroInstructions.concat(
  x86Instructions,
  mipsInstructions,
  armInstructions,
  riscInstructions,
  arm64Instructions,
  cxxInstructions
);

export let instructionSet = new Set(instructions);

function hasFallThrough(instruction: string): boolean {
  return instruction !== 'ret' && instruction !== 'jmp';
}

function isBranch(instruction: string): boolean {
  return RegExp('/^b/').test(instruction);
}


