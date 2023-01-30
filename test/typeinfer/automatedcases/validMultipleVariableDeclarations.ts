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

// === tests/cases/conformance/statements/VariableStatements/validMultipleVariableDeclarations.ts ===
declare function AssertType(value:any, type:string):void;
// all expected to be valid

let x: number;
AssertType(x, "number");

let x = 2;
AssertType(x, "number");
AssertType(2, "int");

if (true) {
    let x = 3;
AssertType(x, "number");
AssertType(3, "int");

    for (let x = 0; ;) { 
AssertType(x, "number");

AssertType(0, "int");
}
}
let x = <number>undefined;
AssertType(x, "number");
AssertType(<number>undefined, "number");
AssertType(undefined, "undefined");

// new declaration space, making redeclaring x as a string valid
function declSpace() {
    let x = 'this is a string';
AssertType(x, "string");
AssertType('this is a string', "string");
}

interface Point { x: number; y: number; }

let p: Point;
AssertType(p, "Point");

let p = { x: 1, y: 2 };
AssertType(p, "Point");
AssertType({ x: 1, y: 2 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(2, "int");

let p: Point = { x: 0, y: undefined };
AssertType(p, "Point");
AssertType({ x: 0, y: undefined }, "{ x: number; y: undefined; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "undefined");
AssertType(undefined, "undefined");

let p = { x: 1, y: <number>undefined };
AssertType(p, "Point");
AssertType({ x: 1, y: <number>undefined }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(<number>undefined, "number");
AssertType(undefined, "undefined");

let p: { x: number; y: number; } = { x: 1, y: 2 };
AssertType(p, "Point");
AssertType(x, "number");
AssertType(y, "number");
AssertType({ x: 1, y: 2 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(2, "int");

let p = <{ x: number; y: number; }>{ x: 0, y: undefined };
AssertType(p, "Point");
AssertType(<{ x: number; y: number; }>{ x: 0, y: undefined }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType({ x: 0, y: undefined }, "{ x: number; y: undefined; }");
AssertType(x, "number");
AssertType(0, "int");
AssertType(y, "undefined");
AssertType(undefined, "undefined");

let p: typeof p;
AssertType(p, "Point");
AssertType(p, "Point");

let fn = function (s: string) { 
AssertType(fn, "(string) => number");
return 42; 

AssertType(function (s: string) { return 42; }, "(string) => number");

AssertType(s, "string");

AssertType(42, "int");
}

let fn = (s: string) => 3;
AssertType(fn, "(string) => number");
AssertType((s: string) => 3, "(string) => number");
AssertType(s, "string");
AssertType(3, "int");

let fn: (s: string) => number;
AssertType(fn, "(string) => number");
AssertType(s, "string");

let fn: { (s: string): number };
AssertType(fn, "(string) => number");
AssertType(s, "string");

let fn = <(s: string) => number> null;
AssertType(fn, "(string) => number");
AssertType(<(s: string) => number> null, "(string) => number");
AssertType(s, "string");
AssertType(null, "null");

let fn: typeof fn;
AssertType(fn, "(string) => number");
AssertType(fn, "(string) => number");

let a: string[]; 
AssertType(a, "string[]");

let a = ['a', 'b']
AssertType(a, "string[]");
AssertType(['a', 'b'], "string[]");
AssertType('a', "string");
AssertType('b', "string");

let a = <string[]>[];
AssertType(a, "string[]");
AssertType(<string[]>[], "string[]");
AssertType([], "undefined[]");

let a: string[] = [];
AssertType(a, "string[]");
AssertType([], "undefined[]");

let a = new Array<string>();
AssertType(a, "string[]");
AssertType(new Array<string>(), "string[]");
AssertType(Array, "ArrayConstructor");

let a: typeof a;
AssertType(a, "string[]");
AssertType(a, "string[]");


