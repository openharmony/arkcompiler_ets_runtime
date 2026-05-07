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
let result = 0;

function addResult(value: number) {
    result += value;
}

function loop_break_6(n: number, r: number, s: number, t: number)
{
    for (let i = 1; i <= n; i++) {
        const value = 2 ** i;
        if (value % r === s) {
            break;
        }
        addResult(value);
    }
    for (let i = 1; i <= n; i++) {
        const value = 2 ** i;
        if (value % r === t) {
            break;
        }
        addResult(value * 10000);
    }
}

ArkTools.arkSteedCompileAsync(loop_break_6);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_break_6);
// print(res);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

result = 0;
loop_break_6(10, 11, 7, 5);
print(result);

result = 0;
loop_break_6(10, 13, 11, 7);
print(result);

result = 0;
loop_break_6(10, 17, 13, 11);
print(result);

result = 0;
loop_break_6(10, 513, 512, 511);
print(result);

result = 0;
loop_break_6(10, 514, 513, 512);
print(result);
