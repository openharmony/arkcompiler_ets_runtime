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

// === tests/cases/compiler/trailingCommasES3.ts ===
declare function AssertType(value:any, type:string):void;
let o1 = { a: 1, b: 2 };
AssertType(o1, "{ a: number; b: number; }");
AssertType({ a: 1, b: 2 }, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "number");
AssertType(2, "int");

let o2 = { a: 1, b: 2, };
AssertType(o2, "{ a: number; b: number; }");
AssertType({ a: 1, b: 2, }, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "number");
AssertType(2, "int");

let o3 = { a: 1, };
AssertType(o3, "{ a: number; }");
AssertType({ a: 1, }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

let o4 = {};
AssertType(o4, "{}");
AssertType({}, "{}");

let a1 = [1, 2];
AssertType(a1, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

let a2 = [1, 2, ];
AssertType(a2, "number[]");
AssertType([1, 2, ], "number[]");
AssertType(1, "int");
AssertType(2, "int");

let a3 = [1, ];
AssertType(a3, "number[]");
AssertType([1, ], "number[]");
AssertType(1, "int");

let a4 = [];
AssertType(a4, "any[]");
AssertType([], "undefined[]");

let a5 = [1, , ];
AssertType(a5, "number[]");
AssertType([1, , ], "number[]");
AssertType(1, "int");
AssertType(, "undefined");

let a6 = [, , ];
AssertType(a6, "any[]");
AssertType([, , ], "undefined[]");
AssertType(, "undefined");
AssertType(, "undefined");


