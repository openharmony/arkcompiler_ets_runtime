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

// === tests/cases/conformance/expressions/unaryOperators/voidOperator/voidOperatorWithNumberType.ts ===
declare function AssertType(value:any, type:string):void;
// void  operator on number type
let NUMBER: number;
AssertType(NUMBER, "number");

let NUMBER1: number[] = [1, 2];
AssertType(NUMBER1, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

function foo(): number { 
AssertType(1, "int");
return 1; 
}

class A {
    public a: number;
    static foo() { 
AssertType(1, "int");
return 1; 
}
}
module M {
    export let n: number;
}

let objA = new A();
AssertType(objA, "A");
AssertType(new A(), "A");
AssertType(A, "typeof A");

// number type let
let ResultIsAny1 = void NUMBER;
AssertType(ResultIsAny1, "any");
AssertType(void NUMBER, "undefined");
AssertType(NUMBER, "number");

let ResultIsAny2 = void NUMBER1;
AssertType(ResultIsAny2, "any");
AssertType(void NUMBER1, "undefined");
AssertType(NUMBER1, "number[]");

// number type literal
let ResultIsAny3 = void 1;
AssertType(ResultIsAny3, "any");
AssertType(void 1, "undefined");
AssertType(1, "int");

let ResultIsAny4 = void { x: 1, y: 2};
AssertType(ResultIsAny4, "any");
AssertType(void { x: 1, y: 2}, "undefined");
AssertType({ x: 1, y: 2}, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(2, "int");

let ResultIsAny5 = void { x: 1, y: (n: number) => { 
AssertType(ResultIsAny5, "any");
AssertType(void { x: 1, y: (n: number) => { return n; } }, "undefined");
AssertType({ x: 1, y: (n: number) => { return n; } }, "{ x: number; y: (number) => number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "(number) => number");
AssertType((n: number) => { return n; }, "(number) => number");
AssertType(n, "number");
AssertType(n, "number");
return n; } };

// number type expressions
let ResultIsAny6 = void objA.a;
AssertType(ResultIsAny6, "any");
AssertType(void objA.a, "undefined");
AssertType(objA.a, "number");

let ResultIsAny7 = void M.n;
AssertType(ResultIsAny7, "any");
AssertType(void M.n, "undefined");
AssertType(M.n, "number");

let ResultIsAny8 = void NUMBER1[0];
AssertType(ResultIsAny8, "any");
AssertType(void NUMBER1[0], "undefined");
AssertType(NUMBER1[0], "number");
AssertType(NUMBER1, "number[]");
AssertType(0, "int");

let ResultIsAny9 = void foo();
AssertType(ResultIsAny9, "any");
AssertType(void foo(), "undefined");
AssertType(foo(), "number");
AssertType(foo, "() => number");

let ResultIsAny10 = void A.foo();
AssertType(ResultIsAny10, "any");
AssertType(void A.foo(), "undefined");
AssertType(A.foo(), "number");
AssertType(A.foo, "() => number");

let ResultIsAny11 = void (NUMBER + NUMBER);
AssertType(ResultIsAny11, "any");
AssertType(void (NUMBER + NUMBER), "undefined");
AssertType((NUMBER + NUMBER), "number");
AssertType(NUMBER + NUMBER, "number");
AssertType(NUMBER, "number");
AssertType(NUMBER, "number");

// multiple void  operators
let ResultIsAny12 = void void NUMBER;
AssertType(ResultIsAny12, "any");
AssertType(void void NUMBER, "undefined");
AssertType(void NUMBER, "undefined");
AssertType(NUMBER, "number");

let ResultIsAny13 = void void void (NUMBER + NUMBER);
AssertType(ResultIsAny13, "any");
AssertType(void void void (NUMBER + NUMBER), "undefined");
AssertType(void void (NUMBER + NUMBER), "undefined");
AssertType(void (NUMBER + NUMBER), "undefined");
AssertType((NUMBER + NUMBER), "number");
AssertType(NUMBER + NUMBER, "number");
AssertType(NUMBER, "number");
AssertType(NUMBER, "number");

// miss assignment operators
void 1;
AssertType(void 1, "undefined");
AssertType(1, "int");

void NUMBER;
AssertType(void NUMBER, "undefined");
AssertType(NUMBER, "number");

void NUMBER1;
AssertType(void NUMBER1, "undefined");
AssertType(NUMBER1, "number[]");

void foo();
AssertType(void foo(), "undefined");
AssertType(foo(), "number");
AssertType(foo, "() => number");

void objA.a;
AssertType(void objA.a, "undefined");
AssertType(objA.a, "number");

void M.n;
AssertType(void M.n, "undefined");
AssertType(M.n, "number");

void objA.a, M.n;
AssertType(void objA.a, M.n, "number");
AssertType(void objA.a, "undefined");
AssertType(objA.a, "number");
AssertType(M.n, "number");


