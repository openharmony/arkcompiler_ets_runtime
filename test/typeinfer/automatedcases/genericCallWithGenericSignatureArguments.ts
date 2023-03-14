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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithGenericSignatureArguments.ts ===
declare function AssertType(value:any, type:string):void;
// When a function expression is inferentially typed (section 4.9.3) and a type assigned to a parameter in that expression references type parameters for which inferences are being made, 
// the corresponding inferred type arguments to become fixed and no further candidate inferences are made for them.

function foo<T>(a: (x: T) => T, b: (x: T) => T) {
    let r: (x: T) => T;
AssertType(r, "(T) => T");
AssertType(x, "T");

AssertType(r, "(T) => T");
    return r;
}

let r1b = foo((x) => 1, (x) => ''); // {} => {
AssertType(r1b, "(unknown) => unknown");

AssertType(foo((x) => 1, (x) => ''), "(unknown) => unknown");

AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");

AssertType((x) => 1, "(unknown) => number");

AssertType(x, "unknown");

AssertType(1, "int");

AssertType((x) => '', "(unknown) => string");

AssertType(x, "unknown");

AssertType('', "string");
}

let r2 = foo((x: Object) => null, (x: string) => ''); // Object => Object
AssertType(r2, "(any) => any");
AssertType(foo((x: Object) => null, (x: string) => ''), "(any) => any");
AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");
AssertType((x: Object) => null, "(Object) => any");
AssertType(x, "Object");
AssertType(null, "null");
AssertType((x: string) => '', "(string) => string");
AssertType(x, "string");
AssertType('', "string");

let r3 = foo((x: number) => 1, (x: Object) => null); // number => number
AssertType(r3, "(any) => any");
AssertType(foo((x: number) => 1, (x: Object) => null), "(any) => any");
AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");
AssertType((x: number) => 1, "(number) => number");
AssertType(x, "number");
AssertType(1, "int");
AssertType((x: Object) => null, "(Object) => any");
AssertType(x, "Object");
AssertType(null, "null");

let r3ii = foo((x: number) => 1, (x: number) => 1); // number => number
AssertType(r3ii, "(number) => number");
AssertType(foo((x: number) => 1, (x: number) => 1), "(number) => number");
AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");
AssertType((x: number) => 1, "(number) => number");
AssertType(x, "number");
AssertType(1, "int");
AssertType((x: number) => 1, "(number) => number");
AssertType(x, "number");
AssertType(1, "int");

let a: { x: number; y?: number; };
AssertType(a, "{ x: number; y?: number; }");
AssertType(x, "number");
AssertType(y, "number");

let b: { x: number; z?: number; };
AssertType(b, "{ x: number; z?: number; }");
AssertType(x, "number");
AssertType(z, "number");

let r4 = foo((x: typeof a) => a, (x: typeof b) => b); // typeof a => typeof a
AssertType(r4, "({ x: number; y?: number; }) => { x: number; y?: number; }");
AssertType(foo((x: typeof a) => a, (x: typeof b) => b), "({ x: number; y?: number; }) => { x: number; y?: number; }");
AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");
AssertType((x: typeof a) => a, "(typeof a) => { x: number; y?: number; }");
AssertType(x, "{ x: number; y?: number; }");
AssertType(a, "{ x: number; y?: number; }");
AssertType(a, "{ x: number; y?: number; }");
AssertType((x: typeof b) => b, "(typeof b) => { x: number; z?: number; }");
AssertType(x, "{ x: number; z?: number; }");
AssertType(b, "{ x: number; z?: number; }");
AssertType(b, "{ x: number; z?: number; }");

let r5 = foo((x: typeof b) => b, (x: typeof a) => a); // typeof b => typeof b
AssertType(r5, "({ x: number; z?: number; }) => { x: number; z?: number; }");
AssertType(foo((x: typeof b) => b, (x: typeof a) => a), "({ x: number; z?: number; }) => { x: number; z?: number; }");
AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");
AssertType((x: typeof b) => b, "(typeof b) => { x: number; z?: number; }");
AssertType(x, "{ x: number; z?: number; }");
AssertType(b, "{ x: number; z?: number; }");
AssertType(b, "{ x: number; z?: number; }");
AssertType((x: typeof a) => a, "(typeof a) => { x: number; y?: number; }");
AssertType(x, "{ x: number; y?: number; }");
AssertType(a, "{ x: number; y?: number; }");
AssertType(a, "{ x: number; y?: number; }");

function other<T>(x: T) {
    let r6 = foo((a: T) => a, (b: T) => b); // T => T
AssertType(r6, "(T) => T");
AssertType(foo((a: T) => a, (b: T) => b), "(T) => T");
AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");
AssertType((a: T) => a, "(T) => T");
AssertType(a, "T");
AssertType(a, "T");
AssertType((b: T) => b, "(T) => T");
AssertType(b, "T");
AssertType(b, "T");

    let r6b = foo((a) => a, (b) => b); // {} => {
AssertType(r6b, "(unknown) => unknown");

AssertType(foo((a) => a, (b) => b), "(unknown) => unknown");

AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");

AssertType((a) => a, "(unknown) => unknown");

AssertType(a, "unknown");

AssertType(a, "unknown");

AssertType((b) => b, "(unknown) => unknown");

AssertType(b, "unknown");

AssertType(b, "unknown");
}
}

function other2<T extends Date>(x: T) {
    let r7 = foo((a: T) => a, (b: T) => b); // T => T
AssertType(r7, "(T) => T");
AssertType(foo((a: T) => a, (b: T) => b), "(T) => T");
AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");
AssertType((a: T) => a, "(T) => T");
AssertType(a, "T");
AssertType(a, "T");
AssertType((b: T) => b, "(T) => T");
AssertType(b, "T");
AssertType(b, "T");

    let r7b = foo((a) => a, (b) => b); // {} => {
AssertType(r7b, "(unknown) => unknown");

AssertType(foo((a) => a, (b) => b), "(unknown) => unknown");

AssertType(foo, "<T>((T) => T, (T) => T) => (T) => T");

AssertType((a) => a, "(unknown) => unknown");

AssertType(a, "unknown");

AssertType(a, "unknown");

AssertType((b) => b, "(unknown) => unknown");

AssertType(b, "unknown");

AssertType(b, "unknown");
}

    let r8 = r7(null);
AssertType(r8, "T");
AssertType(r7(null), "T");
AssertType(r7, "(T) => T");
AssertType(null, "null");
}


function foo2<T extends Date>(a: (x: T) => T, b: (x: T) => T) {
    let r: (x: T) => T;
AssertType(r, "(T) => T");
AssertType(x, "T");

AssertType(r, "(T) => T");
    return r;
}

function other3<T extends RegExp>(x: T) {
    let r8 = foo2((a: Date) => a, (b: Date) => b); // Date => Date
AssertType(r8, "(Date) => Date");
AssertType(foo2((a: Date) => a, (b: Date) => b), "(Date) => Date");
AssertType(foo2, "<T extends Date>((T) => T, (T) => T) => (T) => T");
AssertType((a: Date) => a, "(Date) => Date");
AssertType(a, "Date");
AssertType(a, "Date");
AssertType((b: Date) => b, "(Date) => Date");
AssertType(b, "Date");
AssertType(b, "Date");
}

