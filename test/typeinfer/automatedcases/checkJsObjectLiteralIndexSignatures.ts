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

// === tests/cases/compiler/file.js ===
declare function AssertType(value:any, type:string):void;
// @ts-check

let n = Math.random();
AssertType(n, "number");
AssertType(Math.random(), "number");
AssertType(Math.random, "() => number");

let s = `${n}`;
AssertType(s, "string");
AssertType(`${n}`, "string");
AssertType(n, "number");

const numericIndex = { [n]: 1 };
AssertType(numericIndex, "{ [number]: number; }");
AssertType({ [n]: 1 }, "{ [number]: number; }");
AssertType([n], "number");
AssertType(n, "number");
AssertType(1, "int");

numericIndex[n].toFixed();
AssertType(numericIndex[n].toFixed(), "string");
AssertType(numericIndex[n].toFixed, "(?number) => string");

const stringIndex = { [s]: 1 };
AssertType(stringIndex, "{ [string]: number; }");
AssertType({ [s]: 1 }, "{ [string]: number; }");
AssertType([s], "number");
AssertType(s, "string");
AssertType(1, "int");

stringIndex[s].toFixed();
AssertType(stringIndex[s].toFixed(), "string");
AssertType(stringIndex[s].toFixed, "(?number) => string");



