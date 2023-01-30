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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesOverloads03.ts ===
declare function AssertType(value:any, type:string):void;
interface Base {
    x: string;
    y: number;
}

interface HelloOrWorld extends Base {
    p1: boolean;
}

interface JustHello extends Base {
    p2: boolean;
}

interface JustWorld extends Base {
    p3: boolean;
}

let hello: "hello";
AssertType(hello, "string");

let world: "world";
AssertType(world, "string");

let helloOrWorld: "hello" | "world";
AssertType(helloOrWorld, "union");

function f(p: "hello"): JustHello;
function f(p: "hello" | "world"): HelloOrWorld;
function f(p: "world"): JustWorld;
function f(p: string): Base;
function f(...args: any[]): any {
AssertType(undefined, "undefined");
    return undefined;
}

let fResult1 = f(hello);
AssertType(fResult1, "JustHello");
AssertType(f(hello), "JustHello");
AssertType(f, "{ ("hello"): JustHello; (union): HelloOrWorld; ("world"): JustWorld; (string): Base; }");
AssertType(hello, "string");

let fResult2 = f(world);
AssertType(fResult2, "JustWorld");
AssertType(f(world), "JustWorld");
AssertType(f, "{ ("hello"): JustHello; (union): HelloOrWorld; ("world"): JustWorld; (string): Base; }");
AssertType(world, "string");

let fResult3 = f(helloOrWorld);
AssertType(fResult3, "HelloOrWorld");
AssertType(f(helloOrWorld), "HelloOrWorld");
AssertType(f, "{ ("hello"): JustHello; (union): HelloOrWorld; ("world"): JustWorld; (string): Base; }");
AssertType(helloOrWorld, "union");

function g(p: string): Base;
function g(p: "hello"): JustHello;
function g(p: "hello" | "world"): HelloOrWorld;
function g(p: "world"): JustWorld;
function g(...args: any[]): any {
AssertType(undefined, "undefined");
    return undefined;
}

let gResult1 = g(hello);
AssertType(gResult1, "JustHello");
AssertType(g(hello), "JustHello");
AssertType(g, "{ (string): Base; ("hello"): JustHello; (union): HelloOrWorld; ("world"): JustWorld; }");
AssertType(hello, "string");

let gResult2 = g(world);
AssertType(gResult2, "JustWorld");
AssertType(g(world), "JustWorld");
AssertType(g, "{ (string): Base; ("hello"): JustHello; (union): HelloOrWorld; ("world"): JustWorld; }");
AssertType(world, "string");

let gResult3 = g(helloOrWorld);
AssertType(gResult3, "Base");
AssertType(g(helloOrWorld), "Base");
AssertType(g, "{ (string): Base; ("hello"): JustHello; (union): HelloOrWorld; ("world"): JustWorld; }");
AssertType(helloOrWorld, "union");


