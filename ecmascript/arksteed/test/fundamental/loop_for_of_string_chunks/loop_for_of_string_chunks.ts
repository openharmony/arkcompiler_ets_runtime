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

function loop_for_of_string_chunks(str: string): number {
    let chunks: string[] = [];
    let current = "";
    for (let c of str) {
        current = current.concat(c);
        if (current.length == 2) {
            chunks = chunks.concat(current);
            current = "";
        }
    }
    if (current.length > 0) {
        chunks = chunks.concat(current);
    }
    return chunks.length;
}

ArkTools.arkSteedCompileAsync(loop_for_of_string_chunks);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_of_string_chunks("abcd"));
print(loop_for_of_string_chunks("abcde"));
print(loop_for_of_string_chunks("a"));
