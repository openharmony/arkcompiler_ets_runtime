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

// === tests/cases/conformance/classes/members/privateNames/privateNameStaticFieldUnaryMutation.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    static #test: number = 24;
    constructor() {
        C.#test++;
AssertType(C.#test++, "number");
AssertType(C.#test, "number");

        C.#test--;
AssertType(C.#test--, "number");
AssertType(C.#test, "number");

        ++C.#test;
AssertType(++C.#test, "number");
AssertType(C.#test, "number");

        --C.#test;
AssertType(--C.#test, "number");
AssertType(C.#test, "number");

        const a = C.#test++;
AssertType(a, "number");
AssertType(C.#test++, "number");
AssertType(C.#test, "number");

        const b = C.#test--;
AssertType(b, "number");
AssertType(C.#test--, "number");
AssertType(C.#test, "number");

        const c = ++C.#test;
AssertType(c, "number");
AssertType(++C.#test, "number");
AssertType(C.#test, "number");

        const d = --C.#test;
AssertType(d, "number");
AssertType(--C.#test, "number");
AssertType(C.#test, "number");

        for (C.#test = 0; C.#test < 10; ++C.#test) {
AssertType(C.#test = 0, "int");

AssertType(C.#test, "number");

AssertType(0, "int");

AssertType(C.#test < 10, "boolean");

AssertType(C.#test, "number");

AssertType(10, "int");

AssertType(++C.#test, "number");

AssertType(C.#test, "number");
}

        for (C.#test = 0; C.#test < 10; C.#test++) {
AssertType(C.#test = 0, "int");

AssertType(C.#test, "number");

AssertType(0, "int");

AssertType(C.#test < 10, "boolean");

AssertType(C.#test, "number");

AssertType(10, "int");

AssertType(C.#test++, "number");

AssertType(C.#test, "number");
}
    }
    test() {
        this.getClass().#test++;
AssertType(this.getClass().#test++, "number");
AssertType(this.getClass().#test, "number");
AssertType(this.getClass(), "typeof C");
AssertType(this.getClass, "() => typeof C");
AssertType(this, "this");

        this.getClass().#test--;
AssertType(this.getClass().#test--, "number");
AssertType(this.getClass().#test, "number");
AssertType(this.getClass(), "typeof C");
AssertType(this.getClass, "() => typeof C");
AssertType(this, "this");

        ++this.getClass().#test;
AssertType(++this.getClass().#test, "number");
AssertType(this.getClass().#test, "number");
AssertType(this.getClass(), "typeof C");
AssertType(this.getClass, "() => typeof C");
AssertType(this, "this");

        --this.getClass().#test;
AssertType(--this.getClass().#test, "number");
AssertType(this.getClass().#test, "number");
AssertType(this.getClass(), "typeof C");
AssertType(this.getClass, "() => typeof C");
AssertType(this, "this");

        const a = this.getClass().#test++;
AssertType(a, "number");
AssertType(this.getClass().#test++, "number");
AssertType(this.getClass().#test, "number");
AssertType(this.getClass(), "typeof C");
AssertType(this.getClass, "() => typeof C");
AssertType(this, "this");

        const b = this.getClass().#test--;
AssertType(b, "number");
AssertType(this.getClass().#test--, "number");
AssertType(this.getClass().#test, "number");
AssertType(this.getClass(), "typeof C");
AssertType(this.getClass, "() => typeof C");
AssertType(this, "this");

        const c = ++this.getClass().#test;
AssertType(c, "number");
AssertType(++this.getClass().#test, "number");
AssertType(this.getClass().#test, "number");
AssertType(this.getClass(), "typeof C");
AssertType(this.getClass, "() => typeof C");
AssertType(this, "this");

        const d = --this.getClass().#test;
AssertType(d, "number");
AssertType(--this.getClass().#test, "number");
AssertType(this.getClass().#test, "number");
AssertType(this.getClass(), "typeof C");
AssertType(this.getClass, "() => typeof C");
AssertType(this, "this");

        for (this.getClass().#test = 0; this.getClass().#test < 10; ++this.getClass().#test) {
AssertType(this.getClass().#test = 0, "int");

AssertType(this.getClass().#test, "number");

AssertType(this.getClass(), "typeof C");

AssertType(this.getClass, "() => typeof C");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.getClass().#test < 10, "boolean");

AssertType(this.getClass().#test, "number");

AssertType(this.getClass(), "typeof C");

AssertType(this.getClass, "() => typeof C");

AssertType(this, "this");

AssertType(10, "int");

AssertType(++this.getClass().#test, "number");

AssertType(this.getClass().#test, "number");

AssertType(this.getClass(), "typeof C");

AssertType(this.getClass, "() => typeof C");

AssertType(this, "this");
}

        for (this.getClass().#test = 0; this.getClass().#test < 10; this.getClass().#test++) {
AssertType(this.getClass().#test = 0, "int");

AssertType(this.getClass().#test, "number");

AssertType(this.getClass(), "typeof C");

AssertType(this.getClass, "() => typeof C");

AssertType(this, "this");

AssertType(0, "int");

AssertType(this.getClass().#test < 10, "boolean");

AssertType(this.getClass().#test, "number");

AssertType(this.getClass(), "typeof C");

AssertType(this.getClass, "() => typeof C");

AssertType(this, "this");

AssertType(10, "int");

AssertType(this.getClass().#test++, "number");

AssertType(this.getClass().#test, "number");

AssertType(this.getClass(), "typeof C");

AssertType(this.getClass, "() => typeof C");

AssertType(this, "this");
}
    }
    getClass() { 
AssertType(C, "typeof C");
return C; 
}
}


