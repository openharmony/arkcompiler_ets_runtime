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

//! METHOD      compute
//! COUNT       BranchIfReferenceEqual  2
//! COUNT       CallCommonStub   GetStringFromConstPool  2
// Test: non-hole fact survives the loop back-edge (CloneForLoopHeader facts).
// The hole guards sit inside a loop body; the RecordNonHole fact recorded in the first
// iteration's blocks must remain valid for the loop header clone, and the guards in
// later iterations must never throw because x is initialized before the loop.

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_repeat_read_in_loop(): number {
    let x = 3;

    function compute(n: number): number {
        let s = 0;
        for (let i = 0; i < n; i++) {
            s += x + x;  // hole guards re-executed each iteration
        }
        return s;
    }
    ArkTools.arkSteedCompileSync(compute);

    return compute(3) + compute(0);
}

print(tdz_repeat_read_in_loop());
