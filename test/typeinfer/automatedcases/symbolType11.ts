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

// === tests/cases/conformance/es6/Symbols/symbolType11.ts ===
declare function AssertType(value:any, type:string):void;
let s = Symbol.for("logical");
AssertType(s, "symbol");
AssertType(Symbol.for("logical"), "symbol");
AssertType(Symbol.for, "(string) => symbol");
AssertType("logical", "string");

s && s;
AssertType(s && s, "symbol");
AssertType(s, "symbol");
AssertType(s, "symbol");

s && [];
AssertType(s && [], "undefined[]");
AssertType(s, "symbol");
AssertType([], "undefined[]");

0 && s;
AssertType(0 && s, "int");
AssertType(0, "int");
AssertType(s, "symbol");

s || s;
AssertType(s || s, "symbol");
AssertType(s, "symbol");
AssertType(s, "symbol");

s || 1;
AssertType(s || 1, "union");
AssertType(s, "symbol");
AssertType(1, "int");

({}) || s;
AssertType(({}) || s, "union");
AssertType(({}), "{}");
AssertType({}, "{}");
AssertType(s, "symbol");


