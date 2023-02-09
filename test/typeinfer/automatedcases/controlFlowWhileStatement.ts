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

// === tests/cases/conformance/controlFlow/controlFlowWhileStatement.ts ===
declare function AssertType(value:any, type:string):void;
let cond: boolean;
AssertType(cond, "boolean");

function a() {
    let x: string | number;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (cond) {
AssertType(cond, "boolean");

        x; // string
AssertType(x, "string");
    }
}
function b() {
    let x: string | number;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (cond) {
AssertType(cond, "boolean");

        x; // string
AssertType(x, "string");

        x = 42;
AssertType(x = 42, "int");
AssertType(x, "union");
AssertType(42, "int");

        break;
    }
}
function c() {
    let x: string | number;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (cond) {
AssertType(cond, "boolean");

        x; // string
AssertType(x, "string");

        x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "union");
AssertType(undefined, "undefined");

        if (typeof x === "string") continue;
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        break;
    }
}
function d() {
    let x: string | number;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (x = x.length) {
AssertType(x = x.length, "number");
AssertType(x, "union");
AssertType(x.length, "number");

        x; // number
AssertType(x, "number");

        x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");
    }
}
function e() {
    let x: string | number;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (cond) {
AssertType(cond, "boolean");

        x; // string | number
AssertType(x, "union");

        x = 42;
AssertType(x = 42, "int");
AssertType(x, "union");
AssertType(42, "int");

        x; // number
AssertType(x, "number");
    }
    x; // string | number
AssertType(x, "union");
}
function f() {
    let x: string | number | boolean | RegExp | Function;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (cond) {
AssertType(cond, "boolean");

        if (cond) {
AssertType(cond, "boolean");

            x = 42;
AssertType(x = 42, "int");
AssertType(x, "union");
AssertType(42, "int");

            break;
        }
        if (cond) {
AssertType(cond, "boolean");

            x = true;
AssertType(x = true, "boolean");
AssertType(x, "union");
AssertType(true, "boolean");

            continue;
        }
        x = /a/;
AssertType(x = /a/, "RegExp");
AssertType(x, "union");
AssertType(/a/, "RegExp");
    }
    x; // string | number | boolean | RegExp
AssertType(x, "union");
}
function g() {
    let x: string | number | boolean | RegExp | Function;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (true) {
AssertType(true, "boolean");

        if (cond) {
AssertType(cond, "boolean");

            x = 42;
AssertType(x = 42, "int");
AssertType(x, "union");
AssertType(42, "int");

            break;
        }
        if (cond) {
AssertType(cond, "boolean");

            x = true;
AssertType(x = true, "boolean");
AssertType(x, "union");
AssertType(true, "boolean");

            continue;
        }
        x = /a/;
AssertType(x = /a/, "RegExp");
AssertType(x, "union");
AssertType(/a/, "RegExp");
    }
    x; // number
AssertType(x, "number");
}
function h1() {
    let x: string | number | boolean;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (x > 1) {
AssertType(x > 1, "boolean");
AssertType(x, "union");
AssertType(1, "int");

        x; // string | number
AssertType(x, "union");

        x = 1;
AssertType(x = 1, "int");
AssertType(x, "union");
AssertType(1, "int");

        x; // number
AssertType(x, "number");
    }
    x; // string | number
AssertType(x, "union");
}
declare function len(s: string | number): number;
function h2() {
    let x: string | number | boolean;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (cond) {
AssertType(cond, "boolean");

        x = len(x);
AssertType(x = len(x), "number");
AssertType(x, "union");
AssertType(len(x), "number");
AssertType(len, "(union) => number");
AssertType(x, "union");

        x; // number
AssertType(x, "number");
    }
    x; // string | number
AssertType(x, "union");
}
function h3() {
    let x: string | number | boolean;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    while (cond) {
AssertType(cond, "boolean");

        x; // string | number
AssertType(x, "union");

        x = len(x);
AssertType(x = len(x), "number");
AssertType(x, "union");
AssertType(len(x), "number");
AssertType(len, "(union) => number");
AssertType(x, "union");
    }
    x; // string | number
AssertType(x, "union");
}


