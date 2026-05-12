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

function loop_for_in_obj_nested_break_continue(outer: Record<string, Record<string, number>>): number {
    let sum = 0;
    let outerCount = 0;
    for (const key in outer) {
        outerCount += 1;
        let inner = outer[key];
        let innerSum = 0;
        let skipCount = 0;
        for (const innerKey in inner) {
            if (inner[innerKey] < 0) {
                skipCount += 1;
                continue;
            }
            innerSum += inner[innerKey];
            if (innerSum > 10) {
                break;
            }
        }
        sum += innerSum * 100 + skipCount;
    }
    return sum * 10 + outerCount;
}

ArkTools.arkSteedCompileAsync(loop_for_in_obj_nested_break_continue);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_obj_nested_break_continue({a: {x: 1, y: 2}, b: {x: 3, y: 4}}));
print(loop_for_in_obj_nested_break_continue({a: {x: -1, y: -2}, b: {x: 3, y: 4}}));
print(loop_for_in_obj_nested_break_continue({a: {x: 5, y: 6, z: 7}}));
