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
function loop_for_of_label_break_3(cube: number[][][], target: number): number[] {
    let result: number[] = [];
    outer:
    for (let i = 0; i < cube.length; i++) {
        const matrix = cube[i];
        for (let j = 0; j < matrix.length; j++) {
            const row = matrix[j];
            for (let k = 0; k < row.length; k++) {
                if (row[k] === target) {
                    result.push(i);
                    result.push(j);
                    result.push(k);
                    break outer;
                }
            }
        }
    }
    return result;
}

ArkTools.arkSteedCompileSync(loop_for_of_label_break_3);


const cube1: number[][][] = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]];
const cube2: number[][][] = [[[1, 2, 3]], [[4, 5, 6]], [[7, 8, 9]]];

const r1 = loop_for_of_label_break_3(cube1, 5);
const r2 = loop_for_of_label_break_3(cube1, 8);
const r3 = loop_for_of_label_break_3(cube1, 1);
const r4 = loop_for_of_label_break_3(cube2, 7);
const r5 = loop_for_of_label_break_3(cube2, 100);

print(r1[0] + r1[1] * 10 + r1[2] * 100);
print(r2[0] + r2[1] * 10 + r2[2] * 100);
print(r3[0] + r3[1] * 10 + r3[2] * 100);
print(r4[0] + r4[1] * 10 + r4[2] * 100);
print(r5.length);
