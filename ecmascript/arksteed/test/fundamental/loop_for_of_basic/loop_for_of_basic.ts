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

function loop_for_of_basic(arr: number[]): number {
    let count = 0;
    let sum = 0;
    for (const val of arr) {
        count += 1;
        sum += val;
    }
    return count * 1000 + sum;
}

ArkTools.arkSteedCompileSync(loop_for_of_basic);


print(loop_for_of_basic([]));
print(loop_for_of_basic([1, 2, 3]));
print(loop_for_of_basic([10, 20, 30, 40]));
