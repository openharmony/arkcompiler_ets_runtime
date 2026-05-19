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

function string_to_camel_case(str: string): string {
    let result = "";
    let nextUpper = false;
    for (let i = 0; i < str.length; i++) {
        let ch = str.charAt(i);
        if (ch == "_" || ch == "-") {
            nextUpper = true;
        } else if (nextUpper) {
            result = result.concat(ch.toUpperCase());
            nextUpper = false;
        } else {
            result = result.concat(ch.toLowerCase());
        }
    }
    return result;
}

ArkTools.arkSteedCompileSync(string_to_camel_case);


print(string_to_camel_case("hello_world"));
print(string_to_camel_case("foo-bar"));
