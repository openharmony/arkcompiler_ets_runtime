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
// Test do-while loop with break statement
function do_while_break(n, p, q)
{
    let sum = 0;
    let i = 0;
    do {
        if (i > n) {
            break;
        }
        sum += i * p + q;
        i++;
    } while (i < n);
    return sum;
}

ArkTools.arkSteedCompileAsync(do_while_break);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(do_while_break(0, 3, 1));
print(do_while_break(5, 7, 2));
print(do_while_break(10, 11, 3));
