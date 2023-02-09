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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypesOfTypeParameterWithConstraints2.ts ===
declare function AssertType(value:any, type:string):void;
// checking whether other types are subtypes of type parameters with constraints

function f1<T extends U, U>(x: T, y: U) {
    let r = true ? x : y;
AssertType(r, "U");
AssertType(true ? x : y, "U");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(y, "U");

    let r = true ? y : x;
AssertType(r, "U");
AssertType(true ? y : x, "U");
AssertType(true, "boolean");
AssertType(y, "U");
AssertType(x, "T");
}

// V > U > T
function f2<T extends U, U extends V, V>(x: T, y: U, z: V) {
    let r = true ? x : y;
AssertType(r, "U");
AssertType(true ? x : y, "U");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(y, "U");

    let r = true ? y : x;
AssertType(r, "U");
AssertType(true ? y : x, "U");
AssertType(true, "boolean");
AssertType(y, "U");
AssertType(x, "T");

    // ok
    let r2 = true ? z : y;
AssertType(r2, "V");
AssertType(true ? z : y, "V");
AssertType(true, "boolean");
AssertType(z, "V");
AssertType(y, "U");

    let r2 = true ? y : z;
AssertType(r2, "V");
AssertType(true ? y : z, "V");
AssertType(true, "boolean");
AssertType(y, "U");
AssertType(z, "V");

    // ok
    let r2a = true ? z : x;
AssertType(r2a, "V");
AssertType(true ? z : x, "V");
AssertType(true, "boolean");
AssertType(z, "V");
AssertType(x, "T");

    let r2b = true ? x : z;
AssertType(r2b, "V");
AssertType(true ? x : z, "V");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(z, "V");
}

// Date > U > T
function f3<T extends U, U extends Date>(x: T, y: U) {
    let r = true ? x : y;
AssertType(r, "U");
AssertType(true ? x : y, "U");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(y, "U");

    let r = true ? y : x;
AssertType(r, "U");
AssertType(true ? y : x, "U");
AssertType(true, "boolean");
AssertType(y, "U");
AssertType(x, "T");

    // ok
    let r2 = true ? x : new Date(); 
AssertType(r2, "Date");
AssertType(true ? x : new Date(), "Date");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");

    let r2 = true ? new Date() : x;
AssertType(r2, "Date");
AssertType(true ? new Date() : x, "Date");
AssertType(true, "boolean");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
AssertType(x, "T");

    // ok
    let r3 = true ? y : new Date(); 
AssertType(r3, "Date");
AssertType(true ? y : new Date(), "Date");
AssertType(true, "boolean");
AssertType(y, "U");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");

    let r3 = true ? new Date() : y;
AssertType(r3, "Date");
AssertType(true ? new Date() : y, "Date");
AssertType(true, "boolean");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
AssertType(y, "U");
}


interface I1 { foo: number; }
class C1 { foo: number; }
class C2<T> { foo: T; }
enum E { A }
function f() { }
module f {
    export let bar = 1;
}
class c { baz: string }
module c {
    export let bar = 1;
}

function f4<T extends Number>(x: T) {
    let r0 = true ? x : null; // ok
AssertType(r0, "T");
AssertType(true ? x : null, "T");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(null, "null");

    let r0 = true ? null : x; // ok
AssertType(r0, "T");
AssertType(true ? null : x, "T");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(x, "T");

    let u: typeof undefined;
AssertType(u, "any");
AssertType(undefined, "undefined");

    let r0b = true ? u : x; // ok
AssertType(r0b, "any");
AssertType(true ? u : x, "any");
AssertType(true, "boolean");
AssertType(u, "any");
AssertType(x, "T");

    let r0b = true ? x : u; // ok
AssertType(r0b, "any");
AssertType(true ? x : u, "any");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(u, "any");
}

function f5<T extends Number>(x: T) {
    let r1 = true ? 1 : x; // ok
AssertType(r1, "union");
AssertType(true ? 1 : x, "union");
AssertType(true, "boolean");
AssertType(1, "int");
AssertType(x, "T");

    let r1 = true ? x : 1; // ok
AssertType(r1, "union");
AssertType(true ? x : 1, "union");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(1, "int");
}

function f6<T extends String>(x: T) {
    let r2 = true ? '' : x; // ok
AssertType(r2, "union");
AssertType(true ? '' : x, "union");
AssertType(true, "boolean");
AssertType('', "string");
AssertType(x, "T");

    let r2 = true ? x : ''; // ok
AssertType(r2, "union");
AssertType(true ? x : '', "union");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType('', "string");
}

function f7<T extends Boolean>(x: T) {
    let r3 = true ? true : x; // ok
AssertType(r3, "union");
AssertType(true ? true : x, "union");
AssertType(true, "boolean");
AssertType(true, "boolean");
AssertType(x, "T");

    let r3 = true ? x : true; // ok
AssertType(r3, "union");
AssertType(true ? x : true, "union");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(true, "boolean");
}

function f8<T extends Date>(x: T) {
    let r4 = true ? new Date() : x; // ok
AssertType(r4, "Date");
AssertType(true ? new Date() : x, "Date");
AssertType(true, "boolean");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
AssertType(x, "T");

    let r4 = true ? x : new Date(); // ok
AssertType(r4, "Date");
AssertType(true ? x : new Date(), "Date");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
}

function f9<T extends RegExp>(x: T) {
    let r5 = true ? /1/ : x; // ok
AssertType(r5, "RegExp");
AssertType(true ? /1/ : x, "RegExp");
AssertType(true, "boolean");
AssertType(/1/, "RegExp");
AssertType(x, "T");

    let r5 = true ? x : /1/; // ok
AssertType(r5, "RegExp");
AssertType(true ? x : /1/, "RegExp");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(/1/, "RegExp");
}

function f10<T extends { foo: number }>(x: T) {
    let r6 = true ? { foo: 1 } : x; // ok
AssertType(r6, "{ foo: number; }");
AssertType(true ? { foo: 1 } : x, "{ foo: number; }");
AssertType(true, "boolean");
AssertType({ foo: 1 }, "{ foo: number; }");
AssertType(foo, "number");
AssertType(1, "int");
AssertType(x, "T");

    let r6 = true ? x : { foo: 1 }; // ok
AssertType(r6, "{ foo: number; }");
AssertType(true ? x : { foo: 1 }, "{ foo: number; }");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType({ foo: 1 }, "{ foo: number; }");
AssertType(foo, "number");
AssertType(1, "int");
}

function f11<T extends () => void>(x: T) {
    let r7 = true ? () => { } : x; // ok
AssertType(r7, "() => void");
AssertType(true ? () => { } : x, "() => void");
AssertType(true, "boolean");
AssertType(() => { }, "() => void");
AssertType(x, "T");

    let r7 = true ? x : () => { }; // ok
AssertType(r7, "() => void");
AssertType(true ? x : () => { }, "() => void");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(() => { }, "() => void");
}

function f12<T extends <U>(x: U) => U>(x: T) {
    let r8 = true ? <T>(x: T) => { 
AssertType(r8, "<T>(T) => T");
AssertType(true ? <T>(x: T) => { return x } : x, "<T>(T) => T");
AssertType(true, "boolean");
AssertType(<T>(x: T) => { return x }, "<T>(T) => T");
AssertType(x, "T");
AssertType(x, "T");
AssertType(x, "T");
return x } : x; // ok

    let r8b = true ? x : <T>(x: T) => { 
AssertType(r8b, "<T>(T) => T");
AssertType(true ? x : <T>(x: T) => { return x }, "<T>(T) => T");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(<T>(x: T) => { return x }, "<T>(T) => T");
AssertType(x, "T");
AssertType(x, "T");
return x }; // ok, type parameters not identical across declarations
}

function f13<T extends I1>(x: T) {
    let i1: I1;
AssertType(i1, "I1");

    let r9 = true ? i1 : x; // ok
AssertType(r9, "I1");
AssertType(true ? i1 : x, "I1");
AssertType(true, "boolean");
AssertType(i1, "I1");
AssertType(x, "T");

    let r9 = true ? x : i1; // ok
AssertType(r9, "I1");
AssertType(true ? x : i1, "I1");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(i1, "I1");
}

function f14<T extends C1>(x: T) {
    let c1: C1;
AssertType(c1, "C1");

    let r10 = true ? c1 : x; // ok
AssertType(r10, "C1");
AssertType(true ? c1 : x, "C1");
AssertType(true, "boolean");
AssertType(c1, "C1");
AssertType(x, "T");

    let r10 = true ? x : c1; // ok
AssertType(r10, "C1");
AssertType(true ? x : c1, "C1");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(c1, "C1");
}

function f15<T extends C2<number>>(x: T) {
    let c2: C2<number>;
AssertType(c2, "C2<number>");

    let r12 = true ? c2 : x; // ok
AssertType(r12, "C2<number>");
AssertType(true ? c2 : x, "C2<number>");
AssertType(true, "boolean");
AssertType(c2, "C2<number>");
AssertType(x, "T");

    let r12 = true ? x : c2; // ok
AssertType(r12, "C2<number>");
AssertType(true ? x : c2, "C2<number>");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(c2, "C2<number>");
}

function f16<T extends E>(x: T) {
    let r13 = true ? E : x; // ok
AssertType(r13, "union");
AssertType(true ? E : x, "union");
AssertType(true, "boolean");
AssertType(E, "typeof E");
AssertType(x, "T");

    let r13 = true ? x : E; // ok
AssertType(r13, "union");
AssertType(true ? x : E, "union");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(E, "typeof E");

    let r14 = true ? E.A : x; // ok
AssertType(r14, "E");
AssertType(true ? E.A : x, "E");
AssertType(true, "boolean");
AssertType(E.A, "E");
AssertType(x, "T");

    let r14 = true ? x : E.A; // ok
AssertType(r14, "E");
AssertType(true ? x : E.A, "E");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(E.A, "E");
}

function f17<T extends typeof f>(x: T) {
    let af: typeof f;
AssertType(af, "typeof f");
AssertType(f, "typeof f");

    let r15 = true ? af : x; // ok
AssertType(r15, "typeof f");
AssertType(true ? af : x, "typeof f");
AssertType(true, "boolean");
AssertType(af, "typeof f");
AssertType(x, "T");

    let r15 = true ? x : af; // ok
AssertType(r15, "typeof f");
AssertType(true ? x : af, "typeof f");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(af, "typeof f");
}

function f18<T extends typeof c>(x: T) {
    let ac: typeof c;
AssertType(ac, "typeof c");
AssertType(c, "typeof c");

    let r16 = true ? ac : x; // ok
AssertType(r16, "typeof c");
AssertType(true ? ac : x, "typeof c");
AssertType(true, "boolean");
AssertType(ac, "typeof c");
AssertType(x, "T");

    let r16 = true ? x : ac; // ok
AssertType(r16, "typeof c");
AssertType(true ? x : ac, "typeof c");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(ac, "typeof c");
}

function f19<T>(x: T) {
    function f17<U extends T>(a: U) {
AssertType(f17, "<U extends T>(U) => void");
AssertType(a, "U");

        let r17 = true ? x : a; // ok
AssertType(r17, "T");
AssertType(true ? x : a, "T");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(a, "U");

        let r17 = true ? a : x; // ok
AssertType(r17, "T");
AssertType(true ? a : x, "T");
AssertType(true, "boolean");
AssertType(a, "U");
AssertType(x, "T");
    }

    function f18<V extends U, U extends T>(a: V) {
AssertType(f18, "<V extends U, U extends T>(V) => void");
AssertType(a, "V");

        let r18 = true ? x : a; // ok
AssertType(r18, "T");
AssertType(true ? x : a, "T");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(a, "V");

        let r18 = true ? a : x; // ok
AssertType(r18, "T");
AssertType(true ? a : x, "T");
AssertType(true, "boolean");
AssertType(a, "V");
AssertType(x, "T");
    }
}

function f20<T extends Number>(x: T) {
    let r19 = true ? new Object() : x; // ok
AssertType(r19, "union");
AssertType(true ? new Object() : x, "union");
AssertType(true, "boolean");
AssertType(new Object(), "Object");
AssertType(Object, "ObjectConstructor");
AssertType(x, "T");

    let r19 = true ? x : new Object(); // ok
AssertType(r19, "union");
AssertType(true ? x : new Object(), "union");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(new Object(), "Object");
AssertType(Object, "ObjectConstructor");
}

function f21<T extends Number>(x: T) {
    let r20 = true ? {} : x; // ok
AssertType(r20, "{}");
AssertType(true ? {} : x, "union");
AssertType(true, "boolean");
AssertType({}, "{}");
AssertType(x, "T");

    let r20 = true ? x : {}; // ok
AssertType(r20, "{}");
AssertType(true ? x : {}, "union");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType({}, "{}");
}

