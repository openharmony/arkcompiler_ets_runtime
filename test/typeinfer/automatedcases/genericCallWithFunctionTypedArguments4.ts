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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithFunctionTypedArguments4.ts ===
declare function AssertType(value:any, type:string):void;
// No inference is made from function typed arguments which have multiple call signatures

class C { foo: string }
class D { bar: string }
let a: {
AssertType(a, "{ new (boolean): C; new (string): D; }");

    new(x: boolean): C;
AssertType(x, "boolean");

    new(x: string): D;
AssertType(x, "string");
}

function foo4<T, U>(cb: new(x: T) => U) {
    let u: U;
AssertType(u, "U");

AssertType(u, "U");
    return u;
}

let r = foo4(a); // T is {} (candidates boolean and string), U is {} (candidates C and D)
AssertType(r, "D");
AssertType(foo4(a), "D");
AssertType(foo4, "<T, U>(new (T) => U) => U");
AssertType(a, "{ new (boolean): C; new (string): D; }");

let b: {
AssertType(b, "{ new <T>(boolean): T; new <T>(T): any; }");

    new<T>(x: boolean): T;
AssertType(x, "boolean");

    new<T>(x: T): any;
AssertType(x, "T");
}

let r2 = foo4(b); // T is {} (candidates boolean and {}), U is any (candidates any and {})
AssertType(r2, "any");
AssertType(foo4(b), "any");
AssertType(foo4, "<T, U>(new (T) => U) => U");
AssertType(b, "{ new <T>(boolean): T; new <T>(T): any; }");


