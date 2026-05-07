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
function loop_for_6(limit: number, p: number, q: number, r: number): number
{
    let c1 = 0;
    let c2 = 0;
    let c3 = 0;
    for(var i = 0; i < limit; i++) {
        if (i % p === q) {
            c1 += i;
        } else if (i % q === r) {
            c2 += i;
        } else {
            c3 += i;
        }
    }
    return c1 * 1000000 + c2 * 1000 + c3;
}

ArkTools.arkSteedCompileAsync(loop_for_6);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_for_3);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(loop_for_6(0, 3, 2, 1));
print(loop_for_6(10, 5, 3, 2));
print(loop_for_6(20, 7, 5, 3));
