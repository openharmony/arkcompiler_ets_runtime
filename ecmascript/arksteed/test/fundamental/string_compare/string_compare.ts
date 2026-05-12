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

function string_compare(str1: string, str2: string): number {
    if (str1 == str2) {
        return 0;
    } else if (str1 > str2) {
        return 1;
    } else {
        return -1;
    }
}

ArkTools.arkSteedCompileAsync(string_compare);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(string_compare("apple", "apple"));
print(string_compare("banana", "apple"));
print(string_compare("apple", "banana"));
