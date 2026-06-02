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

// Test: throw.patternnoncoercible no-throw path.
// Object rest destructuring with a valid object → type check passes → no throw.

let cnt = 0;

function test(obj: any): void {
    const { a, ...rest } = obj;     // obj is an object → no throw
    if (a === 1 && Object.keys(rest).length === 2) {
        cnt++;
    }
}

ArkTools.arkSteedCompileSync(test);
test({ a: 1, b: 2, c: 3 });
print(cnt);
