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
// Single loop: if with action before break
function loop_break_with_action(n, breakVal)
{
    let sum = 0;
    let breakCount = 0;

    for (let i = 0; i < n; i++) {
        if (i == breakVal) {
            breakCount += 1;
            break;
        }
        sum += i;
    }

    return sum + breakCount * 1000;
}

ArkTools.arkSteedCompileSync(loop_break_with_action);


print(loop_break_with_action(10, 5));
print(loop_break_with_action(10, 8));
print(loop_break_with_action(10, 0));
print(loop_break_with_action(10, 10));
