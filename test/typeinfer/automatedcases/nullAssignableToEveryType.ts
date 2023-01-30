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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/nullAssignableToEveryType.ts ===
declare function AssertType(value:any, type:string):void;
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

let b: number = null;
AssertType(b, "number");
AssertType(null, "null");

let c: string = null;
AssertType(c, "string");
AssertType(null, "null");

let d: boolean = null;
AssertType(d, "boolean");
AssertType(null, "null");

let e: Date = null;
AssertType(e, "Date");
AssertType(null, "null");

let f: any = null;
AssertType(f, "any");
AssertType(null, "null");

let g: void = null;
AssertType(g, "void");
AssertType(null, "null");

let h: Object = null;
AssertType(h, "Object");
AssertType(null, "null");

let i: {} = null;
AssertType(i, "{}");
AssertType(null, "null");

let j: () => {} = null;
AssertType(j, "() => {}");
AssertType(null, "null");

let k: Function = null;
AssertType(k, "Function");
AssertType(null, "null");

let l: (x: number) => string = null;
AssertType(l, "(number) => string");
AssertType(x, "number");
AssertType(null, "null");

ac = null;
AssertType(ac = null, "null");
AssertType(ac, "C");
AssertType(null, "null");

ai = null;
AssertType(ai = null, "null");
AssertType(ai, "I");
AssertType(null, "null");

ae = null;
AssertType(ae = null, "null");
AssertType(ae, "E");
AssertType(null, "null");

let m: number[] = null;
AssertType(m, "number[]");
AssertType(null, "null");

let n: { foo: string } = null;
AssertType(n, "{ foo: string; }");
AssertType(foo, "string");
AssertType(null, "null");

let o: <T>(x: T) => T = null;
AssertType(o, "<T>(T) => T");
AssertType(x, "T");
AssertType(null, "null");

let p: Number = null;
AssertType(p, "Number");
AssertType(null, "null");

let q: String = null;
AssertType(q, "String");
AssertType(null, "null");

function foo<T, U, V extends Date>(x: T, y: U, z: V) {
    x = null;
AssertType(x = null, "null");
AssertType(x, "T");
AssertType(null, "null");

    y = null;
AssertType(y = null, "null");
AssertType(y, "U");
AssertType(null, "null");

    z = null;
AssertType(z = null, "null");
AssertType(z, "V");
AssertType(null, "null");
}

//function foo<T, U extends T, V extends Date>(x: T, y: U, z: V) {
//    x = null;
//    y = null;
//    z = null;
//}

