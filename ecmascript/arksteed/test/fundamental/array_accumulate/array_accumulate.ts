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
function array_accumulate(src: number[], dest: number[]): void
{
    let len = src.length;
    let sum = 0;
    for (let i = 0; i < len; i++) {
        sum += src[i];
        dest[i] = sum;
    }
}

ArkTools.arkSteedCompileAsync(array_accumulate);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let src: number[] = [1, 2, 3, 4, 5];
let dest: number[] = [0, 0, 0, 0, 0];
array_accumulate(src, dest);
print(dest[0]);
print(dest[1]);
print(dest[2]);
print(dest[3]);
print(dest[4]);
