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
//! COUNT       BranchIfReferenceEqual  3   # one guard per branch + one after the merge
// Test: non-hole fact survives a control-flow merge (NodeInfo::MergeWith intersection).
// Each branch reads x (guard fires per branch); after the join a third guard reads x again.
// The merged facts keep nonHole only when BOTH branches verified it, which holds here,
// and the final guard must not throw.

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function tdz_repeat_read_after_merge(): number {
    let x = 5;

    function compute(flag: boolean): number {
        let a = 0;
        if (flag) {
            a = x + 1;  // guard on branch 1
        } else {
            a = x + 2;  // guard on branch 2
        }
        return a + x;   // guard after merge
    }
    ArkTools.arkSteedCompileSync(compute);

    return compute(true) + compute(false);
}

print(tdz_repeat_read_after_merge());
