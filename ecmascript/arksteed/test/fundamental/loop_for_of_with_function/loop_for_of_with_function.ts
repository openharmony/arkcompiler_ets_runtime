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

function double(x: number): number {
    return x * 2;
}

function loop_for_of_with_function(arr: number[]): number {
    let sum = 0;
    for (const val of arr) {
        sum += double(val);
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_for_of_with_function);


print(loop_for_of_with_function([1, 2, 3, 4]));
print(loop_for_of_with_function([5, 10]));
print(loop_for_of_with_function([]));
