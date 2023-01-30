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

// === tests/cases/conformance/types/objectTypeLiteral/methodSignatures/methodSignaturesWithOverloads2.ts ===
declare function AssertType(value:any, type:string):void;
// Object type literals permit overloads with optionality but they must match

let c: {
AssertType(c, "{ func4?(number): number; func4?(string): string; func5?: { (number): number; (string): string; }; }");

    func4?(x: number): number;
AssertType(func4, "{ (number): number; (string): string; }");
AssertType(x, "number");

    func4?(s: string): string;
AssertType(func4, "{ (number): number; (string): string; }");
AssertType(s, "string");

    func5?: {
AssertType(func5, "{ (number): number; (string): string; }");

        (x: number): number;
AssertType(x, "number");

        (s: string): string;
AssertType(s, "string");

    };
};

// no errors
c.func4 = c.func5;
AssertType(c.func4 = c.func5, "{ (number): number; (string): string; }");
AssertType(c.func4, "{ (number): number; (string): string; }");
AssertType(c.func5, "{ (number): number; (string): string; }");

c.func5 = c.func4;
AssertType(c.func5 = c.func4, "{ (number): number; (string): string; }");
AssertType(c.func5, "{ (number): number; (string): string; }");
AssertType(c.func4, "{ (number): number; (string): string; }");


let c2: {
AssertType(c2, "{ func4?<T>(T): number; func4?<T>(T): string; func5?: { <T>(T): number; <T>(T): string; }; }");

    func4?<T>(x: T): number;
AssertType(func4, "{ <T>(T): number; <T>(T): string; }");
AssertType(x, "T");

    func4? <T>(s: T): string;
AssertType(func4, "{ <T>(T): number; <T>(T): string; }");
AssertType(s, "T");

    func5?: {
AssertType(func5, "{ <T>(T): number; <T>(T): string; }");

        <T>(x: T): number;
AssertType(x, "T");

        <T>(s: T): string;
AssertType(s, "T");

    };
};

// no errors
c2.func4 = c2.func5;
AssertType(c2.func4 = c2.func5, "{ <T>(T): number; <T>(T): string; }");
AssertType(c2.func4, "{ <T>(T): number; <T>(T): string; }");
AssertType(c2.func5, "{ <T>(T): number; <T>(T): string; }");

c2.func5 = c2.func4;
AssertType(c2.func5 = c2.func4, "{ <T>(T): number; <T>(T): string; }");
AssertType(c2.func5, "{ <T>(T): number; <T>(T): string; }");
AssertType(c2.func4, "{ <T>(T): number; <T>(T): string; }");


