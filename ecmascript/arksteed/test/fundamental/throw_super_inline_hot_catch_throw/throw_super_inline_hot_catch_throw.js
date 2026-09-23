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

// Test: inlined throw.ifsupernotcorrectcall TAKES the throwing path in compiled code
// with a HOT (pre-executed) catch inside the same constructor.
// Warm-up: one interpreted construction throws at "this before super" and runs the catch.
// Compiled construction repeats the same flow: the inlined 0x0 check detects the unbound
// this, the compiled catch handles the throw, then super() binds this and the final
// inlined 0x0 check passes.

//! METHOD      CatchInCtor
//! HAS_NOT     CallRuntime      ThrowIfSuperNotCorrectCall     # inlined: no stub call remains
//! COUNT_GE    BranchIfReferenceEqual  4                      # >= 2 inline cmp per check site

class Base {
    constructor(x) {
        this.x = x;
    }
}

var caught = 0;

class CatchInCtor extends Base {
    constructor(x) {
        try {
            this.y = x + 1;  // throw.ifsupernotcorrectcall 0x0 (inlined): this undefined -> THROWS
            super(x);
        } catch (e) {
            caught++;
        }
        super(x);            // throw.ifsupernotcorrectcall 0x1 (inlined): this undefined -> ok
        this.y = x + 2;      // throw.ifsupernotcorrectcall 0x0 (inlined): this bound -> ok
    }
}

new CatchInCtor(1);  // warm the catch (interpreted)

ArkTools.arkSteedCompileSync(CatchInCtor);

var d = new CatchInCtor(5);  // compiled: inlined throw -> compiled catch -> recovery
print(`caught = ${caught}, d.x = ${d.x}, d.y = ${d.y}`);
