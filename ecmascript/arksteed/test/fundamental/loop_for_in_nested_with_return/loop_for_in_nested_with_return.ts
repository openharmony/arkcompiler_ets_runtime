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

function findFirstPositive(obj: Record<string, number>): number {
    for (const key in obj) {
        let val = obj[key];
        if (val > 0) {
            return val;
        }
    }
    return -1;
}

function loop_for_in_nested_with_return(obj1: Record<string, number>, obj2: Record<string, number>): number {
    let sum = 0;
    let v1 = findFirstPositive(obj1);
    if (v1 > 0) {
        sum += v1;
    }
    let v2 = findFirstPositive(obj2);
    if (v2 > 0) {
        sum += v2;
    }
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_for_in_nested_with_return);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_nested_with_return({a: -1, b: 2}, {c: -3, d: 4}));
print(loop_for_in_nested_with_return({x: 5}, {y: -1}));
print(loop_for_in_nested_with_return({a: -1}, {b: -2}));
print(loop_for_in_nested_with_return({}, {x: 10}));
