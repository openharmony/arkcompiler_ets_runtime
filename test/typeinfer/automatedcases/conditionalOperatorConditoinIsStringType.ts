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

// === tests/cases/conformance/expressions/conditonalOperator/conditionalOperatorConditoinIsStringType.ts ===
declare function AssertType(value:any, type:string):void;
//Cond ? Expr1 : Expr2,  Cond is of string type, Expr1 and Expr2 have the same type
let condString: string;
AssertType(condString, "string");

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

//Cond is a string type letiable
condString ? exprAny1 : exprAny2;
AssertType(condString ? exprAny1 : exprAny2, "any");
AssertType(condString, "string");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

condString ? exprBoolean1 : exprBoolean2;
AssertType(condString ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condString, "string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

condString ? exprNumber1 : exprNumber2;
AssertType(condString ? exprNumber1 : exprNumber2, "number");
AssertType(condString, "string");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

condString ? exprString1 : exprString2;
AssertType(condString ? exprString1 : exprString2, "string");
AssertType(condString, "string");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

condString ? exprIsObject1 : exprIsObject2;
AssertType(condString ? exprIsObject1 : exprIsObject2, "Object");
AssertType(condString, "string");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

condString ? exprString1 : exprBoolean1; // union
AssertType(condString ? exprString1 : exprBoolean1, "union");
AssertType(condString, "string");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Cond is a string type literal
"" ? exprAny1 : exprAny2;
AssertType("" ? exprAny1 : exprAny2, "any");
AssertType("", "string");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

"string" ? exprBoolean1 : exprBoolean2;
AssertType("string" ? exprBoolean1 : exprBoolean2, "boolean");
AssertType("string", "string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

'c' ? exprNumber1 : exprNumber2;
AssertType('c' ? exprNumber1 : exprNumber2, "number");
AssertType('c', "string");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

'string' ? exprString1 : exprString2;
AssertType('string' ? exprString1 : exprString2, "string");
AssertType('string', "string");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

"  " ? exprIsObject1 : exprIsObject2;
AssertType("  " ? exprIsObject1 : exprIsObject2, "Object");
AssertType("  ", "string");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

"hello " ? exprString1 : exprBoolean1; // union
AssertType("hello " ? exprString1 : exprBoolean1, "union");
AssertType("hello ", "string");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Cond is a string type expression
function foo() { 
AssertType("string", "string");
return "string" };

let array = ["1", "2", "3"];
AssertType(array, "string[]");
AssertType(["1", "2", "3"], "string[]");
AssertType("1", "string");
AssertType("2", "string");
AssertType("3", "string");

typeof condString ? exprAny1 : exprAny2;
AssertType(typeof condString ? exprAny1 : exprAny2, "any");
AssertType(typeof condString, "union");
AssertType(condString, "string");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

condString.toUpperCase ? exprBoolean1 : exprBoolean2;
AssertType(condString.toUpperCase ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condString.toUpperCase, "() => string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

condString + "string" ? exprNumber1 : exprNumber2;
AssertType(condString + "string" ? exprNumber1 : exprNumber2, "number");
AssertType(condString + "string", "string");
AssertType(condString, "string");
AssertType("string", "string");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

foo() ? exprString1 : exprString2;
AssertType(foo() ? exprString1 : exprString2, "string");
AssertType(foo(), "string");
AssertType(foo, "() => string");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

array[1] ? exprIsObject1 : exprIsObject2;
AssertType(array[1] ? exprIsObject1 : exprIsObject2, "Object");
AssertType(array[1], "string");
AssertType(array, "string[]");
AssertType(1, "int");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

foo() ? exprString1 : exprBoolean1; // union
AssertType(foo() ? exprString1 : exprBoolean1, "union");
AssertType(foo(), "string");
AssertType(foo, "() => string");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Results shoud be same as Expr1 and Expr2
let resultIsAny1 = condString ? exprAny1 : exprAny2;
AssertType(resultIsAny1, "any");
AssertType(condString ? exprAny1 : exprAny2, "any");
AssertType(condString, "string");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean1 = condString ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean1, "boolean");
AssertType(condString ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condString, "string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber1 = condString ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber1, "number");
AssertType(condString ? exprNumber1 : exprNumber2, "number");
AssertType(condString, "string");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString1 = condString ? exprString1 : exprString2;
AssertType(resultIsString1, "string");
AssertType(condString ? exprString1 : exprString2, "string");
AssertType(condString, "string");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject1 = condString ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject1, "Object");
AssertType(condString ? exprIsObject1 : exprIsObject2, "Object");
AssertType(condString, "string");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean1 = condString ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean1, "union");
AssertType(condString ? exprString1 : exprBoolean1, "union");
AssertType(condString, "string");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsAny2 = "" ? exprAny1 : exprAny2;
AssertType(resultIsAny2, "any");
AssertType("" ? exprAny1 : exprAny2, "any");
AssertType("", "string");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean2 = "string" ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean2, "boolean");
AssertType("string" ? exprBoolean1 : exprBoolean2, "boolean");
AssertType("string", "string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber2 = 'c' ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber2, "number");
AssertType('c' ? exprNumber1 : exprNumber2, "number");
AssertType('c', "string");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString2 = 'string' ? exprString1 : exprString2;
AssertType(resultIsString2, "string");
AssertType('string' ? exprString1 : exprString2, "string");
AssertType('string', "string");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject2 = "  " ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject2, "Object");
AssertType("  " ? exprIsObject1 : exprIsObject2, "Object");
AssertType("  ", "string");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean2 = "hello" ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean2, "union");
AssertType("hello" ? exprString1 : exprBoolean1, "union");
AssertType("hello", "string");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsAny3 = typeof condString ? exprAny1 : exprAny2;
AssertType(resultIsAny3, "any");
AssertType(typeof condString ? exprAny1 : exprAny2, "any");
AssertType(typeof condString, "union");
AssertType(condString, "string");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean3 = condString.toUpperCase ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean3, "boolean");
AssertType(condString.toUpperCase ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condString.toUpperCase, "() => string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber3 = condString + "string" ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber3, "number");
AssertType(condString + "string" ? exprNumber1 : exprNumber2, "number");
AssertType(condString + "string", "string");
AssertType(condString, "string");
AssertType("string", "string");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString3 = foo() ? exprString1 : exprString2;
AssertType(resultIsString3, "string");
AssertType(foo() ? exprString1 : exprString2, "string");
AssertType(foo(), "string");
AssertType(foo, "() => string");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject3 = array[1] ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject3, "Object");
AssertType(array[1] ? exprIsObject1 : exprIsObject2, "Object");
AssertType(array[1], "string");
AssertType(array, "string[]");
AssertType(1, "int");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean3 = typeof condString ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean3, "union");
AssertType(typeof condString ? exprString1 : exprBoolean1, "union");
AssertType(typeof condString, "union");
AssertType(condString, "string");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsStringOrBoolean4 = condString.toUpperCase ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean4, "union");
AssertType(condString.toUpperCase ? exprString1 : exprBoolean1, "union");
AssertType(condString.toUpperCase, "() => string");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");


