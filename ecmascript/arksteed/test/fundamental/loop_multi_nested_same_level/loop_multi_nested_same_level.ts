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
// Multiple independent nested loops at same level
function multi_nested_same_level(n)
{
    let sum1 = 0;
    let sum2 = 0;
    let sum3 = 0;

    // First nested loop
    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            sum1 += i + j;
        }
    }

    // Second nested loop (independent)
    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            sum2 += (i + 1) * (j + 1);
        }
    }

    // Third nested loop (independent)
    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            sum3 += i - j;
        }
    }

    return sum1 + sum2 + sum3;
}

ArkTools.arkSteedCompileAsync(multi_nested_same_level);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

for (let i = 0; i < 10; i++) {
    print(multi_nested_same_level(i))
}
