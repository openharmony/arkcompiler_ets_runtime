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

// 3-predecessor merge: the FIRST predecessor does NOT set a vreg.
// The Phi is created at predIndex=2 (last predecessor).
// Tests that pred0 (no vreg) gets undefinedValue_, not the value from pred1.
//
//   Path A (pred0): sets x, y only   -> first, no z
//   Path B (pred1): sets x, y, z     -> middle, has z=100
//   Path C (pred2): sets x, y, z     -> last, has z=200
//
// Phi for z must have: input[0]=undefined, input[1]=100, input[2]=200
// Bug would give: input[0]=100 (incorrectly inheriting cur=100 from Path B)

function if_elseif_else_5(a: number, b: number, c: number, d: number): number
{
    let x: number = 0;
    let y: number = 0;
    let z: number = 0;

    if (a > 20) {
        x = b;
        y = c;
        // z is NOT set here — the FIRST predecessor
    } else if (a > 10) {
        x = c;
        y = b;
        z = 100;
    } else {
        x = d;
        y = b;
        z = 200;
    }

    // z Phi: pred0=0(undefined), pred1=100, pred2=200
    return x + y * 10 + z;
}

ArkTools.arkSteedCompileSync(if_elseif_else_5);

print(if_elseif_else_5(25, 2, 3, 4));   // a>20: x=b=2, y=c=3, z=0 -> 2+30+0=32
print(if_elseif_else_5(15, 2, 3, 4));   // a>10: x=c=3, y=b=2, z=100 -> 3+20+100=123
print(if_elseif_else_5(5, 7, 8, 9));    // else: x=d=9, y=b=7, z=200 -> 9+70+200=279
