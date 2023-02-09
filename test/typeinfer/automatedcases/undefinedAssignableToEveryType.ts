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

// === tests/cases/conformance/types/typeRelationships/assignmentCompatibility/undefinedAssignableToEveryType.ts ===
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

let b: number = undefined;
AssertType(b, "number");
AssertType(undefined, "undefined");

let c: string = undefined;
AssertType(c, "string");
AssertType(undefined, "undefined");

let d: boolean = undefined;
AssertType(d, "boolean");
AssertType(undefined, "undefined");

let e: Date = undefined;
AssertType(e, "Date");
AssertType(undefined, "undefined");

let f: any = undefined;
AssertType(f, "any");
AssertType(undefined, "undefined");

let g: void = undefined;
AssertType(g, "void");
AssertType(undefined, "undefined");

let h: Object = undefined;
AssertType(h, "Object");
AssertType(undefined, "undefined");

let i: {} = undefined;
AssertType(i, "{}");
AssertType(undefined, "undefined");

let j: () => {} = undefined;
AssertType(j, "() => {}");
AssertType(undefined, "undefined");

let k: Function = undefined;
AssertType(k, "Function");
AssertType(undefined, "undefined");

let l: (x: number) => string = undefined;
AssertType(l, "(number) => string");
AssertType(x, "number");
AssertType(undefined, "undefined");

ac = undefined;
AssertType(ac = undefined, "undefined");
AssertType(ac, "C");
AssertType(undefined, "undefined");

ai = undefined;
AssertType(ai = undefined, "undefined");
AssertType(ai, "I");
AssertType(undefined, "undefined");

ae = undefined;
AssertType(ae = undefined, "undefined");
AssertType(ae, "E");
AssertType(undefined, "undefined");

let m: number[] = undefined;
AssertType(m, "number[]");
AssertType(undefined, "undefined");

let n: { foo: string } = undefined;
AssertType(n, "{ foo: string; }");
AssertType(foo, "string");
AssertType(undefined, "undefined");

let o: <T>(x: T) => T = undefined;
AssertType(o, "<T>(T) => T");
AssertType(x, "T");
AssertType(undefined, "undefined");

let p: Number = undefined;
AssertType(p, "Number");
AssertType(undefined, "undefined");

let q: String = undefined;
AssertType(q, "String");
AssertType(undefined, "undefined");

function foo<T, U, V extends Date>(x: T, y: U, z: V) {
    x = undefined;
AssertType(x = undefined, "undefined");
AssertType(x, "T");
AssertType(undefined, "undefined");

    y = undefined;
AssertType(y = undefined, "undefined");
AssertType(y, "U");
AssertType(undefined, "undefined");

    z = undefined;
AssertType(z = undefined, "undefined");
AssertType(z, "V");
AssertType(undefined, "undefined");
}
//function foo<T, U extends T, V extends Date>(x: T, y: U, z: V) {
//    x = undefined;
//    y = undefined;
//    z = undefined;
//}

