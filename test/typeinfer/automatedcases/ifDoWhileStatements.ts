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

// === tests/cases/conformance/statements/ifDoWhileStatements/ifDoWhileStatements.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
    id: number;
}

class C implements I {
    id: number;
    name: string;
}

class C2 extends C {
    valid: boolean;
}

class D<T>{
    source: T;
    recurse: D<T>;
    wrapped: D<D<T>>
}

function F(x: string): number { 
AssertType(42, "int");
return 42; 
}

function F2(x: number): boolean { 
AssertType(x < 42, "boolean");
return x < 42; 

AssertType(x, "number");

AssertType(42, "int");
}

module M {
    export class A {
        name: string;
    }

    export function F2(x: number): string { 
AssertType(x.toString(), "string");
return x.toString(); 

AssertType(x.toString, "(?number) => string");
}
}

module N {
    export class A {
        id: number;
    }

    export function F2(x: number): string { 
AssertType(x.toString(), "string");
return x.toString(); 

AssertType(x.toString, "(?number) => string");
}
}

// literals
if (true) { }
while (true) { }
do { }while(true)

if (null) { }
while (null) { }
do { }while(null)

if (undefined) { }
while (undefined) { }
do { }while(undefined)

if (0.0) { }
while (0.0) { }
do { }while(0.0)

if ('a string') { }
while ('a string') { }
do { }while('a string')

if ('') { }
while ('') { }
do { }while('')

if (/[a-z]/) { }
while (/[a-z]/) { }
do { }while(/[a-z]/)

if ([]) { }
while ([]) { }
do { }while([])

if ([1, 2]) { }
while ([1, 2]) { }
do { }while([1, 2])

if ({}) { }
while ({}) { }
do { }while({})

if ({ x: 1, y: 'a' }) { }
while ({ x: 1, y: 'a' }) { }
do { }while({ x: 1, y: 'a' })

if (() => 43) { }
while (() => 43) { }
do { }while(() => 43)

if (new C()) { }
while (new C()) { }
do { }while(new C())

if (new D<C>()) { }
while (new D<C>()) { }
do { }while(new D<C>())

// references
let a = true;
AssertType(a, "boolean");
AssertType(true, "boolean");

if (a) { }
while (a) { }
do { }while(a)

let b = null;
AssertType(b, "any");
AssertType(null, "null");

if (b) { }
while (b) { }
do { }while(b)

let c = undefined;
AssertType(c, "any");
AssertType(undefined, "undefined");

if (c) { }
while (c) { }
do { }while(c)

let d = 0.0;
AssertType(d, "number");
AssertType(0.0, "int");

if (d) { }
while (d) { }
do { }while(d)

let e = 'a string';
AssertType(e, "string");
AssertType('a string', "string");

if (e) { }
while (e) { }
do { }while(e)

let f = '';
AssertType(f, "string");
AssertType('', "string");

if (f) { }
while (f) { }
do { }while(f)

let g = /[a-z]/
AssertType(g, "RegExp");
AssertType(/[a-z]/, "RegExp");

if (g) { }
while (g) { }
do { }while(g)

let h = [];
AssertType(h, "any[]");
AssertType([], "undefined[]");

if (h) { }
while (h) { }
do { }while(h)

let i = [1, 2];
AssertType(i, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

if (i) { }
while (i) { }
do { }while(i)

let j = {};
AssertType(j, "{}");
AssertType({}, "{}");

if (j) { }
while (j) { }
do { }while(j)

let k = { x: 1, y: 'a' };
AssertType(k, "{ x: number; y: string; }");
AssertType({ x: 1, y: 'a' }, "{ x: number; y: string; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "string");
AssertType('a', "string");

if (k) { }
while (k) { }
do { }while(k)

function fn(x?: string): I { 
AssertType(null, "null");
return null; 
}

if (fn()) { }
while (fn()) { }
do { }while(fn())

if (fn) { }
while (fn) { }
do { }while(fn)




