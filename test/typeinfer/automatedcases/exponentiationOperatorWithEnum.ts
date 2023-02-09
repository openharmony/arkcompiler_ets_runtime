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

// === tests/cases/conformance/es7/exponentiationOperator/exponentiationOperatorWithEnum.ts ===
declare function AssertType(value:any, type:string):void;
// operands of an enum type are treated as having the primitive type Number.

enum E {
    a,
    b
}

let a: any;
AssertType(a, "any");

let b: number;
AssertType(b, "number");

let c: E;
AssertType(c, "E");

// operator **
let r1 = c ** a;
AssertType(r1, "number");
AssertType(c ** a, "number");
AssertType(c, "E");
AssertType(a, "any");

let r2 = c ** b;
AssertType(r2, "number");
AssertType(c ** b, "number");
AssertType(c, "E");
AssertType(b, "number");

let r3 = c ** c;
AssertType(r3, "number");
AssertType(c ** c, "number");
AssertType(c, "E");
AssertType(c, "E");

let r4 = a ** c;
AssertType(r4, "number");
AssertType(a ** c, "number");
AssertType(a, "any");
AssertType(c, "E");

let r5 = b ** c;
AssertType(r5, "number");
AssertType(b ** c, "number");
AssertType(b, "number");
AssertType(c, "E");

let r6 = E.a ** a;
AssertType(r6, "number");
AssertType(E.a ** a, "number");
AssertType(E.a, "E.a");
AssertType(a, "any");

let r7 = E.a ** b;
AssertType(r7, "number");
AssertType(E.a ** b, "number");
AssertType(E.a, "E.a");
AssertType(b, "number");

let r8 = E.a ** E.b;
AssertType(r8, "number");
AssertType(E.a ** E.b, "number");
AssertType(E.a, "E.a");
AssertType(E.b, "E.b");

let r9 = E.a ** 1;
AssertType(r9, "number");
AssertType(E.a ** 1, "number");
AssertType(E.a, "E.a");
AssertType(1, "int");

let r10 = a ** E.b;
AssertType(r10, "number");
AssertType(a ** E.b, "number");
AssertType(a, "any");
AssertType(E.b, "E.b");

let r11 = b ** E.b;
AssertType(r11, "number");
AssertType(b ** E.b, "number");
AssertType(b, "number");
AssertType(E.b, "E.b");

let r12 = 1 ** E.b;
AssertType(r12, "number");
AssertType(1 ** E.b, "number");
AssertType(1, "int");
AssertType(E.b, "E.b");


