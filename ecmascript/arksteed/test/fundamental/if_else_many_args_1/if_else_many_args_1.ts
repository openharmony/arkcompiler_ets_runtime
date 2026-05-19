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
function if_else_many_args_1(x: number,
                             a: number,
                             b: number,
                             c: number,
                             d: number,
                             e: number,
                             f: number): number
{
    let v1: number;
    let v2: number;
    let v3: number;
    let v4: number;
    let v5: number;
    let v6: number;
    // if-else
    if (x > 0) {
        v1 = a;
        v2 = c;
        v3 = e;
        v4 = b;
        v5 = d;
        v6 = f;
    } else {
        v1 = b;
        v2 = d;
        v3 = f;
        v4 = e;
        v5 = c;
        v6 = a;
    }
    return v1 * 100_000 + v2 * 10_000 + v3 * 1_000 + v4 * 100 + v5 * 10 + v6;
}

ArkTools.arkSteedCompileSync(if_else_many_args_1);

// Spin loop: A temporary approach to wait for ArkSteed JIT compiler

let output = if_else_many_args_1(15, 1, 2, 3, 4, 5, 6);
print(output);
output = if_else_many_args_1(-15, 1, 2, 3, 4, 5, 6);
print(output);
