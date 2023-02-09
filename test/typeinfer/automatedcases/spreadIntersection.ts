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

// === tests/cases/compiler/spreadIntersection.ts ===
declare function AssertType(value:any, type:string):void;
let intersection: { a: number } & { b: string };
AssertType(intersection, "{ a: number; } & { b: string; }");
AssertType(a, "number");
AssertType(b, "string");

let o1: { a: number, b: string };
AssertType(o1, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(b, "string");

let o1 = { ...intersection };
AssertType(o1, "{ a: number; b: string; }");
AssertType({ ...intersection }, "{ a: number; b: string; }");
AssertType(intersection, "{ a: number; } & { b: string; }");

let o2: { a: number, b: string, c: boolean };
AssertType(o2, "{ a: number; b: string; c: boolean; }");
AssertType(a, "number");
AssertType(b, "string");
AssertType(c, "boolean");

let o2 = { ...intersection, c: false };
AssertType(o2, "{ a: number; b: string; c: boolean; }");
AssertType({ ...intersection, c: false }, "{ c: boolean; a: number; b: string; }");
AssertType(intersection, "{ a: number; } & { b: string; }");
AssertType(c, "boolean");
AssertType(false, "boolean");


