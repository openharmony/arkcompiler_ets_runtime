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

// === tests/cases/conformance/es6/destructuring/restElementWithAssignmentPattern3.ts ===
declare function AssertType(value:any, type:string):void;
let a: string, b: number;
AssertType(a, "string");
AssertType(b, "number");

let tuple: [string, number] = ["", 1];
AssertType(tuple, "[string, number]");
AssertType(["", 1], "[string, number]");
AssertType("", "string");
AssertType(1, "int");

[...[a, b = 0]] = tuple;
AssertType([...[a, b = 0]] = tuple, "[string, number]");
AssertType([...[a, b = 0]], "[string, number]");
AssertType(...[a, b = 0], "union");
AssertType([a, b = 0], "[string, number]");
AssertType(a, "string");
AssertType(b = 0, "int");
AssertType(b, "number");
AssertType(0, "int");
AssertType(tuple, "[string, number]");


