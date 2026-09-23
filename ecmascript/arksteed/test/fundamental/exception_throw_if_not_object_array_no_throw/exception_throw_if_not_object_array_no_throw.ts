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

//! METHOD      sumOverArray
//! HAS         BranchIfTaggedHeapObject       # fallback inline check chain is emitted
//! COUNT       BranchIfInt64Compare  2        # [ECMA_OBJECT_FIRST, ECMA_OBJECT_LAST] range
// Test: throw.ifnotobject no-throw path over a real array iterator (compiled code).
// The for-of loop guards each iterator.next() result with throw.ifnotobject.
// Whether the guard is elided by known-type facts or emitted inline, the compiled
// loop must iterate correctly and never throw.

declare function print(arg: number): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function sumOverArray(arr: number[]): number {
    let sum = 0;
    for (const v of arr) {  // each next() result guarded by throw.ifnotobject
        sum += v;
    }
    return sum;
}
ArkTools.arkSteedCompileSync(sumOverArray);

print(sumOverArray([1, 2, 3, 4]));
print(sumOverArray([5]));
