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

// === tests/cases/conformance/types/intersection/intersectionTypeMembers.ts ===
declare function AssertType(value:any, type:string):void;
// An intersection type has those members that are present in any of its constituent types,
// with types that are intersections of the respective members in the constituent types

interface A { a: string }
interface B { b: string }
interface C { c: string }

let abc: A & B & C;
AssertType(abc, "A & B & C");

abc.a = "hello";
AssertType(abc.a = "hello", "string");
AssertType(abc.a, "string");
AssertType("hello", "string");

abc.b = "hello";
AssertType(abc.b = "hello", "string");
AssertType(abc.b, "string");
AssertType("hello", "string");

abc.c = "hello";
AssertType(abc.c = "hello", "string");
AssertType(abc.c, "string");
AssertType("hello", "string");

interface X { x: A }
interface Y { x: B }
interface Z { x: C }

let xyz: X & Y & Z;
AssertType(xyz, "X & Y & Z");

xyz.x.a = "hello";
AssertType(xyz.x.a = "hello", "string");
AssertType(xyz.x.a, "string");
AssertType("hello", "string");

xyz.x.b = "hello";
AssertType(xyz.x.b = "hello", "string");
AssertType(xyz.x.b, "string");
AssertType("hello", "string");

xyz.x.c = "hello";
AssertType(xyz.x.c = "hello", "string");
AssertType(xyz.x.c, "string");
AssertType("hello", "string");

type F1 = (x: string) => string;
type F2 = (x: number) => number;

let f: F1 & F2;
AssertType(f, "F1 & F2");

let s = f("hello");
AssertType(s, "string");
AssertType(f("hello"), "string");
AssertType(f, "F1 & F2");
AssertType("hello", "string");

let n = f(42);
AssertType(n, "number");
AssertType(f(42), "number");
AssertType(f, "F1 & F2");
AssertType(42, "int");

interface D {
    nested: { doublyNested: { d: string; }, different: { e: number } };
}
interface E {
    nested: { doublyNested: { f: string; }, other: {g: number } };
}
const de: D & E = {
AssertType(de, "D & E");
AssertType({    nested: {        doublyNested: {            d: 'yes',            f: 'no'        },        different: { e: 12 },        other: { g: 101 }    }}, "{ nested: { doublyNested: { d: string; f: string; }; different: { e: number; }; other: { g: number; }; }; }");

    nested: {
AssertType(nested, "{ doublyNested: { d: string; f: string; }; different: { e: number; }; other: { g: number; }; }");
AssertType({        doublyNested: {            d: 'yes',            f: 'no'        },        different: { e: 12 },        other: { g: 101 }    }, "{ doublyNested: { d: string; f: string; }; different: { e: number; }; other: { g: number; }; }");

        doublyNested: {
AssertType(doublyNested, "{ d: string; f: string; }");
AssertType({            d: 'yes',            f: 'no'        }, "{ d: string; f: string; }");

            d: 'yes',
AssertType(d, "string");
AssertType('yes', "string");

            f: 'no'
AssertType(f, "string");
AssertType('no', "string");

        },
        different: { e: 12 },
AssertType(different, "{ e: number; }");
AssertType({ e: 12 }, "{ e: number; }");
AssertType(e, "number");
AssertType(12, "int");

        other: { g: 101 
AssertType(other, "{ g: number; }");

AssertType({ g: 101 }, "{ g: number; }");

AssertType(g, "number");

AssertType(101, "int");
}
    }
}

// Additional test case with >2 doubly nested members so fix for #31441 is tested w/ excess props
interface F {
    nested: { doublyNested: { g: string; } }
}

interface G {
    nested: { doublyNested: { h: string; } }
}

const defg: D & E & F & G = {
AssertType(defg, "D & E & F & G");
AssertType({    nested: {        doublyNested: {            d: 'yes',            f: 'no',            g: 'ok',            h: 'affirmative'        },        different: { e: 12 },        other: { g: 101 }    }}, "{ nested: { doublyNested: { d: string; f: string; g: string; h: string; }; different: { e: number; }; other: { g: number; }; }; }");

    nested: {
AssertType(nested, "{ doublyNested: { d: string; f: string; g: string; h: string; }; different: { e: number; }; other: { g: number; }; }");
AssertType({        doublyNested: {            d: 'yes',            f: 'no',            g: 'ok',            h: 'affirmative'        },        different: { e: 12 },        other: { g: 101 }    }, "{ doublyNested: { d: string; f: string; g: string; h: string; }; different: { e: number; }; other: { g: number; }; }");

        doublyNested: {
AssertType(doublyNested, "{ d: string; f: string; g: string; h: string; }");
AssertType({            d: 'yes',            f: 'no',            g: 'ok',            h: 'affirmative'        }, "{ d: string; f: string; g: string; h: string; }");

            d: 'yes',
AssertType(d, "string");
AssertType('yes', "string");

            f: 'no',
AssertType(f, "string");
AssertType('no', "string");

            g: 'ok',
AssertType(g, "string");
AssertType('ok', "string");

            h: 'affirmative'
AssertType(h, "string");
AssertType('affirmative', "string");

        },
        different: { e: 12 },
AssertType(different, "{ e: number; }");
AssertType({ e: 12 }, "{ e: number; }");
AssertType(e, "number");
AssertType(12, "int");

        other: { g: 101 
AssertType(other, "{ g: number; }");

AssertType({ g: 101 }, "{ g: number; }");

AssertType(g, "number");

AssertType(101, "int");
}
    }
}


