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

// === tests/cases/conformance/types/specifyingTypes/typeLiterals/functionLiteralForOverloads.ts ===
declare function AssertType(value:any, type:string):void;
// basic uses of function literals with overloads

let f: {
AssertType(f, "{ (string): string; (number): number; }");

    (x: string): string;
AssertType(x, "string");

    (x: number): number;
AssertType(x, "number");

} = (x) => x;
AssertType((x) => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");

let f2: {
AssertType(f2, "{ <T>(string): string; <T>(number): number; }");

    <T>(x: string): string;
AssertType(x, "string");

    <T>(x: number): number;
AssertType(x, "number");

} = (x) => x;
AssertType((x) => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");

let f3: {
AssertType(f3, "{ <T>(T): string; <T>(T): number; }");

    <T>(x: T): string;
AssertType(x, "T");

    <T>(x: T): number;
AssertType(x, "T");

} = (x) => x;
AssertType((x) => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");

let f4: {
AssertType(f4, "{ <T>(string): T; <T>(number): T; }");

    <T>(x: string): T;
AssertType(x, "string");

    <T>(x: number): T;
AssertType(x, "number");

} = (x) => x;
AssertType((x) => x, "(any) => any");
AssertType(x, "any");
AssertType(x, "any");


