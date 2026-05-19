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
// Deep nested loops with break at different levels
function nested_for_break_3(n, p)
{
    let sum = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            for (let k = 0; k < n; k++) {
                if ((i + 1) * (j + 1) % p === 1) {
                    break;
                }
                sum += 1;
            }
        }
    }

    return sum;
}

ArkTools.arkSteedCompileSync(nested_for_break_3);


print(nested_for_break_3(0, 5));
print(nested_for_break_3(3, 5));
print(nested_for_break_3(6, 5));
print(nested_for_break_3(10, 5));
print(nested_for_break_3(13, 7));
print(nested_for_break_3(16, 9));
print(nested_for_break_3(20, 11));
print(nested_for_break_3(23, 5));
print(nested_for_break_3(26, 7));
print(nested_for_break_3(30, 9));
print(nested_for_break_3(33, 11));
print(nested_for_break_3(36, 13));
