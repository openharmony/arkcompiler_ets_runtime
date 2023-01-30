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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInFunctionAndModuleBlock.ts ===
declare function AssertType(value:any, type:string):void;
// typeguards are scoped in function/module block

function foo(x: number | string | boolean) {
AssertType(typeof x === "string"        ? x        : function f() {            let b = x; // number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        } (), "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x
AssertType(x, "string");

        : function f() {
AssertType(function f() {            let b = x; // number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        }, "() => string");
AssertType(f, "() => string");
AssertType(function f() {            let b = x; // number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        } (), "string");

            let b = x; // number | boolean
AssertType(b, "union");
AssertType(x, "union");

AssertType(typeof x === "boolean"                ? x.toString() // boolean                : x.toString(), "string");
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");
            return typeof x === "boolean"

                ? x.toString() // boolean
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");

                : x.toString(); // number
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");

        } ();
}
function foo2(x: number | string | boolean) {
AssertType(typeof x === "string"        ? x        : function f(a: number | boolean) {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        } (x), "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x
AssertType(x, "string");

        : function f(a: number | boolean) {
AssertType(function f(a: number | boolean) {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        }, "(union) => string");
AssertType(f, "(union) => string");
AssertType(a, "union");
AssertType(function f(a: number | boolean) {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        } (x), "string");

            let b = x; // new scope - number | boolean
AssertType(b, "union");
AssertType(x, "union");

AssertType(typeof x === "boolean"                ? x.toString() // boolean                : x.toString(), "string");
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");
            return typeof x === "boolean"

                ? x.toString() // boolean
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");

                : x.toString(); // number
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");

        } (x); // x here is narrowed to number | boolean
AssertType(x, "union");
}
function foo3(x: number | string | boolean) {
AssertType(typeof x === "string"        ? x        : (() => {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        })(), "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x
AssertType(x, "string");

        : (() => {
AssertType((() => {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        }), "() => string");
AssertType(() => {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        }, "() => string");
AssertType((() => {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        })(), "string");

            let b = x; // new scope - number | boolean
AssertType(b, "union");
AssertType(x, "union");

AssertType(typeof x === "boolean"                ? x.toString() // boolean                : x.toString(), "string");
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");
            return typeof x === "boolean"

                ? x.toString() // boolean
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");

                : x.toString(); // number
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");

        })();
}
function foo4(x: number | string | boolean) {
AssertType(typeof x === "string"        ? x        : ((a: number | boolean) => {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        })(x), "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x
AssertType(x, "string");

        : ((a: number | boolean) => {
AssertType(((a: number | boolean) => {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        }), "(union) => string");
AssertType((a: number | boolean) => {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        }, "(union) => string");
AssertType(a, "union");
AssertType(((a: number | boolean) => {            let b = x; // new scope - number | boolean            return typeof x === "boolean"                ? x.toString() // boolean                : x.toString(); // number        })(x), "string");

            let b = x; // new scope - number | boolean
AssertType(b, "union");
AssertType(x, "union");

AssertType(typeof x === "boolean"                ? x.toString() // boolean                : x.toString(), "string");
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");
            return typeof x === "boolean"

                ? x.toString() // boolean
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");

                : x.toString(); // number
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");

        })(x); // x here is narrowed to number | boolean
AssertType(x, "union");
}
// Type guards do not affect nested function declarations
function foo5(x: number | string | boolean) {
    if (typeof x === "string") {
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        let y = x; // string;
AssertType(y, "string");
AssertType(x, "string");

        function foo() {
AssertType(foo, "() => void");

            let z = x; // string
AssertType(z, "union");
AssertType(x, "union");
        }
    }
}
module m {
    let x: number | string | boolean;
    module m2 {
        let b = x; // new scope - number | boolean | string
        let y: string;
        if (typeof x === "string") {
            y = x // string;
AssertType(y = x, "string");
AssertType(y, "string");
AssertType(x, "string");

        } else {
            y = typeof x === "boolean"
AssertType(y = typeof x === "boolean"            ? x.toString() // boolean            : x.toString(), "string");
AssertType(y, "string");
AssertType(typeof x === "boolean"            ? x.toString() // boolean            : x.toString(), "string");
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");

            ? x.toString() // boolean
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");

            : x.toString(); // number
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");
        }
    }
}
module m1 {
    let x: number | string | boolean;
    module m2.m3 {
        let b = x; // new scope - number | boolean | string
        let y: string;
        if (typeof x === "string") {
            y = x // string;
AssertType(y = x, "string");
AssertType(y, "string");
AssertType(x, "string");

        } else {
            y = typeof x === "boolean"
AssertType(y = typeof x === "boolean"            ? x.toString() // boolean            : x.toString(), "string");
AssertType(y, "string");
AssertType(typeof x === "boolean"            ? x.toString() // boolean            : x.toString(), "string");
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");

            ? x.toString() // boolean
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");

            : x.toString(); // number
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");
        }
    }
}

