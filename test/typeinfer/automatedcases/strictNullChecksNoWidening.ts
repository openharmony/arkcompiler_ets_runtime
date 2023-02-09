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

// === tests/cases/conformance/types/typeRelationships/widenedTypes/strictNullChecksNoWidening.ts ===
declare function AssertType(value:any, type:string):void;
let a1 = null;
AssertType(a1, "null");
AssertType(null, "null");

let a2 = undefined;
AssertType(a2, "undefined");
AssertType(undefined, "undefined");

let a3 = void 0;
AssertType(a3, "undefined");
AssertType(void 0, "undefined");
AssertType(0, "int");

let b1 = [];
AssertType(b1, "never[]");
AssertType([], "never[]");

let b2 = [,];
AssertType(b2, "undefined[]");
AssertType([,], "undefined[]");
AssertType(, "undefined");

let b3 = [undefined];
AssertType(b3, "undefined[]");
AssertType([undefined], "undefined[]");
AssertType(undefined, "undefined");

let b4 = [[], []];
AssertType(b4, "never[][]");
AssertType([[], []], "never[][]");
AssertType([], "never[]");
AssertType([], "never[]");

let b5 = [[], [,]];
AssertType(b5, "undefined[][]");
AssertType([[], [,]], "undefined[][]");
AssertType([], "never[]");
AssertType([,], "undefined[]");
AssertType(, "undefined");

declare function f<T>(x: T): T;

let c1 = f(null);
AssertType(c1, "null");
AssertType(f(null), "null");
AssertType(f, "<T>(T) => T");
AssertType(null, "null");

let c2 = f(undefined);
AssertType(c2, "undefined");
AssertType(f(undefined), "undefined");
AssertType(f, "<T>(T) => T");
AssertType(undefined, "undefined");

let c3 = f([]);
AssertType(c3, "never[]");
AssertType(f([]), "never[]");
AssertType(f, "<T>(T) => T");
AssertType([], "never[]");


