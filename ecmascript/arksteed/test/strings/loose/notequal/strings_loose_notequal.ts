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

function string_loose_notequal(str1: string, str2: string): boolean {
    return str1 != str2;
}

ArkTools.arkSteedCompileSync(string_loose_notequal);


print(string_loose_notequal("hello", "world") ? 1 : 0);
print(string_loose_notequal("hello", "hello") ? 1 : 0);
print(string_loose_notequal("123", 123) ? 1 : 0);
