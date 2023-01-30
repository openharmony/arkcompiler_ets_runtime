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

// === tests/cases/compiler/arrayFind.ts ===
declare function AssertType(value:any, type:string):void;
// test fix for #18112, type guard predicates should narrow returned element
function isNumber(x: any): x is number {
AssertType(typeof x === "number", "boolean");
AssertType(typeof x, "union");
AssertType(x, "any");
AssertType("number", "string");
  return typeof x === "number";
}

const arrayOfStringsNumbersAndBooleans = ["string", false, 0, "strung", 1, true];
AssertType(arrayOfStringsNumbersAndBooleans, "(union)[]");
AssertType(["string", false, 0, "strung", 1, true], "(union)[]");
AssertType("string", "string");
AssertType(false, "boolean");
AssertType(0, "int");
AssertType("strung", "string");
AssertType(1, "int");
AssertType(true, "boolean");

const foundNumber: number | undefined = arrayOfStringsNumbersAndBooleans.find(isNumber);
AssertType(foundNumber, "number");
AssertType(arrayOfStringsNumbersAndBooleans.find(isNumber), "number");
AssertType(arrayOfStringsNumbersAndBooleans.find, "{ <S extends union>((union, number, (union)[]) => value is S, ?any): S; ((union, number, (union)[]) => unknown, ?any): union; }");
AssertType(isNumber, "(any) => x is number");

const readonlyArrayOfStringsNumbersAndBooleans = arrayOfStringsNumbersAndBooleans as ReadonlyArray<string | number | boolean>;
AssertType(readonlyArrayOfStringsNumbersAndBooleans, "readonly (union)[]");
AssertType(arrayOfStringsNumbersAndBooleans as ReadonlyArray<string | number | boolean>, "readonly (union)[]");
AssertType(arrayOfStringsNumbersAndBooleans, "(union)[]");

const readonlyFoundNumber: number | undefined = readonlyArrayOfStringsNumbersAndBooleans.find(isNumber);
AssertType(readonlyFoundNumber, "number");
AssertType(readonlyArrayOfStringsNumbersAndBooleans.find(isNumber), "number");
AssertType(readonlyArrayOfStringsNumbersAndBooleans.find, "{ <S extends union>((union, number, readonly (union)[]) => value is S, ?any): S; ((union, number, readonly (union)[]) => unknown, ?any): union; }");
AssertType(isNumber, "(any) => x is number");


