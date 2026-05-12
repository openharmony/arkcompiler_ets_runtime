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

function loop_for_in_arr_nested(matrix: number[][]): number {
    let sum = 0;
    for (const rowKey in matrix) {
        let row = matrix[rowKey];
        for (const colKey in row) {
            sum += row[colKey];
        }
    }
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_for_in_arr_nested);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_arr_nested([[1, 2], [3, 4]]));
print(loop_for_in_arr_nested([[10], [20], [30]]));
print(loop_for_in_arr_nested([[1, 2, 3], [4, 5, 6]]));
print(loop_for_in_arr_nested([[]]));
