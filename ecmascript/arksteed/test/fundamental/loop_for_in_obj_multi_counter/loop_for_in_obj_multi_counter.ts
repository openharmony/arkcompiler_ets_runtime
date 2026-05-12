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

function loop_for_in_obj_multi_counter(obj: Record<string, number>): number {
    let sumVal = 0;
    let sumKeyLen = 0;
    let countPos = 0;
    let countNeg = 0;
    for (const key in obj) {
        sumVal += obj[key];
        sumKeyLen += key.length;
        if (obj[key] > 0) {
            countPos += 1;
        } else if (obj[key] < 0) {
            countNeg += 1;
        }
    }
    return sumVal * 10000 + sumKeyLen * 100 + countPos * 10 + countNeg;
}

ArkTools.arkSteedCompileAsync(loop_for_in_obj_multi_counter);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_obj_multi_counter({a: 1, bb: 2, ccc: -3}));
print(loop_for_in_obj_multi_counter({x: 10, y: -20, z: 30}));
print(loop_for_in_obj_multi_counter({}));
print(loop_for_in_obj_multi_counter({a: 5, b: -5, c: 0}));
