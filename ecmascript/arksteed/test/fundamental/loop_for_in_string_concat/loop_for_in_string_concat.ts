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

function loop_for_in_string_concat(obj: Record<string, number>): number {
    let result = "";
    for (const key in obj) {
        if (obj[key] > 0) {
            result = result + key + ",";
        }
    }
    return result.length;
}

ArkTools.arkSteedCompileAsync(loop_for_in_string_concat);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_in_string_concat({a: 1, b: 2, c: -1}));
print(loop_for_in_string_concat({x: -5, y: 10}));
print(loop_for_in_string_concat({}));
print(loop_for_in_string_concat({aa: 1, bb: 2}));
