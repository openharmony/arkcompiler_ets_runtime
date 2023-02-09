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

// === tests/cases/conformance/es6/classExpressions/classExpressionES63.ts ===
declare function AssertType(value:any, type:string):void;
let C = class extends class extends class { a = 1 } { b = 2 } { c = 3 };
AssertType(C, "typeof C");
AssertType(class extends class extends class { a = 1 } { b = 2 } { c = 3 }, "typeof C");
AssertType(class extends class { a = 1 } { b = 2 }, "(Anonymous class)");
AssertType(class { a = 1 }, "(Anonymous class)");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "number");
AssertType(2, "int");
AssertType(c, "number");
AssertType(3, "int");

let c = new C();
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

c.a;
AssertType(c.a, "number");

c.b;
AssertType(c.b, "number");

c.c;
AssertType(c.c, "number");


