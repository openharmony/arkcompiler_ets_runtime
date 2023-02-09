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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/functionConstraintSatisfaction3.ts ===
declare function AssertType(value:any, type:string):void;
// satisfaction of a constraint to Function, no errors expected

function foo<T extends (x: string) => string>(x: T): T { 
AssertType(x, "T");
return x; 
}

interface I {
    (): string;
}
let i: I;
AssertType(i, "I");

class C {
    foo: string;
}

let a: { (): string };
AssertType(a, "() => string");

let b: { new (): string };
AssertType(b, "new () => string");

let c: { (): string; (x): string };
AssertType(c, "{ (): string; (any): string; }");
AssertType(x, "any");

let r1 = foo((x) => x);
AssertType(r1, "(string) => string");
AssertType(foo((x) => x), "(string) => string");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType((x) => x, "(string) => string");
AssertType(x, "string");
AssertType(x, "string");

let r2 = foo((x: string) => x);
AssertType(r2, "(string) => string");
AssertType(foo((x: string) => x), "(string) => string");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType((x: string) => x, "(string) => string");
AssertType(x, "string");
AssertType(x, "string");

let r3 = foo(function (x) { 
AssertType(r3, "(string) => string");
AssertType(foo(function (x) { return x }), "(string) => string");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType(function (x) { return x }, "(string) => string");
AssertType(x, "string");
AssertType(x, "string");
return x });

let r4 = foo(function (x: string) { 
AssertType(r4, "(string) => string");
AssertType(foo(function (x: string) { return x }), "(string) => string");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType(function (x: string) { return x }, "(string) => string");
AssertType(x, "string");
AssertType(x, "string");
return x });

let r5 = foo(i);
AssertType(r5, "I");
AssertType(foo(i), "I");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType(i, "I");

let r8 = foo(c);
AssertType(r8, "{ (): string; (any): string; }");
AssertType(foo(c), "{ (): string; (any): string; }");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType(c, "{ (): string; (any): string; }");

interface I2<T> {
    (x: T): T;
}
let i2: I2<string>;
AssertType(i2, "I2<string>");

class C2<T> {
    foo: T;
}

let a2: { <T>(x: T): T };
AssertType(a2, "<T>(T) => T");
AssertType(x, "T");

let b2: { new <T>(x: T): T };
AssertType(b2, "new <T>(T) => T");
AssertType(x, "T");

let c2: { <T>(x: T): T; <T>(x: T, y: T): T };
AssertType(c2, "{ <T>(T): T; <T>(T, T): T; }");
AssertType(x, "T");
AssertType(x, "T");
AssertType(y, "T");

let r9 = foo(function <U>(x: U) { 
AssertType(r9, "<U>(U) => U");
AssertType(foo(function <U>(x: U) { return x; }), "<U>(U) => U");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType(function <U>(x: U) { return x; }, "<U>(U) => U");
AssertType(x, "U");
AssertType(x, "U");
return x; });

let r10 = foo(<U extends string>(x: U) => x);
AssertType(r10, "<U extends string>(U) => U");
AssertType(foo(<U extends string>(x: U) => x), "<U extends string>(U) => U");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType(<U extends string>(x: U) => x, "<U extends string>(U) => U");
AssertType(x, "U");
AssertType(x, "U");

let r12 = foo(i2);
AssertType(r12, "I2<string>");
AssertType(foo(i2), "I2<string>");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType(i2, "I2<string>");

let r15 = foo(c2);
AssertType(r15, "{ <T>(T): T; <T>(T, T): T; }");
AssertType(foo(c2), "{ <T>(T): T; <T>(T, T): T; }");
AssertType(foo, "<T extends (string) => string>(T) => T");
AssertType(c2, "{ <T>(T): T; <T>(T, T): T; }");


