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

// === tests/cases/conformance/expressions/typeGuards/typeGuardTypeOfUndefined.ts ===
declare function AssertType(value:any, type:string):void;
// undefined type guard adds no new type information
function test1(a: any) {
    if (typeof a !== "undefined") {
AssertType(typeof a !== "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "any");
AssertType("undefined", "string");

        if (typeof a === "boolean") {
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "any");
AssertType("boolean", "string");

            a;
AssertType(a, "boolean");
        }
        else {
            a;
AssertType(a, "any");
        }
    }
    else {
        a;
AssertType(a, "undefined");
    }
}

function test2(a: any) {
    if (typeof a === "undefined") {
AssertType(typeof a === "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "any");
AssertType("undefined", "string");

        if (typeof a === "boolean") {
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "undefined");
AssertType("boolean", "string");

            a;
AssertType(a, "never");
        }
        else {
            a;
AssertType(a, "undefined");
        }
    }
    else {
        a;
AssertType(a, "any");
    }
}

function test3(a: any) {
    if (typeof a === "undefined" || typeof a === "boolean") {
AssertType(typeof a === "undefined" || typeof a === "boolean", "boolean");
AssertType(typeof a === "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "any");
AssertType("undefined", "string");
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "any");
AssertType("boolean", "string");

		a;
AssertType(a, "boolean");
    }
    else {
        a;
AssertType(a, "any");
    }
}

function test4(a: any) {
    if (typeof a !== "undefined" && typeof a === "boolean") {
AssertType(typeof a !== "undefined" && typeof a === "boolean", "boolean");
AssertType(typeof a !== "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "any");
AssertType("undefined", "string");
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "any");
AssertType("boolean", "string");

		a;
AssertType(a, "boolean");
    }
    else {
        a;
AssertType(a, "any");
    }
}

function test5(a: boolean | void) {
    if (typeof a !== "undefined") {
AssertType(typeof a !== "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");

        if (typeof a === "boolean") {
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "boolean");
AssertType("boolean", "string");

            a;
AssertType(a, "boolean");
        }
        else {
            a;
AssertType(a, "never");
        }
    }
    else {
        a;
AssertType(a, "undefined");
    }
}

function test6(a: boolean | void) {
    if (typeof a === "undefined") {
AssertType(typeof a === "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");

        if (typeof a === "boolean") {
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "undefined");
AssertType("boolean", "string");

            a;
AssertType(a, "never");
        }
        else {
            a;
AssertType(a, "undefined");
        }
    }
    else {
        a;
AssertType(a, "boolean");
    }
}

function test7(a: boolean | void) {
    if (typeof a === "undefined" || typeof a === "boolean") {
AssertType(typeof a === "undefined" || typeof a === "boolean", "boolean");
AssertType(typeof a === "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "boolean");
AssertType("boolean", "string");

		a;
AssertType(a, "boolean");
    }
    else {
        a;
AssertType(a, "never");
    }
}

function test8(a: boolean | void) {
    if (typeof a !== "undefined" && typeof a === "boolean") {
AssertType(typeof a !== "undefined" && typeof a === "boolean", "boolean");
AssertType(typeof a !== "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "boolean");
AssertType("boolean", "string");

		a;
AssertType(a, "boolean");
    }
    else {
        a;
AssertType(a, "undefined");
    }
}

function test9(a: boolean | number) {
    if (typeof a !== "undefined") {
AssertType(typeof a !== "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");

        if (typeof a === "boolean") {
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("boolean", "string");

            a;
AssertType(a, "boolean");
        }
        else {
            a;
AssertType(a, "number");
        }
    }
    else {
        a;
AssertType(a, "undefined");
    }
}

function test10(a: boolean | number) {
    if (typeof a === "undefined") {
AssertType(typeof a === "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");

        if (typeof a === "boolean") {
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "undefined");
AssertType("boolean", "string");

            a;
AssertType(a, "never");
        }
        else {
            a;
AssertType(a, "undefined");
        }
    }
    else {
        a;
AssertType(a, "union");
    }
}

function test11(a: boolean | number) {
    if (typeof a === "undefined" || typeof a === "boolean") {
AssertType(typeof a === "undefined" || typeof a === "boolean", "boolean");
AssertType(typeof a === "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("boolean", "string");

		a;
AssertType(a, "boolean");
    }
    else {
        a;
AssertType(a, "number");
    }
}

function test12(a: boolean | number) {
    if (typeof a !== "undefined" && typeof a === "boolean") {
AssertType(typeof a !== "undefined" && typeof a === "boolean", "boolean");
AssertType(typeof a !== "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("boolean", "string");

		a;
AssertType(a, "boolean");
    }
    else {
        a;
AssertType(a, "number");
    }
}

function test13(a: boolean | number | void) {
    if (typeof a !== "undefined") {
AssertType(typeof a !== "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");

        if (typeof a === "boolean") {
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("boolean", "string");

            a;
AssertType(a, "boolean");
        }
        else {
            a;
AssertType(a, "number");
        }
    }
    else {
        a;
AssertType(a, "undefined");
    }
}

function test14(a: boolean | number | void) {
    if (typeof a === "undefined") {
AssertType(typeof a === "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");

        if (typeof a === "boolean") {
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "undefined");
AssertType("boolean", "string");

            a;
AssertType(a, "never");
        }
        else {
            a;
AssertType(a, "undefined");
        }
    }
    else {
        a;
AssertType(a, "union");
    }
}

function test15(a: boolean | number | void) {
    if (typeof a === "undefined" || typeof a === "boolean") {
AssertType(typeof a === "undefined" || typeof a === "boolean", "boolean");
AssertType(typeof a === "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("boolean", "string");

		a;
AssertType(a, "boolean");
    }
    else {
        a;
AssertType(a, "number");
    }
}

function test16(a: boolean | number | void) {
    if (typeof a !== "undefined" && typeof a === "boolean") {
AssertType(typeof a !== "undefined" && typeof a === "boolean", "boolean");
AssertType(typeof a !== "undefined", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("undefined", "string");
AssertType(typeof a === "boolean", "boolean");
AssertType(typeof a, "union");
AssertType(a, "union");
AssertType("boolean", "string");

		a;
AssertType(a, "boolean");
    }
    else {
        a;
AssertType(a, "number");
    }
}


