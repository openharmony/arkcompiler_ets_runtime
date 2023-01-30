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

// === tests/cases/conformance/es6/destructuring/destructuringVariableDeclaration1ES5iterable.ts ===
declare function AssertType(value:any, type:string):void;
// The type T associated with a destructuring letiable declaration is determined as follows:
//      If the declaration includes a type annotation, T is that type.
let {a1, a2}: { a1: number, a2: string } = { a1: 10, a2: "world" 
AssertType(a1, "number");

AssertType(a2, "string");

AssertType(a1, "number");

AssertType(a2, "string");

AssertType({ a1: 10, a2: "world" }, "{ a1: number; a2: string; }");

AssertType(a1, "number");

AssertType(10, "int");

AssertType(a2, "string");

AssertType("world", "string");
}

let [a3, [[a4]], a5]: [number, [[string]], boolean] = [1, [["hello"]], true];
AssertType(a3, "number");
AssertType(a4, "string");
AssertType(a5, "boolean");
AssertType([1, [["hello"]], true], "[number, [[string]], true]");
AssertType(1, "int");
AssertType([["hello"]], "[[string]]");
AssertType(["hello"], "[string]");
AssertType("hello", "string");
AssertType(true, "boolean");

// The type T associated with a destructuring letiable declaration is determined as follows:
//      Otherwise, if the declaration includes an initializer expression, T is the type of that initializer expression.
let { b1: { b11 } = { b11: "string" }  } = { b1: { b11: "world" } };
AssertType(b1, "any");
AssertType(b11, "string");
AssertType({ b11: "string" }, "{ b11: string; }");
AssertType(b11, "string");
AssertType("string", "string");
AssertType({ b1: { b11: "world" } }, "{ b1?: { b11: string; }; }");
AssertType(b1, "{ b11: string; }");
AssertType({ b11: "world" }, "{ b11: string; }");
AssertType(b11, "string");
AssertType("world", "string");

let temp = { t1: true, t2: "false" };
AssertType(temp, "{ t1: boolean; t2: string; }");
AssertType({ t1: true, t2: "false" }, "{ t1: boolean; t2: string; }");
AssertType(t1, "boolean");
AssertType(true, "boolean");
AssertType(t2, "string");
AssertType("false", "string");

let [b2 = 3, b3 = true, b4 = temp] = [3, false, { t1: false, t2: "hello" }];
AssertType(b2, "number");
AssertType(3, "int");
AssertType(b3, "boolean");
AssertType(true, "boolean");
AssertType(b4, "{ t1: boolean; t2: string; }");
AssertType(temp, "{ t1: boolean; t2: string; }");
AssertType([3, false, { t1: false, t2: "hello" }], "[number, false, { t1: false; t2: string; }]");
AssertType(3, "int");
AssertType(false, "boolean");
AssertType({ t1: false, t2: "hello" }, "{ t1: false; t2: string; }");
AssertType(t1, "boolean");
AssertType(false, "boolean");
AssertType(t2, "string");
AssertType("hello", "string");

let [b5 = 3, b6 = true, b7 = temp] = [undefined, undefined, undefined];
AssertType(b5, "number");
AssertType(3, "int");
AssertType(b6, "boolean");
AssertType(true, "boolean");
AssertType(b7, "{ t1: boolean; t2: string; }");
AssertType(temp, "{ t1: boolean; t2: string; }");
AssertType([undefined, undefined, undefined], "[undefined, undefined, undefined]");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");
AssertType(undefined, "undefined");

// The type T associated with a binding element is determined as follows:
//      If the binding element is a rest element, T is an array type with
//          an element type E, where E is the type of the numeric index signature of S.
let [...c1] = [1,2,3];
AssertType(c1, "number[]");
AssertType([1,2,3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [...c2] = [1,2,3, "string"];
AssertType(c2, "(union)[]");
AssertType([1,2,3, "string"], "(union)[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType("string", "string");

// The type T associated with a binding element is determined as follows:
//      Otherwise, if S is a tuple- like type (section 3.3.3):
//          	Let N be the zero-based index of the binding element in the array binding pattern.
// 	            If S has a property with the numerical name N, T is the type of that property.
let [d1,d2] = [1,"string"]
AssertType(d1, "number");
AssertType(d2, "string");
AssertType([1,"string"], "[number, string]");
AssertType(1, "int");
AssertType("string", "string");

// The type T associated with a binding element is determined as follows:
//      Otherwise, if S is a tuple- like type (section 3.3.3):
//              Otherwise, if S has a numeric index signature, T is the type of the numeric index signature.
let temp1 = [true, false, true]
AssertType(temp1, "boolean[]");
AssertType([true, false, true], "boolean[]");
AssertType(true, "boolean");
AssertType(false, "boolean");
AssertType(true, "boolean");

let [d3, d4] = [1, "string", ...temp1];
AssertType(d3, "number");
AssertType(d4, "string");
AssertType([1, "string", ...temp1], "[number, string, ...boolean[]]");
AssertType(1, "int");
AssertType("string", "string");
AssertType(...temp1, "boolean");
AssertType(temp1, "boolean[]");

//  Combining both forms of destructuring,
let {e: [e1, e2, e3 = { b1: 1000, b4: 200 }]} = { e: [1, 2, { b1: 4, b4: 0 }] };
AssertType(e, "any");
AssertType(e1, "number");
AssertType(e2, "number");
AssertType(e3, "{ b1: number; b4: number; }");
AssertType({ b1: 1000, b4: 200 }, "{ b1: number; b4: number; }");
AssertType(b1, "number");
AssertType(1000, "int");
AssertType(b4, "number");
AssertType(200, "int");
AssertType({ e: [1, 2, { b1: 4, b4: 0 }] }, "{ e: [number, number, { b1: number; b4: number; }]; }");
AssertType(e, "[number, number, { b1: number; b4: number; }]");
AssertType([1, 2, { b1: 4, b4: 0 }], "[number, number, { b1: number; b4: number; }]");
AssertType(1, "int");
AssertType(2, "int");
AssertType({ b1: 4, b4: 0 }, "{ b1: number; b4: number; }");
AssertType(b1, "number");
AssertType(4, "int");
AssertType(b4, "number");
AssertType(0, "int");

let {f: [f1, f2, { f3: f4, f5 }, , ]} = { f: [1, 2, { f3: 4, f5: 0 }] };
AssertType(f, "any");
AssertType(f1, "number");
AssertType(f2, "number");
AssertType(f3, "any");
AssertType(f4, "number");
AssertType(f5, "number");
AssertType(, "undefined");
AssertType({ f: [1, 2, { f3: 4, f5: 0 }] }, "{ f: [number, number, { f3: number; f5: number; }]; }");
AssertType(f, "[number, number, { f3: number; f5: number; }]");
AssertType([1, 2, { f3: 4, f5: 0 }], "[number, number, { f3: number; f5: number; }]");
AssertType(1, "int");
AssertType(2, "int");
AssertType({ f3: 4, f5: 0 }, "{ f3: number; f5: number; }");
AssertType(f3, "number");
AssertType(4, "int");
AssertType(f5, "number");
AssertType(0, "int");

// When a destructuring letiable declaration, binding property, or binding element specifies
// an initializer expression, the type of the initializer expression is required to be assignable
// to the widened form of the type associated with the destructuring letiable declaration, binding property, or binding element.
let {g: {g1 = [undefined, null]}}: { g: { g1: any[] } } = { g: { g1: [1, 2] } };
AssertType(g, "any");
AssertType(g1, "any[]");
AssertType([undefined, null], "null[]");
AssertType(undefined, "undefined");
AssertType(null, "null");
AssertType(g, "{ g1: any[]; }");
AssertType(g1, "any[]");
AssertType({ g: { g1: [1, 2] } }, "{ g: { g1: number[]; }; }");
AssertType(g, "{ g1: number[]; }");
AssertType({ g1: [1, 2] }, "{ g1: number[]; }");
AssertType(g1, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");

let {h: {h1 = [undefined, null]}}: { h: { h1: number[] } } = { h: { h1: [1, 2] } };
AssertType(h, "any");
AssertType(h1, "number[]");
AssertType([undefined, null], "null[]");
AssertType(undefined, "undefined");
AssertType(null, "null");
AssertType(h, "{ h1: number[]; }");
AssertType(h1, "number[]");
AssertType({ h: { h1: [1, 2] } }, "{ h: { h1: number[]; }; }");
AssertType(h, "{ h1: number[]; }");
AssertType({ h1: [1, 2] }, "{ h1: number[]; }");
AssertType(h1, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");



