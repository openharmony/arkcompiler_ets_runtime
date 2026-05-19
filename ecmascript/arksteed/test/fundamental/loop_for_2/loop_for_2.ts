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
function loop_for_2(limit, p, q, r)
{
    let sum = 0;

    for (let i = 0; i < limit; i++) {
        sum += i * p * q + r;
    }
    for (let i = 0; i < limit; i++) {
        sum += i * q * r + p;
    }
    return sum;
}

ArkTools.arkSteedCompileSync(loop_for_2);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_for_2);
// print(res);

print(loop_for_2(0, 3, 1, 5));
print(loop_for_2(5, 7, 2, 3));
print(loop_for_2(10, 11, 3, 7));
