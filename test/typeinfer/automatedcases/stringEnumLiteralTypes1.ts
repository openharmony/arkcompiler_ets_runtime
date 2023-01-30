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

// === tests/cases/conformance/types/literal/stringEnumLiteralTypes1.ts ===
declare function AssertType(value:any, type:string):void;
const enum Choice { Unknown = "", Yes = "yes", No = "no" };

type YesNo = Choice.Yes | Choice.No;
type NoYes = Choice.No | Choice.Yes;
type UnknownYesNo = Choice.Unknown | Choice.Yes | Choice.No;

function f1() {
    let a: YesNo;
AssertType(a, "YesNo");

    let a: NoYes;
AssertType(a, "YesNo");

    let a: Choice.Yes | Choice.No;
AssertType(a, "YesNo");
AssertType(Choice, "any");
AssertType(Choice, "any");

    let a: Choice.No | Choice.Yes;
AssertType(a, "YesNo");
AssertType(Choice, "any");
AssertType(Choice, "any");
}

function f2(a: YesNo, b: UnknownYesNo, c: Choice) {
    b = a;
AssertType(b = a, "YesNo");
AssertType(b, "UnknownYesNo");
AssertType(a, "YesNo");

    c = a;
AssertType(c = a, "YesNo");
AssertType(c, "Choice");
AssertType(a, "YesNo");

    c = b;
AssertType(c = b, "union");
AssertType(c, "Choice");
AssertType(b, "union");
}

function f3(a: Choice.Yes, b: YesNo) {
    let x = a + b;
AssertType(x, "string");
AssertType(a + b, "string");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = a == b;
AssertType(y, "boolean");
AssertType(a == b, "boolean");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = a != b;
AssertType(y, "boolean");
AssertType(a != b, "boolean");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = a === b;
AssertType(y, "boolean");
AssertType(a === b, "boolean");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = a !== b;
AssertType(y, "boolean");
AssertType(a !== b, "boolean");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = a > b;
AssertType(y, "boolean");
AssertType(a > b, "boolean");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = a < b;
AssertType(y, "boolean");
AssertType(a < b, "boolean");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = a >= b;
AssertType(y, "boolean");
AssertType(a >= b, "boolean");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = a <= b;
AssertType(y, "boolean");
AssertType(a <= b, "boolean");
AssertType(a, "Choice.Yes");
AssertType(b, "YesNo");

    let y = !b;
AssertType(y, "boolean");
AssertType(!b, "boolean");
AssertType(b, "YesNo");
}

declare function g(x: Choice.Yes): string;
declare function g(x: Choice.No): boolean;
declare function g(x: Choice): number;

function f5(a: YesNo, b: UnknownYesNo, c: Choice) {
    let z1 = g(Choice.Yes);
AssertType(z1, "string");
AssertType(g(Choice.Yes), "string");
AssertType(g, "{ (Choice.Yes): string; (Choice.No): boolean; (Choice): number; }");
AssertType(Choice.Yes, "Choice.Yes");

    let z2 = g(Choice.No);
AssertType(z2, "boolean");
AssertType(g(Choice.No), "boolean");
AssertType(g, "{ (Choice.Yes): string; (Choice.No): boolean; (Choice): number; }");
AssertType(Choice.No, "Choice.No");

    let z3 = g(a);
AssertType(z3, "number");
AssertType(g(a), "number");
AssertType(g, "{ (Choice.Yes): string; (Choice.No): boolean; (Choice): number; }");
AssertType(a, "YesNo");

    let z4 = g(b);
AssertType(z4, "number");
AssertType(g(b), "number");
AssertType(g, "{ (Choice.Yes): string; (Choice.No): boolean; (Choice): number; }");
AssertType(b, "UnknownYesNo");

    let z5 = g(c);
AssertType(z5, "number");
AssertType(g(c), "number");
AssertType(g, "{ (Choice.Yes): string; (Choice.No): boolean; (Choice): number; }");
AssertType(c, "Choice");
}

function assertNever(x: never): never {
    throw new Error("Unexpected value");
AssertType(new Error("Unexpected value"), "Error");
AssertType(Error, "ErrorConstructor");
AssertType("Unexpected value", "string");
}

function f10(x: YesNo) {
    switch (x) {
AssertType(x, "YesNo");

        case Choice.Yes: 
AssertType(Choice.Yes, "Choice.Yes");
AssertType("true", "string");
return "true";

        case Choice.No: 
AssertType(Choice.No, "Choice.No");
AssertType("false", "string");
return "false";
    }
}

function f11(x: YesNo) {
    switch (x) {
AssertType(x, "YesNo");

        case Choice.Yes: 
AssertType(Choice.Yes, "Choice.Yes");
AssertType("true", "string");
return "true";

        case Choice.No: 
AssertType(Choice.No, "Choice.No");
AssertType("false", "string");
return "false";
    }
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
    return assertNever(x);
}

function f12(x: UnknownYesNo) {
    if (x) {
AssertType(x, "UnknownYesNo");

        x;
AssertType(x, "union");
    }
    else {
        x;
AssertType(x, "UnknownYesNo");
    }
}

function f13(x: UnknownYesNo) {
    if (x === Choice.Yes) {
AssertType(x === Choice.Yes, "boolean");
AssertType(x, "UnknownYesNo");
AssertType(Choice.Yes, "Choice.Yes");

        x;
AssertType(x, "Choice.Yes");
    }
    else {
        x;
AssertType(x, "union");
    }
}

type Item =
    { kind: Choice.Yes, a: string } |
    { kind: Choice.No, b: string };

function f20(x: Item) {
    switch (x.kind) {
AssertType(x.kind, "union");

        case Choice.Yes: 
AssertType(Choice.Yes, "Choice.Yes");
AssertType(x.a, "string");
return x.a;

        case Choice.No: 
AssertType(Choice.No, "Choice.No");
AssertType(x.b, "string");
return x.b;
    }
}

function f21(x: Item) {
    switch (x.kind) {
AssertType(x.kind, "union");

        case Choice.Yes: 
AssertType(Choice.Yes, "Choice.Yes");
AssertType(x.a, "string");
return x.a;

        case Choice.No: 
AssertType(Choice.No, "Choice.No");
AssertType(x.b, "string");
return x.b;
    }
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
    return assertNever(x);
}

