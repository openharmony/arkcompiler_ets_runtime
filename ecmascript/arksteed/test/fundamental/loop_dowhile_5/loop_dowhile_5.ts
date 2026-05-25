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
declare function print(...args: any[]): void;

function loop_dowhile_5(m: number,
                        n: number,
                        p: number,
                        q: number,
                        callback?: ((x: number) => void) | undefined): void {
    let i = 1;
    const limit = (p % 2 == 0) ? m : n;
    callback ??= (x: number) => {
        print(`[default callback] x = ${x}`);
    };
    do {
        callback(q ** i);
        i++;
    } while (i <= limit);
}

ArkTools.arkSteedCompileSync(loop_dowhile_5);

// TODO: Replace the spin loop with:
// let res = ArkTools.waitJitCompileFinish(loop_dowhile_5);

let sum = 0;
sum = 0; print((loop_dowhile_5(2, 3, 4, 5, (x) => { sum += x; }), sum));
sum = 0; print((loop_dowhile_5(3, 4, 5, 6, (x) => { sum += x; }), sum));
sum = 0; print((loop_dowhile_5(0, 1, 2, 3, (x) => { sum += x; }), sum));
sum = 0; print((loop_dowhile_5(0, 1, 3, 2, (x) => { sum += x; }), sum));

sum = 0; print((loop_dowhile_5(2, 3, 4, 5), sum));
sum = 0; print((loop_dowhile_5(3, 4, 5, 6), sum));
sum = 0; print((loop_dowhile_5(0, 1, 2, 3), sum));
sum = 0; print((loop_dowhile_5(0, 1, 3, 2), sum));
