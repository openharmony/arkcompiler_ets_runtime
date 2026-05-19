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
function loop_dowhile_3(m: number, n: number, p: number, q: number): number {
    let sum = 0;
    let i = 1;
    do {
        sum += q ** i;
        i++;
    } while (i <= ((p % 2 == 0) ? m : n));
    return sum;
}

ArkTools.arkSteedCompileSync(loop_dowhile_3);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_dowhile_3);

print(loop_dowhile_3(2, 3, 4, 5));
print(loop_dowhile_3(3, 4, 5, 6));
print(loop_dowhile_3(0, 1, 2, 3));
print(loop_dowhile_3(0, 1, 3, 2));
