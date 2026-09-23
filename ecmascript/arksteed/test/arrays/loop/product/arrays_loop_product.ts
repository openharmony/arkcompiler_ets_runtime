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
function array_loop_product(arr: number[]): number
{
    let product = 1;
    let len = arr.length;

    for (let i = 0; i < len; i++) {
        product *= arr[i];
    }

    return product;
}

ArkTools.arkSteedCompileSync(array_loop_product);


let arr: number[] = [1, 2, 3, 4, 5];
print(array_loop_product(arr));
