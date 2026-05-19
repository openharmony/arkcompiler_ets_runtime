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
// Test nested loops with break chain at different levels
function nested_break_chain(n, p, q, r)
{
    let sum = 0;
    let outerBreaks = 0;
    let innerBreaks = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            if (i % p === 1) {
                innerBreaks += 1;
                break;
            }
            if ((i + 1) * (j + 1) % q === 1) {
                outerBreaks += 1;
                break;
            }
            sum += 1;
        }
        if (i % r === 2) {
            outerBreaks += 1;
            break;
        }
    }

    return sum * 100 + outerBreaks * 10 + innerBreaks;
}

ArkTools.arkSteedCompileSync(nested_break_chain);


print(nested_break_chain(0, 2, 3, 4));
print(nested_break_chain(3, 2, 3, 4));
print(nested_break_chain(6, 2, 3, 4));
print(nested_break_chain(6, 2, 3, 3));
print(nested_break_chain(8, 2, 4, 3));
print(nested_break_chain(10, 3, 2, 3));
print(nested_break_chain(12, 2, 3, 4));
print(nested_break_chain(14, 4, 3, 2));
print(nested_break_chain(16, 2, 2, 3));
print(nested_break_chain(18, 2, 4, 4));
