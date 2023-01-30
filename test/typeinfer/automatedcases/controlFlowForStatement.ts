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

// === tests/cases/conformance/controlFlow/controlFlowForStatement.ts ===
declare function AssertType(value:any, type:string):void;
let cond: boolean;
AssertType(cond, "boolean");

function a() {
    let x: string | number | boolean;
AssertType(x, "union");

    for (x = ""; cond; x = 5) {
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");
AssertType(cond, "boolean");
AssertType(x = 5, "int");
AssertType(x, "union");
AssertType(5, "int");

        x; // string | number
AssertType(x, "union");
    }
}
function b() {
    let x: string | number | boolean;
AssertType(x, "union");

    for (x = 5; cond; x = x.length) {
AssertType(x = 5, "int");
AssertType(x, "union");
AssertType(5, "int");
AssertType(cond, "boolean");
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
function c() {
    let x: string | number | boolean;
AssertType(x, "union");

    for (x = 5; x = x.toExponential(); x = 5) {
AssertType(x = 5, "int");
AssertType(x, "union");
AssertType(5, "int");
AssertType(x = x.toExponential(), "string");
AssertType(x, "union");
AssertType(x.toExponential(), "string");
AssertType(x.toExponential, "(?number) => string");
AssertType(x = 5, "int");
AssertType(x, "union");
AssertType(5, "int");

        x; // string
AssertType(x, "string");
    }
}
function d() {
    let x: string | number | boolean;
AssertType(x, "union");

    for (x = ""; typeof x === "string"; x = 5) {
AssertType(x = "", "string");
AssertType(x, "union");
AssertType("", "string");
AssertType(typeof x === "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
AssertType(x = 5, "int");
AssertType(x, "union");
AssertType(5, "int");

        x; // string
AssertType(x, "string");
    }
}
function e() {
    let x: string | number | boolean | RegExp;
AssertType(x, "union");

    for (x = "" || 0; typeof x !== "string"; x = "" || true) {
AssertType(x = "" || 0, "int");
AssertType(x, "union");
AssertType("" || 0, "int");
AssertType("", "string");
AssertType(0, "int");
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");
AssertType(x = "" || true, "boolean");
AssertType(x, "union");
AssertType("" || true, "boolean");
AssertType("", "string");
AssertType(true, "boolean");

        x; // number | boolean
AssertType(x, "union");
    }
}
function f() {
    let x: string | number | boolean;
AssertType(x, "union");

    for (; typeof x !== "string";) {
AssertType(typeof x !== "string", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("string", "string");

        x; // number | boolean
AssertType(x, "union");

        if (typeof x === "number") break;
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "union");
AssertType("number", "string");

        x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "union");
AssertType(undefined, "undefined");
    }
    x; // string | number
AssertType(x, "union");
}


