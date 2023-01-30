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

// === tests/cases/conformance/es6/templates/taggedTemplateStringsWithOverloadResolution2_ES6.ts ===
declare function AssertType(value:any, type:string):void;
function foo1(strs: TemplateStringsArray, x: number): string;
function foo1(strs: string[], x: number): number;
function foo1(...stuff: any[]): any {
AssertType(undefined, "undefined");
    return undefined;
}

let a = foo1 `${1}`;
AssertType(a, "string");
AssertType(foo1 `${1}`, "string");
AssertType(foo1, "{ (TemplateStringsArray, number): string; (string[], number): number; }");
AssertType(`${1}`, "string");
AssertType(1, "int");

let b = foo1([], 1);
AssertType(b, "number");
AssertType(foo1([], 1), "number");
AssertType(foo1, "{ (TemplateStringsArray, number): string; (string[], number): number; }");
AssertType([], "undefined[]");
AssertType(1, "int");

function foo2(strs: string[], x: number): number;
function foo2(strs: TemplateStringsArray, x: number): string;
function foo2(...stuff: any[]): any {
AssertType(undefined, "undefined");
    return undefined;
}

let c = foo2 `${1}`;
AssertType(c, "string");
AssertType(foo2 `${1}`, "string");
AssertType(foo2, "{ (string[], number): number; (TemplateStringsArray, number): string; }");
AssertType(`${1}`, "string");
AssertType(1, "int");

let d = foo2([], 1);
AssertType(d, "number");
AssertType(foo2([], 1), "number");
AssertType(foo2, "{ (string[], number): number; (TemplateStringsArray, number): string; }");
AssertType([], "undefined[]");
AssertType(1, "int");


