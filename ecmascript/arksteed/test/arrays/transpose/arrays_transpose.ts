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
function array_transpose(src: number[][], dest: number[][]): void
{
    for (let i = 0; i < 2; i++) {
        for (let j = 0; j < 2; j++) {
            dest[i][j] = src[j][i];
        }
    }
}

ArkTools.arkSteedCompileSync(array_transpose);


let src: number[][] = [[1, 2], [3, 4]];
let dest: number[][] = [[0, 0], [0, 0]];
array_transpose(src, dest);
print(dest[0][0]);
print(dest[0][1]);
print(dest[1][0]);
print(dest[1][1]);
