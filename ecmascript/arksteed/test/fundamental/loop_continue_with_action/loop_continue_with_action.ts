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
// Single loop: if with action before continue
function loop_continue_with_action(n, p, q)
{
    let sum = 0;
    let skipCount = 0;

    for (let i = 0; i < n; i++) {
        if (i % p == q) {
            skipCount += 1;
            continue;
        }
        sum += i;
    }

    return sum * 100 + skipCount;
}

ArkTools.arkSteedCompileSync(loop_continue_with_action);


print(loop_continue_with_action(10, 3, 1));
print(loop_continue_with_action(15, 5, 2));
print(loop_continue_with_action(20, 7, 3));
