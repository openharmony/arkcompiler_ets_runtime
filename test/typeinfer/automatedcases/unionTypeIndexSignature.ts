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

// === tests/cases/conformance/types/union/unionTypeIndexSignature.ts ===
declare function AssertType(value:any, type:string):void;
let numOrDate: number | Date;
AssertType(numOrDate, "union");

let anyVar: number;
AssertType(anyVar, "number");

// If each type in U has a string index signature, 
// U has a string index signature of a union type of the types of the string index signatures from each type in U.

let unionOfDifferentReturnType: { [a: string]: number; } | { [a: string]: Date; };
AssertType(unionOfDifferentReturnType, "union");
AssertType(a, "string");
AssertType(a, "string");

numOrDate = unionOfDifferentReturnType["hello"]; // number | Date
AssertType(numOrDate = unionOfDifferentReturnType["hello"], "union");
AssertType(numOrDate, "union");
AssertType(unionOfDifferentReturnType["hello"], "union");
AssertType(unionOfDifferentReturnType, "union");
AssertType("hello", "string");

numOrDate = unionOfDifferentReturnType[10]; // number | Date
AssertType(numOrDate = unionOfDifferentReturnType[10], "union");
AssertType(numOrDate, "union");
AssertType(unionOfDifferentReturnType[10], "union");
AssertType(unionOfDifferentReturnType, "union");
AssertType(10, "int");

let unionOfTypesWithAndWithoutStringSignature: { [a: string]: number; } | boolean;
AssertType(unionOfTypesWithAndWithoutStringSignature, "union");
AssertType(a, "string");

anyVar = unionOfTypesWithAndWithoutStringSignature["hello"]; // any
AssertType(anyVar = unionOfTypesWithAndWithoutStringSignature["hello"], "error");
AssertType(anyVar, "number");
AssertType(unionOfTypesWithAndWithoutStringSignature["hello"], "error");
AssertType(unionOfTypesWithAndWithoutStringSignature, "union");
AssertType("hello", "string");

anyVar = unionOfTypesWithAndWithoutStringSignature[10]; // any
AssertType(anyVar = unionOfTypesWithAndWithoutStringSignature[10], "error");
AssertType(anyVar, "number");
AssertType(unionOfTypesWithAndWithoutStringSignature[10], "error");
AssertType(unionOfTypesWithAndWithoutStringSignature, "union");
AssertType(10, "int");

// If each type in U has a numeric index signature, 
// U has a numeric index signature of a union type of the types of the numeric index signatures from each type in U.
let unionOfDifferentReturnType1: { [a: number]: number; } | { [a: number]: Date; };
AssertType(unionOfDifferentReturnType1, "union");
AssertType(a, "number");
AssertType(a, "number");

numOrDate = unionOfDifferentReturnType1["hello"]; // any
AssertType(numOrDate = unionOfDifferentReturnType1["hello"], "error");
AssertType(numOrDate, "union");
AssertType(unionOfDifferentReturnType1["hello"], "error");
AssertType(unionOfDifferentReturnType1, "union");
AssertType("hello", "string");

numOrDate = unionOfDifferentReturnType1[10]; // number | Date
AssertType(numOrDate = unionOfDifferentReturnType1[10], "union");
AssertType(numOrDate, "union");
AssertType(unionOfDifferentReturnType1[10], "union");
AssertType(unionOfDifferentReturnType1, "union");
AssertType(10, "int");

let unionOfTypesWithAndWithoutStringSignature1: { [a: number]: number; } | boolean;
AssertType(unionOfTypesWithAndWithoutStringSignature1, "union");
AssertType(a, "number");

anyVar = unionOfTypesWithAndWithoutStringSignature1["hello"]; // any
AssertType(anyVar = unionOfTypesWithAndWithoutStringSignature1["hello"], "error");
AssertType(anyVar, "number");
AssertType(unionOfTypesWithAndWithoutStringSignature1["hello"], "error");
AssertType(unionOfTypesWithAndWithoutStringSignature1, "union");
AssertType("hello", "string");

anyVar = unionOfTypesWithAndWithoutStringSignature1[10]; // any
AssertType(anyVar = unionOfTypesWithAndWithoutStringSignature1[10], "error");
AssertType(anyVar, "number");
AssertType(unionOfTypesWithAndWithoutStringSignature1[10], "error");
AssertType(unionOfTypesWithAndWithoutStringSignature1, "union");
AssertType(10, "int");


