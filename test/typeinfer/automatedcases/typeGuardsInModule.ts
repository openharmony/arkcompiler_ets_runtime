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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInModule.ts ===
declare function AssertType(value:any, type:string):void;
// Note that type guards affect types of letiables and parameters only and 
// have no effect on members of objects such as properties. 

// letiables in global
let num: number;
AssertType(num, "number");

let strOrNum: string | number;
AssertType(strOrNum, "union");

let let1: string | number;
AssertType(let1, "union");

// Inside module
module m1 {
    // global lets in function declaration
    num = typeof let1 === "string" && let1.length; // string

    // letiables in module declaration
    let let2: string | number;
    if (typeof let2 === "string") {
        num = let2.length; // string
AssertType(num = let2.length, "number");
AssertType(num, "number");
AssertType(let2.length, "number");
    }
    else {
        num = let2; // number
AssertType(num = let2, "number");
AssertType(num, "number");
AssertType(let2, "number");
    }

    // exported letiable in the module
    export let let3: string | number;
    if (typeof let3 === "string") {
        strOrNum = let3; // string | number
AssertType(strOrNum = let3, "string");
AssertType(strOrNum, "union");
AssertType(let3, "string");
    }
    else {
        strOrNum = let3; // string | number
AssertType(strOrNum = let3, "number");
AssertType(strOrNum, "union");
AssertType(let3, "number");
    }
}
// local module
module m2 {
    let let2: string | number;
    export let let3: string | number;
    module m3 {
        // global lets in function declaration
        num = typeof let1 === "string" && let1.length; // string

        // local letiables from outer module declaration
        num = typeof let2 === "string" && let2.length; // string

        // exported letiable from outer the module
        strOrNum = typeof let3 === "string" && let3; // string | number

        // letiables in module declaration
        let let4: string | number;
        if (typeof let4 === "string") {
            num = let4.length; // string
AssertType(num = let4.length, "number");
AssertType(num, "number");
AssertType(let4.length, "number");
        }
        else {
            num = let4; // number
AssertType(num = let4, "number");
AssertType(num, "number");
AssertType(let4, "number");
        }

        // exported letiable in the module
        export let let5: string | number;
        if (typeof let5 === "string") {
            strOrNum = let5; // string | number
AssertType(strOrNum = let5, "string");
AssertType(strOrNum, "union");
AssertType(let5, "string");
        }
        else {
            strOrNum = let5; // string | number
AssertType(strOrNum = let5, "number");
AssertType(strOrNum, "union");
AssertType(let5, "number");
        }
    }
}
// Dotted module
module m3.m4 {
    // global lets in function declaration
    num = typeof let1 === "string" && let1.length; // string

    // letiables in module declaration
    let let2: string | number;
    if (typeof let2 === "string") {
        num = let2.length; // string
AssertType(num = let2.length, "number");
AssertType(num, "number");
AssertType(let2.length, "number");
    }
    else {
        num = let2; // number
AssertType(num = let2, "number");
AssertType(num, "number");
AssertType(let2, "number");
    }

    // exported letiable in the module
    export let let3: string | number;
    if (typeof let3 === "string") {
        strOrNum = let3; // string | number
AssertType(strOrNum = let3, "string");
AssertType(strOrNum, "union");
AssertType(let3, "string");
    }
    else {
        strOrNum = let3; // string | number
AssertType(strOrNum = let3, "number");
AssertType(strOrNum, "union");
AssertType(let3, "number");
    }
}


