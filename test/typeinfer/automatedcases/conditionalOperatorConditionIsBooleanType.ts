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

// === tests/cases/conformance/expressions/conditonalOperator/conditionalOperatorConditionIsBooleanType.ts ===
declare function AssertType(value:any, type:string):void;
//Cond ? Expr1 : Expr2,  Cond is of boolean type, Expr1 and Expr2 have the same type
let condBoolean: boolean;
AssertType(condBoolean, "boolean");

let exprAny1: any;
AssertType(exprAny1, "any");

let exprBoolean1: boolean;
AssertType(exprBoolean1, "boolean");

let exprNumber1: number;
AssertType(exprNumber1, "number");

let exprString1: string;
AssertType(exprString1, "string");

let exprIsObject1: Object;
AssertType(exprIsObject1, "Object");

let exprAny2: any;
AssertType(exprAny2, "any");

let exprBoolean2: boolean;
AssertType(exprBoolean2, "boolean");

let exprNumber2: number;
AssertType(exprNumber2, "number");

let exprString2: string;
AssertType(exprString2, "string");

let exprIsObject2: Object;
AssertType(exprIsObject2, "Object");

//Cond is a boolean type letiable
condBoolean ? exprAny1 : exprAny2;
AssertType(condBoolean ? exprAny1 : exprAny2, "any");
AssertType(condBoolean, "boolean");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

condBoolean ? exprBoolean1 : exprBoolean2;
AssertType(condBoolean ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condBoolean, "boolean");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

condBoolean ? exprNumber1 : exprNumber2;
AssertType(condBoolean ? exprNumber1 : exprNumber2, "number");
AssertType(condBoolean, "boolean");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

condBoolean ? exprString1 : exprString2;
AssertType(condBoolean ? exprString1 : exprString2, "string");
AssertType(condBoolean, "boolean");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

condBoolean ? exprIsObject1 : exprIsObject2;
AssertType(condBoolean ? exprIsObject1 : exprIsObject2, "Object");
AssertType(condBoolean, "boolean");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

condBoolean ? exprString1 : exprBoolean1; // union
AssertType(condBoolean ? exprString1 : exprBoolean1, "union");
AssertType(condBoolean, "boolean");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Cond is a boolean type literal
true ? exprAny1 : exprAny2;
AssertType(true ? exprAny1 : exprAny2, "any");
AssertType(true, "boolean");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

false ? exprBoolean1 : exprBoolean2;
AssertType(false ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(false, "boolean");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

true ? exprNumber1 : exprNumber2;
AssertType(true ? exprNumber1 : exprNumber2, "number");
AssertType(true, "boolean");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

false ? exprString1 : exprString2;
AssertType(false ? exprString1 : exprString2, "string");
AssertType(false, "boolean");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

true ? exprIsObject1 : exprIsObject2;
AssertType(true ? exprIsObject1 : exprIsObject2, "Object");
AssertType(true, "boolean");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

true ? exprString1 : exprBoolean1; // union
AssertType(true ? exprString1 : exprBoolean1, "union");
AssertType(true, "boolean");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Cond is a boolean type expression
!true ? exprAny1 : exprAny2;
AssertType(!true ? exprAny1 : exprAny2, "any");
AssertType(!true, "boolean");
AssertType(true, "boolean");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

typeof "123" == "string" ? exprBoolean1 : exprBoolean2;
AssertType(typeof "123" == "string" ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(typeof "123" == "string", "boolean");
AssertType(typeof "123", "union");
AssertType("123", "string");
AssertType("string", "string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

2 > 1 ? exprNumber1 : exprNumber2;
AssertType(2 > 1 ? exprNumber1 : exprNumber2, "number");
AssertType(2 > 1, "boolean");
AssertType(2, "int");
AssertType(1, "int");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

null === undefined ? exprString1 : exprString2;
AssertType(null === undefined ? exprString1 : exprString2, "string");
AssertType(null === undefined, "boolean");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

true || false ? exprIsObject1 : exprIsObject2;
AssertType(true || false ? exprIsObject1 : exprIsObject2, "Object");
AssertType(true || false, "boolean");
AssertType(true, "boolean");
AssertType(false, "boolean");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

null === undefined ? exprString1 : exprBoolean1; // union
AssertType(null === undefined ? exprString1 : exprBoolean1, "union");
AssertType(null === undefined, "boolean");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Results shoud be same as Expr1 and Expr2
let resultIsAny1 = condBoolean ? exprAny1 : exprAny2;
AssertType(resultIsAny1, "any");
AssertType(condBoolean ? exprAny1 : exprAny2, "any");
AssertType(condBoolean, "boolean");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean1 = condBoolean ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean1, "boolean");
AssertType(condBoolean ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condBoolean, "boolean");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber1 = condBoolean ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber1, "number");
AssertType(condBoolean ? exprNumber1 : exprNumber2, "number");
AssertType(condBoolean, "boolean");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString1 = condBoolean ? exprString1 : exprString2;
AssertType(resultIsString1, "string");
AssertType(condBoolean ? exprString1 : exprString2, "string");
AssertType(condBoolean, "boolean");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject1 = condBoolean ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject1, "Object");
AssertType(condBoolean ? exprIsObject1 : exprIsObject2, "Object");
AssertType(condBoolean, "boolean");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean1 = condBoolean ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean1, "union");
AssertType(condBoolean ? exprString1 : exprBoolean1, "union");
AssertType(condBoolean, "boolean");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsAny2 = true ? exprAny1 : exprAny2;
AssertType(resultIsAny2, "any");
AssertType(true ? exprAny1 : exprAny2, "any");
AssertType(true, "boolean");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean2 = false ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean2, "boolean");
AssertType(false ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(false, "boolean");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber2 = true ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber2, "number");
AssertType(true ? exprNumber1 : exprNumber2, "number");
AssertType(true, "boolean");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString2 = false ? exprString1 : exprString2;
AssertType(resultIsString2, "string");
AssertType(false ? exprString1 : exprString2, "string");
AssertType(false, "boolean");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject2 = true ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject2, "Object");
AssertType(true ? exprIsObject1 : exprIsObject2, "Object");
AssertType(true, "boolean");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean2 = true ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean2, "union");
AssertType(true ? exprString1 : exprBoolean1, "union");
AssertType(true, "boolean");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsStringOrBoolean3 = false ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean3, "union");
AssertType(false ? exprString1 : exprBoolean1, "union");
AssertType(false, "boolean");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsAny3 = !true ? exprAny1 : exprAny2;
AssertType(resultIsAny3, "any");
AssertType(!true ? exprAny1 : exprAny2, "any");
AssertType(!true, "boolean");
AssertType(true, "boolean");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean3 = typeof "123" == "string" ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean3, "boolean");
AssertType(typeof "123" == "string" ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(typeof "123" == "string", "boolean");
AssertType(typeof "123", "union");
AssertType("123", "string");
AssertType("string", "string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber3 = 2 > 1 ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber3, "number");
AssertType(2 > 1 ? exprNumber1 : exprNumber2, "number");
AssertType(2 > 1, "boolean");
AssertType(2, "int");
AssertType(1, "int");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString3 = null === undefined ? exprString1 : exprString2;
AssertType(resultIsString3, "string");
AssertType(null === undefined ? exprString1 : exprString2, "string");
AssertType(null === undefined, "boolean");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject3 = true || false ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject3, "Object");
AssertType(true || false ? exprIsObject1 : exprIsObject2, "Object");
AssertType(true || false, "boolean");
AssertType(true, "boolean");
AssertType(false, "boolean");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean4 = typeof "123" === "string" ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean4, "union");
AssertType(typeof "123" === "string" ? exprString1 : exprBoolean1, "union");
AssertType(typeof "123" === "string", "boolean");
AssertType(typeof "123", "union");
AssertType("123", "string");
AssertType("string", "string");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");


