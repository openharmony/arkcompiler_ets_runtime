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

// === tests/cases/conformance/es6/destructuring/iterableArrayPattern27.ts ===
declare function AssertType(value:any, type:string):void;
function takeFirstTwoEntries(...[[k1, v1], [k2, v2]]: [string, number][]) { }
takeFirstTwoEntries(...new Map([["", 0], ["hello", 1]]));
AssertType(takeFirstTwoEntries(...new Map([["", 0], ["hello", 1]])), "void");
AssertType(takeFirstTwoEntries, "(...[string, number][]) => void");
AssertType(...new Map([["", 0], ["hello", 1]]), "[string, number]");
AssertType(new Map([["", 0], ["hello", 1]]), "Map<string, number>");
AssertType(Map, "MapConstructor");
AssertType([["", 0], ["hello", 1]], "[string, number][]");
AssertType(["", 0], "[string, number]");
AssertType("", "string");
AssertType(0, "int");
AssertType(["hello", 1], "[string, number]");
AssertType("hello", "string");
AssertType(1, "int");


