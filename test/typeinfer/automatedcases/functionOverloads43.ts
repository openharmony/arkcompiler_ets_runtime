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

// === tests/cases/compiler/functionOverloads43.ts ===
declare function AssertType(value:any, type:string):void;
function foo(bar: { a:number }[]): number;
function foo(bar: { a:string }[]): string;
function foo([x]: { a:number | string }[]): string | number {
    if (x) {
AssertType(x, "{ a: union; }");

AssertType(x.a, "union");
        return x.a;
    }
    
AssertType(undefined, "undefined");
    return undefined;
}

let x = foo([{a: "str"}]);
AssertType(x, "string");
AssertType(foo([{a: "str"}]), "string");
AssertType(foo, "{ ({ a: number; }[]): number; ({ a: string; }[]): string; }");
AssertType([{a: "str"}], "{ a: string; }[]");
AssertType({a: "str"}, "{ a: string; }");
AssertType(a, "string");
AssertType("str", "string");

let y = foo([{a: 100}]);
AssertType(y, "number");
AssertType(foo([{a: 100}]), "number");
AssertType(foo, "{ ({ a: number; }[]): number; ({ a: string; }[]): string; }");
AssertType([{a: 100}], "{ a: number; }[]");
AssertType({a: 100}, "{ a: number; }");
AssertType(a, "number");
AssertType(100, "int");


