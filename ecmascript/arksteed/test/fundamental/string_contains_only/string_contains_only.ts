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

function string_contains_only(str: string, chars: string): boolean {
    for (let i = 0; i < str.length; i++) {
        let ch = str.charAt(i);
        let found = false;
        for (let j = 0; j < chars.length; j++) {
            if (ch == chars.charAt(j)) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

ArkTools.arkSteedCompileAsync(string_contains_only);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(string_contains_only("abc", "abc") ? 1 : 0);
print(string_contains_only("abc", "ab") ? 1 : 0);
print(string_contains_only("aaa", "a") ? 1 : 0);
