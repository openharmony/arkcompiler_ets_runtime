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

// === tests/cases/compiler/regExpWithSlashInCharClass.ts ===
declare function AssertType(value:any, type:string):void;
let foo1 = "a/".replace(/.[/]/, "");
AssertType(foo1, "string");
AssertType("a/".replace(/.[/]/, ""), "string");
AssertType("a/".replace, "{ (union, string): string; (union, (string, ...any[]) => string): string; }");
AssertType(/.[/]/, "RegExp");
AssertType("", "string");

let foo2 = "a//".replace(/.[//]/g, "");
AssertType(foo2, "string");
AssertType("a//".replace(/.[//]/g, ""), "string");
AssertType("a//".replace, "{ (union, string): string; (union, (string, ...any[]) => string): string; }");
AssertType(/.[//]/g, "RegExp");
AssertType("", "string");

let foo3 = "a/".replace(/.[/no sleep /till/]/, "bugfix");
AssertType(foo3, "string");
AssertType("a/".replace(/.[/no sleep /till/]/, "bugfix"), "string");
AssertType("a/".replace, "{ (union, string): string; (union, (string, ...any[]) => string): string; }");
AssertType(/.[/no sleep /till/]/, "RegExp");
AssertType("bugfix", "string");


