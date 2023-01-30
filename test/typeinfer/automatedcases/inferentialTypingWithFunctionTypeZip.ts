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

// === tests/cases/compiler/inferentialTypingWithFunctionTypeZip.ts ===
declare function AssertType(value:any, type:string):void;
let pair: <T, S>(x: T) => (y: S) => { x: T; y: S; 
AssertType(pair, "<T, S>(T) => (S) => { x: T; y: S; }");

AssertType(x, "T");

AssertType(y, "S");

AssertType(x, "T");

AssertType(y, "S");
}

let zipWith: <T, S, U>(a: T[], b: S[], f: (x: T) => (y: S) => U) => U[];
AssertType(zipWith, "<T, S, U>(T[], S[], (T) => (S) => U) => U[]");
AssertType(a, "T[]");
AssertType(b, "S[]");
AssertType(f, "(T) => (S) => U");
AssertType(x, "T");
AssertType(y, "S");

let result = zipWith([1, 2], ['a', 'b'], pair);
AssertType(result, "{ x: number; y: unknown; }[]");
AssertType(zipWith([1, 2], ['a', 'b'], pair), "{ x: number; y: unknown; }[]");
AssertType(zipWith, "<T, S, U>(T[], S[], (T) => (S) => U) => U[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(['a', 'b'], "string[]");
AssertType('a', "string");
AssertType('b', "string");
AssertType(pair, "<T, S>(T) => (S) => { x: T; y: S; }");

let i = result[0].x; // number
AssertType(i, "number");
AssertType(result[0].x, "number");


