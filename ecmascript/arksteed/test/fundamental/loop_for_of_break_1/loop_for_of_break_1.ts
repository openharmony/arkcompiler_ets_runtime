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
function loop_for_of_break_1(result: number[], arr: number[], r: number, s: number, t: number)
{
    for (let value of arr) {
        if (value % r === s) {
            break;
        }
        result.push(value);
    }
    for (let value of arr) {
        if (value % r === t) {
            break;
        }
        result.push(value * 10000);
    }
}

ArkTools.arkSteedCompileAsync(loop_for_of_break_1);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_for_of_break_1);
// print(res);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

const arr: number[] = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512];

function sum(arr: number[]) {
    let res = 0;
    for (let value of arr) {
        res += value;
    }
    return res;
}

const res1: number[] = [];
loop_for_of_break_1(res1, arr, 11, 7, 5);
print(sum(res1));

const res2: number[] = [];
loop_for_of_break_1(res2, arr, 13, 11, 7);
print(sum(res2));

const res3: number[] = [];
loop_for_of_break_1(res3, arr, 17, 13, 11);
print(sum(res3));

const res4: number[] = [];
loop_for_of_break_1(res4, arr, 513, 512, 511);
print(sum(res4));

const res5: number[] = [];
loop_for_of_break_1(res5, arr, 514, 513, 512);
print(sum(res5));
