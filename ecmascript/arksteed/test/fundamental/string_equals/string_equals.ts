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

function string_equals(str1: string, str2: string): boolean {
    return str1 == str2;
}

ArkTools.arkSteedCompileAsync(string_equals);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(string_equals("hello", "hello") ? 1 : 0);
print(string_equals("hello", "world") ? 1 : 0);
print(string_equals("", "") ? 1 : 0);
