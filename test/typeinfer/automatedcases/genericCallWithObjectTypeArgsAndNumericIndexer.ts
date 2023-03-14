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

// === tests/cases/conformance/types/typeRelationships/typeInference/genericCallWithObjectTypeArgsAndNumericIndexer.ts ===
declare function AssertType(value:any, type:string):void;
// Type inference infers from indexers in target type, no errors expected

function foo<T>(x: T) {
AssertType(x, "T");
    return x;
}

let a: { [x: number]: Date };
AssertType(a, "{ [number]: Date; }");
AssertType(x, "number");

let r = foo(a);
AssertType(r, "{ [number]: Date; }");
AssertType(foo(a), "{ [number]: Date; }");
AssertType(foo, "<T>(T) => T");
AssertType(a, "{ [number]: Date; }");

function other<T>(arg: T) {
    let b: { [x: number]: T };
AssertType(b, "{ [number]: T; }");
AssertType(x, "number");

    let r2 = foo(b); // T
AssertType(r2, "{ [number]: T; }");
AssertType(foo(b), "{ [number]: T; }");
AssertType(foo, "<T>(T) => T");
AssertType(b, "{ [number]: T; }");
}

function other2<T extends Date>(arg: T) {
    let b: { [x: number]: T };
AssertType(b, "{ [number]: T; }");
AssertType(x, "number");

    let r2 = foo(b);
AssertType(r2, "{ [number]: T; }");
AssertType(foo(b), "{ [number]: T; }");
AssertType(foo, "<T>(T) => T");
AssertType(b, "{ [number]: T; }");

    let d = r2[1];
AssertType(d, "T");
AssertType(r2[1], "T");
AssertType(r2, "{ [number]: T; }");
AssertType(1, "int");
}

function other3<T extends Date, U extends Date>(arg: T) {
    let b: { [x: number]: T };
AssertType(b, "{ [number]: T; }");
AssertType(x, "number");

    let r2 = foo(b);
AssertType(r2, "{ [number]: T; }");
AssertType(foo(b), "{ [number]: T; }");
AssertType(foo, "<T>(T) => T");
AssertType(b, "{ [number]: T; }");

    let d = r2[1];
AssertType(d, "T");
AssertType(r2[1], "T");
AssertType(r2, "{ [number]: T; }");
AssertType(1, "int");
}

