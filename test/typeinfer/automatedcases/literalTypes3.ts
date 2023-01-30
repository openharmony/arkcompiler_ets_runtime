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

// === tests/cases/conformance/types/literal/literalTypes3.ts ===
declare function AssertType(value:any, type:string):void;
function f1(s: string) {
    if (s === "foo") {
AssertType(s === "foo", "boolean");
AssertType(s, "string");
AssertType("foo", "string");

        s;  // "foo"
AssertType(s, "string");
    }
    if (s === "foo" || s === "bar") {
AssertType(s === "foo" || s === "bar", "boolean");
AssertType(s === "foo", "boolean");
AssertType(s, "string");
AssertType("foo", "string");
AssertType(s === "bar", "boolean");
AssertType(s, "string");
AssertType("bar", "string");

        s;  // "foo" | "bar"
AssertType(s, "union");
    }
}

function f2(s: string) {
    switch (s) {
AssertType(s, "string");

        case "foo":
AssertType("foo", "string");

        case "bar":
AssertType("bar", "string");

            s;  // "foo" | "bar"
AssertType(s, "union");

        case "baz":
AssertType("baz", "string");

            s;  // "foo" | "bar" | "baz"
AssertType(s, "union");

            break;
        default:
            s;  // string
AssertType(s, "string");
    }
}

function f3(s: string) {
AssertType(s === "foo" || s === "bar" ? s : undefined, "union");
AssertType(s === "foo" || s === "bar", "boolean");
AssertType(s === "foo", "boolean");
AssertType(s, "string");
AssertType("foo", "string");
AssertType(s === "bar", "boolean");
AssertType(s, "string");
AssertType("bar", "string");
AssertType(s, "union");
AssertType(undefined, "undefined");
    return s === "foo" || s === "bar" ? s : undefined;  // "foo" | "bar" | undefined
}

function f4(x: number) {
    if (x === 1 || x === 2) {
AssertType(x === 1 || x === 2, "boolean");
AssertType(x === 1, "boolean");
AssertType(x, "number");
AssertType(1, "int");
AssertType(x === 2, "boolean");
AssertType(x, "number");
AssertType(2, "int");

AssertType(x, "union");
        return x;  // 1 | 2
    }
    throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
}

function f5(x: number, y: 1 | 2) {
    if (x === 0 || x === y) {
AssertType(x === 0 || x === y, "boolean");
AssertType(x === 0, "boolean");
AssertType(x, "number");
AssertType(0, "int");
AssertType(x === y, "boolean");
AssertType(x, "number");
AssertType(y, "union");

        x;  // 0 | 1 | 2
AssertType(x, "union");
    }
}

function f6(x: number, y: 1 | 2) {
    if (y === x || 0 === x) {
AssertType(y === x || 0 === x, "boolean");
AssertType(y === x, "boolean");
AssertType(y, "union");
AssertType(x, "number");
AssertType(0 === x, "boolean");
AssertType(0, "int");
AssertType(x, "number");

        x;  // 0 | 1 | 2
AssertType(x, "union");
    }
}

function f7(x: number | "foo" | "bar", y: 1 | 2 | string) {
    if (x === y) {
AssertType(x === y, "boolean");
AssertType(x, "union");
AssertType(y, "union");

        x;  // "foo" | "bar" | 1 | 2
AssertType(x, "union");
    }
}

function f8(x: number | "foo" | "bar") {
    switch (x) {
AssertType(x, "union");

        case 1:
AssertType(1, "int");

        case 2:
AssertType(2, "int");

            x;  // 1 | 2
AssertType(x, "union");

            break;
        case "foo":
AssertType("foo", "string");

            x;  // "foo"
AssertType(x, "string");

            break;
        default:
            x;  // number | "bar"
AssertType(x, "union");
    }
}

