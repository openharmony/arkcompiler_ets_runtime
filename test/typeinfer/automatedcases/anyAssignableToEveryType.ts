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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/anyAssignableToEveryType.ts ===
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

let b: number = a;
AssertType(b, "number");
AssertType(a, "any");

let c: string = a;
AssertType(c, "string");
AssertType(a, "any");

let d: boolean = a;
AssertType(d, "boolean");
AssertType(a, "any");

let e: Date = a;
AssertType(e, "Date");
AssertType(a, "any");

let f: any = a;
AssertType(f, "any");
AssertType(a, "any");

let g: void = a;
AssertType(g, "void");
AssertType(a, "any");

let h: Object = a;
AssertType(h, "Object");
AssertType(a, "any");

let i: {} = a;
AssertType(i, "{}");
AssertType(a, "any");

let j: () => {} = a;
AssertType(j, "() => {}");
AssertType(a, "any");

let k: Function = a;
AssertType(k, "Function");
AssertType(a, "any");

let l: (x: number) => string = a;
AssertType(l, "(number) => string");
AssertType(x, "number");
AssertType(a, "any");

ac = a;
AssertType(ac = a, "any");
AssertType(ac, "C");
AssertType(a, "any");

ai = a;
AssertType(ai = a, "any");
AssertType(ai, "I");
AssertType(a, "any");

ae = a;
AssertType(ae = a, "any");
AssertType(ae, "E");
AssertType(a, "any");

let m: number[] = a;
AssertType(m, "number[]");
AssertType(a, "any");

let n: { foo: string } = a;
AssertType(n, "{ foo: string; }");
AssertType(foo, "string");
AssertType(a, "any");

let o: <T>(x: T) => T = a;
AssertType(o, "<T>(T) => T");
AssertType(x, "T");
AssertType(a, "any");

let p: Number = a;
AssertType(p, "Number");
AssertType(a, "any");

let q: String = a;
AssertType(q, "String");
AssertType(a, "any");

function foo<T, U /*extends T*/, V extends Date>(x: T, y: U, z: V) {
    x = a;
AssertType(x = a, "any");
AssertType(x, "T");
AssertType(a, "any");

    y = a;
AssertType(y = a, "any");
AssertType(y, "U");
AssertType(a, "any");

    z = a;
AssertType(z = a, "any");
AssertType(z, "V");
AssertType(a, "any");
}

//function foo<T, U extends T, V extends Date>(x: T, y: U, z: V) {
//    x = a;
//    y = a;
//    z = a;
//}

