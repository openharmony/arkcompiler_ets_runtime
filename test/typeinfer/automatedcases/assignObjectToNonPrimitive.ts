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

// === tests/cases/conformance/types/nonPrimitive/assignObjectToNonPrimitive.ts ===
declare function AssertType(value:any, type:string):void;
let x = {};
AssertType(x, "{}");
AssertType({}, "{}");

let y = {foo: "bar"};
AssertType(y, "{ foo: string; }");
AssertType({foo: "bar"}, "{ foo: string; }");
AssertType(foo, "string");
AssertType("bar", "string");

let a: object;
AssertType(a, "object");

a = x;
AssertType(a = x, "{}");
AssertType(a, "object");
AssertType(x, "{}");

a = y;
AssertType(a = y, "{ foo: string; }");
AssertType(a, "object");
AssertType(y, "{ foo: string; }");


