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

// === tests/cases/conformance/expressions/asOperator/asOperator1.ts ===
declare function AssertType(value:any, type:string):void;
let as = 43;
AssertType(as, "number");
AssertType(43, "int");

let x = undefined as number;
AssertType(x, "number");
AssertType(undefined as number, "number");
AssertType(undefined, "undefined");

let y = (null as string).length;
AssertType(y, "number");
AssertType((null as string).length, "number");

let z = Date as any as string;
AssertType(z, "string");
AssertType(Date as any as string, "string");
AssertType(Date as any, "any");
AssertType(Date, "DateConstructor");

// Should parse as a union type, not a bitwise 'or' of (32 as number) and 'string'
let j = 32 as number|string;
AssertType(j, "union");
AssertType(32 as number|string, "union");
AssertType(32, "int");

j = '';
AssertType(j = '', "string");
AssertType(j, "union");
AssertType('', "string");


