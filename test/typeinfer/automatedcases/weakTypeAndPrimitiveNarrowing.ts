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

// === tests/cases/compiler/weakTypeAndPrimitiveNarrowing.ts ===
declare function AssertType(value:any, type:string):void;
type LiteralsAndWeakTypes = 
  | 'A'
  | 'B'
  | { optional?: true }
  | { toLowerCase?(): string }
  | { toUpperCase?(): string, otherOptionalProp?: number };

const g = (arg: LiteralsAndWeakTypes) => {
AssertType(g, "(LiteralsAndWeakTypes) => void");
AssertType((arg: LiteralsAndWeakTypes) => {    if (arg === 'A') {      arg;    } else {      arg;    }}, "(LiteralsAndWeakTypes) => void");
AssertType(arg, "LiteralsAndWeakTypes");

    if (arg === 'A') {
AssertType(arg === 'A', "boolean");
AssertType(arg, "LiteralsAndWeakTypes");
AssertType('A', "string");

      arg;
AssertType(arg, "union");

    } else {
      arg;
AssertType(arg, "union");
    }
}

type PrimitivesAndWeakTypes =
  | string
  | number
  | { optional?: true }
  | { toLowerCase?(): string }
  | { toUpperCase?(): string, otherOptionalProp?: number };

const h = (arg: PrimitivesAndWeakTypes) => {
AssertType(h, "(PrimitivesAndWeakTypes) => void");
AssertType((arg: PrimitivesAndWeakTypes) => {    if (arg === 'A') {      arg;    } else {      arg;    }}, "(PrimitivesAndWeakTypes) => void");
AssertType(arg, "PrimitivesAndWeakTypes");

    if (arg === 'A') {
AssertType(arg === 'A', "boolean");
AssertType(arg, "PrimitivesAndWeakTypes");
AssertType('A', "string");

      arg;
AssertType(arg, "union");

    } else {
      arg;
AssertType(arg, "PrimitivesAndWeakTypes");
    }
}


