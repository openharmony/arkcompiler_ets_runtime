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
// Nested loops: if with action before continue
function nested_continue_with_action(n, p)
{
    let sum = 0;
    let skipCount = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            if ((i + 1) * (j + 1) % p === 1) {
                skipCount += 1;
                continue;
            }
            sum += (i + 1) * (j + 1);
        }
    }

    return sum * 1000 + skipCount;
}

ArkTools.arkSteedCompileAsync(nested_continue_with_action);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(nested_continue_with_action(0, 3));
print(nested_continue_with_action(3, 3));
print(nested_continue_with_action(6, 3));
print(nested_continue_with_action(9, 4));
print(nested_continue_with_action(12, 5));
print(nested_continue_with_action(15, 3));
print(nested_continue_with_action(18, 4));
print(nested_continue_with_action(21, 6));
print(nested_continue_with_action(24, 4));
print(nested_continue_with_action(27, 3));
print(nested_continue_with_action(30, 7));
print(nested_continue_with_action(60, 5));
