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

// === tests/cases/conformance/expressions/conditonalOperator/conditionalOperatorConditionIsNumberType.ts ===
declare function AssertType(value:any, type:string):void;
//Cond ? Expr1 : Expr2,  Cond is of number type, Expr1 and Expr2 have the same type
let condNumber: number;
AssertType(condNumber, "number");

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

//Cond is a number type letiable
condNumber ? exprAny1 : exprAny2;
AssertType(condNumber ? exprAny1 : exprAny2, "any");
AssertType(condNumber, "number");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

condNumber ? exprBoolean1 : exprBoolean2;
AssertType(condNumber ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condNumber, "number");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

condNumber ? exprNumber1 : exprNumber2;
AssertType(condNumber ? exprNumber1 : exprNumber2, "number");
AssertType(condNumber, "number");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

condNumber ? exprString1 : exprString2;
AssertType(condNumber ? exprString1 : exprString2, "string");
AssertType(condNumber, "number");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

condNumber ? exprIsObject1 : exprIsObject2;
AssertType(condNumber ? exprIsObject1 : exprIsObject2, "Object");
AssertType(condNumber, "number");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

condNumber ? exprString1 : exprBoolean1; // Union
AssertType(condNumber ? exprString1 : exprBoolean1, "union");
AssertType(condNumber, "number");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Cond is a number type literal
1 ? exprAny1 : exprAny2;
AssertType(1 ? exprAny1 : exprAny2, "any");
AssertType(1, "int");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

0 ? exprBoolean1 : exprBoolean2;
AssertType(0 ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(0, "int");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

0.123456789 ? exprNumber1 : exprNumber2;
AssertType(0.123456789 ? exprNumber1 : exprNumber2, "number");
AssertType(0.123456789, "double");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

- 10000000000000 ? exprString1 : exprString2;
AssertType(- 10000000000000 ? exprString1 : exprString2, "string");
AssertType(- 10000000000000, "int");
AssertType(10000000000000, "int");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

1000000000000 ? exprIsObject1 : exprIsObject2;
AssertType(1000000000000 ? exprIsObject1 : exprIsObject2, "Object");
AssertType(1000000000000, "int");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

10000 ? exprString1 : exprBoolean1; // Union
AssertType(10000 ? exprString1 : exprBoolean1, "union");
AssertType(10000, "int");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Cond is a number type expression
function foo() { 
AssertType(1, "int");
return 1 };

let array = [1, 2, 3];
AssertType(array, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

1 * 0 ? exprAny1 : exprAny2;
AssertType(1 * 0 ? exprAny1 : exprAny2, "any");
AssertType(1 * 0, "number");
AssertType(1, "int");
AssertType(0, "int");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

1 + 1 ? exprBoolean1 : exprBoolean2;
AssertType(1 + 1 ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(1 + 1, "number");
AssertType(1, "int");
AssertType(1, "int");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

"string".length ? exprNumber1 : exprNumber2;
AssertType("string".length ? exprNumber1 : exprNumber2, "number");
AssertType("string".length, "number");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

foo() ? exprString1 : exprString2;
AssertType(foo() ? exprString1 : exprString2, "string");
AssertType(foo(), "number");
AssertType(foo, "() => number");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

foo() / array[1] ? exprIsObject1 : exprIsObject2;
AssertType(foo() / array[1] ? exprIsObject1 : exprIsObject2, "Object");
AssertType(foo() / array[1], "number");
AssertType(foo(), "number");
AssertType(foo, "() => number");
AssertType(array[1], "number");
AssertType(array, "number[]");
AssertType(1, "int");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

foo() ? exprString1 : exprBoolean1; // Union
AssertType(foo() ? exprString1 : exprBoolean1, "union");
AssertType(foo(), "number");
AssertType(foo, "() => number");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Results shoud be same as Expr1 and Expr2
let resultIsAny1 = condNumber ? exprAny1 : exprAny2;
AssertType(resultIsAny1, "any");
AssertType(condNumber ? exprAny1 : exprAny2, "any");
AssertType(condNumber, "number");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean1 = condNumber ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean1, "boolean");
AssertType(condNumber ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condNumber, "number");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber1 = condNumber ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber1, "number");
AssertType(condNumber ? exprNumber1 : exprNumber2, "number");
AssertType(condNumber, "number");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString1 = condNumber ? exprString1 : exprString2;
AssertType(resultIsString1, "string");
AssertType(condNumber ? exprString1 : exprString2, "string");
AssertType(condNumber, "number");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject1 = condNumber ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject1, "Object");
AssertType(condNumber ? exprIsObject1 : exprIsObject2, "Object");
AssertType(condNumber, "number");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean1 = condNumber ? exprString1 : exprBoolean1; // Union
AssertType(resultIsStringOrBoolean1, "union");
AssertType(condNumber ? exprString1 : exprBoolean1, "union");
AssertType(condNumber, "number");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsAny2 = 1 ? exprAny1 : exprAny2;
AssertType(resultIsAny2, "any");
AssertType(1 ? exprAny1 : exprAny2, "any");
AssertType(1, "int");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean2 = 0 ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean2, "boolean");
AssertType(0 ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(0, "int");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber2 = 0.123456789 ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber2, "number");
AssertType(0.123456789 ? exprNumber1 : exprNumber2, "number");
AssertType(0.123456789, "double");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString2 = - 10000000000000 ? exprString1 : exprString2;
AssertType(resultIsString2, "string");
AssertType(- 10000000000000 ? exprString1 : exprString2, "string");
AssertType(- 10000000000000, "int");
AssertType(10000000000000, "int");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject2 = 1000000000000 ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject2, "Object");
AssertType(1000000000000 ? exprIsObject1 : exprIsObject2, "Object");
AssertType(1000000000000, "int");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean2 = 10000 ? exprString1 : exprBoolean1; // Union
AssertType(resultIsStringOrBoolean2, "union");
AssertType(10000 ? exprString1 : exprBoolean1, "union");
AssertType(10000, "int");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsAny3 = 1 * 0 ? exprAny1 : exprAny2;
AssertType(resultIsAny3, "any");
AssertType(1 * 0 ? exprAny1 : exprAny2, "any");
AssertType(1 * 0, "number");
AssertType(1, "int");
AssertType(0, "int");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean3 = 1 + 1 ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean3, "boolean");
AssertType(1 + 1 ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(1 + 1, "number");
AssertType(1, "int");
AssertType(1, "int");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber3 = "string".length ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber3, "number");
AssertType("string".length ? exprNumber1 : exprNumber2, "number");
AssertType("string".length, "number");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString3 = foo() ? exprString1 : exprString2;
AssertType(resultIsString3, "string");
AssertType(foo() ? exprString1 : exprString2, "string");
AssertType(foo(), "number");
AssertType(foo, "() => number");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject3 = foo() / array[1] ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject3, "Object");
AssertType(foo() / array[1] ? exprIsObject1 : exprIsObject2, "Object");
AssertType(foo() / array[1], "number");
AssertType(foo(), "number");
AssertType(foo, "() => number");
AssertType(array[1], "number");
AssertType(array, "number[]");
AssertType(1, "int");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean3 = foo() / array[1] ? exprString1 : exprBoolean1; // Union
AssertType(resultIsStringOrBoolean3, "union");
AssertType(foo() / array[1] ? exprString1 : exprBoolean1, "union");
AssertType(foo() / array[1], "number");
AssertType(foo(), "number");
AssertType(foo, "() => number");
AssertType(array[1], "number");
AssertType(array, "number[]");
AssertType(1, "int");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");


