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

// === tests/cases/compiler/contextualSignatureInstantiation3.ts ===
declare function AssertType(value:any, type:string):void;
function map<T, U>(items: T[], f: (x: T) => U): U[]{
AssertType(items.map(f), "U[]");
AssertType(items.map, "<U>((T, number, T[]) => U, ?any) => U[]");
AssertType(f, "(T) => U");
    return items.map(f);
}

function identity<T>(x: T) {
AssertType(x, "T");
    return x;
}

function singleton<T>(x: T) {
AssertType([x], "T[]");
AssertType(x, "T");
    return [x];
}

let xs = [1, 2, 3];
AssertType(xs, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

// Have compiler check that we get the correct types
let v1: number[];
AssertType(v1, "number[]");

let v1 = xs.map(identity);      // Error if not number[]
AssertType(v1, "number[]");
AssertType(xs.map(identity), "number[]");
AssertType(xs.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(identity, "<T>(T) => T");

let v1 = map(xs, identity);     // Error if not number[]
AssertType(v1, "number[]");
AssertType(map(xs, identity), "number[]");
AssertType(map, "<T, U>(T[], (T) => U) => U[]");
AssertType(xs, "number[]");
AssertType(identity, "<T>(T) => T");

let v2: number[][];         
AssertType(v2, "number[][]");

let v2 = xs.map(singleton);     // Error if not number[][]
AssertType(v2, "number[][]");
AssertType(xs.map(singleton), "number[][]");
AssertType(xs.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(singleton, "<T>(T) => T[]");

let v2 = map(xs, singleton);    // Error if not number[][]
AssertType(v2, "number[][]");
AssertType(map(xs, singleton), "number[][]");
AssertType(map, "<T, U>(T[], (T) => U) => U[]");
AssertType(xs, "number[]");
AssertType(singleton, "<T>(T) => T[]");


