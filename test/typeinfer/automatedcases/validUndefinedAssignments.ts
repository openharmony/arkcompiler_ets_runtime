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

// === tests/cases/conformance/types/primitives/undefined/validUndefinedAssignments.ts ===
declare function AssertType(value:any, type:string):void;
let x: typeof undefined;
AssertType(x, "any");
AssertType(undefined, "undefined");

let a: number = x;
AssertType(a, "number");
AssertType(x, "any");

let b: boolean = x;
AssertType(b, "boolean");
AssertType(x, "any");

let c: string = x;
AssertType(c, "string");
AssertType(x, "any");

let d: void = x;
AssertType(d, "void");
AssertType(x, "any");

let e: typeof undefined = x;
AssertType(e, "any");
AssertType(undefined, "undefined");
AssertType(x, "any");

e = x; // should work
AssertType(e = x, "any");
AssertType(e, "any");
AssertType(x, "any");

class C { foo: string }
let f: C;
AssertType(f, "C");

f = x;
AssertType(f = x, "any");
AssertType(f, "C");
AssertType(x, "any");

interface I { foo: string }
let g: I;
AssertType(g, "I");

g = x;
AssertType(g = x, "any");
AssertType(g, "I");
AssertType(x, "any");

let h: { f(): void } = x;
AssertType(h, "{ f(): void; }");
AssertType(f, "() => void");
AssertType(x, "any");

function i<T>(a: T) {
    a = x;
AssertType(a = x, "any");
AssertType(a, "T");
AssertType(x, "any");
}

