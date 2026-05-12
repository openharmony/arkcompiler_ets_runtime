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
function array_create_transpose(mat: number[][]): number[][]
{
    let result: number[][] = [[0, 0], [0, 0]];
    for (let i = 0; i < 2; i++) {
        for (let j = 0; j < 2; j++) {
            result[i][j] = mat[j][i];
        }
    }
    return result;
}

ArkTools.arkSteedCompileAsync(array_create_transpose);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let mat: number[][] = [[1, 2], [3, 4]];
let matT = array_create_transpose(mat);
print(matT[0][0]);
print(matT[0][1]);
print(matT[1][0]);
print(matT[1][1]);
