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

// === tests/cases/compiler/noImplicitAnyDestructuringVarDeclaration2.ts ===
declare function AssertType(value:any, type:string):void;
let [a, b, c] = [1, 2, 3]; // no error
AssertType(a, "number");
AssertType(b, "number");
AssertType(c, "number");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [a1 = 10, b1 = 10, c1 = 10] = [1, 2, 3]; // no error
AssertType(a1, "number");
AssertType(10, "int");
AssertType(b1, "number");
AssertType(10, "int");
AssertType(c1, "number");
AssertType(10, "int");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [a2 = undefined, b2 = undefined, c2 = undefined] = [1, 2, 3]; // no error
AssertType(a2, "number");
AssertType(undefined, "undefined");
AssertType(b2, "number");
AssertType(undefined, "undefined");
AssertType(c2, "number");
AssertType(undefined, "undefined");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [a3 = <any>undefined, b3 = <any>null, c3 = <any>undefined] = [1, 2, 3]; // no error
AssertType(a3, "any");
AssertType(<any>undefined, "any");
AssertType(undefined, "undefined");
AssertType(b3, "any");
AssertType(<any>null, "any");
AssertType(null, "null");
AssertType(c3, "any");
AssertType(<any>undefined, "any");
AssertType(undefined, "undefined");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let [a4] = [<any>undefined], [b4] = [<any>null], c4 = <any>undefined, d4 = <any>null; // no error
AssertType(a4, "any");
AssertType([<any>undefined], "[any]");
AssertType(<any>undefined, "any");
AssertType(undefined, "undefined");
AssertType(b4, "any");
AssertType([<any>null], "[any]");
AssertType(<any>null, "any");
AssertType(null, "null");
AssertType(c4, "any");
AssertType(<any>undefined, "any");
AssertType(undefined, "undefined");
AssertType(d4, "any");
AssertType(<any>null, "any");
AssertType(null, "null");

let {x, y, z} = { x: 1, y: 2, z: 3 }; // no error
AssertType(x, "number");
AssertType(y, "number");
AssertType(z, "number");
AssertType({ x: 1, y: 2, z: 3 }, "{ x: number; y: number; z: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(2, "int");
AssertType(z, "number");
AssertType(3, "int");

let {x1 = 10, y1 = 10, z1 = 10} = { x1: 1, y1: 2, z1: 3 }; // no error
AssertType(x1, "number");
AssertType(10, "int");
AssertType(y1, "number");
AssertType(10, "int");
AssertType(z1, "number");
AssertType(10, "int");
AssertType({ x1: 1, y1: 2, z1: 3 }, "{ x1?: number; y1?: number; z1?: number; }");
AssertType(x1, "number");
AssertType(1, "int");
AssertType(y1, "number");
AssertType(2, "int");
AssertType(z1, "number");
AssertType(3, "int");

let {x2 = undefined, y2 = undefined, z2 = undefined} = { x2: 1, y2: 2, z2: 3 }; // no error
AssertType(x2, "number");
AssertType(undefined, "undefined");
AssertType(y2, "number");
AssertType(undefined, "undefined");
AssertType(z2, "number");
AssertType(undefined, "undefined");
AssertType({ x2: 1, y2: 2, z2: 3 }, "{ x2?: number; y2?: number; z2?: number; }");
AssertType(x2, "number");
AssertType(1, "int");
AssertType(y2, "number");
AssertType(2, "int");
AssertType(z2, "number");
AssertType(3, "int");

let {x3 = <any>undefined, y3 = <any>null, z3 = <any>undefined} = { x3: 1, y3: 2, z3: 3 }; // no error
AssertType(x3, "any");
AssertType(<any>undefined, "any");
AssertType(undefined, "undefined");
AssertType(y3, "any");
AssertType(<any>null, "any");
AssertType(null, "null");
AssertType(z3, "any");
AssertType(<any>undefined, "any");
AssertType(undefined, "undefined");
AssertType({ x3: 1, y3: 2, z3: 3 }, "{ x3?: number; y3?: number; z3?: number; }");
AssertType(x3, "number");
AssertType(1, "int");
AssertType(y3, "number");
AssertType(2, "int");
AssertType(z3, "number");
AssertType(3, "int");

let {x4} = { x4: <any>undefined }, {y4} = { y4: <any>null }; // no error
AssertType(x4, "any");
AssertType({ x4: <any>undefined }, "{ x4: any; }");
AssertType(x4, "any");
AssertType(<any>undefined, "any");
AssertType(undefined, "undefined");
AssertType(y4, "any");
AssertType({ y4: <any>null }, "{ y4: any; }");
AssertType(y4, "any");
AssertType(<any>null, "any");
AssertType(null, "null");


