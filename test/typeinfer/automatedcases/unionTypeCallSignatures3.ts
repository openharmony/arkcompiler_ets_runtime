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

// === tests/cases/conformance/types/union/unionTypeCallSignatures3.ts ===
declare function AssertType(value:any, type:string):void;
function f1(s: string) { }
function f2(s?: string) { }
function f3(...s: string[]) { }
function f4(s: string, s2?: string) { }
function f5(s?: string, n?: number) { }
function f6(s?: string, ...n: number[]) { }
function f7(s: string, ...sRest: string[]) { }

let fUnion: typeof f1 | typeof f2 | typeof f3 | typeof f4 | typeof f5 | typeof f6 | typeof f7;
AssertType(fUnion, "union");
AssertType(f1, "(string) => void");
AssertType(f2, "(?string) => void");
AssertType(f3, "(...string[]) => void");
AssertType(f4, "(string, ?string) => void");
AssertType(f5, "(?string, ?number) => void");
AssertType(f6, "(?string, ...number[]) => void");
AssertType(f7, "(string, ...string[]) => void");

fUnion(""); // All constituents can be called by passing a single string.
AssertType(fUnion(""), "void");
AssertType(fUnion, "union");
AssertType("", "string");


