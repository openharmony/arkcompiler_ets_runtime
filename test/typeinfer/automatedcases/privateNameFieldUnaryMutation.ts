/*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Copyright (c) 2023 Huawei Device Co., Ltd.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/conformance/classes/members/privateNames/privateNameFieldUnaryMutation.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    #test: number = 24;
    constructor() {
        this.#test++;
AssertType(this.#test++, "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        this.#test--;
AssertType(this.#test--, "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        ++this.#test;
AssertType(++this.#test, "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        --this.#test;
AssertType(--this.#test, "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        const a = this.#test++;
AssertType(a, "number");
AssertType(this.#test++, "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        const b = this.#test--;
AssertType(b, "number");
AssertType(this.#test--, "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        const c = ++this.#test;
AssertType(c, "number");
AssertType(++this.#test, "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        const d = --this.#test;
AssertType(d, "number");
AssertType(--this.#test, "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        for (this.#test = 0; this.#test < 10; ++this.#test) {
AssertType(this.#test = 0, "int");

AssertType(this.#test, "number");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.#test < 10, "boolean");

AssertType(this.#test, "number");

AssertType(this, "this");

AssertType(10, "int");

AssertType(++this.#test, "number");

AssertType(this.#test, "number");

AssertType(this, "this");
}

        for (this.#test = 0; this.#test < 10; this.#test++) {
AssertType(this.#test = 0, "int");

AssertType(this.#test, "number");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.#test < 10, "boolean");

AssertType(this.#test, "number");

AssertType(this, "this");

AssertType(10, "int");

AssertType(this.#test++, "number");

AssertType(this.#test, "number");

AssertType(this, "this");
}

        (this.#test)++;
AssertType((this.#test)++, "number");
AssertType((this.#test), "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        (this.#test)--;
AssertType((this.#test)--, "number");
AssertType((this.#test), "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        ++(this.#test);
AssertType(++(this.#test), "number");
AssertType((this.#test), "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        --(this.#test);
AssertType(--(this.#test), "number");
AssertType((this.#test), "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        const e = (this.#test)++;
AssertType(e, "number");
AssertType((this.#test)++, "number");
AssertType((this.#test), "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        const f = (this.#test)--;
AssertType(f, "number");
AssertType((this.#test)--, "number");
AssertType((this.#test), "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        const g = ++(this.#test);
AssertType(g, "number");
AssertType(++(this.#test), "number");
AssertType((this.#test), "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        const h = --(this.#test);
AssertType(h, "number");
AssertType(--(this.#test), "number");
AssertType((this.#test), "number");
AssertType(this.#test, "number");
AssertType(this, "this");

        for (this.#test = 0; this.#test < 10; ++(this.#test)) {
AssertType(this.#test = 0, "int");

AssertType(this.#test, "number");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.#test < 10, "boolean");

AssertType(this.#test, "number");

AssertType(this, "this");

AssertType(10, "int");

AssertType(++(this.#test), "number");

AssertType((this.#test), "number");

AssertType(this.#test, "number");

AssertType(this, "this");
}

        for (this.#test = 0; this.#test < 10; (this.#test)++) {
AssertType(this.#test = 0, "int");

AssertType(this.#test, "number");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.#test < 10, "boolean");

AssertType(this.#test, "number");

AssertType(this, "this");

AssertType(10, "int");

AssertType((this.#test)++, "number");

AssertType((this.#test), "number");

AssertType(this.#test, "number");

AssertType(this, "this");
}
    }
    test() {
        this.getInstance().#test++;
AssertType(this.getInstance().#test++, "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        this.getInstance().#test--;
AssertType(this.getInstance().#test--, "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        ++this.getInstance().#test;
AssertType(++this.getInstance().#test, "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        --this.getInstance().#test;
AssertType(--this.getInstance().#test, "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        const a = this.getInstance().#test++;
AssertType(a, "number");
AssertType(this.getInstance().#test++, "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        const b = this.getInstance().#test--;
AssertType(b, "number");
AssertType(this.getInstance().#test--, "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        const c = ++this.getInstance().#test;
AssertType(c, "number");
AssertType(++this.getInstance().#test, "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        const d = --this.getInstance().#test;
AssertType(d, "number");
AssertType(--this.getInstance().#test, "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        for (this.getInstance().#test = 0; this.getInstance().#test < 10; ++this.getInstance().#test) {
AssertType(this.getInstance().#test = 0, "int");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.getInstance().#test < 10, "boolean");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");

AssertType(10, "int");

AssertType(++this.getInstance().#test, "number");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");
}

        for (this.getInstance().#test = 0; this.getInstance().#test < 10; this.getInstance().#test++) {
AssertType(this.getInstance().#test = 0, "int");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.getInstance().#test < 10, "boolean");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");

AssertType(10, "int");

AssertType(this.getInstance().#test++, "number");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");
}

        (this.getInstance().#test)++;
AssertType((this.getInstance().#test)++, "number");
AssertType((this.getInstance().#test), "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        (this.getInstance().#test)--;
AssertType((this.getInstance().#test)--, "number");
AssertType((this.getInstance().#test), "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        ++(this.getInstance().#test);
AssertType(++(this.getInstance().#test), "number");
AssertType((this.getInstance().#test), "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        --(this.getInstance().#test);
AssertType(--(this.getInstance().#test), "number");
AssertType((this.getInstance().#test), "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        const e = (this.getInstance().#test)++;
AssertType(e, "number");
AssertType((this.getInstance().#test)++, "number");
AssertType((this.getInstance().#test), "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        const f = (this.getInstance().#test)--;
AssertType(f, "number");
AssertType((this.getInstance().#test)--, "number");
AssertType((this.getInstance().#test), "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        const g = ++(this.getInstance().#test);
AssertType(g, "number");
AssertType(++(this.getInstance().#test), "number");
AssertType((this.getInstance().#test), "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        const h = --(this.getInstance().#test);
AssertType(h, "number");
AssertType(--(this.getInstance().#test), "number");
AssertType((this.getInstance().#test), "number");
AssertType(this.getInstance().#test, "number");
AssertType(this.getInstance(), "C");
AssertType(this.getInstance, "() => C");
AssertType(this, "this");

        for (this.getInstance().#test = 0; this.getInstance().#test < 10; ++(this.getInstance().#test)) {
AssertType(this.getInstance().#test = 0, "int");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.getInstance().#test < 10, "boolean");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");

AssertType(10, "int");

AssertType(++(this.getInstance().#test), "number");

AssertType((this.getInstance().#test), "number");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");
}

        for (this.getInstance().#test = 0; this.getInstance().#test < 10; (this.getInstance().#test)++) {
AssertType(this.getInstance().#test = 0, "int");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.getInstance().#test < 10, "boolean");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");

AssertType(10, "int");

AssertType((this.getInstance().#test)++, "number");

AssertType((this.getInstance().#test), "number");

AssertType(this.getInstance().#test, "number");

AssertType(this.getInstance(), "C");

AssertType(this.getInstance, "() => C");

AssertType(this, "this");
}
    }
    getInstance() { 
AssertType(new C(), "C");
return new C(); 

AssertType(C, "typeof C");
}
}


