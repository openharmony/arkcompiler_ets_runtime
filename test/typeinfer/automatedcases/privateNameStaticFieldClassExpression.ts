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

// === tests/cases/conformance/classes/members/privateNames/privateNameStaticFieldClassExpression.ts ===
declare function AssertType(value:any, type:string):void;
class B {
    static #foo = class {
        constructor() {
            console.log("hello");
AssertType(console.log("hello"), "void");
AssertType(console.log, "(...any[]) => void");
AssertType("hello", "string");

            new B.#foo2();
AssertType(new B.#foo2(), "Foo");
AssertType(B.#foo2, "typeof Foo");
        }
        static test = 123;
        field = 10;
    };
    static #foo2 = class Foo {
        static otherClass = 123;
    };

    m() {
        console.log(B.#foo.test)
AssertType(console.log(B.#foo.test), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(B.#foo.test, "number");
AssertType(B.#foo, "typeof (Anonymous class)");

        B.#foo.test = 10;
AssertType(B.#foo.test = 10, "int");
AssertType(B.#foo.test, "number");
AssertType(B.#foo, "typeof (Anonymous class)");
AssertType(10, "int");

        new B.#foo().field;
AssertType(new B.#foo().field, "number");
AssertType(new B.#foo(), "(Anonymous class)");
AssertType(B.#foo, "typeof (Anonymous class)");
    }
}




