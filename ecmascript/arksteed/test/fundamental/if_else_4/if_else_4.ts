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

// 2-predecessor merge: one path does NOT set a vreg that the other sets.
// Tests that the Phi for a predecessor without the vreg gets undefinedValue_,
// not the value from another predecessor.

function if_else_4(a: number, b: number, c: number): number
{
    let x: number = 0;
    let y: number = 0;
    let z: number = 0;

    if (a > 10) {
        x = b;
        y = c;
        // z is NOT set here — this predecessor contributes no value for z
    } else {
        x = c;
        y = b;
        z = 300;  // only the else branch sets z
    }

    // At merge: z has a Phi with one input from else-branch,
    // the other from if-branch (which should be undefined/0, not c)
    return x + y * 10 + z;
}

ArkTools.arkSteedCompileSync(if_else_4);

print(if_else_4(5, 2, 3));
print(if_else_4(15, 2, 3));
print(if_else_4(0, 7, 8));
