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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/everyTypeAssignableToAny.ts ===
declare function AssertType(value:any, type:string):void;
let a: any;
AssertType(a, "any");

class C {
    foo: string;
}
let ac: C;
AssertType(ac, "C");

interface I {
    foo: string;
}
let ai: I;
AssertType(ai, "I");

enum E { A }
let ae: E;
AssertType(ae, "E");

let b: number;
AssertType(b, "number");

let c: string;
AssertType(c, "string");

let d: boolean;
AssertType(d, "boolean");

let e: Date;
AssertType(e, "Date");

let f: any;
AssertType(f, "any");

let g: void;
AssertType(g, "void");

let h: Object;
AssertType(h, "Object");

let i: {};
AssertType(i, "{}");

let j: () => {};
AssertType(j, "() => {}");

let k: Function;
AssertType(k, "Function");

let l: (x: number) => string;
AssertType(l, "(number) => string");
AssertType(x, "number");

let m: number[];
AssertType(m, "number[]");

let n: { foo: string };
AssertType(n, "{ foo: string; }");
AssertType(foo, "string");

let o: <T>(x: T) => T;
AssertType(o, "<T>(T) => T");
AssertType(x, "T");

let p: Number;
AssertType(p, "Number");

let q: String;
AssertType(q, "String");

a = b;
AssertType(a = b, "number");
AssertType(a, "any");
AssertType(b, "number");

a = c;
AssertType(a = c, "string");
AssertType(a, "any");
AssertType(c, "string");

a = d;
AssertType(a = d, "boolean");
AssertType(a, "any");
AssertType(d, "boolean");

a = e;
AssertType(a = e, "Date");
AssertType(a, "any");
AssertType(e, "Date");

a = f;
AssertType(a = f, "any");
AssertType(a, "any");
AssertType(f, "any");

a = g;
AssertType(a = g, "void");
AssertType(a, "any");
AssertType(g, "void");

a = h;
AssertType(a = h, "Object");
AssertType(a, "any");
AssertType(h, "Object");

a = i;
AssertType(a = i, "{}");
AssertType(a, "any");
AssertType(i, "{}");

a = j;
AssertType(a = j, "() => {}");
AssertType(a, "any");
AssertType(j, "() => {}");

a = k;
AssertType(a = k, "Function");
AssertType(a, "any");
AssertType(k, "Function");

a = l;
AssertType(a = l, "(number) => string");
AssertType(a, "any");
AssertType(l, "(number) => string");

a = m;
AssertType(a = m, "number[]");
AssertType(a, "any");
AssertType(m, "number[]");

a = o;
AssertType(a = o, "<T>(T) => T");
AssertType(a, "any");
AssertType(o, "<T>(T) => T");

a = p; 
AssertType(a = p, "Number");
AssertType(a, "any");
AssertType(p, "Number");

a = q;
AssertType(a = q, "String");
AssertType(a, "any");
AssertType(q, "String");

a = ac;
AssertType(a = ac, "C");
AssertType(a, "any");
AssertType(ac, "C");

a = ai;
AssertType(a = ai, "I");
AssertType(a, "any");
AssertType(ai, "I");

a = ae;
AssertType(a = ae, "E");
AssertType(a, "any");
AssertType(ae, "E");

function foo<T, U /*extends T*/, V extends Date>(x: T, y: U, z: V) {
    a = x;
AssertType(a = x, "T");
AssertType(a, "any");
AssertType(x, "T");

    a = y;
AssertType(a = y, "U");
AssertType(a, "any");
AssertType(y, "U");

    a = z;
AssertType(a = z, "V");
AssertType(a, "any");
AssertType(z, "V");
}
//function foo<T, U extends T, V extends Date>(x: T, y: U, z: V) {
//    a = x;
//    a = y;
//    a = z;
//}

