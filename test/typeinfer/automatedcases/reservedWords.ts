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

// === tests/cases/compiler/reservedWords.ts ===
declare function AssertType(value:any, type:string):void;
let obj = {
AssertType(obj, "{ if: number; debugger: number; break: number; function: number; }");
AssertType({    if: 0,    debugger: 2,    break: 3,    function: 4}, "{ if: number; debugger: number; break: number; function: number; }");

    if: 0,
AssertType(if, "number");
AssertType(0, "int");

    debugger: 2,
AssertType(debugger, "number");
AssertType(2, "int");

    break: 3,
AssertType(break, "number");
AssertType(3, "int");

    function: 4
AssertType(function, "number");
AssertType(4, "int");
}

//This compiles.

let obj2 = {
AssertType(obj2, "{ if: number; while: number; debugger: number; break: number; function: number; }");
AssertType({    if: 0,    while: 1,    debugger: 2,    break: 3,    function: 4}, "{ if: number; while: number; debugger: number; break: number; function: number; }");

    if: 0,
AssertType(if, "number");
AssertType(0, "int");

    while: 1,
AssertType(while, "number");
AssertType(1, "int");

    debugger: 2,
AssertType(debugger, "number");
AssertType(2, "int");

    break: 3,
AssertType(break, "number");
AssertType(3, "int");

    function: 4
AssertType(function, "number");
AssertType(4, "int");
}


