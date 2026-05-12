/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
function loop_for_in_nested_continue(obj, skip) {
    let result = 0;
    for (const key in obj) {
        const arr = obj[key];
        for (const val of arr) {
            if (val === skip) {
                continue;
            }
            result += val;
        }
    }
    return result;
}

ArkTools.arkSteedCompileAsync(loop_for_in_nested_continue);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

const obj1 = {"a": [1, 2, 3], "b": [4, 5, 6], "c": [7, 8, 9]};
const obj2 = {"x": [10, 20], "y": [30, 40], "z": [50, 60]};

print(loop_for_in_nested_continue(obj1, 5));
print(loop_for_in_nested_continue(obj1, 3));
print(loop_for_in_nested_continue(obj1, 100));
print(loop_for_in_nested_continue(obj2, 40));
print(loop_for_in_nested_continue(obj2, 20));
