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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithNonSymmetricSubtypes.ts ===
declare function AssertType(value:any, type:string):void;
// generic type argument inference where inference leads to two candidates that are both supertypes of all candidates
// we choose the first candidate so the result is dependent on the order of the arguments provided

function foo<T>(x: T, y: T) {
    let r: T;
AssertType(r, "T");

AssertType(r, "T");
    return r;
}

let a: { x: number; y?: number; };
AssertType(a, "{ x: number; y?: number; }");
AssertType(x, "number");
AssertType(y, "number");

let b: { x: number; z?: number; };
AssertType(b, "{ x: number; z?: number; }");
AssertType(x, "number");
AssertType(z, "number");

let r = foo(a, b); // { x: number; y?: number; };
AssertType(r, "{ x: number; y?: number; }");
AssertType(foo(a, b), "{ x: number; y?: number; }");
AssertType(foo, "<T>(T, T) => T");
AssertType(a, "{ x: number; y?: number; }");
AssertType(b, "{ x: number; z?: number; }");

let r2 = foo(b, a); // { x: number; z?: number; };
AssertType(r2, "{ x: number; z?: number; }");
AssertType(foo(b, a), "{ x: number; z?: number; }");
AssertType(foo, "<T>(T, T) => T");
AssertType(b, "{ x: number; z?: number; }");
AssertType(a, "{ x: number; y?: number; }");

let x: { x: number; };
AssertType(x, "{ x: number; }");
AssertType(x, "number");

let y: { x?: number; };
AssertType(y, "{ x?: number; }");
AssertType(x, "number");

let r3 = foo(a, x); // { x: number; y?: number; };
AssertType(r3, "{ x: number; }");
AssertType(foo(a, x), "{ x: number; }");
AssertType(foo, "<T>(T, T) => T");
AssertType(a, "{ x: number; y?: number; }");
AssertType(x, "{ x: number; }");

let r4 = foo(x, a); // { x: number; };
AssertType(r4, "{ x: number; }");
AssertType(foo(x, a), "{ x: number; }");
AssertType(foo, "<T>(T, T) => T");
AssertType(x, "{ x: number; }");
AssertType(a, "{ x: number; y?: number; }");

let r5 = foo(a, y); // { x?: number; };
AssertType(r5, "{ x?: number; }");
AssertType(foo(a, y), "{ x?: number; }");
AssertType(foo, "<T>(T, T) => T");
AssertType(a, "{ x: number; y?: number; }");
AssertType(y, "{ x?: number; }");

let r5 = foo(y, a); // { x?: number; };
AssertType(r5, "{ x?: number; }");
AssertType(foo(y, a), "{ x?: number; }");
AssertType(foo, "<T>(T, T) => T");
AssertType(y, "{ x?: number; }");
AssertType(a, "{ x: number; y?: number; }");

let r6 = foo(x, y); // { x?: number; };
AssertType(r6, "{ x?: number; }");
AssertType(foo(x, y), "{ x?: number; }");
AssertType(foo, "<T>(T, T) => T");
AssertType(x, "{ x: number; }");
AssertType(y, "{ x?: number; }");

let r6 = foo(y, x); // { x?: number; };
AssertType(r6, "{ x?: number; }");
AssertType(foo(y, x), "{ x?: number; }");
AssertType(foo, "<T>(T, T) => T");
AssertType(y, "{ x?: number; }");
AssertType(x, "{ x: number; }");

let s1: (x: Object) => string;
AssertType(s1, "(Object) => string");
AssertType(x, "Object");

let s2: (x: string) => string;
AssertType(s2, "(string) => string");
AssertType(x, "string");

let r7 = foo(s1, s2); // (x: Object) => string;
AssertType(r7, "(string) => string");
AssertType(foo(s1, s2), "(string) => string");
AssertType(foo, "<T>(T, T) => T");
AssertType(s1, "(Object) => string");
AssertType(s2, "(string) => string");

let r8 = foo(s2, s1); // (x: string) => string;
AssertType(r8, "(Object) => string");
AssertType(foo(s2, s1), "(Object) => string");
AssertType(foo, "<T>(T, T) => T");
AssertType(s2, "(string) => string");
AssertType(s1, "(Object) => string");


