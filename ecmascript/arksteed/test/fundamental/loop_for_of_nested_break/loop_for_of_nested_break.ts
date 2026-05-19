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
function loop_for_of_nested_break(matrix: number[][], target: number): number {
    let result = 0;
    for (const row of matrix) {
        for (const val of row) {
            if (val === target) {
                break;
            }
            result += val;
        }
    }
    return result;
}

ArkTools.arkSteedCompileSync(loop_for_of_nested_break);


const matrix1: number[][] = [[1, 2, 3], [4, 5, 6], [7, 8, 9]];
const matrix2: number[][] = [[10, 20], [30, 40], [50, 60]];
const matrix3: number[][] = [[1], [2], [3], [4], [5]];

print(loop_for_of_nested_break(matrix1, 5));
print(loop_for_of_nested_break(matrix1, 9));
print(loop_for_of_nested_break(matrix1, 1));
print(loop_for_of_nested_break(matrix2, 40));
print(loop_for_of_nested_break(matrix2, 60));
print(loop_for_of_nested_break(matrix3, 3));
print(loop_for_of_nested_break(matrix3, 100));
