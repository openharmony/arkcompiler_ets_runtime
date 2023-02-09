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

// === tests/cases/conformance/expressions/binaryOperators/additionOperator/additionOperatorWithStringAndEveryType.ts ===
declare function AssertType(value:any, type:string):void;
enum E { a, b, c }

let a: any;
AssertType(a, "any");

let b: boolean;
AssertType(b, "boolean");

let c: number;
AssertType(c, "number");

let d: string;
AssertType(d, "string");

let e: Object;
AssertType(e, "Object");

let f: void;
AssertType(f, "void");

let g: E;
AssertType(g, "E");

let x: string;
AssertType(x, "string");

// string could plus every type, and the result is always string
// string as left operand
let r1 = x + a;
AssertType(r1, "string");
AssertType(x + a, "string");
AssertType(x, "string");
AssertType(a, "any");

let r2 = x + b;
AssertType(r2, "string");
AssertType(x + b, "string");
AssertType(x, "string");
AssertType(b, "boolean");

let r3 = x + c;
AssertType(r3, "string");
AssertType(x + c, "string");
AssertType(x, "string");
AssertType(c, "number");

let r4 = x + d;
AssertType(r4, "string");
AssertType(x + d, "string");
AssertType(x, "string");
AssertType(d, "string");

let r5 = x + e;
AssertType(r5, "string");
AssertType(x + e, "string");
AssertType(x, "string");
AssertType(e, "Object");

let r6 = x + f;
AssertType(r6, "string");
AssertType(x + f, "string");
AssertType(x, "string");
AssertType(f, "void");

let r7 = x + g;
AssertType(r7, "string");
AssertType(x + g, "string");
AssertType(x, "string");
AssertType(g, "E");

// string as right operand
let r8 = a + x;
AssertType(r8, "string");
AssertType(a + x, "string");
AssertType(a, "any");
AssertType(x, "string");

let r9 = b + x;
AssertType(r9, "string");
AssertType(b + x, "string");
AssertType(b, "boolean");
AssertType(x, "string");

let r10 = c + x;
AssertType(r10, "string");
AssertType(c + x, "string");
AssertType(c, "number");
AssertType(x, "string");

let r11 = d + x;
AssertType(r11, "string");
AssertType(d + x, "string");
AssertType(d, "string");
AssertType(x, "string");

let r12 = e + x;
AssertType(r12, "string");
AssertType(e + x, "string");
AssertType(e, "Object");
AssertType(x, "string");

let r13 = f + x;
AssertType(r13, "string");
AssertType(f + x, "string");
AssertType(f, "void");
AssertType(x, "string");

let r14 = g + x;
AssertType(r14, "string");
AssertType(g + x, "string");
AssertType(g, "E");
AssertType(x, "string");

// other cases
let r15 = x + E;
AssertType(r15, "string");
AssertType(x + E, "string");
AssertType(x, "string");
AssertType(E, "typeof E");

let r16 = x + E.a;
AssertType(r16, "string");
AssertType(x + E.a, "string");
AssertType(x, "string");
AssertType(E.a, "E.a");

let r17 = x + '';
AssertType(r17, "string");
AssertType(x + '', "string");
AssertType(x, "string");
AssertType('', "string");

let r18 = x + 0;
AssertType(r18, "string");
AssertType(x + 0, "string");
AssertType(x, "string");
AssertType(0, "int");

let r19 = x + { a: '' };
AssertType(r19, "string");
AssertType(x + { a: '' }, "string");
AssertType(x, "string");
AssertType({ a: '' }, "{ a: string; }");
AssertType(a, "string");
AssertType('', "string");

let r20 = x + [];
AssertType(r20, "string");
AssertType(x + [], "string");
AssertType(x, "string");
AssertType([], "undefined[]");


