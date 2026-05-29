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

// 3-predecessor merge with disjoint vreg subsets.
// Each predecessor sets a DIFFERENT subset of vregs.
// This is the most comprehensive test of MergeFrameState Phi creation.
//
//   Path A (pred0): sets r, s                 (no t, no u)
//   Path B (pred1): sets s, t                 (no r, no u)
//   Path C (pred2): sets r, t                 (no s)
//   Common:         u always set              (no vreg conflict)
//
// Expected Phis:
//   r: pred0=v1,  pred1=undefined, pred2=v5
//   s: pred0=v2,  pred1=v3,        pred2=undefined
//   t: pred0=undefined, pred1=v4,  pred2=v6
//
// Without the fix, Phis would incorrectly copy values between predecessors
// that never contributed them.

function if_elseif_else_6(a: number,
                           v1: number, v2: number,   // Path A values
                           v3: number, v4: number,   // Path B values
                           v5: number, v6: number)   // Path C values
                           : number
{
    let r: number = 0;
    let s: number = 0;
    let t: number = 0;
    let u: number = 0;

    if (a > 20) {
        // Path A: only sets r and s
        r = v1;
        s = v2;
        // t and u NOT set
        u = 400;
    } else if (a > 10) {
        // Path B: only sets s and t
        s = v3;
        t = v4;
        // r and u NOT set
        u = 500;
    } else {
        // Path C: only sets r and t
        r = v5;
        t = v6;
        // s NOT set
        u = 600;
    }

    // Each vreg's Phi must have exactly 2 defined inputs and 1 undefined
    return r * 10000 + s * 100 + t + u;
}

ArkTools.arkSteedCompileSync(if_elseif_else_6);

print(if_elseif_else_6(25, 1, 2, 3, 4, 5, 6));   // a>20: r=1, s=2, t=0, u=400 -> 10000+200+0+400=10600
print(if_elseif_else_6(15, 1, 2, 3, 4, 5, 6));   // a>10: r=0, s=3, t=4, u=500 -> 0+300+4+500=804
print(if_elseif_else_6(5, 7, 8, 9, 1, 2, 3));    // else: r=2, s=0, t=3, u=600 -> 20000+0+3+600=20603
