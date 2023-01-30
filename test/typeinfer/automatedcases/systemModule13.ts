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

// === tests/cases/compiler/systemModule13.ts ===
declare function AssertType(value:any, type:string):void;
export let [x,y,z] = [1, 2, 3];
AssertType(x, "number");
AssertType(y, "number");
AssertType(z, "number");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

export const {a: z0, b: {c: z1}} = {a: true, b: {c: "123"}};
AssertType(a, "any");
AssertType(z0, "boolean");
AssertType(b, "any");
AssertType(c, "any");
AssertType(z1, "string");
AssertType({a: true, b: {c: "123"}}, "{ a: boolean; b: { c: string; }; }");
AssertType(a, "boolean");
AssertType(true, "boolean");
AssertType(b, "{ c: string; }");
AssertType({c: "123"}, "{ c: string; }");
AssertType(c, "string");
AssertType("123", "string");

for ([x] of [[1]]) {}

