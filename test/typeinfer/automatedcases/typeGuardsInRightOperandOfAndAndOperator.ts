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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInRightOperandOfAndAndOperator.ts ===
declare function AssertType(value:any, type:string):void;
// In the right operand of a && operation, 
// the type of a letiable or parameter is narrowed by any type guard in the left operand when true.
function foo(x: number | string) {
AssertType(typeof x === "string" && x.length === 10, "boolean");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
AssertType(x.length === 10, "boolean");
AssertType(x.length, "number");
AssertType(10, "int");
    return typeof x === "string" && x.length === 10; // string
}
function foo2(x: number | string) {
    // modify x in right hand operand
AssertType(typeof x === "string" && ((x = 10) && x), "number");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
AssertType(((x = 10) && x), "number");
AssertType((x = 10) && x, "number");
AssertType((x = 10), "int");
AssertType(x = 10, "int");
AssertType(x, "union");
AssertType(10, "int");
AssertType(x, "number");
    return typeof x === "string" && ((x = 10) && x); // string | number
}
function foo3(x: number | string) {
    // modify x in right hand operand with string type itself
AssertType(typeof x === "string" && ((x = "hello") && x), "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
AssertType(((x = "hello") && x), "string");
AssertType((x = "hello") && x, "string");
AssertType((x = "hello"), "string");
AssertType(x = "hello", "string");
AssertType(x, "union");
AssertType("hello", "string");
AssertType(x, "string");
    return typeof x === "string" && ((x = "hello") && x); // string | number
}
function foo4(x: number | string | boolean) {
AssertType(typeof x !== "string" // string | number | boolean        && typeof x !== "number"  // number | boolean        && x, "boolean");
AssertType(typeof x !== "string" // string | number | boolean        && typeof x !== "number", "boolean");
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x !== "string" // string | number | boolean

        && typeof x !== "number"  // number | boolean
AssertType(typeof x !== "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        && x;   // boolean
AssertType(x, "boolean");
}
function foo5(x: number | string | boolean) {
    // usage of x or assignment to separate letiable shouldn't cause narrowing of type to stop
    let b: number | boolean;
AssertType(b, "union");

AssertType(typeof x !== "string" // string | number | boolean        && ((b = x) && (typeof x !== "number"  // number | boolean        && x)), "boolean");
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x !== "string" // string | number | boolean

        && ((b = x) && (typeof x !== "number"  // number | boolean
AssertType(((b = x) && (typeof x !== "number"  // number | boolean        && x)), "boolean");
AssertType((b = x) && (typeof x !== "number"  // number | boolean        && x), "boolean");
AssertType((b = x), "union");
AssertType(b = x, "union");
AssertType(b, "union");
AssertType(x, "union");
AssertType((typeof x !== "number"  // number | boolean        && x), "boolean");
AssertType(typeof x !== "number"  // number | boolean        && x, "boolean");
AssertType(typeof x !== "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        && x));   // boolean
AssertType(x, "boolean");
}
function foo6(x: number | string | boolean) {
    // Mixing typeguard narrowing in if statement with conditional expression typeguard
AssertType(typeof x !== "string" // string | number | boolean        && (typeof x !== "number" // number | boolean        ? x // boolean        : x === 10), "boolean");
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x !== "string" // string | number | boolean

        && (typeof x !== "number" // number | boolean
AssertType((typeof x !== "number" // number | boolean        ? x // boolean        : x === 10), "boolean");
AssertType(typeof x !== "number" // number | boolean        ? x // boolean        : x === 10, "boolean");
AssertType(typeof x !== "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        ? x // boolean
AssertType(x, "boolean");

        : x === 10) // number 
AssertType(x === 10, "boolean");
AssertType(x, "number");
AssertType(10, "int");
}
function foo7(x: number | string | boolean) {
    let y: number| boolean | string;
AssertType(y, "union");

    let z: number| boolean | string;
AssertType(z, "union");

    // Mixing typeguard narrowing
AssertType(typeof x !== "string"        && ((z = x) // number | boolean        && (typeof x === "number"        // change value of x        ? ((x = 10) && x.toString()) // x is number        // do not change value        : ((y = x) && x.toString()))), "string");
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
    return typeof x !== "string"

        && ((z = x) // number | boolean
AssertType(((z = x) // number | boolean        && (typeof x === "number"        // change value of x        ? ((x = 10) && x.toString()) // x is number        // do not change value        : ((y = x) && x.toString()))), "string");
AssertType((z = x) // number | boolean        && (typeof x === "number"        // change value of x        ? ((x = 10) && x.toString()) // x is number        // do not change value        : ((y = x) && x.toString())), "string");
AssertType((z = x), "union");
AssertType(z = x, "union");
AssertType(z, "union");
AssertType(x, "union");

        && (typeof x === "number"
AssertType((typeof x === "number"        // change value of x        ? ((x = 10) && x.toString()) // x is number        // do not change value        : ((y = x) && x.toString())), "string");
AssertType(typeof x === "number"        // change value of x        ? ((x = 10) && x.toString()) // x is number        // do not change value        : ((y = x) && x.toString()), "string");
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        // change value of x
        ? ((x = 10) && x.toString()) // x is number
AssertType(((x = 10) && x.toString()), "string");
AssertType((x = 10) && x.toString(), "string");
AssertType((x = 10), "int");
AssertType(x = 10, "int");
AssertType(x, "union");
AssertType(10, "int");
AssertType(x.toString(), "string");
AssertType(x.toString, "(?number) => string");

        // do not change value
        : ((y = x) && x.toString()))); // x is boolean
AssertType(((y = x) && x.toString()), "string");
AssertType((y = x) && x.toString(), "string");
AssertType((y = x), "boolean");
AssertType(y = x, "boolean");
AssertType(y, "union");
AssertType(x, "boolean");
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");
}


