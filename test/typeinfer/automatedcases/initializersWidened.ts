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

// === tests/cases/conformance/types/typeRelationships/widenedTypes/initializersWidened.ts ===
declare function AssertType(value:any, type:string):void;
// these are widened to any at the point of assignment

let x1 = null;
AssertType(x1, "any");
AssertType(null, "null");

let y1 = undefined;
AssertType(y1, "any");
AssertType(undefined, "undefined");

let z1 = void 0;
AssertType(z1, "any");
AssertType(void 0, "undefined");
AssertType(0, "int");

// these are not widened

let x2: null;
AssertType(x2, "null");
AssertType(null, "null");

let y2: undefined;
AssertType(y2, "undefined");

let x3: null = null;
AssertType(x3, "null");
AssertType(null, "null");
AssertType(null, "null");

let y3: undefined = undefined;
AssertType(y3, "undefined");
AssertType(undefined, "undefined");

let z3: undefined = void 0;
AssertType(z3, "undefined");
AssertType(void 0, "undefined");
AssertType(0, "int");

// widen only when all constituents of union are widening

let x4 = null || null;
AssertType(x4, "any");
AssertType(null || null, "null");
AssertType(null, "null");
AssertType(null, "null");

let y4 = undefined || undefined;
AssertType(y4, "any");
AssertType(undefined || undefined, "undefined");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");

let z4 = void 0 || void 0;
AssertType(z4, "any");
AssertType(void 0 || void 0, "undefined");
AssertType(void 0, "undefined");
AssertType(0, "int");
AssertType(void 0, "undefined");
AssertType(0, "int");

let x5 = null || x2;
AssertType(x5, "null");
AssertType(null || x2, "null");
AssertType(null, "null");
AssertType(x2, "null");

let y5 = undefined || y2;
AssertType(y5, "undefined");
AssertType(undefined || y2, "undefined");
AssertType(undefined, "undefined");
AssertType(y2, "undefined");

let z5 = void 0 || y2;
AssertType(z5, "undefined");
AssertType(void 0 || y2, "undefined");
AssertType(void 0, "undefined");
AssertType(0, "int");
AssertType(y2, "undefined");


