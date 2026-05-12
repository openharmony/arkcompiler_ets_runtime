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
// Mixed break and continue in nested loops
function mixed_break_continue(n, p, q)
{
    let sum = 0;
    let count = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            count++;
            if (count % p === 1) {
                break;
            }
            if ((i * j + 1) % q === 2) {
                continue;
            }
            sum += i + j;
        }
        if (count > 50) {
            break;
        }
    }

    return sum;
}

ArkTools.arkSteedCompileAsync(mixed_break_continue);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(mixed_break_continue(0, 2, 3));
print(mixed_break_continue(5, 3, 4));
print(mixed_break_continue(10, 4, 5));
print(mixed_break_continue(15, 5, 6));
