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
// Nested for loops with early break on outer loop
function nested_for_break_2(n, p)
{
    let sum = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            if (i % p === 1) {
                break;
            }
            sum += 1;
        }
    }

    return sum;
}

ArkTools.arkSteedCompileAsync(nested_for_break_2);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(nested_for_break_2(0, 2));
print(nested_for_break_2(3, 2));
print(nested_for_break_2(6, 2));
print(nested_for_break_2(10, 3));
print(nested_for_break_2(13, 2));
print(nested_for_break_2(16, 3));
print(nested_for_break_2(20, 4));
print(nested_for_break_2(23, 2));
print(nested_for_break_2(26, 3));
print(nested_for_break_2(30, 4));
print(nested_for_break_2(33, 5));
print(nested_for_break_2(36, 6));
