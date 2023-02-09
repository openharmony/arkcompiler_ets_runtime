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

// === tests/cases/conformance/classes/members/privateNames/privateNameComputedPropertyName1.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    #a = 'a';
    #b: string;

    readonly #c = 'c';
    readonly #d: string;

    #e = '';

    constructor() {
        this.#b = 'b';
AssertType(this.#b = 'b', "string");
AssertType(this.#b, "string");
AssertType(this, "this");
AssertType('b', "string");

        this.#d = 'd';
AssertType(this.#d = 'd', "string");
AssertType(this.#d, "string");
AssertType(this, "this");
AssertType('d', "string");
    }

    test() {
        const data: Record<string, string> = { a: 'a', b: 'b', c: 'c', d: 'd', e: 'e' };
AssertType(data, "Record<string, string>");
AssertType({ a: 'a', b: 'b', c: 'c', d: 'd', e: 'e' }, "{ a: string; b: string; c: string; d: string; e: string; }");
AssertType(a, "string");
AssertType('a', "string");
AssertType(b, "string");
AssertType('b', "string");
AssertType(c, "string");
AssertType('c', "string");
AssertType(d, "string");
AssertType('d', "string");
AssertType(e, "string");
AssertType('e', "string");

        const {
            [this.#a]: a,
AssertType(this.#a, "string");
AssertType(this, "this");
AssertType(a, "string");

            [this.#b]: b,
AssertType(this.#b, "string");
AssertType(this, "this");
AssertType(b, "string");

            [this.#c]: c,
AssertType(this.#c, "string");
AssertType(this, "this");
AssertType(c, "string");

            [this.#d]: d,
AssertType(this.#d, "string");
AssertType(this, "this");
AssertType(d, "string");

            [this.#e = 'e']: e,
AssertType(this.#e = 'e', "string");
AssertType(this.#e, "string");
AssertType(this, "this");
AssertType('e', "string");
AssertType(e, "string");

        } = data;
AssertType(data, "Record<string, string>");

        console.log(a, b, c, d, e);
AssertType(console.log(a, b, c, d, e), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(a, "string");
AssertType(b, "string");
AssertType(c, "string");
AssertType(d, "string");
AssertType(e, "string");

        const a1 = data[this.#a];
AssertType(a1, "string");
AssertType(data[this.#a], "string");
AssertType(data, "Record<string, string>");
AssertType(this.#a, "string");
AssertType(this, "this");

        const b1 = data[this.#b];
AssertType(b1, "string");
AssertType(data[this.#b], "string");
AssertType(data, "Record<string, string>");
AssertType(this.#b, "string");
AssertType(this, "this");

        const c1 = data[this.#c];
AssertType(c1, "string");
AssertType(data[this.#c], "string");
AssertType(data, "Record<string, string>");
AssertType(this.#c, "string");
AssertType(this, "this");

        const d1 = data[this.#d];
AssertType(d1, "string");
AssertType(data[this.#d], "string");
AssertType(data, "Record<string, string>");
AssertType(this.#d, "string");
AssertType(this, "this");

        const e1 = data[this.#e];
AssertType(e1, "string");
AssertType(data[this.#e], "string");
AssertType(data, "Record<string, string>");
AssertType(this.#e, "string");
AssertType(this, "this");

        console.log(a1, b1, c1, d1);
AssertType(console.log(a1, b1, c1, d1), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(a1, "string");
AssertType(b1, "string");
AssertType(c1, "string");
AssertType(d1, "string");
    }
}

new A().test();
AssertType(new A().test(), "void");
AssertType(new A().test, "() => void");



