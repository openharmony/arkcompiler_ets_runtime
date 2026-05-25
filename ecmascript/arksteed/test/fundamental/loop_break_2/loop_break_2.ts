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
function loop_break_2(n: number, r: number, s: number, t: number)
{
    let sum = 0;
    for (let i = 0; i < n; i++) {
        if (i === r % t) {
            break;
        }
        sum += i;
    }
    for (let i = 0; i < n; i++) {
        if (i === s % t) {
            break;
        }
        sum += i * 100;
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_break_2);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_break_2);
// print(res);

print(loop_break_2(10, 10, 21, 11));
print(loop_break_2(10, 10, 20, 11));
print(loop_break_2(10, 20, 21, 11));
print(loop_break_2(10, 20, 30, 11));
print(loop_break_2(10, 30, 40, 11));
