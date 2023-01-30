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

// === tests/cases/compiler/declarationEmitDestructuringArrayPattern4.ts ===
declare function AssertType(value:any, type:string):void;
let [...a5] = [1, 2, 3];
AssertType(a5, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [x14, ...a6] = [1, 2, 3];
AssertType(x14, "number");
AssertType(a6, "[number, number]");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [x15, y15, ...a7] = [1, 2, 3];
AssertType(x15, "number");
AssertType(y15, "number");
AssertType(a7, "[number]");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [x16, y16, z16, ...a8] = [1, 2, 3];
AssertType(x16, "number");
AssertType(y16, "number");
AssertType(z16, "number");
AssertType(a8, "[]");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [...a9] = [1, "hello", true];
AssertType(a9, "(union)[]");
AssertType([1, "hello", true], "(union)[]");
AssertType(1, "int");
AssertType("hello", "string");
AssertType(true, "boolean");

let [x17, ...a10] = [1, "hello", true];
AssertType(x17, "number");
AssertType(a10, "[string, boolean]");
AssertType([1, "hello", true], "[number, string, boolean]");
AssertType(1, "int");
AssertType("hello", "string");
AssertType(true, "boolean");

let [x18, y18, ...a12] = [1, "hello", true];
AssertType(x18, "number");
AssertType(y18, "string");
AssertType(a12, "[boolean]");
AssertType([1, "hello", true], "[number, string, boolean]");
AssertType(1, "int");
AssertType("hello", "string");
AssertType(true, "boolean");

let [x19, y19, z19, ...a13] = [1, "hello", true];
AssertType(x19, "number");
AssertType(y19, "string");
AssertType(z19, "boolean");
AssertType(a13, "[]");
AssertType([1, "hello", true], "[number, string, boolean]");
AssertType(1, "int");
AssertType("hello", "string");
AssertType(true, "boolean");


