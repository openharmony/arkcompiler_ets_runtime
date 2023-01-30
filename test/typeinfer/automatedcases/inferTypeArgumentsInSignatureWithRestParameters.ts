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

// === tests/cases/compiler/inferTypeArgumentsInSignatureWithRestParameters.ts ===
declare function AssertType(value:any, type:string):void;
function f<T>(array: T[], ...args) { }
function g(array: number[], ...args) { }
function h<T>(nonarray: T, ...args) { }
function i<T>(array: T[], opt?: any[]) { }
let a = [1, 2, 3, 4, 5];
AssertType(a, "number[]");
AssertType([1, 2, 3, 4, 5], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");
AssertType(5, "int");

f(a); // OK
AssertType(f(a), "void");
AssertType(f, "<T>(T[], ...any[]) => void");
AssertType(a, "number[]");

g(a); // OK
AssertType(g(a), "void");
AssertType(g, "(number[], ...any[]) => void");
AssertType(a, "number[]");

h(a); // OK
AssertType(h(a), "void");
AssertType(h, "<T>(T, ...any[]) => void");
AssertType(a, "number[]");

i(a); // OK
AssertType(i(a), "void");
AssertType(i, "<T>(T[], ?any[]) => void");
AssertType(a, "number[]");


