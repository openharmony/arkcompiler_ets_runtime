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

// === tests/cases/conformance/jsdoc/b.ts ===
declare function AssertType(value:any, type:string):void;
let S: string;
AssertType(S, "string");

let s: string;
AssertType(s, "string");

let N: number;
AssertType(N, "number");

let n: number
AssertType(n, "number");

let B: boolean;
AssertType(B, "boolean");

let b: boolean;
AssertType(b, "boolean");

let V :void;
AssertType(V, "void");

let v: void;
AssertType(v, "void");

let U: undefined;
AssertType(U, "undefined");

let u: undefined;
AssertType(u, "undefined");

let Nl: null;
AssertType(Nl, "null");
AssertType(null, "null");

let nl: null;
AssertType(nl, "null");
AssertType(null, "null");

let A: any[];
AssertType(A, "any[]");

let a: any[];
AssertType(a, "any[]");

let P: Promise<any>;
AssertType(P, "Promise<any>");

let p: Promise<any>;
AssertType(p, "Promise<any>");

let nullable: number | null;
AssertType(nullable, "union");
AssertType(null, "null");

let Obj: any;
AssertType(Obj, "any");

let obj: any;
AssertType(obj, "any");

let Func: Function;
AssertType(Func, "Function");

let f: (s: string) => number;
AssertType(f, "(string) => number");
AssertType(s, "string");

let ctor: new (s: string) => { s: string };
AssertType(ctor, "new (string) => { s: string; }");
AssertType(s, "string");
AssertType(s, "string");


