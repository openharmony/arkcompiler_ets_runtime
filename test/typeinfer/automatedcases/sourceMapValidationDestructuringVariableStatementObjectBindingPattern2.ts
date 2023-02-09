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

// === tests/cases/compiler/sourceMapValidationDestructuringVariableStatementObjectBindingPattern2.ts ===
declare function AssertType(value:any, type:string):void;
let {x} = { x: 20 };
AssertType(x, "number");
AssertType({ x: 20 }, "{ x: number; }");
AssertType(x, "number");
AssertType(20, "int");

let { a, b } = { a: 30, b: 40 };
AssertType(a, "number");
AssertType(b, "number");
AssertType({ a: 30, b: 40 }, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(30, "int");
AssertType(b, "number");
AssertType(40, "int");


