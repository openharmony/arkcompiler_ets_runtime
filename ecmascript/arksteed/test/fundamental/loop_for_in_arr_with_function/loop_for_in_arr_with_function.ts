/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

declare function print(str: number):string;

function square(x: number): number {
    return x * x;
}

function loop_for_in_arr_with_function(arr: number[]): number {
    let sumSquares = 0;
    let countOdd = 0;
    for (const key in arr) {
        let val = arr[key];
        sumSquares += square(val);
        if (val % 2 !== 0) {
            countOdd += 1;
        }
    }
    return sumSquares * 10 + countOdd;
}

ArkTools.arkSteedCompileSync(loop_for_in_arr_with_function);


print(loop_for_in_arr_with_function([1, 2, 3, 4]));
print(loop_for_in_arr_with_function([5, 10, 15]));
print(loop_for_in_arr_with_function([2, 4, 6]));
print(loop_for_in_arr_with_function([]));
