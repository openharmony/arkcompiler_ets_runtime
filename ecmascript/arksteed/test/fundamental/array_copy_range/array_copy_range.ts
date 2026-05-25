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
function array_copy_range(src: number[], dest: number[], start: number, end: number): void
{
    let idx = 0;
    for (let i = start; i < end; i++) {
        dest[idx] = src[i];
        idx++;
    }
}

ArkTools.arkSteedCompileSync(array_copy_range);


let src: number[] = [1, 2, 3, 4, 5];
let dest: number[] = [0, 0, 0];
array_copy_range(src, dest, 1, 4);
print(dest[0]);
print(dest[1]);
print(dest[2]);
