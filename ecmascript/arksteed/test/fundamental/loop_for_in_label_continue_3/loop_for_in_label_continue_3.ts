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
function loop_for_in_label_continue_3(obj: {[key: string]: {[key: string]: number[]}}, skipVal: number): number {
    let result = 0;
    outer:
    for (const key1 in obj) {
        const inner = obj[key1];
        for (const key2 in inner) {
            const arr = inner[key2];
            for (let i = 0; i < arr.length; i++) {
                if (arr[i] === skipVal) {
                    result++;
                    continue outer;
                }
                result += arr[i];
            }
        }
    }
    return result;
}

ArkTools.arkSteedCompileSync(loop_for_in_label_continue_3);


const obj1: {[key: string]: {[key: string]: number[]}} = {
    "a": {"a1": [1, 2], "a2": [3, 4]},
    "b": {"b1": [5, 6], "b2": [7, 8]}
};

print(loop_for_in_label_continue_3(obj1, 5));
print(loop_for_in_label_continue_3(obj1, 3));
print(loop_for_in_label_continue_3(obj1, 1));
print(loop_for_in_label_continue_3(obj1, 100));
