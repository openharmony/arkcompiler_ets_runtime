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
// Nested for loops: 4 levels
function nested_for_4(limit)
{
    let sum = 0;

    for (let i = 0; i < limit; i++) {
        for (let j = 0; j < limit; j++) {
            for (let k = 0; k < limit; k++) {
                for (let l = 0; l < limit; l++) {
                    sum += i + j + k + l;
                }
            }
        }
    }

    return sum;
}

ArkTools.arkSteedCompileAsync(nested_for_4);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = nested_for_4(3);
print(output);
output = nested_for_4(2);
print(output);
