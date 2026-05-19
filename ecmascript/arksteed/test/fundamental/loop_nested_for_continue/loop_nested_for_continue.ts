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
// Nested loops with continue statements
function nested_for_continue(n, p)
{
    let sum = 0;
    let count = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            count++;
            if ((i + j) % p === 1) {
                continue;
            }
            sum += (i + 1) * (j + 1);
        }
    }

    return sum;
}

ArkTools.arkSteedCompileSync(nested_for_continue);


print(nested_for_continue(0, 5));
print(nested_for_continue(3, 5));
print(nested_for_continue(6, 5));
print(nested_for_continue(10, 5));
print(nested_for_continue(13, 7));
print(nested_for_continue(16, 5));
print(nested_for_continue(20, 7));
print(nested_for_continue(23, 11));
print(nested_for_continue(26, 5));
print(nested_for_continue(30, 7));
print(nested_for_continue(33, 11));
print(nested_for_continue(36, 13));
