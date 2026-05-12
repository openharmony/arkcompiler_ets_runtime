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
// Nested loops: multiple continues in chain with actions
function nested_continue_chain(n, p, q, r)
{
    let sum = 0;
    let s1 = 0;
    let s2 = 0;
    let s3 = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            if (i % p === 1) {
                s1 += 1;
                continue;
            }
            if (j % q === 1) {
                s2 += 1;
                continue;
            }
            if ((i + j) % r === 1) {
                s3 += 1;
                continue;
            }
            sum += i + j;
        }
    }

    return sum * 1000 + s1 * 100 + s2 * 10 + s3;
}

ArkTools.arkSteedCompileAsync(nested_continue_chain);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(nested_continue_chain(0, 2, 3, 4));
print(nested_continue_chain(3, 2, 3, 4));
print(nested_continue_chain(6, 2, 3, 4));
print(nested_continue_chain(9, 2, 4, 3));
print(nested_continue_chain(12, 2, 3, 4));
print(nested_continue_chain(15, 5, 3, 3));
print(nested_continue_chain(18, 2, 4, 4));
print(nested_continue_chain(21, 4, 3, 4));
print(nested_continue_chain(24, 3, 5, 3));
print(nested_continue_chain(27, 2, 3, 4));
print(nested_continue_chain(30, 3, 3, 3));
print(nested_continue_chain(40, 2, 4, 5));
