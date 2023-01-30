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

// === tests/cases/compiler/contextuallyTypingOrOperator2.ts ===
declare function AssertType(value:any, type:string):void;
let v: { a: (_: string) => number } = { a: s => s.length } || { a: s => 1 };
AssertType(v, "{ a: (string) => number; }");
AssertType(a, "(string) => number");
AssertType(_, "string");
AssertType({ a: s => s.length } || { a: s => 1 }, "{ a: (string) => number; }");
AssertType({ a: s => s.length }, "{ a: (string) => number; }");
AssertType(a, "(string) => number");
AssertType(s => s.length, "(string) => number");
AssertType(s, "string");
AssertType(s.length, "number");
AssertType({ a: s => 1 }, "{ a: (string) => number; }");
AssertType(a, "(string) => number");
AssertType(s => 1, "(string) => number");
AssertType(s, "string");
AssertType(1, "int");

let v2 = (s: string) => s.length || function (s) { s.aaa };
AssertType(v2, "(string) => union");
AssertType((s: string) => s.length || function (s) { s.aaa }, "(string) => union");
AssertType(s, "string");
AssertType(s.length || function (s) { s.aaa }, "union");
AssertType(s.length, "number");
AssertType(function (s) { s.aaa }, "(any) => void");
AssertType(s, "any");
AssertType(s.aaa, "any");


