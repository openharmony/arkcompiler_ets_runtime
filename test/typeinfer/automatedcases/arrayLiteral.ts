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

// === tests/cases/conformance/types/specifyingTypes/typeLiterals/arrayLiteral.ts ===
declare function AssertType(value:any, type:string):void;
// valid uses of array literals

let x = [];
AssertType(x, "any[]");
AssertType([], "undefined[]");

let x = new Array(1);
AssertType(x, "any[]");
AssertType(new Array(1), "any[]");
AssertType(Array, "ArrayConstructor");
AssertType(1, "int");

let y = [1];
AssertType(y, "number[]");
AssertType([1], "number[]");
AssertType(1, "int");

let y = [1, 2];
AssertType(y, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

let y = new Array<number>();
AssertType(y, "number[]");
AssertType(new Array<number>(), "number[]");
AssertType(Array, "ArrayConstructor");

let x2: number[] = [];
AssertType(x2, "number[]");
AssertType([], "undefined[]");

let x2: number[] = new Array(1);
AssertType(x2, "number[]");
AssertType(new Array(1), "any[]");
AssertType(Array, "ArrayConstructor");
AssertType(1, "int");

let y2: number[] = [1];
AssertType(y2, "number[]");
AssertType([1], "number[]");
AssertType(1, "int");

let y2: number[] = [1, 2];
AssertType(y2, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

let y2: number[] = new Array<number>();
AssertType(y2, "number[]");
AssertType(new Array<number>(), "number[]");
AssertType(Array, "ArrayConstructor");


