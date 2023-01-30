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

// === tests/cases/compiler/computedTypesKeyofNoIndexSignatureType.ts ===
declare function AssertType(value:any, type:string):void;

type Compute<A> = { [K in keyof A]: Compute<A[K]>; } & {};

type EqualsTest<T> = <A>() => A extends T ? 1 : 0;
type Equals<A1, A2> = EqualsTest<A2> extends EqualsTest<A1> ? 1 : 0;

type Filter<K, I> = Equals<K, I> extends 1 ? never : K;

type OmitIndex<T, I extends string | number> = {
  [K in keyof T as Filter<K, I>]: T[K];
};

type IndexObject = { [key: string]: unknown; };
type FooBar = { foo: "hello"; bar: "world"; };

type WithIndex = Compute<FooBar & IndexObject>;   // { [x: string]: {}; foo: "hello"; bar: "world"; } <-- OK
type WithoutIndex = OmitIndex<WithIndex, string>; // { foo: "hello"; bar: "world"; }                  <-- OK

type FooBarKey = keyof FooBar;             // "foo" | "bar"   <-- OK
type WithIndexKey = keyof WithIndex;       // string | number <-- Expected: string 
type WithoutIndexKey = keyof WithoutIndex; // number          <-- Expected: "foo" | "bar"

