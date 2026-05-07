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
function makeOutput4(v1: number,
                      v2: number,
                      v3: number,
                      v4: number,
                      v5: number,
                      v6: number,
                      v7: number): number {
    return v1 * 10_000_000 + v2 * 1_000_000 + v3 * 100_000 + v4 * 10_000 + v5 * 1_000 + v6 * 100 + v7;
}

function if_else_many_args_4(x: number,
                             a: number,
                             b: number,
                             c: number,
                             d: number,
                             e: number,
                             f: number,
                             g: number): string
{
    let v1: number;
    let v2: number;
    let v3: number;
    let v4: number;
    let v5: number;
    let v6: number;
    let v7: number;
    // if-else
    if (x > 0) {
        v1 = a;
        v2 = c;
        v3 = e;
        v4 = g;
        v5 = b;
        v6 = d;
        v7 = f;
    } else {
        v1 = b;
        v2 = d;
        v3 = f;
        v4 = a;
        v5 = c;
        v6 = e;
        v7 = g;
    }
    return makeOutput4(v1, v2, v3, v4, v5, v6, v7);
}

ArkTools.arkSteedCompileAsync(if_else_many_args_4);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = if_else_many_args_4(15, 1, 2, 3, 4, 5, 6, 7);
print(output);
output = if_else_many_args_4(-15, 1, 2, 3, 4, 5, 6, 7);
print(output);
