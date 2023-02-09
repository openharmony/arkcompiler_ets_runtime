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

// === tests/cases/conformance/types/any/assignEveryTypeToAny.ts ===
declare function AssertType(value:any, type:string):void;
// all of these are valid

let x: any;
AssertType(x, "any");

x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");

let a = 2;
AssertType(a, "number");
AssertType(2, "int");

x = a;
AssertType(x = a, "number");
AssertType(x, "any");
AssertType(a, "number");

x = true;
AssertType(x = true, "boolean");
AssertType(x, "any");
AssertType(true, "boolean");

let b = true;
AssertType(b, "boolean");
AssertType(true, "boolean");

x = b;
AssertType(x = b, "boolean");
AssertType(x, "any");
AssertType(b, "boolean");

x = "";
AssertType(x = "", "string");
AssertType(x, "any");
AssertType("", "string");

let c = "";
AssertType(c, "string");
AssertType("", "string");

x = c;
AssertType(x = c, "string");
AssertType(x, "any");
AssertType(c, "string");

let d: void;
AssertType(d, "void");

x = d;
AssertType(x = d, "void");
AssertType(x, "any");
AssertType(d, "void");

let e = undefined;
AssertType(e, "any");
AssertType(undefined, "undefined");

x = e;
AssertType(x = e, "any");
AssertType(x, "any");
AssertType(e, "any");

let e2: typeof undefined;
AssertType(e2, "any");
AssertType(undefined, "undefined");

x = e2;
AssertType(x = e2, "any");
AssertType(x, "any");
AssertType(e2, "any");

enum E {
    A
}

x = E.A;
AssertType(x = E.A, "E");
AssertType(x, "any");
AssertType(E.A, "E");

let f = E.A;
AssertType(f, "E");
AssertType(E.A, "E");

x = f;
AssertType(x = f, "E");
AssertType(x, "any");
AssertType(f, "E");

interface I {
    foo: string;
}

let g: I;
AssertType(g, "I");

x = g;
AssertType(x = g, "I");
AssertType(x, "any");
AssertType(g, "I");

class C {
    bar: string;
}

let h: C;
AssertType(h, "C");

x = h;
AssertType(x = h, "C");
AssertType(x, "any");
AssertType(h, "C");

let i: { (): string };
AssertType(i, "() => string");

x = i;
AssertType(x = i, "() => string");
AssertType(x, "any");
AssertType(i, "() => string");

x = { f() { 
AssertType(x = { f() { return 1; } }, "{ f(): number; }");
return 1; } 

AssertType(x, "any");

AssertType({ f() { return 1; } }, "{ f(): number; }");

AssertType(f, "() => number");

AssertType(1, "int");
}

x = { f<T>(x: T) { 
AssertType(x = { f<T>(x: T) { return x; } }, "{ f<T>(T): T; }");
return x; } 

AssertType(x, "any");

AssertType({ f<T>(x: T) { return x; } }, "{ f<T>(T): T; }");

AssertType(f, "<T>(T) => T");

AssertType(x, "T");

AssertType(x, "T");
}

function j<T>(a: T) {
    x = a;
AssertType(x = a, "T");
AssertType(x, "any");
AssertType(a, "T");
}

