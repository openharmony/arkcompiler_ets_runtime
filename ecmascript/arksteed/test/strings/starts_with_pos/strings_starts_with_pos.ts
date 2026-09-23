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

function string_starts_with_pos(str: string, prefix: string): boolean {
    return str.substring(0, prefix.length) == prefix;
}

ArkTools.arkSteedCompileSync(string_starts_with_pos);


print(string_starts_with_pos("hello world", "hello") ? 1 : 0);
print(string_starts_with_pos("hello world", "world") ? 1 : 0);
