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

function loop_for_in_obj_arr_combined(obj: Record<string, number>, arr: number[]): number {
    let sumObj = 0;
    let sumArr = 0;
    for (const key in obj) {
        sumObj += obj[key];
    }
    for (const key in arr) {
        sumArr += arr[key];
    }
    return sumObj * 100 + sumArr;
}

ArkTools.arkSteedCompileAsync(loop_for_in_obj_arr_combined);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_obj_arr_combined({a: 1, b: 2}, [10, 20, 30]));
print(loop_for_in_obj_arr_combined({x: 5, y: 10}, [1, 2]));
print(loop_for_in_obj_arr_combined({}, [100]));
print(loop_for_in_obj_arr_combined({a: 3}, []));
