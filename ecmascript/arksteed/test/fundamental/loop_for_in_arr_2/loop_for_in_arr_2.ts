/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str: number):string;

function loop_for_in_arr_2(arr: number[]): number {
    let sumEven = 0;
    let sumOdd = 0;
    for (const key in arr) {
        let idx = parseInt(key);
        if (idx % 2 === 0) {
            sumEven += arr[idx];
        } else {
            sumOdd += arr[idx];
        }
    }
    return sumEven * 100 + sumOdd;
}

ArkTools.arkSteedCompileAsync(loop_for_in_arr_2);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_arr_2([]));
print(loop_for_in_arr_2([10, 20, 30]));
print(loop_for_in_arr_2([1, 2, 3, 4, 5]));
print(loop_for_in_arr_2([100, 200, 300, 400]));
