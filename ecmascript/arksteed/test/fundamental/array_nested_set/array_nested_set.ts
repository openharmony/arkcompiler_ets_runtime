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
function array_nested_set(arr: number[][], i: number, j: number, value: number): void
{
    arr[i][j] = value;
}

ArkTools.arkSteedCompileSync(array_nested_set);


let arr: number[][] = [[1, 2], [3, 4], [5, 6]];
array_nested_set(arr, 0, 1, 20);
print(arr[0][1]);
