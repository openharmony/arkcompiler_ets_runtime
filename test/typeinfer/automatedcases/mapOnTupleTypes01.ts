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

// === tests/cases/compiler/mapOnTupleTypes01.ts ===
declare function AssertType(value:any, type:string):void;
export let mapOnLooseArrayLiteral = [1, 2, 3, 4].map(n => n * n);
AssertType(mapOnLooseArrayLiteral, "number[]");
AssertType([1, 2, 3, 4].map(n => n * n), "number[]");
AssertType([1, 2, 3, 4].map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(n => n * n, "(number) => number");
AssertType(n, "number");
AssertType(n * n, "number");
AssertType(n, "number");
AssertType(n, "number");

// Length 1

let numTuple: [number] = [1];
AssertType(numTuple, "[number]");
AssertType([1], "[number]");
AssertType(1, "int");

export let a = numTuple.map(x => x * x);
AssertType(a, "number[]");
AssertType(numTuple.map(x => x * x), "number[]");
AssertType(numTuple.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(x => x * x, "(number) => number");
AssertType(x, "number");
AssertType(x * x, "number");
AssertType(x, "number");
AssertType(x, "number");

// Length 2

let numNum: [number, number] = [    100,     100];
AssertType(numNum, "[number, number]");
AssertType([    100,     100], "[number, number]");
AssertType(100, "int");
AssertType(100, "int");

let strStr: [string, string] = ["hello", "hello"];
AssertType(strStr, "[string, string]");
AssertType(["hello", "hello"], "[string, string]");
AssertType("hello", "string");
AssertType("hello", "string");

let numStr: [number, string] = [    100, "hello"];
AssertType(numStr, "[number, string]");
AssertType([    100, "hello"], "[number, string]");
AssertType(100, "int");
AssertType("hello", "string");

export let b = numNum.map(n => n * n);
AssertType(b, "number[]");
AssertType(numNum.map(n => n * n), "number[]");
AssertType(numNum.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(n => n * n, "(number) => number");
AssertType(n, "number");
AssertType(n * n, "number");
AssertType(n, "number");
AssertType(n, "number");

export let c = strStr.map(s => s.charCodeAt(0));
AssertType(c, "number[]");
AssertType(strStr.map(s => s.charCodeAt(0)), "number[]");
AssertType(strStr.map, "<U>((string, number, string[]) => U, ?any) => U[]");
AssertType(s => s.charCodeAt(0), "(string) => number");
AssertType(s, "string");
AssertType(s.charCodeAt(0), "number");
AssertType(s.charCodeAt, "(number) => number");
AssertType(0, "int");

export let d = numStr.map(x => x);
AssertType(d, "(union)[]");
AssertType(numStr.map(x => x), "(union)[]");
AssertType(numStr.map, "<U>((union, number, (union)[]) => U, ?any) => U[]");
AssertType(x => x, "(union) => union");
AssertType(x, "union");
AssertType(x, "union");

// Length 3

let numNumNum: [number, number, number] = [1, 2, 3];
AssertType(numNumNum, "[number, number, number]");
AssertType([1, 2, 3], "[number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

export let e = numNumNum.map(n => n * n);
AssertType(e, "number[]");
AssertType(numNumNum.map(n => n * n), "number[]");
AssertType(numNumNum.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(n => n * n, "(number) => number");
AssertType(n, "number");
AssertType(n * n, "number");
AssertType(n, "number");
AssertType(n, "number");

// Length 4

let numNumNumNum: [number, number, number, number] = [1, 2, 3, 4];
AssertType(numNumNumNum, "[number, number, number, number]");
AssertType([1, 2, 3, 4], "[number, number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");

export let f = numNumNumNum.map(n => n * n);
AssertType(f, "number[]");
AssertType(numNumNumNum.map(n => n * n), "number[]");
AssertType(numNumNumNum.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(n => n * n, "(number) => number");
AssertType(n, "number");
AssertType(n * n, "number");
AssertType(n, "number");
AssertType(n, "number");

// Length 5

let numNumNumNumNum: [number, number, number, number, number] = [1, 2, 3, 4, 5];
AssertType(numNumNumNumNum, "[number, number, number, number, number]");
AssertType([1, 2, 3, 4, 5], "[number, number, number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");
AssertType(5, "int");

export let g = numNumNumNumNum.map(n => n * n);
AssertType(g, "number[]");
AssertType(numNumNumNumNum.map(n => n * n), "number[]");
AssertType(numNumNumNumNum.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(n => n * n, "(number) => number");
AssertType(n, "number");
AssertType(n * n, "number");
AssertType(n, "number");
AssertType(n, "number");


// Length 6

let numNumNumNumNumNum: [number, number, number, number, number, number] = [1, 2, 3, 4, 5, 6];
AssertType(numNumNumNumNumNum, "[number, number, number, number, number, number]");
AssertType([1, 2, 3, 4, 5, 6], "[number, number, number, number, number, number]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");
AssertType(4, "int");
AssertType(5, "int");
AssertType(6, "int");

export let h = numNumNumNumNum.map(n => n * n);
AssertType(h, "number[]");
AssertType(numNumNumNumNum.map(n => n * n), "number[]");
AssertType(numNumNumNumNum.map, "<U>((number, number, number[]) => U, ?any) => U[]");
AssertType(n => n * n, "(number) => number");
AssertType(n, "number");
AssertType(n * n, "number");
AssertType(n, "number");
AssertType(n, "number");


