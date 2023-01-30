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

// === tests/cases/conformance/es6/destructuring/iterableArrayPattern30.ts ===
declare function AssertType(value:any, type:string):void;
const [[k1, v1], [k2, v2]] = new Map([["", true], ["hello", true]])
AssertType(k1, "string");
AssertType(v1, "boolean");
AssertType(k2, "string");
AssertType(v2, "boolean");
AssertType(new Map([["", true], ["hello", true]]), "Map<string, boolean>");
AssertType(Map, "MapConstructor");
AssertType([["", true], ["hello", true]], "[string, true][]");
AssertType(["", true], "[string, true]");
AssertType("", "string");
AssertType(true, "boolean");
AssertType(["hello", true], "[string, true]");
AssertType("hello", "string");
AssertType(true, "boolean");


