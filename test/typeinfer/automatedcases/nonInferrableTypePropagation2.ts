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

// === tests/cases/compiler/nonInferrableTypePropagation2.ts ===
declare function AssertType(value:any, type:string):void;
export interface Predicate<A> {
    (a: A): boolean
}

interface Left<E> {
    readonly _tag: 'Left'
    readonly left: E
}
  
interface Right<A> {
    readonly _tag: 'Right'
    readonly right: A
}

type Either<E, A> = Left<E> | Right<A>;

interface Refinement<A, B extends A> {
    (a: A): a is B
}

declare const filter: {
AssertType(filter, "{ <A, B extends A>(Refinement<A, B>): (readonly A[]) => readonly B[]; <A>(Predicate<A>): <B extends A>(readonly B[]) => readonly B[]; <A>(Predicate<A>): (readonly A[]) => readonly A[]; }");

    <A, B extends A>(refinement: Refinement<A, B>): (as: ReadonlyArray<A>) => ReadonlyArray<B>
AssertType(refinement, "Refinement<A, B>");
AssertType(as, "readonly A[]");

    <A>(predicate: Predicate<A>): <B extends A>(bs: ReadonlyArray<B>) => ReadonlyArray<B>
AssertType(predicate, "Predicate<A>");
AssertType(bs, "readonly B[]");

    <A>(predicate: Predicate<A>): (as: ReadonlyArray<A>) => ReadonlyArray<A>
AssertType(predicate, "Predicate<A>");
AssertType(as, "readonly A[]");

};

declare function pipe<A, B>(a: A, ab: (a: A) => B): B;
declare function exists<A>(predicate: Predicate<A>): <E>(ma: Either<E, A>) => boolean;

declare const es: Either<string, number>[];
AssertType(es, "Either<string, number>[]");

const x = pipe(es, filter(exists((n) => n > 0)))
AssertType(x, "readonly Either<string, number>[]");
AssertType(pipe(es, filter(exists((n) => n > 0))), "readonly Either<string, number>[]");
AssertType(pipe, "<A, B>(A, (A) => B) => B");
AssertType(es, "Either<string, number>[]");
AssertType(filter(exists((n) => n > 0)), "(readonly Either<string, number>[]) => readonly Either<string, number>[]");
AssertType(filter, "{ <A, B extends A>(Refinement<A, B>): (readonly A[]) => readonly B[]; <A>(Predicate<A>): <B extends A>(readonly B[]) => readonly B[]; <A>(Predicate<A>): (readonly A[]) => readonly A[]; }");
AssertType(exists((n) => n > 0), "<E>(Either<E, number>) => boolean");
AssertType(exists, "<A>(Predicate<A>) => <E>(Either<E, A>) => boolean");
AssertType((n) => n > 0, "(number) => boolean");
AssertType(n, "number");
AssertType(n > 0, "boolean");
AssertType(n, "number");
AssertType(0, "int");


