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

function string_capitalize(str: string): string {
    if (str.length == 0) {
        return "";
    }
    let first = str.substring(0, 1).toUpperCase();
    let rest = str.substring(1).toLowerCase();
    return first.concat(rest);
}

ArkTools.arkSteedCompileSync(string_capitalize);


print(string_capitalize("hello"));
print(string_capitalize("WORLD"));
print(string_capitalize("hELLO"));
