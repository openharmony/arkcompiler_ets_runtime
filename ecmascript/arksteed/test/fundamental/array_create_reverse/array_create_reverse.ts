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
function array_create_reverse(src: number[]): number[]
{
    let len = src.length;
    let result: number[] = [];
    for (let i = 0; i < len; i++) {
        result[i] = src[len - 1 - i];
    }
    return result;
}

ArkTools.arkSteedCompileSync(array_create_reverse);


let arr = array_create_reverse([1, 2, 3, 4, 5]);
print(arr[0]);
print(arr[1]);
print(arr[2]);
print(arr[3]);
print(arr[4]);
