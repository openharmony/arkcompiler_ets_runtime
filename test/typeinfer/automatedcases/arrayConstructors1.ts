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

// === tests/cases/compiler/arrayConstructors1.ts ===
declare function AssertType(value:any, type:string):void;
let x: string[];
AssertType(x, "string[]");

x = new Array(1);
AssertType(x = new Array(1), "any[]");
AssertType(x, "string[]");
AssertType(new Array(1), "any[]");
AssertType(Array, "ArrayConstructor");
AssertType(1, "int");

x = new Array('hi', 'bye'); 
AssertType(x = new Array('hi', 'bye'), "string[]");
AssertType(x, "string[]");
AssertType(new Array('hi', 'bye'), "string[]");
AssertType(Array, "ArrayConstructor");
AssertType('hi', "string");
AssertType('bye', "string");

x = new Array<string>('hi', 'bye');
AssertType(x = new Array<string>('hi', 'bye'), "string[]");
AssertType(x, "string[]");
AssertType(new Array<string>('hi', 'bye'), "string[]");
AssertType(Array, "ArrayConstructor");
AssertType('hi', "string");
AssertType('bye', "string");

let y: number[];
AssertType(y, "number[]");

y = new Array(1);
AssertType(y = new Array(1), "any[]");
AssertType(y, "number[]");
AssertType(new Array(1), "any[]");
AssertType(Array, "ArrayConstructor");
AssertType(1, "int");

y = new Array(1,2);
AssertType(y = new Array(1,2), "number[]");
AssertType(y, "number[]");
AssertType(new Array(1,2), "number[]");
AssertType(Array, "ArrayConstructor");
AssertType(1, "int");
AssertType(2, "int");

y = new Array<number>(1, 2);
AssertType(y = new Array<number>(1, 2), "number[]");
AssertType(y, "number[]");
AssertType(new Array<number>(1, 2), "number[]");
AssertType(Array, "ArrayConstructor");
AssertType(1, "int");
AssertType(2, "int");


