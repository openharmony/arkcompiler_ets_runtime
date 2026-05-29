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

// 3-predecessor merge: the MIDDLE predecessor does NOT set a vreg.
// Directly reproduces the 3d-cube pattern where pred[1] (edge-split)
// contributes no value for a vreg.
//
//   Path A (pred0): sets x, y, z     -> first predecessor
//   Path B (pred1): sets x, y only   -> middle, missing z
//   Path C (pred2): sets x, y, z     -> last predecessor
//
// The Phi for z must have: input[0]=PathA_z, input[1]=undefined, input[2]=PathC_z
// Bug would give: input[1]=PathA_z (incorrectly inheriting cur from Path A)

function if_elseif_else_4(a: number, b: number, c: number, d: number): number
{
    let x: number = 0;
    let y: number = 0;
    let z: number = 0;

    if (a > 20) {
        x = b;
        y = c;
        z = 100;
    } else if (a > 10) {
        x = c;
        y = b;
        // z is NOT set here — the MIDDLE predecessor
    } else {
        x = d;
        y = b;
        z = 200;
    }

    // z Phi: pred0=100, pred1=0(undefined), pred2=200
    return x + y * 10 + z;
}

ArkTools.arkSteedCompileSync(if_elseif_else_4);

print(if_elseif_else_4(25, 2, 3, 4));   // a>20: x=b=2, y=c=3, z=100 -> 2+30+100=132
print(if_elseif_else_4(15, 2, 3, 4));   // a>10: x=c=3, y=b=2, z=0 -> 3+20+0=23
print(if_elseif_else_4(5, 7, 8, 9));    // else: x=d=9, y=b=7, z=200 -> 9+70+200=279
