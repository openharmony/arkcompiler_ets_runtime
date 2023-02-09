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

// === tests/cases/compiler/json.stringify.ts ===
declare function AssertType(value:any, type:string):void;
let value = null;
AssertType(value, "null");
AssertType(null, "null");

JSON.stringify(value, undefined, 2);
AssertType(JSON.stringify(value, undefined, 2), "string");
AssertType(JSON.stringify, "{ (any, ?union, ?union): string; (any, ?union, ?union): string; }");
AssertType(value, "null");
AssertType(undefined, "undefined");
AssertType(2, "int");

JSON.stringify(value, null, 2);
AssertType(JSON.stringify(value, null, 2), "string");
AssertType(JSON.stringify, "{ (any, ?union, ?union): string; (any, ?union, ?union): string; }");
AssertType(value, "null");
AssertType(null, "null");
AssertType(2, "int");

JSON.stringify(value, ["a", 1], 2);
AssertType(JSON.stringify(value, ["a", 1], 2), "string");
AssertType(JSON.stringify, "{ (any, ?union, ?union): string; (any, ?union, ?union): string; }");
AssertType(value, "null");
AssertType(["a", 1], "(union)[]");
AssertType("a", "string");
AssertType(1, "int");
AssertType(2, "int");

JSON.stringify(value, (k) => undefined, 2);
AssertType(JSON.stringify(value, (k) => undefined, 2), "string");
AssertType(JSON.stringify, "{ (any, ?union, ?union): string; (any, ?union, ?union): string; }");
AssertType(value, "null");
AssertType((k) => undefined, "(any, string) => undefined");
AssertType(k, "string");
AssertType(undefined, "undefined");
AssertType(2, "int");

JSON.stringify(value, undefined, 2);
AssertType(JSON.stringify(value, undefined, 2), "string");
AssertType(JSON.stringify, "{ (any, ?union, ?union): string; (any, ?union, ?union): string; }");
AssertType(value, "null");
AssertType(undefined, "undefined");
AssertType(2, "int");


