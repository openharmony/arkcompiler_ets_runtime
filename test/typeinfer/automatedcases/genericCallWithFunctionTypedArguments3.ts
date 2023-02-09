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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithFunctionTypedArguments3.ts ===
declare function AssertType(value:any, type:string):void;
// No inference is made from function typed arguments which have multiple call signatures

let a: {
AssertType(a, "{ (boolean): boolean; (string): any; }");

    (x: boolean): boolean;
AssertType(x, "boolean");

    (x: string): any;
AssertType(x, "string");
}

function foo4<T, U>(cb: (x: T) => U) {
    let u: U;
AssertType(u, "U");

AssertType(u, "U");
    return u;
}

let r = foo4(a); // T is {} (candidates boolean and string), U is any (candidates any and boolean)
AssertType(r, "any");
AssertType(foo4(a), "any");
AssertType(foo4, "<T, U>((T) => U) => U");
AssertType(a, "{ (boolean): boolean; (string): any; }");

let b: {
AssertType(b, "{ <T>(boolean): T; <T>(T): any; }");

    <T>(x: boolean): T;
AssertType(x, "boolean");

    <T>(x: T): any;
AssertType(x, "T");
}

let r2 = foo4(b); // T is {} (candidates boolean and {}), U is any (candidates any and {})
AssertType(r2, "any");
AssertType(foo4(b), "any");
AssertType(foo4, "<T, U>((T) => U) => U");
AssertType(b, "{ <T>(boolean): T; <T>(T): any; }");


