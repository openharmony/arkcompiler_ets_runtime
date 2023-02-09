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

// === tests/cases/conformance/expressions/asOperator/asOperator3.ts ===
declare function AssertType(value:any, type:string):void;
declare function tag(...x: any[]): any;

let a = `${123 + 456 as number}`;
AssertType(a, "string");
AssertType(`${123 + 456 as number}`, "string");
AssertType(123 + 456 as number, "number");
AssertType(123 + 456, "number");
AssertType(123, "int");
AssertType(456, "int");

let b = `leading ${123 + 456 as number}`;
AssertType(b, "string");
AssertType(`leading ${123 + 456 as number}`, "string");
AssertType(123 + 456 as number, "number");
AssertType(123 + 456, "number");
AssertType(123, "int");
AssertType(456, "int");

let c = `${123 + 456 as number} trailing`;
AssertType(c, "string");
AssertType(`${123 + 456 as number} trailing`, "string");
AssertType(123 + 456 as number, "number");
AssertType(123 + 456, "number");
AssertType(123, "int");
AssertType(456, "int");

let d = `Hello ${123} World` as string;
AssertType(d, "string");
AssertType(`Hello ${123} World` as string, "string");
AssertType(`Hello ${123} World`, "string");
AssertType(123, "int");

let e = `Hello` as string;
AssertType(e, "string");
AssertType(`Hello` as string, "string");
AssertType(`Hello`, "string");

let f = 1 + `${1} end of string` as string;
AssertType(f, "string");
AssertType(1 + `${1} end of string` as string, "string");
AssertType(1 + `${1} end of string`, "string");
AssertType(1, "int");
AssertType(`${1} end of string`, "string");
AssertType(1, "int");

let g = tag `Hello ${123} World` as string;
AssertType(g, "string");
AssertType(tag `Hello ${123} World` as string, "string");
AssertType(tag `Hello ${123} World`, "any");
AssertType(tag, "(...any[]) => any");
AssertType(`Hello ${123} World`, "string");
AssertType(123, "int");

let h = tag `Hello` as string;
AssertType(h, "string");
AssertType(tag `Hello` as string, "string");
AssertType(tag `Hello`, "any");
AssertType(tag, "(...any[]) => any");
AssertType(`Hello`, "string");


