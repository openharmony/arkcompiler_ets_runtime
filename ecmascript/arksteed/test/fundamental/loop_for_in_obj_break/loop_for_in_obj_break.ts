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

function loop_for_in_obj_break(obj: Record<string, number>, targetKey: string): number {
    let sum = 0;
    for (const key in obj) {
        if (key === targetKey) {
            break;
        }
        sum += obj[key];
    }
    return sum;
}

ArkTools.arkSteedCompileAsync(loop_for_in_obj_break);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_obj_break({a: 1, b: 2, c: 3}, "a"));
print(loop_for_in_obj_break({a: 1, b: 2, c: 3}, "b"));
print(loop_for_in_obj_break({a: 1, b: 2, c: 3}, "c"));
print(loop_for_in_obj_break({a: 1, b: 2, c: 3}, "d"));
print(loop_for_in_obj_break({x: 10, y: 20, z: 30}, "y"));
