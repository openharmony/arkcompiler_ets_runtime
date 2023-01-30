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

// === tests/cases/conformance/expressions/conditonalOperator/conditionalOperatorWithIdenticalBCT.ts ===
declare function AssertType(value:any, type:string):void;
//Cond ? Expr1 : Expr2,  Expr1 and Expr2 have identical best common type
class X { propertyX: any; propertyX1: number; propertyX2: string };
class A extends X { propertyA: number };
class B extends X { propertyB: string };

let x: X;
AssertType(x, "X");

let a: A;
AssertType(a, "A");

let b: B;
AssertType(b, "B");

//Cond ? Expr1 : Expr2,  Expr1 is supertype
//Be Not contextually typed
true ? x : a;
AssertType(true ? x : a, "X");
AssertType(true, "boolean");
AssertType(x, "X");
AssertType(a, "A");

let result1 = true ? x : a;
AssertType(result1, "X");
AssertType(true ? x : a, "X");
AssertType(true, "boolean");
AssertType(x, "X");
AssertType(a, "A");

//Expr1 and Expr2 are literals
true ? {} : 1;
AssertType(true ? {} : 1, "union");
AssertType(true, "boolean");
AssertType({}, "{}");
AssertType(1, "int");

true ? { a: 1 } : { a: 2, b: 'string' };
AssertType(true ? { a: 1 } : { a: 2, b: 'string' }, "union");
AssertType(true, "boolean");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType({ a: 2, b: 'string' }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(2, "int");
AssertType(b, "string");
AssertType('string', "string");

let result2 = true ? {} : 1;
AssertType(result2, "{}");
AssertType(true ? {} : 1, "union");
AssertType(true, "boolean");
AssertType({}, "{}");
AssertType(1, "int");

let result3 = true ? { a: 1 } : { a: 2, b: 'string' };
AssertType(result3, "union");
AssertType(true ? { a: 1 } : { a: 2, b: 'string' }, "union");
AssertType(true, "boolean");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType({ a: 2, b: 'string' }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(2, "int");
AssertType(b, "string");
AssertType('string', "string");

//Contextually typed
let resultIsX1: X = true ? x : a;
AssertType(resultIsX1, "X");
AssertType(true ? x : a, "X");
AssertType(true, "boolean");
AssertType(x, "X");
AssertType(a, "A");

let result4: (t: A) => any = true ? (m) => m.propertyX : (n) => n.propertyA;
AssertType(result4, "(A) => any");
AssertType(t, "A");
AssertType(true ? (m) => m.propertyX : (n) => n.propertyA, "(A) => any");
AssertType(true, "boolean");
AssertType((m) => m.propertyX, "(A) => any");
AssertType(m, "A");
AssertType(m.propertyX, "any");
AssertType((n) => n.propertyA, "(A) => number");
AssertType(n, "A");
AssertType(n.propertyA, "number");

//Cond ? Expr1 : Expr2,  Expr2 is supertype
//Be Not contextually typed
true ? a : x;
AssertType(true ? a : x, "X");
AssertType(true, "boolean");
AssertType(a, "A");
AssertType(x, "X");

let result5 = true ? a : x;
AssertType(result5, "X");
AssertType(true ? a : x, "X");
AssertType(true, "boolean");
AssertType(a, "A");
AssertType(x, "X");

//Expr1 and Expr2 are literals
true ? 1 : {};
AssertType(true ? 1 : {}, "union");
AssertType(true, "boolean");
AssertType(1, "int");
AssertType({}, "{}");

true ? { a: 2, b: 'string' } : { a: 1 };
AssertType(true ? { a: 2, b: 'string' } : { a: 1 }, "union");
AssertType(true, "boolean");
AssertType({ a: 2, b: 'string' }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(2, "int");
AssertType(b, "string");
AssertType('string', "string");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

let result6 = true ? 1 : {};
AssertType(result6, "{}");
AssertType(true ? 1 : {}, "union");
AssertType(true, "boolean");
AssertType(1, "int");
AssertType({}, "{}");

let result7 = true ? { a: 2, b: 'string' } : { a: 1 };
AssertType(result7, "union");
AssertType(true ? { a: 2, b: 'string' } : { a: 1 }, "union");
AssertType(true, "boolean");
AssertType({ a: 2, b: 'string' }, "{ a: number; b: string; }");
AssertType(a, "number");
AssertType(2, "int");
AssertType(b, "string");
AssertType('string', "string");
AssertType({ a: 1 }, "{ a: number; }");
AssertType(a, "number");
AssertType(1, "int");

//Contextually typed
let resultIsX2: X = true ? x : a;
AssertType(resultIsX2, "X");
AssertType(true ? x : a, "X");
AssertType(true, "boolean");
AssertType(x, "X");
AssertType(a, "A");

let result8: (t: A) => any = true ? (m) => m.propertyA : (n) => n.propertyX;
AssertType(result8, "(A) => any");
AssertType(t, "A");
AssertType(true ? (m) => m.propertyA : (n) => n.propertyX, "(A) => any");
AssertType(true, "boolean");
AssertType((m) => m.propertyA, "(A) => number");
AssertType(m, "A");
AssertType(m.propertyA, "number");
AssertType((n) => n.propertyX, "(A) => any");
AssertType(n, "A");
AssertType(n.propertyX, "any");

//Result = Cond ? Expr1 : Expr2,  Result is supertype
//Contextually typed
let resultIsX3: X = true ? a : b;
AssertType(resultIsX3, "X");
AssertType(true ? a : b, "union");
AssertType(true, "boolean");
AssertType(a, "A");
AssertType(b, "B");

let result10: (t: X) => any = true ? (m) => m.propertyX1 : (n) => n.propertyX2;
AssertType(result10, "(X) => any");
AssertType(t, "X");
AssertType(true ? (m) => m.propertyX1 : (n) => n.propertyX2, "union");
AssertType(true, "boolean");
AssertType((m) => m.propertyX1, "(X) => number");
AssertType(m, "X");
AssertType(m.propertyX1, "number");
AssertType((n) => n.propertyX2, "(X) => string");
AssertType(n, "X");
AssertType(n.propertyX2, "string");

//Expr1 and Expr2 are literals
let result11: any = true ? 1 : 'string';
AssertType(result11, "any");
AssertType(true ? 1 : 'string', "union");
AssertType(true, "boolean");
AssertType(1, "int");
AssertType('string', "string");


