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
function loop_dowhile_4(m: number, n: number, p: number, q: number, callback: (x: number) => void): void {
    let i = 1;
    const limit = (p % 2 == 0) ? m : n;
    do {
        callback(q ** i);
        i++;
    } while (i <= limit);
}

ArkTools.arkSteedCompileAsync(loop_dowhile_4);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_dowhile_4);
let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let sum = 0;
sum = 0; print((loop_dowhile_4(2, 3, 4, 5, (x) => { sum += x; }), sum));
sum = 0; print((loop_dowhile_4(3, 4, 5, 6, (x) => { sum += x; }), sum));
sum = 0; print((loop_dowhile_4(0, 1, 2, 3, (x) => { sum += x; }), sum));
sum = 0; print((loop_dowhile_4(0, 1, 3, 2, (x) => { sum += x; }), sum));
