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

// === tests/cases/compiler/indexSignaturesInferentialTyping.ts ===
declare function AssertType(value:any, type:string):void;
function foo<T>(items: { [index: number]: T }): T { 
AssertType(undefined, "undefined");
return undefined; 
}

function bar<T>(items: { [index: string]: T }): T { 
AssertType(undefined, "undefined");
return undefined; 
}

let x1 = foo({ 0: 0, 1: 1 });       // type should be number
AssertType(x1, "number");
AssertType(foo({ 0: 0, 1: 1 }), "number");
AssertType(foo, "<T>({ [number]: T; }) => T");
AssertType({ 0: 0, 1: 1 }, "{ 0: number; 1: number; }");
AssertType(0, "number");
AssertType(0, "int");
AssertType(1, "number");
AssertType(1, "int");

let x2 = bar({ 0: 0, 1: 1 });
AssertType(x2, "number");
AssertType(bar({ 0: 0, 1: 1 }), "number");
AssertType(bar, "<T>({ [string]: T; }) => T");
AssertType({ 0: 0, 1: 1 }, "{ 0: number; 1: number; }");
AssertType(0, "number");
AssertType(0, "int");
AssertType(1, "number");
AssertType(1, "int");

let x3 = bar({ zero: 0, one: 1 });  // type should be number
AssertType(x3, "number");
AssertType(bar({ zero: 0, one: 1 }), "number");
AssertType(bar, "<T>({ [string]: T; }) => T");
AssertType({ zero: 0, one: 1 }, "{ zero: number; one: number; }");
AssertType(zero, "number");
AssertType(0, "int");
AssertType(one, "number");
AssertType(1, "int");


