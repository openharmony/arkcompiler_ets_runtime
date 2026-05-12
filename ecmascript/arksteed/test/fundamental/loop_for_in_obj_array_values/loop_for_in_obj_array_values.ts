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

function loop_for_in_obj_array_values(obj: Record<string, number[]>): number {
    let total = 0;
    let count = 0;
    for (const key in obj) {
        let arr = obj[key];
        for (const arrKey in arr) {
            total += arr[arrKey];
            count += 1;
        }
    }
    return total * 10 + count;
}

ArkTools.arkSteedCompileAsync(loop_for_in_obj_array_values);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_obj_array_values({a: [1, 2], b: [3, 4, 5]}));
print(loop_for_in_obj_array_values({x: [10], y: [20], z: [30]}));
print(loop_for_in_obj_array_values({}));
print(loop_for_in_obj_array_values({a: []}));
