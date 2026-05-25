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

function loop_for_of_string_split(str: string): number {
    let parts: string[] = [];
    let current = "";
    for (let c of str) {
        if (c == ",") {
            parts = parts.concat(current);
            current = "";
        } else {
            current = current.concat(c);
        }
    }
    parts = parts.concat(current);
    return parts.length;
}

ArkTools.arkSteedCompileSync(loop_for_of_string_split);


print(loop_for_of_string_split("a,b,c"));
print(loop_for_of_string_split("one"));
