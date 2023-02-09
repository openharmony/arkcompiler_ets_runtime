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

// === tests/cases/conformance/expressions/binaryOperators/logicalAndOperator/logicalAndOperatorWithTypeParameters.ts ===
declare function AssertType(value:any, type:string):void;
// The && operator permits the operands to be of any type and produces a result of the same
// type as the second operand.

function foo<T, U, V/* extends T*/>(t: T, u: U, v: V) {
    let r1 = t && t;
AssertType(r1, "T");
AssertType(t && t, "T");
AssertType(t, "T");
AssertType(t, "T");

    let r2 = u && t;
AssertType(r2, "T");
AssertType(u && t, "T");
AssertType(u, "U");
AssertType(t, "T");

    let r3 = v && t;
AssertType(r3, "T");
AssertType(v && t, "T");
AssertType(v, "V");
AssertType(t, "T");

    let r4 = t && u;
AssertType(r4, "U");
AssertType(t && u, "U");
AssertType(t, "T");
AssertType(u, "U");

    let r5 = u && u;
AssertType(r5, "U");
AssertType(u && u, "U");
AssertType(u, "U");
AssertType(u, "U");

    let r6 = v && u;
AssertType(r6, "U");
AssertType(v && u, "U");
AssertType(v, "V");
AssertType(u, "U");

    let r7 = t && v;
AssertType(r7, "V");
AssertType(t && v, "V");
AssertType(t, "T");
AssertType(v, "V");

    let r8 = u && v;
AssertType(r8, "V");
AssertType(u && v, "V");
AssertType(u, "U");
AssertType(v, "V");

    let r9 = v && v;
AssertType(r9, "V");
AssertType(v && v, "V");
AssertType(v, "V");
AssertType(v, "V");

    let a: number;
AssertType(a, "number");

    let r10 = t && a;
AssertType(r10, "number");
AssertType(t && a, "number");
AssertType(t, "T");
AssertType(a, "number");
}

