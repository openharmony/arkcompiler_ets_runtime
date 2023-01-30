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

// === tests/cases/compiler/indexedAccessCanBeHighOrder.ts ===
declare function AssertType(value:any, type:string):void;
declare function get<U, Y extends keyof U>(x: U, y: Y): U[Y];
declare function find<T, K extends keyof T>(o: T[K]): [T, K];

function impl<A, B extends keyof A>(a: A, b: B) {
    const item = get(a, b);
AssertType(item, "A[B]");
AssertType(get(a, b), "A[B]");
AssertType(get, "<U, Y extends keyof U>(U, Y) => U[Y]");
AssertType(a, "A");
AssertType(b, "B");

AssertType(find(item), "[A, B]");
AssertType(find, "<T, K extends keyof T>(T[K]) => [T, K]");
AssertType(item, "A[B]");
    return find(item);
}

const o = {x: 42};
AssertType(o, "{ x: number; }");
AssertType({x: 42}, "{ x: number; }");
AssertType(x, "number");
AssertType(42, "int");

const r = impl(o, "x");
AssertType(r, "[{ x: number; }, "x"]");
AssertType(impl(o, "x"), "[{ x: number; }, "x"]");
AssertType(impl, "<A, B extends keyof A>(A, B) => [A, B]");
AssertType(o, "{ x: number; }");
AssertType("x", "string");

r[0][r[1]] = o[r[1]];
AssertType(r[0][r[1]] = o[r[1]], "number");
AssertType(r[0][r[1]], "number");
AssertType(r[0], "{ x: number; }");
AssertType(r, "[{ x: number; }, "x"]");
AssertType(0, "int");
AssertType(r[1], "string");
AssertType(r, "[{ x: number; }, "x"]");
AssertType(1, "int");
AssertType(o[r[1]], "number");
AssertType(o, "{ x: number; }");
AssertType(r[1], "string");
AssertType(r, "[{ x: number; }, "x"]");
AssertType(1, "int");


