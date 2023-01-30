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

// === tests/cases/conformance/es7/exponentiationOperator/emitCompoundExponentiationOperator2.ts ===
declare function AssertType(value:any, type:string):void;
let comp: number;
AssertType(comp, "number");

comp **= 1;
AssertType(comp **= 1, "number");
AssertType(comp, "number");
AssertType(1, "int");

comp **= comp **= 1;
AssertType(comp **= comp **= 1, "number");
AssertType(comp, "number");
AssertType(comp **= 1, "number");
AssertType(comp, "number");
AssertType(1, "int");

comp **= comp **= 1 + 2;
AssertType(comp **= comp **= 1 + 2, "number");
AssertType(comp, "number");
AssertType(comp **= 1 + 2, "number");
AssertType(comp, "number");
AssertType(1 + 2, "number");
AssertType(1, "int");
AssertType(2, "int");

comp **= comp **= 1 - 2;
AssertType(comp **= comp **= 1 - 2, "number");
AssertType(comp, "number");
AssertType(comp **= 1 - 2, "number");
AssertType(comp, "number");
AssertType(1 - 2, "number");
AssertType(1, "int");
AssertType(2, "int");

comp **= comp **= 1 * 2;
AssertType(comp **= comp **= 1 * 2, "number");
AssertType(comp, "number");
AssertType(comp **= 1 * 2, "number");
AssertType(comp, "number");
AssertType(1 * 2, "number");
AssertType(1, "int");
AssertType(2, "int");

comp **= comp **= 1 / 2;
AssertType(comp **= comp **= 1 / 2, "number");
AssertType(comp, "number");
AssertType(comp **= 1 / 2, "number");
AssertType(comp, "number");
AssertType(1 / 2, "number");
AssertType(1, "int");
AssertType(2, "int");

comp **= comp **= (1 + 2);
AssertType(comp **= comp **= (1 + 2), "number");
AssertType(comp, "number");
AssertType(comp **= (1 + 2), "number");
AssertType(comp, "number");
AssertType((1 + 2), "number");
AssertType(1 + 2, "number");
AssertType(1, "int");
AssertType(2, "int");

comp **= comp **= (1 - 2);
AssertType(comp **= comp **= (1 - 2), "number");
AssertType(comp, "number");
AssertType(comp **= (1 - 2), "number");
AssertType(comp, "number");
AssertType((1 - 2), "number");
AssertType(1 - 2, "number");
AssertType(1, "int");
AssertType(2, "int");

comp **= comp **= (1 * 2);
AssertType(comp **= comp **= (1 * 2), "number");
AssertType(comp, "number");
AssertType(comp **= (1 * 2), "number");
AssertType(comp, "number");
AssertType((1 * 2), "number");
AssertType(1 * 2, "number");
AssertType(1, "int");
AssertType(2, "int");

comp **= comp **= (1 / 2);
AssertType(comp **= comp **= (1 / 2), "number");
AssertType(comp, "number");
AssertType(comp **= (1 / 2), "number");
AssertType(comp, "number");
AssertType((1 / 2), "number");
AssertType(1 / 2, "number");
AssertType(1, "int");
AssertType(2, "int");

comp **= comp **= 1 + 2 ** 3;
AssertType(comp **= comp **= 1 + 2 ** 3, "number");
AssertType(comp, "number");
AssertType(comp **= 1 + 2 ** 3, "number");
AssertType(comp, "number");
AssertType(1 + 2 ** 3, "number");
AssertType(1, "int");
AssertType(2 ** 3, "number");
AssertType(2, "int");
AssertType(3, "int");

comp **= comp **= 1 - 2 ** 4;
AssertType(comp **= comp **= 1 - 2 ** 4, "number");
AssertType(comp, "number");
AssertType(comp **= 1 - 2 ** 4, "number");
AssertType(comp, "number");
AssertType(1 - 2 ** 4, "number");
AssertType(1, "int");
AssertType(2 ** 4, "number");
AssertType(2, "int");
AssertType(4, "int");

comp **= comp **= 1 * 2 ** 5;
AssertType(comp **= comp **= 1 * 2 ** 5, "number");
AssertType(comp, "number");
AssertType(comp **= 1 * 2 ** 5, "number");
AssertType(comp, "number");
AssertType(1 * 2 ** 5, "number");
AssertType(1, "int");
AssertType(2 ** 5, "number");
AssertType(2, "int");
AssertType(5, "int");

comp **= comp **= 1 / 2 ** 6;
AssertType(comp **= comp **= 1 / 2 ** 6, "number");
AssertType(comp, "number");
AssertType(comp **= 1 / 2 ** 6, "number");
AssertType(comp, "number");
AssertType(1 / 2 ** 6, "number");
AssertType(1, "int");
AssertType(2 ** 6, "number");
AssertType(2, "int");
AssertType(6, "int");

comp **= comp **= (1 + 2) ** 3;
AssertType(comp **= comp **= (1 + 2) ** 3, "number");
AssertType(comp, "number");
AssertType(comp **= (1 + 2) ** 3, "number");
AssertType(comp, "number");
AssertType((1 + 2) ** 3, "number");
AssertType((1 + 2), "number");
AssertType(1 + 2, "number");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

comp **= comp **= (1 - 2) ** 4;
AssertType(comp **= comp **= (1 - 2) ** 4, "number");
AssertType(comp, "number");
AssertType(comp **= (1 - 2) ** 4, "number");
AssertType(comp, "number");
AssertType((1 - 2) ** 4, "number");
AssertType((1 - 2), "number");
AssertType(1 - 2, "number");
AssertType(1, "int");
AssertType(2, "int");
AssertType(4, "int");

comp **= comp **= (1 * 2) ** 5;
AssertType(comp **= comp **= (1 * 2) ** 5, "number");
AssertType(comp, "number");
AssertType(comp **= (1 * 2) ** 5, "number");
AssertType(comp, "number");
AssertType((1 * 2) ** 5, "number");
AssertType((1 * 2), "number");
AssertType(1 * 2, "number");
AssertType(1, "int");
AssertType(2, "int");
AssertType(5, "int");

comp **= comp **= (1 / 2) ** 6;
AssertType(comp **= comp **= (1 / 2) ** 6, "number");
AssertType(comp, "number");
AssertType(comp **= (1 / 2) ** 6, "number");
AssertType(comp, "number");
AssertType((1 / 2) ** 6, "number");
AssertType((1 / 2), "number");
AssertType(1 / 2, "number");
AssertType(1, "int");
AssertType(2, "int");
AssertType(6, "int");


