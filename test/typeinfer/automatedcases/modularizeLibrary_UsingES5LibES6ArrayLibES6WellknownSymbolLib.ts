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

// === tests/cases/compiler/modularizeLibrary_UsingES5LibES6ArrayLibES6WellknownSymbolLib.ts ===
declare function AssertType(value:any, type:string):void;
function f(x: number, y: number, z: number) {
AssertType(Array.from(arguments), "any[]");
AssertType(Array.from, "{ <T>(ArrayLike<T>): T[]; <T, U>(ArrayLike<T>, (T, number) => U, ?any): U[]; }");
AssertType(arguments, "IArguments");
    return Array.from(arguments);
}

f(1, 2, 3);   // no error
AssertType(f(1, 2, 3), "any[]");
AssertType(f, "(number, number, number) => any[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let a = ['c', 'd'];
AssertType(a, "string[]");
AssertType(['c', 'd'], "string[]");
AssertType('c', "string");
AssertType('d', "string");

a[Symbol.isConcatSpreadable] = false;
AssertType(a[Symbol.isConcatSpreadable] = false, "boolean");
AssertType(a[Symbol.isConcatSpreadable], "error");
AssertType(a, "string[]");
AssertType(Symbol.isConcatSpreadable, "unique symbol");
AssertType(false, "boolean");


