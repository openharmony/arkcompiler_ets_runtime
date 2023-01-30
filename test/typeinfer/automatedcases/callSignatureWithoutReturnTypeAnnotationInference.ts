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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/callSignatureWithoutReturnTypeAnnotationInference.ts ===
declare function AssertType(value:any, type:string):void;
// Call signatures without a return type should infer one from the function body (if present)

// Simple types
function foo(x) {
AssertType(1, "int");
    return 1;
}
let r = foo(1);
AssertType(r, "number");
AssertType(foo(1), "number");
AssertType(foo, "(any) => number");
AssertType(1, "int");

function foo2(x) {
AssertType(foo(x), "number");
AssertType(foo, "(any) => number");
AssertType(x, "any");
    return foo(x);
}
let r2 = foo2(1);
AssertType(r2, "number");
AssertType(foo2(1), "number");
AssertType(foo2, "(any) => number");
AssertType(1, "int");

function foo3() {
AssertType(foo3(), "any");
AssertType(foo3, "() => any");
    return foo3();
}
let r3 = foo3();
AssertType(r3, "any");
AssertType(foo3(), "any");
AssertType(foo3, "() => any");

function foo4<T>(x: T) {
AssertType(x, "T");
    return x;
}
let r4 = foo4(1);
AssertType(r4, "number");
AssertType(foo4(1), "int");
AssertType(foo4, "<T>(T) => T");
AssertType(1, "int");

function foo5(x) {
    if (true) {
AssertType(true, "boolean");

AssertType(1, "int");
        return 1;

    } else {
AssertType(2, "int");
        return 2;
    }
}
let r5 = foo5(1);
AssertType(r5, "number");
AssertType(foo5(1), "union");
AssertType(foo5, "(any) => union");
AssertType(1, "int");

function foo6(x) {
    try {
    }
    catch (e) {
AssertType(e, "any");

AssertType([], "undefined[]");
        return [];
    }
    finally {
AssertType([], "undefined[]");
        return [];
    }
}
let r6 = foo6(1);
AssertType(r6, "any[]");
AssertType(foo6(1), "any[]");
AssertType(foo6, "(any) => any[]");
AssertType(1, "int");

function foo7(x) {
AssertType(typeof x, "union");
AssertType(x, "any");
    return typeof x;
}
let r7 = foo7(1);
AssertType(r7, "union");
AssertType(foo7(1), "union");
AssertType(foo7, "(any) => union");
AssertType(1, "int");

// object types
function foo8(x: number) {
AssertType({ x: x }, "{ x: number; }");
AssertType(x, "number");
AssertType(x, "number");
    return { x: x };
}
let r8 = foo8(1);
AssertType(r8, "{ x: number; }");
AssertType(foo8(1), "{ x: number; }");
AssertType(foo8, "(number) => { x: number; }");
AssertType(1, "int");

interface I {
    foo: string;
}
function foo9(x: number) {
    let i: I;
AssertType(i, "I");

AssertType(i, "I");
    return i;    
}
let r9 = foo9(1);
AssertType(r9, "I");
AssertType(foo9(1), "I");
AssertType(foo9, "(number) => I");
AssertType(1, "int");

class C {
    foo: string;
}
function foo10(x: number) {
    let c: C;
AssertType(c, "C");

AssertType(c, "C");
    return c;
}
let r10 = foo10(1);
AssertType(r10, "C");
AssertType(foo10(1), "C");
AssertType(foo10, "(number) => C");
AssertType(1, "int");

module M {
    export let x = 1;
    export class C { foo: string }
}
function foo11() {
AssertType(M, "typeof M");
    return M;
}
let r11 = foo11();
AssertType(r11, "typeof M");
AssertType(foo11(), "typeof M");
AssertType(foo11, "() => typeof M");

// merged declarations
interface I2 {
    x: number;
}
interface I2 {
    y: number;
}
function foo12() {
    let i2: I2;
AssertType(i2, "I2");

AssertType(i2, "I2");
    return i2;
}
let r12 = foo12();
AssertType(r12, "I2");
AssertType(foo12(), "I2");
AssertType(foo12, "() => I2");

function m1() { 
AssertType(1, "int");
return 1; 
}

module m1 { export let y = 2; }
function foo13() {
AssertType(m1, "typeof m1");
    return m1;
}
let r13 = foo13();
AssertType(r13, "typeof m1");
AssertType(foo13(), "typeof m1");
AssertType(foo13, "() => typeof m1");

class c1 {
    foo: string;
    constructor(x) { }
}
module c1 {
    export let x = 1;
}
function foo14() {
AssertType(c1, "typeof c1");
    return c1;
}
let r14 = foo14();
AssertType(r14, "typeof c1");
AssertType(foo14(), "typeof c1");
AssertType(foo14, "() => typeof c1");

enum e1 { A }
module e1 { export let y = 1; }
function foo15() {
AssertType(e1, "typeof e1");
    return e1;
}
let r15 = foo15();
AssertType(r15, "typeof e1");
AssertType(foo15(), "typeof e1");
AssertType(foo15, "() => typeof e1");


