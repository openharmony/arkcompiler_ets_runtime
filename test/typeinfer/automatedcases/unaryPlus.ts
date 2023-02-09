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

// === tests/cases/compiler/unaryPlus.ts ===
declare function AssertType(value:any, type:string):void;
// allowed per spec
let a = +1;
AssertType(a, "number");
AssertType(+1, "int");
AssertType(1, "int");

let b = +(<any>"");
AssertType(b, "number");
AssertType(+(<any>""), "number");
AssertType((<any>""), "any");
AssertType(<any>"", "any");
AssertType("", "string");

enum E { some, thing };
let c = +E.some;
AssertType(c, "number");
AssertType(+E.some, "number");
AssertType(E.some, "E.some");

// also allowed, used to be errors
let x = +"3"; //should be valid
AssertType(x, "number");
AssertType(+"3", "number");
AssertType("3", "string");

let y = -"3"; // should be valid
AssertType(y, "number");
AssertType(-"3", "number");
AssertType("3", "string");

let z = ~"3"; // should be valid
AssertType(z, "number");
AssertType(~"3", "number");
AssertType("3", "string");


