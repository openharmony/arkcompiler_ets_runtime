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

// Test: throw.patternnoncoercible both paths in the same compiled function.
// Conditionally destructure with null (throw) or object (no-throw).

let okCnt = 0;
let throwCnt = 0;

function test(obj: any): number {
    try {
        const { ...rest } = obj;       // obj is {a:1} → no throw; obj is null → throw
        okCnt++;
        return Object.keys(rest).length;
    } catch (e) {
        throwCnt++;
        return -1;
    }
}

ArkTools.arkSteedCompileSync(test);
print(test({}));                       // no-throw path
print(test({ a: 1, b: 2 }));           // no-throw path
print(test([]));                       // no-throw path
print(test([1, 2, 3, 4, 5]));          // no-throw path
print(test(null));                     // throw path
print(test(undefined));                // throw path
print(test(42));                       // no-throw path
print(test(42n));                      // no-throw path

print(okCnt);
print(throwCnt);
