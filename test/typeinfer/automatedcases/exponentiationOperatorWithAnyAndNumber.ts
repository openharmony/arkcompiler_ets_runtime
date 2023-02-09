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

// === tests/cases/conformance/es7/exponentiationOperator/exponentiationOperatorWithAnyAndNumber.ts ===
declare function AssertType(value:any, type:string):void;
let a: any;
AssertType(a, "any");

let b: number;
AssertType(b, "number");

// operator **
let r1 = a ** a;
AssertType(r1, "number");
AssertType(a ** a, "number");
AssertType(a, "any");
AssertType(a, "any");

let r2 = a ** b;
AssertType(r2, "number");
AssertType(a ** b, "number");
AssertType(a, "any");
AssertType(b, "number");

let r3 = a ** 0;
AssertType(r3, "number");
AssertType(a ** 0, "number");
AssertType(a, "any");
AssertType(0, "int");

let r4 = 0 ** a;
AssertType(r4, "number");
AssertType(0 ** a, "number");
AssertType(0, "int");
AssertType(a, "any");

let r5 = 0 ** 0;
AssertType(r5, "number");
AssertType(0 ** 0, "number");
AssertType(0, "int");
AssertType(0, "int");

let r6 = b ** 0;
AssertType(r6, "number");
AssertType(b ** 0, "number");
AssertType(b, "number");
AssertType(0, "int");

let r7 = 0 ** b;
AssertType(r7, "number");
AssertType(0 ** b, "number");
AssertType(0, "int");
AssertType(b, "number");

let r8 = b ** b;
AssertType(r8, "number");
AssertType(b ** b, "number");
AssertType(b, "number");
AssertType(b, "number");


