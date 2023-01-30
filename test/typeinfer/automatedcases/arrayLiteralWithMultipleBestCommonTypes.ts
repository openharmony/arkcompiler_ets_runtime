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

// === tests/cases/conformance/types/typeRelationships/bestCommonType/arrayLiteralWithMultipleBestCommonTypes.ts ===
declare function AssertType(value:any, type:string):void;
// when multiple best common types exist we will choose the first candidate

let a: { x: number; y?: number };
AssertType(a, "{ x: number; y?: number; }");
AssertType(x, "number");
AssertType(y, "number");

let b: { x: number; z?: number };
AssertType(b, "{ x: number; z?: number; }");
AssertType(x, "number");
AssertType(z, "number");

let c: { x: number; a?: number };
AssertType(c, "{ x: number; a?: number; }");
AssertType(x, "number");
AssertType(a, "number");

let as = [a, b]; // { x: number; y?: number };[]
AssertType(as, "(union)[]");
AssertType([a, b], "(union)[]");
AssertType(a, "{ x: number; y?: number; }");
AssertType(b, "{ x: number; z?: number; }");

let bs = [b, a]; // { x: number; z?: number };[]
AssertType(bs, "(union)[]");
AssertType([b, a], "(union)[]");
AssertType(b, "{ x: number; z?: number; }");
AssertType(a, "{ x: number; y?: number; }");

let cs = [a, b, c]; // { x: number; y?: number };[]
AssertType(cs, "(union)[]");
AssertType([a, b, c], "(union)[]");
AssertType(a, "{ x: number; y?: number; }");
AssertType(b, "{ x: number; z?: number; }");
AssertType(c, "{ x: number; a?: number; }");

let ds = [(x: Object) => 1, (x: string) => 2]; // { (x:Object) => number }[]
AssertType(ds, "((Object) => number)[]");
AssertType([(x: Object) => 1, (x: string) => 2], "((Object) => number)[]");
AssertType((x: Object) => 1, "(Object) => number");
AssertType(x, "Object");
AssertType(1, "int");
AssertType((x: string) => 2, "(string) => number");
AssertType(x, "string");
AssertType(2, "int");

let es = [(x: string) => 2, (x: Object) => 1]; // { (x:string) => number }[]
AssertType(es, "((string) => number)[]");
AssertType([(x: string) => 2, (x: Object) => 1], "((string) => number)[]");
AssertType((x: string) => 2, "(string) => number");
AssertType(x, "string");
AssertType(2, "int");
AssertType((x: Object) => 1, "(Object) => number");
AssertType(x, "Object");
AssertType(1, "int");

let fs = [(a: { x: number; y?: number }) => 1, (b: { x: number; z?: number }) => 2]; // (a: { x: number; y?: number }) => number[]
AssertType(fs, "(union)[]");
AssertType([(a: { x: number; y?: number }) => 1, (b: { x: number; z?: number }) => 2], "(union)[]");
AssertType((a: { x: number; y?: number }) => 1, "({    x: number;    y?: number;}) => number");
AssertType(a, "{ x: number; y?: number; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType(1, "int");
AssertType((b: { x: number; z?: number }) => 2, "({    x: number;    z?: number;}) => number");
AssertType(b, "{ x: number; z?: number; }");
AssertType(x, "number");
AssertType(z, "number");
AssertType(2, "int");

let gs = [(b: { x: number; z?: number }) => 2, (a: { x: number; y?: number }) => 1]; // (b: { x: number; z?: number }) => number[]
AssertType(gs, "(union)[]");
AssertType([(b: { x: number; z?: number }) => 2, (a: { x: number; y?: number }) => 1], "(union)[]");
AssertType((b: { x: number; z?: number }) => 2, "({    x: number;    z?: number;}) => number");
AssertType(b, "{ x: number; z?: number; }");
AssertType(x, "number");
AssertType(z, "number");
AssertType(2, "int");
AssertType((a: { x: number; y?: number }) => 1, "({    x: number;    y?: number;}) => number");
AssertType(a, "{ x: number; y?: number; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType(1, "int");


