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
function loop_for_in_label_break_2(obj, target) {
    let result = 0;
    outer:
    for (const key1 in obj) {
        const inner = obj[key1];
        for (const key2 in inner) {
            const arr = inner[key2];
            for (const val of arr) {
                if (val === target) {
                    break outer;
                }
                result += val;
            }
        }
    }
    return result;
}

ArkTools.arkSteedCompileAsync(loop_for_in_label_break_2);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

const obj1 = {
    "a": {"a1": [1, 2], "a2": [3, 4]},
    "b": {"b1": [5, 6], "b2": [7, 8]}
};

print(loop_for_in_label_break_2(obj1, 5));
print(loop_for_in_label_break_2(obj1, 8));
print(loop_for_in_label_break_2(obj1, 1));
print(loop_for_in_label_break_2(obj1, 100));
