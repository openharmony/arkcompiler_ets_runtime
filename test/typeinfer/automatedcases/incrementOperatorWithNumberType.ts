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

// === tests/cases/conformance/expressions/unaryOperators/incrementOperator/incrementOperatorWithNumberType.ts ===
declare function AssertType(value:any, type:string):void;
// ++ operator on number type
let NUMBER: number;
AssertType(NUMBER, "number");

let NUMBER1: number[] = [1, 2];
AssertType(NUMBER1, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

class A {
    public a: number;
}
module M {
    export let n: number;
}

let objA = new A();
AssertType(objA, "A");
AssertType(new A(), "A");
AssertType(A, "typeof A");

// number type let
let ResultIsNumber1 = ++NUMBER;
AssertType(ResultIsNumber1, "number");
AssertType(++NUMBER, "number");
AssertType(NUMBER, "number");

let ResultIsNumber2 = NUMBER++;
AssertType(ResultIsNumber2, "number");
AssertType(NUMBER++, "number");
AssertType(NUMBER, "number");

// expressions
let ResultIsNumber3 = ++objA.a;
AssertType(ResultIsNumber3, "number");
AssertType(++objA.a, "number");
AssertType(objA.a, "number");

let ResultIsNumber4 = ++M.n;
AssertType(ResultIsNumber4, "number");
AssertType(++M.n, "number");
AssertType(M.n, "number");

let ResultIsNumber5 = objA.a++;
AssertType(ResultIsNumber5, "number");
AssertType(objA.a++, "number");
AssertType(objA.a, "number");

let ResultIsNumber6 = M.n++;
AssertType(ResultIsNumber6, "number");
AssertType(M.n++, "number");
AssertType(M.n, "number");

let ResultIsNumber7 = NUMBER1[0]++;
AssertType(ResultIsNumber7, "number");
AssertType(NUMBER1[0]++, "number");
AssertType(NUMBER1[0], "number");
AssertType(NUMBER1, "number[]");
AssertType(0, "int");

// miss assignment operators
++NUMBER;
AssertType(++NUMBER, "number");
AssertType(NUMBER, "number");

++NUMBER1[0];
AssertType(++NUMBER1[0], "number");
AssertType(NUMBER1[0], "number");
AssertType(NUMBER1, "number[]");
AssertType(0, "int");

++objA.a;
AssertType(++objA.a, "number");
AssertType(objA.a, "number");

++M.n;
AssertType(++M.n, "number");
AssertType(M.n, "number");

++objA.a, M.n;
AssertType(++objA.a, M.n, "number");
AssertType(++objA.a, "number");
AssertType(objA.a, "number");
AssertType(M.n, "number");

NUMBER++;
AssertType(NUMBER++, "number");
AssertType(NUMBER, "number");

NUMBER1[0]++;
AssertType(NUMBER1[0]++, "number");
AssertType(NUMBER1[0], "number");
AssertType(NUMBER1, "number[]");
AssertType(0, "int");

objA.a++;
AssertType(objA.a++, "number");
AssertType(objA.a, "number");

M.n++;
AssertType(M.n++, "number");
AssertType(M.n, "number");

objA.a++, M.n++;
AssertType(objA.a++, M.n++, "number");
AssertType(objA.a++, "number");
AssertType(objA.a, "number");
AssertType(M.n++, "number");
AssertType(M.n, "number");


