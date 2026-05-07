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
function loop_for_of_label_continue(cube: number[][][], skipVal: number): number[] {
    let result: number[] = [];
    outer:
    for (let i = 0; i < cube.length; i++) {
        const matrix = cube[i];
        for (let j = 0; j < matrix.length; j++) {
            const row = matrix[j];
            for (let k = 0; k < row.length; k++) {
                if (row[k] === skipVal) {
                    result.push(i);
                    continue outer;
                }
                result.push(row[k]);
            }
        }
    }
    return result;
}

ArkTools.arkSteedCompileAsync(loop_for_of_label_continue);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

const cube1: number[][][] = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]];
const cube2: number[][][] = [[[1, 2, 3]], [[4, 5, 6]], [[7, 8, 9]]];

const r1 = loop_for_of_label_continue(cube1, 5);
const r2 = loop_for_of_label_continue(cube1, 3);
const r3 = loop_for_of_label_continue(cube2, 4);
const r4 = loop_for_of_label_continue(cube2, 8);
const r5 = loop_for_of_label_continue(cube2, 100);

print(r1.length);
print(r1[0]);
print(r2.length);
print(r2[r2.length - 1]);
print(r3.length);
print(r3[r3.length - 1]);
print(r4.length);
print(r5.length);
