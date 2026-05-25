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

// Basic 2-predecessor merge: both paths set the same set of vregs.
// Tests that a simple if-else with all vregs set on both sides produces
// correct Phis at the merge point.

function if_else_3(a: number, b: number, c: number): number
{
    let x: number = 0;
    let y: number = 0;
    let z: number = 0;

    if (a > 10) {
        x = b;
        y = c;
        z = 100;
    } else {
        x = c;
        y = b;
        z = 200;
    }

    // Both paths set x, y, z — Phis merge them correctly
    return x + y * 10 + z;
}

ArkTools.arkSteedCompileSync(if_else_3);

print(if_else_3(5, 2, 3));
print(if_else_3(15, 2, 3));
print(if_else_3(0, 7, 8));
