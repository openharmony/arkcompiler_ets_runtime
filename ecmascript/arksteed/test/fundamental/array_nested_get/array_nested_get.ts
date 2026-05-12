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
function array_nested_get(arr: number[][], i: number, j: number): number
{
    return arr[i][j];
}

ArkTools.arkSteedCompileAsync(array_nested_get);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let arr: number[][] = [[1, 2], [3, 4], [5, 6]];
print(array_nested_get(arr, 0, 0));
print(array_nested_get(arr, 1, 1));
print(array_nested_get(arr, 2, 0));
