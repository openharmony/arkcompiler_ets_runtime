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

// Test: inlined throw.ifsupernotcorrectcall with a HOT (pre-executed) catch, no-throw path.
// The catch is warmed by one interpreted throwing call BEFORE arkSteedCompileSync, so the
// compiled constructor must keep the inlined super/this checks and the compiled catch
// handler (cold-catch deopt does not apply). The compiled call takes the normal path:
// every inlined check passes and the catch is not re-entered.

//! METHOD      OkDerived
//! HAS_NOT     CallRuntime      ThrowIfSuperNotCorrectCall     # inlined: no stub call remains
//! COUNT_GE    BranchIfReferenceEqual  4                      # >= 2 inline cmp per check site

class Base {
    constructor(x) {
        this.x = x;
    }
}

var caught = 0;

class OkDerived extends Base {
    constructor(x, y) {
        try {
            if (x < 0) {
                throw new Error("negative");  // warm-up trigger, never taken after compile
            }
            super(x);        // throw.ifsupernotcorrectcall 0x1 (inlined): this undefined -> ok
            this.y = y;      // throw.ifsupernotcorrectcall 0x0 (inlined): this bound -> ok
        } catch (e) {
            caught++;
            super(-x);       // recovery: this still undefined -> 0x1 ok
            this.y = -y;
        }
    }
}

new OkDerived(-1, -2);  // warm the catch (interpreted)

ArkTools.arkSteedCompileSync(OkDerived);

var d = new OkDerived(3, 4);  // compiled: inlined checks pass, catch not taken
print(`caught = ${caught}, d.x = ${d.x}, d.y = ${d.y}`);
