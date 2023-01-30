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

// === tests/cases/conformance/expressions/unaryOperators/voidOperator/voidOperatorWithEnumType.ts ===
declare function AssertType(value:any, type:string):void;
// void  operator on enum type

enum ENUM { };
enum ENUM1 { A, B, "" };

// enum type let
let ResultIsAny1 = void ENUM;
AssertType(ResultIsAny1, "any");
AssertType(void ENUM, "undefined");
AssertType(ENUM, "typeof ENUM");

let ResultIsAny2 = void ENUM1;
AssertType(ResultIsAny2, "any");
AssertType(void ENUM1, "undefined");
AssertType(ENUM1, "typeof ENUM1");

// enum type expressions
let ResultIsAny3 = void ENUM1["A"];
AssertType(ResultIsAny3, "any");
AssertType(void ENUM1["A"], "undefined");
AssertType(ENUM1["A"], "ENUM1.A");
AssertType(ENUM1, "typeof ENUM1");
AssertType("A", "string");

let ResultIsAny4 = void (ENUM[0] + ENUM1["B"]);
AssertType(ResultIsAny4, "any");
AssertType(void (ENUM[0] + ENUM1["B"]), "undefined");
AssertType((ENUM[0] + ENUM1["B"]), "string");
AssertType(ENUM[0] + ENUM1["B"], "string");
AssertType(ENUM[0], "string");
AssertType(ENUM, "typeof ENUM");
AssertType(0, "int");
AssertType(ENUM1["B"], "ENUM1.B");
AssertType(ENUM1, "typeof ENUM1");
AssertType("B", "string");

// multiple void  operators
let ResultIsAny5 = void void ENUM;
AssertType(ResultIsAny5, "any");
AssertType(void void ENUM, "undefined");
AssertType(void ENUM, "undefined");
AssertType(ENUM, "typeof ENUM");

let ResultIsAny6 = void void void (ENUM[0] + ENUM1.B);
AssertType(ResultIsAny6, "any");
AssertType(void void void (ENUM[0] + ENUM1.B), "undefined");
AssertType(void void (ENUM[0] + ENUM1.B), "undefined");
AssertType(void (ENUM[0] + ENUM1.B), "undefined");
AssertType((ENUM[0] + ENUM1.B), "string");
AssertType(ENUM[0] + ENUM1.B, "string");
AssertType(ENUM[0], "string");
AssertType(ENUM, "typeof ENUM");
AssertType(0, "int");
AssertType(ENUM1.B, "ENUM1.B");

// miss assignment operators
void ENUM;
AssertType(void ENUM, "undefined");
AssertType(ENUM, "typeof ENUM");

void ENUM1;
AssertType(void ENUM1, "undefined");
AssertType(ENUM1, "typeof ENUM1");

void ENUM1["B"];
AssertType(void ENUM1["B"], "undefined");
AssertType(ENUM1["B"], "ENUM1.B");
AssertType(ENUM1, "typeof ENUM1");
AssertType("B", "string");

void ENUM, ENUM1;
AssertType(void ENUM, ENUM1, "typeof ENUM1");
AssertType(void ENUM, "undefined");
AssertType(ENUM, "typeof ENUM");
AssertType(ENUM1, "typeof ENUM1");


