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

// === tests/cases/compiler/overloadResolutionWithAny.ts ===
declare function AssertType(value:any, type:string):void;
let func: {
AssertType(func, "{ (string): number; (any): string; }");

    (s: string): number;
AssertType(s, "string");

    (s: any): string;
AssertType(s, "any");

};

func(""); // number
AssertType(func(""), "number");
AssertType(func, "{ (string): number; (any): string; }");
AssertType("", "string");

func(3); // string
AssertType(func(3), "string");
AssertType(func, "{ (string): number; (any): string; }");
AssertType(3, "int");

let x: any;
AssertType(x, "any");

func(x); // string
AssertType(func(x), "string");
AssertType(func, "{ (string): number; (any): string; }");
AssertType(x, "any");

let func2: {
AssertType(func2, "{ (string, string): number; (any, string): boolean; (string, any): RegExp; (any, any): string; }");

    (s: string, t: string): number;
AssertType(s, "string");
AssertType(t, "string");

    (s: any, t: string): boolean;
AssertType(s, "any");
AssertType(t, "string");

    (s: string, t: any): RegExp;
AssertType(s, "string");
AssertType(t, "any");

    (s: any, t: any): string;
AssertType(s, "any");
AssertType(t, "any");
}

func2(x, x); // string
AssertType(func2(x, x), "string");
AssertType(func2, "{ (string, string): number; (any, string): boolean; (string, any): RegExp; (any, any): string; }");
AssertType(x, "any");
AssertType(x, "any");

func2("", ""); // number
AssertType(func2("", ""), "number");
AssertType(func2, "{ (string, string): number; (any, string): boolean; (string, any): RegExp; (any, any): string; }");
AssertType("", "string");
AssertType("", "string");

func2(x, ""); // boolean
AssertType(func2(x, ""), "boolean");
AssertType(func2, "{ (string, string): number; (any, string): boolean; (string, any): RegExp; (any, any): string; }");
AssertType(x, "any");
AssertType("", "string");

func2("", x); // RegExp
AssertType(func2("", x), "RegExp");
AssertType(func2, "{ (string, string): number; (any, string): boolean; (string, any): RegExp; (any, any): string; }");
AssertType("", "string");
AssertType(x, "any");


