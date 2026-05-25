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
// Nested for loops with break/continue and early exit
function nested_for_break(n, p, q)
{
    let sum = 0;
    let count = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            count++;
            if (count > 10) {
                break;
            }
            if ((i + j) % p === 1) {
                continue;
            }
            sum += (i + 1) * (j + 1);
        }
        if (count % q === 1) {
            break;
        }
    }

    return sum;
}

ArkTools.arkSteedCompileSync(nested_for_break);


print(nested_for_break(0, 2, 11));
print(nested_for_break(3, 2, 11));
print(nested_for_break(6, 3, 13));
print(nested_for_break(9, 2, 13));
print(nested_for_break(12, 3, 17));
print(nested_for_break(15, 4, 17));
print(nested_for_break(18, 2, 19));
print(nested_for_break(21, 3, 19));
print(nested_for_break(24, 4, 23));
print(nested_for_break(27, 5, 23));
