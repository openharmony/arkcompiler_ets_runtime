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

// === tests/cases/conformance/expressions/unaryOperators/incrementOperator/incrementOperatorWithAnyOtherType.ts ===
declare function AssertType(value:any, type:string):void;
// ++ operator on any type

let ANY: any;
AssertType(ANY, "any");

let ANY1: any;
AssertType(ANY1, "any");

let ANY2: any[] = ["", ""];
AssertType(ANY2, "any[]");
AssertType(["", ""], "string[]");
AssertType("", "string");
AssertType("", "string");

let obj = {x:1,y:null};
AssertType(obj, "{ x: number; y: any; }");
AssertType({x:1,y:null}, "{ x: number; y: null; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "null");
AssertType(null, "null");

class A {
    public a: any;
}
module M {
    export let n: any;
}
let objA = new A();
AssertType(objA, "A");
AssertType(new A(), "A");
AssertType(A, "typeof A");

// any type let
let ResultIsNumber1 = ++ANY;
AssertType(ResultIsNumber1, "number");
AssertType(++ANY, "number");
AssertType(ANY, "any");

let ResultIsNumber2 = ++ANY1;
AssertType(ResultIsNumber2, "number");
AssertType(++ANY1, "number");
AssertType(ANY1, "any");

let ResultIsNumber3 = ANY1++;
AssertType(ResultIsNumber3, "number");
AssertType(ANY1++, "number");
AssertType(ANY1, "any");

let ResultIsNumber4 = ANY1++;
AssertType(ResultIsNumber4, "number");
AssertType(ANY1++, "number");
AssertType(ANY1, "any");

// expressions
let ResultIsNumber5 = ++ANY2[0];
AssertType(ResultIsNumber5, "number");
AssertType(++ANY2[0], "number");
AssertType(ANY2[0], "any");
AssertType(ANY2, "any[]");
AssertType(0, "int");

let ResultIsNumber6 = ++obj.x;
AssertType(ResultIsNumber6, "number");
AssertType(++obj.x, "number");
AssertType(obj.x, "number");

let ResultIsNumber7 = ++obj.y;
AssertType(ResultIsNumber7, "number");
AssertType(++obj.y, "number");
AssertType(obj.y, "any");

let ResultIsNumber8 = ++objA.a;
AssertType(ResultIsNumber8, "number");
AssertType(++objA.a, "number");
AssertType(objA.a, "any");

let ResultIsNumber = ++M.n;
AssertType(ResultIsNumber, "number");
AssertType(++M.n, "number");
AssertType(M.n, "any");

let ResultIsNumber9 = ANY2[0]++;
AssertType(ResultIsNumber9, "number");
AssertType(ANY2[0]++, "number");
AssertType(ANY2[0], "any");
AssertType(ANY2, "any[]");
AssertType(0, "int");

let ResultIsNumber10 = obj.x++;
AssertType(ResultIsNumber10, "number");
AssertType(obj.x++, "number");
AssertType(obj.x, "number");

let ResultIsNumber11 = obj.y++;
AssertType(ResultIsNumber11, "number");
AssertType(obj.y++, "number");
AssertType(obj.y, "any");

let ResultIsNumber12 = objA.a++;
AssertType(ResultIsNumber12, "number");
AssertType(objA.a++, "number");
AssertType(objA.a, "any");

let ResultIsNumber13 = M.n++;
AssertType(ResultIsNumber13, "number");
AssertType(M.n++, "number");
AssertType(M.n, "any");

// miss assignment opertors
++ANY;
AssertType(++ANY, "number");
AssertType(ANY, "any");

++ANY1;
AssertType(++ANY1, "number");
AssertType(ANY1, "any");

++ANY2[0];
AssertType(++ANY2[0], "number");
AssertType(ANY2[0], "any");
AssertType(ANY2, "any[]");
AssertType(0, "int");

++ANY, ++ANY1;
AssertType(++ANY, ++ANY1, "number");
AssertType(++ANY, "number");
AssertType(ANY, "any");
AssertType(++ANY1, "number");
AssertType(ANY1, "any");

++objA.a;
AssertType(++objA.a, "number");
AssertType(objA.a, "any");

++M.n;
AssertType(++M.n, "number");
AssertType(M.n, "any");

ANY++;
AssertType(ANY++, "number");
AssertType(ANY, "any");

ANY1++;
AssertType(ANY1++, "number");
AssertType(ANY1, "any");

ANY2[0]++;
AssertType(ANY2[0]++, "number");
AssertType(ANY2[0], "any");
AssertType(ANY2, "any[]");
AssertType(0, "int");

ANY++, ANY1++;
AssertType(ANY++, ANY1++, "number");
AssertType(ANY++, "number");
AssertType(ANY, "any");
AssertType(ANY1++, "number");
AssertType(ANY1, "any");

objA.a++;
AssertType(objA.a++, "number");
AssertType(objA.a, "any");

M.n++;
AssertType(M.n++, "number");
AssertType(M.n, "any");


