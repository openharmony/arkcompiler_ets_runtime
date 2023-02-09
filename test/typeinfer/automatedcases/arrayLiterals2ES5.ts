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

// === tests/cases/conformance/expressions/arrayLiterals/arrayLiterals2ES5.ts ===
declare function AssertType(value:any, type:string):void;
// ElementList:  ( Modified )
//      Elisionopt   AssignmentExpression
//      Elisionopt   SpreadElement
//      ElementList, Elisionopt   AssignmentExpression
//      ElementList, Elisionopt   SpreadElement

// SpreadElement:
//      ...   AssignmentExpression

let a0 = [,, 2, 3, 4]
AssertType(a0, "number[]");
AssertType([,, 2, 3, 4], "number[]");
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

let a1 = ["hello", "world"]
AssertType(a1, "string[]");
AssertType(["hello", "world"], "string[]");
AssertType("hello", "string");
AssertType("world", "string");

let a2 = [, , , ...a0, "hello"];
AssertType(a2, "(union)[]");
AssertType([, , , ...a0, "hello"], "(union)[]");
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(...a0, "number");
AssertType(a0, "number[]");
AssertType("hello", "string");

let a3 = [,, ...a0]
AssertType(a3, "number[]");
AssertType([,, ...a0], "number[]");
AssertType(, "undefined");
AssertType(, "undefined");
AssertType(...a0, "number");
AssertType(a0, "number[]");

let a4 = [() => 1, ];
AssertType(a4, "(() => number)[]");
AssertType([() => 1, ], "(() => number)[]");
AssertType(() => 1, "() => number");
AssertType(1, "int");

let a5 = [...a0, , ]
AssertType(a5, "number[]");
AssertType([...a0, , ], "number[]");
AssertType(...a0, "number");
AssertType(a0, "number[]");
AssertType(, "undefined");

// Each element expression in a non-empty array literal is processed as follows:
//    - If the array literal contains no spread elements, and if the array literal is contextually typed (section 4.19)
//      by a type T and T has a property with the numeric name N, where N is the index of the element expression in the array literal,
//      the element expression is contextually typed by the type of that property.

// The resulting type an array literal expression is determined as follows:
//     - If the array literal contains no spread elements and is contextually typed by a tuple-like type,
//       the resulting type is a tuple type constructed from the types of the element expressions.

let b0: [any, any, any] = [undefined, null, undefined];
AssertType(b0, "[any, any, any]");
AssertType([undefined, null, undefined], "[undefined, null, undefined]");
AssertType(undefined, "undefined");
AssertType(null, "null");
AssertType(undefined, "undefined");

let b1: [number[], string[]] = [[1, 2, 3], ["hello", "string"]];
AssertType(b1, "[number[], string[]]");
AssertType([[1, 2, 3], ["hello", "string"]], "[number[], string[]]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(["hello", "string"], "string[]");
AssertType("hello", "string");
AssertType("string", "string");

// The resulting type an array literal expression is determined as follows:
//     - If the array literal contains no spread elements and is an array assignment pattern in a destructuring assignment (section 4.17.1),
//       the resulting type is a tuple type constructed from the types of the element expressions.

let [c0, c1] = [1, 2];        // tuple type [number, number]
AssertType(c0, "number");
AssertType(c1, "number");
AssertType([1, 2], "[number, number]");
AssertType(1, "int");
AssertType(2, "int");

let [c2, c3] = [1, 2, true];  // tuple type [number, number, boolean]
AssertType(c2, "number");
AssertType(c3, "number");
AssertType([1, 2, true], "[number, number, boolean]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(true, "boolean");

// The resulting type an array literal expression is determined as follows:
//      - the resulting type is an array type with an element type that is the union of the types of the
//        non - spread element expressions and the numeric index signature types of the spread element expressions
let temp = ["s", "t", "r"];
AssertType(temp, "string[]");
AssertType(["s", "t", "r"], "string[]");
AssertType("s", "string");
AssertType("t", "string");
AssertType("r", "string");

let temp1 = [1, 2, 3];
AssertType(temp1, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

let temp2: [number[], string[]] = [[1, 2, 3], ["hello", "string"]];
AssertType(temp2, "[number[], string[]]");
AssertType([[1, 2, 3], ["hello", "string"]], "[number[], string[]]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(["hello", "string"], "string[]");
AssertType("hello", "string");
AssertType("string", "string");

let temp3 = [undefined, null, undefined];
AssertType(temp3, "any[]");
AssertType([undefined, null, undefined], "null[]");
AssertType(undefined, "undefined");
AssertType(null, "null");
AssertType(undefined, "undefined");

let temp4 = [];
AssertType(temp4, "any[]");
AssertType([], "undefined[]");

interface myArray extends Array<Number> { }
interface myArray2 extends Array<Number|String> { }
let d0 = [1, true, ...temp,];  // has type (string|number|boolean)[]
AssertType(d0, "(union)[]");
AssertType([1, true, ...temp,], "(union)[]");
AssertType(1, "int");
AssertType(true, "boolean");
AssertType(...temp, "string");
AssertType(temp, "string[]");

let d1 = [...temp];            // has type string[]
AssertType(d1, "string[]");
AssertType([...temp], "string[]");
AssertType(...temp, "string");
AssertType(temp, "string[]");

let d2: number[] = [...temp1];
AssertType(d2, "number[]");
AssertType([...temp1], "number[]");
AssertType(...temp1, "number");
AssertType(temp1, "number[]");

let d3: myArray = [...temp1];
AssertType(d3, "myArray");
AssertType([...temp1], "number[]");
AssertType(...temp1, "number");
AssertType(temp1, "number[]");

let d4: myArray2 = [...temp, ...temp1];
AssertType(d4, "myArray2");
AssertType([...temp, ...temp1], "(union)[]");
AssertType(...temp, "string");
AssertType(temp, "string[]");
AssertType(...temp1, "number");
AssertType(temp1, "number[]");

let d5 = [...temp3];
AssertType(d5, "any[]");
AssertType([...temp3], "any[]");
AssertType(...temp3, "any");
AssertType(temp3, "any[]");

let d6 = [...temp4];
AssertType(d6, "any[]");
AssertType([...temp4], "any[]");
AssertType(...temp4, "any");
AssertType(temp4, "any[]");

let d7 = [...[...temp1]];
AssertType(d7, "number[]");
AssertType([...[...temp1]], "number[]");
AssertType(...[...temp1], "number");
AssertType([...temp1], "number[]");
AssertType(...temp1, "number");
AssertType(temp1, "number[]");

let d8: number[][] = [[...temp1]]
AssertType(d8, "number[][]");
AssertType([[...temp1]], "number[][]");
AssertType([...temp1], "number[]");
AssertType(...temp1, "number");
AssertType(temp1, "number[]");

let d9 = [[...temp1], ...["hello"]];
AssertType(d9, "(union)[]");
AssertType([[...temp1], ...["hello"]], "(union)[]");
AssertType([...temp1], "number[]");
AssertType(...temp1, "number");
AssertType(temp1, "number[]");
AssertType(...["hello"], "string");
AssertType(["hello"], "string[]");
AssertType("hello", "string");


