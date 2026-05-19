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

function string_is_numeric(str: string): boolean {
    let parts = str.split(".");
    if (parts.length > 2) {
        return false;
    }
    for (let i = 0; i < parts.length; i++) {
        if (parts[i].length == 0) {
            return false;
        }
        for (let j = 0; j < parts[i].length; j++) {
            let ch = parts[i].charAt(j);
            if (ch < "0" || ch > "9") {
                return false;
            }
        }
    }
    return true;
}

ArkTools.arkSteedCompileSync(string_is_numeric);


print(string_is_numeric("123") ? 1 : 0);
print(string_is_numeric("12.34") ? 1 : 0);
print(string_is_numeric("12a") ? 1 : 0);
print(string_is_numeric(".5") ? 1 : 0);
