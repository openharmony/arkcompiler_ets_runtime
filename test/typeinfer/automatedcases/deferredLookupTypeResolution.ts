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

// === tests/cases/compiler/deferredLookupTypeResolution.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #17456

type StringContains<S extends string, L extends string> = (
    { [K in S]:      'true' } &
    { [key: string]: 'false' }
  )[L]

type ObjectHasKey<O, L extends string> = StringContains<Extract<keyof O, string>, L>

type First<T> = ObjectHasKey<T, '0'>;  // Should be deferred

type T1 = ObjectHasKey<{ a: string }, 'a'>;  // 'true'
type T2 = ObjectHasKey<{ a: string }, 'b'>;  // 'false'

// Verify that mapped type isn't eagerly resolved in type-to-string operation

declare function f1<A extends string, B extends string>(a: A, b: B): { [P in A | B]: any };

function f2<A extends string>(a: A) {
AssertType(f1(a, 'x'), "{ [P in union]: any; }");
AssertType(f1, "<A extends string, B extends string>(A, B) => { [P in union]: any; }");
AssertType(a, "A");
AssertType('x', "string");
    return f1(a, 'x');
}

function f3(x: 'a' | 'b') {
AssertType(f2(x), "{ a: any; b: any; x: any; }");
AssertType(f2, "<A extends string>(A) => { [P in union]: any; }");
AssertType(x, "union");
    return f2(x);
}


