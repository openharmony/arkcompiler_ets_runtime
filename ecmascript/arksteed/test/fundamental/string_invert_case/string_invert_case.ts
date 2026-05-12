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

function string_invert_case(str: string): string {
    let result = "";
    for (let i = 0; i < str.length; i++) {
        let ch = str.charAt(i);
        if (ch >= "a" && ch <= "z") {
            result = result.concat(ch.toUpperCase());
        } else if (ch >= "A" && ch <= "Z") {
            result = result.concat(ch.toLowerCase());
        } else {
            result = result.concat(ch);
        }
    }
    return result;
}

ArkTools.arkSteedCompileAsync(string_invert_case);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(string_invert_case("Hello World"));
print(string_invert_case("ABC"));
print(string_invert_case("abc"));
