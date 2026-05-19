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

function loop_for_of_string_interleave(s1: string, s2: string): string {
    let result = "";
    let i = 0;
    for (let c of s1) {
        result = result.concat(c);
        if (i < s2.length) {
            result = result.concat(s2.charAt(i));
        }
        i++;
    }
    return result;
}

ArkTools.arkSteedCompileSync(loop_for_of_string_interleave);


print(loop_for_of_string_interleave("abc", "xyz"));
print(loop_for_of_string_interleave("ace", "bd"));
