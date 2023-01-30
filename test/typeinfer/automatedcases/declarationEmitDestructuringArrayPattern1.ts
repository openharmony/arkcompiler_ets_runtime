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

// === tests/cases/compiler/declarationEmitDestructuringArrayPattern1.ts ===
declare function AssertType(value:any, type:string):void;
let [] = [1, "hello"]; // Dont emit anything
AssertType([1, "hello"], "(union)[]");
AssertType(1, "int");
AssertType("hello", "string");

let [x] = [1, "hello"]; // emit x: number
AssertType(x, "number");
AssertType([1, "hello"], "[number, string]");
AssertType(1, "int");
AssertType("hello", "string");

let [x1, y1] = [1, "hello"]; // emit x1: number, y1: string
AssertType(x1, "number");
AssertType(y1, "string");
AssertType([1, "hello"], "[number, string]");
AssertType(1, "int");
AssertType("hello", "string");

let [, , z1] = [0, 1, 2]; // emit z1: number
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(z1, "number");
AssertType([0, 1, 2], "[number, number, number]");
AssertType(0, "int");
AssertType(1, "int");
AssertType(2, "int");

let a = [1, "hello"];
AssertType(a, "(union)[]");
AssertType([1, "hello"], "(union)[]");
AssertType(1, "int");
AssertType("hello", "string");

let [x2] = a;          // emit x2: number | string
AssertType(x2, "union");
AssertType(a, "(union)[]");

let [x3, y3, z3] = a;  // emit x3, y3, z3 
AssertType(x3, "union");
AssertType(y3, "union");
AssertType(z3, "union");
AssertType(a, "(union)[]");


