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

// Commands to run individually and inspect ArkSteed JIT logs:
//   es2abc insufficient_profile_cases.ts --output insufficient_profile_cases.abc
//   ark_js_vm --asm-interpreter=true --compiler-enable-jit=true --compiler-enable-litecg=true \
//     --compiler-jit-hotness-threshold=100000000 --compiler-arksteed-deopt-on-insufficient-profile=true \
//     --open-ark-tools=true --compiler-enable-jit-lazy-deopt=true \
//     --log-level=info --log-components=jit --entry-point=insufficient_profile_cases \
//     insufficient_profile_cases.abc
//   ark_disasm insufficient_profile_cases.abc insufficient_profile_cases.disasm

// Hot path fills the first PGO slot, so the profile looks complete and
// nothing is marked insufficient. Code size is identical with the flag on/off.
function preLoopAndLoop(x: number, n: number): number {
    let a = x + 1;
    a = a * 2;
    a = a - 3;
    a = a | 5;
    a = a ^ 7;
    for (let i = 0; i < n; i++) {
        a += i;
        a = a * 3;
        a = a & 15;
    }
    a = a + 100;
    a = a * 2;
    return a;
}

// First PGO slot is the branch bytecode; it never writes, so slot 0 stays
// Undefined and the guard disables all insufficient-profile marking. The
// never-executed else branch is compiled normally (no eager deopt).
function coldBeforeLoop(x: number, n: number): number {
    let a = 0;
    if (x > 0) {
        a = 1;
    } else {
        // never executed
        // this DOES NOT eager deopt
        a = a + 10;
        a = a * 2;
        a = a - 3;
    }
    for (let i = 0; i < n; i++) {
        a += i;
    }
    return a;
}

// Same shape, but the first PGO slot is add2 on the hot path. add2 writes,
// the guard becomes false, and the cold else branch is marked insufficient
// and eager-deopt'd.
function coldBeforeLoop3(x: number, n: number): number {
    let a = x + 1;
    if (x > 0) {
        a = 1;
    } else {
        // never executed
        a = a + 10;
        a = a * 2;
        a = a - 3;
    }
    for (let i = 0; i < n; i++) {
        a += i;
    }
    return a;
}

for (let i = 0; i < 100; i++) {
    preLoopAndLoop(1, 1000);
    coldBeforeLoop(1, 1000);
    coldBeforeLoop3(5, 1000);
}

ArkTools.arkSteedCompileSync(preLoopAndLoop);
ArkTools.arkSteedCompileSync(coldBeforeLoop);
ArkTools.arkSteedCompileSync(coldBeforeLoop3);

print(preLoopAndLoop(1, 1000));
print(coldBeforeLoop(-1, 0));
print(coldBeforeLoop(1, 5));
print(coldBeforeLoop3(-1, 0));
print(coldBeforeLoop3(5, 5));

// Mass closure test: each candidate has a never-executed else branch. The
// first PGO slot is add2 (input + seed), so the cold branch is marked
// insufficient and eager-deopt'd for every compiled closure.
type Candidate = (takeHotPath: boolean, input: number) => number;

function makeCandidate(seed: number): Candidate {
    return function candidate(takeHotPath: boolean, input: number): number {
        let value = input + seed;
        if (takeHotPath) {
            value += 1;
        } else {
            value += 40; // never executed during warmup
        }
        return value + 1;
    };
}

let candidates: Candidate[] = [];
for (let i = 0; i < 100; i++) {
    let candidate = makeCandidate(i);
    for (let warmup = 0; warmup < 10; warmup++) {
        candidate(true, warmup);
    }
    candidates.push(candidate);
}

let compiled = 0;
for (let i = 0; i < candidates.length; i++) {
    if (ArkTools.arkSteedCompileSync(candidates[i])) {
        compiled++;
    }
}

print(compiled);
