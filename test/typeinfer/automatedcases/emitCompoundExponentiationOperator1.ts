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

// === tests/cases/conformance/es7/exponentiationOperator/emitCompoundExponentiationOperator1.ts ===
declare function AssertType(value:any, type:string):void;
let comp: number;
AssertType(comp, "number");

comp **= 1;
AssertType(comp **= 1, "number");
AssertType(comp, "number");
AssertType(1, "int");

comp **= comp ** comp;
AssertType(comp **= comp ** comp, "number");
AssertType(comp, "number");
AssertType(comp ** comp, "number");
AssertType(comp, "number");
AssertType(comp, "number");

comp **= comp ** comp ** 2;
AssertType(comp **= comp ** comp ** 2, "number");
AssertType(comp, "number");
AssertType(comp ** comp ** 2, "number");
AssertType(comp, "number");
AssertType(comp ** 2, "number");
AssertType(comp, "number");
AssertType(2, "int");

comp **= comp ** comp + 2;
AssertType(comp **= comp ** comp + 2, "number");
AssertType(comp, "number");
AssertType(comp ** comp + 2, "number");
AssertType(comp ** comp, "number");
AssertType(comp, "number");
AssertType(comp, "number");
AssertType(2, "int");

comp **= comp ** comp - 2;
AssertType(comp **= comp ** comp - 2, "number");
AssertType(comp, "number");
AssertType(comp ** comp - 2, "number");
AssertType(comp ** comp, "number");
AssertType(comp, "number");
AssertType(comp, "number");
AssertType(2, "int");

comp **= comp ** comp * 2;
AssertType(comp **= comp ** comp * 2, "number");
AssertType(comp, "number");
AssertType(comp ** comp * 2, "number");
AssertType(comp ** comp, "number");
AssertType(comp, "number");
AssertType(comp, "number");
AssertType(2, "int");

comp **= comp ** comp / 2;
AssertType(comp **= comp ** comp / 2, "number");
AssertType(comp, "number");
AssertType(comp ** comp / 2, "number");
AssertType(comp ** comp, "number");
AssertType(comp, "number");
AssertType(comp, "number");
AssertType(2, "int");

comp **= comp ** comp % 2;
AssertType(comp **= comp ** comp % 2, "number");
AssertType(comp, "number");
AssertType(comp ** comp % 2, "number");
AssertType(comp ** comp, "number");
AssertType(comp, "number");
AssertType(comp, "number");
AssertType(2, "int");

comp **= (comp - 2) ** 5;
AssertType(comp **= (comp - 2) ** 5, "number");
AssertType(comp, "number");
AssertType((comp - 2) ** 5, "number");
AssertType((comp - 2), "number");
AssertType(comp - 2, "number");
AssertType(comp, "number");
AssertType(2, "int");
AssertType(5, "int");

comp **= (comp + 2) ** 5;
AssertType(comp **= (comp + 2) ** 5, "number");
AssertType(comp, "number");
AssertType((comp + 2) ** 5, "number");
AssertType((comp + 2), "number");
AssertType(comp + 2, "number");
AssertType(comp, "number");
AssertType(2, "int");
AssertType(5, "int");

comp **= (comp * 2) ** 5;
AssertType(comp **= (comp * 2) ** 5, "number");
AssertType(comp, "number");
AssertType((comp * 2) ** 5, "number");
AssertType((comp * 2), "number");
AssertType(comp * 2, "number");
AssertType(comp, "number");
AssertType(2, "int");
AssertType(5, "int");

comp **= (comp / 2) ** 5;
AssertType(comp **= (comp / 2) ** 5, "number");
AssertType(comp, "number");
AssertType((comp / 2) ** 5, "number");
AssertType((comp / 2), "number");
AssertType(comp / 2, "number");
AssertType(comp, "number");
AssertType(2, "int");
AssertType(5, "int");

comp **= (comp % 2) ** 5;
AssertType(comp **= (comp % 2) ** 5, "number");
AssertType(comp, "number");
AssertType((comp % 2) ** 5, "number");
AssertType((comp % 2), "number");
AssertType(comp % 2, "number");
AssertType(comp, "number");
AssertType(2, "int");
AssertType(5, "int");

comp **= comp ** (5 + 2);
AssertType(comp **= comp ** (5 + 2), "number");
AssertType(comp, "number");
AssertType(comp ** (5 + 2), "number");
AssertType(comp, "number");
AssertType((5 + 2), "number");
AssertType(5 + 2, "number");
AssertType(5, "int");
AssertType(2, "int");

comp **= comp ** (5 - 2);
AssertType(comp **= comp ** (5 - 2), "number");
AssertType(comp, "number");
AssertType(comp ** (5 - 2), "number");
AssertType(comp, "number");
AssertType((5 - 2), "number");
AssertType(5 - 2, "number");
AssertType(5, "int");
AssertType(2, "int");

comp **= comp ** (5 * 2);
AssertType(comp **= comp ** (5 * 2), "number");
AssertType(comp, "number");
AssertType(comp ** (5 * 2), "number");
AssertType(comp, "number");
AssertType((5 * 2), "number");
AssertType(5 * 2, "number");
AssertType(5, "int");
AssertType(2, "int");

comp **= comp ** (5 / 2);
AssertType(comp **= comp ** (5 / 2), "number");
AssertType(comp, "number");
AssertType(comp ** (5 / 2), "number");
AssertType(comp, "number");
AssertType((5 / 2), "number");
AssertType(5 / 2, "number");
AssertType(5, "int");
AssertType(2, "int");

comp **= comp ** (5 % 2);
AssertType(comp **= comp ** (5 % 2), "number");
AssertType(comp, "number");
AssertType(comp ** (5 % 2), "number");
AssertType(comp, "number");
AssertType((5 % 2), "number");
AssertType(5 % 2, "number");
AssertType(5, "int");
AssertType(2, "int");


