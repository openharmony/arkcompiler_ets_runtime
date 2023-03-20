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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithObjectTypeArgsAndStringIndexer.ts ===
declare function AssertType(value:any, type:string):void;
// Type inference infers from indexers in target type, no errors expected

function foo<T>(x: T) {
AssertType(x, "T");
    return x;
}

let a: { [x: string]: Date };
AssertType(a, "{ [string]: Date; }");
AssertType(x, "string");

let r = foo(a);
AssertType(r, "{ [string]: Date; }");
AssertType(foo(a), "{ [string]: Date; }");
AssertType(foo, "<T>(T) => T");
AssertType(a, "{ [string]: Date; }");

function other<T>(arg: T) {
    let b: { [x: string]: T };
AssertType(b, "{ [string]: T; }");
AssertType(x, "string");

    let r2 = foo(b); // T
AssertType(r2, "{ [string]: T; }");
AssertType(foo(b), "{ [string]: T; }");
AssertType(foo, "<T>(T) => T");
AssertType(b, "{ [string]: T; }");
}

function other2<T extends Date>(arg: T) {
    let b: { [x: string]: T };
AssertType(b, "{ [string]: T; }");
AssertType(x, "string");

    let r2 = foo(b);
AssertType(r2, "{ [string]: T; }");
AssertType(foo(b), "{ [string]: T; }");
AssertType(foo, "<T>(T) => T");
AssertType(b, "{ [string]: T; }");

    let d: Date = r2['hm']; // ok
AssertType(d, "Date");
AssertType(r2['hm'], "T");
AssertType(r2, "{ [string]: T; }");
AssertType('hm', "string");
}

function other3<T extends Date, U extends Date>(arg: T) {
    let b: { [x: string]: T };
AssertType(b, "{ [string]: T; }");
AssertType(x, "string");

    let r2 = foo(b);
AssertType(r2, "{ [string]: T; }");
AssertType(foo(b), "{ [string]: T; }");
AssertType(foo, "<T>(T) => T");
AssertType(b, "{ [string]: T; }");

    let d: Date = r2['hm']; // ok
AssertType(d, "Date");
AssertType(r2['hm'], "T");
AssertType(r2, "{ [string]: T; }");
AssertType('hm', "string");
}

