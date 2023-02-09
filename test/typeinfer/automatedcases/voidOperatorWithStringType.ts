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

// === tests/cases/conformance/expressions/unaryOperators/voidOperator/voidOperatorWithStringType.ts ===
declare function AssertType(value:any, type:string):void;
// void  operator on string type
let STRING: string;
AssertType(STRING, "string");

let STRING1: string[] = ["", "abc"];
AssertType(STRING1, "string[]");
AssertType(["", "abc"], "string[]");
AssertType("", "string");
AssertType("abc", "string");

function foo(): string { 
AssertType("abc", "string");
return "abc"; 
}

class A {
    public a: string;
    static foo() { 
AssertType("", "string");
return ""; 
}
}
module M {
    export let n: string;
}

let objA = new A();
AssertType(objA, "A");
AssertType(new A(), "A");
AssertType(A, "typeof A");

// string type let
let ResultIsAny1 = void STRING;
AssertType(ResultIsAny1, "any");
AssertType(void STRING, "undefined");
AssertType(STRING, "string");

let ResultIsAny2 = void STRING1;
AssertType(ResultIsAny2, "any");
AssertType(void STRING1, "undefined");
AssertType(STRING1, "string[]");

// string type literal
let ResultIsAny3 = void "";
AssertType(ResultIsAny3, "any");
AssertType(void "", "undefined");
AssertType("", "string");

let ResultIsAny4 = void { x: "", y: "" };
AssertType(ResultIsAny4, "any");
AssertType(void { x: "", y: "" }, "undefined");
AssertType({ x: "", y: "" }, "{ x: string; y: string; }");
AssertType(x, "string");
AssertType("", "string");
AssertType(y, "string");
AssertType("", "string");

let ResultIsAny5 = void { x: "", y: (s: string) => { 
AssertType(ResultIsAny5, "any");
AssertType(void { x: "", y: (s: string) => { return s; } }, "undefined");
AssertType({ x: "", y: (s: string) => { return s; } }, "{ x: string; y: (string) => string; }");
AssertType(x, "string");
AssertType("", "string");
AssertType(y, "(string) => string");
AssertType((s: string) => { return s; }, "(string) => string");
AssertType(s, "string");
AssertType(s, "string");
return s; } };

// string type expressions
let ResultIsAny6 = void objA.a;
AssertType(ResultIsAny6, "any");
AssertType(void objA.a, "undefined");
AssertType(objA.a, "string");

let ResultIsAny7 = void M.n;
AssertType(ResultIsAny7, "any");
AssertType(void M.n, "undefined");
AssertType(M.n, "string");

let ResultIsAny8 = void STRING1[0];
AssertType(ResultIsAny8, "any");
AssertType(void STRING1[0], "undefined");
AssertType(STRING1[0], "string");
AssertType(STRING1, "string[]");
AssertType(0, "int");

let ResultIsAny9 = void foo();
AssertType(ResultIsAny9, "any");
AssertType(void foo(), "undefined");
AssertType(foo(), "string");
AssertType(foo, "() => string");

let ResultIsAny10 = void A.foo();
AssertType(ResultIsAny10, "any");
AssertType(void A.foo(), "undefined");
AssertType(A.foo(), "string");
AssertType(A.foo, "() => string");

let ResultIsAny11 = void (STRING + STRING);
AssertType(ResultIsAny11, "any");
AssertType(void (STRING + STRING), "undefined");
AssertType((STRING + STRING), "string");
AssertType(STRING + STRING, "string");
AssertType(STRING, "string");
AssertType(STRING, "string");

let ResultIsAny12 = void STRING.charAt(0);
AssertType(ResultIsAny12, "any");
AssertType(void STRING.charAt(0), "undefined");
AssertType(STRING.charAt(0), "string");
AssertType(STRING.charAt, "(number) => string");
AssertType(0, "int");

// multiple void  operators
let ResultIsAny13 = void void STRING;
AssertType(ResultIsAny13, "any");
AssertType(void void STRING, "undefined");
AssertType(void STRING, "undefined");
AssertType(STRING, "string");

let ResultIsAny14 = void void void (STRING + STRING);
AssertType(ResultIsAny14, "any");
AssertType(void void void (STRING + STRING), "undefined");
AssertType(void void (STRING + STRING), "undefined");
AssertType(void (STRING + STRING), "undefined");
AssertType((STRING + STRING), "string");
AssertType(STRING + STRING, "string");
AssertType(STRING, "string");
AssertType(STRING, "string");

// miss assignment operators
void "";
AssertType(void "", "undefined");
AssertType("", "string");

void STRING;
AssertType(void STRING, "undefined");
AssertType(STRING, "string");

void STRING1;
AssertType(void STRING1, "undefined");
AssertType(STRING1, "string[]");

void foo();
AssertType(void foo(), "undefined");
AssertType(foo(), "string");
AssertType(foo, "() => string");

void objA.a,M.n;
AssertType(void objA.a,M.n, "string");
AssertType(void objA.a, "undefined");
AssertType(objA.a, "string");
AssertType(M.n, "string");


