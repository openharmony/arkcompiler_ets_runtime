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

// === tests/cases/conformance/types/literal/booleanLiteralTypes1.ts ===
declare function AssertType(value:any, type:string):void;
type A1 = true | false;
type A2 = false | true;

function f1() {
    let a: A1;
AssertType(a, "A1");

    let a: A2;
AssertType(a, "A1");

    let a: true | false;
AssertType(a, "A1");
AssertType(true, "boolean");
AssertType(false, "boolean");

    let a: false | true;
AssertType(a, "A1");
AssertType(false, "boolean");
AssertType(true, "boolean");
}

function f2(a: true | false, b: boolean) {
    a = b;
AssertType(a = b, "boolean");
AssertType(a, "boolean");
AssertType(b, "boolean");

    b = a;
AssertType(b = a, "boolean");
AssertType(b, "boolean");
AssertType(a, "boolean");
}

function f3(a: true | false, b: true | false) {
    let x = a || b;
AssertType(x, "boolean");
AssertType(a || b, "boolean");
AssertType(a, "boolean");
AssertType(b, "boolean");

    let x = a && b;
AssertType(x, "boolean");
AssertType(a && b, "boolean");
AssertType(a, "boolean");
AssertType(b, "boolean");

    let x = !a;
AssertType(x, "boolean");
AssertType(!a, "boolean");
AssertType(a, "boolean");
}

function f4(t: true, f: false) {
    let x1 = t && f;
AssertType(x1, "boolean");
AssertType(t && f, "boolean");
AssertType(t, "boolean");
AssertType(f, "boolean");

    let x2 = f && t;
AssertType(x2, "boolean");
AssertType(f && t, "boolean");
AssertType(f, "boolean");
AssertType(t, "boolean");

    let x3 = t || f;
AssertType(x3, "boolean");
AssertType(t || f, "boolean");
AssertType(t, "boolean");
AssertType(f, "boolean");

    let x4 = f || t;
AssertType(x4, "boolean");
AssertType(f || t, "boolean");
AssertType(f, "boolean");
AssertType(t, "boolean");

    let x5 = !t;
AssertType(x5, "boolean");
AssertType(!t, "boolean");
AssertType(t, "boolean");

    let x6 = !f;
AssertType(x6, "boolean");
AssertType(!f, "boolean");
AssertType(f, "boolean");
}

declare function g(x: true): string;
declare function g(x: false): boolean;
declare function g(x: boolean): number;

function f5(b: boolean) {
    let z1 = g(true);
AssertType(z1, "string");
AssertType(g(true), "string");
AssertType(g, "{ (true): string; (false): boolean; (boolean): number; }");
AssertType(true, "boolean");

    let z2 = g(false);
AssertType(z2, "boolean");
AssertType(g(false), "boolean");
AssertType(g, "{ (true): string; (false): boolean; (boolean): number; }");
AssertType(false, "boolean");

    let z3 = g(b);
AssertType(z3, "number");
AssertType(g(b), "number");
AssertType(g, "{ (true): string; (false): boolean; (boolean): number; }");
AssertType(b, "boolean");
}

function assertNever(x: never): never {
    throw new Error("Unexpected value");
AssertType(new Error("Unexpected value"), "Error");
AssertType(Error, "ErrorConstructor");
AssertType("Unexpected value", "string");
}

function f10(x: true | false) {
    switch (x) {
AssertType(x, "boolean");

        case true: 
AssertType(true, "boolean");
AssertType("true", "string");
return "true";

        case false: 
AssertType(false, "boolean");
AssertType("false", "string");
return "false";
    }
}

function f11(x: true | false) {
    switch (x) {
AssertType(x, "boolean");

        case true: 
AssertType(true, "boolean");
AssertType("true", "string");
return "true";

        case false: 
AssertType(false, "boolean");
AssertType("false", "string");
return "false";
    }
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
    return assertNever(x);
}

function f12(x: true | false) {
    if (x) {
AssertType(x, "boolean");

        x;
AssertType(x, "boolean");
    }
    else {
        x;
AssertType(x, "boolean");
    }
}

function f13(x: true | false) {
    if (x === true) {
AssertType(x === true, "boolean");
AssertType(x, "boolean");
AssertType(true, "boolean");

        x;
AssertType(x, "boolean");
    }
    else {
        x;
AssertType(x, "boolean");
    }
}

type Item =
    { kind: true, a: string } |
    { kind: false, b: string };

function f20(x: Item) {
    switch (x.kind) {
AssertType(x.kind, "boolean");

        case true: 
AssertType(true, "boolean");
AssertType(x.a, "string");
return x.a;

        case false: 
AssertType(false, "boolean");
AssertType(x.b, "string");
return x.b;
    }
}

function f21(x: Item) {
    switch (x.kind) {
AssertType(x.kind, "boolean");

        case true: 
AssertType(true, "boolean");
AssertType(x.a, "string");
return x.a;

        case false: 
AssertType(false, "boolean");
AssertType(x.b, "string");
return x.b;
    }
AssertType(assertNever(x), "never");
AssertType(assertNever, "(never) => never");
AssertType(x, "never");
    return assertNever(x);
}

