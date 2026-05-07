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
declare function print(arg: number): string;

function loop_for_of_string_max(str: string): string {
    let maxChar = "";
    let maxCode = 0;
    for (let c of str) {
        let code = c.charCodeAt(0);
        if (code > maxCode) {
            maxCode = code;
            maxChar = c;
        }
    }
    return maxChar;
}

ArkTools.arkSteedCompileAsync(loop_for_of_string_max);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_of_string_max("hello"));
print(loop_for_of_string_max("abc"));
