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

// === tests/cases/compiler/ambientConstLiterals.ts ===
declare function AssertType(value:any, type:string):void;
function f<T>(x: T): T {
AssertType(x, "T");
    return x;
}

enum E { A, B, C, "non identifier" }

const c1 = "abc";
AssertType(c1, "string");
AssertType("abc", "string");

const c2 = 123;
AssertType(c2, "int");
AssertType(123, "int");

const c3 = c1;
AssertType(c3, "string");
AssertType(c1, "string");

const c4 = c2;
AssertType(c4, "int");
AssertType(c2, "int");

const c5 = f(123);
AssertType(c5, "int");
AssertType(f(123), "int");
AssertType(f, "<T>(T) => T");
AssertType(123, "int");

const c6 = f(-123);
AssertType(c6, "int");
AssertType(f(-123), "int");
AssertType(f, "<T>(T) => T");
AssertType(-123, "int");
AssertType(123, "int");

const c7 = true;
AssertType(c7, "boolean");
AssertType(true, "boolean");

const c8 = E.A;
AssertType(c8, "E.A");
AssertType(E.A, "E.A");

const c8b = E["non identifier"];
AssertType(c8b, "(typeof E)["non identifier"]");
AssertType(E["non identifier"], "(typeof E)["non identifier"]");
AssertType(E, "typeof E");
AssertType("non identifier", "string");

const c9 = { x: "abc" };
AssertType(c9, "{ x: string; }");
AssertType({ x: "abc" }, "{ x: string; }");
AssertType(x, "string");
AssertType("abc", "string");

const c10 = [123];
AssertType(c10, "number[]");
AssertType([123], "number[]");
AssertType(123, "int");

const c11 = "abc" + "def";
AssertType(c11, "string");
AssertType("abc" + "def", "string");
AssertType("abc", "string");
AssertType("def", "string");

const c12 = 123 + 456;
AssertType(c12, "number");
AssertType(123 + 456, "number");
AssertType(123, "int");
AssertType(456, "int");

const c13 = Math.random() > 0.5 ? "abc" : "def";
AssertType(c13, "union");
AssertType(Math.random() > 0.5 ? "abc" : "def", "union");
AssertType(Math.random() > 0.5, "boolean");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(0.5, "double");
AssertType("abc", "string");
AssertType("def", "string");

const c14 = Math.random() > 0.5 ? 123 : 456;
AssertType(c14, "union");
AssertType(Math.random() > 0.5 ? 123 : 456, "union");
AssertType(Math.random() > 0.5, "boolean");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");
AssertType(0.5, "double");
AssertType(123, "int");
AssertType(456, "int");


