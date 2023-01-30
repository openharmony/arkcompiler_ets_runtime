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

// === tests/cases/compiler/declarationEmitDestructuringArrayPattern5.ts ===
declare function AssertType(value:any, type:string):void;
let [, , z] = [1, 2, 4];
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(z, "number");
AssertType([1, 2, 4], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(4, "int");

let [, a, , ] = [3, 4, 5];
AssertType(, "undefined");
AssertType(a, "number");
AssertType(, "undefined");
AssertType([3, 4, 5], "[number, number, number]");
AssertType(3, "int");
AssertType(4, "int");
AssertType(5, "int");

let [, , [, b, ]] = [3,5,[0, 1]];
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(b, "number");
AssertType([3,5,[0, 1]], "[number, number, [number, number]]");
AssertType(3, "int");
AssertType(5, "int");
AssertType([0, 1], "[number, number]");
AssertType(0, "int");
AssertType(1, "int");


