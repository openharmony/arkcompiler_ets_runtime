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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/typeParameterUsedAsTypeParameterConstraint.ts ===
declare function AssertType(value:any, type:string):void;
// Type parameters are in scope in their own and other type parameter lists

function foo<T, U extends T>(x: T, y: U): T {
    x = y;
AssertType(x = y, "U");
AssertType(x, "T");
AssertType(y, "U");

AssertType(y, "U");
    return y;
}

function foo2<U extends T, T>(x: T, y: U): T {
    x = y;
AssertType(x = y, "U");
AssertType(x, "T");
AssertType(y, "U");

AssertType(y, "U");
    return y;
}

let f = function <T, U extends T>(x: T, y: U): T {
AssertType(f, "<T, U extends T>(T, U) => T");
AssertType(x, "T");
AssertType(y, "U");
AssertType(function <T, U extends T>(x: T, y: U): T {    x = y;    return y;}, "<T, U extends T>(T, U) => T");

    x = y;
AssertType(x = y, "U");
AssertType(x, "T");
AssertType(y, "U");

AssertType(y, "U");
    return y;
}

let f2 = function <U extends T, T>(x: T, y: U): T {
AssertType(f2, "<U extends T, T>(T, U) => T");
AssertType(x, "T");
AssertType(y, "U");
AssertType(function <U extends T, T>(x: T, y: U): T {    x = y;    return y;}, "<U extends T, T>(T, U) => T");

    x = y;
AssertType(x = y, "U");
AssertType(x, "T");
AssertType(y, "U");

AssertType(y, "U");
    return y;
}

let f3 = <T, U extends T>(x: T, y: U): T => {
AssertType(f3, "<T, U extends T>(T, U) => T");
AssertType(x, "T");
AssertType(y, "U");
AssertType(<T, U extends T>(x: T, y: U): T => {    x = y;    return y;}, "<T, U extends T>(T, U) => T");

    x = y;
AssertType(x = y, "U");
AssertType(x, "T");
AssertType(y, "U");

AssertType(y, "U");
    return y;
}

let f4 = <U extends T, T>(x: T, y: U): T => {
AssertType(f4, "<U extends T, T>(T, U) => T");
AssertType(x, "T");
AssertType(y, "U");
AssertType(<U extends T, T>(x: T, y: U): T => {    x = y;    return y;}, "<U extends T, T>(T, U) => T");

    x = y;
AssertType(x = y, "U");
AssertType(x, "T");
AssertType(y, "U");

AssertType(y, "U");
    return y;
}

