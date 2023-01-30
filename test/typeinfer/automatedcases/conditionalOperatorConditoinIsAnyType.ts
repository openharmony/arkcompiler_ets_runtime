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

// === tests/cases/conformance/expressions/conditonalOperator/conditionalOperatorConditoinIsAnyType.ts ===
declare function AssertType(value:any, type:string):void;
//Cond ? Expr1 : Expr2,  Cond is of any type, Expr1 and Expr2 have the same type
let condAny: any;
AssertType(condAny, "any");

let x: any;
AssertType(x, "any");

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

//Cond is an any type letiable
condAny ? exprAny1 : exprAny2;
AssertType(condAny ? exprAny1 : exprAny2, "any");
AssertType(condAny, "any");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

condAny ? exprBoolean1 : exprBoolean2;
AssertType(condAny ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condAny, "any");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

condAny ? exprNumber1 : exprNumber2;
AssertType(condAny ? exprNumber1 : exprNumber2, "number");
AssertType(condAny, "any");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

condAny ? exprString1 : exprString2;
AssertType(condAny ? exprString1 : exprString2, "string");
AssertType(condAny, "any");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

condAny ? exprIsObject1 : exprIsObject2;
AssertType(condAny ? exprIsObject1 : exprIsObject2, "Object");
AssertType(condAny, "any");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

condAny ? exprString1 : exprBoolean1; // union
AssertType(condAny ? exprString1 : exprBoolean1, "union");
AssertType(condAny, "any");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Cond is an any type literal
null ? exprAny1 : exprAny2;
AssertType(null ? exprAny1 : exprAny2, "any");
AssertType(null, "null");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

null ? exprBoolean1 : exprBoolean2;
AssertType(null ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(null, "null");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

undefined ? exprNumber1 : exprNumber2;
AssertType(undefined ? exprNumber1 : exprNumber2, "number");
AssertType(undefined, "undefined");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

[null, undefined] ? exprString1 : exprString2;
AssertType([null, undefined] ? exprString1 : exprString2, "string");
AssertType([null, undefined], "null[]");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

[null, undefined] ? exprIsObject1 : exprIsObject2;
AssertType([null, undefined] ? exprIsObject1 : exprIsObject2, "Object");
AssertType([null, undefined], "null[]");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

undefined ? exprString1 : exprBoolean1; // union
AssertType(undefined ? exprString1 : exprBoolean1, "union");
AssertType(undefined, "undefined");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Cond is an any type expression
x.doSomeThing() ? exprAny1 : exprAny2;
AssertType(x.doSomeThing() ? exprAny1 : exprAny2, "any");
AssertType(x.doSomeThing(), "any");
AssertType(x.doSomeThing, "any");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

x("x") ? exprBoolean1 : exprBoolean2;
AssertType(x("x") ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(x("x"), "any");
AssertType(x, "any");
AssertType("x", "string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

x(x) ? exprNumber1 : exprNumber2;
AssertType(x(x) ? exprNumber1 : exprNumber2, "number");
AssertType(x(x), "any");
AssertType(x, "any");
AssertType(x, "any");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

x("x") ? exprString1 : exprString2;
AssertType(x("x") ? exprString1 : exprString2, "string");
AssertType(x("x"), "any");
AssertType(x, "any");
AssertType("x", "string");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

x.doSomeThing() ? exprIsObject1 : exprIsObject2;
AssertType(x.doSomeThing() ? exprIsObject1 : exprIsObject2, "Object");
AssertType(x.doSomeThing(), "any");
AssertType(x.doSomeThing, "any");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

x.doSomeThing() ? exprString1 : exprBoolean1; // union
AssertType(x.doSomeThing() ? exprString1 : exprBoolean1, "union");
AssertType(x.doSomeThing(), "any");
AssertType(x.doSomeThing, "any");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

//Results shoud be same as Expr1 and Expr2
let resultIsAny1 = condAny ? exprAny1 : exprAny2;
AssertType(resultIsAny1, "any");
AssertType(condAny ? exprAny1 : exprAny2, "any");
AssertType(condAny, "any");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean1 = condAny ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean1, "boolean");
AssertType(condAny ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(condAny, "any");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber1 = condAny ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber1, "number");
AssertType(condAny ? exprNumber1 : exprNumber2, "number");
AssertType(condAny, "any");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString1 = condAny ? exprString1 : exprString2;
AssertType(resultIsString1, "string");
AssertType(condAny ? exprString1 : exprString2, "string");
AssertType(condAny, "any");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject1 = condAny ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject1, "Object");
AssertType(condAny ? exprIsObject1 : exprIsObject2, "Object");
AssertType(condAny, "any");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean1 = condAny ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean1, "union");
AssertType(condAny ? exprString1 : exprBoolean1, "union");
AssertType(condAny, "any");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsAny2 = null ? exprAny1 : exprAny2;
AssertType(resultIsAny2, "any");
AssertType(null ? exprAny1 : exprAny2, "any");
AssertType(null, "null");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean2 = null ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean2, "boolean");
AssertType(null ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(null, "null");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber2 = undefined ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber2, "number");
AssertType(undefined ? exprNumber1 : exprNumber2, "number");
AssertType(undefined, "undefined");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString2 = [null, undefined] ? exprString1 : exprString2;
AssertType(resultIsString2, "string");
AssertType([null, undefined] ? exprString1 : exprString2, "string");
AssertType([null, undefined], "null[]");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject2 = [null, undefined] ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject2, "Object");
AssertType([null, undefined] ? exprIsObject1 : exprIsObject2, "Object");
AssertType([null, undefined], "null[]");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean2 = null ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean2, "union");
AssertType(null ? exprString1 : exprBoolean1, "union");
AssertType(null, "null");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsStringOrBoolean3 = undefined ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean3, "union");
AssertType(undefined ? exprString1 : exprBoolean1, "union");
AssertType(undefined, "undefined");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsStringOrBoolean4 = [null, undefined] ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean4, "union");
AssertType([null, undefined] ? exprString1 : exprBoolean1, "union");
AssertType([null, undefined], "null[]");
AssertType(null, "null");
AssertType(undefined, "undefined");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");

let resultIsAny3 = x.doSomeThing() ? exprAny1 : exprAny2;
AssertType(resultIsAny3, "any");
AssertType(x.doSomeThing() ? exprAny1 : exprAny2, "any");
AssertType(x.doSomeThing(), "any");
AssertType(x.doSomeThing, "any");
AssertType(exprAny1, "any");
AssertType(exprAny2, "any");

let resultIsBoolean3 = x("x") ? exprBoolean1 : exprBoolean2;
AssertType(resultIsBoolean3, "boolean");
AssertType(x("x") ? exprBoolean1 : exprBoolean2, "boolean");
AssertType(x("x"), "any");
AssertType(x, "any");
AssertType("x", "string");
AssertType(exprBoolean1, "boolean");
AssertType(exprBoolean2, "boolean");

let resultIsNumber3 = x(x) ? exprNumber1 : exprNumber2;
AssertType(resultIsNumber3, "number");
AssertType(x(x) ? exprNumber1 : exprNumber2, "number");
AssertType(x(x), "any");
AssertType(x, "any");
AssertType(x, "any");
AssertType(exprNumber1, "number");
AssertType(exprNumber2, "number");

let resultIsString3 = x("x") ? exprString1 : exprString2;
AssertType(resultIsString3, "string");
AssertType(x("x") ? exprString1 : exprString2, "string");
AssertType(x("x"), "any");
AssertType(x, "any");
AssertType("x", "string");
AssertType(exprString1, "string");
AssertType(exprString2, "string");

let resultIsObject3 = x.doSomeThing() ? exprIsObject1 : exprIsObject2;
AssertType(resultIsObject3, "Object");
AssertType(x.doSomeThing() ? exprIsObject1 : exprIsObject2, "Object");
AssertType(x.doSomeThing(), "any");
AssertType(x.doSomeThing, "any");
AssertType(exprIsObject1, "Object");
AssertType(exprIsObject2, "Object");

let resultIsStringOrBoolean5 = x.doSomeThing() ? exprString1 : exprBoolean1; // union
AssertType(resultIsStringOrBoolean5, "union");
AssertType(x.doSomeThing() ? exprString1 : exprBoolean1, "union");
AssertType(x.doSomeThing(), "any");
AssertType(x.doSomeThing, "any");
AssertType(exprString1, "string");
AssertType(exprBoolean1, "boolean");


