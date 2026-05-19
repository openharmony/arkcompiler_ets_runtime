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
function makeOutput12(v1: number,
                       v2: number,
                       v3: number,
                       v4: number,
                       v5: number,
                       v6: number,
                       v7: number,
                       v8: number,
                       v9: number,
                       v10: number,
                       v11: number,
                       v12: number,
                       v13: number,
                       v14: number,
                       v15: number): number {
    return v1 * 100_000_000_000_000_00 + v2 * 10_000_000_000_000_00 + v3 * 1_000_000_000_000_00 + v4 * 100_000_000_000_00 + v5 * 10_000_000_000_00 + v6 * 1_000_000_000_00 + v7 * 100_000_000_00 + v8 * 10_000_000_00 + v9 * 1_000_000_00 + v10 * 100_000_00 + v11 * 10_000_00 + v12 * 1_000_00 + v13 * 100_00 + v14 * 10_00 + v15;
}

function if_else_many_args_12(x: number,
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
                              m: number,
                              n: number,
                              o: number): number
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
    let v14: number;
    let v15: number;
    // if-else
    if (x > 0) {
        v1 = a;
        v2 = c;
        v3 = e;
        v4 = g;
        v5 = i;
        v6 = k;
        v7 = m;
        v8 = o;
        v9 = b;
        v10 = d;
        v11 = f;
        v12 = h;
        v13 = j;
        v14 = l;
        v15 = n;
    } else {
        v1 = b;
        v2 = d;
        v3 = f;
        v4 = h;
        v5 = j;
        v6 = l;
        v7 = n;
        v8 = a;
        v9 = c;
        v10 = e;
        v11 = g;
        v12 = i;
        v13 = k;
        v14 = m;
        v15 = o;
    }
    return makeOutput12(v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15);
}

ArkTools.arkSteedCompileSync(if_else_many_args_12);


let output = if_else_many_args_12(15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
print(output);
output = if_else_many_args_12(-15, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
print(output);
