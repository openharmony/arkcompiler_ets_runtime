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

// === tests/cases/compiler/narrowingIntersection.ts ===
declare function AssertType(value:any, type:string):void;
// Somehow this being an intersection matters.
type FooAndBaz = { foo: unknown } & { baz: unknown };

type Disjoint =
 | { readonly value: string; readonly err?: never; }
 | { readonly value?: never; readonly err: FooAndBaz; };

function test1(result: Disjoint): string {
    if (result.err) {
AssertType(result.err, "FooAndBaz");

        throw result.err;
AssertType(result.err, "FooAndBaz");
    }
    // Error, should OK
AssertType(result.value, "string");
    return result.value;
}

type TrivialIntersection = { a: 1 } & { a: 1 };

function want0(x: 0) {}

function test2(a: 0 | TrivialIntersection) {
    if (a === 0) {
AssertType(a === 0, "boolean");
AssertType(a, "union");
AssertType(0, "int");

        want0(a); // Fails, but expect to work
AssertType(want0(a), "void");
AssertType(want0, "(0) => void");
AssertType(a, "int");
    }
}

