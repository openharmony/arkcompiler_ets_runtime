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

// === tests/cases/compiler/es2018ObjectAssign.ts ===
declare function AssertType(value:any, type:string):void;
const test = Object.assign({}, { test: true });
AssertType(test, "{ test: boolean; }");
AssertType(Object.assign({}, { test: true }), "{ test: boolean; }");
AssertType(Object.assign, "{ <T extends {}, U>(T, U): T & U; <T extends {}, U, V>(T, U, V): T & U & V; <T extends {}, U, V, W>(T, U, V, W): T & U & V & W; (object, ...any[]): any; }");
AssertType({}, "{}");
AssertType({ test: true }, "{ test: true; }");
AssertType(test, "boolean");
AssertType(true, "boolean");

declare const p: Promise<number>;
AssertType(p, "Promise<number>");

p.finally();
AssertType(p.finally(), "Promise<number>");
AssertType(p.finally, "(?() => void) => Promise<number>");


