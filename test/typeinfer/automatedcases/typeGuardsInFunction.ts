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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInFunction.ts ===
declare function AssertType(value:any, type:string):void;
// Note that type guards affect types of letiables and parameters only and 
// have no effect on members of objects such as properties. 

// letiables in global
let num: number;
AssertType(num, "number");

let let1: string | number;
AssertType(let1, "union");

// Inside function declaration
function f(param: string | number) {
    // global lets in function declaration
    num =  typeof let1 === "string" && let1.length; // string
AssertType(num =  typeof let1 === "string" && let1.length, "number");
AssertType(num, "number");
AssertType(typeof let1 === "string" && let1.length, "number");
AssertType(typeof let1 === "string", "boolean");
AssertType(typeof let1, "union");
AssertType(let1, "union");
AssertType("string", "string");
AssertType(let1.length, "number");

    // letiables in function declaration
    let let2: string | number;
AssertType(let2, "union");

    num = typeof let2 === "string" && let2.length; // string
AssertType(num = typeof let2 === "string" && let2.length, "number");
AssertType(num, "number");
AssertType(typeof let2 === "string" && let2.length, "number");
AssertType(typeof let2 === "string", "boolean");
AssertType(typeof let2, "union");
AssertType(let2, "union");
AssertType("string", "string");
AssertType(let2.length, "number");

    // parameters in function declaration
    num = typeof param === "string" && param.length; // string
AssertType(num = typeof param === "string" && param.length, "number");
AssertType(num, "number");
AssertType(typeof param === "string" && param.length, "number");
AssertType(typeof param === "string", "boolean");
AssertType(typeof param, "union");
AssertType(param, "union");
AssertType("string", "string");
AssertType(param.length, "number");
}
// local function declaration
function f1(param: string | number) {
    let let2: string | number;
AssertType(let2, "union");

    function f2(param1: string | number) {
AssertType(f2, "(union) => void");
AssertType(param1, "union");

        // global lets in function declaration
        num = typeof let1 === "string" && let1.length; // string
AssertType(num = typeof let1 === "string" && let1.length, "number");
AssertType(num, "number");
AssertType(typeof let1 === "string" && let1.length, "number");
AssertType(typeof let1 === "string", "boolean");
AssertType(typeof let1, "union");
AssertType(let1, "union");
AssertType("string", "string");
AssertType(let1.length, "number");

        // letiables from outer function declaration
        num = typeof let2 === "string" && let2.length; // string
AssertType(num = typeof let2 === "string" && let2.length, "number");
AssertType(num, "number");
AssertType(typeof let2 === "string" && let2.length, "number");
AssertType(typeof let2 === "string", "boolean");
AssertType(typeof let2, "union");
AssertType(let2, "union");
AssertType("string", "string");
AssertType(let2.length, "number");

        // parameters in outer declaration
        num = typeof param === "string" && param.length; // string
AssertType(num = typeof param === "string" && param.length, "number");
AssertType(num, "number");
AssertType(typeof param === "string" && param.length, "number");
AssertType(typeof param === "string", "boolean");
AssertType(typeof param, "union");
AssertType(param, "union");
AssertType("string", "string");
AssertType(param.length, "number");

        // local
        let let3: string | number;
AssertType(let3, "union");

        num = typeof let3 === "string" && let3.length; // string
AssertType(num = typeof let3 === "string" && let3.length, "number");
AssertType(num, "number");
AssertType(typeof let3 === "string" && let3.length, "number");
AssertType(typeof let3 === "string", "boolean");
AssertType(typeof let3, "union");
AssertType(let3, "union");
AssertType("string", "string");
AssertType(let3.length, "number");

        num = typeof param1 === "string" && param1.length; // string
AssertType(num = typeof param1 === "string" && param1.length, "number");
AssertType(num, "number");
AssertType(typeof param1 === "string" && param1.length, "number");
AssertType(typeof param1 === "string", "boolean");
AssertType(typeof param1, "union");
AssertType(param1, "union");
AssertType("string", "string");
AssertType(param1.length, "number");
    }
}
// Function expression
function f2(param: string | number) {
    // letiables in function declaration
    let let2: string | number;
AssertType(let2, "union");

    // letiables in function expressions
    let r = function (param1: string | number) {
AssertType(r, "void");
AssertType(function (param1: string | number) {        // global lets in function declaration        num = typeof let1 === "string" && let1.length; // string        // letiables from outer function declaration        num = typeof let2 === "string" && let2.length; // string        // parameters in outer declaration        num = typeof param === "string" && param.length; // string        // local        let let3: string | number;        num = typeof let3 === "string" && let3.length; // string        num = typeof param1 === "string" && param1.length; // string    } (param), "void");
AssertType(function (param1: string | number) {        // global lets in function declaration        num = typeof let1 === "string" && let1.length; // string        // letiables from outer function declaration        num = typeof let2 === "string" && let2.length; // string        // parameters in outer declaration        num = typeof param === "string" && param.length; // string        // local        let let3: string | number;        num = typeof let3 === "string" && let3.length; // string        num = typeof param1 === "string" && param1.length; // string    }, "(union) => void");
AssertType(param1, "union");

        // global lets in function declaration
        num = typeof let1 === "string" && let1.length; // string
AssertType(num = typeof let1 === "string" && let1.length, "number");
AssertType(num, "number");
AssertType(typeof let1 === "string" && let1.length, "number");
AssertType(typeof let1 === "string", "boolean");
AssertType(typeof let1, "union");
AssertType(let1, "union");
AssertType("string", "string");
AssertType(let1.length, "number");

        // letiables from outer function declaration
        num = typeof let2 === "string" && let2.length; // string
AssertType(num = typeof let2 === "string" && let2.length, "number");
AssertType(num, "number");
AssertType(typeof let2 === "string" && let2.length, "number");
AssertType(typeof let2 === "string", "boolean");
AssertType(typeof let2, "union");
AssertType(let2, "union");
AssertType("string", "string");
AssertType(let2.length, "number");

        // parameters in outer declaration
        num = typeof param === "string" && param.length; // string
AssertType(num = typeof param === "string" && param.length, "number");
AssertType(num, "number");
AssertType(typeof param === "string" && param.length, "number");
AssertType(typeof param === "string", "boolean");
AssertType(typeof param, "union");
AssertType(param, "union");
AssertType("string", "string");
AssertType(param.length, "number");

        // local
        let let3: string | number;
AssertType(let3, "union");

        num = typeof let3 === "string" && let3.length; // string
AssertType(num = typeof let3 === "string" && let3.length, "number");
AssertType(num, "number");
AssertType(typeof let3 === "string" && let3.length, "number");
AssertType(typeof let3 === "string", "boolean");
AssertType(typeof let3, "union");
AssertType(let3, "union");
AssertType("string", "string");
AssertType(let3.length, "number");

        num = typeof param1 === "string" && param1.length; // string
AssertType(num = typeof param1 === "string" && param1.length, "number");
AssertType(num, "number");
AssertType(typeof param1 === "string" && param1.length, "number");
AssertType(typeof param1 === "string", "boolean");
AssertType(typeof param1, "union");
AssertType(param1, "union");
AssertType("string", "string");
AssertType(param1.length, "number");

    } (param);
AssertType(param, "union");
}
// Arrow expression
function f3(param: string | number) {
    // letiables in function declaration
    let let2: string | number;
AssertType(let2, "union");

    // letiables in function expressions
    let r = ((param1: string | number) => {
AssertType(r, "void");
AssertType(((param1: string | number) => {        // global lets in function declaration        num = typeof let1 === "string" && let1.length; // string        // letiables from outer function declaration        num = typeof let2 === "string" && let2.length; // string        // parameters in outer declaration        num = typeof param === "string" && param.length; // string        // local        let let3: string | number;        num = typeof let3 === "string" && let3.length; // string        num = typeof param1 === "string" && param1.length; // string    })(param), "void");
AssertType(((param1: string | number) => {        // global lets in function declaration        num = typeof let1 === "string" && let1.length; // string        // letiables from outer function declaration        num = typeof let2 === "string" && let2.length; // string        // parameters in outer declaration        num = typeof param === "string" && param.length; // string        // local        let let3: string | number;        num = typeof let3 === "string" && let3.length; // string        num = typeof param1 === "string" && param1.length; // string    }), "(union) => void");
AssertType((param1: string | number) => {        // global lets in function declaration        num = typeof let1 === "string" && let1.length; // string        // letiables from outer function declaration        num = typeof let2 === "string" && let2.length; // string        // parameters in outer declaration        num = typeof param === "string" && param.length; // string        // local        let let3: string | number;        num = typeof let3 === "string" && let3.length; // string        num = typeof param1 === "string" && param1.length; // string    }, "(union) => void");
AssertType(param1, "union");

        // global lets in function declaration
        num = typeof let1 === "string" && let1.length; // string
AssertType(num = typeof let1 === "string" && let1.length, "number");
AssertType(num, "number");
AssertType(typeof let1 === "string" && let1.length, "number");
AssertType(typeof let1 === "string", "boolean");
AssertType(typeof let1, "union");
AssertType(let1, "union");
AssertType("string", "string");
AssertType(let1.length, "number");

        // letiables from outer function declaration
        num = typeof let2 === "string" && let2.length; // string
AssertType(num = typeof let2 === "string" && let2.length, "number");
AssertType(num, "number");
AssertType(typeof let2 === "string" && let2.length, "number");
AssertType(typeof let2 === "string", "boolean");
AssertType(typeof let2, "union");
AssertType(let2, "union");
AssertType("string", "string");
AssertType(let2.length, "number");

        // parameters in outer declaration
        num = typeof param === "string" && param.length; // string
AssertType(num = typeof param === "string" && param.length, "number");
AssertType(num, "number");
AssertType(typeof param === "string" && param.length, "number");
AssertType(typeof param === "string", "boolean");
AssertType(typeof param, "union");
AssertType(param, "union");
AssertType("string", "string");
AssertType(param.length, "number");

        // local
        let let3: string | number;
AssertType(let3, "union");

        num = typeof let3 === "string" && let3.length; // string
AssertType(num = typeof let3 === "string" && let3.length, "number");
AssertType(num, "number");
AssertType(typeof let3 === "string" && let3.length, "number");
AssertType(typeof let3 === "string", "boolean");
AssertType(typeof let3, "union");
AssertType(let3, "union");
AssertType("string", "string");
AssertType(let3.length, "number");

        num = typeof param1 === "string" && param1.length; // string
AssertType(num = typeof param1 === "string" && param1.length, "number");
AssertType(num, "number");
AssertType(typeof param1 === "string" && param1.length, "number");
AssertType(typeof param1 === "string", "boolean");
AssertType(typeof param1, "union");
AssertType(param1, "union");
AssertType("string", "string");
AssertType(param1.length, "number");

    })(param);
AssertType(param, "union");
}
// Return type of function
// Inside function declaration
let strOrNum: string | number;
AssertType(strOrNum, "union");

function f4() {
    let let2: string | number = strOrNum;
AssertType(let2, "union");
AssertType(strOrNum, "union");

AssertType(let2, "union");
    return let2;
}
strOrNum = typeof f4() === "string" && f4(); // string | number 
AssertType(strOrNum = typeof f4() === "string" && f4(), "union");
AssertType(strOrNum, "union");
AssertType(typeof f4() === "string" && f4(), "union");
AssertType(typeof f4() === "string", "boolean");
AssertType(typeof f4(), "union");
AssertType(f4(), "union");
AssertType(f4, "() => union");
AssertType("string", "string");
AssertType(f4(), "union");
AssertType(f4, "() => union");


