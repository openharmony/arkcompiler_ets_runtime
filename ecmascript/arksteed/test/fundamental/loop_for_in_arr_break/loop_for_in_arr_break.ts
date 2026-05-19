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

function loop_for_in_arr_break(arr: number[], target: number): number {
    for (const key in arr) {
        if (arr[key] === target) {
            return parseInt(key);
        }
    }
    return -1;
}

ArkTools.arkSteedCompileSync(loop_for_in_arr_break);


print(loop_for_in_arr_break([10, 20, 30, 40], 30));
print(loop_for_in_arr_break([10, 20, 30, 40], 25));
print(loop_for_in_arr_break([5], 5));
print(loop_for_in_arr_break([], 1));
