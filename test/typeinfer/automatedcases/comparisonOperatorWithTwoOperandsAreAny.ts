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

// === tests/cases/conformance/expressions/binaryOperators/comparisonOperator/comparisonOperatorWithTwoOperandsAreAny.ts ===
declare function AssertType(value:any, type:string):void;
let a: any;
AssertType(a, "any");

let r1 = a < a;
AssertType(r1, "boolean");
AssertType(a < a, "boolean");
AssertType(a, "any");
AssertType(a, "any");

let r2 = a > a;
AssertType(r2, "boolean");
AssertType(a > a, "boolean");
AssertType(a, "any");
AssertType(a, "any");

let r3 = a <= a;
AssertType(r3, "boolean");
AssertType(a <= a, "boolean");
AssertType(a, "any");
AssertType(a, "any");

let r4 = a >= a;
AssertType(r4, "boolean");
AssertType(a >= a, "boolean");
AssertType(a, "any");
AssertType(a, "any");

let r5 = a == a;
AssertType(r5, "boolean");
AssertType(a == a, "boolean");
AssertType(a, "any");
AssertType(a, "any");

let r6 = a != a;
AssertType(r6, "boolean");
AssertType(a != a, "boolean");
AssertType(a, "any");
AssertType(a, "any");

let r7 = a === a;
AssertType(r7, "boolean");
AssertType(a === a, "boolean");
AssertType(a, "any");
AssertType(a, "any");

let r8 = a !== a;
AssertType(r8, "boolean");
AssertType(a !== a, "boolean");
AssertType(a, "any");
AssertType(a, "any");


