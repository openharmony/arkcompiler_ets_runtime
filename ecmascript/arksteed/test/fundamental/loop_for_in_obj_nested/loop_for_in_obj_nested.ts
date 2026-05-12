/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, the software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

declare function print(str: number):string;

function loop_for_in_obj_nested(outer: Record<string, Record<string, number>>): number {
    let sum = 0;
    for (const key in outer) {
        let inner = outer[key];
        for (const innerKey in inner) {
            sum += inner[innerKey];
        }
    }
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_for_in_obj_nested);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_obj_nested({a: {x: 1, y: 2}, b: {x: 3, y: 4}}));
print(loop_for_in_obj_nested({a: {x: 10}, b: {y: 20}, c: {z: 30}}));
print(loop_for_in_obj_nested({}));
print(loop_for_in_obj_nested({a: {}, b: {}}));
