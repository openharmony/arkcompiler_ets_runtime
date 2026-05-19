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
// Single loop: chain of if-else-if with continue/break actions
function loop_continue_break_chain(n)
{
    let sum = 0;
    let cnt1 = 0;
    let cnt2 = 0;
    let cnt3 = 0;
    let cnt4 = 0;

    for (let i = 0; i < n; i++) {
        if (i % 3 == 0) {
            cnt1 += 1;
            continue;
        } else if (i % 5 == 1) {
            cnt2 += 1;
            continue;
        } else if (i % 7 == 2) {
            cnt3 += 1;
            break;
        } else {
            cnt4 += 1;
        }
        sum += i;
    }

    return sum * 10000 + cnt1 * 1000 + cnt2 * 100 + cnt3 * 10 + cnt4;
}

ArkTools.arkSteedCompileSync(loop_continue_break_chain);


print(loop_continue_break_chain(25));
print(loop_continue_break_chain(10));
print(loop_continue_break_chain(5));
print(loop_continue_break_chain(3));
