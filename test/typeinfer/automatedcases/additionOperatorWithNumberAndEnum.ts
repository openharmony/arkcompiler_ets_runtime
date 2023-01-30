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

// === tests/cases/conformance/expressions/binaryOperators/additionOperator/additionOperatorWithNumberAndEnum.ts ===
declare function AssertType(value:any, type:string):void;
enum E { a, b }
enum F { c, d }

let a: number;
AssertType(a, "number");

let b: E;
AssertType(b, "E");

let c: E | F;
AssertType(c, "union");

let r1 = a + a;
AssertType(r1, "number");
AssertType(a + a, "number");
AssertType(a, "number");
AssertType(a, "number");

let r2 = a + b;
AssertType(r2, "number");
AssertType(a + b, "number");
AssertType(a, "number");
AssertType(b, "E");

let r3 = b + a;
AssertType(r3, "number");
AssertType(b + a, "number");
AssertType(b, "E");
AssertType(a, "number");

let r4 = b + b;
AssertType(r4, "number");
AssertType(b + b, "number");
AssertType(b, "E");
AssertType(b, "E");

let r5 = 0 + a;
AssertType(r5, "number");
AssertType(0 + a, "number");
AssertType(0, "int");
AssertType(a, "number");

let r6 = E.a + 0;
AssertType(r6, "number");
AssertType(E.a + 0, "number");
AssertType(E.a, "E.a");
AssertType(0, "int");

let r7 = E.a + E.b;
AssertType(r7, "number");
AssertType(E.a + E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let r8 = E['a'] + E['b'];
AssertType(r8, "number");
AssertType(E['a'] + E['b'], "number");
AssertType(E['a'], "E.a");
AssertType(E, "typeof E");
AssertType('a', "string");
AssertType(E['b'], "E.b");
AssertType(E, "typeof E");
AssertType('b', "string");

let r9 = E['a'] + F['c'];
AssertType(r9, "number");
AssertType(E['a'] + F['c'], "number");
AssertType(E['a'], "E.a");
AssertType(E, "typeof E");
AssertType('a', "string");
AssertType(F['c'], "F.c");
AssertType(F, "typeof F");
AssertType('c', "string");

let r10 = a + c;
AssertType(r10, "number");
AssertType(a + c, "number");
AssertType(a, "number");
AssertType(c, "union");

let r11 = c + a;
AssertType(r11, "number");
AssertType(c + a, "number");
AssertType(c, "union");
AssertType(a, "number");

let r12 = b + c;
AssertType(r12, "number");
AssertType(b + c, "number");
AssertType(b, "E");
AssertType(c, "union");

let r13 = c + b;
AssertType(r13, "number");
AssertType(c + b, "number");
AssertType(c, "union");
AssertType(b, "E");

let r14 = c + c;
AssertType(r14, "number");
AssertType(c + c, "number");
AssertType(c, "union");
AssertType(c, "union");


