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
function loop_for_of_label_break_2(cube: number[][][], target: number): number {
    let result = 0;
    outer:
    for (const matrix of cube) {
        for (const row of matrix) {
            for (const val of row) {
                if (val === target) {
                    break outer;
                }
                result += val;
            }
        }
    }
    return result;
}

ArkTools.arkSteedCompileSync(loop_for_of_label_break_2);


const cube1: number[][][] = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]];
const cube2: number[][][] = [[[1, 2, 3]], [[4, 5, 6]]];

print(loop_for_of_label_break_2(cube1, 5));
print(loop_for_of_label_break_2(cube1, 8));
print(loop_for_of_label_break_2(cube1, 1));
print(loop_for_of_label_break_2(cube2, 4));
print(loop_for_of_label_break_2(cube2, 100));
