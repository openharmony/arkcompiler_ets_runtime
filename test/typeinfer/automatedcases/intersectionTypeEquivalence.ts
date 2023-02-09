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

// === tests/cases/conformance/types/intersection/intersectionTypeEquivalence.ts ===
declare function AssertType(value:any, type:string):void;
interface A { a: string }
interface B { b: string }
interface C { c: string }

// A & B is equivalent to B & A.
let y: A & B;
AssertType(y, "A & B");

let y : B & A;
AssertType(y, "A & B");

// AB & C is equivalent to A & BC, where AB is A & B and BC is B & C.
let z : A & B & C;
AssertType(z, "A & B & C");

let z : (A & B) & C;
AssertType(z, "A & B & C");

let z : A & (B & C);
AssertType(z, "A & B & C");

let ab : A & B;
AssertType(ab, "A & B");

let bc : B & C;
AssertType(bc, "B & C");

let z1: typeof ab & C;
AssertType(z1, "A & B & C");
AssertType(ab, "A & B");

let z1: A & typeof bc;
AssertType(z1, "A & B & C");
AssertType(bc, "B & C");


