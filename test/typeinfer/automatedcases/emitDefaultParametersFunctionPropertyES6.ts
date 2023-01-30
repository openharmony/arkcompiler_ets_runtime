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

// === tests/cases/conformance/es6/defaultParameters/emitDefaultParametersFunctionPropertyES6.ts ===
declare function AssertType(value:any, type:string):void;
let obj2 = {
AssertType(obj2, "{ func1(?number, ...any[]): void; func2(?string): void; func3(string, number, ?string): void; func4(string, number, ?string, ...any[]): void; }");
AssertType({    func1(y = 10, ...rest) { },    func2(x = "hello") { },    func3(x: string, z: number, y = "hello") { },    func4(x: string, z: number, y = "hello", ...rest) { },}, "{ func1(?number, ...any[]): void; func2(?string): void; func3(string, number, ?string): void; func4(string, number, ?string, ...any[]): void; }");

    func1(y = 10, ...rest) { },
AssertType(func1, "(?number, ...any[]) => void");
AssertType(y, "number");
AssertType(10, "int");
AssertType(rest, "any[]");

    func2(x = "hello") { },
AssertType(func2, "(?string) => void");
AssertType(x, "string");
AssertType("hello", "string");

    func3(x: string, z: number, y = "hello") { },
AssertType(func3, "(string, number, ?string) => void");
AssertType(x, "string");
AssertType(z, "number");
AssertType(y, "string");
AssertType("hello", "string");

    func4(x: string, z: number, y = "hello", ...rest) { },
AssertType(func4, "(string, number, ?string, ...any[]) => void");
AssertType(x, "string");
AssertType(z, "number");
AssertType(y, "string");
AssertType("hello", "string");
AssertType(rest, "any[]");
}

