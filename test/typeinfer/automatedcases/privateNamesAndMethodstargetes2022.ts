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

// === tests/cases/conformance/classes/members/privateNames/privateNamesAndMethods.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    #foo(a: number) {}
    async #bar(a: number) {}
    async *#baz(a: number) {
AssertType(3, "int");
        return 3;
    }
    #_quux: number;
    get #quux (): number {
AssertType(this.#_quux, "number");
AssertType(this, "this");
        return this.#_quux;
    }
    set #quux (val: number) {
        this.#_quux = val;
AssertType(this.#_quux = val, "number");
AssertType(this.#_quux, "number");
AssertType(this, "this");
AssertType(val, "number");
    }
    constructor () {
        this.#foo(30);
AssertType(this.#foo(30), "void");
AssertType(this.#foo, "(number) => void");
AssertType(this, "this");
AssertType(30, "int");

        this.#bar(30);
AssertType(this.#bar(30), "Promise<void>");
AssertType(this.#bar, "(number) => Promise<void>");
AssertType(this, "this");
AssertType(30, "int");

        this.#baz(30);
AssertType(this.#baz(30), "AsyncGenerator<never, number, unknown>");
AssertType(this.#baz, "(number) => AsyncGenerator<never, number, unknown>");
AssertType(this, "this");
AssertType(30, "int");

        this.#quux = this.#quux + 1;
AssertType(this.#quux = this.#quux + 1, "number");
AssertType(this.#quux, "number");
AssertType(this, "this");
AssertType(this.#quux + 1, "number");
AssertType(this.#quux, "number");
AssertType(this, "this");
AssertType(1, "int");

        this.#quux++;
AssertType(this.#quux++, "number");
AssertType(this.#quux, "number");
AssertType(this, "this");
 }
}

class B extends A {
    #foo(a: string) {}
    constructor () {
        super();
AssertType(super(), "void");
AssertType(super, "typeof A");

        this.#foo("str");
AssertType(this.#foo("str"), "void");
AssertType(this.#foo, "(string) => void");
AssertType(this, "this");
AssertType("str", "string");
    }
}


