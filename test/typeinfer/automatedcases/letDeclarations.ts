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

// === tests/cases/compiler/letDeclarations.ts ===
declare function AssertType(value:any, type:string):void;
let l1;
AssertType(l1, "any");

let l2: number;
AssertType(l2, "number");

let l3, l4, l5 :string, l6;
AssertType(l3, "any");
AssertType(l4, "any");
AssertType(l5, "string");
AssertType(l6, "any");

let l7 = false;
AssertType(l7, "boolean");
AssertType(false, "boolean");

let l8: number = 23;
AssertType(l8, "number");
AssertType(23, "int");

let l9 = 0, l10 :string = "", l11 = null;
AssertType(l9, "number");
AssertType(0, "int");
AssertType(l10, "string");
AssertType("", "string");
AssertType(l11, "any");
AssertType(null, "null");

for(let l11 in {}) { }

for(let l12 = 0; l12 < 9; l12++) { }


