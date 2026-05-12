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
// Single loop: multiple continue conditions with actions
function loop_continue_multiple(n)
{
    let sum = 0;
    let c1 = 0;
    let c2 = 0;
    let c3 = 0;

    for (let i = 0; i < n; i++) {
        if (i % 3 == 0) {
            c1 += 1;
            continue;
        }
        if (i % 5 == 1) {
            c2 += 1;
            continue;
        }
        if (i % 7 == 2) {
            c3 += 1;
            continue;
        }
        sum += i;
    }

    return sum * 1000 + c1 * 100 + c2 * 10 + c3;
}

ArkTools.arkSteedCompileAsync(loop_continue_multiple);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_continue_multiple(10));
print(loop_continue_multiple(9));
print(loop_continue_multiple(12));
print(loop_continue_multiple(25));
