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

// Test: throw.ifsupernotcorrectcall 0x0 throw path.
// Accessing 'this' before calling super() → this is undefined/hole
// → throws "sub-class must call super before use 'this'"
//
// Written in JavaScript to bypass TypeScript's compile-time check
// that prevents accessing 'this' before 'super()'.

class Base {
    constructor(x) {
        this.x = x;
    }
}

var ok = 0;
var caught = 0;

function test() {
    class Derived1 extends Base {
        constructor(x, y) {
            super(x);
            this.y = y;        // throw.ifsupernotcorrectcall 0x0: this is undefined → no throw
        }
    }

    class Derived2 extends Base {
        constructor(x, y) {
            this.y = y;        // throw.ifsupernotcorrectcall 0x0: this is undefined → THROWS
            super(x);
        }
    }

    class Derived3 extends Base {
        constructor(x, y) {
            super(x);          // first super(): this was undefined → no throw
            super(x);          // throw.ifsupernotcorrectcall 0x1: this is already bound → THROWS
            this.y = y;
        }
    }

    try {
        var d = new Derived1(10, 20);
        ok += 1;
    } catch (e) {
        caught += 1;
    }

    try {
        var d = new Derived2(10, 20);
        ok += 2;
    } catch (e) {
        caught += 2;
    }

    try {
        var d = new Derived3(10, 20);
        ok += 4;
    } catch (e) {
        caught += 4;
    }
}

ArkTools.arkSteedCompileSync(test);
test();
print(`ok = ${ok}, caught = ${caught}`);
