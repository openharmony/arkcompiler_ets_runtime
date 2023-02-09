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

// === tests/cases/compiler/contextualTypingOfObjectLiterals.ts ===
declare function AssertType(value:any, type:string):void;
let obj1: { [x: string]: string; };
AssertType(obj1, "{ [string]: string; }");
AssertType(x, "string");

let obj2 = {x: ""};
AssertType(obj2, "{ x: string; }");
AssertType({x: ""}, "{ x: string; }");
AssertType(x, "string");
AssertType("", "string");

obj1 = {}; // Ok
AssertType(obj1 = {}, "{}");
AssertType(obj1, "{ [string]: string; }");
AssertType({}, "{}");

obj1 = obj2; // Error - indexer doesn't match
AssertType(obj1 = obj2, "{ x: string; }");
AssertType(obj1, "{ [string]: string; }");
AssertType(obj2, "{ x: string; }");

function f(x: { [s: string]: string }) { }

f({}); // Ok
AssertType(f({}), "void");
AssertType(f, "({ [string]: string; }) => void");
AssertType({}, "{}");

f(obj1); // Ok
AssertType(f(obj1), "void");
AssertType(f, "({ [string]: string; }) => void");
AssertType(obj1, "{ [string]: string; }");

f(obj2); // Error - indexer doesn't match
AssertType(f(obj2), "void");
AssertType(f, "({ [string]: string; }) => void");
AssertType(obj2, "{ x: string; }");


