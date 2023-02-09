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

// === tests/cases/conformance/expressions/typeGuards/typeGuardsInClassAccessors.ts ===
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

class ClassWithAccessors {
    // Inside public accessor getter
    get p1() {
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

AssertType(strOrNum, "union");
        return strOrNum;
    }
    // Inside public accessor setter
    set p1(param: string | number) {
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

        // parameter of function declaration
        num = typeof param === "string" && param.length; // string
AssertType(num = typeof param === "string" && param.length, "number");
AssertType(num, "number");
AssertType(typeof param === "string" && param.length, "number");
AssertType(typeof param === "string", "boolean");
AssertType(typeof param, "union");
AssertType(param, "union");
AssertType("string", "string");
AssertType(param.length, "number");

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
    }
    // Inside private accessor getter
    private get pp1() {
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

AssertType(strOrNum, "union");
        return strOrNum;
    }
    // Inside private accessor setter
    private set pp1(param: string | number) {
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

        // parameter of function declaration
        num = typeof param === "string" && param.length; // string
AssertType(num = typeof param === "string" && param.length, "number");
AssertType(num, "number");
AssertType(typeof param === "string" && param.length, "number");
AssertType(typeof param === "string", "boolean");
AssertType(typeof param, "union");
AssertType(param, "union");
AssertType("string", "string");
AssertType(param.length, "number");

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
    }
    // Inside static accessor getter
    static get s1() {
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

AssertType(strOrNum, "union");
        return strOrNum;
    }
    // Inside static accessor setter
    static set s1(param: string | number) {
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

        // parameter of function declaration
        num = typeof param === "string" && param.length; // string
AssertType(num = typeof param === "string" && param.length, "number");
AssertType(num, "number");
AssertType(typeof param === "string" && param.length, "number");
AssertType(typeof param === "string", "boolean");
AssertType(typeof param, "union");
AssertType(param, "union");
AssertType("string", "string");
AssertType(param.length, "number");

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
    }
    // Inside private static accessor getter
    private static get ss1() {
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

AssertType(strOrNum, "union");
        return strOrNum;
    }
    // Inside private static accessor setter
    private static set ss1(param: string | number) {
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

        // parameter of function declaration
        num = typeof param === "string" && param.length; // string
AssertType(num = typeof param === "string" && param.length, "number");
AssertType(num, "number");
AssertType(typeof param === "string" && param.length, "number");
AssertType(typeof param === "string", "boolean");
AssertType(typeof param, "union");
AssertType(param, "union");
AssertType("string", "string");
AssertType(param.length, "number");

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
    }
}


