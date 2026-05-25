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
function loop_for_in_label_break_1(obj, target) {
    let result = 0;
    outer:
    for (const key in obj) {
        const arr = obj[key];
        for (const val of arr) {
            if (val === target) {
                break outer;
            }
            result += val;
        }
    }
    return result;
}

ArkTools.arkSteedCompileSync(loop_for_in_label_break_1);


const obj1 = {"a": [1, 2, 3], "b": [4, 5, 6]};
const obj2 = {"x": [10], "y": [20], "z": [30]};

print(loop_for_in_label_break_1(obj1, 5));
print(loop_for_in_label_break_1(obj1, 6));
print(loop_for_in_label_break_1(obj1, 1));
print(loop_for_in_label_break_1(obj2, 20));
print(loop_for_in_label_break_1(obj2, 100));
