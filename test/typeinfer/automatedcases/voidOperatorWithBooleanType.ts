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

// === tests/cases/conformance/expressions/unaryOperators/voidOperator/voidOperatorWithBooleanType.ts ===
declare function AssertType(value:any, type:string):void;
// void  operator on boolean type
let BOOLEAN: boolean;
AssertType(BOOLEAN, "boolean");

function foo(): boolean { 
AssertType(true, "boolean");
return true; 
}

class A {
    public a: boolean;
    static foo() { 
AssertType(false, "boolean");
return false; 
}
}
module M {
    export let n: boolean;
}

let objA = new A();
AssertType(objA, "A");
AssertType(new A(), "A");
AssertType(A, "typeof A");

// boolean type let
let ResultIsAny1 = void BOOLEAN;
AssertType(ResultIsAny1, "any");
AssertType(void BOOLEAN, "undefined");
AssertType(BOOLEAN, "boolean");

// boolean type literal
let ResultIsAny2 = void true;
AssertType(ResultIsAny2, "any");
AssertType(void true, "undefined");
AssertType(true, "boolean");

let ResultIsAny3 = void { x: true, y: false };
AssertType(ResultIsAny3, "any");
AssertType(void { x: true, y: false }, "undefined");
AssertType({ x: true, y: false }, "{ x: boolean; y: boolean; }");
AssertType(x, "boolean");
AssertType(true, "boolean");
AssertType(y, "boolean");
AssertType(false, "boolean");

// boolean type expressions
let ResultIsAny4 = void objA.a;
AssertType(ResultIsAny4, "any");
AssertType(void objA.a, "undefined");
AssertType(objA.a, "boolean");

let ResultIsAny5 = void M.n;
AssertType(ResultIsAny5, "any");
AssertType(void M.n, "undefined");
AssertType(M.n, "boolean");

let ResultIsAny6 = void foo();
AssertType(ResultIsAny6, "any");
AssertType(void foo(), "undefined");
AssertType(foo(), "boolean");
AssertType(foo, "() => boolean");

let ResultIsAny7 = void A.foo();
AssertType(ResultIsAny7, "any");
AssertType(void A.foo(), "undefined");
AssertType(A.foo(), "boolean");
AssertType(A.foo, "() => boolean");

// multiple void  operator
let ResultIsAny8 = void void BOOLEAN;
AssertType(ResultIsAny8, "any");
AssertType(void void BOOLEAN, "undefined");
AssertType(void BOOLEAN, "undefined");
AssertType(BOOLEAN, "boolean");

// miss assignment operators
void true;
AssertType(void true, "undefined");
AssertType(true, "boolean");

void BOOLEAN;
AssertType(void BOOLEAN, "undefined");
AssertType(BOOLEAN, "boolean");

void foo();
AssertType(void foo(), "undefined");
AssertType(foo(), "boolean");
AssertType(foo, "() => boolean");

void true, false;
AssertType(void true, false, "boolean");
AssertType(void true, "undefined");
AssertType(true, "boolean");
AssertType(false, "boolean");

void objA.a;
AssertType(void objA.a, "undefined");
AssertType(objA.a, "boolean");

void M.n;
AssertType(void M.n, "undefined");
AssertType(M.n, "boolean");


