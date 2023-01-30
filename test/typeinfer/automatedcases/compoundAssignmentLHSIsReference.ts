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

// === tests/cases/conformance/expressions/assignmentOperator/compoundAssignmentLHSIsReference.ts ===
declare function AssertType(value:any, type:string):void;
let value;
AssertType(value, "any");

// identifiers: letiable and parameter
let x1: number;
AssertType(x1, "number");

x1 *= value;
AssertType(x1 *= value, "number");
AssertType(x1, "number");
AssertType(value, "any");

x1 += value;
AssertType(x1 += value, "any");
AssertType(x1, "number");
AssertType(value, "any");

function fn1(x2: number) {
    x2 *= value;
AssertType(x2 *= value, "number");
AssertType(x2, "number");
AssertType(value, "any");

    x2 += value;
AssertType(x2 += value, "any");
AssertType(x2, "number");
AssertType(value, "any");
}

// property accesses
let x3: { a: number };
AssertType(x3, "{ a: number; }");
AssertType(a, "number");

x3.a *= value;
AssertType(x3.a *= value, "number");
AssertType(x3.a, "number");
AssertType(value, "any");

x3.a += value;
AssertType(x3.a += value, "any");
AssertType(x3.a, "number");
AssertType(value, "any");

x3['a'] *= value;
AssertType(x3['a'] *= value, "number");
AssertType(x3['a'], "number");
AssertType(x3, "{ a: number; }");
AssertType('a', "string");
AssertType(value, "any");

x3['a'] += value;
AssertType(x3['a'] += value, "any");
AssertType(x3['a'], "number");
AssertType(x3, "{ a: number; }");
AssertType('a', "string");
AssertType(value, "any");

// parentheses, the contained expression is reference
(x1) *= value;
AssertType((x1) *= value, "number");
AssertType((x1), "number");
AssertType(x1, "number");
AssertType(value, "any");

(x1) += value;
AssertType((x1) += value, "any");
AssertType((x1), "number");
AssertType(x1, "number");
AssertType(value, "any");

function fn2(x4: number) {
    (x4) *= value;
AssertType((x4) *= value, "number");
AssertType((x4), "number");
AssertType(x4, "number");
AssertType(value, "any");

    (x4) += value;
AssertType((x4) += value, "any");
AssertType((x4), "number");
AssertType(x4, "number");
AssertType(value, "any");
}

(x3.a) *= value;
AssertType((x3.a) *= value, "number");
AssertType((x3.a), "number");
AssertType(x3.a, "number");
AssertType(value, "any");

(x3.a) += value;
AssertType((x3.a) += value, "any");
AssertType((x3.a), "number");
AssertType(x3.a, "number");
AssertType(value, "any");

(x3['a']) *= value;
AssertType((x3['a']) *= value, "number");
AssertType((x3['a']), "number");
AssertType(x3['a'], "number");
AssertType(x3, "{ a: number; }");
AssertType('a', "string");
AssertType(value, "any");

(x3['a']) += value;
AssertType((x3['a']) += value, "any");
AssertType((x3['a']), "number");
AssertType(x3['a'], "number");
AssertType(x3, "{ a: number; }");
AssertType('a', "string");
AssertType(value, "any");


