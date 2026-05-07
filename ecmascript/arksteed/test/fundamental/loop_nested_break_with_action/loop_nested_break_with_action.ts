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
// Nested loops: if with action before break
function nested_break_with_action(n, p)
{
    let sum = 0;
    let breakCount = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            if ((i + 1) * (j + 1) % p === 1) {
                breakCount += 1;
                break;
            }
            sum += 1;
        }
    }

    return sum + breakCount * 50;
}

ArkTools.arkSteedCompileAsync(nested_break_with_action);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(nested_break_with_action(0, 2));
print(nested_break_with_action(2, 3));
print(nested_break_with_action(4, 2));
print(nested_break_with_action(6, 3));
print(nested_break_with_action(8, 4));
print(nested_break_with_action(10, 2));
print(nested_break_with_action(12, 3));
print(nested_break_with_action(14, 4));
print(nested_break_with_action(16, 5));
print(nested_break_with_action(18, 2));
print(nested_break_with_action(20, 3));
print(nested_break_with_action(30, 4));
