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
function if_else_many_args_5(x: number,
                             a: number,
                             b: number,
                             c: number,
                             d: number,
                             e: number,
                             f: number,
                             g: number,
                             h: number,
                             i: number): number
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
    // if-else
    if (x > 0) {
        v1 = a;
        v2 = c;
        v3 = e;
        v4 = g;
        v5 = i;
        v6 = b;
        v7 = d;
        v8 = f;
        v9 = h;
    } else {
        v1 = b;
        v2 = d;
        v3 = f;
        v4 = h;
        v5 = a;
        v6 = c;
        v7 = e;
        v8 = g;
        v9 = i;
    }
    return v1 * 1_000_000_000 + v2 * 100_000_000 + v3 * 10_000_000 + v4 * 1_000_000 + v5 * 100_000 + v6 * 10_000 + v7 * 1_000 + v8 * 100 + v9;
}

ArkTools.arkSteedCompileSync(if_else_many_args_5);


let output = if_else_many_args_5(15, 1, 2, 3, 4, 5, 6, 7, 8, 9);
print(output);
output = if_else_many_args_5(-15, 1, 2, 3, 4, 5, 6, 7, 8, 9);
print(output);
