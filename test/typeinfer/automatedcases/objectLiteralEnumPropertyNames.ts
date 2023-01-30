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

// === tests/cases/compiler/objectLiteralEnumPropertyNames.ts ===
declare function AssertType(value:any, type:string):void;
// Fixes #16887
enum Strs {
    A = 'a',
    B = 'b'
}
type TestStrs = { [key in Strs]: string }
const x: TestStrs = {
AssertType(x, "TestStrs");
AssertType({    [Strs.A]: 'xo',    [Strs.B]: 'xe'}, "{ a: string; b: string; }");

    [Strs.A]: 'xo',
AssertType([Strs.A], "string");
AssertType(Strs.A, "Strs.A");
AssertType('xo', "string");

    [Strs.B]: 'xe'
AssertType([Strs.B], "string");
AssertType(Strs.B, "Strs.B");
AssertType('xe', "string");
}
const ux = {
AssertType(ux, "{ a: string; b: string; }");
AssertType({    [Strs.A]: 'xo',    [Strs.B]: 'xe'}, "{ a: string; b: string; }");

    [Strs.A]: 'xo',
AssertType([Strs.A], "string");
AssertType(Strs.A, "Strs.A");
AssertType('xo', "string");

    [Strs.B]: 'xe'
AssertType([Strs.B], "string");
AssertType(Strs.B, "Strs.B");
AssertType('xe', "string");
}
const y: TestStrs = {
AssertType(y, "TestStrs");
AssertType({    ['a']: 'yo',    ['b']: 'ye'}, "{ a: string; b: string; }");

    ['a']: 'yo',
AssertType(['a'], "string");
AssertType('a', "string");
AssertType('yo', "string");

    ['b']: 'ye'
AssertType(['b'], "string");
AssertType('b', "string");
AssertType('ye', "string");
}
const a = 'a';
AssertType(a, "string");
AssertType('a', "string");

const b = 'b';
AssertType(b, "string");
AssertType('b', "string");

const z: TestStrs = {
AssertType(z, "TestStrs");
AssertType({    [a]: 'zo',    [b]: 'ze'}, "{ a: string; b: string; }");

    [a]: 'zo',
AssertType([a], "string");
AssertType(a, "string");
AssertType('zo', "string");

    [b]: 'ze'
AssertType([b], "string");
AssertType(b, "string");
AssertType('ze', "string");
}
const uz = {
AssertType(uz, "{ a: string; b: string; }");
AssertType({    [a]: 'zo',    [b]: 'ze'}, "{ a: string; b: string; }");

    [a]: 'zo',
AssertType([a], "string");
AssertType(a, "string");
AssertType('zo', "string");

    [b]: 'ze'
AssertType([b], "string");
AssertType(b, "string");
AssertType('ze', "string");
}

enum Nums {
    A,
    B
}
type TestNums = { 0: number, 1: number }
const n: TestNums = {
AssertType(n, "TestNums");
AssertType({    [Nums.A]: 1,    [Nums.B]: 2}, "{ 0: number; 1: number; }");

    [Nums.A]: 1,
AssertType([Nums.A], "number");
AssertType(Nums.A, "Nums.A");
AssertType(1, "int");

    [Nums.B]: 2
AssertType([Nums.B], "number");
AssertType(Nums.B, "Nums.B");
AssertType(2, "int");
}
const un = {
AssertType(un, "{ 0: number; 1: number; }");
AssertType({    [Nums.A]: 3,    [Nums.B]: 4}, "{ 0: number; 1: number; }");

    [Nums.A]: 3,
AssertType([Nums.A], "number");
AssertType(Nums.A, "Nums.A");
AssertType(3, "int");

    [Nums.B]: 4
AssertType([Nums.B], "number");
AssertType(Nums.B, "Nums.B");
AssertType(4, "int");
}
const an = 0;
AssertType(an, "int");
AssertType(0, "int");

const bn = 1;
AssertType(bn, "int");
AssertType(1, "int");

const m: TestNums = {
AssertType(m, "TestNums");
AssertType({    [an]: 5,    [bn]: 6}, "{ 0: number; 1: number; }");

    [an]: 5,
AssertType([an], "number");
AssertType(an, "int");
AssertType(5, "int");

    [bn]: 6
AssertType([bn], "number");
AssertType(bn, "int");
AssertType(6, "int");
}
const um = {
AssertType(um, "{ 0: number; 1: number; }");
AssertType({    [an]: 7,    [bn]: 8}, "{ 0: number; 1: number; }");

    [an]: 7,
AssertType([an], "number");
AssertType(an, "int");
AssertType(7, "int");

    [bn]: 8
AssertType([bn], "number");
AssertType(bn, "int");
AssertType(8, "int");
}


