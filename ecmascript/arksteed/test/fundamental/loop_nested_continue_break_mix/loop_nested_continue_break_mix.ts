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
// Nested loops: complex mix of continue and break with actions
function nested_continue_break_mix(n, p, q, r)
{
    let sum = 0;
    let outerBreaks = 0;
    let innerBreaks = 0;
    let continues = 0;

    for (let i = 0; i < n; i++) {
        for (let j = 0; j < n; j++) {
            if ((i + 1) * (j + 1) % p === 1) {
                continues += 1;
                continue;
            }
            if (i % q === 1) {
                innerBreaks += 1;
                break;
            }
            if ((i + 2) * (j + 2) % r === 1) {
                continues += 1;
                continue;
            }
            sum += (i + 1) * (j + 1);
        }
        if (i % (p + q) === r) {
            outerBreaks += 1;
            break;
        }
    }

    return sum * 1000 + outerBreaks * 100 + innerBreaks * 10 + continues;
}

ArkTools.arkSteedCompileSync(nested_continue_break_mix);


print(nested_continue_break_mix(0, 2, 3, 4));
print(nested_continue_break_mix(3, 2, 3, 4));
print(nested_continue_break_mix(6, 2, 4, 3));
print(nested_continue_break_mix(9, 2, 3, 5));
print(nested_continue_break_mix(12, 3, 4, 4));
print(nested_continue_break_mix(15, 2, 3, 4));
print(nested_continue_break_mix(18, 2, 2, 3));
print(nested_continue_break_mix(21, 3, 3, 3));
print(nested_continue_break_mix(24, 4, 5, 4));
print(nested_continue_break_mix(27, 2, 3, 5));
print(nested_continue_break_mix(30, 3, 2, 4));
print(nested_continue_break_mix(40, 2, 4, 3));
