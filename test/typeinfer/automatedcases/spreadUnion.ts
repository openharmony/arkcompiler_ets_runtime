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

// === tests/cases/conformance/types/spread/spreadUnion.ts ===
declare function AssertType(value:any, type:string):void;
let union: { a: number } | { b: string };
AssertType(union, "union");
AssertType(a, "number");
AssertType(b, "string");

let o3: { a: number } | { b: string };
AssertType(o3, "union");
AssertType(a, "number");
AssertType(b, "string");

let o3 =  { ...union };
AssertType(o3, "union");
AssertType({ ...union }, "union");
AssertType(union, "union");

let o4: { a: boolean } | { b: string , a: boolean};
AssertType(o4, "union");
AssertType(a, "boolean");
AssertType(b, "string");
AssertType(a, "boolean");

let o4 =  { ...union, a: false };
AssertType(o4, "union");
AssertType({ ...union, a: false }, "union");
AssertType(union, "union");
AssertType(a, "boolean");
AssertType(false, "boolean");

let o5: { a: number } | { b: string } | { a: number, b: string };
AssertType(o5, "union");
AssertType(a, "number");
AssertType(b, "string");
AssertType(a, "number");
AssertType(b, "string");

let o5 =  { ...union, ...union };
AssertType(o5, "union");
AssertType({ ...union, ...union }, "union");
AssertType(union, "union");
AssertType(union, "union");


