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

// === tests/cases/conformance/types/tuple/named/namedTupleMembers.ts ===
declare function AssertType(value:any, type:string):void;
export type Segment = [length: number, count: number];

export type SegmentAnnotated = [
    /** 
     * Size of message buffer segment handles
     */
    length: number,
    /**
     * Number of segments handled at once
     */
    count: number
];

declare let a: Segment;
AssertType(a, "Segment");

declare let b: SegmentAnnotated;
AssertType(b, "SegmentAnnotated");

declare let c: [number, number];
AssertType(c, "[number, number]");

declare let d: [a: number, b: number];
AssertType(d, "[a: number, b: number]");

a = b;
AssertType(a = b, "SegmentAnnotated");
AssertType(a, "Segment");
AssertType(b, "SegmentAnnotated");

a = c;
AssertType(a = c, "[number, number]");
AssertType(a, "Segment");
AssertType(c, "[number, number]");

a = d;
AssertType(a = d, "[a: number, b: number]");
AssertType(a, "Segment");
AssertType(d, "[a: number, b: number]");

b = a;
AssertType(b = a, "Segment");
AssertType(b, "SegmentAnnotated");
AssertType(a, "Segment");

b = c;
AssertType(b = c, "[number, number]");
AssertType(b, "SegmentAnnotated");
AssertType(c, "[number, number]");

b = d;
AssertType(b = d, "[a: number, b: number]");
AssertType(b, "SegmentAnnotated");
AssertType(d, "[a: number, b: number]");

c = a;
AssertType(c = a, "Segment");
AssertType(c, "[number, number]");
AssertType(a, "Segment");

c = b;
AssertType(c = b, "SegmentAnnotated");
AssertType(c, "[number, number]");
AssertType(b, "SegmentAnnotated");

c = d;
AssertType(c = d, "[a: number, b: number]");
AssertType(c, "[number, number]");
AssertType(d, "[a: number, b: number]");

d = a;
AssertType(d = a, "Segment");
AssertType(d, "[a: number, b: number]");
AssertType(a, "Segment");

d = b;
AssertType(d = b, "SegmentAnnotated");
AssertType(d, "[a: number, b: number]");
AssertType(b, "SegmentAnnotated");

d = c;
AssertType(d = c, "[number, number]");
AssertType(d, "[a: number, b: number]");
AssertType(c, "[number, number]");

export type WithOptAndRest = [first: number, second?: number, ...rest: string[]];

export type Func<T extends any[]> = (...x: T) => void;

export const func = null as any as Func<SegmentAnnotated>;
AssertType(func, "Func<SegmentAnnotated>");
AssertType(null as any as Func<SegmentAnnotated>, "Func<SegmentAnnotated>");
AssertType(null as any, "any");
AssertType(null, "null");

export function useState<T>(initial: T): [value: T, setter: (T) => void] {
AssertType(null as any, "any");
AssertType(null, "null");
    return null as any;
}


export type Iter = Func<[step: number, iterations: number]>;

export function readSegment([length, count]: [number, number]) {}

// documenting binding pattern behavior (currently does _not_ generate tuple names)
export const val = null as any as Parameters<typeof readSegment>[0];
AssertType(val, "[number, number]");
AssertType(null as any as Parameters<typeof readSegment>[0], "[number, number]");
AssertType(null as any, "any");
AssertType(null, "null");
AssertType(readSegment, "([number, number]) => void");

export type RecursiveTupleA = [initial: string, next: RecursiveTupleA];

export type RecursiveTupleB = [first: string, ptr: RecursiveTupleB];

declare let q: RecursiveTupleA;
AssertType(q, "RecursiveTupleA");

declare let r: RecursiveTupleB;
AssertType(r, "RecursiveTupleB");

q = r;
AssertType(q = r, "RecursiveTupleB");
AssertType(q, "RecursiveTupleA");
AssertType(r, "RecursiveTupleB");

r = q;
AssertType(r = q, "RecursiveTupleA");
AssertType(r, "RecursiveTupleB");
AssertType(q, "RecursiveTupleA");

export type RecusiveRest = [first: string, ...rest: RecusiveRest[]];
export type RecusiveRest2 = [string, ...RecusiveRest2[]];

declare let x: RecusiveRest;
AssertType(x, "RecusiveRest");

declare let y: RecusiveRest2;
AssertType(y, "RecusiveRest2");

x = y;
AssertType(x = y, "RecusiveRest2");
AssertType(x, "RecusiveRest");
AssertType(y, "RecusiveRest2");

y = x;
AssertType(y = x, "RecusiveRest");
AssertType(y, "RecusiveRest2");
AssertType(x, "RecusiveRest");

declare function f<T extends any[]>(...x: T): T;
declare function g(elem: object, index: number): object;
declare function getArgsForInjection<T extends (...args: any[]) => any>(x: T): Parameters<T>;

export const argumentsOfGAsFirstArgument = f(getArgsForInjection(g)); // one tuple with captures arguments as first member
AssertType(argumentsOfGAsFirstArgument, "[[elem: object, index: number]]");
AssertType(f(getArgsForInjection(g)), "[[elem: object, index: number]]");
AssertType(f, "<T extends any[]>(...T) => T");
AssertType(getArgsForInjection(g), "[elem: object, index: number]");
AssertType(getArgsForInjection, "<T extends (...any[]) => any>(T) => Parameters<T>");
AssertType(g, "(object, number) => object");

export const argumentsOfG = f(...getArgsForInjection(g)); // captured arguments list re-spread
AssertType(argumentsOfG, "[elem: object, index: number]");
AssertType(f(...getArgsForInjection(g)), "[elem: object, index: number]");
AssertType(f, "<T extends any[]>(...T) => T");
AssertType(...getArgsForInjection(g), "union");
AssertType(getArgsForInjection(g), "[elem: object, index: number]");
AssertType(getArgsForInjection, "<T extends (...any[]) => any>(T) => Parameters<T>");
AssertType(g, "(object, number) => object");


