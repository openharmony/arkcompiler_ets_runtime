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

function loop_for_in_obj_arr_with_condition(obj: Record<string, number>, arr: number[]): number {
    let sumObj = 0;
    let sumArr = 0;
    for (const key in obj) {
        if (obj[key] > 0) {
            sumObj += obj[key];
        }
    }
    for (const key in arr) {
        if (arr[key] % 2 === 0) {
            sumArr += arr[key];
        }
    }
    return sumObj * 100 + sumArr;
}

ArkTools.arkSteedCompileSync(loop_for_in_obj_arr_with_condition);


print(loop_for_in_obj_arr_with_condition({a: 1, b: -2, c: 3}, [2, 3, 4, 5]));
print(loop_for_in_obj_arr_with_condition({x: -5, y: -10}, [1, 3, 5]));
print(loop_for_in_obj_arr_with_condition({}, [2, 4]));
