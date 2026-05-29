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

// Directly mimics 3d-cube drawQube's CFG pattern:
//   Loop → first if (edge-split, early RPO) → deep nested if chains (later RPO)
//          → merge block combining all paths.
// The early edge-split predecessor doesn't set certain vregs; the merge
// creates Phis where the edge-split's input incorrectly references a Phi
// from a later block.

function if_nested_2(x, a, b, c, d, e, f, g, h)
{
    let r0 = 0, r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;

    // Loop: creates several live-in vregs for subsequent blocks
    for (let i = 5; i >= 0; i--) {
        r0 += a;
    }

    // Block A: first if — true target is edge-split (early RPO)
    //           only modifies r1, r2
    if (x > 10) {
        r1 = b * 100;
        r2 = c;
    }

    // Block B: second if with deep nested chain (creates later-RPO blocks)
    //          modifies r3, r4, r5, r6
    if (x < 20) {
        if (a > 3) {
            r3 = d + e;
            r4 = f;
            if (b > 5) {
                r5 = g * 10;
                r6 = h;
            } else {
                r5 = h * 10;
                r6 = g;
            }
        } else {
            r3 = e;
            r4 = d + f;
        }
    }

    // Block C: third if-chain (more deep blocks)
    if (b > 3) {
        if (c > 4) {
            r2 = a + b;
            if (d > 5) {
                r5 = c * 100;
            }
        }
    }

    // Final merge: all paths converge. Creates Phis for vars not set on
    //              all paths (r0-r6). The edge-split from Block A's true
    //              target doesn't have r3-r6 set → Phis get wrong inputs.
    return r0 + r1 + r2 * 10 + r3 * 100 + r4 * 1000 + r5 * 10000 + r6 * 100000;
}

ArkTools.arkSteedCompileSync(if_nested_2);

let output = if_nested_2(5, 1, 2, 3, 4, 5, 6, 7, 8);
print(output);
output = if_nested_2(15, 2, 3, 4, 5, 6, 7, 8, 9);
print(output);
output = if_nested_2(15, 10, 6, 5, 4, 3, 2, 1, 0);
print(output);
