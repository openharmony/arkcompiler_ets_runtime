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

function loop_for_of_return(arr: (number | null | undefined)[], target: number): number {
    for (const val of arr) {
        if (val === target) {
            return val as number;
        }
    }
    return -1;
}

ArkTools.arkSteedCompileAsync(loop_for_of_return);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_of_return([0, 1, 2, 3, 4], 3));
print(loop_for_of_return([null, 1, 2, 3, 4], 3));
print(loop_for_of_return([undefined, 1, 2, 3, 4], 3));
print(loop_for_of_return([1, 2, 3, 4, 5], 3));
print(loop_for_of_return([10, 20, 30], 25));
print(loop_for_of_return([5], 5));
print(loop_for_of_return([], 1));
