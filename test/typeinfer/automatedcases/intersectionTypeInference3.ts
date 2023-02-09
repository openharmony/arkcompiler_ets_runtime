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

// === tests/cases/conformance/types/intersection/intersectionTypeInference3.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #19682

type Nominal<Kind extends string, Type> = Type & {
    [Symbol.species]: Kind;
};

type A = Nominal<'A', string>;

declare const a: Set<A>;
AssertType(a, "Set<A>");

declare const b: Set<A>;
AssertType(b, "Set<A>");

const c1 = Array.from(a).concat(Array.from(b));
AssertType(c1, "A[]");
AssertType(Array.from(a).concat(Array.from(b)), "A[]");
AssertType(Array.from(a).concat, "{ (...ConcatArray<A>[]): A[]; (...(union)[]): A[]; }");
AssertType(Array.from(b), "A[]");
AssertType(Array.from, "{ <T>(ArrayLike<T>): T[]; <T, U>(ArrayLike<T>, (T, number) => U, ?any): U[]; <T>(union): T[]; <T, U>(union, (T, number) => U, ?any): U[]; }");
AssertType(b, "Set<A>");

// Simpler repro

declare function from<T>(): T[];
const c2: ReadonlyArray<A> = from();
AssertType(c2, "readonly A[]");
AssertType(from(), "A[]");
AssertType(from, "<T>() => T[]");


