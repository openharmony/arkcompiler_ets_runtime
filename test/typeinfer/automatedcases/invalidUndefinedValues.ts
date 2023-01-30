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

// === tests/cases/conformance/types/primitives/undefined/invalidUndefinedValues.ts ===
declare function AssertType(value:any, type:string):void;
let x: typeof undefined;
AssertType(x, "any");
AssertType(undefined, "undefined");

x = 1;
AssertType(x = 1, "int");
AssertType(x, "any");
AssertType(1, "int");

x = '';
AssertType(x = '', "string");
AssertType(x, "any");
AssertType('', "string");

x = true;
AssertType(x = true, "boolean");
AssertType(x, "any");
AssertType(true, "boolean");

let a: void;
AssertType(a, "void");

x = a;
AssertType(x = a, "void");
AssertType(x, "any");
AssertType(a, "void");

x = null;
AssertType(x = null, "null");
AssertType(x, "any");
AssertType(null, "null");

class C { foo: string }
let b: C;
AssertType(b, "C");

x = C;
AssertType(x = C, "typeof C");
AssertType(x, "any");
AssertType(C, "typeof C");

x = b;
AssertType(x = b, "C");
AssertType(x, "any");
AssertType(b, "C");

interface I { foo: string }
let c: I;
AssertType(c, "I");

x = c;
AssertType(x = c, "I");
AssertType(x, "any");
AssertType(c, "I");

module M { export let x = 1; }
x = M;
AssertType(x = M, "typeof M");
AssertType(x, "any");
AssertType(M, "typeof M");

x = { f() { } 
AssertType(x = { f() { } }, "{ f(): void; }");

AssertType(x, "any");

AssertType({ f() { } }, "{ f(): void; }");

AssertType(f, "() => void");
}

function f<T>(a: T) {
    x = a;
AssertType(x = a, "T");
AssertType(x, "any");
AssertType(a, "T");
}
x = f;
AssertType(x = f, "<T>(T) => void");
AssertType(x, "any");
AssertType(f, "<T>(T) => void");

enum E { A }
x = E;
AssertType(x = E, "typeof E");
AssertType(x, "any");
AssertType(E, "typeof E");

x = E.A;
AssertType(x = E.A, "E");
AssertType(x, "any");
AssertType(E.A, "E");


