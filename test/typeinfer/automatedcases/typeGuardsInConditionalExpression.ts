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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInConditionalExpression.ts ===
declare function AssertType(value:any, type:string):void;
// In the true expression of a conditional expression, 
// the type of a letiable or parameter is narrowed by any type guard in the condition when true, 
// provided the true expression contains no assignments to the letiable or parameter.
// In the false expression of a conditional expression, 
// the type of a letiable or parameter is narrowed by any type guard in the condition when false, 
// provided the false expression contains no assignments to the letiable or parameter.

function foo(x: number | string) {
AssertType(typeof x === "string"        ? x.length // string        : x++, "number");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x.length // string
AssertType(x.length, "number");

        : x++; // number
AssertType(x++, "number");
AssertType(x, "number");
}
function foo2(x: number | string) {
AssertType(typeof x === "string"        ? ((x = "hello") && x) // string        : x, "union");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? ((x = "hello") && x) // string
AssertType(((x = "hello") && x), "string");
AssertType((x = "hello") && x, "string");
AssertType((x = "hello"), "string");
AssertType(x = "hello", "string");
AssertType(x, "union");
AssertType("hello", "string");
AssertType(x, "string");

        : x; // number
AssertType(x, "number");
}
function foo3(x: number | string) {
AssertType(typeof x === "string"        ? ((x = 10) && x) // number        : x, "number");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? ((x = 10) && x) // number
AssertType(((x = 10) && x), "number");
AssertType((x = 10) && x, "number");
AssertType((x = 10), "int");
AssertType(x = 10, "int");
AssertType(x, "union");
AssertType(10, "int");
AssertType(x, "number");

        : x; // number
AssertType(x, "number");
}
function foo4(x: number | string) {
AssertType(typeof x === "string"        ? x // string        : ((x = 10) && x), "union");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x // string
AssertType(x, "string");

        : ((x = 10) && x); // number
AssertType(((x = 10) && x), "number");
AssertType((x = 10) && x, "number");
AssertType((x = 10), "int");
AssertType(x = 10, "int");
AssertType(x, "union");
AssertType(10, "int");
AssertType(x, "number");
}
function foo5(x: number | string) {
AssertType(typeof x === "string"        ? x // string        : ((x = "hello") && x), "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x // string
AssertType(x, "string");

        : ((x = "hello") && x); // string
AssertType(((x = "hello") && x), "string");
AssertType((x = "hello") && x, "string");
AssertType((x = "hello"), "string");
AssertType(x = "hello", "string");
AssertType(x, "union");
AssertType("hello", "string");
AssertType(x, "string");
}
function foo6(x: number | string) {
    // Modify in both branches
AssertType(typeof x === "string"        ? ((x = 10) && x) // number        : ((x = "hello") && x), "union");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? ((x = 10) && x) // number
AssertType(((x = 10) && x), "number");
AssertType((x = 10) && x, "number");
AssertType((x = 10), "int");
AssertType(x = 10, "int");
AssertType(x, "union");
AssertType(10, "int");
AssertType(x, "number");

        : ((x = "hello") && x); // string
AssertType(((x = "hello") && x), "string");
AssertType((x = "hello") && x, "string");
AssertType((x = "hello"), "string");
AssertType(x = "hello", "string");
AssertType(x, "union");
AssertType("hello", "string");
AssertType(x, "string");
}
function foo7(x: number | string | boolean) {
AssertType(typeof x === "string"        ? x === "hello" // boolean        : typeof x === "boolean"        ? x // boolean        : x == 10, "boolean");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x === "hello" // boolean
AssertType(x === "hello", "boolean");
AssertType(x, "string");
AssertType("hello", "string");

        : typeof x === "boolean"
AssertType(typeof x === "boolean"        ? x // boolean        : x == 10, "boolean");
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");

        ? x // boolean
AssertType(x, "boolean");

        : x == 10; // boolean
AssertType(x == 10, "boolean");
AssertType(x, "number");
AssertType(10, "int");
}
function foo8(x: number | string | boolean) {
    let b: number | boolean;
AssertType(b, "union");

AssertType(typeof x === "string"        ? x === "hello"        : ((b = x) && //  number | boolean        (typeof x === "boolean"        ? x // boolean        : x == 10)), "boolean");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x === "hello"
AssertType(x === "hello", "boolean");
AssertType(x, "string");
AssertType("hello", "string");

        : ((b = x) && //  number | boolean
AssertType(((b = x) && //  number | boolean        (typeof x === "boolean"        ? x // boolean        : x == 10)), "boolean");
AssertType((b = x) && //  number | boolean        (typeof x === "boolean"        ? x // boolean        : x == 10), "boolean");
AssertType((b = x), "union");
AssertType(b = x, "union");
AssertType(b, "union");
AssertType(x, "union");

        (typeof x === "boolean"
AssertType((typeof x === "boolean"        ? x // boolean        : x == 10), "boolean");
AssertType(typeof x === "boolean"        ? x // boolean        : x == 10, "boolean");
AssertType(typeof x === "boolean", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("boolean", "string");

        ? x // boolean
AssertType(x, "boolean");

        : x == 10)); // boolean
AssertType(x == 10, "boolean");
AssertType(x, "number");
AssertType(10, "int");
}
function foo9(x: number | string) {
    let y = 10;
AssertType(y, "number");
AssertType(10, "int");

    // usage of x or assignment to separate letiable shouldn't cause narrowing of type to stop
AssertType(typeof x === "string"        ? ((y = x.length) && x === "hello") // boolean        : x === 10, "boolean");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? ((y = x.length) && x === "hello") // boolean
AssertType(((y = x.length) && x === "hello"), "boolean");
AssertType((y = x.length) && x === "hello", "boolean");
AssertType((y = x.length), "number");
AssertType(y = x.length, "number");
AssertType(y, "number");
AssertType(x.length, "number");
AssertType(x === "hello", "boolean");
AssertType(x, "string");
AssertType("hello", "string");

        : x === 10; // boolean
AssertType(x === 10, "boolean");
AssertType(x, "number");
AssertType(10, "int");
}
function foo10(x: number | string | boolean) {
    // Mixing typeguards
    let b: boolean | number;
AssertType(b, "union");

AssertType(typeof x === "string"        ? x // string        : ((b = x) // x is number | boolean        && typeof x === "number"        && x.toString()), "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x // string
AssertType(x, "string");

        : ((b = x) // x is number | boolean
AssertType(((b = x) // x is number | boolean        && typeof x === "number"        && x.toString()), "string");
AssertType((b = x) // x is number | boolean        && typeof x === "number"        && x.toString(), "string");
AssertType((b = x) // x is number | boolean        && typeof x === "number", "boolean");
AssertType((b = x), "union");
AssertType(b = x, "union");
AssertType(b, "union");
AssertType(x, "union");

        && typeof x === "number"
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        && x.toString()); // x is number
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");
}
function foo11(x: number | string | boolean) {
    // Mixing typeguards
    let b: number | boolean | string;
AssertType(b, "union");

AssertType(typeof x === "string"        ? x // string        : ((b = x) // x is number | boolean        && typeof x === "number"        && (x = 10) // assignment to x        && x), "union");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? x // string
AssertType(x, "string");

        : ((b = x) // x is number | boolean
AssertType(((b = x) // x is number | boolean        && typeof x === "number"        && (x = 10) // assignment to x        && x), "number");
AssertType((b = x) // x is number | boolean        && typeof x === "number"        && (x = 10) // assignment to x        && x, "number");
AssertType((b = x) // x is number | boolean        && typeof x === "number"        && (x = 10), "union");
AssertType((b = x) // x is number | boolean        && typeof x === "number", "boolean");
AssertType((b = x), "union");
AssertType(b = x, "union");
AssertType(b, "union");
AssertType(x, "union");

        && typeof x === "number"
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        && (x = 10) // assignment to x
AssertType((x = 10), "int");
AssertType(x = 10, "int");
AssertType(x, "union");
AssertType(10, "int");

        && x); // x is number
AssertType(x, "number");
}
function foo12(x: number | string | boolean) {
    // Mixing typeguards
    let b: number | boolean | string;
AssertType(b, "union");

AssertType(typeof x === "string"        ? ((x = 10) && x.toString().length) // number        : ((b = x) // x is number | boolean        && typeof x === "number"        && x), "number");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x === "string"

        ? ((x = 10) && x.toString().length) // number
AssertType(((x = 10) && x.toString().length), "number");
AssertType((x = 10) && x.toString().length, "number");
AssertType((x = 10), "int");
AssertType(x = 10, "int");
AssertType(x, "union");
AssertType(10, "int");
AssertType(x.toString().length, "number");
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");

        : ((b = x) // x is number | boolean
AssertType(((b = x) // x is number | boolean        && typeof x === "number"        && x), "number");
AssertType((b = x) // x is number | boolean        && typeof x === "number"        && x, "number");
AssertType((b = x) // x is number | boolean        && typeof x === "number", "boolean");
AssertType((b = x), "union");
AssertType(b = x, "union");
AssertType(b, "union");
AssertType(x, "union");

        && typeof x === "number"
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        && x); // x is number
AssertType(x, "number");
}

