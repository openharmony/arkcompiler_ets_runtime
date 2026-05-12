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

function loop_for_in_arr_3(arr: number[]): number {
    let small = 0;
    let medium = 0;
    let large = 0;
    for (const key in arr) {
        let val = arr[key];
        if (val < 10) {
            small += 1;
        } else if (val < 100) {
            medium += 1;
        } else {
            large += 1;
        }
    }
    return small * 100 + medium * 10 + large;
}

ArkTools.arkSteedCompileAsync(loop_for_in_arr_3);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_arr_3([]));
print(loop_for_in_arr_3([5, 15, 150]));
print(loop_for_in_arr_3([1, 2, 10, 20, 100, 200]));
print(loop_for_in_arr_3([3, 30, 300, 4, 40, 400]));
