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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInClassMethods.ts ===
declare function AssertType(value:any, type:string):void;
// Note that type guards affect types of letiables and parameters only and 
// have no effect on members of objects such as properties. 

// letiables in global
let num: number;
AssertType(num, "number");

let let1: string | number;
AssertType(let1, "union");

class C1 {
    constructor(param: string | number) {
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
    // Inside function declaration
    private p1(param: string | number) {
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
    // Inside function declaration
    p2(param: string | number) {
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
    // Inside function declaration
    private static s1(param: string | number) {
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
    // Inside function declaration
    static s2(param: string | number) {
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
}


