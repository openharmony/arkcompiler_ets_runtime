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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/functionConstraintSatisfaction.ts ===
declare function AssertType(value:any, type:string):void;
// satisfaction of a constraint to Function, no errors expected

function foo<T extends Function>(x: T): T { 
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

let r = foo(new Function());
AssertType(r, "Function");
AssertType(foo(new Function()), "Function");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(new Function(), "Function");
AssertType(Function, "FunctionConstructor");

let r1 = foo((x) => x);
AssertType(r1, "(any) => any");
AssertType(foo((x) => x), "(any) => any");
AssertType(foo, "<T extends Function>(T) => T");
AssertType((x) => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");

let r2 = foo((x: string[]) => x);
AssertType(r2, "(string[]) => string[]");
AssertType(foo((x: string[]) => x), "(string[]) => string[]");
AssertType(foo, "<T extends Function>(T) => T");
AssertType((x: string[]) => x, "(string[]) => string[]");
AssertType(x, "string[]");
AssertType(x, "string[]");

let r3 = foo(function (x) { 
AssertType(r3, "(any) => any");
AssertType(foo(function (x) { return x }), "(any) => any");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(function (x) { return x }, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");
return x });

let r4 = foo(function (x: string[]) { 
AssertType(r4, "(string[]) => string[]");
AssertType(foo(function (x: string[]) { return x }), "(string[]) => string[]");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(function (x: string[]) { return x }, "(string[]) => string[]");
AssertType(x, "string[]");
AssertType(x, "string[]");
return x });

let r5 = foo(i);
AssertType(r5, "I");
AssertType(foo(i), "I");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(i, "I");

let r6 = foo(C);
AssertType(r6, "typeof C");
AssertType(foo(C), "typeof C");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(C, "typeof C");

let r7 = foo(b);
AssertType(r7, "new () => string");
AssertType(foo(b), "new () => string");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(b, "new () => string");

let r8 = foo(c);
AssertType(r8, "{ (): string; (any): string; }");
AssertType(foo(c), "{ (): string; (any): string; }");
AssertType(foo, "<T extends Function>(T) => T");
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

let r9 = foo(<U>(x: U) => x);
AssertType(r9, "<U>(U) => U");
AssertType(foo(<U>(x: U) => x), "<U>(U) => U");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(<U>(x: U) => x, "<U>(U) => U");
AssertType(x, "U");
AssertType(x, "U");

let r10 = foo(function <U>(x: U) { 
AssertType(r10, "<U>(U) => U");
AssertType(foo(function <U>(x: U) { return x; }), "<U>(U) => U");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(function <U>(x: U) { return x; }, "<U>(U) => U");
AssertType(x, "U");
AssertType(x, "U");
return x; });

let r11 = foo(<U extends Date>(x: U) => x);
AssertType(r11, "<U extends Date>(U) => U");
AssertType(foo(<U extends Date>(x: U) => x), "<U extends Date>(U) => U");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(<U extends Date>(x: U) => x, "<U extends Date>(U) => U");
AssertType(x, "U");
AssertType(x, "U");

let r12 = foo(<U, V>(x: U, y: V) => x);
AssertType(r12, "<U, V>(U, V) => U");
AssertType(foo(<U, V>(x: U, y: V) => x), "<U, V>(U, V) => U");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(<U, V>(x: U, y: V) => x, "<U, V>(U, V) => U");
AssertType(x, "U");
AssertType(y, "V");
AssertType(x, "U");

let r13 = foo(i2);
AssertType(r13, "I2<string>");
AssertType(foo(i2), "I2<string>");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(i2, "I2<string>");

let r14 = foo(C2);
AssertType(r14, "typeof C2");
AssertType(foo(C2), "typeof C2");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(C2, "typeof C2");

let r15 = foo(b2);
AssertType(r15, "new <T>(T) => T");
AssertType(foo(b2), "new <T>(T) => T");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(b2, "new <T>(T) => T");

let r16 = foo(c2);
AssertType(r16, "{ <T>(T): T; <T>(T, T): T; }");
AssertType(foo(c2), "{ <T>(T): T; <T>(T, T): T; }");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(c2, "{ <T>(T): T; <T>(T, T): T; }");

interface F2 extends Function { foo: string; }
let f2: F2;
AssertType(f2, "F2");

let r17 = foo(f2);
AssertType(r17, "F2");
AssertType(foo(f2), "F2");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(f2, "F2");

function foo2<T extends { (): void }, U extends { (): void }>(x: T, y: U) {
    foo(x);
AssertType(foo(x), "T");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(x, "T");

    foo(y);
AssertType(foo(y), "U");
AssertType(foo, "<T extends Function>(T) => T");
AssertType(y, "U");
}

