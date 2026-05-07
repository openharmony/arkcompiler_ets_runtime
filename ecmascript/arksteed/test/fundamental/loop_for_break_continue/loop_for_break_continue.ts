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
// Test for loop with mixed break and continue
function for_break_continue(n, p, q)
{
    let sum = 0;
    let breakCount = 0;
    let continueCount = 0;

    for (let i = 0; i < n; i++) {
        if (i % p == 1) {
            breakCount += 1;
            break;
        }
        if (i % q == 1) {
            continueCount += 1;
            continue;
        }
        sum += i;
    }

    return sum * 100 + breakCount * 10 + continueCount;
}

ArkTools.arkSteedCompileAsync(for_break_continue);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(for_break_continue(5, 2, 3));
print(for_break_continue(10, 3, 4));
print(for_break_continue(20, 4, 5));
print(for_break_continue(30, 5, 6));
print(for_break_continue(50, 6, 7));
