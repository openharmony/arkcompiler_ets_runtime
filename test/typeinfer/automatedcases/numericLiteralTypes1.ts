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

// === tests/cases/conformance/types/literal/numericLiteralTypes1.ts ===
declare function AssertType(value:any, type:string):void;
type A1 = 1;
type A2 = 1.0;
type A3 = 1e0;
type A4 = 10e-1;
type A5 = 1 | 1.0 | 1e0 | 10e-1;

function f1() {
    let a: A1 = 1;
AssertType(a, "int");
AssertType(1, "int");

    let a: A2 = 1;
AssertType(a, "int");
AssertType(1, "int");

    let a: A3 = 1;
AssertType(a, "int");
AssertType(1, "int");

    let a: A4 = 1;
AssertType(a, "int");
AssertType(1, "int");

    let a: A5 = 1;
AssertType(a, "int");
AssertType(1, "int");
}

type B1 = -1 | 0 | 1;
type B2 = 1 | 0 | -1;
type B3 = 0 | -1 | 1;

function f2() {
    let b: B1 = -1;
AssertType(b, "B1");
AssertType(-1, "int");
AssertType(1, "int");

    let b: B2 = 0;
AssertType(b, "B1");
AssertType(0, "int");

    let b: B3 = 1;
AssertType(b, "B1");
AssertType(1, "int");
}

function f3(a: 1, b: 0 | 1 | 2) {
    let x = a + b;
AssertType(x, "number");
AssertType(a + b, "number");
AssertType(a, "int");
AssertType(b, "union");

    let x = a - b;
AssertType(x, "number");
AssertType(a - b, "number");
AssertType(a, "int");
AssertType(b, "union");

    let x = a * b;
AssertType(x, "number");
AssertType(a * b, "number");
AssertType(a, "int");
AssertType(b, "union");

    let x = a / b;
AssertType(x, "number");
AssertType(a / b, "number");
AssertType(a, "int");
AssertType(b, "union");

    let x = a % b;
AssertType(x, "number");
AssertType(a % b, "number");
AssertType(a, "int");
AssertType(b, "union");

    let x = a | b;
AssertType(x, "number");
AssertType(a | b, "number");
AssertType(a, "int");
AssertType(b, "union");

    let x = a & b;
AssertType(x, "number");
AssertType(a & b, "number");
AssertType(a, "int");
AssertType(b, "union");

    let x = a ^ b;
AssertType(x, "number");
AssertType(a ^ b, "number");
AssertType(a, "int");
AssertType(b, "union");

    let x = -b;
AssertType(x, "number");
AssertType(-b, "number");
AssertType(b, "union");

    let x = ~b;
AssertType(x, "number");
AssertType(~b, "number");
AssertType(b, "union");

    let y = a == b;
AssertType(y, "boolean");
AssertType(a == b, "boolean");
AssertType(a, "int");
AssertType(b, "union");

    let y = a != b;
AssertType(y, "boolean");
AssertType(a != b, "boolean");
AssertType(a, "int");
AssertType(b, "union");

    let y = a === b;
AssertType(y, "boolean");
AssertType(a === b, "boolean");
AssertType(a, "int");
AssertType(b, "union");

    let y = a !== b;
AssertType(y, "boolean");
AssertType(a !== b, "boolean");
AssertType(a, "int");
AssertType(b, "union");

    let y = a > b;
AssertType(y, "boolean");
AssertType(a > b, "boolean");
AssertType(a, "int");
AssertType(b, "union");

    let y = a < b;
AssertType(y, "boolean");
AssertType(a < b, "boolean");
AssertType(a, "int");
AssertType(b, "union");

    let y = a >= b;
AssertType(y, "boolean");
AssertType(a >= b, "boolean");
AssertType(a, "int");
AssertType(b, "union");

    let y = a <= b;
AssertType(y, "boolean");
AssertType(a <= b, "boolean");
AssertType(a, "int");
AssertType(b, "union");

    let y = !b;
AssertType(y, "boolean");
AssertType(!b, "boolean");
AssertType(b, "union");
}

function f4(a: 1, b: 0 | 1 | 2) {
    a++;
AssertType(a++, "number");
AssertType(a, "number");

    b++;
AssertType(b++, "number");
AssertType(b, "number");
}

declare function g(x: 0): string;
declare function g(x: 1): boolean;
declare function g(x: number): number;

function f5(a: 1, b: 0 | 1 | 2) {
    let z1 = g(0);
AssertType(z1, "string");
AssertType(g(0), "string");
AssertType(g, "{ (0): string; (1): boolean; (number): number; }");
AssertType(0, "int");

    let z2 = g(1);
AssertType(z2, "boolean");
AssertType(g(1), "boolean");
AssertType(g, "{ (0): string; (1): boolean; (number): number; }");
AssertType(1, "int");

    let z3 = g(2);
AssertType(z3, "number");
AssertType(g(2), "number");
AssertType(g, "{ (0): string; (1): boolean; (number): number; }");
AssertType(2, "int");

    let z4 = g(a);
AssertType(z4, "boolean");
AssertType(g(a), "boolean");
AssertType(g, "{ (0): string; (1): boolean; (number): number; }");
AssertType(a, "int");

    let z5 = g(b);
AssertType(z5, "number");
AssertType(g(b), "number");
AssertType(g, "{ (0): string; (1): boolean; (number): number; }");
AssertType(b, "union");
}

function assertNever(x: never): never {
    throw new Error("Unexpected value");
AssertType(new Error("Unexpected value"), "Error");
AssertType(Error, "ErrorConstructor");
AssertType("Unexpected value", "string");
}

type Tag = 0 | 1 | 2;

function f10(x: Tag) {
    switch (x) {
AssertType(x, "Tag");

        case 0: 
AssertType(0, "int");
AssertType("a", "string");
return "a";

        case 1: 
AssertType(1, "int");
AssertType("b", "string");
return "b";

        case 2: 
AssertType(2, "int");
AssertType("c", "string");
return "c";
    }
}

function f11(x: Tag) {
    switch (x) {
AssertType(x, "Tag");

        case 0: 
AssertType(0, "int");
AssertType("a", "string");
return "a";

        case 1: 
AssertType(1, "int");
AssertType("b", "string");
return "b";

        case 2: 
AssertType(2, "int");
AssertType("c", "string");
return "c";
    }
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
    return assertNever(x);
}

function f12(x: Tag) {
    if (x) {
AssertType(x, "Tag");

        x;
AssertType(x, "union");
    }
    else {
        x;
AssertType(x, "Tag");
    }
}

function f13(x: Tag) {
    if (x === 0 || x === 2) {
AssertType(x === 0 || x === 2, "boolean");
AssertType(x === 0, "boolean");
AssertType(x, "Tag");
AssertType(0, "int");
AssertType(x === 2, "boolean");
AssertType(x, "union");
AssertType(2, "int");

        x;
AssertType(x, "union");
    }
    else {
        x;
AssertType(x, "int");
    }
}

function f14(x: 0 | 1 | 2, y: string) {
    let a = x && y;
AssertType(a, "string");
AssertType(x && y, "string");
AssertType(x, "union");
AssertType(y, "string");

    let b = x || y;
AssertType(b, "union");
AssertType(x || y, "union");
AssertType(x, "union");
AssertType(y, "string");
}

function f15(x: 0 | false, y: 1 | "one") {
    let a = x && y;
AssertType(a, "union");
AssertType(x && y, "union");
AssertType(x, "union");
AssertType(y, "union");

    let b = y && x;
AssertType(b, "union");
AssertType(y && x, "union");
AssertType(y, "union");
AssertType(x, "union");

    let c = x || y;
AssertType(c, "union");
AssertType(x || y, "union");
AssertType(x, "union");
AssertType(y, "union");

    let d = y || x;
AssertType(d, "union");
AssertType(y || x, "union");
AssertType(y, "union");
AssertType(x, "union");

    let e = !x;
AssertType(e, "boolean");
AssertType(!x, "boolean");
AssertType(x, "union");

    let f = !y;
AssertType(f, "boolean");
AssertType(!y, "boolean");
AssertType(y, "union");
}

type Item =
    { kind: 0, a: string } |
    { kind: 1, b: string } |
    { kind: 2, c: string };

function f20(x: Item) {
    switch (x.kind) {
AssertType(x.kind, "union");

        case 0: 
AssertType(0, "int");
AssertType(x.a, "string");
return x.a;

        case 1: 
AssertType(1, "int");
AssertType(x.b, "string");
return x.b;

        case 2: 
AssertType(2, "int");
AssertType(x.c, "string");
return x.c;
    }
}

function f21(x: Item) {
    switch (x.kind) {
AssertType(x.kind, "union");

        case 0: 
AssertType(0, "int");
AssertType(x.a, "string");
return x.a;

        case 1: 
AssertType(1, "int");
AssertType(x.b, "string");
return x.b;

        case 2: 
AssertType(2, "int");
AssertType(x.c, "string");
return x.c;
    }
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
    return assertNever(x);
}

