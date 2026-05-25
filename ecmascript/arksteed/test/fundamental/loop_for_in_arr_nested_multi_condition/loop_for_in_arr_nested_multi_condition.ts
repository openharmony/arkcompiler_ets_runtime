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

function loop_for_in_arr_nested_multi_condition(matrix: number[][]): number {
    let sumEven = 0;
    let sumOdd = 0;
    let countZero = 0;
    for (const rowKey in matrix) {
        let row = matrix[rowKey];
        for (const colKey in row) {
            let val = row[colKey];
            if (val === 0) {
                countZero += 1;
            } else if (val % 2 === 0) {
                sumEven += val;
            } else {
                sumOdd += val;
            }
        }
    }
    return sumEven * 10000 + sumOdd * 100 + countZero * 10 + 1;
}

ArkTools.arkSteedCompileSync(loop_for_in_arr_nested_multi_condition);


print(loop_for_in_arr_nested_multi_condition([[1, 2, 0], [4, 5, 6]]));
print(loop_for_in_arr_nested_multi_condition([[2, 4], [6, 8]]));
print(loop_for_in_arr_nested_multi_condition([[1, 3, 5]]));
print(loop_for_in_arr_nested_multi_condition([[0, 0], [0, 0]]));
