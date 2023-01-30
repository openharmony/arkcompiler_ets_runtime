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

// === tests/cases/conformance/controlFlow/controlFlowDoWhileStatement.ts ===
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

    do {
        x; // string
AssertType(x, "string");

    } while (cond)
AssertType(cond, "boolean");
}
function b() {
    let x: string | number;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    do {
        x; // string
AssertType(x, "string");

        x = 42;
AssertType(x = 42, "int");
AssertType(x, "union");
AssertType(42, "int");

        break;
    } while (cond)
AssertType(cond, "boolean");
}
function c() {
    let x: string | number;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    do {
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
    } while (cond)
AssertType(cond, "boolean");
}
function d() {
    let x: string | number;
AssertType(x, "union");

    x = 1000;
AssertType(x = 1000, "int");
AssertType(x, "union");
AssertType(1000, "int");

    do {
        x; // number
AssertType(x, "number");

        x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    } while (x = x.length)
AssertType(x = x.length, "number");
AssertType(x, "union");
AssertType(x.length, "number");

    x; // number
AssertType(x, "number");
}
function e() {
    let x: string | number;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    do {
        x = 42;
AssertType(x = 42, "int");
AssertType(x, "union");
AssertType(42, "int");

    } while (cond)
AssertType(cond, "boolean");

    x; // number
AssertType(x, "number");
}
function f() {
    let x: string | number | boolean | RegExp | Function;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    do {
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

    } while (cond)
AssertType(cond, "boolean");

    x; // number | boolean | RegExp
AssertType(x, "union");
}
function g() {
    let x: string | number | boolean | RegExp | Function;
AssertType(x, "union");

    x = "";
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");

    do {
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

    } while (true)
AssertType(true, "boolean");

    x; // number
AssertType(x, "number");
}


