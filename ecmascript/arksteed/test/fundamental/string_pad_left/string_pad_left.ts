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

function string_pad_left(str: string, targetLen: number): string {
    if (str.length >= targetLen) {
        return str;
    }
    let padding = "";
    for (let i = str.length; i < targetLen; i++) {
        padding = padding.concat(" ");
    }
    return padding.concat(str);
}

ArkTools.arkSteedCompileSync(string_pad_left);


print(string_pad_left("abc", 5));
print(string_pad_left("hello", 10));
