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

function loop_for_of_filter_map(arr: number[]): number {
    let filteredSum = 0;
    let mappedSum = 0;
    for (const val of arr) {
        if (val > 0) {
            filteredSum += val;
            mappedSum += val * 2;
        }
    }
    return filteredSum * 1000 + mappedSum;
}

ArkTools.arkSteedCompileSync(loop_for_of_filter_map);


print(loop_for_of_filter_map([1, -2, 3, -4, 5]));
print(loop_for_of_filter_map([10, 20]));
print(loop_for_of_filter_map([-1, -2]));
print(loop_for_of_filter_map([]));
