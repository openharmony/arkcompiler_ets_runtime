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

// === tests/cases/conformance/classes/members/privateNames/privateNamesAndStaticMethods.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    static #foo(a: number) {}
    static async #bar(a: number) {}
    static async *#baz(a: number) {
AssertType(3, "int");
        return 3;
    }
    static #_quux: number;
    static get #quux (): number {
AssertType(this.#_quux, "number");
AssertType(this, "typeof A");
        return this.#_quux;
    }
    static set #quux (val: number) {
        this.#_quux = val;
AssertType(this.#_quux = val, "number");
AssertType(this.#_quux, "number");
AssertType(this, "typeof A");
AssertType(val, "number");
    }
    constructor () {
        A.#foo(30);
AssertType(A.#foo(30), "void");
AssertType(A.#foo, "(number) => void");
AssertType(30, "int");

        A.#bar(30);
AssertType(A.#bar(30), "Promise<void>");
AssertType(A.#bar, "(number) => Promise<void>");
AssertType(30, "int");

        A.#bar(30);
AssertType(A.#bar(30), "Promise<void>");
AssertType(A.#bar, "(number) => Promise<void>");
AssertType(30, "int");

        A.#quux = A.#quux + 1;
AssertType(A.#quux = A.#quux + 1, "number");
AssertType(A.#quux, "number");
AssertType(A.#quux + 1, "number");
AssertType(A.#quux, "number");
AssertType(1, "int");

        A.#quux++;
AssertType(A.#quux++, "number");
AssertType(A.#quux, "number");
 }
}

class B extends A {
    static #foo(a: string) {}
    constructor () {
        super();
AssertType(super(), "void");
AssertType(super, "typeof A");

        B.#foo("str");
AssertType(B.#foo("str"), "void");
AssertType(B.#foo, "(string) => void");
AssertType("str", "string");
    }
}


