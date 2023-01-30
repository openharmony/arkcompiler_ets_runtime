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

// === tests/cases/conformance/types/keyof/keyofIntersection.ts ===
declare function AssertType(value:any, type:string):void;

type A = { a: string };
type B = { b: string };

type T01 = keyof (A & B);  // "a" | "b"
type T02<T> = keyof (T & B);  // "b" | keyof T
type T03<U> = keyof (A & U);  // "a" | keyof U
type T04<T, U> = keyof (T & U);  // keyof T | keyof U
type T05 = T02<A>;  // "a" | "b"
type T06 = T03<B>;  // "a" | "b"
type T07 = T04<A, B>;  // "a" | "b"

// Repros from #22291

type Example1<T extends string, U extends string> = keyof (Record<T, any> & Record<U, any>);
type Result1 = Example1<'x', 'y'>;  // "x" | "y"

type Result2 = keyof (Record<'x', any> & Record<'y', any>);  // "x" | "y"

type Example3<T extends string> = keyof (Record<T, any>);
type Result3 = Example3<'x' | 'y'>;  // "x" | "y"

type Example4<T extends string, U extends string> = (Record<T, any> & Record<U, any>);
type Result4 = keyof Example4<'x', 'y'>;  // "x" | "y"

type Example5<T, U> = keyof (T & U);
type Result5 = Example5<Record<'x', any>, Record<'y', any>>;  // "x" | "y"


