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

// === tests/cases/conformance/expressions/binaryOperators/logicalOrOperator/logicalOrExpressionIsNotContextuallyTyped.ts ===
declare function AssertType(value:any, type:string):void;
// The || operator permits the operands to be of any type.
// If the || expression is not contextually typed, the right operand is contextually typed
// by the type of the left operand and the result is of the best common type of the two
// operand types.


let a: (a: string) => string;
AssertType(a, "(string) => string");
AssertType(a, "string");

// bug 786110
let r = a || ((a) => a.toLowerCase());
AssertType(r, "(string) => string");
AssertType(a || ((a) => a.toLowerCase()), "(string) => string");
AssertType(a, "(string) => string");
AssertType(((a) => a.toLowerCase()), "(string) => string");
AssertType((a) => a.toLowerCase(), "(string) => string");
AssertType(a, "string");
AssertType(a.toLowerCase(), "string");
AssertType(a.toLowerCase, "() => string");


