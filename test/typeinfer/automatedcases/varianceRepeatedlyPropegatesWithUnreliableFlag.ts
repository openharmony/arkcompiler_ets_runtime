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

// === tests/cases/compiler/varianceRepeatedlyPropegatesWithUnreliableFlag.ts ===
declare function AssertType(value:any, type:string):void;
type A = { a: number };
type B = { b: number };
type X<T> = ({ [K in keyof T]: T[K] } & Record<string, void>)[keyof T];
type P1<T> = { data: X<T> };
type P2<T> = { data: X<T> };

interface I<T> {
    fn<K extends keyof T>(p1: P1<Pick<T, K>>, p2: P2<Pick<T, K>>): void;
}

const i: I<A & B> = null as any;
AssertType(i, "I<A & B>");
AssertType(null as any, "any");
AssertType(null, "null");

const p2: P2<A> = null as any;
AssertType(p2, "P2<A>");
AssertType(null as any, "any");
AssertType(null, "null");

// Commenting out the below line will remove the error on the `const _i: I<A> = i;`
i.fn(null as any, p2);
AssertType(i.fn(null as any, p2), "void");
AssertType(i.fn, "<K extends union>(P1<Pick<A & B, K>>, P2<Pick<A & B, K>>) => void");
AssertType(null as any, "any");
AssertType(null, "null");
AssertType(p2, "P2<A>");

const _i: I<A> = i;
AssertType(_i, "I<A>");
AssertType(i, "I<A & B>");


