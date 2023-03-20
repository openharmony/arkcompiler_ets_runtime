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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/nullIsSubtypeOfEverythingButUndefined.ts ===
declare function AssertType(value:any, type:string):void;
// null is a subtype of any other types except undefined

let r0 = true ? null : null;
AssertType(r0, "any");
AssertType(true ? null : null, "null");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(null, "null");

let r0 = true ? null : null;
AssertType(r0, "any");
AssertType(true ? null : null, "null");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(null, "null");

let u: typeof undefined;
AssertType(u, "any");
AssertType(undefined, "undefined");

let r0b = true ? u : null;
AssertType(r0b, "any");
AssertType(true ? u : null, "any");
AssertType(true, "boolean");
AssertType(u, "any");
AssertType(null, "null");

let r0b = true ? null : u;
AssertType(r0b, "any");
AssertType(true ? null : u, "any");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(u, "any");

let r1 = true ? 1 : null;
AssertType(r1, "number");
AssertType(true ? 1 : null, "int");
AssertType(true, "boolean");
AssertType(1, "int");
AssertType(null, "null");

let r1 = true ? null : 1;
AssertType(r1, "number");
AssertType(true ? null : 1, "int");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(1, "int");

let r2 = true ? '' : null;
AssertType(r2, "string");
AssertType(true ? '' : null, "string");
AssertType(true, "boolean");
AssertType('', "string");
AssertType(null, "null");

let r2 = true ? null : '';
AssertType(r2, "string");
AssertType(true ? null : '', "string");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType('', "string");

let r3 = true ? true : null;
AssertType(r3, "boolean");
AssertType(true ? true : null, "boolean");
AssertType(true, "boolean");
AssertType(true, "boolean");
AssertType(null, "null");

let r3 = true ? null : true;
AssertType(r3, "boolean");
AssertType(true ? null : true, "boolean");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(true, "boolean");

let r4 = true ? new Date() : null;
AssertType(r4, "Date");
AssertType(true ? new Date() : null, "Date");
AssertType(true, "boolean");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
AssertType(null, "null");

let r4 = true ? null : new Date();
AssertType(r4, "Date");
AssertType(true ? null : new Date(), "Date");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");

let r5 = true ? /1/ : null;
AssertType(r5, "RegExp");
AssertType(true ? /1/ : null, "RegExp");
AssertType(true, "boolean");
AssertType(/1/, "RegExp");
AssertType(null, "null");

let r5 = true ? null : /1/;
AssertType(r5, "RegExp");
AssertType(true ? null : /1/, "RegExp");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(/1/, "RegExp");

let r6 = true ? { foo: 1 } : null;
AssertType(r6, "{ foo: number; }");
AssertType(true ? { foo: 1 } : null, "{ foo: number; }");
AssertType(true, "boolean");
AssertType({ foo: 1 }, "{ foo: number; }");
AssertType(foo, "number");
AssertType(1, "int");
AssertType(null, "null");

let r6 = true ? null : { foo: 1 };
AssertType(r6, "{ foo: number; }");
AssertType(true ? null : { foo: 1 }, "{ foo: number; }");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType({ foo: 1 }, "{ foo: number; }");
AssertType(foo, "number");
AssertType(1, "int");

let r7 = true ? () => { } : null;
AssertType(r7, "() => void");
AssertType(true ? () => { } : null, "() => void");
AssertType(true, "boolean");
AssertType(() => { }, "() => void");
AssertType(null, "null");

let r7 = true ? null : () => { };
AssertType(r7, "() => void");
AssertType(true ? null : () => { }, "() => void");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(() => { }, "() => void");

let r8 = true ? <T>(x: T) => { 
AssertType(r8, "<T>(T) => T");
AssertType(true ? <T>(x: T) => { return x } : null, "<T>(T) => T");
AssertType(true, "boolean");
AssertType(<T>(x: T) => { return x }, "<T>(T) => T");
AssertType(x, "T");
AssertType(x, "T");
AssertType(null, "null");
return x } : null;

let r8b = true ? null : <T>(x: T) => { 
AssertType(r8b, "<T>(T) => T");
AssertType(true ? null : <T>(x: T) => { return x }, "<T>(T) => T");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(<T>(x: T) => { return x }, "<T>(T) => T");
AssertType(x, "T");
AssertType(x, "T");
return x }; // type parameters not identical across declarations

interface I1 { foo: number; }
let i1: I1;
AssertType(i1, "I1");

let r9 = true ? i1 : null;
AssertType(r9, "I1");
AssertType(true ? i1 : null, "I1");
AssertType(true, "boolean");
AssertType(i1, "I1");
AssertType(null, "null");

let r9 = true ? null : i1;
AssertType(r9, "I1");
AssertType(true ? null : i1, "I1");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(i1, "I1");

class C1 { foo: number; }
let c1: C1;
AssertType(c1, "C1");

let r10 = true ? c1 : null;
AssertType(r10, "C1");
AssertType(true ? c1 : null, "C1");
AssertType(true, "boolean");
AssertType(c1, "C1");
AssertType(null, "null");

let r10 = true ? null : c1;
AssertType(r10, "C1");
AssertType(true ? null : c1, "C1");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(c1, "C1");

class C2<T> { foo: T; }
let c2: C2<number>;
AssertType(c2, "C2<number>");

let r12 = true ? c2 : null;
AssertType(r12, "C2<number>");
AssertType(true ? c2 : null, "C2<number>");
AssertType(true, "boolean");
AssertType(c2, "C2<number>");
AssertType(null, "null");

let r12 = true ? null : c2;
AssertType(r12, "C2<number>");
AssertType(true ? null : c2, "C2<number>");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(c2, "C2<number>");

enum E { A }
let r13 = true ? E : null;
AssertType(r13, "typeof E");
AssertType(true ? E : null, "typeof E");
AssertType(true, "boolean");
AssertType(E, "typeof E");
AssertType(null, "null");

let r13 = true ? null : E;
AssertType(r13, "typeof E");
AssertType(true ? null : E, "typeof E");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(E, "typeof E");

let r14 = true ? E.A : null;
AssertType(r14, "E");
AssertType(true ? E.A : null, "E");
AssertType(true, "boolean");
AssertType(E.A, "E");
AssertType(null, "null");

let r14 = true ? null : E.A;
AssertType(r14, "E");
AssertType(true ? null : E.A, "E");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(E.A, "E");

function f() { }
module f {
    export let bar = 1;
}
let af: typeof f;
AssertType(af, "typeof f");
AssertType(f, "typeof f");

let r15 = true ? af : null;
AssertType(r15, "typeof f");
AssertType(true ? af : null, "typeof f");
AssertType(true, "boolean");
AssertType(af, "typeof f");
AssertType(null, "null");

let r15 = true ? null : af;
AssertType(r15, "typeof f");
AssertType(true ? null : af, "typeof f");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(af, "typeof f");

class c { baz: string }
module c {
    export let bar = 1;
}
let ac: typeof c;
AssertType(ac, "typeof c");
AssertType(c, "typeof c");

let r16 = true ? ac : null;
AssertType(r16, "typeof c");
AssertType(true ? ac : null, "typeof c");
AssertType(true, "boolean");
AssertType(ac, "typeof c");
AssertType(null, "null");

let r16 = true ? null : ac;
AssertType(r16, "typeof c");
AssertType(true ? null : ac, "typeof c");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(ac, "typeof c");

function f17<T>(x: T) {
    let r17 = true ? x : null;
AssertType(r17, "T");
AssertType(true ? x : null, "T");
AssertType(true, "boolean");
AssertType(x, "T");
AssertType(null, "null");

    let r17 = true ? null : x;
AssertType(r17, "T");
AssertType(true ? null : x, "T");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(x, "T");
}

function f18<T, U>(x: U) {
    let r18 = true ? x : null;
AssertType(r18, "U");
AssertType(true ? x : null, "U");
AssertType(true, "boolean");
AssertType(x, "U");
AssertType(null, "null");

    let r18 = true ? null : x;
AssertType(r18, "U");
AssertType(true ? null : x, "U");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(x, "U");
}

let r19 = true ? new Object() : null;
AssertType(r19, "Object");
AssertType(true ? new Object() : null, "Object");
AssertType(true, "boolean");
AssertType(new Object(), "Object");
AssertType(Object, "ObjectConstructor");
AssertType(null, "null");

let r19 = true ? null : new Object();
AssertType(r19, "Object");
AssertType(true ? null : new Object(), "Object");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType(new Object(), "Object");
AssertType(Object, "ObjectConstructor");

let r20 = true ? {} : null;
AssertType(r20, "{}");
AssertType(true ? {} : null, "{}");
AssertType(true, "boolean");
AssertType({}, "{}");
AssertType(null, "null");

let r20 = true ? null : {};
AssertType(r20, "{}");
AssertType(true ? null : {}, "{}");
AssertType(true, "boolean");
AssertType(null, "null");
AssertType({}, "{}");


