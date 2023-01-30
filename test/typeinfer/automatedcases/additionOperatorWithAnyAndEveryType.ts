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

// === tests/cases/conformance/expressions/binaryOperators/additionOperator/additionOperatorWithAnyAndEveryType.ts ===
declare function AssertType(value:any, type:string):void;
function foo() { }
class C {
    public a: string;
    static foo() { }
}
enum E { a, b, c }
module M { export let a }

let a: any;
AssertType(a, "any");

let b: boolean;
AssertType(b, "boolean");

let c: number;
AssertType(c, "number");

let d: string;
AssertType(d, "string");

let e: Object;
AssertType(e, "Object");

// any as left operand, result is type Any except plusing string
let r1 = a + a;
AssertType(r1, "any");
AssertType(a + a, "any");
AssertType(a, "any");
AssertType(a, "any");

let r2 = a + b;
AssertType(r2, "any");
AssertType(a + b, "any");
AssertType(a, "any");
AssertType(b, "boolean");

let r3 = a + c;
AssertType(r3, "any");
AssertType(a + c, "any");
AssertType(a, "any");
AssertType(c, "number");

let r4 = a + d;
AssertType(r4, "string");
AssertType(a + d, "string");
AssertType(a, "any");
AssertType(d, "string");

let r5 = a + e;
AssertType(r5, "any");
AssertType(a + e, "any");
AssertType(a, "any");
AssertType(e, "Object");

// any as right operand, result is type Any except plusing string
let r6 = b + a;
AssertType(r6, "any");
AssertType(b + a, "any");
AssertType(b, "boolean");
AssertType(a, "any");

let r7 = c + a;
AssertType(r7, "any");
AssertType(c + a, "any");
AssertType(c, "number");
AssertType(a, "any");

let r8 = d + a;
AssertType(r8, "string");
AssertType(d + a, "string");
AssertType(d, "string");
AssertType(a, "any");

let r9 = e + a;
AssertType(r9, "any");
AssertType(e + a, "any");
AssertType(e, "Object");
AssertType(a, "any");

// other cases
let r10 = a + foo;
AssertType(r10, "any");
AssertType(a + foo, "any");
AssertType(a, "any");
AssertType(foo, "() => void");

let r11 = a + foo();
AssertType(r11, "any");
AssertType(a + foo(), "any");
AssertType(a, "any");
AssertType(foo(), "void");
AssertType(foo, "() => void");

let r12 = a + C;
AssertType(r12, "any");
AssertType(a + C, "any");
AssertType(a, "any");
AssertType(C, "typeof C");

let r13 = a + new C();
AssertType(r13, "any");
AssertType(a + new C(), "any");
AssertType(a, "any");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let r14 = a + E;
AssertType(r14, "any");
AssertType(a + E, "any");
AssertType(a, "any");
AssertType(E, "typeof E");

let r15 = a + E.a;
AssertType(r15, "any");
AssertType(a + E.a, "any");
AssertType(a, "any");
AssertType(E.a, "E.a");

let r16 = a + M;
AssertType(r16, "any");
AssertType(a + M, "any");
AssertType(a, "any");
AssertType(M, "typeof M");

let r17 = a + '';
AssertType(r17, "string");
AssertType(a + '', "string");
AssertType(a, "any");
AssertType('', "string");

let r18 = a + 123;
AssertType(r18, "any");
AssertType(a + 123, "any");
AssertType(a, "any");
AssertType(123, "int");

let r19 = a + { a: '' };
AssertType(r19, "any");
AssertType(a + { a: '' }, "any");
AssertType(a, "any");
AssertType({ a: '' }, "{ a: string; }");
AssertType(a, "string");
AssertType('', "string");

let r20 = a + ((a: string) => { 
AssertType(r20, "any");
AssertType(a + ((a: string) => { return a }), "any");
AssertType(a, "any");
AssertType(((a: string) => { return a }), "(string) => string");
AssertType((a: string) => { return a }, "(string) => string");
AssertType(a, "string");
AssertType(a, "string");
return a });


