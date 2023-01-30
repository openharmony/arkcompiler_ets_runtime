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

// === tests/cases/conformance/types/typeRelationships/typeInference/discriminatedUnionInference.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #28862

type Foo<A> = { type: "foo", (): A[] };
type Bar<A> = { type: "bar", (): A };

type FooBar<A> = Foo<A> | Bar<A>;

type InferA<T> = T extends FooBar<infer A> ? A : never;

type FooA = InferA<Foo<number>>;  // number

// Repro from #28862

type Item<T> = { kind: 'a', data: T } | { kind: 'b', data: T[] };

declare function foo<T>(item: Item<T>): T;

let x1 = foo({ kind: 'a', data: 42 });  // number
AssertType(x1, "number");
AssertType(foo({ kind: 'a', data: 42 }), "number");
AssertType(foo, "<T>(Item<T>) => T");
AssertType({ kind: 'a', data: 42 }, "{ kind: "a"; data: number; }");
AssertType(kind, "string");
AssertType('a', "string");
AssertType(data, "number");
AssertType(42, "int");

let x2 = foo({ kind: 'b', data: [1, 2] });  // number
AssertType(x2, "number");
AssertType(foo({ kind: 'b', data: [1, 2] }), "number");
AssertType(foo, "<T>(Item<T>) => T");
AssertType({ kind: 'b', data: [1, 2] }, "{ kind: "b"; data: number[]; }");
AssertType(kind, "string");
AssertType('b', "string");
AssertType(data, "number[]");
AssertType([1, 2], "number[]");
AssertType(1, "int");
AssertType(2, "int");


