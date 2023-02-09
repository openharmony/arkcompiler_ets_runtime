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

// === tests/cases/compiler/intersectionTypeNormalization.ts ===
declare function AssertType(value:any, type:string):void;
interface A { a: string }
interface B { b: string }
interface C { c: string }
interface D { d: string }

// Identical ways of writing the same type
type X1 = (A | B) & (C | D);
type X2 = A & (C | D) | B & (C | D)
type X3 = A & C | A & D | B & C | B & D;

let x: X1;
AssertType(x, "X1");

let x: X2;
AssertType(x, "X1");

let x: X3;
AssertType(x, "X1");

interface X { x: string }
interface Y { y: string }

// Identical ways of writing the same type
type Y1 = (A | X & Y) & (C | D);
type Y2 = A & (C | D) | X & Y & (C | D)
type Y3 = A & C | A & D | X & Y & C | X & Y & D;

let y: Y1;
AssertType(y, "Y1");

let y: Y2;
AssertType(y, "Y1");

let y: Y3;
AssertType(y, "Y1");

interface M { m: string }
interface N { n: string }

// Identical ways of writing the same type
type Z1 = (A | X & (M | N)) & (C | D);
type Z2 = A & (C | D) | X & (M | N) & (C | D)
type Z3 = A & C | A & D | X & (M | N) & C | X & (M | N) & D;
type Z4 = A & C | A & D | X & M & C | X & N & C | X & M & D | X & N & D;

let z: Z1;
AssertType(z, "Z1");

let z: Z2;
AssertType(z, "Z1");

let z: Z3;
AssertType(z, "Z1");

let z: Z4;
AssertType(z, "Z1");

// Repro from #9919

type ToString = {
    toString(): string;
}

type BoxedValue = { kind: 'int',    num: number }
                | { kind: 'string', str: string }

type IntersectionFail = BoxedValue & ToString

type IntersectionInline = { kind: 'int',    num: number } & ToString
                        | { kind: 'string', str: string } & ToString

function getValueAsString(value: IntersectionFail): string {
    if (value.kind === 'int') {
AssertType(value.kind === 'int', "boolean");
AssertType(value.kind, "union");
AssertType('int', "string");

AssertType('' + value.num, "string");
AssertType('', "string");
AssertType(value.num, "number");
        return '' + value.num;
    }
AssertType(value.str, "string");
    return value.str;
}

// Repro from #12535

namespace enums {
    export const enum A {
        a1,
        a2,
        a3,
       // ... elements omitted for the sake of clarity
        a75,
        a76,
        a77,
    }
    export const enum B {
        b1,
        b2,
       // ... elements omitted for the sake of clarity
        b86,
        b87,
    }
    export const enum C {
        c1,
        c2,
       // ... elements omitted for the sake of clarity
        c210,
        c211,
    }
    export type Genre = A | B | C;
}

type Foo = {
    genreId: enums.Genre;
};

type Bar = {
    genreId: enums.Genre;
};

type FooBar = Foo & Bar;

function foo(so: any) {
    const val = so as FooBar;
AssertType(val, "FooBar");
AssertType(so as FooBar, "FooBar");
AssertType(so, "any");

    const isGenre = val.genreId;
AssertType(isGenre, "enums.Genre");
AssertType(val.genreId, "enums.Genre");

AssertType(isGenre, "enums.Genre");
    return isGenre;
}

