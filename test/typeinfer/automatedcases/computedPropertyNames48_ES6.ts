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

// === tests/cases/conformance/es6/computedProperties/computedPropertyNames48_ES6.ts ===
declare function AssertType(value:any, type:string):void;
declare function extractIndexer<T>(p: { [n: number]: T }): T;

enum E { x }

let a: any;
AssertType(a, "any");

extractIndexer({
AssertType(extractIndexer({    [a]: ""}), "string");
AssertType(extractIndexer, "<T>({ [number]: T; }) => T");
AssertType({    [a]: ""}, "{ [number]: string; }");

    [a]: ""
AssertType([a], "string");
AssertType(a, "any");
AssertType("", "string");

}); // Should 
return string

extractIndexer({
AssertType(extractIndexer({    [E.x]: ""}), "string");
AssertType(extractIndexer, "<T>({ [number]: T; }) => T");
AssertType({    [E.x]: ""}, "{ 0: string; }");

    [E.x]: ""
AssertType([E.x], "string");
AssertType(E.x, "E");
AssertType("", "string");

}); // Should 
return string

extractIndexer({
AssertType(extractIndexer({    ["" || 0]: ""}), "string");
AssertType(extractIndexer, "<T>({ [number]: T; }) => T");
AssertType({    ["" || 0]: ""}, "{ 0: string; }");

    ["" || 0]: ""
AssertType(["" || 0], "string");
AssertType("" || 0, "int");
AssertType("", "string");
AssertType(0, "int");
AssertType("", "string");

}); // Should return any (widened form of undefined)

