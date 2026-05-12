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
function if_else_many_args_9(x: number,
                             a: number,
                             b: number,
                             c: number,
                             d: number,
                             e: number,
                             f: number,
                             g: number,
                             h: number,
                             i: number,
                             j: number,
                             k: number,
                             l: number,
                             m: number): number
{
    let v1: number;
    let v2: number;
    let v3: number;
    let v4: number;
    let v5: number;
    let v6: number;
    let v7: number;
    let v8: number;
    let v9: number;
    let v10: number;
    let v11: number;
    let v12: number;
    let v13: number;
    // if-else
    if (x > 0) {
        v1 = a;
        v2 = c;
        v3 = e;
        v4 = g;
        v5 = i;
        v6 = k;
        v7 = m;
        v8 = b;
        v9 = d;
        v10 = f;
        v11 = h;
        v12 = j;
        v13 = l;
    } else {
        v1 = b;
        v2 = d;
        v3 = f;
        v4 = h;
        v5 = j;
        v6 = l;
        v7 = a;
        v8 = c;
        v9 = e;
        v10 = g;
        v11 = i;
        v12 = k;
        v13 = m;
    }
    return v1 * 1_000_000_000_000_0 + v2 * 100_000_000_000_0 + v3 * 10_000_000_000_0 + v4 * 1_000_000_000_0 + v5 * 100_000_000_0 + v6 * 10_000_000_0 + v7 * 1_000_000_0 + v8 * 100_000_0 + v9 * 10_000_0 + v10 * 1_000_0 + v11 * 100_0 + v12 * 10 + v13;
}

ArkTools.arkSteedCompileAsync(if_else_many_args_9);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let output = if_else_many_args_9(15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
print(output);
output = if_else_many_args_9(-15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13);
print(output);
