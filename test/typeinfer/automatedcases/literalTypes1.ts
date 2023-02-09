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

// === tests/cases/conformance/types/literal/literalTypes1.ts ===
declare function AssertType(value:any, type:string):void;
let zero: 0 = 0;
AssertType(zero, "int");
AssertType(0, "int");

let one: 1 = 1;
AssertType(one, "int");
AssertType(1, "int");

let two: 2 = 2;
AssertType(two, "int");
AssertType(2, "int");

let oneOrTwo: 1 | 2 = <1 | 2>1;
AssertType(oneOrTwo, "union");
AssertType(<1 | 2>1, "union");
AssertType(1, "int");

function f1(x: 0 | 1 | 2) {
    switch (x) {
AssertType(x, "union");

        case zero:
AssertType(zero, "int");

            x;
AssertType(x, "int");

            break;
        case one:
AssertType(one, "int");

            x;
AssertType(x, "int");

            break;
        case two:
AssertType(two, "int");

            x;
AssertType(x, "int");

            break;
        default:
            x;
AssertType(x, "never");
    }
}

function f2(x: 0 | 1 | 2) {
    switch (x) {
AssertType(x, "union");

        case zero:
AssertType(zero, "int");

            x;
AssertType(x, "int");

            break;
        case oneOrTwo:
AssertType(oneOrTwo, "union");

            x;
AssertType(x, "union");

            break;
        default:
            x;
AssertType(x, "union");
    }
}

type Falsy = false | 0 | "" | null | undefined;

function f3(x: Falsy) {
    if (x) {
AssertType(x, "Falsy");

        x;
AssertType(x, "never");
    }
    else {
        x;
AssertType(x, "Falsy");
    }
}

function f4(x: 0 | 1 | true | string) {
    switch (x) {
AssertType(x, "union");

        case 0:
AssertType(0, "int");

            x;
AssertType(x, "int");

            break;
        case 1:
AssertType(1, "int");

            x;
AssertType(x, "int");

            break;
        case "abc":
AssertType("abc", "string");

        case "def":
AssertType("def", "string");

            x;
AssertType(x, "union");

            break;
        case null:
AssertType(null, "null");

            x;
AssertType(x, "never");

            break;
        case undefined:
AssertType(undefined, "undefined");

            x;
AssertType(x, "never");

            break;
        default:
            x;
AssertType(x, "union");
    }
}

function f5(x: string | number | boolean) {
    switch (x) {
AssertType(x, "union");

        case "abc":
AssertType("abc", "string");

            x;
AssertType(x, "string");

            break;
        case 0:
AssertType(0, "int");

        case 1:
AssertType(1, "int");

            x;
AssertType(x, "union");

            break;
        case true:
AssertType(true, "boolean");

            x;
AssertType(x, "boolean");

            break;
        case "hello":
AssertType("hello", "string");

        case 123:
AssertType(123, "int");

            x;
AssertType(x, "union");

            break;
        default:
            x;
AssertType(x, "union");
    }
}

